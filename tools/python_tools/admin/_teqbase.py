#!/usr/bin/env python3
"""
Shared runtime for Python tool equivalents in tools/python_tools/admin.

No delegation to C++/.NET binaries: all actions are implemented in Python.
"""

from __future__ import annotations

import argparse
import csv
import hashlib
import json
import os
import re
import socket
import threading
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Callable, Dict, Iterable, List, Optional


@dataclass
class ActionResult:
    ok: bool
    message: str
    payload: Dict[str, Any] | List[Any] | None = None
    code: int = 0


def emit(result: ActionResult, fmt: str = "text") -> int:
    if fmt == "json":
        print(json.dumps({
            "ok": result.ok,
            "message": result.message,
            "payload": result.payload,
            "code": result.code,
        }, ensure_ascii=True, indent=2))
        return result.code

    if fmt == "csv" and isinstance(result.payload, list) and result.payload:
        keys: List[str] = sorted({k for row in result.payload if isinstance(row, dict) for k in row.keys()})
        w = csv.DictWriter(os.sys.stdout, fieldnames=keys)
        w.writeheader()
        for row in result.payload:
            if isinstance(row, dict):
                w.writerow(row)
        return result.code

    print(result.message)
    if isinstance(result.payload, dict):
        for k, v in result.payload.items():
            print(f"{k}: {v}")
    elif isinstance(result.payload, list):
        for item in result.payload:
            print(item)
    return result.code


def sha256_file(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        while True:
            block = f.read(1024 * 1024)
            if not block:
                break
            h.update(block)
    return h.hexdigest()


def health_check(base_url: str, timeout: float = 2.0) -> ActionResult:
    import urllib.request
    import urllib.error

    url = base_url.rstrip("/") + "/health"
    try:
        req = urllib.request.Request(url, headers={"Accept": "application/json"})
        with urllib.request.urlopen(req, timeout=timeout) as resp:
            body = resp.read().decode("utf-8", errors="replace")
            return ActionResult(True, f"Health reachable: HTTP {resp.status}", {"url": url, "status": resp.status, "body": body[:500]})
    except urllib.error.URLError as exc:
        return ActionResult(False, f"Health check failed: {exc}", {"url": url}, code=1)


def discover_instances(prefix: str, host_start: int, host_end: int, ports: Iterable[int], timeout: float = 0.2) -> ActionResult:
    found: List[Dict[str, Any]] = []
    lock = threading.Lock()

    def probe(ip: str, port: int) -> None:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.settimeout(timeout)
        try:
            s.connect((ip, port))
            with lock:
                found.append({"host": ip, "port": port, "confidence": "MEDIUM", "detail": "tcp-open"})
        except OSError:
            pass
        finally:
            s.close()

    threads: List[threading.Thread] = []
    for i in range(host_start, host_end + 1):
        ip = f"{prefix}.{i}"
        for p in ports:
            t = threading.Thread(target=probe, args=(ip, p), daemon=True)
            threads.append(t)
            t.start()

    for t in threads:
        t.join()

    return ActionResult(True, f"Discovery done: {len(found)} candidates", found)


def list_models(model_dir: Path) -> ActionResult:
    if not model_dir.exists():
        return ActionResult(False, f"Model directory not found: {model_dir}", code=2)
    items = []
    for p in sorted(model_dir.rglob("*")):
        if p.is_file() and p.suffix.lower() in {".gguf", ".bin", ".onnx", ".pt", ".pth"}:
            items.append({"path": str(p), "size": p.stat().st_size})
    return ActionResult(True, f"Found {len(items)} model files", items)


def config_migration_scan(root: Path, legacy_patterns: List[str]) -> ActionResult:
    if not root.exists():
        return ActionResult(False, f"Scan root not found: {root}", code=2)

    rx = [re.compile(p) for p in legacy_patterns]
    findings: List[Dict[str, Any]] = []
    for p in root.rglob("*"):
        if not p.is_file():
            continue
        if p.suffix.lower() not in {".yaml", ".yml", ".json", ".toml", ".ini", ".conf", ".txt", ".md"}:
            continue
        try:
            text = p.read_text(encoding="utf-8", errors="replace")
        except OSError:
            continue
        for pat in rx:
            for m in pat.finditer(text):
                findings.append({"file": str(p), "pattern": pat.pattern, "match": m.group(0)})

    return ActionResult(True, f"Config scan complete: {len(findings)} findings", findings)


def embed_certificate(cert_file: Path, symbol: str) -> ActionResult:
    if not cert_file.exists():
        return ActionResult(False, f"Certificate not found: {cert_file}", code=2)

    data = cert_file.read_bytes()
    literal = ", ".join(f"0x{b:02X}" for b in data)
    out = {
        "symbol": symbol,
        "bytes": len(data),
        "cpp": f"static const unsigned char {symbol}[] = {{{literal}}};",
    }
    return ActionResult(True, f"Embedded certificate ({len(data)} bytes)", out)


def migrate_vector_json(input_file: Path, output_file: Path) -> ActionResult:
    if not input_file.exists():
        return ActionResult(False, f"Input not found: {input_file}", code=2)

    try:
        doc = json.loads(input_file.read_text(encoding="utf-8"))
    except Exception as exc:
        return ActionResult(False, f"Invalid JSON: {exc}", code=2)

    migrated = 0

    def visit(node: Any) -> None:
        nonlocal migrated
        if isinstance(node, dict):
            if "embedding" in node and "vector" not in node:
                node["vector"] = node.pop("embedding")
                migrated += 1
            for v in node.values():
                visit(v)
        elif isinstance(node, list):
            for it in node:
                visit(it)

    visit(doc)
    output_file.write_text(json.dumps(doc, ensure_ascii=True, indent=2), encoding="utf-8")
    return ActionResult(True, f"Migration complete: {migrated} nodes updated", {"output": str(output_file), "migrated": migrated})


def create_parser(description: str, actions: List[str], epilog: str = "") -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=description, epilog=epilog, formatter_class=argparse.RawTextHelpFormatter)
    parser.add_argument("--action", choices=actions, help="Action to execute (CLI mode)")
    parser.add_argument("--format", choices=["text", "json", "csv"], default="text")
    parser.add_argument("--headless", action="store_true", help="Disable GUI mode")
    parser.add_argument("--base-url", default="http://localhost:8765")
    parser.add_argument("--path", help="Generic path input")
    parser.add_argument("--output", help="Generic output path")
    parser.add_argument("--symbol", default="embedded_cert")
    parser.add_argument("--subnet", default="127.0.0")
    parser.add_argument("--host-start", type=int, default=1)
    parser.add_argument("--host-end", type=int, default=8)
    parser.add_argument("--ports", default="8080,8765,9000")
    parser.add_argument("--patterns", default="legacy_config,old_config_path")
    return parser


def run_gui(title: str, actions: List[str], handler: Callable[[str, argparse.Namespace], ActionResult], args: argparse.Namespace) -> int:
    try:
        import tkinter as tk
        from tkinter import ttk, messagebox
    except Exception:
        return emit(ActionResult(False, "Tkinter is not available. Use --headless with --action.", code=3), args.format)

    root = tk.Tk()
    root.title(title)
    root.geometry("720x460")

    action_var = tk.StringVar(value=actions[0])
    path_var = tk.StringVar(value=args.path or "")
    out_var = tk.StringVar(value=args.output or "")

    top = ttk.Frame(root, padding=10)
    top.pack(fill="x")
    ttk.Label(top, text="Action").grid(row=0, column=0, sticky="w")
    ttk.Combobox(top, textvariable=action_var, values=actions, state="readonly", width=30).grid(row=0, column=1, sticky="we", padx=6)
    ttk.Label(top, text="Path").grid(row=1, column=0, sticky="w")
    ttk.Entry(top, textvariable=path_var, width=70).grid(row=1, column=1, sticky="we", padx=6)
    ttk.Label(top, text="Output").grid(row=2, column=0, sticky="w")
    ttk.Entry(top, textvariable=out_var, width=70).grid(row=2, column=1, sticky="we", padx=6)
    top.columnconfigure(1, weight=1)

    txt = tk.Text(root, wrap="word")
    txt.pack(fill="both", expand=True, padx=10, pady=10)

    def do_run() -> None:
        args.path = path_var.get().strip() or None
        args.output = out_var.get().strip() or None
        res = handler(action_var.get(), args)
        txt.delete("1.0", "end")
        txt.insert("1.0", json.dumps({
            "ok": res.ok,
            "message": res.message,
            "payload": res.payload,
            "code": res.code,
        }, ensure_ascii=True, indent=2))
        if not res.ok:
            messagebox.showerror(title, res.message)

    btn = ttk.Button(root, text="Run", command=do_run)
    btn.pack(pady=(0, 10))

    root.mainloop()
    return 0
