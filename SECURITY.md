# Security Policy

## Supported Versions

ThemisDB is actively maintained. Security updates are provided for the following versions:

| Version | Supported          |
| ------- | ------------------ |
| 1.x     | :white_check_mark: |
| 0.9.x   | :white_check_mark: |
| < 0.9   | :x:                |

## Reporting a Vulnerability

We take security vulnerabilities seriously. If you discover a security vulnerability within ThemisDB, please follow these steps:

### Do NOT:
- Open a public GitHub issue for security vulnerabilities
- Discuss the vulnerability publicly before it's been addressed
- Exploit the vulnerability beyond what's necessary to demonstrate it

### Do:
1. **Report via GitHub Security Advisories** (Recommended):
   - Go to [Security Advisories](https://github.com/makr-code/ThemisDB/security/advisories/new)
   - Create a new private security advisory
   - Include:
     - A description of the vulnerability
     - Steps to reproduce the issue
     - Potential impact assessment
     - Any suggested fixes (optional)

2. **Use responsible disclosure** - Give us reasonable time to address the issue before any public disclosure

3. **Provide sufficient detail** so we can reproduce and verify the issue

### What to expect:

| Timeframe | Action |
|-----------|--------|
| **24 hours** | Acknowledgment of your report |
| **72 hours** | Initial assessment and severity classification |
| **7-14 days** | Detailed response with remediation plan |
| **30-90 days** | Fix released (depending on severity and complexity) |

## Security Measures

ThemisDB implements the following security measures:

### Authentication & Authorization
- Role-Based Access Control (RBAC) with 4-tier hierarchy
- mTLS (Mutual TLS) support for client authentication
- Token-based API authentication
- HashiCorp Vault integration for secrets management

### Encryption
- **Data-at-Rest:** AES-256-GCM encryption
- **Data-in-Transit:** TLS 1.3 (TLS 1.2 fallback)
- **Field-Level Encryption:** Schema-based selective encryption
- **Key Management:** HSM (PKCS#11), Vault, or Mock providers

### Input Validation
- JSON Schema validation
- AQL injection prevention
- Path traversal protection
- Request body size limits (10MB default)

### Rate Limiting & DoS Protection
- Token bucket algorithm (100 req/min default)
- Per-IP and per-user rate limiting
- Configurable thresholds

### Audit & Compliance
- 65+ security event types
- Encrypt-then-Sign audit logs
- Hash chain for tamper detection
- SIEM integration (Syslog RFC 5424, Splunk HEC)
- GDPR/DSGVO, eIDAS, SOC 2, HIPAA compliance ready

## Security Hardening

For production deployments, please follow our [Security Hardening Guide](docs/security/security_hardening.md):

1. Enable TLS with strong cipher suites
2. Configure RBAC with least-privilege principle
3. Enable audit logging with encryption
4. Use external key management (Vault/HSM)
5. Configure rate limiting appropriately
6. Set up monitoring and alerting
7. Regular security updates and patching

## Security Documentation

- [Security Overview](docs/security/security_overview.md)
- [TLS Setup Guide](docs/guides/guides_tls_setup.md)
- [RBAC Configuration](docs/security/security_implementation.md)
- [Encryption Strategy](docs/security/security_encryption_strategy.md)
- [Key Management](docs/security/security_key_management.md)
- [HSM Integration](docs/security/security_hsm.md)
- [Audit Logging](docs/features/features_audit_logging.md)
- [Threat Model](docs/security/security_threat_model.md)
- [Full Audit Checklist (BSI C5, ISO 27001, DSGVO)](docs/compliance/compliance_full_checklist.md)

## Vulnerability Disclosure Policy

We follow responsible disclosure practices:

1. **Acknowledgment**: Security researchers who responsibly disclose vulnerabilities will be acknowledged in our security advisories (unless they prefer to remain anonymous)

2. **No Legal Action**: We will not take legal action against security researchers who:
   - Act in good faith
   - Follow this security policy
   - Do not access or modify other users' data
   - Do not disrupt our services

3. **CVE Coordination**: For significant vulnerabilities, we will coordinate CVE assignment with MITRE

## Security Scanning

We use the following tools for security scanning:

- **Gitleaks**: Secret detection in source code
- **clang-tidy**: Static analysis for C++ code
- **cppcheck**: Additional C++ security checks
- **Trivy**: Container image vulnerability scanning (CI/CD)
- **OWASP ZAP**: Dynamic application security testing (planned)

To run security scans locally:

```powershell
# Windows (PowerShell)
.\security-scan.ps1

# Linux/WSL (if the script exists in your environment)
./security-scan.ps1
# Or use the underlying tools directly:
# gitleaks detect --source . --verbose
# cppcheck --enable=warning,style --inconclusive ./src ./include
```

## Security Contact

For security-related issues, please use one of the following methods:

- **GitHub Security Advisories**: [Report a vulnerability](https://github.com/makr-code/ThemisDB/security/advisories/new) (Recommended)
- **GitHub Issues**: For non-sensitive security discussions
- **PGP Key**: Available upon request for encrypted communications
- **Response Time**: Within 24 hours for initial acknowledgment

## Changelog

- **2025-11**: Initial security policy publication
