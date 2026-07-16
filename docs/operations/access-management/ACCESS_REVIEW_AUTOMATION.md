# Access Review Automation

**Version:** 1.5.0  
**Last Updated:** 2026-04-06  
**Addresses:** FIND-020 - Manual Access Reviews

---

## Overview

This document describes the automated access review process for ThemisDB, addressing audit finding FIND-020. The automation provides regular CI/CD-based access reviews to ensure proper access controls and compliance with security policies.

---

## Objectives

- **Automate** quarterly access reviews
- **Generate** monthly access review reports
- **Detect** excessive or stale permissions
- **Ensure** compliance with ISO 27001 A.9.2.5 (Review of user access rights)
- **Document** all access review activities

---

## Access Review Process

### 1. Automated Data Collection

The access review system automatically collects:
- User accounts and their roles
- Last login timestamps
- Permission assignments
- Role-to-permission mappings
- Access grant dates
- Account status (active/inactive)

### 2. Analysis & Detection

**Automated checks:**
- **Stale Accounts:** No login in 90+ days
- **Excessive Permissions:** Users with admin rights in multiple systems
- **Dormant Admin Accounts:** Admin accounts with no activity in 30+ days
- **Orphaned Accounts:** Accounts without associated HR records
- **Non-MFA Admin Accounts:** Admin accounts without MFA enabled

### 3. Report Generation

**Monthly Report Contents:**
- Executive summary
- Access review statistics
- Findings and recommendations
- User access matrix
- Compliance status
- Action items

### 4. Review Workflow

```
Data Collection → Analysis → Report Generation → Review → Remediation → Verification
```

**Stakeholders:**
- **Security Team:** Primary reviewers
- **Managers:** Validate team access
- **Compliance Team:** Ensure regulatory compliance
- **Audit Team:** Review findings and documentation

---

## Implementation

### Script: `access-review.sh`

**Location:** `scripts/operations/access-review.sh`

**Usage:**
```bash
# Generate access review report
./scripts/operations/access-review.sh --report

# Generate compliance report (quarterly)
./scripts/operations/access-review.sh --compliance-report

# Export user access matrix
./scripts/operations/access-review.sh --export-matrix

# Check specific user
./scripts/operations/access-review.sh --user <username>

# Dry-run mode (no changes)
./scripts/operations/access-review.sh --dry-run
```

**Options:**
- `--report` - Generate monthly access review report
- `--compliance-report` - Generate quarterly compliance report
- `--export-matrix` - Export user-role-permission matrix (CSV)
- `--user <username>` - Review specific user access
- `--dry-run` - Simulation mode without actual changes
- `--email` - Email report to stakeholders
- `--format <format>` - Output format: markdown, pdf, html (default: markdown)

### CI/CD Integration

**GitHub Actions Workflow:** `.github/workflows/access-review.yml`

**Schedule:** 
- Monthly reports: 1st of each month at 09:00 UTC
- Quarterly compliance reports: 1st of Jan/Apr/Jul/Oct at 09:00 UTC

**Workflow Steps:**
1. Collect user access data
2. Run analysis checks
3. Generate report
4. Upload report as artifact
5. Send notifications to security team
6. Create tracking issue if findings detected

**Workflow Configuration:**
```yaml
name: Access Review Automation
on:
  schedule:
    - cron: '0 9 1 * *'  # Monthly on 1st at 09:00 UTC
  workflow_dispatch:      # Manual trigger
```

---

## Report Structure

### Monthly Access Review Report

**File:** `reports/access-review-YYYY-MM.md`

**Sections:**
1. **Executive Summary**
   - Total users reviewed
   - Findings count by severity
   - Compliance status
   - Action items

2. **Access Review Statistics**
   - Total accounts: Active / Inactive / Stale
   - Role distribution
   - MFA adoption rate
   - Last review date

3. **Findings**
   - Stale accounts (90+ days)
   - Excessive permissions
   - Dormant admin accounts
   - Non-compliant configurations

4. **User Access Matrix**
   - User → Role → Permissions mapping
   - Access grant dates
   - Last activity timestamps

5. **Recommendations**
   - Immediate actions required
   - Policy improvements
   - Process enhancements

6. **Compliance Status**
   - ISO 27001 A.9.2.5: Review of user access rights
   - BSI C5 OIS-03: Access Management
   - SOC 2 CC6.3: Logical Access Controls

### Quarterly Compliance Report

**File:** `reports/compliance/access-review-QN-YYYY.md`

**Additional Sections:**
- Trend analysis (quarterly comparison)
- Policy compliance metrics
- Audit trail verification
- Management review signoff

---

## Access Review Criteria

### User Account Categories

| Category | Criteria | Review Frequency | Action |
|----------|----------|------------------|--------|
| **Active Users** | Login within 30 days | Monthly monitoring | No action |
| **Inactive Users** | No login 30-90 days | Monthly review | Manager confirmation |
| **Stale Accounts** | No login 90+ days | Immediate review | Disable account |
| **Admin Accounts** | Admin role assigned | Weekly monitoring | MFA enforcement |
| **Service Accounts** | System/API accounts | Quarterly review | Rotate credentials |

### Permission Review Checklist

- [ ] User role matches job function
- [ ] Permissions align with role definition
- [ ] No excessive admin privileges
- [ ] MFA enabled for privileged accounts
- [ ] Last access within policy timeframe
- [ ] Account owner confirmed by manager
- [ ] No orphaned accounts detected
- [ ] Access documentation up to date

---

## Metrics & KPIs

### Access Review Metrics

| Metric | Target | Current | Status |
|--------|--------|---------|--------|
| Review completion rate | 100% | - | Tracked |
| Time to remediate findings | < 7 days | - | Tracked |
| Stale account detection rate | 100% | - | Tracked |
| False positive rate | < 5% | - | Tracked |
| Access review cycle time | < 2 weeks | - | Tracked |

### Dashboard Integration

**Grafana Dashboard:** `Access Review Metrics`

**Metrics collected:**
- `access_review_users_total` - Total users reviewed
- `access_review_findings_total` - Total findings by severity
- `access_review_stale_accounts` - Stale accounts detected
- `access_review_completion_rate` - Review completion percentage
- `access_review_remediation_time` - Average remediation time (days)

**Prometheus Metrics:**
```prometheus
# Stale accounts gauge
access_review_stale_accounts{threshold="90d"} 0

# Review completion rate
access_review_completion_rate{period="monthly"} 1.0

# Findings by severity
access_review_findings_total{severity="critical"} 0
access_review_findings_total{severity="high"} 0
access_review_findings_total{severity="medium"} 0
```

---

## Remediation Process

### Automated Remediation

**For stale accounts (90+ days):**
```bash
# Automatic account suspension
./scripts/operations/access-review.sh --auto-suspend-stale
```

**For non-MFA admin accounts:**
```bash
# Enforce MFA enrollment
./scripts/operations/access-review.sh --enforce-mfa-admin
```

### Manual Remediation

**Excessive permissions:**
1. Review user's job function
2. Compare current vs. required permissions
3. Submit change request
4. Manager approval required
5. Apply permission changes
6. Verify and document

**Orphaned accounts:**
1. Attempt to contact account owner
2. Verify with HR department
3. If no owner found within 7 days → Disable
4. Preserve account for 30 days
5. After 30 days → Delete account
6. Maintain audit trail

---

## Compliance Documentation

### ISO 27001 Requirements

**A.9.2.5 - Review of user access rights**

- ✅ Regular review of access rights (monthly/quarterly)
- ✅ Management authorization for access changes
- ✅ Documentation of review activities
- ✅ Removal of unnecessary access rights

### BSI C5 Requirements

**OIS-03 - Access Management**

- ✅ Documented access provisioning process
- ✅ Regular access reviews conducted
- ✅ Access rights aligned with job functions
- ✅ Audit trail of all access changes

### GDPR Compliance

**Article 32 - Security of Processing**

- ✅ Access controls implemented
- ✅ Regular review of access rights
- ✅ Documentation of processing activities
- ✅ Audit logging of access events

---

## Audit Trail

All access review activities are logged:

**Log File:** `logs/access-review-audit.log`

**Log Format:**
```json
{
  "timestamp": "2026-02-03T10:00:00Z",
  "event": "access_review_completed",
  "review_id": "AR-2026-02",
  "users_reviewed": 142,
  "findings": 3,
  "reviewer": "security-team",
  "status": "completed"
}
```

**Audit Events:**
- `access_review_started` - Review initiated
- `access_review_completed` - Review finished
- `finding_detected` - Issue found
- `remediation_applied` - Fix implemented
- `review_approved` - Management signoff

---

## Testing & Validation

### Testing the Automation

```bash
# Test with sample data
./scripts/operations/access-review.sh --test-mode

# Validate report generation
./scripts/operations/access-review.sh --validate

# Dry-run (no actual changes)
./scripts/operations/access-review.sh --dry-run --report
```

### Validation Checklist

- [ ] Script executes without errors
- [ ] Report generated successfully
- [ ] All users included in review
- [ ] Findings accurately detected
- [ ] Metrics exported to dashboard
- [ ] Email notifications sent
- [ ] Audit logs created

---

## Troubleshooting

### Common Issues

**Issue:** Report generation fails
```bash
# Check database connectivity
./scripts/operations/access-review.sh --check-db

# Verify permissions
./scripts/operations/access-review.sh --check-permissions
```

**Issue:** Missing users in report
```bash
# Verify user data sync
./scripts/operations/access-review.sh --sync-users

# Check data source configuration
cat config/access-review.yaml
```

**Issue:** False positives detected
```bash
# Review detection criteria
./scripts/operations/access-review.sh --show-criteria

# Adjust thresholds in config
vim config/access-review.yaml
```

---

## Configuration

**Config File:** `config/access-review.yaml`

```yaml
access_review:
  # Review schedule
  schedule:
    monthly_report: true
    quarterly_compliance: true
  
  # Detection thresholds
  thresholds:
    stale_account_days: 90
    inactive_admin_days: 30
    excessive_permissions: 5
  
  # Notification settings
  notifications:
    enabled: true
    recipients:
      - security-team@example.com
      - compliance@example.com
    
  # Report settings
  reports:
    format: markdown
    retention_days: 730  # 2 years
    
  # Integration
  integrations:
    grafana_enabled: true
    slack_enabled: true
    jira_enabled: true
```

---

## Related Documentation

- [Rights Revocation Automation](RIGHTS_REVOCATION.md)
- [Operations Handbook](../OPERATIONS_HANDBOOK.md)
- [Audit Findings Report](../../../audit-reports/v1.4.1/FINDINGS_AND_RISKS.md)

---

**Document Version:** 1.5.0  
**Compliance:** ISO 27001 A.9.2.5, BSI C5 OIS-03  
**Last Reviewed:** 2026-02-03
