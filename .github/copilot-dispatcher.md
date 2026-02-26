# Copilot Issue Dispatcher

The Copilot Issue Dispatcher is a GitHub Actions automation that delegates
queued issues to the **GitHub Copilot Coding Agent** by posting a structured
comment on each issue.  Copilot then opens a pull request itself – the
dispatcher never creates branches or PRs directly.

---

## Prerequisites

- **GitHub Copilot Coding Agent** must be enabled for the repository or
  organisation.  Without it the `@copilot` mention in the delegation comment
  will have no effect.
- The workflow requires only `issues: write` permission (post comments, add/remove
  labels).  No `contents: write` or `pull-requests: write` permissions are needed.

---

## How it works

1. The dispatcher runs on a schedule (every 30 min) or via manual trigger.
2. It finds all open issues that have the label **`queue/copilot`** but NOT
   **`copilot/delegated`** and are not `blocked`.
3. For each qualifying issue (up to `max_delegations_per_run`) it:
   a. Sorts the eligible issues by **priority** (critical → high → medium → low →
      unlabelled), then by creation date within the same priority tier.
   b. Checks the issue's comment thread for the idempotency marker
      `<!-- copilot-delegated -->`.  If the marker is already present the
      issue is skipped (prevents double-delegation).
   c. Posts a **delegation comment** that mentions `@copilot` and includes:
      - A link to the issue.
      - Acceptance criteria extracted from the issue body (task-list checkboxes)
        or a generated default criterion.
      - Task notes: target branch, coding standards, test requirements,
        `Closes #<N>` instruction.
   d. **Assigns the `copilot` user** to the issue to start the Copilot Coding
      Agent.  The assignment is the authoritative trigger; a `@copilot` mention
      posted by a bot account alone is not sufficient to start the agent.
   e. Applies the label **`copilot/delegated`** to the issue.
   f. Removes the label **`queue/copilot`** from the issue.
4. Copilot picks up the assignment, implements the changes, and opens a PR that
   closes the issue.

---

## Delegation comment format

```markdown
<!-- copilot-delegated -->
@copilot Please implement the changes required to resolve this issue and open a pull request.

## Issue: [#<N> – <title>](<url>)

> <first non-heading line of the issue body, up to 300 characters>

## Requirements / Acceptance Criteria

- [ ] <checkbox items extracted from the issue body>
      (falls back to "Resolve issue #<N>: <title>" when the issue body has no checkboxes)

## Task Notes

- Target the `<base_branch>` branch for your pull request.
- Follow the repository's contribution guidelines and code standards.
- Ensure all existing tests pass and add new tests where appropriate.
- Reference this issue in the PR body with `Closes #<N>`.

---

*This delegation was posted automatically by the Copilot Issue Dispatcher.*
```

The HTML comment `<!-- copilot-delegated -->` acts as the **idempotency marker** –
the dispatcher will skip any issue whose comment thread already contains this string.

---

## Priority ordering

The dispatcher processes eligible issues in the following order within each run:

| Priority label | Aliases | Order |
|---|---|---|
| `priority:critical` | `P0` | 1st – processed first |
| `priority:high` | `P1` | 2nd |
| `priority:medium` | `P2` | 3rd |
| `priority:low` | `P3` | 4th |
| *(no priority label)* | — | Last |

Within the same priority tier issues are processed oldest-first (by creation date).

---

## Labels

### Issue labels

| Label | Purpose |
|---|---|
| `queue/copilot` | Mark an issue as eligible for Copilot delegation. Removed by the dispatcher after posting the delegation comment. |
| `copilot/delegated` | Set by the dispatcher after the delegation comment is posted. Prevents re-delegation on subsequent runs. |
| `blocked` | Issues with this label are skipped by the dispatcher. |

---

## How to queue an issue

### Automatic queuing (fully automated pipeline)

The **Auto-Queue Issues** workflow (`auto-queue-issues.yml`) makes the entire
pipeline hands-free:

- **New issues**: Every issue that is opened automatically receives the
  `queue/copilot` label (unless it is already `blocked`, `status:blocked`, or
  `copilot/delegated`).  No manual step is required.
- **Existing/back-fill**: Trigger the workflow manually via
  **Actions → Auto-Queue Issues for Copilot → Run workflow** to add
  `queue/copilot` to all currently open issues that are not yet queued.  A
  **dry-run** option lets you preview which issues would be queued before
  writing any labels.  A **max_issues** cap prevents accidental bulk-queuing.

The dispatcher then picks up every queued issue within ≤ 30 minutes.

### Manual queuing

1. Open or find an existing issue.
2. Add the label **`queue/copilot`** manually.
3. The dispatcher will pick it up within ≤ 30 minutes (or immediately on the
   next manual trigger).

---

## Configuration

Edit `.github/copilot-dispatcher.yml`:

```yaml
# Maximum issues delegated per dispatcher run.
max_delegations_per_run: 5

# Base branch Copilot should target when opening PRs.
base_branch: develop
```

### Manual override

When triggering the workflow manually via **Actions → Copilot Issue Dispatcher →
Run workflow**, you can override the per-run limit with the `max_delegations`
input (leave at `0` to use the configured default).

---

## Idempotency

The dispatcher is safe to re-run at any time:

- Issues that already have `copilot/delegated` are excluded by the GitHub
  search query.
- As an additional guard, the dispatcher reads the issue's comments and skips
  any issue whose thread already contains the delegation marker
  `<!-- copilot-delegated -->`.  This covers the edge case where the comment
  was posted but the label update failed in a previous run.

---

## Error handling

| Operation | Failure behaviour |
|---|---|
| Fetch queued issues | **Abort** the run |
| Read comments (idempotency check) | Warning + skip the issue |
| Post delegation comment | Warning + skip the issue |
| Assign `copilot` to the issue | Warning only (comment already posted) |
| Apply `copilot/delegated` label | Warning only (comment already posted) |
| Remove `queue/copilot` label | Warning only (404 = already absent, ignored) |

Critical failures (e.g., cannot query issues) abort the entire run immediately
with `core.setFailed()` and emit a budget hint so operators know a re-run costs
Actions minutes.

---

## Required labels

Run the label governance workflow once to ensure all required labels exist:

```
Actions → Label Governance – Setup & Audit → Run workflow → action: create
```

Or create the required labels manually:

| Label | Kind | Purpose |
|---|---|---|
| `queue/copilot` | Issue | Mark issue as eligible for Copilot delegation |
| `copilot/delegated` | Issue | Delegation comment posted; Copilot has been tasked |
| `blocked` | Issue | Skip this issue in the dispatcher |

---

## Important notes

- The dispatcher does **not** create branches, push commits, or open PRs.
  All of that is handled by the GitHub Copilot Coding Agent after receiving
  the issue assignment.
- GitHub Copilot Coding Agent must be **enabled** on the repository or
  organisation.  The dispatcher triggers the agent by **assigning the `copilot`
  user** to the issue; a `@copilot` mention in a bot-posted comment alone is
  not sufficient.
- Auto-merge is **not** implemented.  Human review and merge are fully
  independent of the dispatcher.

---

## Best practices for issue authoring

Writing issues that the dispatcher handles well improves the quality of
Copilot-generated pull requests.

### Use task-list checkboxes as acceptance criteria

The dispatcher extracts acceptance criteria from the issue body by scanning
for Markdown task-list items (`- [ ] …`).  When found, each item becomes a
checkbox in the delegation comment that Copilot uses as its work checklist.

**Good issue body excerpt:**

```markdown
## Acceptance Criteria

- [ ] Add unit tests for the new function covering edge cases.
- [ ] Update the public API documentation in `docs/api.md`.
- [ ] Ensure no regressions in the existing test suite.
```

When no task-list items are found the dispatcher falls back to a generic
`- [ ] Resolve issue #<N>: <title>` line.  Issues with explicit checkboxes
consistently produce more focused, correct pull requests.

### Include a concise description in the first paragraph

The dispatcher extracts the first non-heading, non-empty line of the issue
body (up to 300 characters) as a context excerpt for Copilot.  Front-load the
most important context:

```markdown
## Summary

Implement a connection-pool timeout so idle connections are evicted after
30 s (configurable) to prevent resource exhaustion under low-traffic loads.
```

### Label your issues accurately

| Label | Why it matters |
|---|---|
| `queue/copilot` | Queues the issue for dispatcher pick-up. |
| `priority:critical` / `priority:high` | Processed first; use for blockers. |
| `blocked` / `status:blocked` | Excludes the issue from dispatcher runs. |

### Keep issue scope tight

Copilot works best on issues that map to a single, bounded code change.
Split large epics into child issues before adding `queue/copilot`.

---

## Troubleshooting

### No PR appears after adding `queue/copilot`

1. **Copilot Coding Agent not enabled** – Visit *Repository Settings →
   Copilot → Coding agent* and confirm it is enabled.  Without this, no
   assignment will start an agent session.
2. **Dispatcher did not run** – Go to *Actions → Copilot Issue Dispatcher*
   and check for recent runs.  The schedule fires every 30 minutes; you can
   also trigger a run manually via *Run workflow*.
3. **Issue not picked up** – Open the run's job summary and look at the
   *"Queued issues available"* log line.  If the count is 0, verify the issue
   actually has the `queue/copilot` label and is open (GitHub's search index
   can lag by up to 60 seconds after a label is added; the next scheduled run
   will catch it).
4. **Delegation limit reached** – The `max_delegations_per_run` config cap
   (default 2) may have been exhausted by other issues in the same run.  Wait
   for the next run, increase the cap, or trigger a manual run.
5. **`copilot` was not assigned** – The dispatcher logs a warning if the
   `addAssignees` call fails.  Check the run log for lines like *"Could not
   assign 'copilot'"*.  The most common cause is that `issues: write`
   permission was removed or the token was revoked.
6. **Branch protection blocking Copilot** – If the `develop` branch requires
   status checks or reviews that the agent cannot satisfy, the agent's PR
   creation may fail silently.  Review branch protection rules at
   *Repository Settings → Branches* and consider adding a bypass for the
   Copilot app.

### Delegation comment posted but no PR

The delegation comment is informational; the **assignment** of the `copilot`
user is what starts the agent session.  Confirm both are present on the issue:
- A comment beginning with `<!-- copilot-delegated -->`.
- The `copilot` user listed under *Assignees*.

If the comment exists but the assignee is missing, trigger a manual dispatcher
run; the idempotency path will re-try the `addAssignees` call.

### Duplicate delegation comments

This should not happen under normal operation (the idempotency marker prevents
it), but if it does the extra comments are harmless – only the first assignment
triggers the agent.  Remove duplicate comments manually and consider opening a
bug report.

### Checking agent session status

Navigate to the issue page and look for the Copilot activity section, or visit:
*Repository → Pull requests* and filter by the branch Copilot creates
(format: `copilot/<issue-number>-<slug>`).

See also: [Tracking Copilot sessions](https://docs.github.com/en/copilot/how-tos/use-copilot-agents/coding-agent/track-copilot-sessions)

---

## References

- [GitHub Copilot coding agent overview](https://docs.github.com/en/copilot/how-tos/use-copilot-agents/coding-agent)
- [Troubleshooting the Copilot coding agent](https://docs.github.com/en/copilot/how-tos/use-copilot-agents/coding-agent/troubleshoot-coding-agent)
- [Tracking Copilot sessions](https://docs.github.com/en/copilot/how-tos/use-copilot-agents/coding-agent/track-copilot-sessions)
- [GitHub Actions permissions for `GITHUB_TOKEN`](https://docs.github.com/en/actions/writing-workflows/choosing-what-your-workflow-does/controlling-permissions-for-github_token)

---

## PR Copilot Trigger

After the Copilot Coding Agent opens a **draft PR** in response to a delegated
issue, the companion workflow **`pr-copilot-trigger.yml`** ensures the agent
receives a follow-up signal so it can continue the implementation.

### How it works

1. The workflow fires on every `pull_request` event (`opened`, `synchronize`,
   `reopened`).
2. It passes two gates before posting a comment:
   - **Gate 1 – Draft only**: the PR must be in draft state.
   - **Gate 2 – Label check**: the PR must carry the label `pr/copilot` **or**
     `agent:copilot-task`.
3. **Idempotency**: on `opened` / `reopened` events the workflow checks whether
   a comment with the marker `<!-- copilot-pr-trigger -->` already exists.  If
   it does, no duplicate comment is posted.  On `synchronize` events (new
   commits pushed) a fresh comment is always posted so the agent sees the
   latest CI context.
4. The trigger comment is built from:
   - PR number, title, URL, branch info.
   - An excerpt of the PR body (first 300 characters, headings stripped).
   - **CI scope** – which areas of the codebase were changed, as reported by
     the `ci-scope-classifier` reusable workflow.
   - **CI status** – current pass / fail / pending counts from GitHub check
     runs on the PR head SHA.
   - A *Next Steps* section instructing the agent to finish the implementation
     and convert the PR to ready-for-review when done.

### Trigger comment format

```markdown
<!-- copilot-pr-trigger -->
@copilot This draft PR was created for you. Please continue the implementation.

## PR Context

| Field | Value |
|---|---|
| **PR** | [#<N> – <title>](<url>) |
| **Branch** | `<head>` → `<base>` |
| **Event** | `opened` |
| **Draft** | yes |

> <PR body excerpt>

### CI Scope (changed areas)

- <list of changed areas from ci-scope-classifier>

### CI Status

- ✅ Passed/skipped: N
- ⏳ Pending: N (check-name, ...)
- ❌ Failed: N (check-name, ...)

## Next Steps

- Implement all items listed in the PR description / linked issue.
- Ensure all existing tests pass and add new tests where appropriate.
- Follow the repository's contribution guidelines and code standards.
- When implementation is complete, mark the PR ready for review
  (convert from draft) and request a review.

---

*This comment was posted automatically by the PR Copilot Trigger workflow.*
```

### Required labels

The workflow checks for these labels on the PR:

| Label | Purpose |
|---|---|
| `pr/copilot` | Marks a PR as Copilot-owned; triggers the workflow |
| `agent:copilot-task` | Alternative Copilot-task label; also triggers the workflow |

### Integration with Copilot Readiness Gate

Once the agent has finished and converts the PR from draft to ready-for-review,
the **`copilot-readiness-gate.yml`** workflow takes over: it verifies that all
CI checks pass and a Copilot review has been approved, then promotes the PR with
the `copilot/status-ready` label.
