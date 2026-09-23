# Wiki Publish Review Tracking System

**Author:** ThemisDB Contributors
**Created:** 2026-09-23
**Last Updated:** 2026-09-23
**Status:** active

## Overview

This system implements a human-in-the-loop approval workflow for publishing documentation to the GitHub Wiki repository. It uses a single persistent tracking issue where each wiki build generates a comment with metrics and links, allowing maintainers to review and approve publication.

**Key principle:** No PR branches are created; the publish action is fully user-controlled via issue comments.

## Architecture

### Workflows

The system consists of three interdependent workflows:

#### 1. `wiki-pr-gate.yml` (Maintenance: Wiki PR Gate)

**Responsibility:** Build wiki, validate, and create tracking issue for human review.

**Triggers:**
- `workflow_run`: After "Maintenance: Docs" or "Maintenance: AI Working Cleanup - LLM Wiki" complete successfully
- `workflow_dispatch`: Manual trigger with optional parameters
- `schedule`: Daily at 03:00 UTC

**Key Steps:**
1. Checkout repository
2. Build wiki staging directory (306-806 pages)
3. Validate internal `wiki links`
4. Create or update single tracking issue with build metrics
5. Append a comment with run-level metrics and links

**Outputs:**
- Wiki staging directory (temp)
- Tracking issue number
- Build manifest (pages_written, source_class_totals, etc.)

**Permissions Required:**
- `contents: read`
- `issues: write`
- `pull-requests: read`

#### 2. `wiki-publish-from-issue.yml` (Wiki: Publish from Issue Approval)

**Responsibility:** Listen for maintainer approval on tracking issue and publish to wiki.

**Triggers:**
- `issue_comment: [created, edited]`: Any comment on any issue

**Approval Keywords:**
- `/publish`
- `/approve`
- `@publish-wiki`

**Key Steps:**
1. Verify commenter has maintainer permissions (admin/maintain)
2. React to comment with +1 emoji
3. Extract source branch from latest wiki-run comment
4. Rebuild wiki staging directory
5. Clone GitHub Wiki repository
6. Replace wiki content with staged content
7. Commit and push to wiki repository
8. Comment success on tracking issue
9. Close tracking issue

**Permissions Required:**
- `issues: write`
- `contents: write` (for wiki repository)

**Safety Features:**
- Permission check (maintainers only)
- Automatic rebuild before publishing (ensures freshness)
- No changes = no-op (prevents unnecessary commits)

#### 3. `publish-wiki.yml` (Publish: GitHub Wiki)

**Responsibility:** Build wiki and either publish directly or dispatch human review gate.

**Triggers:**
- `workflow_run`: After documentation-producing workflows
- `schedule`: Nightly at 01:00 UTC
- `workflow_dispatch`: Manual with options

**Manual Options:**
- `approve_and_publish`: true/false (publish directly or create tracking issue)
- `dry_run`: true/false (simulate without publishing)
- `enable_ai_enrichment`: true/false (add LLM overviews)
- `fail_on_broken_links`: true/false
- `fail_on_blocked_private`: true/false (workflow_dispatch only)

**Key Steps:**
1. Build wiki (includes glossary, breadcrumbs, currency sorting)
2. Validate wiki links
3. (Optional) AI enrichment of module pages
4. Either:
   - Publish directly (if `approve_and_publish=true`)
   - Dispatch wiki-pr-gate for human review
5. Close tracking issue (if published directly)

**Permissions Required:**
- `actions: write` (to dispatch other workflows)
- `contents: write` (to clone and push wiki)

### Tracking Issue Format

**Title:** `Wiki publish review: human sign-off tracker`

**Body:**
```markdown
## Wiki publish review tracker

This issue is the single human-in-the-loop sign-off tracker for automated
wiki publish runs. Each run appends a comment below with the current metrics.

**No action is needed on this issue unless you want to trigger or block a publish.**
Close this issue once the wiki state is accepted; it will be re-opened by the
next automation run that requires review.

- Automation behavior: **one issue, one comment per run**
- No PR branches are created; publish action is fully user-controlled.
```

**Comments (one per run):**
```markdown
## Wiki publish run — review required

A new wiki build has completed and is ready for human review.

| Metric | Value |
|--------|-------|
| Pages written | 806 |
| Private blocks | 0 |
| Primary sources | 289 |
| Secondary sources | 402 |
| Metadata sources | 7 |
| Source branch | `develop` |
| Trigger | `schedule` |
| Workflow run | https://github.com/makr-code/ThemisDB/actions/runs/... |
| Commit | `abc1234` |

**Next step:** Review the linked workflow run and confirm the wiki state matches
the intended branch. When satisfied, trigger the publish action manually.
```

## Usage Guide

### For Reviewers (Repository Maintainers)

#### Step 1: Create a Tracking Issue

The issue will be auto-created by wiki-pr-gate.yml, but you can manually trigger it:

1. Go to **Actions** → **Maintenance: Wiki PR Gate**
2. Click **Run workflow**
3. Choose branch (default: develop)
4. Click **Run workflow**

#### Step 2: Review Metrics

Once the workflow completes, a comment will appear on the tracking issue with:
- Number of pages generated
- Private content blocks (if any)
- Evidence sources breakdown
- Link validation results
- Links to workflow logs and source commit

#### Step 3: Approve and Publish

To publish the wiki, post a comment with one of these keywords:
- `/publish`
- `/approve`
- `@publish-wiki`

**Important:** Only users with admin or maintain permissions can publish.

#### Step 4: Monitor Publish Progress

The wiki-publish-from-issue workflow will execute. You can:
- Watch the workflow logs in **Actions**
- Check the tracking issue for success/failure comment
- The issue will be automatically closed on success

### For Developers (Implementing Wiki Changes)

#### Triggering a Wiki Build

1. Edit documentation files
2. Commit to `develop` branch
3. Push to GitHub
4. Wait for "Maintenance: Docs" workflow to complete
5. wiki-pr-gate.yml automatically creates tracking issue

#### Testing Wiki Changes Locally

```bash
# Build wiki staging
python scripts/build_wiki.py \
  --repo-root . \
  --output /tmp/wiki-staging \
  --enable-breadcrumbs \
  --sort-by currency \
  --manifest /tmp/wiki-manifest.json

# Validate links
python scripts/validate_wiki_links.py \
  --wiki-dir /tmp/wiki-staging \
  --report /tmp/wiki-report.txt \
  --fail-on-broken
```

## Governance

### Release Process

1. Documentation changes merge to `develop`
2. Automatic wiki build triggers
3. Human maintainer reviews metrics
4. Maintainer approves via `/publish` comment
5. Wiki publishes automatically
6. Tracking issue closes
7. Issue re-opens on next run

### Approval Requirements

- **Minimum:** One maintainer approval (via comment)
- **Permission:** admin or maintain role required
- **Review focus:** Metrics (pages, sources, links), not content (content already reviewed in PRs)

### Private Content Guardrails

Private content paths are blocked before publishing:
- No credentials exposed in wiki
- No private source paths included
- Community editions see only community content

### Manual Publish via Workflow Dispatch

For emergency publishes or scheduled maintenance, use `publish-wiki.yml` directly:

1. Go to **Actions** → **Publish: GitHub Wiki**
2. Click **Run workflow**
3. Set options (defaults recommend review via issue)
4. Click **Run workflow**

## Testing & Validation

### Metrics Captured Per Run

| Metric | Purpose |
|--------|---------|
| Pages written | Verify wiki completeness |
| Blocked private | Verify guardrails working |
| Source breakdown | Verify evidence quality |
| Link health | Verify navigation integrity |
| Breadcrumbs | Verify user experience feature |
| Currency sorting | Verify freshness prioritization |

### Link Validation

All `[[wiki links]]` are validated:
- ✅ No broken links (0 found in baseline)
- ✅ 4400+ links checked per run
- ✅ Fail-fast on critical breaks (configurable)

## Troubleshooting

### Issue: Workflow doesn't trigger automatically

**Check:** Did docs or ai_context changes commit to develop?
- wiki-pr-gate only triggers from specific workflow_run sources
- Manual workflow_dispatch always works

### Issue: Broken wiki links reported

**Solution:**
1. Fix broken `links` in source docs
2. Commit changes to develop
3. Trigger new wiki build
4. Rerun link validation

### Issue: Private content appears in staging

**Solution:**
1. Check file path against private-content guards
2. Move to plugin/ or appropriate location
3. Rebuild wiki
4. Verify blocked_private metric increases

### Issue: Cannot post `/publish` comment

**Check:** User permission level
- Requires admin or maintain role
- Check collaborators: Settings → Collaborators

## Implementation Details

### Architecture Principles

1. **Single Source of Truth:** One tracking issue per review cycle
2. **Separation of Concerns:** Gate (review) ≠ Publisher (execution)
3. **Immutable Triggers:** Comments are idempotent (can re-post safely)
4. **Fast Feedback:** Metrics available within seconds
5. **Audit Trail:** GitHub Actions logs + issue comments

### Why This Approach?

- **PR branches are not created:** Keeps CI/CD simpler, no merge conflicts
- **Comments for approval:** Familiar GitHub pattern (like `/deploy` in deploy bots)
- **One issue for all runs:** Easier to track state, prevents issue clutter
- **Automatic closure:** Prevents stale issues, supports repeated cycles

### Related Workflows

This tracking pattern is also used for:
- Release Docker images
- Publish to external registries (WinGet, etc.)
- Changelog generation and release notes

See `PUBLISH_WORKFLOW_AUDIT_2026-09-23.md` for full audit.

## References

- **Workflows:** `.github/workflows/wiki-pr-gate.yml`, `wiki-publish-from-issue.yml`, `publish-wiki.yml`
- **Scripts:** `scripts/build_wiki.py`, `scripts/validate_wiki_links.py`
- **Custom Action:** `.github/actions/status-flags-and-issues`
- **Audit:** `docs/governance/PUBLISH_WORKFLOW_AUDIT_2026-09-23.md`
- **Issue Types:** `RELEASE_GOVERNANCE.md`, `RELEASE_STRATEGY.md`

## Quick Links

- [Tracking Issue Template](#tracking-issue-format)
- [Manual Publish Instructions](#manual-publish-via-workflow-dispatch)
- [Troubleshooting Guide](#troubleshooting)

---

**Last Updated:** 2026-09-23  
**System Status:** ✅ Production Ready  
**Support:** Review workflow logs in GitHub Actions or check tracking issue comments
