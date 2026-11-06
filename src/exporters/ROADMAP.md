---
Author: "ThemisDB Team"
Created: "2026-07-29"
Last Updated: "2026-11-06"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

Exact model backend not verifiable from final document text alone.

# Exporters Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] issue  [P] PR  [?] blocked  [!] unclear -->
<!-- Status: review | source checked: 2026-11-06 -->
<!-- Issues: #5644 (historical sync); #7419 (v2.5.0/v3.0.0 research and wave plan) -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->

## Current Status

The module has production implementations for JSONL, Parquet, Arrow IPC, and
Hugging Face dataset output; streaming, watermark-based incremental, and
in-memory join orchestration; and policy, filtering, PII, encryption, metrics,
and Hub-upload controls. Streaming and incremental inputs are caller-provided
(a cursor for streaming, a vector for incremental); `max_buffer_bytes` is a
flush trigger, not a process-wide memory bound. These distinctions limit claims
of end-to-end bounded or source-consistent exports
([ARCHITECTURE.md](./ARCHITECTURE.md), [README.md](./README.md)).

**Maturity assessment:** functional maturity is high for the existing local
formats and orchestration paths; release-evidence maturity is medium. There is
focused contract coverage and a range of benchmarks, but the inspected
benchmark set mixes exporter-path workloads with synthetic/mock microbenchmarks,
and this review found no recorded representative-hardware baseline or end-to-end
scale result. Treat performance gates as testable hypotheses until runs,
hardware, corpus, and configuration are recorded. The prior #5644 configure
blocker is historical evidence, not a result of this roadmap review.

## In Progress

- [x] complete hardening for format pipelines and orchestration internals (Delivered: Q4 2026)
- [x] align security and governance behavior to bounded runtime contracts (Delivered: Q4 2026)
- [x] standardize fail-closed behavior for unauthorized/unsafe export scenarios (Delivered: Q4 2026)
- [x] unify diagnostics across stream/incremental/join and hub upload failures (Delivered: Q4 2026)

### Wave 3B hardening batch (2026-07-29)
- [x] Incremental exporter now applies `ExportOptions::filter_expression` parity with streaming/jsonl flows (`src/exporters/incremental_exporter.cpp`; `tests/exporters/test_incremental_exporter.cpp`).
- [x] Incremental watermark update is now fail-safe on partial scans (size/error stop) to prevent sequence-skip drift (`src/exporters/incremental_exporter.cpp`; `tests/exporters/test_incremental_exporter.cpp`).
- [x] Join exporter now fails closed when `setRightCollection()` was not called before `exportEntities()` (`src/exporters/join_exporter.cpp`; `include/exporters/join_exporter.h`; `tests/exporters/test_join_exporter.cpp`).
- [x] Join exporter now applies additional `ExportOptions::filter_expression` parity on merged records (`src/exporters/join_exporter.cpp`; `tests/exporters/test_join_exporter.cpp`).
- [x] Streaming exporter encryption path is now deterministic: `encryption_config` takes precedence and legacy + v2 double-encryption ambiguity is removed (`src/exporters/streaming_exporter.cpp`).

## Research and Gap Analysis

The assessment compares the current source, contract, focused tests, and
benchmark entry points with published format and security guidance:

| Focus | Evidence in this module | Gap and implication |
|---|---|---|
| Performance and scale | `bench_exporters.cpp` exercises batch, template, streaming, incremental, Parquet, and CSV-labelled cases; `bench_exporters_release_gates.cpp` has six gates. | Several release gates use mock/in-memory operations rather than full exporter I/O. There is no checked-in capture of hardware, corpus, RSS, or p95/p99 for representative end-to-end runs. Capture reproducible baselines before setting absolute SLOs. |
| Use cases and interoperability | JSONL/LLM, Parquet, Arrow IPC, Hub upload, filtering, stream, delta, and join paths have separate APIs and tests. The contract says same-platform determinism, not cross-platform byte identity. | `filter_expression` is not evaluated by Arrow IPC; incremental export consumes a supplied vector and join materializes the right side in memory. Document and test the format/feature matrix before expanding claims of parity or scale. |
| Security and resilience | Configured policy checks, PII helpers, encryption, typed errors, checkpoint/watermark persistence, and hub retry controls are present. | Pattern-based PII detection is not proof of de-identification. Resume correctness under process/output failure, key lifecycle, incomplete artifact handling, and remote upload fault behavior need explicit fault-injection evidence. CSV formula-injection guidance applies if a production spreadsheet-facing CSV path is supported; current CSV-labelled contract/benchmark cases alone do not prove such a path exists. |
| Governance and maintainability | Export statistics, metrics, error codes, focused tests, module runbook, and API docs exist. | No common export manifest/versioned provenance contract is established across formats. Audit metadata must avoid recording credentials or row-level sensitive values and must not be presented as a compliance certification. |
| Architecture and operations | The module separates serialization, orchestration, safety, and integrations. Hub upload is optional at build/runtime; format availability depends on build support. | The API does not own source snapshot/query consistency; `max_buffer_bytes` is not a total-memory limit. Operator guarantees must state caller, build-feature, and storage assumptions. |

### Research basis and local evidence

- Apache Arrow describes a language-independent in-memory columnar format and
  interoperability mechanisms; Apache Parquet publishes a versioned
  column-oriented file-format specification. These support prioritizing explicit
  schema/null/encoding compatibility tests and measuring actual conversion and
  I/O, not assuming format names guarantee parity:
  [Arrow documentation](https://arrow.apache.org/docs/),
  [Parquet documentation](https://parquet.apache.org/docs/).
- OWASP documents spreadsheet formula injection from untrusted CSV cells,
  including quoting/separator edge cases. Any future spreadsheet-facing CSV
  feature needs an explicit safe-export policy and tests; CSV quoting alone is
  not a security control:
  [OWASP CSV Injection](https://owasp.org/www-community/attacks/CSV_Injection).
- NIST SP 800-53 Rev. 5 includes audit/accountability and information-integrity
  control families. It motivates auditable export metadata and integrity tests,
  but is not evidence that this module or product is certified compliant:
  [NIST SP 800-53 Rev. 5](https://csrc.nist.gov/pubs/sp/800/53/r5/upd1/final).
- Local primary evidence: `include/exporters/exporters_api_contract.h`,
  `include/exporters/exporter_interface.h`, `include/exporters/arrow_ipc_exporter.h`,
  `src/exporters/ARCHITECTURE.md`, `src/exporters/RESEARCH.md`,
  `tests/exporters/CMakeLists.txt`, and
  `benchmarks/exporters/README.md`. The tests directory auto-registers focused
  tests; benchmark registration does not prove a benchmark was built or run.

### Assumptions and open questions

- The root release policy treats 2.5.0 as a minor release and 3.0.0 as a major
  release; this module plan is a proposal, not a committed release date or
  project-wide release gate. API breaks belong only in the 3.0.0 review.
- Storage/source snapshot ownership remains upstream of exporters unless the
  3.0.0 API review explicitly assigns it to this module.
- Representative data sizes, formats, hardware, compliance regimes, and numeric
  production SLOs have not been supplied. Wave 1 records these before thresholds
  are fixed; the percentage gates below are proposed regression limits.
- No production CSV exporter was verified in the scoped source inventory.
  Spreadsheet-safe CSV must remain out of scope until a supported implementation
  and security contract are confirmed.
- The generated developer wiki was last refreshed 2026-11-02 and has no
  `EXPORTERS.md` context page. It is an operational summary, not a source of
  exporters status; this roadmap follows the module code/docs and root
  governance precedence.

## Release Roadmap and Wave Plan

Effort estimates are engineering person-weeks (five workdays each), exclude
cross-module storage work and release-approval time, and need owner confirmation.
Each wave is conditional on its dependencies and measured evidence.

### v2.5.0 — compatible hardening and evidence

#### Wave 2.5-1: Contract matrix and baseline (P0; 2–3 person-weeks)
- **Goal:** make current format and option behavior measurable without changing
  the 2.x public contract.
- **Tasks:**
  - [ ] Build a tested matrix for JSONL, Parquet, Arrow IPC, and Hugging Face
    paths covering null/missing fields, projection, filters, schema errors,
    determinism, build-feature availability, and failure-before-output
    (Target: v2.5.0).
  - [ ] Resolve Arrow filter behavior by implementing parity or documenting a
    typed unsupported-operation result; do not silently ignore the filter
    (Target: v2.5.0).
  - [ ] Capture repeatable end-to-end throughput, p95/p99 latency, output bytes,
    and peak RSS for representative batch, template, stream, incremental, and
    join workloads; record hardware, build flags, fixture shape, and baseline
    artifacts (Target: v2.5.0).
- **Dependencies:** current API contract and benchmark mapping; maintainer
  selection of representative hardware and non-sensitive fixtures.
- **Risks / trade-offs:** closing format gaps can expose previously silent
  behavior; baseline capture may show workload-dependent regressions.
- **Success metrics:** every supported format/option combination is either
  covered or explicitly marked unsupported; every mapped benchmark has a
  reproducible result and baseline metadata; no unexplained output difference
  across repeated same-platform deterministic cases.
- **Validation:** focused `tests/exporters` matrix; build with Arrow enabled and
  disabled where supported; run end-to-end benchmarks at least five repetitions
  on the same host/configuration.

#### Wave 2.5-2: Safe recovery and egress controls (P0; 3–5 person-weeks)
- **Goal:** demonstrate that a failed or resumed export cannot be reported as a
  complete artifact or advance progress beyond successfully written records.
- **Tasks:**
  - [ ] Add process/write-failure and restart tests around streaming checkpoints,
    incremental watermarks, and partial output; define operator action for an
    incomplete artifact (Target: v2.5.0).
  - [ ] Verify configured policy denial produces no output activity, encrypted
    artifact tampering is rejected, and secrets/PII test canaries do not appear
    in audit or error logs (Target: v2.5.0).
  - [ ] Document key ownership/rotation expectations and the limits of
    pattern-based PII detection; do not claim de-identification from a clean
    detector result (Target: v2.5.0).
  - [ ] If a production CSV path is confirmed, specify spreadsheet-safe export
    semantics separately from CSV quoting and add OWASP-derived formula and
    delimiter-injection cases (Target: v2.5.0; otherwise defer).
- **Dependencies:** Wave 2.5-1 fixture and behavior matrix; security-owner review
  of encryption, audit, and CSV scope.
- **Risks / trade-offs:** stricter failures may reject exports that previously
  produced partial files; sanitization changes can alter consumer-visible data.
- **Success metrics:** zero lost or duplicate records in the defined restart
  matrix; no progress state beyond acknowledged writes; policy-denied runs write
  zero artifact bytes; tamper tests fail closed; no secret/canary leakage in
  captured diagnostics.
- **Validation:** deterministic fault injection at write, flush, checkpoint,
  watermark, and upload boundaries; focused security tests; verify cleanup and
  recovery with temporary filesystems and mocked network responses.

#### Wave 2.5-3: Performance gates and operations (P1; 2–3 person-weeks)
- **Goal:** turn current microbenchmarks and operational hooks into reproducible
  release evidence.
- **Tasks:**
  - [ ] Separate mock microbenchmark gates from true exporter-path measurements
    in reports and attach benchmark manifests for all mapped cases (Target:
    v2.5.0).
  - [ ] Add sustained mixed-format, large-record, high-cardinality, and Hub
    throttling/failure scenarios; report throughput, p95/p99, RSS, error rate,
    and recovery time (Target: v2.5.0).
  - [ ] Establish a same-host regression gate of no more than 10% throughput or
    p99 regression against the Wave 2.5-1 baseline, with explicit noise
    tolerance and a documented waiver process (Target: v2.5.0).
- **Dependencies:** Waves 2.5-1 and 2.5-2; release-profile benchmark environment
  and operator/runbook ownership.
- **Risks / trade-offs:** fixed thresholds can be noisy across hardware; use
  pinned runners and compare only like-for-like runs.
- **Success metrics:** 100% mapped benchmark manifest completeness; at least
  five measured repetitions per release case; no unexplained regression above
  10%; runbook contains recovery steps for each new failure scenario.
- **Validation:** benchmark-enabled release build, archived result manifest,
  focused exporter tests, and operator review of dashboards/runbook.

### v3.0.0 — versioned contracts and scalable export architecture

#### Wave 3-1: Breaking-contract design and migration proposal (P0; 2–3 person-weeks)
- **Goal:** define a coherent, versioned export contract before changing public
  interfaces.
- **Tasks:**
  - [ ] Design a versioned manifest covering format/schema fingerprint,
    deterministic config identity, row/byte counts, artifact checksum, source
    snapshot/cursor identity, completion state, and non-sensitive audit
    provenance (Target: v3.0.0).
  - [ ] Specify cancellation, retry, checkpoint acknowledgement, output
    finalization, and idempotent replay semantics; distinguish source snapshot
    guarantees from exporter guarantees (Target: v3.0.0).
  - [ ] Propose a canonical cursor/sink contract for streaming and incremental
    exports and publish 2.x-to-3.0 migration examples and deprecation decisions
    (Target: v3.0.0).
- **Dependencies:** v2.5.0 fault and performance evidence; API-owner, storage,
  security, and downstream `server`/`llm` review.
- **Risks / trade-offs:** a general contract may widen module scope or constrain
  upstream APIs; do not promise exactly-once source semantics without snapshot
  and sink support.
- **Success metrics:** design review signs off every field and state transition;
  migration matrix covers all current public entry points and in-tree consumers;
  no credential or row-level PII fields in manifest schema.
- **Validation:** API compatibility inventory, threat model, sequence/state
  diagrams, and review against Arrow/Parquet schema and version behavior.

#### Wave 3-2: Bounded, resumable core (P0; 6–10 person-weeks)
- **Goal:** implement the approved contract so large exports can resume safely
  without requiring caller-owned full input vectors.
- **Tasks:**
  - [ ] Add a cursor-based incremental path with stable ordering/snapshot
    identity, acknowledged checkpoints, and configuration/schema mismatch
    rejection (Target: v3.0.0).
  - [ ] Add bounded backpressure and output finalization semantics; preserve
    ordered deterministic chunks and make replay safe at the documented sink
    boundary (Target: v3.0.0).
  - [ ] Add versioned manifest generation and format capability negotiation;
    keep optional format backends explicit at build and runtime (Target:
    v3.0.0).
  - [ ] Preserve the existing policy-before-egress invariant for every format,
    sink, and retry path (Target: v3.0.0).
- **Dependencies:** approved Wave 3-1 contract; upstream cursor/snapshot API;
  encrypted temporary artifact and sink design.
- **Risks / trade-offs:** durable state and extra integrity checks add I/O and
  latency; snapshot semantics may require work outside exporters.
- **Success metrics:** peak RSS remains within the documented configured
  working-set budget and grows no more than 10% when input cardinality doubles
  at fixed page/buffer/sink settings (excluding explicitly upstream-owned
  memory); injected restarts produce no missing/duplicate acknowledged records;
  changed source/config fingerprints reject resume; no policy bypass in the
  format/sink test matrix.
- **Validation:** property tests over chunk sizes and interruption points,
  multi-million-record soak, RSS profiling, compatibility tests for each format,
  and fault-injected local/remote sinks.

#### Wave 3-3: Compliance, interoperability, and release acceptance (P1; 3–5 person-weeks)
- **Goal:** establish supportable auditability and 3.0 migration/release evidence
  without implying external certification.
- **Tasks:**
  - [ ] Add manifest integrity and audit-event tests, retention/redaction rules,
    and explicit operator ownership for signing keys and export artifacts
    (Target: v3.0.0).
  - [ ] Validate emitted artifacts with independent Arrow/Parquet readers and
    schema/null/encoding fixtures; define spreadsheet-safe behavior only if
    CSV is approved as a supported format (Target: v3.0.0).
  - [ ] Publish the 2.x migration/deprecation guide, security threat model,
    release benchmark report, and recovery runbook; obtain API and security
    sign-off (Target: v3.0.0).
- **Dependencies:** Wave 3-2 feature complete; independent readers and pinned
  release hardware available.
- **Risks / trade-offs:** external-reader differences may require compatibility
  constraints; audit retention and signed manifests add operational overhead.
- **Success metrics:** all supported formats pass independent-reader fixtures;
  all manifest and audit integrity cases pass; no unresolved high-severity
  exporter security finding; measured throughput/p99 remain within the approved
  Wave 2.5 baseline budget or have a reviewed waiver.
- **Validation:** full exporters test label, cross-format interoperability
  suite, security review, performance comparison, documentation/link check, and
  downstream consumer migration sign-off.

## Implementation Phases

### Phase 1: Design / API Contract
- [x] freeze exporter interface and option contracts for active major line (Delivered: Q3 2026)
- [x] define explicit error taxonomy for policy, filter, and output-failure classes (Delivered: Q3 2026)
  - Contract header: `include/exporters/exporters_api_contract.h`
  - Error codes: EXPORT_FORMAT_UNSUPPORTED, EXPORT_WRITE_FAILED, STREAM_INTERRUPTED,
    SCHEMA_MISMATCH, QUOTA_EXCEEDED, COLUMN_NOT_FOUND, SCHEMA_SERIALIZATION_FAILED,
    EXPORT_ABORTED, INTERNAL_ERROR

### Phase 2: Core Implementation
- [x] complete hardening for format pipelines and orchestration internals (Delivered: Q4 2026)
  - Data-race fixes: `export_encryption.cpp` key_provider mutex (lines 131, 157, 691-692, 875); `huggingface_hub_client.cpp` policy_engine + key_provider mutex (lines 207, 408, 591)
  - RAII: removed redundant `src.close()` in `ExportEncryption::encryptFile/decryptFile`; EVP_CIPHER_CTX mutex boundary for ExportEncryptor
  - O(n²)→O(1): `resolveColumns()` upgraded from `std::set` to `std::unordered_set` in `parquet_exporter.cpp`
  - Parquet write path: removed manual `ofs.close()` (RAII handles it)
- [x] align security and governance behavior to bounded runtime contracts (Delivered: Q4 2026)
  - Retry wait NOLINT comments at `file_backoff.wait()` and `shard_backoff.wait()` (bounded by max_backoff_ms=30'000ms)

### Phase 3: Error Handling and Edge Cases
- [x] standardize fail-closed behavior for unauthorized/unsafe export scenarios (Delivered: Q4 2026)
  - Added `PolicyDeniedException : ExporterException` to `include/exporters/exporter_errors.h`; uses `ERR_EXPORT_POLICY_DENIED`; carries `denial_reason`, `requesting_user`, `collection` fields
  - `isResumableError(EXPORT_ABORTED)` == false by contract (fail-closed)
- [x] unify diagnostics across stream/incremental/join and hub upload failures (Delivered: Q4 2026)
  - Standard log prefix: `[EXPORT_DENIED] collection={} user={} reason={}` in `huggingface_hub_client.cpp` and `jsonl_llm_exporter.cpp`
  - Standard log prefix: `[HUB_UPLOAD_FAILED] repo={} reason={} http_status={}` at hub retry-exhausted return sites
  - New metrics: `recordPolicyDenial()`, `getPolicyDenials()`, `recordHubUploadFailure()`, `getHubUploadFailures()` added to `ExporterMetrics`; keys `exporter_policy_denials_total` and `exporter_hub_upload_failures_total` in `toJson()` and `toString()`
  - Focused tests EXCH-17..24 in `tests/exporters/test_exporters_phase23_hardening_focused.cpp`

### Phase 4: Tests
- [x] expand focused regressions for format, policy, and checkpoint edge scenarios (Delivered: Q3 2026)
- [x] extend deterministic fixture coverage for template and join predicate permutations (Delivered: Q3 2026)
  - Test file: `tests/exporters/test_exporters_contract_hardening_focused.cpp`
  - Test cases: EXCH-01..EXCH-16 (CSV, Parquet, streaming, error contract)
  - kExportersContractSeed = 42; all tests self-contained, no external I/O

### Phase 5: Performance and Hardening
- [x] lock benchmark-backed release gates for exporter throughput and latency hot paths (Delivered: Q3 2026)
- [x] validate p95/p99 and throughput behavior against release baselines (Delivered: Q3 2026)
  - Benchmark file: `benchmarks/exporters/bench_exporters_release_gates.cpp`
  - Gates: ERRG-01..ERRG-06 (CSV ≥1M/s, Parquet ≤5ms, schema ≤100µs,
    null ≤10µs, Arrow ≤1ms, quota ≤50µs)
  - kExportersCanonicalSeed = 42; Repetitions(5)

### Phase 6: Documentation and Acceptance
- [x] core exporters module docs aligned to source-verifiable behavior
- [x] roadmap/future planning separated from historical changelog entries
- [x] Phase 1-6 Wave 3B Category-D closure delivered (Q3 2026)
  - Contract header, 16 focused tests (EXCH-01..16), 6 release-gate benchmarks (ERRG-01..06)

## Production Readiness Checklist

- [x] core exporters surfaces documented and source-verified
- [x] module-level security and failure behavior documented
- [x] benchmark mapping documented in performance expectations
- [x] remaining hardening tasks closed for policy/filter/checkpoint edge paths (Phase 2/3 delivered Q4 2026)
- [x] release benchmark stabilization complete — evidence: bench_exporters_dedicated_gates.cpp (EX-BM-01..04)

## Evidence Summary (Issue #5644 Sync — 2026-07-29)

- Configure attempted: `cmake --preset community-release`
- Result: failed in `cmake/Dependencies.cmake` with
  `RocksDB not found. Install via vcpkg (rocksdb) or system package librocksdb-dev.`
- Follow-up build attempt:
  `cmake --build --preset community-release --target module_exporters_test_exporters_contract_hardening_focused`
  failed with `ninja: error: loading 'build.ninja': No such file or directory`
  because configure did not complete.
- Build/Test status: focused exporters target execution is blocked until
  configure succeeds.
- Last known focused test evidence (from issue context): PASS on
  `module_exporters_test_aql_predicate_filter_focused.exe` (`15 tests`, exit 0,
  validated 2026-07-18).

## Open Work (Issue #5644)

- [~] validate and refine extracted roadmap priorities against full module docs in `src/exporters/ROADMAP.md`
- [~] validate and refine extracted future focus points against full module docs in `src/exporters/FUTURE_ENHANCEMENTS.md`
- [~] add/refresh focused build and test evidence for this module (configure blocker documented in Evidence Summary)
- [~] mark completed synced items and risks with explicit status transitions

## Closure Criteria (Issue #5644)

- [x] all module acceptance criteria updated and traceable in roadmap/future docs
- [~] evidence updated or explicit justified gap documented
- [x] parent epic task entry checked by maintainer
- [x] status labels updated by maintainer before close
- [x] close reason documented as "sync pass complete; configure/test evidence blocked by RocksDB dependency in this environment"

## Known Issues and Limitations

- behavior depends on selected format backend and runtime configuration.
- selected join/stream/incremental edge scenarios need continued hardening.
- benchmark breadth should keep expanding for advanced export workflows.

## Breaking Changes

No breaking exporters contract planned. Any contract-breaking change requires migration notes and changelog entry before merge.

## Program Execution Model — Wave Context

This module is a **contributing module** in the program-level Wave A → B → C → D execution model.
It does not own a primary wave deliverable but must remain `release_critical`-green throughout all waves
and must deliver Wave D operability improvements in Q1 2027.
See [`../../ROADMAP.md`](../../ROADMAP.md) for the full wave model and exit criteria.

### Wave D Contribution for `exporters`
- [x] Deliver or validate distributed tracing, high-cardinality stress coverage, exporter reliability, and operator remediation hints as applicable to this module (Target: Q1 2027) — evidence: tests/integration/test_exporters_soak.cpp, tests/exporters/test_exporters_highcardinality_stress.cpp, docs/operability/RUNBOOK_EXPORTERS.md, benchmarks/exporters/bench_exporters_dedicated_gates.cpp
- [x] Contribute to or validate long-duration soak test coverage for this module's primary paths (Target: Q1 2027) — evidence: tests/integration/test_exporters_soak.cpp (ExportersSoak_MetricExportThroughput, ExportersSoak_TraceExportStability, ExportersSoak_BackpressureReliability)
- [x] Ensure runbook coverage for operator-critical scenarios in this module (Target: Q1 2027) — evidence: docs/operability/RUNBOOK_EXPORTERS.md (5 scenarios: BackendUnavailable, MetricDropStorm, TraceBufferOverflow, QueueStall, CardinalityExplosion)

### Cross-Wave Requirements
- `release_critical` CI must remain green on `develop` throughout all waves (Target: ongoing)
- p95/p99 benchmarks must be refreshed on representative hardware before Wave D sign-off (Target: Q1 2027)
- No behavioral regression may be introduced into modules in Wave A/B/C scope from changes in this module.

### Program-Level Success Criteria (contribution)
- [x] This module's distributed/acceleration paths fail closed (Target: Q1 2027) — evidence: Wave D soak + stress + runbook delivered
- [~] Benchmark-backed p95/p99 baselines exist on representative hardware (Target: Q1 2027) — evidence: bench_exporters_dedicated_gates.cpp (EX-BM-01..04); representative-hardware capture pending
- [x] Operator-critical paths have diagnostics, alerts, and runbooks (Target: Q1 2027) — evidence: docs/operability/RUNBOOK_EXPORTERS.md
