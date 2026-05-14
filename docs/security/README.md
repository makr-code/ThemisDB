# Security Documentation

**Version:** 1.8.1  
**Last Updated:** May 2026  
**Target Audience:** Security Engineers, DevOps Engineers, Compliance Officers

---

## Overview

This directory contains security documentation for ThemisDB, covering hardening, key management, access control, encryption, compliance, and HSM integration. For the production security hardening checklist and guide, start with the links below.

---

## Quick Navigation

### Start Here

| Document | Purpose |
|----------|---------|
| [PRODUCTION_HARDENING_CHECKLIST.md](PRODUCTION_HARDENING_CHECKLIST.md) | Step-by-step checklist for hardening a production deployment |
| [INFORMATION_SECURITY_POLICY.md](INFORMATION_SECURITY_POLICY.md) | Organization-wide information security policy |
| [../production/SECURITY.md](../production/SECURITY.md) | Production security hardening guide (GPU, TLS, audit logging, HSM, key rotation) |
| [../production/SECURITY_POSTURE.md](../production/SECURITY_POSTURE.md) | Security defaults vs. production-hardened settings |

### Encryption & Key Management

| Document | Purpose |
|----------|---------|
| [ENCRYPTION_KEY_MANAGEMENT_POLICY.md](ENCRYPTION_KEY_MANAGEMENT_POLICY.md) | Key lifecycle, rotation, and custodian responsibilities |
| [encryption_strategy.md](encryption_strategy.md) | Encryption architecture (at-rest and in-transit) |
| [HSM_IMPLEMENTATION_SUMMARY.md](HSM_IMPLEMENTATION_SUMMARY.md) | Hardware Security Module integration summary |
| [HSM_PRODUCTION_DEPLOYMENT.md](HSM_PRODUCTION_DEPLOYMENT.md) | HSM deployment guide for production |
| [HSM_PRODUCTION_SETUP.md](HSM_PRODUCTION_SETUP.md) | Step-by-step HSM setup procedure |
| [HSM_VENDOR_CONFIGURATIONS.md](HSM_VENDOR_CONFIGURATIONS.md) | Vendor-specific HSM configuration examples |
| [PKCS11_INTEGRATION.md](PKCS11_INTEGRATION.md) | PKCS#11 interface integration |
| [VAULT_SIGNING_PROVIDER.md](VAULT_SIGNING_PROVIDER.md) | HashiCorp Vault signing provider setup |

### Access Control & Authentication

| Document | Purpose |
|----------|---------|
| [access_control_framework.md](access_control_framework.md) | Role-based access control (RBAC) framework |
| [api_authentication_authorization.md](api_authentication_authorization.md) | API authentication and authorization patterns |
| [changefeed_authentication.md](changefeed_authentication.md) | CDC stream authentication |
| [zero_trust_policy_enforcer.md](zero_trust_policy_enforcer.md) | Zero-trust network policy enforcement |

### Compliance & Audit

| Document | Purpose |
|----------|---------|
| [../production/CHECKLISTS/compliance.md](../production/CHECKLISTS/compliance.md) | SOC2, GDPR, HIPAA compliance checklists |
| [../production/CHECKLISTS/operational_compliance.md](../production/CHECKLISTS/operational_compliance.md) | Monthly operational compliance verification |
| [SECURITY_IMPLEMENTATION_SUMMARY.md](SECURITY_IMPLEMENTATION_SUMMARY.md) | Summary of implemented security controls |
| [FIND-002_IMPLEMENTATION_SUMMARY.md](FIND-002_IMPLEMENTATION_SUMMARY.md) | Security finding FIND-002 remediation |

### Specialized Topics

| Document | Purpose |
|----------|---------|
| [usb_admin_feature.md](usb_admin_feature.md) | USB-based admin key feature |
| [usb_admin_implementation_summary.md](usb_admin_implementation_summary.md) | USB admin implementation details |
| [learnable-rope-security-assessment.md](learnable-rope-security-assessment.md) | Security assessment for learnable RoPE embeddings |

---

## Security Hardening Quick Reference

### Mandatory Production Settings

```yaml
# config.yaml – minimum security baseline
tls:
  enabled: true
  version: "1.3"
  cert_file: "/etc/themis/tls/server.crt"
  key_file: "/etc/themis/tls/server.key"

mtls:
  enabled: true  # for inter-node communication

audit_logging:
  enabled: true
  level: "full"
  output: "/var/log/themis/audit.log"

encryption_at_rest:
  enabled: true
  algorithm: "AES-256-GCM"
```

### Security Checklist (Abbreviated)

- [ ] TLS 1.3 configured for all client connections
- [ ] mTLS enabled for inter-node communication
- [ ] Disk encryption enabled (LUKS or equivalent)
- [ ] Audit logging configured and shipping to SIEM
- [ ] GPU access controls configured (cgroups)
- [ ] Key rotation automated (90-day cycle)
- [ ] HSM integrated for key storage (production)
- [ ] Security monitoring active (Prometheus alerts)
- [ ] Zero-trust network policies applied

→ Full checklist: [PRODUCTION_HARDENING_CHECKLIST.md](PRODUCTION_HARDENING_CHECKLIST.md)

---

## Compliance Standards

| Standard | Status | Reference |
|----------|--------|-----------|
| ISO 27001 | Implemented | [INFORMATION_SECURITY_POLICY.md](INFORMATION_SECURITY_POLICY.md) |
| BSI C5 | Implemented | [Operations Handbook](../operations/OPERATIONS_HANDBOOK.md) |
| SOC 2 Type II | In progress | [Compliance Checklist](../production/CHECKLISTS/compliance.md) |
| GDPR | Implemented | [Compliance Checklist](../production/CHECKLISTS/compliance.md) |
| HIPAA | Available | [Compliance Checklist](../production/CHECKLISTS/compliance.md) |

---

## Incident Response for Security Events

1. Follow [Incident Response Playbook](../operations/incident-response/INCIDENT_RESPONSE_PLAYBOOK.md)
2. Consult [Security Incident section in RUNBOOKS.md](../production/RUNBOOKS.md#security-incident-response)
3. Execute [Rights Revocation procedure](../operations/access-management/RIGHTS_REVOCATION.md) if needed

---

## Related Documentation

- **Operations Hub**: [../OPERATIONS.md](../OPERATIONS.md)
- **Operations Handbook**: [../operations/OPERATIONS_HANDBOOK.md](../operations/OPERATIONS_HANDBOOK.md)
- **Production Security Guide**: [../production/SECURITY.md](../production/SECURITY.md)
- **Security Posture**: [../production/SECURITY_POSTURE.md](../production/SECURITY_POSTURE.md)
- **Audit Framework**: [../audit-framework/AUDIT_RUNBOOK.md](../audit-framework/AUDIT_RUNBOOK.md)

---

**Document Classification:** Internal – Confidential  
**Review Cycle:** Quarterly (mandatory) / After any security incident  
**Maintained by:** Security Team
