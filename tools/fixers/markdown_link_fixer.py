#!/usr/bin/env python3
# -*- coding: utf-8 -*-
from __future__ import annotations
import re
from pathlib import Path
from typing import Dict, Any

def fix(root: Path, finding: Dict[str, Any], check_only: bool):
    fp = root / finding["file"]
    if not fp.exists():
        return {"status":"skipped","reason":"file_not_found","file":finding["file"],"line":finding["line"],"type":finding["type"]}

    lines = fp.read_text(encoding="utf-8", errors="ignore").splitlines(keepends=True)
    i = max(0, min(len(lines)-1, int(finding["line"]) - 1))
    before = lines[i]

    m = re.search(r'\[([^\]]+)\]\(([^)]+)\)', before)
    if not m:
        return {"status":"skipped","reason":"no_markdown_link","file":finding["file"],"line":finding["line"],"type":finding["type"]}

    link = m.group(2)
    if link.startswith(("http://","https://","#","mailto:")):
        return {"status":"skipped","reason":"external_or_anchor","file":finding["file"],"line":finding["line"],"type":finding["type"]}

    target = (fp.parent / link).resolve()
    if target.exists():
        return {"status":"skipped","reason":"already_valid","file":finding["file"],"line":finding["line"],"type":finding["type"]}

    alt = (fp.parent / link.lower()).resolve()
    if alt.exists():
        rel = alt.relative_to(fp.parent.resolve()).as_posix()
        after = before.replace(f"]({link})", f"]({rel})")
        lines[i] = after
        if not check_only:
            fp.write_text("".join(lines), encoding="utf-8")
        return {"status":"fixed","reason":"normalized_lowercase_target","file":finding["file"],"line":finding["line"],"type":finding["type"],"before":before,"after":after}

    return {"status":"unsafe","reason":"cannot_resolve_target","file":finding["file"],"line":finding["line"],"type":finding["type"]}
