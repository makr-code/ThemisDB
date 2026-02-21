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
| `llm_aql_handler.cpp` | LLM command handler (INFER/RAG/EMBED/MODEL/LORA) |
| `nl_to_aql.cpp` | Natural language to AQL translation |
| `aql_doc_assistant.cpp` | Function lookup and explanation |
| `query_validator.cpp` | Validation and linting (in progress) |

## Scope

**In Scope:**
- LLM-based AQL query generation and assistance
- Natural language to AQL translation
- Documentation assistant for AQL functions
- LLM command handlers (INFER, RAG, EMBED, MODEL, LORA)
- Query explanation and profiling assistance

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
- **Natural Language to AQL**: `translateNLToAQL()` - Schema-aware query translation
- **Chat Interface**: `executeChat()` - Multi-turn conversations with message history
- **LLM INFER**: Generate text using loaded language models (with model/LoRA selection)
- **LLM RAG**: Retrieval-Augmented Generation with vector search integration
- **LLM EMBED**: Generate embeddings for text (with model selection support)
- **LLM MODEL**: Load/unload/list/ingest GGUF models
- **LLM LORA**: LoRA (Low-Rank Adaptation) adapter management
- **LLM STATS**: Performance monitoring and statistics
- **LLM CACHE**: Prompt cache management

**Recent Improvements (v1.3.2):**
- ✅ Active model and LoRA selection in all inference methods
- ✅ Complete RAG integration with VectorIndexManager for similarity search
- ✅ Automatic similarity threshold filtering for RAG queries
- ✅ Multi-format chat support (ChatML, Llama2, Alpaca, Vicuna)
- ✅ Markdown cleanup for LLM-generated responses
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

## Dependencies

### Internal Dependencies
- **query/**: AQL parsing and execution
- **index/**: Vector index for similarity search
- **storage/**: Persistent storage for embeddings and model metadata
- **llm/**: LLM backend integration (llama.cpp)

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

## Status

**Production Ready** (as of v1.5.0)

✅ **Stable Features:**
- LLM INFER command
- LLM EMBED command
- LLM MODEL management
- Basic RAG queries

⚠️ **Beta Features:**
- LLM LORA adapter support
- Advanced RAG with multi-hop reasoning
- Natural language to AQL translation
- Query explanation AI assistant

🔬 **Experimental:**
- Multi-modal LLM support (images, audio)
- Fine-tuning pipeline integration
- Distributed LLM inference

## Related Documentation

- [Query Module](../query/README.md) - Core AQL parsing and execution
- [LLM Module](../llm/README.md) - LLM backend integration
- [Index Module](../index/README.md) - Vector indexing for RAG
- [AQL Syntax Guide](../../docs/de/aql/aql_syntax.md) - Complete AQL syntax reference
- [AQL Functions Reference](../../docs/de/aql/aql_functions_reference.md) - All AQL functions
- [AQL Hybrid Queries](../../docs/de/aql/aql_hybrid_queries.md) - Multi-model query examples

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

- [FUTURE_ENHANCEMENTS.md](FUTURE_ENHANCEMENTS.md) - Planned AQL improvements
- [Query Module](../query/README.md) - Query execution engine
- [LLM Module](../llm/README.md) - LLM integration
