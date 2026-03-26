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

## Workflow File Structure

All 233 workflow files are stored directly in `.github/workflows/` (flat
directory). Logical grouping is encoded in the filename using `_` as a
path separator (e.g. `02-feature-modules_llm_cot-tracer-ci.yml`).
See **Workflow File Naming Convention** below for the full convention.

---

## Workflow File Naming Convention

All workflow files live directly in `.github/workflows/` (flat structure).
GitHub Actions only shows `workflow_dispatch` triggers in the **Web UI** for
files located at that root level — nested subdirectories are not listed.

To preserve logical grouping while keeping all files discoverable, each
filename encodes its former directory hierarchy using `_` as a separator:

```
{category}_{subcategory}_{name}.yml
```

| Example filename | Former path |
|-----------------|-------------|
| `01-core_ci-scope-classifier.yml` | `01-core/ci-scope-classifier.yml` |
| `02-feature-modules_llm_cot-tracer-ci.yml` | `02-feature-modules/llm/cot-tracer-ci.yml` |
| `06-infrastructure_networking_wire-protocol-v2-ci.yml` | `06-infrastructure/networking/wire-protocol-v2-ci.yml` |

The numeric prefix (`01-` … `09-`) keeps related workflows sorted together in
the **Actions** tab. The categories are:

| Prefix | Category |
|--------|----------|
| `01-core` | Core build & CI infrastructure |
| `02-feature-modules` | Feature module CI workflows |
| `03-editions` | Edition-specific build & test |
| `04-release` | Release automation |
| `05-quality` | Build quality, security & validation |
| `06-infrastructure` | Distributed, networking, GPU, observability |
| `07-data-pipelines` | Data ingestion & pipeline CI |
| `08-maintenance` | Maintenance & tooling |
| `09-pr-gates` | Pull-request gate checks |

---

## Manually Triggering Workflows

All 233 workflow files are now at the root level and appear directly in the
GitHub Actions **Run workflow** UI (if they define a `workflow_dispatch`
trigger). You can also trigger them via the CLI or REST API:

---

### Option 1 — Universal Dispatcher (GitHub UI)

A root-level dispatcher workflow is available in the Actions tab:

**`🚀 Workflow Dispatcher (Universal)`** → `workflow-dispatcher.yml`

| Field | Description | Example |
|-------|-------------|---------|
| `workflow` | Filename in `.github/workflows/` | `03-editions_edition-community-ci.yml` |
| `ref` | Branch, tag, or SHA | `main` |
| `inputs_json` | Optional JSON inputs for the target workflow | `{"version":"1.9.0"}` |

Steps:
1. Go to **Actions → 🚀 Workflow Dispatcher (Universal) → Run workflow**
2. Enter the workflow filename (e.g. `02-feature-modules_llm_cot-tracer-ci.yml`)
3. Set the target ref and any required inputs as JSON
4. Click **Run workflow**

---

### Option 2 — Dedicated Root-Level Triggers (GitHub UI)

For the most commonly used manual-only workflows, dedicated root-level trigger
files mirror the exact input fields of the real workflow:

| Root-level trigger | Workflow file |
|--------------------|---------------|
| `[Manual] Create Release Archive` | `04-release_create-release-archive.yml` |
| `[Manual] Bootstrap Release Branches` | `04-release_bootstrap-release-branches.yml` |
| `[Manual] Build Binary Release · Linux` | `04-release_build-binary-linux.yml` |
| `[Manual] Build Binary Release · Windows` | `04-release_build-binary-windows.yml` |
| `[Manual] Publish Docker Image to Docker Hub` | `04-release_dockerhub-publish-on-release.yml` |
| `[Manual] Publish Enterprise Edition` | `04-release_publish-enterprise.yml` |
| `[Manual] Publish Hyperscaler Edition` | `04-release_publish-hyperscaler.yml` |
| `[Manual] Sync Roadmap Issues` | `08-maintenance_sync-roadmap-issues.yml` |
| `[Manual] Sync Milestones from Roadmap` | `08-maintenance_sync-milestones.yml` |
| `[Manual] Label Governance – Setup & Audit` | `08-maintenance_github_workflows_label-governance.yml` |
| `[Manual] Add Documentation Metadata` | `08-maintenance_add-doc-metadata.yml` |
| `[Manual] Code Maturity Analysis` | `08-maintenance_code-maturity-analysis.yml` |
| `[Manual] CI Scope Classifier (Debug)` | `01-core_ci-scope-classifier.yml` |

These wrappers appear in the **Actions** tab with the same input form as the
real workflow. They dispatch the real workflow via the GitHub CLI and exit.

---

### Option 3 — GitHub CLI (`gh workflow run`)

Trigger any workflow directly from your terminal without the UI:

```bash
# Trigger a workflow with no inputs
gh workflow run "03-editions_edition-community-ci.yml" \
  --repo makr-code/ThemisDB \
  --ref main

# Trigger a workflow with inputs (use --field for each input)
gh workflow run "04-release_create-release-archive.yml" \
  --repo makr-code/ThemisDB \
  --ref main \
  --field version=1.9.0

# Trigger a workflow on a specific branch with multiple inputs
gh workflow run "08-maintenance_sync-roadmap-issues.yml" \
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
  "https://api.github.com/repos/makr-code/ThemisDB/actions/workflows/.github%2Fworkflows%2F03-editions_edition-community-ci.yml/dispatches" \
  -d '{"ref":"main"}'

# Trigger with inputs
curl -X POST \
  -H "Authorization: Bearer $GITHUB_TOKEN" \
  -H "Accept: application/vnd.github+json" \
  -H "X-GitHub-Api-Version: 2022-11-28" \
  "https://api.github.com/repos/makr-code/ThemisDB/actions/workflows/.github%2Fworkflows%2F04-release_create-release-archive.yml/dispatches" \
  -d '{"ref":"main","inputs":{"version":"1.9.0"}}'
```

> **Note:** The workflow file path in the URL must be percent-encoded.
> `/` → `%2F`, `_` stays as `_`

---

### Summary

| Option | Best for |
|--------|----------|
| **Universal Dispatcher** (UI) | Occasional one-off triggers without CLI |
| **Dedicated Wrappers** (UI) | Frequently used release/maintenance workflows |
| **GitHub CLI** | Developer workstations, scripting, CI pipelines |
| **REST API** | Programmatic automation, external CI orchestration |