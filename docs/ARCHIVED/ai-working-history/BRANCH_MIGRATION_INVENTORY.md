# Branch Migration Inventory Template

> Status: Active
> Purpose: operational inventory and decision template for normalizing historical branches into the canonical ThemisDB branch model

## 1. Scope

Use this document to track the migration status of historical branches into the canonical branch set:

- `develop`
- `minimal`
- `community`
- `enterprise`
- `hyperscaler`
- `military`

Historical or legacy branch names such as `main` and `millitary` must be tracked here until fully retired.

## 2. Classification States

Each historical branch must have exactly one state:

- `keep-canonical`
- `temporary-legacy`
- `merged-can-delete`
- `stale-archive`
- `needs-human-decision`

## 3. Inventory Columns

Recommended fields per branch:

| Branch | Type | Historical Source / Context | Intended Edition | Canonical Target | Open PRs | Exclusive Commits? | Workflow / Protection Dependency? | State | Recommended Action | Owner | Notes |
|---|---|---|---|---|---|---|---|---|---|---|---|

## 4. Initial Priority Waves

### Wave 1 — Permanent / Legacy branch normalization

| Branch | Type | Historical Source / Context | Intended Edition | Canonical Target | Open PRs | Exclusive Commits? | Workflow / Protection Dependency? | State | Recommended Action | Owner | Notes |
|---|---|---|---|---|---|---|---|---|---|---|---|
| `develop` | permanent | canonical integration branch | integration | `develop` | TBD | No | Yes | `keep-canonical` | Keep protected | TBD | Default branch target |
| `minimal` | permanent | canonical minimal release lane | minimal | `minimal` | TBD | No | Yes | `keep-canonical` | Keep protected | TBD | |
| `community` | permanent | canonical Community release lane | community | `community` | TBD | No | Yes | `keep-canonical` | Keep protected | TBD | Replaces `main` |
| `enterprise` | permanent | canonical Enterprise release lane | enterprise | `enterprise` | TBD | No | Yes | `keep-canonical` | Keep protected | TBD | |
| `hyperscaler` | permanent | canonical Hyperscaler release lane | hyperscaler | `hyperscaler` | TBD | No | Yes | `keep-canonical` | Keep protected | TBD | |
| `military` | permanent | canonical Military release lane | military | `military` | TBD | No | Yes | `keep-canonical` | Keep protected | TBD | Replaces `millitary` |
| `main` | legacy | historical Community release branch | community | `community` | TBD | TBD | TBD | `temporary-legacy` | Freeze, migrate references, then delete | TBD | Do not use for new work |
| `millitary` | legacy | historical misspelled Military branch | military | `military` | TBD | TBD | TBD | `temporary-legacy` | Freeze, migrate references, then delete | TBD | Typo branch |

### Wave 2 — Historical release branches

Populate with branches such as:

- `release/*`
- `release/community/*`
- `release/enterprise/*`
- `release/military/*`

Template row:

| Branch | Type | Historical Source / Context | Intended Edition | Canonical Target | Open PRs | Exclusive Commits? | Workflow / Protection Dependency? | State | Recommended Action | Owner | Notes |
|---|---|---|---|---|---|---|---|---|---|---|---|
| `release/...` | release | historical release prep branch | TBD | TBD | TBD | TBD | TBD | `needs-human-decision` | Verify tag, merge state, then delete or archive | TBD | |

### Wave 3 — Topic branches

Populate with branches such as:

- `feature/*`
- `bugfix/*`
- `hotfix/*`
- `spike/*`
- `experiment/*`

Recommended action guide:

- merged topic branch → `merged-can-delete`
- stale unused branch → `stale-archive`
- exclusive unclear history → `needs-human-decision`

## 5. Deletion Checklist

Before deleting any branch, confirm:

- [ ] no open PR targets the branch
- [ ] no required workflow still references the branch
- [ ] no protected-branch rule still depends on it
- [ ] no unique required commits would be lost
- [ ] canonical replacement branch is established
- [ ] release/tag traceability is preserved
- [ ] action is recorded in migration notes

## 6. Operational Notes

- Historical branches are not the long-term archive.
- Use tags, release notes, and canonical branch history as the durable record.
- `main` and `millitary` should remain only as temporary migration objects until safely retired.

---
Zuletzt geprueft: 2026-06-15
