# Transaction Module

**Stand:** 6. April 2026  
**Version:** v1.3.0  
**Kategorie:** 🧩 Architecture

---

## 📑 Inhaltsverzeichnis

- [Übersicht](#übersicht)
- [Source-Code Referenz](#source-code-referenz)
- [Implementierte Klassen](#implementierte-klassen)
- [Beispiel](#beispiel)
- [ACID-Garantien](#acid-garantien)
- [Write-Write Conflict Detection](#write-write-conflict-detection)
- [Verwandte Dokumentation](#verwandte-dokumentation)

## Übersicht

Das Transaction-Modul implementiert vollständige ACID-Transaktionen mit MVCC (Multi-Version Concurrency Control) für ThemisDB.

## Source-Code Referenz

| Komponente | Header | Source | Beschreibung |
|------------|--------|--------|--------------|
| TransactionManager | `transaction_manager.h` | `transaction_manager.cpp` | MVCC Manager |
| Transaction | `transaction_manager.h` | `transaction_manager.cpp` | Transaktions-Handle |
| Saga | `saga.h` | `saga.cpp` | Distributed Transactions |

**Gesamt:** 2 Header, 2 Source-Dateien, ~900 LOC

## Implementierte Klassen

### TransactionManager

```cpp
class TransactionManager {
    using TransactionId = uint64_t;
    
    struct Status {
        bool ok;
        std::string message;
        static Status OK();
        static Status Error(std::string msg);
    };
    
    // Begin Transaction
    Transaction begin(IsolationLevel level = IsolationLevel::ReadCommitted);
    
    // Statistics
    size_t activeTransactionCount() const;
};
```

### Transaction

```cpp
class Transaction {
    // Metadata
    TransactionId getId() const;
    IsolationLevel getIsolationLevel() const;
    std::chrono::system_clock::time_point getStartTime() const;
    uint64_t getDurationMs() const;
    bool isFinished() const;
    
    // Relational Operations
    Status putEntity(std::string_view table, const BaseEntity& entity);
    Status eraseEntity(std::string_view table, std::string_view pk);
    
    // Graph Operations
    Status addEdge(const BaseEntity& edgeEntity);
    Status deleteEdge(std::string_view edgeId);
    
    // Vector Operations
    Status addVector(const BaseEntity& entity, vectorField = "embedding");
    Status updateVector(const BaseEntity& entity, vectorField = "embedding");
    Status removeVector(std::string_view pk);
    
    // Commit/Rollback
    Status commit();
    Status rollback();
};
```

### Isolation Levels

```cpp
enum class IsolationLevel {
    ReadCommitted,  // Default: nur committed data sichtbar
    Snapshot        // Snapshot Isolation (point-in-time consistency)
};
```

## Beispiel

```cpp
TransactionManager tm(db, secIdx, graphIdx, vecIdx);

// Begin Transaction
auto txn = tm.begin(IsolationLevel::Snapshot);

// Multi-Model Operations
txn.putEntity("users", userEntity);
txn.addEdge(followsEdge);
txn.addVector(embeddingEntity);

// Commit
auto status = txn.commit();
if (!status.ok) {
    // Automatic rollback on error
    std::cerr << status.message << std::endl;
}
```

## ACID-Garantien

| Property | Implementation |
|----------|----------------|
| **Atomicity** | RocksDB WriteBatch (alle oder keine) |
| **Consistency** | Index-Updates in gleicher Batch |
| **Isolation** | MVCC mit Snapshot Isolation |
| **Durability** | WAL (Write-Ahead Log) |

## Write-Write Conflict Detection

```cpp
// Bei Konflikt:
// - Transaction A: UPDATE users SET name='Alice' WHERE id=1
// - Transaction B: UPDATE users SET name='Bob' WHERE id=1
// → Transaction B wird mit CONFLICT abgebrochen wenn A zuerst committed
```

## Verwandte Dokumentation

- [Features: Transactions](../features/features_transactions.md) - Feature-Details
- [Architecture: MVCC](architecture_mvcc.md) - MVCC-Architektur
- [Research: Git/GitHub/GitOps Vergleich](../../research/git_gitops_themis_vergleich.md) - Vergleich zur Versionskontrolle
