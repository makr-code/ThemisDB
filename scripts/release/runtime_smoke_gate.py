#!/usr/bin/env python3
"""Check deployable artifacts, without borrowing libraries from the build tree."""

import argparse
import json
import os
import re
import socket
import subprocess
import sys
import tarfile
import tempfile
import time
import urllib.error
import urllib.request
import uuid
import zipfile
from pathlib import Path

import validate_runtime_payload as payload


PACKAGE_SUFFIXES = (".tar.gz", ".tgz", ".zip", ".deb", ".rpm", ".msi", ".dmg")


def command(argv, *, env=None, cwd=None, timeout=60):
    result = subprocess.run(argv, env=env, cwd=cwd, timeout=timeout,
                            capture_output=True, text=True, errors="replace")
    if result.returncode:
        raise ValueError(
            f"{argv[0]} failed ({result.returncode}):\n{result.stdout}\n{result.stderr}"
        )
    return result.stdout + result.stderr


def runtime_environment():
    env = dict(os.environ)
    for key in ("LD_LIBRARY_PATH", "LD_PRELOAD", "DYLD_LIBRARY_PATH",
                "DYLD_FALLBACK_LIBRARY_PATH", "DYLD_INSERT_LIBRARIES"):
        env.pop(key, None)
    if os.name == "nt":
        system = env.get("SYSTEMROOT")
        if not system:
            raise ValueError("SYSTEMROOT is required to construct the Windows runtime PATH")
        env["PATH"] = os.pathsep.join((os.path.join(system, "System32"), system))
    else:
        env["PATH"] = "/usr/bin:/bin:/usr/sbin:/sbin"
    return env


def healthy(body):
    try:
        data = json.loads(body)
    except (json.JSONDecodeError, UnicodeDecodeError):
        return False
    if not isinstance(data, dict):
        return False
    if "status" in data:
        return str(data["status"]).lower() in ("ok", "healthy")
    return data.get("healthy") is True


def wait_for_health(endpoint, alive, timeout):
    deadline = time.monotonic() + timeout
    detail = "no response"
    opener = urllib.request.build_opener(urllib.request.ProxyHandler({}))
    while time.monotonic() < deadline:
        if not alive():
            raise ValueError("Server exited before becoming healthy")
        for path in ("/health", "/api/health"):
            try:
                with opener.open(endpoint + path, timeout=2) as response:
                    body = response.read()
                    if response.status == 200 and healthy(body):
                        if not alive():
                            raise ValueError("Server exited after its health response")
                        return {"endpoint": endpoint + path, "body": body.decode("utf-8")}
                    detail = f"{path}: HTTP {response.status}, {body[:200]!r}"
            except (urllib.error.URLError, TimeoutError, ConnectionError) as error:
                detail = str(error)
        time.sleep(0.5)
    raise ValueError(f"Healthcheck timed out after {timeout}s: {detail}")


def check_libraries(root, env, report):
    if not sys.platform.startswith("linux"):
        return
    libraries = []
    report["libraries"] = libraries
    missing = []
    for path in sorted(root.rglob("*")):
        if not path.is_file():
            continue
        with path.open("rb") as stream:
            if stream.read(4) != b"\x7fELF":
                continue
        result = subprocess.run(["ldd", str(path)], env=env, capture_output=True,
                                text=True, timeout=30)
        output = result.stdout + result.stderr
        libraries.append({"file": str(path.relative_to(root)), "ldd": output})
        unresolved = [line.strip() for line in output.splitlines() if "not found" in line]
        if unresolved:
            missing.extend(f"{path.name}: {line}" for line in unresolved)
        elif result.returncode and not any(
            marker in output for marker in ("not a dynamic executable", "statically linked")
        ):
            raise ValueError(f"Cannot inspect runtime dependencies of {path}: {output}")
    if missing:
        raise ValueError("Missing runtime libraries:\n" + "\n".join(missing))


def validate_tree(root, report, timeout, dependencies_only=False):
    names = ("themis_server.exe",) if os.name == "nt" else ("themis_server", "themisdb")
    servers = [path for path in root.rglob("*") if path.is_file() and path.name in names]
    if len(servers) != 1:
        raise ValueError(f"Expected exactly one server executable in {root}, found {len(servers)}")
    server = servers[0].resolve()
    env = runtime_environment()
    check_libraries(root, env, report)
    report["version"] = command([str(server), "--version"], env=env, cwd=server.parent).strip()
    if not re.search(r"\d+\.\d+\.\d+", report["version"]):
        raise ValueError(f"Invalid server version: {report['version']!r}")
    if dependencies_only:
        return
    with socket.socket() as listener:
        listener.bind(("127.0.0.1", 0))
        port = listener.getsockname()[1]
    with tempfile.TemporaryDirectory(prefix="themis-runtime-data-") as data:
        log = Path(data) / "server.log"
        config = Path(data) / "config.json"
        config.write_text(json.dumps({
            "server": {"host": "127.0.0.1", "port": port, "worker_threads": 2},
            "storage": {"rocksdb_path": str(Path(data) / "db")},
        }))
        with log.open("w+") as output:
            process = subprocess.Popen(
                [str(server), "--config", str(config)],
                cwd=server.parent, env=env, stdout=output, stderr=subprocess.STDOUT,
            )
            try:
                report["health"] = wait_for_health(
                    f"http://127.0.0.1:{port}", lambda: process.poll() is None, timeout
                )
            finally:
                if process.poll() is None:
                    process.terminate()
                    try:
                        process.wait(timeout=15)
                    except subprocess.TimeoutExpired:
                        process.kill()
                        process.wait(timeout=15)
                output.flush()
                report["server_log"] = log.read_text(errors="replace")


def validate_package(package, report, timeout, dependencies_only):
    payload.validate_package(
        package, payload_only=False,
        validator=lambda root: validate_tree(root, report, timeout, dependencies_only),
    )


def validate_image(image, report, timeout, dependencies_only):
    report["libraries"] = command(
        ["docker", "run", "--rm", "--network", "none", "--entrypoint", "/bin/sh", image,
         "-ec", 'output="$(ldd /app/themis_server 2>&1)"; printf "%s\\n" "$output"; '
         '! printf "%s\\n" "$output" | grep -q "not found"']
    )
    report["version"] = command(
        ["docker", "run", "--rm", "--network", "none", image, "--version"]
    ).strip()
    if not re.search(r"\d+\.\d+\.\d+", report["version"]):
        raise ValueError(f"Invalid image version: {report['version']!r}")
    if dependencies_only:
        return
    name = "themis-runtime-gate-" + uuid.uuid4().hex
    command(["docker", "create", "--name", name,
             "-p", "127.0.0.1::8080", image])
    try:
        command(["docker", "start", name])
        mapping = command(["docker", "port", name, "8080/tcp"]).strip()
        report["health"] = wait_for_health(
            "http://" + mapping,
            lambda: command(["docker", "inspect", "-f", "{{.State.Running}}", name]).strip() == "true",
            timeout,
        )
    finally:
        try:
            report["server_log"] = command(["docker", "logs", name])
        finally:
            command(["docker", "rm", "-f", name])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    sources = parser.add_mutually_exclusive_group(required=True)
    sources.add_argument("--image", help="Locally available runtime image")
    sources.add_argument("--packages", type=Path, help="Directory of final packages")
    sources.add_argument("--root", type=Path, help="Installed runtime tree")
    parser.add_argument("--dependencies-only", action="store_true",
                        help="Inspect libraries and execute --version; do not start a server")
    parser.add_argument("--timeout", type=int, default=120)
    parser.add_argument("--output", type=Path, default=Path("runtime-smoke-report.json"))
    args = parser.parse_args()
    if args.timeout <= 0:
        parser.error("--timeout must be positive")
    reports = []
    try:
        if args.image:
            sources_to_check = [args.image]
        elif args.root:
            sources_to_check = [args.root.resolve()]
        else:
            sources_to_check = sorted(
                path.resolve() for path in args.packages.iterdir()
                if path.is_file() and path.name.endswith(PACKAGE_SUFFIXES)
            )
            if not sources_to_check:
                raise ValueError(f"No deployable packages found in {args.packages}")
        for source in sources_to_check:
            report = {"artifact": str(source), "ok": False}
            reports.append(report)
            try:
                if args.image:
                    validate_image(source, report, args.timeout, args.dependencies_only)
                elif args.root:
                    validate_tree(source, report, args.timeout, args.dependencies_only)
                else:
                    validate_package(source, report, args.timeout, args.dependencies_only)
                report["ok"] = True
            except (ValueError, OSError, subprocess.SubprocessError, tarfile.TarError,
                    zipfile.BadZipFile) as error:
                report["error"] = str(error)
                print(f"::error::{source}: {error}", file=sys.stderr)
    except (ValueError, OSError) as error:
        reports.append({"artifact": str(args.packages), "ok": False, "error": str(error)})
        print(f"::error::{error}", file=sys.stderr)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps({"checks": reports}, indent=2), encoding="utf-8")
    return 0 if reports and all(report["ok"] for report in reports) else 1


if __name__ == "__main__":
    sys.exit(main())
