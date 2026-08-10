# Experiment Protocol: Ethics AI YAML Policy Enforcement

**Manuscript**: `research/manuscripts/security_governance_ethics/ETHICS_AI_YAML_POLICY_ENFORCEMENT_PAPER_DRAFT.md`  
**Status**: PROTOCOL_DRAFT  
**Last Updated**: 2026-08-10

---

## Prerequisite

`tests/ethics_ai/` focused test infrastructure must be established (`test_ethics_ai_constraint_enforcement_focused.cpp`) before any quantitative measurement in this protocol is meaningful.

---

## Objective

Measure constraint enforcement overhead, canary promotion accuracy, and demotion latency for YAML-defined ethics policies enforced at the query-plan boundary.

---

## Experiment Suite

### Suite P1 — Constraint Enforcement Overhead

- Baseline: 10,000 query-plan evaluations without policy check
- With-constraint: same workload, `ready`-status policy covering 100% of query types
- Action variants: `block`, `warn`, `log`
- Metric: p95/p99 added latency per query per action variant

### Suite P2 — Canary Promotion Cycle

- Generate 1,000 canary argument generations with ground-truth labels
- Measure: fidelity score distribution under normal inputs
- Measure: false-positive promotion rate (unsafe policy promoted as ready)

### Suite P3 — Demotion Under Adversarial Input

- Inject adversarial inputs designed to lower fidelity score below threshold
- Measure: time from fidelity drop onset to `status: deprecated`

### Suite P4 — Diagnostic Completeness

- Run 500 queries against a `block`-action policy
- Verify: exactly 500 diagnostic records emitted with correct `policy_id`, `rule_id`, `version`

---

## Environment

- Build: `linux-release`
- Ethics AI module: `src/ethics_ai/` (policy enforcement hooks enabled)
- Observability: diagnostic emission to `src/observability/` pipeline

---

## Artifact Checklist

- [ ] Focused test file `tests/ethics_ai/test_ethics_ai_constraint_enforcement_focused.cpp` created (precondition)
- [ ] P1 overhead table committed
- [ ] P2 promotion accuracy table committed
- [ ] P3 demotion latency p50/p95 committed
- [ ] P4 diagnostic completeness ratio (expected: 1.0) committed
- [ ] Results at `research/experiments/security_governance_ethics/results/P_<timestamp>.json`
