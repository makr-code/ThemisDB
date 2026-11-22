#!/usr/bin/env python3
"""Inject deterministic disk faults after a real WAL writer kill, not inside syscalls."""

from __future__ import annotations

import argparse
from collections.abc import Iterator
from dataclasses import dataclass, field
from pathlib import Path
import queue
import shutil
import subprocess
import threading
import time
import json

import test_wal_process_crash as crash


@dataclass
class BoundaryReport:
    worker: str
    scenario: str = "killed_writer_deterministic_tail_fault_injection"
    status: str = "running"
    worker_sha256: str | None = None
    harness_sha256: str | None = None
    shared_harness_sha256: str | None = None
    fixture_acknowledged_entries: int = 12
    protected_ack_prefix: int = 11
    final_frame_start: int = 0
    final_frame_bytes: int = 0
    elapsed_seconds: float = 0.0
    error: str | None = None
    failure_artifact: str | None = None
    results: list[dict[str, str | int | bool]] = field(default_factory=list)


def killed_fixture(worker: Path, workspace: Path) -> tuple[dict[str, bytes], str, int]:
    wal = workspace / "wal"
    process = subprocess.Popen(
        [str(worker), "write", str(wal), "12"], stdin=subprocess.PIPE,
        stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True,
    )
    output: queue.Queue[crash.WriterOutput] = queue.Queue()
    transcript: list[str] = []
    reader = threading.Thread(target=crash.read_output, args=(process, output, transcript), daemon=True)
    reader.start()
    prefix: dict[str, bytes] = {}
    try:
        for index in range(1, 13):
            crash.wait_for_append_started(output, process, index)
            crash.wait_for_ack(output, process, index)
            if index == 11:
                prefix = crash.wal_snapshot(wal)
            if index < 12:
                assert process.stdin is not None
                process.stdin.write("continue\n")
                process.stdin.flush()
        process.kill()
        if process.wait(timeout=15) == 0:
            raise RuntimeError("fixture writer was not forcibly terminated")
    finally:
        if process.poll() is None:
            process.kill()
            process.wait(timeout=15)
        if process.stdin is not None:
            process.stdin.close()
        reader.join(timeout=15)
        if reader.is_alive():
            raise RuntimeError("fixture writer reader did not stop")
        if process.stdout is not None:
            process.stdout.close()
        (workspace / "writer.log").write_text("".join(transcript), encoding="utf-8")
    crash.check_sanitizer_output("".join(transcript), "writer")
    while not output.empty():
        item = output.get_nowait()
        if isinstance(item, (OSError, UnicodeError)):
            raise RuntimeError(f"fixture reader failed: {item}") from item
    complete = crash.wal_snapshot(wal)
    if not complete:
        raise RuntimeError("fixture produced no WAL")
    latest = max(complete, key=lambda name: int(name[4:-4]))
    boundary = len(prefix.get(latest, b""))
    if (
        len(complete) != 4 or boundary != 78 or len(complete[latest]) - boundary != 39
        or any(len(data) != 111 for name, data in complete.items() if name != latest)
        or not complete[latest].startswith(prefix.get(latest, b""))
        or any(complete[name] != data for name, data in prefix.items() if name != latest)
    ):
        raise RuntimeError("fixture layout changed; review deterministic fault boundaries")
    return complete, latest, boundary


def fault_cases(
    complete: dict[str, bytes], latest: str, boundary: int,
) -> Iterator[tuple[str, int, int | None, dict[str, bytes], int | None]]:
    for retained in range(40):
        snapshot = dict(complete)
        snapshot[latest] = snapshot[latest][:boundary + retained]
        yield "latest_tail_prefix", retained, None, snapshot, 12 if retained == 39 else 11
    for offset in range(21, 39):
        for bit in range(8):
            snapshot = dict(complete)
            modified = bytearray(snapshot[latest])
            modified[boundary + offset] ^= 1 << bit
            snapshot[latest] = bytes(modified)
            yield "complete_final_frame_payload_or_crc_bit_flip", offset, bit, snapshot, None
    for older in sorted(complete, key=lambda name: int(name[4:-4]))[:-1]:
        for removed in range(1, 37):
            snapshot = dict(complete)
            snapshot[older] = snapshot[older][:-removed]
            yield "older_segment_incomplete_frame", removed, None, snapshot, None
    for name in sorted(complete, key=lambda name: int(name[4:-4])):
        frame_bytes = 39 if name == latest else 37
        for start in range(0, len(complete[name]), frame_bytes):
            if name == latest and start == boundary:
                continue
            for offset in range(start + 21, start + frame_bytes):
                for bit in range(8):
                    snapshot = dict(complete)
                    modified = bytearray(snapshot[name])
                    modified[offset] ^= 1 << bit
                    snapshot[name] = bytes(modified)
                    yield "earlier_complete_frame_payload_or_crc_bit_flip", offset, bit, snapshot, None
    for name in sorted(complete, key=lambda name: int(name[4:-4])):
        frame_bytes = 39 if name == latest else 37
        for start in range(0, len(complete[name]), frame_bytes):
            for offset in range(start, start + 13):
                for bit in range(8):
                    snapshot = dict(complete)
                    modified = bytearray(snapshot[name])
                    modified[offset] ^= 1 << bit
                    snapshot[name] = bytes(modified)
                    yield "complete_frame_magic_sequence_type_bit_flip", offset, bit, snapshot, None


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--worker", type=Path, required=True)
    parser.add_argument("--report", type=Path, required=True)
    args = parser.parse_args(argv)
    if not args.worker.is_file():
        parser.error(f"worker does not exist: {args.worker}")
    worker = args.worker.resolve()
    report = BoundaryReport(worker=str(worker))
    root = args.report.parent / f"{args.report.stem}-failures"
    started = time.monotonic()
    crash.write_report(args.report, report)
    try:
        report.worker_sha256 = crash.file_sha256(worker)
        report.harness_sha256 = crash.file_sha256(Path(__file__))
        report.shared_harness_sha256 = crash.file_sha256(Path(crash.__file__))
        with crash.iteration_workspace(0, root) as workspace:
            complete, latest, boundary = killed_fixture(worker, workspace)
        report.final_frame_start, report.final_frame_bytes = boundary, 39
        for index, (kind, offset, bit, snapshot, expected) in enumerate(
            fault_cases(complete, latest, boundary), 1
        ):
            with crash.iteration_workspace(index, root) as workspace:
                wal = workspace / "wal"
                wal.mkdir()
                for name, data in snapshot.items():
                    (wal / name).write_bytes(data)
                affected = next(
                    (name for name in complete if snapshot[name] != complete[name]), latest
                )
                reproduction: dict[str, str | int | None] = {
                    "kind": kind, "offset": offset, "expected_entries": expected,
                    "affected_segment": affected,
                }
                if bit is not None:
                    reproduction["bit_index"] = bit
                segment_byte_offset = None
                if kind == "complete_final_frame_payload_or_crc_bit_flip":
                    segment_byte_offset = boundary + offset
                elif kind in (
                    "earlier_complete_frame_payload_or_crc_bit_flip",
                    "complete_frame_magic_sequence_type_bit_flip",
                ):
                    segment_byte_offset = offset
                if kind in (
                    "complete_final_frame_payload_or_crc_bit_flip",
                    "earlier_complete_frame_payload_or_crc_bit_flip",
                    "complete_frame_magic_sequence_type_bit_flip",
                ):
                    reproduction["segment_byte_offset"] = segment_byte_offset
                (workspace / "reproduction.json").write_text(
                    json.dumps(reproduction),
                    encoding="utf-8",
                )
                shutil.copytree(wal, workspace / "before-recovery")
                if expected is not None:
                    recovered, before, after = crash.verify_stable_replay(worker, wal, 11, True)
                    repaired = dict(complete)
                    if expected == 11:
                        repaired[latest] = complete[latest][:boundary]
                    if recovered != expected or crash.wal_snapshot(wal) != repaired:
                        raise RuntimeError(f"incorrect tail repair: {kind}, offset={offset}")
                    result: dict[str, str | int | bool] = {
                        "kind": kind, "offset": offset, "recovered_entries": recovered,
                        "verified_reopens": 2, "bytes_before": before, "bytes_after": after,
                        "verifier_processes": 3 if expected == 12 else 2,
                    }
                else:
                    for _ in range(2):
                        verifier = crash.run_verifier(worker, wal, 12)
                        if (
                            verifier.returncode != 5
                            or "WAL recovery failed:" not in verifier.stderr
                            or crash.wal_snapshot(wal) != snapshot
                        ):
                            raise RuntimeError(
                                f"fault did not fail closed: {kind}, offset={offset}; "
                                f"exit={verifier.returncode}; {verifier.stdout}{verifier.stderr}"
                            )
                    result = {
                        "kind": kind, "offset": offset, "failed_open_attempts": 2,
                        "verifier_processes": 2, "recovery_exit": 5, "wal_unchanged": True,
                    }
                result["affected_segment"] = affected
                if segment_byte_offset is not None:
                    result["segment_byte_offset"] = segment_byte_offset
                if bit is not None:
                    result["bit_index"] = bit
                report.results.append(result)
            report.elapsed_seconds = round(time.monotonic() - started, 3)
            crash.write_report(args.report, report)
        if len(report.results) != 2980:
            raise RuntimeError("deterministic campaign did not execute all 2980 cases")
        for path, fingerprint in (
            (worker, report.worker_sha256),
            (Path(__file__), report.harness_sha256),
            (Path(crash.__file__), report.shared_harness_sha256),
        ):
            if crash.file_sha256(path) != fingerprint:
                raise RuntimeError(f"campaign input changed during execution: {path}")
        report.status = "passed"
    except (RuntimeError, OSError, ValueError, subprocess.SubprocessError) as exc:
        report.error = str(exc)
        if isinstance(exc, crash.CrashIterationFailure):
            report.failure_artifact = str(exc.artifact)
        raise
    finally:
        report.elapsed_seconds = round(time.monotonic() - started, 3)
        if report.status != "passed":
            report.status = "failed"
            if report.error is None:
                report.error = "Campaign interrupted before completion"
        crash.write_report(args.report, report)
    print("PASS: 2980 deterministic WAL disk faults; 80 successful reopens, 5880 closed failures")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
