#!/usr/bin/env python3
"""Bound the release dependency configure, including its entire subprocess tree.

Uses the caller's unchanged configure command so preset/manifest features cannot
drift from the build lane. Completed vcpkg ZIPs remain available to cache/save.
No ThemisDB build command is invoked.
"""

import argparse
import os
from pathlib import Path
import re
import signal
import subprocess
import sys
import time


def terminate_tree(process):
    """Terminate by PID, not executable name. Fail closed if cleanup fails."""
    if os.name == "nt":
        result = subprocess.run(
            ["taskkill.exe", "/PID", str(process.pid), "/T", "/F"],
            timeout=60, check=False,
        )
        if result.returncode and process.poll() is None:
            raise RuntimeError("taskkill failed to terminate dependency process tree")
    else:
        os.killpg(process.pid, signal.SIGKILL)
    process.wait(timeout=60)


def run(command, log, budget_seconds):
    if budget_seconds <= 0:
        print("::error::Dependency budget exhausted before configure", flush=True)
        return 124
    if hasattr(sys.stdout, "reconfigure"):
        sys.stdout.reconfigure(encoding="utf-8", errors="replace")
    started = time.monotonic()
    # Regular files avoid a pipe reader blocking on descendants after timeout.
    with log.open("wb") as output, log.open("rb") as reader:
        process = subprocess.Popen(
            command, stdout=output, stderr=subprocess.STDOUT,
            start_new_session=os.name != "nt",
            creationflags=subprocess.CREATE_NEW_PROCESS_GROUP if os.name == "nt" else 0,
        )
        try:
            while process.poll() is None:
                chunk = reader.read()
                if chunk:
                    print(chunk.decode("utf-8", errors="replace"), end="", flush=True)
                if time.monotonic() - started >= budget_seconds:
                    print("::error::Dependency budget expired; terminating process tree", flush=True)
                    terminate_tree(process)
                    print(reader.read().decode("utf-8", errors="replace"), end="", flush=True)
                    return 124
                time.sleep(0.2)
            print(reader.read().decode("utf-8", errors="replace"), end="", flush=True)
            return process.returncode
        finally:
            if process.poll() is None:
                terminate_tree(process)


def report(log, cache):
    archives = sum(1 for path in cache.rglob("*") if path.is_file() and path.suffix.lower() == ".zip")
    print(f"vcpkg completed binary ZIP archives: {archives}")
    text = log.read_text(encoding="utf-8", errors="replace") if log.exists() else ""
    restored = [line for line in text.splitlines() if re.search(r"\bRestored \d+ package", line)]
    print("vcpkg ABI-validated reuse reported by install:")
    for line in restored:
        print(line)
    if not restored:
        print("not reported (archive inventory alone does not prove ABI reuse)")
    for line in text.splitlines():
        if "package ABI:" in line:
            print(line)
    print("sccache zero requests means no compiler requests, not cache misses")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--log", type=Path, required=True)
    parser.add_argument("--metrics-only", action="store_true")
    parser.add_argument("--cache", type=Path)
    args = parser.parse_args()
    if args.metrics_only:
        if args.cache is None:
            parser.error("--metrics-only requires --cache")
        report(args.log, args.cache)
        return 0
    if not os.environ.get("THEMIS_DEPENDENCY_BASH"):
        parser.error("configure requires THEMIS_DEPENDENCY_BASH")
    # The job starts a 300-minute deadline before checkout/cache/setup. Never
    # spend more than 270 minutes in configure, leaving >=60 minutes of the
    # 360-minute job for termination, inventory, cache upload and diagnostics.
    remaining = float(os.environ["THEMIS_DEPENDENCY_DEADLINE"]) - time.time()
    return run(
        [os.environ["THEMIS_DEPENDENCY_BASH"], "-euo", "pipefail", "-c", os.environ["CONFIGURE_COMMAND"]],
        args.log, min(270 * 60, remaining),
    )


if __name__ == "__main__":
    raise SystemExit(main())
