#!/usr/bin/env python3
# -*- coding: utf-8 -*-
from __future__ import annotations
from pathlib import Path
from typing import Dict, Any

def fix(root: Path, finding: Dict[str, Any], check_only: bool):
    fp = root / finding["file"]
    if not fp.exists():
        return {"status":"skipped","reason":"file_not_found","file":finding["file"],"line":finding["line"],"type":finding["type"]}

    lines = fp.read_text(encoding="utf-8", errors="ignore").splitlines(keepends=True)
    i = max(0, min(len(lines)-1, int(finding["line"]) - 1))
    before = lines[i]

    ctx = "".join(lines[max(0, i-2):min(len(lines), i+2)])
    if "+=" in before and ("for (" in ctx or "for(" in ctx):
        # safe-mode: do not auto rewrite complex behavior
        return {"status":"unsafe","reason":"requires_ast_refactor_to_builder","file":finding["file"],"line":finding["line"],"type":finding["type"]}

    return {"status":"skipped","reason":"pattern_not_matched","file":finding["file"],"line":finding["line"],"type":finding["type"]}
