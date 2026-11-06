---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-11-06"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Ingestion Research and Evaluation

## Purpose and scope

This page records the evidence and evaluation method for ingestion behavior that
can be checked against this repository. It is for module developers and
reviewers; it is not a claim that every connector or deployment has been
benchmarked. Runtime contracts are defined in
[`ARCHITECTURE.md`](./ARCHITECTURE.md),
[`ingestion_api_contract.h`](../../include/ingestion/ingestion_api_contract.h),
and the public interfaces under [`include/ingestion/`](../../include/ingestion/).

## Questions and assumptions

The evaluation concerns four observable questions:

1. Does source orchestration report successful and failed documents separately?
2. Are validation, checkpoint, quarantine, and retry outcomes explicit?
3. Do workflow and connector failures stop or reject work rather than silently
   appearing as successful extraction?
4. Can performance changes be compared on the same benchmark case and baseline?

Evaluation inputs are connector-specific. A filesystem walk, a paginated HTTP
endpoint, a Kafka topic, and an object store have different completeness and
availability conditions; their results are not directly comparable without
holding source content, connector options, and downstream sinks constant.
`SourceConfig::options` also includes limits where `0` means unlimited for some
connectors, so test fixtures and production profiles must set deliberate bounds
for untrusted or unbounded sources.

## Evidence and methods

| Property | Evidence in this repository | Review method |
|---|---|---|
| Error classes | [`ingestion_error_contract.h`](../../include/ingestion/ingestion_error_contract.h), [`ingestion_api_contract.h`](../../include/ingestion/ingestion_api_contract.h) | Check classification and the exact error contract used by the caller. |
| Connector and lifecycle outcomes | [`tests/ingestion/`](../../tests/ingestion/) | Run focused connector, retry, checkpoint, schema, and quarantine regressions. |
| Reindex triggers | [`test_reindex_decision_engine.cpp`](../../tests/ingestion/test_reindex_decision_engine.cpp) | Verify each metadata-change trigger and unchanged-metadata behavior. |
| Hot-path targets | [`bench_ingestion_release_gates.cpp`](../../benchmarks/ingestion/bench_ingestion_release_gates.cpp) | Compare the six named cases against their release thresholds and baseline. |
| Extraction and quality paths | [`bench_ingestion_extraction.cpp`](../../benchmarks/ingestion/bench_ingestion_extraction.cpp), [`bench_ingestion_quality_judge.cpp`](../../benchmarks/ingestion/bench_ingestion_quality_judge.cpp) | Separate deterministic fallback/stub measurements from a configured live model. |

The release-gate benchmark explicitly uses an in-memory mock WAL and string
serialization for its write/batch cases. Its thresholds are useful regression
targets for those measured operations, but they do not establish end-to-end
database durability, connector throughput, or production capacity. Likewise, a
scripted quality-judge backend measures parsing and orchestration, not model
quality.

## Evaluation protocol

For a behavior change, keep a reproducible source fixture and connector options,
then verify the outcome at the narrowest applicable layer:

1. Run the focused `tests/ingestion/test_*.cpp` regression for that interface.
2. For error handling, assert the returned structured code and whether the
   document is processed, failed, or quarantined; do not infer success from a
   non-empty report alone.
3. For restart behavior, test checkpoint read/write and replay boundaries using
   the checkpoint implementation configured by the test.
4. For performance, run the mapped release-profile benchmark and compare the
   same case, hardware class, repetitions, and baseline. Do not compare a mock
   backend to a live remote connector as if they were equivalent.
5. Record whether an inference backend was available. An unavailable quality
   backend can produce an unevaluated, fail-open report, so its `passed` flag
   alone is not evidence that model-based quality criteria were evaluated.

## External research and implications

The sources below support engineering principles, not claims that ThemisDB
already implements or meets them. The links were identified/reviewed on
2026-11-06. A direct page fetch was unavailable in this sandbox; re-check the
linked specifications and living guidance during design sign-off.

| Research finding | Source | Implication for ingestion |
|---|---|---|
| Stream processing requires explicit trade-offs among correctness, latency, and cost for unbounded and out-of-order data. | Akidau et al., [“The Dataflow Model”](https://research.google.com/pubs/pub43864.html), PVLDB 8 (2015), pp. 1792–1803. | State ordering, lateness, completeness, and freshness expectations per source rather than promising one global behavior. |
| Backpressure propagates from a slower downstream consumer; checkpoint barriers can take longer to propagate while the pipeline is backpressured. | Apache Flink, [Monitoring Back Pressure](https://nightlies.apache.org/flink/flink-docs-stable/docs/ops/monitoring/back_pressure/), [Checkpointing](https://nightlies.apache.org/flink/flink-docs-stable/docs/dev/datastream/fault-tolerance/checkpointing/), and [Checkpointing under backpressure](https://nightlies.apache.org/flink/flink-docs-stable/docs/ops/state/checkpointing_under_backpressure/), current documentation. | Measure queue occupancy, producer wait/backpressure, checkpoint duration/lag, and downstream sink latency; stress recovery while the sink is slow, not only in an idle pipeline. |
| Fault-tolerant stream recovery requires durable state and a replayable source. Kafka's exactly-once consume/process/produce flow relies on transactional output and consumed-position updates; that guarantee does not automatically cover arbitrary external sinks. | Apache Flink, [Checkpointing](https://nightlies.apache.org/flink/flink-docs-stable/docs/dev/datastream/fault-tolerance/checkpointing/); Apache Kafka, [Design — Message Delivery Semantics](https://kafka.apache.org/design/). | Publish per-connector delivery guarantees. Prefer replay plus sink idempotency where a shared atomic transaction is unavailable; claim exactly-once only for participating components with tested coordination. |
| HTTP idempotency concerns the intended effect of repeating the same request; the property is what makes some retries safe after communication failures. | IETF, [RFC 9110 §9.2.2](https://www.rfc-editor.org/rfc/rfc9110.html#section-9.2.2), June 2022. | Define stable record identity and write/retry semantics separately. Test timeout-and-retry behavior at the actual sink rather than inferring deduplication from checkpointing. |
| User-controlled URLs can create SSRF; defensive guidance includes strict destination validation and attention to DNS resolution and redirects. For file intake, do not trust a supplied MIME type alone and impose file-size/type controls. | OWASP, [SSRF Prevention Cheat Sheet](https://cheatsheetseries.owasp.org/cheatsheets/Server_Side_Request_Forgery_Prevention_Cheat_Sheet.html) and [File Upload Cheat Sheet](https://cheatsheetseries.owasp.org/cheatsheets/File_Upload_Cheat_Sheet.html), living guidance. | Treat crawler/API fetches and document parsers as untrusted-input boundaries. Test denied address classes, redirect resolution, size limits, and malformed content using injected transports; production network policy remains deployment-owned. |
| A closed-loop benchmark generator can itself be throttled by backpressure and understate offered load. | Dixon, Forshaw, and Matthews, [“Monitor, Mitigate, Moderate: Backpressure in Stream Benchmark Generators”](https://doi.org/10.1145/3777911.3800700), ACM/SPEC ICPE Companion 2026. | Record offered as well as accepted/completed work, generator utilization, and backpressure. A completed-records/sec result alone may conceal a load-generator bottleneck. |
| Reproducible benchmark interpretation depends on a declared workload and system configuration; general serving/decision-support suites are not ingestion-specific evidence. | Cooper et al., [“Benchmarking Cloud Serving Systems with YCSB”](https://doi.org/10.1145/1807128.1807152), SoCC 2010; TPC, [TPC-DS v4.0 specification](https://www.tpc.org/TPC_Documents_Current_Versions/pdf/TPC-DS_v4.0.0.pdf). | Publish ingestion-specific source mix, payload sizes/types, concurrency, retry/failure conditions, hardware, software/build, and sink configuration. Do not claim YCSB or TPC-DS validates this module. |

### Code-to-research gap analysis

| Concern | Current repository evidence | Gap and planned release response |
|---|---|---|
| Application usability and connector capability | The source types cover local files, APIs, databases, Kafka/CDC, object storage, web crawling, and dataset-oriented paths. `src/ingestion/MODULE_GUIDE.md` recommends preview/dry-run and checking the selected sink; connector options and integrations differ. | Connector breadth alone is not a support matrix or deployment recipe. v3.0.0 Wave 5 publishes per-connector capabilities, required configuration, limits, and delivery guarantees, with examples validated by connector tests. |
| Success, acknowledgement, and output | `src/ingestion/IMPLEMENTATION.md` documents that the filesystem workflow returns entities without handing them to a sink. Sink interfaces are separately declared in `include/ingestion/ingestion_sinks.h`. | A processed count does not establish a persisted result. v2.5.0 Wave 1 wires or explicitly rejects this path and tests commit/failure outcomes. |
| Error-contract consistency | `src/ingestion/IMPLEMENTATION.md` records conflicting `IngestionErrorCode` declarations; `tests/ingestion/CMakeLists.txt` skips `test_ingestion_error_contract_focused.cpp`. | Resolve the canonical error contract and register passing focused coverage before the v2.5.0 release gate. |
| Bounded intake and backpressure | `tests/ingestion/test_ingestion_queue_saturation_stress.cpp` and `test_queue_boundary_focused.cpp` exercise queue saturation; connector limits and zero-as-unlimited behavior vary. `test_ingestion_web_crawler.cpp` covers scheme validation. | Tests establish selected local behaviors, not consistent limits, address-level SSRF protection, or all connector paths. v2.5.0 Wave 2 adds connector-specific finite profiles and hostile-input regressions. |
| Checkpoint and replay | `src/ingestion/ARCHITECTURE.md` documents in-memory default coordinator checkpoint/leader-election components; checkpoint tests cover configured implementations. | No module-wide durable cross-host guarantee is established. v2.5.0 Wave 3 proves restart/replay with a selected durable store; v3.0.0 Wave 6 adds shared coordination only for capable source/sink pairs. |
| Benchmark validity | `benchmarks/ingestion/README.md` and `src/ingestion/PERFORMANCE_EXPECTATIONS.md` state that release-gate cases use mocks/in-memory operations and no representative-hardware run/baseline is linked. | Existing thresholds are regression targets for their measured operations, not production capacity. v2.5.0 Wave 4 records production-shaped runs and offered-load metadata; the 10% matched-case p99 guard is a proposed threshold pending baseline review. |
| Governance and audit | `include/ingestion/ingestion_manager.h` exposes an in-process admin surface; `src/ingestion/SECURITY.md` leaves network authorization, retention, and deployment controls to the caller/service. | The module does not prove service-level authorization or compliance. v3.0.0 Wave 7 requires owner decisions and server-level authorization, tenant-isolation, redaction, and retention evidence. |
| Embedding/index lifecycle | `test_reindex_decision_engine.cpp` covers the decision helper; `src/ingestion/ROADMAP.md` states that dual-read, traffic routing, and manifest durability are not established by that helper. | v3.0.0 Wave 8 either implements and tests the approved lifecycle or explicitly defers it; do not infer migration behavior from decision logic alone. |

The measurements and exact acceptance gates remain open until Wave 1 owners
select sink/checkpoint implementations and Wave 4 records representative
baselines. The wave-level estimates and targets in
[`ROADMAP.md`](./ROADMAP.md) are planning proposals, not measured current
performance or release commitments.

## Limits and open evidence

- The quality dimensions and default thresholds are configuration values, not a
  published calibration against a labeled ingestion corpus.
- Benchmarks do not cover every connector, deployment topology, or sustained
  production load.
- `RESEARCH.md` does not promote the design examples in
  `EMBEDDING_VERSION_GOVERNANCE.md` into runtime behavior; see
  [`EMBEDDING.md`](./EMBEDDING.md) for what the current decision engine actually
  implements.
- There is no module-specific compiled evidence bundle under
  `ai_context/developer_llm_wiki/modules/ingestion/` in this checkout. The
  source, tests, and benchmarks linked above are the verification sources for
  this page.
- External literature and security guidance inform the proposed design but do
  not establish compliance, a vulnerability finding, or a ThemisDB performance
  target.

## References

- [`ROADMAP.md`](./ROADMAP.md) — active work and acceptance state.
- [`PERFORMANCE_EXPECTATIONS.md`](./PERFORMANCE_EXPECTATIONS.md) — benchmark
  mapping and module-level regression expectations.
- [`MODULE_GUIDE.md`](./MODULE_GUIDE.md) — focused test and benchmark entry points.

> Exact model backend not verifiable from final document text alone.
