<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Prompt Engineering Module

The `prompt_engineering` module provides systematic prompt lifecycle management for ThemisDB LLM integrations.

## Headers

| Header | Key Symbols | Description |
|---|---|---|
| `chain_of_thought.h` | `ChainOfThoughtBuilder` | Structured chain-of-thought prompt construction |
| `context_window_manager.h` | `ContextWindowBudgetManager`, `ITokenCounter`, `CharDivisionCounter`, `ModelTokenBudget`, `BudgetAllocation`, `PromptBudgetExceededError` | Token budget allocation per model |
| `feedback_collector.h` | `FeedbackCollector` | Human/automatic feedback collection |
| `meta_prompt_generator.h` | `MetaPromptGenerator` | Meta-prompt generation for self-improvement |
| `prompt_engineering_integration.h` | `PromptEngineeringIntegration` | Full pipeline integration facade |
| `prompt_engineering_metrics.h` | `PromptEngineeringMetrics` | 4 reflection counters + Prometheus export |
| `prompt_evaluator.h` | `PromptEvaluator` | Automated prompt quality evaluation |
| `prompt_injection_detector.h` | `PromptInjectionDetector` | Injection detection and blocking |
| `prompt_manager.h` | `PromptManager` | Template registry |
| `prompt_optimizer.h` | `PromptOptimizer` | A/B-testing-driven optimization |
| `prompt_performance_tracker.h` | `PromptPerformanceTracker` | Latency, cost, quality tracking |
| `prompt_template_validator.h` | `PromptTemplateValidator` | Schema validation |
| `prompt_version_control.h` | `PromptVersionControl` | Git-like versioning (branch/tag/diff/merge) |
| `rag_prompt_builder.h` | `RAGPromptBuilder` | RAG-augmented prompt construction |
| `self_improvement_orchestrator.h` | `ReflectionTuner`, `ILLMProviderReflectionAdapter` | SELF_REFINE / REFLEXION / CONSTITUTIONAL / SOCRATIC |
| `system_prompt_manager.h` | `SystemPromptManager` | System prompt lifecycle and persona management |

## Links

- Architecture: [ARCHITECTURE.md](ARCHITECTURE.md)
- Roadmap: [ROADMAP.md](ROADMAP.md)
- Implementation: [`../../src/prompt_engineering/`](../../src/prompt_engineering/)

## Installation

This module is included as part of ThemisDB. Add the module headers to your include path:

```cmake
target_include_directories(your_target PRIVATE ${THEMISDB_INCLUDE_DIR})
```

## Usage

Include the relevant headers from this module:

```cpp
#include "prompt_engineering/module_header.h"
```

See [`ARCHITECTURE.md`](ARCHITECTURE.md) and [`ROADMAP.md`](ROADMAP.md) for details.
