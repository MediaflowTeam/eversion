<p align="center">
  <img src="docs/assets/logo.png" alt="EVersion logo" width="160">
</p>

# eversion

EVersion (Easy Version) is a small header-only C++20 library for exposing
project, library, and dynamic-plugin version metadata without public
`VERSION_*` macros. Its CMake package and target are named `eversion`.

EVersion provides these pieces:

| Surface | Purpose |
| --- | --- |
| `eversion::Version` | A constexpr-friendly non-owning version view with weak precedence comparison and string formatting. |
| `eversion::RuntimeVersion` | An owning runtime version value used by parsing and long-lived metadata. |
| `eversion_declare()` | A CMake function that generates and installs an `inline constexpr` version header for a target. |
| `<eversion/format.h>` | `std::format` and `operator<<` support for `Version` and `RuntimeVersion`. |
| `<eversion/component_info.h>` | Stable C ABI component/plugin metadata types and validation APIs. |
| `eversion_declare_plugin()` | A CMake function that generates a stable C ABI metadata entry point for a dynamic plugin. |

EVersion supports Linux only, with GCC or Clang; CI builds with GCC 14 and
Clang 21 on Debian 13. MSVC and clang-cl stop the CMake configure step with an
error. On Windows, use WSL; on macOS, use a Linux container or virtual machine.

The repository was created from
[cpp-library-template](https://github.com/MediaflowTeam/cpp-library-template);
see [Template Version](#template-version).

## Documentation

- [Release Branching Contract](docs/release-branching.md): branch roles,
  SemVer release flow, hotfix flow, and old-version support flow.
- [Plugin Metadata ABI](docs/abi.md): the binary layout shared by hosts and
  plugins, the rules for changing it, how to validate plugin metadata, and the
  history of every layout.
- [Tool Versions](docs/tool-versions.md): recommended versions of
  clang-format, clang-tidy, CMake, Doxygen, and gcovr, and the environments CI
  uses.
- [License](docs/license.md): what GPL-3.0 means for users of the library.
- [Contributing](CONTRIBUTING.md): checks to run before submitting a change,
  branch and commit conventions, and pull requests.
- [Security Policy](SECURITY.md): supported versions and private vulnerability
  reporting.
- [Changelog](CHANGELOG.md): notable changes, in Keep a Changelog format.

Issue forms and the pull request template live under `.github/`.

## Required Local Tools

Building and installing EVersion needs CMake and GCC or Clang with C++20
support. The tests also need GoogleTest, and the quality gate needs
`clang-format` and `clang-tidy`.

CMake 4.2 or newer is required: the CMake files declare it as their minimum version. This repository recommends clang-format and clang-tidy 21.1, GCC 14.2 or clang 21.1 as the compiler, and CMake 4.2, the versions that CI uses. Apart from the CMake minimum, these are recommendations, not requirements: other versions usually work, but formatting and `clang-tidy` results can differ from CI. See [Tool Versions](docs/tool-versions.md) for the reasons and for installing the recommended versions from PyPI. To compare your tools with the recommendations, run:

```sh
python3 scripts/tool_versions.py
```

Ubuntu or Debian:

```sh
sudo apt-get update
sudo apt-get install -y cmake clang-format clang-tidy libgtest-dev
```

On Ubuntu 26.04 these packages have the recommended versions. Older releases, such as Ubuntu 24.04 and Debian 13, provide a CMake older than 4.2, which is too old for the build: install CMake from PyPI instead, as described in [Tool Versions](docs/tool-versions.md), and clang-format and clang-tidy as well if you want to match CI.

The build does not download GoogleTest for you. If CMake cannot find
`GTest CONFIG`, install GoogleTest with your package manager or build it
yourself, then pass the package location through `CMAKE_PREFIX_PATH` or
`GTest_DIR`.

Doxygen is optional. Only the `docs` preset needs it; see
[API Documentation](#api-documentation).

## Install

The recommended usage is to install EVersion once, then consume it with
`find_package()` from other projects. Installing does not need GoogleTest when
the tests are turned off:

```sh
cmake -S eversion -B eversion/build -DCMAKE_BUILD_TYPE=Release -Deversion_BUILD_TESTS=OFF
cmake --build eversion/build
cmake --install eversion/build --prefix /path/to/eversion
```

Point consumer projects at that prefix with `CMAKE_PREFIX_PATH`:

```sh
cmake -S app -B app/build -DCMAKE_PREFIX_PATH=/path/to/eversion
```

```cmake
find_package(eversion CONFIG REQUIRED)
```

`find_package(eversion)` provides the `eversion::eversion` target and the
`eversion_declare()` and `eversion_declare_plugin()` functions. EVersion also
dogfoods itself: installed packages include `<eversion/version_info.h>` for
EVersion's own version constants.

Projects that used the earlier `EVersion` package must change
`find_package(EVersion ...)` to `find_package(eversion ...)` and rename the
`EVERSION_*` build options; see [CHANGELOG.md](CHANGELOG.md). The target, the
CMake functions, and the generated code are unchanged.

## Generate Version Information for a Target

In a consumer CMake project:

```cmake
project(sample_core
  VERSION 1.0.0
  LANGUAGES CXX
)

find_package(eversion CONFIG REQUIRED)

add_library(sample_core ...)
target_link_libraries(sample_core PUBLIC eversion::eversion)

eversion_declare(sample_core
  NAMESPACE sample_core::version_info
  MAJOR ${PROJECT_VERSION_MAJOR}
  MINOR ${PROJECT_VERSION_MINOR}
  PATCH ${PROJECT_VERSION_PATCH}
  OUTPUT sample_core/version_info.h
)
```

Use the generated header from C++:

```cpp
#include <sample_core/version_info.h>

static_assert(sample_core::version_info::kVersionMajor == 1);
static_assert(sample_core::version_info::kVersionString == "1.0.0");

constexpr eversion::Version current = sample_core::version_info::kVersion;
```

The generated header exposes these constants:

```cpp
namespace sample_core::version_info {

inline constexpr std::uint32_t kVersionMajor;
inline constexpr std::uint32_t kVersionMinor;
inline constexpr std::uint32_t kVersionPatch;
inline constexpr std::string_view kVersionPrerelease;
inline constexpr std::string_view kVersionBuildMetadata;
inline constexpr std::string_view kVersionString;
inline constexpr ::eversion::Version kVersion;

}  // namespace sample_core::version_info
```

By default, `eversion_declare()` adds both build and install include paths and
installs the generated header under `include/<OUTPUT directory>`. Use
`NO_INSTALL` for purely internal generated headers. Use `INSTALL_DESTINATION`
when a project intentionally installs public headers somewhere other than
`${CMAKE_INSTALL_INCLUDEDIR}`. `OUTPUT` must be a safe relative path.

`MAJOR`, `MINOR`, and `PATCH` are decimal numbers. Leading zeros are dropped,
so `project(sample_core VERSION 2026.09.1)` with the arguments above generates
`kVersionMinor == 9` and `kVersionString == "2026.9.1"`. A value above
4294967295 stops the configure step with an error.

### Choosing `NAMESPACE` and `OUTPUT`

The generated header is protected by an include guard derived from `OUTPUT`:
characters other than letters and digits become `_`, and the result is upper
case. `sample_core/version_info.h` gets `SAMPLE_CORE_VERSION_INFO_H_`.

Within one configure, `eversion_declare()` stops with an error when two targets
use the same `NAMESPACE`, the same `OUTPUT`, or two `OUTPUT` values that give the
same include guard, such as `ffmpeg-wrap/version_info.h` and
`ffmpeg_wrap/version_info.h`. The error names the target that declared the value
first, its `OUTPUT`, and the `CMakeLists.txt` it was declared in; the CMake call
stack below the error shows the second declaration.

EVersion cannot see the names used by packages that were configured and installed
separately. Two such libraries with clashing names configure without an error,
and a program that uses both breaks later: a header found first on the include
path hides the other one, a second header with the same include guard is skipped,
or the same `inline` constant gets two definitions in different translation
units, which no compiler or linker reports. Follow this convention so that the
names stay unique:

- Start both `NAMESPACE` and `OUTPUT` with your project's own unique name, and
  make them match: `NAMESPACE mediaflow::base::version_info` with
  `OUTPUT mediaflow/base/version_info.h`, and `mediaflow::flow::version_info`
  with `mediaflow/flow/version_info.h` for a second library of the same project.
- Write `OUTPUT` explicitly. Without it, the path is derived from `NAMESPACE`, so
  `mediaflow::base::version_info` would give
  `mediaflow/base/version_info/version_info.h`.
- Do not use generic top-level names such as `version_info.h` or
  `version::info`, and do not tell two libraries apart only by `-` versus `_` or
  by letter case.

The `eversion` names are reserved for EVersion's own generated header. Outside
the `eversion` project, `eversion_declare()` rejects a `NAMESPACE` that is
`eversion` or starts with `eversion::`, an `OUTPUT` that starts with `eversion/`,
and an `OUTPUT` whose include guard starts with `EVERSION_`, such as
`eversion_version.h`, whose guard `EVERSION_VERSION_H_` is the guard of
`<eversion/version.h>`.

## Generate Version Metadata for a Dynamic Plugin

If a host application loads dynamic plugins at runtime, each plugin can export a
small metadata entry point generated by EVersion. EVersion does not load
plugins; it only defines the metadata structure and generates the exported
function.

Plugin CMake example:

```cmake
find_package(eversion CONFIG REQUIRED)

add_library(sample_plugin MODULE ...)
target_link_libraries(sample_plugin PRIVATE eversion::eversion)

eversion_declare_plugin(sample_plugin
  ID sample.plugin
  NAME "Sample Plugin"
  NAMESPACE sample_plugin::version_info
  MAJOR 1
  MINOR 2
  PATCH 3
  BUILD_METADATA git.deadbeef
  OUTPUT sample_plugin/version_info.h
  NO_INSTALL
)
```

The plugin can still use its generated C++ version header internally:

```cpp
#include <sample_plugin/version_info.h>

static_assert(sample_plugin::version_info::kVersionString ==
              "1.2.3+git.deadbeef");
```

The generated plugin source exports `eversion_plugin_info`. A host application
can load the plugin, resolve that symbol, and validate the returned metadata:

```cpp
#include <dlfcn.h>
#include <eversion/component_info.h>
#include <eversion/runtime_version.h>

#include <string>

// dlsym() needs a null-terminated name; a std::string_view does not
// guarantee one.
const std::string symbol_name{eversion::kPluginInfoSymbol};
void* symbol = dlsym(plugin_handle, symbol_name.c_str());
auto info_function =
    reinterpret_cast<eversion::ComponentInfoFunction>(symbol);

const eversion::ComponentInfo* info = info_function();
// IsValidComponentInfo() checks the structure; IsWellFormedComponentVersion()
// checks that the version strings follow the SemVer identifier grammar.
if (eversion::IsValidComponentInfo(info) &&
    eversion::IsWellFormedComponentVersion(info->version)) {
  // Owns copies of the strings, so it stays valid after dlclose().
  const eversion::RuntimeVersion plugin_version =
      eversion::ToRuntimeVersion(info->version);
  // info->id and info->name point into the plugin; copy them before dlclose()
  // if they are needed later.
  const std::string plugin_id{info->id};
}
```

Plugin discovery, `dlopen()` and `dlclose()`, dynamic-library lifetime, and
error handling are host responsibilities; see
[`examples/version_report`](examples/README.md) for a complete host.
`ToRuntimeVersion(ComponentVersion)` copies the plugin-provided strings into a
`RuntimeVersion`, so the version can be kept after the plugin is unloaded. The
`id` and `name` fields of `ComponentInfo` are still `const char*` pointers into
the plugin; copy them as well if they must outlive the plugin library.

## Runtime Metadata ABI

Only `ComponentInfo` and `ComponentVersion` cross the plugin boundary. Those
structures avoid `std::string`, `std::string_view`, virtual interfaces,
exceptions, and RTTI, and contain only integers and `const char*` fields:

```cpp
namespace eversion {

inline constexpr std::uint32_t kComponentInfoAbiVersion = 1U;
inline constexpr std::string_view kPluginInfoSymbol = "eversion_plugin_info";

struct ComponentVersion {
  std::uint32_t major;
  std::uint32_t minor;
  std::uint32_t patch;
  const char* prerelease;
  const char* build_metadata;
};

struct ComponentInfo {
  std::uint32_t abi_version;
  std::size_t struct_size;
  const char* id;
  const char* name;
  ComponentVersion version;
};

inline constexpr std::size_t kComponentInfoRequiredSize;

bool IsComponentInfoFieldAvailable(
    const ComponentInfo* info, std::size_t offset, std::size_t size);

using ComponentInfoFunction = const ComponentInfo* (*)() noexcept;

}  // namespace eversion
```

The rules, in short:

- Existing fields never change. Within one `kComponentInfoAbiVersion`, new
  fields are only appended to the end of `ComponentInfo`, and hosts read them
  only after `IsComponentInfoFieldAvailable()` confirms that the plugin's
  `struct_size` covers them. Any other change bumps `kComponentInfoAbiVersion`.
- `ComponentVersion` is fixed: it holds exactly the five SemVer parts and never
  gains fields.
- The layout holds only between a host and plugins built for the same
  architecture.
- `IsValidComponentInfo()` checks the structure and is required before reading
  any field. `IsWellFormedComponentVersion()` checks that the prerelease and build
  metadata strings follow the SemVer identifier grammar; a valid structure does
  not mean the plugin's strings are well formed.

[docs/abi.md](docs/abi.md) is the full reference: the field-by-field layout of
the current ABI, the reasons for each rule, the validation guidance, the steps
for changing the ABI, and the history of every layout.

## Use Version Values

If a project does not need generated headers, it can include the value type
directly:

```cpp
#include <eversion/version.h>

constexpr eversion::Version current{.major = 1, .minor = 4, .patch = 0};
constexpr eversion::Version minimum{.major = 1, .minor = 2, .patch = 0};
static_assert(current >= minimum);
```

`eversion::Version` is a non-owning view: `prerelease` and `build_metadata` are
`std::string_view` members. Use it for generated constants, string literals, or
other storage that is known to outlive the view. `ToString()` is
`constexpr`, so a version can also be checked as text at compile time, for
example `static_assert(current.ToString() == "1.4.0")`.

`Version` does not validate its string members; any text can be stored in
them. `eversion_declare()` and `TryParseVersion()` accept only valid
identifiers, and plugin versions can be checked with
`IsWellFormedComponentVersion()`. Invalid text causes no undefined behavior; it
only compares in byte order.

Include `<eversion/runtime_version.h>` when parsed strings or metadata need
owned storage. `RuntimeVersion` owns its strings and has the same queries as
`Version`. It compares with other `RuntimeVersion` values and with `Version`
values by the same precedence rules, and ignores build metadata too:

```cpp
#include <eversion/runtime_version.h>

#include <optional>

const std::optional<eversion::RuntimeVersion> parsed =
    eversion::TryParseVersion("1.2.3-rc.1+git.deadbeef");
if (parsed.has_value() && *parsed < sample_core::version_info::kVersion) {
  // The parsed version is older than this build.
}
```

`RuntimeVersion` does not hand out `Version` views of its strings, so a stored
`RuntimeVersion` cannot dangle. The other direction is a copy:
`ToRuntimeVersion()` returns a `RuntimeVersion` that owns copies of a
`Version`'s prerelease and build metadata strings. Use it to keep a `Version`
longer than the storage it views:

```cpp
std::string prerelease = "rc.1";
const eversion::Version view{.major = 1, .minor = 2, .patch = 3,
                             .prerelease = prerelease};
const eversion::RuntimeVersion kept = eversion::ToRuntimeVersion(view);
prerelease = "rc.2";  // kept.prerelease is still "rc.1"
```

`Version` and `RuntimeVersion` do not convert to each other implicitly.

`TryParseVersion()` accepts `MAJOR.MINOR.PATCH[-PRERELEASE][+BUILD]`. Core
components must fit in `std::uint32_t`; prerelease and build metadata use the
same dot-separated identifier grammar as `eversion_declare()`.

Leading zeros are read as numbers, as in `eversion_declare()`, and are not
rejected as strict SemVer would. In the core, they are dropped:
`TryParseVersion("01.2.3")` gives 1.2.3. A numeric prerelease identifier keeps
its text but compares by value, so `1.2.3-rc.007` prints as written and is
weakly equivalent to `1.2.3-rc.7`.

Version comparison rules:

- `major`, `minor`, and `patch` are compared numerically.
- A version with a prerelease tag ranks below the same final version.
- Numeric prerelease identifiers are compared numerically, ignoring leading
  zeros.
- Numeric prerelease identifiers rank below non-numeric identifiers.
- Build metadata is informational and does not affect precedence.

Therefore, `1.2.3+git.a` and `1.2.3+git.b` compare as weakly equivalent even
though their string representations remain distinct.

### Exact Equality

`==` asks whether two versions have the same precedence. To ask whether every
field is the same, including the exact prerelease and build metadata text, use
`IsIdenticalTo()`. Two versions are identical exactly when `ToString()` gives
the same text:

| Pair | `a == b` | `a.IsIdenticalTo(b)` |
| --- | --- | --- |
| `1.2.3+a` and `1.2.3+b` | true | false |
| `1.0.0-rc.007` and `1.0.0-rc.7` | true | false |
| `1.2.3-rc.1+a` and `1.2.3-rc.1+a` | true | true |

`Version::IsIdenticalTo()` takes a `Version` and is `constexpr`.
`RuntimeVersion::IsIdenticalTo()` takes a `RuntimeVersion` or a `Version`. With
a `Version` on the left and a `RuntimeVersion` on the right, swap the operands.

### Hashing

`std::hash` is specialized for `Version` in `<eversion/version.h>` and for
`RuntimeVersion` in `<eversion/runtime_version.h>`, so both work as keys of
`std::unordered_map` and `std::unordered_set`. The hash agrees with `==`: it
ignores build metadata and reads numeric prerelease identifiers as numbers, so
`1.2.3+a` and `1.2.3+b` hash equally, and so do `1.0.0-rc.007` and
`1.0.0-rc.7`. A `RuntimeVersion` hashes to the same value as an equal `Version`.

```cpp
#include <eversion/runtime_version.h>

#include <unordered_map>

std::unordered_map<eversion::RuntimeVersion, int> plugin_count;
```

### Formatting

Include `<eversion/format.h>` to format versions with `std::format` or write
them to a `std::ostream`. Both produce `ToString()`. The format specification is
the one for strings, so width, alignment, and fill work:

```cpp
#include <eversion/format.h>

#include <format>
#include <iostream>
#include <string>

const eversion::Version version{.major = 1, .minor = 2, .patch = 3};
std::cout << version << '\n';                           // 1.2.3
const std::string row = std::format("{:>9}|", version);  // "    1.2.3|"
```

`<eversion/format.h>` is a separate header because `<format>` noticeably slows
down the compilation of every translation unit that includes it. Including
`<eversion/version.h>` alone does not pull it in.

## Add Build Metadata

Pass build metadata such as a git commit or CI build id through
`BUILD_METADATA`:

```cmake
eversion_declare(sample_core
  NAMESPACE sample_core::version_info
  MAJOR 1
  MINOR 2
  PATCH 3
  BUILD_METADATA "${GIT_COMMIT_SHA}"
)
```

Plugins can use the same argument:

```cmake
eversion_declare_plugin(sample_plugin
  ID sample.plugin
  NAME "Sample Plugin"
  NAMESPACE sample_plugin::version_info
  MAJOR 2
  MINOR 0
  PATCH 1
  BUILD_METADATA "${GIT_COMMIT_SHA}"
)
```

## Vendored Source Builds

`add_subdirectory(external/eversion)` is still supported for local development
or vendored source builds. As a subproject, EVersion builds no tests and
installs nothing by default:

```cmake
add_subdirectory(external/eversion)

add_library(sample_core ...)
target_link_libraries(sample_core PUBLIC eversion::eversion)
eversion_declare(sample_core ...)
```

The preferred reusable-library path remains installation plus
`find_package(eversion CONFIG REQUIRED)`.

## Design Choices

- Public version information is exposed as `inline constexpr` C++ symbols.
- No public version macros are generated.
- CMake writes generated headers with `configure_file(... @ONLY)`.
- The plugin ABI uses C-style structs to avoid exposing C++ STL types,
  exceptions, or vtables across dynamic-library boundaries.
- `TryParseVersion()` parses exact version strings, not version ranges.
- EVersion does not implement version range expressions such as `^1.2` or
  `~1.2.3`.
- EVersion does not provide a plugin loading framework.

## Layout

```text
.
├── CMakeLists.txt
├── cmake/                  CMake modules, including eversionDeclare.cmake
├── include/eversion/       public headers; detail/ holds implementation details
├── templates/              templates for the generated header and plugin source
├── test/                   GoogleTest tests and CMake script tests
├── examples/               installed-package consumers
├── docs/
└── scripts/                quality gate, coverage, and contract tests
```

EVersion is header-only, so it has no `src/` directory. Public headers live
under `include/eversion/...`, and C++ sources use the `.cc` extension.

## C++ Style

EVersion follows Google C++ style as the baseline, with the local adjustments
encoded in `.clang-format` and `.clang-tidy`, such as always requiring braces
for control statements.

Use Doxygen comments for public API documentation. Prefer `/** ... */` block comments placed on their own line immediately before the symbol they document. Avoid trailing Doxygen forms after declarations or statements.

Document each public namespace as well as its members, because Doxygen does not check the members of an undocumented namespace. Wrap implementation details in public headers, such as the `eversion::detail` namespace, in `/** @cond INTERNAL */` and `/** @endcond */` to leave them out of the documentation and its checks.

Use standard include guards for public headers. Do not use non-standard pragma-based header guards. Guard names should follow the installed include path, be uppercase, and end in `_H_`, such as `EVERSION_VERSION_H_` for `include/eversion/version.h`.

## CMake Contract

EVersion requires CMake `4.2` or newer and builds as strict C++20. The
header-only `eversion` target is an `INTERFACE` library configured through
`eversion_configure_cxx_interface_target(...)`, which requires `cxx_std_20`.
Test and example executables set `CXX_STANDARD 20`, `CXX_STANDARD_REQUIRED ON`,
and `CXX_EXTENSIONS OFF` through `eversion_configure_cxx_target(...)`, so CMake
emits `-std=c++20` rather than a compiler-extension mode such as
`-std=gnu++20`.

C++20 module scanning is off: the root `CMakeLists.txt` and the example's `CMakeLists.txt` set `CMAKE_CXX_SCAN_FOR_MODULES` to `OFF` right after `project()`, before any target is created. EVersion does not use modules, and since CMake 3.28 (policy CMP0155), scanning is on by default for C++20 targets; with GCC it adds flags to `compile_commands.json` that `clang-tidy` cannot read.

The configure functions accept GCC and Clang and give them `-Wall -Wextra -Wpedantic`, plus `-Werror` when warnings are errors. Any other compiler, and any compiler that takes MSVC-style options, such as MSVC and clang-cl, stops the configure step with an error. AppleClang takes the same options as Clang and is accepted, but CI does not test it.

The project defines one main target named `eversion` and an alias target named
`eversion::eversion`. The alias target is the stable interface used by tests,
examples, and downstream consumers. Its warning and sanitizer options are
build-interface only, so consumers of the installed package do not inherit
them.

Build options:

| Option | Default | Effect |
| --- | --- | --- |
| `eversion_BUILD_TESTS` | ON when top-level | Build the tests in `test/`. |
| `eversion_INSTALL` | ON when top-level | Install the package; when OFF, `eversion_declare()` for EVersion's own header uses `NO_INSTALL`. |
| `eversion_BUILD_DOCS` | OFF | Add the `eversion_docs` Doxygen target. |
| `eversion_WARNINGS_AS_ERRORS` | OFF | Add `-Werror`, and make Doxygen warnings errors. |
| `eversion_ENABLE_SANITIZERS` | OFF | AddressSanitizer and UndefinedBehaviorSanitizer. |
| `eversion_ENABLE_COVERAGE` | OFF | gcov coverage instrumentation. |

## Install Contract

Installing EVersion installs:

- The public headers from `include/`, and the generated
  `<eversion/version_info.h>`.
- The `eversion::eversion` target, exported under the package namespace
  `eversion::`.
- `eversionDeclare.cmake` and the `templates/` it reads, so that
  `find_package(eversion)` provides `eversion_declare()` and
  `eversion_declare_plugin()`.
- A package config file for `find_package`, and a package version file
  matching the project version.

EVersion is header-only: the install tree contains no library binary, and does
not expose source-tree-only include directories.

The package version file uses `SameMinorVersion`: only patch releases are
compatible, before and after 1.0. `find_package(eversion 0.1)` accepts 0.1.x
releases, and rejects 0.2 and 1.0. EVersion's inline functions and the code
generated by `eversion_declare()` are compiled into each consumer, so a minor
release may change what consumers compile; the plugin metadata layout is
versioned separately by `kComponentInfoAbiVersion`. The version file is also
`ARCH_INDEPENDENT`, because a header-only package does not depend on the
pointer size of the build that installed it.

## Testing

The `test/` directory contains GoogleTest tests and CMake script tests. See
[test/README.md](test/README.md) for the test targets, GTest dependency
guidance, and commands for running tests. The tests cover:

- `eversion::Version` comparison, stable-version checks, and string formatting.
- `eversion::RuntimeVersion`, `TryParseVersion()` parsing boundaries, copies
  made by `ToRuntimeVersion()`, and comparisons with `RuntimeVersion` and
  `Version` values.
- Hashing that agrees with `==`, use as keys of unordered containers, and exact
  equality with `IsIdenticalTo()`.
- `std::format` and `operator<<` from `<eversion/format.h>`.
- Compile-time constants generated by `eversion_declare()`.
- Copying a `ComponentVersion` into a `RuntimeVersion` with
  `ToRuntimeVersion()`, including null string handling and use after the plugin
  is unloaded.
- ABI validation, type boundaries, and field availability for `ComponentInfo`.
- Dynamic plugin metadata generated by `eversion_declare_plugin()`.
- The argument checks of `eversion_declare()`.
- Installed-package consumption, including a producer that installs generated
  headers for a second consumer.

## Examples

The `examples/` directory contains downstream projects that use only the
installed package. See [examples/README.md](examples/README.md) for the current
example list and build commands.

## Quality Commands

The `scripts/` directory provides the standard local quality gate:

```sh
python3 scripts/quality.py check
```

The quality runner formats project-owned C++ files, verifies strict C++20 compile flags, runs CTest through the `quality` CMake preset, installs the library, builds and runs the installed-package example, and runs `clang-tidy` on real source translation units, generated header smoke translation units for the public headers, and the example's sources. See [scripts/README.md](scripts/README.md) for the full command list, file scope rules, header-check strategy, and `QUALITY_CMAKE_ARGS` override.

Useful build-variant checks:

```sh
QUALITY_CMAKE_ARGS="-DCMAKE_CXX_COMPILER=clang++" python3 scripts/quality.py check
cmake --preset asan-ubsan
cmake --build --preset asan-ubsan
ctest --preset asan-ubsan
```

The `asan-ubsan` preset compiles with `-fno-sanitize-recover=all`, so any UndefinedBehaviorSanitizer report fails the test instead of only printing a diagnostic.

EVersion has no ThreadSanitizer preset: the library has no threads or shared mutable state, so there is nothing for it to check.

To see which lines of `include/` the tests run, generate a coverage report. It needs gcovr; see [Tool Versions](docs/tool-versions.md).

```sh
python3 scripts/coverage.py
```

The script builds and tests the `coverage` preset, which compiles with `--coverage -O0` (GCC and Clang only), and writes `coverage.txt` and `coverage.html` to `build/coverage-report/`. Coverage is advisory: `quality.py check` does not run it, there is no minimum percentage, and the CI `Coverage` job only uploads the report as an artifact. GCC and Clang count some lines differently, for example the closing brace after a `throw`, so compare reports from the same compiler.

To check formatting before each commit, you can opt in to the [pre-commit hook](scripts/README.md#pre-commit-hook).

## API Documentation

The `docs` preset generates HTML documentation for the public headers in `include/` with Doxygen:

```sh
cmake --preset docs
cmake --build --preset docs
```

The output is written to `build/docs/doxygen/html/index.html`. The preset treats Doxygen warnings as errors, so the build fails when a public namespace, class, or member is undocumented, when a function documents some of its parameters but not all, or when a comment names a parameter that does not exist. Only this preset looks for Doxygen; other builds do not need it.

Use Doxygen 1.13 or newer. Older versions do not report undocumented namespaces and therefore skip the checks for their members; CMake prints a warning when it finds one. CI generates the documentation on Ubuntu 26.04, which provides Doxygen 1.15.

To generate the documentation from another build tree, configure it with `-Deversion_BUILD_DOCS=ON` and build the `eversion_docs` target. Doxygen warnings are errors there only when `eversion_WARNINGS_AS_ERRORS` is also ON.

## Template Version

The root `TEMPLATE_VERSION` file records the
[cpp-library-template](https://github.com/MediaflowTeam/cpp-library-template)
version that EVersion's build, quality, and packaging infrastructure follows,
as a single `vX.Y.Z` line. Update it only when adopting changes from a newer
template release.

`TEMPLATE_VERSION` is independent of the library version in
`project(... VERSION ...)`, which describes EVersion's own API and package.

## License

EVersion is licensed under GPL-3.0; the full text is in `LICENSE`. GPL-3.0 is
the default of the template EVersion was created from. It is a recommendation
of that template, not a requirement, and EVersion keeps it. See
[License](docs/license.md) for what GPL-3.0 means for users of the library.
