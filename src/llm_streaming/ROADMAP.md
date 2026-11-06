---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-11-06"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

Exact model backend not verifiable from final document text alone.

# LLM Streaming Module Roadmap

> Research and source review date: 2026-11-06. This is a module-level proposal,
> not a release commitment. The root roadmap currently identifies v2.4.0-alpha;
> `VERSIONING.md` identifies v2.5.0 as the next planned minor. No v3.0.0
> milestone or llm_streaming release commitment was found in the canonical
> release sources reviewed.

## Current Status

`src/llm_streaming/` is documentation-only (plus `.gitkeep`), not a runtime
implementation. The actual code is split across the `llm` and `server`
subsystems:

- `StreamingHandler` in `include/llm/streaming_handler.h` and
  `src/llm/streaming_handler.cpp` formats SSE/HTTP chunk strings and adapts a
  token callback to a caller-provided sink. Its atomic counter allocates
  indices, but does not serialize concurrent sink calls or guarantee delivery
  order.
- `GET /api/v1/llm/stream` and OpenAI-compatible `POST /v1/chat/completions`
  with `stream: true` both accumulate SSE output and return a complete
  `http::string_body`; neither currently delivers events incrementally to the
  client. The GET path does not use `StreamingHandler`.
- `tests/llm/test_streaming_handler.cpp` directly tests helper formatting and
  sequential callback use. The module stress test, integration soak, and
  dedicated benchmark use separate in-process stubs, not the helper, HTTP
  routes, or model backends. The existing route test covers unauthenticated
  cases, not successful authenticated generation and delivery.
- No source-backed guarantee for stream cancellation, disconnect propagation,
  bounded buffering/backpressure, stream-specific metrics, or deployment limits
  was found.

**Maturity assessment (planning estimate, not an official project score):**
early/experimental, approximately 1/5 for production streaming. There is useful
formatting code and buffered route functionality, but the streaming runtime
contract and end-to-end evidence are missing. Stub test throughput and benchmark
labels are not runtime performance evidence.

### Code and measurement evidence

| Repository source | Observation used in this roadmap |
|---|---|
| [`include/llm/streaming_handler.h`](../../include/llm/streaming_handler.h), [`src/llm/streaming_handler.cpp`](../../src/llm/streaming_handler.cpp) | Formatting helpers and callback adapter; shared atomic index does not serialize sink calls. |
| [`src/server/llm_api_handler.cpp`](../../src/server/llm_api_handler.cpp), [`src/llm/embedded_llm.cpp`](../../src/llm/embedded_llm.cpp) | GET and OpenAI-compatible routes collect SSE data and return complete `string_body` responses. |
| [`tests/llm/test_streaming_handler.cpp`](../../tests/llm/test_streaming_handler.cpp) | Direct formatting and sequential callback tests; no route transport or concurrent sink-order proof. |
| [`tests/llm/test_llm_aql_explain_stream_api.cpp`](../../tests/llm/test_llm_aql_explain_stream_api.cpp) | Route/auth failure cases; no successful authenticated stream-delivery path. |
| [`tests/llm_streaming/CMakeLists.txt`](../../tests/llm_streaming/CMakeLists.txt), [`test_llm_streaming_highcardinality_stress.cpp`](../../tests/llm_streaming/test_llm_streaming_highcardinality_stress.cpp) | Local stub stress target, stress tier, 300-second timeout; does not link the `llm` implementation. |
| [`tests/integration/test_llm_streaming_soak.cpp`](../../tests/integration/test_llm_streaming_soak.cpp) | Stub-only soak; CI uses a reduced duration, not a production route/backend. |
| [`benchmarks/llm_streaming/bench_llm_streaming_dedicated_gates.cpp`](../../benchmarks/llm_streaming/bench_llm_streaming_dedicated_gates.cpp) | Stub operations with descriptive labels; no executable threshold assertions or route measurements. |

### Research and state-of-the-art findings

| Finding | Evidence | Relevance and limit |
|---|---|---|
| KV-cache allocation and sharing constrain LLM serving batch size and throughput. | Kwon et al., *Efficient Memory Management for Large Language Model Serving with PagedAttention*, SOSP 2023, [arXiv:2309.06180](https://arxiv.org/abs/2309.06180); repository synthesis in [`research/LLM_PROCESSING_OPTIMIZATION_PATTERNS.md`](../../research/LLM_PROCESSING_OPTIMIZATION_PATTERNS.md) and implementation mapping in [`research/implementation_influence/by_module.md`](../../research/implementation_influence/by_module.md). | v3.0.0 should integrate with the selected inference backend's cache/scheduling facilities rather than build a competing cache manager in this documentation-only module. This paper does not benchmark ThemisDB. |
| Prefill and decode have different compute/latency profiles; chunked prefill and scheduling can improve throughput under latency constraints. | Agrawal et al., *Taming Throughput-Latency Tradeoff in LLM Inference with Sarathi-Serve*, 2024, [arXiv:2403.02310](https://arxiv.org/abs/2403.02310); repository synthesis and experimental caveats in [`research/LLM_PROCESSING_OPTIMIZATION_PATTERNS.md`](../../research/LLM_PROCESSING_OPTIMIZATION_PATTERNS.md). | Measure TTFT, inter-token latency, and throughput jointly at realistic concurrency; do not adopt the paper's hardware-specific gains as ThemisDB targets. |
| SSE has a defined `text/event-stream` format, event parsing, and reconnection/`Last-Event-ID` behavior. | [WHATWG HTML Living Standard, Server-sent events](https://html.spec.whatwg.org/multipage/server-sent-events.html). | Wire compatibility, terminal events, and reconnect semantics must be explicit. Reconnect/replay is not implied by the current ThemisDB code and needs a product decision. |
| Generative-AI risk management calls for lifecycle governance and risk identification, measurement, and management. | NIST, *AI RMF Generative AI Profile*, NIST AI 600-1 (2024), [DOI:10.6028/NIST.AI.600-1](https://doi.org/10.6028/NIST.AI.600-1). | Apply data minimization, traceable risk controls, and operational monitoring to stream logs and errors; it is a risk framework, not a streaming implementation specification. |
| Load generators can bottleneck and bias serving latency/throughput measurements under concurrency. | Chandrasekar and Kramberger, *Identifying and Mitigating Systemic Measurement Bias in Production LLM Inference Benchmarks*, 2026 preprint, [arXiv:2605.24217](https://arxiv.org/abs/2605.24217). | Keep client-side queueing/load-generator utilization visible and validate the harness before setting release gates. This is a recent preprint, not a peer-reviewed result. |

These findings support a staged, measurement-led plan, not a claim that one
transport or serving architecture is already selected. External serving
benchmarks are backend-, model-, and hardware-dependent; establish a ThemisDB
baseline before adopting numeric SLOs.

### Evidence-based gap analysis

| Focus | Current evidence | Gap and planned response |
|---|---|---|
| Performance and scaling | Routes buffer a complete response; module benchmark measures local stub operations and prints labels, not enforced gates. | Measure real route TTFT/ITL, throughput, tail latency, memory, and concurrency first (v2.5.0); only then decide on backend scheduling or cache integration (v3.0.0). |
| Use cases and usability | Two SSE-shaped HTTP APIs exist, but successful authenticated end-to-end delivery is not covered. | Define client/product contract, document buffered versus incremental behavior, and test clients against terminal/error events before expanding protocols. |
| Security and resilience | Direct helper tests include JSON escaping; route-level success, disconnect, cancellation, overload, resource bounds, and log-redaction evidence is absent. | Add boundary tests and explicit request/resource budgets; exercise client disconnect, overload, and cleanup at the actual transport boundary. |
| Governance and maintainability | Helper is separate from route; callbacks have no stated concurrency/order guarantee; stub fixtures duplicate modeled behavior. | Assign ownership, state API and failure contracts, keep synthetic results labelled, and require evidence links and human approval for SLOs and protocol changes. |
| Architecture and operations | No verified stream lifecycle/metrics/configuration/runbook; current API is buffered. | Establish production diagnostics, capacity limits, cancellation/timeout behavior, and operator procedures before any readiness claim. |

## In Progress / Planned

- [x] Trace helper, HTTP routes, callback path, and current buffering behavior.
- [x] Separate direct helper evidence from stub stress/soak/benchmark results.
- [ ] Resolve the buffered-versus-incremental delivery decision with the API
      owner and product owner before committing to transport work.
- [ ] Define callback ordering, ownership, terminal/error semantics, timeout,
      cancellation, and per-request resource limits.
- [ ] Deliver route-level correctness and security evidence in the v2.5.0 plan.
- [ ] Treat v3.0.0 serving scale-out and any incompatible contract as strategic,
      contingent work; it is not approved by this roadmap.

## Versioned Wave Plan

Effort estimates are planning ranges in person-weeks for implementation and
focused validation, assuming one experienced C++/server engineer plus part-time
test, security, and operations review. They exclude inference-engine vendor
changes, hardware procurement, and release-wide qualification. Re-estimate
after Wave 1; release owners must confirm inclusion in their release plan.

### v2.5.0 — correctness, contract, and evidence (proposed next-minor scope)

#### Wave 1 — Contract and product decision (Priority: P0; estimate: 1–2 person-weeks)

- **Goal:** define whether current buffered responses satisfy the product use
  case or true incremental delivery is required; make the existing observable
  contract testable.
- **Tasks:**
  - [ ] Document route ownership, authentication, input limits, content type,
        framing, request IDs, terminal/error behavior, and callback
        concurrency/order. (Target: v2.5.0)
  - [ ] Agree on buffered/incremental behavior, supported clients, timeout and
        cancellation semantics, and whether reconnect/replay is required.
        (Target: v2.5.0)
  - [ ] Define benchmark workload, reference hardware/backend, and provisional
        latency/throughput/resource SLOs from a measured baseline; do not reuse
        the stub labels as targets. (Target: v2.5.0)
- **Dependencies:** product/API owner, `LLMApiHandler` owner, deployment profile.
- **Risks / trade-offs:** choosing incremental delivery expands transport
  lifecycle and disconnect complexity; retaining full buffering is simpler but
  has memory and time-to-first-byte costs. Missing owner decisions block Wave 2
  feature selection.
- **Success metrics:** signed-off contract covers both streaming routes and all
  terminal/error cases; benchmark records model/backend, hardware, prompt/output
  distributions, concurrency, and baseline TTFT, ITL, p95/p99 latency,
  throughput, and peak memory.
- **Validation:** API/architecture review against current source; capture
  baseline using an independent load client and report client saturation and
  queueing separately.

#### Wave 2 — Route correctness and bounded transport (Priority: P0; estimate: 3–6 person-weeks)

- **Goal:** make the selected v2.5.0 transport semantics correct and prove them
  through the real request-to-response path.
- **Tasks:**
  - [ ] Add authenticated success-path integration tests for GET and
        OpenAI-compatible POST, including SSE framing, empty/terminal/error
        cases, backend/stub differences, and response headers. (Target: v2.5.0)
  - [ ] If incremental delivery is required, implement it in the server-owned
        transport path with bounded buffering, ordered writes, explicit
        backpressure, and disconnect-to-inference cancellation; do not merely
        expose the formatting helper as a runtime. Otherwise, explicitly retain
        and test bounded buffered behavior. (Target: v2.5.0)
  - [ ] Test helper callback copies, concurrent invocation, sink failure, and
        deterministic ordering only if ordering is part of the accepted
        contract. (Target: v2.5.0)
- **Dependencies:** Wave 1 contract and transport decision; existing inference
  callback/cancellation facilities must be verified before reuse.
- **Risks / trade-offs:** an unbounded queue can exhaust memory; blocking
  inference callbacks can reduce throughput; cancellation may not be supported
  by every backend. Do not promise uniform semantics where an engine cannot
  provide them.
- **Success metrics:** all agreed route contract cases pass; no unbounded
  response accumulation in incremental mode; every stream terminates exactly
  once as complete, failed, timed out, or cancelled; no post-disconnect
  inference remains past the agreed cancellation bound.
- **Validation:** focused helper unit tests; authenticated server integration
  tests against deterministic stub and at least one supported real backend;
  concurrent producer/slow-consumer and disconnect tests under sanitizers where
  available.

#### Wave 3 — Security, resilience, and operations (Priority: P1; estimate: 2–4 person-weeks)

- **Goal:** establish bounded, observable, privacy-conscious runtime behavior.
- **Tasks:**
  - [ ] Enforce/document per-request token, duration, and in-flight stream
        limits; test overload and cleanup on timeout, client disconnect,
        inference error, and server shutdown. (Target: v2.5.0)
  - [ ] Add low-cardinality metrics for active streams, time-to-first-event,
        inter-token latency, completion/error/cancellation counts, and bytes;
        exclude prompts, tokens, credentials, and raw request IDs from metric
        labels. (Target: v2.5.0)
  - [ ] Review authentication/authorization, output encoding, error disclosure,
        and log redaction at both route boundaries; add regression tests for
        sensitive-data leakage. (Target: v2.5.0)
  - [ ] Publish verified configuration and operator guidance for limits,
        timeouts, dashboards, and failure diagnosis. (Target: v2.5.0)
- **Dependencies:** Wave 2 transport and lifecycle contract; metrics/logging
  conventions of the server.
- **Risks / trade-offs:** aggressive limits can reject long generations; high
  cardinality or content-bearing telemetry can leak data and overload metrics
  storage.
- **Success metrics:** all configured limits have boundary tests; no leaked
  stream resources after each injected failure; metrics distinguish outcomes
  without content or unbounded labels; operators can diagnose a failed stream
  using documented non-sensitive identifiers.
- **Validation:** abuse/overload tests, cancellation and shutdown tests,
  security review, privacy/log inspection, and runbook walkthrough.

#### Wave 4 — Representative performance gates and v2.5.0 acceptance (Priority: P1; estimate: 1–3 person-weeks)

- **Goal:** replace stub claims with reproducible end-to-end evidence and
  version-scoped acceptance.
- **Tasks:**
  - [ ] Add a route/backend benchmark with realistic prompt/output lengths,
        concurrency steps, slow clients, and sustained load; record TTFT, ITL,
        p50/p95/p99 latency, completed requests/tokens per second, errors,
        cancellation, and peak memory. (Target: v2.5.0)
  - [ ] Set final SLOs only after comparison to the Wave 1 baseline and a
        representative deployment profile; archive raw results and exact
        hardware/model/backend/configuration. (Target: v2.5.0)
  - [ ] Keep stub stress/soak results explicitly labelled as unit-model tests;
        make any release benchmark gate fail on threshold violations rather
        than printing a threshold label. (Target: v2.5.0)
- **Dependencies:** Waves 1–3; reproducible backend and reference hardware.
- **Risks / trade-offs:** results do not generalize across models or hardware;
  noisy load clients can distort tail-latency numbers.
- **Success metrics:** repeated runs reproduce p95/p99 and throughput within
  the agreed variance; accepted workloads meet owner-approved SLOs without
  memory growth or resource leaks; release evidence is attributable and
  reviewable.
- **Validation:** benchmark-harness self-test, repeated baseline/current runs,
  load-generator saturation check, soak run, and release-owner sign-off.

### v3.0.0 — scalable serving architecture (strategic, contingent scope)

#### Wave 5 — Multi-tenant lifecycle and resource control (Priority: P1; estimate: 4–8 person-weeks)

- **Goal:** design a scalable stream lifecycle and admission policy only if
  measured v2.5.0 workloads show single-request/resource controls are
  insufficient.
- **Tasks:**
  - [ ] Specify stream state transitions, identity/tenant isolation, quotas,
        fair admission, cancellation ownership, bounded queues, and shutdown
        behavior. (Target: v3.0.0)
  - [ ] Prototype and test fairness, overload shedding, and bounded memory
        across slow and fast consumers; avoid implementing a second scheduler
        unless the inference backend cannot provide the required behavior.
        (Target: v3.0.0)
  - [ ] Define restart/retry/reconnect semantics and whether resumable streams
        are needed; do not imply replay from SSE support alone. (Target: v3.0.0)
- **Dependencies:** v2.5.0 route evidence and workload/SLO data; inference
  backend capability review.
- **Risks / trade-offs:** fairness and queueing policy can worsen single-user
  latency; tenant state and replay increase privacy, consistency, and resource
  costs.
- **Success metrics:** at the approved concurrency/tenant profile, memory and
  in-flight work remain within configured bounds; no cross-tenant delivery;
  overload outcomes are deterministic; p95/p99 latency and fairness meet
  explicitly approved SLOs.
- **Validation:** multi-tenant stress, noisy-neighbor, slow-consumer, restart,
  cancellation, and soak tests using production-path components.

#### Wave 6 — Inference-engine scheduling and scale integration (Priority: P2; estimate: 4–8 person-weeks)

- **Goal:** improve serving capacity through backend-native scheduling and
  memory management where measurement shows a bottleneck.
- **Tasks:**
  - [ ] Profile prefill/decode, queue wait, KV-cache use, and stream transport
        separately; select backend-native continuous batching, chunked prefill,
        or cache sharing only when supported and beneficial for the target
        workload. (Target: v3.0.0)
  - [ ] Define backend adapter capability contracts for incremental tokens,
        cancellation, deadlines, batching, and resource reporting; specify
        behavior for backends lacking a capability. (Target: v3.0.0)
  - [ ] Compare candidate scheduler/backend against v2.5.0 with the same model,
        workload, hardware, quality settings, and latency SLO. (Target: v3.0.0)
- **Dependencies:** Waves 4–5; inference provider support and representative
  accelerator/backend availability.
- **Risks / trade-offs:** engine-specific optimization can increase
  portability/maintenance burden; prefill/decode scheduling trades throughput
  against first-token and tail latency.
- **Success metrics:** measured improvement in accepted throughput or capacity
  at unchanged approved p95/p99 and output correctness; no regression beyond
  the agreed compatibility envelope; memory/cache utilization reported for
  each backend.
- **Validation:** backend parity tests, mixed prompt/output workloads,
  concurrency sweeps, output-equivalence checks, and representative-hardware
  benchmarks. Research paper speedups are not acceptance targets.

#### Wave 7 — Protocol/API evolution and strategic qualification (Priority: P2; estimate: 3–6 person-weeks)

- **Goal:** consolidate the mature streaming contract and qualify compatibility,
  deployment, and governance for v3.0.0.
- **Tasks:**
  - [ ] Decide whether to retain current endpoint semantics or introduce a
        versioned protocol/API; publish migration guidance and deprecation
        windows before any incompatible change. (Target: v3.0.0)
  - [ ] Verify standards-conformant SSE framing, event IDs and retry behavior
        where supported, intermediary/proxy behavior, TLS/auth policy, and
        client disconnect propagation. (Target: v3.0.0)
  - [ ] Complete capacity model, dashboards, alerts, runbooks, compliance
        evidence, and human architecture/security approval; keep content out of
        logs and traces unless an approved data policy explicitly allows it.
        (Target: v3.0.0)
- **Dependencies:** Waves 5–6; root release/version governance and API-owner
  approval. Any major-version contract change must follow `VERSIONING.md`.
- **Risks / trade-offs:** an API/protocol change imposes migration costs;
  compliance controls and retained diagnostics can conflict with privacy and
  debugging needs.
- **Success metrics:** compatibility suite passes for supported clients and
  proxy profiles; migration and rollback procedures are exercised; security,
  operations, and API owners sign off; capacity/SLO evidence is traceable.
- **Validation:** protocol conformance tests, compatibility/migration tests,
  deployment failure drills, final security/privacy review, and release
  readiness review.

## Implementation Phases

These mandatory delivery phases map to the versioned waves above. Completion
does not imply a release commitment or readiness sign-off.

### Phase 1: Design / API contract

- [ ] Wave 1: settle buffering versus incremental delivery, route behavior,
      callback ordering, lifecycle, supported clients, and owner-approved
      workload/SLO. (Target: v2.5.0)
- [ ] Wave 5: define multi-tenant and resource-control requirements from
      measured v2.5.0 evidence before choosing a v3 architecture.
      (Target: v3.0.0)

### Phase 2: Core implementation

- [ ] Wave 2: implement only the agreed route contract in the server-owned
      path; use bounded buffering and explicit cancellation/backpressure if
      incremental delivery is selected. (Target: v2.5.0)
- [ ] Waves 5–6: integrate backend-native scheduling/resource capabilities
      where the measured workload justifies them. (Target: v3.0.0)

### Phase 3: Error handling and edge cases

- [ ] Waves 2–3: handle disconnect, timeout, overload, sink/backend failure,
      shutdown, terminal events, and resource cleanup at the real runtime
      boundary. (Target: v2.5.0)
- [ ] Wave 5: test tenant isolation, fairness, and overload behavior.
      (Target: v3.0.0)

### Phase 4: Tests

- [ ] Waves 2–4: add real route integration, helper concurrency-contract,
      failure, load, and soak coverage; keep stub-only evidence distinct.
      (Target: v2.5.0)
- [ ] Waves 5–7: add multi-tenant, backend parity, protocol compatibility,
      migration, and deployment failure tests. (Target: v3.0.0)

### Phase 5: Performance / hardening

- [ ] Wave 4: measure end-to-end latency/throughput/resource KPIs, validate
      load-generator capacity, and set release thresholds from agreed workload
      and representative hardware. (Target: v2.5.0)
- [ ] Waves 5–6: evaluate scalability and scheduler trade-offs against the
      same model/workload and approved SLOs. (Target: v3.0.0)

### Phase 6: Documentation & acceptance

- [ ] Waves 3–4: synchronize API, security, metrics, and operator docs with
      tested behavior; obtain v2.5.0 API/security/operations acceptance.
      (Target: v2.5.0)
- [ ] Wave 7: publish any migration guidance and complete v3.0.0 human
      architecture/security/release approval. (Target: v3.0.0)

## KPIs and Measurement Protocol

Report all applicable metrics by model/backend and workload profile, at
concurrency steps including idle/single-client, expected production load, and
overload. Include prompt/output token distributions, warm/cold state,
authentication, client/network profile, hardware, software revision, and
configuration. Report client-side queueing and load-generator saturation
separately so the load tool is not mistaken for server latency.

| KPI | Measurement | Proposed acceptance rule |
|---|---|---|
| Time to first event (TTFT) | Request start to first complete SSE event; p50/p95/p99. | Wave 1 owners approve profile-specific SLO after baseline; v2.5 gate compares the same route/backend/workload. |
| Inter-token latency (ITL) and tail latency | Per-token interval plus end-to-end p50/p95/p99. | Meet approved latency SLO at expected concurrency; report overload separately. |
| Throughput and success | Completed requests/s and generated tokens/s, with success/error/cancel/timeout rates. | Meet approved capacity target with no silent loss; all terminal outcomes accounted for. |
| Resource bounds | Peak RSS and active streams/bytes per stream; queue high-water mark where applicable. | Stay within configured admission and buffering limits, with no monotonic leak during soak. |
| Cancellation and cleanup | Disconnect/timeout to generation stop and resource release. | Meet the Wave 1 cancellation bound; zero retained stream resources after test cleanup. |
| Fairness/isolation (v3) | Per-tenant latency, admitted capacity, and cross-tenant delivery checks. | Meet approved fairness envelope and zero cross-tenant delivery. |
| Reproducibility | Variance across repeated same-profile runs and harness utilization. | Report run-to-run variance and demonstrate the load client is not saturated before adopting thresholds. |

Numeric SLOs remain **open** until workload owner, model/backend, and reference
hardware are selected. Values in the current stub benchmark and soak sources
are not carried forward as production targets.

## Risks, Trade-offs, and Open Assumptions

1. **Buffered response vs. incremental delivery:** buffered mode has simpler
   lifecycle but holds the completed response in memory and delays delivery;
   incremental mode improves time-to-first-byte but requires explicit write
   ordering, backpressure, cancellation, and cleanup.
2. **Backend limitations:** callback cancellation/deadlines and token
   granularity differ by backend. The roadmap assumes no common capability
   until proven by adapter-level tests.
3. **Performance portability:** published serving gains depend on model,
   accelerator, scheduler, and request mix; they justify measurement, not a
   ThemisDB performance guarantee.
4. **Observability/privacy:** stream labels, logs, traces, and request
   identifiers can leak sensitive content or create unbounded cardinality.
   Record operational metadata only, with bounded labels and a reviewed
   retention policy.
5. **Compatibility:** incremental delivery changes observable behavior and
   might require response/header/client adjustments. Any incompatible API or
   wire change requires explicit v3.0.0 owner approval and migration planning.
6. **Estimate uncertainty:** person-week estimates exclude backend changes,
   infrastructure provisioning, security certification, and broad release
   qualification; estimates must be revisited after Wave 1.

### Explicit assumptions and knowledge gaps

- The API/product owner has not confirmed whether incremental network delivery
  is required or whether buffered SSE is acceptable.
- The reference inference backend, supported client/proxy matrix, production
  concurrency, request-size distribution, latency SLO, and deployment hardware
  are unspecified.
- No evidence was found that the current HTTP streaming routes use the
  `AsyncInferenceEngine` cancellation/deadline APIs.
- No successful authenticated route test, route-level performance result, or
  actual streaming-runtime benchmark was found in the reviewed paths.
- `ai_context/developer_llm_wiki/WIKI_STATUS.json` was generated 2026-11-02 and
  reports a large source delta; it has no curated llm_streaming page.
  Repository source and module-local canonical docs take precedence pending
  wiki refresh.
- The referenced 2026 measurement-bias work is an arXiv preprint. Reassess
  external research and standards at design approval; avoid treating it as a
  normative specification.

## Production Readiness Checklist

- [x] Current helper, HTTP routes, callback, response buffering, and test/benchmark boundaries are source-traceable.
- [ ] Product/API owner approves transport semantics and per-backend capability contract.
- [ ] Successful authenticated route behavior and all terminal/error outcomes are integration-tested.
- [ ] Lifecycle, cancellation, disconnect, backpressure, ordering, and cleanup guarantees are implemented and tested at the owning boundary.
- [ ] Workload-specific TTFT/ITL/tail-latency/throughput/resource targets and representative measurements are approved and reproducible.
- [ ] Security/privacy, metrics cardinality, configuration, runbooks, and deployment limits are reviewed.
- [ ] v3.0.0 compatibility, migration, scalability, and release gates receive explicit owner sign-off before being treated as committed work.

## Known Issues & Limitations

1. `StreamingHandler` is not used by the GET route; both reviewed HTTP paths
   buffer SSE output rather than delivering each event incrementally.
2. The helper's atomic index does not serialize concurrent sink delivery.
3. Module stress/soak/benchmark use in-process stubs, not the helper or runtime;
   their printed thresholds are not executable release gates.
4. Existing direct helper tests do not establish end-to-end transport,
   cancellation, backpressure, or concurrent delivery guarantees.
5. No production streaming SLO, deployment limit, stream-specific metrics,
   operational procedure, or v3.0.0 product requirement is established.

## Breaking Changes

No runtime contract change is made by this documentation. Incremental delivery,
new lifecycle/cancellation guarantees, or altered SSE/API semantics may change
observable behavior. Do not introduce such changes under v2.5.0 without
compatibility review; reserve incompatible protocol/API changes for an
explicitly approved v3.0.0 migration plan consistent with `VERSIONING.md`.
