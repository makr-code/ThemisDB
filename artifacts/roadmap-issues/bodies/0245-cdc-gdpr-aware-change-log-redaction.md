### Context

This issue implements the roadmap item 'GDPR-Aware Change Log Redaction' for the cdc domain. It is sourced from the consolidated roadmap under 🟢 Low Priority — Future (v1.9.0+) and targets milestone v2.0.0.

Primary detail section: GDPR-Aware Change Log Redaction

### Goal

Deliver the scoped changes for GDPR-Aware Change Log Redaction in src/cdc/ and complete the linked detail section in a release-ready state for v2.0.0.

### Detailed Scope

### GDPR-Aware Change Log Redaction
**Priority:** Low
**Target Version:** v2.0.0

When a data-subject deletion request arrives, all historical change log entries referencing that subject's key prefix must have their `value` field scrubbed. Implement `CDCAdmin::redactByKeyPrefix(tenant_id, key_prefix)` that rewrites affected log entries in place.

**Implementation Notes:**
- `[x]` Scan RocksDB change log column family for entries where `key` matches `key_prefix`; replace `value` JSON field with `"[REDACTED]"` and append `"redacted":true` to the event JSON.
- `[x]` Preserve `sequence`, `type`, `key`, `timestamp_ms` for audit trail integrity; only `value` is scrubbed.
- `[x]` `CDCAdmin::redactByKeyPrefix(tenant_id, key_prefix, operator_id)` implemented in `src/cdc/cdc_admin.cpp`; returns `GDPRRedactionResult` with scan/redaction counts, timing, and full audit context.
- `[x]` HTTP endpoint `POST /changefeed/redact` exposed via `ChangefeedApiHandler::handleGdprRedact` (requires `cdc:admin` scope).
- `[x]` Unit + integration tests in `tests/test_cdc_gdpr_redaction.cpp` (10 tests covering redaction, audit-field preservation, idempotency, empty-prefix rejection, DELETE events).
- `[ ]` Record a redaction audit log entry in a separate `cdc_redactions` RocksDB column family: `{"key_prefix":"user:42","redacted_count":17,"timestamp_ms":...,"operator":"admin@acme"}`.
- `[ ]` Propagate redaction to Kafka: publish a tombstone record (null value, key = original key) if Kafka producer is configured.
- `[!]` Whether in-flight SSE/WebSocket consumers receive the redacted value or the original is unclear; decision needed before implementation.

**Performance Targets:**
- Redaction of 10,000 events matching a key prefix completes in < 60 s.
- Redaction does not block new change event delivery during execution (background operation with shared RocksDB iterator).

### Acceptance Criteria

- [ ] Scan RocksDB change log column family for entries where `key` matches `key_prefix`; replace `value` JSON field with `"[REDACTED]"` and append `"redacted":true` to the event JSON.
- [ ] Preserve `sequence`, `type`, `key`, `timestamp_ms` for audit trail integrity; only `value` is scrubbed.
- [ ] `CDCAdmin::redactByKeyPrefix(tenant_id, key_prefix, operator_id)` implemented in `src/cdc/cdc_admin.cpp`; returns `GDPRRedactionResult` with scan/redaction counts, timing, and full audit context.
- [ ] HTTP endpoint `POST /changefeed/redact` exposed via `ChangefeedApiHandler::handleGdprRedact` (requires `cdc:admin` scope).
- [ ] Unit + integration tests in `tests/test_cdc_gdpr_redaction.cpp` (10 tests covering redaction, audit-field preservation, idempotency, empty-prefix rejection, DELETE events).
- [ ] Record a redaction audit log entry in a separate `cdc_redactions` RocksDB column family: `{"key_prefix":"user:42","redacted_count":17,"timestamp_ms":...,"operator":"admin@acme"}`.
- [ ] Propagate redaction to Kafka: publish a tombstone record (null value, key = original key) if Kafka producer is configured.
- [ ] Whether in-flight SSE/WebSocket consumers receive the redacted value or the original is unclear; decision needed before implementation.
- [ ] Redaction of 10,000 events matching a key prefix completes in < 60 s.
- [ ] Redaction does not block new change event delivery during execution (background operation with shared RocksDB iterator).

### Relationships

- Roadmap row: #245 (🟢 Low Priority — Future (v1.9.0+))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/cdc/FUTURE_ENHANCEMENTS.md#gdpr-aware-change-log-redaction
- Source key: roadmap:245:cdc:v2.0.0:gdpr-aware-change-log-redaction

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:245:cdc:v2.0.0:gdpr-aware-change-log-redaction -->
<!-- roadmap-ref: row=245;module=cdc;target=v2.0.0 -->
<!-- roadmap-detail: src/cdc/FUTURE_ENHANCEMENTS.md#gdpr-aware-change-log-redaction -->
