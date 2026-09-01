# LLMJudgeIntegration Configuration and Usage Guide

## Overview

`LLMJudgeIntegration` integrates a real LLM backend into the RAG judge pipeline.
Mock-mode runtime fallback has been removed. If no backend is configured, the class now
fails closed with explicit `llm_unavailable` results.

## Key Features

- **Constructor injection** via `ILLMInferenceEngine*`
- **Function injection** via `setInferenceFunction()`
- **Retry logic** with exponential backoff for transient failures
- **Fail-closed unavailable behavior** when the gate is disabled or no backend is present
- **Multiple evaluation dimensions**: faithfulness, relevance, completeness, coherence

## Configuration Options

```cpp
struct Config {
    std::string model_name = "default";
    double temperature = 0.3;
    int max_tokens = 1024;
    int max_retries = 3;
    int timeout_ms = 30000;
    bool use_json_mode = true;
    bool enable_llm_judge = kDefaultJudgeEnabled;
};
```

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `model_name` | string | `"default"` | Name of the LLM model to use |
| `temperature` | double | `0.3` | Sampling temperature |
| `max_tokens` | int | `1024` | Maximum tokens in response |
| `max_retries` | int | `3` | Retry attempts on backend failure |
| `timeout_ms` | int | `30000` | Timeout budget in milliseconds |
| `use_json_mode` | bool | `true` | Request JSON-formatted responses |
| `enable_llm_judge` | bool | build-dependent | Enable real backend dispatch under `THEMIS_ENABLE_LLM_JUDGE` |

## Usage Patterns

### 1. Production constructor injection

```cpp
struct MyEngine : ILLMInferenceEngine {
    std::string generate(const std::string& prompt) override {
        return myBackend.infer(prompt);
    }
};

MyEngine engine;
LLMJudgeIntegration::Config config;
config.temperature = 0.3;
config.max_tokens = 1024;

LLMJudgeIntegration integration(&engine, config);
```

### 2. Production or test function injection

```cpp
LLMJudgeIntegration integration;
integration.setInferenceFunction([](const std::string& prompt) {
    return myBackend.infer(prompt);
});
```

### 3. Deterministic tests without built-in mock mode

```cpp
TEST(RAGJudgeTest, EvaluationTest) {
    LLMJudgeIntegration integration;
    integration.setInferenceFunction([](const std::string&) {
        return R"({"score":4.0,"confidence":0.9,"reasoning":"test"})";
    });

    auto result = integration.evaluateWithLLM(
        EvaluationDimension::RELEVANCE,
        test_input,
        template_mgr);

    EXPECT_TRUE(result.success);
}
```

## API Reference

### Constructors

```cpp
explicit LLMJudgeIntegration(ILLMInferenceEngine* engine);
explicit LLMJudgeIntegration(ILLMInferenceEngine* engine, const Config& config);
LLMJudgeIntegration();
explicit LLMJudgeIntegration(const Config& config);
```

- Engine constructors require a non-null backend and throw `std::invalid_argument` otherwise.
- Config-only constructors are allowed, but callers must later provide a backend through
  `setInferenceFunction()` before successful evaluation.

### `setInferenceFunction()`

```cpp
void setInferenceFunction(std::function<std::string(const std::string&)> fn);
```

Sets the LLM inference function used for evaluations.

### `evaluateWithLLM()`

```cpp
ParsedResponse evaluateWithLLM(
    EvaluationDimension dimension,
    const EvaluationInput& input,
    const PromptTemplateManager& template_mgr);
```

Returns a parsed judge response. If the gate is disabled or no backend is configured,
this returns `success=false` with `reasoning="llm_unavailable"`.

### `evaluateDimension()`

Returns raw backend JSON. If the gate is disabled or backend calls fail, it returns an
explicit unavailable JSON payload instead of an empty object or mock score.

### `isMockMode()`

```cpp
bool isMockMode() const;
```

Compatibility probe retained for older callers. It now always returns `false`.

## Error Handling

### No backend configured

```cpp
LLMJudgeIntegration integration;
auto result = integration.evaluateWithLLM(...);
// result.success == false
// result.reasoning == "llm_unavailable"
```

### Backend failures

Retry logic remains active. After retries are exhausted, the integration returns an
explicit unavailable result instead of fabricating a synthetic judge score.

## Migration Notes

### Old mock mode

The old built-in mock fallback has been removed. Existing tests or tools that depended
on it must inject deterministic responses through `setInferenceFunction()` or a small
`ILLMInferenceEngine` test double.

### Best Practices

1. Inject a real backend in production.
2. Use deterministic injected test doubles in tests.
3. Check `result.success` before consuming scores.
4. Log backend failures and unavailable returns for operator visibility.
