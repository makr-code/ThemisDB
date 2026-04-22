#!/usr/bin/env python3
"""
ThemisDB Tools Launcher (8.3 name: ttools.py)

Dual-mode launcher:
- UI/TUI mode (default): interactive menu
- CLI mode (fallback): delegate to sub-tools

Supported sub-tools:
- tlogqry.py
- tcertmgr.py
- tbackup.py
- tshard.py
- tlnkown.py
- tpubwki.py
- tspisgn.py
- tplsman.py
- tfindnet.py
- tdiag.py
"""

from __future__ import annotations

import argparse
import subprocess
import sys
from pathlib import Path
from typing import List

BASE_DIR = Path(__file__).resolve().parent
SUPPORTED = {
    "log": "tlogqry.py",
    "cert": "tcertmgr.py",
    "backup": "tbackup.py",
    "shard": "tshard.py",
    "own": "tlnkown.py",
    "wiki": "tpubwki.py",
    "pii": "tspisgn.py",
    "plug": "tplsman.py",
    "plugpy": "tsgnplug.py",
    "ctl": "tthemctl.py",
    "cfg": "tcfgscan.py",
    "gkey": "tgrfkeys.py",
    "exp": "texport.py",
    "imp": "timport.py",
    "lora": "tloraprv.py",
    "disc": "tinstdis.py",
    "model": "tmodelcl.py",
    "txn": "ttxnsmok.py",
    "emb": "tembcert.py",
    "mig": "tmigvec.py",
    "aql": "taqlbld.py",
    "aud": "taudlogv.py",
    "cls": "tclassdb.py",
    "cmp": "tcomplrp.py",
    "gis": "tgisctlp.py",
    "impact": "timpanly.py",
    "ingui": "tingtool.py",
    "key": "tkeyrot.py",
    "piiui": "tpiimgr.py",
    "ret": "tretnmgr.py",
    "saga": "tsagver.py",
    "usb": "tusbadm.py",
    "find": "tfindnet.py",
    "diag": "tdiag.py",
    "aggr": "../bench/taggrshd.py",
}


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="ThemisDB tools launcher (dual-mode UI + CLI)",
        formatter_class=argparse.RawTextHelpFormatter,
        epilog=(
            "Examples:\n"
            "  python ttools.py\n"
            "  python ttools.py --tool log -- --action query --log-dir ./logs --level ERROR\n"
            "  python ttools.py --tool cert -- --action scan --cert-dir ./certs\n"
            "  python ttools.py --tool own -- --mapping ./config/ownership_mapping.example.yaml --output ownership_edges.jsonl"
        ),
    )
    parser.add_argument("--tool", choices=sorted(SUPPORTED.keys()), help="Sub-tool to run")
    parser.add_argument("--headless", action="store_true", help="Force non-interactive mode")
    parser.add_argument("tool_args", nargs=argparse.REMAINDER, help="Arguments passed after '--' to selected sub-tool")
    return parser.parse_args()


def clean_tool_args(values: List[str]) -> List[str]:
    if values and values[0] == "--":
        return values[1:]
    return values


def run_tool(tool_key: str, tool_args: List[str]) -> int:
    script_name = SUPPORTED[tool_key]
    script_path = BASE_DIR / script_name
    if not script_path.exists():
        print(f"ERROR: tool script not found: {script_path}", file=sys.stderr)
        return 4

    cmd = [sys.executable, str(script_path)] + tool_args
    try:
        proc = subprocess.run(cmd, check=False)
        return int(proc.returncode)
    except OSError as exc:
        print(f"ERROR: could not execute {script_name}: {exc}", file=sys.stderr)
        return 4


def interactive_mode() -> int:
    print("ThemisDB Tool Launcher (ttools)")
    print("=" * 29)
    print("1) Log Query Helper (tlogqry)")
    print("2) Certificate Helper (tcertmgr)")
    print("3) Backup Helper (tbackup)")
    print("4) Shard Helper (tshard)")
    print("5) Ownership Linker (tlnkown)")
    print("6) Wiki Publisher (tpubwki)")
    print("7) PII Signer (tspisgn)")
    print("8) Plugin Manifest Signer (tplsman)")
    print("9) Plugin Signer Python (tsgnplug)")
    print("10) ThemisCTL Python (tthemctl)")
    print("11) Config Migration Scanner (tcfgscan)")
    print("12) Instance Discovery (tinstdis)")
    print("13) Model CLI (tmodelcl)")
    print("14) Vector Migration Helper (tmigvec)")
    print("15) Admin App Equivalents (taqlbld/taudlogv/...)")
    print("16) Themis Find Network (tfindnet)")
    print("17) Themis Diagnose (tdiag)")
    print("18) Aggregate Shard Results (taggrshd)")
    print("19) Exit")

    choice = input("Choose tool [1]: ").strip() or "1"
    mapping = {
        "1": "log",
        "2": "cert",
        "3": "backup",
        "4": "shard",
        "5": "own",
        "6": "wiki",
        "7": "pii",
        "8": "plug",
        "9": "plugpy",
        "10": "ctl",
        "11": "cfg",
        "12": "disc",
        "13": "model",
        "14": "mig",
        "15": "aql",
        "16": "find",
        "17": "diag",
        "18": "aggr",
    }
    if choice == "19":
        return 0
    if choice not in mapping:
        print("ERROR: invalid selection", file=sys.stderr)
        return 2

    return run_tool(mapping[choice], [])


def main() -> int:
    args = parse_args()

    if args.tool is None and not args.headless:
        return interactive_mode()

    if args.tool is None:
        print("ERROR: --tool is required in CLI mode", file=sys.stderr)
        return 2

    return run_tool(args.tool, clean_tool_args(args.tool_args))


if __name__ == "__main__":
    sys.exit(main())
