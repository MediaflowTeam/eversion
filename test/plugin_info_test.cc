/**
 * @file plugin_info_test.cc
 * @brief Tests dynamic loading of generated plugin metadata.
 * @author EVersion contributors
 * @date 2026-06-05
 */

#include <dlfcn.h>
#include <eversion/component_info.h>
#include <eversion/runtime_version.h>
#include <gtest/gtest.h>

#include <string>
#include <string_view>

#ifndef EVERSION_TEST_PLUGIN_PATH
#error "EVERSION_TEST_PLUGIN_PATH must be defined by the build system"
#endif

namespace {

using LibraryHandle = void*;

LibraryHandle OpenLibrary(const char* path) { return dlopen(path, RTLD_NOW); }

void CloseLibrary(LibraryHandle handle) {
  if (handle != nullptr) {
    dlclose(handle);
  }
}

void* LoadSymbol(LibraryHandle handle, const char* name) {
  return dlsym(handle, name);
}

TEST(PluginInfo, LoadsAndValidatesGeneratedMetadata) {
  LibraryHandle library = OpenLibrary(EVERSION_TEST_PLUGIN_PATH);
  ASSERT_NE(library, nullptr)
      << "failed to load plugin: " << EVERSION_TEST_PLUGIN_PATH;

  // dlsym needs a null-terminated name; a string_view does not guarantee one.
  const std::string symbol_name{eversion::kPluginInfoSymbol};
  void* symbol = LoadSymbol(library, symbol_name.c_str());
  ASSERT_NE(symbol, nullptr)
      << "failed to find symbol: " << eversion::kPluginInfoSymbol;

  const auto plugin_info =
      reinterpret_cast<eversion::ComponentInfoFunction>(symbol);
  const eversion::ComponentInfo* info = plugin_info();

  ASSERT_TRUE(eversion::IsValidComponentInfo(info));
  EXPECT_EQ(std::string_view{info->id}, "test.plugin");
  EXPECT_EQ(std::string_view{info->name}, "EVersion Test Plugin");
  const eversion::RuntimeVersion plugin_version =
      eversion::ToRuntimeVersion(info->version);
  EXPECT_EQ(plugin_version,
            (eversion::Version{9, 8, 7, "beta.2", "other.build"}));
  EXPECT_EQ(plugin_version.ToString(), "9.8.7-beta.2+build.42");

  CloseLibrary(library);
}

TEST(PluginInfo, RuntimeVersionOutlivesThePlugin) {
  LibraryHandle library = OpenLibrary(EVERSION_TEST_PLUGIN_PATH);
  ASSERT_NE(library, nullptr)
      << "failed to load plugin: " << EVERSION_TEST_PLUGIN_PATH;

  // dlsym needs a null-terminated name; a string_view does not guarantee one.
  const std::string symbol_name{eversion::kPluginInfoSymbol};
  void* symbol = LoadSymbol(library, symbol_name.c_str());
  ASSERT_NE(symbol, nullptr)
      << "failed to find symbol: " << eversion::kPluginInfoSymbol;

  const auto plugin_info =
      reinterpret_cast<eversion::ComponentInfoFunction>(symbol);
  const eversion::ComponentInfo* info = plugin_info();

  ASSERT_TRUE(eversion::IsValidComponentInfo(info));
  const eversion::RuntimeVersion kept =
      eversion::ToRuntimeVersion(info->version);
  CloseLibrary(library);

  EXPECT_EQ(kept.ToString(), "9.8.7-beta.2+build.42");
  EXPECT_EQ(kept.prerelease, "beta.2");
}

}  // namespace
