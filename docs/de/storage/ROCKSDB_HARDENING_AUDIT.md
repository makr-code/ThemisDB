## 5.1 Findings (2026-02-10)

1. RocksDB version observed in checked-in OPTIONS files as 10.4.2 with links to [data/vccdb_server/OPTIONS-000176#L1-L95](data/vccdb_server/OPTIONS-000176#L1-L95) and [OPTIONS-000178#L1-L95](OPTIONS-000178#L1-L95) and note that max_total_wal_size, bytes_per_sync, wal_bytes_per_sync, WAL_ttl_seconds, WAL_size_limit_MB are 0 and use_fsync=false in those files;

2. Note that wal_bytes_per_sync/bytes_per_sync/max_total_wal_size are mentioned in docs and benchmarks but not clearly wired in RocksDBWrapper ([link](docs/de/storage/storage_rocksdb.md#L24-L73), [benchmarks](benchmarks/PHASE2H_OUTLOOK.md#L1-L37), [compendium](compendium/docs/chapter_20_performance.md#L79-L168));

3. Explain dual WAL concept: RocksDB native WAL vs Themis sharding WALManager/WALShipper ([links](include/sharding/wal_manager.h#L44-L168) and [include/sharding/wal_shipper.h#L10-L105));

4. Point out inconsistent per-component WriteOptions usage with BatchWriteOptimizer vs Changefeed default WriteOptions ([links](src/storage/batch_write_optimizer.cpp#L1-L102) and [src/cdc/changefeed.cpp#L66-L162)) and recommend centralizing durability policy;

5. Mention that admin commands like 'themisdb-admin wal verify/recover' appear in runbooks/docs but code implementation not found in limited search results, with links [compendium](compendium/docs/appendix_e_incident_runbooks.md#L370-L503) and [compendium](compendium/docs/appendix_i_troubleshooting.md#L364-L503) and [docs](docs/production/DISASTER_RECOVERY_PLAN.md#L1-L95);

6. Note that code search results are limited to 10 and provide GitHub UI search links for [wal_bytes_per_sync](https://github.com/search?q=wal_bytes_per_sync) , [use_fsync](https://github.com/search?q=use_fsync) , and [themisdb-admin](https://github.com/search?q=themisdb-admin).