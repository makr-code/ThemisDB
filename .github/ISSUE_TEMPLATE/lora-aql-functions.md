---
name: LoRA Framework - AQL Functions
about: Implement AQL functions for LoRA adapter management and querying
title: '[FEATURE] Implement AQL Functions for LoRA Framework'
labels: ['enhancement', 'aql', 'lora-framework']
assignees: ''
---

## Description

Implement ArangoDB Query Language (AQL) user-defined functions (UDFs) to enable native querying and management of LoRA adapters directly from AQL queries.

## Motivation

AQL functions would:
- Enable LoRA operations within complex AQL queries
- Provide declarative adapter management
- Support data-driven adapter selection
- Enable batch operations on adapters
- Integrate LoRA with existing AQL workflows
- Allow adapter recommendations based on query patterns

## Proposed AQL Functions

### 1. LORA_TRAIN

Train a LoRA adapter on a dataset.

```aql
LORA_TRAIN(
  adapter_id: string,
  base_model: string,
  dataset: object,
  config: object
) -> object

// Example
FOR doc IN training_datasets
  FILTER doc.task == "documentation_qa"
  RETURN LORA_TRAIN(
    "themis_help_lora",
    "llama-2-7b",
    doc,
    {
      rank: 8,
      alpha: 16,
      learning_rate: 0.0003,
      epochs: 3
    }
  )
```

**Returns**:
```javascript
{
  "adapter_id": "themis_help_lora",
  "version": "v1.0",
  "status": "training",
  "job_id": "job_123",
  "estimated_completion": "2026-01-11T16:00:00Z"
}
```

### 2. LORA_QUERY

Execute an inference query using a LoRA adapter.

```aql
LORA_QUERY(
  model_id: string,
  adapter_id: string,
  prompt: string,
  options: object
) -> string

// Example
FOR question IN user_questions
  FILTER question.category == "documentation"
  RETURN {
    question: question.text,
    answer: LORA_QUERY(
      "llama-2-7b",
      "themis_help_lora",
      question.text,
      { max_tokens: 500, temperature: 0.7 }
    ),
    timestamp: DATE_NOW()
  }
```

**Returns**: Generated text response from LLM with LoRA adapter applied.

### 3. LORA_SIMILAR

Find similar LoRA adapters based on vector embeddings.

```aql
LORA_SIMILAR(
  adapter_id: string,
  k: number,
  threshold: number
) -> array<object>

// Example
LET source_adapter = "themis_help_lora"
LET similar = LORA_SIMILAR(source_adapter, 5, 0.85)

FOR adapter IN similar
  RETURN {
    adapter_id: adapter.adapter_id,
    similarity: adapter.score,
    task: adapter.task,
    base_model: adapter.base_model
  }
```

**Returns**:
```javascript
[
  {
    "adapter_id": "themis_docs_v2",
    "score": 0.94,
    "task": "documentation_qa",
    "base_model": "llama-2-7b"
  },
  // ... up to k results
]
```

### 4. LORA_PATH

Find adaptation path between models through LoRA adapters (graph traversal).

```aql
LORA_PATH(
  start_model: string,
  end_model: string,
  max_depth: number
) -> array<object>

// Example
LET path = LORA_PATH("llama-2-7b", "llama-2-13b", 3)

FOR step IN path
  RETURN {
    node: step.node_id,
    type: step.node_type,
    edge: step.edge_type
  }
```

**Returns** adaptation path through graph:
```javascript
[
  {"node": "llama-2-7b", "type": "model", "edge": null},
  {"node": "adapter_upscale", "type": "adapter", "edge": "ADAPTED_WITH"},
  {"node": "llama-2-13b", "type": "model", "edge": "QUANTIZED_FROM"}
]
```

### 5. LORA_STATS

Get statistics and metrics for LoRA adapters.

```aql
LORA_STATS(
  adapter_id: string,
  metrics: array<string>
) -> object

// Example
FOR adapter IN @@adapters
  FILTER adapter.status == "ready"
  RETURN {
    adapter_id: adapter.adapter_id,
    stats: LORA_STATS(
      adapter.adapter_id,
      ["validation_accuracy", "inference_count", "avg_latency"]
    )
  }
```

**Returns**:
```javascript
{
  "validation_accuracy": 0.92,
  "inference_count": 12345,
  "avg_latency_ms": 45,
  "cache_hit_rate": 0.84,
  "last_used": "2026-01-11T15:00:00Z"
}
```

### 6. LORA_RECOMMEND

Recommend best LoRA adapter for a query/task.

```aql
LORA_RECOMMEND(
  query: string,
  model_id: string,
  task: string,
  options: object
) -> object

// Example
LET query = "How do I configure replication?"
LET recommendation = LORA_RECOMMEND(
  query,
  "llama-2-7b",
  "documentation_qa",
  { min_accuracy: 0.85, max_latency_ms: 100 }
)

RETURN recommendation
```

**Returns**:
```javascript
{
  "adapter_id": "themis_help_lora",
  "confidence": 0.95,
  "reason": "High accuracy on documentation queries",
  "metrics": {
    "validation_accuracy": 0.92,
    "avg_latency_ms": 45
  }
}
```

### 7. LORA_LINEAGE

Get the complete lineage/versioning history of an adapter.

```aql
LORA_LINEAGE(
  adapter_id: string,
  depth: number
) -> array<object>

// Example
FOR adapter IN @@adapters
  FILTER adapter.adapter_id == "themis_help_lora"
  RETURN {
    current: adapter,
    lineage: LORA_LINEAGE(adapter.adapter_id, 10)
  }
```

**Returns** version history with parent/child relationships:
```javascript
[
  {"version": "v1.0", "parent": null, "created": "2026-01-01T00:00:00Z"},
  {"version": "v1.1", "parent": "v1.0", "created": "2026-01-05T00:00:00Z"},
  {"version": "v2.0", "parent": "v1.1", "created": "2026-01-10T00:00:00Z"}
]
```

## Implementation Details

### File Structure
```
src/aql/functions/
├── lora_functions.cpp (new)
├── lora_functions.h (new)
└── function_registry.cpp (register functions)
```

### Integration Points
- `LoRAOrchestrator` for CRUD operations
- `LoRAAdapterManager` for lifecycle management
- `LoRATrainingService` for training operations
- `LoRAStorageService` for querying
- Graph traversal for LORA_PATH
- Vector search for LORA_SIMILAR

### Function Registration
```cpp
void registerLoRAFunctions(FunctionRegistry& registry) {
  registry.registerUDF("LORA_TRAIN", loraTrainFunction);
  registry.registerUDF("LORA_QUERY", loraQueryFunction);
  registry.registerUDF("LORA_SIMILAR", loraSimilarFunction);
  registry.registerUDF("LORA_PATH", loraPathFunction);
  registry.registerUDF("LORA_STATS", loraStatsFunction);
  registry.registerUDF("LORA_RECOMMEND", loraRecommendFunction);
  registry.registerUDF("LORA_LINEAGE", loraLineageFunction);
}
```

### Error Handling
- Return null for missing adapters
- Throw exceptions for invalid parameters
- Log warnings for deprecated functions
- Provide helpful error messages

## Tasks

### Core Implementation
- [ ] Implement `LORA_TRAIN` function
- [ ] Implement `LORA_QUERY` function
- [ ] Implement `LORA_SIMILAR` function
- [ ] Implement `LORA_PATH` function (graph traversal)
- [ ] Implement `LORA_STATS` function
- [ ] Implement `LORA_RECOMMEND` function
- [ ] Implement `LORA_LINEAGE` function
- [ ] Register functions in AQL function registry

### Testing
- [ ] Unit tests for each function
- [ ] Integration tests with real AQL queries
- [ ] Performance tests (query execution time)
- [ ] Edge case testing (null inputs, invalid IDs)
- [ ] Concurrent query testing

### Documentation
- [ ] Add AQL function reference to documentation
- [ ] Create `LORA_AQL_REFERENCE.md` with examples
- [ ] Update `LORA_USAGE_EXAMPLES.md` with AQL examples
- [ ] Add interactive query examples
- [ ] Document function signatures and return types

### Examples
- [ ] Create 10+ example queries covering common use cases
- [ ] Add data-driven adapter selection examples
- [ ] Add batch operation examples
- [ ] Add graph traversal examples
- [ ] Add recommendation system examples

## Acceptance Criteria

- [ ] All 7 AQL functions implemented and registered
- [ ] Functions work in complex AQL queries
- [ ] Integration with LoRA framework complete
- [ ] Comprehensive error handling
- [ ] Unit tests with > 80% coverage
- [ ] Performance validated (< 100ms overhead)
- [ ] Complete function reference documentation
- [ ] 10+ working example queries
- [ ] Compatible with existing AQL syntax
- [ ] Functions accessible from web interface

## Example Use Cases

### 1. Adaptive Query Routing
```aql
FOR query IN user_queries
  LET adapter = LORA_RECOMMEND(
    query.text,
    "llama-2-7b",
    query.category,
    {}
  )
  RETURN {
    query: query.text,
    answer: LORA_QUERY(
      "llama-2-7b",
      adapter.adapter_id,
      query.text,
      {}
    ),
    adapter_used: adapter.adapter_id
  }
```

### 2. Batch Training
```aql
FOR dataset IN training_datasets
  FILTER dataset.status == "ready"
  FILTER dataset.samples >= 1000
  RETURN LORA_TRAIN(
    CONCAT(dataset.task, "_lora"),
    dataset.base_model,
    dataset,
    { rank: 8, alpha: 16 }
  )
```

### 3. Adapter Performance Analysis
```aql
FOR adapter IN @@adapters
  FILTER adapter.base_model == "llama-2-7b"
  LET stats = LORA_STATS(adapter.adapter_id, [
    "validation_accuracy",
    "inference_count",
    "avg_latency"
  ])
  SORT stats.validation_accuracy DESC
  LIMIT 10
  RETURN {
    adapter_id: adapter.adapter_id,
    accuracy: stats.validation_accuracy,
    usage: stats.inference_count,
    latency: stats.avg_latency
  }
```

### 4. Model Evolution Analysis
```aql
LET lineage = LORA_LINEAGE("themis_help_lora", 100)

FOR version IN lineage
  LET stats = LORA_STATS(version.version_id, ["validation_accuracy"])
  RETURN {
    version: version.version,
    accuracy: stats.validation_accuracy,
    improvement: version.parent ? 
      stats.validation_accuracy - LORA_STATS(version.parent, ["validation_accuracy"]).validation_accuracy 
      : 0
  }
```

## Related Files

- `src/llm/lora_framework/lora_orchestrator.h` - Framework interface
- `src/aql/function_registry.cpp` - AQL function registration
- `LORA_USAGE_EXAMPLES.md` - Usage documentation
- `docs/aql_reference.md` - AQL documentation

## References

- ArangoDB AQL documentation: https://docs.arangodb.com/stable/aql/
- AQL user-defined functions: https://docs.arangodb.com/stable/aql/user-functions/

## Priority

**Medium** - Enhances query capabilities but not blocking for core functionality.

## Estimated Effort

**Large** (20-24 hours)
- Function implementations: 10-12 hours
- AQL integration: 2-3 hours
- Testing: 4-5 hours
- Documentation: 3-4 hours
- Examples: 1-2 hours
