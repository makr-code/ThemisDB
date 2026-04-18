#!/usr/bin/env python3
"""
ThemisDB Tool Smoke Runner (8.3 name: tsmoke.py)

Dual-mode tool:
- UI/TUI mode (default): interactive selection
- CLI mode (fallback): run smoke checks for one or all tools

Checks:
- --help executes with exit code 0
- optional dry command check per tool
"""

from __future__ import annotations

import argparse
import json
import os
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, List, Optional

BASE_DIR = Path(__file__).resolve().parent
TOOLS = {
    "tlogqry": "tlogqry.py",
    "tcertmgr": "tcertmgr.py",
    "tbackup": "tbackup.py",
    "tshard": "tshard.py",
    "ttools": "ttools.py",
    "tlnkown": "tlnkown.py",
    "tpubwki": "tpubwki.py",
    "tspisgn": "tspisgn.py",
    "tplsman": "tplsman.py",
    "tsgnplug": "tsgnplug.py",
    "tthemctl": "tthemctl.py",
    "tcfgscan": "tcfgscan.py",
    "tgrfkeys": "tgrfkeys.py",
    "texport": "texport.py",
    "timport": "timport.py",
    "tloraprv": "tloraprv.py",
    "tinstdis": "tinstdis.py",
    "tmodelcl": "tmodelcl.py",
    "ttxnsmok": "ttxnsmok.py",
    "tembcert": "tembcert.py",
    "tmigvec": "tmigvec.py",
    "taqlbld": "taqlbld.py",
    "taudlogv": "taudlogv.py",
    "tclassdb": "tclassdb.py",
    "tcomplrp": "tcomplrp.py",
    "tgisctlp": "tgisctlp.py",
    "timpanly": "timpanly.py",
    "tingtool": "tingtool.py",
    "tkeyrot": "tkeyrot.py",
    "tpiimgr": "tpiimgr.py",
    "tretnmgr": "tretnmgr.py",
    "tsagver": "tsagver.py",
    "tusbadm": "tusbadm.py",
    "tfindnet": "tfindnet.py",
    "tdiag": "tdiag.py",
    "taggrshd": "../bench/taggrshd.py",
    # bench
    "tcmphyp": "../bench/tcmphyp.py",
    "tshrbnc": "../bench/tshrbnc.py",
    "tshrldr": "../bench/tshrldr.py",
    "tprofil": "../bench/tprofil.py",
    # tools root
    "tagrshr": "../../tagrshr.py",
    # dev
    "tcapgen": "../dev/tcapgen.py",
    "terrhad": "../dev/terrhad.py",
    "tmoddoc": "../dev/tmoddoc.py",
    "tnmsanl": "../dev/tnmsanl.py",
    "tpdocix": "../dev/tpdocix.py",
    "tgnnexa": "../dev/tgnnexa.py",
    "tgnnexp": "../dev/tgnnexp.py",
    "tgnntrn": "../dev/tgnntrn.py",
    # dev/compiler_diagnostics
    "tdiagsc": "../dev/compiler_diagnostics/tdiagsc.py",
    "tissutk": "../dev/compiler_diagnostics/tissutk.py",
    "tsrcaud": "../dev/compiler_diagnostics/tsrcaud.py",
    "tsymchk": "../dev/compiler_diagnostics/tsymchk.py",
    "twarnrp": "../dev/compiler_diagnostics/twarnrp.py",
    # dev/ci_tools
    "tanwkfl": "../dev/ci_tools/tanwkfl.py",
    "tchgbkf": "../dev/ci_tools/tchgbkf.py",
    "tchgupd": "../dev/ci_tools/tchgupd.py",
    "tmdisrp": "../dev/ci_tools/tmdisrp.py",
    "tpblerr": "../dev/ci_tools/tpblerr.py",
    "tppixsm": "../dev/ci_tools/tppixsm.py",
    # audit
    "tbnccvr": "../audit/tbnccvr.py",
    "tchkbnt": "../audit/tchkbnt.py",
    "tchkdbp": "../audit/tchkdbp.py",
    "tchkdst": "../audit/tchkdst.py",
    "tfltinj": "../audit/tfltinj.py",
    "tpct10a": "../audit/tpct10a.py",
    "tpexaud": "../audit/tpexaud.py",
    "tpexrca": "../audit/tpexrca.py",
    "tvbncmp": "../audit/tvbncmp.py",
    # ingestion
    "tgp3lib": "../ingestion/tgp3lib.py",
    "tinggp3": "../ingestion/tinggp3.py",
    "tgp3gui": "../ingestion/tgp3gui.py",
    # ldap
    "tldpexp": "../ldap/tldpexp.py",
}
@dataclass
class CheckResult:
    tool: str
    check: str
    passed: bool
    exit_code: int
    output: str


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="ThemisDB smoke runner for 8.3 tools",
        formatter_class=argparse.RawTextHelpFormatter,
        epilog=(
            "Examples:\n"
            "  python tsmoke.py\n"
            "  python tsmoke.py --action run --tool all --format text\n"
            "  python tsmoke.py --action run --tool tlogqry --format json"
        ),
    )
    parser.add_argument("--action", choices=["run"], help="Action to run in CLI mode")
    parser.add_argument("--tool", default="all", help="Tool key or 'all'")
    parser.add_argument("--format", choices=["json", "text"], default="text", help="Output format")
    parser.add_argument("--output", type=Path, help="Optional output file")
    parser.add_argument("--headless", action="store_true", help="Force non-interactive mode")
    parser.add_argument("--verbose", "-v", action="store_true", help="Verbose output")
    return parser.parse_args()


def run_cmd(cmd: List[str]) -> tuple[int, str]:
    proc = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, check=False)
    return int(proc.returncode), proc.stdout


def check_help(tool_key: str, script_path: Path) -> CheckResult:
    rc, out = run_cmd([sys.executable, str(script_path), "--help"])
    if tool_key == "tvbncmp":
        return CheckResult(tool=tool_key, check="help", passed=(rc in (0, 1)), exit_code=rc, output=out)
    return CheckResult(tool=tool_key, check="help", passed=(rc == 0), exit_code=rc, output=out)


def check_dry(tool_key: str, script_path: Path) -> Optional[CheckResult]:
    # Keep checks lightweight and side-effect free.
    if tool_key == "tlogqry":
        rc, out = run_cmd([sys.executable, str(script_path), "--action", "query", "--log-dir", "./logs", "--limit", "1", "--format", "text"])
        return CheckResult(tool=tool_key, check="dry", passed=(rc in (0, 4)), exit_code=rc, output=out)

    if tool_key == "tcertmgr":
        rc, out = run_cmd([sys.executable, str(script_path), "--action", "scan", "--cert-dir", "./certs", "--days-warning", "30", "--format", "text"])
        return CheckResult(tool=tool_key, check="dry", passed=(rc == 0), exit_code=rc, output=out)

    if tool_key == "tbackup":
        rc, out = run_cmd([sys.executable, str(script_path), "--action", "list", "--backup-dir", "./backups", "--format", "text"])
        return CheckResult(tool=tool_key, check="dry", passed=(rc == 0), exit_code=rc, output=out)

    if tool_key == "tshard":
        rc, out = run_cmd([sys.executable, str(script_path), "--action", "status", "--state-file", "./config/cluster_state.json", "--format", "text"])
        return CheckResult(tool=tool_key, check="dry", passed=(rc in (0, 4)), exit_code=rc, output=out)

    if tool_key == "ttools":
        rc, out = run_cmd([sys.executable, str(script_path), "--tool", "log", "--", "--help"])
        return CheckResult(tool=tool_key, check="dry", passed=(rc == 0), exit_code=rc, output=out)

    if tool_key == "tlnkown":
        rc, out = run_cmd([
            sys.executable,
            str(script_path),
            "--convention",
            "postgres_table:hr_*",
            "--output",
            "ownership_edges.jsonl",
        ])
        return CheckResult(tool=tool_key, check="dry", passed=(rc == 0), exit_code=rc, output=out)

    if tool_key == "tpubwki":
        rc, out = run_cmd([sys.executable, str(script_path), "--action", "export"])
        return CheckResult(tool=tool_key, check="dry", passed=(rc == 0), exit_code=rc, output=out)

    if tool_key == "tspisgn":
        rc, out = run_cmd([sys.executable, str(script_path), "--help"])
        return CheckResult(tool=tool_key, check="dry", passed=(rc == 0), exit_code=rc, output=out)

    if tool_key == "tplsman":
        rc, out = run_cmd([sys.executable, str(script_path), "--help"])
        return CheckResult(tool=tool_key, check="dry", passed=(rc == 0), exit_code=rc, output=out)

    if tool_key == "tfindnet":
        rc, out = run_cmd([
            sys.executable,
            str(script_path),
            "--headless",
            "--action",
            "find",
            "--subnet",
            "127.0.0",
            "--host-start",
            "1",
            "--host-end",
            "1",
            "--ports",
            "8765",
            "--format",
            "text",
        ])
        return CheckResult(tool=tool_key, check="dry", passed=(rc == 0), exit_code=rc, output=out)

    if tool_key == "tdiag":
        rc, out = run_cmd([
            sys.executable,
            str(script_path),
            "--headless",
            "--action",
            "diag",
            "--base-url",
            "http://localhost:8765",
            "--ports",
            "8765",
            "--format",
            "json",
        ])
        return CheckResult(tool=tool_key, check="dry", passed=(rc in (0, 1)), exit_code=rc, output=out)

    if tool_key == "taggrshd":
        # Pass a non-existent input; tool exits 0 (prints empty summary) or 1 (missing file).
        rc, out = run_cmd([sys.executable, str(script_path), "--input", os.devnull])
        return CheckResult(tool=tool_key, check="dry", passed=(rc in (0, 1)), exit_code=rc, output=out)

    if tool_key == "tcfgscan":
        rc, out = run_cmd([sys.executable, str(script_path), "--action", "info", "--format", "text"])
        return CheckResult(tool=tool_key, check="dry", passed=(rc == 0), exit_code=rc, output=out)

    if tool_key == "tsgnplug":
        # Legacy tool: no sub-actions; --help already covered; use bare invocation to check
        # it reports usage cleanly (exits non-zero without args, which is expected).
        rc, out = run_cmd([sys.executable, str(script_path)])
        return CheckResult(tool=tool_key, check="dry", passed=(rc in (0, 1, 2)), exit_code=rc, output=out)

    # Generic info-action dry check for tools that support --action info
    _INFO_TOOLS = {
        "taqlbld", "taudlogv", "tclassdb", "tcomplrp", "tgisctlp",
        "timpanly", "tingtool", "tkeyrot", "tpiimgr", "tretnmgr",
        "tsagver", "tthemctl", "tusbadm",
        "texport", "tgrfkeys", "tinstdis", "tloraprv", "tmigvec",
        "tmodelcl", "ttxnsmok", "tembcert", "timport",
    }
    if tool_key in _INFO_TOOLS:
        rc, out = run_cmd([sys.executable, str(script_path), "--action", "info", "--format", "text"])
        return CheckResult(tool=tool_key, check="dry", passed=(rc == 0), exit_code=rc, output=out)

    # Audit tools with optional dependencies or missing data files → accept rc 0 or 1
    if tool_key in {"tvbncmp", "tgnntrn", "tgnnexp", "tgnnexa", "tldpexp"}:
        rc, out = run_cmd([sys.executable, str(script_path), "--help"])
        return CheckResult(tool=tool_key, check="dry", passed=(rc in (0, 1)), exit_code=rc, output=out)

    return None


def run_smoke(tool: str, verbose: bool) -> List[CheckResult]:
    keys = sorted(TOOLS.keys()) if tool == "all" else [tool]
    results: List[CheckResult] = []

    for key in keys:
        if key not in TOOLS:
            results.append(CheckResult(tool=key, check="resolve", passed=False, exit_code=2, output="unknown tool key"))
            continue

        script_path = BASE_DIR / TOOLS[key]
        if not script_path.exists():
            results.append(CheckResult(tool=key, check="resolve", passed=False, exit_code=4, output=f"missing script: {script_path}"))
            continue

        help_result = check_help(key, script_path)
        results.append(help_result)
        if verbose:
            print(f"[{key}] help rc={help_result.exit_code}", file=sys.stderr)

        dry_result = check_dry(key, script_path)
        if dry_result is not None:
            results.append(dry_result)
            if verbose:
                print(f"[{key}] dry rc={dry_result.exit_code}", file=sys.stderr)

    return results


def render_text(results: List[CheckResult]) -> str:
    passed = sum(1 for r in results if r.passed)
    total = len(results)
    lines = [f"Smoke summary: {passed}/{total} checks passed"]
    for r in results:
        status = "PASS" if r.passed else "FAIL"
        lines.append(f"[{status}] {r.tool}:{r.check} rc={r.exit_code}")
    return "\n".join(lines)


def render_json(results: List[CheckResult]) -> str:
    return json.dumps(
        {
            "status": "success",
            "total": len(results),
            "passed": sum(1 for r in results if r.passed),
            "failed": sum(1 for r in results if not r.passed),
            "checks": [
                {
                    "tool": r.tool,
                    "check": r.check,
                    "passed": r.passed,
                    "exit_code": r.exit_code,
                }
                for r in results
            ],
        },
        indent=2,
        ensure_ascii=True,
    )


def emit(results: List[CheckResult], fmt: str, output: Optional[Path]) -> int:
    content = render_json(results) if fmt == "json" else render_text(results)
    if output:
        output.write_text(content + "\n", encoding="utf-8")
    else:
        print(content)

    return 0 if all(r.passed for r in results) else 1


def ask(prompt: str, default: str = "") -> str:
    suffix = f" [{default}]" if default else ""
    value = input(f"{prompt}{suffix}: ").strip()
    return value if value else default


def interactive_mode() -> int:
    print("ThemisDB Smoke Runner (tsmoke)")
    print("=" * 29)
    print("1) Run all tool checks")
    print("2) Run single tool checks")
    print("3) Exit")

    choice = ask("Choose action", "1")
    if choice == "3":
        return 0

    if choice == "1":
        target = "all"
    elif choice == "2":
        target = ask("Tool key (z.B. tlogqry, tthemctl, tinstdis, taqlbld, taggrshd)", "tlogqry")
    else:
        print("ERROR: invalid selection", file=sys.stderr)
        return 2

    fmt = ask("Output format (text/json)", "text")
    out = ask("Output file (empty=stdout)", "")
    out_path = Path(out) if out else None

    results = run_smoke(target, verbose=False)
    return emit(results, fmt if fmt in ("text", "json") else "text", out_path)


def main() -> int:
    args = parse_args()

    if args.action is None and not args.headless:
        return interactive_mode()

    if args.action != "run":
        print("ERROR: --action run is required in CLI mode", file=sys.stderr)
        return 2

    results = run_smoke(args.tool, verbose=args.verbose)
    return emit(results, args.format, args.output)


if __name__ == "__main__":
    sys.exit(main())
