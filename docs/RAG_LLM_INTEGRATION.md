# RAG Judge LLM Integration

This document describes the LLM integration for ThemisDB RAG Judge, connecting it to InferenceEngineEnhanced for high-performance evaluation.

## Overview

The RAG Judge LLM Integration provides:

1. **LLMJudgeClient** - High-level client for LLM inference
2. **NLIFaithfulnessVerifier** - NLI-based claim verification  
3. **QualityControlPipeline** - Multi-stage evaluation orchestration
4. **Integration with InferenceEngineEnhanced** - Production-ready LLM backend

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                   Quality Control Pipeline                  │
│  • Performance monitoring (<500ms target)                   │
│  • Quality checks and validation                            │
│  • Caching and batch processing                             │
└─────────────────────────────────────────────────────────────┘
                            │
        ┌───────────────────┼───────────────────┐
        │                   │                   │
        ▼                   ▼                   ▼
┌──────────────┐   ┌──────────────┐   ┌──────────────┐
│  RAG Judge   │   │ LLMJudgeClient│  │ NLI Verifier │
│              │   │              │   │              │
│ • Faith.     │   │ • Requests   │   │ • Entailment │
│ • Relevance  │   │ • Token probs│   │ • Batch API  │
│ • Complete.  │   │ • Caching    │   │ • Fallback   │
│ • Coherence  │   └──────────────┘   └──────────────┘
└──────────────┘          │
                          ▼
              ┌───────────────────────┐
              │ InferenceEngine       │
              │ Enhanced              │
              │                       │
              │ • Context caching     │
              │ • Batch processing    │
              │ • Load balancing      │
              └───────────────────────┘
```

## Components

### 1. LLMJudgeClient

High-level client wrapper for InferenceEngineEnhanced.

**Features:**
- Request management with timeouts and retries
- Token probability extraction for G-Eval
- Context caching for performance
- Multiple sample generation for self-consistency
- <500ms performance target

**Usage:**
```cpp
#include "rag/llm_judge_client.h"

// Create client with engine
auto engine = std::make_shared<InferenceEngineEnhanced>(config);
LLMJudgeClient client(engine);

// Generate evaluation
auto response = client.generate("Evaluate this answer...");

// Extract token probabilities
std::vector<std::string> score_tokens = {"1", "2", "3", "4", "5"};
auto probs = client.extractTokenProbabilities(prompt, score_tokens);
```

### 2. NLIFaithfulnessVerifier

NLI (Natural Language Inference) model for claim verification.

**Features:**
- Entailment, neutral, contradiction prediction
- Confidence scores for each prediction
- Batch verification API
- Heuristic fallback when model unavailable
- Result caching

**Usage:**
```cpp
#include "rag/nli_faithfulness_verifier.h"

NLIFaithfulnessVerifier verifier;

// Verify single claim
auto result = verifier.verify(
    "Paris is the capital of France.",  // premise
    "Paris is a capital city."          // hypothesis
);

if (result.prediction == NLIPrediction::ENTAILMENT) {
    std::cout << "Claim is entailed with confidence " 
              << result.confidence << std::endl;
}

// Batch verification
std::vector<std::pair<std::string, std::string>> pairs = {...};
auto results = verifier.verifyBatch(pairs);
```

### 3. QualityControlPipeline

Multi-stage evaluation orchestration with quality checks.

**Features:**
- Orchestrates all evaluators (faithfulness, relevance, completeness, coherence)
- Performance monitoring (<500ms target)
- Quality checks and validation
- Result caching
- Batch processing
- Adaptive sampling

**Usage:**
```cpp
#include "rag/quality_control_pipeline.h"

QualityControlPipeline::Config config;
config.max_evaluation_time_ms = 500.0;  // Target
config.min_confidence = 0.6;

QualityControlPipeline pipeline(config);

// Set components
pipeline.setLLMClient(llm_client);
pipeline.setNLIVerifier(nli_verifier);

// Evaluate
auto result = pipeline.evaluate(query, documents, answer);

// Check results
if (result.metrics.met_time_target && result.overall_quality_passed) {
    std::cout << "Score: " << result.evaluation.overall_score << std::endl;
    std::cout << "Time: " << result.metrics.total_time_ms << "ms" << std::endl;
}
```

## Integration with RAG Judge

The new components integrate seamlessly with existing RAG Judge:

```cpp
#include "rag/rag_judge.h"
#include "rag/llm_judge_client.h"
#include "llm/inference_engine_enhanced.h"

// 1. Set up engine
InferenceEngineEnhanced::Config engine_cfg;
auto engine = std::make_shared<InferenceEngineEnhanced>(engine_cfg);
engine->registerModel("llama2", model_plugin);
engine->start();

// 2. Create judge with LLM client
RAGJudgeConfig judge_cfg;
RAGJudge judge(judge_cfg);

// 3. The judge will automatically use LLMJudgeClient internally
auto result = judge.evaluate(query, documents, answer);
```

## Performance

### Targets

| Metric | Target | Status |
|--------|--------|--------|
| Evaluation Time | <500ms | ✓ Infrastructure ready |
| Test Coverage | >80% | ✓ 60+ tests |
| Cache Hit Rate | >80% | ✓ Caching enabled |
| Throughput | 2x with batching | ✓ Batch API ready |

### Optimization Tips

1. **Enable Caching**
   ```cpp
   client_config.enable_caching = true;
   pipeline_config.enable_result_caching = true;
   ```

2. **Use Batch Processing**
   ```cpp
   std::vector<std::tuple<...>> evaluations = {...};
   auto results = pipeline.evaluateBatch(evaluations);
   ```

3. **Prewarm Cache**
   ```cpp
   std::vector<std::string> common_prompts = {...};
   client->prewarmCache(common_prompts);
   ```

4. **Adaptive Sampling**
   ```cpp
   config.enable_adaptive_sampling = true;  // Reduce samples if time tight
   ```

## Testing

### Unit Tests

Run unit tests:
```bash
./build/tests/themis_test --gtest_filter="*LLMJudgeClient*"
./build/tests/themis_test --gtest_filter="*NLIVerifier*"
./build/tests/themis_test --gtest_filter="*QCPipeline*"
```

### Integration Example

Run the integration example:
```bash
./build/examples/rag_llm_integration_example
```

### Test Coverage

- **test_llm_judge_client.cpp**: 20+ tests
  - Basic operations (generate, batch, token probs)
  - Configuration and lifecycle
  - Performance (<500ms target)
  - Cache functionality

- **test_nli_verifier.cpp**: 18+ tests
  - Entailment detection
  - Contradiction detection
  - Batch processing
  - Cache and warmup

- **test_quality_control_pipeline.cpp**: 22+ tests
  - Full pipeline evaluation
  - Quality checks
  - Performance metrics
  - Component integration

## Future Enhancements

1. **Model Loading** (TODO #XXX)
   - Auto-load NLI models (RoBERTa-large-MNLI, DeBERTa)
   - GPU acceleration and quantization
   - Model download and caching

2. **Parallel Processing** (TODO #XXX)
   - Parallel evaluator execution
   - Parallel batch processing
   - Thread pool for concurrent evaluations

3. **Advanced Token Extraction** (TODO #XXX)
   - Direct logit extraction from InferenceResponse
   - Softmax computation
   - Token ID mapping

4. **Enhanced Metrics** (TODO #XXX)
   - Per-dimension timing
   - Cache hit/miss tracking
   - Throughput monitoring

## API Reference

See headers for full API documentation:
- `include/rag/llm_judge_client.h`
- `include/rag/nli_faithfulness_verifier.h`
- `include/rag/quality_control_pipeline.h`

## Examples

See `examples/rag_llm_integration_example.cpp` for a complete working example.

## Contributing

When contributing to these components:

1. Maintain the <500ms performance target
2. Add tests for new features (>80% coverage)
3. Update this README with new features
4. Run code review and security checks

## License

MIT License - see LICENSE file for details
