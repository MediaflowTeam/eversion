#!/usr/bin/env python3

"""Check the header-only package version and install contract.

The package version file must accept only patch releases within the same
major.minor series, before and after 1.0, and it must be architecture
independent.  The install tree must provide the generated header, the package
declaration helper and its templates, but no library files.  These checks run
on Linux because they inspect the installed tree.
"""

from __future__ import annotations

import os
import re
import shutil
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

from quality import SKIPPED_DIRECTORY_NAMES


ROOT_DIR = Path(__file__).resolve().parents[1]
CMAKE_LISTS = ROOT_DIR / "CMakeLists.txt"
README = ROOT_DIR / "README.md"
FUTURE_VERSION = (1, 2, 3)
LIBRARY_GLOB_PATTERNS = ("lib*.so*", "lib*.a")

PROJECT_RE = re.compile(
    r"\bproject\s*\(\s*(?P<name>[A-Za-z_][A-Za-z0-9_]*)\s+VERSION\s+"
    r"(?P<version>\d+\.\d+\.\d+)",
    re.IGNORECASE,
)
INSTALL_CONTRACT_RE = re.compile(
    r"^## Install Contract[ \t]*$(.*?)(?=^## |\Z)",
    re.MULTILINE | re.DOTALL,
)


def format_version(version: tuple[int, int, int]) -> str:
    return ".".join(str(part) for part in version)


def read_project(cmake_lists: Path) -> tuple[str, tuple[int, int, int]]:
    text = cmake_lists.read_text(encoding="utf-8")
    match = PROJECT_RE.search(text)
    if match is None:
        raise AssertionError(
            f"{cmake_lists} has no project(<name> VERSION X.Y.Z) declaration."
        )
    major, minor, patch = (
        int(part) for part in match.group("version").split(".")
    )
    return match.group("name"), (major, minor, patch)


def set_project_version(
    cmake_lists: Path, version: tuple[int, int, int]
) -> None:
    text = cmake_lists.read_text(encoding="utf-8")
    match = PROJECT_RE.search(text)
    if match is None:
        raise AssertionError(
            f"{cmake_lists} has no project(<name> VERSION X.Y.Z) declaration."
        )
    updated = (
        text[: match.start("version")]
        + format_version(version)
        + text[match.end("version") :]
    )
    cmake_lists.write_text(updated, encoding="utf-8")


def ignore_when_copying(directory: str, names: list[str]) -> set[str]:
    # Skip generated directories and local symlinks such as `review`, whose
    # targets lie outside the repository.
    current = Path(directory)
    return {
        name
        for name in names
        if name in SKIPPED_DIRECTORY_NAMES
        or name.endswith(".dir")
        or (current / name).is_symlink()
    }


def run_checked(command: list[str], cwd: Path | None = None) -> None:
    result = subprocess.run(
        command,
        cwd=cwd,
        capture_output=True,
        text=True,
        check=False,
    )
    if result.returncode != 0:
        raise AssertionError(
            f"Command failed with exit code {result.returncode}: "
            f"{' '.join(command)}\n"
            f"stdout:\n{result.stdout}\n"
            f"stderr:\n{result.stderr}"
        )


def generator_arguments() -> list[str]:
    # The CI contract job installs only Ninja from PyPI and has no make, while
    # a local machine may have only make. An explicit generator wins.
    if "CMAKE_GENERATOR" in os.environ:
        return []
    if shutil.which("ninja") is not None:
        return ["-G", "Ninja"]
    return []


def build_and_install(
    source: Path,
    build: Path,
    install: Path,
    name: str,
) -> Path:
    run_checked(
        [
            "cmake",
            "-S",
            str(source),
            "-B",
            str(build),
            *generator_arguments(),
            f"-D{name}_BUILD_TESTS=OFF",
            "-DCMAKE_BUILD_TYPE=Release",
        ]
    )
    run_checked(["cmake", "--build", str(build), "--config", "Release"])
    run_checked(
        [
            "cmake",
            "--install",
            str(build),
            "--config",
            "Release",
            "--prefix",
            str(install),
        ]
    )
    return install


def installed_library_files(install_dir: Path) -> list[Path]:
    matches: set[Path] = set()
    for pattern in LIBRARY_GLOB_PATTERNS:
        for path in install_dir.rglob(pattern):
            if path.is_file() or path.is_symlink():
                matches.add(path)
    return sorted(matches)


def compatibility_requests(
    version: tuple[int, int, int],
) -> tuple[list[str], list[str]]:
    major, minor, patch = version
    compatible = [f"{major}.{minor}", format_version(version)]
    incompatible = [
        f"{major}.{minor}.{patch + 1}",
        f"{major}.{minor + 1}",
        f"{major + 1}.{minor}",
    ]
    if minor > 0:
        incompatible.append(f"{major}.{minor - 1}")
    return compatible, incompatible


def find_config_version(install_dir: Path, name: str) -> Path:
    matches = sorted(install_dir.rglob(f"{name}ConfigVersion.cmake"))
    if len(matches) != 1:
        relative = [
            str(path.relative_to(install_dir)) for path in matches
        ]
        raise AssertionError(
            f"Expected exactly one installed {name}ConfigVersion.cmake under "
            f"{install_dir}, but found {relative}."
        )
    return matches[0]


def probe_config_version(
    version_file: Path,
    requests: list[str],
    temp_dir: Path,
    pointer_size: int | None = None,
) -> dict[str, tuple[bool, bool]]:
    """Return each request's (compatible, unsuitable) probe result."""
    lines: list[str] = []
    if pointer_size is not None:
        lines.append(f"set(CMAKE_SIZEOF_VOID_P {pointer_size})")
    for request in requests:
        components = [int(part) for part in request.split(".")]
        count = len(components)
        while len(components) < 3:
            components.append(0)
        major, minor, patch = components
        lines.extend(
            [
                f'set(PACKAGE_FIND_VERSION "{request}")',
                f"set(PACKAGE_FIND_VERSION_MAJOR {major})",
                f"set(PACKAGE_FIND_VERSION_MINOR {minor})",
                f"set(PACKAGE_FIND_VERSION_PATCH {patch})",
                f"set(PACKAGE_FIND_VERSION_COUNT {count})",
                "unset(PACKAGE_VERSION_COMPATIBLE)",
                "unset(PACKAGE_VERSION_UNSUITABLE)",
                f'include("{version_file.as_posix()}")',
                "if(PACKAGE_VERSION_COMPATIBLE)",
                f'  message("RESULT|{request}|COMPATIBLE|TRUE")',
                "else()",
                f'  message("RESULT|{request}|COMPATIBLE|FALSE")',
                "endif()",
                "if(PACKAGE_VERSION_UNSUITABLE)",
                f'  message("RESULT|{request}|UNSUITABLE|TRUE")',
                "else()",
                f'  message("RESULT|{request}|UNSUITABLE|FALSE")',
                "endif()",
            ]
        )

    script = temp_dir / "probe_config_version.cmake"
    script.write_text("\n".join(lines) + "\n", encoding="utf-8")
    result = subprocess.run(
        ["cmake", "-P", str(script)],
        cwd=temp_dir,
        capture_output=True,
        text=True,
        check=False,
    )
    output = result.stdout + result.stderr
    if result.returncode != 0:
        raise AssertionError(
            f"cmake -P failed with exit code {result.returncode}:\n{output}"
        )

    compatible_values: dict[str, bool] = {}
    unsuitable_values: dict[str, bool] = {}
    for line in output.splitlines():
        if not line.startswith("RESULT|"):
            continue
        _, request, kind, value = line.split("|")
        if kind == "COMPATIBLE":
            compatible_values[request] = value == "TRUE"
        elif kind == "UNSUITABLE":
            unsuitable_values[request] = value == "TRUE"
    return {
        request: (
            compatible_values[request],
            unsuitable_values[request],
        )
        for request in requests
    }


def check_config_version(
    case: unittest.TestCase,
    install_dir: Path,
    name: str,
    version: tuple[int, int, int],
) -> None:
    version_file = find_config_version(install_dir, name)
    compatible, incompatible = compatibility_requests(version)
    expected = {request: True for request in compatible}
    expected.update({request: False for request in incompatible})
    with tempfile.TemporaryDirectory(
        prefix="package-version-probe-"
    ) as temp:
        probes = probe_config_version(
            version_file, list(expected), Path(temp)
        )
    actual = {
        request: compatible
        for request, (compatible, _) in probes.items()
    }
    case.assertEqual(
        actual,
        expected,
        f"Version compatibility mismatch for {version_file}: "
        f"expected {expected}, got {actual}.",
    )


def check_architecture_independence(
    case: unittest.TestCase,
    install_dir: Path,
    name: str,
    version: tuple[int, int, int],
) -> None:
    version_file = find_config_version(install_dir, name)
    request = format_version(version)
    with tempfile.TemporaryDirectory(
        prefix="package-architecture-probe-"
    ) as temp:
        for pointer_size in (4, 8):
            with case.subTest(pointer_size=pointer_size):
                compatible, unsuitable = probe_config_version(
                    version_file,
                    [request],
                    Path(temp),
                    pointer_size=pointer_size,
                )[request]
                case.assertTrue(
                    compatible,
                    f"{version_file} is not compatible with its own "
                    f"version {request} for a {pointer_size}-byte pointer.",
                )
                case.assertFalse(
                    unsuitable,
                    f"{version_file} is unsuitable for a "
                    f"{pointer_size}-byte pointer, so it is not "
                    "architecture independent.",
                )


def check_no_library_files(
    case: unittest.TestCase, install_dir: Path
) -> None:
    library_files = installed_library_files(install_dir)
    relative = [
        str(path.relative_to(install_dir)) for path in library_files
    ]
    case.assertEqual(
        library_files,
        [],
        f"The header-only install tree {install_dir} contains library "
        f"files: {relative}.",
    )


def check_header_and_templates(
    case: unittest.TestCase,
    install_dir: Path,
    name: str,
) -> None:
    version_file = find_config_version(install_dir, name)
    package_dir = version_file.parent
    expected_files = [
        install_dir / "include" / name / "version_info.h",
        package_dir / f"{name}Declare.cmake",
        package_dir / "templates" / "version_info.h.in",
        package_dir / "templates" / "plugin_info.cc.in",
    ]
    for path in expected_files:
        case.assertTrue(
            path.is_file(),
            f"The header-only install tree {install_dir} is missing "
            f"{path.relative_to(install_dir)}.",
        )


def check_readme_install_contract(case: unittest.TestCase) -> None:
    text = README.read_text(encoding="utf-8")
    match = INSTALL_CONTRACT_RE.search(text)
    if match is None:
        case.fail("README.md has no '## Install Contract' section.")
    section = match.group(1)
    for required in ("`SameMinorVersion`", "`ARCH_INDEPENDENT`"):
        case.assertIn(
            required,
            section,
            "The README.md Install Contract section must mention "
            f"{required}.",
        )


class InstallContractDocsTest(unittest.TestCase):
    def test_readme_documents_the_install_contract(self) -> None:
        check_readme_install_contract(self)


@unittest.skipUnless(
    sys.platform.startswith("linux"),
    "The install tree and package version checks require Linux.",
)
@unittest.skipUnless(
    shutil.which("cmake") is not None,
    "cmake is not on PATH.",
)
class PackageVersionContractTest(unittest.TestCase):
    name: str
    version: tuple[int, int, int]
    current_install: Path
    future_install: Path

    @classmethod
    def setUpClass(cls) -> None:
        temp_dir = tempfile.TemporaryDirectory(
            prefix="package-version-contract-"
        )
        cls.addClassCleanup(temp_dir.cleanup)
        base = Path(temp_dir.name)
        cls.name, cls.version = read_project(CMAKE_LISTS)

        cls.current_install = build_and_install(
            ROOT_DIR,
            base / "current" / "build",
            base / "current" / "install",
            cls.name,
        )

        future_root = base / "future" / "repo"
        shutil.copytree(ROOT_DIR, future_root, ignore=ignore_when_copying)
        set_project_version(future_root / "CMakeLists.txt", FUTURE_VERSION)
        cls.future_install = build_and_install(
            future_root,
            base / "future" / "build",
            base / "future" / "install",
            cls.name,
        )

    def installations(self) -> list[tuple[Path, tuple[int, int, int]]]:
        return [
            (self.current_install, self.version),
            (self.future_install, FUTURE_VERSION),
        ]

    def test_package_version_file_accepts_only_the_same_minor(self) -> None:
        for install_dir, version in self.installations():
            with self.subTest(version=format_version(version)):
                check_config_version(self, install_dir, self.name, version)

    def test_package_version_file_is_architecture_independent(self) -> None:
        for install_dir, version in self.installations():
            with self.subTest(version=format_version(version)):
                check_architecture_independence(
                    self, install_dir, self.name, version
                )

    def test_install_tree_has_no_library_files(self) -> None:
        for install_dir, version in self.installations():
            with self.subTest(version=format_version(version)):
                check_no_library_files(self, install_dir)

    def test_install_tree_has_header_and_templates(self) -> None:
        for install_dir, version in self.installations():
            with self.subTest(version=format_version(version)):
                check_header_and_templates(self, install_dir, self.name)


if __name__ == "__main__":
    unittest.main()
