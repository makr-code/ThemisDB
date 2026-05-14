> **Build:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# Prompt Engineering Module
<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: src/prompt_engineering/README.md · src/prompt_engineering/ARCHITECTURE.md · src/prompt_engineering/ROADMAP.md · src/prompt_engineering/FUTURE_ENHANCEMENTS.md · include/prompt_engineering/README.md · docs/de/prompt_engineering/README.md -->

## Module Purpose

The Prompt Engineering module provides a complete lifecycle management system for LLM prompt templates in ThemisDB. It covers prompt creation and storage, version control (branching, diffing, rollback), iterative optimization via meta-prompts, feedback collection, performance tracking, A/B testing, and a self-improvement orchestrator that automatically detects underperforming prompts and triggers optimization cycles. Prometheus metrics and a high-level integration facade are included for production observability.

## Relevant Interfaces

| Interface / File | Role |
|-----------------|------|
| `prompt_manager.cpp` | Template storage, context-variable rendering, and YAML bulk-load |
| `chain_of_thought.cpp` | Chain-of-thought prompt construction with step delimiters |
| `rag_prompt_builder.cpp` | RAG context injection into prompt templates |
| `system_prompt_manager.cpp` | System prompt management with per-role overrides |

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
- **Prompt injection attack detection** — `PromptInjectionDetector` detects and sanitizes injection attempts in user prompts and model responses

**Out of Scope:**
- LLM inference itself (callers supply the model inference function)
- Long-running training/fine-tuning of model weights
- Distributed workflow orchestration outside `PromptEngineeringIntegration`

## Key Components

### PromptManager
**Location:** `prompt_manager.cpp`

CRUD store for `PromptTemplate` objects backed by an optional RocksDB column family. Supports YAML bulk-load and context injection at retrieval time.

**Features:**
- Thread-safe reads and writes via TBB `concurrent_hash_map`
- `validateTemplate()` — static validation of required fields (`name`, `content`, `version`), metadata type, and warnings for missing optional fields; returns a `ValidationResult` with lists of errors and warnings
- `createTemplate()` — validates before inserting; returns empty-id sentinel on validation failure
- `loadFromYAML()` — bulk-load prompt templates from a YAML configuration file; skips and logs invalid entries
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
- `getFeedbackPaged(offset, page_size, type_filter)` — chunked read API for large feedback archives
- `detectOutliers(z_threshold)` — Z-score based anomaly detection over severity scores
- `FeedbackEntry::checksum` — FNV-1a 64-bit audit checksum automatically computed on record
- Time-range queries, age-based pruning, and bulk clear per prompt
- RocksDB persistence with structured JSON encoding

### PromptEvaluator
**Location:** `prompt_evaluator.cpp`

Computes quality scores for prompt outputs by comparing them against expected results.

**Features:**
- **Semantic similarity** — Jaccard token-overlap similarity (baseline, zero dependencies)
- **Exact match** — normalized string equality
- **Partial match** — normalized Levenshtein distance
- **Relevance** — keyword coverage metric
- **Weighted score** — configurable linear combination of the four metrics
- Batch evaluation (`evaluateBatch`) with per-case breakdowns and pass/fail counts
- `isStatisticallySignificant()` — proper Welch's two-sample t-test with Welch–Satterthwaite degrees of freedom and p-value via the Lentz continued-fraction regularised incomplete-beta CDF
- **Pluggable embedding provider** via `IEmbeddingProvider` interface:
  - `setEmbeddingProvider(provider)` — inject any embedding model (OpenAI, Sentence Transformers, local)
  - `computeEmbeddingSimilarity(s1, s2)` — cosine similarity of provider embeddings; returns -1.0 on error (graceful fallback to Jaccard in `evaluateSingle()`)
  - `computeCosineSimilarity(v1, v2)` — static helper for cosine similarity of dense vectors
  - `clearEmbeddingProvider()` / `hasEmbeddingProvider()` — lifecycle management

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
- Multiple improvement strategies (iterative, analytical, creative)
- Configurable `include_constraints` and `include_examples` flags
- `generateAnalysisPrompt()` — generate a prompt for analyzing failure patterns
- `generateImprovementSuggestions()` — produce targeted suggestions based on identified weaknesses
- **Pluggable LLM integration** via `ILLMProvider` interface:
  - `setLLMProvider(provider)` — inject any LLM backend for real-time prompt improvement
  - `clearLLMProvider()` / `hasLLMProvider()` — manage the provider lifecycle
  - Graceful fallback to template-based generation on LLM error or empty response

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

**Persistence:**
- `snapshotToJson()` — serialize all counter values to JSON for crash-safe restart recovery
- `restoreFromJson(snapshot)` — restore counter values from a snapshot

**Alerting:**
- `setAlertConfig(AlertConfig)` — configure thresholds (max failure rate, max hallucination count)
- `setAlertCallback(fn)` — register a callback fired whenever a threshold is breached
- Alerts fire automatically in `recordPromptExecution()` and `recordHallucinationDetection()`

### PromptEngineeringIntegration
**Location:** `prompt_engineering_integration.cpp`

High-level facade that wires together all subsystems and exposes a single `execute()` entry point. Optionally runs a background worker thread for periodic auto-optimization checks.

**Features:**
- `execute(prompt_id, context)` — retrieves the prompt, injects context, records execution metrics, collects feedback, and optionally commits a new version
- `IntegrationConfig` JSON serialization/deserialization for runtime reconfiguration
- Background worker: periodically calls `SelfImprovementOrchestrator::runAutoOptimization()`
- `getHealthStatus()` — returns a JSON health summary of all subsystems

### PromptInjectionDetector
**Location:** `prompt_injection_detector.cpp`

Pattern-based detection and sanitization layer for prompt injection attacks. Callers should invoke this before dispatching user prompts to any LLM and after receiving model responses (to guard against indirect/second-order injection).

**Features:**
- `detect(prompt)` — analyses user-supplied text; returns a `DetectionResult` with `is_injection`, `risk_score` (0.0–1.0), `matched_patterns`, and a sanitized copy
- `detectInResponse(response)` — applies the same heuristics to model responses to catch indirect injection (adversarially crafted responses that embed override instructions)
- `sanitize(text)` — returns a sanitized copy of the text with all detected patterns replaced by `[REDACTED]`
- 10 built-in case-insensitive regex patterns: instruction override (`ignore`/`disregard`/`forget`), system prompt exfiltration (`reveal`/`tell`/`print`/`show`), special LLM tokens (`[INST]`, `<|system|>`), jailbreak modes, act-as-unrestricted, safety bypass
- Supplementary keyword and syntax scoring (high special-char density, instruction-bracket tokens)
- Pluggable `Config::custom_patterns` — add domain-specific regex patterns at construction time; invalid patterns are silently skipped
- `DetectionResult::toJson()` — serialise result for audit logging
- `Config::enabled` flag for runtime toggle (returns zero-risk result when disabled)

### ChainOfThoughtBuilder
**Location:** `chain_of_thought.cpp`

Constructs chain-of-thought (CoT) prompt strings that guide LLMs through explicit step-by-step reasoning before producing a final answer.

**Features:**
- **Builder mode** — add named reasoning steps incrementally via `addStep()` / `addReasoningStep()`, set a final answer with `setFinalAnswer()`, then call `build()`
- Auto-numbering of steps (`Step 1:`, `Step 2:`, …) or explicit labels per step
- Configurable step delimiter and prefix via `CoTConfig`
- **`buildZeroShot(question)`** — appends "Let's think step by step." to elicit zero-shot CoT
- **`buildFewShot(question, examples)`** — prepends solved (Q, A) examples before the target question
- **`wrapWithCoT(prompt, explicit_steps)`** — wraps an existing prompt with CoT instructions; optionally adds explicit step headings

### RAGPromptBuilder
**Location:** `rag_prompt_builder.cpp`

Assembles Retrieval-Augmented Generation (RAG) prompts by injecting retrieved document chunks as grounding context into LLM prompt templates.

**Features:**
- **`build(template, query, chunks)`** — replaces `{context}` and `{query}` placeholders in a base template with the assembled context block and the user query
- **`buildContextSection(chunks)`** — produces a formatted context block (header + chunks + optional footer) for use in custom templates
- **`buildFullPrompt(system_instruction, query, chunks)`** — combines system instruction, context block, and query into a standard RAG prompt
- **`selectChunks(candidates, max_total_length)`** — greedy budget-aware chunk selection; optionally sorts candidates by `relevance_score` descending
- Source citations — each chunk prefixed with `[Source N: <source_id>]` when enabled
- Configurable `RAGPromptConfig`: `max_context_length`, `context_header/footer`, `chunk_separator`, `template_placeholder`, citation toggle

### SystemPromptManager
**Location:** `system_prompt_manager.cpp`

Manages a registry of system prompts keyed by a strongly-typed `Role` enumeration, with support for arbitrary custom role names.

**Features:**
- Built-in roles: `DEFAULT`, `USER`, `ASSISTANT`, `ADMIN`, `SYSTEM`
- Custom roles via `setCustomPrompt(role_name, …)` / `getCustomPrompt(role_name)`
- `getPromptContent(role, default_content)` — returns registered content or a caller-supplied fallback
- **Context-variable rendering**: `renderPrompt(role, context)` / `renderCustomPrompt(role_name, context)` — substitute `{placeholder}` tokens using a `std::unordered_map`
- `listPrompts()` — enumerate all registered prompts (built-in and custom)
- `SystemPrompt::toJson()` / `SystemPrompt::fromJson()` — JSON serialisation for persistence
- Thread-safe via `std::mutex`

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
        ├─ PromptEngineeringMetrics  (Prometheus export)
        │
        ├─ PromptInjectionDetector   (stateless security layer; called by callers)
        │
        ├─ ChainOfThoughtBuilder     (pure computation; CoT prompt construction)
        ├─ RAGPromptBuilder          (pure computation; RAG context injection)
        └─ SystemPromptManager       (in-memory registry; per-role system prompts)
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
#include "prompt_engineering/chain_of_thought.h"
#include "prompt_engineering/rag_prompt_builder.h"
#include "prompt_engineering/system_prompt_manager.h"

using namespace themis::prompt_engineering;

// --- Basic: create and use a prompt template ---
PromptManager mgr(&db, cf);
mgr.loadFromYAML("config/ai_ml/llm/system_prompts.yaml");

auto result = mgr.getPromptWithContext("sql_generation_v1",
    {{"schema", schema_str}, {"user_query", "list all active cases"}});
if (result) {
    // pass *result to LLM inference
}

// --- Chain-of-Thought prompt construction ---
ChainOfThoughtBuilder cot;
cot.addStep("Identify all entities mentioned in the legal text.")
   .addStep("Determine the relationship between each entity pair.")
   .setFinalAnswer("List each relationship on a separate line.");
std::string cot_prompt = cot.build();

// Zero-shot CoT shortcut
auto zs_prompt = ChainOfThoughtBuilder::buildZeroShot("What are the key obligations?");

// --- RAG prompt assembly ---
std::vector<RetrievedChunk> chunks = {
    {"Clause 4.2: The vendor shall deliver by Q3.", "contract_v2.pdf", 0.95},
    {"Clause 7.1: Liability is limited to …",       "contract_v2.pdf", 0.82}
};
RAGPromptBuilder rag;
std::string rag_prompt = rag.buildFullPrompt(
    "You are a legal contract analyst.",
    "What are the delivery obligations?",
    chunks);

// --- System prompts with per-role overrides ---
SystemPromptManager spm;
spm.setPrompt(Role::USER,  "You are a helpful assistant for {product}.", "1.0");
spm.setPrompt(Role::ADMIN, "You are an expert DBA with full access to {product}.", "1.0");
spm.setCustomPrompt("legal_reviewer", "Review contracts for legal accuracy.", "1.0");

std::string user_sys  = spm.renderPrompt(Role::USER, {{"product", "ThemisDB"}});
std::string admin_sys = spm.renderPrompt(Role::ADMIN, {{"product", "ThemisDB"}});

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

## Troubleshooting

- **Template not found / empty prompt output**: verify template id/version in `PromptManager` and ensure placeholders in `getPromptWithContext()` receive all required keys.
- **Budget exceptions**: configure `ModelTokenBudget` and chunk sizes (`RAGPromptBuilder::selectChunks`) to stay under context limits.
- **Unstable A/B or regression outcomes**: increase sample sizes and validate fixture quality before acting on p-values.
- **Injection detector blocks benign prompts**: tune `PromptInjectionDetector::Config::custom_patterns` and review matched patterns in `DetectionResult`.

## Production Readiness

**Current Status: Production-Ready (v1.x)**

All components are individually tested and the integration facade wires them together. The following capabilities are production-hardened:

- **Template validation**: `PromptManager::validateTemplate()` enforces required fields and metadata structure before any template is stored. `loadFromYAML()` skips and logs invalid entries.
- **Feedback scalability**: `FeedbackCollector::getFeedbackPaged()` provides chunked read access for large feedback archives. `detectOutliers()` identifies anomalous severity values via Z-score. Each entry carries an FNV-1a audit checksum for compliance.
- **Pluggable LLM integration**: `ILLMProvider` interface allows injecting any LLM backend (OpenAI, Cohere, local models) into `MetaPromptGenerator` for real-time prompt improvement. Falls back gracefully to template-based generation on error.
- **Statistical evaluation**: `PromptEvaluator::isStatisticallySignificant()` implements a proper Welch's two-sample t-test with Welch–Satterthwaite degrees of freedom and p-value via the Lentz continued-fraction incomplete-beta CDF.
- **A/B test statistics**: `SelfImprovementOrchestrator::analyzeABTest()` uses the standard normal CDF (`std::erfc`) for accurate two-proportion z-test p-values.
- **Metrics persistence**: `PromptEngineeringMetrics::snapshotToJson()` / `restoreFromJson()` enable crash-safe counter persistence to any key-value store.
- **Threshold alerting**: `setAlertConfig()` / `setAlertCallback()` fire pluggable callbacks when failure rate or hallucination count breach thresholds.

Known limitations (by design):
  - Full LLM-based evaluation in `SelfImprovementOrchestrator::optimizePrompt()` requires callers to execute the prompt through their LLM and supply a custom `eval_fn`; the built-in fallback uses `PromptEvaluator` for structural similarity as a proxy.
  - Background worker optimization interval defaults to 1 hour; tune via `IntegrationConfig::background_worker_interval`.

## Scientific References

1. White, J., Fu, Q., Hays, S., Sandborn, M., Olea, C., Gilbert, H., … Schmidt, D. C. (2023). **A Prompt Pattern Catalog to Enhance Prompt Engineering with ChatGPT**. *arXiv preprint*. https://arxiv.org/abs/2302.11382

2. Wei, J., Wang, X., Schuurmans, D., Bosma, M., Ichter, B., Xia, F., … Zhou, D. (2022). **Chain-of-Thought Prompting Elicits Reasoning in Large Language Models**. *Advances in Neural Information Processing Systems (NeurIPS)*, 35. https://arxiv.org/abs/2201.11903

3. Zhou, Y., Muresanu, A. I., Han, Z., Paster, K., Pitis, S., Chan, H., & Ba, J. (2022). **Large Language Models Are Human-Level Prompt Engineers**. *Proceedings of ICLR 2023*. https://arxiv.org/abs/2211.01910

4. Rubin, O., Herzig, J., & Berant, J. (2022). **Learning To Retrieve Prompts for In-Context Learning**. *Proceedings of NAACL-HLT 2022*, 1523–1535. https://doi.org/10.18653/v1/2022.naacl-main.191

5. Lester, B., Al-Rfou, R., & Constant, N. (2021). **The Power of Scale for Parameter-Efficient Prompt Tuning**. *Proceedings of EMNLP 2021*, 3045–3059. https://doi.org/10.18653/v1/2021.emnlp-main.243

## Installation

This module is built as part of ThemisDB. See the root `CMakeLists.txt` for build configuration.
