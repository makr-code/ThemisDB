# Security Policy

This document describes the security policy, disclosure process, response SLAs, commit signing requirements, dependency management, contributor best practices, and compliance standards for ThemisDB.

> For the full security documentation, see the [repository SECURITY.md](../SECURITY.md).

---

## Supported Versions

| Version | Status  | Security Updates | Support Until |
|---------|:-------:|:----------------:|:-------------:|
| **2.x** | Current | ✅ Yes           | TBD           |
| **1.x** | EOL     | ❌ No            | 2025-12-31    |

Only the **current** version receives security patches. Users on EOL versions are strongly encouraged to upgrade.

---

## Reporting Security Vulnerabilities

**Do NOT open a public GitHub Issue for security vulnerabilities.**

### Preferred Method — GitHub Security Advisories (Private)

1. Navigate to [Security Advisories](https://github.com/makr-code/ThemisDB/security/advisories/new).
2. Open a **new private security advisory**.
3. Include:
   - A clear description of the vulnerability
   - Steps to reproduce
   - Potential impact assessment (CVSS score if known)
   - Suggested mitigations or patches (optional)

### Alternate Method — Encrypted E-Mail

For critical issues that require confidential communication before a GitHub advisory is created, contact the security team at:

```
security@themisdb.io
```

PGP key available upon request for encrypted communication.

---

## Security Response SLA

Response and patch timelines are based on **CVSS v3.1 Base Score**:

| Severity     | CVSS Range | Initial Response | Patch Released |
|--------------|:----------:|:----------------:|:--------------:|
| 🔴 Critical  | 9.0 – 10.0 | Within **24h**   | Within **7 days**  |
| 🟠 High      | 7.0 – 8.9  | Within **48h**   | Within **14 days** |
| 🟡 Medium    | 4.0 – 6.9  | Within **1 week**| Within **30 days** |
| 🟢 Low       | 0.1 – 3.9  | Within **2 weeks**| Within **60 days** |

Timeline starts from the moment the report is received and acknowledged.

---

## Commit Signing

### Requirements for Release PRs

All commits merged into `main` and `release/*` branches via a release PR **must be signed** with a GPG key. Unsigned commits will be rejected by branch protection rules.

### GPG Key Setup

```bash
# Generate a new GPG key (ED25519 recommended)
gpg --full-generate-key

# List keys to find your key ID
gpg --list-secret-keys --keyid-format=long

# Export public key to add to GitHub
gpg --armor --export <KEY_ID>

# Configure Git to sign commits automatically
git config --global user.signingkey <KEY_ID>
git config --global commit.gpgsign true
```

Add your exported public key to **GitHub Settings → SSH and GPG keys**.

### Verification in CI

The `validate-roadmap` and `security-hardening-ci` workflows verify commit signatures on release branches. A failed signature check blocks merge.

---

## Dependency Management

### SBOM (Software Bill of Materials)

ThemisDB generates and maintains a Software Bill of Materials for each release:

- **Format:** CycloneDX (JSON) and SPDX (tag-value)
- **Location:** `releases/<version>/sbom.json`
- **Generation:** Automated via `syft` during the release workflow

### Dependabot Integration

Dependabot is configured (`.github/dependabot.yml`) for:

| Ecosystem     | Update Schedule | Auto-merge Patch |
|---------------|:--------------:|:----------------:|
| GitHub Actions | Weekly         | ✅ Yes            |
| vcpkg / C++    | Weekly         | ❌ Review needed  |
| Docker         | Weekly         | ❌ Review needed  |
| Python (pip)   | Weekly         | ✅ Yes (dev-only) |

### Vulnerability Scanning

Automated scanning runs in CI for every pull request and on a weekly schedule:

| Tool       | Purpose                                | Schedule      |
|------------|----------------------------------------|---------------|
| **Trivy**  | Container image and filesystem CVEs    | Every PR + weekly |
| **Gitleaks** | Secret detection in source code      | Every commit  |
| **clang-tidy** | Static analysis (C++ memory safety) | Every PR      |
| **cppcheck**  | Additional C++ security checks       | Every PR      |

---

## Security Best Practices for Contributors

### Secrets Management

- ❌ Never commit API keys, tokens, passwords, or credentials.
- ✅ Use GitHub Secrets for CI credentials.
- ✅ Use HashiCorp Vault or environment variables for runtime secrets.
- Run `gitleaks detect --source . --verbose` before submitting a PR.

### Input Validation

- All user-supplied data entering the query pipeline must be validated against a JSON Schema or AQL grammar.
- Use bounded `std::string_view` parameters; avoid unbounded `char*` inputs.
- Path parameters must be canonicalized and checked against an allowlist before filesystem access.

### AQL Injection Prevention

- Never concatenate user input directly into AQL query strings.
- Use parameterized queries / bind variables for all dynamic values.
- Test injection variants in `fuzz/` targets against the AQL parser.

### Memory Safety (C++ Specific)

- Prefer RAII wrappers over raw `new`/`delete`.
- Use `std::unique_ptr` or `std::shared_ptr` for heap-allocated objects.
- Enable AddressSanitizer (`-fsanitize=address`) and UBSanitizer (`-fsanitize=undefined`) in debug/CI builds.
- Avoid `reinterpret_cast` across trust boundaries.

### Authentication & Authorization

- All new API endpoints must integrate with the existing RBAC middleware.
- mTLS is required for service-to-service communication in production.
- Token lifetimes must be configurable and default to ≤ 24 hours for user tokens.

---

## Compliance & Standards

### OWASP Top 10 Alignment

| OWASP Category           | ThemisDB Control                               |
|--------------------------|------------------------------------------------|
| A01 Broken Access Control | RBAC with 4-tier hierarchy, mTLS enforcement  |
| A02 Cryptographic Failures | AES-256-GCM, TLS 1.3, PFS                   |
| A03 Injection            | AQL parameterized queries, JSON Schema validation |
| A04 Insecure Design      | Threat model review per major release         |
| A05 Security Misconfiguration | Production mode gating, HSM stub opt-in  |
| A06 Vulnerable Components | Dependabot + Trivy scanning                  |
| A07 Auth Failures        | Token bucket rate limiting, session expiry    |
| A08 Software Integrity   | GPG commit signing, manifest SHA-256          |
| A09 Logging Failures     | 65+ audit event types, encrypt-then-sign logs |
| A10 SSRF                 | Outbound request allowlist, no internal proxying |

### CWE Coverage

Critical CWEs addressed by ThemisDB's secure coding standards:

- **CWE-89** (SQL/AQL Injection) — parameterized queries
- **CWE-119** (Buffer Overflow) — RAII, bounds checking, ASan
- **CWE-200** (Information Disclosure) — audit log redaction, PII scrubbing
- **CWE-284** (Improper Access Control) — RBAC enforcement
- **CWE-295** (Certificate Validation) — mTLS, certificate pinning
- **CWE-798** (Hardcoded Credentials) — Gitleaks scanning, Vault integration

### CVSS Scoring

All security advisories include a **CVSS v3.1** Base Score calculated using the [NIST NVD Calculator](https://nvd.nist.gov/vuln-metrics/cvss/v3-calculator). Severity ratings follow CVSS v3.1 guidelines.

---

## Security Changelog

| Date        | Event                                                          |
|-------------|----------------------------------------------------------------|
| **2026-02** | Security Policy v2 — SLA table, commit signing, SBOM tracking |
| **2026-01** | RocksDB wrapper: 7 critical CVEs fixed, Docker hardening       |
| **2025-12** | Update checker security, manifest signing design               |
| **2025-11** | Initial security policy published                              |
