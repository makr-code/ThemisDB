#!/usr/bin/env python3

import itertools
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile
import textwrap
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parent.parent
COMPILER_OPTIONS = REPO_ROOT / "cmake" / "CompilerOptions.cmake"
MODULAR_BUILD = REPO_ROOT / "cmake" / "ModularBuild.cmake"
MONOLITHIC_BUILD = REPO_ROOT / "cmake" / "CMakeLists.txt"
WORKFLOW = REPO_ROOT / ".github" / "workflows" / "release-build-matrix.yml"
X64_PROCESSORS = ("x86_64", "amd64", "AMD64", "x64")
ARM_PROCESSORS = ("aarch64", "ARM64")
HANDLERS = ("monitoring_api_handler", "monitoring_api_routes", "index_api_handler", "prompt_api_handler")


@unittest.skipUnless(shutil.which("cmake") and shutil.which("ninja"), "requires CMake and Ninja")
class MainlineCompilerConfigurationTests(unittest.TestCase):
    def configure(self, body: str, build_type: str, **variables: str) -> dict[str, list[str]]:
        # Configure real CMake policies without loading a compiler or building ThemisDB.
        # MSVC branch selection below checks CMake semantics, not native cl.exe behavior.
        with tempfile.TemporaryDirectory(prefix="compiler-regression-") as tmp:
            root = Path(tmp)
            source = root / "source"
            source.mkdir()
            (source / "CMakeLists.txt").write_text(
                "cmake_minimum_required(VERSION 3.20)\n"
                "project(MainlineCompilerRegression NONE)\n" + body,
                encoding="utf-8",
            )
            build = root / "build"
            result = subprocess.run(
                [
                    shutil.which("cmake"), "-G", "Ninja",
                    "-S", str(source), "-B", str(build),
                    f"-DCMAKE_BUILD_TYPE={build_type}",
                    *(f"-D{key}={value}" for key, value in variables.items()),
                ],
                capture_output=True,
                text=True,
            )
            self.assertEqual(result.returncode, 0, msg=result.stdout + result.stderr)
            return {
                path.stem: path.read_text(encoding="utf-8").split(";")
                for path in build.glob("*.report")
            }

    def compiler_options(self, processor: str, build_type: str, avx2: str,
                         msvc: str, qnap: str = "OFF") -> dict[str, list[str]]:
        return self.configure(
            # project() initializes native system variables even with NONE;
            # override the target afterwards to exercise cross-target policies.
            f'set(CMAKE_SYSTEM_PROCESSOR "{processor}")\n'
            # Deliberately disagree with the target: host/platform-derived state
            # must not decide whether x86-only flags are emitted.
            f'set(CMAKE_HOST_SYSTEM_PROCESSOR {"aarch64" if processor in X64_PROCESSORS else "AMD64"})\n'
            f'set(THEMIS_ARCH {"aarch64" if processor in X64_PROCESSORS else "x86_64"})\n'
            f'include("{COMPILER_OPTIONS.as_posix()}")\n'
            'get_directory_property(options COMPILE_OPTIONS)\n'
            'get_directory_property(definitions COMPILE_DEFINITIONS)\n'
            'file(GENERATE OUTPUT "${CMAKE_BINARY_DIR}/options.report" CONTENT "${options}")\n'
            'file(GENERATE OUTPUT "${CMAKE_BINARY_DIR}/definitions.report" CONTENT "${definitions}")\n',
            build_type,
            THEMIS_ENABLE_AVX2=avx2,
            MSVC=msvc,
            THEMIS_QNAP_BUILD=qnap,
            # Hardening probes require an enabled native compiler and are
            # independent of the target-architecture flag selection under test.
            THEMIS_DISABLE_SECURITY_HARDENING="ON",
        )

    def assert_simd(self, report: dict[str, list[str]], enabled: bool, msvc: str) -> None:
        options = report["options"]
        self.assertEqual("THEMIS_HAS_AVX2=1" in report["definitions"], enabled)
        for flag in ("/arch:AVX2", "-mavx2", "-mfma"):
            expected = enabled and ((flag == "/arch:AVX2") == (msvc == "ON"))
            self.assertEqual(flag in options, expected, msg=f"{flag}: {options}")

    def test_x64_avx2_requires_release_and_opt_in(self) -> None:
        for processor, build_type, avx2, msvc in itertools.product(
            X64_PROCESSORS, ("Release", "Debug"), ("ON", "OFF"), ("ON", "OFF")
        ):
            with self.subTest(processor=processor, build_type=build_type, avx2=avx2, msvc=msvc):
                self.assert_simd(
                    self.compiler_options(processor, build_type, avx2, msvc),
                    build_type == "Release" and avx2 == "ON", msvc,
                )

    def test_arm64_never_gets_x86_simd(self) -> None:
        for processor, build_type, avx2, msvc in itertools.product(
            ARM_PROCESSORS, ("Release", "Debug"), ("ON", "OFF"), ("ON", "OFF")
        ):
            with self.subTest(processor=processor, build_type=build_type, avx2=avx2, msvc=msvc):
                report = self.compiler_options(processor, build_type, avx2, msvc)
                self.assert_simd(report, False, msvc)
                self.assertNotIn("-march=x86-64", report["options"])
                self.assertNotIn("THEMIS_BASELINE_X64=1", report["definitions"])

    def test_qnap_baseline_is_x64_only_and_does_not_override_avx2(self) -> None:
        for processor, build_type, avx2 in itertools.product(
            X64_PROCESSORS + ARM_PROCESSORS, ("Release", "Debug"), ("ON", "OFF")
        ):
            with self.subTest(processor=processor, build_type=build_type, avx2=avx2):
                report = self.compiler_options(processor, build_type, avx2, "OFF", qnap="ON")
                simd = processor in X64_PROCESSORS and build_type == "Release" and avx2 == "ON"
                baseline = processor in X64_PROCESSORS and not simd
                self.assert_simd(report, simd, "OFF")
                self.assertEqual("-march=x86-64" in report["options"], baseline)
                self.assertEqual("THEMIS_BASELINE_X64=1" in report["definitions"], baseline)

    def handler_options(self, build_type: str, low_memory: str | None = None) -> dict[str, list[str]]:
        # Execute the existing policy verbatim; avoid the dependency-heavy
        # themis_build_modular function and do not duplicate its generator expression.
        policy = re.search(
            r"(?m)^        option\(THEMIS_MSVC_HEAVY_HANDLER_LOW_MEMORY\b"
            r".*?^        set_source_files_properties\(\s*"
            r"\$\{CMAKE_SOURCE_DIR\}/src/observability/distributed_flame_graph\.cpp",
            MODULAR_BUILD.read_text(encoding="utf-8"),
            re.DOTALL | re.MULTILINE,
        )
        self.assertIsNotNone(policy, "heavy-handler policy must remain discoverable")
        body = "add_custom_target(themis_network)\n"
        body += policy.group(0).rsplit("        set_source_files_properties(", 1)[0]
        for handler in HANDLERS:
            body += (
                f'get_source_file_property(options "${{CMAKE_SOURCE_DIR}}/src/server/{handler}.cpp" COMPILE_OPTIONS)\n'
                f'get_source_file_property(unity "${{CMAKE_SOURCE_DIR}}/src/server/{handler}.cpp" SKIP_UNITY_BUILD_INCLUSION)\n'
                f'file(GENERATE OUTPUT "${{CMAKE_BINARY_DIR}}/{handler}.report" '
                'CONTENT "$<GENEX_EVAL:${options}>;${unity}")\n'
            )
        variables = {} if low_memory is None else {"THEMIS_MSVC_HEAVY_HANDLER_LOW_MEMORY": low_memory}
        return self.configure(body, build_type, **variables)

    def test_handlers_and_routes_default_to_low_memory_outside_debug(self) -> None:
        for build_type in ("Release", "Debug", "RelWithDebInfo", "MinSizeRel"):
            with self.subTest(build_type=build_type):
                report = self.handler_options(build_type)
                for handler in HANDLERS:
                    self.assertIn("/bigobj", report[handler], msg=handler)
                    self.assertIn("ON", report[handler], msg=f"{handler}: unity exclusion")
                    self.assertEqual("/Od" in report[handler], build_type != "Debug", msg=handler)
                    self.assertEqual("/Ob0" in report[handler], "/Od" in report[handler], msg=handler)

    def test_handler_low_memory_opt_out_retains_bigobj_and_unity_exclusion(self) -> None:
        for build_type in ("Release", "Debug", "RelWithDebInfo", "MinSizeRel"):
            with self.subTest(build_type=build_type):
                report = self.handler_options(build_type, low_memory="OFF")
                for handler in HANDLERS:
                    self.assertIn("/bigobj", report[handler], msg=handler)
                    self.assertIn("ON", report[handler], msg=handler)
                    self.assertNotIn("/Od", report[handler], msg=handler)
                    self.assertNotIn("/Ob0", report[handler], msg=handler)

    def test_monolithic_routes_retain_handler_compile_properties(self) -> None:
        policy = MONOLITHIC_BUILD.read_text(encoding="utf-8").split(
            "# Per-file compile options for heavy AQL parser translation units (MSVC-only)\n", 1
        )[1].split("\nendif()", 1)[0]
        body = "set(MSVC ON)\n" + policy + "\nendif()\n"
        for source in ("monitoring_api_handler", "monitoring_api_routes"):
            body += (
                f'get_source_file_property(options "../src/server/{source}.cpp" COMPILE_OPTIONS)\n'
                f'get_source_file_property(unity "../src/server/{source}.cpp" SKIP_UNITY_BUILD_INCLUSION)\n'
                f'file(GENERATE OUTPUT "${{CMAKE_BINARY_DIR}}/{source}.report" '
                'CONTENT "$<GENEX_EVAL:${options}>;${unity}")\n'
            )
        report = self.configure(body, "Release")
        self.assertEqual(report["monitoring_api_routes"], ["/bigobj", "ON"])
        self.assertEqual(report["monitoring_api_routes"], report["monitoring_api_handler"])


class MonitoringRouteTranslationUnitTests(unittest.TestCase):
    def test_registration_batches_bound_per_function_initializer_count(self) -> None:
        source = (REPO_ROOT / "src/server/monitoring_api_routes.cpp").read_text(encoding="utf-8")
        helpers = re.findall(
            r"^void (register\w+Routes)\(RouteRegistry& reg\) \{\n(.*?)^\}",
            source, re.MULTILINE | re.DOTALL,
        )
        self.assertTrue(helpers, "route initializers must be split into file-local functions")
        counts = []
        for name, body in helpers:
            with self.subTest(helper=name):
                count = body.count("reg.registerRoute(")
                self.assertGreater(count, 0)
                self.assertLessEqual(count, 10, "large initializer batches exhaust MSVC's heap")
                counts.append(count)
        self.assertEqual(sum(counts), 47)
        dispatcher = source.split("void MonitoringApiHandler::registerRoutes() {", 1)[1]
        self.assertNotIn("reg.registerRoute(", dispatcher)
        self.assertEqual(
            re.findall(r"\b(register\w+Routes)\(reg\);", dispatcher),
            [name for name, _ in helpers],
            "each batch must run exactly once, in the original registration order",
        )
        self.assertIn("namespace {\n", source)
        self.assertLess(source.index("} // namespace\n"), source.index("void MonitoringApiHandler::"))

    def test_routes_registered_next_to_handler_in_both_source_lists(self) -> None:
        for cmake_file, list_name in (
            (MODULAR_BUILD, "THEMIS_NETWORK_SOURCES"),
            (MONOLITHIC_BUILD, "THEMIS_CORE_SOURCES"),
        ):
            with self.subTest(cmake_file=cmake_file):
                source_list = re.search(
                    rf"\bset\({list_name}\s+(.*?)\n\s*\)",
                    cmake_file.read_text(encoding="utf-8"), re.DOTALL,
                )
                self.assertIsNotNone(source_list)
                self.assertEqual(source_list.group(1).count("../src/server/monitoring_api_routes.cpp"), 1)
                self.assertRegex(
                    source_list.group(1),
                    r"\.\./src/server/monitoring_api_handler\.cpp\s+"
                    r"\.\./src/server/monitoring_api_routes\.cpp\b",
                )

    def test_register_routes_defined_only_in_routes_tu_with_all_47_routes(self) -> None:
        server = REPO_ROOT / "src" / "server"
        definition = re.compile(r"\bvoid\s+MonitoringApiHandler::registerRoutes\s*\(\s*\)\s*\{")
        definitions = []
        for source in server.glob("*.cpp"):
            definitions.extend([source.name] * len(definition.findall(source.read_text(encoding="utf-8"))))
        self.assertEqual(definitions, ["monitoring_api_routes.cpp"])
        routes_source = (server / "monitoring_api_routes.cpp").read_text(encoding="utf-8")
        routes = re.findall(r'reg\.registerRoute\(\{\s*"([^"]+)",\s*"([^"]+)"', routes_source)
        expected = {
            ("/health", "get"),
            ("/health/live", "get"),
            ("/health/ready", "get"),
            ("/version", "get"),
            ("/stats", "get"),
            ("/api/capabilities", "get"),
            ("/metrics", "get"),
            ("/metrics/html", "get"),
            ("/api/v1/observability/alerts", "get"),
            ("/api/v1/observability/alerts/{id}/silence", "post"),
            ("/api/v1/observability/health", "get"),
            ("/api/v1/observability/provenance", "get"),
            ("/api/v1/license/status", "get"),
            ("/api/openapi.json", "get"),
            ("/entities", "get"),
            ("/entities", "post"),
            ("/entities/{key}", "get"),
            ("/entities/{key}", "put"),
            ("/entities/{key}", "delete"),
            ("/query", "post"),
            ("/query/aql", "post"),
            ("/api/v1/query/udfs", "post"),
            ("/api/v1/query/udfs", "get"),
            ("/api/v1/query/udfs/{name}", "get"),
            ("/api/v1/query/udfs/{name}", "delete"),
            ("/changefeed/compact", "post"),
            ("/graph/query/incremental", "post"),
            ("/graph/query/incremental/{handle}", "delete"),
            ("/graph/changes", "post"),
            ("/v2/query/stream", "get"),
            ("/api/v1/functions", "post"),
            ("/api/v1/functions", "get"),
            ("/api/v1/functions/{id}", "get"),
            ("/api/v1/functions/{id}", "put"),
            ("/api/v1/functions/{id}", "delete"),
            ("/api/v1/functions/{id}/invoke", "post"),
            ("/api/v1/functions/{id}/versions", "get"),
            ("/api/tasks", "post"),
            ("/api/tasks", "get"),
            ("/api/tasks/stats", "get"),
            ("/api/tasks/{id}", "get"),
            ("/api/tasks/{id}", "put"),
            ("/api/tasks/{id}", "delete"),
            ("/api/tasks/{id}/enable", "post"),
            ("/api/tasks/{id}/disable", "post"),
            ("/api/tasks/{id}/execute", "post"),
            ("/ui/tasks", "get"),
        }
        self.assertEqual(len(routes), 47)
        self.assertEqual(set(routes), expected)
        self.assertEqual(
            re.findall(r'^#include\s+"([^"]+)"', routes_source, re.MULTILINE),
            ["server/monitoring_api_handler.h", "server/openapi_route_registry.h"],
        )
        handler_source = (server / "monitoring_api_handler.cpp").read_text(encoding="utf-8")
        self.assertIn('#include "server/openapi_route_registry.h"', handler_source)
        self.assertIn("RouteRegistry::instance().buildOpenApiSpec", handler_source)


class MainlineWindowsMatrixTests(unittest.TestCase):
    def test_windows_release_build_is_serial_with_independent_configure_concurrency(self) -> None:
        workflow = WORKFLOW.read_text(encoding="utf-8")
        script = workflow.split("python3 - <<'PY' >> \"$GITHUB_OUTPUT\"\n", 1)[1]
        script = textwrap.dedent(script.split("\n          PY", 1)[0])
        env = os.environ.copy()
        env.pop("WINDOWS_DEPENDENCY_CONCURRENCY", None)
        env.update(
            BUILD_MATRIX="windows-amd64-only", OS_SELECTOR="windows", ARCH_SELECTOR="amd64",
            EDITION="community", BUILD_TYPE="Release", PACKAGING="auto",
            DISTRIBUTION="github", RUN_NUMBER="1",
        )
        result = subprocess.run([sys.executable, "-c", script], env=env, capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, msg=result.stdout + result.stderr)
        matrix_line = next(line for line in result.stdout.splitlines() if line.startswith("matrix="))
        entries = json.loads(matrix_line.split("=", 1)[1])["include"]
        self.assertEqual([entry["id"] for entry in entries], ["windows-amd64"])
        entry = entries[0]
        for key, concurrency in (("configure_command", "4"), ("build_command", "1")):
            with self.subTest(command=key):
                command = entry[key]
                self.assertEqual(re.findall(r"export VCPKG_MAX_CONCURRENCY=(\d+)", command), [concurrency])
                self.assertEqual(re.findall(r"export CMAKE_BUILD_PARALLEL_LEVEL=(\d+)", command), [concurrency])
        self.assertRegex(entry["configure_command"], r"cmake --preset windows-release\b")
        self.assertRegex(entry["build_command"], r"cmake --build --preset windows-release --parallel 1\b")
        self.assertNotRegex(entry["build_command"], r"--parallel\s+[2-9]\d*")


if __name__ == "__main__":
    unittest.main()
