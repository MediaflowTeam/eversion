#!/bin/sh
# Installs the tools for the cpp-library-template quality gate on a Debian or Ubuntu system,
# such as the debian:13-slim image that CI uses. Run it as root or as a user
# with sudo:
#
#   scripts/install_linux_tools.sh [--clang] [--doxygen] [--gcovr] VENV_DIR
#
# Then add VENV_DIR/bin to PATH. The script has two parts. The distribution
# packages (compiler, Python, git, GoogleTest) can come from any recent
# release; on another distribution, replace that part with its package
# manager. The tools whose version affects results come from PyPI or from a
# pinned Doxygen release. --clang installs the LLVM release that clang-format
# uses in tool-requirements.txt; on Debian, when the distribution does not
# provide it, the script adds the <codename>-backports source and installs it
# from there. It links clang and clang++ in VENV_DIR/bin.
set -eu

# Pinned Doxygen release, with the SHA-256 that GitHub lists for the asset.
doxygen_version=1.15.0
doxygen_sha256=0ec2e5b2c3cd82b7106d19cb42d8466450730b8cb7a9e85af712be38bf4523a1
# gcovr needs Python 3.10 or newer.
gcovr_version=8.8

with_clang=false
with_doxygen=false
with_gcovr=false
while [ "$#" -gt 0 ]; do
  case "$1" in
    --clang)
      # Install the LLVM release that clang-format uses. On Debian this may
      # come from <codename>-backports; clang and clang++ are linked into the
      # tool directory after the virtual environment is created.
      with_clang=true
      shift
      ;;
    --doxygen)
      with_doxygen=true
      shift
      ;;
    --gcovr)
      with_gcovr=true
      shift
      ;;
    -*)
      echo "usage: $0 [--clang] [--doxygen] [--gcovr] VENV_DIR" >&2
      exit 2
      ;;
    *)
      break
      ;;
  esac
done
if [ "$#" -ne 1 ]; then
  echo "usage: $0 [--clang] [--doxygen] [--gcovr] VENV_DIR" >&2
  exit 2
fi
venv_dir=$1
script_dir=$(CDPATH='' cd -- "$(dirname -- "$0")" && pwd)

if [ "$with_clang" = true ]; then
  llvm_major=$(sed -n 's/^clang-format==\([0-9][0-9]*\)\..*/\1/p' "$script_dir/tool-requirements.txt")
  if [ -z "$llvm_major" ]; then
    echo "error: could not read the clang-format major version from $script_dir/tool-requirements.txt." >&2
    exit 1
  fi
fi

sudo=""
if [ "$(id -u)" -ne 0 ]; then
  sudo=sudo
fi

# The show subcommand succeeds for a package that another package references
# but that has no installable version; madison lists only real candidates.
package_is_available() {
  [ -n "$(apt-cache madison "$1" 2>/dev/null)" ]
}

# Distribution packages.
packages="ca-certificates curl g++ git libgtest-dev python3 python3-venv"
if [ "$with_clang" = true ]; then
  packages="$packages clang-$llvm_major"
fi
$sudo env DEBIAN_FRONTEND=noninteractive apt-get update
if [ "$with_clang" = true ] && ! package_is_available "clang-$llvm_major"; then
  if [ -r /etc/os-release ]; then
    . /etc/os-release
  fi
  if [ "${ID:-}" = debian ] && [ -n "${VERSION_CODENAME:-}" ]; then
    backports_list="/etc/apt/sources.list.d/$VERSION_CODENAME-backports.list"
    echo "deb http://deb.debian.org/debian $VERSION_CODENAME-backports main" \
      | $sudo tee "$backports_list" >/dev/null
    $sudo env DEBIAN_FRONTEND=noninteractive apt-get update
  fi
  if ! package_is_available "clang-$llvm_major"; then
    echo "error: clang-$llvm_major is not available in the configured apt sources." >&2
    exit 1
  fi
fi
$sudo env DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
  $packages

# Tools with recommended versions; see docs/tool-versions.md.
python3 -m venv "$venv_dir"
"$venv_dir/bin/python" -m pip install --disable-pip-version-check --quiet \
  -r "$script_dir/tool-requirements.txt"

if [ "$with_clang" = true ]; then
  if ! clang_path=$(command -v "clang-$llvm_major"); then
    echo "error: clang-$llvm_major was not found after installation." >&2
    exit 1
  fi
  if ! clangxx_path=$(command -v "clang++-$llvm_major"); then
    echo "error: clang++-$llvm_major was not found after installation." >&2
    exit 1
  fi
  ln -sf "$clang_path" "$venv_dir/bin/clang"
  ln -sf "$clangxx_path" "$venv_dir/bin/clang++"
fi

if [ "$with_gcovr" = true ]; then
  "$venv_dir/bin/python" -m pip install --disable-pip-version-check --quiet \
    "gcovr==$gcovr_version.*"
fi

if [ "$with_doxygen" = true ]; then
  archive="$venv_dir/doxygen-$doxygen_version.linux.bin.tar.gz"
  tag=Release_$(echo "$doxygen_version" | tr . _)
  curl -fsSL -o "$archive" \
    "https://github.com/doxygen/doxygen/releases/download/$tag/doxygen-$doxygen_version.linux.bin.tar.gz"
  echo "$doxygen_sha256  $archive" | sha256sum -c -
  tar -xzf "$archive" -C "$venv_dir"
  ln -sf "$venv_dir/doxygen-$doxygen_version/bin/doxygen" "$venv_dir/bin/doxygen"
  rm "$archive"
fi

echo "Installed the tools in $venv_dir/bin; add it to PATH."
