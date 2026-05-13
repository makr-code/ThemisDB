<div align="center">

# 🔐 Security Policy

**ThemisDB Security Team**

[![Security Score](https://img.shields.io/badge/security-A+-green)](https://github.com/makr-code/ThemisDB/security)
[![Gitleaks](https://img.shields.io/badge/Gitleaks-enabled-blue)](https://github.com/gitleaks/gitleaks)
[![Responsible Disclosure](https://img.shields.io/badge/disclosure-responsible-orange)](https://github.com/makr-code/ThemisDB/security/policy)

</div>

---

## 📋 Supported Versions

> [!IMPORTANT]
> ThemisDB is actively maintained. Security updates are provided for supported versions only.

| Version | Status | Security Updates | End of Life |
|---------|:------:|:----------------:|:-----------:|
| **1.x** | ✅ Active | ✅ Yes | TBD |
| **0.9.x** | ✅ Maintenance | ✅ Yes | 2026-12-31 |
| **< 0.9** | ❌ Unsupported | ❌ No | 2024-01-01 |

---

## 🚨 Reporting a Vulnerability

> We take security vulnerabilities **seriously**. If you discover a security issue, please follow our responsible disclosure process.

### ❌ Do NOT

> [!CAUTION]
> **Never do these things:**
> - ❌ Open a public GitHub issue for security vulnerabilities
> - ❌ Discuss the vulnerability publicly before it's addressed
> - ❌ Exploit the vulnerability beyond demonstration purposes

### ✅ Do

<details open>
<summary><b>1️⃣ Report via GitHub Security Advisories (Recommended)</b></summary>

1. Go to [Security Advisories](https://github.com/makr-code/ThemisDB/security/advisories/new)
2. Create a new **private** security advisory
3. Include:
   - 📝 **Description** of the vulnerability
   - 🔄 **Steps to reproduce** the issue
   - 💥 **Potential impact** assessment
   - 🛠️ **Suggested fixes** (optional)

</details>

<details>
<summary><b>2️⃣ Use Responsible Disclosure</b></summary>

- ⏳ Give us **reasonable time** to address the issue
- 🤐 No public disclosure before fix is released
- 🤝 Coordinate disclosure timeline with security team

</details>

<details>
<summary><b>3️⃣ Provide Sufficient Detail</b></summary>

Help us reproduce and verify the issue:
- 🖥️ Environment details (OS, version, configuration)
- 📊 Proof-of-concept (PoC) code or steps
- 📸 Screenshots or logs (if applicable)

</details>

### Response Timeline

| Timeframe | Action | Status |
|-----------|--------|:------:|
| **Within 24 hours** | Acknowledgment of your report | 📨 |
| **Within 72 hours** | Initial assessment & severity classification | 🔍 |
| **7-14 days** | Detailed response with remediation plan | 📋 |
| **30-90 days** | Fix released (depending on severity/complexity) | 🚀 |

> [!NOTE]
> **Critical vulnerabilities** are prioritized and may receive expedited fixes within **7 days**.

---

## 🧭 Governance and Escalation Ownership

To keep governance and community processes consistent across root-level documents:

- **Decision authority and role model:** [GOVERNANCE.md](GOVERNANCE.md)
- **Maintainer ownership and module responsibilities:** [MAINTAINERS.md](MAINTAINERS.md)
- **Contributor workflow and review path:** [CONTRIBUTING.md](CONTRIBUTING.md)
- **Community conduct and behavioral escalation:** [CODE_OF_CONDUCT.md](CODE_OF_CONDUCT.md)
- **Operational execution (hotfix/security/incident):** [SOP.md](SOP.md)

Canonical contact/escalation channels:

- **Security vulnerabilities (private):** [GitHub Security Advisories](https://github.com/makr-code/ThemisDB/security/advisories/new)
- **Non-sensitive security questions:** [GitHub Issues](https://github.com/makr-code/ThemisDB/issues)
- **General governance/process discussion:** [GitHub Discussions](https://github.com/makr-code/ThemisDB/discussions)

---

## 🛡️ Security Measures

ThemisDB implements **defense-in-depth** security across all layers:

<details open>
<summary><b>🔑 Authentication & Authorization</b></summary>

- ✅ **RBAC** (Role-Based Access Control) with 4-tier hierarchy
- ✅ **mTLS** (Mutual TLS) for client authentication
- ✅ **Token-based API** authentication
- ✅ **HashiCorp Vault** integration for secrets management

**Security Level:** ⭐⭐⭐⭐⭐

</details>

<details>
<summary><b>🌐 Network Protocol Security (v1.3.0+)</b></summary>

| Protocol | Security Features | TLS Version |
|----------|------------------|:-----------:|
| **HTTP/2** | Server Push, TLS 1.3 required | 1.3+ |
| **WebSocket** | WSS (WebSocket Secure) | 1.2+ |
| **MQTT** | TLS/mTLS support, auth required | 1.2+ |
| **PostgreSQL Wire** | SSL/TLS encryption, RBAC | 1.2+ |
| **MCP Server** | Transport security (stdio/SSE/WS) | 1.2+ |

> [!IMPORTANT]
> All protocols require **explicit opt-in** build switches for production readiness.

</details>

<details>
<summary><b>🔒 Encryption</b></summary>

**Data-at-Rest:**
- 🔐 **AES-256-GCM** encryption
- 🗄️ **Field-Level Encryption** (schema-based selective encryption)
- 🔑 **Key Management:** HSM (PKCS#11), Vault, or Mock providers

**Data-in-Transit:**
- 🌐 **TLS 1.3** (with TLS 1.2 fallback)
- 🔗 **Perfect Forward Secrecy** (PFS)
- 📜 **Certificate pinning** for HSM/TSA

**🛡️ Security Hardening (v1.4.2+):**

> [!WARNING]
> **NEW: HSM Stub Provider Gating**
> - ⚠️ Requires explicit opt-in via `THEMIS_ALLOW_HSM_STUB=1`
> - ❌ Fails in production mode (`THEMIS_PRODUCTION_MODE=1`)
> - 🔍 Auto-detects production environments (`ENVIRONMENT=production`)
> - 📋 See: [HSM Production Setup](docs/security/HSM_PRODUCTION_SETUP.md)

> [!NOTE]
> **VaultSigningProvider Limitation**
> - ✅ Signing operations only (Transit Engine)
> - ❌ Key management operations throw clear errors
> - 📖 Migration path: Use `VaultKeyProvider` for full key management
> - 📋 See: [Vault Signing Provider](docs/security/VAULT_SIGNING_PROVIDER.md)

> [!TIP]
> **PKCS#11 Integration Strategy**
> - 📁 Development: `pkcs11_minimal.h` (built-in, limited)
> - 🏭 Production: Vendor PKCS#11 headers (required)
> - ✅ Compile-time validation for header compatibility
> - 📋 See: [PKCS#11 Integration](docs/security/PKCS11_INTEGRATION.md)

</details>

<details>
<summary><b>✅ Input Validation</b></summary>

- 📋 **JSON Schema** validation
- 💉 **AQL injection** prevention
- 🚫 **Path traversal** protection
- 📦 **Request body size limits** (10MB default)
- 🔍 **BPMN/EPK/YAML parser hardening** (`src/process/`) — regex-based BPMN parser rejects malformed XML; EPK and VCC-VPB parsers validate schema before import

</details>

<details>
<summary><b>🚦 Rate Limiting & DoS Protection</b></summary>

- ⏱️ **Token bucket algorithm** (100 req/min default)
- 🌍 **Per-IP rate limiting**
- 👤 **Per-user rate limiting**
- ⚙️ **Configurable thresholds**

</details>

<details>
<summary><b>📊 Audit & Compliance</b></summary>

**Audit Logging:**
- 📝 **65+ security event types**
- 🔐 **Encrypt-then-Sign** audit logs
- 🔗 **Hash chain** for tamper detection
- 🔔 **SIEM integration** (Syslog RFC 5424, Splunk HEC)
- 🗂️ **Maintenance operations** (`src/maintenance/`) — all schedule CRUD and job lifecycle events logged via `AuditLogger`; RBAC roles `maintenance:read`, `maintenance:write`, `maintenance:admin`

**Compliance Ready:**
- ✅ GDPR/DSGVO
- ✅ eIDAS
- ✅ SOC 2
- ✅ HIPAA

</details>

---

## 🔒 Security Hardening

> [!IMPORTANT]
> For production deployments, start with the **[Security Posture Guide](docs/production/SECURITY_POSTURE.md)** — it explicitly lists every insecure default, why it is unsafe, and the exact environment variable to change it.
> Then follow the **[Production Runbook](docs/production/RUNBOOKS/CORE_MODULE_RUNBOOK.md)** for the full reference of required and optional environment variables, startup sequence, and failure-mode mitigations.

### Hardening Checklist

| Step | Action | Priority |
|:----:|--------|:--------:|
| 1️⃣ | Set `THEMIS_PRODUCTION_MODE=1` and `THEMIS_ENVIRONMENT=production` | 🔴 Critical |
| 2️⃣ | Set `THEMIS_TOKEN_ADMIN` to a strong random secret (≥32 bytes) | 🔴 Critical |
| 3️⃣ | Enable TLS with strong cipher suites | 🔴 Critical |
| 4️⃣ | Configure RBAC with least-privilege principle | 🔴 Critical |
| 5️⃣ | Use external key management: Vault or hardware HSM | 🟡 High |
| 6️⃣ | Enable audit logging with encryption and SIEM sink | 🟡 High |
| 7️⃣ | Enable WAL gRPC mTLS (`THEMIS_WAL_GRPC_ENABLE_MTLS=1`) | 🟡 High |
| 8️⃣ | Configure rate limiting appropriately | 🟢 Medium |
| 9️⃣ | Set up monitoring and alerting | 🟢 Medium |
| 🔟 | Regular security updates and patching | 🔴 Critical |

---

## 📚 Security Documentation

<details open>
<summary><b>⭐ Production Security (Start Here)</b></summary>

- 🛡️ **[Security Posture Guide](docs/production/SECURITY_POSTURE.md)** — dev vs. production defaults, hardening checklist, threat model, integrator checklist
- 📋 **[Production Runbook](docs/production/RUNBOOKS/CORE_MODULE_RUNBOOK.md)** — required env vars, startup sequence, failure modes, mitigations
- 🖥️ [systemd deployment files](deploy/systemd/) — hardened service unit, production drop-in, env template
- ☸️ [Kubernetes production Helm values](docs/production/examples/k8s_production_values.yaml) — TLS, secrets, probes, autoscaling

</details>

<details>
<summary><b>Core Security Guides</b></summary>

- 🔐 [TLS Setup Guide](docs/de/guides/guides_tls_setup.md)
- 👥 [RBAC Configuration](docs/security/api_authentication_authorization.md)
- 🔒 [Encryption Strategy](docs/security/encryption_strategy.md)
- 🔑 [Key Management](docs/security/ENCRYPTION_KEY_MANAGEMENT_POLICY.md)
- 🏦 [HSM Integration](docs/security/HSM_PRODUCTION_SETUP.md)
- ✅ [Production Hardening Checklist](docs/security/PRODUCTION_HARDENING_CHECKLIST.md)

</details>

<details>
<summary><b>Advanced Security Topics</b></summary>

- 📝 [Audit Logging](docs/de/features/features_audit_logging.md)
- ⚠️ [Threat Model](docs/de/security/security_threat_model.md)
- 🛡️ [Hardware Attack Vectors](docs/de/security/security_hardware_attack_vectors.md) - USB, PCIe, CPU, RAM, I/O threats
- ✅ [Full Audit Checklist (BSI C5, ISO 27001, DSGVO)](docs/de/compliance/compliance_full_checklist.md)

</details>

<details>
<summary><b>🆕 Knowledge Graph Protection (2026)</b></summary>

**Protection against AI data theft and knowledge graph exfiltration:**

- 🛡️ [Knowledge Graph Protection Guide (EN)](docs/en/security/knowledge_graph_protection.md)
- 🛡️ [Wissensgraphen-Schutz (DE)](docs/de/security/knowledge_graph_protection.md)
- 📊 [Impact Summary & Implementation Plan (DE)](docs/de/security/graph_protection_impact_summary.md)
- ⚙️ [Graph Protection Configuration Example](config/graph_protection.yaml)

**Topics covered:**
- Systematic graph exfiltration detection
- Vector embedding theft prevention
- Training data extraction protection
- Access pattern anomaly detection
- Graph watermarking & fingerprinting (planned)

</details>

---

## 🤝 Vulnerability Disclosure Policy

We follow **responsible disclosure practices**:

<details>
<summary><b>1️⃣ Acknowledgment</b></summary>

Security researchers who responsibly disclose vulnerabilities will be **acknowledged in our security advisories** (unless they prefer to remain anonymous).

</details>

<details>
<summary><b>2️⃣ No Legal Action</b></summary>

We will **not take legal action** against security researchers who:
- ✅ Act in good faith
- ✅ Follow this security policy
- ✅ Do not access or modify other users' data
- ✅ Do not disrupt our services

</details>

<details>
<summary><b>3️⃣ CVE Coordination</b></summary>

For significant vulnerabilities, we will coordinate **CVE assignment with MITRE**.

</details>

---

## 🔍 Security Scanning

> **Automated security scanning** is integrated into our CI/CD pipeline.

### Tools

| Tool | Purpose | Integration |
|------|---------|:-----------:|
| **Gitleaks** | Secret detection in source code | ✅ CI/CD |
| **clang-tidy** | Static analysis for C++ code | ✅ CI/CD |
| **cppcheck** | Additional C++ security checks | ✅ CI/CD |
| **Trivy** | Container image vulnerability scanning | ✅ CI/CD |
| **OWASP ZAP** | Dynamic application security testing | 🚧 Planned |
| **Comprehensive Audit** | Systematic security & compliance audit | ✅ Available |

### Run Scans Locally

<details>
<summary><b>Comprehensive Security Audit (Recommended)</b></summary>

Run a systematic security audit covering SAST, dependency scanning, secret detection, and more:

```bash
# Full audit (requires tools: cppcheck, clang-tidy, trivy, gitleaks, semgrep)
./scripts/comprehensive-code-audit.sh

# Quick audit (skip time-consuming checks)
AUDIT_QUICK=1 ./scripts/comprehensive-code-audit.sh

# Audit with specific categories
./scripts/comprehensive-code-audit.sh --skip-dependencies --skip-dynamic

# View all options
./scripts/comprehensive-code-audit.sh --help
```

**Audit Report:** Results are saved in `audit-results-<timestamp>/comprehensive-audit-report.md`

**Compliance Coverage:** BSI C5, ISO 27001, DSGVO, NIS2, OWASP ASVS, NIST CSF

</details>

<details>
<summary><b>Individual Security Tools</b></summary>

```bash
# Secret detection
gitleaks detect --source . --verbose

# Static analysis
cppcheck --enable=warning,style --inconclusive ./src ./include
clang-tidy src/**/*.cpp -- -std=c++20

# Dependency scanning
trivy fs --scanners vuln,secret,misconfig .

# Semgrep patterns
semgrep --config=auto src/ include/
```

</details>

---

## 📞 Security Contact

| Method | Purpose | Link |
|--------|---------|------|
| 🔒 **GitHub Security Advisories** | Report vulnerabilities (Recommended) | [Report](https://github.com/makr-code/ThemisDB/security/advisories/new) |
| 💬 **GitHub Issues** | Non-sensitive security discussions | [Issues](https://github.com/makr-code/ThemisDB/issues) |
| 🔑 **PGP Key** | Encrypted communications | Available upon request |

> [!NOTE]
> **Response Time:** Within 24 hours for initial acknowledgment.

---

## 📅 Changelog

| Date | Event |
|------|-------|
| **2026-03** | 📝 Added `src/process/` (BPMN parser hardening) and `src/maintenance/` (RBAC audit trail) security notes |
| **2026-01** | 🔒 Major security improvements in v1.3.4 (RocksDB, Docker, Updates) |
| **2025-12** | 🔐 Update Checker security features & Manifest signing design |
| **2025-11** | 📝 Initial security policy publication |

---

## 🔒 Recent Security Work

> [!NOTE]
> **Comprehensive Security Summary (v1.3.0 - v1.3.4):**  
> See [Security Work Summary](/docs/de/releases/SECURITY_WORK_SUMMARY_V1.3.4.md) for detailed information about recent security improvements.

### Highlights (v1.3.4)

**RocksDB Wrapper Security Fixes:**
- ✅ 7 critical vulnerabilities fixed (use-after-free, null-pointer, memory leaks)
- ✅ 8 medium-severity issues resolved (deadlocks, resource leaks)
- 📊 100% elimination of segfault risks
- 📖 [Full Audit Report](audit/docs/Audit/ROCKSDB_WRAPPER_AUDIT_REPORT.md)

**Docker Security Improvements:**
- ✅ Ubuntu 24.04 LTS base image (extended security support)
- ✅ Automated security updates during build
- ✅ 80%+ reduction in CVEs
- 📖 [Docker Security Fixes](docs/en/deployment/DOCKER_SECURITY_FIXES.md)

**Update Checker Security:**
- ✅ Token masking and secure handling
- ✅ HTTPS-only communication
- ✅ Thread-safe implementation
- 📖 [Update Security Summary](/docs/de/releases/updates_security_summary.md)

**Binary Authenticity (Design):**
- ✅ Cryptographic manifest signing architecture
- ✅ SHA-256 hash verification
- ✅ RSA-4096 digital signatures
- 📖 [Manifest Security](/docs/de/releases/updates_manifest_security.md)

---

<div align="center">

**🔐 Security is a top priority at ThemisDB**

[🚨 Report a Vulnerability](https://github.com/makr-code/ThemisDB/security/advisories/new) · [📖 Security Docs](docs/security/) · [🛡️ Security Posture Guide](docs/production/SECURITY_POSTURE.md)

</div>
