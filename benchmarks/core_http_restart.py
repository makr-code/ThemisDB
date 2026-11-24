#!/usr/bin/env python3
"""Explicitly opt-in, private Windows clean-restart HTTP persistence check.

Not a benchmark, crash-recovery test, fsync test or ACID proof.
"""

from __future__ import annotations

import argparse
import hashlib
import ipaddress
import json
import math
import os
from pathlib import Path
import secrets
import shutil
import socket
import subprocess
import sys
import time
import uuid

import httpx
import psutil

HANDLER_MESSAGE = "Received shutdown signal, initiating graceful shutdown..."
COMPLETE_MESSAGE = "Shutdown complete. All data saved."


def _port():
    with socket.socket() as sock:
        sock.bind(("127.0.0.1", 0))
        return sock.getsockname()[1]


def _verify_listeners(process, ports):
    if not process.is_running():
        raise RuntimeError("Owned server exited")
    listeners = [connection for connection in process.net_connections("tcp")
                 if connection.status == psutil.CONN_LISTEN and connection.laddr]
    if any(not ipaddress.ip_address(item.laddr.ip).is_loopback for item in listeners):
        raise RuntimeError("Owned server has a non-loopback listener")
    if not set(ports).issubset({item.laddr.port for item in listeners}):
        return False
    return True


def _read(client, key, expected):
    response = client.get(f"/entities/{key}")
    if expected is None:
        if response.status_code != 404:
            raise RuntimeError(f"Expected HTTP 404 for owned key {key}")
        return
    response.raise_for_status()
    try:
        payload = response.json()
        matches = (
            response.status_code == 200 and isinstance(payload, dict)
            and payload.get("key") == key
            and json.dumps(json.loads(payload["blob"]), sort_keys=True, ensure_ascii=True)
            == json.dumps(expected, sort_keys=True, ensure_ascii=True))
    except (ValueError, KeyError, TypeError):
        matches = False
    if not matches:
        raise RuntimeError(f"Exact restart readback mismatch for owned key {key}")


def _exit_proof(returncode, stdout, stderr):
    if (returncode != 0 or HANDLER_MESSAGE not in stderr
            or COMPLETE_MESSAGE not in stdout + stderr):
        raise RuntimeError("Missing signal-handler log, shutdown-complete log or normal exit 0")
    return {"exit_code": returncode, "handler_message": HANDLER_MESSAGE,
            "completion_message": COMPLETE_MESSAGE}


class _OwnedServer:
    def __init__(self, directory, executable, config, environment, ports, token,
                 startup_timeout, shutdown_timeout):
        self.directory, self.executable, self.config = directory, executable, config
        self.environment, self.ports, self.token = environment, ports, token
        self.startup_timeout, self.shutdown_timeout = startup_timeout, shutdown_timeout
        self.process = None
        self.streams = []
        self.index = 0

    def start(self):
        if self.process is not None and self.process.poll() is None:
            raise RuntimeError("Previous owned server is still running")
        self.index += 1
        self.stdout = self.directory / f"server-{self.index}.stdout.log"
        self.stderr = self.directory / f"server-{self.index}.stderr.log"
        self.streams = [self.stdout.open("wb"), self.stderr.open("wb")]
        self.process = subprocess.Popen(
            [str(self.executable), "--config", str(self.config),
             "--server-profile", "minimal", "--host", "127.0.0.1",
             "--port", str(self.ports[0]), "--threads", "2"],
            cwd=self.directory, env=self.environment, stdin=subprocess.DEVNULL,
            stdout=self.streams[0], stderr=self.streams[1],
            creationflags=subprocess.CREATE_NEW_CONSOLE,
        )
        observed = psutil.Process(self.process.pid)
        deadline = time.monotonic() + self.startup_timeout
        with httpx.Client(base_url=f"http://127.0.0.1:{self.ports[0]}",
                          trust_env=False, timeout=2) as client:
            while time.monotonic() < deadline:
                if self.process.poll() is not None:
                    raise RuntimeError(f"Owned server exited at startup: {self.process.returncode}")
                if _verify_listeners(observed, self.ports[:2]):
                    try:
                        if client.get("/health").status_code == 200:
                            statuses = [client.get("/config").status_code,
                                        client.get("/config", headers={
                                            "Authorization": f"Bearer {self.token}"
                                        }).status_code]
                            if statuses != [401, 200]:
                                raise RuntimeError("Temporary authentication preflight failed")
                            return {"pid": self.process.pid,
                                    "created": observed.create_time(),
                                    "auth_http_statuses": statuses}
                    except httpx.TransportError:
                        pass
                time.sleep(0.1)
        raise RuntimeError("Owned server readiness timeout")

    def stop(self):
        if self.process is None or self.process.poll() is not None:
            raise RuntimeError("Owned server exited before controlled stop")
        helper = subprocess.Popen(
            [sys.executable, str(Path(__file__).with_name("windows_private_console.py")),
             str(self.process.pid), str(os.getpid()),
             str(psutil.Process(self.process.pid).create_time()), str(self.executable)],
            cwd=self.directory, stdin=subprocess.DEVNULL, stdout=subprocess.PIPE,
            stderr=subprocess.PIPE, creationflags=subprocess.DETACHED_PROCESS,
        )
        try:
            output, error = helper.communicate(timeout=10)
        except subprocess.TimeoutExpired:
            helper.kill()
            helper.communicate()
            raise RuntimeError("Owned console helper timed out; restart forbidden") from None
        if helper.returncode != 0:
            raise RuntimeError(f"Console helper refused stop: {output.decode(errors='replace')}"
                               f" {error.decode(errors='replace')}")
        evidence = json.loads(output)
        expected = {self.process.pid, helper.pid}
        snapshots = evidence.get("members_before", [])
        if (evidence.get("event_sent") is not True
                or evidence.get("server_pid") != self.process.pid
                or evidence.get("helper_pid") != helper.pid or len(snapshots) != 3
                or any(len(members) != 2 or set(members) != expected for members in snapshots)):
            raise RuntimeError("Console helper returned incomplete isolation evidence")
        try:
            code = self.process.wait(timeout=self.shutdown_timeout)
        except subprocess.TimeoutExpired:
            raise RuntimeError("Owned server did not exit cleanly before deadline; restart forbidden") from None
        self._close_streams()
        proof = _exit_proof(code, self.stdout.read_text(errors="replace"),
                            self.stderr.read_text(errors="replace"))
        proof["console"] = evidence
        return proof

    def _close_streams(self):
        for stream in self.streams:
            stream.close()
        self.streams = []

    def dispose(self):
        # Failure cleanup only, NEVER a restart boundary. Popen retains the
        # handle of exactly the process created here, not a PID discovered later.
        forced = False
        try:
            if self.process is not None and self.process.poll() is None:
                forced = True
                self.process.terminate()
                self.process.wait(timeout=15)
        finally:
            self._close_streams()
        return forced


def run_check(source, scratch_parent, *, allow_private_windows_restart=False,
              startup_timeout=60, shutdown_timeout=65):
    if not allow_private_windows_restart:
        raise ValueError("Explicit --allow-private-windows-restart opt-in is required")
    if sys.platform != "win32":
        raise RuntimeError("This check is Windows-only; no Linux stop/restart contract is implemented")
    for timeout in (startup_timeout, shutdown_timeout):
        if (isinstance(timeout, bool) or not isinstance(timeout, (int, float))
                or not math.isfinite(timeout) or timeout <= 0):
            raise ValueError("Timeouts must be finite and positive")
    source = Path(source).resolve(strict=True)
    scratch_parent = Path(scratch_parent).resolve(strict=True)
    if not source.is_file() or source.suffix.lower() != ".exe" or not scratch_parent.is_dir():
        raise ValueError("An existing server executable and scratch parent directory are required")
    run_id = uuid.uuid4().hex
    directory = scratch_parent / f"themis-private-restart-{run_id}"
    directory.mkdir()
    owned = None
    try:
        # The server also opens ../logs relative to its working directory.
        # Nest the runtime so both that path and cwd-relative data stay inside
        # our exclusive container, never in the caller's scratch parent.
        runtime = directory / "runtime"
        runtime.mkdir()
        executable = runtime / "themis_server.exe"
        inputs = [source, *source.parent.glob("*.dll")]
        identities = {item: (item.stat().st_size, item.stat().st_mtime_ns) for item in inputs}
        shutil.copy2(source, executable)
        for dll in inputs[1:]:
            shutil.copy2(dll, runtime / dll.name)
        if any((item.stat().st_size, item.stat().st_mtime_ns) != identity
               for item, identity in identities.items()):
            raise RuntimeError("Source executable/DLL changed during copy; refusing mixed runtime")
        ports = []
        while len(ports) < 3:
            candidate = _port()
            if candidate not in ports:
                ports.append(candidate)
        token = secrets.token_urlsafe(32)
        # Do not inherit caller Themis, proxy, auth, sharding, or path settings.
        environment = {name: value for name, value in os.environ.items()
                       if name.upper() in {"SYSTEMROOT", "WINDIR", "SYSTEMDRIVE",
                                           "COMSPEC", "PROCESSOR_ARCHITECTURE",
                                           "NUMBER_OF_PROCESSORS"}}
        for name in ("USERPROFILE", "HOME", "APPDATA", "LOCALAPPDATA", "PROGRAMDATA",
                     "TEMP", "TMP"):
            environment[name] = str(directory)
        environment.update(
            THEMIS_TOKEN_ADMIN=token, THEMIS_HEALTH_BIND_ADDRESS="127.0.0.1",
            THEMIS_HEALTH_PORT=str(ports[1]), THEMIS_WAL_GRPC_HOST="127.0.0.1",
            THEMIS_WAL_GRPC_PORT=str(ports[2]), THEMIS_RATE_LIMIT_PER_MINUTE="100000",
            THEMIS_WAL_GRPC_ALLOW_INSECURE="false",
        )
        config = directory / "server.json"
        config.write_text(json.dumps({
            "storage": {"rocksdb_path": str(runtime / "db"),
                        "memtable_size_mb": 64, "block_cache_size_mb": 64},
            "server": {"host": "127.0.0.1", "port": ports[0], "worker_threads": 2},
        }), encoding="utf-8")
        report = {
            "check": "core-http-private-windows-clean-restart",
            "run_id": run_id, "server_sha256": hashlib.sha256(executable.read_bytes()).hexdigest(),
            "source_executable": str(source), "source_mtime_ns": identities[source][1],
            "binary_provenance": "Caller-supplied binary; this check does not build it "
                                 "or establish source-revision provenance",
            "platform": sys.getwindowsversion().platform_version,
            "http_port": ports[0], "health_port": ports[1],
            "startup_timeout_seconds": startup_timeout,
            "shutdown_timeout_seconds": shutdown_timeout,
            "limits": "Clean process restart only; no crash, power-loss, fsync, ACID or performance claim",
            "console_isolation": "Private console with checked membership snapshots; "
                                 "not a security boundary against concurrent hostile AttachConsole",
        }
        owned = _OwnedServer(runtime, executable, config, environment, ports, token,
                             startup_timeout, shutdown_timeout)
        report["first_start"] = owned.start()
        values = {f"restart_{run_id}:record": {"run": run_id, "count": 17,
                                             "text": "Persistenz \u00fcber Neustart", "ok": True},
                  f"restart_{run_id}:sentinel": {"run": run_id, "sentinel": True}}
        with httpx.Client(base_url=f"http://127.0.0.1:{ports[0]}",
                          headers={"Authorization": f"Bearer {token}"},
                          trust_env=False, timeout=10) as client:
            for key, value in values.items():
                _read(client, key, None)
                response = client.put(f"/entities/{key}", json={"blob": json.dumps(value)})
                response.raise_for_status()
                _read(client, key, value)
            report["first_stop"] = owned.stop()
            # No restart unless stop returned handler + completion + exit-0 proof.
            report["second_start"] = owned.start()
            for key, value in values.items():
                _read(client, key, value)
            report["restart_readback"] = True
            record, sentinel = values
            client.delete(f"/entities/{record}").raise_for_status()
            _read(client, record, None)
            _read(client, sentinel, values[sentinel])
            client.delete(f"/entities/{sentinel}").raise_for_status()
            _read(client, sentinel, None)
            report["owned_keys_deleted_and_absent"] = list(values)
            report["second_stop"] = owned.stop()
        report["success"] = True
    finally:
        # Remove private data only after every owned process has exited.
        if owned is not None:
            if owned.dispose() is True:
                print(f"Failure cleanup terminated only owned server PID {owned.process.pid}; "
                      "this is NOT a clean restart result.", file=sys.stderr)
        shutil.rmtree(directory)
    report["private_directory_removed"] = True
    return report


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--allow-private-windows-restart", action="store_true",
                        help="Consent to two private server launches and console-scoped Ctrl+C")
    parser.add_argument("--server-exe", required=True, type=Path)
    parser.add_argument("--scratch-parent", type=Path, default=Path.cwd())
    parser.add_argument("--output", required=True, type=Path, help="New success-report path")
    arguments = parser.parse_args(argv)
    # Reserve the report before starting a process; never overwrite another run.
    try:
        with arguments.output.open("x", encoding="utf-8") as output:
            try:
                report = run_check(
                    arguments.server_exe, arguments.scratch_parent,
                    allow_private_windows_restart=arguments.allow_private_windows_restart)
                json.dump(report, output, indent=2)
                output.write("\n")
            except BaseException:
                output.close()
                arguments.output.unlink()
                raise
    except (OSError, ValueError, RuntimeError, httpx.HTTPError, psutil.Error) as exc:
        print(f"Restart check FAILED (no success report): {exc}", file=sys.stderr)
        return 1
    print("Verified private clean restart, exact HTTP readback and owned-data cleanup.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
