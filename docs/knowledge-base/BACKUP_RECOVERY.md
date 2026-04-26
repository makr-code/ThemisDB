# ThemisDB Backup & Recovery Guide

Complete guide for backing up and recovering ThemisDB data with zero data loss.

## Table of Contents

- [Backup Strategies](#backup-strategies)
- [Online vs Offline Backups](#online-vs-offline-backups)
- [Point-in-Time Recovery](#point-in-time-recovery)
- [Disaster Recovery Planning](#disaster-recovery-planning)
- [Backup Verification](#backup-verification)
- [Restore Procedures](#restore-procedures)
- [Cross-Region Backups](#cross-region-backups)
- [Automation Scripts](#automation-scripts)

---

## Backup Strategies

### Full Backup

**Complete database snapshot:**

```bash
# Basic full backup
themisdb-backup \
  --backup-directory /backups/themisdb/full/$(date +%Y%m%d_%H%M%S) \
  --database production \
  --compress \
  --threads 8

# With encryption
themisdb-backup \
  --backup-directory /backups/themisdb/full/$(date +%Y%m%d_%H%M%S) \
  --database production \
  --compress \
  --encrypt \
  --encryption-key-file /etc/themisdb/backup.key \
  --threads 8

# All databases
themisdb-backup \
  --backup-directory /backups/themisdb/full/$(date +%Y%m%d_%H%M%S) \
  --all-databases \
  --compress \
  --include-system-collections
```

**Pros:**
- Simple to manage
- Fast restore
- Complete point-in-time snapshot

**Cons:**
- Requires significant storage
- Takes longer for large databases
- No granular recovery between backups

**Best For:** Small to medium databases (< 500 GB), weekly/monthly archival

---

### Incremental Backup

**Only backs up changes since last full backup:**

```bash
# Day 1: Full backup
themisdb-backup \
  --backup-directory /backups/themisdb/full/2024-01-24 \
  --database production \
  --type full \
  --compress

# Day 2-7: Incremental backups
themisdb-backup \
  --backup-directory /backups/themisdb/incremental/2024-01-25 \
  --database production \
  --type incremental \
  --base-backup /backups/themisdb/full/2024-01-24 \
  --compress

# Restore process (requires full + all incrementals)
themisdb-restore \
  --backup-directory /backups/themisdb/full/2024-01-24 \
  --incremental-directories \
    /backups/themisdb/incremental/2024-01-25 \
    /backups/themisdb/incremental/2024-01-26 \
  --database production
```

**Pros:**
- Faster backup (only changes)
- Less storage space
- More frequent backups possible

**Cons:**
- Slower restore (chain required)
- Backup chain complexity
- Cannot skip incrementals

**Best For:** Large databases (> 500 GB), daily backups

---

### Differential Backup

**Backs up changes since last full backup:**

```bash
# Week 1: Full backup
themisdb-backup \
  --backup-directory /backups/themisdb/full/week1 \
  --type full

# Days 2-7: Differential backups
for day in {2..7}; do
  themisdb-backup \
    --backup-directory /backups/themisdb/diff/week1_day${day} \
    --type differential \
    --base-backup /backups/themisdb/full/week1
done

# Restore (only need full + latest differential)
themisdb-restore \
  --backup-directory /backups/themisdb/full/week1 \
  --differential-directory /backups/themisdb/diff/week1_day7
```

**Pros:**
- Simpler restore than incremental
- Good balance of speed/space
- Skip intermediate backups

**Cons:**
- Grows larger each day
- More storage than incremental

**Best For:** Medium databases, balance between backup speed and restore speed

---

### Snapshot Backup

**Filesystem/storage-level snapshots:**

```bash
# LVM snapshot
lvcreate -L 50G -s -n themisdb-snapshot /dev/vg0/themisdb-data

# Mount snapshot (read-only)
mkdir /mnt/themisdb-snapshot
mount -o ro /dev/vg0/themisdb-snapshot /mnt/themisdb-snapshot

# Backup from snapshot
tar -czf /backups/themisdb-snapshot-$(date +%Y%m%d).tar.gz \
  -C /mnt/themisdb-snapshot .

# Remove snapshot after backup
umount /mnt/themisdb-snapshot
lvremove -f /dev/vg0/themisdb-snapshot

# ZFS snapshot
zfs snapshot tank/themisdb@backup-$(date +%Y%m%d)
zfs send tank/themisdb@backup-$(date +%Y%m%d) | gzip > /backups/themisdb.zfs.gz

# Btrfs snapshot
btrfs subvolume snapshot -r /var/lib/themisdb /var/lib/themisdb-snapshot-$(date +%Y%m%d)
```

**Pros:**
- Instant snapshot
- Minimal impact on database
- Consistent point-in-time

**Cons:**
- Requires specific filesystem
- Additional storage management
- OS-level dependency

**Best For:** Very large databases (> 1 TB), minimal backup window

---

### Continuous Backup (WAL Archiving)

**Archive transaction logs for point-in-time recovery:**

```yaml
# themisdb.conf
wal:
  enabled: true
  directory: /var/lib/themisdb/wal
  
  # Archive to separate location
  archiveMode: on
  archiveCommand: |
    cp %p /backups/themisdb/wal_archive/%f && \
    s3cmd put %p s3://backups/themisdb/wal/%f

  archiveTimeout: 300  # Force archive every 5 minutes
  maxWalSize: 1GB
```

**Archive Script:**

```bash
#!/bin/bash
# wal_archive.sh

WAL_FILE=$1
ARCHIVE_DIR="/backups/themisdb/wal_archive"
S3_BUCKET="s3://backups/themisdb/wal"

# Local archive
cp "$WAL_FILE" "$ARCHIVE_DIR/"

# Remote archive
aws s3 cp "$WAL_FILE" "$S3_BUCKET/"

# Verify
if [[ $? -eq 0 ]]; then
  echo "$(date): Archived $WAL_FILE" >> /var/log/themisdb/wal_archive.log
  exit 0
else
  echo "$(date): Failed to archive $WAL_FILE" >> /var/log/themisdb/wal_archive.log
  exit 1
fi
```

**Pros:**
- Minimal data loss (seconds)
- Point-in-time recovery
- Continuous protection

**Cons:**
- Additional storage
- More complex restore
- Network dependency

**Best For:** Critical production systems, RPO < 1 minute

---

## Online vs Offline Backups

### Online Backups (Hot Backup)

**Backup while database is running:**

```bash
# Create consistent backup without stopping database
themisdb-backup \
  --backup-directory /backups/themisdb/online/$(date +%Y%m%d_%H%M%S) \
  --database production \
  --online \
  --consistent-snapshot \
  --compress

# Monitor backup progress
themisdb-admin backup-status

# For minimal impact, limit I/O
ionice -c 3 themisdb-backup \
  --backup-directory /backups/themisdb/ \
  --database production \
  --online \
  --throttle 50MB/s
```

**Advantages:**
- Zero downtime
- 24/7 backup capability
- No service interruption

**Considerations:**
- May impact performance during backup
- Requires consistent snapshot capability
- Longer backup time due to active writes

**⚠️ Warning:** Use `--consistent-snapshot` to ensure data consistency

---

### Offline Backups (Cold Backup)

**Backup with database stopped:**

```bash
#!/bin/bash
# offline_backup.sh

# 1. Stop database
systemctl stop themisdb

# 2. Verify stopped
while pgrep themisdb-server > /dev/null; do
  echo "Waiting for database to stop..."
  sleep 2
done

# 3. Perform backup
BACKUP_DIR="/backups/themisdb/offline/$(date +%Y%m%d_%H%M%S)"
mkdir -p "$BACKUP_DIR"

# Copy data directory
tar -czf "$BACKUP_DIR/data.tar.gz" /var/lib/themisdb/

# Copy configuration
cp /etc/themisdb/themisdb.conf "$BACKUP_DIR/"

# Copy logs
tar -czf "$BACKUP_DIR/logs.tar.gz" /var/log/themisdb/

# 4. Calculate checksums
cd "$BACKUP_DIR"
sha256sum * > checksums.txt

# 5. Start database
systemctl start themisdb

# 6. Verify started
until curl -f http://localhost:8529/_api/version; do
  echo "Waiting for database to start..."
  sleep 2
done

echo "Backup complete: $BACKUP_DIR"
```

**Advantages:**
- Guaranteed consistency
- Faster backup
- Simpler process

**Considerations:**
- Requires downtime
- Not suitable for 24/7 systems

**Best For:** Maintenance windows, development/staging environments

---

## Point-in-Time Recovery

### PITR Setup

**Enable WAL archiving:**

```yaml
# themisdb.conf
wal:
  enabled: true
  archiveMode: on
  archiveCommand: "cp %p /backups/themisdb/wal/%f"
  maxWalSize: 1GB
  keepWalFiles: 10

# Alternatively, archive to S3
wal:
  archiveCommand: "aws s3 cp %p s3://backups/themisdb/wal/$(date +%Y%m%d)/%f"
```

---

### Recovery to Specific Time

**Restore to exact point in time:**

```bash
#!/bin/bash
# pitr_recovery.sh

TARGET_TIME="2024-01-24 14:30:00"
BASE_BACKUP="/backups/themisdb/full/2024-01-24"
WAL_ARCHIVE="/backups/themisdb/wal_archive"

echo "Performing PITR to: $TARGET_TIME"

# 1. Stop database
systemctl stop themisdb

# 2. Backup current state (safety)
mv /var/lib/themisdb /var/lib/themisdb.pre-pitr

# 3. Restore base backup
themisdb-restore \
  --backup-directory "$BASE_BACKUP" \
  --target /var/lib/themisdb

# 4. Create recovery configuration
cat > /var/lib/themisdb/recovery.conf << EOF
restore_command = 'cp $WAL_ARCHIVE/%f %p'
recovery_target_time = '$TARGET_TIME'
recovery_target_action = promote
EOF

# 5. Start recovery
themisdb-server \
  --database.path /var/lib/themisdb \
  --database.recovery-mode

# Wait for recovery to complete
tail -f /var/log/themisdb/themisdb.log | grep -m 1 "recovery complete"

# 6. Start normal operations
systemctl start themisdb

# 7. Verify recovery point
RECOVERED_TIME=$(themisdb-admin info | grep lastAppliedTimestamp | cut -d: -f2)
echo "Recovered to: $RECOVERED_TIME"
```

**💡 Pro Tip:** Always test PITR in non-production before relying on it.

---

### Recovery Timeline

```
Full Backup          Incremental     Incremental     CRASH
(Day 1)             (Day 2)         (Day 3)         (Day 3, 14:35)
    |                   |               |               |
    └───────────────────┴───────────────┴───────────────┘
                        WAL Files
                        
Recovery Process:
1. Restore full backup (Day 1)
2. Apply incremental backup (Day 2)
3. Apply incremental backup (Day 3)
4. Replay WAL files until 14:30 (5 minutes before crash)

Result: Database state at 2024-01-24 14:30:00
```

---

### Automated PITR Testing

```bash
#!/bin/bash
# test_pitr.sh

echo "=== Testing PITR Capability ==="

# 1. Create test data with timestamp
TIMESTAMP=$(date +%s)
themisdb-shell << EOF
db._databases();
db._useDatabase('test');
db.pitr_test.save({timestamp: $TIMESTAMP, data: 'PITR test'});
EOF

echo "Inserted test record with timestamp: $TIMESTAMP"

# 2. Wait 60 seconds
echo "Waiting 60 seconds..."
sleep 60

# 3. Insert more data (this should not be recovered)
themisdb-shell << EOF
db._useDatabase('test');
db.pitr_test.save({timestamp: $(date +%s), data: 'After PITR target'});
EOF

# 4. Perform PITR to first timestamp
TARGET_TIME=$(date -d "@$TIMESTAMP" "+%Y-%m-%d %H:%M:%S")
./pitr_recovery.sh "$TARGET_TIME"

# 5. Verify recovery
RECOVERED_COUNT=$(themisdb-shell --quiet << EOF
db._useDatabase('test');
db._query("FOR doc IN pitr_test FILTER doc.timestamp == $TIMESTAMP RETURN doc").toArray().length
EOF
)

if [[ $RECOVERED_COUNT -eq 1 ]]; then
  echo "✓ PITR test passed"
else
  echo "✗ PITR test failed"
fi
```

---

## Disaster Recovery Planning

### Disaster Recovery Objectives

**Define your requirements:**

| Metric | Description | Example Target |
|--------|-------------|----------------|
| **RTO** (Recovery Time Objective) | Maximum acceptable downtime | 4 hours |
| **RPO** (Recovery Point Objective) | Maximum acceptable data loss | 15 minutes |
| **RLO** (Recovery Level Objective) | Minimum service level after recovery | 80% capacity |

---

### DR Strategy Matrix

**Select strategy based on requirements:**

| Strategy | RTO | RPO | Cost | Complexity |
|----------|-----|-----|------|------------|
| Backup/Restore | 4-8 hours | 24 hours | $ | Low |
| Backup/Restore + WAL | 2-4 hours | 15 minutes | $$ | Medium |
| Warm Standby | 30 minutes | 5 minutes | $$$ | Medium |
| Hot Standby (Multi-region) | 1 minute | 0 seconds | $$$$ | High |

---

### Disaster Recovery Plan

**Complete DR runbook:**

```markdown
# ThemisDB Disaster Recovery Plan

## Emergency Contacts
- DBA Team: +1-555-0100 (24/7)
- Infrastructure: +1-555-0101
- Security: +1-555-0102

## Disaster Scenarios

### Scenario 1: Hardware Failure (Single Server)
**RTO:** 2 hours | **RPO:** 15 minutes

1. Declare incident
2. Provision new hardware
3. Restore from latest backup
4. Apply WAL files
5. Verify data integrity
6. Update DNS/load balancer
7. Resume operations

### Scenario 2: Data Corruption
**RTO:** 4 hours | **RPO:** 1 hour

1. Assess corruption extent
2. Stop application writes
3. Identify last good backup
4. Perform PITR to pre-corruption time
5. Verify recovered data
6. Resume operations

### Scenario 3: Complete Site Loss
**RTO:** 4 hours | **RPO:** 30 minutes

1. Activate DR site
2. Restore from off-site backup
3. Apply WAL from remote archive
4. Update application endpoints
5. Verify functionality
6. Resume operations

### Scenario 4: Ransomware Attack
**RTO:** 8 hours | **RPO:** 24 hours

1. Isolate infected systems
2. Verify backup integrity
3. Clean environment
4. Restore from verified backup
5. Security audit
6. Resume operations
```

---

### DR Testing Schedule

```bash
#!/bin/bash
# dr_test_schedule.sh

# Quarterly full DR test
0 2 1 */3 * /opt/themisdb/dr/full_dr_test.sh

# Monthly restore test
0 3 1 * * /opt/themisdb/dr/restore_test.sh

# Weekly backup verification
0 4 * * 0 /opt/themisdb/dr/verify_backups.sh

# Daily PITR capability test
0 5 * * * /opt/themisdb/dr/test_pitr.sh
```

**Full DR Test Script:**

```bash
#!/bin/bash
# full_dr_test.sh

echo "=== Disaster Recovery Test ==="
echo "Date: $(date)"

# 1. Provision DR environment
echo "1. Provisioning DR environment..."
terraform apply -auto-approve -var-file=dr-environment.tfvars

# 2. Restore from backup
echo "2. Restoring from backup..."
LATEST_BACKUP=$(ls -t /backups/themisdb/full/ | head -1)
themisdb-restore \
  --backup-directory "/backups/themisdb/full/$LATEST_BACKUP" \
  --target-host dr-server:8529

# 3. Apply WAL files
echo "3. Applying WAL files..."
themisdb-admin wal-replay \
  --host dr-server:8529 \
  --wal-directory /backups/themisdb/wal_archive/

# 4. Run verification tests
echo "4. Running verification tests..."
./verify_dr_restore.sh dr-server:8529

# 5. Measure RTO
RECOVERY_TIME=$SECONDS
echo "Recovery Time: $RECOVERY_TIME seconds (Target: 7200 seconds)"

if [[ $RECOVERY_TIME -lt 7200 ]]; then
  echo "✓ RTO met"
else
  echo "✗ RTO exceeded"
fi

# 6. Cleanup DR environment
echo "6. Cleaning up DR environment..."
terraform destroy -auto-approve -var-file=dr-environment.tfvars

echo "=== DR Test Complete ==="
```

---

## Backup Verification

### Integrity Checking

**Verify backup integrity:**

```bash
#!/bin/bash
# verify_backup.sh

BACKUP_DIR=$1

echo "Verifying backup: $BACKUP_DIR"

# 1. Check checksums
echo "1. Verifying checksums..."
cd "$BACKUP_DIR"
sha256sum -c checksums.txt || exit 1

# 2. Test decompression
echo "2. Testing decompression..."
for file in *.gz; do
  gunzip -t "$file" || exit 1
done

# 3. Verify backup metadata
echo "3. Checking metadata..."
themisdb-backup-info --backup-directory "$BACKUP_DIR"

# 4. Test restore (to temporary location)
echo "4. Testing restore..."
TMP_DIR=$(mktemp -d)
themisdb-restore \
  --backup-directory "$BACKUP_DIR" \
  --target "$TMP_DIR" \
  --verify-only

# 5. Quick data sampling
echo "5. Sampling data..."
themisdb-server \
  --database.path "$TMP_DIR" \
  --server.endpoint none \
  --database.verify-only

# Cleanup
rm -rf "$TMP_DIR"

echo "✓ Backup verification passed"
```

---

### Automated Verification

```bash
#!/bin/bash
# automated_backup_verification.sh

# Run daily to verify last night's backup

BACKUP_DIR=$(ls -td /backups/themisdb/full/* | head -1)

echo "Daily Backup Verification - $(date)"
echo "Backup: $BACKUP_DIR"

# Verify backup
if ./verify_backup.sh "$BACKUP_DIR"; then
  echo "✓ Backup verification passed" | mail -s "Backup OK" admin@company.com
else
  echo "✗ Backup verification FAILED" | mail -s "ALERT: Backup Failed" admin@company.com
  exit 1
fi

# Update verification log
echo "$(date -Iseconds): $BACKUP_DIR - OK" >> /var/log/themisdb/backup_verification.log
```

---

### Restore Testing

**Regular restore tests:**

```bash
#!/bin/bash
# monthly_restore_test.sh

echo "=== Monthly Restore Test ==="

# 1. Select random backup from last month
RANDOM_BACKUP=$(ls /backups/themisdb/full/ | shuf -n 1)
echo "Testing backup: $RANDOM_BACKUP"

# 2. Provision test environment
docker run -d \
  --name themisdb-restore-test \
  -v /tmp/restore-test:/var/lib/themisdb \
  themisdb/themisdb:latest

# 3. Restore backup
themisdb-restore \
  --backup-directory "/backups/themisdb/full/$RANDOM_BACKUP" \
  --target-host localhost:8529

# 4. Run data validation queries
themisdb-shell << EOF
// Verify collection counts
const collections = db._collections();
collections.forEach(coll => {
  print(\`\${coll.name()}: \${coll.count()} documents\`);
});

// Verify indexes
collections.forEach(coll => {
  const indexes = coll.getIndexes();
  print(\`\${coll.name()}: \${indexes.length} indexes\`);
});

// Sample query test
db._query("FOR doc IN users LIMIT 100 RETURN doc");
EOF

# 5. Performance test
themisdb-bench \
  --host localhost:8529 \
  --workload read \
  --duration 60 \
  --threads 4

# 6. Cleanup
docker stop themisdb-restore-test
docker rm themisdb-restore-test

echo "✓ Restore test complete"
```

---

## Restore Procedures

### Full Database Restore

**Complete restore procedure:**

```bash
#!/bin/bash
# full_restore.sh

set -e

BACKUP_DIR=$1
TARGET_DIR="/var/lib/themisdb"

if [[ -z "$BACKUP_DIR" ]]; then
  echo "Usage: $0 <backup-directory>"
  exit 1
fi

echo "=== Full Database Restore ==="
echo "Backup: $BACKUP_DIR"
echo "Target: $TARGET_DIR"

read -p "This will OVERWRITE existing data. Continue? (yes/no): " CONFIRM

if [[ "$CONFIRM" != "yes" ]]; then
  echo "Aborted."
  exit 1
fi

# 1. Stop database
echo "1. Stopping database..."
systemctl stop themisdb

# 2. Backup current data (safety)
echo "2. Backing up current state..."
mv "$TARGET_DIR" "${TARGET_DIR}.pre-restore.$(date +%s)"

# 3. Restore data
echo "3. Restoring data..."
themisdb-restore \
  --backup-directory "$BACKUP_DIR" \
  --target "$TARGET_DIR" \
  --verbose

# 4. Verify restored data
echo "4. Verifying restored data..."
themisdb-server \
  --database.path "$TARGET_DIR" \
  --database.verify-only

# 5. Fix permissions
echo "5. Fixing permissions..."
chown -R themisdb:themisdb "$TARGET_DIR"

# 6. Start database
echo "6. Starting database..."
systemctl start themisdb

# 7. Wait for startup
echo "7. Waiting for database..."
until curl -f http://localhost:8529/_api/version 2>/dev/null; do
  sleep 2
done

# 8. Run post-restore checks
echo "8. Running post-restore checks..."
themisdb-admin verify-all

echo "✓ Restore complete"
```

---

### Selective Collection Restore

**Restore specific collections:**

```bash
# Restore single collection
themisdb-restore \
  --backup-directory /backups/themisdb/full/2024-01-24 \
  --database production \
  --collection users \
  --target-collection users_restored

# Restore multiple collections
for coll in users orders products; do
  themisdb-restore \
    --backup-directory /backups/themisdb/full/2024-01-24 \
    --database production \
    --collection $coll
done

# Restore with data transformation
themisdb-restore \
  --backup-directory /backups/themisdb/full/2024-01-24 \
  --database production \
  --collection users \
  --transform-script /opt/themisdb/transform_users.js
```

---

### Cross-Version Restore

**Restore from older version:**

```bash
#!/bin/bash
# cross_version_restore.sh

OLD_BACKUP="/backups/themisdb/v1.3.5/full/2024-01-24"
NEW_VERSION="1.4.0"

# 1. Restore to temporary location
TEMP_DIR="/tmp/themisdb-migration"
themisdb-restore \
  --backup-directory "$OLD_BACKUP" \
  --target "$TEMP_DIR"

# 2. Start old version database
docker run -d \
  --name themisdb-old \
  -v "$TEMP_DIR:/var/lib/themisdb" \
  themisdb/themisdb:1.3.5

# 3. Export data in portable format
themisdb-admin export \
  --host localhost:8529 \
  --database production \
  --output-directory /tmp/export \
  --format jsonl

# 4. Stop old version
docker stop themisdb-old

# 5. Start new version
docker run -d \
  --name themisdb-new \
  -p 8529:8529 \
  themisdb/themisdb:$NEW_VERSION

# 6. Import data
themisdb-admin import \
  --host localhost:8529 \
  --database production \
  --input-directory /tmp/export \
  --create-collections

# 7. Verify
themisdb-admin verify-all

echo "Cross-version restore complete: 1.3.5 → $NEW_VERSION"
```

---

## Cross-Region Backups

### Setup Cross-Region Replication

**AWS S3 cross-region backup:**

```bash
#!/bin/bash
# setup_cross_region_backup.sh

PRIMARY_REGION="us-east-1"
DR_REGION="us-west-2"
BUCKET_NAME="themisdb-backups"

# 1. Create buckets in both regions
aws s3 mb s3://${BUCKET_NAME}-${PRIMARY_REGION} --region $PRIMARY_REGION
aws s3 mb s3://${BUCKET_NAME}-${DR_REGION} --region $DR_REGION

# 2. Enable versioning
aws s3api put-bucket-versioning \
  --bucket ${BUCKET_NAME}-${PRIMARY_REGION} \
  --versioning-configuration Status=Enabled

# 3. Setup replication
cat > replication-config.json << EOF
{
  "Role": "arn:aws:iam::123456789:role/s3-replication-role",
  "Rules": [{
    "Status": "Enabled",
    "Priority": 1,
    "Filter": {},
    "Destination": {
      "Bucket": "arn:aws:s3:::${BUCKET_NAME}-${DR_REGION}",
      "ReplicationTime": {
        "Status": "Enabled",
        "Time": {
          "Minutes": 15
        }
      }
    }
  }]
}
EOF

aws s3api put-bucket-replication \
  --bucket ${BUCKET_NAME}-${PRIMARY_REGION} \
  --replication-configuration file://replication-config.json

echo "Cross-region replication configured"
```

---

### Backup to Multiple Regions

```bash
#!/bin/bash
# multi_region_backup.sh

BACKUP_DIR="/backups/themisdb/$(date +%Y%m%d_%H%M%S)"
REGIONS=("us-east-1" "us-west-2" "eu-west-1")

# 1. Create local backup
themisdb-backup \
  --backup-directory "$BACKUP_DIR" \
  --database production \
  --compress \
  --encrypt

# 2. Upload to all regions in parallel
for region in "${REGIONS[@]}"; do
  (
    echo "Uploading to $region..."
    aws s3 sync "$BACKUP_DIR" \
      s3://themisdb-backups-$region/$(date +%Y%m%d)/ \
      --region $region \
      --storage-class STANDARD_IA
    
    echo "✓ Upload to $region complete"
  ) &
done

# Wait for all uploads
wait

echo "Multi-region backup complete"

# 3. Verify uploads
for region in "${REGIONS[@]}"; do
  echo "Verifying $region..."
  aws s3 ls s3://themisdb-backups-$region/$(date +%Y%m%d)/ --region $region
done
```

---

### Cross-Region Restore

```bash
#!/bin/bash
# cross_region_restore.sh

DR_REGION="us-west-2"
BACKUP_DATE="2024-01-24"

echo "Restoring from DR region: $DR_REGION"

# 1. Download backup from DR region
aws s3 sync \
  s3://themisdb-backups-$DR_REGION/$BACKUP_DATE/ \
  /tmp/dr-restore/ \
  --region $DR_REGION

# 2. Verify download
cd /tmp/dr-restore/
sha256sum -c checksums.txt || exit 1

# 3. Restore
themisdb-restore \
  --backup-directory /tmp/dr-restore/ \
  --target /var/lib/themisdb/

# 4. Cleanup
rm -rf /tmp/dr-restore/

echo "Cross-region restore complete"
```

---

## Automation Scripts

### Daily Backup Script

```bash
#!/bin/bash
# daily_backup.sh

set -e

BACKUP_BASE="/backups/themisdb"
DATE=$(date +%Y%m%d)
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
RETENTION_DAYS=30

# Logging
exec 1> >(logger -s -t themisdb-backup) 2>&1

echo "Starting daily backup: $TIMESTAMP"

# Full backup on Sundays, incremental otherwise
if [[ $(date +%u) -eq 7 ]]; then
  BACKUP_TYPE="full"
  BACKUP_DIR="$BACKUP_BASE/full/$DATE"
else
  BACKUP_TYPE="incremental"
  BACKUP_DIR="$BACKUP_BASE/incremental/$DATE"
  LAST_FULL=$(ls -td $BACKUP_BASE/full/* | head -1)
fi

# Create backup
if [[ "$BACKUP_TYPE" == "full" ]]; then
  themisdb-backup \
    --backup-directory "$BACKUP_DIR" \
    --all-databases \
    --compress \
    --encrypt \
    --threads 8 \
    --verbose
else
  themisdb-backup \
    --backup-directory "$BACKUP_DIR" \
    --all-databases \
    --type incremental \
    --base-backup "$LAST_FULL" \
    --compress \
    --encrypt \
    --threads 8 \
    --verbose
fi

# Verify backup
if ./verify_backup.sh "$BACKUP_DIR"; then
  echo "✓ Backup successful: $BACKUP_DIR"
else
  echo "✗ Backup verification failed!"
  exit 1
fi

# Upload to S3
aws s3 sync "$BACKUP_DIR" \
  s3://themisdb-backups/$(hostname)/$DATE/ \
  --storage-class STANDARD_IA

# Cleanup old backups
find $BACKUP_BASE/incremental/* -mtime +$RETENTION_DAYS -exec rm -rf {} \;
find $BACKUP_BASE/full/* -mtime +90 -exec rm -rf {} \;  # Keep full backups 90 days

# Archive old WAL files
find /var/lib/themisdb/wal/* -mtime +7 -exec \
  aws s3 cp {} s3://themisdb-backups/$(hostname)/wal/ \; -delete

echo "Daily backup complete"
```

---

### Backup Monitoring Script

```bash
#!/bin/bash
# monitor_backups.sh

# Check if daily backup completed successfully

EXPECTED_BACKUP="/backups/themisdb/*/$(date +%Y%m%d)*"
BACKUP_AGE_HOURS=24

# Find latest backup
LATEST_BACKUP=$(ls -td $EXPECTED_BACKUP 2>/dev/null | head -1)

if [[ -z "$LATEST_BACKUP" ]]; then
  echo "CRITICAL: No backup found for today" | \
    mail -s "ALERT: Backup Missing" admin@company.com
  exit 2
fi

# Check backup age
BACKUP_TIME=$(stat -c %Y "$LATEST_BACKUP")
CURRENT_TIME=$(date +%s)
AGE_HOURS=$(( ($CURRENT_TIME - $BACKUP_TIME) / 3600 ))

if [[ $AGE_HOURS -gt $BACKUP_AGE_HOURS ]]; then
  echo "WARNING: Latest backup is $AGE_HOURS hours old" | \
    mail -s "WARNING: Old Backup" admin@company.com
  exit 1
fi

# Verify backup integrity
if ! ./verify_backup.sh "$LATEST_BACKUP"; then
  echo "CRITICAL: Backup verification failed" | \
    mail -s "ALERT: Backup Corrupted" admin@company.com
  exit 2
fi

echo "OK: Backup is recent and valid"
exit 0
```

---

### Automated Restore Testing

```bash
#!/bin/bash
# automated_restore_test.sh

# Weekly automated restore test

TEST_ENV="restore-test-$(date +%Y%m%d)"
LATEST_BACKUP=$(ls -td /backups/themisdb/full/* | head -1)

echo "=== Automated Restore Test ==="
echo "Backup: $LATEST_BACKUP"
echo "Test Environment: $TEST_ENV"

# 1. Provision test environment
docker run -d \
  --name $TEST_ENV \
  -p 9529:8529 \
  themisdb/themisdb:latest

# 2. Restore backup
if ! themisdb-restore \
  --backup-directory "$LATEST_BACKUP" \
  --target-host localhost:9529; then
  
  echo "CRITICAL: Restore failed!" | \
    mail -s "ALERT: Restore Test Failed" admin@company.com
  exit 1
fi

# 3. Run validation tests
if ! ./validate_restored_data.sh localhost:9529; then
  echo "CRITICAL: Data validation failed!" | \
    mail -s "ALERT: Restore Validation Failed" admin@company.com
  exit 1
fi

# 4. Cleanup
docker stop $TEST_ENV
docker rm $TEST_ENV

echo "✓ Automated restore test passed" | \
  mail -s "OK: Weekly Restore Test Passed" admin@company.com
```

---

## Quick Reference

### Backup Commands Cheatsheet

```bash
# Full backup
themisdb-backup --backup-directory /backups/full/$(date +%Y%m%d) --all-databases

# Incremental backup
themisdb-backup --type incremental --base-backup /backups/full/latest

# Single collection backup
themisdb-backup --database mydb --collection users --output users_backup.jsonl

# Compressed encrypted backup
themisdb-backup --compress --encrypt --encryption-key-file /etc/themisdb/key

# Verify backup
themisdb-restore --verify-only --backup-directory /backups/full/20240124

# Restore
themisdb-restore --backup-directory /backups/full/20240124

# PITR
themisdb-admin wal-replay --target-time "2024-01-24 14:30:00"
```

---

### Backup Schedule Template

```
Daily:    Incremental backup + WAL archiving
Weekly:   Full backup (Sunday)
Monthly:  Full backup + off-site copy
Quarterly: DR test with full restore
Annually:  DR full-site failover test

Retention:
- Daily incrementals: 7 days
- Weekly full: 4 weeks
- Monthly full: 12 months
- Yearly archive: 7 years
```

---

**Last Updated:** 2026-04-06  
**Version:** 1.4.0  
**Maintainer:** ThemisDB Team
