#!/usr/bin/env python3

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
BUILD_MAINLINE_WORKFLOW = REPO_ROOT / ".github" / "workflows" / "build-mainline.yml"
RELEASE_BUILD_MATRIX_WORKFLOW = REPO_ROOT / ".github" / "workflows" / "release-build-matrix.yml"
RELEASE_MAINLINE_WORKFLOW = REPO_ROOT / ".github" / "workflows" / "release-mainline.yml"
RELEASE_NIGHTLY_WORKFLOW = REPO_ROOT / ".github" / "workflows" / "release-nightly.yml"
REUSABLE_CMAKE_WORKFLOW = REPO_ROOT / ".github" / "workflows" / "reusable-cmake-build.yml"
MAINLINE_WORKFLOW = REPO_ROOT / ".github" / "workflows" / "build-mainline.yml"
MAINLINE_VALIDATION_GATES = REPO_ROOT / ".github" / "workflows" / "reusable-mainline-validation-gates.yml"
WORDPRESS_PRESS_WORKFLOW = REPO_ROOT / ".github" / "workflows" / "release-wordpress-press.yml"
TESTS_CMAKELISTS = REPO_ROOT / "tests" / "CMakeLists.txt"
SHARDING_CMAKELISTS = REPO_ROOT / "tests" / "sharding" / "CMakeLists.txt"
DOCS_ROCKSDB_GENERATOR = REPO_ROOT / "scripts" / "generate_docs_rocksdb.py"


def extract_yaml_job_block(text: str, job_id: str) -> str:
    lines = text.splitlines()
    anchor = f"  {job_id}:"

    for index, line in enumerate(lines):
        if line.rstrip() == anchor:
            block_lines = [line]
            for candidate in lines[index + 1 :]:
                stripped_candidate = candidate.strip()
                if stripped_candidate and candidate.startswith("  ") and not candidate.startswith("    "):
                    break
                if stripped_candidate and not candidate.startswith("  "):
                    break
                block_lines.append(candidate)
            return "\n".join(block_lines)

    raise AssertionError(f"Could not find YAML job block for {job_id!r}")


def extract_cmake_if_block(text: str, anchor: str) -> str:
    lines = text.splitlines()

    for index, line in enumerate(lines):
        if line.strip() == anchor:
            depth = 0
            block_lines = []
            for candidate in lines[index:]:
                stripped = candidate.strip()
                normalized = stripped.lower()
                opens_if = re.match(r"if\s*\(", normalized) is not None
                closes_if = re.match(r"endif\s*\(", normalized) is not None

                if opens_if:
                    depth += 1
                elif closes_if and depth == 0:
                    raise AssertionError(f"Encountered unmatched endif while parsing {anchor!r}")

                if depth > 0:
                    block_lines.append(candidate)

                if closes_if:
                    depth -= 1
                    if depth == 0:
                        return "\n".join(block_lines)
            break

    raise AssertionError(f"Could not find balanced CMake block for {anchor!r}")


def extract_submodule_paths(command_suffix: str) -> list[str]:
    return re.split(r"\s*(?:\|\||&&|;)\s*", command_suffix, maxsplit=1)[0].strip().split()


def extract_release_matrix_lane_slice(workflow_text: str, lane_id: str) -> str:
    lane_match = re.search(
        rf'\{{\s*"id": "{re.escape(lane_id)}",.*?(?=\{{\s*"id": "|$)',
        workflow_text,
        re.DOTALL,
    )
    if lane_match is None:
        raise AssertionError(f"Could not find release lane {lane_id!r}")
    return lane_match.group(0)


class PreflightReleasePolicyRegressionTests(unittest.TestCase):
    @unittest.skipUnless(
        os.name == "posix" and all(shutil.which(tool) for tool in ("bash", "cmake", "ninja")),
        "requires Bash, CMake and Ninja",
    )
    def test_linux_ninja_setup_precedes_shadowing_ninja_on_path(self) -> None:
        workflow_text = REUSABLE_CMAKE_WORKFLOW.read_text(encoding="utf-8")
        step = workflow_text.split("      - name: Ensure Ninja binary is discoverable (Linux)\n", 1)[1]
        script = textwrap.dedent(step.split("        run: |\n", 1)[1].split("\n      - name:", 1)[0])

        with tempfile.TemporaryDirectory() as temporary_directory:
            root = Path(temporary_directory)
            system_bin = root / "system"
            shadow_bin = root / "shadow"
            system_bin.mkdir()
            shadow_bin.mkdir()
            ninja = system_bin / "ninja"
            ninja.symlink_to(shutil.which("ninja"))
            shadow_ninja = shadow_bin / "ninja"
            shadow_ninja.write_text(
                '#!/bin/sh\nif [ "$1" = "--version" ]; then echo 1.11.1; exit 0; fi\nexit 1\n',
                encoding="utf-8",
            )
            shadow_ninja.chmod(0o755)
            github_env = root / "github_env"
            github_path = root / "github_path"
            github_env.touch()
            github_path.touch()
            environment = {
                **os.environ,
                "PATH": f"{shadow_bin}{os.pathsep}{os.environ['PATH']}",
                "GITHUB_ENV": str(github_env),
                "GITHUB_PATH": str(github_path),
                "RUNNER_TEMP": str(root),
            }
            subprocess.run(
                ["bash", "-e", "-o", "pipefail", "-c", script.replace("/usr/bin", str(system_bin))],
                env=environment,
                check=True,
                capture_output=True,
                text=True,
            )
            added_paths = github_path.read_text(encoding="utf-8").splitlines()
            ninja_dir = system_bin
            self.assertEqual(added_paths, [str(ninja_dir)])
            self.assertEqual(list(ninja_dir.iterdir()), [ninja_dir / "ninja"])
            self.assertEqual((ninja_dir / "ninja").resolve(), ninja.resolve())
            environment["PATH"] = os.pathsep.join([*reversed(added_paths), environment["PATH"]])
            # CMake and vcpkg must discover Ninja through PATH, not a shell-only cache variable.
            environment.pop("CMAKE_MAKE_PROGRAM", None)
            source = root / "source"
            source.mkdir()
            (source / "CMakeLists.txt").write_text(
                "cmake_minimum_required(VERSION 3.23)\nproject(NinjaDiscovery NONE)\n",
                encoding="utf-8",
            )
            build = root / "build"
            subprocess.run(
                ["cmake", "-S", str(source), "-B", str(build), "-G", "Ninja"],
                env=environment,
                check=True,
                capture_output=True,
                text=True,
            )
            self.assertIn(
                f"CMAKE_MAKE_PROGRAM:FILEPATH={ninja_dir / 'ninja'}",
                (build / "CMakeCache.txt").read_text(encoding="utf-8"),
            )
            subprocess.run(
                ["cmake", "--build", str(build)],
                env=environment,
                check=True,
                capture_output=True,
                text=True,
            )

    def test_release_build_matrix_submodule_sync_only_references_declared_paths(self) -> None:
        workflow_text = RELEASE_BUILD_MATRIX_WORKFLOW.read_text(encoding="utf-8")
        gitmodules_text = (REPO_ROOT / ".gitmodules").read_text(encoding="utf-8")
        declared_paths = set(re.findall(r"^\s*path = (.+)$", gitmodules_text, re.MULTILINE))

        for job_id in ("linux-release", "windows-release"):
            job_block = extract_yaml_job_block(workflow_text, job_id)
            sync_prefix = "git submodule sync -- "
            update_prefix = "git submodule update --init --depth 1 "
            sync_matches = [
                extract_submodule_paths(line.split(sync_prefix, 1)[1])
                for line in job_block.splitlines()
                if sync_prefix in line
            ]
            update_matches = [
                extract_submodule_paths(line.split(update_prefix, 1)[1])
                for line in job_block.splitlines()
                if update_prefix in line
            ]

            self.assertEqual(len(sync_matches), 1, msg=f"expected one sync command for {job_id}")
            self.assertEqual(len(update_matches), 1, msg=f"expected one update command for {job_id}")

            sync_paths = sync_matches[0]
            update_paths = update_matches[0]

            self.assertTrue(sync_paths)
            self.assertTrue(update_paths)
            self.assertEqual(set(sync_paths), set(update_paths))
            for command_name, command_paths in (("sync", sync_paths), ("update", update_paths)):
                self.assertTrue(
                    set(command_paths).issubset(declared_paths),
                    msg=(
                        f"{job_id} {command_name} command includes undeclared submodule paths "
                        "(for example the stale llama.cpp entry)"
                    ),
                )

    def test_macos_kqueue_lane_installs_googletest(self) -> None:
        workflow_text = MAINLINE_VALIDATION_GATES.read_text(encoding="utf-8")
        macos_kqueue_job = extract_yaml_job_block(workflow_text, "macos-kqueue")

        self.assertRegex(
            macos_kqueue_job,
            re.compile(r"brew install[^\n]*\bgoogletest\b"),
        )

    def test_macos_kqueue_lane_installs_cpp_httplib(self) -> None:
        workflow_text = MAINLINE_VALIDATION_GATES.read_text(encoding="utf-8")
        macos_kqueue_job = extract_yaml_job_block(workflow_text, "macos-kqueue")

        self.assertRegex(
            macos_kqueue_job,
            re.compile(r"brew install[^\n]*\bcpp-httplib\b"),
        )

    def test_macos_kqueue_lane_uses_modular_test_targets(self) -> None:
        workflow_text = MAINLINE_VALIDATION_GATES.read_text(encoding="utf-8")
        macos_kqueue_job = extract_yaml_job_block(workflow_text, "macos-kqueue")

        for target in (
            "module_config_test_config_file_watcher_focused",
            "module_plugins_test_plugin_hot_plug_focused",
            "module_plugins_test_plugin_hot_reload_enhanced_focused",
        ):
            with self.subTest(target=target):
                self.assertIn(f"build_target {target}", macos_kqueue_job)
        for test_name in (
            "test_config_file_watcher_config_FocusedTests",
            "test_plugin_hot_plug_plugins_FocusedTests",
            "test_plugin_hot_reload_enhanced_plugins_FocusedTests",
        ):
            with self.subTest(test_name=test_name):
                self.assertIn(test_name, macos_kqueue_job)

    @unittest.skipUnless(shutil.which("cmake") and shutil.which("ninja"), "requires CMake and Ninja")
    def test_macos_kqueue_targets_are_registered_by_module_discovery(self) -> None:
        root_cmake = TESTS_CMAKELISTS.read_text(encoding="utf-8")
        discovery = root_cmake.split("# Auto-discover modularized test folders", 1)[1].split("\n", 1)[1].split(
            "set(THEMIS_EPIC3_DISTRIBUTED_TENSOR_TESTS_INCLUDED", 1
        )[0]
        expected = {
            "test_config_file_watcher_config_FocusedTests": "module_config_test_config_file_watcher_focused",
            "test_plugin_hot_plug_plugins_FocusedTests": "module_plugins_test_plugin_hot_plug_focused",
            "test_plugin_hot_reload_enhanced_plugins_FocusedTests": "module_plugins_test_plugin_hot_reload_enhanced_focused",
        }
        with tempfile.TemporaryDirectory() as temp:
            source = Path(temp)
            for module in ("config", "plugins"):
                (source / module).symlink_to(REPO_ROOT / "tests" / module, target_is_directory=True)
            (source / "CMakeLists.txt").write_text(
                'cmake_minimum_required(VERSION 3.20)\n'
                'project(KqueueRegistration LANGUAGES CXX)\n'
                'enable_testing()\n'
                f'set(THEMIS_ROOT_DIR "{REPO_ROOT.as_posix()}")\n'
                'add_library(themis_core INTERFACE)\n'
                'add_library(spdlog::spdlog INTERFACE IMPORTED)\n'
                'add_library(Threads::Threads INTERFACE IMPORTED)\n'
                'include("${THEMIS_ROOT_DIR}/tests/cmake/RegisterModuleTests.cmake")\n'
                + discovery
            )
            build = source / "build"
            for modular in ("ON", "OFF", "ON"):
                configure = subprocess.run(
                    ["cmake", "-S", str(source), "-B", str(build), "-G", "Ninja",
                     f"-DTHEMIS_BUILD_MODULAR={modular}"],
                    capture_output=True, text=True,
                )
                self.assertEqual(configure.returncode, 0, configure.stdout + configure.stderr)
                listing = subprocess.run(
                    ["ctest", "--test-dir", str(build), "--show-only=json-v1"],
                    capture_output=True, text=True, check=True,
                )
                names = {test["name"] for test in json.loads(listing.stdout)["tests"]}
                self.assertEqual("test_config_file_watcher_config_FocusedTests" in names, modular == "ON")
            targets = subprocess.run(
                ["cmake", "--build", str(build), "--target", "help"],
                capture_output=True, text=True, check=True,
            ).stdout
            listing = subprocess.run(
                ["ctest", "--test-dir", str(build), "--show-only=json-v1"],
                capture_output=True, text=True, check=True,
            )
            tests = {test["name"]: test for test in json.loads(listing.stdout)["tests"]}
            for name, target in expected.items():
                with self.subTest(name=name):
                    self.assertIn(f"{target}:", targets)
                    self.assertIn(name, tests)
                    self.assertIn(f"-DTARGET_NAME={target}", tests[name]["command"])

    def test_macos_kqueue_lane_retries_without_sccache_on_backend_failure(self) -> None:
        workflow_text = MAINLINE_VALIDATION_GATES.read_text(encoding="utf-8")
        macos_kqueue_job = extract_yaml_job_block(workflow_text, "macos-kqueue")
        helper_match = re.search(
            r"disable_sccache_and_reconfigure\(\) \{(?P<body>.*?)\n\s+\}",
            macos_kqueue_job,
            re.DOTALL,
        )

        self.assertIn("SCCACHE_FALLBACK_APPLIED=0", macos_kqueue_job)
        self.assertIn(
            "grep -Eq 'sccache: error: Server startup failed|cache storage failed to read' \"${build_log}\"",
            macos_kqueue_job,
        )
        self.assertIsNotNone(helper_match)
        helper_body = helper_match.group("body")
        self.assertIn('cmake -S . -B build-macos \\', helper_body)
        self.assertIn("-DCMAKE_C_COMPILER_LAUNCHER=", helper_body)
        self.assertIn("-DCMAKE_CXX_COMPILER_LAUNCHER=", helper_body)

    def test_ai_safety_chaos_links_themis_llm_when_available(self) -> None:
        cmake_text = TESTS_CMAKELISTS.read_text(encoding="utf-8")
        ai_safety_chaos_block = extract_cmake_if_block(
            cmake_text,
            'if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/security/ai_safety/test_ai_safety_chaos.cpp")',
        )

        self.assertIn("if(TARGET themis_llm)", ai_safety_chaos_block)
        self.assertIn(
            "target_link_libraries(test_ai_safety_chaos PRIVATE themis_llm)",
            ai_safety_chaos_block,
        )

    def test_docs_importer_uses_version_safe_rocksdb_open(self) -> None:
        generator_text = DOCS_ROCKSDB_GENERATOR.read_text(encoding="utf-8")

        self.assertIn("std::unique_ptr<DB> db;", generator_text)
        self.assertIn("#if defined(ROCKSDB_MAJOR) && ROCKSDB_MAJOR >= 10", generator_text)
        self.assertIn("Status status = DB::Open(options, db_path, &db);", generator_text)
        self.assertIn("DB* db_raw = nullptr;", generator_text)
        self.assertIn("Status status = DB::Open(options, db_path, &db_raw);", generator_text)
        self.assertIn("db.reset(db_raw);", generator_text)

    def test_release_build_matrix_treats_missing_package_artifacts_as_error(self) -> None:
        workflow_text = RELEASE_BUILD_MATRIX_WORKFLOW.read_text(encoding="utf-8")
        linux_amd64_lane = extract_release_matrix_lane_slice(workflow_text, "linux-amd64")
        linux_arm64_lane = extract_release_matrix_lane_slice(workflow_text, "linux-arm64")
        windows_amd64_lane = extract_release_matrix_lane_slice(workflow_text, "windows-amd64")

        linux_error = "::error::No Linux package artifacts were produced in ${PACKAGES_DIR}"
        windows_error = "::error::No Windows package artifacts were produced in ${PACKAGES_DIR}"

        self.assertIn(linux_error, linux_amd64_lane)
        self.assertIn(linux_error, linux_arm64_lane)
        self.assertIn(windows_error, windows_amd64_lane)
        self.assertNotIn(windows_error, linux_amd64_lane)
        self.assertNotIn(windows_error, linux_arm64_lane)
        self.assertNotIn(linux_error, windows_amd64_lane)
        self.assertNotIn("::warning::No Linux package artifacts were produced", workflow_text)
        self.assertNotIn("::warning::No Windows package artifacts were produced", workflow_text)

    def test_release_matrix_runtime_gate_skips_missing_lane_artifacts(self) -> None:
        workflow_text = RELEASE_BUILD_MATRIX_WORKFLOW.read_text(encoding="utf-8")
        runtime_gate = extract_yaml_job_block(workflow_text, "runtime-artifact-gate")
        run_summary = extract_yaml_job_block(workflow_text, "run-summary")

        self.assertIn("if: ${{ always() && !cancelled() && needs.prepare-release-matrix.result == 'success' }}", runtime_gate)
        self.assertIn("actions/github-script@", runtime_gate)
        self.assertIn("artifact.name === process.env.ARTIFACT_NAME", runtime_gate)
        self.assertIn("steps.package_artifact.outputs.found == 'true'", runtime_gate)
        self.assertIn("No package artifact was produced for ${PLATFORM}", runtime_gate)
        self.assertIn("steps.packages.outputs.available == 'true'", runtime_gate)
        self.assertIn("runtime_gate_result: ${{ steps.release_gate.outputs.runtime_gate_result }}", run_summary)
        self.assertIn("RUNTIME_GATE_RESULT: ${{ needs.runtime-artifact-gate.result }}", run_summary)

    def test_mainline_publishes_validated_platform_artifacts_after_upstream_failure(self) -> None:
        workflow_text = RELEASE_MAINLINE_WORKFLOW.read_text(encoding="utf-8")
        pending_job = extract_yaml_job_block(workflow_text, "mark-release-pending")
        preflight_job = extract_yaml_job_block(workflow_text, "preflight-release-policy")
        package_job = extract_yaml_job_block(workflow_text, "package")

        for job_block in (pending_job, preflight_job):
            self.assertIn("github.event.workflow_run.conclusion == 'failure'", job_block)
            self.assertIn("github.event.workflow_run.event == 'schedule'", job_block)
        self.assertIn("needs.build-matrix.result == 'failure'", package_job)
        self.assertIn("needs.build-matrix.outputs.runtime_gate_result == 'success'", package_job)
        self.assertIn("!cancelled()", package_job)

    def test_nightly_publishes_partial_platform_packages_but_keeps_docker_multiarch_gate(self) -> None:
        workflow_text = RELEASE_NIGHTLY_WORKFLOW.read_text(encoding="utf-8")
        change_job = extract_yaml_job_block(workflow_text, "check-changes")
        metadata_job = extract_yaml_job_block(workflow_text, "generate-metadata")
        release_job = extract_yaml_job_block(workflow_text, "publish-release")
        docker_manifest_job = extract_yaml_job_block(workflow_text, "publish-docker-manifest")

        self.assertIn("github.event.workflow_run.conclusion == 'failure'", change_job)
        for job_block in (metadata_job, release_job):
            self.assertIn("needs.build-matrix.result == 'failure'", job_block)
            self.assertIn("needs.build-matrix.outputs.runtime_gate_result == 'success'", job_block)
        self.assertIn("actions/github-script@", release_job)
        self.assertIn("artifact.name.startsWith('packages-')", release_job)
        self.assertIn("No successful OS lane produced a distributable package.", release_job)
        self.assertIn("needs: [generate-metadata, publish-docker]", docker_manifest_job)
        self.assertIn("for arch in amd64 arm64", docker_manifest_job)

    def test_release_build_matrix_community_only_excludes_experimental_macos_lane(self) -> None:
        workflow_text = RELEASE_BUILD_MATRIX_WORKFLOW.read_text(encoding="utf-8")

        self.assertIn(
            '"community-only": {"linux-amd64", "linux-arm64", "windows-amd64"}',
            workflow_text,
        )
        self.assertNotIn(
            '"macos-arm64"',
            workflow_text.split('"community-only":', 1)[1].split("}", 1)[0],
        )

    def test_release_build_matrix_macos_arm64_disables_sccache_github_backend(self) -> None:
        workflow_text = RELEASE_BUILD_MATRIX_WORKFLOW.read_text(encoding="utf-8")
        lane_block = extract_release_matrix_lane_slice(workflow_text, "macos-arm64")

        self.assertIn('export SCCACHE_GHA_ENABLED=false', lane_block)
        self.assertIn('sccache --stop-server || true', lane_block)
        self.assertIn('SCCACHE_GHA_ENABLED=false', lane_block)
        self.assertIn('if command -v sccache >/dev/null 2>&1 && [ \\\"${SCCACHE_GHA_ENABLED:-true}\\\" != \\\"false\\\" ]; then', lane_block)

    def test_mainline_delegates_ctest_and_benchmarks_to_reusable_gates(self) -> None:
        mainline = MAINLINE_WORKFLOW.read_text(encoding="utf-8")
        gates = MAINLINE_VALIDATION_GATES.read_text(encoding="utf-8")
        release_matrix = RELEASE_BUILD_MATRIX_WORKFLOW.read_text(encoding="utf-8")
        build_job = extract_yaml_job_block(mainline, "build")
        platform_matrix_job = extract_yaml_job_block(mainline, "step2-platform-matrix")

        self.assertIn("uses: ./.github/workflows/reusable-mainline-validation-gates.yml", mainline)
        self.assertIn("run_ctest: false", platform_matrix_job)
        self.assertIn("-DTHEMIS_BUILD_TESTS=OFF", build_job)
        self.assertIn("-DTHEMIS_BUILD_BENCHMARKS=OFF", build_job)
        self.assertNotRegex(mainline, re.compile(r"\bctest\s+--", re.IGNORECASE))
        self.assertNotIn("themis_benchmarks_all_eligible", mainline)
        self.assertIn("run_ctest:", release_matrix)
        self.assertIn('if not run_ctest:', release_matrix)
        self.assertIn('entry["dis10_serializability_test"] = False', release_matrix)
        self.assertIn('r"(?m)^ctest --preset [^\\n]*\\n"', release_matrix)
        self.assertIn("workflow_call:", gates)
        self.assertIn("uses: ./.github/workflows/reusable-cmake-build.yml", gates)
        self.assertIn("uses: ./.github/workflows/reusable-benchmark-runner.yml", gates)
        self.assertIn("ctest --test-dir build-macos", gates)

    def test_dis10_release_platform_test_links_zlib_and_registers_evidence_environment(self) -> None:
        sharding_cmake = SHARDING_CMAKELISTS.read_text(encoding="utf-8")
        reusable_workflow = REUSABLE_CMAKE_WORKFLOW.read_text(encoding="utf-8")

        target_block = sharding_cmake.split("add_executable(test_dis10_concurrent_serializability", 1)[1]
        target_block = target_block.split("if(EXISTS", 1)[0]
        link_block = target_block.split(
            "target_link_libraries(test_dis10_concurrent_serializability PRIVATE", 1
        )[1].split(")", 1)[0]
        self.assertIn("GTest::gtest GTest::gtest_main", link_block)
        self.assertNotIn("${TEST_LIBS}", link_block)
        self.assertNotIn("gmock", link_block.lower())
        self.assertIn("Threads::Threads ZLIB::ZLIB", target_block)
        self.assertIn("themis_link_zlib_if_available(test_dis10_concurrent_serializability)", target_block)
        self.assertIn("ENVIRONMENT \"${DIS10_SERIALIZABILITY_TEST_ENVIRONMENT}\"", target_block)
        self.assertIn("export THEMIS_DIS10_SERIALIZABILITY_EVIDENCE_FILE=", reusable_workflow)
        self.assertIn("THEMIS_CTEST_STRICT_EVIDENCE=1 THEMIS_CTEST_EMIT_TEST_OUTPUT=1", reusable_workflow)

    @unittest.skipUnless(
        sys.platform.startswith("linux") and all(shutil.which(tool) for tool in ("cmake", "ninja", "cc")),
        "requires Linux, CMake, Ninja and a C compiler",
    )
    def test_dis10_release_link_resolves_transitive_zlib(self) -> None:
        cmake_text = (REPO_ROOT / "cmake" / "CMakeLists.txt").read_text(encoding="utf-8")
        helper = "function(themis_link_zlib_if_available" + cmake_text.split(
            "function(themis_link_zlib_if_available", 1
        )[1].split("endfunction()", 1)[0] + "endfunction()\n"
        sharding_cmake = SHARDING_CMAKELISTS.read_text(encoding="utf-8")
        target = "add_executable(test_dis10_concurrent_serializability" + sharding_cmake.split(
            "add_executable(test_dis10_concurrent_serializability", 1
        )[1].split("target_compile_definitions", 1)[0]

        for empty_target in (True, False):
            with self.subTest(empty_target=empty_target), tempfile.TemporaryDirectory() as temp:
                root = Path(temp)
                (root / "dependency.c").write_text(
                    "extern unsigned long compressBound(unsigned long);\n"
                    "unsigned long dependency(void) { return compressBound(10); }\n",
                    encoding="utf-8",
                )
                (root / "test_dis10_concurrent_serializability.cpp").write_text(
                    'extern "C" unsigned long dependency(void);\n'
                    "int main() { return dependency() > 10 ? 0 : 1; }\n",
                    encoding="utf-8",
                )
                zlib = (
                    "add_library(ZLIB::ZLIB INTERFACE IMPORTED)\n"
                    if empty_target else "find_package(ZLIB REQUIRED)\n"
                )
                (root / "CMakeLists.txt").write_text(
                    "cmake_minimum_required(VERSION 3.20)\nproject(DIS10Link C CXX)\n"
                    + zlib + "find_package(Threads REQUIRED)\n"
                    + "add_library(GTest::gtest INTERFACE IMPORTED)\n"
                    + "add_library(GTest::gtest_main INTERFACE IMPORTED)\n"
                    + "add_library(themis_sharding SHARED dependency.c)\n"
                    + "set(THEMIS_BUILD_MODULAR ON)\n" + helper + target,
                    encoding="utf-8",
                )
                for command in (
                    ["cmake", "-S", str(root), "-B", str(root / "build"), "-G", "Ninja"],
                    ["cmake", "--build", str(root / "build"), "--target", "test_dis10_concurrent_serializability"],
                    [str(root / "build" / "test_dis10_concurrent_serializability")],
                ):
                    result = subprocess.run(command, capture_output=True, text=True)
                    self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def test_wordpress_press_dispatch_falls_back_to_synthetic_context(self) -> None:
        workflow_text = WORDPRESS_PRESS_WORKFLOW.read_text(encoding="utf-8")

        self.assertIn("async function buildSyntheticDispatchRelease(tagName)", workflow_text)
        self.assertIn("github.rest.git.getRef({", workflow_text)
        self.assertIn(
            "Dispatch tag ${dispatchTag} has no GitHub Release object; using synthetic context.",
            workflow_text,
        )
        self.assertIn("core.setOutput('has_release_object', hasReleaseObject ? 'true' : 'false');", workflow_text)

    def test_wordpress_press_live_publish_requires_real_release_object(self) -> None:
        workflow_text = WORDPRESS_PRESS_WORKFLOW.read_text(encoding="utf-8")

        self.assertIn(
            "if: steps.context.outputs.should_publish == 'true' && steps.context.outputs.has_release_object == 'true' && steps.dry-run.outputs.enabled != 'true'",
            workflow_text,
        )


if __name__ == "__main__":
    unittest.main()
