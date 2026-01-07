# Transaction Module

**Date:** December 5, 2025  
**Version:** v1.3.0  
**Category:** 🧩 Architecture

---

## 📑 Table of Contents

- [Overview](#overview)
- [Source Code Reference](#source-code-reference)
- [Implemented Classes](#implemented-classes)
- [Example](#example)
- [ACID Guarantees](#acid-guarantees)
- [Write-Write Conflict Detection](#write-write-conflict-detection)
- [Related Documentation](#related-documentation)

## Overview

The Transaction Module implements full ACID transactions with MVCC (Multi-Version Concurrency Control) for ThemisDB.

## Source Code Reference

| Component | Header | Source | Description |
|-----------|--------|--------|-------------|
| TransactionManager | `transaction_manager.h` | `transaction_manager.cpp` | MVCC Manager |
| Transaction | `transaction_manager.h` | `transaction_manager.cpp` | Transaction Handle |
| Saga | `saga.h` | `saga.cpp` | Distributed Transactions |

**Total:** 2 Headers, 2 Source Files, ~900 LOC

## Implemented Classes

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
    ReadCommitted,  // Default: only committed data visible
    Snapshot        // Snapshot Isolation (point-in-time consistency)
};
```

## Example

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

## ACID Guarantees

| Property | Implementation |
|----------|----------------|
| **Atomicity** | RocksDB WriteBatch (all or nothing) |
| **Consistency** | Index updates in same batch |
| **Isolation** | MVCC with Snapshot Isolation |
| **Durability** | WAL (Write-Ahead Log) |

## Write-Write Conflict Detection

```cpp
// On conflict:
// - Transaction A: UPDATE users SET name='Alice' WHERE id=1
// - Transaction B: UPDATE users SET name='Bob' WHERE id=1
// → Transaction B aborts with CONFLICT if A commits first
```

## Related Documentation

- [Features: Transactions](../../de/features/features_transactions.md) - Feature details
- [Architecture: MVCC](../../de/architecture/architecture_mvcc.md) - MVCC architecture

---

> **Note:** For detailed architecture documentation, please refer to the [German architecture documentation](../../de/architecture/).

**Version:** 1.3.0 | **License:** MIT | **Support:** [GitHub Issues](https://github.com/makr-code/ThemisDB/issues)
