# Quality Control System Documentation

## Overview

The ThemisDB Quality Control System provides comprehensive post-generation quality assessment for RAG (Retrieval-Augmented Generation) outputs. It implements multiple evaluation dimensions including faithfulness, relevance, completeness, and coherence.

## Architecture

### Core Components

```
┌─────────────────────────────────────────────────────────────┐
│                Quality Control Pipeline                      │
│  ┌──────────────────────────────────────────────────────┐  │
│  │  Fast Mode (<50ms)     │  Balanced │  Thorough (<2s) │  │
│  └──────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
           ▼                    ▼                    ▼
    ┌──────────────┐   ┌──────────────┐   ┌──────────────┐
    │ LLM Judge    │   │   G-Eval     │   │ NLI Verifier │
    │   Client     │   │  Evaluator   │   │  (DeBERTa)   │
    └──────────────┘   └──────────────┘   └──────────────┘
           │                    │                    │
           └────────────────────┴────────────────────┘
                              ▼
                ┌──────────────────────────────┐
                │ Continuous Learning Client    │
                │  (Metric logging & triggers)  │
                └──────────────────────────────┘
```

### 1. Quality Control Pipeline

**File**: `include/rag/quality_control_pipeline.h`

The main orchestrator that coordinates quality assessment across multiple dimensions.

**Features**:
- Three evaluation modes: Fast, Balanced, Thorough
- Multi-dimensional scoring (faithfulness, relevance, completeness, coherence)
- Automatic retry logic
- Statistics tracking
- Continuous learning integration

**Usage**:
```cpp
#include "rag/quality_control_pipeline.h"

QualityControlPipeline::Config config;
config.mode = QCMode::BALANCED;
config.faithfulness_threshold = 0.75;
config.enable_retry = true;

QualityControlPipeline pipeline(config);

auto result = pipeline.runQualityControl(
    query,           // User query
    documents,       // Retrieved documents
    generated_answer // Generated answer
);

if (result.decision == QCDecision::ACCEPT) {
    // Answer passed quality checks
    std::cout << "Overall score: " << result.overall_score << "\n";
}
```

### 2. LLM Judge Client

**File**: `include/rag/llm_judge_client.h`

Connects LLM-as-Judge evaluation to the InferenceEngineEnhanced for structured quality assessment.

**Features**:
- Structured evaluation prompts
- Response parsing (JSON + text fallback)
- Automatic retry with exponential backoff
- Score normalization
- Confidence computation

**Evaluation Dimensions**:
- `FAITHFULNESS` - Answer is grounded in documents
- `RELEVANCE` - Answer addresses the query
- `COMPLETENESS` - Answer is comprehensive
- `COHERENCE` - Answer is well-structured

**Usage**:
```cpp
auto llm_client = std::make_shared<LLMJudgeClient>(inference_engine);

LLMJudgeClient::EvaluationRequest request;
request.dimension = EvaluationDimension::FAITHFULNESS;
request.query = "What is the capital of France?";
request.context = "France is a country in Europe. Paris is its capital.";
request.answer = "The capital of France is Paris.";

auto response = llm_client->evaluate(request);
// response.score: 0.95, response.confidence: 0.92
```

### 3. G-Eval Evaluator

**File**: `include/rag/geval_evaluator.h`

Implements G-Eval token probability-based scoring for continuous quality assessment.

**Features**:
- Token probability extraction
- Multi-sample aggregation (mean/median/mode)
- Entropy-based confidence
- Variance tracking

**How it works**:
```
Traditional: Sample 5 times → [4, 5, 3, 5, 4] → Average: 4.2/5
G-Eval: P(1)=0.01, P(2)=0.04, P(3)=0.10, P(4)=0.35, P(5)=0.50
        Score = Σ(level × P(level)) = 1×0.01 + 2×0.04 + ... + 5×0.50 = 4.29
```

**Usage**:
```cpp
auto geval = std::make_shared<GEvalEvaluator>(inference_engine);

GEvalEvaluator::GEvalRequest request;
request.dimension = EvaluationDimension::COHERENCE;
request.text = generated_answer;
request.criteria = "Is the answer well-structured?";

auto result = geval->evaluate(request);
// result.score: continuous 0-1, result.confidence: entropy-based
```

### 4. NLI Faithfulness Verifier

**File**: `include/rag/nli_faithfulness_verifier.h`

ONNX-based Natural Language Inference for claim verification.

**Features**:
- ONNX model loading (DeBERTa-v3-large-mnli)
- Fast claim verification (<50ms per claim)
- Batch processing support
- Multi-document verification
- Cache optimization

**Usage**:
```cpp
NLIFaithfulnessVerifier::Config config;
config.model_path = "/models/deberta-v3-large-mnli.onnx";
config.threshold = 0.80;

auto nli_verifier = std::make_shared<NLIFaithfulnessVerifier>(config);

// Single claim verification
auto result = nli_verifier->verifyClaim(
    "Paris is the capital of France",  // claim
    documents                          // supporting documents
);
// result.entailment_prob: 0.95 = fully supported

// Batch verification
auto batch_results = nli_verifier->verifyClaimsBatch(claims, documents);
```

### 5. Continuous Learning Client

**File**: `include/rag/continuous_learning_client.h`

Logs quality metrics and detects when optimization is needed.

**Features**:
- Metric aggregation with sliding window
- Async batching (configurable size/timeout)
- Thread-safe metric collection
- Optimization trigger detection
- Custom trigger callbacks

**Optimization Triggers**:

| Trigger | Threshold | Recommended Action |
|---------|-----------|-------------------|
| `low_faithfulness` | <0.75 | Optimize retrieval system |
| `low_relevance` | <0.70 | Optimize prompts |
| `low_overall_quality` | <0.70 | Trigger LoRA fine-tuning |

**Usage**:
```cpp
ContinuousLearningClient::Config config;
config.window_size = 100;
config.batch_size = 10;
config.endpoint = "http://localhost:8080/metrics";

auto cl_client = std::make_shared<ContinuousLearningClient>(config);

// Set up trigger callback
cl_client->setTriggerCallback([](const OptimizationTrigger& trigger) {
    if (trigger.trigger_type == "low_faithfulness") {
        // Call retrieval optimizer
        retrieval_optimizer->optimize();
    } else if (trigger.trigger_type == "low_relevance") {
        // Call prompt optimizer
        prompt_optimizer->optimize();
    } else {
        // Trigger LoRA fine-tuning
        lora_trainer->start();
    }
});

// Log metrics (done automatically by pipeline)
cl_client->logQCResult(qc_result);

// Manual trigger check
if (auto trigger = cl_client->checkTriggers()) {
    std::cout << "Optimization needed: " << trigger->recommendation << "\n";
}
```

### 6. Quality Control Factory

**File**: `include/rag/quality_control_factory.h`

Provides pre-configured pipeline creation with smart defaults.

**Factory Methods**:

#### `createBasic()`
For testing/development - uses heuristic fallbacks
- No models required
- Fast setup
- Good for integration testing

#### `createProduction(SetupConfig)`
Full production setup with models
- Requires NLI model path
- Requires inference engine
- All features enabled

#### `createLightweight()`
Real-time use cases (<50ms)
- Fast mode only
- Minimal latency
- Basic checks

#### `createComprehensive()`
Batch processing (<2s)
- Thorough mode
- Maximum accuracy
- All dimensions

**Usage**:
```cpp
// Quick start - no models needed
auto pipeline = QualityControlFactory::createBasic();

// Production setup
QualityControlFactory::SetupConfig config;
config.nli_model_path = "/models/deberta-v3-large-mnli.onnx";
config.inference_engine = my_inference_engine;
config.enable_continuous_learning = true;
config.cl_endpoint = "http://localhost:8080/metrics";

auto prod_pipeline = QualityControlFactory::createProduction(config);
```

## Integration Patterns

### Pattern 1: Simple Integration

```cpp
#include "rag/quality_control_factory.h"

// Create pipeline
auto pipeline = QualityControlFactory::createBasic();

// Run quality control
auto result = pipeline->runQualityControl(query, documents, answer);

// Check result
if (result.decision == QCDecision::ACCEPT) {
    return answer;  // Pass to user
} else if (result.decision == QCDecision::RETRY) {
    // Regenerate with different parameters
    return regenerate_answer(query, documents);
} else {
    // Reject - log and fallback
    log_quality_failure(result);
    return fallback_answer();
}
```

### Pattern 2: With Callbacks

```cpp
QualityControlPipeline::Config config;
config.mode = QCMode::BALANCED;

// Set up callbacks for real-time monitoring
config.on_dimension_evaluated = [](const std::string& dimension, double score) {
    metrics_server->recordScore(dimension, score);
};

config.on_quality_check_complete = [](const QualityCheckResult& result) {
    dashboard->updateQualityMetrics(result);
};

QualityControlPipeline pipeline(config);
auto result = pipeline.runQualityControl(query, documents, answer);
```

### Pattern 3: Batch Processing

```cpp
auto pipeline = QualityControlFactory::createComprehensive();

std::vector<QualityCheckResult> results;
for (const auto& item : batch) {
    auto result = pipeline->runQualityControl(
        item.query,
        item.documents,
        item.answer
    );
    results.push_back(result);
}

// Analyze batch results
auto avg_score = std::accumulate(results.begin(), results.end(), 0.0,
    [](double sum, const auto& r) { return sum + r.overall_score; }
) / results.size();
```

### Pattern 4: Integration with RAG Judge

```cpp
#include "rag/rag_judge.h"
#include "rag/quality_control_factory.h"

// Create components
auto rag_judge = std::make_shared<RAGJudge>(config);
auto qc_pipeline = QualityControlFactory::createProduction(setup_config);

// Evaluate with both
auto rag_result = rag_judge->evaluate(query, documents, answer);
auto qc_result = qc_pipeline->runQualityControl(query, documents, answer);

// Combine scores
double combined_score = (rag_result.score + qc_result.overall_score) / 2.0;
```

## Building

### Prerequisites

- CMake 3.23+
- C++20 compiler (GCC 10+, Clang 12+, MSVC 2019+)
- vcpkg (for dependencies)

### Configure

```bash
# Linux
cmake --preset linux-ninja-release -DTHEMIS_ENABLE_LLM=ON

# Windows
cmake --preset msvc-ninja-release
```

### Build

```bash
# Build all
cmake --build --preset linux-ninja-release

# Build specific target
cmake --build --preset linux-ninja-release --target themis_core
```

### Run Tests

```bash
# All tests
ctest --preset linux-ninja-release

# Specific test
./build-linux-ninja-release/tests/test_quality_control_pipeline
```

## Performance Targets

| Component | Target | Current (Stub) | Production (Est.) |
|-----------|--------|----------------|-------------------|
| NLI per claim | <50ms | ~1ms | ~30ms |
| Fast mode | <50ms | ~10ms | ~40ms |
| Balanced mode | <500ms | ~50ms | ~400ms |
| Thorough mode | <2s | ~200ms | ~1.5s |

## Configuration Reference

### QualityControlPipeline::Config

```cpp
struct Config {
    QCMode mode = QCMode::BALANCED;  // FAST, BALANCED, THOROUGH
    
    // Thresholds (0-1)
    double faithfulness_threshold = 0.75;
    double relevance_threshold = 0.70;
    double completeness_threshold = 0.70;
    double coherence_threshold = 0.65;
    double overall_threshold = 0.70;
    
    // Retry settings
    bool enable_retry = true;
    int max_retries = 2;
    RetryStrategy retry_strategy = RetryStrategy::ADAPTIVE;
    
    // Component configuration
    std::shared_ptr<LLMJudgeClient> llm_judge;
    std::shared_ptr<GEvalEvaluator> geval_evaluator;
    std::shared_ptr<NLIFaithfulnessVerifier> nli_verifier;
    
    // Continuous learning
    bool log_to_continuous_learning = false;
    std::shared_ptr<ContinuousLearningClient> cl_client;
    
    // Callbacks
    std::function<void(const std::string&, double)> on_dimension_evaluated;
    std::function<void(const QualityCheckResult&)> on_quality_check_complete;
};
```

### ContinuousLearningClient::Config

```cpp
struct Config {
    size_t window_size = 100;       // Sliding window size
    size_t batch_size = 10;         // Metrics per batch
    int batch_timeout_ms = 5000;    // Batch timeout
    std::string endpoint;           // Optional HTTP endpoint
    
    // Trigger thresholds
    double faithfulness_trigger = 0.75;
    double relevance_trigger = 0.70;
    double overall_trigger = 0.70;
};
```

## Troubleshooting

### Issue: High latency in Balanced/Thorough mode

**Solution**: Use Fast mode for real-time applications or increase timeout thresholds.

```cpp
config.mode = QCMode::FAST;
config.timeout_ms = 100;  // Increase if needed
```

### Issue: NLI model not found

**Solution**: Download DeBERTa-v3-large-mnli ONNX model:

```bash
# Download from HuggingFace
wget https://huggingface.co/microsoft/deberta-v3-large-mnli/resolve/main/model.onnx

# Or use heuristic fallback (development)
config.use_heuristic_fallback = true;
```

### Issue: Triggers firing too frequently

**Solution**: Adjust trigger thresholds or increase window size:

```cpp
cl_config.window_size = 200;  // More samples before trigger
cl_config.faithfulness_trigger = 0.70;  // Lower threshold
```

### Issue: Memory usage growing

**Solution**: Adjust sliding window size and batch frequency:

```cpp
cl_config.window_size = 50;    // Smaller window
cl_config.batch_size = 5;      // More frequent flushes
cl_config.batch_timeout_ms = 2000;  // Faster timeout
```

## Examples

See the `examples/` directory for complete examples:

1. **`quality_control_demo.cpp`** - Complete demonstration (8 scenarios)
2. **`continuous_learning_integration_example.cpp`** - CL integration (5 scenarios)
3. **`simple_qc_integration_example.cpp`** - Quick start (4 scenarios)

## API Reference

### QualityCheckResult

```cpp
struct QualityCheckResult {
    QCDecision decision;           // ACCEPT, REJECT, RETRY, WARN
    double overall_score;          // 0-1
    
    // Dimension scores
    double faithfulness_score;
    double relevance_score;
    double completeness_score;
    double coherence_score;
    
    // Metadata
    std::string explanation;
    std::vector<std::string> issues;
    std::vector<std::string> recommendations;
    
    // Performance
    double latency_ms;
    QCMode mode_used;
};
```

### OptimizationTrigger

```cpp
struct OptimizationTrigger {
    std::string trigger_type;      // "low_faithfulness", "low_relevance", etc.
    double current_avg;            // Current average score
    double threshold;              // Threshold that was crossed
    size_t sample_count;           // Samples in window
    std::string recommendation;    // What to optimize
    std::chrono::system_clock::time_point timestamp;
};
```

## Future Enhancements

### Phase 4: Production Integration
- HTTP client for real metric endpoints
- Connection to SelfImprovementOrchestrator
- Integration with PromptOptimizer
- LoRA fine-tuning triggers

### Phase 5: Advanced Features
- Multi-model NLI ensemble
- Adaptive threshold tuning
- Real-time quality dashboard
- Cross-lingual support
- Explainability features

## References

- **G-Eval Paper**: https://arxiv.org/abs/2303.16634
- **NLI Models**: HuggingFace microsoft/deberta-v3-large-mnli
- **RAG Evaluation**: Best practices and benchmarks
- **Continuous Learning**: Automatic optimization strategies

## License

Copyright © 2026 ThemisDB. All rights reserved.
