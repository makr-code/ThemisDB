# Content Subsystem Production-Readiness Assessment & Roadmap

## Current Assessment

The content management subsystem (`src/content/`, `include/content/`) provides comprehensive functionality for ingesting, processing, versioning, and managing diverse content types (text, images, audio, CAD, GEO, structured data). However, several critical gaps prevent it from being **100% production ready**:

### Identified Gaps

- **VersionManager: In-Memory Storage**: Uses `std::unordered_map` for version storage (see `version_manager.cpp:50`); no persistence to RocksDB
- **No Version Retention Policy**: No automatic cleanup, version limits, or storage quota management for versions
- **No Atomic Operations**: ContentFS operations lack transaction guarantees; partial failures could leave inconsistent state
- **No Deduplication Enforcement**: ContentMeta includes `hash_sha256` field but no global deduplication logic is implemented
- **No Storage Quota Management**: No per-user, per-tenant, or global storage limits enforced
- **ContentPolicy: No Runtime Updates**: Policy changes require restart; no hot-reload or API for dynamic policy updates
- **No Rate Limiting**: AsyncIngestionWorker lacks rate limiting per user/tenant; vulnerable to abuse
- **No Telemetry**: Missing metrics for ingestion throughput, queue depth, processing latency, storage usage
- **No Distributed Tracing**: No OpenTelemetry spans for content operations (upload, processing, retrieval)
- **Insufficient Error Handling**: Many operations use generic exceptions; lacks structured error types with context
- **No Content Lifecycle Management**: No TTL, archival, or cold storage migration for old/unused content
- **No Integrity Validation**: No periodic content hash verification to detect corruption
- **Thread Safety Gaps**: ContentManager operations lack documented concurrency guarantees
- **No Observability**: No audit logs for content access, modification, deletion
- **No Backup/Restore**: No tools for exporting/importing content with metadata preservation
- **Missing Plugin Validation**: AsyncIngestionWorker plugins lack schema validation and sandboxing
- **No Content Search Pagination**: Search operations may load all results into memory
- **No Disaster Recovery**: No replication strategy or multi-region content sync

---

## Roadmap

### Stabilität & Sicherheit (Stability & Security)

- **Persistent Version Storage**
  - Migrate VersionManager from in-memory map to RocksDB-backed storage
  - Use key schema: `version:<content_id>:<version_num>` for version metadata
  - Implement atomic version creation with timestamp-ordered keys

- **Version Retention & Quota Management**
  - Add configurable version retention policy (e.g., keep last N versions, or versions from last M days)
  - Implement storage quota per user/tenant with configurable limits
  - Add automatic version pruning based on retention policy
  - Provide admin API to query and adjust quotas

- **Atomic Content Operations**
  - Wrap ContentFS multi-key operations (metadata + blob + chunks) in RocksDB transactions
  - Ensure rollback on partial failure (e.g., metadata written but blob failed)
  - Add idempotency keys for upload operations to prevent duplicate ingestion

- **Global Content Deduplication**
  - Implement reference-counted storage based on `hash_sha256`
  - Store blobs at `blob:<sha256>` with reference count
  - ContentMeta points to blob hash; delete blob only when refcount reaches zero
  - Add dedup statistics (space saved, duplicate uploads blocked)

- **Storage Quota Enforcement**
  - Track storage usage per user/tenant in separate counters (`quota:<user_id>:used`)
  - Reject uploads that would exceed quota with clear error messages
  - Provide admin dashboard for quota monitoring and adjustment
  - Support quota grace periods and soft/hard limits

- **Dynamic Content Policy Updates**
  - Store ContentPolicy in RocksDB (`config:content_policy`)
  - Add HTTP API endpoint to update policy dynamically (`PUT /admin/content-policy`)
  - Implement policy validation and rollback on invalid updates
  - Broadcast policy changes to all worker nodes (pub/sub or polling)

- **Rate Limiting for Ingestion**
  - Implement token bucket or sliding window rate limiter per user/tenant
  - Add configurable limits for requests/minute and bytes/minute
  - Return HTTP 429 (Too Many Requests) with Retry-After header
  - Track rate limit violations in audit logs

- **Content Integrity Verification**
  - Add periodic background job to verify stored content hashes
  - Detect and alert on hash mismatches (corruption)
  - Support manual verification via admin API
  - Integrate with backup/restore workflows

- **Thread-Safety Guarantees**
  - Document thread-safety model for ContentManager, ContentFS, VersionManager
  - Add reader-writer locks where needed (e.g., ContentPolicy updates)
  - Ensure AsyncIngestionWorker job queue is lock-free or uses fine-grained locking
  - Add thread sanitizer checks to CI pipeline

---

### Korrektheit & Tests (Correctness & Tests)

- **Unit Tests for VersionManager**
  - Test version creation, retrieval, history queries
  - Test edge cases: empty history, duplicate versions, concurrent creation
  - Test persistence: verify versions survive restart (after migration to RocksDB)

- **Integration Tests for ContentFS**
  - Test atomic operations: verify rollback on partial failure
  - Test chunked vs. non-chunked storage with various sizes
  - Test range reads with edge cases (offset beyond size, zero-length, etc.)
  - Test concurrent uploads and retrieval

- **Deduplication Tests**
  - Test identical content uploaded by different users
  - Test reference counting: verify blob deletion only when refcount = 0
  - Test space savings calculation

- **Policy Validation Tests**
  - Test ContentPolicy with allow/deny lists, size limits
  - Test dynamic policy updates without restart
  - Test policy enforcement across all ingestion paths

- **AsyncIngestionWorker Tests**
  - Test job queuing, prioritization, cancellation
  - Test worker thread lifecycle (start, stop, graceful shutdown)
  - Test failure handling and retry logic
  - Test progress tracking and callbacks

- **Rate Limiting Tests**
  - Test rate limit enforcement per user/tenant
  - Test burst handling and token bucket refill
  - Test rate limit reset after time window

- **Fuzz Testing**
  - Fuzz MIME type detection with malformed files
  - Fuzz content chunking with random byte sequences
  - Fuzz JSON metadata parsing
  - Fuzz archive extraction (ZIP, TAR, 7Z) with malicious payloads

- **Regression Tests**
  - Add CI validation for ContentPolicy schema
  - Test that all content processors (PDF, image, audio, CAD) handle edge cases
  - Test malware scanner integration with known malicious samples (disabled by default)

---

### Observability & Operations (Observability & Operations)

- **Metrics (Prometheus/OpenTelemetry)**
  - Ingestion metrics: `content_ingestion_requests_total`, `content_ingestion_bytes_total`
  - Processing metrics: `content_processing_duration_seconds`, `content_processing_failures_total`
  - Storage metrics: `content_storage_used_bytes`, `content_storage_dedup_savings_bytes`
  - Queue metrics: `content_ingestion_queue_depth`, `content_ingestion_queue_wait_time_seconds`
  - Version metrics: `content_versions_total`, `content_version_storage_bytes`
  - Policy metrics: `content_policy_rejections_total` (by reason)
  - Rate limit metrics: `content_rate_limit_exceeded_total`

- **Tracing Spans**
  - Add OpenTelemetry spans for all content operations:
    - `content.upload` (with size, mime_type, user_id attributes)
    - `content.process` (with processor type, duration)
    - `content.version.create`
    - `content.dedup.check`
    - `content.chunk` (with chunk count, embedding dimension)
  - Include parent/child relationships for archives and structured content

- **Audit Logs**
  - Log all content lifecycle events: upload, update, delete, access
  - Include user context, timestamps, IP addresses, request IDs
  - Support log export to SIEM systems (JSON format)
  - Add retention policy for audit logs (default: 90 days)

- **Alerts for Anomalies**
  - Alert on ingestion failures exceeding threshold (e.g., >5% failure rate)
  - Alert on storage quota approaching limit (e.g., >80% used)
  - Alert on malware detection (immediate notification)
  - Alert on content corruption detected during integrity checks
  - Alert on rate limit abuse (sustained violations)

- **Admin Dashboard**
  - Visualize storage usage by user/tenant/content type
  - Display ingestion queue depth and processing throughput
  - Show top content consumers (by storage, by upload frequency)
  - List failed ingestion jobs with error details
  - Show deduplication statistics (space saved, duplicate count)

---

### API/Config & DX (Developer Experience)

- **Structured Error Types**
  - Replace generic `std::runtime_error` with specific error types:
    - `ContentNotFoundError`, `QuotaExceededError`, `PolicyViolationError`
    - `DuplicateContentError`, `CorruptedContentError`, `IngestionFailedError`
  - Include error context: content ID, user ID, quota limits, policy rule violated
  - Provide actionable error messages with remediation steps

- **Content Search API Enhancements**
  - Add pagination support (cursor-based or offset/limit)
  - Add support for streaming large result sets
  - Add faceted search (filter by category, mime_type, date range)
  - Add full-text search within content (leveraging fulltext index)

- **Admin API for Content Management**
  - `GET /admin/content/stats` - Global content statistics
  - `GET /admin/content/quota/:user_id` - Query user quota and usage
  - `PUT /admin/content/quota/:user_id` - Update user quota
  - `POST /admin/content/verify-integrity` - Trigger integrity check
  - `DELETE /admin/content/prune-versions` - Manually prune old versions

- **Content Lifecycle Policies**
  - Add TTL configuration per content category (e.g., temp files expire after 7 days)
  - Support archival to cold storage (S3 Glacier, Azure Archive) after N days of inactivity
  - Provide API to extend TTL or restore archived content

- **Plugin SDK Improvements**
  - Add JSON Schema validation for plugin configuration
  - Provide plugin lifecycle hooks: `onRegister`, `onUnregister`, `onError`
  - Add plugin health checks and auto-restart on failure
  - Support plugin versioning and compatibility checks

- **Dry-Run Mode for Ingestion**
  - Add `--dry-run` flag to ingestion API
  - Validate content without storing (check policy, malware, quota)
  - Return detailed report: estimated storage, processing time, policy violations

---

### Security/Privacy (Security/Privacy)

- **Content Encryption at Rest**
  - Encrypt content blobs using AES-256-GCM with per-tenant keys
  - Store encryption metadata in ContentMeta (`encrypted`, `encryption_type`, `key_id`)
  - Integrate with key management systems (KMS, Vault)
  - Support key rotation without re-encrypting all content (envelope encryption)

- **Access Control for Content**
  - Implement role-based access control (RBAC) for content operations
  - Support per-content ACLs (owner, readers, writers)
  - Audit all content access attempts (authorized and denied)
  - Integrate with external identity providers (OAuth2, SAML)

- **Malware Scanning Enforcement**
  - Make malware scanning mandatory for all ingested content (configurable)
  - Quarantine detected threats with admin notification
  - Support multiple malware scanner backends (ClamAV, VirusTotal API)
  - Add malware signature update monitoring and alerts

- **Content Redaction/Sanitization**
  - Support PII redaction in text content (regex-based or ML-based)
  - Add watermarking for sensitive content (images, documents)
  - Provide audit trail for redaction operations

- **Secure Plugin Execution**
  - Run ingestion plugins in sandboxed environments (containers, seccomp)
  - Limit plugin resource usage (CPU, memory, network)
  - Validate plugin signatures before loading
  - Isolate plugin failures to prevent system-wide crashes

---

### Delivery & Governance (Delivery & Governance)

- **CI Gates for Content Policy**
  - Add CI check to validate ContentPolicy JSON schema
  - Ensure all MIME types have explicit allow/deny rules
  - Fail build on invalid policy configuration

- **Performance Benchmarks**
  - Add CI benchmarks for ingestion throughput (files/sec, MB/sec)
  - Track regression in processing latency (P50, P95, P99)
  - Benchmark deduplication overhead
  - Benchmark vector embedding generation time

- **Feature Flags for New Features**
  - Add feature flags for: deduplication, rate limiting, encryption, archival
  - Support gradual rollout per environment (dev, staging, prod)
  - Allow runtime toggling via admin API

- **Runbooks for Operations**
  - Document ingestion failure troubleshooting
  - Provide rollback procedures for content policy changes
  - Include disaster recovery procedures (backup, restore, replication)
  - Add monitoring and alerting setup guide

- **Backup & Restore Tools**
  - CLI tool to export content with metadata (`themis-content-export`)
  - Support incremental backups (only changed content since last backup)
  - CLI tool to restore content from backup (`themis-content-restore`)
  - Verify content integrity during restore (hash validation)

- **Multi-Region Replication**
  - Add support for content replication across regions
  - Implement eventual consistency model for replicated content
  - Provide conflict resolution strategies (last-write-wins, versioning)
  - Add replication lag monitoring and alerts

---

## Implementation Priority

1. **High Priority** (Weeks 1-4): Persistent version storage, atomic operations, structured errors, basic metrics, thread-safety documentation
2. **Medium Priority** (Weeks 5-8): Deduplication, storage quotas, dynamic policy updates, rate limiting, audit logs, unit/integration tests
3. **Low Priority** (Weeks 9-12): Content encryption, ACLs, lifecycle policies, backup/restore, multi-region replication, admin dashboard

## Related Documentation

- [Content Manager Source Code](../src/content/content_manager.cpp)
- [Version Manager Source Code](../src/content/version_manager.cpp)
- [ContentFS Source Code](../src/content/content_fs.cpp)
- [Async Ingestion Worker Source Code](../src/content/async_ingestion_worker.cpp)
- [Architecture Overview](de/architecture/ARCHITECTURE_OVERVIEW.md)
- [Security Framework](../SECURITY.md)
