# Changelog

All notable changes to this project are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to
[Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

EVersion moved from the `Raricor/EVersion` repository to this repository, which
was created from cpp-library-template v0.2.0 (see `TEMPLATE_VERSION`). EVersion
had no release before the move. The entries below compare this repository with
the last state of the earlier one.

### Changed

- `eversion_declare()` and `eversion_declare_plugin()` stop the configure step
  with an error in two more cases. Before, these configured without an error and
  failed later with a misleading compiler error:
  - two targets whose `OUTPUT` values give the same include guard, such as
    `ffmpeg-wrap/version_info.h` and `ffmpeg_wrap/version_info.h`;
  - outside the `eversion` project, a `NAMESPACE` that is `eversion` or starts
    with `eversion::`, an `OUTPUT` that starts with `eversion/`, or an `OUTPUT`
    whose include guard starts with `EVERSION_`.

  An error about a repeated `NAMESPACE`, `OUTPUT`, or include guard now names the
  earlier target, its `OUTPUT`, and the `CMakeLists.txt` it was declared in.
- The CMake package is named `eversion` instead of `EVersion`. Consumers use
  `find_package(eversion CONFIG REQUIRED)`, and the package files are installed
  to `<libdir>/cmake/eversion/`. The `eversion::eversion` target, the
  `eversion_declare()` and `eversion_declare_plugin()` functions, their
  arguments and target properties, and the generated code are unchanged.
- The build options follow the package name: `EVERSION_BUILD_TESTS`,
  `EVERSION_INSTALL`, `EVERSION_ENABLE_SANITIZERS`, and
  `EVERSION_WARNINGS_AS_ERRORS` are now `eversion_BUILD_TESTS`,
  `eversion_INSTALL`, `eversion_ENABLE_SANITIZERS`, and
  `eversion_WARNINGS_AS_ERRORS`.
- The package version file uses `SameMinorVersion` instead of
  `SameMajorVersion`, so `find_package(eversion 0.1)` no longer accepts 0.2 or
  later. It is also `ARCH_INDEPENDENT`.
- CMake 4.2 or newer is required, instead of 3.21.
- EVersion supports Linux only, with GCC or Clang. Configuring with MSVC or
  clang-cl stops with an error, and `eversion_declare_plugin()` always exports
  the plugin entry point with `__attribute__((visibility("default")))`.
- The tests moved from `tests/` to `test/`.
- The `asan-ubsan` preset compiles with `-fno-sanitize-recover=all`, so an
  UndefinedBehaviorSanitizer report fails the test.

### Added

- `ToRuntimeVersion()` in `<eversion/runtime_version.h>` copies a `Version`
  into an owning `RuntimeVersion`, for example to keep a plugin's version after
  the plugin is unloaded. Before, each field had to be copied by hand.
- `RuntimeVersion` has `<=>` and `==`, both with another `RuntimeVersion` and
  with a `Version` on either side. They use the same precedence rules as
  `Version` and ignore build metadata. They replace converting both sides
  with `AsVersion()`, which is removed.
- `examples/version_report`, a plugin host built only against the installed
  package and run by `quality.py check`.
- The `coverage` and `docs` presets, with the `eversion_ENABLE_COVERAGE` and
  `eversion_BUILD_DOCS` options, and `scripts/coverage.py`. The template's
  ThreadSanitizer preset, option, and CI job were left out: EVersion has no
  threads.
- API documentation with Doxygen; the `eversion::detail` namespace is left out
  of it.
- Contract tests under `scripts/` for the CMake, CI, packaging, and
  documentation conventions; CI runs them.
- `.editorconfig`, `.gitattributes`, an optional pre-commit hook,
  `CONTRIBUTING.md`, `SECURITY.md`, this changelog, and issue and pull request
  templates.

### Fixed

- `eversion_declare()` and `eversion_declare_plugin()` drop leading zeros from
  `MAJOR`, `MINOR`, and `PATCH`. Before, `MINOR 010` became the octal literal
  `010U`, so `kVersionMinor` was 8 while `kVersionString` read `010`, and
  `MINOR 09` did not compile. `project(VERSION 2026.09.1)` passes `09` this
  way. The version string uses the same numbers, so it reads `2026.9.1`.
- A `MAJOR`, `MINOR`, or `PATCH` value above 4294967295 stops the configure
  step with an error. Before, it was silently truncated in the generated
  constants.
- `eversionDeclare.cmake` finds its templates next to itself on every
  configure. Before, a consumer kept the template directory of the first
  eversion installation it was configured with, so switching to another
  installation either failed or used the old templates.
- The plugin metadata test looks up `eversion::kPluginInfoSymbol` through a
  null-terminated copy, because a `std::string_view` does not guarantee a
  terminating null character. The README example does the same.
- The CMake script tests configure their sub-projects with the generator, the
  make program, and the C++ compiler of the enclosing build, and check that the
  sub-projects use them. Before, the sub-projects used CMake's defaults: a build
  machine without `make` failed the tests, and a Clang build compiled the
  sub-projects with the default compiler.

### Removed

- `ToVersion(const ComponentVersion&)`. Use `ToRuntimeVersion()` with the same
  argument instead: it copies the plugin's strings into a `RuntimeVersion`, so
  the version stays valid after the plugin library is unloaded. The view that
  `ToVersion()` returned pointed into the plugin and dangled after `dlclose()`.
  The replacement is not `constexpr` or `noexcept`, because it copies strings.
- `RuntimeVersion::AsVersion()`. A `RuntimeVersion` no longer hands out
  `Version` views of its strings, so a stored value cannot dangle. Before,
  `TryParseVersion(text)->AsVersion()` compiled and returned a view into a
  temporary that was destroyed at the end of the statement. Compare a
  `RuntimeVersion` directly with `<=>` and `==` instead.
- The free function `eversion::ToString(const Version&)`. It did the same as
  the member `Version::ToString()`, which remains; write `version.ToString()`
  instead.
- Windows and macOS CI jobs, and the Windows code paths in the CMake functions
  and tests.
- The `shared` dimension of the CI quality matrix, and `BUILD_SHARED_LIBS` in
  `CMakePresets.json`. EVersion is header-only, so the variable affects no
  target; the CI quality jobs went from four to two.
