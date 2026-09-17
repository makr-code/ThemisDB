#!/usr/bin/env python3

import json
import os
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parent.parent
SCRIPT_PATH = REPO_ROOT / ".github" / "scripts" / "aggregate-build-errors.js"


class AggregateBuildErrorsTests(unittest.TestCase):
    def test_script_aggregates_duplicate_log_findings_without_runtime_error(self) -> None:
        with tempfile.TemporaryDirectory() as tmp_dir:
            temp_root = Path(tmp_dir)
            artifacts_dir = temp_root / "artifacts"
            artifacts_dir.mkdir()

            for index, timestamp in enumerate(
                ["2026-09-15T16:37:31.433Z", "2026-09-15T16:38:31.433Z"], start=1
            ):
                run_dir = artifacts_dir / f"run-{index}"
                run_dir.mkdir()
                (run_dir / "_metadata.json").write_text(
                    json.dumps(
                        {
                            "run_id": 1000 + index,
                            "run_number": index,
                            "workflow": "Build: Mainline",
                            "created_at": timestamp,
                        }
                    ),
                    encoding="utf-8",
                )
                (run_dir / "build.log").write_text(
                    "src/index/multi_gpu_vector_index.cpp:254:7: error: malformed cast syntax\n",
                    encoding="utf-8",
                )

            output_file = temp_root / "aggregated-errors.md"
            grouped_file = temp_root / "aggregated-errors-by-module-file.json"
            state_file = temp_root / "aggregated-error-state.json"
            tracks_file = temp_root / "chronic-triage-baseline.json"

            env = os.environ.copy()
            env.update(
                {
                    "ERROR_ARTIFACTS_DIR": str(artifacts_dir),
                    "OUTPUT_FILE": str(output_file),
                    "GROUPED_OUTPUT_FILE": str(grouped_file),
                    "CURRENT_STATE_FILE": str(state_file),
                    "TRACKS_OUTPUT_FILE": str(tracks_file),
                    "GITHUB_WORKSPACE": str(temp_root),
                }
            )

            result = subprocess.run(
                ["node", str(SCRIPT_PATH)],
                cwd=REPO_ROOT,
                capture_output=True,
                text=True,
                env=env,
            )

            self.assertEqual(result.returncode, 0, msg=result.stdout + result.stderr)
            self.assertTrue(output_file.exists())
            self.assertTrue(grouped_file.exists())
            self.assertIn("Unique diagnostics: 1", result.stdout)
            self.assertIn("Chronic diagnostics: 0", result.stdout)

            grouped = json.loads(grouped_file.read_text(encoding="utf-8"))
            self.assertEqual(grouped["stats"]["total_unique_errors"], 1)
            self.assertEqual(grouped["entries"][0]["counts"]["compiler_error"], 2)

    def test_script_ignores_non_src_diagnostics(self) -> None:
        with tempfile.TemporaryDirectory() as tmp_dir:
            temp_root = Path(tmp_dir)
            artifacts_dir = temp_root / "artifacts"
            run_dir = artifacts_dir / "run-1"
            run_dir.mkdir(parents=True)

            (run_dir / "_metadata.json").write_text(
                json.dumps(
                    {
                        "run_id": 2001,
                        "run_number": 1,
                        "workflow": "Build: Mainline",
                        "created_at": "2026-09-17T10:00:00Z",
                    }
                ),
                encoding="utf-8",
            )
            (run_dir / "build.log").write_text(
                "\n".join(
                    [
                        "/opt/vcpkg/installed/x64-linux/include/fmt/core.h:10:3: error: external lib error",
                        "src/index/multi_gpu_vector_index.cpp:254:7: error: malformed cast syntax",
                    ]
                )
                + "\n",
                encoding="utf-8",
            )

            output_file = temp_root / "aggregated-errors.md"
            grouped_file = temp_root / "aggregated-errors-by-module-file.json"
            state_file = temp_root / "aggregated-error-state.json"
            tracks_file = temp_root / "chronic-triage-baseline.json"

            env = os.environ.copy()
            env.update(
                {
                    "ERROR_ARTIFACTS_DIR": str(artifacts_dir),
                    "OUTPUT_FILE": str(output_file),
                    "GROUPED_OUTPUT_FILE": str(grouped_file),
                    "CURRENT_STATE_FILE": str(state_file),
                    "TRACKS_OUTPUT_FILE": str(tracks_file),
                    "GITHUB_WORKSPACE": str(temp_root),
                }
            )

            result = subprocess.run(
                ["node", str(SCRIPT_PATH)],
                cwd=REPO_ROOT,
                capture_output=True,
                text=True,
                env=env,
            )

            self.assertEqual(result.returncode, 0, msg=result.stdout + result.stderr)
            grouped = json.loads(grouped_file.read_text(encoding="utf-8"))
            self.assertEqual(grouped["stats"]["total_unique_errors"], 1)
            self.assertEqual(grouped["entries"][0]["file"], "src/index/multi_gpu_vector_index.cpp")

    def test_state_snapshot_prefers_chronic_when_capped(self) -> None:
        with tempfile.TemporaryDirectory() as tmp_dir:
            temp_root = Path(tmp_dir)
            artifacts_dir = temp_root / "artifacts"
            run_dir = artifacts_dir / "run-1"
            run_dir.mkdir(parents=True)

            (run_dir / "_metadata.json").write_text(
                json.dumps(
                    {
                        "run_id": 3001,
                        "run_number": 1,
                        "workflow": "Build: Mainline",
                        "created_at": "2026-09-17T10:00:00Z",
                    }
                ),
                encoding="utf-8",
            )
            log_lines = [
                "src/index/multi_gpu_vector_index.cpp:254:7: error: chronic error",
                "src/index/multi_gpu_vector_index.cpp:254:7: error: chronic error",
                "src/index/multi_gpu_vector_index.cpp:254:7: error: chronic error",
            ]
            log_lines.extend(
                [
                    f"src/storage/columnar_format.cpp:{100 + idx}:2: error: one-off error {idx}"
                    for idx in range(60)
                ]
            )
            (run_dir / "build.log").write_text("\n".join(log_lines) + "\n", encoding="utf-8")

            output_file = temp_root / "aggregated-errors.md"
            grouped_file = temp_root / "aggregated-errors-by-module-file.json"
            state_file = temp_root / "aggregated-error-state.json"
            tracks_file = temp_root / "chronic-triage-baseline.json"

            env = os.environ.copy()
            env.update(
                {
                    "ERROR_ARTIFACTS_DIR": str(artifacts_dir),
                    "OUTPUT_FILE": str(output_file),
                    "GROUPED_OUTPUT_FILE": str(grouped_file),
                    "CURRENT_STATE_FILE": str(state_file),
                    "TRACKS_OUTPUT_FILE": str(tracks_file),
                    "GITHUB_WORKSPACE": str(temp_root),
                    "MAX_STATE_ERRORS": "50",
                }
            )

            result = subprocess.run(
                ["node", str(SCRIPT_PATH)],
                cwd=REPO_ROOT,
                capture_output=True,
                text=True,
                env=env,
            )

            self.assertEqual(result.returncode, 0, msg=result.stdout + result.stderr)
            state = json.loads(state_file.read_text(encoding="utf-8"))
            self.assertEqual(state["stored_error_count"], 50)
            self.assertEqual(len(state["errors"]), 50)
            self.assertTrue(
                any(
                    entry["file"] == "src/index/multi_gpu_vector_index.cpp"
                    and entry["frequency"] == 3
                    for entry in state["errors"]
                )
            )


if __name__ == "__main__":
    unittest.main()
