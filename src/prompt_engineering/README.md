# Prompt Engineering Module

## Module Purpose

The Prompt Engineering module provides a complete lifecycle management system for LLM prompt templates in ThemisDB. It covers prompt creation and storage, version control (branching, diffing, rollback), iterative optimization via meta-prompts, feedback collection, performance tracking, A/B testing, and a self-improvement orchestrator that automatically detects underperforming prompts and triggers optimization cycles. Prometheus metrics and a high-level integration facade are included for production observability.

## Relevant Interfaces

| Interface / File | Role |
|-----------------|------|
| `prompt_template_manager.cpp` | Template storage and context-variable rendering |
| `chain_of_thought.cpp` | Chain-of-thought prompt construction |
| `rag_prompt_builder.cpp` | RAG context injection into prompt templates |
| `system_prompt_manager.cpp` | System prompt management and versioning |

## Scope

**In Scope:**
- Prompt template CRUD with RocksDB-backed persistence and YAML bulk-load
- Context injection (variable substitution into `{placeholder}` patterns)
- Git-like version control for prompts (branches, commits, diffs, parent tracking)
- Iterative prompt optimization with pluggable evaluation and improvement functions
- Meta-prompt generation to drive LLM-assisted prompt rewriting
- User and system feedback collection with severity scoring and pattern analysis
- Per-prompt performance metrics (success rate, latency, user satisfaction)
- A/B testing with statistical significance testing (p-value)
- Self-improvement orchestrator with configurable trigger thresholds
- Prometheus-compatible metrics export
- Background worker thread for periodic auto-optimization checks
- Integration facade combining all subsystems behind a single API

**Out of Scope:**
- LLM inference itself (callers supply the model inference function)
- Prompt injection attack detection (callers are responsible for input sanitization)
- Multi-modal prompts (images, audio)
- Token counting or context-window management

## Key Components

### PromptManager
**Location:** `prompt_manager.cpp`

CRUD store for `PromptTemplate` objects backed by an optional RocksDB column family. Supports YAML bulk-load and context injection at retrieval time.

**Features:**
- Thread-safe reads and writes via TBB `concurrent_hash_map`
- `loadFromYAML()` — bulk-load prompt templates from a YAML configuration file
- `getPromptWithContext()` — retrieve a template and substitute `{key}` variables in one call
- `buildContextFromSchema()` — populate context variables from a `SchemaManager` snapshot (table names, row counts, capabilities)
- RocksDB persistence with `scanPrefix` for listing all stored templates

### FeedbackCollector
**Location:** `feedback_collector.cpp`

Records and stores user and system feedback events against named prompt IDs. Computes aggregate statistics and identifies failure patterns.

**Features:**
- Ten `FeedbackType` values: `USER_POSITIVE`, `USER_NEGATIVE`, `HALLUCINATION_DETECTED`, `TIMEOUT`, `PARSE_ERROR`, `VALIDATION_FAILED`, `CONTEXT_MISSING`, `AMBIGUOUS_OUTPUT`, `SECURITY_ISSUE`, `PERFORMANCE_ISSUE`
- Per-prompt `FeedbackStats`: positive/negative ratios, hallucination count, counts by type
- `analyzeFailurePatterns()` — extracts recurring failure patterns above a minimum occurrence threshold
- Time-range queries, age-based pruning, and bulk clear per prompt
- RocksDB persistence with structured JSON encoding

### PromptEvaluator
**Location:** `prompt_evaluator.cpp`

Computes quality scores for prompt outputs by comparing them against expected results.

**Features:**
- **Semantic similarity** — token overlap-based similarity scoring
- **Exact match** — normalized string equality
- **Partial match** — longest common subsequence ratio
- **Relevance** — keyword coverage metric
- **Weighted score** — configurable linear combination of the four metrics
- Batch evaluation (`evaluateBatch`) with per-case breakdowns and pass/fail counts

### PromptOptimizer
**Location:** `prompt_optimizer.cpp`

Iteratively improves a prompt using a provided evaluation function and an optional improvement function (defaults to meta-prompt-driven rewriting).

**Features:**
- Configurable `max_iterations`, `target_score`, and early-stopping on convergence
- Score and prompt history tracking for audit trails
- Optional version control integration (`enable_version_control`)
- Pluggable `EvaluationFunction` and `ImprovementFunction` callbacks

### MetaPromptGenerator
**Location:** `meta_prompt_generator.cpp`

Generates structured meta-prompts that instruct an LLM to rewrite an underperforming prompt. Produces a formatted markdown prompt containing the original prompt, performance feedback, improvement instructions, constraints, and optional examples.

**Features:**
- Multiple improvement strategies (conservative, aggressive, rewrite, targeted)
- Configurable `include_constraints` and `include_examples` flags
- `generateAnalysisPrompt()` — generate a prompt for analyzing failure patterns
- `generateABTestPrompt()` — generate two variant prompts for A/B comparison

### PromptVersionControl
**Location:** `prompt_version_control.cpp`

Git-like version control for prompt content, with branching, committing, diff generation, and rollback.

**Features:**
- SHA-256 content hashing for version IDs
- Branch management (create, list, merge)
- `commit()` — save a new prompt version with message, author, and parent link
- `diff()` — character-level diff between two versions
- `rollback()` — revert a prompt ID to a previous version
- RocksDB persistence; branch and version metadata stored as JSON

### PromptPerformanceTracker
**Location:** `prompt_performance_tracker.cpp`

Tracks execution-level metrics per prompt across its lifetime.

**Features:**
- `recordExecution()` — record success/failure, latency, and optional satisfaction score
- Sliding-window statistics: success rate, average latency, P95/P99 latency (planned)
- `getUnderperformingPrompts()` — list prompts below a configurable success-rate threshold
- RocksDB persistence for metrics durability across restarts

### SelfImprovementOrchestrator
**Location:** `self_improvement_orchestrator.cpp`

Coordinates the full optimization lifecycle: monitoring prompt metrics, triggering optimization when thresholds are breached, running A/B tests, and promoting winners.

**Features:**
- `runAutoOptimization()` — scan all tracked prompts and trigger optimization for those meeting criteria
- `optimizePrompt()` — run the full optimization pipeline for a single prompt with supplied test cases
- `startABTest()` / `evaluateABTest()` — run and evaluate A/B experiments with configurable sample sizes and significance thresholds
- Configurable `ImprovementConfig`: `min_success_rate`, `min_executions`, `max_iterations`, `improvement_threshold`
- Per-prompt `last_optimization_` timestamp to enforce minimum optimization intervals

### PromptEngineeringMetrics
**Location:** `prompt_engineering_metrics.cpp`

Prometheus text-format metrics for the entire prompt engineering subsystem.

**Metric families:**
- `*_optimization_attempts_total`, `*_optimization_successes_total`, `*_optimization_failures_total`
- `*_optimization_duration_ms_total`, `*_optimization_iterations_total`
- `*_feedback_total` (by type), `*_performance_success_rate`, `*_performance_latency_ms`
- `*_abtest_*` counters, `*_version_commits_total`

### PromptEngineeringIntegration
**Location:** `prompt_engineering_integration.cpp`

High-level facade that wires together all subsystems and exposes a single `execute()` entry point. Optionally runs a background worker thread for periodic auto-optimization checks.

**Features:**
- `execute(prompt_id, context)` — retrieves the prompt, injects context, records execution metrics, collects feedback, and optionally commits a new version
- `IntegrationConfig` JSON serialization/deserialization for runtime reconfiguration
- Background worker: periodically calls `SelfImprovementOrchestrator::runAutoOptimization()`
- `getHealthStatus()` — returns a JSON health summary of all subsystems

## Architecture

```
PromptEngineeringIntegration  (facade + background worker)
        │
        ├─ PromptManager          ──► RocksDB (templates)
        ├─ FeedbackCollector      ──► RocksDB (feedback entries)
        ├─ PromptVersionControl   ──► RocksDB (versions, branches)
        ├─ PromptPerformanceTracker ─► RocksDB (metrics)
        │
        ├─ PromptEvaluator        (pure computation, no persistence)
        ├─ MetaPromptGenerator    (pure computation, no persistence)
        ├─ PromptOptimizer        (uses Evaluator + MetaPromptGenerator)
        │
        ├─ SelfImprovementOrchestrator
        │       ├─ reads  PromptPerformanceTracker
        │       ├─ calls  PromptOptimizer
        │       └─ writes PromptManager + PromptVersionControl
        │
        └─ PromptEngineeringMetrics  (Prometheus export)
```

## Dependencies

### Internal Dependencies
- `storage/rocksdb_wrapper.h` — persistence layer
- `metadata/schema_manager.h` — schema context for `buildContextFromSchema()`
- `utils/logger.h` — `THEMIS_INFO/WARN/ERROR/DEBUG` macros

### External Dependencies
- `nlohmann/json` — JSON serialization/deserialization for all stored objects
- `yaml-cpp` — YAML prompt template bulk-load (`PromptManager::loadFromYAML`)
- `openssl/sha.h` — SHA-256 version ID generation in `PromptVersionControl`
- `tbb/concurrent_hash_map.h` — lock-free concurrent hash map in `PromptManager`
- `spdlog` (via logger utils) — structured logging

## Usage Examples

```cpp
#include "prompt_engineering/prompt_engineering_integration.h"
#include "prompt_engineering/prompt_manager.h"
#include "prompt_engineering/feedback_collector.h"

using namespace themis::prompt_engineering;

// --- Basic: create and use a prompt template ---
PromptManager mgr(&db, cf);
mgr.loadFromYAML("config/ai_ml/llm/system_prompts.yaml");

auto result = mgr.getPromptWithContext("sql_generation_v1",
    {{"schema", schema_str}, {"user_query", "list all active cases"}});
if (result) {
    // pass *result to LLM inference
}

// --- Record feedback ---
FeedbackCollector collector(&db, cf_feedback);
collector.recordFeedback("sql_generation_v1", user_query, llm_response,
    FeedbackType::HALLUCINATION_DETECTED, "Table 'cases' does not exist",
    /*severity=*/0.9);

// --- Run optimization ---
PromptOptimizer optimizer({.max_iterations=10, .target_score=0.85});
auto opt_result = optimizer.optimize(
    original_prompt,
    test_cases,
    eval_fn,   // (prompt, cases) -> double
    improve_fn // (prompt, score, feedback) -> string
);

// --- Full integration ---
IntegrationConfig config;
config.enable_auto_optimization = true;
config.background_worker_enabled = true;
PromptEngineeringIntegration integration(config, &db, cf);

auto exec_result = integration.execute("sql_generation_v1",
    {{"schema", schema_str}, {"user_query", "count documents"}});
```

## Production Readiness

**Current Status: Beta**

- All components are individually functional; integration facade is wiring them together
- RocksDB persistence is implemented for Manager, FeedbackCollector, VersionControl, and PerformanceTracker
- Known limitations:
  - `SelfImprovementOrchestrator::runAutoOptimization()` requires callers to supply test cases; without them, candidate prompts are detected but not optimized
  - `PromptEvaluator` uses token-overlap heuristics rather than true embedding-based semantic similarity; integrate a vector model for production accuracy
  - A/B test statistical significance uses a simplified z-test approximation; validate with your expected traffic volumes
  - Background worker optimization interval defaults to 1 hour; tune via `IntegrationConfig::background_worker_interval`
