# Quality Control System - Usage Guide

## Overview

ThemisDB's Quality Control (QC) System provides comprehensive post-generation evaluation for RAG outputs with three modes optimized for different latency/thoroughness trade-offs.

## Quick Start

### 1. Enable Quality Control in Build

Quality control components are built automatically when `THEMIS_ENABLE_LLM` is enabled:

```bash
cmake --preset linux-release -DTHEMIS_ENABLE_LLM=ON
cmake --build --preset linux-release
```

### 2. Basic Usage

```cpp
#include "rag/quality_control_pipeline.h"

using namespace themis::rag::judge;

// Create pipeline with default configuration
QualityControlPipeline pipeline;

// Prepare RAG output data
std::string query = "What is the capital of France?";
std::vector<RetrievedDocument> documents = {
    {"doc1", "Paris is the capital of France.", 0.95, {}}
};
std::string answer = "The capital of France is Paris.";

// Run quality control
auto result = pipeline.runQualityControl(query, documents, answer);

// Check result
if (result.decision == QCDecision::ACCEPT) {
    std::cout << "Quality passed! Score: " << result.overall_score << "\n";
} else {
    std::cout << "Quality check failed. Score: " << result.overall_score << "\n";
}
```

## Configuration Options

### Quality Control Modes

Choose mode based on your latency requirements:

| Mode | Target Latency | Features | Use Case |
|------|---------------|----------|----------|
| **Fast** | <50ms | NLI-only faithfulness | Real-time responses |
| **Balanced** | <500ms | Multi-dimension + selective NLI | Standard production |
| **Thorough** | <2s | Full eval + G-Eval + comprehensive NLI | Batch processing, critical content |

```cpp
// Fast mode - minimal latency
auto result = pipeline.runQualityControl(query, docs, answer, QCMode::FAST);

// Balanced mode - good trade-off (default)
auto result = pipeline.runQualityControl(query, docs, answer, QCMode::BALANCED);

// Thorough mode - maximum accuracy
auto result = pipeline.runQualityControl(query, docs, answer, QCMode::THOROUGH);
```

### Adaptive Mode Selection

Let the system choose the best mode based on time budget:

```cpp
// Automatically select mode based on 500ms budget
auto result = pipeline.runAdaptiveQC(query, documents, answer, 500);
```

### Custom Configuration

```cpp
QualityControlPipeline::Config config;

// Quality thresholds
config.accept_threshold = 0.80;      // Accept if score >= 0.80
config.reject_threshold = 0.50;      // Reject if score < 0.50
config.warn_threshold = 0.65;        // Warn if score < 0.65

// Retry configuration
config.enable_retry = true;
config.max_retries = 2;
config.retry_improvement_threshold = 0.05;

// Feature toggles
config.enable_nli_verification = true;
config.enable_geval_scoring = false;  // Optional, adds latency
config.enable_claim_extraction = true;

// Continuous learning
config.log_to_continuous_learning = true;

QualityControlPipeline pipeline(config);
```

## Integration with RAG Judge

### Option 1: Direct Integration

Enable quality control features in RAG Judge:

```cpp
#include "rag/rag_judge.h"

RAGJudgeConfig config;
config.use_nli_verifier = true;        // Enable NLI verification
config.use_geval_scoring = false;      // Optional G-Eval
config.use_quality_control_pipeline = false;  // Use basic judge

RAGJudge judge(config);
auto result = judge.evaluate(query, documents, answer);
```

### Option 2: Standalone Pipeline

Use the quality control pipeline independently:

```cpp
#include "rag/quality_control_pipeline.h"

auto pipeline = QualityControlPipelineFactory::createBalanced();
auto result = pipeline->runQualityControl(query, documents, answer);
```

## Component Usage

### LLM Judge Client

For structured LLM-based evaluation:

```cpp
#include "rag/llm_judge_client.h"

LLMJudgeClient::Config config;
config.model_id = "default";
config.temperature = 0.3;
config.enable_caching = true;

auto client = std::make_shared<LLMJudgeClient>(
    config, 
    inference_engine  // Your InferenceEngineEnhanced instance
);

auto response = client->evaluate(prompt, EvaluationDimension::FAITHFULNESS);
std::cout << "Score: " << response.score << "\n";
std::cout << "Confidence: " << response.confidence << "\n";
```

### G-Eval Evaluator

For token probability-based continuous scoring:

```cpp
#include "rag/geval_evaluator.h"

GEvalEvaluator::Config config;
config.num_samples = 3;
config.aggregation = AggregationMethod::MEAN;

GEvalEvaluator evaluator(config);

std::vector<std::pair<std::string, std::string>> docs = {
    {"doc1", "Document content..."}
};

auto result = evaluator.evaluate(query, answer, docs, "faithfulness");
std::cout << "G-Eval Score: " << result.geval_score << "\n";
std::cout << "Confidence: " << result.confidence << "\n";
```

### NLI Faithfulness Verifier

For fast claim verification:

```cpp
#include "rag/nli_faithfulness_verifier.h"

NLIFaithfulnessVerifier::Config config;
config.model_path = "path/to/deberta-v3-large-mnli.onnx";  // Optional
config.entailment_threshold = 0.7;

NLIFaithfulnessVerifier verifier(config);

// Verify single claim
auto result = verifier.verifyClaim(
    "Paris is the capital of France",
    "Paris is the capital city of France."
);

if (result.label == NLILabel::ENTAILMENT) {
    std::cout << "Claim supported with " 
              << (result.entailment_prob * 100) << "% confidence\n";
}

// Batch verification
std::vector<std::string> claims = {
    "Paris is the capital",
    "France is in Europe"
};
auto results = verifier.verifyClaimsBatch(claims, document);
```

## Batch Processing

Process multiple RAG outputs efficiently:

```cpp
std::vector<EvaluationInput> inputs;

for (auto& rag_output : outputs) {
    EvaluationInput input;
    input.query = rag_output.query;
    input.documents = rag_output.documents;
    input.generated_answer = rag_output.answer;
    inputs.push_back(input);
}

auto results = pipeline.batchQualityControl(inputs, QCMode::FAST);

for (const auto& result : results) {
    std::cout << "Score: " << result.overall_score << "\n";
}
```

## Monitoring and Callbacks

Set up callbacks for real-time monitoring:

```cpp
QualityControlPipeline pipeline;

pipeline.setQCCallback([](const QCResult& result) {
    // Log to monitoring system
    logger.log("QC Score", result.overall_score);
    logger.log("Decision", static_cast<int>(result.decision));
    
    // Alert on low quality
    if (result.decision == QCDecision::REJECT) {
        alerting_system.send_alert("Low quality RAG output detected");
    }
});

// Quality checks will now trigger callback
auto result = pipeline.runQualityControl(query, documents, answer);
```

## Statistics and Performance

Track quality control performance:

```cpp
// Run several evaluations
for (int i = 0; i < 100; i++) {
    pipeline.runQualityControl(queries[i], docs[i], answers[i]);
}

// Get statistics
auto stats = pipeline.getStatistics();

std::cout << "Total Evaluations: " << stats.total_evaluations << "\n";
std::cout << "Accepted: " << stats.accepted << "\n";
std::cout << "Rejected: " << stats.rejected << "\n";
std::cout << "Average Score: " << stats.avg_score << "\n";
std::cout << "Average Latency: " << stats.avg_latency_ms << " ms\n";
std::cout << "Fast Mode Usage: " << stats.mode_usage[QCMode::FAST] << "\n";
```

## Continuous Learning Integration

Enable automatic metric logging for continuous learning:

```cpp
QualityControlPipeline::Config config;
config.log_to_continuous_learning = true;
config.cl_endpoint = "http://continuous-learning-service:8080/metrics";

QualityControlPipeline pipeline(config);

// Quality metrics will be automatically logged:
// - Faithfulness, relevance, completeness, coherence scores
// - Decision (accept/reject/retry/warn)
// - Latency per stage
// - NLI and G-Eval detailed results

// This triggers:
// - Prompt optimization (low relevance)
// - Retrieval optimization (low faithfulness)
// - LoRA fine-tuning (consistent quality issues)
```

## Decision Handling

Handle different quality control decisions:

```cpp
auto result = pipeline.runQualityControl(query, documents, answer);

switch (result.decision) {
    case QCDecision::ACCEPT:
        // Quality passed - use the answer
        return answer;
        
    case QCDecision::REJECT:
        // Quality too low - regenerate or use fallback
        return regenerate_answer(query);
        
    case QCDecision::RETRY:
        // Quality marginal - retry with different parameters
        return retry_with_better_retrieval(query);
        
    case QCDecision::WARN:
        // Quality acceptable but has issues
        log_warning("Answer quality borderline");
        return answer;  // Still use it
}
```

## Performance Targets

Current stub implementations are faster than production. Expected production performance:

| Component | Target | Stub (Current) | Production (Est.) |
|-----------|--------|----------------|-------------------|
| NLI per claim | <50ms | ~1ms | ~30ms |
| Fast mode | <50ms | ~10ms | ~40ms |
| Balanced mode | <500ms | ~50ms | ~400ms |
| Thorough mode | <2s | ~200ms | ~1.5s |

## Production Deployment

### Prerequisites

1. **ONNX Runtime** (for NLI verification)
   ```bash
   vcpkg install onnxruntime
   ```

2. **NLI Model** (DeBERTa-v3-large-mnli)
   - Download from HuggingFace
   - Convert to ONNX format
   - Place in models directory

3. **LLM Inference** (InferenceEngineEnhanced)
   - Must be initialized and available
   - Configure model paths

### Configuration Example

```cpp
// Production configuration
NLIFaithfulnessVerifier::Config nli_config;
nli_config.model_path = "/models/deberta-v3-large-mnli.onnx";
nli_config.tokenizer_path = "/models/deberta-v3-large-mnli-tokenizer.json";
nli_config.use_gpu = true;  // Enable GPU acceleration

auto nli_verifier = std::make_shared<NLIFaithfulnessVerifier>(nli_config);

// Pipeline with real components
QualityControlPipeline::Config pipeline_config;
pipeline_config.enable_nli_verification = true;
pipeline_config.enable_geval_scoring = true;

auto pipeline = std::make_shared<QualityControlPipeline>(
    pipeline_config,
    llm_judge_client,  // Connected to InferenceEngineEnhanced
    geval_evaluator,
    nli_verifier
);
```

## Troubleshooting

### Common Issues

**Issue: Build fails with missing headers**
```
Solution: Ensure THEMIS_ENABLE_LLM=ON and all dependencies installed
```

**Issue: Tests fail with "NLI model not found"**
```
Solution: This is expected with stub implementation. Tests use heuristic fallback.
```

**Issue: Low performance/high latency**
```
Solution: 
- Use Fast mode for real-time
- Enable caching
- Consider GPU acceleration for NLI
```

**Issue: Quality scores seem random**
```
Solution: Stub implementations use heuristics. Production requires:
- Real ONNX NLI model
- Connected InferenceEngineEnhanced
- Token probability extraction from llama.cpp
```

## Examples

See `examples/quality_control_demo.cpp` for 8 comprehensive usage scenarios:

1. Basic quality control
2. Different QC modes
3. Adaptive QC with time budget
4. Batch quality control
5. Custom configuration
6. Callback monitoring
7. Statistics tracking
8. Factory methods

Build and run:
```bash
cd examples
g++ -std=c++20 quality_control_demo.cpp -o qc_demo \
    -I../include -L../build/lib -lthemis
./qc_demo
```

## API Reference

See detailed API documentation:
- `src/rag/QUALITY_CONTROL_README.md` - Architecture and component details
- `include/rag/quality_control_pipeline.h` - Pipeline API
- `include/rag/llm_judge_client.h` - LLM Judge Client API
- `include/rag/geval_evaluator.h` - G-Eval API
- `include/rag/nli_faithfulness_verifier.h` - NLI Verifier API

## Next Steps

1. **Try the demo**: Build and run `quality_control_demo.cpp`
2. **Integrate into your RAG pipeline**: Add QC after answer generation
3. **Monitor quality metrics**: Use callbacks and statistics
4. **Tune thresholds**: Adjust based on your quality requirements
5. **Enable continuous learning**: Connect to optimization triggers

## Support

For issues or questions:
- Check `IMPLEMENTATION_COMPLETE.md` for implementation details
- Review test files for usage examples
- See `src/rag/QUALITY_CONTROL_README.md` for architecture

## License

Part of ThemisDB - see main LICENSE file.
