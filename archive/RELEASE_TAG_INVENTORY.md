# Release / Tag Migration Inventory Template

> Status: Active
> Purpose: operational inventory and decision template for reassigning historical releases and tags to the canonical ThemisDB edition branches

## 1. Scope

Use this document to track historical:

- Git tags
- GitHub Releases
- release notes alignment
- historical release branches
- branch reachability of tagged commits

## 2. Classification States

Each tag/release entry must have exactly one state:

- `aligned`
- `needs-branch-alignment`
- `needs-doc-fix`
- `needs-human-audit`
- `safe-to-retag`

## 3. Inventory Columns

Recommended fields per tag/release:

| Tag / Release | Commit SHA | Historical Branch Context | Published? | Intended Edition | Canonical Target Branch | Reachable From Canonical Branch? | Release Notes Present? | State | Recommended Action | Owner | Notes |
|---|---|---|---|---|---|---|---|---|---|---|---|

## 4. Initial Priority Waves

### Wave 1 — Community and Military legacy mappings

| Tag / Release | Commit SHA | Historical Branch Context | Published? | Intended Edition | Canonical Target Branch | Reachable From Canonical Branch? | Release Notes Present? | State | Recommended Action | Owner | Notes |
|---|---|---|---|---|---|---|---|---|---|---|---|
| `v*` from historical `main` releases | TBD | `main` | TBD | community | `community` | TBD | TBD | `needs-human-audit` | Verify commit reachability and release docs, then align | TBD | Community historical release set |
| `military-v*` or equivalent from `millitary` context | TBD | `millitary` | TBD | military | `military` | TBD | TBD | `needs-human-audit` | Verify commit reachability and release docs, then align | TBD | Military historical release set |

### Wave 2 — Edition-specific historical releases

| Tag / Release | Commit SHA | Historical Branch Context | Published? | Intended Edition | Canonical Target Branch | Reachable From Canonical Branch? | Release Notes Present? | State | Recommended Action | Owner | Notes |
|---|---|---|---|---|---|---|---|---|---|---|---|
| `minimal-v*` | TBD | legacy or canonical | TBD | minimal | `minimal` | TBD | TBD | `needs-human-audit` | Verify alignment | TBD | |
| `enterprise-v*` | TBD | legacy or canonical | TBD | enterprise | `enterprise` | TBD | TBD | `needs-human-audit` | Verify alignment | TBD | |
| `hyperscaler-v*` | TBD | legacy or canonical | TBD | hyperscaler | `hyperscaler` | TBD | TBD | `needs-human-audit` | Verify alignment | TBD | |

## 5. Action Rules

### If state = `aligned`
- No structural correction required
- Keep tag unchanged
- Ensure docs remain accurate

### If state = `needs-branch-alignment`
- Make tagged commit reachable from canonical target branch
- Keep published tag unchanged
- Update migration notes

### If state = `needs-doc-fix`
- Correct release notes / changelog / governance references
- Keep tag unchanged

### If state = `needs-human-audit`
- Pause destructive actions
- Resolve edition/branch intent first

### If state = `safe-to-retag`
- Only for clearly unpublished/internal tags
- Requires explicit human approval before retagging

## 6. Retagging Guardrails

Do **not** retag by default.

Before any retagging, confirm:

- [ ] tag is internal or unpublished
- [ ] human approval is explicitly recorded
- [ ] no external consumer depends on current tag
- [ ] correction rationale is documented
- [ ] replacement action is recorded in release documentation

## 7. Deletion Dependency Rule

Do not delete historical release branches until:

- release/tag inventory entry exists
- tagged commit alignment is confirmed
- release notes exist or are corrected
- no active PR/workflow depends on that branch

---
Zuletzt geprueft: 2026-06-15
