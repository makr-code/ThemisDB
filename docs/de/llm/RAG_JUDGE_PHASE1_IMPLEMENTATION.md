# RAG Judge Phase 1 - Implementation Guide

## Overview

This document describes the Phase 1 implementation of the LLM-as-Judge framework for RAG quality evaluation in ThemisDB.

## Architecture

### Components

1. **Configuration Management** (`judge_config.h/cpp`)
   - YAML/JSON configuration loading
   - Runtime configuration updates
   - Configuration validation
   - Dotted-key notation support

2. **Prompt Template Management** (`prompt_templates.h/cpp`)
   - Template-based prompt generation
   - Few-shot example management
   - Support for 4 evaluation dimensions
   - Placeholder replacement system

3. **Response Parsing** (`response_parser.h/cpp`)
   - JSON parsing with schema validation
   - Regex-based fallback parsing
   - Score normalization (1-5 to 0-1)
   - Explanation extraction

4. **LLM Integration** (`llm_judge_integration.h/cpp`)
   - Wrapper for LLM inference engine
   - Retry logic with exponential backoff
   - Error handling and logging
   - Dependency injection support

5. **RAG Judge Core** (`rag_judge.h/cpp`)
   - Main evaluation orchestration
   - Multi-mode operation (FAST/BALANCED/THOROUGH)
   - Caching for performance
   - Batch evaluation support

## Usage

### Basic Evaluation

```cpp
#include "rag/rag_judge.h"

using namespace themis::rag::judge;

// Create a balanced judge
auto judge = RAGJudgeFactory::createBalanced();

// Prepare input
std::string query = "What is the capital of France?";
std::vector<RetrievedDocument> docs = {
    {"doc1", "Paris is the capital of France.", 0.95, {}}
};
std::string answer = "The capital of France is Paris.";

// Evaluate
auto result = judge->evaluate(query, docs, answer);

// Check result
if (result.passed_quality_threshold) {
    std::cout << "Answer quality: " << result.overall_score << std::endl;
    std::cout << "Faithfulness: " << result.faithfulness_score << std::endl;
} else {
    std::cout << "Quality below threshold" << std::endl;
    for (const auto& claim : result.unverified_claims) {
        std::cout << "Unverified: " << claim << std::endl;
    }
}
```

### Configuration

```yaml
rag_judge:
  enabled: true
  mode: balanced  # fast, balanced, thorough
  
  llm:
    model: "llama-3-70b-instruct"
    temperature: 0.3
    max_tokens: 1024
  
  scoring:
    faithfulness_weight: 0.4
    relevance_weight: 0.3
    completeness_weight: 0.2
    coherence_weight: 0.1
  
  quality_threshold: 0.7
  faithfulness_threshold: 0.8
```

Load configuration:

```cpp
#include "rag/judge_config.h"

JudgeConfigManager config_mgr;
config_mgr.loadFromYAML("config/rag_judge.yaml");

// Access values
double threshold = config_mgr.getDouble("quality_threshold", 0.7);
std::string mode = config_mgr.getString("mode", "balanced");
```

### Custom Prompt Templates

```cpp
#include "rag/prompt_templates.h"

PromptTemplateManager template_mgr;

// Load custom templates
template_mgr.loadTemplatesFromDirectory("./prompts");

// Or set individual template
std::string custom_template = R"(
Evaluate faithfulness: {query}
Answer: {answer}
Context: {context}
)";
template_mgr.setTemplate(
    EvaluationDimension::FAITHFULNESS, 
    custom_template
);

// Add few-shot examples
FewShotExample example{
    "What is AI?",
    "AI is artificial intelligence.",
    "AI stands for artificial intelligence.",
    5.0,
    "Fully supported by context."
};
template_mgr.setFewShotExamples(
    EvaluationDimension::FAITHFULNESS,
    {example}
);
```

### Response Parsing

```cpp
#include "rag/response_parser.h"

std::string llm_response = R"({
    "score": 4.5,
    "confidence": 0.9,
    "reasoning": "Well supported answer",
    "supporting_claims": ["Paris is capital"],
    "unsupported_claims": []
})";

auto parsed = ResponseParser::parse(llm_response);

if (parsed.success && parsed.score) {
    double normalized = ResponseParser::normalizeScore(*parsed.score, 1.0, 5.0);
    std::cout << "Normalized score: " << normalized << std::endl;
}
```

### LLM Integration

```cpp
#include "rag/llm_judge_integration.h"

LLMJudgeIntegration::Config config;
config.model_name = "llama-3-70b";
config.temperature = 0.3;
config.max_retries = 3;

LLMJudgeIntegration integration(config);

// Set custom inference function for testing
integration.setInferenceFunction([](const std::string& prompt) {
    // Call your LLM here
    return llm_call(prompt);
});

// Use for evaluation
auto parsed = integration.evaluateWithLLM(
    EvaluationDimension::FAITHFULNESS,
    input,
    template_mgr
);
```

## Prompt Templates

### Faithfulness Template

Evaluates if the answer is supported by source documents:

- Identifies all claims in the answer
- Verifies each claim against context
- Rates on 1-5 scale with detailed reasoning
- Returns JSON with score, confidence, and supporting/unsupported claims

### Relevance Template

Evaluates if the answer addresses the query:

- Identifies key aspects of the query
- Checks coverage of each aspect
- Detects irrelevant information
- Returns JSON with score and aspect analysis

### Completeness Template

Evaluates comprehensiveness of the answer:

- Identifies all aspects that should be covered
- Assesses depth and breadth of coverage
- Notes missing information
- Returns JSON with covered/missing aspects

### Coherence Template

Evaluates logical structure and quality:

- Assesses logical flow and structure
- Checks internal consistency
- Evaluates clarity and readability
- Returns JSON with strengths/weaknesses

## Testing

### Running Tests

```bash
# Build tests
cmake --build build --target test_rag_judge_phase1

# Run tests
cd build && ctest -R RAGJudgePhase1Tests --output-on-failure

# Or run directly
./build/tests/test_rag_judge_phase1
```

### Test Coverage

The test suite includes:

- Configuration loading/validation (8 tests)
- Prompt template generation (7 tests)
- Response parsing (9 tests)
- LLM integration (2 tests)
- End-to-end evaluation (5 tests)
- Factory patterns (4 tests)

Total: 35+ unit and integration tests

## Performance

Target performance metrics:

- Config loading: < 10ms
- Prompt rendering: < 5ms
- Response parsing: < 20ms
- Total overhead (excluding LLM call): < 50ms

Cache hit rate: > 80% for repeated evaluations

## Future Work

Phase 2 will include:

- Multi-dimension evaluation with aggregation
- Claim verification with NLI models
- Citation checking
- Reverse question generation
- Aspect coverage analysis

Phase 3 will include:

- Pairwise comparison
- Judge ensemble with voting
- Position bias mitigation
- Calibration against human judgments

## References

- [G-Eval Paper](https://arxiv.org/abs/2303.16634)
- [MT-Bench](https://arxiv.org/abs/2306.05685)
- [RAGAS Framework](https://arxiv.org/abs/2309.15217)
- Issue: RAG-JUDGE-P1 LLM-as-Judge Phase 1

## Contributing

When extending this framework:

1. Add tests for new functionality
2. Update prompt templates with few-shot examples
3. Maintain backward compatibility in configuration
4. Follow existing code style and documentation standards
5. Run all tests before submitting PR

## License

MIT License - See LICENSE file in repository root.
