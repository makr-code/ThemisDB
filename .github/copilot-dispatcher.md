# Copilot Issue Dispatcher

## Overview

The Copilot Issue Dispatcher is a GitHub Actions automation that keeps up to
**5** Copilot-working PRs active at a time.  When capacity is freed it
automatically pulls the next issues from a labeled queue and creates PRs for
them.

---

## Labels

### Issue labels

| Label | Purpose |
|---|---|
| `queue/copilot` | Mark an issue as eligible for automatic Copilot processing. |
| `in-progress/copilot` | Set by the dispatcher once it has claimed the issue. Do **not** remove manually unless you want the issue re-queued. |
| `blocked` | Issues with this label are skipped by the dispatcher. |

### PR labels

| Label | Purpose |
|---|---|
| `pr/copilot` | Applied by the dispatcher to every PR it creates. |
| `copilot-pr` | Applied by the dispatcher alongside `pr/copilot`; indicates this PR is created and managed by the Copilot dispatcher. |
| `copilot/status-working` | Copilot is actively working on this PR. Counts against the 5-slot WIP limit. |
| `copilot/status-ready-requested` | Copilot signals it is done. The readiness gate will promote this to `copilot/status-ready` once all CI checks pass **and** the required Copilot review is present. |
| `copilot/status-ready` | Copilot work is complete and all gates are green. This PR **no longer counts** against the WIP limit. Human review and merge remain fully independent. |
| `copilot/status-blocked` | Copilot cannot proceed. The PR stays in the WIP count until a human intervenes. |

---

## How to queue an issue

1. Open or find an existing issue.
2. Add the label **`queue/copilot`**.
3. The dispatcher will pick it up within ≤ 30 minutes (or immediately on the
   next PR/check-suite event that frees a slot).

---

## How Copilot signals "ready"

There are two paths:

### Path A – via label (recommended)
Copilot (or a human acting on its behalf) adds the label
`copilot/status-ready-requested` to the PR.

The readiness gate workflow then checks:
1. All required CI checks on the PR head commit are `success`.
2. A GitHub review with state `APPROVED` exists from one of the logins
   configured in `.github/copilot-dispatcher.yml` under
   `copilot_reviewer_logins`.

If both conditions are met the gate atomically:
- adds `copilot/status-ready`
- removes `copilot/status-working`
- removes `copilot/status-ready-requested`

### Path B – direct label
Copilot (or automation) adds `copilot/status-ready` directly.  The dispatcher
treats any open `pr/copilot` PR that already carries `copilot/status-ready` as
**free capacity** immediately.

---

## Configuring the Copilot reviewer identity

Edit `.github/copilot-dispatcher.yml`:

```yaml
copilot_reviewer_logins:
  - "copilot-pull-request-reviewer[bot]"   # GitHub Copilot PR review bot
  - "github-actions[bot]"                  # fallback / manual testing
```

If the list is **empty**, the review check is skipped (all CI-passing PRs with
`copilot/status-ready-requested` are promoted immediately).

---

## Configuring draft vs. ready PRs

By default the dispatcher creates PRs as **drafts** so they are not accidentally
merged before Copilot has finished working on them.  To change this, edit
`.github/copilot-dispatcher.yml`:

```yaml
# true  → PRs are opened as drafts (default)
# false → PRs are opened as regular (non-draft) PRs
draft: true
```

---

## Automatic assignees and reviewer

After a PR is successfully created the dispatcher automatically:

1. **Assigns `copilot`** (or the login configured via `copilot_assignee_login`) as
   an assignee on the PR so that GitHub routes the work to the Copilot agent.
2. **Assigns the issue author** as an additional assignee on the PR.
3. **Requests a review from the issue author** so they are notified and can
   approve or request changes once Copilot finishes.

Both the assignee and reviewer steps are non-fatal – if either API call fails
(e.g. the user does not have repository access) a warning is logged and the
dispatcher continues normally.

### Configuration

```yaml
# copilot_assignee_login: GitHub login assigned as Copilot worker on every PR.
# Set to "" to disable automatic Copilot assignment.
copilot_assignee_login: "copilot"

# add_issue_author_as_reviewer: when true (default) the issue author is added
# as both an assignee and a review requester on the PR.
add_issue_author_as_reviewer: true
```

### Prerequisites

| Requirement | Details |
|---|---|
| **`copilot_assignee_login` user** | Must be a repository collaborator (or organisation member) with at least **Write** access. For the `copilot` bot, GitHub Copilot must be enabled at the organisation or repository level. |
| **Issue author** | Must have at least **Read** access to the repository. GitHub silently drops reviewer requests for users with no repository access. |
| **Workflow permissions** | The workflow already requests `pull-requests: write`, which covers both `addAssignees` and `requestReviewers`. No additional permission changes are needed. |
| **`gh` CLI (manual use)** | If you need to assign/review manually: `gh pr edit <PR> --add-assignee copilot` and `gh pr edit <PR> --add-reviewer <login>` |

---

## What to do when a PR is stuck

1. Add the label **`copilot/status-blocked`** to the PR and leave a comment
   explaining what is wrong.
2. Investigate and fix the blocking issue (e.g., merge conflicts, failing
   checks, ambiguous requirements).
3. When ready to resume, remove `copilot/status-blocked` and add
   `copilot/status-working` again.

A blocked PR **still counts** against the WIP limit.  If you want to free the
slot entirely while keeping the PR open, close the PR (the dispatcher will then
skip it) or manually remove the `pr/copilot` label.

---

## Capacity calculation

```
active = open PRs with label pr/copilot AND NOT label copilot/status-ready
needed = wip_limit - active
```

The dispatcher selects up to `needed` open issues that carry
`queue/copilot` but NOT `in-progress/copilot` and NOT `blocked` / `status:blocked`,
in creation-date order (oldest first).

---

## Triggers

| Event | Reason |
|---|---|
| `schedule` every 30 min | Baseline heartbeat |
| `workflow_dispatch` | Manual trigger |
| `pull_request` labeled / unlabeled / closed / synchronize | Capacity may have changed |
| `check_suite` completed | CI results arrived; readiness gate may fire |

---

## Required labels

Run the label governance workflow once to ensure all required labels exist:

```
Actions → Label Governance – Setup & Audit → Run workflow → action: create
```

Or create the required labels manually:

| Label | Kind | Purpose |
|---|---|---|
| `queue/copilot` | Issue | Mark issue as eligible for Copilot processing |
| `in-progress/copilot` | Issue | Claimed by dispatcher; an open Copilot PR exists |
| `pr/copilot` | PR | PR was created by the dispatcher |
| `copilot-pr` | PR | PR was created and is managed by the Copilot dispatcher (alias/supplement to `pr/copilot`) |
| `copilot/status-working` | PR | Copilot is actively working; counts against WIP limit |
| `copilot/status-ready-requested` | PR | Copilot signals done; awaiting readiness gate promotion |
| `copilot/status-ready` | PR | All gates green; PR no longer counts against WIP limit |
| `copilot/status-blocked` | PR | Copilot cannot proceed; human intervention required |

---

## Diff check and dummy commit before PR creation

Before opening a pull request the dispatcher compares the newly-created issue
branch against the base branch using the GitHub Commits Compare API
(`repos.compareCommits`).

| Result | Action |
|---|---|
| Branch is **ahead** of base or has changed files | PR is created as usual. |
| Branch is **identical** to base (no diff) | A **dummy commit** is pushed to the branch automatically (file `.copilot_issue.txt` with issue metadata), making the branch non-empty. The dispatcher then proceeds to open the PR normally. |
| Compare API call fails | PR creation is **skipped** defensively; the `in-progress/copilot` label is removed so the issue stays available for re-dispatch. |

### Dummy commit

When the branch has no changes relative to the base the dispatcher creates a
lightweight placeholder file `.copilot_issue.txt` via `repos.createOrUpdateFileContents`.
The file contains a human-readable reference to the issue (number, title, URL,
branch name, creation timestamp) and a note that Copilot will replace it with
real changes.

Commit message format:

```
chore: init Copilot workspace for issue #<N>
```

This prevents the GitHub API validation error
`"No commits between <base> and <head>"` that would otherwise abort the
workflow with a non-zero exit code.  Copilot is expected to push substantive
changes on top of (or replacing) this placeholder commit while addressing the
issue.

**For developers:** if you push commits to an existing issue branch and re-run
the dispatcher manually, the diff check will detect the changes and skip the
dummy-commit step, creating the PR normally.

---

## Important notes

- The dispatcher is **idempotent**: re-runs do not create duplicate PRs for
  the same issue (it checks for an existing open PR with
  `Closes #<issue-number>` in the body before creating a new one).
- Auto-merge is **not** implemented.  Human review and merge are fully
  independent of the dispatcher.
- All PRs target the `develop` branch (configurable via `base_branch`).
