# Transaction-Dokumentation

**Source Code:** `src/transaction/`, `include/transaction/`

Diese Dokumentation beschreibt die Transaktions-Komponenten von ThemisDB.

## Übersicht

ThemisDB implementiert vollständige ACID-Transaktionen mit:
- MVCC (Multi-Version Concurrency Control)
- Snapshot Isolation
- Write-Write Conflict Detection
- Atomic Rollbacks

## Verwandte Dokumentation

- [Features: Transactions](../features/transactions.md)
- [Architecture: MVCC Design](../architecture/mvcc_design.md)
