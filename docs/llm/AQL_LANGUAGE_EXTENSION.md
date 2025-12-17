# AQL Language Extension for LLM Operations

## Overview

ThemisDB extends AQL (Application Query Language) with native LLM commands, enabling seamless integration of large language model operations directly in queries. This allows combining graph queries, vector search, and LLM inference in a single declarative statement.

**Version**: v1.3.0

**Status**: Design Specification (implementation in v1.3.2)

## Design Principles

1. **Declarative**: Express what you want, not how to do it
2. **Composable**: Combine with existing AQL features
3. **Type-safe**: Strong typing for LLM parameters
4. **Performant**: Leverage caching and parallelization automatically

## New Keywords

- `LLM` - Prefix for all LLM operations
- `INFER` - Standard text generation
- `RAG` - Retrieval-Augmented Generation
- `EMBED` - Embedding generation
- `MODEL` - Model management
- `LORA` - LoRA adapter management
- `STATS` - Statistics retrieval
- `CACHE` - Cache operations

## Command Syntax

### 1. Inference Commands

#### 1.1 Simple Inference

Generate text from a prompt.

**Syntax**:
```aql
LLM INFER <prompt>
  USING MODEL <model_id>
  [WITH LORA <lora_id>]
  [OPTIONS <options_object>]
```

**Examples**:
```aql
-- Basic inference
LLM INFER 'What is ThemisDB?'
  USING MODEL 'mistral-7b';

-- With LoRA adapter
LLM INFER 'Explain contract clause 3.4'
  USING MODEL 'mistral-7b'
  WITH LORA 'legal-qa';

-- With options
LLM INFER 'Write a poem about databases'
  USING MODEL 'mistral-7b'
  OPTIONS {
    max_tokens: 200,
    temperature: 0.9,
    top_p: 0.95
  };

-- Store result
LET summary = LLM INFER doc.content
  USING MODEL 'mistral-7b'
  WITH LORA 'summarization';

RETURN summary;
```

#### 1.2 RAG Inference

Combine vector search with LLM inference.

**Syntax**:
```aql
LLM RAG <query>
  FROM COLLECTION <collection_name>
  [TOP <k>]
  [WHERE <filter>]
  [SIMILARITY > <threshold>]
  USING [MODEL <model_id>]
  [WITH LORA <lora_id>]
  [OPTIONS <options_object>]
```

**Examples**:
```aql
-- Basic RAG
LLM RAG 'What are the penalties for breach of contract?'
  FROM COLLECTION legal_documents
  TOP 5
  USING MODEL 'mistral-7b'
  WITH LORA 'legal-qa';

-- With filtering
LLM RAG 'Treatment options for diabetes'
  FROM COLLECTION medical_papers
  TOP 10
  WHERE publication_year >= 2020
  SIMILARITY > 0.8
  USING MODEL 'mistral-7b'
  WITH LORA 'medical-qa';

-- Store and use
LET legal_answer = LLM RAG @user_query
  FROM COLLECTION legal_docs
  TOP 5
  WITH LORA 'legal-qa';

INSERT {
  query: @user_query,
  answer: legal_answer,
  timestamp: DATE_NOW()
} INTO answers;
```

#### 1.3 Batch Inference

Process multiple documents in parallel.

**Syntax**:
```aql
FOR <var> IN <collection>
  LET <result> = LLM INFER <expression>
    USING MODEL <model_id>
    [WITH LORA <lora_id>]
  RETURN {<fields>}
```

**Examples**:
```aql
-- Summarize all documents
FOR doc IN documents
  LET summary = LLM INFER doc.content
    USING MODEL 'mistral-7b'
    WITH LORA 'summarization'
    OPTIONS {max_tokens: 100}
  RETURN {
    doc_id: doc._id,
    title: doc.title,
    summary: summary
  };

-- Classify documents
FOR doc IN news_articles
  LET category = LLM INFER CONCAT('Classify this article: ', doc.text)
    USING MODEL 'mistral-7b'
    WITH LORA 'classification'
  UPDATE doc WITH {category: category} IN news_articles;

-- Generate embeddings for search
FOR product IN products
  LET embedding = LLM EMBED product.description
    USING MODEL 'mistral-7b'
  UPDATE product WITH {
    description_embedding: embedding
  } IN products;
```

#### 1.4 Embedding Generation

Generate vector embeddings for text.

**Syntax**:
```aql
LLM EMBED <text>
  USING MODEL <model_id>
  [OPTIONS {normalize: true/false}]
  [RETURN VECTOR]
```

**Examples**:
```aql
-- Generate embedding
LET embedding = LLM EMBED 'Sample text for embedding'
  USING MODEL 'mistral-7b'
  RETURN VECTOR;

-- Use in vector search
LET query_embedding = LLM EMBED @search_query
  USING MODEL 'mistral-7b'
  RETURN VECTOR;

FOR doc IN documents
  LET similarity = COSINE_SIMILARITY(doc.embedding, query_embedding)
  FILTER similarity > 0.8
  SORT similarity DESC
  LIMIT 10
  RETURN {
    doc: doc,
    similarity: similarity
  };
```

### 2. Model Management Commands

#### 2.1 Load Model

Load a model into memory.

**Syntax**:
```aql
LLM MODEL LOAD <model_id>
  FROM <path_or_urn>
  [OPTIONS <options_object>]
  [PIN]
```

**Examples**:
```aql
-- Load from file
LLM MODEL LOAD 'mistral-7b'
  FROM '/models/mistral-7b.gguf'
  OPTIONS {
    n_gpu_layers: 32,
    n_ctx: 4096
  };

-- Load from blob store (URN)
LLM MODEL LOAD 'llama-3-8b'
  FROM BLOB 'urn:themis:model:llama-3-8b:v1';

-- Load and pin (prevent eviction)
LLM MODEL LOAD 'important-model'
  FROM '/models/important.gguf'
  PIN;
```

#### 2.2 Unload Model

Unload a model from memory.

**Syntax**:
```aql
LLM MODEL UNLOAD <model_id>
```

**Example**:
```aql
LLM MODEL UNLOAD 'mistral-7b';
```

#### 2.3 List Models

List all available models.

**Syntax**:
```aql
LLM MODEL LIST [WHERE <condition>]
```

**Examples**:
```aql
-- List all models
LLM MODEL LIST;

-- Filter loaded models
LLM MODEL LIST WHERE status == 'loaded';

-- Get model details
FOR model IN LLM MODEL LIST
  RETURN {
    id: model.model_id,
    status: model.status,
    size_gb: model.size_bytes / 1024 / 1024 / 1024,
    usage: model.usage_count
  };
```

#### 2.4 Ingest Model

Upload a model to blob storage.

**Syntax**:
```aql
LLM MODEL INGEST <model_id>
  FROM BLOB <path>
  [VERSION <version>]
  [REPLICATE TO (ALL | <shard_list>)]
```

**Examples**:
```aql
-- Ingest and replicate to all shards
LLM MODEL INGEST 'llama-3-8b'
  FROM BLOB '/local/llama-3-8b.gguf'
  VERSION 'v1.0'
  REPLICATE TO ALL;

-- Ingest to specific shards
LLM MODEL INGEST 'specialized-model'
  FROM BLOB '/models/specialized.gguf'
  REPLICATE TO ('shard1', 'shard2');
```

### 3. LoRA Management Commands

#### 3.1 Load LoRA

Load a LoRA adapter.

**Syntax**:
```aql
LLM LORA LOAD <lora_id>
  FOR MODEL <model_id>
  FROM <path>
  [SCALE <scale_value>]
```

**Examples**:
```aql
-- Load LoRA
LLM LORA LOAD 'legal-qa'
  FOR MODEL 'mistral-7b'
  FROM '/loras/legal-qa.bin';

-- Load with scaling
LLM LORA LOAD 'medical-qa'
  FOR MODEL 'mistral-7b'
  FROM '/loras/medical-qa.bin'
  SCALE 1.5;
```

#### 3.2 Unload LoRA

Unload a LoRA adapter.

**Syntax**:
```aql
LLM LORA UNLOAD <lora_id> FOR MODEL <model_id>
```

**Example**:
```aql
LLM LORA UNLOAD 'legal-qa' FOR MODEL 'mistral-7b';
```

#### 3.3 List LoRAs

List all available LoRA adapters.

**Syntax**:
```aql
LLM LORA LIST [FOR MODEL <model_id>] [WHERE <condition>]
```

**Examples**:
```aql
-- List all LoRAs
LLM LORA LIST;

-- List for specific model
LLM LORA LIST FOR MODEL 'mistral-7b';

-- Filter loaded LoRAs
LLM LORA LIST WHERE status == 'loaded';
```

### 4. Statistics Commands

#### 4.1 LLM Statistics

Get LLM system statistics.

**Syntax**:
```aql
LLM STATS [RETURN <fields>]
```

**Examples**:
```aql
-- Get all statistics
LLM STATS;

-- Get specific fields
LLM STATS RETURN {
  throughput: throughput.requests_per_second,
  avg_latency: latency.avg_ms,
  cache_hit_rate: cache.response_cache.hit_rate
};

-- Monitor performance
FOR stat IN [1..10]
  LET stats = LLM STATS
  SLEEP(1000)  // Wait 1 second
  RETURN {
    time: DATE_NOW(),
    throughput: stats.throughput.requests_per_second,
    active: stats.active_requests
  };
```

#### 4.2 Cache Statistics

Get cache performance statistics.

**Syntax**:
```aql
LLM CACHE STATS [RETURN <fields>]
```

**Examples**:
```aql
-- All cache stats
LLM CACHE STATS;

-- Response cache only
LLM CACHE STATS RETURN response_cache;

-- Calculate savings
LET cache_stats = LLM CACHE STATS;
LET time_saved = cache_stats.response_cache.hits * 148;  // Avg 148ms saved per hit
RETURN {
  cache_hits: cache_stats.response_cache.hits,
  time_saved_seconds: time_saved / 1000,
  hit_rate: cache_stats.response_cache.hit_rate
};
```

### 5. Cache Management Commands

#### 5.1 Clear Cache

Clear specific or all caches.

**Syntax**:
```aql
LLM CACHE CLEAR [RESPONSE | PREFIX | ALL]
```

**Examples**:
```aql
-- Clear response cache
LLM CACHE CLEAR RESPONSE;

-- Clear all caches
LLM CACHE CLEAR ALL;
```

## Advanced Usage

### Combining with Graph Queries

```aql
-- Find related entities and summarize
FOR person IN persons
  FILTER person.role == 'CEO'
  LET companies = (
    FOR v, e IN 1..1 OUTBOUND person works_at
      RETURN v
  )
  LET summary = LLM INFER CONCAT(
    'Summarize the career of ', person.name,
    ' who has worked at: ',
    CONCAT_SEPARATOR(', ', companies[*].name)
  ) USING MODEL 'mistral-7b'
  RETURN {
    person: person.name,
    companies: companies[*].name,
    career_summary: summary
  };
```

### RAG with Graph Context

```aql
-- Legal query with graph relationships
LET contract_context = (
  FOR contract IN contracts
    FILTER contract.id == @contract_id
    LET parties = (
      FOR v IN 1..1 OUTBOUND contract party_to
        RETURN v.name
    )
    RETURN {
      contract: contract,
      parties: parties
    }
)[0]

LET legal_analysis = LLM RAG CONCAT(
  'Analyze contract between ',
  CONCAT_SEPARATOR(' and ', contract_context.parties),
  ': ', @user_question
)
  FROM COLLECTION legal_precedents
  TOP 5
  USING MODEL 'mistral-7b'
  WITH LORA 'legal-qa'

RETURN {
  contract_id: @contract_id,
  parties: contract_context.parties,
  question: @user_question,
  analysis: legal_analysis
};
```

### Conditional Inference

```aql
-- Only infer if no cached result
FOR doc IN documents
  FILTER doc.summary == null
  LET summary = LLM INFER doc.content
    USING MODEL 'mistral-7b'
    WITH LORA 'summarization'
    OPTIONS {max_tokens: 150}
  UPDATE doc WITH {
    summary: summary,
    summary_generated_at: DATE_NOW()
  } IN documents
  RETURN NEW;
```

### Parallel Batch Processing

```aql
-- Process in batches with parallelization
LET batches = (
  FOR doc IN documents
    COLLECT batch = FLOOR(doc._offset / 100)
    AGGREGATE docs = doc._key
    RETURN {batch: batch, docs: docs}
)

FOR batch IN batches
  LET results = (
    FOR doc_key IN batch.docs
      LET doc = DOCUMENT('documents', doc_key)
      LET summary = LLM INFER doc.content
        USING MODEL 'mistral-7b'
        WITH LORA 'summarization'
      RETURN {
        key: doc_key,
        summary: summary
      }
  )
  RETURN {
    batch: batch.batch,
    processed: LENGTH(results),
    results: results
  };
```

## Type System

### Input Types

- **String**: Prompts, model IDs, paths
- **Object**: Options, metadata
- **Number**: max_tokens, temperature, etc.
- **Boolean**: Pin, replicate flags

### Return Types

- **String**: Generated text, model IDs
- **Object**: Complex results (stats, model info)
- **Array[Float]**: Embeddings
- **Array[Object]**: Lists (models, LoRAs)

## Error Handling

AQL LLM commands can fail with specific error codes:

```aql
-- Try-catch pattern (conceptual)
LET result = LLM INFER @prompt
  USING MODEL @model_id
  OPTIONS {max_tokens: 100}

RETURN {
  success: result != null,
  result: result,
  error: ERROR_MESSAGE()  // If failed
};
```

**Error Codes**:
- `LLM_MODEL_NOT_FOUND`: Model doesn't exist
- `LLM_LORA_NOT_FOUND`: LoRA doesn't exist
- `LLM_INFERENCE_FAILED`: Inference error
- `LLM_QUEUE_FULL`: Request queue full
- `LLM_INVALID_PARAMETERS`: Invalid options

## Performance Optimization

### Use Caching

```aql
-- Identical prompts automatically cached
FOR i IN 1..100
  LET answer = LLM INFER 'What is 2+2?'
    USING MODEL 'mistral-7b'
  RETURN answer;
-- Only first request hits LLM, rest served from cache
```

### Batch Similar Queries

```aql
-- Process in single batch for better throughput
LET prompts = ['Query 1', 'Query 2', 'Query 3', ...]

FOR prompt IN prompts
  LET result = LLM INFER prompt
    USING MODEL 'mistral-7b'
  RETURN result;
-- Automatically parallelized by AsyncInferenceEngine
```

### Prefix Caching

```aql
-- Reuse system prompts
LET system_prompt = 'You are a helpful legal assistant.'

FOR question IN questions
  LET answer = LLM INFER CONCAT(system_prompt, '\n\n', question)
    USING MODEL 'mistral-7b'
    WITH LORA 'legal-qa'
  RETURN answer;
-- System prompt tokens cached, only question changes
```

## Migration from Procedural Code

**Before** (procedural):
```python
# Python code
results = []
for doc in documents:
    embedding = llm_client.embed(doc.content)
    doc_results = vector_db.search(embedding, top_k=5)
    prompt = f"Summarize: {doc_results}"
    summary = llm_client.infer(prompt, model="mistral-7b")
    results.append({"doc": doc.id, "summary": summary})
```

**After** (declarative AQL):
```aql
FOR doc IN documents
  LET summary = LLM RAG doc.title
    FROM COLLECTION document_chunks
    TOP 5
    USING MODEL 'mistral-7b'
    WITH LORA 'summarization'
  RETURN {
    doc: doc._id,
    summary: summary
  }
```

## Best Practices

1. **Use RAG for factual queries** - Grounds responses in your data
2. **Leverage caching** - Structure prompts consistently
3. **Batch when possible** - Higher throughput
4. **Use LoRAs for domains** - Better results, less memory
5. **Monitor statistics** - Track cache hit rates, latency
6. **Set appropriate limits** - max_tokens to control costs
7. **Filter before inference** - Reduce unnecessary LLM calls
8. **Use embeddings for search** - Then RAG for final answer

## Limitations

1. **No streaming in AQL** - Use Binary/HTTP APIs for streaming
2. **Synchronous execution** - Query waits for LLM response
3. **Memory limits** - Model must fit in available VRAM
4. **No custom prompts in RAG** - Uses default template
5. **Limited error recovery** - Failed inference aborts query

## Future Enhancements (Roadmap)

- `LLM STREAM` - Token streaming in AQL
- `LLM FUNCTION` - User-defined LLM functions
- `LLM AGENT` - Multi-step reasoning
- `LLM COMPARE` - A/B testing models
- `LLM OPTIMIZE` - Auto-tune parameters
- Graph-aware RAG with relationship traversal
- Multi-model ensemble queries
- Automatic prompt optimization

## Grammar Reference

```ebnf
llm_statement ::= "LLM" (infer_stmt | rag_stmt | embed_stmt | model_stmt | lora_stmt | stats_stmt | cache_stmt)

infer_stmt ::= "INFER" string_expr
               "USING" "MODEL" string_expr
               ["WITH" "LORA" string_expr]
               ["OPTIONS" object_expr]

rag_stmt ::= "RAG" string_expr
             "FROM" "COLLECTION" string_expr
             ["TOP" number_expr]
             ["WHERE" condition_expr]
             ["SIMILARITY" ">" number_expr]
             "USING" ["MODEL" string_expr]
             ["WITH" "LORA" string_expr]
             ["OPTIONS" object_expr]

embed_stmt ::= "EMBED" string_expr
               "USING" "MODEL" string_expr
               ["OPTIONS" object_expr]
               ["RETURN" "VECTOR"]

model_stmt ::= "MODEL" (load_model | unload_model | list_models | ingest_model)

load_model ::= "LOAD" string_expr
               "FROM" (path_expr | "BLOB" urn_expr)
               ["OPTIONS" object_expr]
               ["PIN"]

unload_model ::= "UNLOAD" string_expr

list_models ::= "LIST" ["WHERE" condition_expr]

ingest_model ::= "INGEST" string_expr
                 "FROM" "BLOB" path_expr
                 ["VERSION" string_expr]
                 ["REPLICATE" "TO" (("ALL" | shard_list))]

lora_stmt ::= "LORA" (load_lora | unload_lora | list_loras)

load_lora ::= "LOAD" string_expr
              "FOR" "MODEL" string_expr
              "FROM" path_expr
              ["SCALE" number_expr]

unload_lora ::= "UNLOAD" string_expr "FOR" "MODEL" string_expr

list_loras ::= "LIST" ["FOR" "MODEL" string_expr] ["WHERE" condition_expr]

stats_stmt ::= "STATS" ["RETURN" field_list]

cache_stmt ::= "CACHE" (cache_stats | cache_clear)

cache_stats ::= "STATS" ["RETURN" field_list]

cache_clear ::= "CLEAR" ("RESPONSE" | "PREFIX" | "ALL")
```
