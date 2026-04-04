# Ethics AI Plugin - ThemisDB Architecture Integration

**Version:** 2.0 (BaseEntity Native)  
**Date:** 2026-01-29  
**Status:** Adapted to ThemisDB Architecture

---

## Overview

The Ethics AI Plugin is now fully integrated with ThemisDB's architecture, using:
- **BaseEntity** for unified storage (no SQL tables)
- **AQL** for all queries (no SQL)
- **Direct storage integration** (no wrappers)

---

## Architecture Principles

### 1. No Duplicate Structures ✅

**Before (Wrong):**
```
Plugin → Wrapper Classes → Storage Managers → ThemisDB
         (EthicsGraphStorage)
         (EthicsRelationalStorage)
         (EthicsVectorStorage)
```

**After (Correct):**
```
Plugin → BaseEntity → RocksDB/QueryEngine → ThemisDB
         (Direct Integration)
```

### 2. BaseEntity as Canonical Storage

All ethics data is stored as BaseEntity instances:

```cpp
// Ethical Argument as BaseEntity
BaseEntity::FieldMap fields;
fields["philosophy_school"] = std::string("kant");
fields["argument_type"] = std::string("pro");
fields["content"] = std::string("All persons have inherent dignity...");
fields["strength"] = std::string("strong");
fields["created_at"] = int64_t(timestamp);
fields["principle_basis"] = std::string("[\"autonomy\",\"dignity\"]"); // JSON

BaseEntity entity("arg_001", fields);
```

### 3. ThemisDB Key Schema

Follow ThemisDB's key patterns:

| Entity Type | Key Pattern | Example |
|-------------|-------------|---------|
| Argument | `entity:ethics_arguments:{id}` | `entity:ethics_arguments:arg_001` |
| Decision | `entity:ethics_decisions:{id}` | `entity:ethics_decisions:dec_123` |
| Profile | `entity:ethics_profiles:{school}` | `entity:ethics_profiles:kant` |
| Debate | `entity:ethics_debates:{id}` | `entity:ethics_debates:debate_456` |

---

## Data Storage

### Collections

ThemisDB Collections (not SQL tables):
- **ethics_arguments**: Ethical arguments
- **ethics_decisions**: Ethical decisions
- **ethics_debates**: Debate sessions
- **ethics_profiles**: Philosophy profiles

### BaseEntity Fields

#### Ethical Argument Fields
```cpp
{
    "_key": "arg_001",                    // Primary key
    "philosophy_school": "kant",          // String
    "argument_type": "pro",               // Enum as string
    "content": "...",                     // Text content
    "strength": "strong",                 // Enum as string
    "created_at": 1706515200,             // Unix timestamp (int64)
    "principle_basis": "[\"autonomy\"]",  // JSON array as string
    "counterarguments": "[\"arg_002\"]",  // JSON array as string
    "supports": "[\"arg_003\"]"           // JSON array as string
}
```

#### Ethical Decision Fields
```cpp
{
    "_key": "dec_123",
    "dilemma_id": "dilemma_456",
    "decision_text": "...",
    "primary_philosophy": "kant",
    "confidence": 0.85,                   // Double
    "consensus_level": 0.7,              // Double
    "created_at": 1706515200,
    "supporting_philosophies": "[...]",  // JSON
    "argument_chain_ids": "[...]"        // JSON
}
```

#### Philosophy Profile Fields
```cpp
{
    "_key": "kant",
    "name": "Kantian Ethics",
    "founder": "Immanuel Kant",
    "historical_context": "...",
    "main_thesis": "...",
    "secondary_thesis": "...",
    "decision_framework": "...",
    "strengths": "...",
    "weaknesses": "...",
    "principles": "[...]"                // JSON array
}
```

---

## AQL Queries

### Basic Queries

#### Get Argument by ID
```cpp
std::string aql = R"(
    FOR arg IN ethics_arguments
    FILTER arg._key == @argument_id
    RETURN arg
)";
```

#### Get Arguments by Philosophy
```cpp
std::string aql = R"(
    FOR arg IN ethics_arguments
    FILTER arg.philosophy_school == @school
    LIMIT @limit
    RETURN arg
)";
```

#### Search Arguments by Content
```cpp
std::string aql = R"(
    FOR arg IN ethics_arguments
    FILTER CONTAINS(LOWER(arg.content), LOWER(@search_text))
    LIMIT @limit
    RETURN arg
)";
```

### Vector Similarity Queries

#### Find Similar Dilemmas
```cpp
std::string aql = R"(
    FOR doc IN ethics_dilemmas
    LET similarity = VECTOR_COSINE_SIMILARITY(doc.embedding, @query_vector)
    FILTER similarity >= @threshold
    SORT similarity DESC
    LIMIT @limit
    RETURN {
        id: doc._key,
        description: doc.description,
        similarity: similarity
    }
)";
```

### Graph Traversal Queries

#### Traverse Argument Chain
```cpp
std::string aql = R"(
    FOR v, e, p IN 1..@max_depth OUTBOUND @start_id
    GRAPH 'ethics_arguments_graph'
    RETURN {
        vertex: v,
        edge: e,
        path: p.vertices[*]._key,
        depth: LENGTH(p.vertices) - 1
    }
)";
```

#### Find Shortest Path
```cpp
std::string aql = R"(
    FOR v, e IN OUTBOUND SHORTEST_PATH
    @start_id TO @end_id
    GRAPH 'ethics_arguments_graph'
    OPTIONS {maxDepth: @max_depth}
    RETURN {vertex: v, edge: e}
)";
```

### Complex RAG Context Query

```cpp
std::string aql = R"(
    LET similar_dilemmas = (
        FOR doc IN ethics_dilemmas
        LET similarity = VECTOR_COSINE_SIMILARITY(doc.embedding, @query_vector)
        FILTER similarity >= 0.65
        SORT similarity DESC
        LIMIT 5
        RETURN doc
    )
    
    LET philosophy_args = (
        FOR school IN @schools
            FOR arg IN ethics_arguments
            FILTER arg.philosophy_school == school
            LIMIT 3
            RETURN arg
    )
    
    LET best_practices = (
        FOR arg IN ethics_arguments
        FILTER arg.quality_score >= 0.8
        LIMIT 5
        RETURN arg
    )
    
    RETURN {
        similar_dilemmas: similar_dilemmas,
        philosophy_arguments: philosophy_args,
        best_practices: best_practices
    }
)";
```

---

## Code Examples

### Storing an Argument

```cpp
#include "plugins/ethics_ai/argument_store.h"
#include "storage/rocksdb_wrapper.h"

// Initialize store with ThemisDB storage
auto storage = std::make_shared<RocksDBWrapper>(...);
ArgumentStore store;
store.initialize(storage);

// Create argument
EthicalArgument arg;
arg.id = "arg_001";
arg.philosophy_school = "kant";
arg.argument_type = ArgumentType::PRO;
arg.content = "All persons have inherent dignity...";
arg.strength = ArgumentStrength::STRONG;
arg.created_at = std::chrono::system_clock::now();

// Store (converts to BaseEntity internally)
Status status = store.storeArgument(arg);
```

### Retrieving Arguments

```cpp
// Get single argument
auto result = store.getArgument("arg_001");
if (std::holds_alternative<EthicalArgument>(result)) {
    EthicalArgument arg = std::get<EthicalArgument>(result);
    std::cout << "Argument: " << arg.content << std::endl;
}

// Get by philosophy
auto results = store.getArgumentsByPhilosophy(
    "kant",
    {ArgumentType::PRO, ArgumentType::CONTRA},
    20
);
```

### Using AQL Directly

```cpp
#include "plugins/ethics_ai/ethics_aql_queries.h"
#include "query/query_engine.h"

// Get query template
std::string aql = EthicsAQLQueries::findSimilarDilemmas();

// Prepare parameters
nlohmann::json params = {
    {"query_vector", embedding_vector},
    {"threshold", 0.65},
    {"limit", 10}
};

// Execute via QueryEngine
auto result = query_engine->execute(aql, params);
```

---

## Integration with ThemisDB Features

### Vector Search Integration

When ThemisDB's VectorIndexManager is connected:

```cpp
// Store argument with vector embedding
std::vector<float> embedding = generateEmbedding(argument.content);

// BaseEntity will store the embedding
BaseEntity entity = EthicsBaseEntityAdapter::toBaseEntity(argument);
entity.setField("embedding", embedding);

// VectorIndexManager will automatically index it
```

### Graph Index Integration

Argument relationships as graph edges:

```cpp
// Create edge for "supports" relationship
// FROM: ethics_arguments/arg_001
// TO: ethics_arguments/arg_002
// Type: "supports"

// ThemisDB GraphIndexManager handles edge storage
// Keys: graph:out:arg_001:edge_123, graph:in:arg_002:edge_123
```

### Secondary Index Integration

Automatic indexing on philosophy_school, argument_type:

```cpp
// ThemisDB creates secondary index entries:
// idx:ethics_arguments:philosophy_school:kant:arg_001
// idx:ethics_arguments:argument_type:pro:arg_001
```

---

## Comparison: Old vs New

### Storage

| Aspect | Old (Wrong) | New (Correct) |
|--------|-------------|---------------|
| Storage | Multiple wrapper classes | Direct BaseEntity |
| Tables | SQL CREATE TABLE | No tables, BaseEntity |
| Keys | Custom format | ThemisDB key schema |
| Queries | SQL-like | AQL |
| Integration | Wrappers | Direct |

### Code Volume

| Component | Old | New | Reduction |
|-----------|-----|-----|-----------|
| Storage wrappers | 1,663 lines | 0 lines | -100% |
| Integration | 2,043 lines | 877 lines | -57% |
| Query templates | 0 lines | 300 lines | +300 lines |
| Adapter | 0 lines | 400 lines | +400 lines |
| **Total** | **3,706 lines** | **1,577 lines** | **-57%** |

---

## Benefits of ThemisDB Integration

### 1. Unified Storage Layer
- Single BaseEntity model for all data
- No duplicate storage structures
- Consistent with ThemisDB philosophy

### 2. Native AQL Support
- All features available (vector, graph, fulltext)
- Complex queries possible
- Performance optimized by ThemisDB

### 3. Automatic Indexing
- Secondary indexes on fields
- Vector indexes for embeddings
- Graph indexes for relationships

### 4. ACID Transactions
- Multi-model updates atomic
- Cross-collection consistency
- ThemisDB transaction manager

### 5. Simplicity
- Less code to maintain
- Clearer integration points
- Follows ThemisDB patterns

---

## Future Enhancements

### Phase 1: Basic Integration ✅
- ✅ BaseEntity storage
- ✅ AQL query templates
- ✅ Direct storage integration
- ✅ Remove wrappers

### Phase 2: Advanced Features (TODO)
- [ ] Connect VectorIndexManager for embeddings
- [ ] Connect GraphIndexManager for traversals
- [ ] Use QueryEngine for AQL execution
- [ ] Add graph edges for relationships

### Phase 3: Optimization (TODO)
- [ ] Batch operations
- [ ] Caching layer
- [ ] Index optimization
- [ ] Query planning

---

## References

- **BaseEntity**: `include/storage/base_entity.h`
- **Key Schema**: `include/storage/key_schema.h`
- **AQL Queries**: `plugins/ethics_ai/ethics_aql_queries.h`
- **BaseEntity Adapter**: `plugins/ethics_ai/ethics_base_entity_adapter.h`
- **Argument Store**: `plugins/ethics_ai/argument_store.{h,cpp}`
- **ThemisDB Architecture**: `docs/de/architecture/architecture_base_entity.md`
