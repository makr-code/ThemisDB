#!/usr/bin/env python3
"""
ThemisDB Certificate Manager Helper (8.3 name: tcertmgr.py)

Dual-mode tool:
- UI/TUI mode (default): interactive prompts
- CLI mode (fallback): pass --action and options

Actions:
- scan: scan certificate directory and report expiry state
- show: inspect one certificate file
"""

from __future__ import annotations

import argparse
import csv
import datetime as dt
import json
import ssl
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Dict, List, Optional

DEFAULT_CERT_DIR = Path("./certs")


@dataclass
class CertInfo:
    path: str
    subject: str
    issuer: str
    serial_number: str
    not_before: str
    not_after: str
    days_left: Optional[int]
    status: str
    error: str


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="ThemisDB certificate helper (dual-mode UI + CLI)",
        formatter_class=argparse.RawTextHelpFormatter,
        epilog=(
            "Examples:\n"
            "  python tcertmgr.py\n"
            "  python tcertmgr.py --action scan --cert-dir ./certs --days-warning 30\n"
            "  python tcertmgr.py --action show --cert ./certs/server.crt --format json"
        ),
    )

    parser.add_argument("--action", choices=["scan", "show"], help="Action to run in CLI mode")
    parser.add_argument("--cert-dir", type=Path, default=DEFAULT_CERT_DIR, help="Directory to scan for cert files")
    parser.add_argument("--cert", type=Path, help="Single certificate file for --action show")
    parser.add_argument("--days-warning", type=int, default=30, help="Warn if cert expires in <= N days")
    parser.add_argument("--format", choices=["json", "csv", "text"], default="json", help="Output format")
    parser.add_argument("--output", type=Path, help="Optional output file")
    parser.add_argument("--headless", action="store_true", help="Force non-interactive mode")
    parser.add_argument("--verbose", "-v", action="store_true", help="Verbose logging")

    return parser.parse_args()


def flatten_name(name_seq: Any) -> str:
    parts: List[str] = []
    try:
        for rdn in name_seq:
            if isinstance(rdn, (tuple, list)):
                for item in rdn:
                    if isinstance(item, (tuple, list)) and len(item) == 2:
                        key, value = item
                        parts.append(f"{key}={value}")
    except Exception:
        return ""
    return ", ".join(parts)


def parse_not_after(value: str) -> Optional[dt.datetime]:
    if not value:
        return None
    # ssl._ssl._test_decode_cert commonly returns: "Jun 15 12:00:00 2026 GMT"
    try:
        return dt.datetime.strptime(value, "%b %d %H:%M:%S %Y %Z").replace(tzinfo=dt.timezone.utc)
    except ValueError:
        return None


def cert_status(not_after_text: str, days_warning: int) -> tuple[Optional[int], str]:
    expiry = parse_not_after(not_after_text)
    if expiry is None:
        return None, "UNKNOWN"

    now = dt.datetime.now(dt.timezone.utc)
    days_left = int((expiry - now).total_seconds() // 86400)
    if days_left < 0:
        return days_left, "EXPIRED"
    if days_left <= days_warning:
        return days_left, "WARN"
    return days_left, "OK"


def decode_cert(path: Path, days_warning: int) -> CertInfo:
    try:
        obj: Dict[str, Any] = ssl._ssl._test_decode_cert(str(path))  # type: ignore[attr-defined]

        not_before = str(obj.get("notBefore", ""))
        not_after = str(obj.get("notAfter", ""))
        days_left, status = cert_status(not_after, days_warning)

        return CertInfo(
            path=str(path),
            subject=flatten_name(obj.get("subject", [])),
            issuer=flatten_name(obj.get("issuer", [])),
            serial_number=str(obj.get("serialNumber", "")),
            not_before=not_before,
            not_after=not_after,
            days_left=days_left,
            status=status,
            error="",
        )
    except Exception as exc:
        return CertInfo(
            path=str(path),
            subject="",
            issuer="",
            serial_number="",
            not_before="",
            not_after="",
            days_left=None,
            status="ERROR",
            error=str(exc),
        )


def scan_cert_dir(cert_dir: Path, days_warning: int) -> List[CertInfo]:
    if not cert_dir.exists() or not cert_dir.is_dir():
        return []

    exts = {".pem", ".crt", ".cer"}
    infos: List[CertInfo] = []

    for file_path in sorted(cert_dir.rglob("*")):
        if not file_path.is_file():
            continue
        if file_path.suffix.lower() not in exts:
            continue
        infos.append(decode_cert(file_path, days_warning))

    return infos


def info_to_dict(info: CertInfo) -> Dict[str, Any]:
    return {
        "path": info.path,
        "subject": info.subject,
        "issuer": info.issuer,
        "serial_number": info.serial_number,
        "not_before": info.not_before,
        "not_after": info.not_after,
        "days_left": info.days_left,
        "status": info.status,
        "error": info.error,
    }


def emit_output(items: List[CertInfo], fmt: str, output: Optional[Path]) -> int:
    if fmt == "json":
        payload = {
            "status": "success",
            "count": len(items),
            "data": [info_to_dict(x) for x in items],
        }
        text = json.dumps(payload, indent=2, ensure_ascii=True)
        if output:
            output.write_text(text, encoding="utf-8")
        else:
            print(text)
        return 0

    if fmt == "csv":
        headers = [
            "path",
            "subject",
            "issuer",
            "serial_number",
            "not_before",
            "not_after",
            "days_left",
            "status",
            "error",
        ]
        if output:
            with output.open("w", newline="", encoding="utf-8") as fh:
                writer = csv.DictWriter(fh, fieldnames=headers)
                writer.writeheader()
                for item in items:
                    writer.writerow(info_to_dict(item))
        else:
            writer = csv.DictWriter(sys.stdout, fieldnames=headers)
            writer.writeheader()
            for item in items:
                writer.writerow(info_to_dict(item))
        return 0

    # text format
    lines: List[str] = []
    for item in items:
        base = f"[{item.status}] {item.path} | days_left={item.days_left}"
        if item.error:
            base += f" | error={item.error}"
        else:
            base += f" | subject={item.subject}"
        lines.append(base)
    text = "\n".join(lines) if lines else "No certificate files found."
    if output:
        output.write_text(text + "\n", encoding="utf-8")
    else:
        print(text)
    return 0


def run_scan(cert_dir: Path, days_warning: int, fmt: str, output: Optional[Path], verbose: bool) -> int:
    if days_warning < 0:
        print("ERROR: --days-warning must be >= 0", file=sys.stderr)
        return 2

    if verbose:
        print(f"Scanning cert directory: {cert_dir}", file=sys.stderr)

    infos = scan_cert_dir(cert_dir, days_warning)
    try:
        return emit_output(infos, fmt, output)
    except OSError as exc:
        print(f"ERROR: Could not write output: {exc}", file=sys.stderr)
        return 4


def run_show(cert_file: Path, days_warning: int, fmt: str, output: Optional[Path], verbose: bool) -> int:
    if not cert_file.exists() or not cert_file.is_file():
        print(f"ERROR: Certificate file not found: {cert_file}", file=sys.stderr)
        return 4

    if verbose:
        print(f"Inspecting certificate: {cert_file}", file=sys.stderr)

    info = decode_cert(cert_file, days_warning)
    try:
        return emit_output([info], fmt, output)
    except OSError as exc:
        print(f"ERROR: Could not write output: {exc}", file=sys.stderr)
        return 4


def ask(prompt: str, default: str = "") -> str:
    suffix = f" [{default}]" if default else ""
    value = input(f"{prompt}{suffix}: ").strip()
    return value if value else default


def interactive_mode() -> int:
    print("ThemisDB Certificate Helper (tcertmgr)")
    print("=" * 36)
    print("1) Scan certificate directory")
    print("2) Inspect one certificate file")
    print("3) Exit")

    choice = ask("Choose action", "1")
    if choice == "3":
        return 0

    days_warning = int(ask("Warning threshold in days", "30"))
    out_fmt = ask("Output format (json/csv/text)", "text").lower()
    out_file = ask("Output file (empty=stdout)", "")
    out_path = Path(out_file) if out_file else None

    if choice == "1":
        cert_dir = Path(ask("Certificate directory", str(DEFAULT_CERT_DIR)))
        return run_scan(cert_dir, days_warning, out_fmt, out_path, verbose=False)

    if choice == "2":
        cert_path = Path(ask("Certificate file", ""))
        if not cert_path:
            print("ERROR: Certificate file is required", file=sys.stderr)
            return 2
        return run_show(cert_path, days_warning, out_fmt, out_path, verbose=False)

    print("ERROR: Invalid selection", file=sys.stderr)
    return 2


def main() -> int:
    args = parse_args()

    if args.action is None and not args.headless:
        return interactive_mode()

    if args.action == "scan":
        return run_scan(args.cert_dir, args.days_warning, args.format, args.output, args.verbose)

    if args.action == "show":
        if args.cert is None:
            print("ERROR: --cert is required for --action show", file=sys.stderr)
            return 2
        return run_show(args.cert, args.days_warning, args.format, args.output, args.verbose)

    print("ERROR: --action is required in CLI mode", file=sys.stderr)
    return 2


if __name__ == "__main__":
    sys.exit(main())
