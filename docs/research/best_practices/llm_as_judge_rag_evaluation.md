# LLM-as-Judge RAG Evaluation: G-Eval + Multi-Judge Ensemble + Bias Mitigation

**Metadaten:**
- Source: Liu et al. (2023) — G-Eval (EMNLP); Zheng et al. (2023) — MT-Bench / LLM-as-Judge (NeurIPS); Es et al. (2023) — RAGAS; ThemisDB Engineering
- URL: [G-Eval arXiv:2303.16634](https://arxiv.org/abs/2303.16634) · [MT-Bench arXiv:2306.05685](https://arxiv.org/abs/2306.05685) · [RAGAS arXiv:2309.15217](https://arxiv.org/abs/2309.15217)
- Tags: `g-eval`, `llm-judge`, `mt-bench`, `rag-evaluation`, `position-bias`, `verbosity-bias`, `judge-ensemble`, `calibration`, `faithfulness`, `coherence`, `pairwise-comparison`
- ThemisDB-Versionen: v1.6.0+ (`src/rag/geval_evaluator.cpp`, `src/rag/llm_judge_integration.cpp`, `src/rag/pairwise_comparator.cpp`, `src/rag/rag_judge.cpp`)
- Status: [x] Fully Adopted

## 📋 Summary

LLM-as-Judge RAG Evaluation replaces traditional reference-based metrics (ROUGE, BERTScore) with an LLM-based multi-dimensional quality assessment pipeline. The evaluator scores retrieved and generated content on five dimensions (faithfulness, relevance, completeness, coherence, bias) using calibrated LLM judges with explicit bias mitigations — achieving human-level agreement (> 80%) on standard benchmarks. ThemisDB implements this in `RAGJudge` (orchestrator), `GEvalEvaluator` (probabilistic token-probability scoring), `LLMJudgeIntegration` (inference bridge), `PairwiseComparator` (head-to-head with position-bias mitigation), `DistributedRAGEvaluator` (multi-judge ensemble), and `CalibrationManager` (score calibration).

## 🎯 Core Principles

1. **Probabilistic token-probability scoring (G-Eval)**: Extract log-probabilities for all score tokens ("1"–"5") via `llama_get_logits_ith()`; compute expected score `E[s] = Σ p(s_i) × i`; normalise to [0,1]. Never greedy-decode a single score — this discards calibration information.
2. **Bias-aware pairwise comparison**: Always randomise presentation order in pairwise comparisons; run both orderings; declare TIE if results are inconsistent. Inject verbosity-bias rubric: "Length alone is NOT a quality indicator."
3. **Heterogeneous judge ensemble**: Mix models of different sizes/families to mitigate self-enhancement bias. Use MAJORITY_VOTING for binary decisions; WEIGHTED_MEAN for continuous scores.
4. **Calibration before production use**: Apply `CalibrationManager` (temperature scaling → Platt scaling → isotonic regression) to align LLM judge scores with human annotations. Track ECE; alert when ECE > 0.1.
5. **Mock guard in production**: `LLMJudgeIntegration` requires `ILLMInferenceEngine*` or explicit `allow_mock = true` — no silent mock fallback in production deployments.
6. **Mode-based resource allocation**: FAST mode (< 100 ms): heuristic evaluators only; BALANCED mode (< 500 ms): G-Eval for primary dimensions; THOROUGH mode (< 2 s): full multi-judge ensemble with pairwise comparison.
7. **Evaluation caching**: Cache `(query, answer, docs) → EvaluationReport` with TTL in `EvaluationCache` to avoid redundant LLM calls on repeated content.

## 🔗 Adoption in ThemisDB

### Affected Modules

- `src/rag/rag_judge.cpp` — `RAGJudge`: multi-dimensional evaluation orchestrator (faithfulness, relevance, completeness, coherence, bias)
- `src/rag/geval_evaluator.cpp` — `GEvalEvaluator`: token-probability probabilistic scoring via llama.cpp
- `src/rag/llm_judge_integration.cpp` — `LLMJudgeIntegration`: LLM inference bridge; mock guard; position-bias prompt formatting
- `src/rag/pairwise_comparator.cpp` — `PairwiseComparator`: head-to-head comparison with randomised order + consistency check
- `src/rag/distributed_rag_evaluator.cpp` — `DistributedRAGEvaluator`: N-judge parallel ensemble; inter-judge agreement metric
- `src/rag/calibration_manager.cpp` — `CalibrationManager`: temperature/Platt/isotonic calibration; ECE/Brier/correlation tracking
- `src/rag/evaluation_cache.cpp` — `EvaluationCache`: thread-safe LRU cache with TTL for judge results

### What Was Adopted?

**G-Eval probabilistic scoring**

```cpp
// src/rag/geval_evaluator.cpp
// Token-probability expected-value scoring (G-Eval paper, §3.1)
float GEvalEvaluator::computeExpectedScore(
    llama_context* ctx,
    const std::vector<std::string>& score_tokens)  // {"1","2","3","4","5"}
{
    auto* logits = llama_get_logits_ith(ctx, -1);   // last position
    int   n_vocab = llama_n_vocab(ctx->model);

    // Collect log-probs for score tokens; softmax over score vocab only
    std::vector<float> probs;
    for (const auto& tok : score_tokens) {
        llama_token id = tokenize_single(ctx->model, tok);
        probs.push_back(logits[id]);
    }
    // Stable softmax
    float max_logit = *std::max_element(probs.begin(), probs.end());
    float sum_exp = 0.f;
    for (auto& p : probs) { p = std::exp(p - max_logit); sum_exp += p; }
    for (auto& p : probs) { p /= sum_exp; }

    // E[s] = sum(p_i * (i+1)) for i in 0..4 → range [1,5]
    float expected = 0.f;
    for (size_t i = 0; i < probs.size(); ++i)
        expected += probs[i] * static_cast<float>(i + 1);

    return (expected - 1.f) / 4.f;  // normalise to [0,1]
}
```

**Position-bias mitigation in PairwiseComparator**

```cpp
// src/rag/pairwise_comparator.cpp
PairwiseResult PairwiseComparator::compare(
    const std::string& query,
    const std::string& answer_a, const std::string& answer_b)
{
    // Run in both orderings to cancel position bias
    auto result_ab = judgeOnce(query, answer_a, answer_b, "A", "B");
    auto result_ba = judgeOnce(query, answer_b, answer_a, "B", "A");

    // Consistency check (MT-Bench §4.2 mitigation)
    if (result_ab == WINNER_FIRST && result_ba == WINNER_SECOND)
        return PairwiseResult::A_BETTER;   // consistent
    if (result_ab == WINNER_SECOND && result_ba == WINNER_FIRST)
        return PairwiseResult::B_BETTER;   // consistent
    return PairwiseResult::TIE;            // inconsistent → position bias signal
}
```

**Verbosity-bias rubric injection**

```cpp
// src/rag/prompt_templates.cpp — judge prompt template
static constexpr std::string_view kJudgeRubricSuffix =
    "\n\nIMPORTANT: Length alone is NOT a quality indicator. "
    "Do not prefer longer responses simply because they are longer. "
    "Judge based on accuracy, relevance, and completeness only.";
```

**Multi-judge ensemble (DistributedRAGEvaluator)**

```cpp
// src/rag/distributed_rag_evaluator.cpp
EvaluationReport DistributedRAGEvaluator::evaluate(
    const std::string& query, const std::string& answer,
    const std::vector<RetrievedDocument>& docs)
{
    // Dispatch to N judge instances in parallel thread pool
    std::vector<std::future<EvaluationReport>> futures;
    for (auto& judge : judge_instances_)
        futures.push_back(thread_pool_.enqueue(
            [&] { return judge->evaluate(query, answer, docs); }));

    // Collect + aggregate
    std::vector<EvaluationReport> reports;
    for (auto& f : futures) reports.push_back(f.get());

    return aggregate(reports, config_.aggregation_strategy);
    // MEAN | WEIGHTED_MEAN | MAJORITY_VOTING | BEST_OF_N
}
```

**Calibration pipeline**

```cpp
// src/rag/calibration_manager.cpp
float CalibrationManager::calibrate(float raw_score, const std::string& dimension) {
    // 1. Temperature scaling: s_cal = sigmoid(s_raw / T)
    float temp_scaled = 1.f / (1.f + std::exp(-raw_score / temperature_));
    // 2. Optional Platt scaling: s_platt = sigmoid(A * s_cal + B)
    if (use_platt_) temp_scaled = plattScale(temp_scaled, dimension);
    // 3. Optional isotonic regression (learned from human annotations)
    if (use_isotonic_) temp_scaled = isotonicRegress(temp_scaled, dimension);
    return std::clamp(temp_scaled, 0.f, 1.f);
}
```

### Deviations & Rationale

| Best-Practice Standard | ThemisDB Adaptation | Rationale |
|---|---|---|
| Single GPT-4 judge | `ILLMInferenceEngine*` + local llama.cpp | Cost and privacy: no cloud API required |
| Fixed evaluation rubric | `PromptTemplates` YAML configurable per domain | German administrative domain requires domain-specific rubrics |
| Reference-required RAGAS | Hybrid: reference-free G-Eval + optional RAGAS | Production: references not always available; G-Eval fills the gap |
| No caching | `EvaluationCache` LRU + TTL | High QPS: caching avoids redundant LLM calls on repeated content |
| Static calibration | Online `CalibrationManager` with human annotation import | Production drift: calibration must be updated as model or domain shifts |

## ⚠️ Trade-offs & Limitations

- G-Eval token-probability scoring requires direct model logit access; not available through all inference APIs.
  - Mitigation: `GEvalEvaluator::Config::use_logprobs` flag; greedy-decoded single-score parsing as fallback.
- Multi-judge ensemble multiplies LLM inference cost by N; not feasible in FAST mode.
  - Mitigation: FAST mode uses heuristic evaluators; G-Eval and ensemble activate only in BALANCED/THOROUGH mode.
- Calibration requires human-annotated examples for the target domain; cold-start deployments use uncalibrated scores.
  - Mitigation: Default temperature scaling with T=1.0 (identity) until human annotation data is available.

## 🔬 Validation

- [x] G-Eval token-probability scoring implemented and tested
- [x] Position-bias mitigation (swap + consistency check) implemented in `PairwiseComparator`
- [x] Verbosity-bias rubric injected in judge prompts
- [x] Multi-judge ensemble (MEAN/WEIGHTED_MEAN/MAJORITY_VOTING/BEST_OF_N) implemented
- [x] `CalibrationManager` with temperature/Platt/isotonic implemented
- [x] `allow_mock = false` default enforced in production
- [ ] Benchmark: G-Eval Spearman correlation on ThemisDB domain QA
- [ ] Module README linked (`src/rag/README.md`)
- [x] implementation_influence index updated

## 📚 Related

- [Paper: G-Eval — Liu et al. (2023)](../papers/liu_geval_2023.md)
- [Paper: LLM-as-Judge / MT-Bench — Zheng et al. (2023)](../papers/zheng_llm_judge_2023.md)
- [Paper: Constitutional AI / RLAIF — Bai & Lee (2022/2023)](../papers/bai_constitutional_ai_rlaif_2022.md)
- [LLM Integration Scientific Foundations — RAGAS](../LLM_INTEGRATION_SCIENTIFIC_FOUNDATIONS.md#63-rag-evaluation-ragas)
- [`src/rag/rag_judge.cpp`](../../../src/rag/rag_judge.cpp)
- [`src/rag/geval_evaluator.cpp`](../../../src/rag/geval_evaluator.cpp)
- [`src/rag/pairwise_comparator.cpp`](../../../src/rag/pairwise_comparator.cpp)

---
**Last Updated:** 2026-04-27
