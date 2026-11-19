---
Author: "ThemisDB Team"
Created: "2026-11-17"
Last Updated: "2026-11-19"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "unknown"
---

Exact model backend not verifiable from final document text alone.

# Optional Agent Skills

This directory contains opt-in guidance packages. They are inert instructions;
any corresponding native database tool must be registered and authorized
separately by the host:

| Skill | Purpose |
|---|---|
| `aql-mutation-draft` | Draft narrowly scoped AQL mutation candidates without executing them. |
| `aql-plan-analysis` | Interpret AQL plans without confusing estimates with measurements. |
| `aql-query-review` | Review AQL scope, syntax evidence, and operational risk. |
| `aql-read-query` | Draft bounded read-only AQL from a verified schema. |
| `asset-impact-analysis` | Combine a bounded graph relation with time-series window comparisons. |
| `evidence-answer` | Answer using authorized evidence and attributable sources. |
| `file-edit` | Make minimal, authorized file edits and report validation. |
| `file-import-planning` | Plan flat-file mappings, dry runs, and reconciliation without importing data. |
| `file-inspection` | Inspect authorized files without executing their contents. |
| `graph-exploration` | Plan bounded, read-only graph exploration. |
| `hybrid-search-diagnostics` | Diagnose candidate retrieval, fusion, and reranking failures. |
| `log-analysis` | Analyze authorized logs while protecting sensitive data. |
| `process-bpmn-mining` | Relate process-model YAML, BPMN graphs, mining evidence, and optional German Verwaltungsakt review profiles in `references/verwaltungsakt-pruefung.yaml`. |
| `rag-evaluation` | Assess retrieval and answer quality against labeled evidence. |
| `rag-retrieval` | Plan evidence-grounded retrieval through authorized RAG tools. |
| `similar-case-research` | Compare cases using authorized document and vector or hybrid retrieval with evidence and limitations. |
| `themisdb-yaml-config` | Draft and review ThemisDB YAML against verified schemas and samples. |
| `timeseries-window-analysis` | Compare native time series windows with an optional radius filter. |
| `trace-correlation` | Reconstruct request paths from authorized spans and logs. |

The mutation skill only drafts a candidate; it never executes a query or
grants write permission. Skills do not automatically invoke other skills.
Some packages name optional reusable guidance dependencies in their
instructions. These names are advisory text, not machine-validated metadata:
the host must explicitly load any selected package and handle missing or
failed loads. Carry authorization, budgets, provenance and failure status
between steps; loading guidance neither runs a step nor grants a tool.
The existing CMake runtime install and ZIP staging include the `config/`
directory, so these packages ship with that runtime payload. Their presence
does not make them discoverable, enabled, routed or
exposed to MCP automatically. To use them, a host application must:

1. Point `SkillRegistryOptions::roots` at the deployed `agent_skills`
   directory beneath the runtime `config/` root, or at another trusted root.
   For custom deployments that omit the runtime config payload, copy the
   packages to the chosen trusted root.
   The current Linux CMake install path is
   `/usr/etc/themisdb/agent_skills` (with a compatibility copy at
   `/usr/bin/config/agent_skills`); Windows installs and ZIP packages use
   `config/agent_skills` beneath their install/package root.
2. Construct a `SkillRegistry` and call `refresh()`, handling any error.
3. Use `list()` and `load(name)`, or explicitly call `registerSkillTools()` on
   its `ToolRegistry` and permit the required skill tools in the active
   `ModeSpec`. `read_skill_resource` is optional and separately allowlisted.
4. Provide any separate retrieval, query, graph, or time-series tools only when
   configured and authorized for the caller. In particular,
   `asset-impact-analysis` requires a separately registered
   `analyze_asset_impact` tool with host-bound graph ID, edge type, metric, and
   principal-scoped data sources. A skill does not register or grant tools.

The registry currently checks paths before opening files, but these operations
are not atomic against concurrent filesystem replacement. Protect configured
roots, package files and their parent directories from modification by
untrusted users for the lifetime of the registry. Symlink checks alone are not
a security boundary against a concurrent writer.

Skill content is untrusted prompt input; it cannot grant permissions or execute
scripts. In particular, an allowlisted query tool is not by itself a read-only
authorization boundary. Existing execution, tenant and policy controls must be
enforced separately by the host. See
[`src/llm/ARCHITECTURE.md`](../../src/llm/ARCHITECTURE.md) and
[`src/llm/FUTURE_ENHANCEMENTS.md`](../../src/llm/FUTURE_ENHANCEMENTS.md).
