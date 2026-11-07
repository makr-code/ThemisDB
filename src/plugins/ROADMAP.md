---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-11-07"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

> Exact model backend not verifiable from final document text alone.

# Plugins Module Roadmap

This roadmap assesses source and test/benchmark registration in `src/plugins/`,
`include/plugins/`, `tests/plugins/`, and `benchmarks/plugins/`. The version
labels **v2.5.0** and **v3.0.0** below are planning horizons requested by issue
**#7440**, not approved release commitments. The root
[`../../ROADMAP.md`](../../ROADMAP.md) remains authoritative for product
version, release readiness, and blockers. Estimates are preliminary
person-weeks, not schedule or staffing commitments.

## Current Status

**Qualitative maturity: broad implementation, incomplete release evidence.**
The module is not a blank implementation: source includes native plugin
lifecycle management, a typed factory registry, dependency and edition checks,
health and hot-plug monitoring, metrics, OCI distribution, a pinned-key signed
catalog API, RPC registration, and optional WASM loading. This inventory does
not prove that every declared header interface has runtime/build wiring, nor
that a build or test passed.

The largest gaps are consistency of artifact/publisher policy across load paths,
the native in-process trust boundary, operational ownership of durable audit
retention, and reproducible end-to-end performance evidence. Existing focused
test sources cover lifecycle, contracts, concurrency, monitoring, and security;
their registration in `tests/plugins/CMakeLists.txt` is not execution evidence.
Likewise, source-level benchmark thresholds are not measured results. No test,
build, or benchmark was run for this roadmap update.

| Focus | Source-grounded status and gap |
|---|---|
| Performance and scale | `bench_plugins_release_gates` measures error-code/descriptor micro-operations. The separate `bench_plugin_system` and `bench_plugin_hot_plug` sources include scans, queries, and monitor operations, but the release-gate target does not measure a successful end-to-end load of a real plugin artifact. No verified current baseline was found. See [`PERFORMANCE_EXPECTATIONS.md`](./PERFORMANCE_EXPECTATIONS.md). |
| Use cases and usability | Public declarations cover generic lifecycle, RPC, image/audio, ingestion, OCI, catalog, and WASM surfaces. Distinct registration, loading, and trust paths mean operators and plugin authors need a single documented policy matrix, compatibility rules, and a conformance workflow. Header presence alone does not establish a working integration. |
| Security and resilience | Native libraries run in the server process. Explicit-path and OCI loading do not apply manifest edition/license rules; OCI digest checks establish content equality, not publisher identity. The signed catalog is a separate API, and a registered WASM callback owns its own validation/runtime behavior. These boundaries are documented in [`TRUST.md`](./TRUST.md) and [`SECURITY.md`](./SECURITY.md). |
| Governance and maintainability | The module has separate manager, factory-registry, catalog, distribution, monitor, and runtime surfaces. No single approved publisher-identity policy or durable audit-retention owner was identified in the sources reviewed. `PluginMetrics` and health state are process-local. |
| Architecture and operations | The plugin manager is classified in the trusted T0 core while dynamically loaded plugins are a least-trusted T5 boundary in [`ai_context/ARCHITECTURE_CLASSIFICATION.md`](../../ai_context/ARCHITECTURE_CLASSIFICATION.md). Moving the execution boundary therefore needs explicit architecture and compatibility review, not only a loader change. |

## Research Basis and Gap Analysis

The research points to applicable engineering principles, not guarantees about
ThemisDB or measured performance:

| Finding | Application to this module |
|---|---|
| SLSA provenance describes where, when, and how an artifact was produced; provenance is distinct from an artifact digest or a signer identity. | Specify the provenance evidence required for each plugin distribution path and verify it before activation. Do not equate the current OCI digest, manifest hash comparison, binary signature, and pinned-key catalog with one interchangeable control. |
| Sigstore verification binds a signature to an artifact and verifies signer identity/claims under configured trust policy. | Make accepted identities, issuer/key trust, rotation, revocation, and failure behavior explicit. Select a verification mechanism only after the owner approves policy. |
| The WebAssembly Component Model uses typed interfaces; Wasmtime's documented component execution starts without access to system resources unless the host grants it. | For a future WASM component path, define narrow host imports/capabilities and resource limits. A module hash or optional runtime alone is not evidence that the callback or every configured runtime is sandboxed. |
| NIST SP 800-190 recommends treating containerized software as a supply-chain and runtime-security concern. Its scope is containers, so it is supporting guidance by analogy, not a plugin certification or direct plugin control. | Include artifact provenance, deployment configuration, least privilege, monitoring, and response/rollback in an end-to-end plugin threat model. |
| Google Benchmark documents repetitions, aggregate statistics, and benchmark-environment considerations. | Record raw benchmark output and environment; compare equivalent workloads against a retained baseline. A label in source is not a threshold assertion, a percentile, or a passing result. |

**Research-to-code conclusion:** the most immediate gap is not another extension
surface; it is a consistent, testable control plane around existing extension
surfaces. Near-term work should preserve the current public ABI while resolving
policy and measurement gaps. A major-version redesign can then enforce a
least-authority execution model and a stable plugin compatibility contract.

### External technical references

- [SLSA Provenance, v1](https://slsa.dev/spec/v1/provenance) — artifact build
  provenance model.
- [Sigstore Cosign verification](https://docs.sigstore.dev/cosign/verifying/verify/)
  — signature and identity verification concepts.
- [WebAssembly Component Model: Running components with Wasmtime](https://component-model.bytecodealliance.org/running-components/wasmtime.html)
  — component execution and host resource grants.
- [NIST SP 800-190: Application Container Security Guide](https://csrc.nist.gov/pubs/sp/800/190/final)
  — adjacent supply-chain/runtime security guidance; not plugin-specific.
- [Google Benchmark User Guide](https://google.github.io/benchmark/user_guide.html)
  — repetitions, reporting, and measurement practices.

## In Progress

- [?] Approve the required publisher-identity/provenance policy for every load
  path and name its security owner (Target: v2.5.0 Wave 1).
- [?] Decide the native-plugin isolation and durable audit-retention ownership
  boundary (Target: v2.5.0 Wave 1; architecture decision for v3.0.0).

## Planned Features

- [ ] Close trust-policy and execution-evidence gaps without an unapproved
  public-API/ABI break (Target: v2.5.0; proposed, pending owner approval).
- [ ] Define a capability-limited, resource-bounded plugin execution and
  versioned contract architecture (Target: v3.0.0; proposed, pending design and
  compatibility review).

## Release Wave Plan

All wave estimates are preliminary person-weeks and assume timely policy and
architecture decisions. Dependencies are gates: a later wave must not silently
choose an unresolved security policy. KPIs below are proposed acceptance
criteria, not measured baselines.

### v2.5.0 — compatible trust, operations, and evidence closure

Scope: improve policy consistency and verifiability while preserving the
existing public API/ABI. Any policy that changes who can load a plugin requires
owner approval and migration guidance before implementation.

| Wave / priority / effort | Goal and tasks | Dependencies | Risk / trade-off | Success metrics and validation |
|---|---|---|---|---|
| **Wave 1 — P0 — 1–2 pw** | Approve a load-path matrix for manifest/name, explicit path, OCI, signed catalog, built-in WASM, and registered WASM callback. For each, record artifact integrity, publisher identity, edition/capability policy, failure mode, and audit owner. Reconcile the matrix with `plugins_api_contract.h` without confusing its v1.x contract wording with product semver. | Security, product, and operations owners; current public contracts and root release policy. | Policy work may reveal incompatible existing deployment behavior; premature implementation risks inventing or weakening trust rules. | **100% of identified activation paths** have an approved control/owner matrix and explicit allow/deny semantics. Review contract and threat model; no release readiness claim from this wave alone. |
| **Wave 2 — P0 — 3–5 pw** | Implement the approved publisher/provenance checks at the activation boundary for each supported path. Keep transport digest, signer identity, manifest policy, and catalog signature semantics distinct. Fail closed on missing, malformed, mismatched, revoked, or unauthorized trust evidence where policy requires verification. | Wave 1 approval; chosen trust anchors/issuer policy; artifact and key-rotation procedures. | Unifying checks can reject previously accepted artifacts; key/identity provisioning can block rollouts. Avoid a hidden compatibility fallback. | Every supported path has success and negative tests; **100% of unauthorized or tampered test artifacts are rejected before activation**. Run focused manifest, path, OCI/catalog, and WASM tests in supported build configurations; retain result logs and test artifact hashes. |
| **Wave 3 — P1 — 2–4 pw** | Harden lifecycle and hot-plug failure behavior; exercise dependency failure, initialization rollback, reload, callback errors, and concurrent registration. Emit structured lifecycle/security outcomes to the approved observability sink without logging credentials or tokens. Assign persistence/retention to the owning system rather than claiming process-local metrics are durable audit. | Wave 1 audit owner and retention decision; Wave 2 failure semantics. | Additional event volume and sink failure handling can affect latency or availability; the module cannot guarantee retention if its downstream sink does not. | Injected failures leave **zero partial registry/lifecycle state**; all tested activation and denial outcomes produce the specified event without secrets. Run focused lifecycle, error, hot-plug, and registry-concurrency tests plus a sink-failure case. |
| **Wave 4 — P1 — 2–4 pw** | Establish executable performance baselines using a real test plugin artifact. Measure discovery, successful load/unload/reload, registry query/concurrency, and hot-plug detection with 1/10/100 manifests and 1/4/16 query threads where supported. Document an SDK/conformance example and distinguish built targets from executed targets. | Stable Wave 2 validation setup; representative runner; test plugin artifact and benchmark build. | Filesystem, loader, signing, OS, and hardware variation can dominate results; absolute limits without a controlled baseline are misleading. | Publish repeated-run median and p95/p99 (where the harness supplies valid samples), build/runtime/OS metadata, and raw output for **100% of agreed scenarios**. After baseline acceptance, investigate any **>10% regression** on an equivalent runner; treat this as a proposed relative budget, not an existing SLO. |

**v2.5.0 completion gate:** Waves 1–4 have approved owners and retained
validation evidence; no API/ABI break is introduced; root release readiness is
still determined only by [`../../ROADMAP.md`](../../ROADMAP.md).

### v3.0.0 — strategic execution and contract redesign

Scope: introduce a versioned plugin contract and an enforced least-authority
execution boundary. Product v3.0.0 is a strategic target only; the precise
native/WASM split and migration policy require human architecture approval.

| Wave / priority / effort | Goal and tasks | Dependencies | Risk / trade-off | Success metrics and validation |
|---|---|---|---|---|
| **Wave 1 — P0 — 3–5 pw** | Specify a versioned plugin ABI/manifest and compatibility policy; define capability grants, resource/time budgets, cancellation, health, audit events, and error semantics. Compare a capability-limited WASM component runtime with a separate native worker boundary against supported platforms and threat models. Decide whether in-process native loading is prohibited, restricted to explicitly trusted deployments, or removed; do not retain it silently as a fallback. | v2.5.0 trust matrix and performance evidence; architecture/security owners; supported-platform and edition requirements. | A stronger boundary may reduce portability or throughput and increase operational complexity; under-specified native policy leaves the principal risk unchanged. | Human-approved threat model and architecture decision records all supported trust levels, capabilities, resource limits, failure containment, and migration behavior; each public contract has compatibility and error semantics. |
| **Wave 2 — P0 — 8–16 pw** | Implement the selected boundary. For WASM components, pin supported runtimes, expose only reviewed host interfaces, and enforce memory/fuel/time limits. If native plugins remain supported, execute untrusted native code outside the database process with OS-enforced permissions and bounded resources; contain worker crashes and define restart/health behavior. | Wave 1 design; runtime/worker availability per platform; packaging, IPC, and deployment ownership. | Sandbox escapes, permissive host imports, unbounded resources, or an unisolated fallback can create false assurance. Process boundaries also add IPC latency and operational failure modes. | Denied host/resource requests are rejected in **100% of negative tests**; malformed and resource-exhausting plugins cannot corrupt the manager or exceed configured limits. Validate crash containment, cancellation, restart, and upgrade paths on every supported platform; do not claim isolation from API presence alone. |
| **Wave 3 — P1 — 4–8 pw** | Deliver the major-version SDK/conformance suite, sample plugins, explicit manifest migration tooling/documentation, and a compatibility matrix for supported plugin/runtime versions. Make the new contract the only default activation path; document any separately approved trusted-native deployment mode and its limits. | Wave 1 contract and Wave 2 runtime; public/private plugin maintainers and edition owners. | Migration burden may reduce ecosystem adoption; cross-platform ABI/runtime drift can fragment plugin availability. | All maintained sample plugins pass the conformance suite on the supported matrix; compatibility outcomes are deterministic and versioned; migration tests cover valid and rejected old manifests without silently broadening privileges. |
| **Wave 4 — P1 — 3–6 pw** | Run sustained-load, fault-injection, and security validation for the new boundary; verify signed artifact provenance and key revocation/rotation; connect audit events to the approved durable retention system; measure end-to-end latency and resource overhead against v2.5.0 baselines. | Waves 2–3; representative hardware/OS runners; security review and audit-retention service. | More complex verification can increase release time; a benchmark or scanner pass does not replace penetration testing or operator readiness. | Every supported runtime/platform has retained test and security evidence; audit events survive the approved retention boundary; publish p50/p95/p99 latency, throughput, and resource overhead. Any deviation from the approved v2.5.0 performance budget is explicitly accepted or remediated before release. |

**v3.0.0 completion gate:** approved migration and compatibility plan, enforced
execution boundary for each supported trust level, platform-specific isolation
evidence, security sign-off, and verified operational runbooks. No such
completion is asserted by this roadmap update.

### Effort and sequencing summary

- v2.5.0: approximately **8–15 person-weeks** across Waves 1–4.
- v3.0.0: approximately **18–35 person-weeks** across Waves 1–4, excluding
  platform-specific infrastructure or external security testing.
- v3.0.0 depends on the v2.5.0 policy matrix and executable baseline. Waves
  across both versions require owner approval; estimates can change after
  platform/runtime decisions.

## Risk and Trade-offs

| Risk | Trade-off / mitigation |
|---|---|
| Inconsistent trust checks across direct, manifest, OCI, catalog, and WASM paths | Model each path explicitly and reject unapproved paths before execution; do not infer publisher identity from a digest. |
| Native in-process code can crash or compromise the server | Make the trust boundary an explicit v3.0.0 decision. A WASM hash is not a sandbox, and a process wrapper is not isolation unless OS permissions and resource limits are enforced. |
| Tightened provenance rules break existing deployments | Obtain policy approval, inventory deployed artifacts, provide an explicit migration plan, and avoid implicit compatibility fallbacks. |
| Noisy or misleading performance results | Compare equivalent builds/runners and workloads; preserve raw results and report distributions. Separate source labels, target registration, and executed evidence. |
| Telemetry is mistaken for durable audit | Name the external persistence owner, retention period, access controls, and sink failure semantics before using audit data for compliance claims. |
| Public/private edition boundaries leak | Preserve community guardrails; exercise edition and license-denial tests in the relevant build/package matrix before changing load policy. |

## Implementation Phases

These phases order the work described in the versioned wave plan; they do not
mark any implementation as complete.

### Phase 1: Design / API Contract

- Approve the complete v2.5.0 load-path policy and assign security/audit owners
  (v2.5.0 Wave 1).
- Approve v3.0.0 ABI, capability, runtime-isolation, and migration decisions
  before implementation (v3.0.0 Wave 1).

### Phase 2: Core Implementation

- Implement the owner-approved, ABI-compatible artifact verification and
  activation rules (v2.5.0 Wave 2).
- Implement only the selected, enforceable WASM or worker-process boundary and
  configured resource limits (v3.0.0 Wave 2).

### Phase 3: Error Handling and Edge Cases

- Verify fail-closed behavior for missing/invalid trust evidence, edition
  denial, dependency failure, initialization rollback, and sink failure.
- Verify cancellation, timeout, resource exhaustion, worker crash, and restart
  behavior for every v3.0.0 execution backend.

### Phase 4: Tests

- Build and run the focused targets registered by
  [`../../tests/plugins/CMakeLists.txt`](../../tests/plugins/CMakeLists.txt)
  across the approved platform/edition matrix; retain actual results.
- Add and execute end-to-end conformance tests for each approved load path and
  every supported plugin contract version.

### Phase 5: Performance and Hardening

- Measure actual successful plugin lifecycle and hot-plug scenarios; retain
  repeated-run data and compare against an accepted baseline.
- Run fault injection and security testing against the chosen trust/runtime
  boundary; do not treat benchmark labels or source presence as evidence.

### Phase 6: Documentation and Acceptance

- Synchronize API/SDK, security, operations, rollback, edition, and migration
  guidance with the accepted implementation.
- Obtain owner/security/operations sign-off and verify root release gates before
  promoting any module evidence to release readiness.

## Production Readiness Checklist

- [ ] Approved provenance and publisher-authentication policy for every
  activation path.
- [ ] Documented native-code isolation decision and enforced boundary for each
  supported trust level.
- [ ] Named owner and tested system boundary for durable security audit
  retention.
- [ ] Focused tests built and executed in the approved platform/edition matrix;
  results retained.
- [ ] Reproducible successful lifecycle and hot-plug baselines with raw output
  and environment metadata.
- [ ] v3.0.0 API/ABI migration, compatibility, and rollback plan approved.
- [x] Source-visible load-path and trust distinctions are documented in the
  module trust and architecture material.

## Known Issues & Limitations

- Native shared-library plugins execute in the host process; this module does
  not provide native process isolation.
- Manifest `.sig` hash comparison, binary verification, pinned-key Ed25519
  catalog verification, OCI layer digests, and WASM expected hashes are distinct
  controls; none should be treated as a substitute for an approved publisher
  identity policy. See [`TRUST.md`](./TRUST.md).
- OCI digest verification does not by itself establish publisher identity, and
  direct-path/OCI loading does not apply manifest edition/license policy.
- A registered WASM loader callback owns its validation/runtime behavior; the
  module does not establish a general sandbox guarantee for it.
- In-process health and metrics APIs do not establish durable audit retention.
- `bench_plugins_release_gates` currently measures contract/error-path
  micro-operations. The separate system and hot-plug benchmark sources and
  registered targets need inspection and execution before they can support
  end-to-end performance claims.
- `plugins_api_contract.h` contains a v1.x contract-freeze statement and
  v2.0-bump wording. Its relationship to the product release version and
  current implementation requires owner review; this roadmap does not infer a
  contract-breaking change from product v3.0.0.
- No owners, approved release dates, target runner matrix, or executed test and
  benchmark results were verified for this update.
- The cited external sources are specifications and technical guidance; no
  peer-reviewed comparative study of ThemisDB plugin runtimes or current
  plugin-specific performance results was identified. The proposed performance
  budgets therefore require local baseline measurement and owner approval.

## Breaking Changes

- **v2.5.0:** no public API/ABI break is planned in this proposal. Changes to
  accepted artifacts or authorization behavior still require explicit policy
  approval and deployment migration guidance.
- **v3.0.0:** a versioned plugin ABI/manifest or execution-boundary change may
  require a major plugin-contract migration. The exact breaking changes,
  compatibility window, and removal plan remain open design decisions; no
  compatibility fallback is approved by this roadmap.
