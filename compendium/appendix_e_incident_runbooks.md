# Appendix E: Incident Response Runbooks

> "Ein Runbook erspart dir in der Krise 10 Stunden Debugging. Schreib sie jetzt, nutze sie später."

---

## Überblick

Detaillierte Playbooks für die häufigsten Production-Incidents in ThemisDB: Symptome, Diagnose, Fixes, Verification, Kommunikation.

---

## E.1 Incident: Query Latency > 1s (p99)

### Symptom
- Alerts: `themis_query_latency_p99_gt_1000ms` für >5 Min
- User Report: "Suche dauert 10+ Sekunden"

### Diagnose (5 min)
```bash
# 1. Verify Alert ist real (nicht Sensor-Fehler)
curl -s http://localhost:8529/_admin/stats | jq '.queries[-10:] | .[].latency_ms'

# 2. Finde Slow Query
curl -s http://localhost:8529/_admin/slow-queries?threshold=500 | jq '.queries[0:3]'

# 3. Prüfe Collection-Größe
curl -s http://localhost:8529/_api/collection/users/counts | jq '.count'

# 4. Hole EXPLAIN Plan
aql "EXPLAIN <query>"
```

### Root Cause Analysis (Pick one)

**A. Missing Index**
```aql
-- EXPLAIN zeigt CollectionScan?
EXPLAIN FOR u IN users FILTER u.email == 'alice@example.com' RETURN u
-- Output: type: "CollectionScan" → NO INDEX

-- Fix:
CREATE INDEX idx_email ON users(email)

-- Verify:
EXPLAIN ... -- sollte IndexNode zeigen
```

**B. Poor Index Choice**
```aql
-- EXPLAIN zeigt wrong Index?
-- (z.B. idx_created_at bei Filter auf status)

-- Fix:
DROP INDEX users.old_index
CREATE INDEX idx_status_created ON users(status, created_at)

-- Query-Hint:
FOR u IN users
  FILTER u.status == 'active'
  SORT u.created_at DESC
  OPTIONS {indexHint: 'idx_status_created'}
  RETURN u
```

**C. Huge Result Set (Memory Pressure)**
```aql
-- ❌ Returns 1M rows
FOR u IN users
  FILTER u.status == 'active'
  RETURN u  -- No LIMIT!

-- ✅ Paginated
FOR u IN users
  FILTER u.status == 'active'
  LIMIT @offset, 100
  RETURN u
```

**D. Expensive Operations (Regex, Full-Text)**
```aql
-- ❌ Regex auf 1M Docs
FOR u IN users
  FILTER u.name =~ '.*alice.*'
  RETURN u

-- ✅ Use Index + Full-Text
FOR u IN users
  FILTER u.status == 'active'  -- Reduce set first
  SEARCH u.name IN FULLTEXT('alice')  -- Then search
  RETURN u
```

### Fix (1-5 min)
```bash
# Option 1: Add Index (immediate)
aql "CREATE INDEX idx_fix ON collection(field)"

# Option 2: Patch Query
# → Update client code with LIMIT / Projection

# Option 3: Increase Cache
# → Restart with larger cache_size_mb in themis.conf

# Option 4: Load Rebalance
# → Migrate queries to different node
```

### Verify (2 min)
```bash
# 1. Run same query
time aql "<query>"  # Should be <100ms now

# 2. Check Alert
# Should clear within next evaluation window

# 3. Check Error Logs
journalctl -u themis -n 20 | grep ERROR
```

### Communicate
- Slack: "Query latency incident resolved: Added index on `collection.field`. P99 now 50ms."
- Incident Ticket: Mark RESOLVED, link to index creation timestamp

---

## E.2 Incident: High Memory Usage (RSS > 80%)

### Symptom
- Memory Usage Gauge: > 85% of available
- OOM Killer might start: `systemctl status themis | grep killed`

### Diagnose (3 min)
```bash
# 1. Check Memory
free -h
top -p $(pgrep themisdb)

# 2. Cache Usage
curl -s http://localhost:8529/_admin/cache | jq '.usage_mb'

# 3. Big Query?
curl -s http://localhost:8529/_admin/slow-queries?threshold=1 | jq '.queries[].memory_mb'

# 4. Cursor Leaks?
curl -s http://localhost:8529/_admin/cursors | jq 'length'
```

### Root Cause Analysis

**A. Large Result Set Materializing**
```aql
-- ❌ Materializes all rows
FOR doc IN large_collection
  FILTER doc.type == 'report'
  RETURN doc

-- ✅ Stream results
FOR doc IN large_collection
  FILTER doc.type == 'report'
  OPTIONS {stream: true}
  RETURN doc
```

**B. Cache Size Too Large**
```yaml
# themis.conf
cache:
  size_mb: 16384  # Too big for 32GB system
  
# Fix:
cache:
  size_mb: 8192   # ~25% of total RAM
```

**C. Cursor Leak (Clients not closing)**
```python
# ❌ Leak
cursor = client.query("FOR d IN collection RETURN d")
# Never called: cursor.close()

# ✅ Safe
with client.query("FOR d IN collection RETURN d") as cursor:
    for doc in cursor:
        process(doc)
```

### Fix (5-10 min)

**Quick Win: Restart**
```bash
sudo systemctl restart themis
# Memory drops to baseline, but queries restart
```

**Better: Graceful Drain**
```bash
# 1. Mark DB as read-only
curl -X POST http://localhost:8529/_admin/readonly

# 2. Wait for active queries to finish (30s timeout)
sleep 30

# 3. Restart
sudo systemctl restart themis

# 4. Re-enable writes
curl -X POST http://localhost:8529/_admin/writable
```

### Verify
```bash
# Check memory dropped
free -h
# Cache usage normalized
curl -s http://localhost:8529/_admin/cache | jq '.usage_mb'
```

---

## E.3 Incident: Replication Lag > 5s

### Symptom
- Alert: `themis_replication_lag_ms > 5000`
- Secondary reads stale data (>5s behind Primary)

### Diagnose (2 min)
```bash
# 1. Check Follower Lag
curl -s http://localhost:8529/_admin/replication/lag

# 2. Primary Writes Throughput
curl -s http://localhost:8529/_admin/stats | jq '.writes_per_sec'

# 3. Follower CPU/IO
ssh follower-node "top -bn1 | head -15"

# 4. Network Latency
ping -c 10 follower-ip | tail -1
```

### Root Cause Analysis

**A. Follower Network Slow / Packet Loss**
```bash
# Fix: Check network path
iperf3 -c follower-ip -t 10  # Should be >100 Mbps
mtr follower-ip  # Check for packet loss

# Solution: Add network interface / reduce hops
```

**B. Follower Disk IO Bottleneck**
```bash
# Diagnose
iostat -x 1 5 | grep sda

# If %util > 80%:
# - Reduce write throughput on primary
# - Add SSD to follower
# - Enable compression on WAL
```

**C. Follower CPU Saturated**
```bash
# Check CPU
top -p $(pgrep themis)

# Solution:
# - Add cores to follower
# - Reduce query load
# - Move read-heavy queries to Primary temporarily
```

### Fix (5-30 min)

**Short-term: Manual Catchup**
```bash
# On Follower: Increase WAL read buffer
aql "UPDATE config SET wal_read_buffer_mb = 256"

# Restart follower
sudo systemctl restart themis
# Lag should drop as follower catches up
```

**Medium-term: Throttle Primary**
```yaml
# themis.conf (Primary)
replication:
  max_writes_per_sec: 1000  # Slow down writes slightly
  follower_lag_warn_ms: 3000
```

### Verify
```bash
# Lag should drop
curl -s http://localhost:8529/_admin/replication/lag | jq '.lag_ms'

# Target: < 500ms for p99
```

---

## E.4 Incident: Deadlock Detected

### Symptom
- Error: `ERROR: Deadlock detected (Transaction A ↔ B)`
- Client Retry: Usually succeeds on 2nd attempt

### Diagnose (1 min)
```bash
# Check deadlock frequency
curl -s http://localhost:8529/_admin/stats | jq '.deadlocks_total'

# Should be: ~0 per hour (< 10 per day max)
# If frequent: architecture issue
```

### Root Cause Analysis

**A. Lock Ordering Inconsistency**
```aql
-- Transaction A:
BEGIN
  LOCK collection1
  LOCK collection2
COMMIT

-- Transaction B (opposite order - DEADLOCK):
BEGIN
  LOCK collection2
  LOCK collection1
COMMIT

-- Fix: Enforce strict lock ordering (e.g. alphabetical)
```

**B. Long Transactions**
```aql
-- ❌ 2-minute transaction
BEGIN
  FOR item IN items LIMIT 1000000
    UPDATE item WITH {updated: true} IN items
COMMIT

-- ✅ Break into batches
FOR batch_start IN 0..1000000 STEP 10000
  BEGIN
    FOR item IN items
      FILTER item._id >= batch_start AND item._id < batch_start + 10000
      UPDATE item WITH {updated: true}
  COMMIT
```

### Fix (Immediate)
- **Client Side:** Add exponential retry (most deadlocks resolve on 2nd attempt)
- **Code:** Review and fix lock ordering (alphabetical, same order everywhere)
- **Monitoring:** If deadlocks > 10/day, escalate

### Verify
```bash
# Deadlock rate should stay low
watch -n 60 'curl -s http://localhost:8529/_admin/stats | jq ".deadlocks_total"'
```

---

## E.5 Incident: Disk Space Critical (>95%)

### Symptom
- Alert: `themis_disk_usage_percent > 95`
- Cannot write new data

### Diagnose (2 min)
```bash
# 1. Check disk
df -h /data/themis

# 2. What's big?
du -sh /data/themis/* | sort -h

# 3. Log retention?
du -sh /var/log/themis*
```

### Root Cause Analysis

**A. WAL / Logs Explosion**
```bash
# Check sizes
ls -lh /data/themis/wal/
ls -lh /var/log/themis*

# Likely: Log rotation disabled or interval too long
```

**B. Large Data Import (Staging)**
```bash
# If just imported: Delete staging collections
aql "TRUNCATE staging_collection"
aql "DROP COLLECTION temp_*"
```

**C. Backup Staging Not Cleaned**
```bash
# Check
ls -lh /data/themis-backups/
# Delete old backups
rm /data/themis-backups/backup-*.tar.gz
```

### Fix (5-15 min)

**Immediate:**
```bash
# 1. Cleanup logs
journalctl --vacuum=2d

# 2. Cleanup old WAL
find /data/themis/wal -mtime +7 -delete

# 3. Truncate staging collections
aql "TRUNCATE staging"

# Verify
df -h /data/themis
# Should drop 10-50%
```

**Medium-term:**
```yaml
# themis.conf
logging:
  retention_days: 14
  max_file_size_mb: 100

wal:
  retention_files: 20  # Keep last 20 files
```

---

## E.6 Incident: Database Won't Start (Corruption)

### Symptom
- Startup Error: `ERROR: Corrupted block at offset 12345`
- Service Down: Cannot restart

### Diagnose (2 min)
```bash
# 1. Check logs
journalctl -u themis -n 50 | tail -20

# 2. Try recovery
themisdb recover /data/themis

# 3. Check disk
fsck /dev/sda1
```

### Root Cause Analysis

**A. Unclean Shutdown (Power Loss)**
- **Fix:** Run WAL recovery (automatic on restart)

**B. Disk Corruption (Bad Sector)**
- **Fix:** Replace disk, restore from backup

**C. RocksDB Internal Corruption (Rare)**
- **Fix:** Backup data, rebuild database

### Fix

**Option 1: Automatic Recovery (Most Common)**
```bash
# ThemisDB will auto-recover from WAL
sudo systemctl start themis

# Should boot successfully within 30s
```

**Option 2: Manual WAL Recovery**
```bash
# If auto-recovery fails
themisdb recover --wal-only /data/themis
sudo systemctl start themis
```

**Option 3: Restore from Backup**
```bash
# Last resort
sudo systemctl stop themis
tar -xzf /backup/themis-2026-01-01.tar.gz -C /data/
sudo chown -R themis:themis /data/themis
sudo systemctl start themis
```

### Verify
```bash
# Check startup logs
journalctl -u themis -n 20

# Verify data
aql "RETURN COUNT(*) FROM users"
```

---

## E.7 Incident: Security Alert: Unauthorized Access Attempt

### Symptom
- Alert: `themis_auth_failure_rate > 5_per_minute`
- Logs: `AUTH FAILED: user='admin' from_ip='1.2.3.4' password_attempts=10`

### Diagnose (1 min)
```bash
# 1. Check recent failures
curl -s http://localhost:8529/_admin/audit?event=auth_failure&last_1h | jq '.events[0:10]'

# 2. Identify attacker IP
cat /var/log/themis/auth.log | grep "FAILED" | cut -d' ' -f6 | sort | uniq -c | sort -rn

# 3. Check account
curl -s http://localhost:8529/_admin/users/admin | jq '.locked'
```

### Fix (2 min)

**Immediate:**
```bash
# 1. Block attacker IP (Firewall)
sudo ufw insert 1 deny from 1.2.3.4

# 2. Lock compromised account
aql "UPDATE {name: 'admin'} WITH {locked: true, locked_reason: 'Brute force attempt'} IN users"

# 3. Force password reset
# Send admin new temporary password

# 4. Enable MFA
aql "UPDATE {name: 'admin'} WITH {mfa_required: true} IN users"
```

**Escalation:**
- Notify security team
- Create incident ticket
- Schedule post-mortem

### Verify
```bash
# Auth failures should drop to 0
watch "tail -20 /var/log/themis/auth.log | grep FAILED | wc -l"
```

---

## E.8 Incident Severity Levels & Escalation

### Severity 1 (Critical - All Hands)
- Database completely down (no connections)
- Data corruption/loss detected
- All writes failing

**Escalation:** On-call Lead + CTO + Data Team

### Severity 2 (High - Urgent)
- Performance severely degraded (>10s latency)
- Replication lag > 30s
- Memory/Disk pressure critical

**Escalation:** On-call Engineer + Manager

### Severity 3 (Medium - Standard)
- Single query slow (1-5s)
- Moderate lag (5-30s)
- Non-critical errors in logs

**Escalation:** On-call Engineer

### Severity 4 (Low - Backlog)
- Single error/retry
- Performance degraded slightly
- Informational alerts

**Escalation:** Backlog Ticket

---

## E.9 Communication Template

### Incident Declared
```
🚨 INCIDENT: Query Latency High
Status: INVESTIGATING
Severity: SEV-2
Started: 2026-01-01 12:00:00 UTC
Impact: 5-10% of users experiencing slow search
```

### Root Cause Found
```
✅ ROOT CAUSE IDENTIFIED: Missing index on users.email
Fix: CREATE INDEX idx_email ON users(email)
ETA Resolution: 5 minutes
```

### Resolved
```
✅ INCIDENT RESOLVED
Resolution: Index created, queries now <100ms
Duration: 15 minutes
Post-mortem: Thursday 10am
```

---

## Summary

Keep this runbook near your on-call terminal. Most incidents fall into the categories above. When an alert fires:
1. **Navigate to right section**
2. **Run Diagnose commands**
3. **Match Root Cause**
4. **Execute Fix**
5. **Verify & Communicate**

Average resolution time with this playbook: **10-15 minutes** vs **60+ minutes** without.
