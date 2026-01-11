# LoRA AQL Function Reference

Complete reference for LoRA (Low-Rank Adaptation) functions in ThemisDB's ArangoDB Query Language (AQL).

## Overview

ThemisDB provides 7 native AQL functions for managing and using LoRA adapters directly in queries:

| Function | Purpose | Returns |
|----------|---------|---------|
| [`LORA_TRAIN`](#lora_train) | Train a LoRA adapter | Job information |
| [`LORA_QUERY`](#lora_query) | Execute inference with adapter | Generated text |
| [`LORA_SIMILAR`](#lora_similar) | Find similar adapters | Array of adapters |
| [`LORA_PATH`](#lora_path) | Find adaptation path | Array of path steps |
| [`LORA_STATS`](#lora_stats) | Get adapter statistics | Statistics object |
| [`LORA_RECOMMEND`](#lora_recommend) | Recommend best adapter | Recommendation object |
| [`LORA_LINEAGE`](#lora_lineage) | Get version history | Array of versions |

## Function Details

### LORA_TRAIN

Train a LoRA adapter on a dataset.

**Signature:**
```aql
LORA_TRAIN(
  adapter_id: string,
  base_model: string,
  dataset: object,
  config: object
) -> object
```

**Parameters:**
- `adapter_id` (string, required): Unique identifier for the adapter
- `base_model` (string, required): Base model name (e.g., "llama-2-7b")
- `dataset` (object, required): Training dataset with samples
- `config` (object, optional): Training hyperparameters

**Dataset Structure:**
```json
{
  "task": "documentation_qa",
  "samples": [
    {
      "input": "What is ThemisDB?",
      "output": "ThemisDB is a distributed database...",
      "metadata": {}
    }
  ]
}
```

**Config Options:**
```json
{
  "rank": 8,              // LoRA rank (default: 8)
  "alpha": 16,            // LoRA alpha (default: 16)
  "learning_rate": 0.0003,  // Learning rate (default: 0.0003)
  "epochs": 3,            // Training epochs (default: 3)
  "batch_size": 32,       // Batch size (default: 32)
  "dropout": 0.1          // Dropout rate (default: 0.1)
}
```

**Returns:**
```json
{
  "adapter_id": "themis_help_lora",
  "version": "v1.0",
  "status": "training",
  "job_id": "job_123",
  "estimated_completion": "2026-01-11T16:00:00Z"
}
```

**Examples:**

Basic training:
```aql
FOR doc IN training_datasets
  FILTER doc.task == "documentation_qa"
  RETURN LORA_TRAIN(
    "themis_help_lora",
    "llama-2-7b",
    doc,
    {rank: 8, alpha: 16, learning_rate: 0.0003, epochs: 3}
  )
```

Batch training multiple adapters:
```aql
FOR dataset IN training_datasets
  FILTER dataset.status == "ready"
  FILTER dataset.samples >= 1000
  RETURN LORA_TRAIN(
    CONCAT(dataset.task, "_lora"),
    dataset.base_model,
    dataset,
    {rank: 8, alpha: 16}
  )
```

---

### LORA_QUERY

Execute an inference query using a LoRA adapter.

**Signature:**
```aql
LORA_QUERY(
  model_id: string,
  adapter_id: string,
  prompt: string,
  options: object
) -> string
```

**Parameters:**
- `model_id` (string, required): Base model identifier
- `adapter_id` (string, required): LoRA adapter identifier
- `prompt` (string, required): Input prompt text
- `options` (object, optional): Generation options

**Options:**
```json
{
  "max_tokens": 500,      // Maximum tokens to generate
  "temperature": 0.7,     // Sampling temperature (0.0-2.0)
  "top_p": 0.9,           // Nucleus sampling threshold
  "top_k": 50             // Top-k sampling
}
```

**Returns:** Generated text string

**Examples:**

Single query:
```aql
LET answer = LORA_QUERY(
  "llama-2-7b",
  "themis_help_lora",
  "How do I configure replication?",
  {max_tokens: 500, temperature: 0.7}
)
RETURN answer
```

Batch processing:
```aql
FOR question IN user_questions
  FILTER question.category == "documentation"
  RETURN {
    question: question.text,
    answer: LORA_QUERY(
      "llama-2-7b",
      "themis_help_lora",
      question.text,
      {max_tokens: 500, temperature: 0.7}
    ),
    timestamp: DATE_NOW()
  }
```

Adaptive routing:
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

---

### LORA_SIMILAR

Find similar LoRA adapters based on vector embeddings.

**Signature:**
```aql
LORA_SIMILAR(
  adapter_id: string,
  k: number,
  threshold: number
) -> array<object>
```

**Parameters:**
- `adapter_id` (string, required): Source adapter identifier
- `k` (number, required): Number of similar adapters to return
- `threshold` (number, optional): Similarity threshold (0.0-1.0, default: 0.0)

**Returns:**
```json
[
  {
    "adapter_id": "themis_docs_v2",
    "score": 0.94,
    "task": "documentation_qa",
    "base_model": "llama-2-7b"
  }
]
```

**Examples:**

Find top 5 similar adapters:
```aql
LET similar = LORA_SIMILAR("themis_help_lora", 5, 0.85)

FOR adapter IN similar
  RETURN {
    adapter_id: adapter.adapter_id,
    similarity: adapter.score,
    task: adapter.task,
    base_model: adapter.base_model
  }
```

Discover adapter clusters:
```aql
FOR adapter IN @@adapters
  LET similar = LORA_SIMILAR(adapter.adapter_id, 10, 0.8)
  FILTER LENGTH(similar) >= 3
  RETURN {
    cluster_center: adapter.adapter_id,
    cluster_size: LENGTH(similar),
    members: similar[*].adapter_id
  }
```

---

### LORA_PATH

Find adaptation path between models through LoRA adapters (graph traversal).

**Signature:**
```aql
LORA_PATH(
  start_model: string,
  end_model: string,
  max_depth: number
) -> array<object>
```

**Parameters:**
- `start_model` (string, required): Starting model identifier
- `end_model` (string, required): Target model identifier
- `max_depth` (number, optional): Maximum traversal depth (default: 5)

**Returns:**
```json
[
  {"node": "llama-2-7b", "type": "model", "edge": null},
  {"node": "adapter_upscale", "type": "adapter", "edge": "ADAPTED_WITH"},
  {"node": "llama-2-13b", "type": "model", "edge": "QUANTIZED_FROM"}
]
```

**Examples:**

Find path between models:
```aql
LET path = LORA_PATH("llama-2-7b", "llama-2-13b", 3)

FOR step IN path
  RETURN {
    node: step.node_id,
    type: step.node_type,
    edge: step.edge_type
  }
```

Analyze model evolution:
```aql
FOR source IN base_models
  FOR target IN target_models
    FILTER source != target
    LET path = LORA_PATH(source, target, 5)
    FILTER LENGTH(path) > 0
    RETURN {
      from: source,
      to: target,
      path_length: LENGTH(path),
      adapters_used: path[* FILTER CURRENT.type == "adapter"].node
    }
```

---

### LORA_STATS

Get statistics and metrics for LoRA adapters.

**Signature:**
```aql
LORA_STATS(
  adapter_id: string,
  metrics: array<string>
) -> object
```

**Parameters:**
- `adapter_id` (string, required): Adapter identifier
- `metrics` (array, optional): List of metric names to retrieve (empty = all metrics)

**Available Metrics:**
- `validation_accuracy`: Validation set accuracy
- `inference_count`: Total number of inferences
- `avg_latency`: Average latency in milliseconds
- `cache_hit_rate`: Cache hit rate (0.0-1.0)
- `last_used`: Timestamp of last use

**Returns:**
```json
{
  "validation_accuracy": 0.92,
  "inference_count": 12345,
  "avg_latency_ms": 45,
  "cache_hit_rate": 0.84,
  "last_used": "2026-01-11T15:00:00Z"
}
```

**Examples:**

Get all metrics:
```aql
LET stats = LORA_STATS("themis_help_lora", [])
RETURN stats
```

Get specific metrics:
```aql
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

Performance analysis:
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

---

### LORA_RECOMMEND

Recommend best LoRA adapter for a query/task.

**Signature:**
```aql
LORA_RECOMMEND(
  query: string,
  model_id: string,
  task: string,
  options: object
) -> object
```

**Parameters:**
- `query` (string, required): Input query or prompt
- `model_id` (string, required): Base model identifier
- `task` (string, required): Task type (e.g., "documentation_qa")
- `options` (object, optional): Recommendation options

**Options:**
```json
{
  "min_accuracy": 0.85,     // Minimum acceptable accuracy
  "max_latency_ms": 100     // Maximum acceptable latency
}
```

**Returns:**
```json
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

**Examples:**

Get recommendation:
```aql
LET recommendation = LORA_RECOMMEND(
  "How do I configure replication?",
  "llama-2-7b",
  "documentation_qa",
  {min_accuracy: 0.85, max_latency_ms: 100}
)
RETURN recommendation
```

Adaptive query routing:
```aql
FOR query IN user_queries
  LET adapter = LORA_RECOMMEND(
    query.text,
    "llama-2-7b",
    query.category,
    {}
  )
  LET answer = LORA_QUERY(
    "llama-2-7b",
    adapter.adapter_id,
    query.text,
    {}
  )
  RETURN {
    query: query.text,
    answer: answer,
    adapter: adapter.adapter_id,
    confidence: adapter.confidence
  }
```

---

### LORA_LINEAGE

Get complete lineage/versioning history of an adapter.

**Signature:**
```aql
LORA_LINEAGE(
  adapter_id: string,
  depth: number
) -> array<object>
```

**Parameters:**
- `adapter_id` (string, required): Adapter identifier
- `depth` (number, optional): Maximum lineage depth (default: 10)

**Returns:**
```json
[
  {"version": "v1.0", "parent": null, "created": "2026-01-01T00:00:00Z"},
  {"version": "v1.1", "parent": "v1.0", "created": "2026-01-05T00:00:00Z"},
  {"version": "v2.0", "parent": "v1.1", "created": "2026-01-10T00:00:00Z"}
]
```

**Examples:**

Get version history:
```aql
FOR adapter IN @@adapters
  FILTER adapter.adapter_id == "themis_help_lora"
  RETURN {
    current: adapter,
    lineage: LORA_LINEAGE(adapter.adapter_id, 10)
  }
```

Model evolution analysis:
```aql
LET lineage = LORA_LINEAGE("themis_help_lora", 100)

FOR version IN lineage
  LET stats = LORA_STATS(version.version_id, ["validation_accuracy"])
  RETURN {
    version: version.version,
    accuracy: stats.validation_accuracy,
    improvement: version.parent ? 
      stats.validation_accuracy - LORA_STATS(version.parent, ["validation_accuracy"]).validation_accuracy 
      : 0,
    created: version.created
  }
```

## Complete Examples

### Example 1: Adaptive Query Routing

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
    adapter_used: adapter.adapter_id,
    confidence: adapter.confidence
  }
```

### Example 2: Batch Training

```aql
FOR dataset IN training_datasets
  FILTER dataset.status == "ready"
  FILTER dataset.samples >= 1000
  RETURN LORA_TRAIN(
    CONCAT(dataset.task, "_lora"),
    dataset.base_model,
    dataset,
    {rank: 8, alpha: 16}
  )
```

### Example 3: Adapter Performance Analysis

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
    latency: stats.avg_latency_ms
  }
```

### Example 4: Model Evolution Tracking

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

## Error Handling

All LoRA functions handle errors gracefully:

- **Missing adapters**: Return null or empty results
- **Invalid parameters**: Throw exceptions with helpful messages
- **Training failures**: Return error in job status
- **Query failures**: Return error message string

Example error handling:
```aql
LET result = LORA_QUERY("llama-2-7b", "nonexistent", "test", {})
RETURN result IS_STRING ? result : "Error occurred"
```

## Performance Considerations

- **LORA_TRAIN**: Asynchronous by default, use job_id to check status
- **LORA_QUERY**: Can be parallelized across multiple queries
- **LORA_SIMILAR**: Uses vector indexes for fast similarity search
- **LORA_PATH**: Graph traversal complexity depends on max_depth
- **LORA_STATS**: Fast constant-time lookup
- **LORA_RECOMMEND**: Linear scan of adapters, consider caching
- **LORA_LINEAGE**: Linear in depth, efficient for reasonable depths

## Related Documentation

- [LoRA Usage Examples](../LORA_USAGE_EXAMPLES.md)
- [LoRA Framework Guide](../LORA_FRAMEWORK_ANALYSIS.md)
- [AQL Reference](./docs/aql_reference.md)

## Version History

- v1.0.0 (2026-01-11): Initial release with all 7 functions
