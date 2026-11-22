#!/usr/bin/env python3
"""CTest fixture and benchmark runner; all manifest files are extensionless JSON.

The fixture serializes overlapping CTest invocations sharing a results directory.
An interrupted CTest can leave the lease behind; remove it only after confirming
that its owning CTest is no longer running.
"""

import argparse
import json
import os
from pathlib import Path
import shutil
import signal
import subprocess
import sys
import time
import uuid


def write_json(path, data):
    path.parent.mkdir(parents=True, exist_ok=True)
    scratch = path.with_name(path.name + "." + uuid.uuid4().hex)
    try:
        with scratch.open("x", encoding="utf-8") as stream:
            json.dump(data, stream, indent=2, allow_nan=False)
            stream.write("\n")
        os.replace(scratch, path)
    finally:
        scratch.unlink(missing_ok=True)


def load_json(path):
    with path.open(encoding="utf-8") as stream:
        return json.load(stream)


def lease_path(results):
    return results.with_name(results.name + ".lease")


def owner_is_current(lease):
    try:
        return load_json(lease / "owner")["ctest_pid"] == os.getppid()
    except (OSError, ValueError, KeyError, TypeError):
        return False


def prepare(args):
    results = Path(args.results_dir)
    lease = lease_path(results)
    lease.parent.mkdir(parents=True, exist_ok=True)
    try:
        lease.mkdir()
    except FileExistsError:
        if owner_is_current(lease):
            owner = load_json(lease / "owner")
            current = load_json(results / "manifest")
            if owner["run_id"] == current["run_id"] and current["status"] == "in_progress":
                return
        raise RuntimeError(f"Benchmark results are leased: {lease}; "
                           "if CTest was interrupted, verify its owner is gone before removing it")
    try:
        run_id = uuid.uuid4().hex
        write_json(lease / "owner", {"ctest_pid": os.getppid(), "run_id": run_id})
        subprocess.run([args.cmake, f"-DRESULTS_DIR={results}",
                        f"-DARCHIVE_DIR={args.archive_dir}", "-P", args.prepare_script],
                       check=True)
        registry = load_json(Path(args.registry))
        started = time.time()
        tests = []
        for item in registry:
            relative = (str(Path(item["result"]).relative_to(results))
                        if item["result"] is not None else None)
            tests.append({**item, "result": relative, "result_path_at_run": item["result"],
                          "result_relative": relative,
                          "execution_status": "unknown", "json_status": "unknown",
                          "build_status": ("unknown" if item["executable"] is None else
                                           "present" if Path(item["executable"]).exists() else "missing"),
                          "exit_code": None, "elapsed_seconds": None, "cases": []})
        write_json(results / "manifest", {
            "schema_version": 1, "run_id": run_id, "ctest_pid": os.getppid(),
            "started_unix_seconds": started,
            "finished_unix_seconds": None, "elapsed_seconds": None,
            "status": "in_progress", "tests": tests, "expectations": [],
            "expectations_status": "not_collected", "postprocess_exit_code": None})
        (results / ".manifest").mkdir(exist_ok=True)
    except BaseException:
        (lease / "owner").unlink(missing_ok=True)
        lease.rmdir()
        raise


def read_result(path):
    try:
        text = path.read_text(encoding="utf-8")
    except FileNotFoundError:
        return "missing", []
    except (OSError, UnicodeError):
        return "invalid", []
    try:
        data = json.loads(text, parse_constant=lambda value: (_ for _ in ()).throw(ValueError(value)))
        rows = data["benchmarks"]
        if not isinstance(rows, list) or not rows or any(
                not isinstance(row, dict) or not isinstance(row.get("name"), str)
                or not row["name"] for row in rows):
            raise ValueError("invalid benchmarks")
        cases = []
        for row in rows:
            is_error = row.get("error_occurred") is True
            is_skipped = row.get("skipped") is True
            cases.append({
                "name": row["name"],
                "status": "error" if is_error else "skipped" if is_skipped else "observed",
                "error_message": row.get("error_message") if is_error else None,
                "skip_message": row.get("skip_message") if is_skipped else None,
                "elapsed_seconds": None,
            })
        return "valid", cases
    except (ValueError, KeyError, TypeError):
        return "invalid", []


def run(args):
    results = Path(args.results_dir)
    manifest = load_json(results / "manifest")
    owner = load_json(lease_path(results) / "owner")
    if owner["run_id"] != manifest["run_id"]:
        raise RuntimeError("Manifest lease does not match the current run")
    entry = next(item for item in manifest["tests"] if item["id"] == args.id)
    fragment = results / ".manifest" / args.id
    attempts = []
    if fragment.exists():
        previous = load_json(fragment)
        if previous.pop("run_id") != manifest["run_id"]:
            raise RuntimeError("Execution fragment belongs to another run")
        attempts = previous.pop("attempts", [])
        attempts.append(previous)
    result = results / entry["result"]
    result.parent.mkdir(parents=True, exist_ok=True)
    staged_result = results / ".manifest" / (manifest["run_id"] + "-" + uuid.uuid4().hex)
    result.unlink(missing_ok=True)
    data = {"run_id": manifest["run_id"], "execution_status": "unknown",
            "json_status": "unknown", "exit_code": None, "elapsed_seconds": None,
            "build_status": "unknown", "observation_path": None, "cases": [],
            "observation_relative": None, "attempts": attempts}
    write_json(fragment, data)
    started = time.monotonic()
    command = args.command
    if command and command[0] == "--":
        command = command[1:]
    if not command or command[0] != entry["executable"]:
        raise RuntimeError("Benchmark executable does not match its registration")
    output_args = [i for i, arg in enumerate(command) if arg.startswith("--benchmark_out=")]
    if len(output_args) != 1:
        raise RuntimeError("Expected exactly one benchmark output argument")
    command[output_args[0]] = "--benchmark_out=" + str(staged_result)
    try:
        process = subprocess.Popen(command, start_new_session=(os.name == "posix"))
        data["build_status"] = "present"
        def terminate(signum=None, _frame=None):
            try:
                if os.name == "posix":
                    os.killpg(process.pid, signal.SIGKILL)
                else:
                    process.kill()
            except ProcessLookupError:
                pass
            if signum is not None:
                raise SystemExit(128 + signum)

        old_handlers = {sig: signal.signal(sig, terminate) for sig in (signal.SIGINT, signal.SIGTERM)}
        try:
            try:
                code = process.wait(timeout=args.timeout)
                data["exit_code"] = code
                data["execution_status"] = "completed" if code == 0 else "process_failure"
            except subprocess.TimeoutExpired:
                terminate()
                process.wait()
                code = 124
                data["execution_status"] = "timeout"
        finally:
            for sig, handler in old_handlers.items():
                signal.signal(sig, handler)
    except FileNotFoundError:
        code = 127
        data["build_status"] = "present" if Path(entry["executable"]).exists() else "missing"
        data["execution_status"] = "not_built" if data["build_status"] == "missing" else "launch_failure"
    except OSError as exc:
        print(f"Benchmark launch failed: {exc}", file=sys.stderr)
        code = 126
        data["build_status"] = "present" if Path(entry["executable"]).exists() else "unknown"
        data["execution_status"] = "launch_failure"
    data["elapsed_seconds"] = time.monotonic() - started
    data["json_status"], data["cases"] = read_result(staged_result)
    if data["json_status"] == "valid":
        data["observation_path"] = str(staged_result)
        data["observation_relative"] = str(staged_result.relative_to(results))
    if data["execution_status"] == "completed" and data["json_status"] == "valid":
        published = results / ".manifest" / uuid.uuid4().hex
        try:
            shutil.copyfile(staged_result, published)
            os.replace(published, result)
        finally:
            published.unlink(missing_ok=True)
    write_json(fragment, data)
    if code != 0:
        return code if code > 0 else 128 - code
    if data["json_status"] != "valid":
        return 1
    failed_cases = [case for case in data["cases"] if case["status"] == "error"]
    if failed_cases:
        details = "; ".join(
            f'{case["name"]}: {case["error_message"] or "benchmark case failed"}'
            for case in failed_cases
        )
        print(f"Benchmark case failure(s): {details}", file=sys.stderr)
        return 1
    return 0


def collect_expectations(src_dir, tests):
    # Reuse the comparator's documented table parsing rather than inventing a
    # target-to-source relationship. Exact names and the comparator's explicit
    # parameter suffix rule are evidence, not proof of case success.
    sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
    from compare_expectations import _parse_performance_expectations

    observations = []
    for test in tests:
        for case in test["cases"]:
            observations.append({
                "case": case["name"],
                "test": test["test"], "target": test["target"],
                "result": test.get("observation_relative"),
                "result_path_at_run": test.get("observation_path"),
                "case_status": case["status"],
                "result_relative": test.get("observation_relative"),
                "execution_status": test["execution_status"]})
    expectations = []
    for path in sorted(Path(src_dir).rglob("PERFORMANCE_EXPECTATIONS.md")):
        for expectation in _parse_performance_expectations(path, path.parent.name):
            names = expectation.benchmark_cases or [None]
            for name in names:
                matches = [item for item in observations if name and
                           (item["case"] == name or item["case"].startswith(name + "/"))]
                expectations.append({"source": str(path), "module": expectation.module,
                                     "target_id": expectation.target_id, "case": name,
                                     "mapping_status": "observed" if matches else "unknown",
                                     "observations": matches})
    return expectations


def finish(args):
    results = Path(args.results_dir)
    lease = lease_path(results)
    if not owner_is_current(lease):
        if not lease.exists():
            try:
                previous = load_json(results / "manifest")
                if (previous["ctest_pid"] == os.getppid() and previous["status"] == "finished"
                        and previous["postprocess_exit_code"] is not None):
                    return previous["postprocess_exit_code"]
            except (OSError, ValueError, KeyError, TypeError):
                pass
        print("Benchmark cleanup does not own the result lease; skipping", file=sys.stderr)
        return 1
    try:
        manifest = load_json(results / "manifest")
        owner = load_json(lease / "owner")
        if owner["run_id"] != manifest["run_id"]:
            raise RuntimeError("Manifest lease does not match the current run")
        for entry in manifest["tests"]:
            fragment = results / ".manifest" / entry["id"]
            if fragment.exists():
                data = load_json(fragment)
                if data.pop("run_id") != manifest["run_id"]:
                    raise RuntimeError("Execution fragment belongs to another run")
                entry.update(data)
            else:
                entry["execution_status"] = ("not_executed" if entry["registration_status"] == "not_available"
                                             else "unknown")
                entry["execution_observation"] = "wrapper_not_observed"
        try:
            if not Path(args.src_dir).is_dir():
                raise OSError(f"Expectation source directory is unavailable: {args.src_dir}")
            manifest["expectations"] = collect_expectations(args.src_dir, manifest["tests"])
            manifest["expectations_status"] = "collected"
        except (OSError, ValueError, ImportError) as exc:
            manifest["expectations_status"] = "unavailable"
            manifest["expectations_error"] = str(exc)
            print(f"Benchmark expectation mapping unavailable: {exc}", file=sys.stderr)
        manifest["finished_unix_seconds"] = time.time()
        manifest["elapsed_seconds"] = max(0, manifest["finished_unix_seconds"] -
                                          manifest["started_unix_seconds"])
        manifest["status"] = "finished"
        write_json(results / "manifest", manifest)
        command = args.command[1:] if args.command and args.command[0] == "--" else args.command
        code = subprocess.run(command).returncode
        manifest["postprocess_exit_code"] = code
        write_json(results / "manifest", manifest)
        return code if code >= 0 else 128 - code
    finally:
        (lease / "owner").unlink(missing_ok=True)
        lease.rmdir()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest="action", required=True)
    setup = commands.add_parser("prepare")
    for name in ("results-dir", "archive-dir", "registry", "cmake", "prepare-script"):
        setup.add_argument("--" + name, required=True)
    execute = commands.add_parser("run")
    execute.add_argument("--results-dir", required=True)
    execute.add_argument("--id", required=True)
    execute.add_argument("--timeout", type=float, required=True)
    execute.add_argument("command", nargs=argparse.REMAINDER)
    cleanup = commands.add_parser("finish")
    cleanup.add_argument("--results-dir", required=True)
    cleanup.add_argument("--src-dir", required=True)
    cleanup.add_argument("command", nargs=argparse.REMAINDER)
    args = parser.parse_args()
    try:
        return {"prepare": prepare, "run": run, "finish": finish}[args.action](args) or 0
    except (OSError, ValueError, KeyError, StopIteration, RuntimeError,
            subprocess.SubprocessError) as exc:
        print(f"Benchmark manifest: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
