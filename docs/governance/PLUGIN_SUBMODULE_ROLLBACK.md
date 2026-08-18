/// @file docs/governance/PLUGIN_SUBMODULE_ROLLBACK.md
/// @brief Wave C Batch 3: Plugin Submodule Rollback Procedure & Troubleshooting

# Plugin Submodule Rollback Procedure

Wave C Batch 3 implements fail-closed gates for private submodule scoping and credential leakage prevention.
This document provides rollback procedures when these gates fail and troubleshooting steps for common issues.

---

## 1. Scoped-Checkout Validation Failure

### Symptom
PR targeting `community` or `minimal` branch fails with:
```
FAIL-CLOSED: plugins/themisdb_* is a private plugin but does not have shallow=true in .gitmodules
```

### Root Cause
A private plugin submodule entry in `.gitmodules` is missing the `shallow = true` directive or the submodule is being initialized without the shallow flag during checkout.

### Resolution Procedure

#### Step 1: Identify the Failing Submodule
```bash
# View .gitmodules and find the offending entry
git show HEAD:.gitmodules | grep -A 5 "themisdb_ethic_ai\|themisdb_storage\|themisdb_importer"
```

#### Step 2: Verify Shallow Configuration
Look for entries like:
```ini
[submodule "plugins/themisdb_ethic_ai"]
    path = plugins/themisdb_ethic_ai
    url = https://github.com/makr-code/themisdb_ethic_ai.git
    shallow = true          # THIS MUST BE PRESENT
    branch = develop
    commit = ce401ad9d604012a2c02655e79f5c17f57a9f82d
```

#### Step 3: Add Shallow Flag
If the `shallow = true` line is missing:

```bash
# Option A: Edit .gitmodules directly
git config -f .gitmodules submodule.plugins/themisdb_ethic_ai.shallow true

# Option B: Manual edit
vi .gitmodules
# Add: shallow = true
```

#### Step 4: Re-verify Commit Pin
Ensure the submodule has a known-good commit hash:
```bash
# Check existing commit
git config -f .gitmodules submodule.plugins/themisdb_ethic_ai.commit

# If missing, get the latest known-good commit from the private repo
# (requires access to the private repo)
git ls-remote https://github.com/makr-code/themisdb_ethic_ai.git refs/heads/develop
# Then add:
git config -f .gitmodules submodule.plugins/themisdb_ethic_ai.commit <COMMIT_HASH>
```

#### Step 5: Stage and Commit
```bash
git add .gitmodules
git commit -m "Wave C Batch 3: Fix private submodule shallow configuration for community builds"
```

#### Step 6: Re-run Scoped-Checkout Validation
Push the PR and verify the `Scoped Checkout Validation` gate passes.

---

## 2. Private-Credential Scanning Failure

### Symptom
PR fails with:
```
[CRITICAL] aws_secret_key detected at path/to/file:42
::error::FAIL-CLOSED: Critical credential patterns found in PR diff
```

### Root Cause
Code changes contain patterns matching private credentials:
- AWS Access Keys or Secret Keys
- Azure connection strings
- GCP API keys or service account keys
- GitHub tokens or OAuth tokens
- SSH/PGP private keys
- Private environment variable references

### Resolution Procedure

#### Option A: Remove Credentials (Recommended)

**Step 1: Identify All Matches**
```bash
# Re-run the scanner locally to find all occurrences
git diff origin/community...HEAD | grep -E "AKIA[0-9A-Z]{16}|aws_secret_access_key|github_token|ssh|BEGIN.*PRIVATE KEY"
```

**Step 2: Remove or Replace**
```bash
# Remove hardcoded credentials entirely
# DO NOT use encrypted files in the repository unless explicitly approved
# Use environment variables or secret management instead

# Example: Replace hardcoded AWS key
# BEFORE: AWS_SECRET_KEY="AKIA2XQRJ7NQKL3MOPQR"
# AFTER:  AWS_SECRET_KEY="${env:AWS_SECRET_KEY}"  # Load from environment
```

**Step 3: Commit the Fix**
```bash
git add .
git commit -m "Wave C Batch 3: Remove private credentials from PR diff"
```

#### Option B: False Positive Suppression (Rare Cases)

If the detected pattern is NOT actually a credential (e.g., documentation, example code):

**Step 1: Check Credential Scanner Configuration**
The scanner pattern list is defined in `.github/workflows/ci-pr-gates.yml` under the `private-credential-scan` job.

**Step 2: Request False Positive Allow-List**
Create a file `.github/scanner-allow-list.yml`:
```yaml
# Patterns to exclude from private-credential scanning
# Use ONLY for legitimate false positives with human approval

false_positives:
  - pattern: "AKIA[0-9A-Z]{16}"
    reason: "Example AWS key in documentation (docs/examples/)"
    approved_by: "@yourname"
    approved_date: "2026-08-18"
    justification: "This is a sanitized example in the README, not a real key"
```

**Step 3: Get Approval**
Require explicit review and approval from a maintainer before adding to allow-list.

#### Option C: Re-check Credentials

If the credential is real and you can't remove it:

1. **Rotate the credential immediately** at the source (AWS IAM, Azure, GCP, GitHub, etc.)
2. **Invalidate** the old credential
3. **Update** all systems using that credential with the new one
4. **Only then** can you proceed with code changes (after using the new credential)
5. **Document** the rotation in the commit message and PR body under "Security & Credentials Rotated"

---

## 3. SBOM Hash Verification Failure (Release Gates)

### Symptom
Release candidate (RC) tag fails with:
```
::error::SBOM hash mismatch — FAIL-CLOSED: dependency changed since generation
```

### Root Cause
Dependencies included in the SBOM have changed between SBOM generation and verification,
indicating a potential supply-chain integrity issue.

### Prevention Strategy

SBOM hashes are locked at **tag time** for release artifacts. If a dependency changes after tagging:

#### Do NOT Edit the SBOM After Tag

Once a release candidate tag is created with tag name like `v8.1.0-rc.1`,
the SBOM is locked and MUST match the dependencies at that moment.

#### If Dependencies Need to Change

1. **Delete the RC tag** (if it hasn't been announced or used):
   ```bash
   git tag -d v8.1.0-rc.1
   git push origin :refs/tags/v8.1.0-rc.1
   ```

2. **Make the dependency changes** on develop:
   ```bash
   # Update .gitmodules or vcpkg.json
   git add .gitmodules vcpkg.json
   git commit -m "Wave C Batch 3: Update dependencies for RC rebuild"
   ```

3. **Re-create the RC tag** with a new increment:
   ```bash
   git tag -a v8.1.0-rc.2 -m "Release Candidate 2 (dependency update)"
   git push origin v8.1.0-rc.2
   ```

#### If SBOM Verification Still Fails

**Step 1: Verify SBOM Content**
```bash
# Check the SBOM file generated at tag time
git show v8.1.0-rc.1:SBOM_RELEASE.json | jq '.components[].name'
```

**Step 2: Check Dependencies**
```bash
# Ensure .gitmodules at tag matches SBOM entries
git show v8.1.0-rc.1:.gitmodules | grep -E "path = |url = "

# Verify vcpkg baseline (if using vcpkg)
git show v8.1.0-rc.1:vcpkg-configuration.json
```

**Step 3: Recompute Expected Hash**
```bash
# At tag commit, recompute what the SBOM hash should be
git checkout v8.1.0-rc.1
# Regenerate SBOM locally
# Compare hash
```

---

## 4. Credential Scanner Pattern Reference

### Supported Credential Patterns

| Pattern Name | Description | Severity | Regex Example |
|---|---|---|---|
| `aws_access_key` | AWS Access Key ID | CRITICAL | `AKIA[0-9A-Z]{16}` |
| `aws_secret_key` | AWS Secret Access Key | CRITICAL | `aws_secret_access_key = "..."` |
| `azure_connection` | Azure connection string | CRITICAL | `DefaultEndpointsProtocol=https;...` |
| `gcp_api_key` | GCP API Key | HIGH | `AIza[0-9A-Za-z\-_]{35}` |
| `gcp_service_account` | GCP service account key | CRITICAL | `"type": "service_account"..."private_key"` |
| `github_token` | GitHub personal access token | CRITICAL | `ghp_[A-Za-z0-9_]{36}` |
| `github_oauth` | GitHub OAuth token | CRITICAL | `gho_[A-Za-z0-9_]{36}` |
| `ssh_private_key` | SSH RSA private key | CRITICAL | `-----BEGIN RSA PRIVATE KEY-----` |
| `openssh_private_key` | OpenSSH private key | CRITICAL | `-----BEGIN OPENSSH PRIVATE KEY-----` |
| `pgp_private_key` | PGP private key | CRITICAL | `-----BEGIN PGP PRIVATE KEY BLOCK-----` |
| `private_secret_env` | Private environment variable | CRITICAL | `secrets.*PRIVATE.*` |
| `db_password` | Database password | HIGH | `db_password = "..."` |

### All CRITICAL Patterns Fail the Gate

Any detection of a CRITICAL severity pattern will cause the `private-credential-scan` job to FAIL and block the PR.

---

## 5. Troubleshooting Common Issues

### Issue: "No known merge base found"

**Cause:** The base branch doesn't exist in local git history.

**Fix:**
```bash
# Ensure base branch is available
git fetch origin community  # or minimal, develop, etc.
git log origin/community --oneline | head -5  # Verify it's fetched

# Re-push the PR or manually retry the workflow
```

### Issue: "Scoped checkout validation always passes but checkout fails locally"

**Cause:** The `.gitmodules` configuration is correct, but the actual checkout command doesn't use shallow mode.

**Fix:**
```bash
# When manually checking out, use:
git clone --recurse-submodules --depth 1 <repo>

# Or explicitly disable private plugins:
git config submodule.plugins/themisdb_ethic_ai.update !true
git submodule update --init --depth 1
```

### Issue: "Credential scanner reports false positive for documentation example"

**Cause:** The documentation legitimately contains example credentials for illustration.

**Fix:**
```bash
# Ensure examples use obviously fake credentials:
# GOOD:   AKIA0000000000000000  (obviously fake format)
# GOOD:   ghp_0000000000000000000000000000000000  (zero-padded)
# BAD:    ******  (could be real)

# Update documentation to use sanitized examples
# Then add to allow-list with approval (see Option B above)
```

---

## 6. Prevention Checklist

Before committing code to community/minimal branches:

- [ ] No AWS keys, Azure keys, or GCP keys in code
- [ ] No GitHub/OAuth tokens or PATs in code
- [ ] No SSH or PGP private keys in repository
- [ ] No hardcoded database passwords
- [ ] No references to private environment variables (secrets.*PRIVATE*)
- [ ] All `.gitmodules` entries for private plugins have `shallow = true`
- [ ] All Wave-1 private plugin submodules have commit pins in `.gitmodules`
- [ ] PR body includes security rotation notice if any credentials were modified

---

## 7. Escalation Path

If a gate failure cannot be resolved by following this procedure:

1. **Consult RELEASE_STRATEGY.md §2.3** for gate policy
2. **Contact the security team** if credential leakage is suspected
3. **Request human waiver** via PR comments with justification
4. **Document the waiver** in the High Exception Record section of the PR body
5. **Schedule post-merge audit** to ensure compliance is restored

---

## References

- `.github/workflows/ci-pr-gates.yml` — Gate implementation
- `.github/workflows/governance-gates.yml` — Release gate (SBOM verification)
- `BRANCHING_STRATEGY.md` — Branch organization and protection
- `RELEASE_STRATEGY.md` — Release workflow and gates
- `DOCUMENTATION_GOVERNANCE.md` — Governance framework

