> **Build:** `cmake --preset release && cmake --build build/release`

<!-- Status: current | validated: 2026-05-13 -->
<!-- Links: ../../src/prompt_engineering/README.md · ../../src/prompt_engineering/ROADMAP.md · ../../src/prompt_engineering/FUTURE_ENHANCEMENTS.md · ../../docs/de/prompt_engineering/README.md -->

# Prompt Engineering Module (Public Headers)

Public API surface for ThemisDB prompt lifecycle management, optimization, safety checks, and integration helpers.

## Header Entry Points

| Header | Key Public API | Description |
|---|---|---|
| `prompt_engineering_integration.h` | `PromptEngineeringIntegration`, `IntegrationConfig`, `ExecutionContext` | High-level façade and runtime integration contract |
| `prompt_manager.h` | `PromptManager`, `PromptTemplate`, `ValidationResult`, `ImageDescription` | Template registry, validation, context injection, optional multimodal metadata |
| `feedback_collector.h` | `FeedbackCollector`, `FeedbackEntry`, `FeedbackStats`, `FeedbackType` | Feedback ingestion, aggregation, and failure-pattern analysis |
| `prompt_evaluator.h` | `PromptEvaluator`, `IEmbeddingProvider`, `EvaluatorConfig` | Quality scoring and statistical significance checks |
| `prompt_optimizer.h` | `PromptOptimizer`, `OptimizationConfig`, `OptimizerResult` | Iterative prompt optimization workflow |
| `meta_prompt_generator.h` | `MetaPromptGenerator`, `ILLMProvider`, `MetaPromptConfig` | Meta-prompt generation and LLM-provider extension point |
| `self_improvement_orchestrator.h` | `SelfImprovementOrchestrator`, `ImprovementConfig`, `OptimizationResult` | Automated optimization orchestration and A/B handoff |
| `prompt_version_control.h` | `PromptVersionControl`, `PromptVersion`, `BranchInfo`, `MergeResult` | Git-like prompt versioning (branch/commit/diff/merge) |
| `prompt_performance_tracker.h` | `PromptPerformanceTracker`, `PromptMetrics` | Execution-quality and latency tracking |
| `prompt_engineering_metrics.h` | `PromptEngineeringMetrics`, `Config`, `AlertConfig` | Prometheus-style counters, alert thresholds, snapshot/restore |
| `prompt_injection_detector.h` | `PromptInjectionDetector`, `DetectionResult`, `Config` | Prompt/response injection detection and sanitization |
| `context_window_manager.h` | `ContextWindowBudgetManager`, `ModelTokenBudget`, `BudgetAllocation`, `PromptBudgetExceededError` | Token-budget planning and context-limit enforcement |
| `rag_prompt_builder.h` | `RAGPromptBuilder`, `RetrievedChunk`, `RAGPromptConfig` | Retrieval-augmented prompt composition with source citations |
| `chain_of_thought.h` | `ChainOfThoughtBuilder`, `CoTStep`, `CoTConfig` | Structured chain-of-thought prompt construction |
| `cot_tracer.h` | `IChainOfThoughtTracer`, `RecordingCoTTracer`, `CoTTraceCollector`, `CoTSpanRecord` | CoT execution tracing and span collection |
| `reflection_tuner.h` | `ReflectionTuner`, `ReflectionConfig`, `ReflectionResult`, `IReflectionProvider` | Reflection-based iterative revision with hallucination guard |
| `llm_reflection_adapter.h` | `ILLMProviderReflectionAdapter`, `IReflectionScorer` | Adapter from `ILLMProvider` to reflection provider contracts |
| `prompt_regression_runner.h` | `PromptRegressionRunner`, `RegressionFixture`, `RegressionConfig`, `RegressionResult` | Golden-set regression execution and drift classification |
| `prompt_ab_experiment.h` | `PromptABExperimentFramework`, `PromptExperiment`, `ExperimentOutcome` | Deterministic traffic splitting and experiment evaluation |
| `prompt_library_io.h` | `PromptLibraryIO`, `PromptLibraryBundle`, `ImportResult`, `ExportResult` | Prompt library import/export (JSON/YAML) |
| `prompt_template_compiler.h` | `PromptTemplateCompiler`, `IPromptTemplate`, `PromptContextValue` | Typed template DSL and runtime compilation |
| `prompt_template_validator.h` | `PromptTemplateValidator`, `TemplateValidationResult` | Template schema and structure validation |
| `system_prompt_manager.h` | `SystemPromptManager`, `SystemPrompt`, `Role` | Role-based system prompt lifecycle and rendering |
| `tree_of_thoughts.h` | `TreeOfThoughtsBuilder`, `ToTConfig`, `ToTResult` | Tree-of-thought multi-path reasoning workflow |
| `protegi_optimizer.h` | `ProTeGiOptimizer`, `ProTeGiConfig`, `ProTeGiResult`, `IProTeGiLLMProvider` | Textual-gradient prompt optimization |
| `dspy_module.h` | `DspyModule`, `DspySignature`, `DspyChainOfThought`, `IDspyLLMProvider` | DSPy-style declarative prompt APIs |
| `prompt_compressor.h` | `IPromptCompressor`, `PromptCompressionConfig`, `CompressionResult` | Prompt compression contract |
| `structured_output.h` | `IStructuredOutputEnforcer`, `StructuredOutputConfig`, `StructuredOutputResult` | Structured-output constraints (JSON schema/regex grammar) |
| `prompt_quality_evaluator.h` | `IPromptQualityEvaluator`, `QualityConfig`, `QualityReport` | Prompt-quality audit contract |
| `rag_context_budget_manager.h` | `IRAGContextBudgetManager`, `BudgetHandle`, `BudgetSnapshot` | RAG context-budget reservation contract |
| `adversarial_prompt_tester.h` | `IAdversarialPromptTester`, `AdversarialTestCase`, `AdversarialTestReport` | Adversarial prompt security testing contract |

## Configuration Surfaces

Primary runtime/configuration structs exposed in public headers:

- `IntegrationConfig` (`prompt_engineering_integration.h`)
- `ImprovementConfig` (`self_improvement_orchestrator.h`)
- `EvaluatorConfig` (`prompt_evaluator.h`)
- `OptimizationConfig` (`prompt_optimizer.h`)
- `MetaPromptConfig` (`meta_prompt_generator.h`)
- `PromptInjectionDetector::Config` (`prompt_injection_detector.h`)
- `PromptEngineeringMetrics::Config` + `AlertConfig` (`prompt_engineering_metrics.h`)
- `ModelTokenBudget` (`context_window_manager.h`)
- `RAGPromptConfig` (`rag_prompt_builder.h`)
- `ReflectionConfig` (`reflection_tuner.h`)
- `RegressionConfig` (`prompt_regression_runner.h`)
- `PromptCompressionConfig` (`prompt_compressor.h`)
- `StructuredOutputConfig` (`structured_output.h`)
- `ProTeGiConfig` (`protegi_optimizer.h`)

## Runtime Behavior, Error Cases, and Limits

- Most APIs are synchronous and return status/optional data; validation failures are surfaced via result objects (for example `ValidationResult`, `TemplateValidationResult`, `DetectionResult`) rather than process termination.
- Budget and threshold policies are explicit in config structs; callers should tune these to model limits (`ModelTokenBudget`) and quality/SLO targets (`AlertConfig`, `ImprovementConfig`).
- Statistical/optimization APIs require representative input datasets; poor or tiny datasets can produce unstable significance and regression outcomes.
- `PromptBudgetExceededError` signals context-limit breaches in budget-managed flows.

## Usage

```cpp
#include "prompt_engineering/prompt_engineering_integration.h"
#include "prompt_engineering/prompt_manager.h"

themis::prompt_engineering::IntegrationConfig cfg;
cfg.enable_prompt_injection_detection = true;
cfg.enable_auto_optimization = false;

themis::prompt_engineering::PromptEngineeringIntegration integration(cfg, &db, cf);
auto result = integration.execute("sql_generation_v1", {{"user_query", "count open cases"}});
```

## Troubleshooting

- **Template creation fails**: inspect `ValidationResult.errors` and verify required fields (`name`, `version`, `content`).
- **Unexpected prompt truncation or failure**: check token budget inputs (`ModelTokenBudget`) and context chunk sizes.
- **Low optimization gains**: verify test-case quality and `OptimizationConfig`/`ImprovementConfig` thresholds before increasing iterations.
- **False-positive injection alerts**: review `PromptInjectionDetector::Config::custom_patterns` and adjust pattern strictness.

## Related Documentation

- Source module guide: [`../../src/prompt_engineering/README.md`](../../src/prompt_engineering/README.md)
- Source architecture: [`../../src/prompt_engineering/ARCHITECTURE.md`](../../src/prompt_engineering/ARCHITECTURE.md)
- Module roadmap: [`../../src/prompt_engineering/ROADMAP.md`](../../src/prompt_engineering/ROADMAP.md)
- Future enhancements: [`../../src/prompt_engineering/FUTURE_ENHANCEMENTS.md`](../../src/prompt_engineering/FUTURE_ENHANCEMENTS.md)
- German module docs: [`../../docs/de/prompt_engineering/README.md`](../../docs/de/prompt_engineering/README.md)

## Installation

The module is part of ThemisDB. Add include directories through your target:

```cmake
target_include_directories(your_target PRIVATE ${THEMISDB_INCLUDE_DIR})
```
