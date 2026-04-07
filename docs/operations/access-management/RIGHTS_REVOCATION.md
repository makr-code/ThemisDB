# Rights Revocation Automation

**Version:** 1.5.0  
**Last Updated:** 2026-04-06  
**Addresses:** FIND-028 - Automate Manual Rights Revocations

---

## Overview

This document describes the automated rights revocation process for ThemisDB, addressing audit finding FIND-028. The automation ensures timely and consistent revocation of access rights when users change roles, leave the organization, or when access is no longer required.

---

## Objectives

- **Automate** rights revocation upon user exit or role change
- **Ensure** timely access removal (within 1 hour of termination)
- **Maintain** comprehensive audit trail
- **Comply** with ISO 27001 A.9.2.6 (Removal of access rights)
- **Prevent** unauthorized access by former employees

---

## Revocation Triggers

### Automatic Triggers

1. **User Termination**
   - HR system integration (HRIS webhook)
   - Immediate revocation of all access rights
   - Account suspension within 15 minutes

2. **Role Change**
   - New role assigned in IAM system
   - Excess permissions automatically removed
   - Role transition period: 4 hours

3. **Contract Expiration**
   - Contractor/temporary account expiry
   - Automatic deactivation on end date
   - 7-day grace period with approval

4. **Policy Violation**
   - Security policy violation detected
   - Immediate suspension pending investigation
   - Security team notification

5. **Prolonged Inactivity**
   - No login for 90+ days
   - Automatic account suspension
   - Re-activation requires manager approval

---

## Revocation Process

### Workflow

```
Trigger Event → Validation → Revocation → Audit → Notification → Verification
```

### Step-by-Step Process

**1. Trigger Detection**
- HR system event (termination, role change)
- IAM policy change
- Scheduled inactive account check
- Manual security team action

**2. Validation**
- Verify trigger authenticity
- Check for exceptions/overrides
- Validate user identity
- Review current access rights

**3. Revocation Execution**
- Suspend user account
- Revoke authentication tokens
- Disable API keys
- Remove group memberships
- Revoke database permissions
- Archive user data

**4. Audit Logging**
- Log all revocation actions
- Record timestamp and reason
- Capture before/after state
- Store cryptographically signed audit trail

**5. Notification**
- Notify security team
- Alert user's manager
- Update compliance dashboard
- Create tracking ticket

**6. Verification**
- Confirm access disabled
- Verify no active sessions
- Check for orphaned permissions
- Update access review reports

---

## Implementation

### Script: `revoke-access.sh`

**Location:** `scripts/operations/revoke-access.sh`

**Usage:**
```bash
# Revoke single user access
./scripts/operations/revoke-access.sh --user <username>

# Revoke with reason
./scripts/operations/revoke-access.sh --user <username> --reason "User terminated"

# Batch revocation from CSV
./scripts/operations/revoke-access.sh --batch users.csv

# Revoke specific role/group
./scripts/operations/revoke-access.sh --user <username> --role admin

# Dry-run mode (simulation)
./scripts/operations/revoke-access.sh --user <username> --dry-run

# Emergency revocation (bypass approval)
./scripts/operations/revoke-access.sh --user <username> --emergency

# Partial revocation (specific permissions only)
./scripts/operations/revoke-access.sh --user <username> --permissions "db.write,api.admin"
```

**Options:**
- `--user <username>` - Target user account
- `--batch <file>` - Batch revocation from CSV file
- `--role <role>` - Revoke specific role only
- `--permissions <perms>` - Revoke specific permissions (comma-separated)
- `--reason <reason>` - Revocation reason (required for audit)
- `--dry-run` - Simulation mode without actual changes
- `--emergency` - Bypass approval workflow
- `--preserve-data` - Keep user data (don't archive)
- `--schedule <time>` - Schedule revocation for future time

### Batch Revocation CSV Format

**File:** `users.csv`

```csv
username,reason,revocation_type,preserve_data
john.doe,Terminated,full,false
jane.smith,Role Change,partial,true
bob.jones,Contract Expired,full,false
```

**Fields:**
- `username` - User account identifier
- `reason` - Revocation reason (for audit)
- `revocation_type` - `full` or `partial`
- `preserve_data` - `true` to keep user data, `false` to archive

---

## Integration with HR System

### HRIS Webhook Integration

**Endpoint:** `POST /api/v1/webhooks/hr-events`

**Authentication:** Bearer token (configured in `config/integrations.yaml`)

**Event Payload:**
```json
{
  "event_type": "user_terminated",
  "event_id": "evt_123456",
  "timestamp": "2026-02-03T14:30:00Z",
  "user": {
    "username": "john.doe",
    "employee_id": "EMP-12345",
    "email": "john.doe@example.com",
    "termination_date": "2026-02-03",
    "termination_reason": "Resignation"
  },
  "metadata": {
    "department": "Engineering",
    "manager": "jane.smith",
    "last_working_day": "2026-02-03"
  }
}
```

**Event Types:**
- `user_terminated` - Employee termination
- `user_role_changed` - Role/department change
- `user_suspended` - Account suspension
- `contractor_expired` - Contract end date reached

### Automation Workflow

```yaml
# .github/workflows/rights-revocation.yml
name: Rights Revocation Automation

on:
  repository_dispatch:
    types: [hr_event]
  workflow_dispatch:
    inputs:
      username:
        description: 'Username to revoke access'
        required: true
      reason:
        description: 'Revocation reason'
        required: true

jobs:
  revoke-access:
    runs-on: ubuntu-latest
    steps:
      - name: Checkout
        uses: actions/checkout@v3
      
      - name: Execute revocation
        run: |
          ./scripts/operations/revoke-access.sh \
            --user ${{ github.event.client_payload.username || github.event.inputs.username }} \
            --reason "${{ github.event.client_payload.reason || github.event.inputs.reason }}"
      
      - name: Verify revocation
        run: |
          ./scripts/operations/verify-revocation.sh \
            --user ${{ github.event.client_payload.username || github.event.inputs.username }}
      
      - name: Notify stakeholders
        run: |
          ./scripts/operations/notify-revocation.sh \
            --user ${{ github.event.client_payload.username || github.event.inputs.username }}
```

---

## Revocation Actions

### Account Actions

**Full Revocation:**
1. ✅ Disable user account
2. ✅ Revoke all authentication tokens
3. ✅ Terminate active sessions
4. ✅ Disable API keys
5. ✅ Remove SSH keys
6. ✅ Revoke database credentials
7. ✅ Remove from all groups/roles
8. ✅ Archive user data
9. ✅ Notify stakeholders

**Partial Revocation (Role Change):**
1. ✅ Remove excess permissions
2. ✅ Update role assignments
3. ✅ Maintain required access
4. ✅ Update access documentation
5. ✅ Notify user and manager

### Data Handling

**Data Archival:**
- User files moved to `archive/users/<username>/`
- Retention period: 90 days (configurable)
- Encrypted at rest
- Access restricted to compliance team

**Data Preservation (for legal hold):**
- Mark account with legal hold flag
- Prevent automated deletion
- Maintain audit trail
- Document preservation reason

---

## Audit Trail

### Revocation Events

**Log File:** `logs/access-revocation-audit.log`

**Log Format:**
```json
{
  "event_id": "rev_2026020314301234",
  "timestamp": "2026-02-03T14:30:12Z",
  "event_type": "access_revoked",
  "user": {
    "username": "john.doe",
    "employee_id": "EMP-12345",
    "email": "john.doe@example.com"
  },
  "revocation": {
    "type": "full",
    "reason": "User terminated",
    "triggered_by": "hr_system_webhook",
    "executor": "automation-service",
    "actions_taken": [
      "account_disabled",
      "tokens_revoked",
      "sessions_terminated",
      "groups_removed",
      "data_archived"
    ]
  },
  "before_state": {
    "account_status": "active",
    "roles": ["developer", "db_user"],
    "groups": ["engineering", "backend-team"],
    "permissions": ["db.read", "db.write", "api.access"]
  },
  "after_state": {
    "account_status": "disabled",
    "roles": [],
    "groups": [],
    "permissions": []
  },
  "verification": {
    "status": "success",
    "verified_at": "2026-02-03T14:30:25Z",
    "verifier": "automation-service"
  }
}
```

### Audit Events

- `revocation_triggered` - Revocation process started
- `validation_completed` - Trigger validation finished
- `access_revoked` - Access rights removed
- `account_disabled` - Account suspended
- `data_archived` - User data archived
- `verification_completed` - Revocation verified
- `notification_sent` - Stakeholders notified

---

## Metrics & Monitoring

### Revocation Metrics

| Metric | Target | Description |
|--------|--------|-------------|
| Time to revoke (termination) | < 15 minutes | From trigger to completion |
| Time to revoke (role change) | < 4 hours | Transition period allowed |
| Verification success rate | 100% | All revocations verified |
| False revocation rate | < 0.1% | Incorrect revocations |
| Audit trail completeness | 100% | All events logged |

### Dashboard Integration

**Grafana Dashboard:** `Access Revocation Metrics`

**Prometheus Metrics:**
```prometheus
# Revocation execution time
access_revocation_duration_seconds{type="full"} 45
access_revocation_duration_seconds{type="partial"} 120

# Revocation counts
access_revocations_total{reason="terminated"} 12
access_revocations_total{reason="role_change"} 8
access_revocations_total{reason="inactive"} 3

# Verification status
access_revocation_verification_success_rate 1.0

# Active revocations in progress
access_revocations_in_progress 0
```

**Alerts:**
```yaml
# Alert if revocation takes too long
- alert: SlowRevocation
  expr: access_revocation_duration_seconds > 900
  for: 1m
  annotations:
    summary: "Access revocation taking longer than 15 minutes"

# Alert on verification failure
- alert: RevocationVerificationFailed
  expr: access_revocation_verification_failed_total > 0
  for: 1m
  annotations:
    summary: "Access revocation verification failed"
```

---

## Exception Handling

### Grace Periods

**Contractor Extensions:**
```bash
# Extend contractor account for 30 days
./scripts/operations/revoke-access.sh --user <username> --extend 30d --reason "Contract extension approved"
```

**Temporary Access:**
```bash
# Grant temporary access (7 days)
./scripts/operations/grant-temporary-access.sh --user <username> --duration 7d --permissions "db.read"
```

### Emergency Access

**Break-glass Scenario:**
```bash
# Emergency re-activation
./scripts/operations/emergency-access.sh --user <username> --duration 2h --reason "Production incident" --approver <manager>
```

**Requirements:**
- Manager approval required
- Time-limited access (max 24 hours)
- Enhanced audit logging
- Post-incident review mandatory

---

## Compliance Documentation

### ISO 27001 Requirements

**A.9.2.6 - Removal or adjustment of access rights**

- ✅ Timely removal of access rights on termination
- ✅ Review of access rights on role change
- ✅ Removal of logical access to systems and applications
- ✅ Return/removal of physical access devices

### BSI C5 Requirements

**OIS-04 - Segregation of Duties**

- ✅ Automated revocation prevents manual errors
- ✅ Separation of revocation and approval
- ✅ Audit trail of all changes
- ✅ Regular verification of revocations

### GDPR Compliance

**Article 32 - Security of Processing**

- ✅ Access controls properly maintained
- ✅ Timely revocation on employee exit
- ✅ Data handling documented
- ✅ Audit logs preserved

---

## Testing & Validation

### Testing the Automation

```bash
# Test revocation with test user
./scripts/operations/revoke-access.sh --user test.user --dry-run

# Verify revocation script
./scripts/operations/test-revocation.sh

# Integration test
./scripts/operations/test-hr-webhook.sh
```

### Validation Checklist

- [ ] Account disabled successfully
- [ ] All tokens revoked
- [ ] Active sessions terminated
- [ ] Group memberships removed
- [ ] Database permissions revoked
- [ ] API keys disabled
- [ ] Data archived (if applicable)
- [ ] Audit log created
- [ ] Notifications sent
- [ ] Verification completed

---

## Rollback Procedure

### Accidental Revocation

```bash
# Restore access (requires manager approval)
./scripts/operations/restore-access.sh --user <username> --approval <ticket-id>

# Restore from backup state
./scripts/operations/restore-access.sh --user <username> --from-backup <timestamp>
```

**Rollback Steps:**
1. Verify revocation was accidental
2. Obtain manager/security approval
3. Restore account to previous state
4. Notify affected user
5. Document incident
6. Review automation logic

---

## Troubleshooting

### Common Issues

**Issue:** Revocation script fails
```bash
# Check database connectivity
./scripts/operations/revoke-access.sh --check-db

# Verify IAM permissions
./scripts/operations/revoke-access.sh --check-permissions
```

**Issue:** Partial revocation incomplete
```bash
# Review revocation log
tail -f logs/access-revocation-audit.log

# Verify current user state
./scripts/operations/check-user-access.sh --user <username>

# Complete partial revocation
./scripts/operations/revoke-access.sh --user <username> --force
```

**Issue:** HR webhook not triggered
```bash
# Test webhook endpoint
curl -X POST https://api.example.com/webhooks/hr-events \
  -H "Authorization: Bearer <token>" \
  -H "Content-Type: application/json" \
  -d '{"event_type": "test"}'

# Check webhook logs
tail -f logs/webhook-events.log
```

---

## Configuration

**Config File:** `config/rights-revocation.yaml`

```yaml
rights_revocation:
  # Timing settings
  timing:
    termination_timeout: 900  # 15 minutes
    role_change_timeout: 14400  # 4 hours
    verification_delay: 60  # 1 minute
  
  # Revocation actions
  actions:
    disable_account: true
    revoke_tokens: true
    terminate_sessions: true
    remove_groups: true
    revoke_db_permissions: true
    disable_api_keys: true
    archive_data: true
  
  # Data handling
  data:
    archive_enabled: true
    archive_retention_days: 90
    encryption_enabled: true
    legal_hold_check: true
  
  # Notifications
  notifications:
    security_team: true
    user_manager: true
    compliance_team: true
    
  # Integration
  integrations:
    hr_webhook_enabled: true
    iam_sync_enabled: true
    audit_logging_enabled: true
```

---

## Related Documentation

- [Access Review Automation](ACCESS_REVIEW_AUTOMATION.md)
- [Operations Handbook](../OPERATIONS_HANDBOOK.md)
- [Security Deployment Guide](../../security/SECURITY_DEPLOYMENT_GUIDE.md)
- [Audit Findings Report](../../../audit-reports/v1.4.1/FINDINGS_AND_RISKS.md)

---

**Document Version:** 1.5.0  
**Compliance:** ISO 27001 A.9.2.6, BSI C5 OIS-04  
**Last Reviewed:** 2026-02-03
