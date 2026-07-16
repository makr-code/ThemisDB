# ThemisDB CLI Tools - Getting Started

**Version:** 1.0.0  
**Stand:** 6. April 2026  
**Kategorie:** Tools  
**Status:** ✅ Produktionsreif

---

## 📑 Inhaltsverzeichnis

- [Übersicht](#übersicht)
- [Installation](#installation)
- [themis-cli](#themis-cli)
- [themis-admin](#themis-admin)
- [themis-import](#themis-import)
- [themis-export](#themis-export)
- [themis-backup](#themis-backup)
- [Beispiele](#beispiele)

---

## Übersicht

ThemisDB bietet eine Suite von CLI-Tools für Administration, Datenverwaltung und Wartung.

### Verfügbare Tools

| Tool | Beschreibung | Verwendung |
|------|--------------|------------|
| **themis-cli** | Interaktive Shell für AQL-Queries | Entwicklung, Testing |
| **themis-admin** | Admin-Tool für Server-Management | Administration, Monitoring |
| **themis-import** | Bulk-Import von Daten | Migration, Initial Load |
| **themis-export** | Daten-Export in verschiedene Formate | Backup, Analytics |
| **themis-backup** | Backup und Recovery | Disaster Recovery |

---

## Installation

### Binary Installation

```bash
# Linux (x64)
wget https://github.com/themisdb/themis/releases/download/v1.4.0/themis-tools-linux-amd64.tar.gz
tar -xzf themis-tools-linux-amd64.tar.gz
sudo mv themis-* /usr/local/bin/

# macOS (ARM64)
brew install themisdb-tools

# Windows
choco install themisdb-tools
```

### Docker

```bash
# Alle Tools in einem Container
docker run -it themisdb/tools:1.4.0 themis-cli --help
```

### Build from Source

```bash
git clone https://github.com/themisdb/themis.git
cd themis/tools
mkdir build && cd build
cmake ..
make -j$(nproc)
sudo make install
```

---

## themis-cli

Interaktive Shell für AQL-Queries und Server-Interaktion.

### Basic Usage

```bash
# Verbinden zum Server
themis-cli --server localhost:8765

# Mit Authentifizierung
themis-cli --server localhost:8765 --username admin --password secret

# Mit SSL/TLS
themis-cli --server localhost:8765 --ssl --ssl-verify
```

### Interactive Mode

```bash
$ themis-cli --server localhost:8765

ThemisDB Interactive Shell v1.4.0
Type 'help' for commands, 'exit' to quit

themis> FOR doc IN users LIMIT 5 RETURN doc
[
  {"_key": "1", "name": "Alice", "age": 30},
  {"_key": "2", "name": "Bob", "age": 25},
  {"_key": "3", "name": "Charlie", "age": 35},
  {"_key": "4", "name": "Diana", "age": 28},
  {"_key": "5", "name": "Eve", "age": 32}
]
Query executed in 23ms

themis> .stats
Server: localhost:8765
Version: 1.4.0
Collections: 12
Documents: 1,234,567
Memory: 2.3 GB / 16 GB (14%)
Uptime: 5d 12h 34m

themis> .collections
┌──────────────┬───────────┬────────────┐
│ Name         │ Documents │ Size       │
├──────────────┼───────────┼────────────┤
│ users        │ 10,000    │ 2.4 MB     │
│ products     │ 50,000    │ 12.5 MB    │
│ orders       │ 100,000   │ 45.2 MB    │
└──────────────┴───────────┴────────────┘

themis> .indexes users
┌──────────────┬──────────┬────────────┐
│ Name         │ Type     │ Fields     │
├──────────────┼──────────┼────────────┤
│ idx_email    │ hash     │ email      │
│ idx_age      │ skiplist │ age        │
│ idx_location │ geo      │ location   │
└──────────────┴──────────┴────────────┘
```

### Scripting Mode

```bash
# Execute single query
themis-cli --server localhost:8765 --query "FOR doc IN users RETURN doc"

# Execute from file
themis-cli --server localhost:8765 --file queries.aql

# Batch mode with multiple queries
cat << EOF | themis-cli --server localhost:8765
FOR doc IN users FILTER doc.age > 30 RETURN doc
FOR doc IN products FILTER doc.price < 100 RETURN doc
EOF
```

### Output Formats

```bash
# JSON (default)
themis-cli --query "FOR doc IN users LIMIT 3 RETURN doc" --format json

# CSV
themis-cli --query "FOR doc IN users RETURN doc" --format csv > users.csv

# Table
themis-cli --query "FOR doc IN users LIMIT 5 RETURN doc" --format table

# Pretty JSON
themis-cli --query "FOR doc IN users LIMIT 3 RETURN doc" --format json --pretty
```

### Special Commands

```
.help                    Show all commands
.stats                   Server statistics
.collections             List collections
.indexes <collection>    List indexes
.explain <query>         Explain query execution plan
.profile <query>         Profile query performance
.clear                   Clear screen
.history                 Show command history
.exit / .quit            Exit shell
```

---

## themis-admin

Administrative tool für Server-Management.

### Server Status

```bash
# Server health check
themis-admin health --server localhost:8765

# Detailed status
themis-admin status --server localhost:8765
```

**Output:**
```
Server Status:
  Status: Running
  Version: 1.4.0
  Uptime: 5 days, 12 hours, 34 minutes
  
Memory:
  Used: 2.3 GB / 16 GB (14%)
  Caches: 1.2 GB
  Indexes: 512 MB
  
Storage:
  Collections: 12
  Documents: 1,234,567
  Size: 45.2 GB
  
Performance:
  Queries/sec: 1,234
  Avg latency: 12ms
  Active connections: 45
```

### User Management

```bash
# Create user
themis-admin user create --username alice --password secret --role user

# List users
themis-admin user list

# Update user
themis-admin user update --username alice --role admin

# Delete user
themis-admin user delete --username alice

# Change password
themis-admin user password --username alice --new-password newsecret
```

### Collection Management

```bash
# Create collection
themis-admin collection create --name products --type document

# Drop collection
themis-admin collection drop --name products --force

# Collection info
themis-admin collection info --name products

# Compact collection
themis-admin collection compact --name products
```

### Index Management

```bash
# Create index
themis-admin index create \
  --collection users \
  --type hash \
  --fields email \
  --unique

# List indexes
themis-admin index list --collection users

# Drop index
themis-admin index drop --collection users --name idx_email

# Rebuild index
themis-admin index rebuild --collection users --name idx_email
```

### Cache Management

```bash
# Clear all caches
themis-admin cache clear

# Clear query cache
themis-admin cache clear --type query

# Cache statistics
themis-admin cache stats
```

---

## themis-import

Bulk-Import von Daten aus verschiedenen Quellen.

### JSON Import

```bash
# Import JSON array
themis-import \
  --server localhost:8765 \
  --collection users \
  --file users.json \
  --format json

# Import JSONL (one document per line)
themis-import \
  --server localhost:8765 \
  --collection users \
  --file users.jsonl \
  --format jsonl \
  --batch-size 1000
```

### CSV Import

```bash
# Import CSV with header
themis-import \
  --server localhost:8765 \
  --collection users \
  --file users.csv \
  --format csv \
  --header

# Import CSV with custom delimiter
themis-import \
  --server localhost:8765 \
  --collection products \
  --file products.csv \
  --format csv \
  --delimiter ";" \
  --quote "\""
```

### SQL Database Import

```bash
# Import from PostgreSQL
themis-import \
  --server localhost:8765 \
  --collection users \
  --source postgres \
  --connection "postgresql://user:pass@localhost/db" \
  --table users

# Import with query
themis-import \
  --server localhost:8765 \
  --collection active_users \
  --source postgres \
  --connection "postgresql://user:pass@localhost/db" \
  --query "SELECT * FROM users WHERE active = true"
```

### Options

```bash
--batch-size 1000        Batch size for bulk inserts
--parallel 4             Parallel workers
--on-duplicate update    Action on duplicate: skip, update, error
--create-collection      Create collection if not exists
--progress               Show progress bar
--dry-run               Don't actually import, just validate
```

---

## themis-export

Daten-Export in verschiedene Formate.

### JSON Export

```bash
# Export collection to JSON
themis-export \
  --server localhost:8765 \
  --collection users \
  --output users.json \
  --format json

# Export with query
themis-export \
  --server localhost:8765 \
  --query "FOR doc IN users FILTER doc.age > 30 RETURN doc" \
  --output adult_users.json \
  --format json \
  --pretty
```

### CSV Export

```bash
# Export to CSV
themis-export \
  --server localhost:8765 \
  --collection users \
  --output users.csv \
  --format csv \
  --header

# Export with selected fields
themis-export \
  --server localhost:8765 \
  --collection users \
  --output users.csv \
  --format csv \
  --fields "_key,name,email,age"
```

### Database Export

```bash
# Export to PostgreSQL
themis-export \
  --server localhost:8765 \
  --collection users \
  --destination postgres \
  --connection "postgresql://user:pass@localhost/db" \
  --table users

# Export to MySQL
themis-export \
  --server localhost:8765 \
  --collection products \
  --destination mysql \
  --connection "mysql://user:pass@localhost/db" \
  --table products
```

---

## themis-backup

Backup und Recovery Management.

### Create Backup

```bash
# Full backup
themis-backup create \
  --server localhost:8765 \
  --output /backups/themis-$(date +%Y%m%d-%H%M%S).tar.gz

# Incremental backup
themis-backup create \
  --server localhost:8765 \
  --output /backups/incremental.tar.gz \
  --incremental \
  --since "2026-01-20 00:00:00"

# Backup specific collections
themis-backup create \
  --server localhost:8765 \
  --collections users,products,orders \
  --output /backups/critical.tar.gz
```

### Restore Backup

```bash
# Full restore
themis-backup restore \
  --server localhost:8765 \
  --input /backups/themis-20260124.tar.gz

# Restore specific collections
themis-backup restore \
  --server localhost:8765 \
  --input /backups/themis-20260124.tar.gz \
  --collections users,products

# Restore to different server
themis-backup restore \
  --server new-server:8765 \
  --input /backups/themis-20260124.tar.gz
```

### Backup Schedule

```bash
# Create backup job
themis-backup schedule \
  --name daily \
  --cron "0 2 * * *" \
  --output /backups/daily-{date}.tar.gz \
  --retention 7

# List scheduled backups
themis-backup schedule list

# Remove schedule
themis-backup schedule remove --name daily
```

---

## Beispiele

### Complete Migration Workflow

```bash
# 1. Export from old system
themis-export \
  --server old-server:8765 \
  --collection users \
  --output users.json

# 2. Transform data (optional)
jq '.[] | select(.active == true)' users.json > active_users.json

# 3. Import to new system
themis-import \
  --server new-server:8765 \
  --collection users \
  --file active_users.json \
  --batch-size 5000

# 4. Verify import
themis-cli --server new-server:8765 --query "RETURN COUNT(FOR doc IN users RETURN 1)"
```

### Automated Backup Script

```bash
#!/bin/bash
# backup-script.sh

DATE=$(date +%Y%m%d-%H%M%S)
BACKUP_DIR=/backups
RETENTION_DAYS=7

# Create backup
themis-backup create \
  --server localhost:8765 \
  --output ${BACKUP_DIR}/themis-${DATE}.tar.gz \
  --compress gzip

# Upload to S3
aws s3 cp ${BACKUP_DIR}/themis-${DATE}.tar.gz s3://my-backups/

# Clean old backups
find ${BACKUP_DIR} -name "themis-*.tar.gz" -mtime +${RETENTION_DAYS} -delete

# Send notification
curl -X POST https://hooks.slack.com/services/YOUR/WEBHOOK \
  -d '{"text": "ThemisDB backup completed: '${DATE}'"}'
```

---

## Siehe auch

- [Admin Tools Guide](../../de/admin_tools/admin_guide.md)
- [Advanced Usage](tools_cli_advanced.md)
- [Dashboard Documentation](tools_dashboard.md)
- [Backup Strategy](../guides/GUIDE_BACKUP_STRATEGY.md)
