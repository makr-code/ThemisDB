#!/usr/bin/env python3
from __future__ import annotations

import argparse
import os
import platform
import socket
import sys
from pathlib import Path
from typing import Any, Dict, List

from _teqbase import ActionResult, create_parser, emit, health_check, run_gui

TITLE = "Themis Diagnose (Python)"
ACTIONS = ["diag", "health", "info"]


def _port_probe(host: str, port: int, timeout: float = 0.25) -> Dict[str, Any]:
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.settimeout(timeout)
    try:
        s.connect((host, port))
        return {"host": host, "port": port, "open": True}
    except OSError:
        return {"host": host, "port": port, "open": False}
    finally:
        s.close()


def _check_paths() -> List[Dict[str, Any]]:
    candidates = [
        Path("build-msvc-ninja-release") / "bin",
        Path("models"),
        Path("certs"),
        Path("config"),
        Path("logs"),
    ]
    out: List[Dict[str, Any]] = []
    for p in candidates:
        out.append({
            "path": str(p),
            "exists": p.exists(),
            "is_dir": p.is_dir(),
        })
    return out


def run_diag(args: argparse.Namespace) -> ActionResult:
    ports = [int(p.strip()) for p in args.ports.split(",") if p.strip()]
    probes = [_port_probe("127.0.0.1", p) for p in ports]

    health = health_check(args.base_url)

    payload = {
        "python": {
            "executable": sys.executable,
            "version": sys.version.split()[0],
            "platform": platform.platform(),
        },
        "system": {
            "hostname": socket.gethostname(),
            "cwd": os.getcwd(),
        },
        "paths": _check_paths(),
        "ports": probes,
        "health": {
            "ok": health.ok,
            "message": health.message,
            "payload": health.payload,
        },
    }

    ok = any(p["open"] for p in probes) or health.ok
    msg = "Diagnose completed: service signals detected" if ok else "Diagnose completed: no service signal detected"
    return ActionResult(ok, msg, payload, code=0 if ok else 1)


def handle(action: str, args: argparse.Namespace) -> ActionResult:
    if action == "info":
        return ActionResult(True, f"{TITLE}: Python implementation active", {"kind": "diag"})
    if action == "health":
        return health_check(args.base_url)
    if action == "diag":
        return run_diag(args)
    return ActionResult(False, f"Unsupported action: {action}", code=3)


def main() -> int:
    parser = create_parser(
        TITLE,
        ACTIONS,
        epilog=(
            "Examples:\n"
            "  python tdiag.py --headless --action diag --base-url http://localhost:8765 --ports 8080,8765,9000 --format json\n"
            "  python tdiag.py --headless --action health --base-url http://localhost:8765"
        ),
    )
    args = parser.parse_args()

    if not args.action and not args.headless:
        return run_gui(TITLE, ACTIONS, handle, args)

    if not args.action:
        return emit(ActionResult(False, "--action is required in CLI mode", code=2), args.format)

    return emit(handle(args.action, args), args.format)


if __name__ == "__main__":
    raise SystemExit(main())
