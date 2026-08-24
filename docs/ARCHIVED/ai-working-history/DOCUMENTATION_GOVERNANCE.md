# Documentation Governance And Update Strategy

**Status:** Active  
**Effective Date:** 2026-06-25  
**Scope:** Root, module, and generated docs in this repository

---

## 1. Goal

This document defines a deterministic documentation pipeline with clear source precedence, update intervals, and issue orchestration.

Design principle:

**Docs flow from code-adjacent module truth to generated public documentation.**

---

## 2. Canonical 4-Level Documentation Strategy

## Level 1: Primary Developer Documentation (Per Module)

Primary module documentation is maintained with strong source proximity and separation of concerns.

Paths:
- `src/<module>/`
- `include/<module>/`
- `tests/<module>/`
- `benchmarks/<module>/`
- `ai_working/` (working evidence, curation notes, draft consolidation)

Structure:
- Multiple markdown files per module are allowed and encouraged when separation of concerns is clearer than one large file.
- Typical split: architecture, roadmap, security, audit, testing, operational notes.

Update model:
- Updated from source code changes, architecture decisions, draft work, and curated `ai_working` evidence.
- AI-assisted updates are allowed, but final curation remains maintainers' responsibility.

`ai_working` guardrails:
- `ai_working` may contain both curated evidence and drafts.
- Any `ai_working` file marked as historical/snapshot/draft is non-normative and must link to its canonical upstream source.
- No release or security claim is valid only because it exists in `ai_working`.

## Level 2: Secondary Developer Summaries (Base Aggregates)

Secondary developer docs summarize module-level Level-1 content.

Paths and scope:
- Base-level summaries under repository roots and aggregate module overviews.
- Typical files: top-level module summaries in `src/` and `include/`, cross-module technical summaries.

Rule:
- Level 2 may only summarize Level 1 and may not introduce contradictory status semantics.

## Level 3: Root Governance And Product Docs (Generated/Curated Aggregates)

Root docs are generated or updated from Level 1 and Level 2.

Primary targets:
- `CHANGELOG.md`
- `README.md`
- `CTEST.md`
- benchmark root docs (including benchmark status and execution docs)
- `FUTURE_ENHANCEMENTS.md`
- `SECURITY.md`
- related root governance docs when impacted

Rule:
- Level 3 is downstream of Level 1 and Level 2. No root status claim without upstream evidence.

Scope note:
- This downstream rule applies to implementation/status summaries.
- Governance policy ownership remains with the canonical governance files themselves (for example branch/release/version policy documents).

## Level 4: Public docs/ Output

`docs/` is generated/updated from Doxygen output plus Level 1 to Level 3 inputs.

Inputs:
- Doxygen output (API-level truth)
- Module docs (Level 1)
- Secondary summaries (Level 2)
- Root aggregates (Level 3)

Rule:
- `docs/` is publication-oriented output and must not become an untracked parallel source of truth.

---

## 2.0a Compendium Sync Process

The `docs/compendium/` directory hosts the ThemisDB Kompendium — a comprehensive German-language handbook (43+ chapters, 7 appendices) that aggregates Level 1–3 content into a structured publication.

### Position in the 4-Level Model

- **Level:** 3/4 aggregate (compendium chapters are downstream of Level 1 module docs and Level 2 summaries)
- **SOT domain:** architecture-governance (structure/pipeline), module-behavior (content per chapter)
- **Primary input:** `docs/de/<domain>/` directories (canonical docs/de sources)
- **Secondary inputs:** `src/<module>/README.md`, `include/<module>/README.md` (Level 1)

### Sync Authority Rules

1. **`docs/de/` beats compendium chapters.** If a compendium chapter contradicts a `docs/de/` source file, the `docs/de/` file is authoritative.
2. **Module source docs beat `docs/de/` for implementation status.** `src/<module>/`, `include/<module>/` are Level 1 truth for code-behavior claims.
3. **No invented facts.** Compendium content must always cite an existing `docs/de/` file or Level 1 source as canonical reference.
4. **Bidirectional cross-references are mandatory** for Phase-3-Sync enrichment sections — each new section must link to its `docs/de/` source.

### Chapter→Source Mapping

See `docs/compendium/PHASE3_MAPPING_TABLE.md` for the full chapter → docs/de source mapping.

High-priority chapters (Q3 2026 enriched):

| Chapter | docs/de Sources |
|---------|----------------|
| Kapitel 17 — LLM Integration | `docs/de/llm/`, `docs/de/lora/`, `docs/de/rag/` |
| Kapitel 29 — Analytics & Process Mining | `docs/de/analytics/`, `docs/de/process/` |
| Kapitel 31 — API Protocols | `docs/de/apis/`, `docs/de/rpc_grpc/` |
| Kapitel 40 — Data Governance & Compliance | `docs/de/compliance/`, `docs/de/governance/` |

### Update Cadence

- **Content enrichment (Phase 3 chapters):** `DOC-WEEKLY-YYYY-WW` milestone per batch
- **Full compendium rebuild:** `DOC-MONTHLY-YYYY-MM` or `DOC-RELEASE-vX.Y.Z`
- **VERSION bump (x.y.z-dev → x.y.z):** only after human-maintainer review and build validation

### Guardrails

- Never use `docs/compendium/` as the sole source for release or security claims.
- `docs/compendium/output/` (generated PDFs/HTML) is not a source of truth.
- Historical snapshots in `docs/compendium/` phase reports are non-normative.
- `ai_working/` snapshots referenced in compendium content must link to their canonical upstream source.

---



To avoid ambiguity, each documentation issue must be mapped to one source-of-truth domain.

| SOT Domain | Canonical Primary Sources | Typical Downstream Docs |
|---|---|---|
| Module behavior and implementation status | `src/<module>/`, `include/<module>/`, `tests/<module>/`, `benchmarks/<module>/` | `README.md`, `ROADMAP.md`, `docs/` |
| API contracts | `openapi/`, `proto/`, public headers in `include/` | `README.md`, SDK docs, `docs/` |
| Build and test truth | `CMakeLists.txt`, `cmake/`, `CMakePresets.json`, `CTEST.md`, test registration in `tests/` | `README.md`, runbooks, `docs/` |
| Release and versioning policy | `RELEASE_STRATEGY.md`, `VERSIONING.md`, `VERSION`, `RELEASE_TYPE`, `CHANGELOG.md` | release notes, `README.md`, `docs/` |
| Security posture and process | `SECURITY.md`, security-relevant source/tests, audit evidence | `README.md`, `docs/`, advisories |
| Architecture and governance rules | `ARCHITECTURE.md`, `BRANCHING_STRATEGY.md`, `GOVERNANCE.md` | `README.md`, `docs/` |
| Public/private plugin governance | `include/plugins/`, `src/plugins/`, `plugins/`, `RELEASE_STRATEGY.md`, `VERSIONING.md`, `.gitmodules` | `ROADMAP.md`, `FUTURE_ENHANCEMENTS.md`, workflow/runbook docs |

Domain rule:
- If two documents disagree, the canonical source of the mapped SOT domain wins.
- Missing canonical source mapping is a blocking issue.

---

## 2.2 Repository Documentation Conventions (Mandatory)

All docs changes must follow repository conventions. Violations are review blockers.

### Naming

- Keep established root style for canonical files (for example `README.md`, `ROADMAP.md`, `SECURITY.md`, `CHANGELOG.md`).
- Module-local docs must follow the dominant existing style of their scope/directory, not an abstract global style.
- ThemisDB default (reality-based): module-adjacent docs are predominantly UPPER_SNAKE names (for example `README.md`, `ROADMAP.md`, `ARCHITECTURE.md`, `FUTURE_ENHANCEMENTS.md`) and should stay that way unless a directory already uses a different local canon.
- No semantic duplicates in one scope (for example both `ARCHITECTURE.md` and `architecture.md`).

### Structure

- Start maintained markdown files with matching title.
- Include scope and provenance/source references for status claims.
- Keep stable order where feasible: purpose/scope -> current state -> decisions/actions -> validation/evidence -> follow-ups.
- Use repository-consistent table/checklist labels and status semantics.

### Duktus

- Technical, precise, neutral; no marketing/vague claims.
- Status claims must be evidence-backed and traceable to canonical sources.
- Prefer short declarative sentences and consistent terminology.
- Reuse repository lifecycle vocabulary; avoid contradictory synonyms.
- Public documentation must not disclose private repository URLs, private source layouts, or confidential implementation details; describe private plugin boundaries only at the governance/interface level.
- When private plugin repos or submodule paths are named in governance docs, prefer names that mirror the current plugin names to keep repository-to-plugin mapping obvious.

### Mandatory checks per change

- naming check
- structure check
- tone/duktus check
- SOT consistency check

---

## 3. Source Precedence And Conflict Resolution

When statements conflict:

1. Level 1 wins over all other levels.
2. Level 2 wins over Level 3 and Level 4.
3. Level 3 wins over Level 4.
4. If Level-1 evidence is missing, create a blocking docs issue before making status claims.

Domain override:
- For branch/release/version/security governance topics, the canonical governance file for that domain is authoritative even if a downstream level says otherwise.

---

## 4. Update Intervals And SLAs

| Level | Interval | Trigger | SLA |
|---|---|---|---|
| Level 1 | Event-driven | code/API/test/benchmark/module decision changes | same PR or within 24h |
| Level 2 | Weekly | drift between module docs and base summaries | within 7 days |
| Level 3 | Weekly + release-driven | release scope, security posture, test/benchmark baseline updates | within 7 days or release PR |
| Level 4 | Release-driven + monthly | doxygen refresh, publication sync, docs drift | release PR or within current month |

Immediate update triggers:
- Module status changes
- Security posture changes
- Test baseline changes affecting `CTEST.md`
- Benchmark baseline changes affecting benchmark docs
- Release/version/release-type changes
- Release-readiness gate changes (for example Wave 7 / Wave 8 status, `release_critical` policy, security exit criteria, or SLA sign-off rules)

---

## 5. GitHub Issue And Milestone Orchestration

All documentation synchronization is orchestrated via GitHub issues and milestones.

## Milestone scheme

- `DOC-WEEKLY-YYYY-WW` (Level 2 and Level 3 sync)
- `DOC-MONTHLY-YYYY-MM` (Level 4 publication sync and Level 3 cleanup)
- `DOC-RELEASE-vX.Y.Z` (release-bound Level 1 to Level 4 final alignment)

## Required labels

- `type:documentation`
- `docs:level1` or `docs:level2` or `docs:level3` or `docs:level4`
- `status:open`
- `priority:*`

## Required issue metadata

- Documentation level
- SOT domain
- Canonical upstream source references
- Drift reason
- Acceptance criteria
- Target milestone
- Release-gate evidence references when the SOT domain is `build-test` or `release-versioning`

Use issue template:
- `.github/ISSUE_TEMPLATE/docs_audit.md`

---

## 6. Execution Workflow

1. Create docs issue from the docs audit template.
2. Classify issue to Level 1 to Level 4.
3. Assign milestone and labels.
4. Map the issue to one SOT domain and confirm canonical source files.
5. Update upstream level first, then downstream levels.
6. Record provenance in changed markdown files (`Last Updated`, source references, and where possible commit/scan IDs).
7. Validate links, status consistency, and evidence references.
8. Close issue only when all acceptance criteria are complete.

---

## 7. Repository-Specific Decisions (2026-06-25)

- `ai_working/MODULE_MATURITY_MATRIX.md` is a historical snapshot and is non-canonical for current maturity claims.
- `ROADMAP.md` is the canonical root module maturity aggregate.
- `README.md` remains a Level-3 summary and must mirror `ROADMAP.md` maturity language.

---

## 8. Ownership

- Governance owner: maintainers listed in `MAINTAINERS.md`
- Security owner: security process in `SECURITY.md`
- Release owner: `RELEASE_STRATEGY.md`

If ownership is unclear, default to maintainers and mark issue as blocked.

---

## 9. Compendium Sync Process (docs/compendium/)

### 9.1 Role of the Compendium

`docs/compendium/` is the **consolidated Level 3/4 documentation aggregate** — a complete German-language handbook for ThemisDB users, operators, and architects.

It occupies the following position in the 4-level hierarchy:
- Level-1 input: `src/<module>/` and `docs/de/<module>/` (primary module docs)
- Level-2 input: Base aggregates and cross-module summaries
- **Level-3/4 output:** `docs/compendium/docs/chapter_*.md` (curated handbook chapters)

### 9.2 Sync Cadence

| Trigger | Action |
|---|---|
| New module documentation in `docs/de/` | Corresponding compendium chapter(s) enrichment or creation |
| Breaking API/behavior changes | Compendium chapter updated in same PR |
| Release milestone (v1.x.0) | Full compendium chapter review + VERSION bump |
| Quarterly review | Phase-3 mapping table audit; new cross-references added |

### 9.3 Chapter Enrichment Process

When enriching a compendium chapter from `docs/de/` sources:

1. **Identify sources:** Use `docs/compendium/PHASE3_MAPPING_TABLE.md` to find canonical source files.
2. **Read source content:** Read relevant `docs/de/<module>/` files; never invent content.
3. **Integrate:** Add content sections to the chapter under appropriate section numbers.
4. **Add cross-references:** Add a "Weiterführende Referenzen" section with bidirectional links.
5. **Update mapping table:** Mark chapter status as `✅ VOLLSTÄNDIG` when complete.
6. **Update STATUS_UPDATE.md and ROADMAP.md:** Reflect progress.

### 9.4 Status Markers

Use these markers in `PHASE3_MAPPING_TABLE.md`:
- `✅ VOLLSTÄNDIG` — Chapter fully enriched; all major docs/de sources integrated
- `🔄 ERGÄNZUNG` — Chapter exists; partial content; more docs/de integration needed
- `📝 STUB` — Chapter exists but needs substantial content
- `❌ FEHLT` — Topic missing from compendium entirely

### 9.5 Version Management

- `docs/compendium/VERSION` tracks the current compendium version.
- Bump from `v1.x.0-dev` to `v1.x.0` only after Human-Reviewer sign-off.
- Version is `v1.5.0-dev` as of 2026-08-12.

### 9.6 Build Artifacts

The compendium produces HTML + PDF via Python build scripts (`step1_generate_svgs.py` through `step5_cleanup.py`) and `mkdocs-nav.yml`. The build is currently manual — triggered by a maintainer. CI automation is a planned enhancement (see `docs/compendium/ROADMAP.md`).

### 9.7 Language Policy

The primary language of the compendium is **German**. An English parallel build is not currently planned. English source material from `docs/de/` or `docs/en/` may be referenced via cross-links but is not translated within compendium chapters.
