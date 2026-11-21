"""Contract tests for the WAL process-crash CI callers."""

from pathlib import Path
import unittest

import yaml


ROOT = Path(__file__).resolve().parents[1]


def workflow(name):
    return yaml.safe_load(
        (ROOT / ".github" / "workflows" / name).read_text(encoding="utf-8")
    )


class WALCrashWorkflowTests(unittest.TestCase):
    def test_pr_storage_changes_trigger_fifty_iteration_ctest(self):
        document = workflow("gate-pr-core.yml")
        trigger = document.get("on", document.get(True))["pull_request"]
        self.assertIn("develop", trigger["branches"])
        for path in (
            "src/storage/**", "include/storage/**", "tests/storage/*.py",
            "tests/test_wal_process_crash_report.py",
        ):
            self.assertIn(path, trigger["paths"])
        settings = document["jobs"]["quality-test-build"]["with"]
        self.assertIn("--target test_wal_process_crash_worker", settings["build_command"])
        self.assertIn("set -euo pipefail", settings["build_command"])
        self.assertIn("--no-tests=error", settings["validation_command"])
        self.assertIn("-R '^WALProcess(CrashRecovery|UnobservedAppend)Tests$'",
                      settings["validation_command"])
        self.assertIn("test_wal_process_crash_report.py", settings["validation_command"])
        self.assertIn("wal-process-crash.json", settings["artifact_paths_secondary"])
        self.assertIn("wal-unobserved-append.json", settings["artifact_paths_secondary"])
        self.assertIn("test-results/*-failures/", settings["artifact_paths_secondary"])
        cmake = (ROOT / "tests" / "storage" / "CMakeLists.txt").read_text()
        self.assertIn("--iterations 50", cmake)
        self.assertIn('--report "${CMAKE_BINARY_DIR}/test-results/wal-process-crash.json"', cmake)
        self.assertIn("NAME WALProcessUnobservedAppendTests", cmake)
        self.assertIn("--scenario unobserved-append", cmake)

    def test_nightly_builds_worker_and_runs_thousand_iterations(self):
        document = workflow("build-sanitizer-nightly.yml")
        self.assertTrue(document.get("on", document.get(True))["schedule"])
        settings = document["jobs"]["sanitizer-build"]["with"]
        self.assertIn("test_wal_process_crash_worker", settings["build_command"])
        self.assertIn("set -euo pipefail", settings["validation_command"])
        self.assertIn("--iterations 1000", settings["validation_command"])
        self.assertIn("--worker build-sanitizer/bin/test_wal_process_crash_worker",
                      settings["validation_command"])
        self.assertIn("--report build-sanitizer/test-results/wal-process-crash.json",
                      settings["validation_command"])
        self.assertIn("build-sanitizer/test-results/wal-process-crash.json",
                      settings["artifact_paths_secondary"])
        self.assertIn("build-sanitizer/test-results/wal-unobserved-append.json",
                      settings["artifact_paths_secondary"])
        self.assertIn("--scenario unobserved-append", settings["validation_command"])
        self.assertEqual(settings["validation_command"].count("--iterations 1000"), 2)
        self.assertIn("test-results/*-failures/", settings["artifact_paths_secondary"])

    def test_secondary_evidence_upload_runs_after_failure_but_not_cancellation(self):
        document = workflow("reusable-cmake-build.yml")
        steps = document["jobs"]["build"]["steps"]
        upload = next(step for step in steps if step.get("name") == "Upload secondary artifacts")
        self.assertIn("!cancelled()", upload["if"])
        self.assertIn("inputs.artifact_name_secondary != ''", upload["if"])
        self.assertIn("inputs.artifact_paths_secondary != ''", upload["if"])
        self.assertNotIn("success()", upload["if"])
        self.assertEqual(upload["with"]["path"], "${{ inputs.artifact_paths_secondary }}")

    def test_boundary_campaign_runs_and_uploads_in_both_callers(self):
        for name, job, build in (
            ("gate-pr-core.yml", "quality-test-build", "build-pr-release-critical"),
            ("build-sanitizer-nightly.yml", "sanitizer-build", "build-sanitizer"),
        ):
            with self.subTest(workflow=name):
                settings = workflow(name)["jobs"][job]["with"]
                self.assertIn("python3 tests/storage/test_wal_tail_boundaries.py",
                              settings["validation_command"])
                self.assertIn(f"--worker {build}/bin/test_wal_process_crash_worker",
                              settings["validation_command"])
                self.assertIn(f"--report {build}/test-results/wal-tail-boundaries.json",
                              settings["validation_command"])
                self.assertIn(f"{build}/test-results/wal-tail-boundaries.json",
                              settings["artifact_paths_secondary"])

    def test_sanitizer_build_disables_recovery_for_both_modes(self):
        settings = workflow("build-sanitizer-nightly.yml")["jobs"]["sanitizer-build"]["with"]
        configure = settings["configure_command"]
        self.assertIn(
            'SAN_FLAGS="-fsanitize=address -fno-sanitize-recover=all -fno-omit-frame-pointer"',
            configure,
        )
        self.assertIn(
            'SAN_FLAGS="-fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer"',
            configure,
        )
        self.assertIn('-DCMAKE_C_FLAGS="$SAN_FLAGS"', configure)
        self.assertIn('-DCMAKE_CXX_FLAGS="$SAN_FLAGS"', configure)


if __name__ == "__main__":
    unittest.main()
