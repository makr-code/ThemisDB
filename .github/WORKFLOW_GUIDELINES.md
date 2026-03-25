# Workflow Guidelines

## Naming Conventions
- Use descriptive and clear names for branches and pull requests.
- Follow a consistent pattern such as `feature/description`, `bugfix/description`, or `hotfix/description`.

## Best Practices
- Write clean and maintainable code with adequate comments.
- Ensure that all new features are covered by tests.
- Maintain thorough documentation for all components.

## Security Guidelines
- Avoid hardcoding sensitive information (e.g., API keys) in your codebase.
- Regularly review dependencies for vulnerabilities and keep them updated.

## Troubleshooting
- When encountering issues, first check the existing logs for error messages.
- Use version control tools to revert to the last known good state if necessary.
- Document any resolved issues to aid future troubleshooting efforts.

## Reorganized Workflow Structure
- Ensure all team members are familiar with the new structure.
- Regularly gather feedback to improve workflow processes and guidelines.

---

## Manually Triggering Subdirectory Workflows

GitHub Actions only shows `workflow_dispatch` triggers in the **Web UI and
GitHub App** for workflow files located **directly** in `.github/workflows/`.
Workflows organised into subdirectories (e.g. `02-feature-modules/…`,
`03-editions/…`) are not listed in the manual-trigger UI.

The following options let you trigger any of the 216 subdirectory workflows
without moving files or restructuring the workflow hierarchy.

---

### Option 1 — Universal Dispatcher (GitHub UI)

A root-level dispatcher workflow is available in the Actions tab:

**`🚀 Workflow Dispatcher (Universal)`** → `workflow-dispatcher.yml`

| Field | Description | Example |
|-------|-------------|---------|
| `workflow` | Path relative to `.github/workflows/` | `03-editions/edition-community-ci.yml` |
| `ref` | Branch, tag, or SHA | `main` |
| `inputs_json` | Optional JSON inputs for the target workflow | `{"version":"1.9.0"}` |

Steps:
1. Go to **Actions → 🚀 Workflow Dispatcher (Universal) → Run workflow**
2. Enter the relative workflow path (e.g. `02-feature-modules/llm/cot-tracer-ci.yml`)
3. Set the target ref and any required inputs as JSON
4. Click **Run workflow**

---

### Option 2 — Dedicated Root-Level Triggers (GitHub UI)

For the most commonly used manual-only workflows, dedicated root-level trigger
files mirror the exact input fields of the real workflow:

| Root-level trigger | Real workflow |
|--------------------|---------------|
| `[Manual] Create Release Archive` | `04-release/create-release-archive.yml` |
| `[Manual] Bootstrap Release Branches` | `04-release/bootstrap-release-branches.yml` |
| `[Manual] Sync Roadmap Issues` | `08-maintenance/sync-roadmap-issues.yml` |
| `[Manual] Sync Milestones from Roadmap` | `08-maintenance/sync-milestones.yml` |
| `[Manual] Label Governance – Setup & Audit` | `08-maintenance/github_workflows_label-governance.yml` |

These wrappers appear in the **Actions** tab with the same input form as the
real workflow. They dispatch the real workflow via the GitHub CLI and exit.

---

### Option 3 — GitHub CLI (`gh workflow run`)

Trigger any workflow directly from your terminal without the UI:

```bash
# Trigger a workflow with no inputs
gh workflow run "03-editions/edition-community-ci.yml" \
  --repo makr-code/ThemisDB \
  --ref main

# Trigger a workflow with inputs (use --field for each input)
gh workflow run "04-release/create-release-archive.yml" \
  --repo makr-code/ThemisDB \
  --ref main \
  --field version=1.9.0

# Trigger a workflow on a specific branch with multiple inputs
gh workflow run "08-maintenance/sync-roadmap-issues.yml" \
  --repo makr-code/ThemisDB \
  --ref main \
  --field mode=preview \
  --field priority=all
```

Install the GitHub CLI: <https://cli.github.com/>
Authenticate: `gh auth login`

---

### Option 4 — GitHub REST API

Use the [Create a workflow dispatch event](https://docs.github.com/en/rest/actions/workflows#create-a-workflow-dispatch-event)
endpoint directly (useful for automation scripts):

```bash
# Trigger without inputs
curl -X POST \
  -H "Authorization: Bearer $GITHUB_TOKEN" \
  -H "Accept: application/vnd.github+json" \
  -H "X-GitHub-Api-Version: 2022-11-28" \
  "https://api.github.com/repos/makr-code/ThemisDB/actions/workflows/.github%2Fworkflows%2F03-editions%2Fedition-community-ci.yml/dispatches" \
  -d '{"ref":"main"}'

# Trigger with inputs
curl -X POST \
  -H "Authorization: Bearer $GITHUB_TOKEN" \
  -H "Accept: application/vnd.github+json" \
  -H "X-GitHub-Api-Version: 2022-11-28" \
  "https://api.github.com/repos/makr-code/ThemisDB/actions/workflows/.github%2Fworkflows%2F04-release%2Fcreate-release-archive.yml/dispatches" \
  -d '{"ref":"main","inputs":{"version":"1.9.0"}}'
```

> **Note:** The workflow file path in the URL must be percent-encoded.
> `/` → `%2F`

---

### Summary

| Option | Best for |
|--------|----------|
| **Universal Dispatcher** (UI) | Occasional one-off triggers without CLI |
| **Dedicated Wrappers** (UI) | Frequently used release/maintenance workflows |
| **GitHub CLI** | Developer workstations, scripting, CI pipelines |
| **REST API** | Programmatic automation, external CI orchestration |