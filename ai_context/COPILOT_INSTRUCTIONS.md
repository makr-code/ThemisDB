# Copilot Instructions (Repository-wide)

Datum: 2026-08-03
Status: Active
Bezug: Repository-weite AI-/Copilot-Arbeitsregeln fuer Dokumentation, Governance und Release-Sync
Primary (Quelle der Wahrheit): DOCUMENTATION_GOVERNANCE.md, .github/copilot-instructions.md, BRANCHING_STRATEGY.md, RELEASE_STRATEGY.md

## AI Context Knowledge Base (NEW in 2026-08-03)

Before starting any implementation task, consult these curated references:

- **[MODULES_AND_NAMESPACES.md](./MODULES_AND_NAMESPACES.md)** — 62 modules × namespace mapping, SOC boundaries, consumer relationships
- **[AI_CONTEXT_NAMESPACE_INDEX.md](./AI_CONTEXT_NAMESPACE_INDEX.md)** — Alphabetical namespace directory; public API headers per namespace
- **[MEMORY_MANAGEMENT_POLICY.md](./memory_management_policy.md)** — 5 core RAII rules + ownership model (compressed, production-ready)
- **[OOP_AND_SOC_PRINCIPLES.md](./OOP_AND_SOC_PRINCIPLES.md)** — Interfaces, templates/concepts, adapters, const-correctness, module boundaries
- **[FUNCTION_CLASSIFICATION.md](./FUNCTION_CLASSIFICATION.md)** — API levels, task types, performance criticality (P0/P1/P2), error patterns
- **[API_CONTRACT_TEMPLATES.md](./API_CONTRACT_TEMPLATES.md)** — Template + filling rules for machine-readable API contracts

### Prioritized API Contracts (Master Reference)

- **[api_contracts/api.md](./api_contracts/api.md)** — HTTP/gRPC/GraphQL transport, routing, error taxonomy; 6 release gates (GATE-API-01..06)
- **[api_contracts/llm.md](./api_contracts/llm.md)** — LLM inference, embeddings, model switching; GATE-LLM-01..04
- **[api_contracts/index.md](./api_contracts/index.md)** — Index ops (HNSW, B-tree, R-tree); GATE-INDEX-01..04
- **[api_contracts/storage.md](./api_contracts/storage.md)** — RocksDB K-V, snapshots, transactions; GATE-STOR-01..04
- **[api_contracts/transaction.md](./api_contracts/transaction.md)** — 2PC/SAGA/isolation; GATE-TXN-01..05
- **[api_contracts/auth.md](./api_contracts/auth.md)** — Authentication, RBAC authorization, principal contract v1.x; GATE-AUTH-01..04

**Usage:** Link to contract row when reviewing/implementing public APIs in these modules.

## Documentation tasks (MUST)
If the task involves documentation (any markdown docs change; `docs/**` keeps the stricter format rules below):

1) Use the standard:
   - Spec: `docs/_standards/doc_header.schema.yml`
   - Template: `docs/_standards/DOC_TEMPLATE.md`

2) Every doc under `docs/**` MUST start with a clickable breadcrumb link chain:
   - Format requirement: one line with Markdown links separated by the literal ` > ` token (minimum 3 levels: `docs > <lang> > <doc_kind>`; add domain/module levels when available).

3) Every doc header MUST include:
   - **Datum** (YYYY-MM-DD)
   - **Status**
   - **Primary (Quelle der Wahrheit)** (links to `src/**`, `include/**`, `examples/**`)
   - **Bezug / Reference** (issue/PR/module context)

4) Prefer linking to Primary docs instead of duplicating canonical information.

5) Apply repository naming reality rules:
   - Use the dominant existing style in the target scope/directory.
   - ThemisDB default for module-adjacent docs is UPPER_SNAKE naming unless local canon differs.
   - Do not create semantic filename duplicates in one scope (e.g. `ARCHITECTURE.md` and `architecture.md`).

6) Keep task instructions compact/non-redundant for token efficiency without removing normative constraints.

## Documentation governance sync (MUST)

Treat these files as one aligned documentation rule set:

- `DOCUMENTATION_GOVERNANCE.md`
- `.github/copilot-instructions.md`
- `ai_context/COPILOT_INSTRUCTIONS.md`
- `.github/ISSUE_TEMPLATE/docs_audit.md`

Sync rules:

1) Source precedence and SOT domain mapping come from `DOCUMENTATION_GOVERNANCE.md`.
2) Conformance checks per docs change are mandatory: naming, structure, duktus, SOT consistency.
3) If one file in this set changes policy semantics, update the other files in the same change.

## Root governance and release/versioning alignment (MUST)

For root governance or release/versioning updates, treat the following files as one aligned set:

- `.github/copilot-instructions.md` (AI-/Agent-Prozessregeln)
- `ai_context/COPILOT_INSTRUCTIONS.md` (AI-/Agent-Prozessregeln, mirror)
- `DOCUMENTATION_GOVERNANCE.md` (Doku-SOT, Konventionen, Checks)
- `BRANCHING_STRATEGY.md` (canonical branch/edition/merge model)
- `VERSIONING.md` (SemVer + release type model)
- `RELEASE_STRATEGY.md` (branch/tag/milestone flow)
- `CHANGELOG.md` (released + unreleased traceability)
- `ROADMAP.md` (feature/milestone source of truth)
- `FUTURE_ENHANCEMENTS.md` (open enhancement backlog)

Alignment rules:

1) Keep release type mapping consistent across `VERSIONING.md`, `RELEASE_STRATEGY.md`, and `CHANGELOG.md`.
2) Keep branch, edition, and release-lane naming consistent across `BRANCHING_STRATEGY.md`, `RELEASE_STRATEGY.md`, `.github/copilot-instructions.md`, and this file.
3) Keep terminology consistent: shipped scope = `ROADMAP.md`; open backlog = `FUTURE_ENHANCEMENTS.md`.
4) Use canonical branch names for all new governance changes:
   - `develop`
   - `minimal`
   - `community`
   - `enterprise`
   - `hyperscaler`
   - `military`
5) Treat `main` and `millitary` as legacy-only names for migration/audit context; do not use them as canonical targets in new AI-authored governance, workflow, or process updates.
6) For governance updates, include review/audit references from:
   - `docs/DOCUMENTATION_REVIEW_GUIDELINES.md`
   - `docs/SYSTEMATISCHER_REVIEWPLAN.md`
   - `docs/PR_DOCUMENTATION_CHECKLIST.md`
   - `docs/de/development/SOURCE_CODE_AUDIT.md`
   - `docs/audit-framework/AUDIT_RUNBOOK.md`
7) For beta-to-GA or release-hardening updates, keep the gate model aligned across `ROADMAP.md`, `FUTURE_ENHANCEMENTS.md`, `RELEASE_STRATEGY.md`, `VERSIONING.md`, and `CHANGELOG.md`:
   - Wave 7 baseline PASS
   - `release_critical` on `develop`
   - top-risk module sign-off (`server`, `llm`, `sharding`)
   - resilience/security/operations evidence (Wave 5/6 retention, Wave 8 or equivalent, chaos, sanitizer/recovery, penetration test, SLA, runbooks)
8) Private plugin changes in the public superproject are limited to SDK, governance, packaging, CI, and optional-submodule wiring under `plugins/private/*`; private implementation code belongs only in the dedicated plugin-named submodules.
9) When naming private plugin repositories or submodule paths, prefer names that mirror the current plugin names so the repository-to-plugin mapping stays obvious.
10) Community and Minimal lanes must remain private-free: no private credentials, no mandatory private checkout, and no private artefacts or confidential path disclosures in public docs.
