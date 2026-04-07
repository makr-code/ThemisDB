# GitHub Branch Protection Configuration Guide

This guide describes how to configure GitHub branch protection rules to enforce the ThemisDB branching strategy.

## Overview

ThemisDB uses a Git Flow branching strategy with two main branches:
- **`main`**: Production-ready release branch (fully protected)
- **`develop`**: Active development integration branch (protected)

See [BRANCHING_STRATEGY.md](BRANCHING_STRATEGY.md) for the complete branching strategy.

## Quick Setup

### Prerequisites

- Repository admin access
- GitHub repository settings access

### Steps

1. Navigate to your repository on GitHub
2. Go to **Settings** → **Branches**
3. Click **Add rule** for each branch below

## Branch Protection Rules

### 1. Protecting `main` Branch

**Branch name pattern:** `main`

#### Settings to Enable

**Protect matching branches:**
- ✅ **Require a pull request before merging**
  - Required approvals: **1**
  - ✅ Dismiss stale pull request approvals when new commits are pushed
  - ✅ Require review from Code Owners
  - ✅ Require approval of the most recent reviewable push

**Status checks:**
- ✅ **Require status checks to pass before merging**
  - ✅ Require branches to be up to date before merging
  - Add required status checks:
    - `CI / Build & Test (ubuntu-latest)`
    - `CI / Build & Test (windows-latest)`
    - `Code Quality / clang-tidy`
    - `Code Quality / cppcheck`
    - `Security / Gitleaks`
    - Any other critical CI checks

**Additional settings:**
- ✅ **Require conversation resolution before merging**
- ✅ **Require signed commits** (optional but recommended)
- ✅ **Require linear history** (optional - prevents merge commits if desired)
- ✅ **Include administrators** (recommended)
- ❌ **Allow force pushes** - Keep disabled
- ❌ **Allow deletions** - Keep disabled

**Restrict who can push to matching branches:**
- Select: **Restrict pushes that create matching branches**
- Add: Maintainers team only

### 2. Protecting `develop` Branch

**Branch name pattern:** `develop`

#### Settings to Enable

**Protect matching branches:**
- ✅ **Require a pull request before merging**
  - Required approvals: **1**
  - ✅ Dismiss stale pull request approvals when new commits are pushed
  - ⚠️ Require review from Code Owners (optional for develop)

**Status checks:**
- ✅ **Require status checks to pass before merging**
  - ✅ Require branches to be up to date before merging
  - Add required status checks:
    - `CI / Build & Test (ubuntu-latest)`
    - `Code Quality / clang-tidy`
    - `Code Quality / cppcheck`

**Additional settings:**
- ✅ **Require conversation resolution before merging**
- ⚠️ **Include administrators** (optional - more flexible than main)
- ❌ **Allow force pushes** - Keep disabled
- ❌ **Allow deletions** - Keep disabled

### 3. Protecting `release/*` Branches

**Branch name pattern:** `release/*`

#### Settings to Enable

**Protect matching branches:**
- ✅ **Require a pull request before merging**
  - Required approvals: **1**
  - ✅ Dismiss stale pull request approvals when new commits are pushed

**Status checks:**
- ✅ **Require status checks to pass before merging**
  - Add required status checks:
    - `CI / Build & Test (ubuntu-latest)`
    - `CI / Build & Test (windows-latest)`

**Additional settings:**
- ❌ **Allow force pushes** - Keep disabled
- ❌ **Allow deletions** - Can be enabled (releases are temporary)

## CODEOWNERS Configuration

Create or update `.github/CODEOWNERS` file to define code ownership:

```
# ThemisDB Code Owners
# These owners will be requested for review when someone opens a pull request.

# Global owners
* @makr-code/themisdb-maintainers

# Core database components
/src/storage/ @makr-code/storage-team
/src/query/ @makr-code/query-team
/src/transaction/ @makr-code/transaction-team

# LLM and AI features
/src/llm/ @makr-code/llm-team
/plugins/image_analysis/ @makr-code/ai-team

# Documentation
/docs/ @makr-code/docs-team
*.md @makr-code/docs-team

# Build and CI/CD
/.github/ @makr-code/devops-team
/scripts/ @makr-code/devops-team
CMakeLists.txt @makr-code/build-team

# Security-sensitive files
/security/ @makr-code/security-team
SECURITY.md @makr-code/security-team
```

## Rulesets (GitHub Enterprise/Advanced)

If your repository has access to GitHub Rulesets (available in GitHub Enterprise or with GitHub Team/Enterprise Cloud), you can create more advanced rules:

### Main Branch Ruleset

```yaml
name: "Main Branch Protection"
target: branch
enforcement: active

conditions:
  ref_name:
    include:
      - "refs/heads/main"

rules:
  - type: pull_request
    parameters:
      required_approving_review_count: 1
      dismiss_stale_reviews_on_push: true
      require_code_owner_review: true
      require_last_push_approval: true
      
  - type: required_status_checks
    parameters:
      strict_required_status_checks_policy: true
      required_status_checks:
        - context: "CI / Build & Test (ubuntu-latest)"
        - context: "CI / Build & Test (windows-latest)"
        - context: "Code Quality / clang-tidy"
        - context: "Code Quality / cppcheck"
        - context: "Security / Gitleaks"
        
  - type: non_fast_forward
    # Prevents force pushes
    
  - type: required_signatures
    # Requires signed commits (optional)
```

### Develop Branch Ruleset

```yaml
name: "Develop Branch Protection"
target: branch
enforcement: active

conditions:
  ref_name:
    include:
      - "refs/heads/develop"

rules:
  - type: pull_request
    parameters:
      required_approving_review_count: 1
      dismiss_stale_reviews_on_push: true
      
  - type: required_status_checks
    parameters:
      strict_required_status_checks_policy: true
      required_status_checks:
        - context: "CI / Build & Test (ubuntu-latest)"
        - context: "Code Quality / clang-tidy"
```

## Automated Configuration Script

You can use the GitHub API to configure branch protection programmatically:

```bash
#!/bin/bash
# configure-branch-protection.sh

REPO_OWNER="makr-code"
REPO_NAME="ThemisDB"
GITHUB_TOKEN="${GITHUB_TOKEN}"

# Protect main branch
curl -X PUT \
  -H "Authorization: token ${GITHUB_TOKEN}" \
  -H "Accept: application/vnd.github.v3+json" \
  "https://api.github.com/repos/${REPO_OWNER}/${REPO_NAME}/branches/main/protection" \
  -d '{
    "required_status_checks": {
      "strict": true,
      "contexts": [
        "CI / Build & Test (ubuntu-latest)",
        "CI / Build & Test (windows-latest)",
        "Code Quality / clang-tidy",
        "Code Quality / cppcheck",
        "Security / Gitleaks"
      ]
    },
    "enforce_admins": true,
    "required_pull_request_reviews": {
      "dismissal_restrictions": {},
      "dismiss_stale_reviews": true,
      "require_code_owner_reviews": true,
      "required_approving_review_count": 1
    },
    "restrictions": null,
    "allow_force_pushes": false,
    "allow_deletions": false
  }'

# Protect develop branch
curl -X PUT \
  -H "Authorization: token ${GITHUB_TOKEN}" \
  -H "Accept: application/vnd.github.v3+json" \
  "https://api.github.com/repos/${REPO_OWNER}/${REPO_NAME}/branches/develop/protection" \
  -d '{
    "required_status_checks": {
      "strict": true,
      "contexts": [
        "CI / Build & Test (ubuntu-latest)",
        "Code Quality / clang-tidy"
      ]
    },
    "enforce_admins": false,
    "required_pull_request_reviews": {
      "dismissal_restrictions": {},
      "dismiss_stale_reviews": false,
      "require_code_owner_reviews": false,
      "required_approving_review_count": 1
    },
    "restrictions": null,
    "allow_force_pushes": false,
    "allow_deletions": false
  }'

echo "Branch protection configured successfully!"
```

**Usage:**
```bash
export GITHUB_TOKEN="your_personal_access_token"
chmod +x configure-branch-protection.sh
./configure-branch-protection.sh
```

## Verification

After configuring branch protection, verify the settings:

### Test main Branch Protection

1. Try to push directly to main (should fail):
   ```bash
   git checkout main
   git commit --allow-empty -m "test"
   git push origin main
   # Expected: Error - protected branch
   ```

2. Create a PR from a feature branch to main (should require review)

3. Try to merge PR without status checks passing (should fail)

### Test develop Branch Protection

1. Try to push directly to develop (should fail or warn)

2. Create a PR from a feature branch to develop

3. Verify required checks must pass before merge

## Troubleshooting

### Problem: Can't enable branch protection

**Symptom**: Branch protection options are grayed out

**Solution**: 
- Verify you have admin access to the repository
- Check if organization policies restrict branch protection
- Contact organization admin

### Problem: Status checks not appearing

**Symptom**: Required status checks list is empty

**Solution**:
- Status checks only appear after they've run at least once
- Create a test PR to trigger workflows
- Wait for workflows to complete, then add them to required checks

### Problem: Accidentally locked out of branch

**Symptom**: Can't make critical fixes due to branch protection

**Solution**:
1. Create a hotfix branch from main
2. Make changes in hotfix branch
3. Create PR (even if urgent)
4. Use "bypass" permission if available (Enterprise only)
5. Or temporarily disable "Include administrators" setting

## Best Practices

1. **Start Strict**: Begin with strict protection and relax if needed

2. **Test First**: Test branch protection in a test repository first

3. **Document Exceptions**: Document any bypass or special permissions

4. **Regular Review**: Review and update protection rules quarterly

5. **Communicate**: Inform team members about protection rules before enabling

6. **Monitor**: Use GitHub insights to monitor branch protection compliance

## Additional Resources

- [GitHub Branch Protection Documentation](https://docs.github.com/en/repositories/configuring-branches-and-merges-in-your-repository/managing-protected-branches/about-protected-branches)
- [GitHub Rulesets Documentation](https://docs.github.com/en/repositories/configuring-branches-and-merges-in-your-repository/managing-rulesets/about-rulesets)
- [GitHub CODEOWNERS Documentation](https://docs.github.com/en/repositories/managing-your-repositorys-settings-and-features/customizing-your-repository/about-code-owners)

---

**Last Updated**: 2026-04-06  
**Version**: 1.0  
**Maintainer**: ThemisDB DevOps Team
