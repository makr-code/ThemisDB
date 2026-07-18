# EPIC 5518 PR Retarget

Automated workflow to retarget pull requests to the EPIC branch when they are marked as part of EPIC #5518 (Hybrid Retrieval Execution Boundaries).

## Purpose

This workflow implements policy [makr-code/ThemisDB#5545](https://github.com/makr-code/ThemisDB/issues/5545), which requires that sub-PRs of EPIC #5518 be automatically retargeted to the EPIC branch instead of merging directly into `develop`.

## Policy

When a PR implements a subtask of EPIC #5518, contributors should indicate this by **either**:

1. **Adding the marker in PR description** (recommended):
   ```
   Part of makr-code/ThemisDB#5518
   ```

2. **Adding the label `epic/5518`** (alternative):
   - Label: `epic/5518`
   - Description: "Part of EPIC #5518: Hybrid Retrieval Execution Boundaries"

When either marker is detected, the automation will automatically retarget the PR to `epic/hybrid-boundaries-5518`.

## Trigger Events

The workflow runs on the following `pull_request_target` events:

- `opened` — when a new PR is created
- `edited` — when PR body or title is edited
- `labeled` — when a label is added to the PR
- `unlabeled` — when a label is removed from the PR
- `synchronize` — when new commits are pushed to the PR

## Workflow Behavior

### Detection Phase

1. Checks if PR body contains "Part of makr-code/ThemisDB#5518" or "Part of #5518"
2. Checks if PR has the `epic/5518` label
3. Compares current base branch against `epic/hybrid-boundaries-5518`

### Retargeting Phase

If both conditions are met:

1. **Verifies** that `epic/hybrid-boundaries-5518` branch exists
2. **Updates** the PR base branch to `epic/hybrid-boundaries-5518`
3. **Posts** an informational comment on the PR
4. **Generates** a workflow summary with diagnostics

### Safety

- Workflow is **non-destructive**: only modifies base branch when explicit markers are present
- Workflow **warns** if epic/hybrid-boundaries-5518 branch doesn't exist
- Maintainers can **manually override** if needed by changing the base branch

## Requirements

### Branch

- The `epic/hybrid-boundaries-5518` branch must exist in the repository

### Label

- The `epic/5518` label must be created in the repository
- Can be created by running `.github/scripts/create-labels.py`

## Example Workflow

### Scenario 1: PR Body Marker

```yaml
Pull Request:
  Title: "Implement query optimization for EPIC 5518"
  Body: |
    ## Description
    This PR optimizes the query planner.

    Part of makr-code/ThemisDB#5518

  Current Base: develop
  Desired Base: epic/hybrid-boundaries-5518

Result: ✅ Automatically retargeted to epic/hybrid-boundaries-5518
```

### Scenario 2: Label

```yaml
Pull Request:
  Title: "Add artifact detection for EPIC 5518"
  Labels: [epic/5518, type:feature, priority:high]
  Current Base: develop
  Desired Base: epic/hybrid-boundaries-5518

Result: ✅ Automatically retargeted to epic/hybrid-boundaries-5518
```

### Scenario 3: Already on EPIC Branch

```yaml
Pull Request:
  Title: "EPIC 5518 follow-up work"
  Body: |
    Part of makr-code/ThemisDB#5518

  Current Base: epic/hybrid-boundaries-5518
  Desired Base: epic/hybrid-boundaries-5518

Result: ℹ️ No action needed (already on target branch)
```

### Scenario 4: Branch Doesn't Exist

```yaml
Pull Request:
  Title: "EPIC 5518 work (branch not ready)"
  Body: |
    Part of makr-code/ThemisDB#5518

  epic/hybrid-boundaries-5518: ❌ Does not exist

Result: ⚠️ Workflow warns but does not retarget
```

## Workflow Summary Output

The workflow generates a summary table showing:

| Field | Meaning |
|-------|---------|
| PR Number | Pull request number |
| Current Base | Current base branch |
| Has epic/5518 Label | Whether the `epic/5518` label is present |
| Body Has EPIC Reference | Whether "Part of makr-code/ThemisDB#5518" was found in body |
| Should Retarget | Whether retargeting criteria are met |
| Branch Exists | Whether `epic/hybrid-boundaries-5518` exists |
| Retargeted | Whether PR was successfully retargeted |

## Workflow File

- **Location**: `.github/workflows/08-maintenance_epic-5518-retarget.yml`
- **Classification**: Maintenance (lane 08)
- **Required Gate**: No (informational/automation only)
- **Permissions**: `pull-requests: write`, `contents: read`

## Support

For questions about this policy or workflow:

- **Policy**: See [makr-code/ThemisDB#5545](https://github.com/makr-code/ThemisDB/issues/5545)
- **EPIC**: See [makr-code/ThemisDB#5518](https://github.com/makr-code/ThemisDB/issues/5518)
- **Branch Strategy**: See [BRANCHING_STRATEGY.md](../../../BRANCHING_STRATEGY.md)
