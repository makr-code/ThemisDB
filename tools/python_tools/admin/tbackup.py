#!/usr/bin/env python3
"""
ThemisDB Backup Helper (8.3 name: tbackup.py)

Dual-mode tool:
- UI/TUI mode (default): interactive prompts
- CLI mode (fallback): pass --action and options

Actions:
- list: list backup files in directory
- verify: verify one backup via SHA-256 and sidecar checksum file
"""

from __future__ import annotations

import argparse
import csv
import hashlib
import json
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Dict, List, Optional

DEFAULT_BACKUP_DIR = Path("./backups")


@dataclass
class BackupItem:
    path: str
    size_bytes: int
    checksum_file: str
    checksum_status: str
    error: str


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="ThemisDB backup helper (dual-mode UI + CLI)",
        formatter_class=argparse.RawTextHelpFormatter,
        epilog=(
            "Examples:\n"
            "  python tbackup.py\n"
            "  python tbackup.py --action list --backup-dir ./backups --format json\n"
            "  python tbackup.py --action verify --file ./backups/snap-20260418.tar.zst"
        ),
    )

    parser.add_argument("--action", choices=["list", "verify"], help="Action to run in CLI mode")
    parser.add_argument("--backup-dir", type=Path, default=DEFAULT_BACKUP_DIR, help="Backup directory")
    parser.add_argument("--file", type=Path, help="Backup file for --action verify")
    parser.add_argument("--format", choices=["json", "csv", "text"], default="json", help="Output format")
    parser.add_argument("--output", type=Path, help="Optional output file")
    parser.add_argument("--headless", action="store_true", help="Force non-interactive mode")
    parser.add_argument("--verbose", "-v", action="store_true", help="Verbose output")

    return parser.parse_args()


def checksum_sidecar(path: Path) -> Path:
    return path.with_suffix(path.suffix + ".sha256")


def compute_sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as fh:
        for chunk in iter(lambda: fh.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def read_expected_checksum(sidecar: Path) -> Optional[str]:
    if not sidecar.exists() or not sidecar.is_file():
        return None
    text = sidecar.read_text(encoding="utf-8", errors="replace").strip()
    if not text:
        return None
    # Accept plain hash or "<hash> <filename>" format.
    return text.split()[0].lower()


def detect_backups(backup_dir: Path) -> List[Path]:
    if not backup_dir.exists() or not backup_dir.is_dir():
        return []

    allowed = {".tar", ".gz", ".zst", ".zip", ".bak", ".dump", ".sql"}
    result: List[Path] = []

    for path in sorted(backup_dir.rglob("*")):
        if not path.is_file():
            continue
        if path.suffix.lower() in allowed:
            result.append(path)
            continue
        # Handle double suffix variants like .tar.zst
        suffixes = "".join(path.suffixes).lower()
        if suffixes.endswith(".tar.zst") or suffixes.endswith(".tar.gz"):
            result.append(path)

    return result


def build_item(path: Path) -> BackupItem:
    sidecar = checksum_sidecar(path)
    return BackupItem(
        path=str(path),
        size_bytes=path.stat().st_size,
        checksum_file=str(sidecar) if sidecar.exists() else "",
        checksum_status="UNKNOWN",
        error="",
    )


def list_backups(backup_dir: Path) -> List[BackupItem]:
    return [build_item(path) for path in detect_backups(backup_dir)]


def verify_backup(path: Path) -> BackupItem:
    if not path.exists() or not path.is_file():
        return BackupItem(
            path=str(path),
            size_bytes=0,
            checksum_file="",
            checksum_status="ERROR",
            error="backup file not found",
        )

    item = build_item(path)
    sidecar = checksum_sidecar(path)
    expected = read_expected_checksum(sidecar)

    try:
        actual = compute_sha256(path)
        if expected is None:
            item.checksum_status = "MISSING"
            item.error = "missing or invalid .sha256 sidecar"
            return item

        item.checksum_status = "OK" if actual.lower() == expected else "MISMATCH"
        if item.checksum_status == "MISMATCH":
            item.error = "checksum mismatch"
        return item
    except OSError as exc:
        item.checksum_status = "ERROR"
        item.error = str(exc)
        return item


def as_dict(item: BackupItem) -> Dict[str, Any]:
    return {
        "path": item.path,
        "size_bytes": item.size_bytes,
        "checksum_file": item.checksum_file,
        "checksum_status": item.checksum_status,
        "error": item.error,
    }


def emit(items: List[BackupItem], fmt: str, output: Optional[Path]) -> int:
    if fmt == "json":
        payload = {
            "status": "success",
            "count": len(items),
            "data": [as_dict(it) for it in items],
        }
        text = json.dumps(payload, indent=2, ensure_ascii=True)
        if output:
            output.write_text(text, encoding="utf-8")
        else:
            print(text)
        return 0

    if fmt == "csv":
        headers = ["path", "size_bytes", "checksum_file", "checksum_status", "error"]
        if output:
            with output.open("w", newline="", encoding="utf-8") as fh:
                writer = csv.DictWriter(fh, fieldnames=headers)
                writer.writeheader()
                for item in items:
                    writer.writerow(as_dict(item))
        else:
            writer = csv.DictWriter(sys.stdout, fieldnames=headers)
            writer.writeheader()
            for item in items:
                writer.writerow(as_dict(item))
        return 0

    lines: List[str] = []
    for item in items:
        lines.append(
            f"[{item.checksum_status}] {item.path} size={item.size_bytes}"
            + (f" error={item.error}" if item.error else "")
        )
    text = "\n".join(lines) if lines else "No backup files found."
    if output:
        output.write_text(text + "\n", encoding="utf-8")
    else:
        print(text)
    return 0


def run_list(backup_dir: Path, fmt: str, output: Optional[Path], verbose: bool) -> int:
    if verbose:
        print(f"Listing backups in: {backup_dir}", file=sys.stderr)

    items = list_backups(backup_dir)
    try:
        return emit(items, fmt, output)
    except OSError as exc:
        print(f"ERROR: Could not write output: {exc}", file=sys.stderr)
        return 4


def run_verify(path: Path, fmt: str, output: Optional[Path], verbose: bool) -> int:
    if verbose:
        print(f"Verifying backup: {path}", file=sys.stderr)

    item = verify_backup(path)
    try:
        return emit([item], fmt, output)
    except OSError as exc:
        print(f"ERROR: Could not write output: {exc}", file=sys.stderr)
        return 4


def ask(prompt: str, default: str = "") -> str:
    suffix = f" [{default}]" if default else ""
    value = input(f"{prompt}{suffix}: ").strip()
    return value if value else default


def interactive_mode() -> int:
    print("ThemisDB Backup Helper (tbackup)")
    print("=" * 31)
    print("1) List backups")
    print("2) Verify one backup")
    print("3) Exit")

    choice = ask("Choose action", "1")
    if choice == "3":
        return 0

    out_fmt = ask("Output format (json/csv/text)", "text").lower()
    out_file = ask("Output file (empty=stdout)", "")
    out_path = Path(out_file) if out_file else None

    if choice == "1":
        backup_dir = Path(ask("Backup directory", str(DEFAULT_BACKUP_DIR)))
        return run_list(backup_dir, out_fmt, out_path, verbose=False)

    if choice == "2":
        file_path = Path(ask("Backup file", ""))
        if not str(file_path):
            print("ERROR: backup file is required", file=sys.stderr)
            return 2
        return run_verify(file_path, out_fmt, out_path, verbose=False)

    print("ERROR: invalid selection", file=sys.stderr)
    return 2


def main() -> int:
    args = parse_args()

    if args.action is None and not args.headless:
        return interactive_mode()

    if args.action == "list":
        return run_list(args.backup_dir, args.format, args.output, args.verbose)

    if args.action == "verify":
        if args.file is None:
            print("ERROR: --file is required for --action verify", file=sys.stderr)
            return 2
        return run_verify(args.file, args.format, args.output, args.verbose)

    print("ERROR: --action is required in CLI mode", file=sys.stderr)
    return 2


if __name__ == "__main__":
    sys.exit(main())
