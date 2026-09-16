# Runbook: Evaluation Framework

<!-- Wave D operability deliverable — evaluation module -->
<!-- Source: src/evaluation/ROADMAP.md § Wave D Contribution -->

**Module:** `src/evaluation/`  
**Version:** 1.0.0 (Wave D, 2026-Q1)  
**Owner:** ThemisDB Evaluation Team  
**Labels:** `wave_d;operability;runbook`

---

## Overview

This runbook covers operator response procedures for the five most critical
evaluation framework incident classes. Each scenario includes detection signals
(log patterns and metrics), immediate mitigations, and escalation paths.

---

## Scenario 1 — Metric Computation Failure

**Log pattern:** `[EVAL:MetricFailed]`

### Detection

- Log line: `[EVAL:MetricFailed] metric=<M> sample_id=<ID> reason=<R>`
- Metric: `eval_metric_failure_count_total` nonzero
- Alert: `EvalMetricFailureRate`

### Immediate actions

1. **Identify the failing metric.** The log's `metric=` tag names the affected
   metric (e.g., `precision`, `recall`, `ndcg`).
2. **Check input validity.** `MetricErrorKind` codes in the log indicate input
   validation failure (null sample, divide-by-zero guard, out-of-range value).
3. **Re-run the evaluation in isolation.** Use the focused test target
   `module_evaluation_test_evaluation_retrieval_metrics_focused_FocusedTests`.
4. **Enable strict input validation.** Set `eval.strict_input_validation=true`
   to surface silent numeric edge cases.

### Escalation

Metric failures affecting a benchmark gate result require immediate escalation
to prevent false gate-pass signals.

---

## Scenario 2 — Benchmark Harness Crash

**Log pattern:** `[EVAL:HarnessCrash]`

### Detection

- Log line: `[EVAL:HarnessCrash] harness=<H> exit_code=<C> signal=<S>`
- Metric: `eval_harness_crash_count_total`
- Alert: `EvalHarnessCrash`

### Immediate actions

1. **Collect the core dump** (if enabled) and stack trace from the harness
   process.
2. **Check for memory violations.** Run the harness under AddressSanitizer with
   `THEMIS_ASAN=1` cmake preset to detect buffer overruns.
3. **Reproduce in isolation.** Run the crashing harness target directly with
   `ctest -R <harness_name> --output-on-failure`.
4. **Disable the crashing harness** in CI until the root cause is fixed to
   prevent blocking the release gate.

### Escalation

Any harness crash in the `release_critical` tier is a P1 blocker.

---

## Scenario 3 — Result Aggregation Overflow

**Log pattern:** `[EVAL:AggregationOverflow]`

### Detection

- Log line: `[EVAL:AggregationOverflow] aggregator=<A> sample_count=<N> overflow_at=<V>`
- Metric: `eval_aggregation_overflow_count_total`
- Alert: `EvalAggregationOverflow`

### Immediate actions

1. **Check result-set size.** Confirm `eval.max_aggregation_samples` is configured.
2. **Switch to incremental aggregation.** Set `eval.aggregation_mode=incremental`
   to avoid accumulating all results in memory.
3. **Reduce benchmark sample count** temporarily while the overflow guard is
   implemented.

### Escalation

Aggregation overflow that produces incorrect numeric gate results is a P2 incident.

---

## Scenario 4 — Judge Timeout

**Log pattern:** `[EVAL:JudgeTimeout]`

### Detection

- Log line: `[EVAL:JudgeTimeout] judge=<J> query_id=<ID> timeout_ms=<T>`
- Metric: `eval_judge_timeout_count_total`
- Alert: `EvalJudgeTimeoutRate`

### Immediate actions

1. **Check judge endpoint availability.** Confirm the LLM judge endpoint
   responds within the configured `eval.judge_timeout_ms`.
2. **Enable the fallback judge.** Set `eval.fallback_judge=heuristic` to use the
   in-process judge when the external judge is slow.
3. **Increase judge timeout** (`eval.judge_timeout_ms=<N>`) as a temporary
   measure during high-load periods.
4. **Review judge request batch size** — large batches may exceed the judge
   service's per-request limit.

### Escalation

Judge timeouts affecting > 10 % of evaluation samples in a release run are P2.

---

## Scenario 5 — Evaluation Budget Exceeded

**Log pattern:** `[EVAL:BudgetExceeded]`

### Detection

- Log line: `[EVAL:BudgetExceeded] budget_ms=<B> elapsed_ms=<E> remaining_samples=<R>`
- Metric: `eval_budget_exceeded_count_total`
- Alert: `EvalBudgetExceeded`

### Immediate actions

1. **Reduce sample count** for the offending benchmark matrix run.
2. **Enable early-stopping.** Set `eval.early_stop_on_budget_exceeded=true` to
   emit a partial result rather than blocking forever.
3. **Profile the slowest evaluation path.** Use `benchmarks/epic2_evaluation/`
   benchmark harness to identify the bottleneck metric or judge call.
4. **Schedule the benchmark run outside business hours** if it legitimately
   requires more time.

### Escalation

Budget overruns that block the release gate CI pipeline for > 30 minutes require
a P2 escalation and a decision on whether to skip the affected gate this cycle.

---

## Related resources

- `src/evaluation/ROADMAP.md` — module roadmap and Wave D closure
- `tests/integration/test_evaluation_framework_soak.cpp` — soak test
- `tests/evaluation/test_evaluation_highcardinality_stress.cpp` — stress test
- `benchmarks/evaluation/bench_evaluation_dedicated_gates.cpp` — EV-BM-01..04
- `src/evaluation/PHASE_6_ACCEPTANCE_CHECKLIST.md` — acceptance criteria
