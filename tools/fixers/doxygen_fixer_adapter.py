#!/usr/bin/env python3
# -*- coding: utf-8 -*-
from __future__ import annotations
import subprocess
from pathlib import Path
from typing import Dict, Any, List

def fix(root: Path, finding: Dict[str, Any], check_only: bool):
    # not used directly; batch mode preferred
    return {"status":"skipped","reason":"use_batch_mode","file":finding["file"],"line":finding["line"],"type":finding["type"]}

def fix_batch(root: Path, findings: List[Dict[str, Any]], check_only: bool):
    script = root / "tools/doxygen_autofix.py"
    if not script.exists():
        return [{
            "status":"unsafe","reason":"missing_tools_doxygen_autofix.py","file":f["file"],"line":f["line"],"type":f["type"]
        } for f in findings]

    cmd = ["python3", str(script), "--root", str(root)]
    cmd.append("--check-only" if check_only else "--apply")

    cp = subprocess.run(cmd, capture_output=True, text=True)
    status = "fixed" if cp.returncode in (0,1) else "unsafe"
    reason = "doxygen_autofix_executed" if cp.returncode in (0,1) else f"doxygen_autofix_failed:{cp.returncode}"

    return [{
        "status": status,
        "reason": reason,
        "file": f["file"],
        "line": f["line"],
        "type": f["type"],
        "before": "",
        "after": ""
    } for f in findings]
