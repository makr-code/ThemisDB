# License Compliance Process

## Overview

ThemisDB implements an automated license compliance process to ensure that all dependencies are compatible with the project license (MIT with Government Clause) and do not pose legal risks.

## Automated License Scanning

### GitHub Actions Workflow

The License Compliance workflow (`.github/workflows/license-compliance.yml`) runs automatically on:

- **Pull Requests**: Checks all dependency changes
- **Push to Main Branches**: Validates license compliance in main/develop branches
- **Monthly Audit**: Automatic check on the first day of each month
- **Manual Trigger**: Can be started anytime via GitHub Actions UI

### Monitored Files

The workflow monitors changes to:

- `vcpkg.json` - C++ dependencies (vcpkg)
- `package.json` / `package-lock.json` - Node.js/JavaScript dependencies
- `pom.xml` - Java Maven dependencies
- `build.gradle` - Java Gradle dependencies
- `Cargo.toml` - Rust dependencies
- `go.mod` - Go dependencies
- `requirements.txt` / `Pipfile` - Python dependencies
- `composer.json` - PHP dependencies
- `.license-policy.json` - License policy changes

## License Policy

The license policy is defined in `.license-policy.json` and categorizes licenses into three groups:

### 1. Allowed Licenses ✅

These licenses are permitted without restrictions:

- **MIT** - Maximum freedom, minimal restrictions
- **Apache-2.0** - Permissive with patent protection
- **BSD-2-Clause / BSD-3-Clause** - Permissive with attribution
- **ISC** - Functionally identical to MIT
- **Unlicense / 0BSD / CC0-1.0** - Public domain-like
- **BSL-1.0** - Boost Software License
- **MPL-2.0** - Mozilla Public License (file-based copyleft)

### 2. Restricted Licenses

#### Strong Copyleft (Blocked) ⛔

These licenses are **blocked** as they require derivative works to be published under the same license:

- **GPL-2.0 / GPL-3.0** - GNU General Public License
- **AGPL-3.0** - GNU Affero General Public License

**Action**: Pull request will be blocked, dependency must be replaced.

#### Weak Copyleft (Warning) ⚠️

These licenses are accepted with **warning** when using dynamic linking:

- **LGPL-2.1 / LGPL-3.0** - GNU Lesser General Public License
- **EPL-1.0 / EPL-2.0** - Eclipse Public License
- **CDDL-1.0 / CDDL-1.1** - Common Development and Distribution License

**Action**: Warning displayed, but pull request is not blocked.

**Example**: FFmpeg uses LGPL-2.1, which is acceptable as ThemisDB links FFmpeg as a dynamic library.

#### Proprietary Licenses (Blocked) ⛔

- **Proprietary / Commercial / UNLICENSED**

**Action**: Pull request will be blocked.

### 3. Unknown Licenses ❓

Licenses not defined in the policy require manual review.

**Action**: Warning displayed, manual review required.

## Workflow Integration

### Pull Request Checks

For each pull request, automatically:

1. **License Scan**: All dependencies are checked for their licenses
2. **Policy Validation**: Licenses are checked against the defined policy
3. **PR Comment**: Result is displayed as a comment in the PR
4. **Status Check**: PR status is set to "failed" on violations

### Example PR Comment

```markdown
## 📋 License Compliance Check

### ✅ Check Passed

All dependencies comply with the license policy.

**Summary:**
- 🔴 Violations: 0
- 🟡 Warnings: 1

[View detailed report in workflow summary](...)
```

### Blocking PRs on Violations

When a license violation is detected:

1. ❌ The PR check fails
2. 📝 A detailed report is added as a PR comment
3. 🚫 The PR cannot be merged
4. 📋 The workflow summary shows details of violations

## Process for License Violations

### For Developers

When the license check fails:

1. **Review the violation**:
   - Open the workflow run and check details
   - Identify the problematic dependency

2. **Actions**:
   
   **Option A: Replace dependency** (preferred)
   - Search for an alternative with compatible license
   - Update dependencies
   - Push changes

   **Option B: Request exception**
   - Create an issue with label `license-exception`
   - Justify why the dependency is necessary
   - Wait for review by compliance team

3. **Re-Check**:
   - Workflow runs automatically on every push
   - Verify that the violation has been resolved

### For Maintainers

For exception requests:

1. **Review**: Check justification and legal risks
2. **Decision**: Approve or deny
3. **Documentation**: On approval:
   - Add exception to `.license-policy.json` under `exceptions.list`
   - Document justification
   - Update compliance documentation

## License Audit

### Monthly Audit

On the first day of each month (3:00 AM UTC), a full license audit is automatically performed:

- Scans all dependencies
- Creates a detailed report
- Stores artifacts for 90 days
- Notifies on violations

### Manual Audit

Run a manual audit anytime:

```bash
# Via GitHub Actions UI
# 1. Go to Actions → License Compliance
# 2. Click "Run workflow"
# 3. Select branch
# 4. Click "Run workflow"
```

### Audit Reports

All audit reports are stored as artifacts:

- `license-report.txt` - Summary
- `vcpkg-licenses.json` - vcpkg dependencies
- `npm-licenses.json` - npm dependencies (if present)

**Retention**: 90 days

## Best Practices for Developers

### Before Adding New Dependencies

1. **Check license**: Verify the dependency's license
2. **Consult policy**: Ensure the license is allowed
3. **Documentation**: Add license information to your PR

### Preferred Licenses

Prefer dependencies with these licenses:

1. MIT
2. Apache-2.0
3. BSD-3-Clause
4. ISC

### Licenses to Avoid

Avoid dependencies with:

- GPL (all versions) - except for dynamic linking
- AGPL - always avoid
- Proprietary licenses
- Unclear or missing license information

## Integration with Other Compliance Processes

### SBOM (Software Bill of Materials)

The license compliance workflow works together with the SBOM workflow:

- SBOM contains license information for all dependencies
- Generated with every release
- See `.github/workflows/sbom.yml`

### Security Scanning

The security scan workflow (`.github/workflows/security-scan.yml`) includes a basic license check:

- Validates presence of license files
- References the detailed license compliance workflow
- Part of overall security strategy

### Code Review

During code reviews:

1. Check license compliance status
2. Manually review new dependencies
3. Ask questions if unclear
4. Document license decisions in PR

## Compliance Standards

This process meets the following standards and regulations:

- **BSI C5 (SSO-02)**: Software Supply Chain Security
- **NIS2**: Network and Information Security
- **GDPR**: General Data Protection Regulation
- **Executive Order 14028**: Improving the Nation's Cybersecurity (SBOM)
- **ISO 27001**: Information Security Management System

## Frequently Asked Questions (FAQ)

### Q: Why is my PR blocked?

**A**: The license check found a dependency with an incompatible license. Check the workflow report for details and either replace the dependency or request an exception.

### Q: What does "weak copyleft" mean?

**A**: Weak copyleft (e.g., LGPL) requires that changes to the library itself be published under the same license. However, with dynamic linking this is acceptable as your code is not considered a derivative work.

### Q: Can I use GPL libraries?

**A**: GPL libraries are only acceptable with dynamic linking and will generate a warning. Static linking or embedding GPL code is not allowed as it would make ThemisDB a derivative work under GPL.

### Q: How do I request an exception?

**A**: Create an issue with:
- Title: "License Exception Request: [Dependency Name]"
- Label: `license-exception`
- Description: Justification and legal analysis
- Wait for review by compliance team

### Q: How do I update the license policy?

**A**: Edit `.license-policy.json` and create a PR. Policy changes require detailed justification and review by maintainers.

## Contact and Support

For license compliance questions:

- **Issues**: Create an issue with label `license-compliance`
- **Discussions**: Start a discussion in the "Compliance" section
- **Email**: Contact the compliance team (see SUPPORT.md)

## Further Resources

- [License Policy (.license-policy.json)](../../../.license-policy.json)
- [License Compliance Workflow](../../../.github/workflows/license-compliance.yml)
- [Security Scanning Workflow](../../../.github/workflows/security-scan.yml)
- [SBOM Workflow](../../../.github/workflows/sbom.yml)
- [CONTRIBUTING.md](../../../CONTRIBUTING.md)
- [SECURITY.md](../../../SECURITY.md)

---

**Version**: 1.0.0  
**Last Updated**: 2026-04-06  
**Status**: ✅ Active
