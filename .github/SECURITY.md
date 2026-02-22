# Security Policy

## Vulnerability Disclosure Policy

### Contact Information

Report security vulnerabilities via **GitHub Security Advisories** (preferred):
[https://github.com/makr-code/ThemisDB/security/advisories/new](https://github.com/makr-code/ThemisDB/security/advisories/new)

**Email:** security@makr-code.com  
**GPG Key:** Available upon request (fingerprint provided on first contact)  
**Response Time:** 24–48 hours for Critical issues

> **Do NOT** open public GitHub issues for security vulnerabilities.

### Responsible Disclosure Timeline

| Stage | Timeframe | Description |
|-------|-----------|-------------|
| Acknowledgment | ≤ 24 hours | Confirm receipt of the report |
| Initial Assessment | ≤ 72 hours | Classify severity and scope |
| Remediation Plan | ≤ 14 days | Provide a fix timeline |
| Patch Release | 7–90 days | Depending on severity (see SLA below) |
| Public Disclosure | After patch | Coordinated with reporter |

### CVE Coordination

For significant vulnerabilities, we coordinate CVE assignment with MITRE. Reporters who wish to remain anonymous may request so during the disclosure process.

---

## Security Incident Response

### Security Severity Definitions

| Severity | Description | Examples |
|----------|-------------|---------|
| **Critical** | Remote code execution, auth bypass, data exfiltration without auth | RCE, unauthenticated admin access |
| **High** | Privilege escalation, significant data exposure, denial-of-service | SQL/AQL injection, SSRF, auth token leakage |
| **Medium** | Information disclosure, limited impact auth bypass | Verbose error messages, CSRF |
| **Low** | Minor information disclosure, low-impact issues | Path disclosure, non-sensitive metadata leaks |

### SLA for Fixes

| Severity | Patch SLA | Patch Release Cadence |
|----------|-----------|----------------------|
| Critical | 7 days | Emergency hotfix release |
| High | 30 days | Out-of-band patch release |
| Medium | 90 days | Next scheduled patch release |
| Low | Next major/minor | Bundled with regular release |

### Patch Release Cadence

- **Emergency hotfixes** (Critical): Released as soon as fix is validated.
- **Security patches** (High): Out-of-band releases within 30 days.
- **Maintenance patches** (Medium/Low): Bundled into regular monthly/quarterly releases.
- All security-related releases are tagged with a CVE reference where applicable.

---

## Supported Versions Matrix

| Version | Release Date | Support Ends | Status |
|---------|-------------|--------------|--------|
| **1.x.x** | 2025-Q4 | 2027-Q4 | ✅ Active |
| **2.0.0** | TBD | TBD | 🔮 Planned |
| **< 1.0** | 2024 | 2025-Q4 | ❌ End of Life |

Security updates are provided **only for Active supported versions**.

---

## Dependency Security

### vcpkg Lock-File Management

- All dependencies are pinned via `vcpkg.json` and `vcpkg-configuration.json`.
- Lock files must be committed and reviewed for every dependency update.
- Dependency upgrades require a dedicated PR with security justification.

### Known Vulnerable Dependencies Audit

- Run `trivy fs --scanners vuln .` or the comprehensive audit script to check for known CVEs.
- CVE audits are performed as part of the `security-hardening-ci` workflow.
- Known exceptions are documented in `.github/security-exceptions.md` (if present).

### SBOM (Software Bill of Materials) Generation

- SBOM is generated during release builds using `trivy sbom` or `syft`.
- Published as a release artifact alongside each tagged release.
- Format: CycloneDX JSON and SPDX.

---

## Code Security Practices

### Static Analysis

- **clang-tidy**: Enforced via `.clang-tidy` config; runs in CI on every PR.
- **cppcheck**: Runs with security-focused checks (`--enable=warning,style`).
- Configuration: `.cppcheck` and `.cppcheck-suppressions`.

### Address Sanitizer (ASan) in CI

- ASan builds run in CI (`security-hardening-ci` workflow) to detect memory errors.
- UBSan (Undefined Behavior Sanitizer) is also enabled for fuzz targets.
- Any new ASan failure blocks merge.

### Memory Safety Guidelines

- No raw pointer ownership — use `std::unique_ptr` / `std::shared_ptr`.
- All array access via bounds-checked containers.
- Buffer overflows caught via ASan + unit tests.
- See `docs/security/PRODUCTION_HARDENING_CHECKLIST.md` for full guidelines.

### Input Validation Requirements

- All user-supplied input validated against JSON schema before processing.
- AQL injection prevention is mandatory for all query paths.
- Path traversal protection required for all file-system operations.
- Request body size limited to 10 MB by default (configurable).

---

## Third-Party Security Review

### Review Process for External Dependencies

1. New dependency proposed in PR with justification.
2. License checked for copyleft conflicts (GPL, AGPL) — see License Compliance below.
3. Security audit via `trivy` or `osv-scanner` before merge.
4. Pin exact version in `vcpkg.json`; no floating versions.
5. Periodic re-audit via `security-hardening-ci` workflow.

### License Compliance (Copyleft Check)

- **Allowed:** MIT, Apache 2.0, BSD (2/3-clause), ISC, BSL-1.0.
- **Requires review:** LGPL, MPL-2.0, EPL-2.0.
- **Not allowed:** GPL-2.0, GPL-3.0, AGPL-3.0 (without explicit legal approval).
- License policy documented in `.license-policy.json`.

---

## Security Contact

| Method | Details |
|--------|---------|
| **GitHub Security Advisories** | [Report here](https://github.com/makr-code/ThemisDB/security/advisories/new) (preferred) |
| **Email** | security@makr-code.com |
| **GPG Key** | Fingerprint provided on first contact |
| **Response Time** | 24–48 hours for Critical; 72 hours for others |

---

*This security policy is reviewed and updated quarterly. Last review: 2026-Q1.*
