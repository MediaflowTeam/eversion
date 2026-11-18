/**
 * @file main.cc
 * @brief Demonstrates declaring and consuming EVersion metadata.
 * @author EVersion contributors
 * @date 2026-11-15
 */

#include <dlfcn.h>
#include <eversion/component_info.h>
#include <eversion/runtime_version.h>
#include <eversion/version_info.h>
#include <version_report/version_info.h>

#include <cstddef>
#include <iostream>
#include <span>
#include <string>
#include <string_view>

namespace {

using LibraryHandle = void*;

class Library {
 public:
  explicit Library(const char* path) : handle_(dlopen(path, RTLD_NOW)) {}

  ~Library() {
    if (handle_ != nullptr) {
      dlclose(handle_);
    }
  }

  Library(const Library&) = delete;
  Library& operator=(const Library&) = delete;
  Library(Library&&) = delete;
  Library& operator=(Library&&) = delete;

  [[nodiscard]] LibraryHandle Get() const { return handle_; }

 private:
  LibraryHandle handle_ = nullptr;
};

void PrintHostVersions() {
  std::cout << "version_report " << version_report::version_info::kVersionString
            << '\n';
  std::cout << "eversion " << eversion::version_info::kVersionString << '\n';
}

bool PrintPluginInfo(const char* path) {
  Library library{path};
  if (library.Get() == nullptr) {
    const char* error = dlerror();
    std::cerr << "error: failed to load plugin '" << path << "'";
    if (error != nullptr) {
      std::cerr << ": " << error;
    }
    std::cerr << '\n';
    return false;
  }

  // dlsym needs a null-terminated name; a string_view does not guarantee one.
  const std::string symbol_name{eversion::kPluginInfoSymbol};
  void* symbol = dlsym(library.Get(), symbol_name.c_str());
  if (symbol == nullptr) {
    std::cerr << "error: failed to find symbol '" << symbol_name << "'\n";
    return false;
  }

  const auto plugin_info =
      reinterpret_cast<eversion::ComponentInfoFunction>(symbol);
  const eversion::ComponentInfo* info = plugin_info();
  if (!eversion::IsValidComponentInfo(info)) {
    std::cerr << "error: plugin metadata is invalid\n";
    return false;
  }
  if (!eversion::IsWellFormedComponentVersion(info->version)) {
    std::cerr << "error: plugin version metadata is malformed\n";
    return false;
  }

  const eversion::RuntimeVersion plugin_version =
      eversion::ToRuntimeVersion(info->version);
  PrintHostVersions();
  // info->id and info->name point into the plugin, so print them before the
  // library is closed.
  std::cout << "plugin " << info->id << " (" << info->name << ") "
            << plugin_version.ToString() << '\n';
  return true;
}

}  // namespace

int main(int argc, char* argv[]) {
  const std::span<char* const> arguments{argv, static_cast<std::size_t>(argc)};
  if (arguments.size() > 2U) {
    std::cerr << "error: expected at most one plugin path\n";
    return 1;
  }

  if (arguments.size() < 2U) {
    PrintHostVersions();
    return 0;
  }

  if (!PrintPluginInfo(arguments[1])) {
    return 1;
  }
  return 0;
}
