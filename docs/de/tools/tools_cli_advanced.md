# ThemisDB CLI Tools - Advanced Usage

**Version:** 1.0.0  
**Stand:** 6. April 2026  
**Kategorie:** Tools  
**Status:** ✅ Produktionsreif

---

## 📑 Inhaltsverzeichnis

- [Performance Tuning](#performance-tuning)
- [Scripting und Automation](#scripting-und-automation)
- [Monitoring und Profiling](#monitoring-und-profiling)
- [Cluster Management](#cluster-management)
- [Advanced Import/Export](#advanced-importexport)
- [Troubleshooting](#troubleshooting)

---

## Performance Tuning

### Query Optimization

```bash
# Query execution plan analysieren
themis-cli --server localhost:8765 << 'EOF'
EXPLAIN
FOR doc IN users
  FILTER doc.age > 30 AND doc.city == "Berlin"
  SORT doc.age DESC
  LIMIT 100
  RETURN doc
EOF
```

**Output:**
```
Execution Plan:
  1. IndexNode (idx_city_age) - Estimated: 1000 docs
  2. FilterNode (age > 30) - Estimated: 800 docs
  3. SortNode (age DESC) - In-Memory Sort
  4. LimitNode (100)
  5. ReturnNode

Estimated Cost: 1250
Indexes Used: idx_city_age (composite)
```

### Query Profiling

```bash
# Query mit Profiling ausführen
themis-cli --profile << 'EOF'
FOR doc IN large_collection
  FILTER doc.category == "electronics"
  LET reviews = (
    FOR review IN reviews
      FILTER review.product_id == doc._key
      RETURN review
  )
  RETURN {doc, reviews}
EOF
```

**Output:**
```
Profile Results:
┌──────────────────┬──────────┬──────────┬──────────┐
│ Node             │ Calls    │ Time     │ Items    │
├──────────────────┼──────────┼──────────┼──────────┤
│ IndexNode        │ 1        │ 12ms     │ 50,000   │
│ FilterNode       │ 1        │ 45ms     │ 5,000    │
│ SubqueryNode     │ 5,000    │ 1,234ms  │ 25,000   │
│ ReturnNode       │ 1        │ 8ms      │ 5,000    │
├──────────────────┼──────────┼──────────┼──────────┤
│ Total            │          │ 1,299ms  │ 5,000    │
└──────────────────┴──────────┴──────────┴──────────┘

Optimization Hints:
  ⚠️  Subquery called 5,000 times - Consider JOIN or materialization
  ✅  Index used efficiently
  💡  Total memory: 45 MB
```

### Batch Processing

```bash
# Große Operationen in Batches
themis-admin collection process \
  --collection large_collection \
  --batch-size 10000 \
  --parallel 8 \
  --query '
    FOR doc IN @@collection
      FILTER doc.status == "pending"
      UPDATE doc WITH {status: "processed"} IN @@collection
  '
```

---

## Scripting und Automation

### Shell Scripts

```bash
#!/bin/bash
# maintenance.sh - Nightly maintenance script

set -e

SERVER="localhost:8765"
LOG_FILE="/var/log/themis-maintenance.log"

log() {
    echo "[$(date +'%Y-%m-%d %H:%M:%S')] $1" | tee -a "$LOG_FILE"
}

# 1. Backup
log "Starting backup..."
themis-backup create \
  --server "$SERVER" \
  --output "/backups/themis-$(date +%Y%m%d).tar.gz" \
  --compress gzip

# 2. Optimize indexes
log "Optimizing indexes..."
for collection in users products orders; do
    themis-admin collection optimize \
      --server "$SERVER" \
      --collection "$collection" \
      --reindex
done

# 3. Cleanup old data
log "Cleaning up old data..."
themis-cli --server "$SERVER" << 'EOF'
FOR doc IN audit_logs
  FILTER doc.timestamp < DATE_SUBTRACT(DATE_NOW(), 30, "day")
  REMOVE doc IN audit_logs
EOF

# 4. Update statistics
log "Updating statistics..."
themis-admin stats update --server "$SERVER"

# 5. Health check
if themis-admin health --server "$SERVER" --quiet; then
    log "Maintenance completed successfully"
else
    log "ERROR: Health check failed"
    exit 1
fi
```

### Python Integration

```python
#!/usr/bin/env python3
# advanced_operations.py

import subprocess
import json
import sys

def run_query(query, bind_vars=None):
    """Execute AQL query via CLI"""
    cmd = ['themis-cli', '--server', 'localhost:8765', '--format', 'json']
    
    # Build query with bind vars
    full_query = query
    if bind_vars:
        bind_str = json.dumps(bind_vars)
        full_query = f"LET vars = {bind_str}\n{query}"
    
    cmd.extend(['--query', full_query])
    
    result = subprocess.run(cmd, capture_output=True, text=True)
    if result.returncode != 0:
        raise Exception(f"Query failed: {result.stderr}")
    
    return json.loads(result.stdout)

def bulk_update_with_transformation(collection, transform_fn):
    """Update documents with custom transformation"""
    # Fetch documents in batches
    batch_size = 1000
    offset = 0
    
    while True:
        # Get batch
        docs = run_query(f"""
            FOR doc IN {collection}
              LIMIT {offset}, {batch_size}
              RETURN doc
        """)
        
        if not docs:
            break
        
        # Transform
        updates = []
        for doc in docs:
            new_doc = transform_fn(doc)
            if new_doc != doc:
                updates.append({
                    '_key': doc['_key'],
                    'update': new_doc
                })
        
        # Update batch
        if updates:
            run_query(f"""
                FOR item IN @updates
                  UPDATE item._key WITH item.update IN {collection}
            """, {'updates': updates})
        
        offset += batch_size
        print(f"Processed {offset} documents...")

# Example transformation
def normalize_email(doc):
    if 'email' in doc:
        doc['email'] = doc['email'].lower().strip()
    return doc

if __name__ == '__main__':
    bulk_update_with_transformation('users', normalize_email)
```

### Cron Jobs

```cron
# /etc/cron.d/themisdb

# Hourly backups
0 * * * * themis-user themis-backup create --server localhost:8765 --output /backups/hourly.tar.gz

# Daily maintenance at 2 AM
0 2 * * * themis-user /usr/local/bin/maintenance.sh

# Weekly index optimization
0 3 * * 0 themis-user themis-admin collection optimize --all

# Monthly cleanup
0 4 1 * * themis-user themis-cli --query "FOR doc IN logs FILTER doc.date < DATE_SUBTRACT(DATE_NOW(), 90, 'day') REMOVE doc IN logs"
```

---

## Monitoring und Profiling

### Real-time Monitoring

```bash
# Watch server metrics
watch -n 5 'themis-admin stats --server localhost:8765 --format table'

# Monitor active queries
watch -n 2 'themis-admin queries list --server localhost:8765 --running'

# Connection monitoring
themis-admin monitor connections --server localhost:8765 --interval 5s
```

### Performance Analysis

```bash
# Slow query log analysis
themis-admin queries slow \
  --server localhost:8765 \
  --min-duration 1000 \
  --output slow_queries.json

# Analyze slow queries
jq -r '.[] | "\(.duration)ms - \(.query)"' slow_queries.json | sort -rn | head -10

# Index usage statistics
themis-admin index stats \
  --server localhost:8765 \
  --collection users \
  --format table
```

### Memory Profiling

```bash
# Memory breakdown
themis-admin memory analyze --server localhost:8765

# Top memory consumers
themis-admin memory top --limit 10

# Detect memory leaks
themis-admin memory leak-check --duration 1h
```

---

## Cluster Management

### Multi-Node Operations

```bash
# Execute on all nodes
for node in node1:8765 node2:8765 node3:8765; do
    echo "Processing $node..."
    themis-admin health --server $node
done

# Parallel execution
parallel -j 3 themis-admin health --server {} ::: \
    node1:8765 node2:8765 node3:8765
```

### Shard Management

```bash
# List shards
themis-admin shard list --server localhost:8765

# Rebalance shards
themis-admin shard rebalance \
  --server localhost:8765 \
  --collection users \
  --strategy even

# Shard status
themis-admin shard status \
  --server localhost:8765 \
  --collection products \
  --detailed
```

### Leader Election

```bash
# Check leader status
themis-admin cluster leader --server localhost:8765

# Force leader election
themis-admin cluster elect \
  --server localhost:8765 \
  --node node2

# Cluster health
themis-admin cluster health --server localhost:8765
```

---

## Advanced Import/Export

### Streaming Import

```bash
# Import from stdin (streaming)
curl https://api.example.com/data/stream | \
  themis-import \
    --server localhost:8765 \
    --collection streaming_data \
    --format jsonl \
    --stdin

# Import with transformation
cat data.json | \
  jq -c '.[] | {_key: .id, name: .title, value: .price}' | \
  themis-import \
    --server localhost:8765 \
    --collection products \
    --format jsonl \
    --stdin
```

### Parallel Export

```bash
# Export in parallel chunks
parallel -j 4 themis-export \
  --server localhost:8765 \
  --query "FOR doc IN large_collection FILTER doc.shard == {} RETURN doc" \
  --output "chunk_{}.json" \
  ::: 0 1 2 3

# Merge chunks
jq -s 'add' chunk_*.json > complete.json
```

### ETL Pipeline

```bash
#!/bin/bash
# etl_pipeline.sh - Extract, Transform, Load

# 1. Extract from source
themis-export \
  --server source:8765 \
  --collection raw_data \
  --output /tmp/extract.json

# 2. Transform
cat /tmp/extract.json | \
  jq 'map({
    _key: .id,
    name: .full_name | ascii_downcase,
    email: .email_address | ascii_downcase,
    created: .timestamp | strptime("%Y-%m-%d") | mktime
  })' > /tmp/transform.json

# 3. Load to destination
themis-import \
  --server destination:8765 \
  --collection clean_data \
  --file /tmp/transform.json \
  --on-duplicate update

# 4. Validate
SOURCE_COUNT=$(themis-cli --server source:8765 --query "RETURN COUNT(FOR doc IN raw_data RETURN 1)" | jq '.')
DEST_COUNT=$(themis-cli --server destination:8765 --query "RETURN COUNT(FOR doc IN clean_data RETURN 1)" | jq '.')

if [ "$SOURCE_COUNT" -eq "$DEST_COUNT" ]; then
    echo "✅ ETL completed successfully: $DEST_COUNT records"
else
    echo "❌ ETL validation failed: $SOURCE_COUNT != $DEST_COUNT"
    exit 1
fi
```

---

## Troubleshooting

### Debug Mode

```bash
# Enable verbose logging
themis-cli --server localhost:8765 --debug --query "FOR doc IN users RETURN doc"

# Trace network requests
themis-cli --server localhost:8765 --trace --query "FOR doc IN users RETURN doc"

# Full debug with timing
themis-cli \
  --server localhost:8765 \
  --debug \
  --timing \
  --memory-tracking \
  --query "FOR doc IN users RETURN doc"
```

### Connection Issues

```bash
# Test connectivity
themis-admin ping --server localhost:8765

# Check TLS certificate
themis-cli --server localhost:8765 --ssl --ssl-verify --ssl-debug

# Connection pool stats
themis-admin connection stats --server localhost:8765

# Force reconnect
themis-admin connection reset --server localhost:8765
```

### Query Debugging

```bash
# Explain query plan
themis-cli --explain << 'EOF'
FOR doc IN users
  FILTER doc.age > @age
  RETURN doc
EOF

# Validate query syntax
themis-cli --validate --query "FOR doc IN users RETRN doc"
# Error: Syntax error near 'RETRN' (expected 'RETURN')

# Find missing indexes
themis-admin index suggest \
  --server localhost:8765 \
  --collection users \
  --query "FOR doc IN users FILTER doc.email == @email RETURN doc"
```

### Performance Issues

```bash
# Identify slow queries
themis-admin queries slow --min-duration 1000

# Kill problematic query
themis-admin queries kill --id 12345

# Check resource usage
themis-admin resources \
  --server localhost:8765 \
  --watch \
  --interval 1s

# Analyze table locks
themis-admin locks list --server localhost:8765
```

---

## Best Practices

### 1. Error Handling in Scripts

```bash
#!/bin/bash
set -euo pipefail  # Exit on error, undefined vars, pipe failures

# Function for error handling
handle_error() {
    echo "Error on line $1"
    # Cleanup
    themis-admin connection close --all
    exit 1
}

trap 'handle_error $LINENO' ERR

# Your operations here
themis-cli --query "..."
```

### 2. Secure Credential Management

```bash
# Use environment variables
export THEMIS_SERVER="localhost:8765"
export THEMIS_USERNAME="admin"
export THEMIS_PASSWORD="$(cat /secure/password.txt)"

# Or use credential file
cat > ~/.themisrc << EOF
server=localhost:8765
username=admin
password_command=pass show themisdb/admin
EOF

chmod 600 ~/.themisrc

# Use in commands
themis-cli --config ~/.themisrc
```

### 3. Logging and Auditing

```bash
# Log all operations
exec > >(tee -a /var/log/themis-ops.log)
exec 2>&1

echo "Starting operation at $(date)"
themis-cli --query "..."
echo "Completed operation at $(date)"
```

---

## Siehe auch

- [Getting Started Guide](tools_cli_getting_started.md)
- [Dashboard Documentation](tools_dashboard.md)
- [Admin Tools](../../de/admin_tools/admin_guide.md)
- [Production Operations](../production/PRODUCTION_OPERATIONS.md)
