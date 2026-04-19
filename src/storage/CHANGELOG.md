> ⚠️ **Historisches Changelog** – Einträge beschreiben den Stand zum Zeitpunkt der Erstellung.

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog — Storage Module

All notable changes to the Storage module are documented here.
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]

## [1.6.0] — 2026-03-12
### Added
- Tiered storage (hot/warm/cold) with age- and access-based migration policies (`tiered_storage.cpp`)
- GCS (Google Cloud Storage) blob backend (`blob_backend_gcs.cpp`, requires `THEMIS_ENABLE_GCS`)
- BackupManager in-memory scheduling: `scheduleBackup`, `cancelScheduledBackup`, `listScheduledBackups`
- Cloud backup routing: `uploadBackupToCloud`, `restoreFromCloud` gated on `THEMIS_ENABLE_S3/AZURE/GCS`
- Production-mode safety flag (`THEMIS_PRODUCTION_MODE`) preventing insecure encryption defaults
- Build system audit: all `src/storage/*.cpp` registered; 21 focused standalone test targets added

### Changed
- `StorageEngine::createDefault()` deprecated in favour of explicit DI constructor

### Fixed
- Jitter value validation in `TransactionRetryManager` (prevents non-finite delay calculation)
- `const_cast` UB removed in storage audit code

## [1.5.0] — 2025-09-01
### Added
- `TransactionRetryManager` with exponential/linear/fixed backoff and circuit breaker
- `BlobRedundancyManager` RAID-1 mirror across multiple blob backends
- `RaftMVCCBridge` integrating Raft consensus log with MVCC storage
- `HLC` Hybrid Logical Clock for causally-consistent distributed timestamps
- `HistoryManager` version history and change tracking per key
- `SecuritySignature` + `SecuritySignatureManager` AES-GCM encryption and HMAC-SHA256

## [1.0.0] — 2024-01-01
### Added
- `RocksDBWrapper` with MVCC, WAL, BlobDB, multi-path SSTables
- `MVCCStore` snapshot isolation
- `WALStorage` write-ahead log management
- `KeySchema` unified multi-model key encoding
- `StorageEngine` high-level abstraction with dependency injection
- `BackupManager` incremental and full backups
- `PITRManager` point-in-time recovery
