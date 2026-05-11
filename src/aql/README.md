> **Build:** `cmake --preset release && cmake --build build/release --target <target>`

# ThemisDB AQL Module

## Module Purpose

The AQL module provides specialized components for AQL (Advanced Query Language) integration with LLM systems and documentation assistants. This module extends ThemisDB's query capabilities with natural language query processing and AI-assisted documentation generation.

### About AQL (Advanced Query Language)

**AQL** is ThemisDB's multi-paradigm, declarative query language based on ArangoDB's AQL (ArangoQL) but significantly extended to support:

- **Multi-Model Data**: Relational tables, document collections, property graphs, vector embeddings, geospatial data, timeseries
- **Hybrid Queries**: Combine vector similarity + geospatial filters, fulltext + graph traversal, and more
- **LLM Integration**: Native support for embedding generation, RAG (Retrieval-Augmented Generation), and inference
- **100+ Functions**: Comprehensive function library including graph, geospatial, vector, AI/ML, process mining, and ethics

**Language Heritage:**
- **Based on ArangoDB AQL**: Maintains familiar syntax (FOR, FILTER, SORT, LIMIT, RETURN, LET, COLLECT)
- **SQL-Compatible**: Declarative approach similar to SQL SELECT statements
- **Extended for AI**: Native vector similarity, embedding functions, LLM commands
- **Multi-Paradigm**: Single language for all data models (not just documents/graphs like ArangoDB)

## Relevant Interfaces

| Interface / File | Role |
|-----------------|------|
| `llm_aql_handler.cpp` | LLM command handler (INFER/RAG/EMBED/MODEL/LORA), NL-to-AQL translation |
| `docs_assistant_functions.cpp` | Function lookup and explanation; native NLP intent detection via `IClassifyFn` |
| `aql_query_validator.cpp` | Query validation and linting |
| `aql_query_builder.cpp` | Schema-aware programmatic AQL construction |
| `aql_schema_provider.cpp` | Live schema context for query generation |
| `aql_syntax_highlighter.cpp` | ANSI color highlighting and error annotation |
| `aql_confidence_scorer.cpp` | Confidence scoring for generated queries |
| `aql_autocomplete.cpp` | Token-level autocompletion (LSP-compatible) |
| `aql_fewshot_example_library.cpp` | Few-shot NL/AQL example corpus with optional semantic ranking |
| `aql_optimizer_advisor.cpp` | Query plan explanation and rewrite suggestions |
| `aql_conversation_context.cpp` | Multi-turn conversation history with bounded context window |
| `aql_query_template_library.cpp` | Pre-validated query templates for common patterns |
| `aql_lora_finetuner.cpp` | LoRA adapter fine-tuning on AQL corpora |
| `aql_migration_assistant.cpp` | Legacy AQL migration (ArangoDB → ThemisDB AQL) |
| `llm_metrics_collector.cpp` | Latency, token counts, and cache-hit metrics |
| `aql_agent.cpp` | ReAct (Reasoning+Acting) multi-step agent with tool calling |
| `aql_query_diff_explainer.cpp` | Clause-level structural diff between two AQL queries |
| `aql_rollback_suggester.cpp` | Rule-based rollback query generation for mutating statements |
| `aql_ingestion_bridge.cpp` | Connects AQL INSERT/UPSERT operations to the ingestion pipeline |
| `aql_model_router.cpp` | Routes AQL queries to the best-matching LLM backend by type |
| `classify_bridge.cpp` | Zero-shot text classification bridge (`IClassifyFn` / `NullClassifyFn`) |
| `llm_aql_embedding_bridge.cpp` | Adapts `LLMAQLHandler::executeEmbed()` to the `IEmbeddingProvider` interface |

## Scope

**In Scope:**
- LLM-based AQL query generation and assistance
- Natural language to AQL translation
- Documentation assistant for AQL functions
- LLM command handlers (INFER, RAG, EMBED, MODEL, LORA)
- Query explanation and profiling assistance
- **AQL syntax highlighting and error annotation for LLM responses**

**Out of Scope:**
- Core AQL parsing (handled by query module)
- Query execution (handled by query engine)
- Index management (handled by index module)
- Storage operations (handled by storage module)

## Key Components

### LlmAqlHandler
**Location:** `llm_aql_handler.cpp`, `../include/aql/llm_aql_handler.h`

Handles LLM-specific AQL commands for AI model integration with full implementation of natural language translation, RAG, and chat capabilities.

**Core Features:**
- **Natural Language to AQL**: `translateNLToAQL()` - Schema-aware query translation with automatic syntax validation of the generated AQL
- **Few-Shot Translation**: `translateNLToAQLWithExamples()` - Improved accuracy via curated NL/AQL example library (`AQLFewShotExampleLibrary`)
- **Streaming AQL Explanations**: `streamExplainAQL()` / `streamExplainAQLAsSSE()` — token-by-token streaming of natural language explanations for long AQL queries
- **Chat Interface**: `executeChat()` - Multi-turn conversations with message history
- **LLM INFER**: Generate text using loaded language models (with model/LoRA selection)
- **LLM RAG**: Retrieval-Augmented Generation with vector search integration
- **LLM EMBED**: Generate embeddings for text (with model selection support)
- **LLM MODEL**: Load/unload/list/ingest GGUF models
- **LLM LORA**: LoRA (Low-Rank Adaptation) adapter management
- **LLM STATS**: Performance monitoring and statistics
- **LLM CACHE**: Prompt cache management

**Improvements through v1.5.0:**
- ✅ Active model and LoRA selection in all inference methods
- ✅ Complete RAG integration with VectorIndexManager for similarity search
- ✅ Automatic similarity threshold filtering for RAG queries
- ✅ Multi-format chat support (ChatML, Llama2, Alpaca, Vicuna)
- ✅ Markdown cleanup for LLM-generated responses
- ✅ AQL syntax highlighting and error annotation via `AQLSyntaxHighlighter`
- ✅ `formatLLMResponse()` method for post-processing arbitrary LLM output
- ✅ `translateNLToAQL()` validates generated AQL and logs structural issues
- ✅ `translateNLToAQL()` sanitizes `nl_query` and `schema_context` inputs to prevent prompt injection (instruction overrides, persona hijacking, system-block markers, null bytes)
- ✅ `streamExplainAQL()` / `streamExplainAQLAsSSE()` — real-time streaming explanations for long AQL queries
- ✅ `translateNLToAQLWithExamples()` — few-shot prompt injection from `AQLFewShotExampleLibrary` for improved NL-to-AQL accuracy
- ✅ Comprehensive test coverage

**Syntax Examples:**

```aql
-- Generate text using LLM
LLM INFER 'Explain quantum computing in simple terms'
  MODEL 'llama-3-8b'
  LORA 'physics-tuned'
  MAX_TOKENS 500
  TEMPERATURE 0.7

-- Retrieval-Augmented Generation
LLM RAG 'What are the best practices for database indexing?'
  SEARCH IN documentation
  TOP 10
  MODEL 'llama-3-8b'
  TEMPERATURE 0.3

-- Generate embeddings
LLM EMBED 'The quick brown fox jumps over the lazy dog'
  MODEL 'all-minilm-l6-v2'
  RETURN AS ARRAY

-- Load a model
LLM MODEL LOAD 'models/llama-3-8b-instruct.gguf'
  ALIAS 'llama-3-8b'
  GPU_LAYERS 32

-- Apply LoRA adapter
LLM LORA LOAD 'adapters/medical-terminology.safetensors'
  ALIAS 'medical'
  BASE_MODEL 'llama-3-8b'

-- Get inference statistics
LLM STATS
  MODEL 'llama-3-8b'
  INCLUDE_HISTORY true
```

**LLM Integration Architecture:**

```
User Query (Natural Language or AQL+LLM)
    ↓
LlmAqlHandler.handleLLMCommand()
    ↓
Parse LLM-specific syntax
    ↓
┌─────────────────────────────────────────────────────┐
│ LLM Command Type                                    │
├─────────────────────────────────────────────────────┤
│ INFER    → Generate text                           │
│ RAG      → Vector search + LLM generation          │
│ EMBED    → Generate embeddings                     │
│ MODEL    → Manage GGUF models                      │
│ LORA     → Manage LoRA adapters                    │
│ STATS    → Query performance metrics               │
│ CACHE    → Manage prompt cache                     │
└─────────────────────────────────────────────────────┘
    ↓
Interact with LLM Backend (llama.cpp)
    ↓
Return structured results
```

**Thread Safety:**
- Handler instances are thread-safe
- Model loading is synchronized
- Inference requests can be concurrent (up to batch size)

**Performance Characteristics:**
- Model loading: 1-30 seconds (depends on model size, GPU layers)
- Embedding generation: 10-100ms per text
- Inference: 10-100 tokens/sec (depends on model size, hardware)
- RAG queries: 50-500ms (vector search + LLM generation)

### DocsAssistantFunctions
**Location:** `docs_assistant_functions.cpp`, `../include/aql/docs_assistant_functions.h`

Provides AI-assisted documentation generation and query explanation.

**Features:**
- Natural language to AQL translation
- AQL query explanation
- Function documentation lookup
- Query optimization suggestions
- Example query generation

**Usage:**

```cpp
#include "aql/docs_assistant_functions.h"

DocsAssistant assistant(llm_client);

// Translate natural language to AQL
std::string nl_query = "Find all users in Seattle older than 30";
auto aql = assistant.translateToAQL(nl_query);
// Returns: FOR user IN users FILTER user.city == "Seattle" AND user.age > 30 RETURN user

// Explain an AQL query
std::string query = "FOR doc IN collection FILTER doc.x > 10 RETURN doc";
auto explanation = assistant.explainQuery(query);
// Returns: This query iterates over all documents in 'collection',
//          filters those with x greater than 10, and returns matching documents.

// Get function documentation
auto docs = assistant.getFunctionDocs("SIMILARITY");
// Returns: SIMILARITY(vectorA, vectorB) - Computes cosine similarity between two vectors.
//          Returns: Float between -1.0 and 1.0

// Suggest optimizations
auto suggestions = assistant.suggestOptimizations(query);
// Returns: Consider adding an index on 'collection.x' for better performance.
```

### AQLSyntaxHighlighter
**Location:** `aql_syntax_highlighter.cpp`, `../include/aql/aql_syntax_highlighter.h`

Tokenizes AQL source text, applies ANSI color highlighting, validates structure, and
annotates syntax errors in LLM responses that contain AQL code blocks.

**Features:**
- **Tokenizer**: classifies every character sequence as `KEYWORD`, `LLM_KEYWORD`,
  `FUNCTION`, `IDENTIFIER`, `STRING`, `NUMBER`, `OPERATOR`, `PUNCTUATION`, or `COMMENT`
- **Syntax highlighting**: wraps tokens in ANSI escape codes for terminal display
  (pass `use_ansi = false` for plain-text output)
- **Error annotation**: structural checks – balanced brackets/braces/parentheses,
  unterminated string literals, `FOR` clause missing `IN`
- **LLM response formatter**: scans a full LLM response for ` ```aql ``` ` blocks,
  highlights each block in place, and returns a merged list of error annotations

**Usage (standalone):**

```cpp
#include "aql/aql_syntax_highlighter.h"

themis::aql::AQLSyntaxHighlighter h; // ANSI mode by default

// Highlight a single block
std::string highlighted = h.highlightBlock("FOR doc IN users RETURN doc");

// Validate and annotate errors
auto errors = h.annotateErrors("FOR doc RETURN { name: doc.name");
for (const auto& err : errors)
    std::cerr << "Line " << err.line << ": " << err.message << '\n';

// Process an entire LLM response
auto result = h.formatLLMResponse(llm_output);
std::cout << result.text;                           // colored AQL blocks inline
for (const auto& ann : result.annotations)
    std::cerr << "AQL error: " << ann.message << '\n';
```

**Usage (via LLMAQLHandler):**

```cpp
#include "aql/llm_aql_handler.h"

themis::aql::LLMAQLHandler handler;

// Forward the raw LLM output through the highlighter
auto result = handler.formatLLMResponse(llm_output);
std::cout << result.text;
if (!result.annotations.empty()) {
    for (const auto& ann : result.annotations)
        std::cerr << "Syntax error at line " << ann.line
                  << ": " << ann.message << '\n';
}
```

## Architecture

### AQL Language Philosophy

**Declarative over Imperative:**
AQL follows a declarative approach where you specify WHAT you want, not HOW to get it:

```aql
-- Declarative (AQL)
FOR user IN users
  FILTER user.age > 30
  RETURN user

-- vs Imperative (JavaScript)
users.filter(u => u.age > 30)
```

**Multi-Model Unification:**
AQL provides a single language for all data models:

```aql
-- Relational (table)
FOR row IN orders WHERE customer_id == "C123" RETURN row

-- Document (collection)
FOR doc IN products FILTER doc.category == "Electronics" RETURN doc

-- Graph (traversal)
FOR v, e IN 1..3 OUTBOUND "users/alice" GRAPH "social" RETURN v

-- Vector (similarity)
FOR doc IN articles LET score = SIMILARITY(doc.embedding, @query) RETURN doc

-- Geospatial (location)
FOR place IN places FILTER ST_DWithin(place.location, @point, 1000) RETURN place
```

**Composability:**
Complex queries are built by composing simple operations:

```aql
-- Hybrid: Vector similarity + Geospatial + Relational filtering
FOR place IN places
  LET score = SIMILARITY(place.embedding, @queryVector)
  FILTER score > 0.7                              -- Vector filter
  FILTER ST_DWithin(place.location, @point, 5000)  -- Geo filter
  FILTER place.rating >= 4.0                      -- Relational filter
  FILTER place.price_range IN ["$$", "$$$"]       -- Relational filter
  SORT score DESC
  LIMIT 10
  RETURN {
    name: place.name,
    score: score,
    distance: ST_Distance(place.location, @point),
    rating: place.rating
  }
```

### LLM Integration Design

**RAG Pipeline Architecture:**

```
1. User Query (Natural Language)
   ↓
2. Embedding Generation
   query_vector = LLM EMBED(user_query)
   ↓
3. Vector Search
   FOR doc IN knowledge_base
     LET score = SIMILARITY(doc.embedding, query_vector)
     FILTER score > threshold
     SORT score DESC
     LIMIT k
     RETURN doc
   ↓
4. Context Assembly
   context = CONCAT(retrieved_docs)
   ↓
5. LLM Generation
   response = LLM INFER(prompt_template + context + user_query)
   ↓
6. Return Results
```

**LoRA Adapter Support:**
Fine-tune models for domain-specific tasks without full retraining:

```aql
-- Load base model
LLM MODEL LOAD 'llama-3-8b.gguf' ALIAS 'base'

-- Load domain-specific adapter
LLM LORA LOAD 'medical-terminology.safetensors'
  ALIAS 'medical'
  BASE_MODEL 'base'

-- Use specialized model
LLM INFER 'Explain the mechanism of action for ACE inhibitors'
  MODEL 'base'
  LORA 'medical'
```

## Integration Points

### With Query Module
The AQL module extends the query module with LLM capabilities:

```cpp
// Query module parses AQL syntax
AQLParser parser;
auto ast = parser.parse(query_string);

// If LLM command detected
if (ast.type == ASTNodeType::LLMCommand) {
  // AQL module handles LLM-specific logic
  LlmAqlHandler llm_handler(llm_backend);
  return llm_handler.handleLLMCommand(ast);
}
```

### With Index Module
RAG queries leverage vector indexes:

```aql
-- Uses VectorIndexManager for similarity search
LLM RAG 'Find papers about neural networks'
  SEARCH IN research_papers  -- Uses HNSW index on embeddings
  TOP 10
```

### With Storage Module
Embeddings and model metadata stored persistently:

```
Key Schema:
  llm:model:<model_id> → Model metadata (path, alias, config)
  llm:lora:<adapter_id> → LoRA adapter metadata
  llm:embedding:<doc_id> → Cached embeddings
  llm:cache:<prompt_hash> → Prompt cache entries
```

## API/Usage Examples

### Natural Language Queries

ThemisDB now supports direct natural language to AQL translation via the `translateNLToAQL()` method. This feature uses LLM-powered translation with schema awareness for accurate query generation.

```cpp
#include "aql/llm_aql_handler.h"

LLMAQLHandler handler;

// Basic translation
std::string nl_query = "What are the top 10 restaurants in Seattle with high ratings?";
auto aql_query = handler.translateNLToAQL(nl_query);

// Generated AQL:
// FOR place IN restaurants
//   FILTER place.city == "Seattle"
//   SORT place.rating DESC
//   LIMIT 10
//   RETURN place

// Schema-aware translation for better accuracy
std::string schema_context = R"(
Collections:
- restaurants: {_id, name, city, rating, cuisine, price_level}
- reviews: {_id, restaurant_id, user_id, rating, comment}
)";

nl_query = "Find Italian restaurants in Seattle with average rating above 4.5";
aql_query = handler.translateNLToAQL(nl_query, schema_context);

// The translation uses schema context to generate more accurate queries:
// FOR r IN restaurants
//   FILTER r.city == "Seattle" AND r.cuisine == "Italian" AND r.rating > 4.5
//   RETURN r
```

**Features:**
- **Schema-Aware**: Provide database schema for more accurate translations
- **Markdown Cleanup**: Automatically removes code fences from LLM responses
- **Error Handling**: Clear error messages for translation failures
- **Multi-Turn Context**: Uses chat interface for better prompt handling

**Performance:**
- Translation typically completes in < 2 seconds (target)
- Depends on LLM model size and hardware acceleration

### Few-Shot Example Library

The `AQLFewShotExampleLibrary` provides a curated corpus of natural-language / AQL pairs that
can be injected into LLM prompts to improve NL-to-AQL translation accuracy, especially for
uncommon or complex query patterns.

```cpp
#include "aql/llm_aql_handler.h"
#include "aql/aql_fewshot_example_library.h"

LLMAQLHandler handler;
AQLFewShotExampleLibrary lib;  // 37 built-in examples across 6 domains

// Translate using few-shot examples (auto-selected by relevance)
std::string nl_query = "Find the 5 nearest restaurants to my location";
std::string aql = handler.translateNLToAQLWithExamples(nl_query, lib);

// With schema context and custom example count
std::string schema = "Collections:\n- restaurants: {name, location, rating, cuisine}";
aql = handler.translateNLToAQLWithExamples(nl_query, lib, schema, /*max_examples=*/3);

// Retrieve examples manually for custom prompt construction
auto relevant = lib.findRelevant(nl_query, 3);
std::string prompt_section = AQLFewShotExampleLibrary::formatForPrompt(relevant);

// Find examples by domain
auto graph_examples = lib.findByDomain(AQLExampleDomain::GRAPH);
auto vector_examples = lib.findByDomain(AQLExampleDomain::VECTOR);

// Register custom examples
lib.registerExample({
    "custom_timeseries",
    "Get sensor readings from last hour",
    "FOR r IN sensors\n  FILTER r.ts >= DATE_SUBTRACT(DATE_NOW(), 1, \"hour\")\n  RETURN r",
    AQLExampleDomain::TIMESERIES,
    "Custom time-range example",
    {"timeseries", "sensor"}
});
```

**Built-in domains and example counts:**

| Domain       | Description                              | Examples |
|--------------|------------------------------------------|----------|
| DOCUMENT     | CRUD, filter, sort, update, delete       | 9        |
| GRAPH        | Traversal, shortest path, edge filters   | 6        |
| VECTOR       | ANN search, hybrid, L2, dot-product      | 4        |
| GEOSPATIAL   | Radius, nearest, polygon, combined       | 4        |
| TIMESERIES   | Range, between, aggregation, latest      | 4        |
| AGGREGATION  | Count, sum, avg, HAVING-style filters    | 4        |
| GENERAL      | Subquery join, nested, fulltext, array   | 6        |

**Security:** `translateNLToAQLWithExamples()` applies the same prompt-injection prevention
as `translateNLToAQL()` — both `nl_query` and `schema_context` are sanitized before prompt
assembly. Injection patterns (instruction overrides, persona hijacking, system markers, DAN
jailbreaks, null bytes) raise `LLMException(PROMPT_INJECTION)`.

### Streaming AQL Explanations

For long, complex AQL queries the `streamExplainAQL()` and `streamExplainAQLAsSSE()` methods
let callers receive the LLM-generated explanation token by token, enabling real-time display
without waiting for the full response.

```cpp
#include "aql/llm_aql_handler.h"

LLMAQLHandler handler;

const std::string aql = R"(
FOR u IN users
  COLLECT city = u.city WITH COUNT INTO cnt
  SORT cnt DESC
  LIMIT 5
  RETURN { city, cnt }
)";

// Token-by-token streaming (terminal, WebSocket, ...)
std::string full_explanation = handler.streamExplainAQL(
    aql,
    [](const std::string& token) {
        std::cout << token << std::flush;
    }
);

// SSE streaming for HTTP endpoints
handler.streamExplainAQLAsSSE(
    aql,
    [&response](const std::string& sse_event) {
        response.write(sse_event); // each event is "data: <token>\n\n"
    },
    "req-12345" // optional request_id echoed in every event
);

// With optional schema context for richer explanations
const std::string schema = "Collection users: {name, city, age}";
handler.streamExplainAQL(aql, [](const std::string& t){ std::cout << t; }, schema);
```

**Security:** Both methods apply the same prompt injection prevention as `translateNLToAQL()`
— `aql_query` and `schema_context` are sanitized before being embedded in the LLM prompt.
Any input containing injection patterns (instruction overrides, persona hijacking, system
markers, DAN jailbreaks, null bytes) raises `LLMException(PROMPT_INJECTION)`.

### RAG Query Example

ThemisDB's RAG implementation now includes full vector search integration with similarity filtering.

```cpp
#include "aql/llm_aql_handler.h"

LLMAQLHandler handler;

// Basic RAG query
std::string query = "How do I create a vector index in ThemisDB?";
std::unordered_map<std::string, std::string> options;
options["max_tokens"] = "300";
options["temperature"] = "0.3";
options["similarity_threshold"] = "0.7"; // Filter by relevance

auto result = handler.executeRAG(query, "documentation", 5, "", options);
// Returns: To create a vector index in ThemisDB, use the CREATE INDEX statement...

// RAG with LoRA adapter for domain-specific responses
options["temperature"] = "0.5";
result = handler.executeRAG(
    "Explain database replication strategies",
    "technical_docs",
    10,
    "technical-assistant", // LoRA adapter ID
    options
);

// RAG workflow:
// 1. Generate query embedding via executeEmbed()
// 2. Search vector index for similar documents (top-k with similarity threshold)
// 3. Build RAGContext with retrieved documents
// 4. Generate response using context-aware LLM inference
```

**RAG Features:**
- **Vector Search Integration**: Automatic similarity search via VectorIndexManager
- **Similarity Filtering**: Filter results by configurable threshold (0.0-1.0)
- **Top-K Control**: Retrieve variable number of relevant documents
- **LoRA Support**: Apply domain-specific adapters for specialized responses
- **Graceful Degradation**: Falls back to context-free generation if vector index unavailable

### Chat Interface

Multi-turn conversations with message history support.

```cpp
#include "aql/llm_aql_handler.h"
#include "llm/llama_wrapper.h"

LLMAQLHandler handler;

// Build conversation history
std::vector<llm::ChatMessage> messages;
messages.emplace_back("system", "You are a helpful database assistant.");
messages.emplace_back("user", "What is a vector index?");

std::unordered_map<std::string, std::string> options;
options["chat_format"] = "llama2"; // or "chatml", "alpaca", "vicuna"

auto response = handler.executeChat(messages, "", options);

// Continue conversation
messages.emplace_back("assistant", response);
messages.emplace_back("user", "How do I create one?");

response = handler.executeChat(messages, "", options);
```

**Chat Features:**
- **Multi-Turn Context**: Maintains conversation history
- **Format Support**: ChatML, Llama2, Alpaca, Vicuna templates
- **Model Selection**: Specify model for chat (future: active)
- **Used by translateNLToAQL**: Internal use for better prompt handling

### Embedding Generation

```cpp
// Generate embeddings for documents
std::string embed_query = R"(
  FOR doc IN articles
    LET embedding = LLM EMBED(doc.content, MODEL: 'all-minilm-l6-v2')
    UPDATE doc WITH {embedding: embedding} IN articles
)";

auto result = handler.execute(embed_query);
```

### AQLAgent / ReActAgent
**Location:** `aql_agent.cpp`, `../include/aql/aql_agent.h`

Autonomous multi-step reasoning agent using the ReAct (Reasoning+Acting) pattern with tool
calling. The agent iterates Thought→Action→Observation cycles up to a configurable
`max_iterations` limit, stopping when the LLM emits a "Final Answer:" prefix.

**Key types:**
- `AgentTool` — named callable with JSON Schema parameter description
- `AgentConfig` — model alias, max iterations, temperature
- `ReasoningStep` — thought, tool name, tool input/output, observation
- `AgentResult` — final answer, reasoning trace, iterations used, success flag
- `IAgent` — abstract interface
- `ReActAgent` — Pimpl concrete implementation

**Usage:**

```cpp
#include "aql/aql_agent.h"

themis::aql::AgentConfig cfg;
cfg.model_alias   = "llama-3-8b";
cfg.max_iterations = 5;

themis::aql::ReActAgent agent(handler, cfg);

// Register a tool the agent can call
agent.registerTool({
    "query_db",
    "Execute an AQL query and return results as JSON",
    {{"type","object"},{"properties",{{"aql",{{"type","string"}}}}}},
    [&](const nlohmann::json& args) -> nlohmann::json {
        auto res = engine.executeAql(args["aql"]);
        return res.has_value() ? res.value() : nlohmann::json{{"error","failed"}};
    }
});

auto result = agent.run("Find the 3 most active users in the last week");
if (result.succeeded) {
    std::cout << result.final_answer << '\n';
}
```

**Error behaviour:**
- Duplicate tool registration → `std::invalid_argument`
- Unknown tool removal → `std::invalid_argument`
- LLM failure → `LLMException(INFERENCE_FAILED)`
- Max iterations reached → `AgentResult{succeeded=false}`; tool executor exceptions are
  captured as JSON and fed back as observations (never propagate to caller)

**Note:** `ReActAgent` currently has production implementation but no active call-site in the
server stack. See `src/STUB_INVENTORY.md` and the "Latente Symbole" section in
[ROADMAP.md](ROADMAP.md) for the planned production-integration ticket.

---

### AQLQueryDiffExplainer
**Location:** `aql_query_diff_explainer.cpp`, `../include/aql/aql_query_diff_explainer.h`

Rule-based clause-level structural diff between two AQL query strings. Normalises whitespace,
splits each query into canonical clauses (FOR, LET, FILTER, SORT, LIMIT, RETURN, COLLECT,
INSERT, UPDATE, REMOVE, UPSERT, REPLACE), and returns every clause that was added, removed,
or changed. No LLM required; runs in O(n) time.

**Usage:**

```cpp
#include "aql/aql_query_diff_explainer.h"

themis::aql::AQLQueryDiffExplainer explainer;
auto diff = explainer.explain(
    "FOR u IN users FILTER u.age > 18 RETURN u",
    "FOR u IN users FILTER u.age > 21 SORT u.name RETURN u");

if (!diff.is_equivalent) {
    std::cout << diff.summary << '\n';
    for (const auto& entry : diff.diffs)
        std::cout << "  " << entry.explanation << '\n';
}
```

**Typical use cases:**
- Show what changed when a query is auto-migrated by `AQLMigrationAssistant`
- Diff view in query history / versioning UI
- Regression tests: assert that an optimised query is semantically equivalent to the original

---

### AQLRollbackSuggester
**Location:** `aql_rollback_suggester.cpp`, `../include/aql/aql_rollback_suggester.h`

Derives a compensating (rollback) AQL query for a given mutation statement (INSERT / UPDATE /
REPLACE / REMOVE / UPSERT). All logic is rule-based and runs in O(n). No LLM required.

**Usage:**

```cpp
#include "aql/aql_rollback_suggester.h"

themis::aql::AQLRollbackSuggester suggester;
auto suggestion = suggester.suggest(
    "FOR u IN users FILTER u.status == 'trial' "
    "UPDATE u WITH { status: 'active' } IN users");

if (suggestion.is_automatic) {
    std::cout << "Rollback:\n" << suggestion.rollback_query << '\n';
} else {
    std::cout << "Manual steps required:\n";
    for (const auto& step : suggestion.manual_steps)
        std::cout << "  - " << step << '\n';
}
```

**Limitations:**
- REMOVE rollback requires a pre-mutation document snapshot; the suggested query uses
  a `@snapshot` bind-parameter placeholder.
- UPDATE/REPLACE rollback uses a `@old_values` bind-parameter placeholder.
- Dynamic collection names (bind parameters as collection refs) yield `is_automatic=false`.
- Only the outermost mutation in nested sub-queries is analysed.

---

### AQLIngestionBridge
**Location:** `aql_ingestion_bridge.cpp`, `../include/aql/aql_ingestion_bridge.h`

Connects AQL INSERT/UPSERT operations to the ingestion pipeline: when a document payload
contains a text field, `enrichInsertPayload()` runs the `WorkflowEngine`, appends extracted
entities to the document under `"_entities"`, and optionally writes them to a graph store.
All public methods are thread-safe.

**Usage:**

```cpp
#include "aql/aql_ingestion_bridge.h"

auto toolbox = themis::toolbox::IngestionToolbox::createDefault();
auto bridge  = std::make_shared<themis::aql::AQLIngestionBridge>(toolbox);
handler.setIngestionBridge(bridge);

// Automatically enriched on any INSERT/UPSERT with a "text" field:
// handler.execute("INSERT {text: 'EU Regulation 2024/1234'} INTO documents");
// → document stored with "_entities": [{"id":"law:EU:2024/1234","type":"LEGAL_PROVISION",...}]

// Manual entity extraction for NL→AQL context
nlohmann::json payload = {{"text", "ThemisDB supports HNSW vector indexes"}};
std::string ctx = bridge->enrichInsertPayload(payload);
// ctx: "Extracted entities: SOFTWARE org:themisdb | ALGORITHM algo:hnsw"
```

---

### AQLModelRouter
**Location:** `aql_model_router.cpp`, `../include/aql/aql_model_router.h`

Classifies an AQL query into one of several `QueryModelType` categories (VECTOR, GRAPH, GEO,
FULLTEXT, TIMESERIES, RELATIONAL, PROCESS) by scanning for keyword patterns, then selects
the highest-priority enabled `ModelRoute` from a registered route table. Falls back to the
next enabled route when the primary is unavailable.

**Usage:**

```cpp
#include "aql/aql_model_router.h"

themis::aql::AQLModelRouter router;
router.registerRoute({themis::aql::QueryModelType::VECTOR,  "embed-model", 100});
router.registerRoute({themis::aql::QueryModelType::RELATIONAL, "llama-3-8b", 10});

auto route = router.route("FOR d IN docs LET s = SIMILARITY(d.emb, @q) RETURN d");
if (route) {
    std::cout << "Model: " << route->model_alias << '\n';  // "embed-model"
}
```

---

### ClassifyBridge / IClassifyFn
**Location:** `classify_bridge.cpp`, `../include/aql/classify_bridge.h`

Defines the `IClassifyFn` interface for zero-shot text classification and a `NullClassifyFn`
no-op fallback. The concrete `AQLFunctionClassifyBridge` (registered in the module initialiser)
delegates to the AQL `CLASSIFY(text, categories)` built-in, enabling native NLP intent
detection in `DocsAssistantFunctions::detectIntentWithNativeNLP()` without an LLM round-trip.

**Usage:**

```cpp
#include "aql/classify_bridge.h"
#include "aql/docs_assistant_functions.h"

// Inject a real classifier (e.g. wired to the CLASSIFY function registry)
auto classifier = std::make_shared<themis::aql::AQLFunctionClassifyBridge>(registry);
docs_assistant.setClassifier(classifier.get());

// Now detectIntentWithNativeNLP() returns real categories instead of "unknown"
// Fallback: when no classifier is set, NullClassifyFn is used and the LLM path takes over
```

---

### LLMAQLEmbeddingBridge
**Location:** `llm_aql_embedding_bridge.cpp`, `../include/aql/llm_aql_embedding_bridge.h`

Adapter that bridges `LLMAQLHandler::executeEmbed()` to the `IEmbeddingProvider` interface
required by `AQLFewShotExampleLibrary`. Wiring the bridge enables semantic (cosine similarity)
few-shot selection, replacing the default Jaccard word-overlap ranking.

**Usage:**

```cpp
#include "aql/llm_aql_embedding_bridge.h"
#include "aql/aql_fewshot_example_library.h"

AQLFewShotExampleLibrary library;
library.addBuiltinSamples();

// Wire semantic ranking via the handler's embed circuit
auto bridge = handler.makeEmbeddingBridge();
library.setEmbeddingProvider(bridge.get());
library.rebuildEmbeddingIndex();

// Now translateNLToAQLWithExamples() uses cosine similarity for example selection
std::string aql = handler.translateNLToAQLWithExamples(
    "find all users with role admin", library, schema_context, 5);
```

## Dependencies

### External Dependencies
- **llama.cpp**: GGUF model loading and inference
- **hnswlib** (via index module): Vector similarity search
- **nlohmann/json**: JSON parsing for structured outputs

## Performance Characteristics

### LLM Operations
- **Model Loading**: 1-30 seconds (one-time per model)
- **Embedding Generation**: 10-100ms per text (batched)
- **Inference**: 10-100 tokens/sec (depends on model size, hardware)
- **RAG Query**: 50-500ms (vector search + generation)

### Optimization Strategies
- **Prompt Caching**: Cache common prompts to avoid recomputation
- **Batch Processing**: Process multiple embeddings/inferences in batches
- **GPU Acceleration**: Offload model layers to GPU (CUDA, Vulkan, Metal)
- **Model Quantization**: Use 4-bit or 8-bit quantized models (GGUF format)

### Performance Benchmarks

Two Google Benchmark suites cover the deterministic (LLM-independent) layers of the AQL module:

| Benchmark file | What is measured |
|----------------|-----------------|
| `benchmarks/bench_hybrid_aql_sugar.cpp` | `AQLSyntaxHighlighter` (tokenize, highlight, annotateErrors, formatLLMResponse); `AQLConfidenceScorer` (score with/without schema context); `AQLFewShotExampleLibrary` (findRelevant, buildPromptSection, findByDomain, formatForPrompt) |
| `benchmarks/bench_aql_functions.cpp` | AQL function registry: string, math, array, date, geo, vector, graph, collection, security, and registry overhead |

Run the benchmarks after a Release build:

```bash
# Syntax-sugar components (tokenizer, confidence scorer, few-shot library)
./build/benchmarks/bench_hybrid_aql_sugar

# AQL function registry
./build/benchmarks/bench_aql_functions

# Filter to a specific benchmark group
./build/benchmarks/bench_hybrid_aql_sugar --benchmark_filter=Highlighter
./build/benchmarks/bench_hybrid_aql_sugar --benchmark_filter=ConfidenceScorer
./build/benchmarks/bench_hybrid_aql_sugar --benchmark_filter=FewShot
```

## Known Limitations

1. **Model Size Constraints**
   - Limited by available RAM/VRAM
   - Large models (70B+) require significant memory

2. **Inference Latency**
   - LLM inference is slower than database queries
   - Not suitable for real-time critical paths

3. **Context Window**
   - Models have limited context windows (2K-128K tokens)
   - Long documents may need chunking for RAG

4. **Natural Language Ambiguity**
   - NL to AQL translation may not always be accurate
   - Complex queries may require manual AQL writing

## Troubleshooting

### NL-to-AQL Translation Returns Invalid Queries

**Symptom:** `translateNLToAQL()` throws `LLMException(INVALID_RESPONSE, ...)` or the
returned AQL fails on execution.

**Causes and fixes:**
1. **Ambiguous natural language** — Rephrase the query to be more specific, or provide a
   `schema_context` string listing collection names and fields.
2. **Wrong model** — Code-generation models (DeepSeek-Coder, Codestral) produce much better
   AQL than general instruction models; switch via `LLM MODEL LOAD`.
3. **Validation mode** — Check `handler.getValidationLimits()`. Use `RETRY_ON_ERROR` mode so
   the handler automatically re-submits with the error annotation as feedback.
4. **Prompt injection detected** — Input containing instruction-override patterns raises
   `PROMPT_INJECTION`; sanitise the NL query before calling the handler.

### LLM Inference Times Out

**Symptom:** `LLMException(TIMEOUT)` from `executeInfer()` / `executeRAG()`.

**Fixes:**
- Increase timeouts via `handler.setTimeoutConfig({.infer_timeout = 600, ...})`.
- Reduce `max_tokens` in the inference options.
- Load a smaller/quantised model (4-bit GGUF).
- Enable GPU offload: reload model with `GPU_LAYERS <n>`.

### Circuit Breaker Open for INFER/RAG/EMBED

**Symptom:** `LLMException(CIRCUIT_OPEN)` for one command type while others work.

**Fix:** Per-command circuit breakers are isolated. Inspect state via `handler.getCircuitBreakerStates()`.
Wait for the timeout window to expire (default 60 s) or reset programmatically.
Use `LLM STATS` command to see current breaker state.

### Conversation Context Grows Without Bound

**Symptom:** `executeChat()` latency increases over a long session; backend reports context-
window overflow.

**Fix:** Configure bounded history on construction:
```cpp
themis::aql::AQLConversationContext::Config cfg;
cfg.max_turns          = 20;
cfg.max_history_tokens = 8192;
AQLConversationContext ctx(cfg);
```
Bounded eviction drops the oldest user+assistant pair while preserving the system message.

### Few-Shot Examples Appear Irrelevant

**Symptom:** `translateNLToAQLWithExamples()` picks examples that don't match the query domain.

**Fix:** Enable semantic (cosine) ranking by wiring the embedding bridge:
```cpp
auto bridge = handler.makeEmbeddingBridge();
library.setEmbeddingProvider(bridge.get());
library.rebuildEmbeddingIndex();
```
Without a provider, the library falls back to Jaccard word-overlap, which is
vocabulary-sensitive and may miss semantically similar examples.

### Rollback Query Uses Placeholder Bind Parameters

**Symptom:** `AQLRollbackSuggester::suggest()` returns `is_automatic = false` or a query with
`@snapshot` / `@old_values` placeholders.

**Cause:** REMOVE and UPDATE rollbacks require pre-mutation snapshots that the suggester
cannot infer from the query alone.

**Fix:** Capture the affected documents with a pre-query before executing the mutation, then
bind them to the rollback query:
```cpp
// Before mutation: capture snapshot
auto snapshot = engine.executeAql(
    "FOR u IN users FILTER u.status == 'trial' RETURN u");

// Apply mutation …

// After failure: execute rollback with snapshot
auto suggestion = rollback_suggester.suggest(mutation_query);
engine.executeAqlWithBindParams(suggestion.rollback_query,
                                {{"snapshot", snapshot.value()}});
```

## Status

**Production Ready** (as of v1.5.0)

✅ **Stable Features:**
- LLM INFER command
- LLM EMBED command
- LLM MODEL management
- Basic RAG queries

✅ **Also Stable (added in v1.5.0):**
- LLM LORA adapter support
- Natural language to AQL translation (with confidence scoring, prompt-injection prevention)
- Query explanation AI assistant (SSE streaming)

⚠️ **Beta Features:**
- Advanced RAG with multi-hop reasoning

🔬 **Experimental:**
- Multi-modal LLM support (images, audio)
- Fine-tuning pipeline integration
- Distributed LLM inference

## Related Documentation

- [ROADMAP.md](ROADMAP.md) - Implementation phases, completed features, production readiness checklist
- [FUTURE_ENHANCEMENTS.md](FUTURE_ENHANCEMENTS.md) - Planned improvements with interface specs and acceptance criteria
- [ARCHITECTURE.md](ARCHITECTURE.md) - Component diagram, data-flow, threading model
- [include/aql/README.md](../../include/aql/README.md) - Header interface reference
- [Query Module](../query/README.md) - Core AQL parsing and execution
- [LLM Module](../llm/README.md) - LLM backend integration
- [Index Module](../index/README.md) - Vector indexing for RAG
- [AQL Syntax Guide](../../docs/de/aql/aql_syntax.md) - Complete AQL syntax reference
- [AQL Functions Reference](../../docs/de/aql/aql_functions_reference.md) - All AQL functions
- [AQL Hybrid Queries](../../docs/de/aql/aql_hybrid_queries.md) - Multi-model query examples
- [docs/de/aql/README.md](../../docs/de/aql/README.md) - German overview (DE)
- [docs/en/aql/README.md](../../docs/en/aql/README.md) - English overview (EN)

## Contributing

When contributing to the AQL module:

1. Understand ArangoDB AQL compatibility
2. Maintain SQL-like declarative syntax
3. Add comprehensive function documentation
4. Test with various LLM models (llama, mistral, etc.)
5. Optimize for batch processing
6. Consider GPU acceleration opportunities

For detailed contribution guidelines, see [CONTRIBUTING.md](../../CONTRIBUTING.md).

## See Also

- [ROADMAP.md](ROADMAP.md) - Implementation history and planned phases
- [FUTURE_ENHANCEMENTS.md](FUTURE_ENHANCEMENTS.md) - Detailed enhancement specs
- [ARCHITECTURE.md](ARCHITECTURE.md) - Module architecture guide
- [Query Module](../query/README.md) - Query execution engine
- [LLM Module](../llm/README.md) - LLM integration

## Scientific References

1. Chamberlin, D. D., & Boyce, R. F. (1974). **SEQUEL: A Structured English Query Language**. *Proceedings of the 1974 ACM SIGFIDET Workshop on Data Description, Access and Control*, 249–264. https://doi.org/10.1145/800296.811515

2. Wood, P. T. (2012). **Query Languages for Graph Databases**. *SIGMOD Record*, 41(1), 50–60. https://doi.org/10.1145/2206869.2206879

3. Angles, R., Arenas, M., Barceló, P., Hogan, A., Reutter, J., & Vrgoc, D. (2017). **Foundations of Modern Query Languages for Graph Databases**. *ACM Computing Surveys*, 50(5), 68:1–68:40. https://doi.org/10.1145/3104031

4. Li, F., & Jagadish, H. V. (2014). **Constructing an Interactive Natural Language Interface for Relational Databases**. *Proceedings of the VLDB Endowment*, 8(1), 73–84. https://doi.org/10.14778/2735461.2735468

5. Bonawitz, K., Ivanov, V., Kreuter, B., Marcedone, A., McMahan, H. B., Patel, S., … Ramage, D. (2017). **Practical Secure Aggregation for Privacy-Preserving Machine Learning**. *Proceedings of ACM CCS 2017*, 1175–1191. https://doi.org/10.1145/3133956.3133982

## Installation

This module is built as part of ThemisDB. See the root `CMakeLists.txt` for build configuration.
