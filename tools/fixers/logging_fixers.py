#!/usr/bin/env python3
# -*- coding: utf-8 -*-
from __future__ import annotations
import json, re
from pathlib import Path
from typing import Dict, Any

def _load_policy(root: Path) -> Dict[str, Any]:
    p = root / "tools/fixers/logging_policy.json"
    if not p.exists():
        return {
            "macros": {"info": "THEMIS_LOG_INFO", "warn": "THEMIS_LOG_WARN", "error": "THEMIS_LOG_ERROR", "debug": "THEMIS_LOG_DEBUG"},
            "default_level": "info",
            "replace_printf": True,
            "replace_cout": True,
            "replace_cerr": True
        }
    return json.loads(p.read_text(encoding="utf-8"))

def _replace_line(line: str, policy: Dict[str, Any]) -> str:
    m = policy["macros"]
    default = m.get(policy.get("default_level", "info"), m["info"])

    if policy.get("replace_printf", True):
        line = re.sub(r'\bprintf\s*\((.*)\)\s*;', rf'{default}(\1);', line)

    if policy.get("replace_cout", True):
        line = re.sub(r'\bstd::cout\s*<<\s*(.*)\s*;', rf'{m["info"]}(\1);', line)

    if policy.get("replace_cerr", True):
        line = re.sub(r'\bstd::cerr\s*<<\s*(.*)\s*;', rf'{m["error"]}(\1);', line)

    # unstructured_log heuristic: LOG("text") -> THEMIS_LOG_INFO("text")
    line = re.sub(r'(^|\s)LOG\s*\((.*)\)\s*;', rf'\1{m["info"]}(\2);', line)
    return line

def fix(root: Path, finding: Dict[str, Any], check_only: bool):
    fp = root / finding["file"]
    if not fp.exists():
        return {"status":"skipped","reason":"file_not_found","file":finding["file"],"line":finding["line"],"type":finding["type"]}

    lines = fp.read_text(encoding="utf-8", errors="ignore").splitlines(keepends=True)
    i = max(0, min(len(lines)-1, int(finding["line"]) - 1))
    before = lines[i]
    policy = _load_policy(root)
    after = _replace_line(before, policy)

    if after == before:
        return {"status":"skipped","reason":"no_safe_logging_pattern","file":finding["file"],"line":finding["line"],"type":finding["type"]}

    lines[i] = after if after.endswith("\n") else after + "\n"
    if not check_only:
        fp.write_text("".join(lines), encoding="utf-8")

    return {"status":"fixed","reason":"logging_policy_rewrite","file":finding["file"],"line":finding["line"],"type":finding["type"],"before":before,"after":lines[i]}
