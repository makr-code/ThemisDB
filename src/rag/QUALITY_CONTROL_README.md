# ThemisDB Quality Control System

Complete post-generation quality control for RAG (Retrieval-Augmented Generation) outputs with multi-stage evaluation, automatic retry logic, and continuous learning integration.

## Overview

The Quality Control System provides comprehensive evaluation of RAG-generated answers through multiple specialized components:

- **LLM-as-Judge Integration**: Structured evaluation using LLM inference
- **G-Eval Scoring**: Token probability-based continuous scoring (0-1 range)
- **NLI Verification**: ONNX-based Natural Language Inference for claim verification
- **Quality Control Pipeline**: Multi-stage orchestration with Fast/Balanced/Thorough modes

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                  Quality Control Pipeline                    │
├─────────────────────────────────────────────────────────────┤
│  Fast Mode         Balanced Mode        Thorough Mode       │
│  (<50ms)           (<500ms)             (<2s)               │
│  ────────          ──────────           ─────────           │
│  • NLI check       • Multi-dimension    • Full LLM judge    │
│                    • Selective NLI      • G-Eval scoring    │
│                                         • Comprehensive NLI  │
└─────────────────────────────────────────────────────────────┘
                            │
        ┌───────────────────┼───────────────────┐
        │                   │                   │
        ▼                   ▼                   ▼
┌──────────────┐  ┌──────────────────┐  ┌─────────────────┐
│ LLM Judge    │  │ G-Eval           │  │ NLI Verifier    │
│ Client       │  │ Evaluator        │  │                 │
├──────────────┤  ├──────────────────┤  ├─────────────────┤
│• Inference   │  │• Token probs     │  │• ONNX Runtime   │
│• Structured  │  │• Continuous      │  │• DeBERTa-v3     │
│  responses   │  │  scoring         │  │• Claim-doc      │
│• Retries     │  │• Multi-sample    │  │  entailment     │
└──────────────┘  └──────────────────┘  └─────────────────┘
```

## Components

### 1. LLM Judge Client

Connects RAG Judge to InferenceEngineEnhanced for structured evaluations.

**Features:**
- Automatic retry with exponential backoff
- Response parsing (JSON and text-based)
- Score extraction and normalization
- Confidence computation
- Caching support

**Usage:**
```cpp
#include "rag/llm_judge_client.h"

// Create client
LLMJudgeClient::Config config;
config.model_id = "default";
config.temperature = 0.3;
config.enable_caching = true;

auto client = std::make_shared<LLMJudgeClient>(
    config, 
    inference_engine
);

// Evaluate
auto response = client->evaluate(prompt, EvaluationDimension::FAITHFULNESS);

// Check results
if (response.success) {
    std::cout << "Score: " << response.score << "\n";
    std::cout << "Confidence: " << response.confidence << "\n";
    std::cout << "Reasoning: " << response.reasoning << "\n";
}
```

### 2. G-Eval Evaluator

Token probability-based continuous scoring for fine-grained evaluation.

**Features:**
- Continuous scores (0-1) instead of discrete levels
- Token probability extraction from LLM logits
- Multiple sample aggregation (mean/median/mode)
- Entropy-based confidence computation

**Formula:**
```
score = Σ(level × P(level)) for levels 1-5
normalized_score = (score - 1) / 4
```

**Usage:**
```cpp
#include "rag/geval_evaluator.h"

GEvalEvaluator::Config config;
config.num_samples = 3;
config.aggregation = AggregationMethod::MEAN;

GEvalEvaluator evaluator(config);

auto result = evaluator.evaluate(
    query,
    answer,
    documents,
    "faithfulness"
);

std::cout << "G-Eval Score: " << result.geval_score << "\n";
std::cout << "Confidence: " << result.confidence << "\n";
std::cout << "Variance: " << result.variance << "\n";
```

### 3. NLI Faithfulness Verifier

ONNX-based Natural Language Inference for accurate claim verification.

**Features:**
- Fast claim-document entailment checking (<50ms per claim)
- Batch processing support
- Multi-document verification
- Caching for repeated claims
- Heuristic fallback when ONNX model not available

**Supported Models:**
- microsoft/deberta-v3-large-mnli
- roberta-large-mnli
- bart-large-mnli

**Usage:**
```cpp
#include "rag/nli_faithfulness_verifier.h"

NLIFaithfulnessVerifier::Config config;
config.model_path = "path/to/deberta-v3-large-mnli.onnx";
config.entailment_threshold = 0.7;

NLIFaithfulnessVerifier verifier(config);

// Verify single claim
auto result = verifier.verifyClaim(claim, document);

if (result.label == NLILabel::ENTAILMENT) {
    std::cout << "Claim supported with " 
              << (result.entailment_prob * 100) << "% confidence\n";
}

// Batch verify
auto results = verifier.verifyClaimsBatch(claims, document);

// Verify against multiple documents
auto best_result = verifier.verifyAgainstMultipleDocs(claim, documents);
```

### 4. Quality Control Pipeline

Multi-stage orchestration with automatic mode selection and retry logic.

**Modes:**

| Mode | Target Latency | Features |
|------|---------------|----------|
| **Fast** | <50ms | NLI-based faithfulness check only |
| **Balanced** | <500ms | Multi-dimension evaluation + selective NLI |
| **Thorough** | <2s | Full evaluation with G-Eval + comprehensive NLI |

**Usage:**
```cpp
#include "rag/quality_control_pipeline.h"

// Create pipeline
QualityControlPipeline::Config config;
config.default_mode = QCMode::BALANCED;
config.accept_threshold = 0.75;
config.enable_retry = true;
config.max_retries = 2;

QualityControlPipeline pipeline(config);

// Run quality control
auto result = pipeline.runQualityControl(
    query,
    documents,
    generated_answer
);

// Check decision
switch (result.decision) {
    case QCDecision::ACCEPT:
        std::cout << "Quality passed!\n";
        break;
    case QCDecision::REJECT:
        std::cout << "Quality too low, reject\n";
        break;
    case QCDecision::RETRY:
        std::cout << "Quality marginal, retry recommended\n";
        break;
    case QCDecision::WARN:
        std::cout << "Quality acceptable with warnings\n";
        break;
}

// View scores
std::cout << "Overall: " << result.overall_score << "\n";
std::cout << "Faithfulness: " << result.faithfulness_score << "\n";
std::cout << "Relevance: " << result.relevance_score << "\n";
```

## Integration with RAG Judge

The Quality Control components can be integrated with the existing RAG Judge:

```cpp
#include "rag/rag_judge.h"

// Configure RAG Judge with QC features
RAGJudgeConfig config;
config.use_nli_verifier = true;             // Enable NLI verification
config.use_geval_scoring = false;            // G-Eval (optional, adds latency)
config.use_quality_control_pipeline = false; // Full pipeline (alternative to basic judge)

RAGJudge judge(config);

// Evaluate as usual
auto result = judge.evaluate(query, documents, answer);
```

## Performance Targets

| Component | Target | Actual (Stub) |
|-----------|--------|---------------|
| NLI per claim | <50ms | ~1ms (heuristic) |
| Fast mode | <50ms | ~10ms |
| Balanced mode | <500ms | ~50ms |
| Thorough mode | <2s | ~200ms |

*Note: Stub implementations use heuristics and are faster than production ONNX/LLM inference.*

## Continuous Learning Integration

Quality metrics are automatically logged to the continuous learning orchestrator (when enabled):

```cpp
QualityControlPipeline::Config config;
config.log_to_continuous_learning = true;
config.cl_endpoint = "http://localhost:8080/metrics";

QualityControlPipeline pipeline(config);
```

**Logged Metrics:**
- Dimension scores (faithfulness, relevance, completeness, coherence)
- Overall quality score
- Decision (accept/reject/retry/warn)
- Latency per stage
- NLI and G-Eval detailed results

**Triggers:**
- **Low faithfulness** → Retrieval optimization
- **Low relevance** → Prompt optimization
- **Consistent quality issues** → LoRA fine-tuning
- **High latency** → Model/pipeline optimization

## Testing

Comprehensive test suite with 118 test cases:

```bash
# Run all quality control tests
./build/tests/test_geval                    # 46 tests
./build/tests/test_nli_verifier             # 42 tests
./build/tests/test_quality_control_pipeline # 30 tests
```

**Test Coverage:**
- ✅ Constructor and configuration
- ✅ Basic evaluation functionality
- ✅ Edge cases (empty inputs, special characters, long texts)
- ✅ Performance benchmarks
- ✅ Caching and statistics
- ✅ Batch processing
- ✅ Error handling

## Example

See `examples/quality_control_demo.cpp` for 8 comprehensive usage scenarios:

```bash
./build/examples/quality_control_demo
```

**Examples Include:**
1. Basic Quality Control
2. Different QC Modes
3. Adaptive QC with Time Budget
4. Batch Quality Control
5. Custom Configuration
6. Callback for Monitoring
7. Statistics and Monitoring
8. Factory Methods

## Future Enhancements

### Phase 1 (Current)
- ✅ LLM Judge Client integration
- ✅ G-Eval token probability scoring (stub)
- ✅ NLI verification (heuristic fallback)
- ✅ Quality Control Pipeline
- ✅ Continuous learning hooks

### Phase 2 (Production)
- [ ] Real ONNX Runtime integration for NLI
- [ ] Token probability extraction from llama.cpp
- [ ] GPU acceleration for NLI inference
- [ ] Advanced retry strategies
- [ ] A/B testing integration

### Phase 3 (Advanced)
- [ ] Multi-model NLI ensemble
- [ ] Adaptive threshold tuning
- [ ] Real-time quality monitoring dashboard
- [ ] Explainable AI for decisions
- [ ] Cross-lingual quality control

## References

- **G-Eval Paper**: Liu et al., 2023 - https://arxiv.org/abs/2303.16634
- **NLI Models**: HuggingFace microsoft/deberta-v3-large-mnli
- **RAG Judge**: `src/rag/rag_judge.cpp`
- **Inference Engine**: `include/llm/inference_engine_enhanced.h`

## License

Part of ThemisDB - see main LICENSE file.
