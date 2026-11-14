# Tool Versions

This page lists the tool versions that this repository recommends and that CI
uses.

**These versions are recommendations, not requirements.** The quality gate runs
with whatever versions are on your `PATH`, and other versions usually work. The
recommendations exist because formatting and static analysis results change
between tool releases: with other versions, `clang-format` can reformat code
that CI accepts, and `clang-tidy` can report findings that CI does not, or miss
findings that CI reports. Use the recommended versions when you want local
results to match CI.

**The exception is the CMake minimum.** The CMake files require CMake 4.2 or
newer, the same version that is recommended, so CI always builds with the
minimum version. Newer CMake releases work.

## Recommended Versions

| Tool | Recommended | Notes |
| --- | --- | --- |
| clang-format | 21.1 | Formatting output differs between minor releases. |
| clang-tidy | 21.1 | New releases add and extend checks, and `.clang-tidy` treats every warning as an error. |
| clang | 21.1 | The compiler, when you build with Clang. The same LLVM release as clang-format and clang-tidy; the CI Clang build checks it with `tool_versions.py --tool clang++`. |
| GCC | 14.2 | The compiler, when you build with GCC: Debian 13's version, which CI uses. Later GCC 14 releases only fix bugs, so any 14.x works the same. GCC 14 implements all of C++20 except modules. The ABI of C++20 library features is stable only from GCC 16, so build libraries that pass C++20 types across their interfaces with one GCC major version. CI does not check the version. |
| CMake | 4.2 | The version that CI builds with, and also the minimum: the CMake files declare `cmake_minimum_required(VERSION 4.2)`. Newer versions work. |
| Doxygen | 1.15 | Only needed for the `docs` preset. Use 1.13 or newer: older versions do not report undocumented namespaces. |
| gcovr | 8.8 | Only needed for coverage reports (`scripts/coverage.py`). Needs Python 3.10 or newer. |
| Ninja | any | The version does not affect results. |

Only major.minor matters. Patch releases of clang-format, clang-tidy, and
clang 21.1 give the same results, so any 21.1.x is fine. LLVM publishes only N.1.x
releases for each major version, so 21.1 also means LLVM 21.

`scripts/tool-requirements.txt` holds the recommended versions of the tools
that come from PyPI. CI installs from that file, and the contract tests check
that this page, the file, and CI agree.

## Check Your Tools

```sh
python3 scripts/tool_versions.py
```

The script prints the version of each tool on `PATH` next to the recommended
version. A different or missing tool only produces a warning, and the script
exits with status 0. CI runs it with `--strict`, which fails instead.
`--tool clang` or `--tool clang++` also checks the clang compiler against the
recommended LLVM release. The CI Clang rows use `--tool clang++`, see
[What CI Uses](#what-ci-uses).

## Install the Recommended Versions

Install clang-format, clang-tidy, CMake, and Ninja from PyPI into a virtual
environment, then put its `bin` directory first on `PATH`:

```sh
python3 -m venv ~/.venvs/cpp-tools
~/.venvs/cpp-tools/bin/pip install -r scripts/tool-requirements.txt
export PATH="$HOME/.venvs/cpp-tools/bin:$PATH"
```

Keep the virtual environment outside the repository: `quality.py` checks every
C++ file under the repository root, and some packages contain C++ headers.

PyPI does not provide the clang compiler. On Debian and Ubuntu,
`scripts/install_linux_tools.sh --clang` installs clang 21 and links `clang`
and `clang++` into the virtual environment, see [Linux Image](#linux-image).

Distribution packages also provide the recommended versions:

- Ubuntu 26.04: `sudo apt-get install clang clang-format clang-tidy cmake doxygen`
  installs clang, clang-format, and clang-tidy 21.1, CMake 4.2, and Doxygen
  1.15.
- Debian 13: the `clang-21` package in `trixie-backports` provides clang 21.1;
  Debian 13's own `clang` is 19.
- Doxygen: the [release binaries](https://github.com/doxygen/doxygen/releases)
  run on other distributions.

Older distributions ship a CMake below the 4.2 minimum, for example Ubuntu
24.04 (3.28) and Debian 13 (3.31). Install CMake from PyPI on those systems.

## What CI Uses

Every CI job installs the recommended versions and runs
`tool_versions.py --strict` before it builds anything.

| Runner | Compilers | Tools |
| --- | --- | --- |
| `debian:13-slim` container on `ubuntu-24.04` | GCC 14.2 from Debian; clang 21 from Debian 13 backports | PyPI; Doxygen 1.15.0 release |

Every job builds with GCC 14.2. The quality job also builds with clang 21, the
same LLVM release as clang-format and clang-tidy, so that CI builds with a
second compiler. `install_linux_tools.sh --clang` installs it from Debian's
`trixie-backports`, because Debian 13 itself ships clang 19, and links
`clang++` to it in the tools directory. The Clang rows check its version with
`tool_versions.py --strict --tool clang++`.

GoogleTest comes from Debian. Its version does not affect results.

### Actions and Token

The workflow uses two actions: `actions/checkout`, and
`actions/upload-artifact` for the coverage report. Each is pinned to the full commit SHA of
a release, with the release in a trailing comment, so that moving a tag cannot
change what runs. To update an action, look up the commit of the new release,
for example with
`git ls-remote --tags https://github.com/actions/checkout v7.0.1` (for an
annotated tag, use the line that ends in `^{}`), and change both the SHA and
the comment. Use one release of each action in the whole workflow;
`scripts/ci_hardening_contract_test.py` checks this.

Every job's `GITHUB_TOKEN` can only read the repository, whatever the
repository or organization default is: the workflow sets
`permissions: contents: read`. Checkout runs with
`persist-credentials: false`, so the token is not left in the git
configuration for the tools and tests that run after it. A job that needs more,
for example to publish documentation, should declare it in its own
`permissions` block instead of widening the workflow's.

### Caches

The workflow uses no caches. pip downloads are not cached: installing the
PyPI tools takes about the same time with a cache, because most of it goes into
creating the virtual environment and installing the packages.

### Linux Image

Every job runs in the `debian:13-slim` image instead of directly on the
runner's Ubuntu image. The slim image contains only a minimal Debian system,
and `scripts/install_linux_tools.sh` installs everything the jobs need, so the
CI environment is defined in this repository rather than by the runner image.

The script has two parts:

- **Distribution packages**: GCC, Python, git, and GoogleTest, from `apt-get`.
  Their versions do not need to match a recommendation. The script works on
  Debian and Ubuntu images, such as `debian:13-slim`, `ubuntu:24.04`, and
  `ubuntu:26.04`; for another distribution, replace this part with its
  package manager.
- **Clang, with `--clang`**: the `clang-N` package, where N is the LLVM major
  version of clang-format in `tool-requirements.txt`, and `clang` and
  `clang++` links to it in the virtual environment. When the distribution
  does not provide that package, the script adds Debian's
  `<codename>-backports` source on Debian and stops with an error elsewhere.
  Debian 13 gets clang 21 from `trixie-backports`, and Ubuntu 26.04 provides
  it directly; Ubuntu 24.04 does not have it.
- **Tools with recommended versions**: clang-format, clang-tidy, CMake, and
  Ninja from PyPI; with `--doxygen`, the pinned Doxygen release, checked
  against its SHA-256; and with `--gcovr`, the pinned gcovr release from PyPI. This part is the same on every Linux image with glibc
  2.28 or newer; the Doxygen release needs glibc 2.34 or newer, which Debian
  12, Ubuntu 22.04, and later releases provide.

To reproduce a CI job locally with Docker:

```sh
docker run --rm -it -v "$PWD:/work" -w /work debian:13-slim
# Inside the container:
apt-get update && apt-get install -y --no-install-recommends ca-certificates git
sh scripts/install_linux_tools.sh /opt/tools
export PATH="/opt/tools/bin:$PATH"
python3 scripts/tool_versions.py --strict
QUALITY_CMAKE_ARGS="-G Ninja" python3 scripts/quality.py check
```

The container builds in the mounted `build/` directory. Remove `build/quality`
before and after, or mount a separate clone, because a CMake build directory
cannot be shared between the host and the container.

The script can also set up another Linux image for this repository, for
example as a `RUN` step in a Dockerfile.

ThreadSanitizer aborts when the kernel's mmap randomization breaks its memory
layout. The CI job therefore runs the `tsan` build and tests under
`setarch "$(uname -m)" -R`, which turns randomization off for those processes.
Docker's default seccomp profile blocks this, so the container is started with
`--security-opt seccomp=unconfined`; add that option to `docker run` to run
the `tsan` preset locally.

## Change a Recommendation

1. Update `scripts/tool-requirements.txt` and this page. For LLVM, the CI
   Clang build follows: `install_linux_tools.sh --clang` installs `clang-N`
   for the new major version, so check that Debian 13 or its backports
   provide that package; for Doxygen,
   update the version and SHA-256 in `scripts/install_linux_tools.sh`, and for
   gcovr, `gcovr_version` in the same script. For
   CMake, the recommendation is also the minimum version: update
   `cmake_minimum_required` in `CMakeLists.txt` and
   `examples/universe_answer/CMakeLists.txt`, `cmakeMinimumRequired` in
   `CMakePresets.json`, and the CMake Contract in `README.md`.
   `scripts/cmake_minimum_contract_test.py` checks that they agree.
2. Run `python3 scripts/quality.py check` with the new versions and fix new
   formatting differences and clang-tidy findings in the same change.
3. Record the change in `CHANGELOG.md`.
