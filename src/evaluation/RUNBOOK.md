---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-10-30"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Evaluation Module Runbook

AI provenance: Exact model backend not verifiable from final document text alone.

## Scope

This runbook supports a maintainer diagnosing evaluation-library behavior or
failed validation. `src/evaluation` is a library, not a standalone service: it
does not define a module-owned health endpoint, CLI, dashboard, or service
configuration. Use the consuming application's logs and runtime context, then
inspect the typed results described below. Do not rely on operational metric
names or configuration switches unless the consuming application documents
them.

## Planner chose an unexpected or exact path

1. Capture the full `PlannerDecision`: `path`, `fallback_reason`, decision note,
   policy version, threshold key, and usage flags.
2. Capture the exact `ExecutionEligibility`, `TensorArtifactFreshness`, and
   `PlannerConfig` supplied to `selectPath()`. Check `force_exact`,
   `force_cpu`, ANN and readiness gates, artifact age, residual, delta lag,
   rank cap, rebuild state, multi-shard state, and manifest availability.
3. Map `FallbackReason` to the failed gate in `query_planner.h`. For example,
   stale or rebuilding artifacts, an exceeded rank cap, or missing shard
   manifests produce typed reasons. A planner decision reports selection; it
   does not execute the selected retrieval path.
4. If truth-bearing correctness is in doubt, keep or set the caller's
   `force_exact` input and validate against the exact graph path. Do not force an
   approximate route to remove the symptom.
5. Reproduce the input combination in `query_planner_test` when that target is
   available, then update the regression test for any confirmed defect.

## Metric calculation failed

1. Catch `MetricError` and inspect `kind()` and `what()` at the call site.
2. Validate ground-truth and returned IDs for the expected uniqueness, cutoff
   range, finite numeric values, and domain-specific probability/count rules.
3. Correct the fixture or caller input. Do not convert an exception to a
   successful zero-valued metric; that can turn invalid evidence into a false
   gate result.
4. Re-run the focused retrieval-metrics test:

   ```bash
   cmake --build <build-dir> --target module_evaluation_test_evaluation_retrieval_metrics_focused
   ctest --test-dir <build-dir> -R '^test_evaluation_retrieval_metrics_focused_EvaluationFocusedTests$' --output-on-failure
   ```

## Artifact lifecycle is stale or failed

1. Inspect `LifecycleMetadata`: lifecycle state, invalidation reason, source
   sequence range, delta lag, artifact age, residual, rank, and rebuild
   timestamps/attempt count.
2. Apply the relevant `StalenessPolicy` thresholds and call
   `diagnoseStalenessCause()` or `computeState()` to identify the state
   transition.
3. Use lifecycle transitions to record invalidation or rebuild completion; the
   evaluation manager does not perform the rebuild or persist the artifact.
4. Keep a failed, invalidated, or stale artifact out of planning until the
   owning rebuild/validation path has restored acceptable metadata. Preserve
   the exact-path fallback for truth-bearing requests.
5. Run `test_evaluation_artifact_lifecycle_focused` after a code change.

## Benchmark evidence is missing or suspicious

1. Confirm the benchmark target was built and executed; the presence of a
   benchmark source file is not a measurement.
2. Check scenario, dimension, dataset tag, hardware tag, runner version, sample
   count, and edge flags. `BenchmarkResult::isClean()` and
   `hasSufficientData()` help identify annotated or undersampled results.
3. Re-run the scenario with fixed inputs and retain raw output plus compiler,
   build type, hardware, and run configuration.
4. Compare results only when dataset, measurement definition, and configuration
   match. Do not promote thresholds or guardrails as measured baselines.

## Escalation and rollback

Escalate correctness-affecting fallback behavior, Category C policy violations,
unrecovered summary-first false negatives, or benchmark evidence that could
change a release gate to the module owner and release reviewer. Until resolved,
disable the affected approximate caller route or force the exact path; withdraw
unsupported performance claims and retain the incident inputs and test output.

## References

- [Operations checks](./OPERATIONS.md)
- [Trust and risk model](./TRUST.md)
- [API and failure contracts](./API.md)
- [Existing framework runbook](../../docs/operability/RUNBOOK_EVALUATION_FRAMEWORK.md)
- `src/evaluation/include/query_planner.h`
- `src/evaluation/include/retrieval_metrics.h`
- `src/evaluation/include/artifact_lifecycle.h`
- `src/evaluation/include/benchmark_matrix.h`
