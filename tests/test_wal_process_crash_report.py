"""Regression checks for crash-harness reporting, without storage dependencies."""

import importlib.util
import hashlib
import json
import queue
from pathlib import Path
import sys
import tempfile
import unittest
from types import SimpleNamespace
from unittest.mock import patch
import subprocess


SCRIPT = Path(__file__).parent / "storage" / "test_wal_process_crash.py"
SPEC = importlib.util.spec_from_file_location("wal_process_crash", SCRIPT)
assert SPEC is not None and SPEC.loader is not None
HARNESS = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = HARNESS
SPEC.loader.exec_module(HARNESS)
BOUNDARY_SPEC = importlib.util.spec_from_file_location(
    "wal_tail_boundaries", SCRIPT.with_name("test_wal_tail_boundaries.py")
)
assert BOUNDARY_SPEC is not None and BOUNDARY_SPEC.loader is not None
BOUNDARIES = importlib.util.module_from_spec(BOUNDARY_SPEC)
sys.modules[BOUNDARY_SPEC.name] = BOUNDARIES
with patch.dict(sys.modules, {"test_wal_process_crash": HARNESS}):
    BOUNDARY_SPEC.loader.exec_module(BOUNDARIES)


class CrashReportTests(unittest.TestCase):
    def test_boundary_campaign_exact_fault_sets_preserve_input(self):
        complete = {f"wal_{i:06}.log": b"x" * (117 if i == 4 else 111)
                    for i in range(1, 5)}
        original = dict(complete)
        cases = list(BOUNDARIES.fault_cases(complete, "wal_000004.log", 78))
        self.assertEqual(len(cases), 2980)
        self.assertEqual([case[1] for case in cases[:40]], list(range(40)))
        self.assertEqual([case[1] for case in cases[40:184]],
                         [offset for offset in range(21, 39) for bit in range(8)])
        self.assertEqual([case[1] for case in cases[184:292]], list(range(1, 37)) * 3)
        self.assertEqual(complete, original)
        for _, retained, bit, snapshot, expected in cases[:40]:
            self.assertIsNone(bit)
            self.assertEqual(len(snapshot["wal_000004.log"]), 78 + retained)
            self.assertEqual(expected, 12 if retained == 39 else 11)
        for kind, offset, bit, snapshot, expected in cases[40:184]:
            self.assertEqual(kind, "complete_final_frame_payload_or_crc_bit_flip")
            self.assertIsNone(expected)
            differing = [i for i, (a, b) in enumerate(zip(
                snapshot["wal_000004.log"], complete["wal_000004.log"])) if a != b]
            self.assertEqual(differing, [78 + offset])
            self.assertEqual(snapshot["wal_000004.log"][78 + offset]
                             ^ complete["wal_000004.log"][78 + offset], 1 << bit)
        for index, (_, removed, bit, snapshot, expected) in enumerate(cases[184:292]):
            self.assertIsNone(bit)
            self.assertIsNone(expected)
            affected = f"wal_{index // 36 + 1:06}.log"
            self.assertEqual(len(snapshot[affected]), 111 - removed)
            self.assertEqual([name for name in complete if snapshot[name] != complete[name]],
                             [affected])
            self.assertEqual(snapshot["wal_000004.log"], complete["wal_000004.log"])
        expected_offsets = {
            name: [start + offset for start in range(0, size * frames, size)
                   for offset in range(21, size)]
            for name, size, frames in (
                ("wal_000001.log", 37, 3), ("wal_000002.log", 37, 3),
                ("wal_000003.log", 37, 3), ("wal_000004.log", 39, 2),
            )
        }
        observed = {name: set() for name in complete}
        for kind, offset, bit, snapshot, expected in cases[292:1732]:
            self.assertEqual(kind, "earlier_complete_frame_payload_or_crc_bit_flip")
            self.assertIsNone(expected)
            changed = [name for name in complete if snapshot[name] != complete[name]]
            self.assertEqual(len(changed), 1)
            name = changed[0]
            differing = [i for i, (a, b) in enumerate(zip(snapshot[name], complete[name]))
                         if a != b]
            self.assertEqual(differing, [offset])
            self.assertEqual(snapshot[name][offset] ^ complete[name][offset], 1 << bit)
            observed[name].add((offset, bit))
        self.assertEqual(observed, {
            name: {(offset, bit) for offset in offsets for bit in range(8)}
            for name, offsets in expected_offsets.items()
        })
        header_offsets = {name: set() for name in complete}
        for kind, offset, bit, snapshot, expected in cases[1732:]:
            self.assertEqual(kind, "complete_frame_magic_sequence_type_bit_flip")
            self.assertIsNone(expected)
            changed = [name for name in complete if snapshot[name] != complete[name]]
            self.assertEqual(len(changed), 1)
            name = changed[0]
            differing = [i for i, (a, b) in enumerate(zip(snapshot[name], complete[name]))
                         if a != b]
            self.assertEqual(differing, [offset])
            self.assertEqual(snapshot[name][offset] ^ complete[name][offset], 1 << bit)
            header_offsets[name].add((offset, bit))
        self.assertEqual(header_offsets, {
            name: {(start + offset, bit)
                   for start in range(0, len(data), 39 if name.endswith("4.log") else 37)
                   for offset in range(13) for bit in range(8)}
            for name, data in complete.items()
        })

    def test_boundary_fixture_failure_replaces_stale_report(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "report.json"
            path.write_text('{"status":"passed"}')
            with patch.object(BOUNDARIES, "killed_fixture",
                              side_effect=RuntimeError("fixture layout changed")):
                with self.assertRaisesRegex(HARNESS.CrashIterationFailure, "fixture layout"):
                    BOUNDARIES.main(["--worker", sys.executable, "--report", str(path)])
            saved = json.loads(path.read_text())
            self.assertEqual(saved["status"], "failed")
            self.assertEqual(saved["results"], [])
            self.assertTrue(Path(saved["failure_artifact"]).is_dir())
            self.assertIsNotNone(saved["shared_harness_sha256"])

    def test_boundary_campaign_rejects_corruption_accepted_by_verifier(self):
        snapshot = {"wal_000001.log": b"corrupt"}
        complete = {"wal_000001.log": b"original"}
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "report.json"
            with patch.object(BOUNDARIES, "killed_fixture",
                              return_value=(complete, "wal_000001.log", 0)), \
                 patch.object(BOUNDARIES, "fault_cases",
                              return_value=iter([("corruption", 1, None, snapshot, None)])), \
                 patch.object(HARNESS, "run_verifier",
                              return_value=subprocess.CompletedProcess([], 0, "VERIFIED 12", "")):
                with self.assertRaisesRegex(HARNESS.CrashIterationFailure, "did not fail closed"):
                    BOUNDARIES.main(["--worker", sys.executable, "--report", str(path)])
            saved = json.loads(path.read_text())
            self.assertEqual(saved["status"], "failed")
            self.assertEqual(saved["results"], [])
            artifact = Path(saved["failure_artifact"])
            self.assertEqual((artifact / "wal" / "wal_000001.log").read_bytes(), b"corrupt")
            reproduction = json.loads((artifact / "reproduction.json").read_text())
            self.assertEqual(reproduction["affected_segment"], "wal_000001.log")
            self.assertEqual(reproduction["offset"], 1)

    def test_boundary_bit_flip_failure_records_absolute_byte_offset(self):
        complete = {"wal_000001.log": b"original"}
        snapshot = {"wal_000001.log": b"damaged"}
        for kind, offset, absolute in (
            ("complete_final_frame_payload_or_crc_bit_flip", 21, 99),
            ("earlier_complete_frame_payload_or_crc_bit_flip", 58, 58),
            ("complete_frame_magic_sequence_type_bit_flip", 39, 39),
        ):
            with self.subTest(kind=kind), tempfile.TemporaryDirectory() as directory:
                report = Path(directory) / "report.json"
                with patch.object(BOUNDARIES, "killed_fixture",
                                  return_value=(complete, "wal_000001.log", 78)), \
                     patch.object(BOUNDARIES, "fault_cases",
                                  return_value=iter([(kind, offset, 0, snapshot, None)])), \
                     patch.object(HARNESS, "run_verifier",
                                  side_effect=RuntimeError("verifier failed")):
                    with self.assertRaisesRegex(HARNESS.CrashIterationFailure, "verifier failed"):
                        BOUNDARIES.main(["--worker", sys.executable, "--report", str(report)])
                saved = json.loads(report.read_text())
                artifact = Path(saved["failure_artifact"])
                reproduction = json.loads((artifact / "reproduction.json").read_text())
                self.assertEqual(reproduction["segment_byte_offset"], absolute)
                self.assertEqual(reproduction["bit_index"], 0)
                self.assertEqual(reproduction["affected_segment"], "wal_000001.log")
                self.assertEqual(HARNESS.wal_snapshot(artifact / "before-recovery"), snapshot)

    def test_boundary_failure_preserves_wal_before_recovery_mutation(self):
        complete = {"wal_000001.log": b"original"}
        snapshot = {"wal_000001.log": b"damaged"}
        for expected in (None, 11):
            with self.subTest(expected=expected), tempfile.TemporaryDirectory() as directory:
                report = Path(directory) / "report.json"

                def mutate(worker, wal, *args):
                    (wal / "wal_000001.log").write_bytes(b"changed by recovery")
                    raise RuntimeError("recovery mutated WAL")

                with patch.object(BOUNDARIES, "killed_fixture",
                                  return_value=(complete, "wal_000001.log", 0)), \
                     patch.object(BOUNDARIES, "fault_cases",
                                  return_value=iter([("disk_fault", 1, None, snapshot, expected)])), \
                     patch.object(HARNESS, "run_verifier", side_effect=mutate), \
                     patch.object(HARNESS, "verify_stable_replay", side_effect=mutate):
                    with self.assertRaisesRegex(HARNESS.CrashIterationFailure, "mutated WAL"):
                        BOUNDARIES.main(["--worker", sys.executable, "--report", str(report)])
                saved = json.loads(report.read_text())
                self.assertEqual(saved["status"], "failed")
                self.assertEqual(saved["results"], [])
                artifact = Path(saved["failure_artifact"])
                self.assertEqual(HARNESS.wal_snapshot(artifact / "before-recovery"), snapshot)
                self.assertEqual(HARNESS.wal_snapshot(artifact / "wal"),
                                 {"wal_000001.log": b"changed by recovery"})

    def test_boundary_snapshot_failure_stops_before_verifier(self):
        snapshot = {"wal_000001.log": b"damaged"}
        with tempfile.TemporaryDirectory() as directory:
            report = Path(directory) / "report.json"
            with patch.object(BOUNDARIES, "killed_fixture",
                              return_value=(snapshot, "wal_000001.log", 0)), \
                 patch.object(BOUNDARIES, "fault_cases",
                              return_value=iter([("disk_fault", 1, None, snapshot, None)])), \
                 patch("shutil.copytree", side_effect=OSError("snapshot unavailable")), \
                 patch.object(HARNESS, "run_verifier") as verifier, \
                 patch.object(HARNESS, "verify_stable_replay") as replay:
                with self.assertRaisesRegex(HARNESS.CrashIterationFailure, "snapshot unavailable"):
                    BOUNDARIES.main(["--worker", sys.executable, "--report", str(report)])
                verifier.assert_not_called()
                replay.assert_not_called()
            saved = json.loads(report.read_text())
            self.assertEqual(saved["status"], "failed")
            self.assertEqual(saved["results"], [])
            self.assertEqual(HARNESS.wal_snapshot(Path(saved["failure_artifact"]) / "wal"),
                             snapshot)

    def test_completed_iteration_is_persisted_before_next_iteration(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "report.json"

            def iteration(worker, index, acknowledged, pending, delay, failure_dir):
                if index == 2:
                    saved = json.loads(path.read_text())
                    self.assertEqual(saved["status"], "running")
                    self.assertEqual(saved["attempted_iteration"], 1)
                    self.assertEqual(len(saved["results"]), 1)
                    self.assertEqual(saved["results"][0]["verified_reopens"], 2)
                    self.assertIsNotNone(saved["harness_sha256"])
                    self.assertGreater(saved["elapsed_seconds"], 0)
                return HARNESS.IterationResult(index, acknowledged, 4, acknowledged, 480, 480, 2)

            with patch.object(HARNESS, "run_iteration", side_effect=iteration), \
                 patch.object(HARNESS.time, "monotonic", side_effect=[10.0, 11.0, 12.0, 13.0]):
                self.assertEqual(HARNESS.main(["--worker", sys.executable,
                                              "--iterations", "2", "--report", str(path)]), 0)
            self.assertEqual(json.loads(path.read_text())["status"], "passed")

    def test_progress_report_io_failure_stops_before_next_iteration(self):
        original_write = HARNESS.write_report
        failed_once = False

        def write_report(path, report):
            nonlocal failed_once
            if report.status == "running" and report.results and not failed_once:
                failed_once = True
                raise OSError("progress report disk full")
            original_write(path, report)

        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "report.json"
            result = HARNESS.IterationResult(1, 12, 4, 12, 480, 480, 2)
            with patch.object(HARNESS, "run_iteration", return_value=result) as run, \
                 patch.object(HARNESS, "write_report", side_effect=write_report):
                with self.assertRaisesRegex(OSError, "progress report disk full"):
                    HARNESS.main(["--worker", sys.executable,
                                  "--iterations", "2", "--report", str(path)])
            self.assertEqual(run.call_count, 1)
            saved = json.loads(path.read_text())
            self.assertEqual(saved["status"], "failed")
            self.assertEqual(len(saved["results"]), 1)
            self.assertIn("progress report disk full", saved["error"])

    def test_hard_parent_termination_leaves_running_validated_prefix(self):
        code = (
            "import importlib.util,sys,time\n"
            "spec=importlib.util.spec_from_file_location('harness',sys.argv[1])\n"
            "h=importlib.util.module_from_spec(spec)\n"
            "sys.modules[spec.name]=h\n"
            "spec.loader.exec_module(h)\n"
            "def iteration(worker,index,ack,pending,delay,failure_dir):\n"
            "    if index == 2:\n"
            "        print('SECOND_ITERATION',flush=True)\n"
            "        time.sleep(30)\n"
            "    return h.IterationResult(index,ack,4,ack,480,480,2)\n"
            "h.run_iteration=iteration\n"
            "h.main(['--worker',sys.executable,'--iterations','2','--report',sys.argv[2]])\n"
        )
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "report.json"
            with self.assertRaises(subprocess.TimeoutExpired) as raised:
                subprocess.run(
                    [sys.executable, "-c", code, str(SCRIPT.resolve()), str(path)],
                    capture_output=True, timeout=2, check=True,
                )
            self.assertIn(b"SECOND_ITERATION", raised.exception.stdout)
            saved = json.loads(path.read_text())
            self.assertEqual(saved["status"], "running")
            self.assertEqual(saved["attempted_iteration"], 1)
            self.assertEqual(len(saved["results"]), 1)
            self.assertEqual(saved["results"][0]["verified_reopens"], 2)
            self.assertIsNone(saved["error"])

    def test_report_round_trip_and_replacement(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "nested" / "result.json"
            report = HARNESS.CrashReport(worker="worker", requested_iterations=1)
            HARNESS.write_report(path, report)
            self.assertEqual(json.loads(path.read_text())["status"], "running")
            report.results.append(HARNESS.IterationResult(1, 12, 4, 12, 480, 480, 2))
            report.status = "passed"
            HARNESS.write_report(path, report)
            saved = json.loads(path.read_text())
            self.assertEqual(saved["status"], "passed")
            self.assertEqual(saved["scenario"], "wal_ack_boundary_process_kill")
            self.assertEqual(saved["results"], [
                {"iteration": 1, "acknowledged_entries": 12, "segment_count": 4,
                 "recovered_entries": 12, "unobserved_append_released": False,
                 "wal_bytes_before_recovery": 480, "wal_bytes_after_recovery": 480,
                 "verified_reopens": 2,
                 "kill_delay_seconds": 0.0}
            ])
            self.assertEqual(list(path.parent.iterdir()), [path])

    def test_failed_worker_replaces_stale_success_report(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "result.json"
            path.write_text('{"status": "passed"}', encoding="utf-8")
            with self.assertRaises(RuntimeError):
                HARNESS.main([
                    "--worker", sys.executable,
                    "--iterations", "1",
                    "--report", str(path),
                ])
            saved = json.loads(path.read_text())
            self.assertEqual(saved["status"], "failed")
            self.assertEqual(saved["requested_iterations"], 1)
            self.assertEqual(saved["attempted_iteration"], 1)
            self.assertEqual(saved["results"], [])
            self.assertIn("before APPEND_STARTED 1", saved["error"])
            self.assertEqual(saved["worker_sha256"],
                             hashlib.sha256(Path(sys.executable).read_bytes()).hexdigest())
            self.assertEqual(saved["harness_sha256"],
                             hashlib.sha256(SCRIPT.read_bytes()).hexdigest())
            artifact = Path(saved["failure_artifact"])
            self.assertTrue(artifact.is_dir())
            self.assertIn("before APPEND_STARTED 1", (artifact / "failure.txt").read_text())
            self.assertIn("can't open file", (artifact / "writer.log").read_text(encoding="utf-8"))
            self.assertEqual(json.loads((artifact / "reproduction.json").read_text())[
                "acknowledged_target"], 12)

    def test_fingerprint_read_failure_replaces_stale_success_without_starting_worker(self):
        with tempfile.TemporaryDirectory() as directory:
            report = Path(directory) / "report.json"
            report.write_text('{"status":"passed"}')
            with patch.object(HARNESS, "file_sha256", side_effect=OSError("fingerprint unreadable")), \
                 patch.object(HARNESS, "run_iteration") as run:
                with self.assertRaisesRegex(OSError, "fingerprint unreadable"):
                    HARNESS.main(["--worker", sys.executable, "--report", str(report)])
            run.assert_not_called()
            saved = json.loads(report.read_text())
            self.assertEqual(saved["status"], "failed")
            self.assertEqual(saved["attempted_iteration"], 0)
            self.assertIn("fingerprint unreadable", saved["error"])

    def test_changed_worker_cannot_produce_passed_report(self):
        with tempfile.TemporaryDirectory() as directory:
            worker = Path(directory) / "worker"
            worker.write_bytes(b"original")
            report = Path(directory) / "report.json"

            def iteration(*args):
                worker.write_bytes(b"changed")
                return HARNESS.IterationResult(1, 12, 4, 12, 480, 480, 2)

            with patch.object(HARNESS, "run_iteration", side_effect=iteration):
                with self.assertRaisesRegex(RuntimeError, "worker changed during"):
                    HARNESS.main(["--worker", str(worker), "--iterations", "1",
                                  "--report", str(report)])
            saved = json.loads(report.read_text())
            self.assertEqual(saved["status"], "failed")
            self.assertEqual(saved["worker_sha256"], hashlib.sha256(b"original").hexdigest())

    def test_changed_harness_cannot_produce_passed_report(self):
        with tempfile.TemporaryDirectory() as directory:
            report = Path(directory) / "report.json"
            result = HARNESS.IterationResult(1, 12, 4, 12, 480, 480, 2)
            with patch.object(HARNESS, "file_sha256",
                              side_effect=["worker", "harness", "worker", "changed"]), \
                 patch.object(HARNESS, "run_iteration", return_value=result):
                with self.assertRaisesRegex(RuntimeError, "harness changed during"):
                    HARNESS.main(["--worker", sys.executable, "--iterations", "1",
                                  "--report", str(report)])
            self.assertEqual(json.loads(report.read_text())["status"], "failed")

    def test_file_fingerprint_matches_multichunk_input(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "binary"
            content = b"\x00\xffpayload" * 250000
            path.write_bytes(content)
            self.assertEqual(HARNESS.file_sha256(path), hashlib.sha256(content).hexdigest())

    def test_reader_retains_all_output_including_unconsumed_lines(self):
        process = subprocess.Popen(
            [sys.executable, "-c",
             "import sys; print('startup', flush=True); "
             "print('ACK 1 1', flush=True); "
             "print('late diagnostic', file=sys.stderr, flush=True)"],
            stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True,
        )
        output = queue.Queue()
        transcript = []
        try:
            HARNESS.read_output(process, output, transcript)
            self.assertEqual(process.wait(timeout=15), 0)
            self.assertEqual(transcript, ["startup\n", "ACK 1 1\n", "late diagnostic\n"])
            self.assertEqual(list(output.queue), ["startup", "ACK 1 1", "late diagnostic", None])
        finally:
            if process.poll() is None:
                process.kill()
                process.wait(timeout=15)
            process.stdout.close()

    def test_writer_log_io_failure_preserves_writer_error_in_report(self):
        original_write = Path.write_text

        def write_text(path, *args, **kwargs):
            if path.name == "writer.log":
                raise OSError("writer log disk full")
            return original_write(path, *args, **kwargs)

        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "report.json"
            with patch.object(Path, "write_text", write_text):
                with self.assertRaises(HARNESS.CrashIterationFailure):
                    HARNESS.main(["--worker", sys.executable,
                                  "--iterations", "1", "--report", str(path)])
            saved = json.loads(path.read_text())
            self.assertEqual(saved["status"], "failed")
            self.assertIn("before APPEND_STARTED 1", saved["error"])
            self.assertIn("writer log disk full", saved["error"])
            self.assertTrue(Path(saved["failure_artifact"]).is_dir())

    def test_reader_errors_are_delivered_instead_of_ack_timeout(self):
        for error in (OSError("pipe read failed"),
                      UnicodeDecodeError("utf-8", b"\xff", 0, 1, "invalid byte")):
            with self.subTest(error=type(error).__name__):
                def lines():
                    yield "startup\n"
                    raise error

                output = queue.Queue()
                transcript = []
                process = SimpleNamespace(stdout=lines())
                HARNESS.read_output(process, output, transcript)
                self.assertEqual(transcript, ["startup\n"])
                with self.assertRaisesRegex(RuntimeError, "writer output reader failed") as raised:
                    HARNESS.wait_for_ack(output, process, 1)
                self.assertIs(raised.exception.__cause__, error)
                self.assertIsNone(output.get_nowait())

    def test_append_start_handshake_checks_the_expected_index(self):
        output = queue.Queue()
        output.put("APPEND_STARTED 2")
        HARNESS.wait_for_append_started(output, SimpleNamespace(), 2)

        output.put("APPEND_STARTED 3")
        with self.assertRaisesRegex(RuntimeError, "expected=2"):
            HARNESS.wait_for_append_started(output, SimpleNamespace(), 2)

    def test_reader_error_after_last_ack_fails_before_recovery(self):
        original_popen = subprocess.Popen

        def start_process(*args, **kwargs):
            return original_popen(
                [sys.executable, "-c", "import time; time.sleep(30)"], **kwargs
            )

        def failing_reader(process, output, transcript):
            transcript.extend(["APPEND_STARTED 1\n", "ACK 1 1\n"])
            output.put("APPEND_STARTED 1")
            output.put("ACK 1 1")
            output.put(OSError("late pipe error"))
            output.put(None)

        with tempfile.TemporaryDirectory() as directory:
            with patch.object(HARNESS.subprocess, "Popen", side_effect=start_process), \
                 patch.object(HARNESS, "read_output", side_effect=failing_reader), \
                 patch.object(HARNESS, "verify_stable_replay") as verify:
                with self.assertRaisesRegex(HARNESS.CrashIterationFailure, "late pipe error") as raised:
                    HARNESS.run_iteration(Path(sys.executable), 1, 1,
                                          failure_dir=Path(directory))
            verify.assert_not_called()
            artifact = raised.exception.artifact
            self.assertEqual(
                (artifact / "writer.log").read_text(),
                "APPEND_STARTED 1\nACK 1 1\n",
            )
            self.assertIn("late pipe error", (artifact / "failure.txt").read_text())

    def test_report_io_failure_is_not_success(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "not_a_directory"
            path.write_text("occupied", encoding="utf-8")
            report = HARNESS.CrashReport(worker="worker", requested_iterations=1)
            with self.assertRaises(OSError):
                HARNESS.write_report(path / "report.json", report)

    def test_failure_retains_original_and_modified_wal_without_overwrite(self):
        with tempfile.TemporaryDirectory() as directory:
            failure_dir = Path(directory) / "failures"
            destinations = []
            for _ in range(2):
                with self.assertRaises(HARNESS.CrashIterationFailure) as raised:
                    with HARNESS.iteration_workspace(1, failure_dir) as workspace:
                        (workspace / "before-recovery").mkdir()
                        (workspace / "before-recovery" / "wal_000001.log").write_bytes(b"rawtail")
                        (workspace / "wal").mkdir()
                        (workspace / "wal" / "wal_000001.log").write_bytes(b"raw")
                        raise RuntimeError("invalid recovery")
                artifact = raised.exception.artifact
                self.assertEqual((artifact / "before-recovery" / "wal_000001.log").read_bytes(), b"rawtail")
                self.assertEqual((artifact / "wal" / "wal_000001.log").read_bytes(), b"raw")
                self.assertEqual(workspace.resolve(), artifact)
                destinations.append(artifact)
            self.assertNotEqual(*destinations)

    def test_success_creates_no_failure_artifact(self):
        with tempfile.TemporaryDirectory() as directory:
            failure_dir = Path(directory) / "failures"
            with HARNESS.iteration_workspace(1, failure_dir) as workspace:
                (workspace / "data").write_bytes(b"test")
            self.assertFalse(workspace.exists())
            self.assertEqual(list(failure_dir.iterdir()), [])

    def test_artifact_io_failure_is_explicit(self):
        with tempfile.TemporaryDirectory() as directory:
            failure_dir = Path(directory) / "blocked"
            failure_dir.write_text("occupied", encoding="utf-8")
            with self.assertRaises(OSError):
                with HARNESS.iteration_workspace(1, failure_dir):
                    raise RuntimeError("recovery failure")

    def test_failure_does_not_require_copying_original_workspace(self):
        with tempfile.TemporaryDirectory() as directory:
            failure_dir = Path(directory) / "failures"
            with patch.object(HARNESS.shutil, "copytree",
                              side_effect=OSError("copy unavailable")):
                with self.assertRaises(HARNESS.CrashIterationFailure) as raised:
                    with HARNESS.iteration_workspace(1, failure_dir) as workspace:
                        (workspace / "wal").mkdir()
                        (workspace / "wal" / "wal_000001.log").write_bytes(b"original")
                        raise RuntimeError("recovery failure")
            self.assertEqual(raised.exception.artifact, workspace.resolve())
            self.assertEqual((workspace / "wal" / "wal_000001.log").read_bytes(), b"original")
            self.assertEqual(workspace.parent, failure_dir)

    def test_failure_marker_io_error_preserves_original_and_both_errors(self):
        original_write = Path.write_text

        def write_text(path, *args, **kwargs):
            if path.name == "failure.txt":
                raise OSError("diagnostic disk full")
            return original_write(path, *args, **kwargs)

        with tempfile.TemporaryDirectory() as directory:
            failure_dir = Path(directory) / "failures"
            with patch.object(Path, "write_text", write_text):
                with self.assertRaises(HARNESS.CrashIterationFailure) as raised:
                    with HARNESS.iteration_workspace(1, failure_dir) as workspace:
                        (workspace / "wal").mkdir()
                        (workspace / "wal" / "wal_000001.log").write_bytes(b"original")
                        raise RuntimeError("recovery failure")
            error = raised.exception
            self.assertEqual(error.artifact, workspace.resolve())
            self.assertEqual((workspace / "wal" / "wal_000001.log").read_bytes(), b"original")
            self.assertIn("recovery failure", str(error))
            self.assertIn("diagnostic disk full", str(error))
            self.assertEqual(error.retention_error, "diagnostic disk full")
            self.assertIsInstance(error.__cause__, RuntimeError)

    def test_failure_marker_io_error_is_in_failed_json_report(self):
        original_write = Path.write_text

        def write_text(path, *args, **kwargs):
            if path.name == "failure.txt":
                raise OSError("diagnostic disk full")
            return original_write(path, *args, **kwargs)

        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "report.json"
            with patch.object(Path, "write_text", write_text):
                with self.assertRaises(HARNESS.CrashIterationFailure):
                    HARNESS.main(["--worker", sys.executable,
                                  "--iterations", "1", "--report", str(path)])
            saved = json.loads(path.read_text())
            self.assertEqual(saved["status"], "failed")
            self.assertIn("before APPEND_STARTED 1", saved["error"])
            self.assertEqual(saved["retention_error"], "diagnostic disk full")
            artifact = Path(saved["failure_artifact"])
            self.assertTrue(artifact.is_dir())
            self.assertTrue((artifact / "reproduction.json").is_file())
            self.assertFalse((artifact / "failure.txt").exists())

    def test_failure_without_report_still_cleans_workspace(self):
        with self.assertRaisesRegex(RuntimeError, "recovery failure"):
            with HARNESS.iteration_workspace(1, None) as workspace:
                (workspace / "wal").write_bytes(b"original")
                raise RuntimeError("recovery failure")
        self.assertFalse(workspace.exists())

    def test_unobserved_append_accepts_only_exact_additional_entry(self):
        responses = [
            subprocess.CompletedProcess([], 6, "", "Expected 3 recovered entries, got 4\n"),
            subprocess.CompletedProcess([], 0, "VERIFIED 4\n", ""),
        ]
        with patch.object(HARNESS.subprocess, "run", side_effect=responses) as run:
            self.assertEqual(HARNESS.verify_replay(Path("worker"), Path("wal"), 3, True), 4)
            self.assertEqual(run.call_count, 2)
            self.assertEqual(run.call_args.args[0][-1], "4")

    def test_unobserved_append_may_be_absent(self):
        response = subprocess.CompletedProcess([], 0, "VERIFIED 3\n", "")
        with patch.object(HARNESS.subprocess, "run", return_value=response):
            self.assertEqual(HARNESS.verify_replay(Path("worker"), Path("wal"), 3, True), 3)

    def test_verifier_timeout_preserves_output_and_never_retries(self):
        for stdout, stderr in (
            (b"replaying segment 3\n\xff", b"wal.cpp:12:3: runtime error: invalid load"),
            ("replaying segment 3", "recovery stalled"),
            (None, None),
        ):
            with self.subTest(stdout=stdout, stderr=stderr):
                error = subprocess.TimeoutExpired(["worker"], 30, stdout, stderr)
                with patch.object(HARNESS.subprocess, "run", side_effect=error) as run:
                    with self.assertRaisesRegex(RuntimeError, "recovery timed out") as raised:
                        HARNESS.verify_replay(Path("worker"), Path("wal"), 3, True)
                self.assertIn(f"stdout={stdout!r}", str(raised.exception))
                self.assertIn(f"stderr={stderr!r}", str(raised.exception))
                self.assertIs(raised.exception.__cause__, error)
                self.assertEqual(run.call_count, 1)

    def test_additional_entry_retry_timeout_preserves_output(self):
        error = subprocess.TimeoutExpired(["worker"], 30, b"retry progress", b"retry stalled")
        responses = [
            subprocess.CompletedProcess([], 6, "", "Expected 3 recovered entries, got 4\n"),
            error,
        ]
        with patch.object(HARNESS.subprocess, "run", side_effect=responses):
            with self.assertRaisesRegex(RuntimeError, "retry stalled"):
                HARNESS.verify_replay(Path("worker"), Path("wal"), 3, True)

    def test_real_verifier_timeout_preserves_flushed_output(self):
        original_run = subprocess.run

        def run(*args, **kwargs):
            kwargs["timeout"] = 0.5
            return original_run(
                [sys.executable, "-c",
                 "import sys,time; print('replay started', flush=True); "
                 "print('recovery stalled', file=sys.stderr, flush=True); time.sleep(30)"],
                **kwargs,
            )

        with patch.object(HARNESS.subprocess, "run", side_effect=run):
            with self.assertRaisesRegex(RuntimeError, "recovery timed out") as raised:
                HARNESS.verify_replay(Path("worker"), Path("wal"), 3, False)
        self.assertIn("replay started", str(raised.exception))
        self.assertIn("recovery stalled", str(raised.exception))

    def test_timeout_preserves_wal_and_diagnostics_in_failed_report(self):
        def iteration(worker, index, acknowledged, pending, delay, failure_dir):
            with HARNESS.iteration_workspace(index, failure_dir) as workspace:
                wal = workspace / "wal"
                wal.mkdir()
                (wal / "wal_000001.log").write_bytes(b"unchanged")
                HARNESS.verify_replay(worker, wal, acknowledged, pending)

        error = subprocess.TimeoutExpired(["worker"], 30, b"replay progress", b"timeout detail")
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "report.json"
            with patch.object(HARNESS, "run_iteration", side_effect=iteration), \
                 patch.object(HARNESS.subprocess, "run", side_effect=error):
                with self.assertRaises(HARNESS.CrashIterationFailure):
                    HARNESS.main(["--worker", sys.executable, "--iterations", "1",
                                  "--report", str(path)])
            saved = json.loads(path.read_text())
            self.assertEqual(saved["status"], "failed")
            self.assertEqual(saved["results"], [])
            self.assertIn("replay progress", saved["error"])
            self.assertIn("timeout detail", saved["error"])
            artifact = Path(saved["failure_artifact"])
            self.assertEqual((artifact / "wal" / "wal_000001.log").read_bytes(), b"unchanged")
            self.assertIn("timeout detail", (artifact / "failure.txt").read_text())

    def test_sanitizer_diagnostics_reject_successful_verifier(self):
        diagnostics = [
            "==42==ERROR: AddressSanitizer: heap-buffer-overflow",
            "AddressSanitizer:DEADLYSIGNAL",
            "SUMMARY: UndefinedBehaviorSanitizer: undefined-behavior",
            "==42==ERROR: LeakSanitizer: detected memory leaks",
            "WARNING: ThreadSanitizer: data race",
            "WARNING: MemorySanitizer: use-of-uninitialized-value",
            "C:\\source\\wal.cpp:12:4: runtime error: signed integer overflow",
        ]
        for diagnostic in diagnostics:
            for channel in ("stdout", "stderr"):
                with self.subTest(diagnostic=diagnostic, channel=channel):
                    response = subprocess.CompletedProcess(
                        [], 0, diagnostic + "\nVERIFIED 3\n" if channel == "stdout"
                        else "VERIFIED 3\n",
                        diagnostic if channel == "stderr" else "",
                    )
                    with patch.object(HARNESS.subprocess, "run", return_value=response):
                        with self.assertRaisesRegex(RuntimeError, "sanitizer"):
                            HARNESS.verify_replay(Path("worker"), Path("wal"), 3, False)

    def test_sanitizer_on_count_mismatch_never_retries(self):
        response = subprocess.CompletedProcess(
            [], 6, "SUMMARY: UndefinedBehaviorSanitizer: undefined-behavior\n",
            "Expected 3 recovered entries, got 4\n",
        )
        with patch.object(HARNESS.subprocess, "run", return_value=response) as run:
            with self.assertRaisesRegex(RuntimeError, "sanitizer"):
                HARNESS.verify_replay(Path("worker"), Path("wal"), 3, True)
            self.assertEqual(run.call_count, 1)

    def test_sanitizer_on_additional_entry_retry_fails(self):
        responses = [
            subprocess.CompletedProcess([], 6, "", "Expected 3 recovered entries, got 4\n"),
            subprocess.CompletedProcess([], 0, "VERIFIED 4\n",
                                        "wal.cpp:10:2: runtime error: invalid load"),
        ]
        with patch.object(HARNESS.subprocess, "run", side_effect=responses):
            with self.assertRaisesRegex(RuntimeError, "sanitizer"):
                HARNESS.verify_replay(Path("worker"), Path("wal"), 3, True)

    def test_writer_sanitizer_before_ack_fails(self):
        output = queue.Queue()
        output.put("==42==ERROR: AddressSanitizer: heap-buffer-overflow")
        output.put("ACK 1 1")
        with self.assertRaisesRegex(RuntimeError, "writer sanitizer"):
            HARNESS.wait_for_ack(output, SimpleNamespace(), 1)

    def test_benign_sanitizer_mentions_do_not_fail_verifier(self):
        response = subprocess.CompletedProcess(
            [], 0, "AddressSanitizer enabled\nVERIFIED 3\n",
            "startup: runtime error: unavailable optional diagnostics\n",
        )
        with patch.object(HARNESS.subprocess, "run", return_value=response):
            self.assertEqual(HARNESS.verify_replay(Path("worker"), Path("wal"), 3, False), 3)

    def test_late_writer_sanitizer_preserves_log_and_prevents_recovery(self):
        original_popen = subprocess.Popen
        diagnostic = "wal.cpp:12:3: runtime error: invalid load\n"

        def start_process(*args, **kwargs):
            return original_popen(
                [sys.executable, "-c", "import time; time.sleep(30)"], **kwargs
            )

        def reader(process, output, transcript):
            transcript.extend(["APPEND_STARTED 1\n", "ACK 1 1\n", diagnostic])
            output.put("APPEND_STARTED 1")
            output.put("ACK 1 1")
            output.put(diagnostic.rstrip())
            output.put(None)

        with tempfile.TemporaryDirectory() as directory:
            with patch.object(HARNESS.subprocess, "Popen", side_effect=start_process), \
                 patch.object(HARNESS, "read_output", side_effect=reader), \
                 patch.object(HARNESS, "verify_stable_replay") as verify:
                with self.assertRaisesRegex(HARNESS.CrashIterationFailure, "writer sanitizer") as raised:
                    HARNESS.run_iteration(Path(sys.executable), 1, 1,
                                          failure_dir=Path(directory))
            verify.assert_not_called()
            artifact = raised.exception.artifact
            self.assertEqual(
                (artifact / "writer.log").read_text(),
                "APPEND_STARTED 1\nACK 1 1\n" + diagnostic,
            )
            self.assertIn("invalid load", (artifact / "failure.txt").read_text())

    def test_real_zero_exit_subprocess_with_sanitizer_text_is_rejected(self):
        original_run = subprocess.run

        def run(*args, **kwargs):
            return original_run(
                [sys.executable, "-c",
                 "import sys; print('VERIFIED 3'); "
                 "print('wal.cpp:12:3: runtime error: invalid load', file=sys.stderr)"],
                **kwargs,
            )

        with patch.object(HARNESS.subprocess, "run", side_effect=run):
            with self.assertRaisesRegex(RuntimeError, "verifier sanitizer"):
                HARNESS.verify_replay(Path("worker"), Path("wal"), 3, False)

    def test_corruption_missing_ack_and_extra_entries_never_retry(self):
        cases = [
            (5, "WAL recovery failed: corrupt"),
            (6, "Expected 3 recovered entries, got 2"),
            (6, "Expected 3 recovered entries, got 5"),
            (7, "Recovered entry mismatch at index 3"),
        ]
        for code, error in cases:
            with self.subTest(code=code, error=error):
                response = subprocess.CompletedProcess([], code, "", error)
                with patch.object(HARNESS.subprocess, "run", return_value=response) as run:
                    with self.assertRaises(RuntimeError):
                        HARNESS.verify_replay(Path("worker"), Path("wal"), 3, True)
                    self.assertEqual(run.call_count, 1)

    def test_ack_boundary_rejects_even_one_extra_entry(self):
        response = subprocess.CompletedProcess([], 6, "", "Expected 3 recovered entries, got 4")
        with patch.object(HARNESS.subprocess, "run", return_value=response) as run:
            with self.assertRaises(RuntimeError):
                HARNESS.verify_replay(Path("worker"), Path("wal"), 3, False)
            self.assertEqual(run.call_count, 1)

    def test_additional_entry_content_failure_is_not_accepted(self):
        responses = [
            subprocess.CompletedProcess([], 6, "", "Expected 3 recovered entries, got 4"),
            subprocess.CompletedProcess([], 7, "", "Recovered entry mismatch at index 4"),
        ]
        with patch.object(HARNESS.subprocess, "run", side_effect=responses):
            with self.assertRaises(RuntimeError):
                HARNESS.verify_replay(Path("worker"), Path("wal"), 3, True)

    def test_repeated_replay_locks_the_first_recovered_count(self):
        with tempfile.TemporaryDirectory() as directory:
            wal = Path(directory)
            (wal / "wal_000001.log").write_bytes(b"record")
            with patch.object(HARNESS, "verify_replay", side_effect=[4, 4]) as verify:
                self.assertEqual(
                    HARNESS.verify_stable_replay(Path("worker"), wal, 3, True),
                    (4, 6, 6),
                )
                self.assertEqual(verify.call_args_list[1].args,
                                 (Path("worker"), wal, 4, False))

    def test_first_replay_tail_repair_is_recorded(self):
        with tempfile.TemporaryDirectory() as directory:
            wal = Path(directory)
            segment = wal / "wal_000001.log"
            segment.write_bytes(b"recordtail")

            def replay(*args):
                segment.write_bytes(b"record")
                return 3

            with patch.object(HARNESS, "verify_replay", side_effect=replay):
                self.assertEqual(
                    HARNESS.verify_stable_replay(Path("worker"), wal, 3, True),
                    (3, 10, 6),
                )

    def test_second_replay_cannot_mutate_same_size_bytes_or_segment_set(self):
        for mutation in ("content", "truncate", "add", "delete"):
            with self.subTest(mutation=mutation), tempfile.TemporaryDirectory() as directory:
                wal = Path(directory)
                segment = wal / "wal_000001.log"
                segment.write_bytes(b"record")
                calls = 0

                def replay(*args):
                    nonlocal calls
                    calls += 1
                    if calls == 2:
                        if mutation == "content":
                            segment.write_bytes(b"change")
                        elif mutation == "truncate":
                            segment.write_bytes(b"rec")
                        elif mutation == "add":
                            (wal / "wal_000002.log").write_bytes(b"")
                        else:
                            segment.unlink()
                    return 3

                with patch.object(HARNESS, "verify_replay", side_effect=replay):
                    with self.assertRaisesRegex(RuntimeError, "changed WAL"):
                        HARNESS.verify_stable_replay(Path("worker"), wal, 3, False)

    def test_repeated_replay_count_change_and_empty_wal_fail(self):
        with tempfile.TemporaryDirectory() as directory:
            wal = Path(directory)
            with self.assertRaisesRegex(RuntimeError, "no WAL segments"):
                HARNESS.verify_stable_replay(Path("worker"), wal, 3, False)
            (wal / "wal_000001.log").write_bytes(b"record")
            with patch.object(HARNESS, "verify_replay", side_effect=[3, 4]):
                with self.assertRaisesRegex(RuntimeError, "entry count"):
                    HARNESS.verify_stable_replay(Path("worker"), wal, 3, False)

    def test_first_replay_cannot_change_older_segments_or_retained_bytes(self):
        for mutation in ("older", "rewrite", "grow", "add", "delete", "ack-tail"):
            with self.subTest(mutation=mutation), tempfile.TemporaryDirectory() as directory:
                wal = Path(directory)
                older = wal / "wal_000001.log"
                newest = wal / "wal_000002.log"
                older.write_bytes(b"older")
                newest.write_bytes(b"recordtail")

                def replay(*args):
                    if mutation == "older":
                        older.write_bytes(b"old")
                    elif mutation == "rewrite":
                        newest.write_bytes(b"change")
                    elif mutation == "grow":
                        newest.write_bytes(b"recordtailmore")
                    elif mutation == "add":
                        (wal / "wal_000003.log").write_bytes(b"")
                    elif mutation == "delete":
                        older.unlink(missing_ok=True)
                    else:
                        newest.write_bytes(b"record")
                    return 3

                with patch.object(HARNESS, "verify_replay", side_effect=replay) as verify:
                    with self.assertRaisesRegex(RuntimeError, "first recovery"):
                        HARNESS.verify_stable_replay(
                            Path("worker"), wal, 3, mutation != "ack-tail"
                        )
                    self.assertEqual(verify.call_count, 1)

    def test_latest_tail_permission_uses_numeric_segment_order(self):
        before = {
            "wal_999999.log": b"older",
            "wal_1000000.log": b"recordtail",
        }
        after = {**before, "wal_1000000.log": b"record"}
        HARNESS.validate_first_recovery(before, after, True)
        with self.assertRaisesRegex(RuntimeError, "first recovery"):
            HARNESS.validate_first_recovery(
                before, {**before, "wal_999999.log": b"old"}, True
            )


if __name__ == "__main__":
    unittest.main()
