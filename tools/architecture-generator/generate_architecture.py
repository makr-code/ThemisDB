#!/usr/bin/env python3
"""
ThemisDB Architecture Generator
================================
Scans repository knowledge sources and produces:
  - architecture.json  (repo root) — machine-readable architecture model
  - architecture.md    (repo root) — Mermaid diagram rendered from the JSON

Source inputs (in priority order):
  ai_context/MODULES_AND_NAMESPACES.md     — canonical module/namespace table
  ai_context/ARCHITECTURE_CLASSIFICATION.md — tier model (T0–T5)
  ai_context/api_contracts/                — per-module API contracts
  ai_context/developer_llm_wiki/           — LLM wiki artifacts (MODULES_AND_APIS,
                                             GOVERNANCE_AND_ROADMAP, WIKI_STATUS)
  ai_working/                              — working artifacts, wave/batch status
  ARCHITECTURE.md                          — root architecture document
  ROADMAP.md / FUTURE_ENHANCEMENTS.md      — roadmap & feature planning
  docs/                                    — governance and documentation sources

Usage:
  python tools/architecture-generator/generate_architecture.py [--repo-root PATH]

Options:
  --repo-root PATH   Path to repository root (default: two levels up from this file)
  --json-out PATH    Output JSON path (default: <repo-root>/architecture.json)
  --md-out PATH      Output Markdown path (default: <repo-root>/architecture.md)
  --dry-run          Print outputs to stdout instead of writing files
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import re
import sys
from datetime import datetime, timezone
from pathlib import Path
from typing import Any

# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------

TIER_NAMES = {
    "T0": "Trusted Core",
    "T1": "Engine (Query/Storage/Index)",
    "T2": "Engine (Extended)",
    "T3": "Infrastructure & Governance",
    "T4": "Managed Extension Runtime",
    "T5": "Plugin Boundary (Least Trusted)",
}

TIER_COLORS = {
    "T0": "#ff6b6b",
    "T1": "#ffa94d",
    "T2": "#ffd43b",
    "T3": "#74c0fc",
    "T4": "#a9e34b",
    "T5": "#da77f2",
}

# Known consumer→provider dependency edges derived from module semantics.
# These are stable architectural relationships; individual header-level deps
# are not extracted here to keep the diagram readable.
KNOWN_DEPENDENCIES: list[tuple[str, str, str]] = [
    # (consumer, provider, label)
    ("aql", "core", "parses/plans"),
    ("query", "aql", "optimizes"),
    ("execution", "query", "executes"),
    ("execution", "storage", "reads/writes"),
    ("execution", "index", "uses"),
    ("index", "storage", "persists"),
    ("cache", "execution", "caches"),
    ("metadata", "storage", "schema store"),
    ("transaction", "core", "MVCC"),
    ("transaction", "storage", "data commit"),
    ("transaction", "replication", "distributes"),
    ("replication", "storage", "replica I/O"),
    ("sharding", "storage", "partitions"),
    ("sharding", "replication", "replicates shards"),
    ("api", "server", "routes"),
    ("api", "auth", "authenticates"),
    ("server", "execution", "dispatches"),
    ("auth", "security", "primitives"),
    ("governance", "auth", "policy check"),
    ("governance", "observability", "audit events"),
    ("rag", "retrieval", "retrieves"),
    ("rag", "llm", "generates"),
    ("rag", "storage", "knowledge store"),
    ("llm", "llama_cpp", "local inference"),
    ("llm_wiki", "llm", "wiki retrieval"),
    ("llm_wiki", "rag", "provenance"),
    ("retrieval", "index", "vector search"),
    ("ai", "llm", "orchestrates"),
    ("ai", "retrieval", "semantic search"),
    ("acceleration", "gpu", "GPU dispatch"),
    ("observability", "core", "metrics hooks"),
    ("network", "server", "transport"),
    ("distributed_knowledge", "graph", "knowledge graph"),
    ("distributed_knowledge", "replication", "distributed"),
    ("analytics", "storage", "reads data"),
    ("training", "llm", "fine-tunes"),
    ("training", "storage", "model storage"),
    ("content", "storage", "content store"),
    ("search", "index", "fulltext index"),
    ("search", "retrieval", "semantic search"),
    ("security", "core", "crypto primitives"),
    ("config", "core", "config bootstrap"),
    ("process", "core", "lifecycle"),
    ("utils", "base", "utilities"),
    ("themis", "core", "root aggregation"),
    ("themis", "plugins", "plugin loader"),
    ("plugins", "base", "plugin base"),
]


# ---------------------------------------------------------------------------
# Source file hashing
# ---------------------------------------------------------------------------

def _file_sha256(path: Path) -> str:
    h = hashlib.sha256()
    try:
        h.update(path.read_bytes())
    except OSError:
        return ""
    return h.hexdigest()[:16]


def _dir_hash(directory: Path) -> str:
    """Compute a combined hash over all files in a directory tree."""
    h = hashlib.sha256()
    for p in sorted(directory.rglob("*")):
        if p.is_file():
            try:
                h.update(p.read_bytes())
            except OSError:
                pass
    return h.hexdigest()[:16]


# ---------------------------------------------------------------------------
# Module extraction from ai_context/MODULES_AND_NAMESPACES.md
# ---------------------------------------------------------------------------

def _parse_modules_table(repo_root: Path) -> list[dict[str, Any]]:
    """Parse the canonical module table from MODULES_AND_NAMESPACES.md."""
    src = repo_root / "ai_context" / "MODULES_AND_NAMESPACES.md"
    if not src.exists():
        return []

    modules: list[dict[str, Any]] = []
    for line in src.read_text(encoding="utf-8").splitlines():
        # Table rows look like: | **base** | `themis::resource` | **T0** | ... |
        m = re.match(
            r"\|\s*\*\*([a-z_]+)\*\*\s*\|\s*`([^`]+)`\s*\|\s*\*\*([^*]+)\*\*\s*\|\s*([^|]*)\|",
            line,
        )
        if not m:
            continue
        name, namespace, tier, purpose = (g.strip() for g in m.groups())
        tier = tier.rstrip("–").strip()
        # Detect plugin indicator
        plugin_col = line.split("|")
        plugin_info = plugin_col[-2].strip() if len(plugin_col) >= 2 else ""
        has_public_plugin = "✅" in plugin_info
        has_private_plugin = "🔒" in plugin_info
        modules.append(
            {
                "name": name,
                "namespace": namespace,
                "tier": tier,
                "purpose": purpose,
                "src_path": f"src/{name}/",
                "has_public_plugin": has_public_plugin,
                "has_private_plugin": has_private_plugin,
            }
        )

    return sorted(modules, key=lambda m: (m["tier"], m["name"]))


# ---------------------------------------------------------------------------
# API contract extraction
# ---------------------------------------------------------------------------

def _parse_api_contracts(repo_root: Path) -> dict[str, list[dict[str, str]]]:
    """Parse API contract markdown files and return per-module contract entries."""
    contracts_dir = repo_root / "ai_context" / "api_contracts"
    result: dict[str, list[dict[str, str]]] = {}
    if not contracts_dir.exists():
        return result

    for md_file in sorted(contracts_dir.glob("*.md")):
        if md_file.name == "README.md":
            continue
        module_name = md_file.stem.replace("_api_contracts", "").replace(
            "_contracts", ""
        )
        entries: list[dict[str, str]] = []
        for line in md_file.read_text(encoding="utf-8").splitlines():
            # Table rows: | API | Namespace | Input | Output | Errors | ...
            parts = [p.strip() for p in line.split("|") if p.strip()]
            if len(parts) >= 4 and not parts[0].startswith("-") and parts[0] != "API":
                entries.append(
                    {
                        "api": parts[0],
                        "namespace": parts[1] if len(parts) > 1 else "",
                        "input_contract": parts[2] if len(parts) > 2 else "",
                        "output_contract": parts[3] if len(parts) > 3 else "",
                        "errors": parts[4] if len(parts) > 4 else "",
                    }
                )
        if entries:
            result[module_name] = entries

    return result


# ---------------------------------------------------------------------------
# LLM Wiki metadata extraction
# ---------------------------------------------------------------------------

def _parse_llm_wiki(repo_root: Path) -> dict[str, Any]:
    """Extract metadata from the developer LLM wiki artifacts."""
    wiki_dir = repo_root / "ai_context" / "developer_llm_wiki"
    result: dict[str, Any] = {
        "wiki_status": {},
        "governance_summary": "",
        "module_api_count": 0,
        "source_count": 0,
        "generated_at": "",
        "source_hash": "",
    }
    if not wiki_dir.exists():
        return result

    status_file = wiki_dir / "WIKI_STATUS.json"
    if status_file.exists():
        try:
            status = json.loads(status_file.read_text(encoding="utf-8"))
            result["wiki_status"] = status
            result["source_count"] = status.get("source_count", 0)
            result["generated_at"] = status.get("generated_at", "")
            result["source_hash"] = status.get("source_hash", "")
        except (json.JSONDecodeError, OSError):
            pass

    modules_file = wiki_dir / "MODULES_AND_APIS.md"
    if modules_file.exists():
        text = modules_file.read_text(encoding="utf-8")
        result["module_api_count"] = text.count("## include/") + text.count("## src/")

    gov_file = wiki_dir / "GOVERNANCE_AND_ROADMAP.md"
    if gov_file.exists():
        lines = gov_file.read_text(encoding="utf-8").splitlines()
        summary_lines = []
        for line in lines[:40]:
            if line.startswith("#") or line.strip():
                summary_lines.append(line)
            if len(summary_lines) >= 8:
                break
        result["governance_summary"] = "\n".join(summary_lines[:8])

    return result


# ---------------------------------------------------------------------------
# ai_working artifact inventory
# ---------------------------------------------------------------------------

def _parse_ai_working(repo_root: Path) -> list[dict[str, str]]:
    """List ai_working artifacts with type classification."""
    working_dir = repo_root / "ai_working"
    if not working_dir.exists():
        return []
    artifacts = []
    for p in sorted(working_dir.iterdir()):
        if not p.is_file():
            continue
        suffix = p.suffix.lower()
        artifact_type = "unknown"
        if suffix == ".json":
            artifact_type = "json"
        elif suffix in {".md", ".txt"}:
            name_upper = p.name.upper()
            if "WAVE" in name_upper or "BATCH" in name_upper:
                artifact_type = "wave_report"
            elif "CHECKLIST" in name_upper:
                artifact_type = "checklist"
            elif "SESSION" in name_upper or "SUMMARY" in name_upper:
                artifact_type = "session_summary"
            elif "PLAN" in name_upper or "DESIGN" in name_upper:
                artifact_type = "design"
            else:
                artifact_type = "report"
        artifacts.append(
            {
                "name": p.name,
                "type": artifact_type,
                "size_bytes": str(p.stat().st_size),
                "sha256_prefix": _file_sha256(p),
            }
        )
    return artifacts


# ---------------------------------------------------------------------------
# Documentation source inventory
# ---------------------------------------------------------------------------

def _parse_docs_sources(repo_root: Path) -> list[dict[str, str]]:
    """Inventory key documentation files from docs/ and repo root."""
    docs_entries: list[dict[str, str]] = []

    # Root governance docs
    root_docs = [
        "ARCHITECTURE.md",
        "ROADMAP.md",
        "FUTURE_ENHANCEMENTS.md",
        "BRANCHING_STRATEGY.md",
        "RELEASE_STRATEGY.md",
        "DOCUMENTATION_GOVERNANCE.md",
        "CHANGELOG.md",
        "CONTRIBUTING.md",
        "VERSIONING.md",
        "AUDIT.md",
    ]
    for name in root_docs:
        p = repo_root / name
        if p.exists():
            docs_entries.append(
                {
                    "path": name,
                    "category": "governance",
                    "sha256_prefix": _file_sha256(p),
                }
            )

    # docs/ directory — first two levels only
    docs_dir = repo_root / "docs"
    if docs_dir.exists():
        for p in sorted(docs_dir.rglob("*.md")):
            rel = p.relative_to(repo_root)
            depth = len(rel.parts)
            if depth <= 3:
                docs_entries.append(
                    {
                        "path": str(rel),
                        "category": "docs",
                        "sha256_prefix": _file_sha256(p),
                    }
                )

    return docs_entries


# ---------------------------------------------------------------------------
# Graph relationship builder
# ---------------------------------------------------------------------------

def _build_graph_relationships(
    modules: list[dict[str, Any]],
) -> list[dict[str, str]]:
    """Build consumer/provider graph relationships from KNOWN_DEPENDENCIES."""
    module_names = {m["name"] for m in modules}
    relationships = []
    for consumer, provider, label in KNOWN_DEPENDENCIES:
        if consumer in module_names and provider in module_names:
            relationships.append(
                {
                    "from": consumer,
                    "to": provider,
                    "label": label,
                    "type": "dependency",
                }
            )
    return relationships


# ---------------------------------------------------------------------------
# Source hash computation
# ---------------------------------------------------------------------------

def _compute_source_hashes(repo_root: Path) -> dict[str, str]:
    source_paths = [
        repo_root / "ai_context" / "MODULES_AND_NAMESPACES.md",
        repo_root / "ai_context" / "ARCHITECTURE_CLASSIFICATION.md",
        repo_root / "ARCHITECTURE.md",
        repo_root / "ROADMAP.md",
        repo_root / "FUTURE_ENHANCEMENTS.md",
    ]
    hashes: dict[str, str] = {}
    for p in source_paths:
        if p.exists():
            hashes[str(p.relative_to(repo_root))] = _file_sha256(p)

    # Directory hashes
    for d_name in ("ai_context/api_contracts", "ai_context/developer_llm_wiki", "ai_working", "docs"):
        d = repo_root / d_name
        if d.exists():
            hashes[d_name + "/"] = _dir_hash(d)

    return hashes


# ---------------------------------------------------------------------------
# JSON model assembly
# ---------------------------------------------------------------------------

def build_architecture_model(repo_root: Path) -> dict[str, Any]:
    """Assemble the full architecture model from all knowledge sources."""
    modules = _parse_modules_table(repo_root)
    api_contracts = _parse_api_contracts(repo_root)
    llm_wiki = _parse_llm_wiki(repo_root)
    ai_working = _parse_ai_working(repo_root)
    docs_sources = _parse_docs_sources(repo_root)
    relationships = _build_graph_relationships(modules)
    source_hashes = _compute_source_hashes(repo_root)

    # Per-module dependency maps (from relationships)
    consumers: dict[str, list[str]] = {}
    providers: dict[str, list[str]] = {}
    for rel in relationships:
        consumers.setdefault(rel["from"], []).append(rel["to"])
        providers.setdefault(rel["to"], []).append(rel["from"])

    # Enrich module list with dependency data and API contracts
    for mod in modules:
        name = mod["name"]
        mod["consumes"] = sorted(consumers.get(name, []))
        mod["consumed_by"] = sorted(providers.get(name, []))
        mod["api_contracts"] = api_contracts.get(name, [])

    # Tier summary
    tier_summary: dict[str, list[str]] = {}
    for mod in modules:
        tier = mod["tier"]
        tier_summary.setdefault(tier, []).append(mod["name"])

    model: dict[str, Any] = {
        "schema_version": "1.0.0",
        "generated_at": datetime.now(timezone.utc).isoformat(),
        "generator": "tools/architecture-generator/generate_architecture.py",
        "source_hashes": source_hashes,
        "modules": modules,
        "tier_summary": tier_summary,
        "relationships": relationships,
        "llm_wiki": {
            "source_count": llm_wiki["source_count"],
            "wiki_generated_at": llm_wiki["generated_at"],
            "wiki_source_hash": llm_wiki["source_hash"],
            "module_api_count": llm_wiki["module_api_count"],
            "governance_summary_preview": llm_wiki["governance_summary"],
        },
        "ai_working_artifacts": ai_working,
        "documentation_sources": docs_sources,
        "statistics": {
            "total_modules": len(modules),
            "total_relationships": len(relationships),
            "modules_with_public_plugin": sum(
                1 for m in modules if m["has_public_plugin"]
            ),
            "modules_with_private_plugin": sum(
                1 for m in modules if m["has_private_plugin"]
            ),
            "modules_with_api_contracts": len(api_contracts),
            "ai_working_artifact_count": len(ai_working),
            "documentation_source_count": len(docs_sources),
        },
    }
    return model


# ---------------------------------------------------------------------------
# Mermaid diagram renderer
# ---------------------------------------------------------------------------


# ---------------------------------------------------------------------------
# Module documentation URL resolver
# ---------------------------------------------------------------------------

# Base URL for GitHub wiki pages; override via MODULE_WIKI_BASE env var at
# generation time so the generator works with any fork or mirror.
_GITHUB_REPO = "makr-code/ThemisDB"
_GITHUB_BASE = f"https://github.com/{_GITHUB_REPO}"
_WIKI_BASE = f"{_GITHUB_BASE}/wiki"

# Modules that have a dedicated wiki or developer-docs page.
# Checked against the GitHub wiki; keys are module names as they appear in
# MODULES_AND_NAMESPACES.md.  Values are either a full URL or a wiki anchor
# path (relative to _WIKI_BASE).
_MODULE_WIKI_OVERRIDES: dict[str, str] = {
    "llm_wiki": f"{_WIKI_BASE}/LLM-Wiki",
    "rag": f"{_WIKI_BASE}/RAG-Module",
    "transaction": f"{_WIKI_BASE}/Transaction-Module",
    "sharding": f"{_WIKI_BASE}/Sharding-Module",
    "replication": f"{_WIKI_BASE}/Replication-Module",
    "graph": f"{_WIKI_BASE}/Graph-Module",
    "index": f"{_WIKI_BASE}/Index-Module",
    "storage": f"{_WIKI_BASE}/Storage-Module",
    "auth": f"{_WIKI_BASE}/Auth-Module",
    "security": f"{_WIKI_BASE}/Security-Module",
    "api": f"{_WIKI_BASE}/API-Module",
    "llm": f"{_WIKI_BASE}/LLM-Module",
    "acceleration": f"{_WIKI_BASE}/Acceleration-Module",
    "observability": f"{_WIKI_BASE}/Observability-Module",
}


def _module_doc_url(module_name: str) -> str:
    """Return the documentation URL for a module.

    Priority:
    1. Explicit wiki override for well-known modules
    2. GitHub blob link to src/<module>/ROADMAP.md on develop branch
    """
    if module_name in _MODULE_WIKI_OVERRIDES:
        return _MODULE_WIKI_OVERRIDES[module_name]
    return f"{_GITHUB_BASE}/blob/develop/src/{module_name}/ROADMAP.md"


# ---------------------------------------------------------------------------
# Mermaid renderer
# ---------------------------------------------------------------------------

# Mermaid keyword set — node IDs that match these must be prefixed to avoid
# parse errors.  The list covers flowchart/graph reserved words in Mermaid ≥ 10.
_MERMAID_RESERVED = frozenset(
    {
        "graph", "flowchart", "subgraph", "end", "direction",
        "click", "style", "classDef", "class", "linkStyle",
        "default", "start", "stop", "note", "loop", "alt",
        "else", "opt", "par", "and", "critical", "break",
        "rect", "title", "accTitle", "accDescr",
        # common English words that may also be module names
        "index", "cache", "process", "config", "search",
    }
)


def _mermaid_node_id(name: str) -> str:
    """Return a safe Mermaid node identifier for a module name.

    Always prefixes with 'mod_' to guarantee no collision with any current or
    future Mermaid reserved keyword (e.g. 'graph', 'index', 'config').
    """
    safe = name.replace("-", "_").replace(" ", "_")
    return f"mod_{safe}"


def render_mermaid(model: dict[str, Any]) -> str:
    """Render a Mermaid flowchart from the architecture model.

    Each node is rendered with a click directive linking to the module's
    documentation (src/<module>/ROADMAP.md on develop, or a wiki page for
    well-known modules).
    """
    lines: list[str] = [
        "flowchart TB",
        "    %% Vertical-first layout and GitHub-friendly styling",
    ]

    tier_summary: dict[str, list[str]] = model.get("tier_summary", {})
    relationships: list[dict[str, str]] = model.get("relationships", [])
    modules_by_name: dict[str, dict] = {
        m["name"]: m for m in model.get("modules", [])
    }

    all_rendered_nodes: list[tuple[str, str]] = []  # (nid, module_name)
    tier_node_ids: dict[str, list[str]] = {}
    private_plugin_nodes: list[str] = []
    public_plugin_nodes: list[str] = []

    # Subgraphs per tier (T0 first, ascending number = higher trust first)
    for tier in sorted(tier_summary.keys()):
        tier_label = TIER_NAMES.get(tier, tier)
        lines.append(f"    subgraph {tier}[\"{tier}: {tier_label}\"]")
        lines.append("        direction TB")
        for mod_name in sorted(tier_summary[tier]):
            nid = _mermaid_node_id(mod_name)
            mod = modules_by_name.get(mod_name, {})
            ns = mod.get("namespace", "")
            # Use only the module name in the visible label to keep it compact;
            # namespace is in the tooltip provided by the click directive.
            label = mod_name
            has_priv = mod.get("has_private_plugin", False)
            has_pub = mod.get("has_public_plugin", False)
            if has_priv:
                label += " 🔒"
                private_plugin_nodes.append(nid)
            elif has_pub:
                label += " ✅"
                public_plugin_nodes.append(nid)
            lines.append(f'        {nid}["{label}"]')
            all_rendered_nodes.append((nid, mod_name))
            tier_node_ids.setdefault(tier, []).append(nid)
        lines.append("    end")

    lines.extend(
        [
            "",
            "    classDef tierT0 fill:#EAF2FF,stroke:#1D4ED8,color:#0F172A,stroke-width:1.2px;",
            "    classDef tierT1 fill:#ECFDF3,stroke:#15803D,color:#0F172A,stroke-width:1.2px;",
            "    classDef tierT3 fill:#FFF7ED,stroke:#C2410C,color:#0F172A,stroke-width:1.2px;",
            "    classDef publicPlugin fill:#E0F2FE,stroke:#0369A1,color:#0F172A,stroke-dasharray: 3 2;",
            "    classDef privatePlugin fill:#FCE7F3,stroke:#9D174D,color:#0F172A,stroke-dasharray: 2 2;",
            "    linkStyle default stroke:#64748B,stroke-width:1.1px,opacity:0.85;",
        ]
    )
    for tier in sorted(tier_node_ids.keys()):
        class_name = f"tier{tier}"
        tier_nodes = ",".join(sorted(tier_node_ids[tier]))
        if tier_nodes:
            lines.append(f"    class {tier_nodes} {class_name};")
    if public_plugin_nodes:
        lines.append(f"    class {','.join(sorted(public_plugin_nodes))} publicPlugin;")
    if private_plugin_nodes:
        lines.append(f"    class {','.join(sorted(private_plugin_nodes))} privatePlugin;")

    lines.append("")

    # Edges
    rendered_edges: set[tuple[str, str]] = set()
    for rel in relationships:
        src = _mermaid_node_id(rel["from"])
        dst = _mermaid_node_id(rel["to"])
        key = (src, dst)
        if key in rendered_edges:
            continue
        rendered_edges.add(key)
        label = rel.get("label", "")
        if label:
            lines.append(f'    {src} -->|"{label}"| {dst}')
        else:
            lines.append(f"    {src} --> {dst}")

    lines.append("")

    # Click directives — link each node to its documentation page.
    # Format: click <nodeId> href "<url>" "<tooltip>" _blank
    for nid, mod_name in all_rendered_nodes:
        url = _module_doc_url(mod_name)
        mod = modules_by_name.get(mod_name, {})
        ns = mod.get("namespace", mod_name)
        tooltip = f"{ns} — module documentation"
        lines.append(f'    click {nid} href "{url}" "{tooltip}" _blank')

    return "\n".join(lines)


# ---------------------------------------------------------------------------
# Markdown renderer
# ---------------------------------------------------------------------------

def render_markdown(model: dict[str, Any]) -> str:
    stats = model.get("statistics", {})
    generated_at = model.get("generated_at", "")
    llm_wiki = model.get("llm_wiki", {})
    mermaid = render_mermaid(model)

    tier_table_rows = []
    tier_summary: dict[str, list[str]] = model.get("tier_summary", {})
    for tier in sorted(tier_summary.keys()):
        names = sorted(tier_summary[tier])
        label = TIER_NAMES.get(tier, tier)
        tier_table_rows.append(
            f"| {tier} | {label} | {len(names)} | {', '.join(names[:6])}"
            + (" …" if len(names) > 6 else "") + " |"
        )

    # Consumer/provider table (top 20 by out-degree)
    consumers_map: dict[str, list[str]] = {}
    for rel in model.get("relationships", []):
        consumers_map.setdefault(rel["from"], []).append(rel["to"])
    top_consumers = sorted(consumers_map.items(), key=lambda x: -len(x[1]))[:20]
    dep_rows = [
        f"| `{c}` | {len(ps)} | {', '.join(f'`{p}`' for p in sorted(ps))} |"
        for c, ps in top_consumers
    ]

    # Working artifact summary
    ai_working = model.get("ai_working_artifacts", [])
    wave_reports = [a["name"] for a in ai_working if a["type"] == "wave_report"]
    json_artifacts = [a["name"] for a in ai_working if a["type"] == "json"]

    docs_count = stats.get("documentation_source_count", 0)
    wiki_src_count = llm_wiki.get("source_count", 0)
    wiki_gen = llm_wiki.get("wiki_generated_at", "")
    wiki_api_count = llm_wiki.get("module_api_count", 0)

    md = f"""\
# ThemisDB Architecture

> **Auto-generated** — do not edit manually.
> Source: `tools/architecture-generator/generate_architecture.py`
> Generated: `{generated_at}`

## Statistics

| Metric | Value |
|--------|-------|
| Total modules | {stats.get('total_modules', 0)} |
| Total relationships | {stats.get('total_relationships', 0)} |
| Modules with public plugin | {stats.get('modules_with_public_plugin', 0)} |
| Modules with private plugin | {stats.get('modules_with_private_plugin', 0)} |
| Modules with API contracts | {stats.get('modules_with_api_contracts', 0)} |
| Documentation sources scanned | {docs_count} |
| LLM Wiki source files | {wiki_src_count} |
| LLM Wiki module/API entries | {wiki_api_count} |
| LLM Wiki generated | `{wiki_gen}` |
| ai_working artifacts | {stats.get('ai_working_artifact_count', 0)} |

## Module Architecture Diagram

```mermaid
{mermaid}
```

## Tier Classification

| Tier | Description | Count | Modules (sample) |
|------|-------------|-------|-------------------|
{chr(10).join(tier_table_rows)}

## Consumer / Provider Dependencies

Top modules by outgoing dependency count:

| Module | Provides to (count) | Consumes from |
|--------|---------------------|---------------|
{chr(10).join(dep_rows)}

## Knowledge Sources

### Developer LLM Wiki
- Generated at: `{wiki_gen}`
- Source hash: `{llm_wiki.get('wiki_source_hash', 'n/a')}`
- Source file count: {wiki_src_count}
- Module/API entries indexed: {wiki_api_count}

### ai_working Artifacts
Wave/batch reports: {len(wave_reports)}
JSON analysis artifacts: {len(json_artifacts)}

### Documentation Sources
{docs_count} documentation files scanned from `docs/` and repository root governance files.

## Source Hashes

| Source | SHA-256 prefix |
|--------|----------------|
{chr(10).join(f'| `{k}` | `{v}` |' for k, v in sorted(model.get('source_hashes', {}).items()))}
"""
    return md


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------

def main() -> int:
    parser = argparse.ArgumentParser(
        description="Generate ThemisDB architecture JSON and Markdown"
    )
    parser.add_argument(
        "--repo-root",
        default=None,
        help="Path to repository root (default: inferred from script location)",
    )
    parser.add_argument(
        "--json-out",
        default=None,
        help="Output JSON path (default: <repo-root>/architecture.json)",
    )
    parser.add_argument(
        "--md-out",
        default=None,
        help="Output Markdown path (default: <repo-root>/architecture.md)",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Print outputs to stdout without writing files",
    )
    args = parser.parse_args()

    # Determine repo root
    if args.repo_root:
        repo_root = Path(args.repo_root).resolve()
    else:
        # This script lives at tools/architecture-generator/generate_architecture.py
        repo_root = Path(__file__).resolve().parent.parent.parent

    if not repo_root.is_dir():
        print(f"ERROR: repo root not found: {repo_root}", file=sys.stderr)
        return 1

    json_out = Path(args.json_out) if args.json_out else repo_root / "architecture.json"
    md_out = Path(args.md_out) if args.md_out else repo_root / "architecture.md"

    print(f"[architecture-generator] repo root: {repo_root}", file=sys.stderr)
    print("[architecture-generator] scanning knowledge sources …", file=sys.stderr)

    model = build_architecture_model(repo_root)

    json_text = json.dumps(model, indent=2, sort_keys=True, ensure_ascii=False)
    md_text = render_markdown(model)

    if args.dry_run:
        print("=== architecture.json ===")
        print(json_text[:2000], "…" if len(json_text) > 2000 else "")
        print()
        print("=== architecture.md ===")
        print(md_text[:2000], "…" if len(md_text) > 2000 else "")
        return 0

    json_out.write_text(json_text, encoding="utf-8")
    print(f"[architecture-generator] wrote {json_out}", file=sys.stderr)

    md_out.write_text(md_text, encoding="utf-8")
    print(f"[architecture-generator] wrote {md_out}", file=sys.stderr)

    stats = model.get("statistics", {})
    print(
        f"[architecture-generator] done — {stats.get('total_modules', 0)} modules, "
        f"{stats.get('total_relationships', 0)} relationships",
        file=sys.stderr,
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
