# ThemisDB Migration & Upgrade Guide

Complete guide for migrating data and upgrading ThemisDB between versions.

## Table of Contents

- [Upgrading Between Versions](#upgrading-between-versions)
- [Breaking Changes Checklist](#breaking-changes-checklist)
- [Data Migration Strategies](#data-migration-strategies)
- [Zero-Downtime Upgrades](#zero-downtime-upgrades)
- [Rolling Updates](#rolling-updates)
- [Rollback Procedures](#rollback-procedures)
- [Configuration Migration](#configuration-migration)
- [Testing Migrations](#testing-migrations)

---

## Upgrading Between Versions

### Pre-Upgrade Checklist

**Before starting any upgrade:**

```bash
# 1. Review release notes
curl https://raw.githubusercontent.com/ThemisDB/ThemisDB/main/CHANGELOG.md

# 2. Check current version
themisdb-server --version

# 3. Backup everything
themisdb-backup \
  --backup-directory /backups/themisdb/pre-upgrade-$(date +%Y%m%d) \
  --compress \
  --include-config

# 4. Verify backup integrity
themisdb-restore --verify-only \
  --backup-directory /backups/themisdb/pre-upgrade-$(date +%Y%m%d)

# 5. Export critical data
themisdb-admin export \
  --database production \
  --collection critical_data \
  --output /tmp/critical_backup.jsonl

# 6. Document current configuration
cp /etc/themisdb/themisdb.conf /etc/themisdb/themisdb.conf.backup
themisdb-admin config-dump > /tmp/current_config.json

# 7. Check disk space (need 2x data size)
df -h /var/lib/themisdb/

# 8. Review system requirements
curl https://docs.themisdb.org/requirements.txt
```

**⚠️ Critical Warning:** Always test upgrades in a non-production environment first!

---

### Minor Version Upgrades (1.4.0 → 1.4.x)

**Procedure:**

```bash
# 1. Stop database
systemctl stop themisdb

# 2. Install new version
# Debian/Ubuntu
wget https://download.themisdb.org/themisdb-1.4.5.deb
dpkg -i themisdb-1.4.5.deb

# RHEL/CentOS
wget https://download.themisdb.org/themisdb-1.4.5.rpm
rpm -Uvh themisdb-1.4.5.rpm

# 3. Run upgrade check
themisdb-server --database.auto-upgrade \
  --database.check-version \
  --database.path /var/lib/themisdb/

# 4. Start database
systemctl start themisdb

# 5. Verify upgrade
themisdb-server --version
curl http://localhost:8529/_api/version

# 6. Check logs for errors
journalctl -u themisdb -n 100 --no-pager

# 7. Run integrity check
themisdb-admin verify-all --database production
```

**Expected Duration:** 5-15 minutes (depending on data size)

---

### Major Version Upgrades (1.3.x → 1.4.x)

**More complex due to potential breaking changes:**

```bash
# 1. Read upgrade guide
curl https://docs.themisdb.org/upgrades/1.3-to-1.4.md > upgrade_guide.md
less upgrade_guide.md

# 2. Check compatibility
themisdb-compat-check \
  --from-version 1.3.5 \
  --to-version 1.4.0 \
  --database-path /var/lib/themisdb/

# Output example:
# ✓ Data format: Compatible
# ⚠ Configuration: 2 deprecated options
# ✗ API: 3 breaking changes
# ✓ Indexes: Compatible

# 3. Fix compatibility issues
# Update configuration
sed -i 's/storage.journalSize/storage.rocksdb.walSize/g' /etc/themisdb/themisdb.conf

# 4. Export data (safety measure)
themisdb-admin export-all \
  --output-directory /tmp/full-export/ \
  --include-system-collections

# 5. Stop database
systemctl stop themisdb

# 6. Backup database directory
tar -czf /backups/themisdb-1.3.5-data.tar.gz /var/lib/themisdb/

# 7. Install new version
dpkg -i themisdb-1.4.0.deb

# 8. Run migration scripts
themisdb-migrate \
  --from-version 1.3.5 \
  --to-version 1.4.0 \
  --database-path /var/lib/themisdb/ \
  --verbose

# 9. Upgrade database format
themisdb-server --database.auto-upgrade \
  --database.path /var/lib/themisdb/

# 10. Start database
systemctl start themisdb

# 11. Comprehensive verification
./verify_upgrade.sh
```

**Expected Duration:** 30-120 minutes (depending on data size)

---

### Verification Script

**Create comprehensive verification:**

```bash
#!/bin/bash
# verify_upgrade.sh

set -e

echo "=== Post-Upgrade Verification ==="

# 1. Version check
echo "1. Checking version..."
VERSION=$(curl -s http://localhost:8529/_api/version | jq -r '.version')
echo "   Current version: $VERSION"

if [[ ! "$VERSION" == "1.4.0" ]]; then
  echo "   ✗ Version mismatch!"
  exit 1
fi
echo "   ✓ Version correct"

# 2. Service health
echo "2. Checking service health..."
systemctl is-active --quiet themisdb && echo "   ✓ Service running" || exit 1

# 3. Database connectivity
echo "3. Testing database connectivity..."
curl -s -f http://localhost:8529/_api/version > /dev/null && echo "   ✓ Database responding" || exit 1

# 4. Collection counts
echo "4. Verifying collection counts..."
COUNTS_BEFORE=$(cat /tmp/collection_counts_before.json)
COUNTS_AFTER=$(curl -s http://localhost:8529/_api/collection | jq '[.result[] | {name: .name, count: .count}]')

if [[ "$COUNTS_BEFORE" != "$COUNTS_AFTER" ]]; then
  echo "   ⚠ Collection counts changed"
  diff <(echo "$COUNTS_BEFORE") <(echo "$COUNTS_AFTER")
else
  echo "   ✓ Collection counts match"
fi

# 5. Index verification
echo "5. Verifying indexes..."
themisdb-admin verify-indexes --all-collections

# 6. Sample queries
echo "6. Running sample queries..."
curl -s -X POST http://localhost:8529/_api/cursor \
  -H "Content-Type: application/json" \
  -d '{"query": "FOR doc IN users LIMIT 10 RETURN doc"}' > /dev/null && \
  echo "   ✓ Queries working" || exit 1

# 7. Write test
echo "7. Testing write operations..."
curl -s -X POST http://localhost:8529/_api/document/test_collection \
  -H "Content-Type: application/json" \
  -d '{"test": true, "timestamp": "'$(date -Iseconds)'"}' > /dev/null && \
  echo "   ✓ Writes working" || exit 1

# 8. Replication status (if clustered)
if themisdb-admin cluster-status 2>/dev/null; then
  echo "8. Checking replication..."
  themisdb-admin replication-status | grep -q "in-sync" && \
    echo "   ✓ Replication healthy" || echo "   ⚠ Replication lag detected"
fi

echo ""
echo "=== Verification Complete ==="
echo "✓ Upgrade successful!"
```

---

## Breaking Changes Checklist

### Version 1.3 → 1.4

**Configuration Changes:**

```yaml
# REMOVED (1.3)
storage.journalSize: 32MB

# REPLACED WITH (1.4)
storage.rocksdb.walSize: 32MB

# NEW REQUIRED (1.4)
storage.rocksdb.compression: lz4
```

**Migration Script:**

```bash
#!/bin/bash
# migrate_config_1.3_to_1.4.sh

CONFIG_FILE="/etc/themisdb/themisdb.conf"
BACKUP="${CONFIG_FILE}.pre-1.4-backup"

# Backup original
cp "$CONFIG_FILE" "$BACKUP"

# Update configuration
sed -i 's/storage\.journalSize/storage.rocksdb.walSize/g' "$CONFIG_FILE"
sed -i 's/cache\.maxSize/cache.size/g' "$CONFIG_FILE"

# Add new required options
if ! grep -q "storage.rocksdb.compression" "$CONFIG_FILE"; then
  echo "storage.rocksdb.compression: lz4" >> "$CONFIG_FILE"
fi

echo "Configuration migrated from 1.3 to 1.4"
echo "Backup saved to: $BACKUP"
echo "Please review: $CONFIG_FILE"
```

---

**API Changes:**

| 1.3 API | Status | 1.4 API | Notes |
|---------|--------|---------|-------|
| `/_api/collection/create` | Deprecated | `/_api/collection` (POST) | Old API still works with warning |
| `/_admin/log` | Removed | `/_admin/log/entries` | Must update client code |
| `/_api/simple/all` | Removed | Use AQL queries | No replacement |
| Response format | Changed | Includes metadata | Check error handling |

**Client Code Migration:**

```javascript
// 1.3 Code (Deprecated)
const result = await db._connection.POST('/_api/collection/create', {
  name: 'new_collection',
  type: 2
});

// 1.4 Code (Current)
const result = await db._connection.POST('/_api/collection', {
  name: 'new_collection',
  type: 2  // 2 = document, 3 = edge
});

// 1.3 Simple API (Removed)
const docs = await db._connection.PUT('/_api/simple/all', {
  collection: 'users'
});

// 1.4 AQL (Required)
const docs = await db._query('FOR doc IN users RETURN doc');
```

---

**Index Changes:**

```javascript
// Check for affected indexes
db._databases().forEach(dbName => {
  db._useDatabase(dbName);
  
  db._collections().forEach(coll => {
    const indexes = coll.getIndexes();
    
    indexes.forEach(idx => {
      // Skiplist indexes deprecated in 1.4
      if (idx.type === 'skiplist') {
        console.log(`⚠️  ${coll.name()}: Skiplist index '${idx.name}' should be converted to persistent`);
        
        // Migration
        coll.dropIndex(idx.id);
        coll.ensureIndex({
          type: 'persistent',
          fields: idx.fields,
          unique: idx.unique,
          sparse: idx.sparse
        });
      }
    });
  });
});
```

---

**Query Language Changes:**

```aql
-- 1.3: FULLTEXT function (Deprecated)
FOR doc IN FULLTEXT(articles, "content", "search term")
  RETURN doc

-- 1.4: SEARCH operator (Recommended)
FOR doc IN articles
  SEARCH ANALYZER(doc.content IN TOKENS("search term", "text_en"), "text_en")
  RETURN doc

-- 1.3: WITHIN (Removed)
FOR doc IN locations
  FILTER WITHIN(doc, 40.7128, -74.0060, 10000)
  RETURN doc

-- 1.4: GEO_DISTANCE (Required)
FOR doc IN locations
  FILTER GEO_DISTANCE([doc.longitude, doc.latitude], [-74.0060, 40.7128]) <= 10000
  RETURN doc
```

---

## Data Migration Strategies

### Strategy 1: In-Place Upgrade (Downtime Required)

**Best For:**
- Single server deployments
- Small to medium datasets (< 100 GB)
- Acceptable maintenance window (1-2 hours)

**Procedure:**

```bash
# 1. Announce maintenance
echo "Scheduled maintenance: 2024-01-24 02:00-04:00 UTC" | mail -s "ThemisDB Upgrade" users@company.com

# 2. Stop application traffic
# Update load balancer or DNS
curl -X POST http://loadbalancer/api/drain/database-server

# 3. Backup
themisdb-backup --backup-directory /backups/$(date +%Y%m%d-%H%M)

# 4. Stop database
systemctl stop themisdb

# 5. Upgrade
dpkg -i themisdb-1.4.0.deb

# 6. Migrate data
themisdb-migrate --verbose

# 7. Start database
systemctl start themisdb

# 8. Verify
./verify_upgrade.sh

# 9. Resume traffic
curl -X POST http://loadbalancer/api/enable/database-server
```

**Pros:** Simple, reliable  
**Cons:** Downtime required  
**Downtime:** 1-2 hours

---

### Strategy 2: Blue-Green Deployment (Zero Downtime)

**Best For:**
- Production systems requiring high availability
- Medium to large datasets
- Clustered deployments

**Architecture:**

```
                    ┌──────────────┐
                    │ Load Balancer│
                    └──────┬───────┘
                           │
              ┌────────────┴────────────┐
              │                         │
         ┌────▼─────┐            ┌─────▼────┐
         │  BLUE    │            │  GREEN   │
         │(Current) │            │  (New)   │
         │v1.3.5    │            │  v1.4.0  │
         └──────────┘            └──────────┘
```

**Procedure:**

```bash
#!/bin/bash
# blue_green_upgrade.sh

# 1. Setup green environment
echo "Setting up green environment..."
# Provision new server(s) with v1.4.0
ansible-playbook -i inventory/green setup_themisdb_1.4.0.yml

# 2. Replicate data to green
echo "Replicating data..."
themisdb-sync \
  --source blue-cluster:8529 \
  --target green-cluster:8529 \
  --continuous \
  --background

# 3. Monitor replication lag
while true; do
  LAG=$(themisdb-sync --check-lag)
  echo "Replication lag: $LAG seconds"
  
  if [[ $LAG -lt 10 ]]; then
    echo "Lag acceptable, proceeding..."
    break
  fi
  
  sleep 30
done

# 4. Stop replication, capture final state
themisdb-sync --stop
themisdb-sync --final-sync

# 5. Verify green environment
ssh green-cluster './verify_upgrade.sh'

# 6. Switch traffic (gradual)
echo "Switching 10% traffic to green..."
curl -X POST http://loadbalancer/api/weight/green/10

sleep 300  # Monitor for 5 minutes

echo "Switching 50% traffic to green..."
curl -X POST http://loadbalancer/api/weight/green/50

sleep 300

echo "Switching 100% traffic to green..."
curl -X POST http://loadbalancer/api/weight/green/100

# 7. Monitor green under full load
./monitor_health.sh --duration 1800  # 30 minutes

# 8. Decommission blue (keep as backup for 24h)
echo "Green is now primary. Blue kept as backup."
echo "To rollback: ./blue_green_rollback.sh"
```

**Pros:** Zero downtime, easy rollback  
**Cons:** Requires double infrastructure temporarily  
**Downtime:** 0 seconds

---

### Strategy 3: Rolling Update (Clustered)

**Best For:**
- Multi-node clusters (3+ nodes)
- Large datasets
- High availability requirements

**Procedure:**

```bash
#!/bin/bash
# rolling_update.sh

CLUSTER_NODES=("node1" "node2" "node3" "node4")
NEW_VERSION="1.4.0"

for NODE in "${CLUSTER_NODES[@]}"; do
  echo "=== Upgrading $NODE ==="
  
  # 1. Drain node
  echo "Draining $NODE..."
  themisdb-admin cluster-node-drain --node $NODE --wait
  
  # 2. Wait for replica sync
  while true; do
    STATUS=$(themisdb-admin cluster-node-status --node $NODE)
    if [[ "$STATUS" == "drained" ]]; then
      break
    fi
    sleep 10
  done
  
  # 3. Stop node
  ssh $NODE "systemctl stop themisdb"
  
  # 4. Upgrade
  ssh $NODE "dpkg -i /tmp/themisdb-${NEW_VERSION}.deb"
  
  # 5. Migrate data
  ssh $NODE "themisdb-migrate --verbose"
  
  # 6. Start node
  ssh $NODE "systemctl start themisdb"
  
  # 7. Wait for node to join cluster
  while true; do
    STATUS=$(themisdb-admin cluster-node-status --node $NODE)
    if [[ "$STATUS" == "healthy" ]]; then
      echo "$NODE is healthy"
      break
    fi
    sleep 10
  done
  
  # 8. Verify node
  ssh $NODE "./verify_upgrade.sh"
  
  # 9. Wait before next node
  echo "Waiting 5 minutes before upgrading next node..."
  sleep 300
done

echo "=== Rolling update complete ==="
echo "All nodes upgraded to $NEW_VERSION"
```

**Pros:** Zero downtime, gradual rollout  
**Cons:** Complex, requires cluster setup  
**Downtime:** 0 seconds

---

## Zero-Downtime Upgrades

### Requirements

**Infrastructure:**
- Load balancer with health checks
- Minimum 2 database servers
- Replication configured
- Monitoring in place

**Code Requirements:**
- Connection retry logic
- Graceful degradation
- Feature flags for new features

---

### Health Check Configuration

**Load Balancer Health Check:**

```nginx
# nginx.conf
upstream themisdb_cluster {
  server node1:8529 max_fails=3 fail_timeout=30s;
  server node2:8529 max_fails=3 fail_timeout=30s;
  server node3:8529 max_fails=3 fail_timeout=30s;
}

server {
  listen 8529;
  
  location / {
    proxy_pass http://themisdb_cluster;
    proxy_next_upstream error timeout http_502 http_503 http_504;
    proxy_connect_timeout 5s;
    proxy_send_timeout 60s;
    proxy_read_timeout 60s;
  }
  
  location /_health {
    access_log off;
    proxy_pass http://themisdb_cluster/_api/version;
    proxy_connect_timeout 2s;
    proxy_read_timeout 2s;
  }
}
```

**ThemisDB Health Endpoint:**

```bash
# Health check script
curl -f http://localhost:8529/_api/version && \
curl -f http://localhost:8529/_admin/cluster/health && \
themisdb-admin verify-replication-status
```

---

### Application Connection Handling

**Retry Logic:**

```javascript
// Node.js example with exponential backoff
const { Database } = require('themisdb');

async function connectWithRetry(maxRetries = 5) {
  for (let i = 0; i < maxRetries; i++) {
    try {
      const db = new Database({
        url: process.env.THEMISDB_URL,
        databaseName: 'production',
        auth: {
          username: process.env.THEMISDB_USER,
          password: process.env.THEMISDB_PASS
        }
      });
      
      // Test connection
      await db._query('RETURN 1');
      
      console.log('Connected to ThemisDB');
      return db;
      
    } catch (err) {
      const delay = Math.min(1000 * Math.pow(2, i), 30000);
      console.log(`Connection failed, retrying in ${delay}ms... (${i + 1}/${maxRetries})`);
      
      if (i === maxRetries - 1) throw err;
      
      await new Promise(resolve => setTimeout(resolve, delay));
    }
  }
}

// Use in application
const db = await connectWithRetry();
```

---

### Graceful Degradation

**Feature Flags:**

```javascript
// Feature flag system
const features = {
  newQueryAPI: process.env.THEMISDB_VERSION >= '1.4.0',
  advancedIndexes: process.env.THEMISDB_VERSION >= '1.4.0'
};

// Use appropriate API based on version
async function getUsers() {
  if (features.newQueryAPI) {
    // Use 1.4 API
    return await db._query('FOR u IN users RETURN u');
  } else {
    // Fall back to 1.3 API
    return await db._connection.PUT('/_api/simple/all', {
      collection: 'users'
    });
  }
}
```

---

## Rolling Updates

### Pre-requisites Check

```bash
#!/bin/bash
# check_rolling_update_ready.sh

echo "Checking rolling update prerequisites..."

# 1. Cluster size
NODE_COUNT=$(themisdb-admin cluster-nodes --count)
if [[ $NODE_COUNT -lt 3 ]]; then
  echo "✗ Need at least 3 nodes for rolling update (found: $NODE_COUNT)"
  exit 1
fi
echo "✓ Cluster size: $NODE_COUNT nodes"

# 2. Replication factor
REP_FACTOR=$(themisdb-admin cluster-config | jq '.replicationFactor')
if [[ $REP_FACTOR -lt 2 ]]; then
  echo "✗ Replication factor must be >= 2 (found: $REP_FACTOR)"
  exit 1
fi
echo "✓ Replication factor: $REP_FACTOR"

# 3. All nodes healthy
UNHEALTHY=$(themisdb-admin cluster-health | grep -c "unhealthy")
if [[ $UNHEALTHY -gt 0 ]]; then
  echo "✗ $UNHEALTHY unhealthy nodes detected"
  exit 1
fi
echo "✓ All nodes healthy"

# 4. No ongoing operations
PENDING=$(themisdb-admin cluster-pending-operations --count)
if [[ $PENDING -gt 0 ]]; then
  echo "⚠ $PENDING pending operations - wait for completion"
  exit 1
fi
echo "✓ No pending operations"

# 5. Sufficient storage
for NODE in $(themisdb-admin cluster-nodes --list); do
  USAGE=$(ssh $NODE "df -h /var/lib/themisdb | tail -1 | awk '{print \$5}' | sed 's/%//'")
  if [[ $USAGE -gt 80 ]]; then
    echo "✗ Node $NODE storage at ${USAGE}% (need < 80%)"
    exit 1
  fi
done
echo "✓ Storage capacity sufficient"

echo ""
echo "✓ All prerequisites met - ready for rolling update"
```

---

### Monitoring During Rolling Update

```bash
#!/bin/bash
# monitor_rolling_update.sh

watch -n 5 'echo "=== Cluster Status ===" && \
themisdb-admin cluster-health && \
echo -e "\n=== Node Versions ===" && \
for node in $(themisdb-admin cluster-nodes --list); do \
  echo -n "$node: "; \
  ssh $node "themisdb-server --version | head -1"; \
done && \
echo -e "\n=== Replication Lag ===" && \
themisdb-admin replication-status && \
echo -e "\n=== Query Performance ===" && \
curl -s http://localhost:8529/_admin/statistics | jq ".server.queryTime"'
```

---

## Rollback Procedures

### When to Rollback

**Trigger rollback if:**
- Critical bugs in new version
- Unacceptable performance degradation (>50%)
- Data inconsistencies detected
- Failed health checks after upgrade
- Application errors > 5% of requests

---

### Immediate Rollback (Single Server)

```bash
#!/bin/bash
# rollback_single_server.sh

set -e

echo "=== EMERGENCY ROLLBACK ==="
read -p "Are you sure? Type 'ROLLBACK' to confirm: " CONFIRM

if [[ "$CONFIRM" != "ROLLBACK" ]]; then
  echo "Aborted."
  exit 1
fi

# 1. Stop current version
systemctl stop themisdb

# 2. Restore previous version
dpkg -i /backups/themisdb-1.3.5.deb

# 3. Restore data from backup
tar -xzf /backups/themisdb-pre-upgrade-20240124.tar.gz -C /

# 4. Restore configuration
cp /etc/themisdb/themisdb.conf.backup /etc/themisdb/themisdb.conf

# 5. Start database
systemctl start themisdb

# 6. Verify
./verify_upgrade.sh

echo "=== Rollback complete ==="
echo "Database restored to version 1.3.5"
```

**⚠️ Warning:** This restores to backup point - any data written after backup is lost!

---

### Blue-Green Rollback

```bash
#!/bin/bash
# blue_green_rollback.sh

echo "Rolling back to blue environment..."

# 1. Stop replication from green to blue
themisdb-sync --stop

# 2. Switch all traffic back to blue
curl -X POST http://loadbalancer/api/weight/blue/100
curl -X POST http://loadbalancer/api/weight/green/0

# 3. Verify blue is serving traffic
for i in {1..10}; do
  curl -s http://loadbalancer/_api/version | jq '.version'
  sleep 1
done

# 4. Monitor blue under load
./monitor_health.sh --duration 600

echo "Rollback complete - traffic restored to blue"
echo "Green environment can be decomissioned"
```

**Pros:** Instant rollback, no data loss  
**Cons:** Requires blue environment maintained

---

### Rolling Rollback (Clustered)

```bash
#!/bin/bash
# rolling_rollback.sh

CLUSTER_NODES=("node1" "node2" "node3" "node4")
OLD_VERSION="1.3.5"

echo "Performing rolling rollback to v${OLD_VERSION}..."

for NODE in "${CLUSTER_NODES[@]}"; do
  echo "=== Rolling back $NODE ==="
  
  # 1. Check if node was upgraded
  CURRENT=$(ssh $NODE "themisdb-server --version | grep -oP '\d+\.\d+\.\d+'")
  
  if [[ "$CURRENT" == "$OLD_VERSION" ]]; then
    echo "$NODE already on $OLD_VERSION, skipping"
    continue
  fi
  
  # 2. Drain node
  themisdb-admin cluster-node-drain --node $NODE --wait
  
  # 3. Stop node
  ssh $NODE "systemctl stop themisdb"
  
  # 4. Restore old version
  ssh $NODE "dpkg -i /backups/themisdb-${OLD_VERSION}.deb"
  
  # 5. Restore data (if necessary)
  # Only if data format changed
  ssh $NODE "tar -xzf /backups/themisdb-data-pre-upgrade.tar.gz -C /"
  
  # 6. Start node
  ssh $NODE "systemctl start themisdb"
  
  # 7. Wait for node to rejoin
  while true; do
    STATUS=$(themisdb-admin cluster-node-status --node $NODE)
    if [[ "$STATUS" == "healthy" ]]; then
      echo "$NODE rolled back successfully"
      break
    fi
    sleep 10
  done
  
  sleep 120  # Wait before next node
done

echo "=== Rolling rollback complete ==="
```

---

### Point-in-Time Recovery (PITR)

**For data corruption after upgrade:**

```bash
#!/bin/bash
# pitr_recovery.sh

TARGET_TIMESTAMP="2024-01-24T01:30:00Z"

echo "Performing PITR to $TARGET_TIMESTAMP..."

# 1. Stop database
systemctl stop themisdb

# 2. Restore base backup
themisdb-restore \
  --backup-directory /backups/themisdb/daily/2024-01-24/ \
  --target /var/lib/themisdb/

# 3. Replay WAL logs up to target time
themisdb-admin wal-replay \
  --wal-directory /backups/themisdb/wal/ \
  --target-time "$TARGET_TIMESTAMP" \
  --database-path /var/lib/themisdb/

# 4. Start database
systemctl start themisdb

# 5. Verify timestamp
themisdb-admin info | grep "lastAppliedTimestamp"

echo "PITR complete - database restored to $TARGET_TIMESTAMP"
```

---

## Configuration Migration

### Automated Migration Tool

```python
#!/usr/bin/env python3
# migrate_config.py

import yaml
import sys
from pathlib import Path

def migrate_1_3_to_1_4(config):
    """Migrate configuration from 1.3 to 1.4 format"""
    
    changes = []
    
    # 1. Rename keys
    renames = {
        'storage.journalSize': 'storage.rocksdb.walSize',
        'cache.maxSize': 'cache.size',
        'server.maximalQueueSize': 'server.maxQueueSize'
    }
    
    for old_key, new_key in renames.items():
        if old_key in config:
            config[new_key] = config.pop(old_key)
            changes.append(f"Renamed {old_key} → {new_key}")
    
    # 2. Add required new options
    defaults = {
        'storage.rocksdb.compression': 'lz4',
        'server.statisticsEnabled': True,
        'query.spillToDisk': True
    }
    
    for key, value in defaults.items():
        if key not in config:
            config[key] = value
            changes.append(f"Added {key} = {value}")
    
    # 3. Remove deprecated options
    deprecated = ['server.requireAuthentication']
    
    for key in deprecated:
        if key in config:
            del config[key]
            changes.append(f"Removed deprecated {key}")
    
    return config, changes

def main():
    if len(sys.argv) != 3:
        print("Usage: migrate_config.py <input.conf> <output.conf>")
        sys.exit(1)
    
    input_file = Path(sys.argv[1])
    output_file = Path(sys.argv[2])
    
    # Load configuration
    with open(input_file, 'r') as f:
        config = yaml.safe_load(f)
    
    # Migrate
    migrated_config, changes = migrate_1_3_to_1_4(config)
    
    # Save migrated configuration
    with open(output_file, 'w') as f:
        yaml.dump(migrated_config, f, default_flow_style=False)
    
    # Report changes
    print(f"Configuration migrated: {input_file} → {output_file}")
    print(f"\nChanges made:")
    for change in changes:
        print(f"  • {change}")
    
    print(f"\nPlease review {output_file} before using!")

if __name__ == '__main__':
    main()
```

**Usage:**

```bash
# Migrate configuration
python3 migrate_config.py \
  /etc/themisdb/themisdb.conf \
  /etc/themisdb/themisdb.conf.1.4

# Review changes
diff /etc/themisdb/themisdb.conf /etc/themisdb/themisdb.conf.1.4

# Apply if correct
mv /etc/themisdb/themisdb.conf.1.4 /etc/themisdb/themisdb.conf
```

---

### Configuration Validation

```bash
# Validate configuration before starting
themisdb-server --validate-config /etc/themisdb/themisdb.conf

# Check for deprecated options
themisdb-admin config-check \
  --config /etc/themisdb/themisdb.conf \
  --target-version 1.4.0

# Example output:
# ✓ All required options present
# ⚠ 2 deprecated options found:
#   - storage.journalSize (use storage.rocksdb.walSize)
#   - cache.maxSize (use cache.size)
# ✗ 1 invalid option:
#   - server.invalidOption (unknown)
```

---

## Testing Migrations

### Test Environment Setup

```bash
#!/bin/bash
# setup_test_environment.sh

# 1. Create isolated test environment
docker run -d \
  --name themisdb-test-old \
  -p 8529:8529 \
  -v /tmp/themisdb-old:/var/lib/themisdb \
  themisdb/themisdb:1.3.5

# 2. Load production-like data
themisdb-admin import \
  --server http://localhost:8529 \
  --database test \
  --collection users \
  --file production_sample.jsonl

# 3. Create snapshot
docker commit themisdb-test-old themisdb-test-baseline

# 4. Test upgrade
docker stop themisdb-test-old
docker run -d \
  --name themisdb-test-new \
  -p 8529:8529 \
  -v /tmp/themisdb-old:/var/lib/themisdb \
  themisdb/themisdb:1.4.0

# 5. Run migration
docker exec themisdb-test-new themisdb-migrate --verbose

# 6. Verify
./verify_upgrade.sh
```

---

### Migration Test Suite

```bash
#!/bin/bash
# test_migration.sh

echo "=== Migration Test Suite ==="

# Test 1: Version check
echo "Test 1: Version verification..."
VERSION=$(curl -s http://localhost:8529/_api/version | jq -r '.version')
[[ "$VERSION" == "1.4.0" ]] && echo "✓ PASS" || echo "✗ FAIL"

# Test 2: Data integrity
echo "Test 2: Data integrity..."
HASH_BEFORE=$(cat /tmp/data_hash_before.txt)
HASH_AFTER=$(themisdb-admin data-hash --database test)
[[ "$HASH_BEFORE" == "$HASH_AFTER" ]] && echo "✓ PASS" || echo "✗ FAIL"

# Test 3: Query compatibility
echo "Test 3: Query compatibility..."
curl -X POST http://localhost:8529/_api/cursor \
  -d '{"query": "FOR doc IN users LIMIT 10 RETURN doc"}' \
  && echo "✓ PASS" || echo "✗ FAIL"

# Test 4: Index functionality
echo "Test 4: Index functionality..."
themisdb-admin verify-indexes --all-collections \
  && echo "✓ PASS" || echo "✗ FAIL"

# Test 5: Performance regression
echo "Test 5: Performance regression..."
TIME_BEFORE=$(cat /tmp/query_time_before.txt)
TIME_AFTER=$(curl -s http://localhost:8529/_admin/statistics | jq '.server.queryTime.avg')

REGRESSION=$(echo "scale=2; ($TIME_AFTER - $TIME_BEFORE) / $TIME_BEFORE * 100" | bc)

if (( $(echo "$REGRESSION < 20" | bc -l) )); then
  echo "✓ PASS (${REGRESSION}% change)"
else
  echo "✗ FAIL (${REGRESSION}% regression)"
fi

echo ""
echo "=== Test Suite Complete ==="
```

---

### Load Testing

```bash
# Simulate production load during migration test
themisdb-bench \
  --workload mixed \
  --threads 32 \
  --duration 3600 \
  --read-ratio 0.8 \
  --write-ratio 0.2 \
  --collection users \
  --report-interval 60 \
  > load_test_results.log &

# Monitor during test
watch -n 10 'curl -s http://localhost:8529/_admin/statistics | jq ".server | {opsPerSec, queryTime, cacheHitRate}"'
```

---

## Quick Reference

### Migration Checklist

```markdown
Pre-Migration:
- [ ] Read release notes and changelog
- [ ] Check system requirements
- [ ] Backup all data
- [ ] Backup configuration files
- [ ] Export critical data
- [ ] Document current state
- [ ] Test in non-production environment
- [ ] Schedule maintenance window
- [ ] Notify stakeholders

During Migration:
- [ ] Stop application traffic
- [ ] Verify backups
- [ ] Run compatibility checks
- [ ] Perform upgrade
- [ ] Run data migration
- [ ] Update configuration
- [ ] Start database
- [ ] Verify functionality
- [ ] Check logs for errors

Post-Migration:
- [ ] Run verification tests
- [ ] Monitor performance metrics
- [ ] Check replication status
- [ ] Resume application traffic
- [ ] Monitor error rates
- [ ] Document any issues
- [ ] Keep old version for 24-48h
- [ ] Update documentation

Emergency Rollback:
- [ ] Stop database
- [ ] Restore from backup
- [ ] Verify restored state
- [ ] Resume traffic
- [ ] Investigate failure cause
```

---

**Last Updated:** 2026-04-06  
**Version:** 1.4.0  
**Maintainer:** ThemisDB Team
