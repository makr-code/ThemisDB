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
   d. Applies the label **`copilot/delegated`** to the issue.
   e. Removes the label **`queue/copilot`** from the issue.
4. Copilot picks up the mention, implements the changes, and opens a PR that
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

1. Open or find an existing issue.
2. Add the label **`queue/copilot`**.
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
  the delegation comment.
- GitHub Copilot Coding Agent must be **enabled** on the repository or
  organisation for the `@copilot` mention to trigger automated work.
- Auto-merge is **not** implemented.  Human review and merge are fully
  independent of the dispatcher.

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
