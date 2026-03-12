### Context

This issue implements the roadmap item 'HuggingFace Hub Client: HTTP 429 Back-Off' for the exporters domain. It is sourced from the consolidated roadmap under 🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.8.0.

Primary detail section: HuggingFace Hub Client: HTTP Rate Limit Handling (429 Back-Off)

### Goal

Deliver the scoped changes for HuggingFace Hub Client: HTTP 429 Back-Off in src/exporters/ and complete the linked detail section in a release-ready state for v1.8.0.

### Detailed Scope

### HuggingFace Hub Client: HTTP Rate Limit Handling (429 Back-Off)
**Priority:** Medium
**Target Version:** v1.8.0

`huggingface_hub_client.cpp` implements exponential retry back-off for file uploads (line 423) but does not check HTTP 429 (Too Many Requests) or the `Retry-After` response header before retrying. Retrying immediately on a 429 wastes the retry budget and may result in account throttling.

**Implementation Notes:**
- `[ ]` After each curl response, check HTTP status 429; if present, parse the `Retry-After` header (seconds or HTTP-date format) and sleep for that duration before retrying.
- `[ ]` Cap total sleep from `Retry-After` at `config_.timeout_seconds` to prevent indefinite blocking.
- `[ ]` Emit a `exporters.huggingface.rate_limit_hit` metric via `ExporterMetrics` whenever a 429 is received.

---


**Priority:** Low
**Target Version:** v2.0.0 (Issue: #1722)

Export a joined view of two or more collections (e.g., `documents JOIN annotations`) into a single JSONL or Parquet output file. Uses the AQL engine to evaluate the join predicate.

**Implementation Notes:**
- Add `JoinExportConfig` struct with `left_collection`, `right_collection`, `join_predicate` (AQL expression), and `output_fields`.
- Implement as a new exporter class `JoinExporter` that opens two `AqlPredicateFilter` cursors and merges record batches.
- PII detection runs on the merged record before serialization.
- Error cases: collection not found, join predicate parse failure, ambiguous field names (rename via `output_fields` alias map).

**Performance Targets:**
- Join export throughput ≥ 50 000 merged docs/sec (hash-join on in-memory right side ≤ 10 M rows).
- Memory budget for right-side hash table ≤ 1 GB configurable.

---

### Acceptance Criteria

- [ ] After each curl response, check HTTP status 429; if present, parse the `Retry-After` header (seconds or HTTP-date format) and sleep for that duration before retrying.
- [ ] Cap total sleep from `Retry-After` at `config_.timeout_seconds` to prevent indefinite blocking.
- [ ] Emit a `exporters.huggingface.rate_limit_hit` metric via `ExporterMetrics` whenever a 429 is received.
- [ ] Add `JoinExportConfig` struct with `left_collection`, `right_collection`, `join_predicate` (AQL expression), and `output_fields`.
- [ ] Implement as a new exporter class `JoinExporter` that opens two `AqlPredicateFilter` cursors and merges record batches.
- [ ] PII detection runs on the merged record before serialization.
- [ ] Error cases: collection not found, join predicate parse failure, ambiguous field names (rename via `output_fields` alias map).
- [ ] Join export throughput ≥ 50 000 merged docs/sec (hash-join on in-memory right side ≤ 10 M rows).
- [ ] Memory budget for right-side hash table ≤ 1 GB configurable.

### Relationships

- Roadmap row: #169 (🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/exporters/FUTURE_ENHANCEMENTS.md#huggingface-hub-client-http-rate-limit-handling-429-back-off
- Source key: roadmap:169:exporters:v1.8.0:huggingface-hub-client-http-rate-limit-handling-429-back-off

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:169:exporters:v1.8.0:huggingface-hub-client-http-rate-limit-handling-429-back-off -->
<!-- roadmap-ref: row=169;module=exporters;target=v1.8.0 -->
<!-- roadmap-detail: src/exporters/FUTURE_ENHANCEMENTS.md#huggingface-hub-client-http-rate-limit-handling-429-back-off -->
