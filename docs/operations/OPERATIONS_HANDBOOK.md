# ThemisDB Operations Handbook

**Version:** 1.5.0  
**Last Updated:** 2026-02-03  
**Target Audience:** Site Reliability Engineers, Database Administrators, Security Operations, Compliance Officers

---

## Table of Contents

1. [Overview](#overview)
2. [Access Management](#access-management)
3. [Incident Response](#incident-response)
4. [Disaster Recovery](#disaster-recovery)
5. [Logging & Monitoring](#logging--monitoring)
6. [Compliance & Audit](#compliance--audit)
7. [Automation & CI/CD](#automation--cicd)

---

## Overview

This Operations Handbook provides comprehensive operational procedures for ThemisDB deployments. It covers access management, incident response, disaster recovery, logging configuration, and compliance requirements aligned with ISO 27001 and BSI C5 standards.

### Operational Objectives

- **Availability:** 99.95% uptime (RTO: 1 hour, RPO: 15 minutes)
- **Security:** Zero-trust access model with automated reviews
- **Compliance:** ISO 27001, BSI C5, GDPR, SOC 2
- **Automation:** 90% of operational tasks automated

---

## Access Management

### Overview

Access management procedures ensure proper user access controls throughout the user lifecycle, from onboarding to offboarding.

### Access Review Procedures

**Frequency:** Quarterly (automated monthly reports)

**Automated Access Review:**
```bash
# Run access review report
./scripts/operations/access-review.sh --report

# Generate compliance report
./scripts/operations/access-review.sh --compliance-report
```

**Manual Review Process:**
1. Review automated access report
2. Validate user roles against current job functions
3. Identify and flag excessive permissions
4. Document review findings
5. Submit remediation plan

**See also:** [Access Review Automation](access-management/ACCESS_REVIEW_AUTOMATION.md)

### User Onboarding

**Process:**
1. Submit access request via ServiceNow/JIRA
2. Manager approval required
3. Security team validation
4. Role-based access provisioning
5. MFA enrollment mandatory
6. Security training completion required

**Standard Roles:**
- **Admin:** Full system access (requires MFA)
- **Operator:** Operational access (requires MFA)
- **Developer:** Read/write access to dev environments
- **Auditor:** Read-only access to logs and audit trails
- **User:** Basic authenticated access

### User Offboarding

**Process:**
1. HR initiates offboarding
2. Automated rights revocation triggered
3. Access keys rotated
4. Audit trail preserved
5. Knowledge transfer documented

**Automated Rights Revocation:**
```bash
# Revoke user access
./scripts/operations/revoke-access.sh --user <username>

# Bulk revocation for terminated users
./scripts/operations/revoke-access.sh --batch users.csv
```

**See also:** [Rights Revocation Automation](access-management/RIGHTS_REVOCATION.md)

### Access Documentation

All access changes must be documented in:
- **Change Log:** `logs/access-changes.log`
- **Audit Trail:** `logs/audit.log`
- **Compliance Dashboard:** Grafana metrics

---

## Incident Response

### Overview

Incident response procedures follow ISO 27001 (A.16) and BSI C5 (OIS-01 to OIS-04) requirements.

### Incident Classification

| Severity | Response Time | Examples |
|----------|---------------|----------|
| **P0 - Critical** | 15 minutes | Data breach, service outage |
| **P1 - High** | 1 hour | Security vulnerability, significant degradation |
| **P2 - Medium** | 4 hours | Minor service issues, non-critical bugs |
| **P3 - Low** | 24 hours | Documentation issues, feature requests |

### Incident Response Process

**Detection → Triage → Containment → Eradication → Recovery → Lessons Learned**

**See also:** [Incident Response Playbook](incident-response/INCIDENT_RESPONSE_PLAYBOOK.md)

### Automated Incident Response Drills

**Frequency:** Monthly

```bash
# Run incident response drill
./scripts/operations/incident-drill.sh --scenario data-breach

# Generate drill report
./scripts/operations/incident-drill.sh --report
```

**Drill Scenarios:**
1. Data breach response
2. Ransomware attack
3. DDoS attack
4. Insider threat
5. Service outage recovery

**See also:** [Incident Response Testing](incident-response/INCIDENT_RESPONSE_TESTING.md)

---

## Disaster Recovery

### Overview

Disaster Recovery (DR) procedures ensure business continuity with defined Recovery Time Objective (RTO) and Recovery Point Objective (RPO).

**RTO Target:** 1 hour  
**RPO Target:** 15 minutes

### Backup Procedures

**Backup Schedule:**
- **Full Backup:** Daily at 02:00 UTC
- **Incremental Backup:** Every 4 hours
- **Transaction Log Backup:** Every 15 minutes

**Backup Retention:**
- Daily backups: 30 days
- Weekly backups: 90 days
- Monthly backups: 1 year

### Automated DR Testing

**Frequency:** Weekly

```bash
# Run automated DR test
./scripts/operations/dr-test.sh --full

# Test backup restore
./scripts/operations/dr-test.sh --test-restore

# Verify RTO/RPO metrics
./scripts/operations/dr-test.sh --verify-metrics
```

**See also:** [DR Testing Automation](disaster-recovery/DR_TESTING.md)

### DR Checklists

**Pre-Disaster:**
- [ ] Backup verification completed
- [ ] DR site connectivity verified
- [ ] Runbooks up to date
- [ ] Contact lists current

**During Disaster:**
- [ ] Incident declared
- [ ] Stakeholders notified
- [ ] DR site activated
- [ ] Data restoration initiated

**Post-Disaster:**
- [ ] Service restored
- [ ] Data integrity verified
- [ ] Incident report completed
- [ ] Lessons learned documented

**See also:** [DR Checklists](disaster-recovery/DR_CHECKLISTS.md)

---

## Logging & Monitoring

### Logging Configuration

**Centralized Logging:** All logs forwarded to centralized logging system (ELK/Splunk)

**Log Categories:**
- **Application Logs:** `/var/log/themisdb/app.log`
- **Audit Logs:** `/var/log/themisdb/audit.log`
- **Security Logs:** `/var/log/themisdb/security.log`
- **Performance Logs:** `/var/log/themisdb/performance.log`

**Log Retention:**
- Application logs: 90 days
- Audit logs: 7 years (compliance requirement)
- Security logs: 2 years

**See also:** [Logging Configuration Guide](logging/LOGGING_CONFIGURATION.md)

### Monitoring Configuration

**Metrics Collection:**
- System metrics (CPU, memory, disk, network)
- Application metrics (transactions, queries, errors)
- Security metrics (authentication failures, access violations)
- Compliance metrics (audit events, policy violations)

**Alerting Thresholds:**
- CPU utilization > 80% for 5 minutes
- Memory utilization > 85%
- Disk space < 15% free
- Error rate > 1% of requests
- Authentication failure rate > 10 per minute

**See also:** [Monitoring Setup Guide](MONITORING_SETUP_GUIDE.md)

---

## Compliance & Audit

### Compliance Standards

ThemisDB operations comply with:
- **ISO 27001:2013** - Information Security Management
- **BSI C5:2020** - Cloud Computing Compliance Controls Catalogue
- **GDPR** - General Data Protection Regulation
- **SOC 2 Type II** - Service Organization Controls

### Audit Requirements

**Audit Log Requirements:**
- All administrative actions logged
- All access attempts logged (success and failure)
- All data modifications logged
- All configuration changes logged

**Audit Trail Integrity:**
- Logs cryptographically signed
- Tamper-proof storage
- Immutable audit records
- Regular integrity verification

### Compliance Dashboard

**Metrics Dashboard:** Grafana dashboards track compliance indicators

**Key Metrics:**
- Access review completion rate
- Incident response time (average)
- Backup success rate
- DR test success rate
- Security event rate
- Compliance policy violations

**Dashboard URL:** `https://grafana.example.com/d/compliance`

**See also:** [Metrics Dashboard Configuration](../observability/METRICS_DASHBOARD.md)

---

## Automation & CI/CD

### Operational Automation

**CI/CD Integration:**
- Access reviews: Monthly automated reports
- DR testing: Weekly automated tests
- Incident drills: Monthly automated scenarios
- Compliance reporting: Real-time dashboard updates

**GitHub Actions Workflows:**
- `.github/workflows/access-review.yml` - Automated access reviews
- `.github/workflows/dr-testing.yml` - Automated DR testing
- `.github/workflows/incident-drill.yml` - Incident response drills

### Operational Scripts

**Location:** `scripts/operations/`

**Available Scripts:**
- `access-review.sh` - Access review automation
- `revoke-access.sh` - Automated rights revocation
- `incident-drill.sh` - Incident response testing
- `dr-test.sh` - Disaster recovery testing
- `logging-config.sh` - Logging configuration management
- `compliance-report.sh` - Compliance reporting

### Script Usage

```bash
# Access review
./scripts/operations/access-review.sh --report

# Rights revocation
./scripts/operations/revoke-access.sh --user john.doe

# Incident drill
./scripts/operations/incident-drill.sh --scenario data-breach

# DR test
./scripts/operations/dr-test.sh --full

# Compliance report
./scripts/operations/compliance-report.sh --monthly
```

---

## Related Documentation

- [Operational Procedures](OPERATIONAL_PROCEDURES.md)
- [Monitoring Setup Guide](MONITORING_SETUP_GUIDE.md)
- [Troubleshooting Guide](TROUBLESHOOTING_GUIDE.md)
- [Security Deployment Guide](../security/SECURITY_DEPLOYMENT_GUIDE.md)
- [Audit Findings Report](../../audit-reports/v1.4.1/FINDINGS_AND_RISKS.md)

---

## Change History

| Version | Date | Changes | Author |
|---------|------|---------|--------|
| 1.5.0 | 2026-02-03 | Initial comprehensive handbook created | Operations Team |

---

**Document Version:** 1.5.0  
**ThemisDB Compatibility:** 1.5.0+  
**Last Reviewed:** 2026-02-03  
**Next Review:** 2026-05-03 (Quarterly)
