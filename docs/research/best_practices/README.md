# Best Practices Index

This directory documents engineering best practices — from open-source projects, industry standards, and internal ThemisDB learnings — that influence the codebase.

## Purpose

When a design or implementation pattern is adopted from an established external source (e.g., another open-source database, an industry blog post, a reference implementation), it should be documented here.

## Index

| Best Practice | Module(s) | Version | Status |
|---------------|-----------|---------|--------|
| [Lock-Free L1 Cache Reads](lock_free_cache_reads.md) | `src/cache/` | v1.9.0+ | ✅ Fully Adopted |
| [Argon2id KDF](argon2id_kdf.md) | `src/user_storage_encrypted/`, `plugins/user_storage_encrypted/` | v0.1.0+ | ✅ Fully Adopted |
| [Exponential Backoff with Jitter](exponential_backoff_retry.md) | `src/chimera/` | v1.8.0+ | ✅ Fully Adopted |
| [PIMPL ABI Stability](pimpl_abi_stability.md) | `src/server/` | v1.9.0+ | ✅ Fully Adopted |
| [Token Bucket Rate Limiting](token_bucket_rate_limiting.md) | `src/server/` | v1.6.0+ | ✅ Fully Adopted |
| [RocksDB WriteBatch Atomicity](rocksdb_write_batch_atomicity.md) | `src/index/` | v1.0.0+ | ✅ Fully Adopted |
| [OpenTelemetry Span Tracing](opentelemetry_tracing.md) | `src/server/handlers/`, `utils/` | v1.9.0+ | ✅ Fully Adopted |
| [JWT Short-Lived Tokens](jwt_short_lived_tokens.md) | `src/server/` | v1.6.0+ | ✅ Fully Adopted |
| [TLS 1.3 Cipher Hardening](tls13_cipher_hardening.md) | `src/server/` | v1.0.0+ | ✅ Fully Adopted |
| [Consistent Hash Ring](consistent_hash_ring.md) | `src/server/` | v2.1.0+ | ✅ Fully Adopted |
| [MurmurHash3 Deterministic Sharding](murmur_hash_deterministic_sharding.md) | `src/prompt_engineering/`, `src/sharding/` | v1.9.0+ | ✅ Fully Adopted |
| [FNV-1a 64-bit Checksums](fnv1a_checksums.md) | `src/prompt_engineering/` | v2.0.0+ | ✅ Fully Adopted |
| [Secure Key Zeroing](secure_key_zeroing.md) | `plugins/user_storage_encrypted/`, `src/user_storage_encrypted/` | v0.1.0+ | ✅ Fully Adopted |
| [std::shared_mutex Read-Write Locks](shared_mutex_read_write_locks.md) | `src/config/`, `src/cache/`, `src/server/` | v1.8.0+ | ✅ Fully Adopted |
| [Boost.Asio Proactor Async I/O](boost_asio_async_io.md) | `src/server/` | v1.0.0+ | ✅ Fully Adopted |

## Adding a New Best Practice

1. Copy [_template_best_practice.md](_template_best_practice.md) to a new file  
   Example: `zero_copy_io.md`, `lock_free_queues.md`
2. Fill in all required fields (see [TEMPLATES.md](TEMPLATES.md))
3. Link it in the relevant module README under *Wissenschaftliche Grundlagen & Einflüsse*
4. Register it in [implementation_influence/README.md](../implementation_influence/README.md)

## Naming Convention

```
<short_descriptive_name>.md
```

Examples:
- `zero_copy_io.md`
- `lock_free_queue_design.md`
- `rocksdb_compaction_tuning.md`

## See Also

- [TEMPLATES.md](TEMPLATES.md) — required fields and formatting rules
- [_template_best_practice.md](_template_best_practice.md) — copy-paste starter
- [../RESEARCH_GUIDE.md](../RESEARCH_GUIDE.md) — end-to-end contributor workflow
- [../implementation_influence/README.md](../implementation_influence/README.md) — master cross-reference index
