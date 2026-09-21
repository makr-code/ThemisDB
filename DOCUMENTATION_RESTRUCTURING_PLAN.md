# ThemisDB Documentation Restructuring Plan

**Author:** ThemisDB Contributors
**Created:** 2026-09-21
**Last Updated:** 2026-09-21
**Status:** review
**Audience:** dev, ops, ai
**Source of Truth:** true

---

## 1. Executive Summary

ThemisDB has accumulated documentation across more than a dozen distinct locations without a
unified ownership model. The result is **documentation drift**: conflicting descriptions of the
same module in `src/`, `include/`, `docs/`, and the GitHub Wiki; stale API contracts; and
AI-context artifacts that gradually diverge from the root governance sources.

This document defines:
- the canonical **four-layer architecture** for all ThemisDB documentation,
- an **SOT decision matrix** that answers "where is the truth for X?",
- a **concrete Soll-Ist gap table** with remediation owners,
- **operational rules** (naming, metadata, source precedence, conflict resolution),
- an **automation roadmap** (lint, drift detection, PR gates),
- a **staged migration plan** that avoids a big-bang rewrite.

The plan is aligned with [`DOCUMENTATION_GOVERNANCE.md`](DOCUMENTATION_GOVERNANCE.md),
existing Doxygen infrastructure (`Doxyfile`, `Doxyfile.audit`), and the
[`ai_context/developer_llm_wiki/`](ai_context/developer_llm_wiki/) wiki-synthesis pipeline.

---

## 2. As-Is Analysis

### 2.1 Current Documentation Locations

| Location | Volume | Audience | Status |
|---|---|---|---|
| Root `*.md` (30 files) | High-value governance + accumulated debris | Developers, AI agents | Mixed — policy files intentional; BATCH_8 reports and temp `.txt` files are misplaced |
| `src/<module>/*.md` (~1 200 files) | Per-module ROADMAP, README, ARCHITECTURE, FUTURE_ENHANCEMENTS | Developers | Inconsistent template coverage; some modules fully documented, others empty |
| `include/<module>/*.md` | Public API contracts, ROADMAP, README | Developers, integrators | Partially aligned with `ai_context/api_contracts/`; overlap and drift present |
| `tests/<area>/*.md` | Test strategy, coverage notes | Developers | Sparse; mostly absent |
| `benchmarks/<area>/*.md` | Benchmark methodology, KPIs | Developers | Very sparse |
| `docs/` (221 files) | User/admin/operations guides | User, admin, ops | High volume but heterogeneous quality; many archived items mixed with live guides |
| `doxygen_output/` / `doxyfile_output/` | Generated API reference | Developers, integrators | Generated artifact — not hand-edited, but not consistently published |
| `ai_context/` (25+ files) | AI policies, API contracts, wiki synthesis | AI agents, developers | Best-maintained layer; wiki-synthesis pipeline exists but manual sync needed |
| `ai_context/developer_llm_wiki/` (8 files) | Compiled wiki view | AI agents | Synthesized from root SOT; drift risk when root sources change |
| `ai_working/` | Ephemeral work artifacts | AI agents | Intentionally not normative; inclusion in PR reviews is a known false-positive risk |
| `compendium/` | Product compendium | User, admin | Partially overlaps `docs/`; ownership unclear |
| GitHub Wiki | User-facing portal pages | User, admin | Not synchronized with `docs/`; content frequently lags behind code |
| Scattered root artefacts | `BATCH_8_*.md`, `*.txt`, `fix_*.py`, binaries | — | Should be deleted or moved to `docs/reports/`, `scripts/`, or `/tmp` |

### 2.2 Identified Structural Problems

1. **No canonical layer model enforced** — the same information (e.g. transaction module architecture)
   appears in `src/transaction/ARCHITECTURE.md`, `include/transaction/ARCHITECTURE.md`,
   `docs/DISTRIBUTED_TRANSACTIONS.md`, and the GitHub Wiki without a stated SOT.

2. **Root directory pollution** — temporary reports, `.txt` build logs, and helper scripts
   (`fix_http.py`, `fix_patterns.py`, `verify_fixes.cpp`) are committed at root alongside
   first-class governance documents.

3. **Documentation metadata gate is partial** — `gate-pr-doc-metadata.yml` enforces metadata
   fields on changed files but does not enforce them proactively on the existing corpus.

4. **No enforced per-module template** — modules vary from fully documented (`plugins`, `rag`,
   `transaction`) to near-empty (`fuzz`, `demo`, `loras`).

5. **Doxygen builds are not gated on public API completeness** — coverage gate exists
   (`Doxyfile.audit`) but threshold is permissive and not enforced at merge time for all modules.

6. **Wiki is a documentation dead end** — GitHub Wiki pages are not generated from repo sources
   and are not included in any PR gate.

7. **`ai_context/` drift** — `developer_llm_wiki/` is periodically regenerated but the cadence
   is ad hoc; no automated delta report triggers a re-generation.

8. **`docs/` is mixed-use** — live operational guides, archived material, CI/CD strategy docs,
   and one-off implementation reports coexist at the same level without clear separation.

---

## 3. Target Architecture (To-Be)

### 3.1 Four-Layer Model

```
Layer 0 — Governance (Root SOT)
  ROADMAP.md, FUTURE_ENHANCEMENTS.md, BRANCHING_STRATEGY.md,
  RELEASE_STRATEGY.md, VERSIONING.md, DOCUMENTATION_GOVERNANCE.md,
  CHANGELOG.md, SECURITY.md, CONTRIBUTING.md

Layer 1 — Developer / Engineering Docs (near code)
  src/<module>/README.md        — design intent, invariants, module flow
  src/<module>/ARCHITECTURE.md  — internal structure
  include/<module>/README.md    — public contract description
  include/<module>/ROADMAP.md   — module-level roadmap (delegates to root)
  tests/<area>/README.md        — test strategy, coverage expectations
  benchmarks/<area>/README.md   — benchmark methodology, KPIs

Layer 2 — Generated API Reference
  doxygen_output/               — generated; not hand-edited
  Source: header comments + Doxygen annotations
  Published: GitHub Pages (optional)

Layer 3 — User / Admin / Operations Docs
  docs/user/         — end-user guides
  docs/admin/        — administration and configuration
  docs/operations/   — runbooks, monitoring, SLA
  docs/security/     — security policies and evidence
  docs/governance/   — release and process governance docs
  docs/reports/      — dated build/CI reports (archived after 90 days)
  docs/ARCHIVED/     — superseded material

Layer 4 — AI Context (policy + synthesized views)
  ai_context/                            — normative AI policies
  ai_context/api_contracts/              — API contract files (derived from Layer 1)
  ai_context/developer_llm_wiki/         — compiled/synthesized wiki view
  ai_working/                            — ephemeral; not SOT
```

### 3.2 Root-Level Governance Files (Permitted at Root)

Only the following file categories belong at root level:

| Category | Examples |
|---|---|
| Build system | `CMakeLists.txt`, `CMakePresets.json`, `CMakeUserPresets.json.example` |
| Build toolchain configs | `Doxyfile*`, `vcpkg.json`, `pom.xml`, `mkdocs.yml` |
| Container/deployment | `Dockerfile*`, `docker-compose*.yml`, `docker-bake.hcl` |
| Governance docs | `ROADMAP.md`, `CHANGELOG.md`, `CONTRIBUTING.md`, `SECURITY.md`, `BRANCHING_STRATEGY.md`, `VERSIONING.md`, `RELEASE_STRATEGY.md`, `DOCUMENTATION_GOVERNANCE.md`, `CODE_OF_CONDUCT.md`, `MAINTAINERS.md` |
| Navigation / onboarding | `README.md`, `INDEX.md`, `QUICKSTART.md`, `SETUP.md` |
| Tool/IDE config | `.vscode/`, `.devcontainer/`, `.github/`, `.gitignore`, `.pre-commit-config.yaml` |
| Module index | `MODULE_INDEX.md` |

Everything else must be in a subdirectory or deleted.

---

## 4. SOT Decision Matrix

> When two sources conflict, the entry with the lower number wins.

| Domain | Canonical SOT | Secondary / Derived | Do Not Use As SOT |
|---|---|---|---|
| Product governance (release, versions) | `ROADMAP.md`, `RELEASE_STRATEGY.md`, `VERSIONING.md`, `CHANGELOG.md` | `ai_context/developer_llm_wiki/GOVERNANCE_AND_ROADMAP.md` | GitHub Wiki release pages |
| Module design and invariants | `src/<module>/README.md` + `src/<module>/ARCHITECTURE.md` | `ai_context/developer_llm_wiki/MODULES_AND_APIS.md` | `docs/` architecture docs |
| Public API contracts | `include/<module>/` header comments (Doxygen) | `ai_context/api_contracts/<module>.md` | Any wiki or `docs/` API doc |
| Build, test, CI | `BUILD_TEST_CI_AND_OPERATIONS.md` (ai_context wiki) | `.github/workflows/`, `CTEST.md` | Scattered `docs/CI_CD_*.md` |
| User/admin/ops guides | `docs/user/`, `docs/admin/`, `docs/operations/` | `compendium/` (review quarterly) | Root `*.md` files |
| Security policy | `SECURITY.md` (root) + `docs/security/` | `src/<module>/SECURITY.md` | GitHub Wiki security pages |
| AI agent instructions | `ai_context/COPILOT_INSTRUCTIONS.md` + `.github/copilot-instructions.md` | `ai_context/developer_llm_wiki/` | `ai_working/` |
| Branch / edition governance | `BRANCHING_STRATEGY.md` | `RELEASE_STRATEGY.md` | Any `docs/` branching docs |
| Documentation rules | `DOCUMENTATION_GOVERNANCE.md` | `DOCUMENTATION_RESTRUCTURING_PLAN.md` (this file) | `docs/DOCS_ORGANIZATION_PLAN.md` (archived) |

---

## 5. Soll-Ist Comparison

### 5.1 Gap Table

| # | Gap | Severity | Remediation | Owner |
|---|---|---|---|---|
| G-01 | Root contains temporary build reports (`BATCH_8_*.md`, `*.txt`) and helper scripts | High | Move to `docs/reports/compiler-warnings/` or delete; add `.gitignore` rule | Any contributor |
| G-02 | Doxygen gate does not enforce public API completeness per module | High | Lower Doxyfile.audit threshold; add per-module coverage floor in CI | CI team |
| G-03 | No enforced per-module documentation template | High | Define template in `docs/_standards/module-template.md`; add PR gate check | Documentation owner |
| G-04 | `docs/` mixes live guides, archived material, CI docs, and one-off reports | High | Enforce `docs/ARCHIVED/` migration for stale content; add structure guard workflow | Documentation owner |
| G-05 | GitHub Wiki diverges from `docs/` content | Medium | Establish Wiki as portal only; sync key overview pages from `docs/` via workflow | Any contributor |
| G-06 | `ai_context/developer_llm_wiki/` regeneration is ad hoc | Medium | Add scheduled or post-merge wiki regeneration workflow | AI infra team |
| G-07 | `compendium/` has undefined ownership and overlaps `docs/` | Medium | Audit and merge relevant compendium content into `docs/`; archive or delete remainder | Documentation owner |
| G-08 | `include/<module>/` docs partially duplicate `src/<module>/` docs | Medium | Define canonical split: `include/` = public contract surface; `src/` = internals | Each module owner |
| G-09 | `tests/` and `benchmarks/` directories lack standard README coverage | Low | Roll out test-strategy README template to high-priority modules first | Each module owner |
| G-10 | Metadata frontmatter gate is not retroactively enforced | Low | Run inventory script to identify non-conforming files; schedule remediation | Any contributor |
| G-11 | `ai_context/api_contracts/` not consistently updated with header changes | Medium | Add PR gate: `include/**` change → require `ai_context/api_contracts/<module>.md` update | CI team |
| G-12 | `docs/` contains `CI_CD_*.md` files that duplicate workflow source | Low | Archive or delete; workflow source in `.github/workflows/` is SOT | Any contributor |

### 5.2 Quick-Win Deletions (safe to remove now)

| File | Reason |
|---|---|
| Root `*.txt` log files (`cmake_config_test_error.txt`, `llvm_filtered_diag.txt`, `files_with_*.txt`) | Build-time scratch files; not documentation |
| Root `cs.txt`, `cs_out.txt`, `out_content.txt` | Empty scratch files |
| Root `error_fail.xml` | Test run artifact |
| `verify_fixes`, `verify_implementation` (binaries) | Build artifacts; should not be committed |
| `fix_http.py`, `fix_patterns.py` (root) | Ad hoc scripts; move to `scripts/` if still needed |

---

## 6. Operational Rules

### 6.1 Naming Conventions

- **Root governance documents**: `UPPER_SNAKE_CASE.md`
- **Module docs near code**: `UPPER_SNAKE_CASE.md` (e.g. `README.md`, `ARCHITECTURE.md`)
- **User/admin docs in `docs/`**: `UPPER_SNAKE_CASE.md` (consistent with current corpus)
- **No semantic duplicates within one scope**: a module must not have both `ARCHITECTURE.md`
  and `architecture.md` in the same directory
- **Dated reports**: `REPORT_NAME_YYYY-MM-DD.md` under `docs/reports/<topic>/`
- **Archived files**: move to `docs/ARCHIVED/<original-path>/` or `docs/archive/<original-path>/`;
  do not rename unless the original name was ambiguous

### 6.2 Mandatory Metadata Block

Every Markdown file governed by the doc-metadata gate (see `DOCUMENTATION_GOVERNANCE.md`)
must declare near the top:

```markdown
**Author:** <team or person>
**Created:** YYYY-MM-DD
**Last Updated:** YYYY-MM-DD
**Status:** draft | review | active | approved | stable | deprecated | archived
```

Or equivalent YAML front matter.

### 6.3 Source Precedence and Conflict Resolution

When two documents make conflicting claims:

1. Identify the applicable SOT domain from Section 4 (SOT Decision Matrix).
2. The SOT file wins. The non-SOT file must be corrected or marked as derived.
3. If both files claim to be SOT for the same domain, escalate to the documentation owner;
   interim: prefer the file with the more recent `Last Updated` date.
4. Derived / generated files (Doxygen output, wiki synthesis) must never be edited manually;
   fix the source instead.

### 6.4 Module Documentation Minimum Standard

Every C++ module (`src/<module>/`, `include/<module>/`) that has at least one public header
must maintain:

| File | Required Content |
|---|---|
| `include/<module>/README.md` | Purpose, public surface summary, threading model, ownership rules |
| `src/<module>/README.md` | Design intent, internal structure, error/timeout/cancellation behavior |
| `src/<module>/ROADMAP.md` | Status, active work, known issues (may delegate to root ROADMAP) |

Template: [`docs/_standards/module-template.md`](docs/_standards/module-template.md)
(to be created in Phase 2).

---

## 7. Automation Strategy

### 7.1 Required Workflows

| Workflow | Trigger | Action |
|---|---|---|
| `docs-lint.yml` | PR touching `*.md` | markdownlint + lychee (broken links) |
| `docs-structure-guard.yml` | PR touching `docs/`, root `*.md`, `src/**/*.md` | Check forbidden locations (e.g. temp reports at root); check metadata completeness |
| `doxygen-audit.yml` | PR touching `include/**`, `src/**` | Doxygen build with `Doxyfile.audit`; fail on coverage below threshold |
| `docs-drift-check.yml` | Push to `develop`, scheduled weekly | Compare `ai_context/api_contracts/*.md` timestamps vs `include/**` last-changed; report drift as issue or PR comment |
| `change-impact-doc-gate.yml` | PR touching `include/**` or root governance files | Require paired update in `ai_context/api_contracts/` or wiki; block merge if missing |
| `wiki-sync.yml` | Push to `develop` (post-merge) | Regenerate `ai_context/developer_llm_wiki/` from root SOT sources; open auto-PR if delta exceeds threshold |

### 7.2 Scripts

| Script | Location | Purpose |
|---|---|---|
| `inventory.py` | `scripts/docs/` | Enumerate all `.md` files, extract metadata, output gap report |
| `drift_check.py` | `scripts/docs/` | Compare API contract files vs header timestamps; emit drift list |
| `archive_stale.py` | `scripts/docs/` | Move docs with `Status: deprecated` older than N days to `ARCHIVED/` |
| `validate_module_coverage.py` | `scripts/docs/` | Check every C++ module directory has required README/ROADMAP |

### 7.3 PR Template Extension

Add to `.github/pull_request_template.md`:

```markdown
## Documentation Impact

- [ ] Root governance files affected? If yes, which ones? <!-- ROADMAP, VERSIONING, etc. -->
- [ ] Module/API docs updated in the same PR?
- [ ] Doxygen impact assessed (new/changed public API)?
- [ ] `ai_context/api_contracts/` updated if `include/**` changed?
- [ ] `ai_context/developer_llm_wiki/` regeneration triggered / not required?
```

---

## 8. Staged Migration Plan

### Phase 1 — Inventory and Classify (Weeks 1–2)

- [ ] Run `scripts/docs/inventory.py` to enumerate all `.md` files with metadata status.
- [ ] Classify each root-level `*.md`, `*.txt`, binary: governance | reports | debris.
- [ ] Move or delete root debris (Section 5.2 quick-wins).
- [ ] Tag `docs/` stale or clearly archive-ready files with `Status: deprecated`.
- [ ] Publish inventory result to `docs/governance/DOCS_INVENTORY_YYYY-MM-DD.md`.

**Exit criteria:** all root files are either governance, navigation, or removed.

### Phase 2 — Canonical Structure and Templates (Weeks 3–5)

- [ ] Create `docs/_standards/module-template.md` and `docs/_standards/api-contract-template.md`.
- [ ] Migrate `docs/` into subdirectory structure (`user/`, `admin/`, `operations/`, `security/`,
  `governance/`, `reports/`, `ARCHIVED/`).
- [ ] Roll out module README template to top-10 most-referenced C++ modules.
- [ ] Update `DOCUMENTATION_GOVERNANCE.md` SOT precedence table to match this plan.
- [ ] Establish wiki as portal only: remove duplicate full-text content; replace with links to `docs/`.

**Exit criteria:** `docs/` has clear subdirectory structure; top-10 modules have compliant READMEs.

### Phase 3 — Automation (Weeks 6–8)

- [ ] Implement and activate `docs-lint.yml`, `docs-structure-guard.yml`.
- [ ] Implement and activate `doxygen-audit.yml` with per-module coverage floor.
- [ ] Implement `change-impact-doc-gate.yml` for `include/**` changes.
- [ ] Implement `docs-drift-check.yml` (weekly schedule + PR trigger).
- [ ] Extend PR template with documentation impact checklist.
- [ ] Implement `scripts/docs/inventory.py` and `scripts/docs/drift_check.py`.

**Exit criteria:** all 5 workflows passing on `develop`; drift check produces actionable report.

### Phase 4 — Rollout and Migration (Weeks 9–14)

- [ ] Systematically remediate G-01 through G-12 gap items (Section 5.1).
- [ ] Expand module README coverage from top-10 to all modules with public headers.
- [ ] Complete `ai_context/api_contracts/` alignment with `include/**` headers.
- [ ] Trigger `wiki-sync.yml` to regenerate `ai_context/developer_llm_wiki/`.
- [ ] Archive or migrate remaining `compendium/` content.
- [ ] Publish `docs/governance/DOCS_MIGRATION_COMPLETION_YYYY-MM-DD.md`.

**Exit criteria:** zero G-HIGH gaps open; drift check reports no stale API contracts.

### Phase 5 — Governance and Health (Ongoing)

- [ ] Monthly: automated `DOCS_HEALTH_REPORT` generated by `inventory.py`; reviewed in sprint.
- [ ] Quarterly: architecture review against this plan; update plan if structure changes.
- [ ] Annually: full re-audit of all `docs/` content for relevance and accuracy.

**Exit criteria (continuous):** drift check clean for 3 consecutive months; no open G-HIGH gaps.

---

## 9. Success Criteria and Review Cadence

### 9.1 Success Criteria

| Criterion | Measurable Target |
|---|---|
| Root pollution cleared | Zero non-governance, non-build-system files at root |
| Module doc coverage | ≥ 80% of public C++ modules have compliant README + ROADMAP |
| API contract freshness | Zero `ai_context/api_contracts/` files more than 30 days behind matching header |
| Doxygen coverage | ≥ 70% symbol documentation for all `include/**` public APIs |
| Metadata compliance | 100% of `docs/` live files have valid metadata block |
| Broken links | Zero broken local links in root governance files and `docs/` live files |
| Wiki sync | `ai_context/developer_llm_wiki/` regenerated within 7 days of any root-SOT change |

### 9.2 Review Cadence

| Review Type | Cadence | Owner |
|---|---|---|
| Docs health snapshot | Monthly | Documentation owner |
| Drift report triage | Weekly (automated) | CI bot + documentation owner |
| Module doc coverage audit | Quarterly | Each module owner |
| Full architecture review | Annually | Lead architects + documentation owner |
| This plan (DOCUMENTATION_RESTRUCTURING_PLAN.md) | After each Phase exit | Documentation owner |

---

## Related Files

- [`DOCUMENTATION_GOVERNANCE.md`](DOCUMENTATION_GOVERNANCE.md) — SOT precedence and metadata gate rules (normative)
- [`docs/README.md`](docs/README.md) — docs navigation entry point
- [`ai_context/COPILOT_INSTRUCTIONS.md`](ai_context/COPILOT_INSTRUCTIONS.md) — AI agent instructions including documentation enforcement
- [`ai_context/developer_llm_wiki/INDEX.md`](ai_context/developer_llm_wiki/INDEX.md) — compiled wiki index
- [`AI_WIKI_INTEGRATION_PLAYBOOK.md`](AI_WIKI_INTEGRATION_PLAYBOOK.md) — AI wiki usage playbook
- [`ROADMAP.md`](ROADMAP.md) — product roadmap (release gate model)
