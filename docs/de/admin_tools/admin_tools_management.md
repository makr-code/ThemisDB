# Admin Tools - Management und Monitoring

**Version:** 1.0.0  
**Stand:** 6. April 2026  
**Kategorie:** Admin Tools  
**Status:** ✅ Produktionsreif

---

## 📑 Inhaltsverzeichnis

- [Übersicht](#übersicht)
- [User Management](#user-management)
- [Collection Management](#collection-management)
- [Backup und Recovery](#backup-und-recovery)
- [Performance Monitoring](#performance-monitoring)
- [Security Auditing](#security-auditing)
- [Praktische Beispiele](#praktische-beispiele)

---

## Übersicht

ThemisDB bietet umfassende Admin Tools für das Management und Monitoring der Datenbank.

### Verfügbare Tools

| Tool | Beschreibung | Zugriff |
|------|--------------|---------|
| **Web Dashboard** | Browser-basiertes Admin Interface | http://localhost:8765/admin |
| **CLI Tools** | Command-line Administration | `themis-admin` |
| **REST API** | Programmatischer Zugriff | `/api/v1/admin/*` |
| **Audit Log Viewer** | Log-Analyse und Audit | Dashboard + CLI |
| **Metrics Dashboard** | Performance Monitoring | Grafana Integration |

---

## User Management

### User erstellen

```bash
# CLI
themis-admin user create \
  --username alice \
  --password secret123 \
  --role user \
  --email alice@example.com

# REST API
curl -X POST http://localhost:8765/api/v1/admin/users \
  -u admin:secret \
  -H "Content-Type: application/json" \
  -d '{
    "username": "alice",
    "password": "secret123",
    "role": "user",
    "email": "alice@example.com",
    "permissions": ["read", "write"]
  }'
```

**Response:**
```json
{
  "success": true,
  "user_id": "user-12345",
  "username": "alice",
  "role": "user",
  "created_at": "2026-01-24T14:30:00Z"
}
```

### User auflisten

```bash
# CLI
themis-admin user list

# REST API
curl http://localhost:8765/api/v1/admin/users \
  -u admin:secret
```

**Output:**
```
┌──────────┬───────────┬────────┬────────────────────────┐
│ Username │ Role      │ Status │ Last Login             │
├──────────┼───────────┼────────┼────────────────────────┤
│ admin    │ admin     │ active │ 2026-01-24 14:25:00    │
│ alice    │ user      │ active │ 2026-01-24 12:00:00    │
│ bob      │ developer │ active │ 2026-01-23 18:30:00    │
└──────────┴───────────┴────────┴────────────────────────┘
```

### User Permissions aktualisieren

```bash
# CLI
themis-admin user update \
  --username alice \
  --role admin \
  --add-permissions "query,admin"

# REST API
curl -X PATCH http://localhost:8765/api/v1/admin/users/alice \
  -u admin:secret \
  -H "Content-Type: application/json" \
  -d '{
    "role": "admin",
    "permissions": ["read", "write", "query", "admin"]
  }'
```

### Passwort ändern

```bash
# CLI
themis-admin user password \
  --username alice \
  --new-password newSecret456

# REST API
curl -X POST http://localhost:8765/api/v1/admin/users/alice/password \
  -u admin:secret \
  -H "Content-Type: application/json" \
  -d '{
    "new_password": "newSecret456"
  }'
```

---

## Collection Management

### Collection erstellen

```bash
# CLI
themis-admin collection create \
  --name products \
  --type document \
  --shards 4 \
  --replication 2

# REST API
curl -X POST http://localhost:8765/api/v1/admin/collections \
  -u admin:secret \
  -H "Content-Type: application/json" \
  -d '{
    "name": "products",
    "type": "document",
    "shards": 4,
    "replication_factor": 2,
    "schema": {
      "properties": {
        "name": {"type": "string"},
        "price": {"type": "number"}
      },
      "required": ["name", "price"]
    }
  }'
```

### Collection Statistics

```bash
# CLI
themis-admin collection stats --name products

# REST API
curl http://localhost:8765/api/v1/admin/collections/products/stats \
  -u admin:secret
```

**Output:**
```json
{
  "name": "products",
  "type": "document",
  "count": 150000,
  "size_bytes": 524288000,
  "indexes": 3,
  "shards": 4,
  "replication_factor": 2,
  "avg_doc_size": 3495,
  "read_ops_per_sec": 1234,
  "write_ops_per_sec": 456
}
```

### Index Management

```bash
# Create Index
themis-admin index create \
  --collection products \
  --type hash \
  --fields name \
  --unique

# List Indexes
themis-admin index list --collection products

# Drop Index
themis-admin index drop \
  --collection products \
  --name idx_name

# Rebuild Index
themis-admin index rebuild \
  --collection products \
  --name idx_price
```

**REST API:**
```bash
# Create Index
curl -X POST http://localhost:8765/api/v1/admin/collections/products/indexes \
  -u admin:secret \
  -H "Content-Type: application/json" \
  -d '{
    "type": "hash",
    "fields": ["name"],
    "unique": true,
    "name": "idx_name"
  }'
```

---

## Backup und Recovery

### Backup erstellen

```bash
# Full Backup
themis-admin backup create \
  --output /backups/themis-$(date +%Y%m%d).tar.gz \
  --compress gzip

# Collection-specific Backup
themis-admin backup create \
  --collections users,products,orders \
  --output /backups/critical-data.tar.gz

# Incremental Backup
themis-admin backup create \
  --incremental \
  --since "2026-01-20 00:00:00" \
  --output /backups/incremental.tar.gz
```

**REST API:**
```bash
curl -X POST http://localhost:8765/api/v1/admin/backup \
  -u admin:secret \
  -H "Content-Type: application/json" \
  -d '{
    "type": "full",
    "compression": "gzip",
    "collections": ["users", "products", "orders"]
  }'
```

**Response:**
```json
{
  "backup_id": "backup-20260124-143000",
  "status": "in_progress",
  "started_at": "2026-01-24T14:30:00Z",
  "estimated_size": 5368709120
}
```

### Backup Status

```bash
# CLI
themis-admin backup status --id backup-20260124-143000

# REST API
curl http://localhost:8765/api/v1/admin/backup/backup-20260124-143000 \
  -u admin:secret
```

### Restore Backup

```bash
# Full Restore
themis-admin backup restore \
  --input /backups/themis-20260124.tar.gz

# Selective Restore
themis-admin backup restore \
  --input /backups/themis-20260124.tar.gz \
  --collections users,products

# Restore to different server
themis-admin backup restore \
  --server new-server:8765 \
  --input /backups/themis-20260124.tar.gz
```

---

## Performance Monitoring

### Server Metrics

```bash
# CLI
themis-admin metrics --server localhost:8765

# Real-time monitoring
themis-admin monitor --interval 5s
```

**Output:**
```
Server Metrics (updated every 5s)

CPU Usage:           45% (28/64 cores active)
Memory:              12.4 GB / 128 GB (9.7%)
  - Data:            5.2 GB
  - Cache:           4.1 GB
  - Indexes:         2.3 GB
  - Overhead:        0.8 GB

Storage:             245 GB / 1 TB (24.5%)
  - Documents:       180 GB
  - Indexes:         45 GB
  - WAL:             15 GB
  - Temp:            5 GB

Network:
  - In:              125 MB/s
  - Out:             89 MB/s

Operations:
  - Reads/sec:       5,234
  - Writes/sec:      1,456
  - Queries/sec:     892
  - Avg Latency:     12ms
```

### Query Performance

```bash
# Slow Queries
themis-admin queries slow \
  --min-duration 1000 \
  --limit 10

# Active Queries
themis-admin queries active

# Kill Query
themis-admin queries kill --id query-12345
```

**REST API:**
```bash
# Slow Queries
curl http://localhost:8765/api/v1/admin/queries/slow?min_duration=1000 \
  -u admin:secret
```

**Response:**
```json
{
  "slow_queries": [
    {
      "query_id": "query-12345",
      "query": "FOR doc IN large_collection ...",
      "duration_ms": 5234,
      "started_at": "2026-01-24T14:25:00Z",
      "user": "alice",
      "state": "running"
    }
  ]
}
```

### Cache Statistics

```bash
# CLI
themis-admin cache stats

# REST API
curl http://localhost:8765/api/v1/admin/cache/stats \
  -u admin:secret
```

**Output:**
```json
{
  "query_cache": {
    "size_bytes": 1073741824,
    "entries": 12456,
    "hit_rate": 0.78,
    "evictions": 234
  },
  "document_cache": {
    "size_bytes": 2147483648,
    "entries": 45678,
    "hit_rate": 0.92,
    "evictions": 567
  }
}
```

---

## Security Auditing

### Audit Log Viewer

```bash
# View recent audit logs
themis-admin audit logs \
  --limit 100 \
  --format table

# Filter by user
themis-admin audit logs \
  --user alice \
  --since "2026-01-24 00:00:00"

# Filter by action
themis-admin audit logs \
  --action "DELETE" \
  --collection users
```

**REST API:**
```bash
curl "http://localhost:8765/api/v1/admin/audit/logs?user=alice&action=DELETE" \
  -u admin:secret
```

**Response:**
```json
{
  "logs": [
    {
      "timestamp": "2026-01-24T14:30:15Z",
      "user": "alice",
      "action": "DELETE",
      "collection": "users",
      "document_key": "user-123",
      "ip_address": "192.168.1.100",
      "success": true
    }
  ],
  "total": 1
}
```

### Access Control Audit

```bash
# List permissions
themis-admin permissions list --user alice

# Check specific permission
themis-admin permissions check \
  --user alice \
  --action "DELETE" \
  --collection users
```

**Output:**
```
User: alice
Role: user

Permissions:
  ✅ READ   on users
  ✅ WRITE  on users
  ❌ DELETE on users
  ✅ QUERY  on *
```

### Security Report

```bash
# Generate security report
themis-admin security report \
  --output /reports/security-$(date +%Y%m%d).pdf

# Check for security issues
themis-admin security check
```

**Output:**
```
Security Check Report

✅ SSL/TLS enabled
✅ Authentication required
⚠️  Weak password policy detected
✅ Audit logging enabled
⚠️  Default admin account still active
✅ Encryption at rest enabled
❌ Backup encryption not configured

Recommendations:
1. Enable stricter password policy (min 12 chars, special chars required)
2. Rename or disable default admin account
3. Enable backup encryption with KMS
```

---

## Praktische Beispiele

### Beispiel 1: Tägliche Wartung

```bash
#!/bin/bash
# daily-maintenance.sh

set -e

ADMIN_USER="admin"
ADMIN_PASS="secret"
SERVER="localhost:8765"

echo "=== Daily Maintenance $(date) ==="

# 1. Create Backup
echo "Creating backup..."
themis-admin backup create \
  --server $SERVER \
  --user $ADMIN_USER \
  --password $ADMIN_PASS \
  --output "/backups/daily-$(date +%Y%m%d).tar.gz" \
  --compress gzip

# 2. Optimize Collections
echo "Optimizing collections..."
for collection in users products orders; do
    themis-admin collection optimize \
      --server $SERVER \
      --user $ADMIN_USER \
      --password $ADMIN_PASS \
      --collection $collection
done

# 3. Clean old logs
echo "Cleaning old audit logs..."
curl -X DELETE "http://$SERVER/api/v1/admin/audit/logs?before=$(date -d '90 days ago' +%Y-%m-%d)" \
  -u "$ADMIN_USER:$ADMIN_PASS"

# 4. Generate Reports
echo "Generating reports..."
themis-admin report \
  --server $SERVER \
  --user $ADMIN_USER \
  --password $ADMIN_PASS \
  --type daily \
  --output "/reports/daily-$(date +%Y%m%d).pdf"

# 5. Check Health
echo "Health check..."
if ! themis-admin health --server $SERVER; then
    echo "ERROR: Health check failed!"
    # Send alert
    curl -X POST https://hooks.slack.com/services/YOUR/WEBHOOK \
      -d '{"text": "ThemisDB health check failed!"}'
    exit 1
fi

echo "=== Maintenance completed successfully ==="
```

### Beispiel 2: User Onboarding

```bash
#!/bin/bash
# onboard-user.sh

USERNAME=$1
EMAIL=$2
ROLE=${3:-user}

if [ -z "$USERNAME" ] || [ -z "$EMAIL" ]; then
    echo "Usage: $0 <username> <email> [role]"
    exit 1
fi

# Generate secure password
PASSWORD=$(openssl rand -base64 32)

# Create user
themis-admin user create \
  --username "$USERNAME" \
  --email "$EMAIL" \
  --role "$ROLE" \
  --password "$PASSWORD"

# Set password expiry
themis-admin user update \
  --username "$USERNAME" \
  --password-expires 90d

# Send welcome email
cat <<EOF | sendmail "$EMAIL"
Subject: Welcome to ThemisDB

Hello $USERNAME,

Your ThemisDB account has been created:

Username: $USERNAME
Temporary Password: $PASSWORD

Please change your password on first login.

Access: http://localhost:8765/admin

Best regards,
ThemisDB Admin Team
EOF

echo "User $USERNAME onboarded successfully"
```

### Beispiel 3: Performance Monitoring Dashboard

```python
#!/usr/bin/env python3
# performance_monitor.py

import requests
import time
from datetime import datetime

class ThemisMonitor:
    def __init__(self, server, username, password):
        self.server = server
        self.auth = (username, password)
    
    def get_metrics(self):
        response = requests.get(
            f'{self.server}/api/v1/admin/metrics',
            auth=self.auth
        )
        return response.json()
    
    def get_slow_queries(self, min_duration=1000):
        response = requests.get(
            f'{self.server}/api/v1/admin/queries/slow',
            params={'min_duration': min_duration},
            auth=self.auth
        )
        return response.json()
    
    def monitor(self, interval=5):
        while True:
            metrics = self.get_metrics()
            
            print(f"\n=== {datetime.now().strftime('%Y-%m-%d %H:%M:%S')} ===")
            print(f"CPU: {metrics['cpu_usage']}%")
            print(f"Memory: {metrics['memory_used_gb']:.1f} GB / {metrics['memory_total_gb']:.1f} GB")
            print(f"Queries/sec: {metrics['queries_per_sec']}")
            print(f"Avg Latency: {metrics['avg_latency_ms']:.1f}ms")
            
            # Check for slow queries
            slow_queries = self.get_slow_queries()
            if slow_queries['slow_queries']:
                print("\n⚠️  Slow Queries Detected:")
                for query in slow_queries['slow_queries'][:3]:
                    print(f"  - {query['duration_ms']}ms: {query['query'][:50]}...")
            
            time.sleep(interval)

if __name__ == '__main__':
    monitor = ThemisMonitor(
        'http://localhost:8765',
        'admin',
        'secret'
    )
    
    monitor.monitor(interval=5)
```

---

## Siehe auch

- [CLI Tools Getting Started](../tools/tools_cli_getting_started.md)
- [CLI Tools Advanced](../tools/tools_cli_advanced.md)
- [Security Guide](../security/SECURITY_GUIDE.md)
- [Backup Strategy](../guides/GUIDE_BACKUP_STRATEGY.md)
- [Monitoring Guide](../production/PRODUCTION_MONITORING.md)
