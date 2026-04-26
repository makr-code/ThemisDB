"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            fix_doc_links.py                                   ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:48:20                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     303                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

"""Auto-repair relative Markdown links by pointing them to existing files.

Usage:
    python scripts/fix_doc_links.py [--base docs] [--lang de]

Behavior:
    - Scans Markdown files under the selected base (optionally scoped to a language subfolder).
    - For each relative link whose target file does not exist, it searches for a file with the
      same basename elsewhere under the base directory. If exactly one match is found, it rewrites
      the link to the correct relative path (preserving any fragment).
    - Leaves external links (http, https, mailto, data, tel) and pure anchors untouched.
    - Emits a summary of fixes and unresolved links.

Limitations:
    - Only fixes paths by basename matching; if multiple candidates exist, the link is left
      unchanged and logged as unresolved.
    - Does not modify directory links or anchors-only links.
"""

from __future__ import annotations

import argparse
import os
import re
from collections import defaultdict
from pathlib import Path
from typing import Dict, Iterable, List, Mapping, Sequence, Tuple

import yaml


MARKDOWN_EXTENSIONS = {".md", ".markdown"}

# Hard-coded preferred targets for common ambiguous basenames.
PREFERRED_TARGETS = {
    "README.md": ["de/{current_dir}/README.md", "de/README.md"],
    "INDEX.md": ["de/INDEX.md"],
    "DOCUMENTATION_INDEX.md": ["de/DOCUMENTATION_INDEX.md"],
    "CONTENT_SEARCH_API.md": ["de/search/content_search_api.md"],
    "VECTOR_AUTO_BUFFER.md": ["de/search/VECTOR_AUTO_BUFFER.md"],
    "AQL_REFERENCE.md": ["de/aql/aql_functions_reference.md", "de/aql/aql_syntax.md"],
    "REST_API.md": ["de/apis/HTTP_API_REFERENCE.md"],
    "BUILD_STRATEGY.md": ["de/guides/guides_build_strategy.md", "de/guides/guides_build.md"],
    "CONFIGURATION.md": ["de/guides/CONFIGURATION_TUNING_GUIDE.md", "de/guides/guides_operations_runbook.md"],
    "INSTALLATION.md": ["de/guides/QUICK_START.md"],
    "DEPLOYMENT.md": ["de/guides/guides_deployment.md"],
    "operations_runbook.md": ["de/guides/guides_operations_runbook.md"],
    "FEATURES.md": ["de/features/features_overview.md"],
    "SECURITY.md": ["de/security/README.md", "SECURITY.md"],
    "SECURITY_SIGNATURES.md": ["de/security/security_signatures.md"],
    "TLS_SETUP.md": ["de/guides/guides_tls_setup.md"],
    "AUDIT_LOGGING.md": ["de/features/features_audit_logging.md"],
    "GUIDE_TRANSACTIONS.md": ["de/features/features_transactions.md"],
    "FEATURE_VECTOR_SEARCH.md": ["de/features/features_vector_ops.md", "de/features/features_overview.md"],
    "FEATURE_GRAPH_TRAVERSAL.md": ["de/features/features_recursive_path.md", "de/features/features_property_graph.md"],
    "CONTRIBUTING.md": ["CONTRIBUTING.md"],
    "LICENSE": ["LICENSE"],
    "openapi.yaml": ["docs/openapi.yaml"],
}


def iter_markdown_files(base: Path, lang: str | None) -> Iterable[Path]:
    root = base / lang if lang else base
    for path in root.rglob("*"):
        if path.is_file() and path.suffix.lower() in MARKDOWN_EXTENSIONS:
            yield path


def collect_candidates(scope: Path) -> Dict[str, List[Path]]:
    candidates: Dict[str, List[Path]] = defaultdict(list)
    for path in scope.rglob("*"):
        if path.is_file() and path.suffix.lower() in MARKDOWN_EXTENSIONS:
            candidates[path.name].append(path)
    return candidates


LINK_PATTERN = re.compile(r"\[(?P<text>[^\]]+)\]\((?P<target>[^)]+)\)")
REF_DEF_PATTERN = re.compile(r"^\[(?P<label>[^\]]+)\]:\s*(?P<target>\S+)")

SKIP_PREFIXES = ("http://", "https://", "mailto:", "data:", "tel:")


def split_target(target: str) -> Tuple[str, str]:
    target = target.strip()
    if "#" in target:
        path_part, frag = target.split("#", 1)
        return path_part, f"#{frag}"
    return target, ""


def normalize_target(target: str) -> str:
    # Strip surrounding angle brackets often used in Markdown links.
    target = target.strip()
    if target.startswith("<") and target.endswith(">"):
        target = target[1:-1].strip()
    return target


def score_candidate(path: Path, scope_root: Path, priorities: Sequence[str]) -> Tuple[int, int]:
    """Return a sortable score: lower is better. First by priority folder, then by path depth."""
    try:
        rel = path.relative_to(scope_root)
    except ValueError:
        rel = path
    parts = rel.parts
    top = parts[0] if parts else ""
    # Priority score: index in priorities or len(priorities) if not found.
    prio_score = priorities.index(top) if top in priorities else len(priorities)
    depth = len(parts)
    return prio_score, depth


def resolve_candidate(
    file_path: Path,
    path_part: str,
    candidates: Dict[str, List[Path]],
    scope_root: Path,
    priorities: Sequence[str],
    manual_map: Mapping[str, Mapping[str, str]],
    base_root: Path,
) -> Tuple[str | None, str]:
    """Return new relative path (if resolvable uniquely) and a reason tag."""

    anchorless = Path(path_part)
    # Skip anchors and directory-style links.
    if path_part.startswith("#") or path_part.endswith("/"):
        return None, "skip-anchor-or-dir"

    # Already valid?
    if (file_path.parent / anchorless).exists():
        return None, "exists"

    # Manual mapping override (per-file or __all__). Values are relative to scope_root.
    rel_key = str(file_path.relative_to(base_root))
    manual_target = None
    if rel_key in manual_map:
        manual_target = manual_map[rel_key].get(path_part)
    if manual_target is None and "__all__" in manual_map:
        manual_target = manual_map["__all__"].get(path_part)
    if manual_target:
        target_path = (scope_root / manual_target).resolve()
        if target_path.exists():
            rel_str = Path(os.path.relpath(target_path, start=file_path.parent)).as_posix()
            return rel_str, "fixed-mapping"

    matches = candidates.get(anchorless.name, [])

    # Preferred mapping first.
    if anchorless.name in PREFERRED_TARGETS:
        for tmpl in PREFERRED_TARGETS[anchorless.name]:
            target_str = tmpl.replace("{current_dir}", file_path.parent.relative_to(scope_root).as_posix())
            target_path = (scope_root / target_str).resolve()
            if target_path.exists():
                rel_str = Path(os.path.relpath(target_path, start=file_path.parent)).as_posix()
                return rel_str, "fixed-preferred"
    if len(matches) == 1:
        rel_str = Path(os.path.relpath(matches[0], start=file_path.parent)).as_posix()
        return rel_str, "fixed"
    if len(matches) > 1:
        sorted_matches = sorted(matches, key=lambda p: score_candidate(p, scope_root, priorities))
        best = sorted_matches[0]
        if score_candidate(best, scope_root, priorities) != score_candidate(sorted_matches[1], scope_root, priorities):
            rel_str = Path(os.path.relpath(best, start=file_path.parent)).as_posix()
            return rel_str, "fixed-priority"
        return None, "ambiguous"
    return None, "missing"


def process_file(
    file_path: Path,
    candidates: Dict[str, List[Path]],
    scope_root: Path,
    base_root: Path,
    priorities: Sequence[str],
    manual_map: Mapping[str, Mapping[str, str]],
) -> Tuple[int, List[str]]:
    content = file_path.read_text(encoding="utf-8")
    replacements: List[Tuple[int, int, str]] = []
    unresolved: List[str] = []

    def handle_match(match: re.Match, is_ref: bool) -> None:
        target = normalize_target(match.group("target"))
        if not target or target.startswith(SKIP_PREFIXES):
            return
        path_part, frag = split_target(target)
        new_rel, reason = resolve_candidate(
            file_path,
            path_part,
            candidates,
            scope_root,
            priorities,
            manual_map,
            base_root,
        )
        if new_rel:
            new_target = f"{new_rel}{frag}"
            replacements.append((match.start("target"), match.end("target"), new_target))
        elif reason not in {"exists", "skip-anchor-or-dir"}:
            unresolved.append(f"{target} ({reason})")

    for m in LINK_PATTERN.finditer(content):
        handle_match(m, is_ref=False)

    for m in REF_DEF_PATTERN.finditer(content):
        handle_match(m, is_ref=True)

    if replacements:
        # Apply replacements from the end to keep offsets valid.
        parts = list(content)
        for start, end, new_val in sorted(replacements, key=lambda x: x[0], reverse=True):
            parts[start:end] = new_val
        content = "".join(parts)
        file_path.write_text(content, encoding="utf-8")

    return len(replacements), unresolved


def load_manual_map(path: str | None) -> Dict[str, Dict[str, str]]:
    if not path:
        return {}
    map_path = Path(path)
    if not map_path.exists():
        raise SystemExit(f"Mapping file not found: {map_path}")
    data = yaml.safe_load(map_path.read_text(encoding="utf-8")) or {}
    manual_map: Dict[str, Dict[str, str]] = {}
    for file_key, mapping in data.items():
        if mapping is None:
            continue
        manual_map[str(file_key)] = {str(k): str(v) for k, v in mapping.items()}
    return manual_map


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--base", default="docs", help="Root docs directory")
    parser.add_argument("--lang", default=None, help="Language subfolder to restrict to (e.g. de)")
    parser.add_argument(
        "--priority",
        default="de,compiled,reports,features,guides,security,performance,plugins,deployment,development,enterprise,observability,timeseries,storage,search,content,connectors,compliance,governance,geo,clients,query,aql,apis,architecture,server,scheduler,strategy,legal,llm,tools",
        help="Comma-separated folder priorities (top-level under base); earlier wins when ambiguous",
    )
    parser.add_argument("--mapping", help="YAML mapping file: file -> {broken_path: fixed_relative_target}")
    parser.add_argument("--unresolved-report", help="Write unresolved links to this YAML file")
    args = parser.parse_args()

    base = Path(args.base).resolve()
    if not base.exists():
        raise SystemExit(f"Base directory not found: {base}")

    scope = base / args.lang if args.lang else base
    priorities = [p.strip() for p in args.priority.split(",") if p.strip()]
    manual_map = load_manual_map(args.mapping)
    candidates = collect_candidates(scope)
    total_fixes = 0
    unresolved_total: List[Tuple[Path, str]] = []

    for md_file in iter_markdown_files(base, args.lang):
        fixed, unresolved = process_file(md_file, candidates, scope, base, priorities, manual_map)
        total_fixes += fixed
        for item in unresolved:
            unresolved_total.append((md_file, item))

    print(f"Fixed links: {total_fixes}")
    if unresolved_total:
        print("Unresolved links:")
        for path, target in unresolved_total:
            rel = path.relative_to(base)
            print(f"  {rel}: {target}")

    if args.unresolved_report:
        report_path = Path(args.unresolved_report)
        grouped: Dict[str, List[str]] = defaultdict(list)
        for path, target in unresolved_total:
            grouped[str(path.relative_to(base))].append(target)
        report_path.parent.mkdir(parents=True, exist_ok=True)
        report_path.write_text(yaml.safe_dump(dict(grouped), sort_keys=True, allow_unicode=False), encoding="utf-8")
        print(f"Unresolved report written to {report_path}")


if __name__ == "__main__":
    main()