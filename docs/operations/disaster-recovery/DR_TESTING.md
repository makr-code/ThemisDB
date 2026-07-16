# Disaster Recovery Testing Automation

**Version:** 1.5.0  
**Last Updated:** 2026-04-06  
**Addresses:** FIND-032 - Automate Backup Restore Testing (DR)  
**Standards:** ISO 27001 A.17.1, BSI C5 BCR-01

---

## Overview

This document describes the automated disaster recovery (DR) testing framework for ThemisDB. Regular automated tests ensure backup systems are functional and recovery objectives (RTO/RPO) are met.

---

## DR Objectives

### Recovery Time Objective (RTO)

**Target:** 1 hour  
**Definition:** Maximum acceptable downtime

### Recovery Point Objective (RPO)

**Target:** 15 minutes  
**Definition:** Maximum acceptable data loss

### Testing Objectives

- **Validate** backup integrity and completeness
- **Verify** restore procedures function correctly
- **Measure** actual RTO/RPO achievement
- **Identify** gaps in DR procedures
- **Ensure** compliance with business continuity requirements

---

## Backup Strategy

### Backup Types

**1. Full Backup**
- **Frequency:** Daily at 02:00 UTC
- **Retention:** 30 days
- **Size:** ~500 GB (estimated)
- **Duration:** ~2 hours

**2. Incremental Backup**
- **Frequency:** Every 4 hours
- **Retention:** 7 days
- **Size:** ~50 GB average
- **Duration:** ~15 minutes

**3. Transaction Log Backup**
- **Frequency:** Every 15 minutes
- **Retention:** 24 hours
- **Size:** ~5 GB average
- **Duration:** ~2 minutes

### Backup Locations

**Primary:** `/mnt/backup/primary/`  
**Secondary:** `/mnt/backup/secondary/` (offsite)  
**Archive:** S3-compatible object storage (long-term)

---

## Automated DR Testing

### Script: `dr-test.sh`

**Location:** `scripts/operations/dr-test.sh`

**Usage:**
```bash
# Run full DR test
./scripts/operations/dr-test.sh --full

# Test specific backup
./scripts/operations/dr-test.sh --backup /mnt/backup/full-20260203

# Test restore only (no verification)
./scripts/operations/dr-test.sh --test-restore

# Verify backup integrity
./scripts/operations/dr-test.sh --verify-backup

# Measure RTO/RPO
./scripts/operations/dr-test.sh --verify-metrics

# Generate DR test report
./scripts/operations/dr-test.sh --report

# Dry-run mode
./scripts/operations/dr-test.sh --dry-run

# Quick smoke test
./scripts/operations/dr-test.sh --smoke-test
```

**Options:**
- `--full` - Complete DR test (backup → restore → verify)
- `--backup <path>` - Test specific backup
- `--test-restore` - Restore test only
- `--verify-backup` - Verify backup integrity
- `--verify-metrics` - Check RTO/RPO compliance
- `--report` - Generate test report
- `--dry-run` - Simulation mode
- `--smoke-test` - Quick validation (15 min)
- `--target <env>` - Target environment (staging/dr-site)

---

## DR Testing Process

### Weekly Automated Test

**Schedule:** Every Sunday at 03:00 UTC

**Test Steps:**

**1. Pre-Test Validation (5 minutes)**
```bash
# Verify backup availability
check_backup_exists()
check_backup_integrity()
check_dr_environment_ready()
```

**2. Backup Verification (10 minutes)**
```bash
# Validate backup
verify_backup_metadata()
verify_backup_checksums()
verify_backup_encryption()
verify_backup_completeness()
```

**3. Restore Test (30 minutes)**
```bash
# Restore to DR environment
stop_dr_services()
clear_dr_data_directory()
restore_full_backup()
restore_incremental_backups()
restore_transaction_logs()
```

**4. Data Integrity Verification (10 minutes)**
```bash
# Verify restored data
compare_record_counts()
verify_data_checksums()
test_query_execution()
verify_index_integrity()
```

**5. Functional Testing (10 minutes)**
```bash
# Test basic operations
test_database_connectivity()
test_read_operations()
test_write_operations()
test_query_performance()
```

**6. RTO/RPO Measurement (5 minutes)**
```bash
# Calculate metrics
calculate_restore_time()  # RTO
calculate_data_loss()     # RPO
compare_against_targets()
```

**7. Cleanup (5 minutes)**
```bash
# Clean up test environment
stop_dr_services()
archive_test_results()
send_notifications()
```

**Total Duration:** ~75 minutes (within RTO target)

---

## Test Scenarios

### Scenario 1: Full Database Restore

**Objective:** Validate complete database restoration

**Steps:**
1. Select latest full backup
2. Restore to DR environment
3. Apply incremental backups
4. Apply transaction logs
5. Verify data integrity
6. Test application connectivity

**Success Criteria:**
- [ ] All data restored successfully
- [ ] No data corruption detected
- [ ] Record counts match source
- [ ] Application can connect and query
- [ ] Restore completed within RTO (1 hour)
- [ ] Data loss within RPO (15 minutes)

### Scenario 2: Point-in-Time Recovery

**Objective:** Restore database to specific timestamp

**Steps:**
1. Select backup before target time
2. Apply transaction logs to target time
3. Verify data state at target time
4. Test data consistency

**Success Criteria:**
- [ ] Database restored to exact timestamp
- [ ] Data matches expected state
- [ ] No data inconsistencies
- [ ] Transaction integrity maintained

### Scenario 3: Partial Data Restore

**Objective:** Restore specific tables/collections

**Steps:**
1. Identify target data to restore
2. Extract from backup
3. Restore to target environment
4. Verify specific data only

**Success Criteria:**
- [ ] Targeted data restored
- [ ] Surrounding data unchanged
- [ ] No data corruption
- [ ] Minimal downtime

### Scenario 4: Disaster Site Failover

**Objective:** Complete failover to DR site

**Steps:**
1. Declare disaster scenario
2. Activate DR site
3. Restore latest backup
4. Update DNS/routing
5. Redirect traffic to DR site
6. Verify service availability

**Success Criteria:**
- [ ] DR site activated successfully
- [ ] Data fully restored
- [ ] Service accessible
- [ ] Failover completed within RTO
- [ ] All critical functions working

---

## Backup Verification

### Integrity Checks

**Checksum Verification:**
```bash
# Verify backup checksums
sha256sum -c backup.sha256

# Expected output:
# database-full-20260203.tar.gz: OK
# database-incremental-20260203.tar.gz: OK
# transaction-logs-20260203.tar.gz: OK
```

**Encryption Verification:**
```bash
# Verify backup encryption
gpg --verify backup.sig

# Check encryption algorithm
file database-full-20260203.tar.gz.gpg
# Expected: GPG encrypted data (AES256)
```

**Metadata Verification:**
```bash
# Check backup metadata
cat backup-metadata.json
{
  "backup_id": "FULL-20260203-020000",
  "timestamp": "2026-02-03T02:00:00Z",
  "type": "full",
  "size_bytes": 536870912000,
  "compressed_size_bytes": 107374182400,
  "checksum": "sha256:abcd1234...",
  "encryption": "AES256",
  "retention_days": 30,
  "database_version": "1.4.1",
  "record_count": 150000000
}
```

---

## RTO/RPO Monitoring

### Metrics Collection

**Prometheus Metrics:**
```prometheus
# Backup success rate
backup_success_rate{type="full"} 1.0
backup_success_rate{type="incremental"} 1.0

# Backup duration
backup_duration_seconds{type="full"} 7200
backup_duration_seconds{type="incremental"} 900

# Restore time (RTO)
dr_restore_duration_seconds{scenario="full"} 3240
dr_rto_target_seconds 3600

# Data loss window (RPO)
dr_data_loss_seconds{scenario="full"} 600
dr_rpo_target_seconds 900

# DR test results
dr_test_success_rate 1.0
dr_test_last_run_timestamp 1738656000
```

### Grafana Dashboard

**Dashboard:** `Disaster Recovery Metrics`

**Panels:**
1. **RTO Achievement**
   - Current RTO vs. Target
   - RTO trend over time
   - RTO by restore scenario

2. **RPO Achievement**
   - Current RPO vs. Target
   - RPO trend over time
   - Data loss analysis

3. **Backup Health**
   - Backup success rate
   - Backup duration trends
   - Backup size growth
   - Failed backups alerts

4. **DR Test Results**
   - Test success rate
   - Last successful test
   - Test duration
   - Issues detected

5. **Compliance Status**
   - RTO/RPO compliance rate
   - Test frequency compliance
   - Backup retention compliance

---

## DR Test Report

### Report Template

**File:** `reports/dr-test-YYYY-MM-DD.md`

```markdown
# DR Test Report

**Test ID:** DR-TEST-2026-02-03  
**Date:** 2026-02-03 03:00 UTC  
**Test Type:** Full Restore Test  
**Status:** ✅ PASSED

## Executive Summary

- **RTO Achievement:** 54 minutes ✅ (Target: 60 minutes)
- **RPO Achievement:** 8 minutes ✅ (Target: 15 minutes)
- **Test Duration:** 72 minutes
- **Data Integrity:** 100% verified
- **Overall Result:** PASSED

## Test Timeline

| Step | Start Time | Duration | Status |
|------|------------|----------|--------|
| Pre-test validation | 03:00 | 5 min | ✅ PASS |
| Backup verification | 03:05 | 10 min | ✅ PASS |
| Restore execution | 03:15 | 30 min | ✅ PASS |
| Data verification | 03:45 | 10 min | ✅ PASS |
| Functional testing | 03:55 | 10 min | ✅ PASS |
| Metrics calculation | 04:05 | 5 min | ✅ PASS |
| Cleanup | 04:10 | 2 min | ✅ PASS |

## Metrics

### RTO (Recovery Time Objective)

- **Target:** 60 minutes
- **Achieved:** 54 minutes
- **Status:** ✅ WITHIN TARGET
- **Breakdown:**
  - Backup restore: 30 minutes
  - Transaction log replay: 15 minutes
  - Service startup: 9 minutes

### RPO (Recovery Point Objective)

- **Target:** 15 minutes
- **Achieved:** 8 minutes
- **Status:** ✅ WITHIN TARGET
- **Last backup:** 02:52 UTC
- **Test start:** 03:00 UTC
- **Data loss window:** 8 minutes

## Data Integrity

- **Total records:** 150,000,000
- **Records verified:** 150,000,000 (100%)
- **Checksum mismatches:** 0
- **Data corruption:** None detected
- **Index integrity:** ✅ All indexes valid

## Functional Tests

| Test | Result | Notes |
|------|--------|-------|
| Database connectivity | ✅ PASS | Connected successfully |
| Read operations | ✅ PASS | Query execution normal |
| Write operations | ✅ PASS | Insert/update working |
| Query performance | ✅ PASS | Within baseline |
| API endpoints | ✅ PASS | All endpoints responsive |

## Issues Detected

No issues detected during this test.

## Recommendations

1. Continue weekly automated DR tests
2. No action required at this time
3. RTO/RPO well within targets

## Next Test

**Scheduled:** 2026-02-10 03:00 UTC  
**Type:** Full Restore Test
```

---

## CI/CD Integration

### GitHub Actions Workflow

**File:** `.github/workflows/dr-testing.yml`

```yaml
name: DR Testing Automation

on:
  schedule:
    - cron: '0 3 * * 0'  # Weekly on Sunday at 03:00 UTC
  workflow_dispatch:
    inputs:
      test_type:
        description: 'DR test type'
        required: true
        type: choice
        options:
          - full
          - smoke-test
          - point-in-time
          - partial-restore

jobs:
  dr-test:
    runs-on: ubuntu-latest
    timeout-minutes: 120
    
    steps:
      - name: Checkout
        uses: actions/checkout@v3
      
      - name: Setup DR environment
        run: |
          ./scripts/operations/setup-dr-env.sh
      
      - name: Run DR test
        id: dr-test
        run: |
          ./scripts/operations/dr-test.sh --${{ inputs.test_type || 'full' }}
        continue-on-error: true
      
      - name: Generate report
        run: |
          ./scripts/operations/dr-test.sh --report
      
      - name: Upload report
        uses: actions/upload-artifact@v3
        with:
          name: dr-test-report
          path: reports/dr-test-*.md
      
      - name: Update metrics
        run: |
          ./scripts/operations/export-dr-metrics.sh
      
      - name: Notify team
        if: steps.dr-test.outcome == 'failure'
        run: |
          ./scripts/operations/notify-dr-failure.sh
      
      - name: Cleanup
        if: always()
        run: |
          ./scripts/operations/cleanup-dr-env.sh
```

---

## DR Checklists

### Pre-Disaster Checklist

- [ ] All backups completed successfully
- [ ] Backup integrity verified (last 7 days)
- [ ] DR site connectivity verified
- [ ] DR site capacity sufficient
- [ ] Contact lists up to date
- [ ] DR procedures documented and tested
- [ ] Team members trained on DR procedures
- [ ] Communication plan established

### During Disaster Checklist

- [ ] Disaster declared by authorized personnel
- [ ] Incident response team activated
- [ ] DR site activated
- [ ] Latest backup identified
- [ ] Restore process initiated
- [ ] Stakeholders notified (internal)
- [ ] Status updates provided (every 15 min)
- [ ] Progress tracked and documented

### Post-Disaster Checklist

- [ ] Service restored and verified
- [ ] Data integrity confirmed
- [ ] All critical functions tested
- [ ] Customers notified of resolution
- [ ] DNS/routing updated (if permanent)
- [ ] Post-incident review scheduled
- [ ] Lessons learned documented
- [ ] DR procedures updated

---

## Compliance Documentation

### ISO 27001 Requirements

**A.17.1 - Information security continuity**

- ✅ DR procedures documented
- ✅ Regular DR testing conducted
- ✅ RTO/RPO defined and measured
- ✅ Backup integrity verified

### BSI C5 Requirements

**BCR-01 - Business Continuity Management**

- ✅ Business continuity plan exists
- ✅ Regular testing and validation
- ✅ Recovery procedures documented
- ✅ Continuous improvement process

---

## Troubleshooting

### Common Issues

**Issue:** Backup restore fails
```bash
# Check backup integrity
./scripts/operations/dr-test.sh --verify-backup

# Try alternative backup
./scripts/operations/dr-test.sh --backup /mnt/backup/secondary/

# Check logs
tail -f logs/dr-test.log
```

**Issue:** RTO target exceeded
```bash
# Analyze restore timeline
./scripts/operations/analyze-restore-time.sh

# Check for bottlenecks
./scripts/operations/dr-performance-analysis.sh

# Optimize restore process
# Consider: parallel restore, faster storage, optimized network
```

**Issue:** Data integrity check fails
```bash
# Review specific failures
./scripts/operations/dr-test.sh --verify-data --verbose

# Compare checksums
./scripts/operations/compare-checksums.sh

# Restore from alternative backup
./scripts/operations/dr-test.sh --backup /mnt/backup/secondary/
```

---

## Configuration

**Config File:** `config/dr-testing.yaml`

```yaml
dr_testing:
  # RTO/RPO targets
  targets:
    rto_seconds: 3600  # 1 hour
    rpo_seconds: 900   # 15 minutes
  
  # Test schedule
  schedule:
    full_test: "0 3 * * 0"      # Weekly
    smoke_test: "0 4 * * *"     # Daily
    point_in_time: "0 3 1 * *"  # Monthly
  
  # Backup locations
  backup:
    primary_path: "/mnt/backup/primary"
    secondary_path: "/mnt/backup/secondary"
    archive_bucket: "s3://themisdb-backups"
  
  # DR environment
  dr_site:
    enabled: true
    endpoint: "dr.themisdb.example.com"
    capacity_check: true
  
  # Notifications
  notifications:
    enabled: true
    on_failure: true
    on_success: false
    recipients:
      - ops-team@example.com
      - dr-lead@example.com
```

---

## Related Documentation

- [DR Checklists](DR_CHECKLISTS.md)
- [Operations Handbook](../OPERATIONS_HANDBOOK.md)
- [Operational Procedures](../OPERATIONAL_PROCEDURES.md)
- [Audit Findings Report](../../../audit-reports/v1.4.1/FINDINGS_AND_RISKS.md)

---

**Document Version:** 1.5.0  
**Compliance:** ISO 27001 A.17.1, BSI C5 BCR-01  
**Last Reviewed:** 2026-02-03
