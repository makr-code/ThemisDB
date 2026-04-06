# Operational Compliance Checklist

**Version:** 1.0  
**Last Updated:** April 2026  
**Review Frequency:** Monthly  
**Document Owner:** Operations Team

---

## Overview

This checklist ensures ThemisDB operations comply with operational excellence standards, SLAs, and industry best practices.

---

## 1. Availability & Uptime (SLA: 99.9%)

### Monthly Targets
- [ ] **Availability >= 99.9%** (max 43 minutes downtime/month)
- [ ] **MTTD < 2 minutes** (Mean Time To Detect)
- [ ] **MTTR < 30 minutes** (Mean Time To Recover)
- [ ] **Zero unplanned outages affecting all users**

### Verification
```bash
# Check monthly availability
themisdb-cli metrics get-availability --period 30d
# Target: >= 99.9%

# Check MTTD
themisdb-cli metrics get-mttd --period 30d
# Target: < 2 minutes

# Check MTTR
themisdb-cli metrics get-mttr --period 30d
# Target: < 30 minutes
```

### Documentation
- [ ] All incidents documented in incident management system
- [ ] Root cause analysis completed for P0/P1 incidents
- [ ] Remediation actions tracked and completed

---

## 2. Performance & Latency (SLO)

### Performance Targets
- [ ] **P95 Latency < 200ms** for API requests
- [ ] **P99 Latency < 500ms** for API requests
- [ ] **Throughput >= 10,000 requests/second** at peak
- [ ] **Query timeout rate < 0.01%**

### Verification
```bash
# Check P95 latency
themisdb-cli metrics get-latency --percentile 95 --period 30d
# Target: < 200ms

# Check throughput
themisdb-cli metrics get-qps --period 30d --aggregation max
# Target: >= 10,000 QPS
```

### Actions
- [ ] Performance baselines updated monthly
- [ ] Performance degradation incidents investigated
- [ ] Optimization opportunities identified and tracked

---

## 3. Error Rate & Reliability (SLO)

### Reliability Targets
- [ ] **Error rate < 0.1%** (99.9% success rate)
- [ ] **No data loss incidents**
- [ ] **No data corruption incidents**
- [ ] **Zero security breaches**

### Verification
```bash
# Check error rate
themisdb-cli metrics get-error-rate --period 30d
# Target: < 0.1%

# Check data integrity
themisdb-cli data verify --sample-size 10000
# Expected: No errors
```

### Documentation
- [ ] All errors logged and categorized
- [ ] High-impact errors investigated
- [ ] Error trends analyzed monthly

---

## 4. Backup & Recovery (RTO/RPO)

### Backup Requirements
- [ ] **Daily full backups completed** (30-day retention)
- [ ] **Incremental backups every 15 minutes** (24-hour retention)
- [ ] **All backups verified** for integrity
- [ ] **Cross-region backup replication working**

### Recovery Objectives
- [ ] **RTO < 4 hours** for complete data loss
- [ ] **RPO < 15 minutes** for point-in-time recovery
- [ ] **Quarterly DR drill completed** and documented

### Verification
```bash
# Verify recent backups
themisdb-cli backup list --last 7-days --verify

# Check backup integrity
themisdb-cli backup verify-all --period 7d

# Test restore (staging)
themisdb-cli restore test --environment staging --latest-backup
```

### Documentation
- [ ] Backup schedule documented and followed
- [ ] DR plan reviewed and updated quarterly
- [ ] DR test results documented

---

## 5. Security & Access Control

### Security Requirements
- [ ] **TLS 1.3 enabled** for all connections
- [ ] **mTLS enabled** for inter-node communication
- [ ] **RBAC properly configured** and audited
- [ ] **Audit logging enabled** and reviewed
- [ ] **Security patches applied** within SLA (7 days for critical)

### Access Control
- [ ] **Principle of least privilege** enforced
- [ ] **Access reviews completed** monthly
- [ ] **Unused accounts disabled**
- [ ] **MFA enabled** for all administrative access

### Verification
```bash
# Check TLS configuration
themisdb-cli security check-tls

# Audit active sessions
themisdb-cli security list-sessions

# Review recent access
themisdb-cli security audit-log --period 7d --type access
```

### Documentation
- [ ] Security incidents documented
- [ ] Access control matrix up to date
- [ ] Security audit findings tracked

---

## 6. Monitoring & Alerting

### Monitoring Requirements
- [ ] **All critical metrics monitored** (CPU, memory, disk, network)
- [ ] **SLA metrics tracked** (availability, latency, errors)
- [ ] **Alerting rules tested** and functional
- [ ] **On-call rotation staffed** 24/7

### Dashboard Requirements
- [ ] **SLA dashboard** accessible and up to date
- [ ] **Operations dashboard** showing cluster health
- [ ] **Capacity dashboard** showing resource utilization
- [ ] **Security dashboard** showing auth/audit metrics

### Verification
```bash
# Test alerting
themisdb-cli alerts test-all

# Verify metrics collection
themisdb-cli metrics verify-collection

# Check dashboard status
curl -s http://grafana.example.com/api/dashboards/themisdb
```

### Documentation
- [ ] Runbooks linked from all critical alerts
- [ ] Alert thresholds reviewed and tuned monthly
- [ ] False positive alerts investigated and fixed

---

## 7. Capacity Planning & Scaling

### Capacity Requirements
- [ ] **CPU utilization < 75%** average
- [ ] **Memory utilization < 80%** average  
- [ ] **Storage capacity > 25% free**
- [ ] **Auto-scaling configured** and tested

### Capacity Review
- [ ] **Monthly capacity review** completed
- [ ] **90-day capacity forecast** updated
- [ ] **Scaling events tracked** and analyzed

### Verification
```bash
# Check resource utilization
themisdb-cli cluster capacity-status

# Forecast capacity needs
themisdb-cli capacity forecast --horizon 90d

# Review scaling history
themisdb-cli cluster scaling-history --period 30d
```

### Documentation
- [ ] Capacity trends documented
- [ ] Scaling triggers documented
- [ ] Right-sizing recommendations tracked

---

## 8. Change Management

### Change Requirements
- [ ] **All changes documented** in change management system
- [ ] **Testing completed** before production deployment
- [ ] **Rollback plan prepared** for all changes
- [ ] **Change approval** obtained per policy

### Deployment Requirements
- [ ] **Zero-downtime deployments** for standard changes
- [ ] **Maintenance windows scheduled** for high-risk changes
- [ ] **Post-deployment validation** completed

### Verification
- [ ] Recent changes documented in CHANGELOG.md
- [ ] All production changes have tickets
- [ ] Rollback procedures tested

### Documentation
- [ ] Deployment runbooks up to date
- [ ] Known issues documented
- [ ] Lessons learned captured

---

## 9. Documentation & Knowledge Management

### Documentation Requirements
- [ ] **Runbooks current** (reviewed quarterly)
- [ ] **Architecture diagrams** up to date
- [ ] **API documentation** complete and accurate
- [ ] **Operational procedures** documented

### Knowledge Sharing
- [ ] **Post-incident reviews** conducted for P0/P1
- [ ] **Lessons learned** shared with team
- [ ] **Training materials** updated quarterly

### Verification
- [ ] Documentation reviewed in last 90 days
- [ ] Links verified (no broken links)
- [ ] All major incidents have PIRs

---

## 10. Compliance & Audit

### Compliance Requirements
- [ ] **SOC 2 Type II** controls validated
- [ ] **GDPR compliance** verified (if applicable)
- [ ] **HIPAA compliance** verified (if applicable)
- [ ] **Data retention policies** followed

### Audit Requirements
- [ ] **Quarterly security audits** completed
- [ ] **Access reviews** completed monthly
- [ ] **Compliance gaps** tracked and remediated

### Verification
```bash
# Run compliance checks
themisdb-cli compliance check --standard soc2

# Generate audit report
themisdb-cli compliance report --period quarter --output pdf

# Check data retention
themisdb-cli data retention-status
```

### Documentation
- [ ] Audit findings tracked in issue tracker
- [ ] Compliance evidence collected and stored
- [ ] Remediation plans documented

---

## 11. Operational Excellence

### Process Requirements
- [ ] **Incident response procedures** followed
- [ ] **On-call handoffs** documented
- [ ] **Team training** completed quarterly
- [ ] **Operational metrics** reviewed monthly

### Continuous Improvement
- [ ] **Automation opportunities** identified
- [ ] **Technical debt** tracked and prioritized
- [ ] **Improvement initiatives** tracked

### Team Readiness
- [ ] **On-call training** completed for all team members
- [ ] **DR drills** conducted quarterly
- [ ] **Runbook walkthroughs** completed monthly

---

## Monthly Sign-Off

### Review Checklist

| Area | Compliant | Notes | Action Items |
|------|-----------|-------|-------------|
| Availability & Uptime | ☐ Yes ☐ No | | |
| Performance & Latency | ☐ Yes ☐ No | | |
| Error Rate & Reliability | ☐ Yes ☐ No | | |
| Backup & Recovery | ☐ Yes ☐ No | | |
| Security & Access | ☐ Yes ☐ No | | |
| Monitoring & Alerting | ☐ Yes ☐ No | | |
| Capacity Planning | ☐ Yes ☐ No | | |
| Change Management | ☐ Yes ☐ No | | |
| Documentation | ☐ Yes ☐ No | | |
| Compliance & Audit | ☐ Yes ☐ No | | |
| Operational Excellence | ☐ Yes ☐ No | | |

### Overall Compliance Score

**Score**: _____ / 11 areas compliant

**Status**: 
- ✅ **Pass**: 10-11 areas compliant
- ⚠️ **Conditional**: 8-9 areas compliant (action plan required)
- ❌ **Fail**: < 8 areas compliant (immediate remediation required)

### Sign-Off

| Role | Name | Signature | Date |
|------|------|-----------|------|
| Operations Lead | | | |
| Engineering Manager | | | |
| Security Officer | | | |
| Compliance Officer | | | |

---

## Automated Compliance Checking

### Run Full Compliance Check

```bash
#!/bin/bash
# operational-compliance-check.sh

echo "=== Operational Compliance Check ==="
echo "Date: $(date)"
echo ""

PASS=0
FAIL=0

# 1. Availability
echo "1. Checking Availability..."
AVAIL=$(themisdb-cli metrics get-availability --period 30d --format json | jq -r '.availability')
if (( $(echo "$AVAIL >= 99.9" | bc -l) )); then
  echo "✅ Availability: ${AVAIL}% (>= 99.9%)"
  ((PASS++))
else
  echo "❌ Availability: ${AVAIL}% (< 99.9%)"
  ((FAIL++))
fi

# 2. Latency
echo "2. Checking Latency..."
P95=$(themisdb-cli metrics get-latency --percentile 95 --period 30d --format json | jq -r '.latency_ms')
if (( $(echo "$P95 <= 200" | bc -l) )); then
  echo "✅ P95 Latency: ${P95}ms (<= 200ms)"
  ((PASS++))
else
  echo "❌ P95 Latency: ${P95}ms (> 200ms)"
  ((FAIL++))
fi

# 3. Error Rate
echo "3. Checking Error Rate..."
ERRORS=$(themisdb-cli metrics get-error-rate --period 30d --format json | jq -r '.error_rate')
if (( $(echo "$ERRORS <= 0.1" | bc -l) )); then
  echo "✅ Error Rate: ${ERRORS}% (<= 0.1%)"
  ((PASS++))
else
  echo "❌ Error Rate: ${ERRORS}% (> 0.1%)"
  ((FAIL++))
fi

# 4. Backups
echo "4. Checking Backups..."
BACKUP_COUNT=$(themisdb-cli backup list --last 7-days --format json | jq 'length')
if [ "$BACKUP_COUNT" -ge 7 ]; then
  echo "✅ Backups: ${BACKUP_COUNT} backups in last 7 days (>= 7)"
  ((PASS++))
else
  echo "❌ Backups: ${BACKUP_COUNT} backups in last 7 days (< 7)"
  ((FAIL++))
fi

# ... Add more checks ...

echo ""
echo "=== Summary ==="
echo "Passed: $PASS"
echo "Failed: $FAIL"
TOTAL=$((PASS + FAIL))
SCORE=$(echo "scale=1; ($PASS * 100) / $TOTAL" | bc)
echo "Compliance Score: ${SCORE}%"

if [ "$FAIL" -eq 0 ]; then
  echo "✅ All checks passed"
  exit 0
else
  echo "❌ $FAIL check(s) failed"
  exit 1
fi
```

---

## Appendix: Quick Reference

### Critical Thresholds

| Metric | Warning | Critical | SLA |
|--------|---------|----------|-----|
| Availability | < 99.9% | < 99.5% | >= 99.9% |
| P95 Latency | > 200ms | > 500ms | <= 200ms |
| Error Rate | > 0.1% | > 1% | <= 0.1% |
| CPU Usage | > 75% | > 90% | <= 75% |
| Memory Usage | > 80% | > 95% | <= 80% |
| Storage Usage | > 75% | > 90% | <= 75% |
| MTTR | > 30min | > 60min | <= 30min |
| MTTD | > 2min | > 5min | <= 2min |

### Emergency Contacts

- **On-Call**: [PagerDuty]
- **Operations Lead**: [Phone/Email]
- **Engineering Manager**: [Phone/Email]
- **Security Team**: [Phone/Email]
- **Compliance Officer**: [Phone/Email]

---

**Document Version:** 1.0  
**Next Review:** End of next month  
**Review Cycle:** Monthly
