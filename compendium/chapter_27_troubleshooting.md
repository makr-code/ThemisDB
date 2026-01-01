# Kapitel 27: Troubleshooting & Problem Resolution

> *"The difference between a good engineer and a great engineer is how quickly they can diagnose and fix production issues."*

---

## Überblick

Production-Probleme erfordern systematische Diagnose und schnelle Remediation. Dieses Kapitel bietet konkrete Lösungen für häufige ThemisDB-Probleme mit reproduzierbaren Debugging-Workflows.

**Was Sie in diesem Kapitel lernen:**
- Systematische Problemanalyse (5-Why-Methode)
- Performance-Probleme diagnostizieren
- Replikations-Lag beheben
- Out-of-Memory Errors lösen
- Deadlock-Detection & Resolution
- Korrupte Indizes reparieren
- Network Partition Recovery

---

## 27.1 Systematische Problem-Diagnose

### 5-Why Root Cause Analysis

```
Problem: Query dauert 30 Sekunden statt 100ms

Why 1: Warum ist die Query langsam?
  → Full Collection Scan statt Index-Nutzung

Why 2: Warum wird kein Index genutzt?
  → Filter auf nicht-indexiertes Feld `metadata.custom_field`

Why 3: Warum ist das Feld nicht indexiert?
  → Index wurde nach Schema-Änderung nicht aktualisiert

Why 4: Warum wurde Index nicht aktualisiert?
  → Deployment-Prozess hat Migrations-Step übersprungen

Why 5: Warum wurde Step übersprungen?
  → CI/CD Pipeline hatte keine Pre-Deploy Validation

Root Cause: Fehlende Pre-Deploy Index-Validation
Solution: Pre-Deploy Hook für Schema-Validation hinzufügen
```

### Diagnostic Checklist

```bash
#!/bin/bash
# troubleshoot.sh: Schneller Healthcheck

echo "=== ThemisDB Health Check ==="

# 1. Cluster Status
echo "1. Cluster Status:"
curl -s http://localhost:8529/_admin/cluster/health | jq .

# 2. Memory Usage
echo "2. Memory Usage:"
curl -s http://localhost:8529/_admin/statistics | jq '.memory'

# 3. Slow Queries (>1s)
echo "3. Slow Queries:"
curl -s http://localhost:8529/_admin/slow-queries?threshold=1000

# 4. Replication Lag
echo "4. Replication Lag:"
curl -s http://localhost:8529/_admin/replication/lag

# 5. Connection Pool
echo "5. Active Connections:"
netstat -an | grep :8529 | wc -l

# 6. Disk Space
echo "6. Disk Space:"
df -h /data/themis

# 7. Last Errors
echo "7. Recent Errors:"
journalctl -u themis -n 50 --no-pager | grep ERROR
```

---

## 27.2 Performance-Probleme

### Problem: Langsame Queries

**Symptom:** Query dauert >5 Sekunden  
**Diagnose:**

```aql
-- Query mit EXPLAIN analysieren
EXPLAIN FOR u IN users
  FILTER u.email == 'alice@example.com'
  RETURN u

-- Ausgabe prüfen:
{
  "plan": {
    "nodes": [
      {"type": "SingletonNode"},
      {"type": "EnumerateCollectionNode", "collection": "users"},  // ❌ SCHLECHT: Full Scan
      {"type": "FilterNode"},
      {"type": "ReturnNode"}
    ]
  },
  "stats": {
    "executionTime": 5.234,
    "scannedFull": 1000000  // ❌ 1M Dokumente gescannt
  }
}
```

**Solution:**

```aql
-- Index erstellen
CREATE INDEX idx_users_email ON users (email)

-- Erneut EXPLAIN
EXPLAIN FOR u IN users
  FILTER u.email == 'alice@example.com'
  RETURN u

-- Jetzt mit Index:
{
  "plan": {
    "nodes": [
      {"type": "SingletonNode"},
      {"type": "IndexNode", "index": "idx_users_email"},  // ✅ GUT: Index genutzt
      {"type": "ReturnNode"}
    ]
  },
  "stats": {
    "executionTime": 0.023,  // ✅ 200x schneller
    "scannedIndex": 1
  }
}
```

### Problem: High CPU Usage

**Symptom:** CPU >90% dauerhaft  
**Diagnose:**

```bash
# Top Queries nach CPU-Zeit
curl -s http://localhost:8529/_admin/query-stats \
  | jq '.queries | sort_by(.cpu_time) | reverse | .[0:10]'

# Beispiel-Output:
[
  {
    "query": "FOR doc IN large_collection RETURN doc",
    "cpu_time": 45.2,
    "count": 120,
    "avg_duration_ms": 8500
  }
]
```

**Solution:**

```aql
-- Option 1: Query optimieren (Projection)
FOR doc IN large_collection
  RETURN {id: doc._id, name: doc.name}  -- Nur benötigte Felder

-- Option 2: Limit hinzufügen
FOR doc IN large_collection
  LIMIT 100
  RETURN doc

-- Option 3: Background-Job für Batch-Processing
FOR doc IN large_collection
  FILTER doc.needs_processing == true
  UPDATE doc WITH {needs_processing: false, processed_at: DATE_NOW()}
  OPTIONS {waitForSync: false}  -- Async I/O
```

---

## 27.3 Replikations-Probleme

### Problem: Replication Lag

**Symptom:** Replica ist 5 Minuten hinter Primary  
**Diagnose:**

```bash
# Replication Status prüfen
curl http://localhost:8529/_admin/replication/status

# Output:
{
  "primary": "node-1",
  "replica": "node-2",
  "lag_seconds": 300,
  "last_applied_timestamp": "2025-12-31T22:55:00Z",
  "pending_operations": 15000
}
```

**Root Causes & Solutions:**

**1. Netzwerk-Latenz:**
```bash
# Latenz messen
ping node-2
# > 50ms → Problem!

# Lösung: Gleiche Region/AZ nutzen
terraform apply -var="replica_region=eu-central-1"
```

**2. Replica überlastet:**
```bash
# CPU/Memory auf Replica prüfen
ssh node-2 "top -b -n 1 | head -20"

# Lösung: Replica upgraden
kubectl scale statefulset themis-replica --replicas=0
kubectl set resources statefulset themis-replica \
  --limits=cpu=8,memory=32Gi
kubectl scale statefulset themis-replica --replicas=1
```

**3. Zu viele Schreibvorgänge:**
```aql
-- Schreibrate reduzieren mit Batching
LET batch = @documents  -- Array von 1000 Docs
FOR doc IN batch
  INSERT doc INTO collection
  OPTIONS {waitForSync: false}  -- Async für höheren Throughput
```

---

## 27.4 Memory-Probleme

### Problem: Out of Memory (OOM)

**Symptom:** Server crashed mit OOM  
**Diagnose:**

```bash
# Memory-Statistiken
curl http://localhost:8529/_admin/statistics | jq '.memory'

{
  "resident_set_size_mb": 15800,  # Actual RAM usage
  "virtual_size_mb": 18200,
  "heap_used_mb": 14500,
  "heap_limit_mb": 16000,  # ❌ 90% ausgelastet!
  "buffer_cache_mb": 1200
}
```

**Solutions:**

**1. Memory Limit erhöhen:**
```yaml
# k8s/themis-deployment.yaml
resources:
  limits:
    memory: 32Gi  # Von 16Gi auf 32Gi
  requests:
    memory: 24Gi
```

**2. Query Memory Leaks finden:**
```aql
-- Große Result Sets vermeiden
FOR doc IN huge_collection
  LIMIT 1000000  -- ❌ 1M Docs in Memory!
  RETURN doc

-- Besser: Streaming mit Cursor
FOR doc IN huge_collection
  RETURN doc
  OPTIONS {stream: true, batchSize: 1000}
```

**3. Buffer Cache tunen:**
```bash
# themis.conf: Buffer Cache reduzieren
buffer_cache_size_mb=512  # Von 2048 auf 512
```

---

## 27.5 Deadlock-Probleme

### Problem: Deadlock Detected

**Symptom:** Transaction failed mit "Deadlock detected"  
**Diagnose:**

```aql
-- Deadlock-Log abrufen
FOR dl IN deadlock_log
  SORT dl.timestamp DESC
  LIMIT 1
  RETURN dl

{
  "timestamp": "2025-12-31T23:00:00Z",
  "transactions": [
    {
      "tx_id": "tx-1234",
      "locked_collections": ["orders", "inventory"],
      "waiting_for": "inventory/item-5"
    },
    {
      "tx_id": "tx-5678",
      "locked_collections": ["inventory", "orders"],  // ❌ Umgekehrte Reihenfolge!
      "waiting_for": "orders/order-10"
    }
  ]
}
```

**Solution: Lock Ordering**

```aql
-- ❌ FALSCH: Inkonsistente Lock-Reihenfolge
// Transaction 1
UPDATE "orders/o1" ...
UPDATE "inventory/i1" ...

// Transaction 2
UPDATE "inventory/i1" ...  // Deadlock!
UPDATE "orders/o1" ...

-- ✅ RICHTIG: Konsistente Reihenfolge (alphabetisch)
// Beide Transactions
UPDATE "inventory/i1" ...  // Immer zuerst inventory
UPDATE "orders/o1" ...     // Dann orders
```

**Lock Timeout setzen:**
```aql
FOR doc IN collection
  UPDATE doc WITH {...}
  OPTIONS {lockTimeout: 5000}  -- 5s statt default 30s
```

---

## 27.6 Index-Probleme

### Problem: Korrupter Index

**Symptom:** Query returned wrong results  
**Diagnose:**

```bash
# Index Integrity Check
curl http://localhost:8529/_admin/index/check/users/idx_email

{
  "index": "idx_email",
  "status": "corrupted",
  "missing_entries": 42,
  "extra_entries": 5
}
```

**Solution:**

```aql
-- Index neu erstellen
DROP INDEX users/idx_email
CREATE INDEX idx_email ON users (email)

-- Verify
FOR u IN users
  FILTER u.email == 'test@example.com'
  RETURN u
```

### Problem: Index zu groß

**Symptom:** Index verbraucht 50 GB  
**Diagnose:**

```bash
curl http://localhost:8529/_admin/index/stats | jq '.indexes[] | select(.size_mb > 10000)'

{
  "name": "idx_fulltext_articles",
  "size_mb": 52000,  # 52 GB!
  "document_count": 10000000
}
```

**Solution:**

```aql
-- Option 1: Sparse Index (nur nicht-NULL Werte)
CREATE INDEX idx_email_sparse ON users (email)
  OPTIONS {sparse: true}

-- Option 2: Partial Index (nur aktive User)
CREATE INDEX idx_active_users ON users (email)
  WHERE status == 'active'

-- Option 3: Archivierung alter Daten
FOR doc IN articles
  FILTER doc.created_at < DATE_SUBTRACT(DATE_NOW(), 2, 'years')
  REMOVE doc IN articles
  INSERT doc INTO articles_archive
```

---

## 27.7 Network Partition Recovery

### Problem: Split-Brain nach Partition

**Symptom:** Zwei separate Cluster nach Netzwerk-Trennung  
**Diagnose:**

```bash
# Node 1 (Europe)
curl http://node-1:8529/_admin/cluster/status
{"nodes": ["node-1", "node-2"], "quorum": true}

# Node 3 (US - isoliert)
curl http://node-3:8529/_admin/cluster/status
{"nodes": ["node-3"], "quorum": false}  # ❌ Lost quorum
```

**Recovery:**

```bash
# 1. Netzwerk reparieren
# (Fix Firewall/VPN/Router)

# 2. Manuelles Rejoin erzwingen
curl -X POST http://node-3:8529/_admin/cluster/rejoin \
  -d '{"primary": "http://node-1:8529"}'

# 3. Replication Catch-up warten
watch -n 5 'curl -s http://node-3:8529/_admin/replication/lag'

# 4. Quorum wiederherstellen
curl http://node-1:8529/_admin/cluster/status
{"nodes": ["node-1", "node-2", "node-3"], "quorum": true}  # ✅
```

---

## 27.8 Backup & Restore Issues

### Problem: Backup schlägt fehl

**Symptom:** Backup job timeout after 2 hours  
**Diagnose:**

```bash
# Backup-Logs prüfen
journalctl -u themis-backup -f

# Error: "Disk full on backup volume"
df -h /backup
# /backup: 98% verwendet
```

**Solutions:**

```bash
# Option 1: Alte Backups löschen
find /backup -name "*.backup" -mtime +30 -delete

# Option 2: Inkrementales Backup statt Full
themis-backup --mode=incremental --since=2025-12-30

# Option 3: Compression erhöhen
themis-backup --compression=zstd --compression-level=19
```

---

## 27.9 Common Error Messages

### Error: "Collection not found"

```
ERROR: Collection 'user' not found
```

**Solution:**
```aql
-- Typo: 'user' statt 'users'
FOR u IN users  -- ✅ Richtig
  RETURN u
```

### Error: "Index hint invalid"

```
ERROR: Index hint 'idx_name' does not exist
```

**Solution:**
```aql
-- Index existiert nicht mehr → neu erstellen oder Hint entfernen
CREATE INDEX idx_name ON users (name)

-- Oder Query ohne Hint:
FOR u IN users
  FILTER u.name == 'Alice'
  RETURN u
```

### Error: "Transaction timeout"

```
ERROR: Transaction timeout after 30000ms
```

**Solution:**
```aql
-- Transaction zu lang → in kleinere Batches aufteilen
LET batch_size = 1000
FOR doc IN large_update
  LIMIT @offset, batch_size
  UPDATE doc IN collection
```

---

## 27.10 Troubleshooting Playbook

| Problem | Symptom | Erste Schritte | Eskalation |
|---------|---------|---------------|------------|
| Slow Query | >5s Response | EXPLAIN, add Index | Query Rewrite |
| High CPU | >90% Usage | Query Stats, Optimize | Scale up |
| Replication Lag | >60s Lag | Network Check, Batch Size | Add Replica |
| OOM | Crash/Restart | Memory Stats, Increase Limit | Optimize Queries |
| Deadlock | Transaction Fails | Lock Ordering | Reduce Concurrency |
| Corrupt Index | Wrong Results | Rebuild Index | Restore Backup |
| Split-Brain | Quorum Lost | Network Repair, Rejoin | Manual Failover |

**Best Practices:**
- ✅ Immer EXPLAIN vor Production-Deployment
- ✅ Monitoring-Alerts für Lag >30s, CPU >80%, Memory >85%
- ✅ Regelmäßige Index-Maintenance (weekly VACUUM/ANALYZE)
- ✅ Backup-Tests (monatlich Restore-Drill)
- ✅ Runbooks für alle kritischen Fehler
- ✅ Post-Mortems nach jedem Incident
