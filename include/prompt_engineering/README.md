> **Build:** `cmake --preset release && cmake --build build/release`

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Prompt Engineering Module

The `prompt_engineering` module provides systematic prompt lifecycle management for ThemisDB LLM integrations.

## Headers

| Header | Key Symbols | Description |
|---|---|---|
| `chain_of_thought.h` | `ChainOfThoughtBuilder` | Structured chain-of-thought prompt construction |
| `context_window_manager.h` | `ContextWindowBudgetManager`, `ITokenCounter`, `CharDivisionCounter`, `ModelTokenBudget`, `BudgetAllocation`, `PromptBudgetExceededError` | Token budget allocation per model |
| `cot_tracer.h` | `CotTracer` <!-- TODO: verify --> | Traces chain-of-thought reasoning steps for debugging and analysis |
| `dspy_module.h` | `DspyModule` <!-- TODO: verify --> | DSPy-style declarative prompt module integration |
| `feedback_collector.h` | `FeedbackCollector` | Human/automatic feedback collection |
| `llm_reflection_adapter.h` | `LLMReflectionAdapter` <!-- TODO: verify --> | Adapter bridging LLM backends with the reflection/self-improvement pipeline |
| `meta_prompt_generator.h` | `MetaPromptGenerator` | Meta-prompt generation for self-improvement |
| `prompt_ab_experiment.h` | `PromptAbExperiment` <!-- TODO: verify --> | A/B experiment definitions for prompt variant comparison |
| `prompt_compressor.h` | `PromptCompressor` <!-- TODO: verify --> | Lossy and lossless prompt compression to fit context windows |
| `prompt_engineering_integration.h` | `PromptEngineeringIntegration` | Full pipeline integration facade |
| `prompt_engineering_metrics.h` | `PromptEngineeringMetrics` | 4 reflection counters + Prometheus export |
| `prompt_evaluator.h` | `PromptEvaluator` | Automated prompt quality evaluation |
| `prompt_injection_detector.h` | `PromptInjectionDetector` | Injection detection and blocking |
| `prompt_library_io.h` | `PromptLibraryIO` <!-- TODO: verify --> | Serialisation and deserialisation of prompt libraries (JSON/YAML) |
| `prompt_manager.h` | `PromptManager` | Template registry |
| `prompt_optimizer.h` | `PromptOptimizer` | A/B-testing-driven optimization |
| `prompt_performance_tracker.h` | `PromptPerformanceTracker` | Latency, cost, quality tracking |
| `prompt_regression_runner.h` | `PromptRegressionRunner` <!-- TODO: verify --> | Runs regression suites against prompt versions to detect quality degradation |
| `prompt_template_compiler.h` | `PromptTemplateCompiler` <!-- TODO: verify --> | Compiles Jinja-style prompt templates to executable prompt objects |
| `prompt_template_validator.h` | `PromptTemplateValidator` | Schema validation |
| `prompt_version_control.h` | `PromptVersionControl` | Git-like versioning (branch/tag/diff/merge) |
| `protegi_optimizer.h` | `ProtegiOptimizer` <!-- TODO: verify --> | ProTeGi (automatic prompt optimization via gradient-free search) |
| `rag_prompt_builder.h` | `RAGPromptBuilder` | RAG-augmented prompt construction |
| `reflection_tuner.h` | `ReflectionTuner` <!-- TODO: verify --> | Fine-tunes prompts based on self-reflection feedback signals |
| `self_improvement_orchestrator.h` | `ReflectionTuner`, `ILLMProviderReflectionAdapter` | SELF_REFINE / REFLEXION / CONSTITUTIONAL / SOCRATIC |
| `structured_output.h` | `StructuredOutputFormatter` <!-- TODO: verify --> | Enforces JSON / XML / typed structured outputs from LLM responses |
| `system_prompt_manager.h` | `SystemPromptManager` | System prompt lifecycle and persona management |
| `tree_of_thoughts.h` | `TreeOfThoughtsBuilder` <!-- TODO: verify --> | Tree-of-Thoughts multi-path reasoning prompt construction |

## Links

- Implementation: [`../../src/prompt_engineering/`](../../src/prompt_engineering/)

## Installation

This module is included as part of ThemisDB. Add the module headers to your include path:

```cmake
target_include_directories(your_target PRIVATE ${THEMISDB_INCLUDE_DIR})
```
