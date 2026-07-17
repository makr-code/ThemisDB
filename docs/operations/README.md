# Operations Documentation

**Version:** 1.8.1  
**Last Updated:** May 2026  
**Target Audience:** Site Reliability Engineers, Database Administrators, Security Operations, Compliance Officers

---

## Overview

This directory contains operational handbooks, runbooks, and specialized guides for ThemisDB day-to-day operations. It covers access management, incident response, disaster recovery, logging, and the admin CLI.

---

## Contents

### Core Handbooks & Runbooks

| Document | Purpose |
|----------|---------|
| [OPERATIONS_HANDBOOK.md](OPERATIONS_HANDBOOK.md) | Comprehensive operational procedures: access management, incident response, DR, logging, compliance, CI/CD automation |
| [OPERATIONS_RUNBOOK.md](OPERATIONS_RUNBOOK.md) | Phase 5 operational tooling runbook: startup, monitoring, troubleshooting, backup/recovery, scaling |
| [THEMISCTL_ADMIN_GUIDE.md](THEMISCTL_ADMIN_GUIDE.md) | Admin CLI (`themisctl`) reference: installation, all commands, REPL, configuration |
| [PIPELINE_E2E_SOPs.md](PIPELINE_E2E_SOPs.md) | Role-based SOPs for the full RAG→Package→Build→Deploy→Recover pipeline (Dev, Operator, Auditor, Data Owner) |

### Access Management

| Document | Purpose |
|----------|---------|
| [access-management/ACCESS_REVIEW_AUTOMATION.md](access-management/ACCESS_REVIEW_AUTOMATION.md) | Automated quarterly access review procedures and scripts |
| [access-management/RIGHTS_REVOCATION.md](access-management/RIGHTS_REVOCATION.md) | Immediate access revocation procedures for offboarding / security incidents |

### Disaster Recovery

| Document | Purpose |
|----------|---------|
| [disaster-recovery/DR_CHECKLISTS.md](disaster-recovery/DR_CHECKLISTS.md) | Step-by-step DR execution checklists |
| [disaster-recovery/DR_TESTING.md](disaster-recovery/DR_TESTING.md) | Quarterly DR drill procedures and pass/fail criteria |

### Incident Response

| Document | Purpose |
|----------|---------|
| [incident-response/INCIDENT_RESPONSE_PLAYBOOK.md](incident-response/INCIDENT_RESPONSE_PLAYBOOK.md) | Complete incident response playbook (severity classification, escalation, communication) |
| [incident-response/INCIDENT_RESPONSE_TESTING.md](incident-response/INCIDENT_RESPONSE_TESTING.md) | Incident response drill procedures and review criteria |

### Logging & Observability

| Document | Purpose |
|----------|---------|
| [logging/LOGGING_CONFIGURATION.md](logging/LOGGING_CONFIGURATION.md) | Log configuration, retention policies, and shipping to log aggregation systems |

---

## Operational Objectives

| Objective | Target |
|-----------|--------|
| Availability | 99.95% uptime |
| Recovery Time (RTO) | ≤ 1 hour |
| Recovery Point (RPO) | ≤ 15 minutes |
| Access Review Frequency | Quarterly (automated monthly) |
| Compliance Standards | ISO 27001, BSI C5, GDPR, SOC 2 |

---

## Common Operational Procedures

### Health Check

```bash
themisctl --port 8765 health
```

### Emergency Stop

```bash
sudo systemctl stop themisdb
```

### Create Full Backup

```bash
themisdb-cli backup create --type full
```

→ Full backup guide: [../backup_recovery_system.md](../backup_recovery_system.md)  
→ Restore runbook: [../production/RUNBOOKS/RESTORE_RUNBOOK.md](../production/RUNBOOKS/RESTORE_RUNBOOK.md)

### Incident Response Quick Start

1. **Assess severity** (P0=Critical, P1=High, P2=Medium, P3=Low)
2. Follow [Incident Response Playbook](incident-response/INCIDENT_RESPONSE_PLAYBOOK.md)
3. Use [Troubleshooting Guide](../production/TROUBLESHOOTING.md)
4. Execute [Emergency Procedures](../production/RUNBOOKS.md#emergency-procedures)

---

## Related Documentation

- **Main Operations Hub**: [../OPERATIONS.md](../OPERATIONS.md)
- **Production Operations**: [../production/README.md](../production/README.md)
- **Deployment Guide**: [../deployment/README.md](../deployment/README.md)
- **Maintenance Schedule**: [../maintenance/README.md](../maintenance/README.md)
- **Security Guide**: [../security/README.md](../security/README.md)
- **CI/CD Documentation**: [../ci-cd/README.md](../ci-cd/README.md)
- **CDC Operations**: [../CDC_OPERATIONS_RUNBOOK.md](../CDC_OPERATIONS_RUNBOOK.md)
- **Backup & Recovery**: [../backup_recovery_system.md](../backup_recovery_system.md)

---

**Document Classification:** Internal – Operations  
**Review Cycle:** Quarterly  
**Maintained by:** Operations Team / SRE
