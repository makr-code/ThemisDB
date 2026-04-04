# License Compliance Implementation Summary

## Overview

This document summarizes the license compliance scanning implementation added to ThemisDB's CI/CD pipeline.

## Implementation Date

**Date**: 2026-02-02  
**Issue**: Lizenz Compliance Scanning zur CI/CD Pipeline hinzufügen  
**PR**: copilot/add-license-compliance-scanning

## Components Added

### 1. License Policy Configuration

**File**: `.license-policy.json`

Defines the project's license compliance policy:

- **13 allowed licenses**: MIT, Apache-2.0, BSD variants, ISC, Unlicense, MPL-2.0, etc.
- **9 blocked licenses**: GPL variants, AGPL variants (strong copyleft)
- **Weak copyleft handling**: LGPL, EPL (warnings, acceptable with dynamic linking)
- **Exception mechanism**: For special cases requiring manual approval

### 2. GitHub Actions Workflow

**File**: `.github/workflows/license-compliance.yml`

**Triggers**:
- Pull requests that modify dependency files
- Push to main/develop branches
- Monthly audit (1st of month, 3:00 AM UTC)
- Manual dispatch

**Features**:
- Scans multiple dependency ecosystems (vcpkg, npm, pip, go, maven, cargo, composer)
- Validates licenses against policy
- Blocks PRs on violations
- Posts detailed comments on PRs
- Generates audit reports as artifacts (90-day retention)
- Integration with GitHub Actions checks

**Outputs**:
- PR comments with compliance status
- Workflow summary with detailed report
- Artifact downloads for audit records

### 3. Documentation

#### German Documentation
**File**: `docs/de/compliance/license-compliance.md`

Complete guide including:
- Automated scanning process
- License categories (allowed/restricted/blocked)
- Developer workflow for handling violations
- Maintainer process for exceptions
- Monthly audit procedures
- FAQ section
- Compliance standards (BSI C5, NIS2, DSGVO, ISO 27001)

#### English Documentation
**File**: `docs/en/compliance/license-compliance.md`

English version of the compliance documentation.

#### CONTRIBUTING.md Integration
Updated `CONTRIBUTING.md` with:
- License compliance section
- Quick reference for developers
- Links to detailed documentation
- Added to table of contents

### 4. Integration with Existing Workflows

**Modified**: `.github/workflows/security-scan.yml`

Enhanced the existing license check to:
- Reference the new comprehensive workflow
- Show allowed licenses from policy
- Link to detailed compliance workflow

## Compliance Standards

The implementation meets requirements for:

- **BSI C5 (SSO-02)**: Software Supply Chain Security
- **NIS2**: Network and Information Security Directive
- **DSGVO/GDPR**: Data Protection Regulation
- **Executive Order 14028**: Improving the Nation's Cybersecurity (SBOM)
- **ISO 27001**: Information Security Management System

## Workflow Details

### Dependency Scanning

The workflow scans the following files:
- `vcpkg.json` - C++ dependencies
- `package.json` / `package-lock.json` - Node.js
- `pom.xml` - Java Maven
- `build.gradle` - Java Gradle
- `Cargo.toml` - Rust
- `go.mod` - Go
- `requirements.txt` / `Pipfile` - Python
- `composer.json` - PHP

### License Categories

#### Allowed (✅)
- MIT, MIT-0
- Apache-2.0
- BSD-2-Clause, BSD-3-Clause, 0BSD
- ISC
- Unlicense
- CC0-1.0
- BSL-1.0 (Boost)
- MPL-2.0 (Mozilla)
- Python-2.0
- Zlib

#### Warning - Weak Copyleft (⚠️)
- LGPL-2.1, LGPL-3.0
- EPL-1.0, EPL-2.0
- CDDL-1.0, CDDL-1.1

Action: Warning displayed, but PR not blocked. Acceptable for dynamic linking.

#### Blocked - Strong Copyleft (⛔)
- GPL-2.0, GPL-3.0
- AGPL-3.0

Action: PR blocked. Must replace dependency or request exception.

#### Blocked - Proprietary (⛔)
- Proprietary
- Commercial
- UNLICENSED

Action: PR blocked.

## Current Dependencies Status

All current ThemisDB dependencies comply with the policy:

| Package | License | Status |
|---------|---------|--------|
| openssl | Apache-2.0 | ✅ Allowed |
| rocksdb | Apache-2.0 / GPL-2.0 dual | ✅ Allowed (Apache-2.0) |
| gtest | BSD-3-Clause | ✅ Allowed |
| spdlog | MIT | ✅ Allowed |
| nlohmann-json | MIT | ✅ Allowed |
| c-ares | MIT | ✅ Allowed |
| zstd | BSD-3-Clause | ✅ Allowed |
| mimalloc | MIT | ✅ Allowed |
| libzip | BSD-3-Clause | ✅ Allowed |
| pugixml | MIT | ✅ Allowed |
| crc32c | BSD-3-Clause | ✅ Allowed |
| tl-expected | CC0-1.0 | ✅ Allowed |
| ffmpeg | LGPL-2.1 | ⚠️ Warning (dynamic linking OK) |

## Process Flow

### For Pull Requests

1. Developer creates PR with dependency changes
2. Workflow automatically triggered
3. All dependencies scanned for licenses
4. Licenses validated against policy
5. Results posted as PR comment
6. Status check updated (pass/fail)
7. If violations: PR blocked until resolved
8. If warnings: PR passes with notification

### For Monthly Audit

1. Workflow runs on 1st of month at 3:00 AM UTC
2. Full scan of all dependencies
3. Report generated and stored as artifact
4. Violations trigger notifications
5. Reports retained for 90 days

### For Violations

Developers have two options:

**Option A: Replace Dependency**
1. Find alternative with compatible license
2. Update dependency file
3. Push changes
4. Workflow re-runs automatically

**Option B: Request Exception**
1. Create issue with label `license-exception`
2. Provide justification and legal analysis
3. Wait for compliance team review
4. If approved: Exception added to `.license-policy.json`

## Testing

All components tested and validated:
- ✅ JSON policy file syntax valid
- ✅ YAML workflow syntax valid
- ✅ Policy parsing works correctly
- ✅ Documentation complete (German & English)
- ✅ CONTRIBUTING.md integration complete
- ✅ Current dependencies all compliant

## Future Enhancements

Possible improvements for future iterations:

1. **Enhanced Scanning**: Add more dependency ecosystems (Dart, Swift, etc.)
2. **Automated License Detection**: Use tools like Licensee or SPDX to auto-detect licenses
3. **SBOM Integration**: Tighter integration with SBOM generation workflow
4. **Dashboard**: Visual dashboard for license compliance status
5. **Notification System**: Email/Slack notifications for violations
6. **License Compatibility Matrix**: More nuanced compatibility checking

## Maintenance

### Updating the Policy

To update the license policy:

1. Edit `.license-policy.json`
2. Update policy version number
3. Document changes in PR description
4. Require maintainer approval
5. Update documentation if categories change

### Exception Management

Exceptions are tracked in `.license-policy.json` under `exceptions.list`:

```json
"exceptions": {
  "description": "Manually approved exceptions to policy",
  "list": []
}
```

Each exception should include:
- Package name
- License
- Justification
- Approval date
- Approver

## References

- [License Policy](.license-policy.json)
- [License Compliance Workflow](.github/workflows/license-compliance.yml)
- [German Documentation](docs/de/compliance/license-compliance.md)
- [English Documentation](docs/en/compliance/license-compliance.md)
- [CONTRIBUTING.md](CONTRIBUTING.md)
- [SECURITY.md](SECURITY.md)
- [SBOM Workflow](.github/workflows/sbom.yml)

## Compliance Checklist

- [x] License policy defined and documented
- [x] Automated scanning implemented
- [x] PR integration complete
- [x] Policy enforcement (blocking) configured
- [x] PR comments/annotations implemented
- [x] Documentation created (German & English)
- [x] Developer guidelines added to CONTRIBUTING.md
- [x] Monthly audit scheduled
- [x] Artifact retention configured
- [x] Integration with security scanning
- [x] Testing completed
- [x] Current dependencies validated

## Contact

For questions or issues with license compliance:

- **Issues**: Create issue with label `license-compliance`
- **Discussions**: Start discussion in "Compliance" section
- **Documentation**: See docs/de/compliance/ or docs/en/compliance/

---

**Status**: ✅ Implementation Complete  
**Version**: 1.0.0  
**Last Updated**: 2026-02-02
