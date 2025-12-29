---
category: "?? Infrastructure"
version: "v1.3.0"
status: "?"
date: "22.12.2025"
---

# ?? Transaction Management

ACID-konforme Transaktionen �ber alle Index-Typen mit MVCC, Snapshot-Isolation und Konflikterkennung.

## ?? Inhaltsverzeichnis

- [?? �bersicht](#-�bersicht)
- [? Features](#-features)
- [?? Schnellstart](#-schnellstart)
- [?? Detaillierte Dokumentation](#-detaillierte-dokumentation)
- [?? Best Practices](#-best-practices)
- [?? Troubleshooting](#-troubleshooting)
- [?? Siehe auch](#-siehe-auch)
- [?? Changelog](#-changelog)

---

## ?? �bersicht

THEMIS bietet ACID-konforme Transaktionen mit umfassender Unterst�tzung f�r alle Index-Typen:

- **Atomicity**: All-or-nothing Ausf�hrung aller Operationen
- **MVCC & Isolation**: ReadCommitted und Snapshot-Isolation mit konsistentem Sichtfenster
- **Konflikterkennung**: Automatische Write-Write-Konflikt-Erkennung
- **Session-Management**: Eindeutige Transaction-IDs und Session-Tracking
- **Multi-Index Support**: Konsistente Updates �ber Secondary, Graph und Vector-Indizes

### Kernfeatures

- **Atomicity**: Alle Operationen innerhalb einer Transaktion werden atomar ausgef�hrt (all-or-nothing)
- **MVCC & Isolation**: ReadCommitted (default) und Snapshot-Isolation mit konsistentem Sichtfenster
- **Konflikterkennung**: Write-Write-Konflikte werden beim Commit/Put erkannt und f�hren zu Fehlern/Abbr�chen
- **Session-Management**: Transaktionen sind �ber eindeutige Transaction-IDs identifizierbar
- **Statistics Tracking**: Umfassende Metriken (begun, committed, aborted, durations, success rate)
- **Auto-Rollback**: RAII-Pattern f�r automatisches Rollback bei Exception/Destruktion
- **Multi-Index Support**: Konsistente Updates �ber Secondary, Graph und Vector-Indizes

## ? Features

| Feature | Beschreibung | Status |
|---------|--------------|--------|
| **ACID-Transaktionen** | Vollst�ndig ACID-konform �ber alle Index-Typen | ? Implementiert |
| **MVCC** | Multi-Version Concurrency Control | ? Implementiert |
| **Snapshot-Isolation** | Konsistente Snapshot-Sichtfenster | ? Implementiert |
| **Konflikterkennung** | Automatische Write-Write-Erkennung | ? Implementiert |
| **Statistics Tracking** | Metriken f�r Monitoring | ? Implementiert |
| **Auto-Rollback** | RAII-Pattern f�r automatisches Rollback | ? Implementiert |

## ?? Schnellstart

```bash
# 1. Server starten
./themis_server --config config.json

# 2. Transaktion starten und committen
curl -X POST http://localhost:8080/api/transactions \
  -d '{"isolation_level": "snapshot_isolation"}'

# 3. Operationen durchf�hren und commit
curl -X PUT http://localhost:8080/api/transactions/{tx_id}/commit
```

---

## ?? Detaillierte Dokumentation

## Architektur

### TransactionManager

Zentrale Komponente f�r Session-basiertes Transaction-Management:

```cpp
class TransactionManager {
public:
    // Session-based API (empfohlen f�r HTTP/Multi-Client)
    TransactionId beginTransaction(IsolationLevel isolation = IsolationLevel::ReadCommitted);
    Status commitTransaction(TransactionId id);
    void rollbackTransaction(TransactionId id);
    std::shared_ptr<Transaction> getTransaction(TransactionId id);
    
    // Direct API (f�r Single-Threaded/Embedded)
    Transaction begin(IsolationLevel isolation = IsolationLevel::ReadCommitted);
    
    // Statistics
    Stats getStats() const;
    void cleanupOldTransactions(std::chrono::seconds max_age);
};
```

### Transaction Class

Stellt Operationen innerhalb einer Transaktion bereit:

```cpp
class Transaction {
public:
    // Metadata
    TransactionId getId() const;
    IsolationLevel getIsolationLevel() const;
    uint64_t getDurationMs() const;
    
    // Relational Operations
    Status putEntity(std::string_view table, const BaseEntity& entity);
    Status eraseEntity(std::string_view table, std::string_view pk);
    
    // Graph Operations
    Status addEdge(const BaseEntity& edgeEntity);
    Status deleteEdge(std::string_view edgeId);
    
    // Vector Operations
    Status addVector(const BaseEntity& entity, std::string_view vectorField = "embedding");
    Status updateVector(const BaseEntity& entity, std::string_view vectorField = "embedding");
    Status removeVector(std::string_view pk);
    
    // Finalization
    Status commit();
    void rollback();
};
```

### Isolation Levels

#### ReadCommitted (Default)
- Lesezugriffe sehen nur committed data
- Keine Dirty Reads
- Non-Repeatable Reads m�glich
- Geeignet f�r: Standard OLTP-Workloads

#### Snapshot
- Point-in-time Konsistenz (Sichtfenster fixiert beim Begin)
- Repeatable Reads innerhalb der Transaktion
- H�here Isolation, potenziell h�herer Overhead
- Geeignet f�r: Analytische Queries, konsistente Reports

---

## HTTP REST API

### POST /transaction/begin

Startet eine neue Transaktion.

**Request:**
```json
{
  "isolation": "read_committed"  // optional, default: "read_committed"
}
```

**Response:**
```json
{
  "transaction_id": 42,
  "isolation": "read_committed",
  "status": "begun"
}
```

**cURL Beispiel:**
```bash
curl -X POST http://localhost:8080/transaction/begin \
  -H "Content-Type: application/json" \
  -d '{"isolation": "snapshot"}'
```

---

### POST /transaction/commit

Committed eine Transaktion.

**Request:**
```json
{
  "transaction_id": 42
}
```

**Response (Success):**
```json
{
  "transaction_id": 42,
  "status": "committed",
  "message": "Transaction committed successfully"
}
```

**Response (Error):**
```json
{
  "transaction_id": 42,
  "status": "error",
  "error": "Transaction not found or already completed"
}
```

**cURL Beispiel:**
```bash
curl -X POST http://localhost:8080/transaction/commit \
  -H "Content-Type: application/json" \
  -d '{"transaction_id": 42}'
```

---

### POST /transaction/rollback

Rollt eine Transaktion zur�ck.

**Request:**
```json
{
  "transaction_id": 42
}
```

**Response:**
```json
{
  "transaction_id": 42,
  "status": "rolled_back",
  "message": "Transaction rolled back successfully"
}
```

**cURL Beispiel:**
```bash
curl -X POST http://localhost:8080/transaction/rollback \
  -H "Content-Type: application/json" \
  -d '{"transaction_id": 42}'
```

---

### GET /transaction/stats

Liefert Statistiken �ber alle Transaktionen.

**Response:**
```json
{
  "total_begun": 1523,
  "total_committed": 1401,
  "total_aborted": 122,
  "active_count": 3,
  "avg_duration_ms": 45,
  "max_duration_ms": 523,
  "success_rate": 0.92
}
```

**cURL Beispiel:**
```bash
curl http://localhost:8080/transaction/stats
```

---

## Use Cases & Best Practices

### 1. Atomares Multi-Entity Update

**Problem:** Mehrere Entities mit verschiedenen Indizes m�ssen atomar gespeichert werden.

**L�sung:**
```bash
# Begin transaction
TXN_ID=$(curl -s -X POST http://localhost:8080/transaction/begin | jq -r '.transaction_id')

# Insert entities via transaction (hypothetisch - API-Erweiterung erforderlich)
# Aktuell: Direkte C++ API verwenden

# C++ Code:
auto txn_id = tx_manager->beginTransaction();
auto txn = tx_manager->getTransaction(txn_id);

BaseEntity user("user123");
user.setField("name", std::string("Alice"));
user.setField("city", std::string("Berlin"));
txn->putEntity("users", user);

BaseEntity order("order456");
order.setField("user_id", std::string("user123"));
order.setField("total", 99.99);
txn->putEntity("orders", order);

// Commit via HTTP
curl -X POST http://localhost:8080/transaction/commit \
  -d "{\"transaction_id\": $TXN_ID}"
```

---

### 2. Graph-Operationen mit Rollback

**Problem:** Mehrere Graph-Edges sollen atomar hinzugef�gt werden, bei Fehler Rollback.

**C++ Beispiel:**
```cpp
auto txn_id = tx_manager->beginTransaction();
auto txn = tx_manager->getTransaction(txn_id);

try {
    // Add multiple edges
    BaseEntity edge1("edge1");
    edge1.setField("_from", std::string("user1"));
    edge1.setField("_to", std::string("user2"));
    edge1.setField("type", std::string("follows"));
    txn->addEdge(edge1);
    
    BaseEntity edge2("edge2");
    edge2.setField("_from", std::string("user1"));
    edge2.setField("_to", std::string("user3"));
    edge2.setField("type", std::string("follows"));
    txn->addEdge(edge2);
    
    // Validate business logic
    if (!validateFollowerLimit(txn)) {
        tx_manager->rollbackTransaction(txn_id);
        return Status::Error("Follower limit exceeded");
    }
    
    // Commit if all OK
    return tx_manager->commitTransaction(txn_id);
} catch (...) {
    tx_manager->rollbackTransaction(txn_id);
    throw;
}
```

---

### 3. Vector-Index mit Transaktionen

**Problem:** Vector-Embedding und Metadaten sollen atomar gespeichert werden.

**C++ Beispiel:**
```cpp
auto txn_id = tx_manager->beginTransaction();
auto txn = tx_manager->getTransaction(txn_id);

// Document with embedding
BaseEntity doc("doc123");
doc.setField("title", std::string("AI Research Paper"));
doc.setField("author", std::string("Dr. Smith"));
std::vector<float> embedding = {0.23f, 0.45f, 0.67f, /* ... 768 dims */};
doc.setField("embedding", embedding);

// Store entity (relational)
auto status = txn->putEntity("documents", doc);
if (!status.ok) {
    tx_manager->rollbackTransaction(txn_id);
    return status;
}

// Store vector (vector index)
status = txn->addVector(doc, "embedding");
if (!status.ok) {
    tx_manager->rollbackTransaction(txn_id);
    return status;
}

// Commit both atomically
return tx_manager->commitTransaction(txn_id);
```

---

### 4. Long-Running Transactions & Cleanup

**Problem:** Vergessene/abgest�rzte Clients hinterlassen offene Transaktionen.

**Best Practice:**
```cpp
// Periodischer Cleanup (z.B. via Cronjob oder Background-Thread)
tx_manager->cleanupOldTransactions(std::chrono::hours(1));

// Metriken �berwachen
auto stats = tx_manager->getStats();
if (stats.active_count > 100) {
    THEMIS_WARN("High number of active transactions: {}", stats.active_count);
}
```

**HTTP Monitoring:**
```bash
# Statistiken abrufen
curl http://localhost:8080/transaction/stats | jq '.active_count'

# Alert bei hoher Anzahl
if [ $(curl -s http://localhost:8080/transaction/stats | jq '.active_count') -gt 50 ]; then
  echo "WARNING: Too many active transactions"
fi
```

---

## Performance-�berlegungen

### MVCC Overhead

- Snapshot-Verwaltung und Konflikterkennung erzeugen geringen Overhead gegen�ber einfachen Writes
- Benefit: Korrektheit unter Parallelit�t (kein Last-Write-Wins), konsistente Sicht

### Isolation Level Trade-offs

| Isolation Level | Read Overhead | Write Overhead | Use Case |
|----------------|---------------|----------------|----------|
| ReadCommitted  | Minimal       | Minimal        | OLTP, Standard-Queries |
| Snapshot       | +5-10%        | +5-10%         | Reports, Analytics |

### Index-spezifische Kosten

- **Secondary Index**: ~0.05ms pro Index-Entry (Put/Delete)
- **Graph Index**: ~0.1ms pro Edge (2x Index-Entries: out + in)
- **Vector Index**: ~0.5-2ms (abh�ngig von HNSW-Parametern, Dimension)

---

## Bekannte Einschr�nkungen

### 1. Vector Index In-Memory Cache

**Problem:**  
Der Vector-Index h�lt einen In-Memory Cache (HNSW-Struktur, `cache_` map). Bei Rollback werden RocksDB-�nderungen r�ckg�ngig gemacht, aber der Cache bleibt inkonsistent.

**Auswirkung:**
- Nach Rollback k�nnen Vector-Searches falsch-positive Ergebnisse liefern
- Cache enth�lt Vektoren, die in RocksDB nicht existieren

**Workaround:**
```cpp
// Nach Rollback: Vector-Index neu laden
vector_index->rebuildFromStorage();
```

**Zuk�nftige Verbesserung:**  
Callback-Mechanismus f�r commit/rollback, um Cache synchron zu halten.

---

### 2. Konflikte unter Parallelit�t

**Verhalten:**  
Write-Write-Konflikte werden erkannt und f�hren zu Fehlern beim Commit/Put. Clients sollten Retry-Logik mit Backoff implementieren, wenn eine Transaktion aufgrund eines Konflikts abgelehnt wird.

---

### 3. Transaction Timeout

**Limitation:**  
Keine automatischen Timeouts f�r aktive Transaktionen implementiert.

**Empfehlung:**
```cpp
// Client-seitig: Timeout �berwachen
auto start = std::chrono::system_clock::now();
auto txn_id = tx_manager->beginTransaction();

// ... operations ...

auto duration = std::chrono::system_clock::now() - start;
if (duration > std::chrono::seconds(30)) {
    tx_manager->rollbackTransaction(txn_id);
    return Status::Error("Transaction timeout");
}
```

---

## Fehlerbehandlung

### Commit Failures

```cpp
auto status = tx_manager->commitTransaction(txn_id);
if (!status.ok) {
    // M�gliche Ursachen:
    // - Transaction nicht gefunden
    // - Bereits committed/rolled back
    // - RocksDB Write-Fehler (Disk-Full, Permissions)
    
    THEMIS_ERROR("Commit failed: {}", status.message);
    
    // Cleanup
    tx_manager->rollbackTransaction(txn_id);  // Safe auch bei bereits abgeschlossener Txn
}
```

### Network Failures (HTTP)

```bash
# Retry-Logic mit Exponential Backoff
for i in {1..3}; do
  if curl -X POST http://localhost:8080/transaction/commit \
       -d "{\"transaction_id\": $TXN_ID}" \
       --max-time 5; then
    break
  fi
  sleep $((2**i))
done
```

### Auto-Rollback bei Exception

```cpp
{
    auto txn = tx_manager->begin();  // Direct API
    
    txn.putEntity("users", user);
    
    // Exception thrown -> Destruktor ruft automatisch rollback()
    throw std::runtime_error("Business logic error");
    
}  // txn Destruktor: THEMIS_WARN + rollback()
```

---

## Metriken & Monitoring

### Prometheus-Integration

Transaction-Statistiken sind via `/metrics` Endpoint verf�gbar:

```
# TYPE vccdb_transactions_begun_total counter
vccdb_transactions_begun_total 1523

# TYPE vccdb_transactions_committed_total counter
vccdb_transactions_committed_total 1401

# TYPE vccdb_transactions_aborted_total counter
vccdb_transactions_aborted_total 122

# TYPE vccdb_transactions_active gauge
vccdb_transactions_active 3

# TYPE vccdb_transaction_duration_ms histogram
vccdb_transaction_duration_ms_bucket{le="10"} 834
vccdb_transaction_duration_ms_bucket{le="50"} 1245
vccdb_transaction_duration_ms_bucket{le="100"} 1398
vccdb_transaction_duration_ms_bucket{le="+Inf"} 1523
vccdb_transaction_duration_ms_sum 68035
vccdb_transaction_duration_ms_count 1523
```

### Grafana Dashboard

Empfohlene Metriken:
- **Transaction Rate**: `rate(vccdb_transactions_begun_total[5m])`
- **Success Rate**: `vccdb_transactions_committed_total / vccdb_transactions_begun_total`
- **Active Transactions**: `vccdb_transactions_active`
- **Avg Duration**: `vccdb_transaction_duration_ms_sum / vccdb_transaction_duration_ms_count`

---

## Migrationshinweise

### Von Direct API zu Session-based API

**Vorher (Direct):**
```cpp
auto txn = tx_manager->begin();
txn.putEntity("users", user);
txn.commit();
```

**Nachher (Session-based, HTTP-kompatibel):**
```cpp
auto txn_id = tx_manager->beginTransaction();
auto txn = tx_manager->getTransaction(txn_id);
txn->putEntity("users", user);
tx_manager->commitTransaction(txn_id);
```

**Vorteil:** Transaction-ID kann �ber HTTP/Network �bertragen werden.

---

## Weiterf�hrende Dokumentation

- [Architecture Overview](architecture.md) - Systemarchitektur
- [MVCC Design](mvcc_design.md) - Hintergr�nde & Optionen
- [RocksDB Storage](storage/rocksdb_layout.md) - WAL/Snapshots/Compaction
- [Deployment Guide](deployment.md) - Production Setup
- [OpenAPI Specification](openapi.yaml) - Vollst�ndige API-Referenz
- [Base Entity Documentation](base_entity.md) - Entity-Serialisierung

---

## Changelog

### Version 1.0 (28. Oktober 2025)
- Initial release
- Session-based Transaction Management
- Isolation Levels: ReadCommitted, Snapshot
- Multi-Index Support: Secondary, Graph, Vector
- HTTP REST API: begin, commit, rollback, stats
- Comprehensive Statistics & Monitoring
- 23 Unit-Tests (100% Pass-Rate)

## ?? Best Practices

| ? Empfohlen | ? Vermeiden |
|--------------|--------------|
| Dokumentierte Best Practices | Anti-Patterns ignorieren |
| Regelm��iges Testing | Deployment ohne Tests |
| Monitoring aktivieren | Blind Deployments |

## ? Troubleshooting

<details>
<summary><b>H�ufige Probleme</b></summary>

Siehe Logs f�r Details.

</details>

##  Siehe auch

- [Security Best Practices](../security/best_practices.md)
- [Architecture Guide](../architecture/overview.md)

##  Changelog

| Version | Datum | �nderungen |
|---------|-------|-----------|
| v1.3.0 | 2025-12-22 | Template-Aktualisierung f�r v1.3.0 Standard |

---

**Letzte Aktualisierung:** 22. Dezember 2025  
**Autor:** ThemisDB Team  
**Status:**  Produktiv
## 💡 Best Practices

| ✅ Empfohlen | ❌ Vermeiden |
|--------------|--------------|
| Dokumentierte Best Practices | Anti-Patterns ignorieren |
| Regelmäßiges Testing | Deployment ohne Tests |
| Monitoring aktivieren | Blind Deployments |

## 🔧 Troubleshooting

<details>
<summary><b>Häufige Probleme</b></summary>

Siehe Logs für Details.

</details>

## 📚 Siehe auch

- [Security Best Practices](../security/best_practices.md)
- [Architecture Guide](../architecture/overview.md)

## 📝 Changelog

| Version | Datum | Änderungen |
|---------|-------|-----------|
| v1.3.0 | 2025-12-22 | Template-Aktualisierung für v1.3.0 Standard |

---

**Letzte Aktualisierung:** 22. Dezember 2025  
**Autor:** ThemisDB Team  
**Status:** ✅ Produktiv