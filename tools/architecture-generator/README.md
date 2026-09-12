# Architecture Generator

Generates and validates the ThemisDB architecture model from repository knowledge sources.

## Files

| File | Purpose |
|------|---------|
| `generate_architecture.py` | Scans knowledge sources and writes `architecture.json` + `architecture.md` |
| `validate_architecture.py` | Validates the generated files for schema and Mermaid sanity |

## Usage

```bash
# Generate from repository root
python tools/architecture-generator/generate_architecture.py

# Generate with explicit paths
python tools/architecture-generator/generate_architecture.py \
  --repo-root /path/to/ThemisDB \
  --json-out architecture.json \
  --md-out architecture.md

# Dry run (print to stdout, no file writes)
python tools/architecture-generator/generate_architecture.py --dry-run

# Validate generated outputs
python tools/architecture-generator/validate_architecture.py
```

## Knowledge Sources Scanned

| Source | Content |
|--------|---------|
| `ai_context/MODULES_AND_NAMESPACES.md` | Canonical module list, namespaces, tier classification |
| `ai_context/ARCHITECTURE_CLASSIFICATION.md` | T0–T5 tier model |
| `ai_context/api_contracts/*.md` | Per-module API contracts |
| `ai_context/developer_llm_wiki/` | LLM wiki artifacts (MODULES_AND_APIS, GOVERNANCE, WIKI_STATUS) |
| `ai_working/` | Working artifacts: wave/batch reports, JSON analysis, checklists |
| `ARCHITECTURE.md` | Root architecture document |
| `ROADMAP.md` / `FUTURE_ENHANCEMENTS.md` | Roadmap and feature planning |
| `docs/` | Governance and documentation sources (depth ≤ 3) |

## Output Files

| File | Location | Format |
|------|----------|--------|
| `architecture.json` | Repository root | Machine-readable JSON, sorted keys |
| `architecture.md` | Repository root | Markdown with Mermaid diagram |

## JSON Schema

```json
{
  "schema_version": "1.0.0",
  "generated_at": "<ISO 8601 timestamp>",
  "generator": "tools/architecture-generator/generate_architecture.py",
  "source_hashes": { "<path>": "<sha256-prefix>" },
  "modules": [
    {
      "name": "<module>",
      "namespace": "themis::<module>",
      "tier": "T0|T1|T2|T3|T4|T5",
      "purpose": "<description>",
      "src_path": "src/<module>/",
      "has_public_plugin": false,
      "has_private_plugin": false,
      "consumes": ["<module>"],
      "consumed_by": ["<module>"],
      "api_contracts": [...]
    }
  ],
  "tier_summary": { "T0": ["base", "core", ...] },
  "relationships": [
    { "from": "<consumer>", "to": "<provider>", "label": "<label>", "type": "dependency" }
  ],
  "llm_wiki": { ... },
  "ai_working_artifacts": [ ... ],
  "documentation_sources": [ ... ],
  "statistics": { ... }
}
```

## CI Workflow

The generator is run automatically by `.github/workflows/maintenance-architecture-ci.yml`:

- **Schedule**: weekly (Monday 02:00 UTC)
- **Trigger**: `workflow_dispatch` for manual runs
- **Path filter**: runs when `ai_context/**`, `ai_working/**`, `docs/**`, or root governance docs change (on push to `develop`)
- On change: commits updated files and opens/updates a PR

## Governance

- Generated files (`architecture.json`, `architecture.md`) must not be edited manually.
- Re-run the generator after changes to any source listed in **Knowledge Sources Scanned**.
- The CI workflow will surface diffs via PR for review before merge.
- All dependency edges in `KNOWN_DEPENDENCIES` inside `generate_architecture.py` must reflect architectural reality; update them when the module structure changes.
