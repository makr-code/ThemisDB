> **Hinweis:** Vage Einträge ohne messbares Ziel, Interface-Spezifikation oder Teststrategie mit `<!-- TODO: add measurable target, interface spec, test strategy -->` markieren.

<!-- Status: current | validated: 2026-06-09 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

# AQL Module - Future Enhancements

## Scope

The AQL module is ThemisDB's query language and LLM-integration layer. It covers the complete pipeline from natural-language query input through LLM-assisted translation, syntax validation, and query execution including `LLM INFER`, `LLM RAG`, `LLM EMBED`, and `LLM FINETUNE` commands. Supporting components handle conversation context (`aql_conversation_context.cpp`), confidence scoring (`aql_confidence_scorer.cpp`), few-shot example selection (`aql_fewshot_example_library.cpp`), LoRA fine-tuning (`aql_lora_finetuner.cpp`), query building (`aql_query_builder.cpp`), query validation (`aql_query_validator.cpp`), syntax highlighting / annotation (`aql_syntax_highlighter.cpp`), schema provision (`aql_schema_provider.cpp`), and metrics collection (`llm_metrics_collector.cpp`). All LLM command dispatch passes through `llm_aql_handler.cpp`, which is the heaviest file in the module and the primary source of complexity. A timeout-and-retry framework (`llm_timeout_manager.h`) and a circuit breaker (`sharding/circuit_breaker.h`) provide resilience. Streaming is available through a header-only `AQLTokenStream` and through SSE helpers.

---

## Design Constraints

- `[ ]` Lexer tokenisation rate must be ≥ 50 MB/s for ASCII query text on a single core
- `[ ]` Parser must produce a complete AST or a structured error within 10 ms for queries ≤ 64 KB
- `[ ]` AST node count hard cap: 100 000 nodes per query; queries exceeding this are rejected with a clear error
- `[ ]` Evaluator must support query cancellation via co-operative cancellation token within 200 ms
- `[ ]` LLM command dispatch must be fully asynchronous; the evaluator must not block on model inference
- `[ ]` All grammar changes must be backward-compatible or gated behind a feature flag with a migration path
- `[ ]` No `std::exception` propagation across the module public API; all errors as `Result<T>`
- `[ ]` No raw `std::thread::detach()` after timeout; every spawned worker must have a bounded lifetime or a cooperative cancellation channel
- `[ ]` Circuit breakers must be scoped per operation type (INFER, RAG, EMBED) so a failure domain in one command does not block others
- `[ ]` All hard-coded scoring weights, token limits, and training hyperparameters must be overridable at runtime without recompilation

---

## Required Interfaces

| Interface | Consumer | Notes |
|-----------|----------|-------|
| `AQLLexer::tokenise(query_text)` | `AQLParser` | Returns token stream; invalid UTF-8 rejected |
| `AQLParser::parse(token_stream)` | `AQLEvaluator`, query planner | Returns `ASTNode` tree or structured `ParseError` |
| `ASTVisitor::visit(node)` | Semantic analyser, code-gen, optimiser | Visitor pattern; must be re-entrant |
| `AQLEvaluator::execute(ast, context)` | Core query executor | Accepts `CancellationToken`; streams results |
| `LLMDispatcher::dispatch(LLMCommand, callback)` | `AQLEvaluator` | Async; result delivered via callback/future |
| `QueryOptimiser::optimise(ast, stats)` | `AQLEvaluator` pre-execution | Returns rewritten AST with cost annotation |
| `AQLQueryValidator::validate(query)` | `LLMAQLHandler::translateNLToAQL()` | Must be called post-LLM-generation, not only on explicit user queries |
| `ConfidenceScorerConfig` | `AQLConfidenceScorer` | Runtime-injectable weights replacing hard-coded `0.50f`/`0.30f`/`0.20f` |
| `IAsyncLLMBackend` | `LLMAQLHandler` | Non-blocking async interface replacing current synchronous `executeChat()` |
| `FewShotEmbeddingIndex` | `AQLFewShotExampleLibrary` | Semantic similarity search to replace current Jaccard word-overlap |

---

## Planned Features

### 1 · Post-Generation AQL Validation in `translateNLToAQL()`
**Priority:** High
**Target Version:** v1.6.0

**Problem (from code):** `llm_aql_handler.cpp:translateNLToAQL()` (lines 1038–1059) validates the LLM-generated query using `AQLSyntaxHighlighter::annotateErrors()` which **only logs warnings** — it never rejects or sanitises the output. `AQLQueryValidator::validate()` (which can produce `ValidationResult` with severity-based `issues`) is never invoked on LLM-generated queries. The same pattern is repeated in `translateNLToAQLStreaming()` (line 1141) and `translateNLToAQLWithExamples()` (line 1389). A structurally invalid query silently reaches the caller and may be executed against the database.

**Implementation Notes:**
- `[x]` In `llm_aql_handler.cpp:translateNLToAQL()`, after the markdown-fence stripping and `trim()` step, call `AQLQueryValidator::validate(aql_query)` and inspect `ValidationResult::issues`; if any issue has severity `ERROR`, throw `LLMException(LLMErrorCode::INVALID_RESPONSE, ...)` with the first error message instead of silently returning the malformed query
- `[x]` Apply the same fix to `translateNLToAQLStreaming()` (line 1141) and `translateNLToAQLWithExamples()` (line 1389) — both currently use `annotateErrors()` as the sole post-processing check
- `[x]` Add a retry path: if validation fails and `retry_policy_` has remaining retries, re-invoke the LLM with an augmented prompt that includes the error annotation as feedback ("Your previous attempt produced this error: …")
- `[x]` Expose a `TranslationValidationMode` enum (`WARN_ONLY`, `REJECT_ON_ERROR`, `RETRY_ON_ERROR`) on `LLMAQLHandler` so callers can choose enforcement level
- `[x]` Unit-test: craft an NL query that reliably causes the mock LLM to return broken AQL (`FOR x`) and assert that `translateNLToAQL` throws instead of returning it

**Performance Targets:**
- Validation overhead ≤ 1 ms per generated query (the validator is string-based with no I/O)

---

### 2 · Eliminate Thread Leak in `LLMTimeoutManager::executeWithTimeout()`
**Priority:** High
**Target Version:** v1.6.0
**Status:** ✅ Implemented (Issue #32)

**Problem (from code):** `include/aql/llm_timeout_manager.h:executeWithTimeout()` (line ~90) calls `worker.detach()` when the timeout fires. The comment on that line explicitly acknowledges: *"the worker thread is detached and may continue executing"*. A detached thread holds all resources it has captured by reference or value and cannot be joined. Under sustained load a burst of LLM timeouts will accumulate many detached threads, each one consuming a stack (~8 MB default on Linux) and holding a reference to the plugin manager. The `executeWithCancelToken()` variant sets the cancel token before detaching but still has the same thread-leak problem if the worker ignores the token.

**Implementation Notes:**
- `[x]` Replace the `std::thread` + `std::packaged_task` approach in `executeWithTimeout()` with `std::jthread` (C++20) and a `std::stop_token`; `jthread::request_stop()` signals the token and the destructor joins automatically — no detach needed
- `[x]` On timeout: call `request_stop()` and transfer jthread ownership to a thin background cleanup thread that joins the worker when it finishes — eliminates the thread leak without blocking the calling thread
- `[x]` Same fix applied to `executeWithCancelToken()`: cancel token is set first, then the jthread is handed to the cleanup thread
- `[x]` Add a test asserting that after `executeWithTimeout()` throws `TIMEOUT`, the associated worker thread has terminated within `timeout + 500 ms` (use a latch decremented by the worker on exit)
- `[x]` Document in the `TimeoutConfig` struct that `infer_timeout{300}`, `rag_timeout{600}`, `embed_timeout{60}`, and `model_load_timeout{900}` are soft defaults and show how to override them via `LLMTimeoutManager::setConfig()`

**Performance Targets:**
- Zero leaked threads after 1 000 sequential timeout events in the test suite

---

### 3 · Per-Operation-Type Circuit Breakers
**Priority:** High
**Target Version:** v1.6.0
**Status:** ✅ Implemented (Issue #33)

**Problem (from code):** `llm_aql_handler.cpp:Impl` (lines 216–222 and 247–248) creates a single `sharding::CircuitBreaker` instance shared across `executeInfer()`, `executeInferStreaming()`, `executeRAG()`, and `executeEmbed()`. When `executeInfer` accumulates 5 failures (`failure_threshold = 5`), the breaker trips and `allowRequest()` returns false — this blocks all RAG and EMBED commands as well, even if those operations would succeed. The 60-second `timeout` window is also a single global parameter.

**Implementation Notes:**
- `[x]` In `LLMAQLHandler::Impl`, replace the single `circuit_breaker_` member with a map: `std::unordered_map<std::string, sharding::CircuitBreaker> circuit_breakers_` keyed by `"infer"`, `"rag"`, `"embed"`, `"finetune"`
- `[x]` Refactor `executeInfer()`, `executeRAG()`, `executeEmbed()` to each look up their own breaker by key
- `[x]` Allow per-command `CircuitBreaker::Config` to be injected via a `LLMAQLHandler::Config` struct so failure thresholds and windows are tunable per command type
- `[x]` Add a `getCircuitBreakerStates()` method for observability; expose via `LLM STATS` command output
- `[x]` Circuit breaker state is already recorded in metrics via `metrics.recordCircuitBreakerState("infer", "open")` — preserve and extend to all command types

---

### 4 · Runtime-Configurable Confidence Scoring Weights
**Priority:** Medium
**Target Version:** v1.6.0
**Status:** ✅ Implemented (Issue #144)

**Problem (from code):** `aql_confidence_scorer.cpp` (lines 58–61) hard-codes the final scoring formula as `structural_score * 0.50f + completeness_score * 0.30f + schema_match_score * 0.20f`. The keyword-bonus table (lines 100–111) is also a hard-coded `std::vector<std::pair<std::string, float>>` with values like `{"filter", 0.20f}`, `{"sort ", 0.15f}`. The `0.5f` neutral return value for missing schema (line 129 and 134) and the `0.1f` floor for zero collection matches (line 145) are also untunable. Field names that appear in confidence scoring (like keyword `"upsert"` on line 111) use substring matching which can accidentally match sub-tokens.

**Implementation Notes:**
- `[x]` Introduce an `AQLConfidenceScorer::Config` struct with fields: `float structural_weight`, `float completeness_weight`, `float schema_match_weight`, keyword `std::unordered_map<std::string, float> keyword_bonuses`, `float no_schema_neutral`, `float zero_match_floor`; default values match current hard-coded constants for backward compatibility
- `[x]` Inject `Config` via constructor; `AQLConfidenceScorer()` (the default ctor) keeps existing behaviour
- `[x]` Fix substring keyword matching (e.g. `"insert"` inside `"upsert"`) by checking word boundaries with `\b` regex or a tokenised lookup — implemented via `containsKeyword()` static helper using manual word-boundary checks (no regex dependency)
- `[x]` Add a `calibrate(const std::vector<std::pair<std::string,float>>& labelled_pairs)` method that fits the three top-level weights via least-squares regression on (query, ground-truth-confidence) pairs — implemented via OLS normal equations + Cramer's rule (3×3, no external deps)
- `[x]` Unit-test: verify that calling `score()` on an empty query returns 0.0 and on a complete `FOR x IN c FILTER x.a == 1 RETURN x` returns > 0.7

---

### 5 · Accurate Token-Count Estimation
**Priority:** Medium
**Target Version:** v1.6.0

**Problem (from code):** `llm_aql_handler.cpp:Impl` (line 238) defines `static constexpr size_t CHARS_PER_TOKEN = 4` and uses `text.length() / CHARS_PER_TOKEN` (line 242) as the sole token-count estimator for prompt budget checks. This approximation is derived from English-language ASCII text and BPE tokenizers; it is materially wrong for multilingual content, code, and especially for few-shot schema context blocks that are dominated by JSON/AQL keywords (which tokenize more compactly). An underestimate causes context-window overflow inside the model; an overestimate wastes capacity, truncating schema context unnecessarily.

**Implementation Notes:**
- `[x]` Introduce a `TokenEstimator` abstraction in `include/aql/llm_token_estimator.h` with `virtual size_t estimate(const std::string& text) const`; provide two implementations: `CharDivisionEstimator` (current behaviour, ratio configurable) and `TiktokenEstimator` (wraps the `tiktoken-cpp` or llama.cpp tokenizer)
- `[x]` Inject `TokenEstimator` into `LLMAQLHandler::Impl`; default to `CharDivisionEstimator` with `ratio=4` for no breaking change
- `[x]` Replace all three call-sites of `estimateTokenCount()` in `llm_aql_handler.cpp` (lines 336, 492, 658) with the injected estimator
- `[x]` Add a benchmark comparing estimator accuracy against the actual llama.cpp tokenizer on the built-in few-shot corpus from `aql_fewshot_example_library.cpp`; accuracy target: ≤ 10 % error at the 95th percentile

---

### 6 · Wire `detectIntentWithNativeNLP()` to the CLASSIFY Function
**Priority:** Medium
**Target Version:** v1.7.0

**Problem (from code):** `docs_assistant_functions.cpp:201` contains an explicit `TODO`:
```
// TODO: Integrate with native CLASSIFY function when execution context available
```
`detectIntentWithNativeNLP()` (line 196) always returns `"unknown"` and then falls through to the slower LLM path. The comment (lines 203–209) describes the intended call signature: `CLASSIFY(text, categories) -> {category, confidence, scores}`, but the function has no access to the AQL function registry at call time. This means every docs-assistant query that could be handled cheaply via the local CLASSIFY function instead triggers a full LLM round-trip.

**Implementation Notes:**
- `[x]` Add a `FunctionRegistry*` or `IClassifyFn` interface pointer parameter to `DocsAssistantFunctions` (injectable via constructor or `setClassifier()`); when non-null, call it directly in `detectIntentWithNativeNLP()` instead of returning `"unknown"`
- `[x]` Define an `IClassifyFn` interface: `virtual ClassifyResult classify(const std::string& text, const std::vector<std::string>& categories) const = 0`; provide a `NullClassifyFn` no-op fallback
- `[x]` Register `AQLFunctionClassifyBridge` as the concrete implementation in the AQL module initialiser, binding it to the global function registry
- `[x]` Remove the `return "unknown"` early exit once a real implementation is wired; the `catch` block at line 215 serves as the fallback
- `[x]` Add an integration test that verifies `detectIntentWithNativeNLP("how do I create an index?")` returns `"configuration"` with confidence > 0.7 when the bridge is wired

---

### 7 · Parallel Execution of `translateBatchNLToAQL()`
**Priority:** Medium
**Target Version:** v1.7.0

**Problem (from code):** `llm_aql_handler.cpp:translateBatchNLToAQL()` (lines 1172–1188) processes each request sequentially in a `for` loop. Each call to `translateNLToAQL()` performs a synchronous LLM inference (potentially 1–30 seconds). A batch of 10 independent translation requests therefore takes 10× the single-request latency. There is no parallelism despite each request being completely independent.

**Implementation Notes:**
- `[x]` Replace the sequential loop with a bounded worker pool: launch `min(n, max_concurrent_requests)` workers via `std::async(std::launch::async, ...)`; each worker claims requests from a shared `std::atomic<size_t>` work index; results collected in original request order via pre-allocated output vector
- `[x]` Respect a `max_concurrent_requests` limit (default: `std::thread::hardware_concurrency()`) to avoid exhausting the LLM backend thread pool; implemented via bounded thread pool (`min(n, concurrency)` workers), directly bounding both thread creation and active inferences
- `[x]` Propagate per-request cancellation: if one request in the batch throws a non-retryable exception, do not cancel others (current sequential behaviour accidentally provides this; parallel version must preserve it)
- `[x]` Add a `translateBatchNLToAQLAsync()` overload that returns `std::future<std::vector<BatchNLToAQLResult>>`
- `[x]` Benchmark: 10 independent requests with a mock LLM (each 50 ms) should complete in ≤ 150 ms wall-time when concurrency ≥ 4

---

### 8 · Semantic Few-Shot Example Selection
**Priority:** Medium
**Target Version:** v1.7.0

**Problem (from code):** `aql_fewshot_example_library.cpp:computeRelevance_()` (line ~177) uses Jaccard word-overlap between the incoming natural-language query and each example's `nl_query`. Jaccard similarity is cheap but vocabulary-dependent: the query *"retrieve all users with email addresses"* scores zero overlap against an example titled *"fetch members by contact info"* even though they are semantically identical. This causes the few-shot examples injected into LLM prompts to be less relevant than they could be, degrading translation quality.

**Implementation Notes:**
- `[x]` Add an optional `IEmbeddingProvider* embedding_provider_` pointer to `AQLFewShotExampleLibrary`; when set, pre-embed all examples on first `add()` or `addBuiltinSamples()` and store embeddings alongside examples
- `[x]` Implement `computeRelevanceSemantic_()` using cosine similarity between the query embedding and each stored example embedding; fall back to `computeRelevance_()` (Jaccard) when no provider is set
- `[x]` Add `AQLFewShotExampleLibrary::setEmbeddingProvider(IEmbeddingProvider*)` and `rebuildEmbeddingIndex()` methods
- `[x]` Use the `LLMAQLHandler`'s existing `executeEmbed()` as the default embedding provider bridge — `LLMAQLEmbeddingBridge` in `include/aql/llm_aql_embedding_bridge.h`; factory `LLMAQLHandler::makeEmbeddingBridge()`. Tests: `test_llm_aql_embedding_bridge.cpp` (EMB_01..05).
- `[ ]` Add a benchmark comparing Jaccard vs. semantic selection on 50 held-out NL queries from the built-in sample set; target: ≥ 15 % improvement in top-3 relevance@k

---

### 9 · Bounded Conversation History with Context-Window Budget
**Priority:** High
**Target Version:** v1.6.0

**Problem (from code):** `aql_conversation_context.cpp` grows `history_` (`std::vector<llm::ChatMessage>`, line 47) indefinitely with each call to `chat()` (lines 92–101). There is no `max_turns` cap, no token-budget check, and no sliding-window eviction. The `turn_count_` (line 49) is tracked but never compared against any limit. For a long interactive session this means the accumulated context eventually exceeds the model's context window length, causing either silent truncation by the backend or an OOM crash inside the inference engine. The `history_` also has no per-session mutex, making it unsafe to call `chat()` from two threads on the same `AQLConversationContext` object.

**Implementation Notes:**
- `[x]` Add `std::size_t max_turns = 50` and `std::size_t max_history_tokens = 8192` to `AQLConversationContext::Config` (new struct); enforce in `chat()`: when either limit is reached, evict the oldest user+assistant message pair (preserve the system message)
- `[x]` Use the `TokenEstimator` abstraction (Feature 5) to count tokens before each `chat()` call; if adding the new user message would exceed `max_history_tokens`, evict oldest pairs first
- `[x]` Add a `std::mutex history_mutex_` to `AQLConversationContext::Impl` and hold it around all reads/writes to `history_` and `turn_count_`
- `[x]` Expose `AQLConversationContext::tokenCount() const` so callers can observe current usage
- `[x]` Unit-test: create a context with `max_turns=3`, drive 5 turns, assert `turn_count() == 3` and `history_.size() == 7` (system + 3×(user+assistant))

---

### 10 · Deduplicated Prompt-Building and Markdown-Stripping Logic
**Priority:** Low
**Target Version:** v1.7.0

**Problem (from code):** The same prompt-construction and post-processing logic is copy-pasted three times in `llm_aql_handler.cpp`:
- `translateNLToAQL()` (lines 978–1059): system prompt build + markdown fence strip + trim
- `translateNLToAQLStreaming()` (lines 1078–1148): identical prompt structure + identical fence-strip code
- `translateNLToAQLWithExamples()` (lines ~1330–1410): same prompt structure with examples added

Each copy independently strips backtick fences, trims whitespace, and performs `annotateErrors()` logging. A bug fix in one function must be applied to all three manually.

**Implementation Notes:**
- `[ ]` Extract `std::string LLMAQLHandler::buildNLToAQLSystemPrompt(const std::string& schema_context, const std::vector<FewShotExample>& examples) const` as a private helper; use `std::string::reserve()` with a pre-estimated capacity before the first append
- `[x]` Extract `std::string LLMAQLHandler::stripMarkdownFences(std::string raw) const` as a private static helper containing the `find("```")` / `find('\n')` / `substr` logic
- `[ ]` Extract `void LLMAQLHandler::logAnnotations(const std::vector<Annotation>& anns, const std::string& query_preview) const` to consolidate the three copies of the annotation-logging block
- `[ ]` Replace the three functions' duplicated code with calls to these helpers
- `[x]` Add a unit test that verifies `stripMarkdownFences("```aql\nFOR x IN c RETURN x\n```")` returns `"FOR x IN c RETURN x"`

---

### 11 · `AQLQueryBuilder` — Graph Traversal and DML Support
**Priority:** High
**Target Version:** v1.7.0

**Problem (from code):** `aql_query_builder.cpp:Impl::render()` and the public API (`include/aql/aql_query_builder.h`) support only `FOR`, `LET`, `FILTER`, `COLLECT`, `SORT`, `LIMIT`, `RETURN`. Graph traversal (`FOR v, e, p IN 1..N OUTBOUND start GRAPH g`), DML (`INSERT`, `UPDATE`, `REMOVE`, `UPSERT`, `REPLACE`), subquery expressions (`( FOR x IN ... RETURN x )`), and `WINDOW` analytics clauses are completely absent from the builder API. Any caller that needs these constructs must fall back to raw string concatenation, losing all validation and type-safety.

**Implementation Notes:**
- `[x]` Add `AQLQueryBuilder& forTraverse(const std::string& vertex_var, const std::string& edge_var, const std::string& path_var, const std::string& start, const std::string& graph, const std::string& direction = "OUTBOUND", int min_depth = 1, int max_depth = 1)` to the builder
- `[x]` Add `AQLQueryBuilder& insertInto(const std::string& collection, const std::string& doc_expr)`, `updateIn()`, `removeIn()`, `upsertIn()`, `replaceIn()` DML methods
- `[x]` Add `AQLQueryBuilder& window(const std::string& partition_expr, const std::string& window_spec)` for timeseries queries
- `[x]` Add `AQLQueryBuilder& subquery(const std::string& variable, const AQLQueryBuilder& inner)` that renders `LET variable = ( <inner> )`
- `[x]` Update `Impl::render()` to emit these new clauses in correct AQL clause-ordering position
- `[x]` Update `AQLQueryValidator` to check new clauses for common mistakes (e.g. `min_depth > max_depth`)
- `[x]` Add grammar-coverage tests: at least one test per new clause type

---

### 12 · Schema-Aware Semantic Validation in `AQLQueryValidator`
**Priority:** Medium
**Target Version:** v1.7.0

**Problem (from code):** `aql_query_validator.cpp` contains only six regex/string-scan checks: `checkLimitZero`, `checkCollectAfterSort`, `checkMissingReturn`, `checkMissingFor`, `checkAssignmentInFilter`, `checkMissingLimit`. There is no schema-aware validation: collection names in `FOR x IN <collection>` are never checked against known collections, field names in `FILTER x.<field>` are never checked against the schema, and type errors (e.g. arithmetic on a string field) are not detected. `AQLQueryBuilder` has a `setSchema()` method and `getFieldsForCollection()` but validator does not accept a schema parameter.

**Implementation Notes:**
- `[x]` Add `ValidationResult AQLQueryValidator::validate(const std::string& query, const AQLSchemaProvider& schema) const` overload that also performs schema-aware checks — implemented; takes `const std::vector<CollectionMetadata>& schema` (see `aql_query_validator.cpp:452`)
- `[x]` Implement `checkUnknownCollections()`: extract `FOR x IN <name>` identifiers using a regex; for each, call `schema.getCollectionMeta(name)`; if missing, add a `WARNING`-severity issue
- `[x]` Implement `checkUnknownFields()`: extract `<var>.<field>` accesses and check against the schema's known field list for each collection variable in scope — implemented (`aql_query_validator.cpp:368`)
- `[x]` Integrate with `AQLQueryBuilder::validate()` (line 243) which already calls the schema-less version — added `AQLQueryBuilder::validate(const std::vector<CollectionMetadata>& schema)` overload that delegates to the new `AQLQueryValidator::validate(builder, schema)` overload, which applies structural + schema-aware checks on the partial query
- `[x]` Add dedicated tests with a mock schema: query referencing a non-existent collection must produce a WARNING; query referencing a valid collection's known fields must produce no issues — tests in `test_aql_query_validator.cpp:439`

---

### 13 · Runtime-Overridable Validation and Timeout Limits
**Priority:** Medium
**Target Version:** v1.7.0

**Problem (from code):** `include/aql/llm_error_codes.h:ValidationLimits` (lines 201–224) defines all limits as compile-time `constexpr`:
```cpp
constexpr size_t MAX_PROMPT_LENGTH       = 128000;
constexpr size_t MAX_NL_QUERY_LENGTH     = 4096;
constexpr size_t MAX_SCHEMA_CONTEXT_LENGTH = 32768;
constexpr int    MAX_RAG_TOP_K           = 100;
constexpr int    DEFAULT_TIMEOUT_SECONDS = 300;
```
These values cannot be adjusted without recompilation, making it impossible to tune the system for different deployment profiles (embedded device vs. server cluster) without a build change. Similarly, `LLMTimeoutManager::TimeoutConfig` embeds default values in the struct definition rather than reading from a configuration file.

**Implementation Notes:**
- `[x]` Add a `ValidationLimitsConfig` struct (or extend an existing config struct) with the same fields as `ValidationLimits` but as instance members with the current `constexpr` values as defaults
- `[x]` Inject `ValidationLimitsConfig` into `LLMAQLHandler` via constructor; propagate to `sanitizePromptInput()` call sites (lines 967–970, 1073–1076, 1235–1238, 1261–1264)
- `[ ]` Load `ValidationLimitsConfig` and `LLMTimeoutManager::TimeoutConfig` from a TOML/JSON config section at startup; fall back to defaults when the section is absent
- `[x]` Expose `LLMAQLHandler::setValidationLimits(const ValidationLimitsConfig&)` and `LLMAQLHandler::setTimeoutConfig(const TimeoutConfig&)` for runtime adjustment
- `[ ]` Add a test that overrides `MAX_NL_QUERY_LENGTH=10` and confirms `translateNLToAQL("this is a long query beyond 10 chars", "")` throws `PROMPT_TOO_LONG`

---

### 14 · Hard-Coded LoRA Training Hyperparameters Should Be Config-Driven
**Priority:** Low
**Target Version:** v1.7.0

**Problem (from code):** `aql_lora_finetuner.cpp:AQLLoRAFinetuner::Config::Config()` (lines 549–558) sets:
```cpp
hyperparameters.batch_size     = 4;
hyperparameters.num_epochs     = 3;
hyperparameters.max_seq_length = 512;
hyperparameters.learning_rate  = 3e-4f;
hyperparameters.rank           = 8;
hyperparameters.alpha          = 16.0f;
hyperparameters.dropout        = 0.05f;
hyperparameters.warmup_steps   = 10;
```
These values are AQL-optimised starting points but there are no named constants, no documentation of the rationale behind each value, and no runtime path to override them via the AQL `LLM FINETUNE … WITH { … }` options map that is parsed elsewhere in the handler.

**Implementation Notes:**
- `[x]` Replace the magic numbers in `Config::Config()` with named `static constexpr` members (`kDefaultBatchSize = 4`, `kDefaultEpochs = 3`, etc.) with a one-line comment justifying each value
- `[ ]` Parse the `WITH { … }` options map passed to `LLM FINETUNE` in `llm_aql_handler.cpp` and forward relevant keys (`rank`, `alpha`, `epochs`, `learning_rate`, `batch_size`, `max_seq_length`) to `AQLLoRAFinetuner::Config` before calling `train()`
- `[x]` Add `Config::fromOptions(const std::unordered_map<std::string, std::string>&)` factory that constructs a `Config` from the AQL `WITH` map, with the existing defaults as fallback
- `[ ]` Validate hyperparameter ranges on construction: `rank` must be 1–256, `alpha` must be > 0, `dropout` must be in [0, 1), `learning_rate` must be > 0; throw `std::invalid_argument` on violation

---

### 15 · `DocsAssistantFunctions` Silent Failure Reporting
**Priority:** Medium
**Target Version:** v1.6.0

**Problem (from code):** `docs_assistant_functions.cpp` (lines 55–63) initialises the `DocsAssistant` in a try block but silently resets it to `nullptr` if `loadDatabase()` fails:
```cpp
if (!docs_assistant_->loadDatabase()) {
    // Failed to load, but don't throw - just mark as not ready
    docs_assistant_.reset();
}
```
The caller receives a `DocsAssistantFunctions` object that appears healthy but silently falls back to a degraded mode. There is no diagnostic surface: no log message at `WARN` level, no `isReady()` method that callers can check, and no way to distinguish "database not found" from "database failed to load". The same pattern occurs for `ThemisHelpLoRA` initialisation.

**Implementation Notes:**
- `[x]` Add a `DegradedReason` enum (`OK`, `DATABASE_NOT_FOUND`, `DATABASE_LOAD_FAILED`, `LORA_LOAD_FAILED`) and a `degraded_reason_` member to `DocsAssistantFunctions::Impl`
- `[ ]` Emit a `spdlog::warn` with a human-readable message before each `.reset()` call, including the exception message if one was caught
- `[ ]` Expose `bool DocsAssistantFunctions::isFullyReady() const` and `std::string DocsAssistantFunctions::degradedReason() const` in the public API
- `[ ]` Document in the header that degraded mode is expected in embedded deployments without a docs database, and explain which commands will fall back to LLM generation

---

## Performance Optimizations

### Continuous Batching (Iteration-Level Batching)
**Priority:** High
**Target Version:** v1.6.0

Process multiple inference requests in the same batch for higher throughput.

**Current:** Sequential processing of requests
**Target:** Continuous batching with dynamic sequence insertion/removal

**Expected Improvement:** 2–5× throughput increase

---

### Speculative Decoding
**Priority:** High
**Target Version:** v1.6.0

Use small draft model + large target model for faster inference.

**Approach:**
- Small model (1–3B) generates tokens quickly
- Large model (70B+) validates/corrects in parallel
- Accept speculative tokens when correct

**Expected Improvement:** 2–3× latency reduction

---

### KV Cache Optimization
**Priority:** Medium
**Target Version:** v1.7.0

Optimize key-value cache memory usage and reuse.

**Techniques:**
- PagedAttention for efficient memory allocation
- KV cache compression (quantization)
- Prefix caching for common prompts
- Multi-request KV cache sharing

**Expected Improvement:** 50% memory reduction, 30% faster repeated queries

---

### Embedding Cache
**Priority:** High
**Target Version:** v1.6.0

Cache embeddings to avoid recomputation.

**Features:**
- Document content hash → embedding mapping
- LRU eviction with configurable size
- Automatic cache warming for frequently accessed documents
- Distributed cache for multi-node deployments

**Expected Improvement:** 10–100× faster for cached embeddings

---

### Model Sharding Across GPUs
**Priority:** Medium
**Target Version:** v1.7.0

Distribute large models across multiple GPUs.

**Techniques:**
- Tensor parallelism for within-layer distribution
- Pipeline parallelism for across-layer distribution
- Automatic sharding based on available GPUs

**Expected Improvement:** Support for 70B+ models on consumer GPUs

---

## Multi-Modal LLM Support
**Priority:** High
**Target Version:** v1.7.0

Extend LLM commands to support images, audio, and video inputs.

**Features:**
- Image understanding (vision models like LLaVA, CogVLM)
- Audio transcription and understanding (Whisper integration)
- Video frame analysis
- Multi-modal embeddings (CLIP, ImageBind)

**Syntax:**
```aql
-- Image understanding
LLM INFER 'Describe this image'
  IMAGE FROM 'images/photo.jpg'
  MODEL 'llava-v1.6-34b'

-- Audio transcription + summarization
LLM INFER 'Summarize this audio recording'
  AUDIO FROM 'recordings/meeting.mp3'
  MODEL 'whisper-large-v3'
  THEN MODEL 'llama-3-70b'
```

---

## Advanced RAG Techniques
**Priority:** High
**Target Version:** v1.6.0

Implement state-of-the-art RAG enhancements.

**Techniques:**
- **HyDE (Hypothetical Document Embeddings)**: Generate hypothetical answers, search with their embeddings
- **Multi-Query RAG**: Generate multiple search queries for comprehensive coverage
- **Re-ranking**: Two-stage retrieval with cross-encoder re-ranking
- **Parent Document Retrieval**: Retrieve chunks, return full parent documents
- **RAG Fusion**: Combine results from multiple retrieval strategies

**Syntax:**
```aql
LLM RAG 'What are quantum computing applications?'
  SEARCH IN knowledge_base
  STRATEGY 'hyde'
  TOP 10
  RERANK true
```

---

## Fine-Tuning Pipeline Integration
**Priority:** Medium
**Target Version:** v1.7.0

In-database model fine-tuning with LoRA.

**Syntax:**
```aql
LLM FINETUNE
  BASE_MODEL 'llama-3-8b'
  DATASET medical_training_set
  EPOCHS 3
  LEARNING_RATE 1e-4
  LORA_RANK 16
  OUTPUT 'medical-llama-3-8b'
```

---

## Agent Framework Integration
**Priority:** Medium
**Target Version:** v1.8.0

Multi-step reasoning with tool calling and planning.

**Syntax:**
```aql
LLM AGENT CREATE data_analyst
  MODEL 'llama-3-70b-instruct'
  TOOLS [
    {name: 'query_database', aql: 'FOR doc IN @collection FILTER @condition RETURN doc'},
    {name: 'calculate', fn: 'MATH.eval'}
  ]
  MAX_ITERATIONS 10
```

---

## Refactoring Opportunities

### Unified LLM Backend Interface
**Priority:** High
**Target Version:** v1.6.0

Abstract LLM backend for multiple inference engines.

**Proposed:**
```cpp
class ILLMBackend {
public:
    virtual Result<std::string> infer(const InferenceRequest& req) = 0;
    virtual Result<std::vector<float>> embed(const std::string& text) = 0;
};

class LlamaCppBackend : public ILLMBackend { /* ... */ };
class VLLMBackend     : public ILLMBackend { /* ... */ };
class OllamaBackend   : public ILLMBackend { /* ... */ };
class OpenAIBackend   : public ILLMBackend { /* ... */ };
```

---

### Streaming Response API ✅ SHIPPED (v1.7.0)
**Priority:** Medium
**Target Version:** v1.7.0 — **Implemented**

SSE streaming for AQL explanations is shipped via `streamExplainAQLAsSSE()`.
The generic `AQLTokenStream` API is also shipped (`include/aql/aql_token_stream.h`).

**Shipped: Generic `AQLTokenStream` (v1.7.0)**
```cpp
auto stream = std::make_shared<AQLTokenStream>();
for (const auto& token : *stream) { std::cout << token; }
stream->cancel();  // cooperative cancellation
```

---

### Prompt Template Engine
**Priority:** Medium
**Target Version:** v1.7.0

Structured prompt templates with variable substitution.

**Proposed:**
```cpp
PromptTemplate rag_template(R"(
Context:
{{#each documents}}
- {{this.title}}: {{this.content}}
{{/each}}

Question: {{question}}
)");
auto prompt = rag_template.render({
    {"documents", retrieved_docs},
    {"question", user_question}
});
```

---

### Query Optimization AI
**Priority:** Medium
**Target Version:** v1.7.0

LLM-powered query optimization suggestions.

**Syntax:**
```aql
LLM OPTIMIZE QUERY
  QUERY @slowQuery
  ANALYZE_PLAN true
  SUGGEST_INDEXES true
  RETURN ALTERNATIVES 3
```

---

## Known Issues

### Issue #1: Long Context Handling
**Severity:** Medium
**Reported:** v1.5.0

Models with limited context windows (2K–8K tokens) struggle with long documents.

**Workaround:** Chunk documents and retrieve top-k chunks

**Fix:** Implement sliding window attention, sparse attention, or long-context models (Llama-3.1 with 128K context)

**Planned Fix:** v1.6.0

---

### Issue #2: RAG Accuracy
**Severity:** Medium
**Reported:** v1.5.1

Retrieved documents sometimes don't contain relevant information.

**Workaround:** Increase retrieval count, improve chunking strategy

**Fix:** Hybrid search (vector + BM25), re-ranking, query expansion

**Planned Fix:** v1.6.0

---

### Issue #3: Model Loading Performance
**Severity:** Low
**Reported:** v1.5.0

Large models take 10–30 seconds to load.

**Workaround:** Pre-load models at server startup

**Fix:** Memory-mapped model loading, model caching in shared memory, lazy weight loading

**Planned Fix:** v1.6.0

---

### Issue #4: Embedding Dimension Mismatch
**Severity:** Medium
**Reported:** v1.5.2

Switching embedding models breaks existing indexes.

**Workaround:** Re-index all documents with new model

**Fix:** Dimension adapters (PCA, autoencoder), multi-model index support, migration tools

**Planned Fix:** v1.7.0

---

## Research Areas

### Mixture of Experts (MoE) Optimization
**Focus:** Efficient sparse MoE inference

Optimize inference for MoE models (Mixtral, Grok):
- Expert caching
- Expert selection prediction
- Dynamic expert routing

**Research Questions:**
- How to optimize expert selection for latency?
- Can we predict which experts will be needed?
- What's the optimal expert count for different tasks?

---

### Quantization-Aware RAG
**Focus:** Balancing quality vs performance

Explore quantization impact on RAG:
- 4-bit vs 8-bit quantization for embeddings
- Mixed precision: FP16 retrieval, INT8 generation
- Adaptive quantization based on query complexity

---

### Personalized RAG
**Focus:** User-specific context and preferences

Adapt RAG to individual users:
- User embedding profiles
- Personalized retrieval ranking
- Context-aware response generation

---

### Cross-Lingual RAG
**Focus:** Multi-language query and retrieval

Support queries and documents in multiple languages:
- Multilingual embeddings (mBERT, XLM-R)
- Translation-based RAG
- Language-aware re-ranking

---

## Migration Paths

### v1.5.x → v1.6.x: Unified LLM Backend
**Breaking Changes:** Backend interface changes

**Migration Steps:**
1. Update to v1.6.0
2. Replace direct `LlamaCppBackend` with factory
3. Test with existing models

**Timeline:** 6 months deprecation period

---

### v1.6.x → v1.7.x: Streaming API ✅ Done
**Breaking Changes:** None (additive)

**Shipped (v1.7.0):**
- `AQLTokenStream` (`include/aql/aql_token_stream.h`) – header-only thread-safe token streaming
- `IAgent` / `ReActAgent` (`include/aql/aql_agent.h`, `src/aql/aql_agent.cpp`) – ReAct agent with tool calling

---

### v1.7.x → v1.8.x: Agent Framework
**Breaking Changes:** None (new features)

**Shipped (v1.8.0):**
- `MultiModalInferRequest` + `MultiModalInput` + `ModalityType` (`include/aql/multimodal_infer_request.h`) – MIME-validated multi-modal request extending `llm::InferenceRequest`
- `IAsyncLLMBackend` + `ThreadPoolAsyncLLMBackend` (`include/aql/iasync_llm_backend.h`) – non-blocking async inference interface

---

## Community Contributions Welcome

### High-Impact, Beginner-Friendly
- `[ ]` Additional prompt templates for common use cases
- `[ ]` Documentation improvements and examples
- `[ ]` Embedding model benchmarks
- `[ ]` RAG quality evaluation metrics

### Medium Complexity
- `[ ]` Additional LLM backend implementations (VLLM, Ollama, OpenAI)
- `[x]` Streaming response API (`include/aql/aql_token_stream.h`, v1.7.0)
- `[ ]` HyDE RAG implementation
- `[ ]` Cross-encoder re-ranking

### Advanced Topics
- `[ ]` Fine-tuning pipeline integration
- `[x]` Agent framework with tool calling (`include/aql/aql_agent.h`, `src/aql/aql_agent.cpp`, v1.7.0)
- `[ ]` Speculative decoding
- `[x]` Multi-modal LLM support (`include/aql/multimodal_infer_request.h`, v1.8.0)
- `[ ]` Distributed model sharding

**Contribution Guide:** See [CONTRIBUTING.md](../../CONTRIBUTING.md)

---

## Implementation Notes

### AQLTokenStream (v1.7.0)
- **Header-only** (`include/aql/aql_token_stream.h`); no `.cpp` file required.
- Uses `std::queue<std::string>` protected by `std::mutex` + `std::condition_variable` for blocking consumer.
- `cancelled_` is `std::atomic<bool>` so producers can check `isCancelled()` without holding the mutex.
- `push()` after `cancel()` or `close()` is silently discarded (no exception, no undefined behaviour).
- Destructor calls `close()` to unblock any waiting consumer thread — prevents deadlocks on early destruction.
- No heap allocation per token (queue node is small-string-optimised by the STL implementation).

### ReActAgent (v1.7.0)
- **Pimpl pattern** keeps `ReActAgent.h` free of internal implementation details and provides ABI stability.
- Tool registry is `std::unordered_map<std::string, AgentTool>` for O(1) lookup.
- LLM prompt format follows the standard ReAct template: `Thought:` / `Action:` / `Action Input:` / `Observation:` / `Final Answer:`.
- Tool executor exceptions are caught inside `invokeTool()` and returned as a JSON error object — they never propagate to `execute()` callers.
- `Action Input:` is parsed as JSON; if parsing fails the raw string is wrapped as `{"input": "<raw>"}`.
- `verbose = true` logs each reasoning step at `spdlog::debug` level.

### MultiModalInferRequest (v1.8.0)
- **Header-only** (`include/aql/multimodal_infer_request.h`); no `.cpp` file required.
- `ModalityType` is a scoped enum with four values: `TEXT`, `IMAGE`, `AUDIO`, `VIDEO`.
- `MultiModalInput::validate()` checks MIME type against per-modality allowlists stored as `static const std::unordered_set<std::string>` — O(1) lookup.
- Empty binary payloads (`std::vector<uint8_t>{}`) for IMAGE/AUDIO/VIDEO are rejected with `std::invalid_argument`; file-path payloads are not checked for on-disk existence.
- `MultiModalInferRequest::addInput()` calls `validate()` before appending to `inputs`, ensuring the vector never contains an invalid entry.
- Extends `llm::InferenceRequest` so all existing inference parameters (prompt, model_id, temperature, stop_sequences, …) are available without duplication.

### IAsyncLLMBackend (v1.8.0)
- **Header-only** (`include/aql/iasync_llm_backend.h`); no `.cpp` file required.
- `IAsyncLLMBackend` is a pure abstract class. New virtual methods may only be appended at the end of the vtable.
- `ThreadPoolAsyncLLMBackend` wraps any `ILLMPlugin` and dispatches each call via `std::async(std::launch::async, …)`. Plugin exceptions are caught and returned as `Err<T>(ERR_UNKNOWN, message)` — they never propagate through the future.
- `supportsMultiModal()` delegates to `ILLMPlugin::getCapabilities().supports_multimodal`.
- `InferenceRequest` is copied into the `std::async` lambda to prevent dangling reference when the caller's request object is destroyed before the async task completes.

---

## Test Strategy

- **Unit tests** (≥ 90 % line coverage): lexer tokenisation for all token types including edge cases (empty input, max-length identifiers, Unicode identifiers); parser round-trip for every grammar production rule
- **Integration tests**: execute a suite of ≥ 500 canonical AQL queries (SELECT, INSERT, UPDATE, LLM INFER, LLM RAG, sub-queries, CTEs) against an in-memory dataset; verify result correctness and row counts
- **Property-based tests** (libFuzzer + grammar-aware fuzzer): ≥ 10 M random query strings; parser must never crash or produce undefined behaviour — only structured errors
- **LLM dispatch tests** (mock LLM backend): verify async callback delivery within 5 s timeout; verify cancellation propagation within 200 ms
- **Optimiser regression tests**: ensure rewritten AST produces identical results to original AST on the same dataset for ≥ 100 query pairs
- **Coverage gate**: CI blocks merge if total line coverage drops below 85 %

---

## Performance Targets

- Lexer tokenisation: ≥ 50 MB/s on a single core for ASCII query text
- Parser AST construction: ≤ 10 ms for a 64 KB query on a modern 3 GHz CPU
- Full evaluator round-trip (parse + execute) for a 10-table join over 100 000 rows: ≤ 500 ms
- LLM command async dispatch overhead (excluding model inference): ≤ 5 ms per command
- Query optimiser rewrite pass: ≤ 2 ms per 1 000 AST nodes
- Memory allocation per parsed query: ≤ 10 MB for a 64 KB query text
- Batch NL-to-AQL (10 requests, mock LLM 50 ms each): ≤ 150 ms wall-time at concurrency ≥ 4

---

## Security / Reliability

- Lexer and parser are fuzz-hardened: CI runs libFuzzer for ≥ 1 hour per release; no crashes permitted
- AST node cap (100 000 nodes) enforced to prevent memory exhaustion via adversarial deeply-nested queries
- LLM prompt inputs sanitised: prompt injection patterns blocked in `sanitizePromptInput()` (`llm_aql_handler.cpp` lines 78–154); null bytes and known override phrases rejected
- Post-LLM-generation AQL must pass `AQLQueryValidator::validate()` before being returned to callers (Feature 1)
- All spawned timeout threads must terminate or be joined within `timeout + 500 ms`; no raw `detach()` after timeout (Feature 2)
- Circuit breakers scoped per operation type (INFER / RAG / EMBED) so failure in one domain cannot block others (Feature 3)
- Evaluator enforces per-query CPU and memory resource limits configurable at context level
- Query results never include raw error stack traces in the public API response; internal details logged server-side only

---

## Identified Gaps (from AI_ML_IMPACT_ASSESSMENT.md)

### Gap 4b — AQLAgent: Session Token-Budget Cap (Target: Q3 2026)

**Source:** `AI_ML_IMPACT_ASSESSMENT.md §7, Gap 4 (Severity: Medium/S1)`
**See also:** `src/rag/FUTURE_ENHANCEMENTS.md §Gap 4` (AgenticRAG counterpart).

**Problem:** `AQLAgent` (`src/aql/aql_agent.cpp`) orchestrates multi-step AQL
construction using an LLM with an iteration limit, but has no upper bound on the
total tokens consumed across all steps.  An adversarial or poorly-constrained query
can drive the agent into long token chains that exhaust shared LLM capacity without
triggering the existing `CircuitBreaker` (which only fires on repeated backend errors,
not on cost overrun).

**Solution:**
- Add `AQLAgentConfig::max_session_tokens` (default: 8192; 0 = disabled).
- Track cumulative `InferenceResponse::tokens_generated` across agent steps.
- When budget is exceeded, break the agent loop and return an `AQLAgentResult` with
  `status=BUDGET_EXCEEDED` and the partial AQL generated so far (or an empty AQL with
  the error flag set, depending on partial-result policy).
- Wire the same `LLMTokenBudgetManager` (from `llm/FUTURE_ENHANCEMENTS.md §Gap 6`) if
  available, so per-session limits and global limits are enforced jointly.

**Inputs:** Cumulative token count from `InferenceResponse`; `max_session_tokens` config.
**Outputs:** `AQLAgentResult::status == BUDGET_EXCEEDED` when limit reached.
**Constraints:** No change to existing caller contracts when `max_session_tokens=0`.
**Errors:** Budget exceeded → partial result or empty result with error status.
**Tests:** 2 unit tests — budget exceeded mid-agent-loop (stops early with status flag);
`max_session_tokens=0` disables enforcement.
**Perf target:** One integer addition per step; no measurable overhead.

---

*Last Updated: June 2026*
*Module Version: v1.5.x → v1.6.0 target*
*Next Review: v1.6.0 Release*
