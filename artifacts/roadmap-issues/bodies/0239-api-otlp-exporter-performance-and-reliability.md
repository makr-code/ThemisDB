### Context

This issue implements the roadmap item 'OTLP Exporter Performance and Reliability' for the api domain. It is sourced from the consolidated roadmap under 🟢 Low Priority — Future (v1.9.0+) and targets milestone v2.1.0.

Primary detail section: OTLP Exporter Performance and Reliability

### Goal

Deliver the scoped changes for OTLP Exporter Performance and Reliability in src/api/ and complete the linked detail section in a release-ready state for v2.1.0.

### Detailed Scope

### OTLP Exporter Performance and Reliability
**Priority:** Medium
**Target Version:** v2.1.0

`otlp_exporter.cpp` implements an async queue + background-thread OTLP/HTTP exporter using libcurl. Two structural inefficiencies limit throughput and reliability at production scale.

**Implementation Notes:**
- `[ ]` **New `CURL*` handle per flush batch** (`otlp_exporter.cpp::flushBatch()`): every call to `flushBatch()` opens a new TCP connection via `curl_easy_init()` and cleans up with `curl_easy_cleanup()` after the POST. Under the default flush interval (5 s) with 64-span batches this is infrequent, but if the batch interval is reduced or the collector is remote, connection setup becomes the dominant latency. Replace with a persistent `CURL*` handle created once in `start()` and reused across batches (set `CURLOPT_FORBID_REUSE=0L` and `CURLOPT_TCP_KEEPALIVE=1L`).
- `[ ]` **`queue_` uses `std::vector` with `erase(begin, begin+n)` dequeue** (`otlp_exporter.h` + `otlp_exporter.cpp::flushLoop()`): the internal span queue is a `std::vector<SpanData>` and the dequeue path calls `queue_.erase(queue_.begin(), queue_.begin() + take_offset)`, which is O(n) because it shifts all remaining elements. Replace with `std::deque<SpanData>` or a fixed-size ring buffer to get O(1) pop-front at the cost of a trivial container change.
- `[ ]` **No retry on transient HTTP errors**: `flushBatch()` logs a warning and drops the entire batch if the collector returns a non-2xx status or `curl_easy_perform` fails. Add exponential-backoff retry (e.g., up to 3 attempts, 100/200/400 ms delays) for retriable status codes (429, 503) before dropping, to survive brief collector restarts without data loss.
- `[ ]` **`droppedSpanCount` metric not exposed via Prometheus**: `OtlpExporter::droppedSpanCount()` and `exportedSpanCount()` exist but are not wired to the Prometheus `/metrics` endpoint. Register `otlp_spans_exported_total` and `otlp_spans_dropped_total` counters in the Prometheus registry at `OtlpExporter::start()` time.

**Performance Targets:**
- Span enqueue (hot path) < 500 ns per call (single lock acquire + vector push_back or deque push_back).
- Flush of 64 spans to a local OTLP collector < 5 ms end-to-end (reusing a persistent connection).

---

### Acceptance Criteria

- [ ] **New `CURL*` handle per flush batch** (`otlp_exporter.cpp::flushBatch()`): every call to `flushBatch()` opens a new TCP connection via `curl_easy_init()` and cleans up with `curl_easy_cleanup()` after the POST. Under the default flush interval (5 s) with 64-span batches this is infrequent, but if the batch interval is reduced or the collector is remote, connection setup becomes the dominant latency. Replace with a persistent `CURL*` handle created once in `start()` and reused across batches (set `CURLOPT_FORBID_REUSE=0L` and `CURLOPT_TCP_KEEPALIVE=1L`).
- [ ] **`queue_` uses `std::vector` with `erase(begin, begin+n)` dequeue** (`otlp_exporter.h` + `otlp_exporter.cpp::flushLoop()`): the internal span queue is a `std::vector<SpanData>` and the dequeue path calls `queue_.erase(queue_.begin(), queue_.begin() + take_offset)`, which is O(n) because it shifts all remaining elements. Replace with `std::deque<SpanData>` or a fixed-size ring buffer to get O(1) pop-front at the cost of a trivial container change.
- [ ] **No retry on transient HTTP errors**: `flushBatch()` logs a warning and drops the entire batch if the collector returns a non-2xx status or `curl_easy_perform` fails. Add exponential-backoff retry (e.g., up to 3 attempts, 100/200/400 ms delays) for retriable status codes (429, 503) before dropping, to survive brief collector restarts without data loss.
- [ ] **`droppedSpanCount` metric not exposed via Prometheus**: `OtlpExporter::droppedSpanCount()` and `exportedSpanCount()` exist but are not wired to the Prometheus `/metrics` endpoint. Register `otlp_spans_exported_total` and `otlp_spans_dropped_total` counters in the Prometheus registry at `OtlpExporter::start()` time.
- [ ] Span enqueue (hot path) < 500 ns per call (single lock acquire + vector push_back or deque push_back).
- [ ] Flush of 64 spans to a local OTLP collector < 5 ms end-to-end (reusing a persistent connection).

### Relationships

- Roadmap row: #239 (🟢 Low Priority — Future (v1.9.0+))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/api/FUTURE_ENHANCEMENTS.md#otlp-exporter-performance-and-reliability
- Source key: roadmap:239:api:v2.1.0:otlp-exporter-performance-and-reliability

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:239:api:v2.1.0:otlp-exporter-performance-and-reliability -->
<!-- roadmap-ref: row=239;module=api;target=v2.1.0 -->
<!-- roadmap-detail: src/api/FUTURE_ENHANCEMENTS.md#otlp-exporter-performance-and-reliability -->
