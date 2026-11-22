#!/usr/bin/env python3
"""Exercise WAL recovery after the writer process is forcibly terminated."""

from __future__ import annotations

import argparse
from collections.abc import Iterator
from contextlib import contextmanager
from dataclasses import asdict, dataclass, field
import hashlib
import json
import os
import queue
import random
import re
import shutil
import subprocess
import tempfile
import threading
import time
from pathlib import Path


WriterOutput = str | None | OSError | UnicodeError


SANITIZER_DIAGNOSTIC = re.compile(
    r"(?:ERROR|WARNING|SUMMARY): "
    r"(?:AddressSanitizer|LeakSanitizer|UndefinedBehaviorSanitizer|ThreadSanitizer|MemorySanitizer):"
    r"|AddressSanitizer:DEADLYSIGNAL"
    r"|^.+:\d+(?::\d+)?: runtime error:",
    re.MULTILINE,
)


def check_sanitizer_output(output: str, role: str) -> None:
    for line in output.splitlines():
        if SANITIZER_DIAGNOSTIC.search(line):
            raise RuntimeError(f"{role} sanitizer diagnostic: {line.strip()}")


@dataclass
class IterationResult:
    iteration: int
    acknowledged_entries: int
    segment_count: int
    recovered_entries: int
    wal_bytes_before_recovery: int
    wal_bytes_after_recovery: int
    verified_reopens: int
    unobserved_append_released: bool = False
    kill_delay_seconds: float = 0.0


@dataclass
class CrashReport:
    worker: str
    requested_iterations: int
    seed: int = 0x5EED
    scenario: str = "wal_ack_boundary_process_kill"
    status: str = "running"
    attempted_iteration: int = 0
    elapsed_seconds: float = 0.0
    error: str | None = None
    failure_artifact: str | None = None
    retention_error: str | None = None
    worker_sha256: str | None = None
    harness_sha256: str | None = None
    results: list[IterationResult] = field(default_factory=list)


def file_sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def write_report(path: Path, report: CrashReport) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary_path: Path | None = None
    try:
        with tempfile.NamedTemporaryFile(
            mode="w", encoding="utf-8", dir=path.parent, delete=False
        ) as stream:
            temporary_path = Path(stream.name)
            json.dump(asdict(report), stream, indent=2)
            stream.write("\n")
        os.replace(temporary_path, path)
    finally:
        if temporary_path is not None:
            temporary_path.unlink(missing_ok=True)


class CrashIterationFailure(RuntimeError):
    def __init__(self, message: str, artifact: Path, retention_error: str | None = None):
        if retention_error is not None:
            message += f"; failed to write failure diagnostics: {retention_error}"
        super().__init__(message)
        self.artifact = artifact
        self.retention_error = retention_error


@contextmanager
def iteration_workspace(iteration: int, failure_dir: Path | None) -> Iterator[Path]:
    if failure_dir is not None:
        failure_dir.mkdir(parents=True, exist_ok=True)
    workspace = Path(tempfile.mkdtemp(
        prefix=f"iteration-{iteration}-", dir=failure_dir
    ))
    retain = False
    try:
        yield workspace
    except (RuntimeError, OSError, ValueError, subprocess.SubprocessError) as exc:
        if failure_dir is None:
            raise
        retain = True
        retention_error = None
        try:
            (workspace / "failure.txt").write_text(str(exc) + "\n", encoding="utf-8")
        except OSError as diagnostic_error:
            retention_error = str(diagnostic_error)
        raise CrashIterationFailure(
            str(exc), workspace.resolve(), retention_error
        ) from exc
    finally:
        if not retain:
            shutil.rmtree(workspace)


def read_output(
    process: subprocess.Popen[str], output: queue.Queue[WriterOutput],
    transcript: list[str],
) -> None:
    assert process.stdout is not None
    try:
        for line in process.stdout:
            transcript.append(line)
            output.put(line.rstrip("\r\n"))
    except (OSError, UnicodeError) as exc:
        output.put(exc)
    finally:
        output.put(None)


def wait_for_ack(
    output: queue.Queue[WriterOutput],
    process: subprocess.Popen[str],
    expected_index: int,
) -> None:
    deadline = time.monotonic() + 15
    while True:
        try:
            line = output.get(timeout=max(0.0, deadline - time.monotonic()))
        except queue.Empty as exc:
            raise RuntimeError(f"writer timed out before ACK {expected_index}") from exc
        if isinstance(line, (OSError, UnicodeError)):
            raise RuntimeError(f"writer output reader failed: {line}") from line
        if line is None:
            return_code = process.wait(timeout=15)
            raise RuntimeError(
                f"writer exited before ACK {expected_index}; exit code={return_code}"
            )
        check_sanitizer_output(line, "writer")
        if line.startswith("WAL open failed:") or line.startswith("WAL append failed:"):
            raise RuntimeError(f"writer failed before ACK {expected_index}: {line}")
        if line.startswith("ACK "):
            break

    parts = line.split()
    if len(parts) != 3 or parts[0] != "ACK":
        raise RuntimeError(f"unexpected writer output: {line}")
    index, sequence = int(parts[1]), int(parts[2])
    if index != expected_index or sequence != expected_index:
        raise RuntimeError(
            f"unexpected ACK ordering: index={index}, sequence={sequence}, "
            f"expected={expected_index}"
        )


def wait_for_append_started(
    output: queue.Queue[WriterOutput],
    process: subprocess.Popen[str],
    expected_index: int,
) -> None:
    deadline = time.monotonic() + 15
    while True:
        try:
            line = output.get(timeout=max(0.0, deadline - time.monotonic()))
        except queue.Empty as exc:
            raise RuntimeError(
                f"writer timed out before APPEND_STARTED {expected_index}"
            ) from exc
        if isinstance(line, (OSError, UnicodeError)):
            raise RuntimeError(f"writer output reader failed: {line}") from line
        if line is None:
            return_code = process.wait(timeout=15)
            raise RuntimeError(
                f"writer exited before APPEND_STARTED {expected_index}; "
                f"exit code={return_code}"
            )
        check_sanitizer_output(line, "writer")
        if line.startswith("WAL open failed:") or line.startswith("WAL append failed:"):
            raise RuntimeError(
                f"writer failed before APPEND_STARTED {expected_index}: {line}"
            )
        if line.startswith("ACK "):
            raise RuntimeError(
                f"writer acknowledged before APPEND_STARTED {expected_index}: {line}"
            )
        if not line.startswith("APPEND_STARTED "):
            continue
        parts = line.split()
        if len(parts) != 2:
            raise RuntimeError(f"invalid APPEND_STARTED output: {line}")
        try:
            index = int(parts[1])
        except ValueError as exc:
            raise RuntimeError(f"invalid APPEND_STARTED output: {line}") from exc
        if index != expected_index:
            raise RuntimeError(
                f"unexpected append start ordering: index={index}, "
                f"expected={expected_index}"
            )
        return


def run_verifier(worker: Path, wal_dir: Path, expected: int) -> subprocess.CompletedProcess[str]:
    try:
        verifier = subprocess.run(
            [str(worker), "verify", str(wal_dir), str(expected)],
            capture_output=True, text=True, timeout=30, check=False,
        )
    except subprocess.TimeoutExpired as exc:
        raise RuntimeError(
            f"recovery timed out after {exc.timeout} seconds (expected={expected}); "
            f"stdout={exc.stdout!r}; stderr={exc.stderr!r}"
        ) from exc
    check_sanitizer_output(verifier.stdout, "verifier")
    check_sanitizer_output(verifier.stderr, "verifier")
    return verifier


def verify_replay(worker: Path, wal_dir: Path, acknowledged: int, allow_pending: bool) -> int:
    verifier = run_verifier(worker, wal_dir, acknowledged)
    expected = acknowledged
    # Exit 6 means replay succeeded but its entry count differed. Only the
    # single released append may explain that difference; all other errors fail.
    if (
        allow_pending
        and verifier.returncode == 6
        and verifier.stderr.strip()
        == f"Expected {acknowledged} recovered entries, got {acknowledged + 1}"
    ):
        expected += 1
        verifier = run_verifier(worker, wal_dir, expected)
    if verifier.returncode != 0:
        raise RuntimeError(
            f"recovery failed (exit={verifier.returncode}): "
            f"{verifier.stdout}{verifier.stderr}"
        )
    lines = verifier.stdout.splitlines()
    if not lines or lines[-1] != f"VERIFIED {expected}":
        raise RuntimeError(f"unexpected verifier output: {verifier.stdout.strip()}")
    return expected


def wal_snapshot(wal_dir: Path) -> dict[str, bytes]:
    return {path.name: path.read_bytes() for path in wal_dir.glob("wal_*.log")}


def validate_first_recovery(
    before: dict[str, bytes], after: dict[str, bytes], allow_pending: bool,
) -> None:
    if before.keys() != after.keys():
        raise RuntimeError("first recovery changed WAL segment names")
    latest = max(before, key=lambda name: int(name[4:-4]))
    for name, original in before.items():
        recovered = after[name]
        if recovered == original:
            continue
        if (
            not allow_pending or name != latest or len(recovered) >= len(original)
            or not original.startswith(recovered)
        ):
            raise RuntimeError(
                f"first recovery changed WAL data outside the permitted latest tail: {name}"
            )


def verify_stable_replay(
    worker: Path, wal_dir: Path, acknowledged: int, allow_pending: bool,
) -> tuple[int, int, int]:
    before = wal_snapshot(wal_dir)
    if not before:
        raise RuntimeError("no WAL segments found after process termination")
    recovered = verify_replay(worker, wal_dir, acknowledged, allow_pending)
    after = wal_snapshot(wal_dir)
    validate_first_recovery(before, after, allow_pending)
    repeated = verify_replay(worker, wal_dir, recovered, False)
    if repeated != recovered:
        raise RuntimeError("repeated recovery changed the recovered entry count")
    if wal_snapshot(wal_dir) != after:
        raise RuntimeError("repeated recovery changed WAL segment names or contents")
    return recovered, sum(len(data) for data in before.values()), sum(
        len(data) for data in after.values()
    )


def run_iteration(
    worker: Path, iteration: int, kill_after: int,
    release_pending: bool = False, kill_delay: float = 0.0,
    failure_dir: Path | None = None,
) -> IterationResult:
    with iteration_workspace(iteration, failure_dir) as workspace:
        wal_dir = workspace / "wal"
        (workspace / "reproduction.json").write_text(json.dumps({
            "worker": str(worker), "iteration": iteration,
            "acknowledged_target": kill_after,
            "unobserved_append_released": release_pending,
            "kill_delay_seconds": kill_delay,
        }, indent=2) + "\n", encoding="utf-8")
        process = subprocess.Popen(
            [str(worker), "write", str(wal_dir), "12"],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            bufsize=1,
        )
        output: queue.Queue[WriterOutput] = queue.Queue()
        transcript: list[str] = []
        reader = threading.Thread(
            target=read_output, args=(process, output, transcript), daemon=True
        )
        reader.start()
        writer_error: Exception | None = None
        try:
            for index in range(1, kill_after + 1):
                wait_for_append_started(output, process, index)
                wait_for_ack(output, process, index)
                if index < kill_after:
                    assert process.stdin is not None
                    process.stdin.write("continue\n")
                    process.stdin.flush()

            if release_pending:
                assert process.stdin is not None
                process.stdin.write("continue\n")
                process.stdin.flush()
                wait_for_append_started(output, process, kill_after + 1)
                if kill_delay > 0:
                    time.sleep(kill_delay)

            if process.poll() is not None:
                raise RuntimeError("writer exited before forced termination")
            process.kill()
            return_code = process.wait(timeout=15)
            if return_code == 0:
                raise RuntimeError("writer exited successfully instead of being killed")
        except (RuntimeError, OSError, ValueError, subprocess.SubprocessError) as exc:
            writer_error = exc
            raise
        finally:
            if process.poll() is None:
                process.kill()
                process.wait(timeout=15)
            if process.stdin is not None:
                process.stdin.close()
            reader.join(timeout=15)
            if reader.is_alive():
                raise RuntimeError("writer output reader did not stop after termination")
            if process.stdout is not None:
                process.stdout.close()
            reader_error: OSError | UnicodeError | None = None
            late_writer_error: RuntimeError | None = None
            # A released append can fail after its start marker but before the kill.
            while True:
                try:
                    remaining = output.get_nowait()
                except queue.Empty:
                    break
                if isinstance(remaining, (OSError, UnicodeError)):
                    reader_error = remaining
                elif isinstance(remaining, str) and (
                    remaining.startswith("WAL open failed:")
                    or remaining.startswith("WAL append failed:")
                ):
                    if late_writer_error is None:
                        late_writer_error = RuntimeError(f"writer failed: {remaining}")
            if late_writer_error is not None:
                if writer_error is not None:
                    writer_error = RuntimeError(f"{writer_error}; {late_writer_error}")
                else:
                    writer_error = late_writer_error
            if failure_dir is not None:
                try:
                    (workspace / "writer.log").write_text(
                        "".join(transcript), encoding="utf-8"
                    )
                except OSError as log_error:
                    if writer_error is not None:
                        raise RuntimeError(
                            f"{writer_error}; writer transcript could not be saved: {log_error}"
                        ) from log_error
                    raise
            if reader_error is not None:
                detail = f"writer output reader failed: {reader_error}"
                if writer_error is not None:
                    detail = f"{writer_error}; {detail}"
                raise RuntimeError(detail) from reader_error
            try:
                check_sanitizer_output("".join(transcript), "writer")
            except RuntimeError as sanitizer_error:
                if writer_error is not None:
                    raise RuntimeError(f"{writer_error}; {sanitizer_error}") from sanitizer_error
                raise
            if late_writer_error is not None:
                assert writer_error is not None
                raise writer_error

        segments = list(wal_dir.glob("wal_*.log"))
        if failure_dir is not None:
            shutil.copytree(wal_dir, workspace / "before-recovery")
        recovered, bytes_before, bytes_after = verify_stable_replay(
            worker, wal_dir, kill_after, release_pending
        )
        return IterationResult(
            iteration, kill_after, len(segments), recovered,
            bytes_before, bytes_after, 2,
            release_pending, kill_delay,
        )


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--worker", type=Path, required=True)
    parser.add_argument("--iterations", type=int, default=50)
    parser.add_argument("--report", type=Path, help=(
        "JSON result path; failed iterations retain evidence beside it in "
        "<report-stem>-failures/iteration-<number>-<unique-id>"
    ))
    parser.add_argument("--scenario", choices=("ack-boundary", "unobserved-append"),
                        default="ack-boundary")
    args = parser.parse_args(argv)
    if args.iterations < 1:
        parser.error("--iterations must be positive")
    if not args.worker.is_file():
        parser.error(f"worker executable does not exist: {args.worker}")

    report = CrashReport(
        worker=str(args.worker.resolve()), requested_iterations=args.iterations
    )
    release_pending = args.scenario == "unobserved-append"
    if release_pending:
        report.scenario = "wal_unobserved_append_process_kill"
    if args.report is not None:
        write_report(args.report, report)
    rng = random.Random(report.seed)
    started = time.monotonic()
    rotated_runs = 0
    try:
        report.worker_sha256 = file_sha256(args.worker.resolve())
        report.harness_sha256 = file_sha256(Path(__file__).resolve())
        if args.report is not None:
            write_report(args.report, report)
        for iteration in range(1, args.iterations + 1):
            report.attempted_iteration = iteration
            maximum_ack = 11 if release_pending else 12
            acknowledged = maximum_ack if iteration == 1 else rng.randint(1, maximum_ack)
            delay = rng.choice((0.0, 0.0001, 0.001, 0.005)) if release_pending else 0.0
            result = run_iteration(
                args.worker.resolve(), iteration, acknowledged, release_pending, delay,
                args.report.parent / f"{args.report.stem}-failures"
                if args.report is not None else None,
            )
            report.results.append(result)
            rotated_runs += result.segment_count > 1
            if args.report is not None:
                report.elapsed_seconds = round(time.monotonic() - started, 3)
                write_report(args.report, report)

        if rotated_runs == 0:
            raise RuntimeError("the run did not exercise WAL segment rotation")
        if file_sha256(args.worker.resolve()) != report.worker_sha256:
            raise RuntimeError("worker changed during the crash run")
        if file_sha256(Path(__file__).resolve()) != report.harness_sha256:
            raise RuntimeError("harness changed during the crash run")
        report.status = "passed"
    except (RuntimeError, OSError, ValueError, subprocess.SubprocessError) as exc:
        report.error = str(exc)
        if isinstance(exc, CrashIterationFailure):
            report.failure_artifact = str(exc.artifact)
            report.retention_error = exc.retention_error
        raise
    finally:
        report.elapsed_seconds = round(time.monotonic() - started, 3)
        if report.status != "passed":
            report.status = "failed"
            if report.error is None:
                report.error = "Run interrupted before completion"
        if args.report is not None:
            write_report(args.report, report)
    print(
        f"PASS: {args.iterations}/{args.iterations} forced process crashes; "
        f"{args.scenario} replay verified; "
        f"{rotated_runs} iterations crossed segment rotation"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
