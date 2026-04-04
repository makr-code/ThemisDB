# LLMJudgeIntegration Configuration and Usage Guide

## Overview

The `LLMJudgeIntegration` class provides a flexible interface for integrating LLM inference into the RAG Judge evaluation system. It supports both production deployment with real LLM backends and testing with mock responses.

## Key Features

- **Configurable Mock Mode**: Explicitly enable/disable mock responses for testing
- **Custom Inference Functions**: Inject any LLM backend via dependency injection
- **Retry Logic**: Automatic retry with exponential backoff for transient failures
- **Warning System**: Rate-limited warnings when mock mode is active
- **Multiple Evaluation Dimensions**: Support for faithfulness, relevance, completeness, and coherence

## Configuration Options

### Config Structure

```cpp
struct Config {
    std::string model_name = "default";
    double temperature = 0.3;
    int max_tokens = 1024;
    int max_retries = 3;
    int timeout_ms = 30000;
    bool use_json_mode = true;
    
    // Mock mode configuration
    bool use_mock_mode = false;           // Enable mock responses (for testing only)
    bool warn_on_mock_mode = true;        // Log warning once when mock mode is used
};
```

### Configuration Parameters

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `model_name` | string | "default" | Name of the LLM model to use |
| `temperature` | double | 0.3 | Sampling temperature (0.0-1.0) |
| `max_tokens` | int | 1024 | Maximum tokens in response |
| `max_retries` | int | 3 | Number of retry attempts on failure |
| `timeout_ms` | int | 30000 | Timeout in milliseconds |
| `use_json_mode` | bool | true | Request JSON-formatted responses |
| `use_mock_mode` | bool | false | Enable mock responses (testing only) |
| `warn_on_mock_mode` | bool | true | Log warning when mock mode active |

## Usage Patterns

### 1. Production Deployment with Real LLM

#### Option A: Using EmbeddedLLM

```cpp
#include "rag/llm_judge_integration.h"
#include "llm/embedded_llm.h"

// Initialize LLM Judge Integration
LLMJudgeIntegration::Config config;
config.use_mock_mode = false;  // Production mode
config.temperature = 0.3;
config.max_tokens = 1024;

LLMJudgeIntegration integration(config);

// Get EmbeddedLLM instance
auto& llm = EmbeddedLLMManager::instance().get();

// Set inference function
integration.setInferenceFunction([&llm](const std::string& prompt) {
    return llm.generate(prompt, 1024);
});

// Use for evaluation
PromptTemplateManager template_mgr = PromptTemplateManager::createDefault();
EvaluationInput input;
input.query = "What is quantum computing?";
input.generated_answer = "Quantum computing uses quantum mechanics...";

auto result = integration.evaluateWithLLM(
    EvaluationDimension::FAITHFULNESS,
    input,
    template_mgr
);
```

#### Option B: Using Custom LLM Backend

```cpp
// Custom LLM backend (e.g., OpenAI, Ollama)
class MyLLMBackend {
public:
    std::string infer(const std::string& prompt) {
        // Your LLM inference implementation
        // Should return JSON response
        return callMyLLMAPI(prompt);
    }
};

// Setup integration
LLMJudgeIntegration integration;
MyLLMBackend backend;

integration.setInferenceFunction([&backend](const std::string& prompt) {
    return backend.infer(prompt);
});
```

#### Option C: Using OpenAI API

```cpp
#include <curl/curl.h>
#include <nlohmann/json.hpp>

std::string openai_inference(const std::string& prompt) {
    // Call OpenAI API
    json request = {
        {"model", "gpt-4"},
        {"messages", {{{"role", "user"}, {"content", prompt}}}},
        {"temperature", 0.3}
    };
    
    // Make HTTP request to OpenAI
    std::string response = makeOpenAIRequest(request);
    
    // Parse and return in expected format
    return formatResponse(response);
}

integration.setInferenceFunction(openai_inference);
```

### 2. Testing with Mock Mode

```cpp
#include "rag/llm_judge_integration.h"
#include <gtest/gtest.h>

TEST(RAGJudgeTest, EvaluationTest) {
    // Enable mock mode for testing
    LLMJudgeIntegration::Config config;
    config.use_mock_mode = true;
    config.warn_on_mock_mode = false;  // Optional: disable warnings in tests
    
    LLMJudgeIntegration integration(config);
    
    // No need to set inference function - uses mock responses
    // Mock responses return score: 4.0, confidence: 0.85
    
    EvaluationInput input;
    input.query = "Test query";
    input.generated_answer = "Test answer";
    
    PromptTemplateManager template_mgr = PromptTemplateManager::createDefault();
    
    auto result = integration.evaluateWithLLM(
        EvaluationDimension::FAITHFULNESS,
        input,
        template_mgr
    );
    
    ASSERT_TRUE(result.success);
    EXPECT_DOUBLE_EQ(*result.score, 4.0);
}
```

### 3. Testing with Custom Mock Responses

```cpp
TEST(RAGJudgeTest, CustomMockTest) {
    LLMJudgeIntegration integration;
    
    // Set custom mock inference function
    integration.setInferenceFunction([](const std::string& prompt) {
        return R"({
            "score": 5.0,
            "confidence": 0.99,
            "reasoning": "Custom test response",
            "supporting_claims": ["Claim 1", "Claim 2"],
            "unsupported_claims": []
        })";
    });
    
    // Test with custom mock
    auto result = integration.evaluateWithLLM(...);
    EXPECT_DOUBLE_EQ(*result.score, 5.0);
}
```

## API Reference

### Constructor

```cpp
explicit LLMJudgeIntegration(const Config& config = {});
```

Creates a new LLMJudgeIntegration instance with the given configuration.

- **Parameters**: `config` - Configuration options
- **Behavior**:
  - If `config.use_mock_mode == true`: Uses built-in mock responses
  - If `config.use_mock_mode == false`: Requires `setInferenceFunction()` to be called

### Methods

#### setInferenceFunction()

```cpp
void setInferenceFunction(std::function<std::string(const std::string&)> fn);
```

Sets the LLM inference function to use for evaluations.

- **Parameters**: `fn` - Function that takes a prompt and returns JSON response
- **Required**: Must be called before evaluation unless mock mode is enabled
- **Thread Safety**: Not thread-safe, call before concurrent evaluations

#### evaluateWithLLM()

```cpp
ParsedResponse evaluateWithLLM(
    EvaluationDimension dimension,
    const EvaluationInput& input,
    const PromptTemplateManager& template_mgr
);
```

Evaluates an input for a specific dimension using LLM inference.

- **Parameters**:
  - `dimension` - Evaluation dimension (FAITHFULNESS, RELEVANCE, COMPLETENESS, COHERENCE)
  - `input` - Input containing query, answer, and documents
  - `template_mgr` - Prompt template manager for generating prompts
- **Returns**: ParsedResponse with score, confidence, reasoning, and claims
- **Throws**: `std::runtime_error` if inference function not set (in production mode)

#### isMockMode()

```cpp
bool isMockMode() const;
```

Checks if the integration is currently in mock mode.

- **Returns**: `true` if using mock responses, `false` otherwise
- **Use Case**: Runtime checks, conditional logic, testing

#### setConfig() / getConfig()

```cpp
void setConfig(const Config& config);
Config getConfig() const;
```

Updates or retrieves the current configuration.

- **Note**: Changing config at runtime may not affect already-set inference function

## Response Format

The inference function must return a JSON string with the following structure:

```json
{
    "score": 4.5,
    "confidence": 0.92,
    "reasoning": "Explanation of the evaluation",
    "supporting_claims": [
        "Claim supported by documents",
        "Another supported claim"
    ],
    "unsupported_claims": [
        "Claim not found in documents"
    ]
}
```

### Response Fields

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `score` | number | Yes | Evaluation score (typically 0.0-5.0) |
| `confidence` | number | Yes | Confidence level (0.0-1.0) |
| `reasoning` | string | Yes | Explanation for the score |
| `supporting_claims` | array[string] | Yes | Claims supported by documents |
| `unsupported_claims` | array[string] | Yes | Claims not supported by documents |

## Error Handling

### No Inference Function Set

```cpp
// This will throw std::runtime_error
LLMJudgeIntegration integration;  // use_mock_mode = false by default
auto result = integration.evaluateWithLLM(...);  // ERROR!

// Error message: "No inference function set. Call setInferenceFunction() 
// with a valid LLM inference function, or enable mock mode 
// (config.use_mock_mode = true) for testing."
```

### Inference Function Failures

The integration includes retry logic with exponential backoff:

```cpp
LLMJudgeIntegration::Config config;
config.max_retries = 3;  // Retry up to 3 times

integration.setInferenceFunction([](const std::string& prompt) {
    // If this throws, will retry automatically
    throw std::runtime_error("Transient error");
});

// Retries: attempt 1 (0ms), attempt 2 (+100ms), attempt 3 (+200ms)
auto result = integration.evaluateWithLLM(...);

if (!result.success) {
    // All retries failed
    std::cerr << "Error: " << result.error_message << std::endl;
}
```

## Performance Considerations

### Mock Mode Performance

Mock mode is optimized for testing with minimal overhead:

- **Evaluation time**: < 10ms per evaluation
- **Memory overhead**: Negligible (static JSON string)
- **Throughput**: > 100 evaluations/second

### Production Performance

Performance depends on the LLM backend:

| Backend | Typical Latency | Throughput |
|---------|----------------|------------|
| Local llama.cpp | 50-500ms | 2-20 eval/s |
| OpenAI API | 500-2000ms | 0.5-2 eval/s |
| Ollama Local | 100-1000ms | 1-10 eval/s |
| Custom API | Varies | Varies |

### Optimization Tips

1. **Batch Processing**: Evaluate multiple inputs in parallel when possible
2. **Caching**: Cache evaluation results for identical inputs
3. **Async Inference**: Use async I/O for API calls
4. **Token Limits**: Reduce `max_tokens` for faster responses
5. **Temperature**: Use lower temperature (0.1-0.3) for consistent, faster results

## Migration Guide

### From Old Hardcoded Mock Mode

**Before (v1.4.0 and earlier):**
```cpp
// Always used mock responses
LLMJudgeIntegration integration;
// Evaluations returned hardcoded score: 4.0
```

**After (v1.4.1+):**
```cpp
// For testing: Explicitly enable mock mode
LLMJudgeIntegration::Config config;
config.use_mock_mode = true;
LLMJudgeIntegration integration(config);

// For production: Set real inference function
LLMJudgeIntegration integration;
integration.setInferenceFunction([](const std::string& prompt) {
    return myLLM.infer(prompt);
});
```

### Updating Existing Tests

Tests that already call `setInferenceFunction()` require no changes:

```cpp
// This code works unchanged
TEST_F(MyTest, EvaluationTest) {
    LLMJudgeIntegration integration;
    
    integration.setInferenceFunction([](const std::string& prompt) {
        return mockResponse();
    });
    
    // Test continues as before...
}
```

## Troubleshooting

### Issue: "No inference function set" error

**Cause**: Attempting to evaluate without setting an inference function in production mode.

**Solution**: Either:
1. Call `setInferenceFunction()` with a valid function
2. Enable mock mode: `config.use_mock_mode = true`

### Issue: Mock mode warnings in production

**Cause**: `use_mock_mode = true` in production deployment.

**Solution**: Set `config.use_mock_mode = false` and provide real inference function.

### Issue: High latency in production

**Cause**: Slow LLM backend or network issues.

**Solutions**:
- Use local LLM (llama.cpp) instead of cloud API
- Reduce `max_tokens` for faster responses
- Implement caching for repeated queries
- Use lower temperature for faster inference

### Issue: Evaluation failures

**Cause**: Inference function throwing exceptions or returning invalid JSON.

**Solutions**:
- Check inference function error handling
- Validate JSON response format
- Increase `max_retries` for transient failures
- Add logging in inference function

## Best Practices

1. **Always set inference function in production**: Don't rely on mock mode
2. **Use mock mode only for testing**: Never deploy with `use_mock_mode = true`
3. **Handle errors gracefully**: Check `result.success` before using scores
4. **Log evaluation results**: Track scores and confidence for debugging
5. **Monitor performance**: Track evaluation latency and throughput
6. **Version control**: Document which LLM model/version is used
7. **Test with real LLM**: Run integration tests with actual inference before deployment

## Examples

See the following files for complete examples:

- **Unit Tests**: `tests/test_llm_judge_integration.cpp`
- **Benchmarks**: `benchmarks/bench_llm_judge_integration.cpp`
- **Integration Tests**: `tests/test_rag_judge_phase1.cpp`
- **Production Usage**: `src/rag/rag_judge.cpp`

## Support

For issues, questions, or feature requests related to LLMJudgeIntegration:

1. Check this documentation first
2. Review example code in test files
3. Consult the RAG Judge documentation
4. Open an issue on GitHub with:
   - Your configuration
   - Error messages
   - Steps to reproduce
   - Expected vs actual behavior
