# Examples

The examples directory contains small downstream projects that use EVersion
only through the installed CMake package, with
`find_package(eversion CONFIG REQUIRED)`.

## `version_report`

`examples/version_report` is a plugin host. Its `CMakeLists.txt`:

- calls `eversion_declare()` for the `version_report_cli` executable, which
  generates `<version_report/version_info.h>` with the project version and the
  build metadata `example.1`;
- builds the `version_report_plugin` module and calls
  `eversion_declare_plugin()` for it, which generates the plugin's metadata
  entry point. The plugin has no source file of its own.

`version_report_cli` prints its own version and EVersion's version, from the
installed `<eversion/version_info.h>`. Given the path of a plugin, it also
loads the plugin with `dlopen()`, resolves `eversion::kPluginInfoSymbol`,
checks the metadata with `eversion::IsValidComponentInfo()`, and prints the
plugin's ID, name, and version. The CTest test runs it with the example plugin:

```text
version_report 1.4.2+example.1
eversion 0.1.0
plugin example.report.plugin (Version Report Plugin) 2.0.1-beta.1
```

If the plugin cannot be loaded, has no metadata entry point, or returns invalid
metadata, the program prints the reason to standard error and exits with
status 1.

Build it from an installed tree:

```sh
cmake --preset quality
cmake --build --preset quality
cmake --install build/quality --prefix build/install
cmake -S examples/version_report -B build/examples/version_report -DCMAKE_PREFIX_PATH="$PWD/build/install"
cmake --build build/examples/version_report
ctest --test-dir build/examples/version_report --output-on-failure
```

The example sets the same warning options as the library. Configure it with
`-Dversion_report_WARNINGS_AS_ERRORS=ON` to make warnings fail the build.

The standard quality gate also runs this installed-package consumer check, with
warnings as errors, and runs `clang-tidy` on the example's sources:

```sh
python3 scripts/quality.py check
```
