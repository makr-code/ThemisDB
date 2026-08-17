# Branch and Release Migration Runbook

> Status: Active
> Purpose: step-by-step operational runbook for migrating historical branches, releases, and tags into the canonical ThemisDB edition model

## 1. Preparation

Before making destructive changes:

- review `BRANCHING_STRATEGY.md`
- review `RELEASE_STRATEGY.md`
- review `RELEASE_TAG_MIGRATION.md`
- create or update:
  - `BRANCH_MIGRATION_INVENTORY.md`
  - `RELEASE_TAG_INVENTORY.md`

## 2. Order of Execution

### Step 1 — Lock canonical model

Confirm canonical branches exist and are protected:

- `develop`
- `minimal`
- `community`
- `enterprise`
- `hyperscaler`
- `military`

### Step 2 — Inventory historical branches

Populate `BRANCH_MIGRATION_INVENTORY.md` with:

- legacy permanent branches
- historical release branches
- topic branches
- stale experiment branches

### Step 3 — Inventory historical tags/releases

Populate `RELEASE_TAG_INVENTORY.md` with:

- Community historical tags from `main`
- Military historical tags from `millitary`
- edition-specific release tags

### Step 4 — Fix canonical branch reachability

For each historical release/tag that matters:

- identify intended edition
- ensure tagged commit is reachable from canonical target branch
- avoid retagging unless explicitly approved and safe

### Step 5 — Correct docs and governance references

Update:

- release notes
- changelog references
- migration notes
- branch/release governance docs

### Step 6 — Freeze legacy branches

For `main` and `millitary`:

- stop new PR targeting
- remove canonical references in docs/workflows
- keep only for temporary migration use

### Step 7 — Delete safe historical branches

Delete branches only when their inventory state and checklist allow it.

## 3. Suggested Execution Waves

### Wave A — Highest Priority

- `main` → `community`
- `millitary` → `military`
- open PR targets
- workflow references
- branch protection references

### Wave B — Historical releases

- historical `release/*` branches
- release note alignment
- tag reachability checks

### Wave C — Topic branch cleanup

- `feature/*`
- `bugfix/*`
- `hotfix/*`
- `spike/*`
- `experiment/*`

## 4. Decision Rules

### Delete branch now if:

- merged and no longer referenced
- no open PRs
- no workflow dependency
- no exclusive required commits
- canonical replacement exists

### Keep temporarily if:

- still referenced in migration activity
- open PRs exist
- release/tag mapping not yet verified
- exclusive history still under audit

### Escalate to human review if:

- edition intent is unclear
- branch contains exclusive commits with unclear business value
- tag appears wrong but published
- deletion may affect external consumers

## 5. Minimum Evidence Before Final Legacy Deletion

Before deleting `main` or `millitary`, record that:

- [ ] canonical successor branch is active and protected
- [ ] release/tag mappings are inventoried
- [ ] required historical commits are reachable from canonical branches
- [ ] documentation no longer treats the legacy branch as canonical
- [ ] workflows and settings no longer depend on it
- [ ] open PRs have been retargeted or closed

## 6. Recommended Outcome

At the end of migration, the active operational branch model should be limited to:

- `develop`
- `minimal`
- `community`
- `enterprise`
- `hyperscaler`
- `military`

Historical truth should primarily live in:

- tags
- release notes
- changelog
- migration inventories

---
Zuletzt geprueft: 2026-06-15
