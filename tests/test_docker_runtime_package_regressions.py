#!/usr/bin/env python3

import json
import os
import re
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path


REPO_ROOT = Path(
    os.environ.get("THEMIS_SOURCE_ROOT", Path(__file__).resolve().parent.parent)
).resolve()
DOCKERFILE = REPO_ROOT / "docker" / "Dockerfile.unified"

EXPECTED_UBUNTU_2404_RUNTIME_PACKAGE_TOKENS = frozenset(
    {
        "librocksdb8.9",
        "libgrpc++1.51t64",
        "libprotobuf32t64",
        "libcurl4t64",
    }
)
LEGACY_UBUNTU_2204_RUNTIME_PACKAGE_TOKENS = frozenset(
    {
        # Exact package tokens only: the Ubuntu 24.04 Noble runtime/debug stages
        # intentionally use the distinct t64 package names instead.
        # In particular, libprotobuf32 and libprotobuf32t64 are treated as
        # different exact tokens by this regression guard.
        "librocksdb7",
        "libgrpc++1",
        "libprotobuf32",
        # Exact-token guard: keep the bare libcurl4 package token out of the
        # Noble runtime/debug stages so the Dockerfile stays pinned to the
        # explicit Ubuntu 24.04 t64 package family.
        "libcurl4",
    }
)


def extract_stage(text: str, stage_name: str, base_image: str = "ubuntu:24.04") -> str:
    match = re.search(
        rf"^FROM {re.escape(base_image)} AS {re.escape(stage_name)}\n(?P<body>.*?)(?=^FROM |\Z)",
        text,
        re.MULTILINE | re.DOTALL,
    )
    if match is None:
        raise AssertionError(f"Could not find Docker stage {stage_name!r}")
    return match.group("body")


def extract_installed_packages(stage_text: str, stage_name: str) -> set[str]:
    packages: set[str] = set()
    if "xargs -r apt-get install" in stage_text:
        manifest = REPO_ROOT / "scripts/release/runtime-dependencies-ubuntu-24.04.txt"
        packages.update(manifest.read_text(encoding="utf-8").split())
    lines = stage_text.splitlines()

    for index, line in enumerate(lines):
        if "apt-get install" not in line:
            continue

        inline_tail = line.split("apt-get install", 1)[1].strip().rstrip("\\")
        for token in inline_tail.split():
            if not token.startswith("-"):
                packages.add(token)

        continuation_open = line.rstrip().endswith("\\")
        for candidate in lines[index + 1 :]:
            if not continuation_open:
                break

            stripped = candidate.strip()
            if not stripped:
                continuation_open = False
                break

            cleaned = stripped.rstrip("\\").strip()
            before_and = cleaned.split("&&", 1)[0].strip()
            if before_and:
                packages.update(token for token in before_and.split() if not token.startswith("-"))
            if "&&" in cleaned:
                break
            continuation_open = stripped.endswith("\\")

    if not packages:
        raise AssertionError(f"Docker stage {stage_name!r} did not yield any parsed apt package tokens")
    return packages


class DockerRuntimePackageRegressionTests(unittest.TestCase):
    @staticmethod
    def _dockerfile_text() -> str:
        return DOCKERFILE.read_text(encoding="utf-8")

    def test_documentation_payload_sources_are_in_build_context(self) -> None:
        rules = [
            line.strip()
            for line in (REPO_ROOT / ".dockerignore").read_text(encoding="utf-8").splitlines()
            if line.strip() and not line.lstrip().startswith("#")
        ]
        self.assertEqual(rules[0], "*", "Keep the build context deny-by-default")
        build = extract_stage(self._dockerfile_text(), "build", base_image="deps")
        for directory in ("docs", "ai_context"):
            with self.subTest(directory=directory):
                self.assertTrue((REPO_ROOT / directory).is_dir())
                self.assertIn(f"COPY {directory} ./{directory}", build)
                self.assertIn(f"!{directory}/", rules)
                self.assertIn(f"!{directory}/**", rules)
                self.assertNotIn(f"{directory}/", rules)
                self.assertNotIn(f"{directory}/**", rules)

    def test_build_configure_preset_exists(self) -> None:
        build = extract_stage(self._dockerfile_text(), "build", base_image="deps")
        invocations = re.findall(r"\bcmake --preset ([\w-]+)", build)
        self.assertTrue(invocations, "Docker build must use a configure preset")
        presets = json.loads(
            (REPO_ROOT / "CMakePresets.json").read_text(encoding="utf-8")
        )["configurePresets"]
        available = {preset["name"] for preset in presets if not preset.get("hidden", False)}
        for name in invocations:
            with self.subTest(preset=name):
                self.assertIn(
                    name, available,
                    f"Docker configure preset {name!r} does not exist in the selected source. "
                    "Build a source revision containing the canonical Linux preset migration.",
                )

    def test_build_directory_matches_preset_and_image_copies(self) -> None:
        text = self._dockerfile_text()
        build = extract_stage(text, "build", base_image="deps")
        invocation = re.search(r"\bcmake --preset ([\w-]+)", build)
        self.assertIsNotNone(invocation)
        presets = json.loads(
            (REPO_ROOT / "CMakePresets.json").read_text(encoding="utf-8")
        )["configurePresets"]
        preset = next(
            (preset for preset in presets if preset["name"] == invocation.group(1)), None
        )
        self.assertIsNotNone(preset, "Docker configure preset is missing from selected source")
        directory = re.search(r"^ARG THEMIS_BUILD_DIR=(\S+)$", build, re.MULTILINE)
        self.assertIsNotNone(directory)
        self.assertEqual(preset["binaryDir"], "${sourceDir}/" + directory.group(1))
        self.assertIn('cmake --build "${THEMIS_BUILD_DIR}"', build)
        self.assertIn('cmake --install "${THEMIS_BUILD_DIR}"', build)
        for stage_name in ("runtime", "debug"):
            with self.subTest(stage=stage_name):
                self.assertIn(
                    f"COPY --from=build /src/{directory.group(1)}/bin/themis_server ",
                    extract_stage(text, stage_name),
                )

    def test_vcpkg_bootstrap_supports_native_arm64(self) -> None:
        base = extract_stage(self._dockerfile_text(), "base")
        self.assertIn(
            "${VCPKG_ROOT}/bootstrap-vcpkg.sh -disableMetrics", base,
            "Use the pinned vcpkg bootstrap script: it builds vcpkg natively on ARM64. "
            "Publish a new release tag containing the corrected Dockerfile.",
        )
        self.assertNotRegex(
            base, r"(?m)^ADD\s+https://\S+/vcpkg-glibc\s",
            "vcpkg-glibc is x86-64-only and fails with Exec format error on ARM64.",
        )

    def test_base_installs_python_autoreconf_prerequisites(self) -> None:
        base = extract_stage(self._dockerfile_text(), "base")
        bootstrap = base.index("${VCPKG_ROOT}/bootstrap-vcpkg.sh")
        packages = extract_installed_packages(
            base[:bootstrap], "base"
        )
        for package in ("autoconf", "automake", "autoconf-archive", "pkg-config"):
            with self.subTest(package=package):
                self.assertIn(
                    package, packages,
                    "Install Python autoreconf prerequisites before vcpkg bootstrap. "
                    "Without autoconf-archive, CPython 3.11 configure.ac cannot expand "
                    "AX_C_FLOAT_WORDS_BIGENDIAN and autoreconf exits with status 1. "
                    "Build a source revision containing this package fix.",
                )

    def test_compose_base_installs_python_autoreconf_prerequisites(self) -> None:
        compose = (REPO_ROOT / "docker-compose.yml").read_text(encoding="utf-8")
        self.assertIn("dockerfile: Dockerfile", compose)

        dockerfile = (REPO_ROOT / "Dockerfile").read_text(encoding="utf-8")
        base = extract_stage(dockerfile, "base")
        bootstrap = base.index("./bootstrap-vcpkg.sh")
        packages = extract_installed_packages(base[:bootstrap], "base")
        for package in ("autoconf", "automake", "autoconf-archive", "pkg-config"):
            with self.subTest(package=package):
                self.assertIn(
                    package,
                    packages,
                    "The Compose build must install Python autoreconf prerequisites "
                    "before vcpkg bootstrap.",
                )

    def test_base_configures_bounded_apt_retries_before_update(self) -> None:
        base = extract_stage(self._dockerfile_text(), "base")
        retry_config = r"""printf '%s\n' 'Acquire::Retries "3";' > /etc/apt/apt.conf.d/80-retries"""
        self.assertIn(retry_config, base)
        self.assertLess(base.index(retry_config), base.index("apt-get update"))

    @unittest.skipUnless(Path("/bin/sh").is_file(), "Requires the Docker build's POSIX shell")
    def test_runtime_apt_refresh_recovers_and_fails_closed(self) -> None:
        stage = extract_stage(self._dockerfile_text(), "runtime")
        match = re.search(
            r"^RUN --mount=type=cache,id=apt-cache-runtime-[^\n]*\\\n"
            r"\s*--mount=[^\n]*\\\n(?P<body>(?:[^\n]*\\\n)*[^\n]*)",
            stage, re.MULTILINE,
        )
        self.assertIsNotNone(match, "Missing runtime APT installation step")
        command = match.group("body").replace("\\\n", "")
        shell_commands = """
            calls=0
            apt_get() {
                calls=$((calls + 1))
                printf 'APT: %s\\n' "$*"
                [ "$calls" -gt "$FAILURES" ]
            }
            rm() { printf 'CLEAN: %s\\n' "$*"; }
            sleep() { printf 'SLEEP: %s\\n' "$*"; }
            xargs() {
                printf 'INSTALL: %s\\n' "$*"
                cat
                return "$INSTALL_STATUS"
            }
            alias apt-get=apt_get
        """
        manifest = REPO_ROOT / "scripts/release/runtime-dependencies-ubuntu-24.04.txt"
        for failures, install_status in ((0, 0), (2, 0), (5, 0), (0, 123)):
            with self.subTest(failures=failures, install_status=install_status):
                result = subprocess.run(
                    ["/bin/sh", "-c", shell_commands + command.replace(
                        "/tmp/runtime-dependencies.txt", f'"{manifest}"'
                    )],
                    env={**os.environ, "FAILURES": str(failures),
                         "INSTALL_STATUS": str(install_status)},
                    capture_output=True, text=True, timeout=10,
                )
                exhausted = failures >= 5
                expected_status = 100 if exhausted else install_status
                self.assertEqual(result.returncode, expected_status, result.stderr)
                lines = result.stdout.splitlines()
                self.assertEqual(
                    lines.count("APT: update --error-on=any"), min(failures + 1, 5)
                )
                self.assertEqual(lines.count("SLEEP: 15"), min(failures, 4))
                self.assertEqual(
                    sum(line.startswith("CLEAN: ") for line in lines),
                    min(failures, 4) + int(expected_status == 0),
                )
                self.assertEqual(
                    "INSTALL: -r apt-get install -y --no-install-recommends" in lines,
                    not exhausted,
                )
                if exhausted:
                    self.assertIn("failed after 5 attempts", result.stderr)
                else:
                    for package in manifest.read_text(encoding="utf-8").split():
                        self.assertIn(package, result.stdout)

    def test_community_logging_dependencies_use_system_packages(self) -> None:
        deps_packages = extract_installed_packages(
            extract_stage(self._dockerfile_text(), "deps", base_image="base"), "deps"
        )
        manifest = json.loads(
            (REPO_ROOT / "docker" / "vcpkg-community.json").read_text(encoding="utf-8")
        )
        vcpkg_dependencies = {
            dependency if isinstance(dependency, str) else dependency["name"]
            for dependency in manifest["dependencies"]
        }

        # Compiled spdlog and its fmt headers must use the same ABI/provider.
        for library in ("fmt", "spdlog"):
            with self.subTest(library=library):
                self.assertIn(f"lib{library}-dev", deps_packages)
                self.assertNotIn(library, vcpkg_dependencies)

    def test_community_manifest_includes_boost_required_by_cmake(self) -> None:
        manifest = json.loads(
            (REPO_ROOT / "docker" / "vcpkg-community.json").read_text(encoding="utf-8")
        )
        vcpkg_dependencies = {
            dependency if isinstance(dependency, str) else dependency["name"]
            for dependency in manifest["dependencies"]
        }

        self.assertIn("boost", vcpkg_dependencies)

    @unittest.skipUnless(Path("/bin/sh").is_file(), "Requires the Docker build's POSIX shell")
    def test_community_dependency_guard_rejects_mixed_logging_providers(self) -> None:
        deps_stage = extract_stage(self._dockerfile_text(), "deps", base_image="base")
        match = re.search(
            r"^RUN set -eu; \\\n(?P<body>(?:[^\n]*\\\n)*[^\n]*)",
            deps_stage,
            re.MULTILINE,
        )
        self.assertIsNotNone(match, "Missing COMMUNITY logging provider guard")
        command = match.group("body").replace("\\\n", "")

        for triplet in ("x64-linux", "arm64-linux"):
            for edition in ("COMMUNITY", "community", "ENTERPRISE"):
                for library in (None, "fmt", "spdlog"):
                    with self.subTest(triplet=triplet, edition=edition, library=library):
                        with tempfile.TemporaryDirectory() as temporary:
                            root = Path(temporary)
                            (root / "triplet.txt").write_text(triplet, encoding="utf-8")
                            if library:
                                (root / triplet / "include" / library).mkdir(parents=True)
                            result = subprocess.run(
                                [
                                    "/bin/sh", "-eu", "-c",
                                    command.replace("/build/vcpkg_installed", temporary)
                                    .replace("/tmp/triplet.txt", str(root / "triplet.txt")),
                                ],
                                env={**os.environ, "THEMIS_EDITION": edition},
                                capture_output=True,
                                text=True,
                            )
                            rejected = edition.lower() == "community" and library is not None
                            self.assertEqual(result.returncode, int(rejected), result.stderr)
                            if rejected:
                                self.assertIn(f"found vcpkg {library} for {triplet}", result.stdout)

    def test_runtime_stage_uses_ubuntu_2404_runtime_packages(self) -> None:
        runtime_stage = extract_stage(self._dockerfile_text(), "runtime")
        runtime_packages = extract_installed_packages(runtime_stage, "runtime")

        for package_name in EXPECTED_UBUNTU_2404_RUNTIME_PACKAGE_TOKENS:
            self.assertIn(package_name, runtime_packages)

    @unittest.skipUnless(Path("/bin/sh").is_file(), "Requires the Docker build's POSIX shell")
    def test_llama_libraries_come_from_server_build(self) -> None:
        text = self._dockerfile_text()
        build = extract_stage(text, "build", base_image="deps")
        match = re.search(
            r"^RUN set -eux; \\\n(?P<body>    mkdir -p /opt/themis-runtime/lib; \\\n"
            r"(?:[^\n]*\\\n)*[^\n]*)",
            build, re.MULTILINE,
        )
        self.assertIsNotNone(match, "Missing server-linked llama library staging")
        command = match.group("body").replace("\\\n", "")
        for stage_name in ("runtime", "debug"):
            stage = extract_stage(text, stage_name)
            self.assertIn("COPY --from=build /opt/themis-runtime/lib/ /usr/local/lib/", stage)
            self.assertNotIn("COPY --from=llama /opt/llama.cpp/build/lib", stage)
            self.assertIn("ldd /app/themis_server", stage)
            self.assertIn('grep -q "not found"', stage)

        for enabled, missing in (("ON", False), ("ON", True), ("OFF", True)):
            with self.subTest(enabled=enabled, missing=missing), tempfile.TemporaryDirectory() as temporary:
                root = Path(temporary)
                source = root / "build" / "bin"
                source.mkdir(parents=True)
                for library in ("libllama", "libggml", "libggml-cpu", "libggml-base"):
                    if missing and library.startswith("libggml"):
                        continue
                    (source / f"{library}.so.0.1").write_text(library, encoding="utf-8")
                    (source / f"{library}.so.0").symlink_to(f"{library}.so.0.1")
                destination = root / "runtime" / "lib"
                result = subprocess.run(
                    ["/bin/sh", "-eu", "-c", command.replace("/src/", f"{root}/")
                     .replace("/opt/themis-runtime/lib", str(destination))],
                    env={**os.environ, "ENABLE_LLM": enabled, "THEMIS_BUILD_DIR": "build"},
                    capture_output=True, text=True,
                )
                self.assertEqual(result.returncode == 0, not (enabled == "ON" and missing), result.stderr)
                if enabled == "ON" and not missing:
                    for library in ("libllama", "libggml", "libggml-cpu", "libggml-base"):
                        link = destination / f"{library}.so.0"
                        self.assertTrue(link.is_symlink())
                        self.assertEqual(link.read_text(encoding="utf-8"), library)
                if enabled == "OFF":
                    self.assertEqual(list(destination.iterdir()), [])

    def test_runtime_checks_linker_and_rootless_loader_before_publishing(self) -> None:
        stage = extract_stage(self._dockerfile_text(), "runtime")
        for marker in ("ldd /app/themis_server", 'grep -q "not found"',
                       "RUN /app/themis_server --version",
                       'CMD ["--port", "8080", "--data-dir", "/app/data/themis_server"]'):
            self.assertIn(marker, stage)
        self.assertLess(stage.index("USER themis"), stage.index("RUN /app/themis_server --version"))
        packages = extract_installed_packages(stage, "runtime")
        self.assertTrue({"curl", "libfmt9", "libspdlog1.12"}.issubset(packages))

    def test_runtime_and_debug_include_modular_libraries_and_numa(self) -> None:
        dockerfile = self._dockerfile_text()
        build = extract_stage(dockerfile, "build", base_image="deps")
        directory = re.search(r"^ARG THEMIS_BUILD_DIR=(\S+)$", build, re.MULTILINE)
        self.assertIsNotNone(directory)
        copy = f"COPY --from=build /src/{directory.group(1)}/lib/libthemis_*.so* /usr/local/lib/"
        for stage_name in ("runtime", "debug"):
            with self.subTest(stage=stage_name):
                stage = extract_stage(dockerfile, stage_name)
                self.assertIn("libnuma1", extract_installed_packages(stage, stage_name))
                self.assertIn(copy, stage)
                self.assertLess(stage.index(copy), stage.index("RUN ldconfig"))
                self.assertIn("ldd /app/themis_server", stage)
                self.assertIn('grep -q "not found"', stage)

    def test_selected_source_preflight_rejects_missing_runtime_dependencies(self) -> None:
        dockerfile = self._dockerfile_text()
        build = extract_stage(dockerfile, "build", base_image="deps")
        directory = re.search(r"^ARG THEMIS_BUILD_DIR=(\S+)$", build, re.MULTILINE)
        self.assertIsNotNone(directory)
        copy = f"COPY --from=build /src/{directory.group(1)}/lib/libthemis_*.so* /usr/local/lib/\n"
        manifest_path = Path("scripts/release/runtime-dependencies-ubuntu-24.04.txt")
        manifest = (REPO_ROOT / manifest_path).read_text(encoding="utf-8")
        self.assertEqual(dockerfile.count(copy), 2)
        self.assertIn("libnuma1\n", manifest)

        for missing in (None, "runtime", "debug", "numa"):
            with self.subTest(missing=missing), tempfile.TemporaryDirectory() as temporary:
                root = Path(temporary)
                (root / "docker").mkdir()
                (root / manifest_path.parent).mkdir(parents=True)
                selected_dockerfile = dockerfile
                if missing in ("runtime", "debug"):
                    stage_start = selected_dockerfile.index(f"FROM ubuntu:24.04 AS {missing}\n")
                    copy_start = selected_dockerfile.index(copy, stage_start)
                    selected_dockerfile = (
                        selected_dockerfile[:copy_start]
                        + selected_dockerfile[copy_start + len(copy):]
                    )
                (root / "docker/Dockerfile.unified").write_text(selected_dockerfile, encoding="utf-8")
                (root / manifest_path).write_text(
                    manifest.replace("libnuma1\n", "") if missing == "numa" else manifest,
                    encoding="utf-8",
                )
                result = subprocess.run(
                    [sys.executable, str(Path(__file__).resolve()),
                     "DockerRuntimePackageRegressionTests.test_runtime_and_debug_include_modular_libraries_and_numa"],
                    env={**os.environ, "THEMIS_SOURCE_ROOT": str(root)},
                    capture_output=True, text=True, timeout=10,
                )
                self.assertEqual(result.returncode == 0, missing is None, result.stderr)
                if missing:
                    self.assertIn("libnuma1" if missing == "numa" else copy.strip(), result.stderr)

    def test_debug_stage_uses_ubuntu_2404_runtime_packages(self) -> None:
        debug_stage = extract_stage(self._dockerfile_text(), "debug")
        debug_packages = extract_installed_packages(debug_stage, "debug")

        for package_name in EXPECTED_UBUNTU_2404_RUNTIME_PACKAGE_TOKENS:
            self.assertIn(package_name, debug_packages)

    def test_runtime_and_debug_stages_do_not_reintroduce_legacy_runtime_packages(self) -> None:
        dockerfile_text = self._dockerfile_text()
        runtime_packages = extract_installed_packages(extract_stage(dockerfile_text, "runtime"), "runtime")
        debug_packages = extract_installed_packages(extract_stage(dockerfile_text, "debug"), "debug")
        legacy_runtime_overlap = runtime_packages & LEGACY_UBUNTU_2204_RUNTIME_PACKAGE_TOKENS
        legacy_debug_overlap = debug_packages & LEGACY_UBUNTU_2204_RUNTIME_PACKAGE_TOKENS

        self.assertFalse(
            legacy_runtime_overlap,
            msg=(
                "Runtime stage must not reinstall Ubuntu 22.04 runtime package tokens: "
                f"{sorted(legacy_runtime_overlap)}"
            ),
        )
        self.assertFalse(
            legacy_debug_overlap,
            msg=(
                "Debug stage must not reinstall Ubuntu 22.04 runtime package tokens: "
                f"{sorted(legacy_debug_overlap)}"
            ),
        )


if __name__ == "__main__":
    unittest.main()
