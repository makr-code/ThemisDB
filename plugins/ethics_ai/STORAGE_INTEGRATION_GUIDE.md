# Ethics AI Plugin - Storage Integration Guide

## Overview

This guide explains how to integrate the Ethics AI Plugin with ThemisDB's multi-model storage layer (Graph, Relational, Vector), analogous to existing integrations like geospatial, timeseries, and process mining features.

## Architecture

### Storage Layer Hierarchy

```
┌────────────────────────────────────────────────────────────────┐
│                    Ethics AI Plugin                             │
│                 (Business Logic Layer)                          │
├────────────────────────────────────────────────────────────────┤
│  PhilosophyLoader │ ArgumentStore │ DiscourseEngine │ Evaluator │
└──────────────────┬─────────────────────────────────────────────┘
                   │
         ┌─────────▼──────────┐
         │ EthicsStorageManager│
         │  (Coordinator)      │
         └─────────┬───────────┘
                   │
      ┌────────────┼────────────┐
      │            │            │
┌─────▼─────┐ ┌───▼────────┐ ┌▼──────────┐
│  Ethics   │ │  Ethics    │ │  Ethics   │
│  Graph    │ │ Relational │ │  Vector   │
│  Storage  │ │  Storage   │ │  Storage  │
└─────┬─────┘ └───┬────────┘ └┬──────────┘
      │            │            │
┌─────▼────────────▼────────────▼─────────────┐
│        ThemisDB Storage Layer                │
├──────────────────────────────────────────────┤
│ GraphIndexManager │ RocksDBWrapper │ Vector │
│                   │                │IndexMgr│
└──────────────────────────────────────────────┘
```

## Storage Backend Responsibilities

### 1. Graph Storage (EthicsGraphStorage)

**Purpose:** Store and traverse ethical argument relationships

**Data Model:**
```
Nodes:
- Type: "EthicalArgument"
- Properties:
  - id: string (primary key)
  - philosophy_school: string
  - argument_type: enum (pro/contra/rebuttal/synthesis)
  - strength: enum (weak/moderate/strong/decisive)
  - content: text
  - created_at: timestamp

Edges:
- Types: "supports", "counters", "rebuts", "synthesizes"
- Properties:
  - weight: float (0.0-1.0)
  - created_at: timestamp
```

**Operations:**
- Create argument nodes
- Create relationship edges
- Traverse chains (BFS, DFS, shortest path)
- Find all paths between arguments
- Get supporting/countering arguments
- Calculate PageRank (influence scores)

**Example Usage:**
```cpp
auto graph_storage = storage_manager->graph();

// Store argument as node
graph_storage.storeArgumentNode(argument);

// Create relationship
graph_storage.createArgumentEdge("arg_001", "arg_002", "supports", 0.9);

// Traverse chain
auto chain = graph_storage.traverseArgumentChain("arg_001", 5, "both", "BFS");

// Calculate influence
auto influence = graph_storage.calculateArgumentInfluence(20, 0.85);
```

### 2. Relational Storage (EthicsRelationalStorage)

**Purpose:** Structured queries and metadata management

**Schema:**

```sql
-- Arguments table
CREATE TABLE ethics_arguments (
    id TEXT PRIMARY KEY,
    philosophy_school TEXT NOT NULL,
    argument_type TEXT NOT NULL,
    content TEXT NOT NULL,
    strength TEXT NOT NULL,
    created_at TIMESTAMP NOT NULL,
    INDEX idx_philosophy (philosophy_school),
    INDEX idx_type (argument_type),
    INDEX idx_strength (strength)
);

-- Decisions table
CREATE TABLE ethics_decisions (
    decision_id TEXT PRIMARY KEY,
    dilemma_id TEXT NOT NULL,
    decision_text TEXT NOT NULL,
    primary_philosophy TEXT NOT NULL,
    confidence REAL NOT NULL,
    consensus_level REAL NOT NULL,
    created_at TIMESTAMP NOT NULL,
    INDEX idx_philosophy (primary_philosophy),
    INDEX idx_confidence (confidence),
    INDEX idx_consensus (consensus_level)
);

-- Evaluations table
CREATE TABLE ethics_evaluations (
    decision_id TEXT PRIMARY KEY,
    overall_score REAL NOT NULL,
    decision_quality_score REAL NOT NULL,
    consistency_score REAL NOT NULL,
    fairness_score REAL NOT NULL,
    alignment_score REAL NOT NULL,
    transparency_score REAL NOT NULL,
    FOREIGN KEY (decision_id) REFERENCES ethics_decisions(decision_id)
);

-- Debates table
CREATE TABLE ethics_debates (
    debate_id TEXT PRIMARY KEY,
    dilemma_description TEXT NOT NULL,
    category TEXT NOT NULL,
    created_at TIMESTAMP NOT NULL,
    INDEX idx_category (category)
);
```

**Operations:**
- Schema initialization
- Insert arguments/decisions/evaluations
- Query with filters (WHERE, ORDER BY, LIMIT)
- Aggregate statistics (COUNT, AVG, etc.)
- Foreign key relationships

**Example Usage:**
```cpp
auto relational_storage = storage_manager->relational();

// Initialize schema
relational_storage.initializeSchema();

// Store argument
relational_storage.storeArgument(argument);

// Query with filters
auto arguments = relational_storage.queryArguments(
    "kant",                          // philosophy_school
    {ArgumentType::PRO},             // argument_types
    ArgumentStrength::MODERATE,      // min_strength
    100,                             // limit
    "created_at"                     // order_by
);

// Get statistics
auto stats = relational_storage.getStatistics("kant");
// Returns: total_arguments, avg_confidence, etc.
```

### 3. Vector Storage (EthicsVectorStorage)

**Purpose:** Semantic similarity search

**Data Model:**
```
Index: "ethics_arguments"
- Dimension: 768 (or embedding model dimension)
- Metric: Cosine similarity
- Documents:
  - id: argument_id
  - vector: embedding (float[768])
  - metadata:
    - philosophy_school: string
    - argument_type: string
    - strength: string

Index: "ethics_decisions"
- Dimension: 768
- Documents:
  - id: decision_id
  - vector: embedding (float[768])
  - metadata:
    - category: string
    - confidence: float
```

**Operations:**
- Store embeddings with metadata
- Semantic similarity search
- Filtered search (by metadata)
- Clustering (k-means)
- Find similar stances

**Example Usage:**
```cpp
auto vector_storage = storage_manager->vector();

// Store argument embedding
std::vector<float> embedding = generateEmbedding(argument.content);
vector_storage.storeArgumentEmbedding(argument, embedding);

// Semantic search
auto similar = vector_storage.searchSimilarArguments(
    query_embedding,      // query vector
    "kant",              // philosophy filter
    20,                  // top_k
    0.65                 // min_similarity
);

// Cluster arguments
auto clusters = vector_storage.clusterArguments(5, "kant");
```

## Integration Steps

### Step 1: Connect Storage Managers

**File:** `plugins/ethics_ai/ethics_storage_integration.cpp`

Replace TODO comments with actual storage manager calls:

```cpp
// Example: Graph storage
Status EthicsGraphStorage::storeArgumentNode(const EthicalArgument& argument) {
    if (!graph_manager_) {
        return Status::Error("Graph manager not initialized");
    }
    
    // BEFORE (TODO):
    // TODO: Implement when GraphIndexManager interface is available
    
    // AFTER (Actual implementation):
    nlohmann::json properties = {
        {"philosophy_school", argument.philosophy_school},
        {"type", argumentTypeToString(argument.argument_type)},
        {"strength", argumentStrengthToString(argument.strength)},
        {"content", argument.content}
    };
    
    return graph_manager_->addNode(
        argument.id, 
        "EthicalArgument", 
        properties
    );
}
```

### Step 2: Update ArgumentStore

**File:** `plugins/ethics_ai/argument_store.cpp`

Replace in-memory storage with actual storage manager:

```cpp
class ArgumentStore {
private:
    // BEFORE:
    // std::map<std::string, EthicalArgument> arguments_;
    
    // AFTER:
    std::shared_ptr<EthicsStorageManager> storage_manager_;
    
public:
    Status initialize(const std::map<std::string, std::string>& config) {
        // Initialize storage manager
        auto graph_mgr = /* get from config */;
        auto relational = /* get from config */;
        auto vector_mgr = /* get from config */;
        
        storage_manager_ = std::make_shared<EthicsStorageManager>(
            graph_mgr, relational, vector_mgr
        );
        
        return storage_manager_->initialize();
    }
    
    Status storeArgument(const EthicalArgument& argument, bool store_vector) {
        // Generate embedding if requested
        std::optional<std::vector<float>> embedding;
        if (store_vector) {
            embedding = generateEmbedding(argument.content);
        }
        
        // Store across all backends
        return storage_manager_->storeArgumentMultiModel(argument, embedding);
    }
};
```

### Step 3: Integrate REST API

**File:** `src/server/http_server.cpp`

Add Ethics API handler to routing:

```cpp
#include "server/ethics_api_handler.h"

// In HttpServer constructor:
void HttpServer::initialize() {
    // ... existing initialization ...
    
    // Create Ethics API handler
    auto ethics_plugin = /* load from plugin manager */;
    auto ethics_storage = std::make_shared<EthicsStorageManager>(
        graph_index_,
        storage_,
        vector_index_
    );
    ethics_api_handler_ = std::make_unique<EthicsApiHandler>(
        ethics_plugin,
        ethics_storage,
        auth_middleware_
    );
}

// In route handler:
void HttpServer::handleRequest(const http::request<http::string_body>& req) {
    std::string target = req.target();
    
    if (target.starts_with("/ethics/")) {
        if (target == "/ethics/debate/init") {
            return ethics_api_handler_->handleDebateInit(req);
        }
        else if (target == "/ethics/decision/make") {
            return ethics_api_handler_->handleMakeDecision(req);
        }
        // ... other ethics endpoints ...
    }
    
    // ... existing routes ...
}
```

### Step 4: Configure CMake

**File:** `plugins/ethics_ai/CMakeLists.txt`

Add storage dependencies:

```cmake
target_link_libraries(ethics_ai_plugin
    PRIVATE
        themis_core
        graph_index_manager      # Add
        vector_index_manager     # Add
        rocksdb_wrapper         # Add
        nlohmann_json
)
```

### Step 5: Initialize in Plugin

**File:** `plugins/ethics_ai/ethics_ai_plugin.cpp`

```cpp
Status EthicsAIPlugin::initialize(const std::string& config_path) {
    // Load configuration
    auto config = loadConfig(config_path);
    
    // Initialize storage manager
    storage_manager_ = std::make_shared<EthicsStorageManager>(
        getGraphManager(),
        getRelationalStorage(),
        getVectorManager()
    );
    
    auto status = storage_manager_->initialize();
    if (!status.isOK()) {
        return status;
    }
    
    // Initialize argument store with storage
    argument_store_ = std::make_shared<ArgumentStore>();
    argument_store_->setStorageManager(storage_manager_);
    
    return Status::OK();
}
```

## Query Patterns

### Pattern 1: Simple Relational Query

```cpp
// Find all strong Kantian pro arguments
auto arguments = storage_manager->relational().queryArguments(
    "kant",
    {ArgumentType::PRO},
    ArgumentStrength::STRONG,
    50
);
```

### Pattern 2: Graph Traversal

```cpp
// Find all arguments in a chain
auto chain = storage_manager->graph().traverseArgumentChain(
    "arg_001",
    5,           // max_depth
    "forward",   // direction
    "BFS"        // algorithm
);
```

### Pattern 3: Semantic Search

```cpp
// Find semantically similar arguments
auto embedding = generateEmbedding("Should AI respect privacy?");
auto similar = storage_manager->vector().searchSimilarArguments(
    embedding,
    "",          // all philosophies
    20,          // top 20
    0.65         // min similarity
);
```

### Pattern 4: Multi-Model Query

```cpp
// Complex query across all backends
nlohmann::json query = {
    {"operation", "find_related_arguments"},
    {"start_argument", "arg_001"},
    {"filters", {
        {"philosophy_schools", {"kant", "utilitarianism"}},
        {"min_strength", "moderate"}
    }},
    {"semantic_threshold", 0.7},
    {"max_results", 50}
};

auto results = storage_manager->executeComplexQuery(query);
// Returns: JSON with arguments from graph traversal + semantic search + relational filters
```

### Pattern 5: Influence Analysis

```cpp
// Calculate and store influence scores
auto influence = storage_manager->graph().calculateArgumentInfluence(20, 0.85);

// Store in relational for querying
for (const auto& [arg_id, score] : influence) {
    // UPDATE ethics_arguments SET influence_score = score WHERE id = arg_id
}

// Query top influential arguments
// SELECT * FROM ethics_arguments ORDER BY influence_score DESC LIMIT 10
```

## Testing

### Unit Tests

**File:** `tests/test_ethics_storage_integration.cpp`

```cpp
TEST(EthicsStorageIntegration, StoreArgumentMultiModel) {
    auto storage = createTestStorageManager();
    
    EthicalArgument arg = createTestArgument();
    auto embedding = generateTestEmbedding();
    
    auto status = storage->storeArgumentMultiModel(arg, embedding);
    ASSERT_TRUE(status.isOK());
    
    // Verify in relational
    auto rel_result = storage->relational().queryArguments("kant", {}, {}, 10);
    ASSERT_TRUE(std::holds_alternative<std::vector<EthicalArgument>>(rel_result));
    
    // Verify in graph
    auto graph_result = storage->graph().getSupportingArguments(arg.id);
    ASSERT_TRUE(std::holds_alternative<std::vector<std::string>>(graph_result));
    
    // Verify in vector
    auto vec_result = storage->vector().searchSimilarArguments(embedding, "", 10, 0.5);
    ASSERT_TRUE(std::holds_alternative<std::vector<std::pair<std::string, double>>>(vec_result));
}
```

### Integration Tests

**File:** `tests/integration/test_ethics_full_flow.cpp`

```cpp
TEST(EthicsFullFlow, DebateToDecisionToEvaluation) {
    // Initialize plugin with storage
    auto plugin = createEthicsPlugin();
    
    // 1. Store arguments
    for (auto& arg : test_arguments) {
        plugin->storeArgument(arg, true);
    }
    
    // 2. Initialize debate
    auto debate = plugin->initializeDebate(
        "Test dilemma",
        {"kant", "utilitarianism"},
        "test"
    );
    
    // 3. Make decision (uses graph + vector + relational)
    auto decision = plugin->makeDecision(
        "Test dilemma",
        {"kant", "utilitarianism"},
        "test",
        true  // use RAG
    );
    
    // 4. Evaluate
    auto evaluation = plugin->evaluateDecision(decision, {});
    
    // Verify data in all backends
    // ...
}
```

## Performance Considerations

### Caching Strategy

```cpp
class EthicsStorageManager {
private:
    // Cache frequently accessed data
    LRUCache<std::string, EthicalArgument> argument_cache_;
    LRUCache<std::string, std::vector<float>> embedding_cache_;
    
public:
    Status storeArgumentMultiModel(...) {
        // Store in backends
        // ...
        
        // Update cache
        argument_cache_.put(argument.id, argument);
        if (embedding) {
            embedding_cache_.put(argument.id, *embedding);
        }
    }
};
```

### Batch Operations

```cpp
// Batch insert for performance
Status EthicsStorageManager::storeArgumentsBatch(
    const std::vector<EthicalArgument>& arguments,
    const std::vector<std::vector<float>>& embeddings) {
    
    // Batch relational
    relational_storage_->beginTransaction();
    for (const auto& arg : arguments) {
        relational_storage_->storeArgument(arg);
    }
    relational_storage_->commit();
    
    // Batch graph
    for (const auto& arg : arguments) {
        graph_storage_->storeArgumentNode(arg);
    }
    
    // Batch vector
    vector_storage_->bulkInsert(arguments, embeddings);
    
    return Status::OK();
}
```

### Indexing

```sql
-- Ensure proper indexing for common queries
CREATE INDEX idx_ethics_args_school_type 
ON ethics_arguments(philosophy_school, argument_type);

CREATE INDEX idx_ethics_decisions_confidence 
ON ethics_decisions(confidence DESC);

CREATE INDEX idx_ethics_evaluations_quality 
ON ethics_evaluations(decision_quality_score DESC);
```

## Monitoring

### Metrics to Track

```cpp
// In EthicsStorageManager
std::map<std::string, double> getStorageMetrics() const {
    return {
        // Graph metrics
        {"graph_nodes_total", graph_storage_->getNodeCount()},
        {"graph_edges_total", graph_storage_->getEdgeCount()},
        {"graph_traversal_time_avg_ms", graph_traversal_metrics_.avg()},
        
        // Relational metrics
        {"relational_arguments_total", relational_storage_->getArgumentCount()},
        {"relational_decisions_total", relational_storage_->getDecisionCount()},
        {"relational_query_time_avg_ms", relational_query_metrics_.avg()},
        
        // Vector metrics
        {"vector_embeddings_total", vector_storage_->getEmbeddingCount()},
        {"vector_search_time_avg_ms", vector_search_metrics_.avg()},
        {"vector_index_size_mb", vector_storage_->getIndexSizeMB()},
        
        // Cache metrics
        {"cache_hit_rate", argument_cache_.hitRate()},
        {"cache_size_mb", argument_cache_.sizeMB()}
    };
}
```

### Prometheus Export

```cpp
std::string EthicsStorageManager::getPrometheusMetrics() const {
    auto metrics = getStorageMetrics();
    
    std::stringstream ss;
    ss << "# HELP ethics_storage_graph_nodes_total Total graph nodes\n";
    ss << "# TYPE ethics_storage_graph_nodes_total gauge\n";
    ss << "ethics_storage_graph_nodes_total " << metrics["graph_nodes_total"] << "\n";
    
    // ... more metrics ...
    
    return ss.str();
}
```

## Troubleshooting

### Common Issues

**Issue 1: Storage manager not initialized**
```
Error: Graph manager not initialized
```
Solution: Ensure storage manager is created before plugin initialization

**Issue 2: Schema not created**
```
Error: Table ethics_arguments does not exist
```
Solution: Call `relational_storage->initializeSchema()` during setup

**Issue 3: Vector index not found**
```
Error: Index ethics_arguments not found
```
Solution: Create vector index before storing embeddings

**Issue 4: Graph traversal timeout**
```
Error: Traversal exceeded timeout
```
Solution: Limit max_depth or add traversal timeout configuration

## Migration from In-Memory

### Phase 1: Dual-Write
```cpp
Status storeArgument(...) {
    // Write to old in-memory store
    in_memory_store_[arg.id] = arg;
    
    // Write to new storage (best effort)
    storage_manager_->storeArgumentMultiModel(arg, embedding);
}
```

### Phase 2: Dual-Read
```cpp
std::variant<EthicalArgument, Status> getArgument(const std::string& id) {
    // Try new storage first
    auto result = storage_manager_->relational().queryArguments(...);
    if (result has value) return result;
    
    // Fallback to in-memory
    if (in_memory_store_.contains(id)) {
        return in_memory_store_[id];
    }
    
    return Status::NotFound();
}
```

### Phase 3: Full Migration
```cpp
// Remove in-memory storage completely
// Only use storage_manager_
```

## Summary

The Ethics AI Plugin storage integration follows ThemisDB patterns:
- **Multi-model architecture** similar to geospatial/timeseries
- **Clear separation** between business logic and storage
- **Unified coordinator** for cross-backend queries
- **Performance-oriented** with caching and batching
- **Monitoring-ready** with Prometheus metrics

All integration points are clearly marked with TODO comments and ready for connection to actual storage managers.
