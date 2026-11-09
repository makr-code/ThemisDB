import contextlib
import importlib.util
import io
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import time
import unittest
from unittest.mock import Mock, patch

import yaml


ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location(
    "prepare_dependencies", ROOT / "scripts/prepare-windows-dependencies.py"
)
helper = importlib.util.module_from_spec(spec)
spec.loader.exec_module(helper)


class WindowsDependencyPreparationTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.pipeline = yaml.safe_load(
            (ROOT / ".github/workflows/reusable-cmake-build.yml").read_text()
        )
        cls.release = yaml.safe_load(
            (ROOT / ".github/workflows/release-build-matrix.yml").read_text()
        )
        cls.steps = cls.pipeline["jobs"]["build"]["steps"]

    def named(self, name):
        return next(step for step in self.steps if step["name"] == name)

    def plan(self, selector="all", edition="community", build_type="Release", concurrency="2"):
        command = self.release["jobs"]["prepare-release-matrix"]["steps"][0]["run"]
        result = subprocess.run(
            ["bash", "-euo", "pipefail", "-c", command.replace(' >> "$GITHUB_OUTPUT"', "")],
            env={**os.environ, "BUILD_MATRIX": selector, "OS_SELECTOR": "all",
                 "ARCH_SELECTOR": "all", "EDITION": edition, "BUILD_TYPE": build_type,
                 "PACKAGING": "auto", "DISTRIBUTION": "all", "RUN_NUMBER": "1",
                 "WINDOWS_DEPENDENCY_CONCURRENCY": concurrency},
            capture_output=True, text=True,
        )
        return result, dict(line.split("=", 1) for line in result.stdout.splitlines() if "=" in line)

    def test_timeout_kills_descendants_and_preserves_completed_archives(self):
        with tempfile.TemporaryDirectory(prefix="dependency space ") as temp:
            directory = Path(temp)
            marker = directory / "child-still-running"
            archive = directory / "completed.zip"
            child = f"import time,pathlib; time.sleep(0.8); pathlib.Path({str(marker)!r}).touch()"
            parent = (
                "import subprocess,sys,time,pathlib;"
                f"pathlib.Path({str(archive)!r}).write_bytes(b'completed');"
                f"subprocess.Popen([sys.executable,'-c',{child!r}]);"
                "time.sleep(30)"
            )
            started = time.monotonic()
            self.assertEqual(helper.run([sys.executable, "-c", parent], directory / "log", 0.3), 124)
            self.assertLess(time.monotonic() - started, 5)
            time.sleep(1)
            self.assertFalse(marker.exists(), "descendant survived controlled timeout")
            self.assertEqual(archive.read_bytes(), b"completed")

    def test_ordinary_install_error_is_not_hidden(self):
        with tempfile.TemporaryDirectory() as temp:
            log = Path(temp) / "log"
            self.assertEqual(helper.run(
                [sys.executable, "-c", "print('BUILD_FAILED',flush=True); raise SystemExit(37)"],
                log, 5,
            ), 37)
            self.assertIn("BUILD_FAILED", log.read_text())

    def test_exhausted_job_deadline_does_not_start_install(self):
        with tempfile.TemporaryDirectory() as temp:
            marker = Path(temp) / "started"
            self.assertEqual(helper.run(
                [sys.executable, "-c", f"open({str(marker)!r},'w').close()"],
                Path(temp) / "log", -1,
            ), 124)
            self.assertFalse(marker.exists())

    def test_windows_termination_uses_pid_tree_force_and_bounded_wait(self):
        process = Mock(pid=12345)
        process.poll.return_value = None
        with patch.object(helper.os, "name", "nt"), patch.object(helper.subprocess, "run") as kill:
            kill.return_value.returncode = 0
            helper.terminate_tree(process)
        kill.assert_called_once_with(
            ["taskkill.exe", "/PID", "12345", "/T", "/F"], timeout=60, check=False,
        )
        process.wait.assert_called_once_with(timeout=60)

    def test_windows_tree_termination_failure_propagates(self):
        process = Mock(pid=12345)
        process.poll.return_value = None
        with patch.object(helper.os, "name", "nt"), patch.object(helper.subprocess, "run") as kill:
            kill.return_value.returncode = 5
            with self.assertRaisesRegex(RuntimeError, "taskkill failed"):
                helper.terminate_tree(process)

    def test_main_bounds_runtime_by_both_job_deadline_and_subprocess_budget(self):
        for remaining, expected in [(60, 60), (100000, 270 * 60), (-1, -1)]:
            with self.subTest(remaining=remaining), patch.object(helper.time, "time", return_value=1000), \
                 patch.dict(os.environ, {"THEMIS_DEPENDENCY_DEADLINE": str(1000 + remaining),
                                         "THEMIS_DEPENDENCY_BASH": "/git path/bash.exe",
                                         "CONFIGURE_COMMAND": "exit 37"}), \
                 patch.object(sys, "argv", ["prepare", "--log", "test.log"]), \
                 patch.object(helper, "run", return_value=37) as run:
                self.assertEqual(helper.main(), 37)
                self.assertEqual(run.call_args.args[2], expected)
                self.assertEqual(run.call_args.args[0], ["/git path/bash.exe", "-euo", "pipefail", "-c", "exit 37"])

    def test_prepare_step_passes_selected_bash_to_python(self):
        step = self.named("Prepare Windows dependencies with bounded configure")
        with tempfile.TemporaryDirectory(prefix="git bash ") as temp:
            directory = Path(temp)
            bash = directory / "git bash"
            bash.symlink_to("/bin/bash")
            tools = directory / "tools"
            tools.mkdir()
            wrong_bash = tools / "bash"
            wrong_bash.write_text('#!/bin/sh\nexit 43\n')
            wrong_bash.chmod(0o755)
            cygpath = tools / "cygpath"
            cygpath.write_text('#!/bin/sh\nprintf "%s\\n" "$2"\n')
            cygpath.chmod(0o755)
            python = tools / "python"
            python.write_text(
                f'#!/bin/sh\n[ "$THEMIS_DEPENDENCY_BASH" = "$EXPECTED_BASH" ] || exit 42\n'
                f'exec "{sys.executable}" "$@"\n'
            )
            python.chmod(0o755)
            result = subprocess.run(
                [str(bash), "-euo", "pipefail", "-c",
                 step["run"].replace("${{ inputs.working_directory }}", str(ROOT))],
                env={**os.environ, "PATH": f"{tools}{os.pathsep}{os.environ['PATH']}",
                     "GITHUB_WORKSPACE": str(ROOT), "THEMIS_CACHE_TOOLS_PREFIX": "",
                     "RUNNER_TEMP": temp, "THEMIS_DEPENDENCY_DEADLINE": str(time.time() + 30),
                     "CONFIGURE_COMMAND": "printf 'configure ran\\n'", "EXPECTED_BASH": str(bash)},
                text=True, capture_output=True,
            )
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertIn("configure ran", result.stdout)

    def test_configure_requires_explicit_bash_without_path_fallback(self):
        for bash in [None, ""]:
            with self.subTest(bash=bash), patch.dict(os.environ, {}, clear=True), \
                 patch.object(sys, "argv", ["prepare", "--log", "test.log"]), \
                 patch.object(helper, "run") as run, \
                 contextlib.redirect_stderr(io.StringIO()) as error:
                if bash is not None:
                    os.environ["THEMIS_DEPENDENCY_BASH"] = bash
                with self.assertRaises(SystemExit) as exit:
                    helper.main()
                self.assertEqual(exit.exception.code, 2)
                self.assertIn("configure requires THEMIS_DEPENDENCY_BASH", error.getvalue())
                run.assert_not_called()

    def test_metrics_only_does_not_require_bash_or_start_configure(self):
        with patch.dict(os.environ, {}, clear=True), \
             patch.object(sys, "argv", ["prepare", "--metrics-only", "--log", "test.log",
                                        "--cache", "cache"]), \
             patch.object(helper, "report") as report, \
             patch.object(helper, "run") as run:
            self.assertEqual(helper.main(), 0)
            report.assert_called_once_with(Path("test.log"), Path("cache"))
            run.assert_not_called()

    def test_partial_snapshot_saved_after_success_failure_or_controlled_timeout_before_build(self):
        prepare = self.named("Prepare Windows dependencies with bounded configure")
        save = self.named("Save dependency snapshot before ThemisDB compile")
        self.assertLess(self.steps.index(prepare), self.steps.index(save))
        self.assertLess(self.steps.index(save), self.steps.index(self.named("Build")))
        self.assertIn("steps.dependency_prepare.outcome == 'failure'", save["if"])
        self.assertIn("steps.dependency_prepare.outcome == 'success'", save["if"])
        self.assertNotIn("fingerprint", save["if"], "even unchanged restores need a new complete key")
        self.assertEqual(save["uses"], "actions/cache/save@v5")
        self.assertEqual(save["timeout-minutes"], 30)
        self.assertEqual(prepare["timeout-minutes"], 305)
        self.assertFalse(save["with"]["enableCrossOsArchive"])
        complete = self.named("Confirm completed dependency snapshot")
        self.assertIn("steps.dependency_prepare.outcome == 'success'", complete["if"])
        self.assertIn("steps.dependency_save.outcome == 'success'", complete["if"])
        for name in ["Configure", "Build", "Validation"]:
            self.assertIn("inputs.windows_dependency_mode != 'prepare'", self.named(name)["if"])

    def test_windows_disabled_skips_preparation_and_does_not_block_other_lanes(self):
        result, values = self.plan("linux-amd64-only")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(values["windows_enabled"], "false")
        self.assertEqual(len(json.loads(values["matrix"])["include"]), 1)
        jobs = self.release["jobs"]
        self.assertIn("windows_enabled == 'true'", jobs["windows-dependencies"]["if"])
        self.assertIn("!cancelled()", jobs["release-platform-matrix"]["if"])
        self.assertNotIn("windows-dependencies.result", jobs["release-platform-matrix"]["if"])
        self.assertEqual(jobs["windows-dependencies"]["with"]["windows_dependency_mode"], "prepare")
        self.assertIn("matrix.id == 'windows-amd64'", jobs["release-platform-matrix"]["with"]["windows_dependency_mode"])

    def test_windows_dependency_jobs_do_not_register_sccache_action_cleanup(self):
        step = self.named("Install sccache")
        self.assertEqual(
            step["if"],
            "inputs.enable_sccache_gha && !(runner.os == 'Windows' && inputs.windows_dependency_mode != 'off')",
        )
        self.assertEqual(step["uses"], "mozilla-actions/sccache-action@v0.0.6")

    def test_windows_sccache_installation_retries_and_requires_a_working_binary(self):
        result, values = self.plan("windows-amd64-only")
        self.assertEqual(result.returncode, 0, result.stderr)
        setup = json.loads(values["windows_lane"])["setup_command"]
        install = setup.split("git submodule", 1)[0]
        for installed, failures, unusable, attempts, waits, status in [
            (True, 0, False, 0, [], 0),
            (False, 0, False, 1, [], 0),
            (False, 1, False, 2, ["15"], 0),
            (False, 2, False, 3, ["15", "30"], 0),
            (False, 3, False, 3, ["15", "30"], 37),
            (False, 0, True, 3, ["15", "30"], 42),
        ]:
            with self.subTest(installed=installed, failures=failures, unusable=unusable):
                command = """
                    count=0
                    sccache() {
                      [ "$*" = "--version" ] || return 99
                      [ "$UNUSABLE" = "false" ] || return 42
                      [ "$INSTALLED" = "true" ] || return 127
                    }
                    choco() {
                      [ "$*" = "install -y sccache --no-progress" ] || return 99
                      count=$((count + 1))
                      printf 'install:%s\\n' "$count"
                      [ "$count" -gt "$FAILURES" ] || return 37
                      INSTALLED=true
                    }
                    sleep() { printf 'sleep:%s\\n' "$1"; }
                """ + install + "\nprintf 'setup continued\\n'\n"
                result = subprocess.run(
                    ["bash", "-euo", "pipefail", "-c", command],
                    env={**os.environ, "INSTALLED": str(installed).lower(),
                         "FAILURES": str(failures), "UNUSABLE": str(unusable).lower()},
                    text=True, capture_output=True, timeout=10,
                )
                self.assertEqual(result.returncode, status, result.stderr)
                lines = result.stdout.splitlines()
                self.assertEqual(
                    [line for line in lines if line.startswith("install:")],
                    [f"install:{attempt}" for attempt in range(1, attempts + 1)],
                )
                self.assertEqual(
                    [line for line in lines if line.startswith("sleep:")],
                    [f"sleep:{wait}" for wait in waits],
                )
                self.assertEqual("setup continued" in lines, status == 0)
                if status:
                    self.assertIn("::error::sccache installation failed after 3 attempts", lines)

    def test_windows_bootstrap_retries_are_bounded_and_preserve_failure_status(self):
        result, values = self.plan("windows-amd64-only")
        self.assertEqual(result.returncode, 0, result.stderr)
        setup = json.loads(values["windows_lane"])["setup_command"]
        for failures, attempts, waits, status in [
            (0, 1, [], 0),
            (1, 2, ["15"], 0),
            (2, 3, ["15", "30"], 0),
            (3, 3, ["15", "30"], 37),
        ]:
            with self.subTest(failures=failures), tempfile.TemporaryDirectory(prefix="bootstrap space ") as temp:
                directory = Path(temp)
                tools = directory / "tools"
                tools.mkdir()
                for name in ["git", "choco", "sccache"]:
                    (tools / name).symlink_to("/bin/true")
                sleep = tools / "sleep"
                sleep.write_text('#!/bin/sh\nprintf "%s\\n" "$1" >> "$WAIT_LOG"\n')
                sleep.chmod(0o755)
                toolchain = directory / "vcpkg/scripts/buildsystems/vcpkg.cmake"
                toolchain.parent.mkdir(parents=True)
                toolchain.touch()
                bootstrap = directory / "vcpkg/bootstrap-vcpkg.bat"
                bootstrap.write_text(
                    '#!/bin/bash\nset -euo pipefail\n'
                    '[ "$#" -eq 1 ] && [ "$1" = "-disableMetrics" ] || exit 42\n'
                    'attempt=0\n'
                    '[ ! -f "$ATTEMPT_LOG" ] || read -r attempt < "$ATTEMPT_LOG"\n'
                    'attempt=$((attempt + 1))\n'
                    'printf "%s\\n" "$attempt" > "$ATTEMPT_LOG"\n'
                    'if [ "$attempt" -le "$FAILURES" ]; then\n'
                    '  echo "Download failed, server returned HTTP status: 504"\n'
                    '  exit 37\n'
                    'fi\n'
                )
                bootstrap.chmod(0o755)
                downloader = directory / "scripts/download-tinyllama-hf.sh"
                downloader.parent.mkdir()
                downloader.write_text('touch "$GITHUB_WORKSPACE/model-step-ran"\n')
                attempt_log = directory / "attempts"
                wait_log = directory / "waits"
                result = subprocess.run(
                    ["bash", "-euo", "pipefail", "-c", setup],
                    cwd=directory,
                    env={**os.environ, "PATH": f"{tools}{os.pathsep}{os.environ['PATH']}",
                         "FAILURES": str(failures), "ATTEMPT_LOG": str(attempt_log),
                         "WAIT_LOG": str(wait_log), "GITHUB_WORKSPACE": str(directory)},
                    text=True, capture_output=True, timeout=10,
                )
                self.assertEqual(result.returncode, status, result.stderr)
                self.assertEqual(int(attempt_log.read_text()), attempts)
                self.assertEqual(wait_log.read_text().splitlines() if wait_log.exists() else [], waits)
                self.assertEqual((directory / "model-step-ran").exists(), status == 0)
                if status:
                    self.assertIn("::error::vcpkg bootstrap failed after 3 attempts", result.stdout)

    def test_prepare_consume_use_same_pinned_source_configure_features_and_triplet(self):
        jobs = self.release["jobs"]
        prerequisite = jobs["windows-dependencies"]["with"]
        build = jobs["release-platform-matrix"]["with"]
        self.assertEqual(prerequisite["source_sha"], build["source_sha"])
        self.assertEqual(prerequisite["source_sha"], "${{ inputs.source_sha || github.sha }}")
        self.assertIn(".configure_command", prerequisite["configure_command"])
        self.assertEqual(build["configure_command"], "${{ matrix.configure_command }}")
        for edition in ["minimal", "community", "enterprise"]:
            for config in ["Debug", "Release"]:
                result, values = self.plan("windows-amd64-only", edition, config)
                self.assertEqual(result.returncode, 0, result.stderr)
                lane = json.loads(values["windows_lane"])
                self.assertEqual(lane, json.loads(values["matrix"])["include"][0])
                self.assertIn(f"-DCMAKE_BUILD_TYPE={config}", lane["configure_command"])
                self.assertIn("-DVCPKG_HOST_TRIPLET=x64-windows", lane["configure_command"])
                self.assertEqual(lane["triplet"], prerequisite["vcpkg_triplet"])
                self.assertEqual("-DTHEMIS_ENABLE_GRPC=ON" in lane["configure_command"], edition != "minimal")
                self.assertNotIn("VCPKG_BUILD_TYPE", lane["configure_command"])
                self.assertEqual("VCPKG_OVERLAY_TRIPLETS" in lane["configure_command"], config == "Release")

    def test_release_triplet_is_available_for_pinned_sources(self):
        triplet = (ROOT / "triplets/x64-windows.cmake").read_text()
        for setting in [
            "set(VCPKG_TARGET_ARCHITECTURE x64)",
            "set(VCPKG_CRT_LINKAGE dynamic)",
            "set(VCPKG_LIBRARY_LINKAGE dynamic)",
            "set(VCPKG_BUILD_TYPE release)",
        ]:
            self.assertIn(setting, triplet)
        checkout = self.named("Checkout cache tooling for pinned release sources")
        self.assertIn("triplets/x64-windows.cmake", checkout["with"]["sparse-checkout"])
        result, values = self.plan("windows-amd64-only")
        self.assertEqual(result.returncode, 0, result.stderr)
        command = json.loads(values["windows_lane"])["configure_command"]
        self.assertIn('${GITHUB_WORKSPACE}/${THEMIS_CACHE_TOOLS_PREFIX}triplets', command)
        self.assertIn('"-DVCPKG_OVERLAY_TRIPLETS=${VCPKG_RELEASE_TRIPLETS}"', command)
        for prefix in ["", ".ci-cache-tools/"]:
            with self.subTest(prefix=prefix):
                result = subprocess.run(
                    ["bash", "-euo", "pipefail", "-c",
                     'cygpath() { printf "%s\\n" "$2"; }\n'
                     'cmake() { printf "<%s>\\n" "$@"; }\n'
                     'tee() { cat; }\n' + command],
                    env={**os.environ, "GITHUB_WORKSPACE": "/workspace with spaces",
                         "THEMIS_CACHE_TOOLS_PREFIX": prefix,
                         "THEMIS_WINDOWS_DEPENDENCY_FLAGS": "-DVCPKG_INSTALL_OPTIONS=--only-binarycaching"},
                    text=True, capture_output=True,
                )
                self.assertEqual(result.returncode, 0, result.stderr)
                self.assertIn(f"<-DVCPKG_OVERLAY_TRIPLETS=/workspace with spaces/{prefix}triplets>", result.stdout)
                self.assertIn("<-DVCPKG_INSTALL_OPTIONS=--only-binarycaching>", result.stdout)

    def test_release_configure_evaluates_release_only_triplet_for_pinned_sources(self):
        result, values = self.plan("windows-amd64-only")
        self.assertEqual(result.returncode, 0, result.stderr)
        command = json.loads(values["windows_lane"])["configure_command"]
        triplet = (ROOT / "triplets/x64-windows.cmake").read_text()
        with tempfile.TemporaryDirectory(prefix="release triplet ") as temp:
            directory = Path(temp)
            probe = directory / "probe.cmake"
            probe.write_text(
                'include("${VCPKG_OVERLAY_TRIPLETS}/${VCPKG_HOST_TRIPLET}.cmake")\n'
                'if(NOT VCPKG_BUILD_TYPE STREQUAL "release")\n'
                '  message(FATAL_ERROR "Debug dependencies would exhaust the release budget")\n'
                'endif()\n'
                'if(NOT VCPKG_TARGET_ARCHITECTURE STREQUAL "x64" OR\n'
                '   NOT VCPKG_CRT_LINKAGE STREQUAL "dynamic" OR\n'
                '   NOT VCPKG_LIBRARY_LINKAGE STREQUAL "dynamic")\n'
                '  message(FATAL_ERROR "Release dependency ABI changed")\n'
                'endif()\n'
                'message(STATUS "Release-only dependency triplet loaded")\n'
            )
            for prefix in ["", ".ci-cache-tools/"]:
                with self.subTest(prefix=prefix):
                    source_triplets = directory / "triplets"
                    source_triplets.mkdir(exist_ok=True)
                    (source_triplets / "x64-windows.cmake").write_text(
                        "set(VCPKG_TARGET_ARCHITECTURE x64)\n"
                    )
                    selected_triplets = directory / prefix / "triplets"
                    selected_triplets.mkdir(parents=True, exist_ok=True)
                    (selected_triplets / "x64-windows.cmake").write_text(triplet)
                    result = subprocess.run(
                        ["bash", "-euo", "pipefail", "-c",
                         'cygpath() { printf "%s\\n" "$2"; }\n'
                         + command.replace("cmake --preset windows-release", "cmake")
                         .replace("2>&1 | tee configure-windows-release.log",
                                  '-P "$TRIPLET_PROBE" 2>&1')],
                        cwd=directory,
                        env={**os.environ, "GITHUB_WORKSPACE": temp,
                             "THEMIS_CACHE_TOOLS_PREFIX": prefix,
                             "THEMIS_WINDOWS_DEPENDENCY_FLAGS": "", "TRIPLET_PROBE": str(probe)},
                        text=True, capture_output=True,
                    )
                    self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                    self.assertIn("Release-only dependency triplet loaded", result.stdout)

    def test_windows_configure_placeholders_are_not_github_expressions(self):
        script = self.release["jobs"]["prepare-release-matrix"]["steps"][0]["run"]
        self.assertNotIn("${{CMAKE_FLAGS}}", script)
        self.assertNotIn("${{THEMIS_WINDOWS_DEPENDENCY_FLAGS:-}}", script)
        result, values = self.plan("windows-amd64-only")
        self.assertEqual(result.returncode, 0, result.stderr)
        command = json.loads(values["windows_lane"])["configure_command"]
        self.assertNotIn("${CMAKE_FLAGS}", command)
        self.assertIn("-DTHEMIS_EDITION=COMMUNITY", command)
        self.assertIn("${THEMIS_WINDOWS_DEPENDENCY_FLAGS:-}", command)
        self.assertIn("-DVCPKG_HOST_TRIPLET=x64-windows", command)

    def test_dependency_parallelism_is_configurable_without_changing_build_parallelism(self):
        for concurrency in ["1", "2", "4"]:
            result, values = self.plan("windows-amd64-only", concurrency=concurrency)
            self.assertEqual(result.returncode, 0, result.stderr)
            lane = json.loads(values["windows_lane"])
            self.assertIn(f"VCPKG_MAX_CONCURRENCY={concurrency}", lane["configure_command"])
            self.assertIn(f"CMAKE_BUILD_PARALLEL_LEVEL={concurrency}", lane["configure_command"])
            self.assertIn("VCPKG_MAX_CONCURRENCY=1", lane["build_command"])
            self.assertIn("--parallel 1", lane["build_command"])
        result, _ = self.plan(concurrency="16")
        self.assertNotEqual(result.returncode, 0)

    def test_consume_fails_closed_without_successful_prerequisite(self):
        gate = self.named("Validate dependency mode and start job budget")
        with tempfile.TemporaryDirectory() as temp:
            result = subprocess.run(
                ["bash", "-euo", "pipefail", "-c", gate["run"]],
                env={**os.environ, "DEPENDENCY_MODE": "consume", "DEPENDENCY_CACHE_KEY": "",
                     "RUNNER_OS": "Windows", "GITHUB_ENV": str(Path(temp) / "env")},
                text=True, capture_output=True,
            )
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("prerequisite incomplete", result.stdout)

    def test_windows_bootstrap_retries_are_bounded_and_fail_closed(self):
        result, values = self.plan("windows-amd64-only")
        self.assertEqual(result.returncode, 0, result.stderr)
        setup = json.loads(values["windows_lane"])["setup_command"]
        prerequisite = self.release["jobs"]["windows-dependencies"]["with"]
        self.assertIn(".setup_command", prerequisite["setup_command"])
        self.assertEqual(
            self.release["jobs"]["release-platform-matrix"]["with"]["setup_command"],
            "${{ matrix.setup_command }}",
        )
        bootstrap = "for attempt in" + setup.rsplit("for attempt in", 1)[1].split("\nMODEL_DIR=", 1)[0]
        bootstrap = bootstrap.replace("./vcpkg/bootstrap-vcpkg.bat", "bootstrap")
        for failures, attempts, delays, status in [
            (0, 1, [], 0), (1, 2, [15], 0), (2, 3, [15, 30], 0),
            (3, 3, [15, 30], 37),
        ]:
            with self.subTest(failures=failures):
                command = """
                    count=0
                    bootstrap() {
                      count=$((count + 1))
                      printf 'bootstrap:%s:%s\\n' "$count" "$*"
                      if [ "$count" -le "$FAILURES" ]; then return 37; fi
                    }
                    sleep() { printf 'sleep:%s\\n' "$1"; }
                """ + bootstrap + "\nprintf 'setup continued\\n'\n"
                result = subprocess.run(
                    ["bash", "-euo", "pipefail", "-c", command],
                    env={**os.environ, "FAILURES": str(failures)},
                    text=True, capture_output=True,
                )
                self.assertEqual(result.returncode, status, result.stderr)
                lines = result.stdout.splitlines()
                self.assertEqual(
                    [line for line in lines if line.startswith("bootstrap:")],
                    [f"bootstrap:{attempt}:-disableMetrics" for attempt in range(1, attempts + 1)],
                )
                self.assertEqual(
                    [line for line in lines if line.startswith("sleep:")],
                    [f"sleep:{delay}" for delay in delays],
                )
                self.assertEqual("setup continued" in lines, status == 0)
                self.assertEqual("failed after 3 attempts" in result.stdout, status != 0)

    def test_cache_diagnostics_distinguish_exact_prefix_and_none_and_enforce_exact_consume(self):
        script = self.named("Print initial cache hit state")["run"]
        with tempfile.TemporaryDirectory() as temp:
            for mode, hit, matched, state, status in [
                ("prepare", "true", "key", "exact-match", 0),
                ("prepare", "false", "older", "prefix-restored", 0),
                ("prepare", "", "", "no-cache", 0),
                ("consume", "false", "older", "prefix-restored", 1),
                ("consume", "true", "key", "exact-match", 0),
                ("consume", "true", "wrong-source", "exact-match", 1),
            ]:
                with self.subTest(mode=mode, state=state):
                    result = subprocess.run(
                        ["bash", "-c", script.replace("${{ inputs.windows_dependency_mode }}", mode)],
                        env={**os.environ, "CACHE_HIT": hit, "CACHE_MATCHED_KEY": matched,
                             "CACHE_EXPECTED_PREFIX": "key",
                             "GITHUB_ENV": str(Path(temp) / "env")},
                        text=True, capture_output=True,
                    )
                    self.assertEqual(result.returncode, status, result.stderr)
                    self.assertIn(state, result.stdout)
                    self.assertIn(f"matched key: {matched}", result.stdout)
            self.assertIn("--only-binarycaching", (Path(temp) / "env").read_text())

    def test_cache_identity_paths_and_build_tree_are_isolated_and_native_safe(self):
        init = self.named("Initialize compiler and dependency cache environment")["run"]
        self.assertIn("release-deps-v2", init)
        self.assertIn("git rev-parse HEAD", init)
        self.assertIn('printf \'%s\' "$DEPENDENCY_CONFIGURE_SIGNATURE" | sha256sum', init)
        self.assertIn('cygpath -m "$vcpkg_cache_dir"', init)
        cache = self.named("Restore CMake build caches")["with"]
        for token in ["runner.os", "THEMIS_CACHE_TAG", "hashFiles", "THEMIS_VCPKG_REVISION",
                      "THEMIS_COMPILER_VERSION", "THEMIS_DEPENDENCY_SOURCE",
                      "github.run_id", "github.run_attempt", "inputs.dependency_cache_key"]:
            self.assertIn(token, cache["key"])
        self.assertIn("!= 'consume'", cache["restore-keys"])
        self.assertFalse(cache["enableCrossOsArchive"])
        tree = self.named("Restore CMake build directory cache")
        self.assertIn("inputs.windows_dependency_mode == 'off'", tree["if"])
        checkout = self.named("Checkout cache tooling for pinned release sources")
        self.assertIn("scripts/prepare-windows-dependencies.py", checkout["with"]["sparse-checkout"])

    def test_metrics_count_only_zips_and_report_actual_install_reuse(self):
        with tempfile.TemporaryDirectory() as temp:
            cache = Path(temp)
            for name in ["one.zip", "two.ZIP", "temporary.tmp", "README"]:
                (cache / name).touch()
            log = cache / "install.log"
            log.write_text("Restored 12 package(s) from binary cache in 2 s.\npackage ABI: abc123\n")
            output = io.StringIO()
            with contextlib.redirect_stdout(output):
                helper.report(log, cache)
            self.assertIn("ZIP archives: 2", output.getvalue())
            self.assertIn("Restored 12 package(s)", output.getvalue())
            self.assertIn("package ABI: abc123", output.getvalue())
            self.assertIn("zero requests means no compiler requests, not cache misses", output.getvalue())
            log.write_text("vcpkg install failed before restore\n")
            output = io.StringIO()
            with contextlib.redirect_stdout(output):
                helper.report(log, cache)
            self.assertIn("not reported", output.getvalue())
            self.assertNotIn("Restored 0", output.getvalue())


if __name__ == "__main__":
    unittest.main()
