# Release Publishing Approval Governance

> Status: Active (2026)
> Purpose: Centralized reference for all release publishing approval workflows, keywords, and tracking

This document is the single source of truth for:
- All approval gate workflows and their triggers
- Maintainer permission requirements
- Issue tracking and labeling conventions
- Tracking issue templates and lifecycle
- Approval keyword registry
- Implementation details for automation

**Related Documents:**
- [RELEASE_STRATEGY.md](../../RELEASE_STRATEGY.md) — Release process overview
- [BRANCHING_STRATEGY.md](../../BRANCHING_STRATEGY.md) — Branch governance
- [VERSIONING.md](../../VERSIONING.md) — Version numbering
- [ACTION_PIN_POLICY.md](../.github/ACTION_PIN_POLICY.md) — Workflow action pinning

## Approval Gate Workflows

### 1. GitHub Release Publication
**Workflow:** `.github/workflows/release-mainline-approval.yml`

| Property | Value |
|----------|-------|
| Trigger Type | Issue comment |
| Trigger Labels | `release-review` |
| Trigger Keywords | `/publish-release`, `/approve-release`, `@publish-release` |
| Requires Permission | `admin` or `maintain` |
| Action | Publishes draft release to GitHub + creates tracking issues for downstream registries |
| Downstream | Creates 4 tracking issues for independent approval of each registry (WinGet, Docker, Linux Distro, Windows Distro) |
| Auto-close Issue | No - issue stays open as central release coordination point |

**Workflow Steps:**
1. Validate maintainer permission via GitHub API
2. Extract version from issue body (pattern: `## Release Version: X.Y.Z`)
3. Verify release tag exists
4. Publish release (calls `gh release edit <tag> --draft=false`)
5. Comment approval status with next-steps
6. Close tracking issue with success status

**Tracking Issue Template:**
```markdown
## Release Version: v2.5.0

| Item | Value |
|------|-------|
| Build Artifacts | [Release Link](https://github.com/.../releases/tag/v2.5.0) |
| Checksums | [SHA256SUMS.txt](...) |
| Signing Certificates | Valid for 30 days |
| Changes | [#347, #348, #351] |
| Release Notes | docs/CHANGELOG.md |
| Quality Gate Status | ✅ All gates passing |
```

### 2. WinGet Community Package Submission
**Workflow:** `.github/workflows/release-winget-approval.yml`

| Property | Value |
|----------|-------|
| Trigger Type | Issue comment |
| Trigger Labels | `winget-release` |
| Trigger Keywords | `/publish-winget`, `/approve-winget`, `@publish-winget` |
| Requires Permission | `admin` or `maintain` |
| Action | Dispatches `release-winget.yml` to generate manifests and create PR |
| Target Repository | `microsoft/winget-pkgs` |
| Auto-close Issue | Yes, on success |
| External Review | Yes, community reviewers at microsoft/winget-pkgs |

**Workflow Steps:**
1. Validate maintainer permission
2. Extract version from issue
3. Verify corresponding GitHub Release exists
4. Dispatch `release-winget.yml` with version and fork owner
5. Maintainers approve WinGet publication via release-winget-approval.yml gate
6. Gate dispatches release-winget.yml to generate and submit WinGet manifest
6. PR created in fork (author: github-actions[bot])
7. Comment approval status
8. Close tracking issue

**Workflow Dispatch Only:** `release-winget.yml` is triggered exclusively by the `release-winget-approval.yml` gate (via `workflow_call`). It does not auto-trigger on release events.
- Downloads release assets (ZIP/MSI)
- Generates checksums
- Creates WinGet manifest files (3-part YAML)
- Validates manifests with `winget validate`
- Creates PR to upstream `microsoft/winget-pkgs`
- Uploads artifacts for traceability

**Tracking Issue Template:**
```markdown
## WinGet Release: v2.5.0

| Item | Value |
|------|-------|
| Release Link | v2.5.0 |
| Windows Installers | themisdb-2.5.0-community-binary-x64.zip, ...x86.zip |
| Manifest Version | 2.5.0 |
| Manifest Location | packaging/winget/manifests/t/ThemisDB/ThemisDB/2.5.0/ |
| Target PR | [microsoft/winget-pkgs#XXXXX](https://github.com/microsoft/winget-pkgs/pull/XXXXX) |
| Package ID | `ThemisDB.ThemisDB` |
| End-User Install | `winget install ThemisDB.ThemisDB` |

### Next Steps
After approval, community reviewers will validate and merge the PR to microsoft/winget-pkgs.
```

### 3. Docker Registry Publication
**Workflow:** `.github/workflows/release-docker-approval.yml`

| Property | Value |
|----------|-------|
| Trigger Type | Issue comment |
| Trigger Labels | `docker-release` |
| Trigger Keywords | `/publish-docker`, `/approve-docker`, `@publish-docker` |
| Requires Permission | `admin` or `maintain` |
| Action | Dispatches `release-docker-image.yml` with `push_to_registry=true` |
| Target Registries | GHCR (primary), optional Docker Hub |
| Auto-close Issue | Yes, on success |
| Security Model | OIDC-based (no long-lived Docker Hub token) |

**Workflow Steps:**
1. Validate maintainer permission
2. Extract version from issue
3. Verify corresponding GitHub Release exists
4. Dispatch `release-docker-image.yml` with parameters:
   - `tag_name=v<version>`
   - `push_to_registry=true`
   - `registry=ghcr.io`
   - `include_tinyllama=true`
5. Maintainers approve Docker publication via release-docker-approval.yml gate
6. Gate dispatches release-docker-image.yml to build and push multi-arch images
6. Push to GHCR
7. Comment approval status with pull commands
8. Close tracking issue

**Workflow Dispatch Only:** `release-docker-image.yml` is triggered exclusively by the `release-docker-approval.yml` gate (via `workflow_call`). It does not auto-trigger on release events.
- Builds Dockerfile for each architecture (amd64, arm64)
- Attaches build provenance attestation (SLSA Level 2)
- Pushes images tagged as:
  - `ghcr.io/makr-code/themisdb:<version>`
  - `ghcr.io/makr-code/themisdb:2.5` (minor version)
  - `ghcr.io/makr-code/themisdb:latest` (stable only, not pre-release)
- Optional: push to docker.io (Docker Hub)

**Tracking Issue Template:**
```markdown
## Docker Release: v2.5.0

| Item | Value |
|------|-------|
| Release Link | v2.5.0 |
| Image Name | `themisdb/themisdb` |
| Registries | GHCR, Docker Hub |
| Stable Tags | v2.5.0, 2.5, latest |
| Pre-Release Tags | v2.5.0-rc1 (no `latest` tag) |
| Platforms | linux/amd64, linux/arm64 |
| Build Context | Dockerfile.unified (COMMUNITY edition) |
| Bundle | TinyLlama model included |
| Provenance | SLSA Level 2 attestation |

### Pull Commands
\`\`\`bash
docker pull ghcr.io/makr-code/themisdb:2.5.0
docker pull ghcr.io/makr-code/themisdb:latest
\`\`\`
```

### 4. Linux Distro Publication Approval
**Workflow:** `.github/workflows/release-linux-distro-approval.yml`

| Property | Value |
|----------|-------|
| Trigger Type | Issue comment |
| Trigger Labels | `linux-distro-review` |
| Trigger Keywords | `/publish-linux`, `/approve-linux`, `@publish-linux` |
| Requires Permission | `admin` or `maintain` |
| Action | Dispatches `release-linux-distribution.yml` with `publish_bundle=true` |
| Target Repositories | Linux distro endpoints (Debian/Ubuntu, RHEL/Fedora) |
| Auto-close Issue | Yes, on success |

**Workflow Steps:**
1. Validate maintainer permission
2. Extract version from issue
3. Verify corresponding GitHub Release exists
4. Dispatch `release-linux-distribution.yml` with version and publish flag
5. Comment approval status
6. Close tracking issue

**Workflow Dispatch Only:** `release-linux-distribution.yml` is triggered exclusively by the `release-linux-distro-approval.yml` gate (via `workflow_call`). It does not auto-trigger on release events.
- Detects release type (stable/testing/nightly)
- Downloads GitHub Release assets
- Generates DEB packages (Debian/Ubuntu)
- Generates RPM packages (RHEL/Fedora/Rocky/AlmaLinux)
- Generates TGZ source bundles
- Creates repository metadata (Release file, repomd.xml)
- Optional: Signs metadata with GPG
- Uploads bundle as artifact
- Publishes to distro endpoints if `publish_bundle=true`

**Tracking Issue Template:**
```markdown
## Linux Distro Bundle: v2.5.0 (stable)

| Item | Value |
|------|-------|
| Version | 2.5.0 |
| Channel | stable |
| Bundle | linux-distro-2.5.0-stable-12345 |
| Prepared At | 2026-09-23T10:15:00Z |
| Workflow | [#12345](https://github.com/.../actions/runs/12345) |

### Contents
- **DEB:** Debian 12+, Ubuntu 22.04+ (packages + Release metadata)
- **RPM:** RHEL 9+, Fedora 39+, Rocky, AlmaLinux (packages + repodata)
- **TGZ:** Universal Linux x86_64 source

### Next Steps
1. Download artifact: `linux-distro-2.5.0-stable-12345`
2. Verify checksums and signatures (if signed)
3. Test on target distributions
4. Publish to repository endpoints (manual or via bot)

### Repository Setup
For APT (Debian/Ubuntu):
\`\`\`bash
curl https://repo.themisdb.local/public.key | sudo apt-key add -
echo "deb [arch=amd64] https://repo.themisdb.local/debian bookworm main" | sudo tee /etc/apt/sources.list.d/themisdb.list
sudo apt update && sudo apt install themisdb
\`\`\`

For YUM (RHEL/Fedora):
\`\`\`bash
sudo rpm --import https://repo.themisdb.local/public.key
sudo yum-config-manager --add-repo https://repo.themisdb.local/rhel/9/stable
sudo yum install themisdb
\`\`\`
```

### 5. Windows Distro Publication Approval
**Workflow:** `.github/workflows/release-windows-distro-approval.yml`

| Property | Value |
|----------|-------|
| Trigger Type | Issue comment |
| Trigger Labels | `windows-distro-review` |
| Trigger Keywords | `/publish-windows`, `/approve-windows`, `@publish-windows` |
| Requires Permission | `admin` or `maintain` |
| Action | Dispatches `release-windows-distribution.yml` with `publish_bundle=true` |
| Target Repositories | Windows distro endpoints (Scoop, Chocolatey) |
| Auto-close Issue | Yes, on success |

**Workflow Steps:**
1. Validate maintainer permission
2. Extract version from issue
3. Verify corresponding GitHub Release exists
4. Dispatch `release-windows-distribution.yml` with version and publish flag
5. Comment approval status
6. Close tracking issue

**Workflow Dispatch Only:** `release-windows-distribution.yml` is triggered exclusively by the `release-windows-distro-approval.yml` gate (via `workflow_call`). It does not auto-trigger on release events.
- Detects release channel (stable/testing/nightly)
- Downloads GitHub Release assets
- Generates Scoop manifest candidates
- Generates Chocolatey manifest candidates
- Validates manifests with manifest validation tools
- Uploads bundle as artifact
- Publishes to distro endpoints if `publish_bundle=true`

**Tracking Issue Template:**
```markdown
## Windows Distro Bundle: v2.5.0 (stable)

| Item | Value |
|------|-------|
| Version | 2.5.0 |
| Channel | stable |
| Bundle | windows-distro-2.5.0-stable-12345 |
| Prepared At | 2026-09-23T10:15:00Z |
| Workflow | [#12345](https://github.com/.../actions/runs/12345) |

### Contents
- **Scoop Manifest Candidates** (requires community review)
- **Chocolatey Manifest Candidates** (requires community review)
- Installation scripts and metadata

### Next Steps
1. Download artifact: `windows-distro-2.5.0-stable-12345`
2. Review manifest candidates
3. Test installation on Windows
4. Submit to package managers (manual PR or script)

### Installation Commands (once published)

**Scoop:**
\`\`\`powershell
scoop bucket add themisdb https://github.com/makr-code/scoop-bucket
scoop install themisdb
\`\`\`

**Chocolatey:**
\`\`\`powershell
choco install themisdb
\`\`\`
```

## Approval Keywords Registry

### Global Keywords
These keywords work in any tracking issue with the appropriate label:

| Keyword | Used In | Workflow | Effect |
|---------|---------|----------|--------|
| `/publish-release` | `release-review` | `release-mainline-approval.yml` | Publish GitHub Release |
| `/approve-release` | `release-review` | `release-mainline-approval.yml` | Alias for `/publish-release` |
| `@publish-release` | `release-review` | `release-mainline-approval.yml` | Mention-based trigger |
| `/publish-docker` | `docker-release` | `release-docker-approval.yml` | Publish Docker images |
| `/approve-docker` | `docker-release` | `release-docker-approval.yml` | Alias for `/publish-docker` |
| `@publish-docker` | `docker-release` | `release-docker-approval.yml` | Mention-based trigger |
| `/publish-winget` | `winget-release` | `release-winget-approval.yml` | Submit to WinGet |
| `/approve-winget` | `winget-release` | `release-winget-approval.yml` | Alias for `/publish-winget` |
| `@publish-winget` | `winget-release` | `release-winget-approval.yml` | Mention-based trigger |
| `/publish` | `wiki-review` | `wiki-publish-from-issue.yml` | Publish wiki (different system) |

### Multi-Word Approval Context
If a comment contains multiple approval keywords, only the first recognized one is processed:

```
/publish-release and /publish-docker

→ Only /publish-release is processed
→ Follow up in a separate comment for /publish-docker
```

### Approval Comment Best Practices

**Recommended Format:**
```
/publish-release

**Verification Summary:**
- [x] Checksums verified
- [x] Signatures valid
- [x] Release notes complete
- [x] Changelog accurate
- [x] No known blockers

Ready to publish.
```

**Avoid:**
- Multiple approval keywords in one comment
- Approval comments on unrelated issues
- Comments without context or verification summary
- Approval by non-maintainers (will be rejected)

## Tracking Issue Lifecycle

### Creation
- **Trigger:** Upstream workflow completes (GitHub Release published, bundle prepared, etc.)
- **Automation:** GitHub Actions creates issue via `upsert_issue` pattern
- **Initial Labels:** Assigned based on workflow type
- **Initial State:** `open`

### Review
- **Duration:** 1-24 hours depending on release criticality
- **Maintainer Review:** Inspects metrics, checksums, signatures, release notes
- **Stakeholder Review:** Module leads or security team may add comments
- **Policy:** No blocking constraints, maintainer has final call

### Approval
- **Trigger:** Maintainer posts approval keyword in issue comment
- **Validation:** Approval workflow checks:
  - Commenter has `admin` or `maintain` permission
  - Issue has expected label
  - Required release artifacts/tag exist
- **Reaction:** GitHub adds +1 emoji to approval comment
- **Execution:** Approval workflow dispatches or executes publishing action

### Closure
- **Trigger:** Publishing action completes successfully
- **Action:** Approval workflow auto-closes issue with `state: closed` + `state_reason: completed`
- **Final Comment:** Summary of what was published with links to results
- **Artifact Retention:** Artifacts retained per standard retention policy (90 days)

### Failure Handling
- **If Publishing Fails:** Workflow comments with error and leaves issue open
- **Retry:** Maintainer investigates, fixes issue, posts approval keyword again
- **Escalation:** For critical failures, notify release lead via issue mention

## Permission Model

### Collaboration Permission Levels
All approval gates enforce GitHub's standard repository permission model:

```
admin (repository owner)
  ↑
maintain (release manager / maintainer)
  ↑
push (contributor)
  ↑
triage (issue triager)
  ↑
pull (read-only)
```

**Approval Permission Rule:** Workflows reject approval comments from users below `maintain` level.

### Granting Approval Permission
To grant a user approval permissions:

1. **Via GitHub UI:**
   - Repository Settings → Collaborators → Add collaborator
   - Select permission level: `Maintain` or `Admin`
   - User must accept invitation

2. **Via GitHub Team (Recommended):**
   - Organization Settings → Teams
   - Create or select team (e.g., `Release Managers`)
   - Add members with desired permissions
   - Grant team `Maintain` role on repository

3. **Verification:**
   ```bash
   gh api repos/makr-code/ThemisDB/collaborators/<username>/permission
   ```

## Implementation Checklist

### For Release Managers
- [ ] GitHub user account has `maintain` or `admin` permission on repository
- [ ] Familiar with release numbering (semantic versioning, edition prefixes)
- [ ] Can access release artifacts on GitHub (checksums, binaries, SBOM)
- [ ] Know how to verify GPG signatures: `gpg --verify <file>.asc <file>`
- [ ] Know how to verify SHA256 checksums: `sha256sum -c SHA256SUMS.txt`
- [ ] Understand risk level for release (major/minor/patch/pre-release)
- [ ] Have reviewed CHANGELOG.md and release notes
- [ ] Confirm no known critical issues for this release
- [ ] Post approval comment with verification summary
- [ ] Monitor approval workflow for completion (should take 5-15 min)

### For Automation Developers
- [ ] All approval workflows pass actionlint lint check
- [ ] Approval workflows check `admin`/`maintain` permission (not push)
- [ ] Approval keywords are clearly documented in workflow comments
- [ ] Tracking issues are created/updated atomically (upsert pattern)
- [ ] Approval comments include next-steps guidance
- [ ] Issue auto-closes on success with final summary comment
- [ ] Failures leave issue open with error details for retry
- [ ] All workflows log to GitHub Actions (visible in workflow UI)

### For Security Review
- [ ] OIDC-based auth used for public registries (no long-lived tokens)
- [ ] Approval keywords require maintainer permission (not push)
- [ ] Tracking issues provide audit trail (approver, timestamp, artifacts)
- [ ] No approval keywords in auto-generated comments (prevent re-triggering)
- [ ] Secrets limited to minimal scope per registry
- [ ] All public publications logged and traceable
- [ ] Emergency override procedure documented

## FAQ

**Q: Can I approve multiple releases with one comment?**
A: No. Each approval keyword triggers one action. Use separate comments for each approval.

**Q: What if I need to reject a release after it's approved?**
A: If publishing is in progress, contact release lead immediately. If already published, create follow-up patch release with fixes and publish separately.

**Q: Can a bot or workflow approve releases?**
A: No. Only users with `maintain` or `admin` permission can approve (manual sign-off requirement).

**Q: What if the GitHub Release was already published (draft=false)?**
A: Approval workflow will attempt to publish anyway (no-op if already published). This is safe and idempotent.

**Q: How do I know when a downstream workflow (WinGet, Docker) is ready to approve?**
A: GitHub Release publication triggers creation of separate tracking issues for each downstream workflow. Wait for those issues to appear, then post their corresponding approval keywords.

**Q: Can I publish just Docker without publishing to WinGet?**
A: Yes. Each approval gate is independent. Post `/publish-docker` alone without `/publish-winget`.

**Q: What's the difference between `/publish-release` and `/publish-docker`?**
A: `/publish-release` publishes the GitHub Release (makes it public). Each downstream registry (Docker, WinGet, Linux/Windows distro) has its own independent approval gate. No automatic triggering of downstream workflows; maintainers must explicitly approve each registry via its own gate.

**Q: Who should approve releases?**
A: Typically the release manager or project lead. Must have `maintain` or `admin` permission.

**Q: What if a maintainer account is compromised?**
A: Change password, revoke session tokens, and review recent approval comments. No published artifacts can be retracted; follow incident response procedures and document in release notes.

## Changelog

### 2026-09-23 (Initial Release)
- Added 3 approval gate workflows: release-mainline, release-winget, release-docker
- Added tracking jobs to release-linux-distro and release-windows-distro workflows
- Implemented permission model (admin/maintain only)
- Created tracking issue pattern with labels and templates
- Documented all approval keywords
- Added permission grant procedures
- Approved by: [Release Governance Committee]

