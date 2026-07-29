# ThemisDB Branching Strategy

> Status: Active
> Purpose: canonical branch, edition, merge, and branch-normalization governance for ThemisDB
>
> Canonical root governance set:
> `BRANCHING_STRATEGY.md` → `RELEASE_STRATEGY.md` → `VERSIONING.md` → `ROADMAP.md` → `FUTURE_ENHANCEMENTS.md` → `CHANGELOG.md`

## 1. Purpose

This document defines the canonical Git branch model for ThemisDB across all editions.

It exists to ensure:

- clear mapping between product editions and release lanes
- reproducible release preparation and tagging
- deterministic merge and hotfix flows
- clean classification and normalization of long-lived and historical branches
- unambiguous rules for human maintainers, contributors, and AI agents

## 2. Canonical Branch Model

### 2.1 Permanent Branches

- `develop` — primary integration branch
- `minimal` — release lane for Minimal Edition
- `community` — release lane for Community Edition
- `enterprise` — release lane for Enterprise Edition
- `hyperscaler` — release lane for Hyperscaler Edition
- `military` — release lane for Military Edition

### 2.2 Legacy Branches

The following names are legacy-only and must not be used for new work:

- `main` — historical name of the Community release lane; replaced by `community`
- `millitary` — historical misspelling; replaced by `military`

Rules:

- no new feature, fix, release, or hotfix branch may target `main`
- no new feature, fix, release, or hotfix branch may target `millitary`
- any remaining references must be migrated to the canonical names

## 3. Edition-to-Branch Mapping

| Edition | Canonical Branch |
|---|---|
| Minimal | `minimal` |
| Community | `community` |
| Enterprise | `enterprise` |
| Hyperscaler | `hyperscaler` |
| Military | `military` |

`develop` is not an edition branch. It is the integration branch for ongoing work.

## 4. Default Branch Policy

`develop` is the default branch for normal development and pull requests.

Implications:

- feature work targets `develop`
- normal bugfix work targets `develop`
- release preparation promotes selected states from `develop` into edition release lanes
- edition branches are not primary development branches

### 4.1 Release-Readiness Gate On `develop`

- The beta-to-GA hardening path must be proven on `develop` before any promotion into a canonical edition lane.
- `release_critical` CI on `develop` is the mandatory entry gate for release work; edition-lane promotion must not bypass it.
- GA promotion additionally requires current Wave-7 PASS evidence and documented sign-off for the highest-risk modules (`server`, `llm`, `sharding`).
- GA promotion follows the tracked execution batches (A-D) defined in root governance docs; skipping a batch boundary is not allowed.
- Direct release-lane bypass is allowed only for the hotfix exception flow in §6.4.

## 4.2 Private Plugin Family Repositories

Private plugin family repositories follow the same canonical branch model when they are provisioned.

Provisioned Wave-1 private repositories (2026-07):

| Repository | Submodule path | Contents |
|---|---|---|
| `makr-code/themisdb_ethic_ai` | `plugins/private/themisdb_ethic_ai/` | ethics_ai plugin root |
| `makr-code/themisdb_storage` | `plugins/private/themisdb_storage/` | `user_storage_encrypted/`, `azure_blob_storage/`, `s3_blob_storage/` |
| `makr-code/themisdb_importer` | `plugins/private/themisdb_importer/` | `mysql_importer/`, `mongo_importer/`, `kafka_importer/`, `s3_importer/` |
| `makr-code/themisdb_llm_wiki` | `plugins/private/themisdb_llm_wiki/` | LLM Wiki tool |

Branch rules:

- normal implementation targets `develop`
- release promotion uses `enterprise`, `hyperscaler`, or `military` only when the family publishes edition-specific artefacts
- no private plugin repository may introduce new `main` or `millitary` automation, documentation, or PR targets
- the superproject consumes private plugin repositories only through commit-pinned submodules at the paths above, never floating branches

## 5. Branch Types

### 5.1 Feature Branches

Format:

- `feature/<area>/<ticket>-<slug>`

Base branch:

- `develop`

Target branch:

- `develop`

### 5.2 Bugfix Branches

Format:

- `bugfix/<area>/<ticket>-<slug>`

Base branch:

- `develop`

Target branch:

- `develop`

### 5.3 Release Preparation Branches

Format:

- `release/<edition>/vX.Y.Z`
- `release/<edition>/vX.Y.Z-rcN`

Base branch:

- usually `develop`

Target branch:

- `minimal`
- `community`
- `enterprise`
- `hyperscaler`
- `military`

depending on the edition

### 5.4 Hotfix Branches

Format:

- `hotfix/<edition>/<ticket>-<slug>`

Base branch:

- affected edition release lane

Target branch:

- affected edition release lane

Mandatory follow-up:

- back-merge or cherry-pick into `develop`
- evaluate propagation into other edition release lanes

### 5.5 Spike / Experiment Branches

Format:

- `spike/<topic>`
- `experiment/<topic>`

These branches must not become permanent release carriers.

## 6. Merge and Promotion Rules

### 6.1 Normal Development Flow

- `feature/*` → `develop`
- `bugfix/*` → `develop`

### 6.2 Release Promotion Flow

Releases are promoted from `develop` into the target edition lane.

Examples:

- `release/community/v1.9.0` → `community`
- `release/enterprise/v1.9.0` → `enterprise`
- `release/military/v1.9.0` → `military`

### 6.3 Edition Branch Rules

- do not develop directly on edition release lanes
- do not use edition lanes as long-lived feature branches
- only merge release-prepared or emergency hotfix content into edition lanes

### 6.4 Hotfix Rules

If an urgent production fix must land directly on an edition lane:

1. branch from the affected edition lane
2. merge hotfix into that edition lane
3. back-merge or cherry-pick the fix into `develop`
4. assess whether the fix must also be applied to other editions

## 7. Canonical Naming Rules

Mandatory canonical names:

- `community` instead of `main`
- `military` instead of `millitary`

AI agents, automation, workflow definitions, documentation, and review templates must use canonical names only.

## 8. Branch Protection Intent

Protected permanent branches:

- `develop`
- `minimal`
- `community`
- `enterprise`
- `hyperscaler`
- `military`

Protection intent:

- no force-pushes
- no direct pushes except explicitly authorized release maintenance
- PR review required
- required checks as defined by repository policy
- conversation resolution required for protected release lanes

## 9. Branch Inventory and Normalization Program

The historical branch inventory must be classified into one of these states:

- `active-topic`
- `active-release-prep`
- `active-hotfix`
- `merged-can-delete`
- `stale-archive`
- `legacy-rename`
- `needs-human-decision`

For each historical branch capture:

- branch name
- canonical target edition (if any)
- source/base branch
- last commit date
- last author
- open PR state
- merged/unmerged status
- legacy naming issues
- recommended action

## 10. Historical Branch Retention and Deletion Policy

Historical branches are temporary migration objects, not the long-term archive of release history.

### 10.1 Archive principle

The durable historical record should be preserved through:

- Git tags
- release notes
- `CHANGELOG.md`
- roadmap/governance documents
- Git history reachable from canonical branches where required

Historical branches should not be kept indefinitely if they no longer serve an active operational purpose.

### 10.2 Retention classes

Each historical branch must be assigned exactly one retention state:

- `keep-canonical` — permanent branch in active use
- `temporary-legacy` — legacy branch kept only during migration
- `merged-can-delete` — fully merged and safe to remove
- `stale-archive` — not active, retained only for short-term review/audit
- `needs-human-decision` — contains unclear or exclusive history

### 10.3 Deletion preconditions

A historical branch may be deleted only when all of the following are true:

1. no open pull requests target it
2. no required workflow or branch protection depends on it
3. no active documentation still points contributors to it as a canonical target
4. no required exclusive commits would be lost
5. its relevant release state is preserved by tags, release notes, or canonical branch history
6. the canonical replacement branch is established and protected

### 10.4 Legacy branch policy

For current legacy branches:

- `main` should be treated as `temporary-legacy` until Community release migration is complete, then deleted
- `millitary` should be treated as `temporary-legacy` until Military release migration is complete, then deleted

### 10.5 Release branches

Historical `release/*` branches should normally be deleted after:

- release tagging is complete
- release notes exist
- the target release lane contains the released state
- any required back-merge/cherry-pick work is done

### 10.6 Feature and bugfix branches

Historical `feature/*` and `bugfix/*` branches should be deleted after merge. Stale, abandoned, or superseded topic branches should be classified and removed in cleanup waves.

## 11. AI / Agent Governance

AI agents operating in this repository MUST follow this branch model.

### 11.1 Canonical assumptions

Unless the user explicitly says otherwise:

- normal implementation work targets `develop`
- community release work targets `community`
- military release work targets `military`
- legacy names `main` and `millitary` must not be proposed for new work

### 11.2 Required document synchronization

When branch governance changes, keep these files aligned:

- `BRANCHING_STRATEGY.md`
- `RELEASE_STRATEGY.md`
- `.github/copilot-instructions.md`
- `ai_context/COPILOT_INSTRUCTIONS.md`
- `VERSIONING.md`
- `CHANGELOG.md`
- `ROADMAP.md`
- `FUTURE_ENHANCEMENTS.md`

### 11.3 AI review blockers

AI-generated changes are incomplete if they:

- introduce or retain `main` as canonical community branch
- introduce or retain `millitary` as canonical branch name
- propose PR targets against legacy branches
- update release rules without updating AI instruction files
- update governance docs without cross-document synchronization

## 12. Migration Status

Current migration intent:

- rename conceptual Community lane from `main` to `community`
- normalize `millitary` to `military`
- keep temporary legacy awareness only for migration and audit purposes
- remove legacy references progressively from docs, workflows, templates, and automation

## 13. Review and Ownership

- Project Lead owns final branch governance decisions
- Release management must enforce canonical branch names
- Documentation and AI-instruction updates are part of governance completion, not optional follow-up work

---
Zuletzt geprueft: 2026-06-15
