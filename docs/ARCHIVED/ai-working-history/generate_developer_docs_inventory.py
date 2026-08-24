#!/usr/bin/env python3
"""Generate a baseline inventory for developer-facing documentation.

The script scans `src/`, `include/`, `tests/`, and `benchmarks/` and writes a
JSON snapshot plus a human-readable Markdown report. The output is intended to
support Phase 0 of the developer documentation consolidation workflow.
"""

from __future__ import annotations

import argparse
import json
import re
from collections import Counter, defaultdict
from dataclasses import dataclass, asdict
from datetime import datetime
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
DEFAULT_OUTPUT_JSON = ROOT / "ai_working" / "developer_docs_inventory.json"
DEFAULT_OUTPUT_MD = ROOT / "ai_working" / "developer_docs_inventory_report.md"
AREAS = ("src", "include", "tests", "benchmarks")
CANONICAL_FILES = (
    "README.md",
    "MODULE_GAPS.md",
    "ROADMAP.md",
    "FUTURE_ENHANCEMENTS.md",
)

CORE_DOC_FILES = (
    "README.md",
    "ARCHITECTURE.md",
    "MODULE_GAPS.md",
    "ROADMAP.md",
    "FUTURE_ENHANCEMENTS.md",
    "SECURITY.md",
    "AUDIT.md",
    "PERFORMANCE_EXPECTATIONS.md",
    "PRODUCTION_REQUIREMENTS.md",
    "VCCDB Design.md",
)

SRC_MARKDOWN_TYPE_CLASSIFICATION = {
    "README.md": {
        "tier": "core",
        "rationale": "Module entry point, scope, build, and usage contract.",
    },
    "ARCHITECTURE.md": {
        "tier": "core",
        "rationale": "Primary structural and design reference for implementation work.",
    },
    "MODULE_GAPS.md": {
        "tier": "core",
        "rationale": "Tracks known implementation gaps and follow-up work.",
    },
    "ROADMAP.md": {
        "tier": "core",
        "rationale": "Master backlog and planning source for upcoming work.",
    },
    "FUTURE_ENHANCEMENTS.md": {
        "tier": "core",
        "rationale": "Actionable future-work specification with constraints and tests.",
    },
    "SECURITY.md": {
        "tier": "core",
        "rationale": "Security requirements, validation rules, and hardening guidance.",
    },
    "AUDIT.md": {
        "tier": "core",
        "rationale": "Evidence-backed review record for production-critical source paths.",
    },
    "PERFORMANCE_EXPECTATIONS.md": {
        "tier": "core",
        "rationale": "Module-level performance contract and tuning targets.",
    },
    "PRODUCTION_REQUIREMENTS.md": {
        "tier": "core",
        "rationale": "Operational requirements that define production-ready behavior.",
    },
    "VCCDB Design.md": {
        "tier": "core",
        "rationale": "Core architecture/design note for the primary source system.",
    },
    "ADVANCED_FEATURES_README.md": {
        "tier": "specialist",
        "rationale": "Developer-facing reference for advanced feature behavior.",
    },
    "AI_ML_IMPACT_ASSESSMENT.md": {
        "tier": "specialist",
        "rationale": "Governance and impact analysis for ML/AI source touchpoints.",
    },
    "AI_SAFETY_ARCHITECTURE.md": {
        "tier": "specialist",
        "rationale": "Safety and security reference for AI-related code paths.",
    },
    "CHANGELOG.md": {
        "tier": "specialist",
        "rationale": "Historical change record that supports maintenance and review.",
    },
    "CONSOLIDATION_ANALYSIS.md": {
        "tier": "specialist",
        "rationale": "Refactoring and deduplication analysis for source modules.",
    },
    "GGUF_LOADER_README.md": {
        "tier": "specialist",
        "rationale": "Feature-specific loader reference for model integration work.",
    },
    "LLAMA_LORA_ADAPTER_README.md": {
        "tier": "specialist",
        "rationale": "Feature-specific adapter reference for model integration work.",
    },
    "MODULE_FUNCTION_USAGE_MAP.md": {
        "tier": "specialist",
        "rationale": "Static usage map that supports impact analysis and refactoring.",
    },
    "QUALITY_CONTROL_README.md": {
        "tier": "specialist",
        "rationale": "Quality-control workflow and verification reference.",
    },
    "STUB_INVENTORY.md": {
        "tier": "specialist",
        "rationale": "Inventory of non-production code paths and simulation markers.",
    },
    "UNUSED_FUNCTIONS_REPORT.md": {
        "tier": "specialist",
        "rationale": "Maintenance report for dead or unreferenced source symbols.",
    },
    "VECTOR_ADVANCED_FEATURES_README.md": {
        "tier": "specialist",
        "rationale": "Advanced vector feature reference for source-level implementation work.",
    },
}

INCLUDE_FOCUS_PRIORITY = {
    "MODULE_GAPS.md": {
        "priority": "must_first",
        "reason": "Directly tracks API-contract and implementation drift against headers.",
    },
    "PERFORMANCE_EXPECTATIONS.md": {
        "priority": "must_first",
        "reason": "Defines public-facing performance expectations for API consumers.",
    },
    "PRODUCTION_REQUIREMENTS.md": {
        "priority": "must_first",
        "reason": "Captures production-readiness constraints tied to the public interface.",
    },
    "AI_SAFETY_ARCHITECTURE.md": {
        "priority": "later",
        "reason": "Important specialist guardrails, but not required for every API surface change.",
    },
    "AI_ML_IMPACT_ASSESSMENT.md": {
        "priority": "later",
        "reason": "Governance context helps review decisions after core contract docs are in place.",
    },
    "ADVANCED_FEATURES_README.md": {
        "priority": "later",
        "reason": "Useful for advanced behavior, secondary to baseline API contract docs.",
    },
    "QUALITY_CONTROL_README.md": {
        "priority": "later",
        "reason": "Quality pipeline details support implementation hardening after core docs.",
    },
    "MODULE_FUNCTION_USAGE_MAP.md": {
        "priority": "later",
        "reason": "Impact-analysis aid that improves refactoring workflows.",
    },
    "STUB_INVENTORY.md": {
        "priority": "later",
        "reason": "Supports governance and cleanup but is not a first-line API contract artifact.",
    },
    "UNUSED_FUNCTIONS_REPORT.md": {
        "priority": "later",
        "reason": "Maintenance artifact for cleanup cycles rather than initial include alignment.",
    },
    "CONSOLIDATION_ANALYSIS.md": {
        "priority": "later",
        "reason": "Planning aid that can follow after baseline include parity.",
    },
    "GGUF_LOADER_README.md": {
        "priority": "referential",
        "reason": "Feature-specific implementation reference, mostly relevant to owning subteams.",
    },
    "LLAMA_LORA_ADAPTER_README.md": {
        "priority": "referential",
        "reason": "Feature-specific implementation reference, mostly relevant to owning subteams.",
    },
    "VECTOR_ADVANCED_FEATURES_README.md": {
        "priority": "referential",
        "reason": "Specialized vector feature reference, not required for baseline include contracts.",
    },
    "VCCDB Design.md": {
        "priority": "referential",
        "reason": "Broad architecture context document, useful as supplemental reference.",
    },
}

SECTION_PATTERNS = {
    "overview": re.compile(r"^##\s+.*(Overview|Einführung|Introduction|Zweck|Purpose)", re.IGNORECASE),
    "usage": re.compile(r"^##\s+.*(Usage|Nutzung|How to use|Use|Einbindung)", re.IGNORECASE),
    "structure": re.compile(r"^##\s+.*(Struktur|Structure|Layout)", re.IGNORECASE),
    "entry": re.compile(r"^##\s+.*(Einstieg|Entry|Start|Index|Navigation)", re.IGNORECASE),
    "links": re.compile(r"^##\s+.*(Bezug|Links|References|Related|Navigation)", re.IGNORECASE),
    "tasks": re.compile(r"^##\s+.*(Offene Doku-Tasks|Tasks|Open Tasks|TODO|Roadmap)", re.IGNORECASE),
}


@dataclass(slots=True)
class AreaStats:
    readme_count: int
    module_gap_count: int
    roadmap_count: int
    future_count: int


def find_module_root(path: Path, area_root: Path) -> str:
    relative = path.relative_to(area_root)
    return relative.parts[0] if relative.parts else "<root>"


def collect_readmes(area_root: Path) -> list[Path]:
    return sorted(area_root.rglob("README.md")) if area_root.exists() else []


def collect_counts(area_root: Path) -> AreaStats:
    readmes = list(area_root.rglob("README.md")) if area_root.exists() else []
    gaps = list(area_root.rglob("MODULE_GAPS.md")) if area_root.exists() else []
    roadmaps = list(area_root.rglob("ROADMAP.md")) if area_root.exists() else []
    futures = list(area_root.rglob("FUTURE_ENHANCEMENTS.md")) if area_root.exists() else []
    return AreaStats(len(readmes), len(gaps), len(roadmaps), len(futures))


def collect_markdown_files(area_root: Path) -> list[Path]:
    return sorted(p for p in area_root.rglob("*") if p.is_file() and p.suffix.lower() == ".md") if area_root.exists() else []


def extract_section_hits(readme: Path) -> list[str]:
    hits: list[str] = []
    try:
        lines = readme.read_text(encoding="utf-8", errors="ignore").splitlines()
    except OSError:
        return hits

    for line in lines:
        for name, pattern in SECTION_PATTERNS.items():
            if pattern.search(line):
                hits.append(name)
    return sorted(set(hits))


def canonical_refs_missing(text: str) -> list[str]:
    missing = []
    for name in CANONICAL_FILES:
        if name not in text:
            missing.append(name)
    return missing


def classify_src_markdown_type(filename: str) -> dict[str, str]:
    classification = SRC_MARKDOWN_TYPE_CLASSIFICATION.get(
        filename,
        {
            "tier": "needs_review",
            "rationale": "Unclassified src markdown type; needs manual review.",
        },
    )
    return {
        "filename": filename,
        "tier": classification["tier"],
        "developer_relevant": "yes",
        "rationale": classification["rationale"],
    }


def build_src_markdown_type_classification(area_root: Path) -> list[dict[str, str]]:
    filenames = sorted({path.name for path in collect_markdown_files(area_root)})
    return [classify_src_markdown_type(filename) for filename in filenames]


def build_include_focus_priority(missing_types: list[str]) -> dict[str, list[dict[str, str]]]:
    buckets: dict[str, list[dict[str, str]]] = {
        "must_first": [],
        "later": [],
        "referential": [],
        "unclassified": [],
    }
    for filename in missing_types:
        priority_info = INCLUDE_FOCUS_PRIORITY.get(filename)
        if priority_info is None:
            buckets["unclassified"].append(
                {
                    "filename": filename,
                    "priority": "unclassified",
                    "reason": "No explicit include-priority mapping defined yet.",
                }
            )
            continue
        buckets[priority_info["priority"]].append(
            {
                "filename": filename,
                "priority": priority_info["priority"],
                "reason": priority_info["reason"],
            }
        )
    return buckets


def format_mtime(path: Path) -> str:
    if not path.exists():
        return "—"
    return datetime.fromtimestamp(path.stat().st_mtime).strftime("%Y-%m-%d")


def filename_to_key(filename: str) -> str:
    return re.sub(r"[^A-Za-z0-9]", "_", filename).upper()


def build_module_doc_matrix(area_root: Path) -> list[dict[str, str]]:
    rows: list[dict[str, str]] = []
    if not area_root.exists():
        return rows

    module_dirs = sorted(p for p in area_root.iterdir() if p.is_dir())
    for module_dir in module_dirs:
        row = {"module": module_dir.name}
        core_present = 0
        newest_core = None
        for core_filename in CORE_DOC_FILES:
            core_path = module_dir / core_filename
            mtime = format_mtime(core_path)
            row[filename_to_key(core_filename)] = mtime
            if mtime != "—":
                core_present += 1
                if newest_core is None or core_path.stat().st_mtime > newest_core.stat().st_mtime:
                    newest_core = core_path

        row["CORE_DOCS_PRESENT"] = f"{core_present}/{len(CORE_DOC_FILES)}"
        row["NEWEST_CORE_DOC"] = (
            f"{newest_core.relative_to(area_root).as_posix()} @ {format_mtime(newest_core)}" if newest_core else "—"
        )
        rows.append(row)
    return rows


def build_filename_matrix(area_root: Path) -> tuple[list[str], list[dict[str, str]]]:
    filenames = sorted({path.name for path in collect_markdown_files(area_root)})
    rows: list[dict[str, str]] = []
    if not area_root.exists():
        return filenames, rows

    row_names = sorted(
        {"<root>" if len(path.relative_to(area_root).parts) == 1 else path.relative_to(area_root).parts[0] for path in collect_markdown_files(area_root)}
    )
    for row_name in row_names:
        row = {"module": row_name}
        for filename in filenames:
            if row_name == "<root>":
                matches = [path for path in area_root.glob(filename) if path.is_file()]
            else:
                row_root = area_root / row_name
                matches = [path for path in row_root.rglob(filename) if path.is_file()]
            if matches:
                newest = max(matches, key=lambda path: path.stat().st_mtime)
                row[filename] = format_mtime(newest)
            else:
                row[filename] = "—"
        rows.append(row)
    return filenames, rows


def build_inventory() -> dict:
    inventory: dict[str, object] = {
        "generated_by": Path(__file__).name,
        "root": str(ROOT),
        "areas": {},
        "src": {},
        "section_coverage": {},
        "missing_refs": {},
        "module_with_most_gaps": [],
        "module_doc_matrix": [],
        "area_filename_matrices": {},
        "missing_markdown_types_by_area": {},
        "all_markdown_types": [],
        "src_markdown_type_classification": [],
        "developer_relevant_src_markdown_types": [],
        "include_focus_missing_relevant_types": [],
        "include_focus_priority": {},
    }

    all_markdown_types = sorted(
        {
            path.name
            for area in AREAS
            for path in collect_markdown_files(ROOT / area)
        }
    )
    inventory["all_markdown_types"] = all_markdown_types

    for area in AREAS:
        area_root = ROOT / area
        stats = collect_counts(area_root)
        inventory["areas"][area] = asdict(stats)

        markdown_files = collect_markdown_files(area_root)
        uppercase_md = [path.relative_to(ROOT).as_posix() for path in markdown_files if path.suffix == ".MD" or path.name.endswith(".MD")]
        inventory.setdefault("markdown_inventory", {})[area] = {
            "count": len(markdown_files),
            "uppercase_md": uppercase_md,
        }

        filename_columns, filename_rows = build_filename_matrix(area_root)
        inventory.setdefault("area_filename_matrices", {})[area] = {
            "columns": filename_columns,
            "rows": filename_rows,
        }
        inventory.setdefault("missing_markdown_types_by_area", {})[area] = [
            filename for filename in all_markdown_types if filename not in filename_columns
        ]

        if area == "src":
            readmes = collect_readmes(area_root)
            top_modules = sorted({find_module_root(path, area_root) for path in readmes if len(path.relative_to(area_root).parts) >= 2})
            module_docs = {}
            missing_refs = {}
            section_counter = Counter()

            for module in top_modules:
                module_root = area_root / module
                readme = module_root / "README.md"
                if not readme.exists():
                    continue
                text = readme.read_text(encoding="utf-8", errors="ignore")
                section_hits = extract_section_hits(readme)
                for section in section_hits:
                    section_counter[section] += 1

                module_docs[module] = {
                    "readme": str(readme.relative_to(ROOT)),
                    "has_module_gaps": (module_root / "MODULE_GAPS.md").exists(),
                    "has_roadmap": (module_root / "ROADMAP.md").exists(),
                    "has_future_enhancements": (module_root / "FUTURE_ENHANCEMENTS.md").exists(),
                    "section_hits": section_hits,
                    "missing_canonical_refs": canonical_refs_missing(text),
                }
                if module_docs[module]["missing_canonical_refs"]:
                    missing_refs[module] = module_docs[module]["missing_canonical_refs"]

            inventory["src"] = {
                "module_readmes": len(top_modules),
                "modules": module_docs,
                "markdown_files": len(markdown_files),
            }
            inventory["section_coverage"] = dict(section_counter)
            inventory["missing_refs"] = missing_refs
            inventory["module_doc_matrix"] = build_module_doc_matrix(area_root)
            src_classification = build_src_markdown_type_classification(area_root)
            inventory["src_markdown_type_classification"] = src_classification
            inventory["developer_relevant_src_markdown_types"] = [
                row["filename"] for row in src_classification if row["developer_relevant"] == "yes"
            ]

    include_columns = inventory.get("area_filename_matrices", {}).get("include", {}).get("columns", [])
    inventory["include_focus_missing_relevant_types"] = [
        filename
        for filename in inventory.get("developer_relevant_src_markdown_types", [])
        if filename not in include_columns
    ]
    inventory["include_focus_priority"] = build_include_focus_priority(inventory["include_focus_missing_relevant_types"])

    gap_aggregate = ROOT / "ai_working" / "gap_scan_v3_aggregate.json"
    if gap_aggregate.exists():
        try:
            data = json.loads(gap_aggregate.read_text(encoding="utf-8"))
            module_totals = []
            for module, payload in data.items():
                if isinstance(payload, list):
                    module_totals.append((module, len(payload)))
                elif isinstance(payload, dict):
                    total = payload.get("total")
                    if isinstance(total, int):
                        module_totals.append((module, total))
            inventory["module_with_most_gaps"] = sorted(module_totals, key=lambda item: item[1], reverse=True)[:20]
        except Exception as exc:  # pragma: no cover - defensive reporting path
            inventory["module_with_most_gaps_error"] = str(exc)

    return inventory


def render_markdown(report: dict) -> str:
    lines: list[str] = []
    lines.append("# Developer Documentation Phase 0 Baseline")
    lines.append("")
    lines.append(f"**Generated By:** {report.get('generated_by')}")
    lines.append(f"**Root:** {report.get('root')}")
    lines.append("")
    lines.append("## Area Summary")
    lines.append("")
    lines.append("| Area | README.md | MODULE_GAPS.md | ROADMAP.md | FUTURE_ENHANCEMENTS.md |")
    lines.append("|---|---:|---:|---:|---:|")
    for area in AREAS:
        stats = report["areas"][area]
        lines.append(
            f"| {area} | {stats['readme_count']} | {stats['module_gap_count']} | {stats['roadmap_count']} | {stats['future_count']} |"
        )
    lines.append("")
    lines.append("## Source Documentation Coverage")
    lines.append("")
    lines.append(f"- Top-level source modules with READMEs: {report['src'].get('module_readmes', 0)}")
    lines.append(f"- Markdown files under src/: {report['src'].get('markdown_files', 0)}")
    lines.append(f"- Modules missing at least one canonical reference: {len(report.get('missing_refs', {}))}")
    lines.append("")
    lines.append("### Markdown Inventory By Area")
    if report.get("markdown_inventory"):
        for area in AREAS:
            area_info = report["markdown_inventory"].get(area, {})
            lines.append(f"- {area}: {area_info.get('count', 0)} markdown files")
    lines.append("")
    lines.append("### Section Vocabulary Frequency")
    if report.get("section_coverage"):
        for name, count in sorted(report["section_coverage"].items(), key=lambda item: (-item[1], item[0])):
            lines.append(f"- {name}: {count}")
    else:
        lines.append("- No section hits detected")
    lines.append("")
    lines.append("## Developer-Relevant Src Markdown Types")
    lines.append("")
    lines.append("- All currently observed `src/` markdown types are developer-relevant; the practical split is core contract versus specialist support.")
    lines.append("")
    lines.append("| File | Tier | Developer-Relevant | Rationale |")
    lines.append("|---|---|---|---|")
    for row in report.get("src_markdown_type_classification", []):
        lines.append(f"| {row['filename']} | {row['tier']} | {row['developer_relevant']} | {row['rationale']} |")
    lines.append("")
    lines.append("## Modules Missing Canonical References")
    lines.append("")
    if report.get("missing_refs"):
        for module, refs in sorted(report["missing_refs"].items()):
            lines.append(f"- {module}: {', '.join(refs)}")
    else:
        lines.append("- None")
    lines.append("")
    lines.append("## Missing Markdown Types By Area")
    lines.append("")
    lines.append(f"- All markdown types across the four areas: {len(report.get('all_markdown_types', []))}")
    lines.append("")

    include_priority = report.get("include_focus_priority", {})
    lines.append("### Include Quickwin Priorities")
    lines.append("")
    lines.append("- These buckets prioritize developer-relevant src markdown types that are currently missing in include.")
    for bucket_name, label in (
        ("must_first", "Must First"),
        ("later", "Later"),
        ("referential", "Referential"),
        ("unclassified", "Unclassified"),
    ):
        entries = include_priority.get(bucket_name, [])
        lines.append(f"- {label}: {len(entries)}")
        for entry in entries:
            lines.append(f"  - {entry['filename']}: {entry['reason']}")
    lines.append("")

    for area in AREAS:
        missing_types = report.get("missing_markdown_types_by_area", {}).get(area, [])
        lines.append(f"### {area}")
        lines.append(f"- Missing types: {len(missing_types)}")
        if area == "include":
            lines.append("- Include focus: prioritize the following developer-relevant types first; they are the most useful contract mirrors from src/.")
            include_focus = report.get("include_focus_missing_relevant_types", [])
            lines.append(f"- Missing developer-relevant types: {len(include_focus)}")
            if include_focus:
                lines.append("- " + ", ".join(include_focus))
            else:
                lines.append("- None")
        if missing_types:
            lines.append("- " + ", ".join(missing_types))
        else:
            lines.append("- None")
        lines.append("")
    lines.append("## Module Core Documentation Matrix")
    lines.append("")
    lines.append(
        "| Module | README.md | ARCHITECTURE.md | MODULE_GAPS.md | ROADMAP.md | FUTURE_ENHANCEMENTS.md | SECURITY.md | AUDIT.md | PERFORMANCE_EXPECTATIONS.md | PRODUCTION_REQUIREMENTS.md | VCCDB Design.md | CORE_DOCS_PRESENT | NEWEST_CORE_DOC |"
    )
    lines.append("|---|---|---|---|---|---|---|---|---|---|---|---:|---|")
    for row in report.get("module_doc_matrix", []):
        lines.append(
            f"| {row['module']} | {row.get('README_MD', '—')} | {row.get('ARCHITECTURE_MD', '—')} | {row.get('MODULE_GAPS_MD', '—')} | {row.get('ROADMAP_MD', '—')} | {row.get('FUTURE_ENHANCEMENTS_MD', '—')} | {row.get('SECURITY_MD', '—')} | {row.get('AUDIT_MD', '—')} | {row.get('PERFORMANCE_EXPECTATIONS_MD', '—')} | {row.get('PRODUCTION_REQUIREMENTS_MD', '—')} | {row.get('VCCDB_DESIGN_MD', '—')} | {row['CORE_DOCS_PRESENT']} | {row['NEWEST_CORE_DOC']} |"
        )
    lines.append("")
    lines.append("## Module Markdown Filename Matrix By Area")
    lines.append("")
    lines.append("- Legend: `—` means that the area row does not contain that markdown type.")
    lines.append("")
    for area in AREAS:
        matrix = report.get("area_filename_matrices", {}).get(area, {})
        filename_columns = matrix.get("columns", [])
        filename_rows = matrix.get("rows", [])
        lines.append(f"### {area}")
        lines.append("")
        lines.append(f"- Unique markdown types in {area}: {len(filename_columns)}")
        lines.append("| Module | " + " | ".join(filename_columns) + " |")
        lines.append("|---|" + "---|" * len(filename_columns))
        for row in filename_rows:
            cells = [row[filename] for filename in filename_columns]
            lines.append("| " + row["module"] + " | " + " | ".join(cells) + " |")
        lines.append("")
    lines.append("## Top Modules By Gap Count")
    lines.append("")
    if report.get("module_with_most_gaps"):
        for module, total in report["module_with_most_gaps"][:20]:
            lines.append(f"- {module}: {total}")
    else:
        lines.append("- gap_scan_v3_aggregate.json not available or not readable")
    lines.append("")
    lines.append("## Phase 0 Interpretation")
    lines.append("")
    lines.append("- `src/` is the strongest canonical source for developer docs but still needs section normalization.")
    lines.append("- Every folder in `src/` may contain more than one markdown document, so the update scope must include all `.md` files, not just the core ten contract docs.")
    lines.append("- `include/` should be aligned to API contracts and ownership, not implementation details.")
    lines.append("- `tests/` and `benchmarks/` need clearer purpose, execution, and reproducibility guidance.")
    lines.append("")
    return "\n".join(lines)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output-json", type=Path, default=DEFAULT_OUTPUT_JSON)
    parser.add_argument("--output-md", type=Path, default=DEFAULT_OUTPUT_MD)
    args = parser.parse_args()

    report = build_inventory()
    args.output_json.write_text(json.dumps(report, indent=2, ensure_ascii=False), encoding="utf-8")
    args.output_md.write_text(render_markdown(report), encoding="utf-8")
    print(f"Wrote {args.output_json}")
    print(f"Wrote {args.output_md}")
    print(f"src modules: {report['src'].get('module_readmes', 0)}")
    print(f"modules missing canonical refs: {len(report.get('missing_refs', {}))}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())