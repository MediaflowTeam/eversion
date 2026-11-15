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

- `examples/version_report`, a plugin host built only against the installed
  package and run by `quality.py check`.
- The `tsan`, `coverage`, and `docs` presets, with the
  `eversion_ENABLE_THREAD_SANITIZER`, `eversion_ENABLE_COVERAGE`, and
  `eversion_BUILD_DOCS` options, and `scripts/coverage.py`.
- API documentation with Doxygen; the `eversion::detail` namespace is left out
  of it.
- Contract tests under `scripts/` for the CMake, CI, packaging, and
  documentation conventions; CI runs them.
- `.editorconfig`, `.gitattributes`, an optional pre-commit hook,
  `CONTRIBUTING.md`, `SECURITY.md`, this changelog, and issue and pull request
  templates.

### Fixed

- The plugin metadata test looks up `eversion::kPluginInfoSymbol` through a
  null-terminated copy, because a `std::string_view` does not guarantee a
  terminating null character. The README example does the same.
- The CMake script tests configure their sub-projects with the generator, the
  make program, and the C++ compiler of the enclosing build, and check that the
  sub-projects use them. Before, the sub-projects used CMake's defaults: a build
  machine without `make` failed the tests, and a Clang build compiled the
  sub-projects with the default compiler.

### Removed

- Windows and macOS CI jobs, and the Windows code paths in the CMake functions
  and tests.
