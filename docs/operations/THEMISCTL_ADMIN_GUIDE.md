# themisctl — ThemisDB Unified Management CLI

**Version:** 0.0.2  
**Last Updated:** 2026-04-04  
**Status:** ✅ Production-Ready

---

## Overview

`themisctl` is a unified command-line interface for managing and operating ThemisDB clusters. It provides convenient access to the most frequently-used operations via simple sub-commands.

### Key Features

- **Health & Status** — Monitor server liveness and readiness
- **Queries** — Execute AQL queries and retrieve results
- **Data Operations** — Get, Put, Delete entities
- **Schema Management** — View and manage database schema
- **Configuration** — Live hot-reload of server configuration
- **Branching** — Create, switch, and delete branches for schema management
- **Snapshots** — Create and manage point-in-time snapshots
- **Admin Utilities** — View cache health, observability statistics
- **REPL Mode** — Interactive shell with command history and completion

---

## Installation

### From Release

```bash
# Download the themisctl binary for your platform
# macOS/Linux: themisctl / Windows: themisctl.exe
wget https://releases.themisdb.io/themisctl/v1.8.1/themisctl-linux-x64
chmod +x themisctl-linux-x64
sudo mv themisctl-linux-x64 /usr/local/bin/themisctl
```

### From Build

```bash
# Build from source (requires C++17 compiler)
cd /path/to/themis
cmake --build build-release --target themisctl
sudo cp build-release/cmake/themisctl /usr/local/bin/
```

### Shell Completion

Install completion scripts for your shell:

**Bash:**
```bash
sudo cp tools/completion/themisctl.bash /etc/bash_completion.d/themisctl
# or
curl -s https://releases.themisdb.io/themisctl/completion/bash | sudo tee /etc/bash_completion.d/themisctl
```

**Zsh:**
```bash
sudo cp tools/completion/_themisctl /usr/share/zsh/site-functions/
# or
fpath=(/usr/share/zsh/site-functions $fpath)
```

**Fish:**
```bash
sudo cp tools/completion/themisctl.fish /usr/share/fish/vendor_completions.d/
```

---

## Configuration

### Environment Variables

Set default connection parameters via environment:

```bash
export THEMIS_HOST="db.internal"         # Default: localhost
export THEMIS_PORT="9000"                # Default: 8765
export THEMIS_TOKEN="eyJhbGc..."         # Bearer token for auth
export THEMIS_TIMEOUT="60"               # Request timeout (seconds)
```

### Global Options

All commands support these global flags (must come before the command name):

```bash
themisctl [global-options] <command> [command-options]

Global Options:
  --host <h>      ThemisDB host       (default: localhost / $THEMIS_HOST)
  --port <p>      ThemisDB port       (default: 8765 / $THEMIS_PORT)
  --token <jwt>   Bearer auth token   ($THEMIS_TOKEN)
  --timeout <s>   Request timeout     (default: 30 seconds)
  --json          Print raw JSON responses
  --no-color      Disable ANSI color output
  --help, -h      Print help message
```

---

## Commands

### health

Check server liveness and readiness status.

```bash
themisctl health
```

**Output:**
```
liveness:  ✓ healthy
readiness: ✓ healthy
```

**Exit Codes:**
- `0`: Both liveness and readiness are healthy
- `1`: Server unhealthy (503 or other error)
- `3`: Cannot connect to server

---

### version

Display server version information.

```bash
themisctl version
themisctl --json version         # Raw JSON output
```

**Output Example:**
```
Version:    1.8.1-rc1
Build Date: 2026-04-03
Commit:     b607e5a29fe1
Edition:    COMMUNITY
```

---

### query

Execute an AQL query and retrieve results.

```bash
themisctl query 'FOR d IN users FILTER d.active == true RETURN d'
themisctl query --timeout 120 'EXPENSIVE QUERY SCAN users ...'
themisctl --json query 'RETURN {status: "ok"}'
```

**Features:**
- Multi-line query input (use shell escaping or here-docs)
- Configurable timeout per query
- Pretty-printed output (or raw JSON with `--json`)

---

### get

Retrieve an entity by key.

```bash
themisctl get user:42
themisctl --json get user:42        # Raw JSON response
```

**Output Example:**
```
{
  "id": "user:42",
  "name": "Alice",
  "email": "alice@example.com",
  "active": true
}
```

---

### put

Create or update an entity.

```bash
themisctl put user:42 '{"name":"Alice","email":"alice@example.com"}'
themisctl put user:43 @user.json    # Load from file (shell syntax)
```

**Features:**
- Supports inline JSON or file input
- Creates a new entity if ID doesn't exist
- Updates existing entity if ID exists (merge semantics)

---

### delete

Delete an entity by key.

```bash
themisctl delete user:42
themisctl delete user:42 --confirm   # Skip confirmation prompt
```

**Safety:**
- Requires confirmation by default (prompts "Delete user:42? [y/N]")
- Use `--confirm` to skip confirmation (useful in scripts)

---

### schema

Display database schema information.

```bash
themisctl schema                     # All tables
themisctl schema users               # Specific table
themisctl --json schema users        # Raw JSON schema
```

**Output Example:**
```
Table: users
  Columns:
    • id (string, primary key)
    • name (string)
    • email (string, indexed)
    • active (boolean)
    • created_at (timestamp)
```

---

### config

Read and modify server configuration with hot-reload.

#### Get Configuration

```bash
themisctl config get                 # Print all config
themisctl config get logging.level   # Specific key
themisctl --json config get          # Raw JSON
```

#### Set Configuration (Hot-Reload)

```bash
themisctl config set logging.level=debug
themisctl config set \
  logging.level=debug \
  request_timeout_ms=60000 \
  features.cdc=true
```

**Supported Keys (examples):**
- `logging.level` — info, debug, warn, error
- `logging.format` — json, text
- `request_timeout_ms` — milliseconds
- `features.cdc` — true/false
- `features.vector_search` — true/false
- `cdc_retention_hours` — hours to retain CDC logs

**Features:**
- Dotted key notation for nested configuration
- Changes are persisted (SIGHUP triggers reload)
- Full validation on server side

---

### branch

Manage schema branches for non-breaking schema changes.

#### List Branches

```bash
themisctl branch list               # Show all branches
themisctl --json branch list        # Raw JSON
```

**Output Example:**
```
Branches:
  • main (active)
  • feature-x
  • v2-schema
```

#### Create Branch

```bash
themisctl branch create feature-x
themisctl branch create feature-x --from main  # Explicit source
```

#### Switch Branch

```bash
themisctl branch switch feature-x   # Switch to branch
themisctl branch switch main        # Back to main
```

#### Delete Branch

```bash
themisctl branch delete feature-x
themisctl branch delete feature-x --force      # Force delete (use with caution)
```

---

### snapshot

Create and manage point-in-time snapshots for recovery.

#### List Snapshots

```bash
themisctl snapshot list              # Show all snapshots
themisctl --json snapshot list       # Raw JSON
```

**Output Example:**
```
Snapshots:
  • v1.8.1-rc1 (2026-04-03T12:34:56Z, 4.2 GB)
  • hotfix-backup (2026-04-02T18:20:00Z, 4.1 GB)
```

#### Create Snapshot

```bash
themisctl snapshot create           # Auto-generated timestamp
themisctl snapshot create v1.8.1-rc1 # With custom tag
```

**Features:**
- Creates a consistent point-in-time backup
- Can be used for recovery or branching
- Includes all schema and data

---

### admin

View server statistics and health metrics.

#### Server Statistics

```bash
themisctl admin stats               # Observability health
themisctl admin stats --json        # Raw JSON metrics
```

**Output Example:**
```
Observability Metrics:
  Uptime:           72h 15m 30s
  Requests/sec:     1,234
  Avg Latency:      42ms
  Error Rate:       0.01%
  Cache Hit Rate:   87.3%
  Memory:           2.1 GB / 16 GB
```

#### Cache Statistics

```bash
themisctl admin cache               # Cache health
themisctl admin cache --json        # Raw JSON
```

**Output Example:**
```
Cache Health:
  Size:             512 MB / 2 GB
  Hit Rate:         87.3%
  Eviction Rate:    2.1/sec
  Entries:          1,256,432
  Hottest Values:   [top 5 entries...]
```

---

### repl

Start interactive REPL mode for exploration and scripting.

```bash
themisctl repl
```

**Features:**
- GNU Readline support for line editing (if available)
- Command history persisted to `~/.themisctl_history`
- Tab completion for commands and keys
- Multi-line input support (backslash continuation)

**REPL Commands:**
```
> help                         Show available commands
> health                       Run health check
> query SELECT * FROM users    Execute AQL query
> config get logging.level     Get config value
> exit                         Exit REPL (or Ctrl-D)
```

**History:**
- Automatically saved between sessions
- Access with `Up`/`Down` arrow keys
- Search with `Ctrl-R`

---

## Examples

### Cluster Health Monitoring

```bash
#!/bin/bash
# Monitor cluster health every 30 seconds

while true; do
  echo "=== $(date) ==="
  themisctl --host db1.internal health
  themisctl --host db2.internal health
  sleep 30
done
```

### Configuration Management

```bash
#!/bin/bash
# Enable debug logging for troubleshooting

themisctl \
  --host db-staging \
  config set \
    logging.level=debug \
    logging.format=json

# Verify
themisctl --host db-staging config get logging
```

### Data Backup via Snapshot

```bash
#!/bin/bash
# Create daily backup snapshot

DATE=$(date +%Y-%m-%d_%H%M%S)
SNAPSHOT_TAG="backup-$DATE"

echo "Creating snapshot: $SNAPSHOT_TAG"
themisctl snapshot create "$SNAPSHOT_TAG"

echo "Snapshot created. To restore later:"
echo "  themisctl snapshot restore $SNAPSHOT_TAG"
```

### Query Results Export

```bash
#!/bin/bash
# Execute query and export to JSON file

QUERY='
FOR user IN users
  RETURN {
    id: user._id,
    name: user.name,
    email: user.email,
    created_at: user._created
  }
'

themisctl --json query "$QUERY" | jq '.' > users_export.json
echo "Exported to users_export.json"
```

### Bulk Data Import

```bash
#!/bin/bash
# Import entities from CSV

while IFS=',' read id name email; do
  themisctl put "user:$id" "{\"name\":\"$name\",\"email\":\"$email\"}"
done < users.csv
```

---

## Authentication & Security

### Bearer Tokens

```bash
# Provide token via environment
export THEMIS_TOKEN="eyJhbGc..."
themisctl health

# Or inline
themisctl --token "eyJhbGc..." health
```

### mTLS / TLS

```bash
# HTTPS connection (automatically detected for :443)
themisctl --host db.internal --port 8765 health

# Disable certificate verification (dev/testing only!)
# Note: Requires recompilation with CPPHTTPLIB_OPENSSL_SUPPORT
```

### IP Whitelisting

Configure firewall rules on the server:

```bash
# Only allow admin CLI from trusted networks
iptables -A INPUT -p tcp --dport 8765 \
  -s 10.0.0.0/8 -j ACCEPT
iptables -A INPUT -p tcp --dport 8765 -j DROP
```

---

## Troubleshooting

### Connection Error

**Error:** `Connection error: Could not establish connection`

**Solutions:**
1. Check server is running: `ping hostname`
2. Check correct port: `netstat -tlnp | grep 8765`
3. Verify firewall: `sudo ufw allow 8765`
4. Check credentials: `export THEMIS_TOKEN=...`

### Timeout

**Error:** `Connection error: Request timeout`

**Solutions:**
1. Increase timeout: `themisctl --timeout 120 query ...`
2. Check network latency: `ping -c 5 hostname`
3. Check server load: `themisctl admin stats`
4. Check query complexity: simplify or add LIMIT

### Authentication Failure

**Error:** `HTTP 401: Unauthorized`

**Solutions:**
1. Check token format: `echo $THEMIS_TOKEN`
2. Verify token expiration: decode JWT header
3. Check token scope: ensure it includes required permissions
4. Regenerate token on server

### Query Error

**Error:** `HTTP 400: Bad Request`

**Solutions:**
1. Validate AQL syntax: start with simple query
2. Check collection names: `themisctl schema`
3. Add EXPLAIN: `themisctl query 'EXPLAIN RETURN ...'`
4. Check field names: case-sensitive in AQL

---

## Performance Tips

### Query Optimization

```bash
# Add LIMIT to avoid large result sets
themisctl query 'FOR d IN users RETURN d LIMIT 1000'

# Use FILTER early to reduce intermediate results
themisctl query 'FOR d IN users FILTER d.active == true RETURN d'

# Check query plan with EXPLAIN
themisctl query 'EXPLAIN FOR d IN users FILTER d.active == true RETURN d'
```

### Batch Operations

```bash
# Use loops for bulk imports (parallel if safe)
for i in {1..10000}; do
  themisctl put "doc:$i" "{\"value\":$i}" &
done
wait
```

### Caching

```bash
# Check cache effectiveness
themisctl admin cache

# Warm cache before heavy queries
themisctl query 'FOR d IN users LIMIT 1'  # Triggers index loads
```

---

## Advanced Usage

### REPL Scripting

```bash
# Run commands from a file via REPL
cat << 'EOF' | themisctl repl
health
version
schema users
config get logging.level
EOF
```

### Continuous Monitoring

```bash
#!/bin/bash
# Monitor and alert on metrics

THRESHOLD=85  # 85% memory
while true; do
  MEMORY=$(themisctl admin stats --json | jq '.memory_percent')
  if (( $(echo "$MEMORY > $THRESHOLD" | bc -l) )); then
    echo "ALERT: High memory usage: $MEMORY%" | mail -s "ThemisDB Alert" ops@company.com
  fi
  sleep 300
done
```

### Integration with CI/CD

```bash
# Kubernetes sidecar health check
kubectl set probe deployment/themisdb \
  --liveness \
  -- /usr/local/bin/themisctl health
```

---

## Exit Codes

| Code | Meaning |
|------|---------|
| 0 | Success |
| 1 | Server error (HTTP 4xx / 5xx) |
| 2 | Usage / argument error |
| 3 | Connection / transport error |

---

## See Also

- [ThemisDB REST API](../api/REST_API.md)
- [gRPC API](../api/GRPC_API.md)
- [AQL Query Language](../aql/AQL_REFERENCE.md)
- [Configuration Reference](../config/CONFIG_REFERENCE.md)
- [Operations Runbooks](./OPERATIONS_RUNBOOK.md)

---

**Status:** ✅ Production-Ready  
**Maturity:** Stable (v0.0.2)  
**Last Tested:** 2026-04-04
