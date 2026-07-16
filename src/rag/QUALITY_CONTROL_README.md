<!-- Status: current | validated: 2026-05-13 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · include/rag/README.md · docs/de/features/quality_control_pipeline.md -->

# RAG Quality Control — Implementation Reference

## Scope

This document is the **implementation-level reference** for the post-generation quality control
subsystem inside `src/rag/`.  It covers component inventory, control flow, metrics, deviation
handling, configuration, and the build/test surface.

**It does not duplicate:**

| Topic | Canonical location |
|---|---|
| Public API contracts | [`include/rag/README.md`](../../include/rag/README.md) and individual headers under `include/rag/` |
| User-facing feature guide and architecture diagrams | [`docs/de/features/quality_control_pipeline.md`](../../docs/de/features/quality_control_pipeline.md) |
| Module-level roadmap and planned work | [`src/rag/ROADMAP.md`](ROADMAP.md) |
| Open enhancement specifications | [`src/rag/FUTURE_ENHANCEMENTS.md`](FUTURE_ENHANCEMENTS.md) |
| Module architecture | [`src/rag/ARCHITECTURE.md`](ARCHITECTURE.md) |

---

## Components

### Core Pipeline Components

#### LLM Judge Client
- **Header**: `include/rag/llm_judge_client.h`
- **Source**: `src/rag/llm_judge_client.cpp`
- **Purpose**: Connects evaluation prompts to `InferenceEngineEnhanced`
- **Features**: Context caching, batch processing, multi-model load balancing, configurable temperature/sampling

#### G-Eval Evaluator
- **Header**: `include/rag/geval_evaluator.h`
- **Source**: `src/rag/geval_evaluator.cpp`
- **Purpose**: Probabilistic scoring via token probabilities (Liu et al., 2023)
- **Features**: Continuous `[0, 1]` scores, multiple-sample aggregation, confidence estimation via entropy

#### NLI Faithfulness Verifier
- **Header**: `include/rag/nli_faithfulness_verifier.h`
- **Source**: `src/rag/nli_faithfulness_verifier.cpp`
- **Purpose**: Claim-level faithfulness verification using Natural Language Inference
- **Features**: Entailment checking, contradiction detection; interface ready for RoBERTa-large-MNLI / DeBERTa-v3 (currently uses documented heuristic — see NLI note below)

#### Quality Control Pipeline
- **Header**: `include/rag/quality_control_pipeline.h`
- **Source**: `src/rag/quality_control_pipeline.cpp`
- **Purpose**: Multi-stage orchestration of all quality checks with configurable gates
- **Features**: Fast / Balanced / Thorough modes, auto-retry, citation-coverage check, learning feedback callbacks

#### Quality Pipeline Factory
- **Header**: `include/rag/quality_control_pipeline.h` (`QualityPipelineFactory`)
- **Source**: `src/rag/quality_control_factory.cpp`
- **Purpose**: Pre-configured factory presets (`createFast`, `createBalanced`, `createThorough`, `createProduction`)

### Supporting Quality Components

#### Calibration Manager
- **Header**: `include/rag/calibration_manager.h`
- **Source**: `src/rag/calibration_manager.cpp`
- **Purpose**: Aligns raw judge scores with human annotations using temperature scaling, Platt scaling, and isotonic regression
- **Metrics exported**: ECE (Expected Calibration Error), Brier score, Pearson/Spearman correlation

#### Citation Highlighter
- **Header**: `include/rag/citation_highlighter.h`
- **Source**: `src/rag/citation_highlighter.cpp`
- **Purpose**: Maps each answer sentence to its source document chunk; populates `QualityCheckResult::citation_coverage`
- **Used by**: Stage 3 (Thorough) when `Config::enable_citation_check = true`

#### Hallucination Dashboard
- **Header**: `include/rag/hallucination_dashboard.h`
- **Source**: `src/rag/hallucination_dashboard.cpp`
- **Purpose**: Rolling-window hallucination rate tracking and alerting based on QC pipeline results

#### Judge Ensemble
- **Header**: `include/rag/judge_ensemble.h`
- **Source**: `src/rag/judge_ensemble.cpp`
- **Purpose**: Multi-judge voting strategies (mean, weighted mean, majority, best-of-N) for combining scores from multiple judge models

> **NLI placeholder note:** `NLIFaithfulnessVerifier` uses a heuristic scorer when no real NLI model
> is loaded (ONNX model path not configured).  The heuristic is documented in `nli_faithfulness_verifier.cpp`
> and the `FaithfulnessEvaluator` falls back to it gracefully.  Load a real model via
> `OnnxModelLoader` to activate neural entailment checking.

---

## Quality Control Flow

```
User Query + Generated Answer + Retrieved Documents
          │
          ▼
┌─────────────────────────────────────────────────────┐
│ Stage 1: Fast Screening             (<50 ms target)  │
│   • LLM Judge on faithfulness only                   │
│   • threshold: fast_stage_threshold (default 0.6)    │
│   • Early-reject clearly bad answers → FAILED        │
└───────────────────┬─────────────────────────────────┘
                    │ PASS
                    ▼
┌─────────────────────────────────────────────────────┐
│ Stage 2: Balanced Evaluation       (<500 ms target)  │
│   • Multi-dimension LLM judging                      │
│     (faithfulness, relevance, completeness,          │
│      coherence, ethical compliance)                  │
│   • G-Eval probabilistic scoring                     │
│   • Aggregate score weighted by dimension weights    │
│   • threshold: balanced_stage_threshold (default 0.7)│
└───────────────────┬─────────────────────────────────┘
                    │ PASS
                    ▼
┌─────────────────────────────────────────────────────┐
│ Stage 3: Thorough Verification       (<2 s target)   │
│   • NLI faithfulness verification (claim-level)      │
│   • Contradiction detection                          │
│   • Citation coverage check (CitationHighlighter)    │
│   • threshold: thorough_stage_threshold (default 0.8)│
└───────────────────┬─────────────────────────────────┘
                    │ PASS / FAILED / RETRY_NEEDED
                    ▼
┌─────────────────────────────────────────────────────┐
│ Stage 4: Continuous Learning                         │
│   • Async feedback to ContinuousLearningOrchestrator │
│   • HallucinationDashboard rolling-window update     │
│   • CalibrationManager score alignment               │
└─────────────────────────────────────────────────────┘
```

### Gate Statuses and Deviation Handling

| `QualityGateStatus` | Meaning | Default action |
|---|---|---|
| `PASSED` | All active stage thresholds met | Deliver answer |
| `FAILED` | Score below threshold, retry budget exhausted | Return failure result; `failure_reasons` populated |
| `RETRY_NEEDED` | Score between `retry_threshold` and stage threshold | Re-trigger generation up to `max_retries` times |
| `ESCALATE` | Score too low even for retry heuristic | Flag for human review; `improvement_suggestions` populated |

Retry logic is controlled by `Config::enable_auto_retry`, `Config::max_retries`, and
`Config::retry_threshold`.  When all retries are exhausted and the answer still fails,
`QualityGateStatus::FAILED` is returned with populated `failure_reasons`.

---

## Quality Metrics

### Dimension Weights (defaults)

| Dimension | Weight | Primary method |
|---|---|---|
| Faithfulness | 35 % | LLM Judge + NLI entailment |
| Relevance | 25 % | LLM Judge + G-Eval |
| Completeness | 15 % | LLM Judge + G-Eval |
| Coherence | 15 % | LLM Judge + G-Eval |
| Ethical Compliance | 10 % | LLM Judge |

All weights are configurable via `QualityControlPipeline::Config`.

### `QualityCheckResult` Key Fields

| Field | Type | Description |
|---|---|---|
| `status` | `QualityGateStatus` | Overall gate decision |
| `overall_score` | `double [0,1]` | Weighted aggregate across all evaluated dimensions |
| `confidence` | `double [0,1]` | Aggregate confidence of the evaluation methods |
| `dimension_scores` | `vector<DimensionScore>` | Per-dimension breakdown (score, confidence, method, explanation) |
| `citation_coverage` | `double [0,1]` | Fraction of answer sentences with ≥ 1 source citation (Stage 3) |
| `failure_reasons` | `vector<string>` | Human-readable failure explanations |
| `improvement_suggestions` | `vector<string>` | Suggestions emitted on ESCALATE |
| `should_retry` | `bool` | Set when `RETRY_NEEDED` |
| `sent_to_learning_system` | `bool` | Feedback delivery confirmation |

---

## Usage

### Quick Start

```cpp
#include "rag/quality_control_pipeline.h"

// Create production pipeline (all stages + learning feedback)
auto pipeline = QualityPipelineFactory::createProduction();

// Run quality control
auto result = pipeline->runQualityControl(query, answer, documents);

if (result.status == QualityGateStatus::PASSED) {
    // Answer passed all quality checks
}
```

### Custom Configuration

```cpp
QualityControlPipeline::Config config;
config.enable_fast_stage = true;
config.enable_balanced_stage = true;
config.enable_thorough_stage = true;
config.fast_stage_threshold = 0.6;
config.balanced_stage_threshold = 0.7;
config.thorough_stage_threshold = 0.8;
config.enable_citation_check = true;
config.enable_auto_retry = true;
config.max_retries = 2;
config.retry_threshold = 0.5;

auto pipeline = std::make_unique<QualityControlPipeline>(config);
```

### Factory Presets

```cpp
// Fast screening only — lowest latency
auto fast = QualityPipelineFactory::createFast();

// Fast + Balanced — standard production
auto balanced = QualityPipelineFactory::createBalanced();

// All stages — research / benchmarking
auto thorough = QualityPipelineFactory::createThorough();

// All stages + learning feedback — full production
auto production = QualityPipelineFactory::createProduction();
```

### Registering Callbacks

```cpp
pipeline->setFailureCallback([](const QualityCheckResult& r) {
    spdlog::warn("QC failed: {}", r.failure_reasons.front());
});

pipeline->setLearningCallback([](const std::string& query,
                                  const QualityCheckResult& r) {
    // Custom routing to external learning endpoint
});
```

---

## Configuration Reference

### Key `Config` Fields

| Field | Default | Description |
|---|---|---|
| `enable_fast_stage` | `true` | Enable Stage 1 |
| `enable_balanced_stage` | `true` | Enable Stage 2 |
| `enable_thorough_stage` | `true` | Enable Stage 3 |
| `enable_learning_feedback` | `true` | Enable Stage 4 async feedback |
| `fast_stage_threshold` | `0.6` | Minimum score to pass Stage 1 |
| `balanced_stage_threshold` | `0.7` | Minimum score to pass Stage 2 |
| `thorough_stage_threshold` | `0.8` | Minimum score to pass Stage 3 |
| `enable_citation_check` | `true` | Run `CitationHighlighter` in Stage 3 |
| `enable_auto_retry` | `true` | Retry generation on `RETRY_NEEDED` |
| `max_retries` | `2` | Maximum retry attempts |
| `retry_threshold` | `0.5` | Minimum score to allow a retry |
| `fast_stage_timeout_ms` | `50` | Hard timeout for Stage 1 |
| `balanced_stage_timeout_ms` | `500` | Hard timeout for Stage 2 |
| `thorough_stage_timeout_ms` | `2000` | Hard timeout for Stage 3 |
| `learning_orchestrator_url` | `""` | URL for `ContinuousLearningOrchestrator` endpoint |
| `enable_async_feedback` | `true` | Send feedback asynchronously |

JSON config example:
```json
{
  "quality_pipeline": {
    "enable_fast_stage": true,
    "enable_balanced_stage": true,
    "enable_thorough_stage": true,
    "fast_stage_threshold": 0.6,
    "balanced_stage_threshold": 0.7,
    "thorough_stage_threshold": 0.8,
    "enable_citation_check": true,
    "enable_auto_retry": true,
    "max_retries": 2
  }
}
```

---

## Integration Points

### With `InferenceEngineEnhanced`
- `LLMJudgeClient` routes evaluation prompts through `InferenceEngineEnhanced`
- Supports context caching (`enable_caching = true`) and batch processing
- Multi-model load balancing is configurable via `LLMJudgeClient::Config`

### With `ContinuousLearningOrchestrator`
- Stage 4 sends quality scores as async feedback
- Low-quality patterns are surfaced for `RLAIFTrainer` data selection
- `LearningMetrics` accumulates sliding-window accuracy/faithfulness/relevance trends

### With `RAGJudge`
- `RAGJudge` (multi-dimensional evaluator) is used inside Stage 2
- `FaithfulnessEvaluator` integrates `NLIFaithfulnessVerifier` for entailment-based claim checking
- `HallucinationDashboard` is updated after each Stage 3 result

### With `CalibrationManager`
- Raw LLM/G-Eval scores can be post-processed through `CalibrationManager` to reduce systematic bias
- Calibrated scores are used for threshold comparisons when calibration is enabled

---

## Performance Targets

| Mode | Target | Typical | Active components |
|---|---|---|---|
| Fast | < 50 ms | ~30 ms | LLM Judge (faithfulness only) |
| Balanced | < 500 ms | ~300 ms | LLM Judge (all dims) + G-Eval |
| Thorough | < 2 s | ~1.2 s | LLM Judge + G-Eval + NLI + CitationHighlighter |

---

## Dependencies

| Library | Purpose |
|---|---|
| `InferenceEngineEnhanced` | LLM calls for judge evaluation |
| `llama.cpp` | Token probability extraction for G-Eval |
| `nlohmann/json` | JSON config / response parsing |
| `spdlog` | Structured logging throughout the pipeline |
| ONNX Runtime (optional) | Real NLI model inference via `OnnxModelLoader` |

---

## Testing

```bash
# Unit tests for the QC pipeline
cmake --preset linux-release && cmake --build --preset linux-release --target test_quality_control_pipeline

# Demo / integration example
cmake --build --preset linux-release --target quality_control_demo
```

---

## Future Enhancements

1. **Advanced NLI Models** (open)
   - RoBERTa-large-MNLI and DeBERTa-v3 ONNX integration via `OnnxModelLoader`
   - Custom fine-tuned models for domain-specific entailment
   - See [`FUTURE_ENHANCEMENTS.md`](FUTURE_ENHANCEMENTS.md) for specification details

2. **Active Learning** (open)
   - Uncertain-case identification from low-confidence QC results
   - Human-in-the-loop feedback collection pipeline
   - See [`ROADMAP.md`](ROADMAP.md) for planned timeline

---

## Documentation

| Document | Purpose |
|---|---|
| [`docs/de/features/quality_control_pipeline.md`](../../docs/de/features/quality_control_pipeline.md) | User-facing feature guide with architecture diagrams and usage examples |
| [`include/rag/quality_control_pipeline.h`](../../include/rag/quality_control_pipeline.h) | Public API reference (Doxygen) |
| [`examples/quality_control_demo.cpp`](../../examples/quality_control_demo.cpp) | Runnable integration example |
| [`docs/reviews/REVIEW_SUMMARY_QC_PIPELINE.md`](../../docs/reviews/REVIEW_SUMMARY_QC_PIPELINE.md) | Production approval review (2026-02-19) |

---

## Review / Audit Trail

| Event | Date | Reference |
|---|---|---|
| Initial implementation review — approved for production | 2026-02-19 | [`docs/reviews/REVIEW_SUMMARY_QC_PIPELINE.md`](../../docs/reviews/REVIEW_SUMMARY_QC_PIPELINE.md) |
| Source-code audit (ThemisDB global) | 2026-04-06 | [`docs/de/development/SOURCE_CODE_AUDIT.md`](../../docs/de/development/SOURCE_CODE_AUDIT.md) |
| Documentation audit — QC README synchronised with module state | 2026-05-13 | This document (GitHub issue: `[Docs][Module] rag - QUALITY_CONTROL_README.md aktualisieren`) |

**Affected files reviewed in this audit:**
- `src/rag/QUALITY_CONTROL_README.md` (this file) — updated
- `include/rag/quality_control_pipeline.h` — verified current, no changes needed
- `src/rag/quality_control_pipeline.cpp` — verified current, no changes needed
- `src/rag/calibration_manager.cpp` — verified current (replaces "Future Enhancement" entry)
- `src/rag/citation_highlighter.cpp` — verified current (replaces "Future Enhancement" entry)
- `src/rag/judge_ensemble.cpp` — verified current (replaces "Future Enhancement" entry)

---

## License

See the root [`LICENSE`](../../LICENSE) file.

## Issue Scope Traceability

- Wave B tracking issue: `https://github.com/makr-code/ThemisDB/issues/5039`
- dependent Wave A issue: `https://github.com/makr-code/ThemisDB/issues/5038`
- follow-on Wave C issue: `https://github.com/makr-code/ThemisDB/issues/5040`
