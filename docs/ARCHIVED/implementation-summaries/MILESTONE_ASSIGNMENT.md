# Milestone Assignment – One-shot bulk assignment

## Purpose

The **Assign Issues to Milestone** workflow (`assign-milestone.yml`) lets any
maintainer assign all open issues that currently have **no milestone** to a
chosen milestone in a single run.  It is idempotent: issues that already have
a milestone are never touched.

---

## How to run

### Via GitHub Actions UI

1. Go to **Actions → Assign Issues to Milestone** in the repository.
2. Click **Run workflow**.
3. Fill in the inputs:

   | Input | Required | Description |
   |-------|----------|-------------|
   | `milestone` | ✅ | Milestone **number** (e.g. `3`) or **exact title** (e.g. `v1.5.0`) |
   | `dry_run` | ❌ | Set to `true` to preview changes without writing anything (default: `false`) |

4. Click **Run workflow** to start.

### Via GitHub CLI

```bash
# Assign by milestone number
gh workflow run assign-milestone.yml \
  -f milestone=3 \
  -f dry_run=false

# Preview without making changes (dry-run)
gh workflow run assign-milestone.yml \
  -f milestone="v1.5.0" \
  -f dry_run=true
```

---

## How the script works

The underlying script is `.github/scripts/assign_milestone.py`.

1. **Resolve milestone** – looks up the milestone by number or exact title
   (searches open *and* closed milestones). Exits with an error when nothing
   or more than one milestone matches.
2. **Fetch unassigned issues** – paginates through all open issues where
   `milestone == null`, filtering out pull requests.
3. **Assign** – sends a `PATCH /repos/{owner}/{repo}/issues/{number}` request
   for each unassigned issue.  Already-assigned issues are skipped (idempotent
   guard).
4. **Summary** – prints the number of assigned / skipped issues.

---

## Required permissions

The workflow uses the built-in `GITHUB_TOKEN` with:

```yaml
permissions:
  issues: write   # update issue milestones
  contents: read  # check out the repository
```

No Personal Access Token (PAT) or additional secrets are required.

---

## Running locally

```bash
export GITHUB_TOKEN="ghp_…"          # token with repo / issues:write scope
export GITHUB_REPOSITORY="makr-code/ThemisDB"
export MILESTONE_INPUT="3"            # number or title
export DRY_RUN="true"                 # optional, preview only

python .github/scripts/assign_milestone.py
```

---

## Idempotency

Re-running the workflow after all issues have been assigned is safe.  The API
query uses `milestone=none`, so already-assigned issues are never returned and
never modified.
