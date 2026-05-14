# Chaos Engineering in ThemisDB: Source-Backed Fault Injection Review

**Status**: Technical Review Candidate
**Version**: 0.2
**Last Updated**: 2026-05-14
**Scope**: ThemisDB OSS repository (`include/chaos`, `src/chaos`, related module roadmaps)

---

## Abstract / Zusammenfassung

This article documents and reviews the current ThemisDB chaos-engineering implementation with a strict source-first method. The verified core is an **in-process fault-injection framework** composed of `FaultInjector` and `ChaosScheduler` (`include/chaos/chaos_framework.h`, `src/chaos/chaos_framework.cpp`). The framework provides seven typed fault categories, callback hooks, expiry handling, and a scheduler with configurable wake behavior (`FIXED_TICK` and `CONDVAR`).

The contribution of this review is a reproducible evidence map where each central claim is linked to concrete source artifacts, roadmap state, or benchmark expectation documents. This gives engineering and research teams a verifiable baseline for resilience testing, and it clearly separates implemented behavior from future validation scope.

---

## 1. Introduction / Einleitung

### 1.1 Problem Context

Chaos engineering in distributed systems is valuable only when failure scenarios are reproducible and claims are traceable to implementation. In practice, many documents mix:

- protocol theory (Raft/Paxos/Byzantine tolerance),
- product intent (roadmaps), and
- implemented behavior.

This review addresses that gap for ThemisDB by separating **implemented mechanics** from **planned or theoretical extensions**.

### 1.2 Research Questions

This revised article answers four repository-grounded questions:

1. Which chaos primitives are implemented and callable today in ThemisDB OSS?
2. Which scheduler and fault-lifecycle semantics are guaranteed by current code?
3. Which performance claims are documented as benchmark targets versus measured outcomes?
4. Which resilience claims remain roadmap-level and require additional experiment evidence?

### 1.3 Terminology (Normalized)

- **Chaos Framework**: The in-process fault simulation API in `include/chaos/chaos_framework.h` and `src/chaos/chaos_framework.cpp`.
- **Fault Injection**: Registration and lifecycle management of simulated faults via `FaultInjector`.
- **Scheduler**: Time-driven fault triggering via `ChaosScheduler`.
- **Consensus / Federated Learning integration**: Existing module-level capabilities and roadmap references; not automatically equivalent to end-to-end validated chaos experiments.

---

## 2. Methodology / Ansatz

### 2.1 Verification Method

This review used a repository-only verification workflow:

1. **API/behavior verification** from:
   - `include/chaos/chaos_framework.h`
   - `src/chaos/chaos_framework.cpp`
2. **Module state and scope verification** from:
   - `src/chaos/ROADMAP.md`
   - `src/chaos/ARCHITECTURE.md`
   - `src/chaos/PERFORMANCE_EXPECTATIONS.md`
3. **Cross-module context verification** from:
   - `src/sharding/ROADMAP.md`
   - `src/training/ROADMAP.md`
   - `src/distributed_knowledge/ROADMAP.md`
   - `docs/en/security/FEDERATED_DISTILLATION_THREAT_MODEL.md`

### 2.2 Inclusion Rules for Claims

A claim is included only if it is backed by at least one of:

- concrete type/function behavior in source code,
- explicit roadmap state in module documents,
- explicit benchmark target documentation.

Unbacked numeric outcomes and unverified end-to-end assertions are removed.

### 2.3 Evidence Map (Claim -> Artifact)

| Claim | Evidence |
|---|---|
| Seven fault types exist | `FaultType` enum in `include/chaos/chaos_framework.h` |
| Fault validation is enforced | `injectFault` checks `target_node_id` and `probability` bounds in `src/chaos/chaos_framework.cpp` |
| Permanent vs expiring faults are supported | `duration` semantics in `FaultSpec`, `expires_at` handling and `pruneExpired` in source |
| Scheduler wake strategy is configurable | `WakeStrategy` + `ChaosSchedulerConfig` in header, run-loop branching in source |
| `ChaosScheduler` requires non-null injector | constructor throws `std::invalid_argument` in source |
| Framework scope is in-process simulation | architecture/readme statements + source comments |
| Performance values are targets, not measured results | `src/chaos/PERFORMANCE_EXPECTATIONS.md` |

---

## 3. Verified System Description / Verifizierte Systembeschreibung

### 3.1 Implemented Fault Model

The chaos API defines the following fault categories:

- `NODE_FAILURE`
- `NETWORK_PARTITION`
- `LEADER_CRASH`
- `DELAYED_RESPONSE`
- `DISK_FAILURE`
- `RANDOM_FAILURE`
- `DISASTER_RECOVERY_DRILL`

`FaultInjector` stores active faults keyed by node and type, supports recovery by node or `(node, type)`, exposes query/snapshot APIs, and supports event callbacks.

### 3.2 Fault Lifecycle Semantics

Verified semantics from code:

- empty node IDs are rejected,
- probabilities outside `[0.0, 1.0]` are rejected,
- `duration > 0` creates expiring faults,
- `duration == 0` creates permanent faults until explicit recovery,
- expired faults are pruned lazily when reading active-fault state.

### 3.3 Scheduler Semantics

`ChaosScheduler` provides:

- background worker lifecycle (`start`, `stop`, `isRunning`),
- absolute and relative scheduling (`schedule`, `scheduleIn`),
- pending queue management (`pendingCount`, `clearPending`),
- wake policy selection:
  - `FIXED_TICK`: periodic sleep,
  - `CONDVAR`: condition-variable wait with notification on schedule/stop.

This supports deterministic scheduling behavior at API level and lower stop-latency behavior for notification-driven waits.

### 3.4 Scope Boundary

Current OSS scope is **simulation-oriented and in-process**. The framework does not directly perform OS-level sabotage (for example packet filtering, process killing, filesystem corruption).

---

## 4. Evaluation / Experimente

### 4.1 Evaluation Type

This revision reports an **artifact-backed implementation evaluation** (code + documented benchmark expectations), not fresh runtime measurement campaigns.

### 4.2 Benchmark Coverage (Documented)

`src/chaos/PERFORMANCE_EXPECTATIONS.md` documents coverage for:

- fault injection throughput,
- active-fault query behavior,
- recovery throughput,
- expired-fault pruning,
- callback dispatch scaling,
- concurrent stress,
- scheduler scheduling behavior,
- active fault counting.

### 4.3 Documented Performance Targets

The current module document defines target gates such as:

- `CHAG-1`: `>= 70000 ops/s` (`InjectFault_Throughput`)
- `CHAG-2`: `<= 20 ms` RecoverFault P95
- `CHAG-3`: `<= 35 ms` Concurrent Stress P99
- `CHAG-4`: regression `<= 8 %` versus baseline

and additional module-wide release gates (`NG-1..NG-3`).

### 4.4 Cross-Module Resilience Context

Repository artifacts indicate active work in:

- sharding consensus hardening and chaos-focused tests (`src/sharding/ROADMAP.md`),
- federated LoRA/distillation controls (`src/training/ROADMAP.md`, `src/distributed_knowledge/ROADMAP.md`),
- threat-model controls for distillation and DP (`docs/en/security/FEDERATED_DISTILLATION_THREAT_MODEL.md`).

These artifacts support architectural relevance, but they are not by themselves a substitute for a dedicated end-to-end experiment report linking specific chaos schedules to measured consensus/FL outcomes.

---

## 5. Limitations / Known Issues

1. **No cluster-wide chaos control plane in current chaos module**: synchronized multi-node orchestration remains a planned enhancement.
2. **No direct OS/network/disk sabotage in module scope**: current implementation is intentionally simulation-based.
3. **Documented targets vs observed measurements**: the module provides benchmark target definitions, but this article does not add new measured result tables.
4. **Cross-module causality evidence gap**: strong claims such as exact consensus recovery times under specific injected schedules require dedicated reproducible experiment runs and published datasets/logs.
5. **Roadmap != proof**: roadmap completion items indicate implementation intent/state, not full scientific validation.

---

## 6. Conclusion / Fazit

ThemisDB currently provides a concrete and usable in-process chaos-engineering core (`FaultInjector` + `ChaosScheduler`) with clear API contracts, fault lifecycle semantics, and documented benchmark targets. The framework is suitable for deterministic test-harness fault simulation and resilience testing at module/integration level.

For publication-grade systems claims beyond this scope (for example, precise Raft/Paxos failover timing distributions or Byzantine tolerance curves under controlled chaos sweeps), the next step is a reproducible experiment package with run configuration, logs, and statistical analysis tied to exact source revisions.

---

## References / Referenzen

### A. External Literature

1. Ongaro, D., Ousterhout, J. (2014). *In Search of an Understandable Consensus Algorithm*. USENIX ATC.
   URL: https://www.usenix.org/conference/atc14/technical-sessions/presentation/ongaro
2. Lamport, L., Shostak, R., Pease, M. (1982). *The Byzantine Generals Problem*. ACM TOPLAS 4(3).
   DOI: https://doi.org/10.1145/357172.357176
3. Castro, M., Liskov, B. (1999). *Practical Byzantine Fault Tolerance*. OSDI.
   URL: https://www.usenix.org/conference/osdi-99/practical-byzantine-fault-tolerance
4. Blanchard, P., El Mhamdi, E. M., Guerraoui, R., Stainer, J. (2017). *Machine Learning with Adversaries: Byzantine Tolerant Gradient Descent*. NeurIPS.
   URL: https://proceedings.neurips.cc/paper/2017/hash/f4b9ec30ad9f68f89b29639786cb62ef-Abstract.html
5. Mironov, I. (2017). *Rényi Differential Privacy*. IEEE CSF.
   DOI: https://doi.org/10.1109/CSF.2017.11
6. Basiri, A., Behnam, N., de Rooij, R., Hochstein, L., Kosewski, L., Reynolds, J., Rosenthal, C. (2016). *Chaos Engineering*. IEEE Software 33(3).
   DOI: https://doi.org/10.1109/MS.2016.60
7. Kingsbury, K. (2014). *Jepsen: Call Me Maybe*.
   URL: https://jepsen.io
8. Huang, J., Xu, Y., Mukherjee, A., et al. (2021). *FoundationDB: A Distributed Unbundled Transactional Key Value Store*. SIGMOD.
   DOI: https://doi.org/10.1145/3448016.3457559

### B. ThemisDB Source Artifacts (Primary Evidence)

- `include/chaos/chaos_framework.h`
- `src/chaos/chaos_framework.cpp`
- `src/chaos/ARCHITECTURE.md`
- `src/chaos/ROADMAP.md`
- `src/chaos/PERFORMANCE_EXPECTATIONS.md`
- `src/sharding/ROADMAP.md`
- `src/training/ROADMAP.md`
- `src/distributed_knowledge/ROADMAP.md`
- `docs/en/security/FEDERATED_DISTILLATION_THREAT_MODEL.md`
