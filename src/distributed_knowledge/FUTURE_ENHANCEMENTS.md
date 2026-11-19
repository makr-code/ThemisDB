---
Author: "ThemisDB Contributors"
Created: "2026-05-31"
Last Updated: "2026-11-19"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

> Exact model backend not verifiable from final document text alone.

# Distributed Knowledge Module - Future Enhancements

<!-- Status: current | evidence reconciliation: 2026-11-19 -->
<!-- Links: README.md · ROADMAP.md · PERFORMANCE_EXPECTATIONS.md -->

## Scope

- v2.5.0 is an in-process coordination baseline for capability announcements,
  LoRA aggregation, RAG result merging, feedback callbacks, and distillation.
- v3.0.0 is a release target, not a claim that graph synchronization, transport,
  durable replication, or recovery already exists.
- Source of truth: `include/distributed_knowledge/*.h`,
  `src/distributed_knowledge/*.cpp`, focused tests under
  `tests/distributed_knowledge/`, and mapped benchmark sources under
  `benchmarks/distributed_knowledge/`.

## Design Constraints

- Missing or rejecting capability trust policy and unknown capability domains
  must not dispatch inbound announcements.
- Privacy, participant, and policy limits must fail closed where implemented;
  no default or `AlwaysPermitTrustPolicy` setting is production authorization.
- Partial results, callback skips, in-memory state, and non-cancellable timeout
  behavior must be visible to callers and operators.
- Do not claim network synchronization, durable delivery, bounded retries, or
  restart recovery without source, tests, and operational evidence for each.

## Required Interfaces

| Interface | Requirement |
|---|---|
| `GossipAdapterPublisher` | mandatory inbound `IFederationTrustPolicy`; reject unsupported domains before callback dispatch |
| `LoRAFederationCoordinator` | preserve participant/privacy gates and expose ignored stale/future/duplicate submissions as a documented limitation until a result-bearing API is designed |
| `FederatedRAGMerger` | report queried/responded counts and distinguish partial results from all-timeout failure; caller owns fan-out and deadlines |
| `CrossShardFeedbackSync` | expose best-effort callback failures and bounded process-local dedup behavior; no delivery guarantee |
| `FederatedDistillationCoordinator` | keep policy/privacy checks before dispatch and document partial callback failure and in-memory round state |

## Implementation Notes

- Keep `federated_distillation_coordinator.cpp` in the
  `themis_distributed_knowledge` production library source set; tests must link
  the library rather than silently compiling a separate implementation copy.
- Add real three-shard integration coverage: one failed/timed-out shard must
  produce an explicitly partial response; all three unavailable must fail.
- Add cancellation-aware timeout semantics before treating
  `triggerAggregation(timeout_ms)` as a hard wall-clock limit.
- Add durable round and dedup recovery only with an explicit persistence
  contract, replay/idempotency behavior, and restart tests.

## Test Strategy

- Run all registered `module_distributed_knowledge_*_focused` tests against the
  production library in the `community-release` profile.
- Assert zero callback dispatch for unknown domains, absent trust policy, and
  rejecting trust policy; verify invalid configuration throws.
- Exercise RAG with three supplied responses (one failed and all failed), plus
  feedback callback failure, policy rejection, duplicate replay, and cache
  eviction. Label all in-process tests as component evidence, not topology
  evidence.
- Keep the explicitly stubbed high-cardinality suite out of release-runtime
  evidence; add integration/chaos tests before making live-topology claims.

## Performance Targets

- For each production-path benchmark, collect p50/p95/p99 in five independent
  release-profile runs and require p95 and p99 regression no greater than 10%
  against a pinned same-hardware baseline.
- Require zero missing mapped cases and retain commit, compiler, build profile,
  hardware, workload, and comparison artifacts.
- Treat `bench_dk_release_gates` as local in-memory/mock evidence only; it does
  not satisfy a live shard-fan-out performance gate.

## Security / Reliability

- The integrating service must authenticate peers and authorize aggregation
  inputs; the module's JSON and trust-policy callback do not authenticate a
  transport.
- Keep invalid policy/privacy inputs fail-closed and expose partial, skipped,
  or rejected outcomes rather than interpreting them as successful delivery.
- No automatic retry or recovery loop is currently implemented. Any future
  retry must have a finite attempt/deadline budget and observable terminal
  status; any recovery must prove idempotency and restart behavior.
- Do not approve v3.0.0 graph-synchronization or recovery claims until
  source-backed implementation, real failure-path tests, runbooks, and
  reproducible performance evidence exist.