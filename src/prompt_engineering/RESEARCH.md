---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Prompt Engineering Research and Evaluation

## Purpose and scope

This page records the evaluation assumptions developers need when comparing prompt variants. It covers the deterministic scoring and experiment machinery in `prompt_engineering`; it does not claim that the module trains or serves an LLM.

## Evaluation assumptions

- Evaluation is only as representative as the supplied `(output, expected)` pairs or regression fixtures. The module does not supply a canonical domain dataset.
- `PromptEvaluator` computes normalized exact match, normalized edit-distance partial match, expected-token relevance, and semantic similarity. Without an `IEmbeddingProvider`, semantic similarity is token-set Jaccard overlap.
- Batch scores are averages over the submitted cases. A small or biased fixture set can hide regressions even when the aggregate score is stable.
- `PromptOptimizer` accepts caller-provided evaluation and improvement functions. Its result therefore measures the behavior of those functions and the supplied test cases, not an intrinsic measure of prompt quality.

## Methods and measurements

`PromptEvaluator::evaluateBatch()` reports the overall weighted score, mean and standard deviation of semantic similarity, exact-match count, pass rate, and per-case metrics. The default evaluator weights are 0.5 similarity, 0.3 exact match, and 0.2 relevance, with a 0.7 pass threshold. Partial match contributes as a floor to each of those three score components in the weighted score.

`PromptEvaluator::isStatisticallySignificant()` compares baseline and candidate score samples with a one-tailed Welch test. Empty samples and a candidate mean no greater than the baseline are not significant. Treat significance as one input to rollout decisions; it does not establish that the fixtures cover production traffic.

`PromptABExperimentFramework` assigns control/treatment by hashing a stable request identifier, records score observations, and summarizes sample counts, means, delta, p-value, and winner. Its descriptor defaults to 200 minimum samples per variant and 0.95 confidence. Set the traffic split, minimum samples, and confidence level deliberately for the deployment and outcome metric.

`PromptRegressionRunner` compares baseline and candidate outputs over configured fixtures. The defaults allow at most a 5 percentage-point score drop, require at least one fixture, and block on a regression. A result below the fixture minimum is inconclusive; it is not evidence of parity.

## Reproducible evaluation

1. Keep a versioned, reviewed fixture set with expected outputs and identify fixtures derived from curated "golden" data versus positive feedback.
2. Record the evaluator configuration, embedding-provider implementation and model/version (if any), candidate and baseline prompt versions, and the same test inputs for each run.
3. Use per-case metrics and fixture deltas to investigate aggregate changes; do not promote solely on an average.
4. Run the focused module tests and the prompt-engineering benchmark cases before comparing release baselines.

Focused behavioral evidence for template, evaluator, optimizer, feedback, and injection paths is in [`tests/prompt_engineering/test_prompt_engineering_focused.cpp`](../../tests/prompt_engineering/test_prompt_engineering_focused.cpp) and [`tests/prompt_engineering/test_prompt_engineering_adversarial_focused.cpp`](../../tests/prompt_engineering/test_prompt_engineering_adversarial_focused.cpp). [`test_rewrite_engine_focused.cpp`](../../tests/prompt_engineering/test_rewrite_engine_focused.cpp) checks interface/compile contracts; it does not exercise rule execution. The roadmap tracks functional rule-behavior and request-pipeline tests as incomplete. Benchmark case mapping and release thresholds are in [`PERFORMANCE_EXPECTATIONS.md`](./PERFORMANCE_EXPECTATIONS.md) and [`benchmarks/prompt_engineering/bench_prompt_engineering.cpp`](../../benchmarks/prompt_engineering/bench_prompt_engineering.cpp).

## Limits and trade-offs

- Jaccard overlap is lexical, not a semantic understanding of equivalent answers. Embedding-based scoring depends on an external provider and its stable model behavior.
- Normalization, tokenization, and edit distance are implementation-defined comparison methods; results from another evaluator are not directly interchangeable.
- Stochastic generation, data leakage between tuning and evaluation sets, unreviewed feedback fixtures, and changing provider versions can invalidate a comparison.
- Statistical significance cannot compensate for unrepresentative fixtures or an unsuitable score definition.

See [`MODELING.md`](./MODELING.md), [`EMBEDDING.md`](./EMBEDDING.md), and [`API.md`](./API.md) for the corresponding model, provider, and interface contracts.

Exact model backend not verifiable from final document text alone.
