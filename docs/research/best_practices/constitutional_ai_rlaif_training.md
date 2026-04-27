# Constitutional AI / RLAIF Training Pipeline

**Metadaten:**
- Source: Bai et al. (2022) — Constitutional AI (Anthropic); Lee et al. (2023) — RLAIF (Google, ICML 2024); ThemisDB Engineering
- URL: [Constitutional AI arXiv:2212.08073](https://arxiv.org/abs/2212.08073) · [RLAIF arXiv:2309.00267](https://arxiv.org/abs/2309.00267)
- Tags: `constitutional-ai`, `rlaif`, `ai-feedback`, `preference-learning`, `reward-model`, `ppo`, `lora-finetuning`, `continuous-learning`, `rlhf-alternative`, `guardrails`
- ThemisDB-Versionen: v1.6.0+ (`src/rag/rlaif_trainer.cpp`, `src/rag/continuous_learning_orchestrator.cpp`)
- Status: [x] Fully Adopted

## 📋 Summary

The Constitutional AI / RLAIF Training Pipeline eliminates the need for human preference labellers in ThemisDB's continuous model improvement loop. An `IAIJudge` — either heuristic or LLM-backed — generates pairwise preference labels from candidate response pairs according to a set of constitutional principles. These preferences train a `RewardModel`, which drives `IncrementalLoRATrainer` LoRA fine-tuning via `ContinuousLearningOrchestrator` Loop 4. A `RLAIFGuardrailPlugin` blocks LoRA updates that fail the reward threshold, preventing reward hacking. The entire pipeline runs autonomously on a configurable feedback cadence.

## 🎯 Core Principles

1. **Constitutional principles as safety specification**: Encode domain-specific compliance rules (GDPR, administrative law, data privacy) as explicit principles in `config/prompts/constitutional_principles.yaml`. The AI judge applies these principles to generate preference labels — no human labellers required.
2. **AI feedback hierarchy**: Prefer `LLMBackedAIJudge` when an inference engine is available; fall back to `HeuristicAIJudge` (quality + diversity heuristic) in offline deployments. Document which judge type is active in `LearningStats`.
3. **Reward model training threshold**: Do not trigger LoRA fine-tuning until `PreferenceDataset.size() >= min_feedback_count` (default 500 pairs). Insufficient training data produces unreliable reward signals.
4. **Guardrail before every LoRA update**: `RLAIFGuardrailPlugin::check()` must pass before `IncrementalLoRATrainer::finetune()` is called. Reject if reward model score < `min_reward_threshold` or if `BiasDetector` flags the trained response distribution.
5. **Federated LoRA coordination**: After a successful Loop 4, `ILoRAFederationCoordinator::onFederatedRoundStart()` is called to propagate the updated adapter to peer nodes — enabling distributed, privacy-preserving model improvement.
6. **Continuous learning loop integration**: RLAIF is Loop 4 in `ContinuousLearningOrchestrator`; it builds on Loops 1–3 (HNSW query, workload, schema/index) and triggers only when sufficient feedback has accumulated.
7. **No full RLHF / PPO**: ThemisDB uses LoRA fine-tuning as the practical lightweight alternative to PPO reward maximisation. DPO (Direct Preference Optimisation) is planned as a v2.2.0 upgrade path.

## 🔗 Adoption in ThemisDB

### Affected Modules

- `src/rag/rlaif_trainer.cpp` — `RLAIFTrainer`, `IAIJudge`, `RewardModel`, `PreferenceDataset`, `RLAIFGuardrailPlugin`
- `src/rag/continuous_learning_orchestrator.cpp` — `ContinuousLearningOrchestrator::triggerLoop(LOOP_4_RLAIF)`; `ILoRAFederationCoordinator`
- `src/prompt_engineering/reflection_tuner.cpp` — `ReflectionTuner::CONSTITUTIONAL` (supervised critique-revision phase)
- `src/rag/bias_detector.cpp` — `BiasDetector`: constitutional compliance filter in evaluation pipeline
- `config/prompts/constitutional_principles.yaml` — domain-specific principles (GDPR, admin law, data privacy)

### What Was Adopted?

**Constitutional principles YAML**

```yaml
# config/prompts/constitutional_principles.yaml
constitutional_principles:
  # ThemisDB domain: German administrative law + GDPR
  - "Response must not reveal personal data (PII) without explicit consent."
  - "Response must not contain discriminatory language based on race, gender, or religion."
  - "Response must accurately cite legal basis when referencing administrative regulations."
  - "Response must not hallucinate statutory references; use only verified sources."
  - "Response must be proportionate: do not collect more information than necessary."
  - "Response must respect the purpose limitation principle (GDPR Art. 5(1)(b))."
  - "Response must be helpful without being misleading; acknowledge uncertainty when present."
  - "Response must not recommend actions that violate administrative procedural law."
```

**`IAIJudge` interface + concrete implementations**

```cpp
// include/rag/rlaif_trainer.h
class IAIJudge {
public:
    virtual ~IAIJudge() = default;
    // Returns A_BETTER, B_BETTER, or TIE based on constitutional criteria.
    // Thread-safe: called from parallel preference-generation threads.
    virtual Preference compare(
        const std::string& prompt,
        const std::string& response_a,
        const std::string& response_b,
        const std::vector<std::string>& principles) = 0;
};

// HeuristicAIJudge: quality (length-capped) + diversity (unique-token ratio)
// LLMBackedAIJudge: uses LLMJudgeIntegration; returns preference from structured JSON response
```

**RLAIF pipeline (Loop 4)**

```cpp
// src/rag/continuous_learning_orchestrator.cpp — triggerLoop(LOOP_4_RLAIF)
LoopResult ContinuousLearningOrchestrator::runLoop4RLAIF() {
    // 1. Collect candidate response pairs from interaction log
    auto pairs = collectResponsePairs(config_.min_feedback_count);
    if (pairs.size() < config_.min_feedback_count) {
        return LoopResult::SKIPPED_INSUFFICIENT_DATA;
    }

    // 2. Generate preference labels via IAIJudge
    PreferenceDataset dataset = rlaif_trainer_.generatePreferences(
        pairs, constitutional_principles_);

    // 3. Train reward model from preferences
    rlaif_trainer_.trainRewardModel(dataset);

    // 4. Guardrail check before LoRA update
    if (!rlaif_trainer_.checkGuardrails(dataset)) {
        THEMIS_WARN("RLAIF Loop 4: guardrail check failed; LoRA update suppressed");
        return LoopResult::GUARDRAIL_BLOCKED;
    }

    // 5. LoRA fine-tuning driven by reward model
    incremental_lora_trainer_.finetune(dataset, rlaif_trainer_.getRewardModel());

    // 6. Federated coordination (if federation coordinator registered)
    if (federation_coordinator_) {
        federation_coordinator_->onFederatedRoundStart(
            incremental_lora_trainer_.getLatestAdapterId());
    }

    return LoopResult::SUCCESS;
}
```

**`RLAIFGuardrailPlugin` safety gate**

```cpp
// src/rag/rlaif_trainer.cpp
bool RLAIFTrainer::checkGuardrails(const PreferenceDataset& dataset) {
    // Guardrail 1: Reward model score must meet minimum threshold
    float avg_reward = rewardModel_.averageScore(dataset.chosen_responses);
    if (avg_reward < config_.min_reward_threshold) {
        THEMIS_WARN("RLAIF guardrail: avg_reward={:.3f} < min={:.3f}",
                    avg_reward, config_.min_reward_threshold);
        return false;
    }
    // Guardrail 2: No constitutional violations in chosen responses
    for (const auto& response : dataset.chosen_responses) {
        for (const auto& principle : constitutional_principles_) {
            if (!biasDetector_.complies(response, principle)) {
                THEMIS_WARN("RLAIF guardrail: principle violation detected");
                return false;
            }
        }
    }
    return true;
}
```

### Deviations & Rationale

| Constitutional AI / RLAIF Standard | ThemisDB Adaptation | Rationale |
|---|---|---|
| PPO reward maximisation | `IncrementalLoRATrainer` LoRA fine-tuning | PPO requires full model access; LoRA is lightweight and compatible with llama.cpp |
| Single GPT-4 as judge | `IAIJudge` + `HeuristicAIJudge` fallback | LLM-agnostic; offline deployments use heuristic judge |
| Anthropic generic principles | Domain-specific YAML (GDPR, admin law) | ThemisDB's primary domain requires legal and regulatory specificity |
| Single training run | `ContinuousLearningOrchestrator` Loop 4 (periodic) | Continuous improvement aligned with production feedback cadence |
| No federated learning | `ILoRAFederationCoordinator` for distributed adapter propagation | ThemisDB is multi-tenant; federated LoRA enables privacy-preserving distributed improvement |

## ⚠️ Trade-offs & Limitations

- `HeuristicAIJudge` quality is substantially lower than a properly aligned LLM judge; heuristic-generated preferences may introduce label noise.
  - Mitigation: `LearningMetrics` tracks reward model quality trends; alert when quality regresses > 10%.
- LoRA fine-tuning (vs. PPO) cannot shape the entire policy distribution; it may underfit on complex preference patterns.
  - Mitigation: DPO (Direct Preference Optimisation) planned as v2.2.0 upgrade; DPO is more effective than LoRA for preference learning.
- Constitutional principle compliance checking is string/heuristic-based for `HeuristicAIJudge`; shallow pattern matching may miss semantic violations.
  - Mitigation: LLM-backed judge for principle compliance when inference engine is available; heuristic is documented as partial.
- Federated LoRA coordination (`ILoRAFederationCoordinator`) is an interface placeholder; real federation client not yet implemented.
  - Open: Implement `FederatedLoRAClient` with S-LoRA-compatible adapter exchange protocol (Target: v2.0.0).

## 🔬 Validation

- [x] `IAIJudge`, `HeuristicAIJudge`, `LLMBackedAIJudge` implemented and tested
- [x] `RewardModel` + `PreferenceDataset` + `RLAIFGuardrailPlugin` implemented
- [x] Loop 4 `ContinuousLearningOrchestrator::triggerLoop(LOOP_4_RLAIF)` implemented
- [x] Constitutional principles YAML configuration implemented
- [x] Unit tests written (`rlaif_trainer` tests)
- [ ] Benchmark executed (RLAIF-trained LoRA vs. baseline quality on ThemisDB domain prompts)
- [ ] Module README linked (`src/rag/README.md`)
- [x] implementation_influence index updated

## 📚 Related

- [Paper: Constitutional AI + RLAIF — Bai & Lee (2022/2023)](../papers/bai_constitutional_ai_rlaif_2022.md)
- [Paper: LLM-as-Judge / MT-Bench — Zheng et al. (2023)](../papers/zheng_llm_judge_2023.md) — `LLMBackedAIJudge` uses `LLMJudgeIntegration`
- [Best Practice: LLM-as-Judge RAG Evaluation](llm_as_judge_rag_evaluation.md)
- [Paper: Self-Refine + Reflexion — Madaan & Shinn (2023)](../papers/madaan_self_refine_2023.md) — CONSTITUTIONAL reflection strategy = supervised CAI phase
- [Paper: S-LoRA — Sheng et al. (2023)](../papers/sheng_slora_concurrent_adapters_2023.md) — LoRA serving for trained adapters
- [`src/rag/rlaif_trainer.cpp`](../../../src/rag/rlaif_trainer.cpp)
- [`src/rag/continuous_learning_orchestrator.cpp`](../../../src/rag/continuous_learning_orchestrator.cpp)

---
**Last Updated:** 2026-04-27
