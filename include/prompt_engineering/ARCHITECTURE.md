<!-- Status: current | validated: 2026-03-22 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Prompt Engineering Module — Architecture Guide

## Overview

The prompt_engineering module provides systematic prompt lifecycle management for ThemisDB LLM integrations: template management, version control, A/B testing, injection detection, RAG-augmented prompt building, chain-of-thought construction, context window budget management, self-improvement via reflection (SELF-REFINE, REFLEXION, CONSTITUTIONAL, SOCRATIC), and Prometheus metrics.

## Design Principles

- **Version-controlled prompts** — `prompt_version_control.h` provides git-like branching and tagging for prompt evolution.
- **Budget-aware context** — `ContextWindowBudgetManager` enforces per-model token budgets with configurable allocation strategies.
- **Injection-safe** — `PromptInjectionDetector` screens all user-supplied input before it enters prompt templates.
- **Reflection-driven improvement** — `ReflectionTuner` (v1.5.0) supports four strategies; `ILLMProviderReflectionAdapter` (v1.6.0) abstracts provider-specific APIs.
- **Metrics-first** — `PromptEngineeringMetrics` tracks 4 reflection counters and exports to Prometheus.

## Interface Inventory

| Header | Classes / Interfaces | Purpose |
|---|---|---|
| `chain_of_thought.h` | `ChainOfThoughtBuilder` | Structured CoT prompt construction |
| `context_window_manager.h` | `ContextWindowBudgetManager`, `ITokenCounter`, `CharDivisionCounter`, `ModelTokenBudget`, `BudgetAllocation`, `PromptBudgetExceededError` | Token budget allocation and enforcement per model |
| `feedback_collector.h` | `FeedbackCollector` | Collects human/automatic feedback for prompt optimization |
| `meta_prompt_generator.h` | `MetaPromptGenerator` | Generates meta-prompts for self-improvement cycles |
| `prompt_engineering_integration.h` | `PromptEngineeringIntegration` | Integration facade for the full prompt engineering pipeline |
| `prompt_engineering_metrics.h` | `PromptEngineeringMetrics` | 4 reflection counters + Prometheus export |
| `prompt_evaluator.h` | `PromptEvaluator` | Automated prompt quality evaluation |
| `prompt_injection_detector.h` | `PromptInjectionDetector` | Detects and blocks prompt injection attempts |
| `prompt_manager.h` | `PromptManager` | Central registry for prompt templates |
| `prompt_optimizer.h` | `PromptOptimizer` | Automated prompt optimization via A/B testing |
| `prompt_performance_tracker.h` | `PromptPerformanceTracker` | Latency, cost, and quality tracking per prompt version |
| `prompt_template_validator.h` | `PromptTemplateValidator` | Schema validation for prompt templates |
| `prompt_version_control.h` | `PromptVersionControl` | Git-like versioning: branch, tag, diff, merge |
| `rag_prompt_builder.h` | `RAGPromptBuilder` | RAG-augmented prompt construction with citation tracking |
| `self_improvement_orchestrator.h` | `ReflectionTuner`, `ILLMProviderReflectionAdapter` | Four reflection strategies (SELF_REFINE, REFLEXION, CONSTITUTIONAL, SOCRATIC) |
| `system_prompt_manager.h` | `SystemPromptManager` | System prompt lifecycle: roles, personas, versioned system instructions |

## Integration Points

| Integrates With | Via | Notes |
|---|---|---|
| `rag` | `RAGPromptBuilder` | RAG context injection into prompts |
| `llm` | `ILLMProviderReflectionAdapter` | Provider-specific reflection APIs |
| `observability` | `PromptEngineeringMetrics` | Prometheus reflection counters |
| `query` | `PromptEngineeringIntegration` | Prompt-driven query generation |

## Implementation

Implementation in `../../src/prompt_engineering/`.
