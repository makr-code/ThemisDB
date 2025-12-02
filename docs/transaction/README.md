# Transaction-Dokumentation

**Source Code:** `src/transaction/`, `include/transaction/`

Diese Dokumentation beschreibt die Transaktions-Komponenten von ThemisDB.

## Übersicht

ThemisDB implementiert vollständige ACID-Transaktionen mit:
- MVCC (Multi-Version Concurrency Control)
- Snapshot Isolation
- Write-Write Conflict Detection
- Atomic Rollbacks

## Dokumentation in diesem Ordner

| Datei | Beschreibung | Status |
|-------|--------------|--------|
| [transaction_overview.md](./transaction_overview.md) | ACID-Konzept | 📋 TODO |
| [transaction_implementation.md](./transaction_implementation.md) | MVCC-Details | 📋 TODO |
| [transaction_api.md](./transaction_api.md) | Transaction API | 📋 TODO |
| [transaction_isolation.md](./transaction_isolation.md) | Isolation Levels | 📋 TODO |
| [transaction_performance.md](./transaction_performance.md) | Benchmarks | 📋 TODO |

## Verwandte Dokumentation

- [Features: Transactions](../features/features_transactions.md)
- [Architecture: MVCC Design](../architecture/architecture_mvcc.md)
