# YAML-Defined AI Ethics Policy as a First-Class Database Constraint Layer in ThemisDB

**Status**: ACTIVE_DRAFT  
**Version**: 0.1  
**Last Updated**: 2026-08-10  
**Target Venue**: arXiv (cs.AI / cs.DB / cs.CY) — Q4 2026  
**Portfolio Cluster**: `research/manuscripts/security_governance_ethics/`  
**Predecessor / Companion**: `research/ETHICS_AI_YAML_DISCOURSE_ENGINE_PAPER_DRAFT.md` (ACTIVE_DRAFT — this manuscript extracts the constraint-layer framing as a standalone contribution)

---

## Metadata

- **Scientific Delta**: Formalize YAML-defined ethics policies as a constraint language with three lifecycle states (canary / ready / deprecated), enforced at query-plan level within the database engine — not as an external filter or application-layer guard.
- **Canonical Evidence Sources**: `research/ETHICS_AI_YAML_DISCOURSE_ENGINE_PAPER_DRAFT.md`, `src/ethics_ai/README.md`.
- **Required Experiments**: constraint enforcement overhead at query-plan boundary; canary promotion/demotion cycle latency; fidelity-score-gated promotion accuracy under adversarial inputs.
- **Open Risks / Claim Boundaries**: ethics_ai test suite currently has placeholder-only CMake/README and no `test_*_focused.cpp` focused tests; quantitative constraint-evaluation claims require focused test coverage to be established first.
- **Overlap / Successor / Predecessor**: this manuscript extracts the constraint-enforcement contribution from the larger discourse-engine paper; both can coexist if scopes remain distinct.

---

## Abstract

AI ethics policy is typically enforced at application layer, as a post-hoc filter on generated outputs. This design decouples policy from database execution semantics: policies cannot observe or intercept query-plan-level operations, cannot participate in ACID transactions, and cannot be validated against stored data schemas. ThemisDB proposes a different model: YAML-defined ethics policies are first-class constraint objects, enforced at query-plan boundaries with the same lifecycle management as schema constraints. The policy lifecycle uses three states — `canary` (10% traffic sample), `ready` (full enforcement), `deprecated` — with fidelity-score-gated promotion and automatic demotion. This paper formalizes the constraint model, describes the canary/degrade/fidelity cycle, and proposes an evaluation methodology for enforcement overhead, promotion accuracy, and constraint-violation observability.

---

## I. Introduction

Ethics policies for AI systems are hard to maintain because they live outside the data processing stack. Application-layer filters do not know the query context, cannot access ground-truth stored data for calibration, and fail silently when bypassed by direct API access. Database-native constraint enforcement offers a fundamentally different architecture: the policy participates in query execution, has access to the full data context, and is observable through standard database diagnostics.

ThemisDB's ethics AI module instantiates this architecture using YAML-defined policy documents. The policy lifecycle is managed like a schema migration: canary deployment to a traffic sample, fidelity-score-gated promotion to full enforcement, and deprecation on performance regression. This is not application-layer policy filtering; it is constraint-layer enforcement with database-grade lifecycle semantics.

### Contributions

1. A formal constraint model for YAML-defined AI ethics policies with three lifecycle states.
2. A fidelity-score-gated promotion protocol that ties policy promotion to measured behavioral conformance.
3. An evaluation plan for constraint enforcement overhead, promotion accuracy, and operator-visible observability.

---

## II. Related Work

- constitutional AI and RLAIF: policy-as-reward-signal for training
- output filtering and guardrail systems: application-layer post-hoc filters
- database constraint languages (SQL CHECK, assertion-based constraints)
- novelty delta: YAML-defined ethics constraints enforced at query-plan level with transactional lifecycle, not application-layer filtering or training-time reinforcement

---

## III. System Model / Repository Scope

- ethics AI module: `src/ethics_ai/`
- policy lifecycle: canary → ready → deprecated (YAML `status` field)
- promotion gate: `mean_fidelity_score ≥ predecessor_version` over 1,000 canary argument generations
- demotion: `status: "deprecated"` on fidelity regression below threshold
- enforcement point: query-plan boundary (intercepted before execution, not post-filter)
- evidence sources: `research/ETHICS_AI_YAML_DISCOURSE_ENGINE_PAPER_DRAFT.md` §canary/degrade model

---

## IV. Method / Design

### A. YAML Policy Constraint Model

An ethics policy document in ThemisDB is a YAML schema object with:

```yaml
policy_id: <uuid>
version: <semver>
status: canary | ready | deprecated
enforcement_scope: [query_types]
fidelity_threshold: <float 0.0–1.0>
rules:
  - id: <rule_id>
    predicate: <constraint expression>
    action: block | warn | log
    justification: <human-readable rationale>
```

The `status` field is the constraint lifecycle state. The database engine treats `ready` policies as hard constraints at query-plan level; `canary` policies intercept 10% of matching requests and log fidelity scores; `deprecated` policies are ignored.

### B. Fidelity-Score-Gated Promotion

Promotion from `canary` to `ready` requires:
1. At least 1,000 canary argument generations evaluated against the policy
2. `mean_fidelity_score ≥ mean_fidelity_score(predecessor_version)` over the evaluation window
3. No safety-critical rule violations in the canary window

Automatic demotion from `ready` to `deprecated` triggers when rolling fidelity drops below threshold for a configured window.

### C. Query-Plan Enforcement

Ethics constraints are evaluated at the query plan boundary:
- the constraint predicate receives the query plan's intent (query type, target entities, context scope)
- `block` action halts plan execution and returns a structured error with `policy_id` and `rule_id`
- `warn` action logs a diagnostic event but allows execution
- `log` action records the constraint match without any execution effect

All constraint evaluation events are emitted as structured diagnostics for operator review.

### D. Constraint Observability

Every constraint check produces a diagnostic record:
- `policy_id`, `rule_id`, `version`, `status`
- matched query type and context scope
- fidelity score for canary evaluations
- action taken (block / warn / log)

These records feed into the standard observability pipeline (`src/observability/`) for dashboarding and alerting.

---

## V. Repository-Grounded Evidence

| Evidence ID | File | Scope | Claim anchor | Status |
|---|---|---|---|---|
| E1 | `research/ETHICS_AI_YAML_DISCOURSE_ENGINE_PAPER_DRAFT.md` | §canary/degrade/fidelity | policy lifecycle model, canary gate, demotion trigger | ready (companion paper) |
| E2 | `src/ethics_ai/README.md` | module scope | ethics AI module structure and integration points | ready |
| E3 | `tests/ethics_ai/CMakeLists.txt` | test registration | current placeholder state; no focused tests yet | gap (known) |
| E4 | `src/observability/README.md` | diagnostics pipeline | constraint event emission via standard observability path | ready |

**Known Gap (E3)**: `tests/ethics_ai/` currently has placeholder CMake/README only. Focused tests (`test_*_focused.cpp`) are required before quantitative enforcement claims can be validated.

---

## VI. Experimental Methodology

### A. Setup
- baseline: query workload without ethics constraint evaluation
- with-constraint: same workload with `ready`-status policy covering 100% of query types
- canary: same workload with `canary`-status policy covering 10% of requests

### B. Workloads
- W1: policy enforcement overhead at query-plan boundary (constraint check latency)
- W2: canary promotion cycle — 1,000 argument generations; fidelity score distribution
- W3: demotion trigger — inject adversarial inputs that lower fidelity; measure demotion latency
- W4: `block` action correctness — verify zero plan executions after block event

### C. Metrics
- constraint-check overhead: p95/p99 added latency per query
- promotion accuracy: false-positive promotion rate (unsafe policy promoted as ready)
- demotion latency: time from fidelity drop to `status: deprecated`
- diagnostic completeness: constraint events emitted vs. expected per workload

---

## VII. Results

### A. Primary Results
- policy lifecycle model (canary/ready/deprecated) and fidelity-gated promotion are architecturally specified (`E1`)
- constraint observability path (`E4`) is operational
- quantitative enforcement overhead and promotion accuracy require focused test infrastructure first (`E3`)

### B. Ablations / Sensitivity
- enforcement overhead: `block` vs. `warn` vs. `log` action paths
- canary fraction sensitivity: 10% vs. 1% vs. 50% traffic sample
- fidelity threshold sensitivity: promotion accuracy vs. safety margin

### C. Negative Results
- `tests/ethics_ai/` currently has no focused tests; this is a hard gap before quantitative evaluation
- YAML policy constraint language expressiveness limits: complex multi-entity predicates require AQL integration not yet implemented

---

## VIII. Discussion

### Supported claims
- database-native ethics constraint lifecycle is architecturally distinct from application-layer filtering (`E1`, `E2`)
- fidelity-gated promotion and automatic demotion are formally specified (`E1`)
- constraint events participate in the standard observability pipeline (`E4`)

### Deferred claims
- quantitative enforcement overhead measurements (blocked on E3 focused test completion)
- promotion accuracy under adversarial inputs (blocked on E3)
- production-scale constraint evaluation throughput

---

## IX. Reproducibility & Artifact

- policy lifecycle spec: `research/ETHICS_AI_YAML_DISCOURSE_ENGINE_PAPER_DRAFT.md`
- module scope: `src/ethics_ai/README.md`
- focused tests to create: `tests/ethics_ai/test_ethics_ai_constraint_enforcement_focused.cpp`
- experiment protocol: `research/experiments/security_governance_ethics/ethics_ai_policy_protocol.md` (to be created)

---

## X. Limitations, Risk, Ethics

- YAML-defined policies are not Turing-complete; complex fairness or multi-step reasoning constraints require integration with the full discourse engine, not just the constraint layer
- fidelity score is a behavioral proxy, not a formal safety proof; adversarial inputs designed to maximize fidelity while violating intent can fool the promotion gate
- ethics constraint enforcement at the database layer raises responsibility questions: the database operator, not the application developer, now owns the policy lifecycle

---

## XI. Conclusion

ThemisDB's ethics AI module formalizes an architectural distinction that is rarely made explicit in the literature: ethics policy as a first-class database constraint, not an application-layer filter. The canary/fidelity/deprecation lifecycle provides an operational path for policy evolution without manual deployment gates. The most immediate next step is creating focused tests for the constraint enforcement path — a requirement before any quantitative evaluation claim can be made.
