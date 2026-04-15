"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            check_openapi_completeness.py                      ║
  Version:         0.0.9                                              ║
  Last Modified:   2026-04-15 07:11:10                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     189                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • dac91fef60  2026-04-04  Add local production readiness checklist and OpenAPI comp... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
"""
Local OpenAPI completeness checker (no CI dependency).

Compares HTTP route hints found in source comments against documented OpenAPI
paths in openapi/openapi.yaml.

Route hint format expected in C++ files:
    // GET /v2/jobs
    // POST /api/v1/diff

Outputs JSON report and exits non-zero when undocumented endpoints are found.
"""

from __future__ import annotations

import argparse
import json
import re
from pathlib import Path
from typing import Dict, List, Set, Tuple


ROUTE_RE = re.compile(r"^\s*//\s*(GET|POST|PUT|PATCH|DELETE|HEAD|OPTIONS)\s+(/[^\s]+)\s*$")
OPENAPI_PATH_RE = re.compile(r"^\s{2}(/[^:]+):\s*$")
PATH_PARAM_RE = re.compile(r"\{[^{}]+\}")


def normalize_path(path: str) -> str:
    # Remove query string if present and normalize duplicate slashes.
    path = path.split("?", 1)[0].strip()
    while "//" in path:
        path = path.replace("//", "/")
    return path


def is_literal_path(path: str) -> bool:
    # Ignore template/documentation hints that are not literal routable paths.
    if "<" in path or ">" in path:
        return False
    if "[" in path or "]" in path:
        return False
    return True


def to_openapi_style(path: str) -> str:
    # Convert :id style to {id} to improve matching quality.
    parts = path.split("/")
    converted = []
    for p in parts:
        if p.startswith(":") and len(p) > 1:
            converted.append("{" + p[1:] + "}")
        else:
            converted.append(p)
    return "/".join(converted)


def canonicalize_path(path: str) -> str:
    # Normalize path-template variable names so
    # /x/{id} == /x/{job_id} for completeness checks.
    return PATH_PARAM_RE.sub("{}", path)


def collect_source_routes(src_root: Path) -> Dict[str, Set[str]]:
    routes: Dict[str, Set[str]] = {}
    for cpp in src_root.rglob("*.cpp"):
        try:
            lines = cpp.read_text(encoding="utf-8", errors="ignore").splitlines()
        except OSError:
            continue
        for line in lines:
            m = ROUTE_RE.match(line)
            if not m:
                continue
            method = m.group(1)
            path = to_openapi_style(normalize_path(m.group(2)))
            if not is_literal_path(path):
                continue
            routes.setdefault(method, set()).add(path)
    return routes


def collect_openapi_paths(openapi_file: Path) -> Set[str]:
    paths: Set[str] = set()
    lines = openapi_file.read_text(encoding="utf-8", errors="ignore").splitlines()
    in_paths = False
    for line in lines:
        if line.strip() == "paths:":
            in_paths = True
            continue
        if in_paths and line and not line.startswith(" "):
            # Leaving top-level paths section.
            break
        if not in_paths:
            continue

        m = OPENAPI_PATH_RE.match(line)
        if m:
            paths.add(normalize_path(m.group(1)))
    return paths


def load_allowlist(path: Path | None) -> Set[str]:
    if not path or not path.exists():
        return set()
    allowed = set()
    for line in path.read_text(encoding="utf-8", errors="ignore").splitlines():
        s = line.strip()
        if not s or s.startswith("#"):
            continue
        allowed.add(normalize_path(s))
    return allowed


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--repo-root", required=True)
    parser.add_argument("--openapi", default="openapi/openapi.yaml")
    parser.add_argument("--src", default="src/server")
    parser.add_argument("--allowlist", default="scripts/operations/openapi_allowlist.txt")
    parser.add_argument("--output", required=True)
    args = parser.parse_args()

    root = Path(args.repo_root).resolve()
    openapi_file = (root / args.openapi).resolve()
    src_root = (root / args.src).resolve()
    allowlist_file = (root / args.allowlist).resolve()
    output_file = Path(args.output).resolve()

    openapi_paths = collect_openapi_paths(openapi_file)
    source_routes = collect_source_routes(src_root)
    allowlist = load_allowlist(allowlist_file)

    canonical_openapi_paths = {canonicalize_path(p) for p in openapi_paths}

    undocumented: List[Tuple[str, str]] = []
    for method, paths in sorted(source_routes.items()):
        for p in sorted(paths):
            if p in allowlist:
                continue
            if p not in openapi_paths and canonicalize_path(p) not in canonical_openapi_paths:
                undocumented.append((method, p))

    report = {
        "openapi_file": str(openapi_file),
        "source_root": str(src_root),
        "allowlist_file": str(allowlist_file),
        "openapi_path_count": len(openapi_paths),
        "openapi_canonical_path_count": len(canonical_openapi_paths),
        "source_route_count": sum(len(v) for v in source_routes.values()),
        "undocumented_route_count": len(undocumented),
        "undocumented_routes": [
            {"method": method, "path": path} for method, path in undocumented
        ],
    }

    output_file.parent.mkdir(parents=True, exist_ok=True)
    output_file.write_text(json.dumps(report, indent=2), encoding="utf-8")

    print(f"OpenAPI completeness report written: {output_file}")
    print(f"Undocumented routes: {len(undocumented)}")

    return 1 if undocumented else 0


if __name__ == "__main__":
    raise SystemExit(main())
