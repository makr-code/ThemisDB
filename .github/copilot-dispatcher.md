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
`queue/copilot` but NOT `in-progress/copilot` and NOT `blocked`, in
creation-date order (oldest first).

---

## Triggers

| Event | Reason |
|---|---|
| `schedule` every 30 min | Baseline heartbeat |
| `workflow_dispatch` | Manual trigger |
| `pull_request` labeled / unlabeled / closed / synchronize | Capacity may have changed |
| `check_suite` completed | CI results arrived; readiness gate may fire |
| `workflow_run` completed | Alternative to check_suite for some CI setups |

---

## Important notes

- The dispatcher is **idempotent**: re-runs do not create duplicate PRs for
  the same issue (it checks for an existing open PR with
  `Closes #<issue-number>` in the body before creating a new one).
- Auto-merge is **not** implemented.  Human review and merge are fully
  independent of the dispatcher.
- All PRs target the `develop` branch.
