# Governance Troubleshooting Guide

The `governance` module provides the policy engine for ThemisDB, including compliance reporting, versioned policy management, policy templates, review workflows, and regulatory compliance enforcement.

## Quick Diagnostics

| Symptom | Likely Cause | Quick Fix |
|---------|-------------|-----------|
| `PolicyEngine: no matching policy` | Default policy set to `allow`; no explicit policy | Create explicit policy; set `default_action: deny` |
| Compliance report generation fails | Report template not found | Check `governance.reports.template_dir` |
| Policy change not applied | Watcher not running | Enable `governance.policy.watch.enabled: true` |
| `PolicyReview: approval loop` | Reviewer circular approval | Break cycle; assign different reviewer |
| `PolicyCoordinator: quorum failed` | Not enough nodes for policy vote | Check coordinator connectivity |
| Versioned policy rollback fails | Version not in history | Check `governance.versioning.max_history` |
| Old policy version still active | Cache not invalidated after update | Enable `governance.policy.invalidate_on_update` |
| Compliance report missing data | Collection not in policy scope | Add collection to compliance policy |
| Policy template variable not substituted | Template engine error | Check template syntax |
| `PolicyFileWatcher: permission denied` | Wrong file permissions on policy dir | Fix permissions on policy directory |

## Common Issues

### Issue 1: Policy Engine Allows Unintended Actions

**Description:** Actions that should be blocked by governance policies are being allowed.

**Symptoms:**
- Audit log shows writes to `financial_data` without policy match
- Log: `PolicyEngine: no matching policy for action=write resource=financial_data; using default=allow`

**Cause:** Default policy is `allow`; explicit deny policy not defined.

**Solution:**
```yaml
governance:
  policy:
    default_action: deny           # deny anything not explicitly allowed
    strict_mode: true
  policies:
    - name: allow_analytics_read
      action: read
      resources: [analytics_*]
      principals: [role:analyst]
      effect: allow
    - name: block_direct_write
      action: write
      resources: [financial_data]
      principals: ["*"]
      effect: deny
      exceptions: [role:finance_admin]
```

---

### Issue 2: Compliance Report Generation Fails

**Description:** Scheduled compliance reports cannot be generated.

**Symptoms:**
- Log: `ComplianceReporter: template 'gdpr_audit' not found in /etc/themisdb/governance/templates/`
- Compliance API returns 500

**Cause:** Report template directory is wrong or templates are missing.

**Solution:**
```bash
# Check template directory
ls /etc/themisdb/governance/templates/

# Install default templates
themisdb-admin governance install-templates --type gdpr,sox,hipaa

# Generate report manually
themisdb-admin governance report generate \
  --type gdpr_audit \
  --period "2025-Q1" \
  --output /tmp/gdpr_report.pdf
```
```yaml
governance:
  reports:
    template_dir: /etc/themisdb/governance/templates
    output_dir: /var/lib/themisdb/reports
    schedule: "0 6 * * 1"        # weekly Monday 06:00
```

---

### Issue 3: Policy Changes Not Applied

**Description:** After updating a policy file, the new policy is not enforced.

**Symptoms:**
- Policy file on disk is updated
- Server still enforcing old policy
- Log: `PolicyFileWatcher: watching disabled`

**Cause:** File watcher is disabled; policies loaded only at startup.

**Solution:**
```yaml
governance:
  policy:
    watch:
      enabled: true
      poll_interval_ms: 5000
      debounce_ms: 1000
    invalidate_on_update: true
    reload_timeout_ms: 10000
```
```bash
# Manual reload
themisdb-admin governance policy reload

# Verify active policies
themisdb-admin governance policy list --active
```

---

### Issue 4: Policy Cache Serves Stale Entry

**Description:** A policy that was deleted is still being enforced.

**Symptoms:**
- Deleted policy still blocks access
- Log: `PolicyManager: serving cached policy 'old_policy' (age=3600s)`

**Cause:** Policy cache TTL too long.

**Solution:**
```yaml
governance:
  policy:
    cache:
      ttl_ms: 5000               # 5 seconds instead of 1 hour
      invalidate_on_update: true
      max_entries: 10000
```

---

### Issue 5: Versioned Policy Rollback Fails

**Description:** Rolling back to a previous policy version fails.

**Symptoms:**
- Error: `PolicyManagerVersioned: version=5 not found in history`
- Rollback API returns 404

**Cause:** Policy version history is too short; old versions have been pruned.

**Solution:**
```yaml
governance:
  versioning:
    enabled: true
    max_history: 100              # keep 100 previous versions (from default 10)
    retain_forever: [1, 2, 3]    # always keep these version numbers
```
```bash
# List policy version history
themisdb-admin governance policy versions --policy allow_analytics_read

# Rollback to specific version
themisdb-admin governance policy rollback \
  --policy allow_analytics_read \
  --version 5
```

---

### Issue 6: Compliance Report Missing Collections

**Description:** Compliance report does not include data from certain collections.

**Symptoms:**
- GDPR report missing access records for `customer_data` collection
- Collection not in policy scope

**Cause:** Collection was not added to the compliance policy scope.

**Solution:**
```yaml
governance:
  compliance:
    scope:
      collections:
        - customer_data
        - orders
        - payments
    include_schema_changes: true
    include_access_logs: true
    include_data_modifications: true
```

---

### Issue 7: Policy Review Approval Loop

**Description:** A policy change requires approval but the approval chain creates a loop.

**Symptoms:**
- Log: `PolicyReview: circular approval chain detected: alice → bob → alice`
- Policy change stuck in `PENDING_REVIEW` state

**Cause:** Reviewer assignments create a circular dependency.

**Solution:**
```bash
# Break the loop by assigning a neutral approver
themisdb-admin governance policy review \
  --policy new_data_retention \
  --action reassign-reviewer \
  --new-reviewer compliance_officer

# Or force approve (admin only)
themisdb-admin governance policy review \
  --policy new_data_retention \
  --action force-approve \
  --reason "Breaking approval loop – reviewed by CISO"
```

---

### Issue 8: Policy File Watcher Permission Denied

**Description:** The policy file watcher cannot read policy files due to permission issues.

**Symptoms:**
- Log: `PolicyFileWatcher: permission denied: /etc/themisdb/governance/policies/`
- Log: `PolicyEngine: running with last known policies`

**Cause:** ThemisDB service user does not have read permissions on the policy directory.

**Solution:**
```bash
# Fix permissions
chown -R root:themisdb /etc/themisdb/governance/policies/
chmod -R 640 /etc/themisdb/governance/policies/
chmod 750 /etc/themisdb/governance/policies/
```

## Diagnostic Commands

```bash
# List active policies
themisdb-admin governance policy list

# Evaluate a policy for a specific request
themisdb-admin governance policy evaluate \
  --action write \
  --resource financial_data \
  --principal user:alice

# Compliance report status
themisdb-admin governance report status

# Pending policy reviews
themisdb-admin governance policy reviews --state pending

# Live governance metrics
curl -s http://localhost:9100/metrics | grep themisdb_governance

# Tail governance logs
journalctl -u themisdb -f | grep -E "governance|policy|compliance|review"
```

## Configuration Reference

```yaml
governance:
  enabled: true
  policy:
    default_action: deny
    watch:
      enabled: true
      poll_interval_ms: 5000
    cache:
      ttl_ms: 10000
  versioning:
    enabled: true
    max_history: 50
  compliance:
    scope:
      collections: []
  reports:
    template_dir: /etc/themisdb/governance/templates
    schedule: ""
```

## Known Limitations

- Policy evaluation adds latency (typically 1–5ms) to every request; complex policies with many rules add more.
- Compliance reports are generated synchronously by default; large reports may time out; use async generation.
- Policy versioning stores full policy text; very large policies (>1MB each) may cause high history storage usage.
- Cross-collection policy rules (e.g., user can write to A only if B exists) require custom policy functions.

## Related Documentation

- [Governance Module ROADMAP](../../src/governance/ROADMAP.md)
- [Tenant Isolation Guide](../de/security/TENANT_ISOLATION_GUIDE.md)
- [Security Executive Summary](../de/security/SECURITY_EXECUTIVE_SUMMARY.md)
- [Information Security Policy](../security/INFORMATION_SECURITY_POLICY.md)
- [Audit Log Retention](../ARCHIVED/implementation-summaries/AUDIT_LOG_RETENTION_IMPLEMENTATION.md)
