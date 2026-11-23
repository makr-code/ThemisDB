"""Contract tests for the WAL process-crash CI callers."""

from pathlib import Path
import subprocess
import sys
import unittest

import yaml


ROOT = Path(__file__).resolve().parents[1]


def workflow(name):
    return yaml.safe_load(
        (ROOT / ".github" / "workflows" / name).read_text(encoding="utf-8")
    )


def sanitizer_selection(mode, *paths):
    result = subprocess.run(
        [
            sys.executable,
            str(ROOT / "tools" / "ci" / "sanitizer_targets.py"),
            "--mode",
            mode,
            "--changed-files",
            *paths,
        ],
        capture_output=True,
        text=True,
        check=True,
        cwd=ROOT,
    )
    return result.stdout.strip()


class WALCrashWorkflowTests(unittest.TestCase):
    def test_pr_storage_gate_stays_release_critical_and_stress_runs_nightly(self):
        document = workflow("gate-pr-core.yml")
        trigger = document.get("on", document.get(True))["pull_request"]
        self.assertIn("develop", trigger["branches"])
        self.assertIn("src/storage/**", trigger["paths"])
        self.assertIn("include/storage/**", trigger["paths"])
        self.assertIn("tests/storage/**", trigger["paths"])
        self.assertNotIn("quality-test-build", document["jobs"])
        self.assertIn("build-components-gate", document["jobs"])

        sanitizer = workflow("build-sanitizer-nightly.yml")
        sanitizer_events = sanitizer.get("on", sanitizer.get(True))
        self.assertNotIn("pull_request", sanitizer_events)
        settings = sanitizer["jobs"]["sanitizer-build"]["with"]
        self.assertIn("--iterations 1000", settings["validation_command"])
        self.assertTrue(sanitizer_events["schedule"])

        cmake = (ROOT / "tests" / "storage" / "CMakeLists.txt").read_text()
        self.assertIn("NAME WALProcessUnobservedAppendTests", cmake)
        self.assertIn("--scenario unobserved-append", cmake)

    def test_nightly_builds_worker_and_runs_thousand_iterations(self):
        document = workflow("build-sanitizer-nightly.yml")
        self.assertTrue(document.get("on", document.get(True))["schedule"])
        settings = document["jobs"]["sanitizer-build"]["with"]
        self.assertIn("tools/ci/sanitizer_targets.py", settings["build_command"])
        self.assertIn(
            "test_wal_process_crash_worker",
            sanitizer_selection("build", "tests/storage/test_wal_process_crash.py"),
        )
        self.assertIn("set -euo pipefail", settings["validation_command"])
        self.assertIn("--mode regex", settings["validation_command"])
        self.assertIn(
            "WALProcessCrashRecoveryTests",
            sanitizer_selection("regex", "src/storage/wal_storage.cpp"),
        )
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

    def test_boundary_campaign_runs_and_uploads_nightly(self):
        settings = workflow("build-sanitizer-nightly.yml")["jobs"]["sanitizer-build"]["with"]
        self.assertIn("python3 tests/storage/test_wal_tail_boundaries.py",
                      settings["validation_command"])
        self.assertIn("--worker build-sanitizer/bin/test_wal_process_crash_worker",
                      settings["validation_command"])
        self.assertIn("--report build-sanitizer/test-results/wal-tail-boundaries.json",
                      settings["validation_command"])
        self.assertIn("build-sanitizer/test-results/wal-tail-boundaries.json",
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
