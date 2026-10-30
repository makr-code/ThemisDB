---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-10-30"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# LLM Wiki Data Model

## Purpose and source of truth

This document describes the persisted wiki entity contract and the workspace
state shape for developers evolving the model. The JSON Schema at
[`schema/llm_wiki_entity.schema.json`](schema/llm_wiki_entity.schema.json) is
the normative entity contract. The separate filesystem workspace state is
represented by `WorkspaceState` in
[`../../include/llm_wiki/workspace_state_manager.h`](../../include/llm_wiki/workspace_state_manager.h).

Do not treat roadmap concepts such as a knowledge-sphere registry or automated
schema migration as implemented model features unless corresponding code and
tests exist.

## Entity model

The entity schema supports five `entity_type` values: `page`, `chunk`, `claim`,
`edge`, and `evidence_package`. Each entity requires:

- `schema_version`, represented as a dotted numeric version.
- `entity_type`.
- `provenance`, including origin type, non-empty source reference and revision
  ID, non-negative transform-chain length, and capture timestamp.
- `confidence`, a number from 0 through 1.
- `created_at` and `updated_at` date-time strings.

`content` carries type-specific payload. Optional `extensions` are namespaced
and restricted by the schema's pattern rule; `governance` records an
`allow`, `deny`, or `defer` decision plus a non-empty reason code. Governance
is required when `entity_type` is `evidence_package`. Unknown top-level fields
are rejected by the schema.

Provenance is not just descriptive metadata: source references, revision
identity, transform depth, and policy snapshot information allow downstream
review to establish where generated or ingested material came from. A schema
validity result alone is not proof of source trust or answer correctness.

## Workspace state model

The filesystem state manager serializes a separate `WorkspaceState` object
with version, creation/update timestamps, workspace root, page-link mapping,
and task mapping. The manager stores it at `wiki/state.json` and appends JSON
records to `wiki/state.log`. This state structure is not interchangeable with
the entity schema above: it does not contain the entity-level provenance
fields.

`WorkspaceStateManager` methods are explicitly documented as not thread-safe;
callers must serialize access. Do not rely on the current checksum label as a
cryptographic integrity control: `workspace_state_manager.cpp` currently
computes a 64-bit FNV-1a value with a `sha256:` prefix. This is a source-level
implementation limitation, not SHA-256.

## Invariants and evolution

1. Preserve the required entity fields and the confidence range.
2. Keep extensions namespaced; do not redefine core field meanings inside an
   extension.
3. Make version changes explicit. The schema currently validates field shape,
   but the module does not provide a general migration runner.
4. Keep workspace state and entity-schema changes distinct and document any
   conversion between them.
5. Add validation tests for malformed provenance, invalid confidence,
   unsupported entity types, and unknown top-level fields when changing the
   entity schema.

## Validation and trade-offs

The JSON Schema is the machine-readable contract for entity shape. The YAML
process policy has its own schema and runtime semantic checks; see
[`process/llm_wiki_process_policy.yaml`](process/llm_wiki_process_policy.yaml)
and [`schema/llm_wiki_process_policy.schema.json`](schema/llm_wiki_process_policy.schema.json).
Schema validation catches structural violations, while runtime validation
must enforce cross-field and policy invariants. Neither schema currently
establishes cryptographic provenance, access-control identity, or a full
reader/writer compatibility matrix.

## Related documents

- [`API.md`](API.md) — public plugin-level request and response types
- [`IMPLEMENTATION.md`](IMPLEMENTATION.md) — persistence implementation details
- [`TRUST.md`](TRUST.md) — trust boundaries and residual risks

> Exact model backend not verifiable from final document text alone.
