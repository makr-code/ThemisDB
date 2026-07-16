> **Build:** `cmake --preset release && cmake --build build/release`

# ThemisDB AQL Headers

> **See also:**
> [src/aql/README.md](../../src/aql/README.md) — implementation details · components · troubleshooting
> | [src/aql/ROADMAP.md](../../src/aql/ROADMAP.md) — implementation phases · production-readiness checklist
> | [src/aql/FUTURE_ENHANCEMENTS.md](../../src/aql/FUTURE_ENHANCEMENTS.md) — planned features with interface specs

## Module Purpose

This directory contains the header files for AQL (Advanced Query Language) specialized components, specifically focusing on LLM integration and AI-assisted documentation. These headers define the interfaces for natural language query processing, LLM command handling, and documentation generation.

### About AQL (Advanced Query Language)

**AQL** is ThemisDB's multi-paradigm, declarative query language inspired by ArangoDB's AQL (ArangoQL) but extended for comprehensive multi-model database support:

**Language Characteristics:**
- **Declarative Syntax**: SQL-like with FOR-FILTER-SORT-RETURN pattern
- **Multi-Model**: Single language for relational, document, graph, vector, spatial, and timeseries data
- **Composable**: Build complex hybrid queries from simple operations
- **Extensible**: 100+ built-in functions with plugin architecture

**ArangoDB Compatibility:**
- ✅ Core syntax compatible (FOR, FILTER, SORT, LIMIT, RETURN, LET, COLLECT)
- ✅ Graph traversal syntax preserved
- ➕ Extended with vector similarity functions
- ➕ Enhanced geospatial support (ST_* functions)
- ➕ Native LLM integration (INFER, RAG, EMBED)
- ➕ Timeseries and window functions
- ➕ Process mining and ethics functions

## Scope

**In Scope:**
- LLM command handler interfaces
- Documentation assistant function declarations
- Natural language to AQL translation interfaces
- Query explanation and profiling interfaces
- LLM backend abstractions

**Out of Scope:**
- Core AQL parsing (see include/query/)
- Query execution engine (see include/query/)
- Storage interfaces (see include/storage/)
- Index management (see include/index/)

## Header Files

| Header | Primary Class / Interface |
|--------|--------------------------|
| `llm_aql_handler.h` | `LlmAqlHandler` — routes LLM AQL commands (INFER, RAG, EMBED) to backend |
| `iasync_llm_backend.h` | `IAsyncLLMBackend` — async LLM backend abstraction |
| `docs_assistant_functions.h` | `DocsAssistant` — NL-to-AQL translation and query explanation |
| `aql_agent.h` | `AQLAgent` — autonomous AQL query agent |
| `aql_autocomplete.h` | `AQLAutocomplete` — token-level AQL autocomplete engine |
| `aql_confidence_scorer.h` | `AQLConfidenceScorer` — scores NL-to-AQL translation confidence |
| `aql_conversation_context.h` | `AQLConversationContext` — multi-turn query conversation state |
| `aql_fewshot_example_library.h` | `AQLFewshotExampleLibrary` — curated few-shot examples for NL translation |
| `aql_ingestion_bridge.h` | `AQLIngestionBridge` — connects AQL queries to ingestion pipeline |
| `aql_lora_finetuner.h` | `AQLLoRAFinetuner` — LoRA fine-tuning interface for AQL model adaptation |
| `aql_migration_assistant.h` | `AQLMigrationAssistant` — assists with AQL query migration between versions |
| `aql_model_router.h` | `AQLModelRouter` — routes queries to appropriate LLM backend |
| `aql_optimizer_advisor.h` | `AQLOptimizerAdvisor` — suggests query optimizations |
| `aql_query_builder.h` | `AQLQueryBuilder` — programmatic AQL query construction |
| `aql_query_diff_explainer.h` | `AQLQueryDiffExplainer` — explains semantic differences between queries |
| `aql_query_template_library.h` | `AQLQueryTemplateLibrary` — parameterized query template registry |
| `aql_query_validator.h` | `AQLQueryValidator` — syntactic and semantic query validation |
| `aql_rollback_suggester.h` | `AQLRollbackSuggester` — suggests rollback queries for mutations |
| `aql_schema_provider.h` | `AQLSchemaProvider` — exposes collection schemas for query context |
| `aql_syntax_highlighter.h` | `AQLSyntaxHighlighter` — token classification for syntax highlighting |
| `aql_token_stream.h` | `AQLTokenStream` — streaming token iterator for AQL parsing |
| `classify_bridge.h` | `ClassifyBridge` — bridges classification results into AQL pipelines |
| `llm_error_codes.h` | `LLMErrorCode` — structured error codes for LLM operations |
| `llm_metrics_collector.h` | `LLMMetricsCollector` — latency, token and error metrics |
| `llm_timeout_manager.h` | `LLMTimeoutManager` — per-request and global timeout enforcement |
| `llm_token_estimator.h` | `LLMTokenEstimator` — estimates token count before inference |
| `multimodal_infer_request.h` | `MultimodalInferRequest` — request structure for image+text inference |

## Key Components

### LlmAqlHandler
**Location:** `llm_aql_handler.h`, `../../src/aql/llm_aql_handler.cpp`

Header interface for LLM-specific AQL command handling.

**Class Definition:**
```cpp
class LlmAqlHandler {
public:
    explicit LlmAqlHandler(std::shared_ptr<ILLMBackend> backend);

    // LLM command execution
    Result<json> handleLLMCommand(const LLMCommand& cmd);

    // Individual command handlers
    Result<std::string> handleInfer(const InferRequest& req);
    Result<std::string> handleRAG(const RAGRequest& req);
    Result<std::vector<float>> handleEmbed(const EmbedRequest& req);
    Result<void> handleModelLoad(const ModelLoadRequest& req);
    Result<void> handleLoraLoad(const LoraLoadRequest& req);
    Result<json> handleStats(const StatsRequest& req);
    Result<void> handleCacheClear();

    // Natural language translation
    Result<std::string> translateNLToAQL(const std::string& nl_query);

private:
    std::shared_ptr<ILLMBackend> backend_;
    std::shared_ptr<ModelRegistry> models_;
    std::shared_ptr<LoraRegistry> loras_;
    std::shared_ptr<PromptCache> cache_;
};
```

**Request Structures:**
```cpp
struct InferRequest {
    std::string prompt;
    std::string model_alias;
    std::optional<std::string> lora_alias;
    int max_tokens = 512;
    float temperature = 0.7;
    float top_p = 0.9;
    int top_k = 40;
    std::vector<std::string> stop_sequences;
};

struct RAGRequest {
    std::string query;
    std::string collection;
    int top_k = 10;
    float similarity_threshold = 0.7;
    std::string model_alias;
    std::optional<std::string> filter_expr;
    float temperature = 0.3;  // Lower for factual responses
};

struct EmbedRequest {
    std::string text;
    std::string model_alias;
    bool normalize = true;
};

struct ModelLoadRequest {
    std::string model_path;
    std::string alias;
    int gpu_layers = 0;
    size_t context_size = 4096;
    std::map<std::string, std::string> params;
};

struct LoraLoadRequest {
    std::string adapter_path;
    std::string alias;
    std::string base_model;
    float scale = 1.0;
};
```

**Thread Safety:**
- All methods are thread-safe
- Model loading is serialized internally
- Inference can be concurrent up to batch size

### DocsAssistantFunctions
**Location:** `docs_assistant_functions.h`, `../../src/aql/docs_assistant_functions.cpp`

AI-powered documentation and query assistance.

**Class Definition:**
```cpp
class DocsAssistant {
public:
    explicit DocsAssistant(std::shared_ptr<ILLMBackend> llm);

    // Natural language to AQL translation
    Result<std::string> translateToAQL(const std::string& nl_query);

    // Query explanation
    Result<std::string> explainQuery(const std::string& aql_query);

    // Function documentation lookup
    Result<std::string> getFunctionDocs(const std::string& function_name);

    // Query optimization suggestions
    Result<std::vector<std::string>> suggestOptimizations(const std::string& query);

    // Example query generation
    Result<std::vector<std::string>> generateExamples(const std::string& description);

    // Schema recommendation
    Result<json> recommendSchema(const std::string& description);

private:
    std::shared_ptr<ILLMBackend> llm_;
    std::shared_ptr<FunctionRegistry> functions_;
    std::shared_ptr<SemanticCache> cache_;
};
```

**Usage Example:**
```cpp
#include "aql/docs_assistant_functions.h"

DocsAssistant assistant(llm_backend);

// Translate natural language
auto aql = assistant.translateToAQL("Find users in Seattle older than 30");
// Returns: FOR user IN users FILTER user.city == "Seattle" AND user.age > 30 RETURN user

// Explain query
auto explanation = assistant.explainQuery(aql.value());

// Get function documentation
auto docs = assistant.getFunctionDocs("SIMILARITY");
```

## Additional Components

### AQLQueryDiffExplainer
**Location:** `aql_query_diff_explainer.h`, `../../src/aql/aql_query_diff_explainer.cpp`

Clause-level structural diff between two AQL query strings. No LLM required; runs in O(n).

```cpp
#include "aql/aql_query_diff_explainer.h"

themis::aql::AQLQueryDiffExplainer explainer;
auto result = explainer.explain(query_a, query_b);
if (!result.is_equivalent) {
    for (const auto& d : result.diffs)
        std::cout << d.explanation << '\n';
}
```

**Types:** `QueryDiffEntry` (kind, clause_a, clause_b, explanation), `QueryDiffResult`
(diffs, summary, is_equivalent).

---

### AQLRollbackSuggester
**Location:** `aql_rollback_suggester.h`, `../../src/aql/aql_rollback_suggester.cpp`

Derives a compensating rollback AQL query for mutating statements (INSERT / UPDATE / REPLACE /
REMOVE / UPSERT). Rule-based, O(n), no LLM dependency.

```cpp
#include "aql/aql_rollback_suggester.h"

themis::aql::AQLRollbackSuggester suggester;
auto s = suggester.suggest(
    "FOR u IN users FILTER u.role == 'guest' REMOVE u IN users");
// s.is_automatic == true
// s.rollback_query: "INSERT @snapshot INTO users"
// s.caveat: "Pre-mutation snapshot required"
```

**Types:** `MutationType` enum, `RollbackSuggestion` (is_automatic, rollback_query,
mutation_type, collection, caveat, manual_steps), `IAQLRollbackSuggester`.

---

### AQLIngestionBridge
**Location:** `aql_ingestion_bridge.h`, `../../src/aql/aql_ingestion_bridge.cpp`

Connects AQL INSERT/UPSERT document payloads to the ingestion `WorkflowEngine`. Enriches
documents with extracted entities (`"_entities"`) and optionally writes to a graph store.

```cpp
#include "aql/aql_ingestion_bridge.h"

auto bridge = std::make_shared<themis::aql::AQLIngestionBridge>(toolbox, graph_writer);
handler.setIngestionBridge(bridge);
// Enrichment is automatic for INSERT/UPSERT with a "text" field.
```

**Key methods:** `enrichInsertPayload(json&)`, `extractEntitiesForContext(text)`,
`buildEntityContext(entities)` (static).

---

### AQLModelRouter
**Location:** `aql_model_router.h`, `../../src/aql/aql_model_router.cpp`

Routes an AQL query to the best-matching LLM backend by detecting query type keywords
(VECTOR, GRAPH, GEO, FULLTEXT, TIMESERIES, RELATIONAL, PROCESS).

```cpp
#include "aql/aql_model_router.h"

themis::aql::AQLModelRouter router;
router.registerRoute({themis::aql::QueryModelType::VECTOR, "embed-model", 100, true});
router.registerRoute({themis::aql::QueryModelType::RELATIONAL, "llama-3-8b", 10, true});

auto route = router.route("FOR d IN docs LET s = SIMILARITY(d.emb, @q) RETURN d");
// route->model_alias == "embed-model"
```

**Types:** `QueryModelType` enum, `ModelRoute` (model_type, model_alias, priority, enabled),
`IModelRouter`, `AQLModelRouter`.

---

### ClassifyBridge / IClassifyFn
**Location:** `classify_bridge.h`, `../../src/aql/classify_bridge.cpp`

Defines `IClassifyFn` for zero-shot text classification, a `NullClassifyFn` no-op fallback,
and `ClassifyResult`. Used by `DocsAssistantFunctions::setClassifier()` to enable native NLP
intent detection without LLM round-trips.

```cpp
#include "aql/classify_bridge.h"
#include "aql/docs_assistant_functions.h"

auto clf = std::make_shared<MyClassifyFn>();
docs_assistant.setClassifier(clf.get());
// detectIntentWithNativeNLP("how do I create an index?") -> "configuration"
```

---

### LLMAQLEmbeddingBridge
**Location:** `llm_aql_embedding_bridge.h`, `../../src/aql/llm_aql_embedding_bridge.cpp`

Adapts `LLMAQLHandler::executeEmbed()` to `IEmbeddingProvider`, enabling semantic few-shot
selection in `AQLFewShotExampleLibrary`.

```cpp
#include "aql/llm_aql_embedding_bridge.h"

auto bridge = handler.makeEmbeddingBridge();
library.setEmbeddingProvider(bridge.get());
library.rebuildEmbeddingIndex();
// Semantic (cosine) ranking now active in translateNLToAQLWithExamples()
```

---

### AQLAgent / ReActAgent
**Location:** `aql_agent.h`, `../../src/aql/aql_agent.cpp`

Multi-step reasoning agent implementing the ReAct (Reasoning+Acting) pattern. Iterates
Thought→Action→Observation cycles, calling registered `AgentTool` functions until the LLM
emits "Final Answer:" or `max_iterations` is reached.

```cpp
#include "aql/aql_agent.h"

themis::aql::AgentConfig cfg{"llama-3-8b", 5};
themis::aql::ReActAgent  agent(handler, cfg);
agent.registerTool({"query_db", "Run AQL query", schema, executor});

auto result = agent.run("Find top 3 most active users last week");
if (result.succeeded)
    std::cout << result.final_answer;
```

**Types:** `AgentTool`, `AgentConfig`, `ReasoningStep`, `AgentResult`, `IAgent`.

---

### IAsyncLLMBackend / ThreadPoolAsyncLLMBackend
**Location:** `iasync_llm_backend.h`

Non-blocking `std::future<Result<T>>` inference interface wrapping any `ILLMBackend`. The
`ThreadPoolAsyncLLMBackend` adapter dispatches work to a configurable thread pool.

```cpp
#include "aql/iasync_llm_backend.h"

auto async_backend = std::make_shared<themis::aql::ThreadPoolAsyncLLMBackend>(
    sync_backend, /*threads=*/4);

auto future = async_backend->inferAsync(req);
// … do other work …
auto result = future.get();
```

---

### MultiModalInferRequest
**Location:** `multimodal_infer_request.h`

Request structure for image+text (and audio/video) inference. Each `MultiModalInput` carries
a MIME-typed bytes blob validated at construction.

```cpp
#include "aql/multimodal_infer_request.h"

themis::aql::MultiModalInferRequest req;
req.text_prompt = "Describe the chart";
req.inputs.push_back(themis::aql::MultiModalInput::fromFile(
    "/tmp/chart.png", "image/png"));
// Pass to IAsyncLLMBackend::inferMultiModalAsync(req)
```

## Architecture

### LLM Integration Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    AQL Query with LLM                        │
│  FOR doc IN collection LET emb = LLM EMBED(doc.text) ...   │
└─────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────┐
│                    AQL Parser (query module)                 │
│  Detects LLM command syntax, creates LLMCommand AST node    │
└─────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────┐
│                    LlmAqlHandler (aql module)                │
│  Routes to appropriate handler (infer/rag/embed/model)      │
└─────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────┐
│                    ILLMBackend Interface                     │
│  Abstract backend (LlamaCpp, VLLM, Ollama, OpenAI)         │
└─────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────┐
│                    LLM Runtime (llama.cpp)                   │
│  GGUF model loading, inference, GPU acceleration           │
└─────────────────────────────────────────────────────────────┘
```

### RAG Pipeline

```
User Query → Embedding → Vector Search → Context Assembly → LLM → Response
     ↓            ↓            ↓              ↓            ↓        ↓
  NL Text    Embed Model   HNSW Index    Top-K Docs    Generate  JSON
```

### Type Hierarchy

```cpp
// Base backend interface
class ILLMBackend {
public:
    virtual Result<std::string> infer(const InferRequest& req) = 0;
    virtual Result<std::vector<float>> embed(const std::string& text) = 0;
    virtual Result<ModelInfo> getModelInfo(const std::string& alias) = 0;
};

// Concrete implementations
class LlamaCppBackend : public ILLMBackend { /* ... */ };
class VLLMBackend : public ILLMBackend { /* ... */ };
class OllamaBackend : public ILLMBackend { /* ... */ };
class OpenAIBackend : public ILLMBackend { /* ... */ };

// Model and adapter registries
class ModelRegistry {
public:
    void registerModel(const std::string& alias, std::shared_ptr<Model> model);
    std::shared_ptr<Model> getModel(const std::string& alias);
    std::vector<std::string> listModels();
};

class LoraRegistry {
public:
    void registerAdapter(const std::string& alias, std::shared_ptr<LoraAdapter> adapter);
    std::shared_ptr<LoraAdapter> getAdapter(const std::string& alias);
};
```

## Integration Points

### With Query Module
```cpp
// In query execution
if (node->type == ASTNodeType::LLMCommand) {
    LlmAqlHandler handler(llm_backend);
    return handler.handleLLMCommand(*node->llm_command);
}
```

### With Index Module
```cpp
// RAG uses vector index
VectorIndexManager vector_idx(storage);
auto results = vector_idx.search(embedding, k);
```

### With Storage Module
```cpp
// Store embeddings
storage->put("llm:embedding:" + doc_id, embedding_json);

// Store model metadata
storage->put("llm:model:" + model_id, model_metadata);
```

## API/Usage Examples

### Basic LLM Commands

```cpp
#include "aql/llm_aql_handler.h"

LlmAqlHandler handler(llm_backend);

// Text generation
InferRequest infer_req;
infer_req.prompt = "Explain quantum computing";
infer_req.model_alias = "llama-3-8b";
infer_req.max_tokens = 500;
infer_req.temperature = 0.7;

auto result = handler.handleInfer(infer_req);
if (result) {
    std::cout << "Response: " << result.value() << std::endl;
}

// Embedding generation
EmbedRequest embed_req;
embed_req.text = "The quick brown fox";
embed_req.model_alias = "all-minilm-l6-v2";

auto embedding = handler.handleEmbed(embed_req);
if (embedding) {
    std::cout << "Embedding dims: " << embedding.value().size() << std::endl;
}
```

### RAG Query

```cpp
// Retrieval-Augmented Generation
RAGRequest rag_req;
rag_req.query = "What are the benefits of vector databases?";
rag_req.collection = "documentation";
rag_req.top_k = 5;
rag_req.similarity_threshold = 0.7;
rag_req.model_alias = "llama-3-8b";
rag_req.temperature = 0.3;

auto answer = handler.handleRAG(rag_req);
if (answer) {
    std::cout << "Answer: " << answer.value() << std::endl;
}
```

### Model Management

```cpp
// Load a model
ModelLoadRequest load_req;
load_req.model_path = "/models/llama-3-8b-instruct.gguf";
load_req.alias = "llama-3-8b";
load_req.gpu_layers = 32;
load_req.context_size = 8192;

auto load_result = handler.handleModelLoad(load_req);

// Load LoRA adapter
LoraLoadRequest lora_req;
lora_req.adapter_path = "/adapters/medical-terminology.safetensors";
lora_req.alias = "medical";
lora_req.base_model = "llama-3-8b";
lora_req.scale = 1.0;

auto lora_result = handler.handleLoraLoad(lora_req);

// Use specialized model
InferRequest specialized_req;
specialized_req.prompt = "Explain ACE inhibitors mechanism";
specialized_req.model_alias = "llama-3-8b";
specialized_req.lora_alias = "medical";

auto specialized_answer = handler.handleInfer(specialized_req);
```

## Dependencies

### Internal Dependencies
- **query/**: AQL AST node types
- **index/**: Vector index for similarity search
- **storage/**: Persistent storage for embeddings and metadata
- **llm/**: LLM backend implementations

### External Dependencies
- **llama.cpp** (optional): GGUF model inference
- **nlohmann/json**: JSON serialization
- **spdlog** (optional): Logging

### Compilation
```cmake
# Link AQL module
target_link_libraries(my_app themis-aql)

# Dependencies
target_link_libraries(my_app
    themis-query
    themis-index
    llama  # Optional: llama.cpp
)
```

## Performance Characteristics

### Header-Only Overhead
- Interfaces are pure virtual (vtable indirection only)
- Request structures are POD types (no overhead)
- Template methods for zero-cost abstractions

### Runtime Overhead
- Model loading: 1-30 seconds (one-time per model)
- Embedding: 10-100ms per text (batched)
- Inference: 10-100 tokens/sec
- RAG: 50-500ms (retrieval + generation)

## Known Limitations

1. **Backend Dependency**
   - `LlmAqlHandler` now supports multiple backends via `ILLMBackend` (llama.cpp, VLLM,
     Ollama, OpenAI); direct llama.cpp coupling is no longer required.

2. **Streaming Interface**
   - SSE token streaming for AQL explanations is implemented (`streamExplainAQLAsSSE()`).
   - ✅ Generic `AQLTokenStream` iterator API for arbitrary inference implemented in Phase 4
     (`include/aql/aql_token_stream.h`, header-only, thread-safe push/pop/cancel/range-for).

3. **Multi-Modal Support**
   - ✅ `MultiModalInferRequest` / `MultiModalInput` implemented in Phase 4
     (`include/aql/multimodal_infer_request.h`); image, audio, video modalities with MIME
     validation; runtime flag `THEMIS_MULTIMODAL=1` required for full pipeline.

4. **Model Size Constraints**
   - Limited by available RAM/VRAM.
   - Large models (70B+) require significant resources.

5. **Async Interface**
   - ✅ `IAsyncLLMBackend` non-blocking interface implemented in Phase 4
     (`include/aql/iasync_llm_backend.h`); `ThreadPoolAsyncLLMBackend` adapter provided.

## Status

**Production Ready** (as of v1.5.0)

✅ **Stable Interfaces:**
- `LlmAqlHandler` — LLM command dispatch, NL-to-AQL, RAG, streaming
- `DocsAssistantFunctions` — function lookup, query explanation, intent detection
- `InferRequest`, `RAGRequest`, `EmbedRequest`, `ModelLoadRequest`, `LoraLoadRequest`
- LoRA adapter support
- Natural language to AQL translation (`translateNLToAQL`, `translateNLToAQLWithConfidence`)
- Query explanation via SSE streaming (`streamExplainAQLAsSSE`)
- `AQLTokenStream` — thread-safe streaming token iterator (Phase 4)
- `ReActAgent` — multi-step reasoning agent with tool calling (Phase 4)
- `MultiModalInferRequest` — image/audio/video inference request (Phase 4)
- `IAsyncLLMBackend` / `ThreadPoolAsyncLLMBackend` — async backend interface (Phase 4)
- `AQLQueryDiffExplainer` — clause-level structural diff (rule-based, no LLM)
- `AQLRollbackSuggester` — compensating query generation (rule-based, no LLM)
- `AQLIngestionBridge` — ingestion pipeline enrichment for INSERT/UPSERT
- `AQLModelRouter` — query-type-based model routing
- `IClassifyFn` / `ClassifyBridge` — zero-shot classification interface
- `LLMAQLEmbeddingBridge` — embedding bridge for semantic few-shot ranking

🔬 **Experimental:**
- Fine-tuning pipeline integration (`AQLLoRAFinetuner`)
- Distributed LLM inference

## Related Documentation

- [src/aql/README.md](../../src/aql/README.md) - Implementation details, component guide, troubleshooting
- [src/aql/ROADMAP.md](../../src/aql/ROADMAP.md) - Implementation phases and production-readiness checklist
- [src/aql/FUTURE_ENHANCEMENTS.md](../../src/aql/FUTURE_ENHANCEMENTS.md) - Planned improvements with interface specs
- [src/aql/ARCHITECTURE.md](../../src/aql/ARCHITECTURE.md) - Component diagram and data-flow
- [Query Module](../query/README.md) - Core AQL parsing
- [LLM Module](../llm/README.md) - LLM backend implementations
- [Index Module](../index/README.md) - Vector indexing

## Contributing

When modifying AQL headers:

1. Maintain ArangoDB AQL compatibility where applicable
2. Use clear, self-documenting interface names
3. Add comprehensive Doxygen comments
4. Consider backward compatibility
5. Test with multiple LLM backends

For detailed contribution guidelines, see [CONTRIBUTING.md](../../CONTRIBUTING.md).

## See Also

- [src/aql/README.md](../../src/aql/README.md) - Implementation details, troubleshooting
- [src/aql/ROADMAP.md](../../src/aql/ROADMAP.md) - Implementation history
- [src/aql/FUTURE_ENHANCEMENTS.md](../../src/aql/FUTURE_ENHANCEMENTS.md) - Planned enhancements
- [Query Module](../query/README.md) - Core AQL parsing
- [LLM Module](../llm/README.md) - LLM backend implementations
- [Index Module](../index/README.md) - Vector indexing

## Installation

This module is included as part of ThemisDB. Add the module headers to your include path:

```cmake
target_include_directories(your_target PRIVATE ${THEMISDB_INCLUDE_DIR})
```
