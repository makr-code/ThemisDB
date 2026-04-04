# Quality Control System - Quick Reference

## Quick Start (3 Steps)

### 1. Include Headers
```cpp
#include "rag/quality_control_factory.h"
```

### 2. Create Pipeline
```cpp
auto pipeline = QualityControlFactory::createBasic();
```

### 3. Run Quality Check
```cpp
auto result = pipeline->runQualityControl(query, documents, answer);

if (result.decision == QCDecision::ACCEPT) {
    // Answer passed quality checks
}
```

## Common Use Cases

### Real-Time RAG (< 50ms)
```cpp
auto pipeline = QualityControlFactory::createLightweight();
auto result = pipeline->runQualityControl(query, docs, answer);
```

### Production RAG (< 500ms)
```cpp
QualityControlFactory::SetupConfig config;
config.nli_model_path = "/models/deberta-v3-large-mnli.onnx";
config.inference_engine = my_inference_engine;

auto pipeline = QualityControlFactory::createProduction(config);
auto result = pipeline->runQualityControl(query, docs, answer);
```

### Batch Processing (< 2s per item)
```cpp
auto pipeline = QualityControlFactory::createComprehensive();

for (const auto& item : batch) {
    auto result = pipeline->runQualityControl(
        item.query, item.docs, item.answer
    );
    process_result(result);
}
```

### With Continuous Learning
```cpp
QualityControlFactory::SetupConfig config;
config.enable_continuous_learning = true;
config.cl_endpoint = "http://localhost:8080/metrics";

auto pipeline = QualityControlFactory::createProduction(config);

// Automatic optimization triggers
// - low_faithfulness → optimize retrieval
// - low_relevance → optimize prompts
// - low_overall_quality → trigger LoRA fine-tuning
```

## Decision Types

| Decision | Meaning | Action |
|----------|---------|--------|
| `ACCEPT` | Quality passed | Return answer to user |
| `REJECT` | Quality failed | Use fallback or error |
| `RETRY` | Fixable issues | Regenerate with adjustments |
| `WARN` | Borderline | Accept but log warning |

## Score Dimensions

```cpp
result.faithfulness_score;   // Answer grounded in docs (0-1)
result.relevance_score;      // Answer addresses query (0-1)
result.completeness_score;   // Answer is comprehensive (0-1)
result.coherence_score;      // Answer is well-structured (0-1)
result.overall_score;        // Combined score (0-1)
```

## Modes Comparison

| Mode | Latency | Components | Use Case |
|------|---------|------------|----------|
| **FAST** | <50ms | NLI only | Real-time chat |
| **BALANCED** | <500ms | Multi-dimension + selective NLI | Production API |
| **THOROUGH** | <2s | Full evaluation + G-Eval | Batch processing |

## Configuration Quick Ref

```cpp
QualityControlPipeline::Config config;

// Mode
config.mode = QCMode::BALANCED;  // FAST, BALANCED, THOROUGH

// Thresholds (0-1)
config.faithfulness_threshold = 0.75;
config.relevance_threshold = 0.70;
config.completeness_threshold = 0.70;
config.coherence_threshold = 0.65;
config.overall_threshold = 0.70;

// Retry
config.enable_retry = true;
config.max_retries = 2;

// Continuous Learning
config.log_to_continuous_learning = true;
```

## Callbacks

```cpp
// Monitor each dimension
config.on_dimension_evaluated = [](const std::string& dim, double score) {
    std::cout << dim << ": " << score << "\n";
};

// Track completion
config.on_quality_check_complete = [](const QualityCheckResult& result) {
    metrics_server->record(result);
};
```

## Continuous Learning

```cpp
auto cl_client = std::make_shared<ContinuousLearningClient>();

// Custom trigger handling
cl_client->setTriggerCallback([](const OptimizationTrigger& trigger) {
    if (trigger.trigger_type == "low_faithfulness") {
        retrieval_optimizer->run();
    } else if (trigger.trigger_type == "low_relevance") {
        prompt_optimizer->run();
    } else {
        lora_trainer->start();
    }
});

// Manual logging
cl_client->logQCResult(result);

// Check triggers
if (auto trigger = cl_client->checkTriggers()) {
    std::cout << "Optimize: " << trigger->recommendation << "\n";
}
```

## Error Handling

```cpp
try {
    auto result = pipeline->runQualityControl(query, docs, answer);
    
    switch (result.decision) {
        case QCDecision::ACCEPT:
            return answer;
        
        case QCDecision::RETRY:
            // Adjust parameters and retry
            return regenerate_with_adjustments(query, docs);
        
        case QCDecision::REJECT:
            // Log and use fallback
            logger->error("Quality check failed", result.explanation);
            return fallback_answer(query);
        
        case QCDecision::WARN:
            // Log warning but accept
            logger->warn("Quality borderline", result.explanation);
            return answer;
    }
} catch (const std::exception& e) {
    logger->error("Quality control error", e.what());
    return fallback_answer(query);
}
```

## Performance Tips

### 1. Use Appropriate Mode
- **Real-time**: Use FAST mode
- **Production**: Use BALANCED mode
- **Batch**: Use THOROUGH mode

### 2. Adjust Thresholds
```cpp
// More lenient (fewer rejections)
config.overall_threshold = 0.60;

// More strict (higher quality)
config.overall_threshold = 0.80;
```

### 3. Enable Caching
```cpp
config.enable_cache = true;
config.cache_size = 1000;
```

### 4. Batch Processing
```cpp
// Process in parallel
#pragma omp parallel for
for (size_t i = 0; i < items.size(); i++) {
    results[i] = pipeline->runQualityControl(
        items[i].query, items[i].docs, items[i].answer
    );
}
```

## Troubleshooting

### High Latency?
- Switch to FAST mode
- Increase timeout thresholds
- Enable caching

### Too Many Rejections?
- Lower thresholds
- Check document quality
- Verify retrieval system

### Triggers Firing Too Often?
- Increase window size
- Adjust trigger thresholds
- Check data distribution

## Examples

Run built-in examples:
```bash
# Complete demo (8 scenarios)
./build/examples/quality_control_demo

# CL integration (5 scenarios)
./build/examples/continuous_learning_integration_example

# Quick start (4 scenarios)
./build/examples/simple_qc_integration_example
```

## Tests

Run tests to verify:
```bash
# Pipeline tests (30 test cases)
./build/tests/test_quality_control_pipeline

# CL client tests (24 test cases)
./build/tests/test_continuous_learning_client
```

## Full Documentation

See `docs/QUALITY_CONTROL_SYSTEM.md` for complete documentation including:
- Architecture details
- Component descriptions
- API reference
- Integration patterns
- Configuration guide
- Troubleshooting

## Support

For issues or questions:
1. Check `docs/QUALITY_CONTROL_SYSTEM.md`
2. Run verification: `bash scripts/verify_quality_control_build.sh`
3. Review examples in `examples/` directory
4. Check test files for usage patterns
