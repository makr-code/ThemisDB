# Ethics AI Plugin - Implementation Guide

## Overview

This document provides detailed guidance for completing and extending the Ethics AI Plugin implementation.

## Current Implementation Status

### ✅ Complete (Phase 1-8)

1. **Core Types and Structures** (`ethics_ai_types.h/cpp`)
   - All data structures defined
   - Enum conversion functions implemented
   - Status/error handling in place

2. **Plugin Interface** (`ethics_ai_plugin_interface.h`)
   - Complete IEthicsAIPlugin interface
   - All method signatures defined
   - Documentation complete

3. **Philosophy Loader** (`philosophy_loader.h/cpp`)
   - YAML parsing (with yaml-cpp)
   - Profile caching
   - Profile validation

4. **Argument Store** (`argument_store.h/cpp`)
   - In-memory storage implementation
   - CRUD operations for arguments, chains, decisions
   - Thread-safe operations
   - **Ready for storage integration**

5. **RAG Context Engine** (`rag_context_engine.h/cpp`)
   - Basic structure in place
   - Pattern stubs implemented
   - **Ready for AQL integration**

6. **Discourse Engine** (`discourse_engine.h/cpp`)
   - Debate initialization
   - Basic argument generation
   - Decision synthesis
   - **Ready for enhancement**

7. **Ethics Evaluator** (`ethics_evaluator.h/cpp`)
   - 5-dimension evaluation complete
   - Scoring algorithms implemented
   - Weighted averages

8. **Plugin Implementation** (`src/ethics_ai/ethics_ai_plugin.cpp`)
   - Full IEthicsAIPlugin implementation
   - Metrics collection
   - Prometheus export
   - Configuration management

### 🔄 In Progress / TODO

## Integration Tasks

### Task 1: Storage Manager Integration

**Priority:** HIGH  
**Complexity:** MEDIUM  
**Files:** `argument_store.h/cpp`

Replace in-memory storage with actual ThemisDB storage managers:

```cpp
// Current (in-memory):
std::map<std::string, EthicalArgument> arguments_;
std::map<std::string, ArgumentChain> chains_;
std::map<std::string, EthicalDecision> decisions_;

// Target (integrated):
std::shared_ptr<GraphManager> graph_mgr_;
std::shared_ptr<RelationalManager> relational_mgr_;
std::shared_ptr<VectorIndexManager> vector_mgr_;
std::shared_ptr<TimelineManager> timeline_mgr_;
```

**Steps:**
1. Add storage manager includes
2. Update `initialize()` to accept storage managers
3. Implement multi-model storage in `storeArgument()`:
   - Graph: Store argument relationships (counters, supports)
   - Relational: Store metadata (SQL INSERT/UPDATE)
   - Vector: Store embeddings (if `store_vector` is true)
   - Timeline: Store creation events
4. Update retrieval methods to query actual storage
5. Add transaction support

**Example Integration:**

```cpp
Status ArgumentStore::storeArgument(
    const EthicalArgument& argument, 
    bool store_vector) {
    
    // Start transaction
    auto txn = relational_mgr_->beginTransaction();
    
    try {
        // 1. Store in relational
        std::string sql = "INSERT INTO ethical_arguments "
                         "(id, philosophy_school, type, content, strength, created_at) "
                         "VALUES (?, ?, ?, ?, ?, ?)";
        relational_mgr_->execute(sql, {
            argument.id,
            argument.philosophy_school,
            argumentTypeToString(argument.argument_type),
            argument.content,
            argumentStrengthToString(argument.strength),
            formatTimestamp(argument.created_at)
        });
        
        // 2. Store relationships in graph
        for (const auto& counter_id : argument.counterarguments) {
            graph_mgr_->addEdge(argument.id, counter_id, "counters");
        }
        for (const auto& support_id : argument.supports) {
            graph_mgr_->addEdge(argument.id, support_id, "supports");
        }
        
        // 3. Store vector embedding if requested
        if (store_vector) {
            auto embedding = generateEmbedding(argument.content);
            vector_mgr_->insert("ethical_arguments", {
                .id = argument.id,
                .vector = embedding,
                .metadata = serializeMetadata(argument)
            });
        }
        
        // 4. Store timeline event
        timeline_mgr_->insert({
            .timestamp = argument.created_at,
            .event_type = "argument_created",
            .entity_id = argument.id,
            .data = serializeArgument(argument)
        });
        
        txn->commit();
        return Status::OK();
        
    } catch (const std::exception& e) {
        txn->rollback();
        return Status::Error(e.what());
    }
}
```

### Task 2: AQL Query Implementation

**Priority:** HIGH  
**Complexity:** MEDIUM-HIGH  
**Files:** `rag_context_engine.h/cpp`

Implement the 7 RAG query patterns using AQL:

#### Pattern 1: Textual Similarity Search

```cpp
std::variant<std::vector<std::string>, Status> 
RAGContextEngine::findSimilarDilemmas(
    const std::string& query_text,
    double threshold,
    size_t limit) {
    
    // Use ThemisDB's full-text search
    std::string aql = R"(
        SELECT id, SIMILARITY(description, ?) as score
        FROM ethical_dilemmas
        WHERE SIMILARITY(description, ?) >= ?
        ORDER BY score DESC
        LIMIT ?
    )";
    
    auto result = aql_engine_->execute(aql, {
        query_text, query_text, threshold, limit
    });
    
    // Extract IDs from result
    std::vector<std::string> dilemma_ids;
    for (const auto& row : result) {
        dilemma_ids.push_back(row["id"].as<std::string>());
    }
    
    return dilemma_ids;
}
```

#### Pattern 2: Philosophy-Specific Arguments

```cpp
// Already implemented using ArgumentStore::getArgumentsByPhilosophy()
// Example AQL if needed:
SELECT * FROM ethical_arguments
WHERE philosophy_school = ?
AND argument_type IN (?, ?)
ORDER BY created_at DESC
LIMIT ?
```

#### Pattern 3: Best Practices

```cpp
std::variant<std::vector<std::string>, Status> 
RAGContextEngine::getBestPractices(
    const std::string& category,
    double min_satisfaction,
    size_t limit) {
    
    std::string aql = R"(
        SELECT id, satisfaction_score
        FROM ethical_decisions
        WHERE category = ?
        AND satisfaction_score >= ?
        ORDER BY satisfaction_score DESC
        LIMIT ?
    )";
    
    auto result = aql_engine_->execute(aql, {
        category, min_satisfaction, limit
    });
    
    // Extract decision IDs
    // ...
}
```

#### Pattern 4: Vector Semantic Search

```cpp
std::variant<std::vector<std::pair<std::string, double>>, Status> 
RAGContextEngine::vectorSemanticSearch(
    const std::vector<float>& query_embedding,
    const std::string& philosophy_school,
    size_t limit) {
    
    // Use ThemisDB's vector search
    auto results = vector_mgr_->search(
        "ethical_arguments",
        query_embedding,
        limit,
        [&](const auto& metadata) {
            // Filter by philosophy if specified
            if (!philosophy_school.empty()) {
                return metadata["philosophy_school"] == philosophy_school;
            }
            return true;
        }
    );
    
    std::vector<std::pair<std::string, double>> arg_ids;
    for (const auto& result : results) {
        arg_ids.push_back({result.id, result.similarity});
    }
    
    return arg_ids;
}
```

#### Pattern 5: Argument Chain Traversal

```cpp
std::variant<std::vector<std::string>, Status> 
RAGContextEngine::traverseArgumentChain(
    const std::string& start_argument_id,
    size_t max_depth,
    const std::string& direction) {
    
    std::string edge_type;
    if (direction == "supports") {
        edge_type = "supports";
    } else if (direction == "counters") {
        edge_type = "counters";
    } else {
        edge_type = "*";  // both directions
    }
    
    // Use graph traversal
    auto results = graph_mgr_->traverse(
        start_argument_id,
        edge_type,
        max_depth,
        GraphTraversalMode::BFS
    );
    
    std::vector<std::string> arg_ids;
    for (const auto& node : results) {
        arg_ids.push_back(node.id);
    }
    
    return arg_ids;
}
```

#### Pattern 6: Temporal Filtering

```cpp
// Get recent debates (last 30 days)
SELECT * FROM ethical_debates
WHERE created_at >= NOW() - INTERVAL 30 DAYS
ORDER BY created_at DESC
LIMIT ?
```

#### Pattern 7: Multi-Philosophy Consensus

```cpp
// Find decisions with multi-philosophy support
SELECT decision_id, COUNT(DISTINCT philosophy_school) as philosophy_count
FROM decision_supporting_philosophies
GROUP BY decision_id
HAVING philosophy_count >= 3
ORDER BY philosophy_count DESC
LIMIT ?
```

### Task 3: Enhanced Argument Generation

**Priority:** MEDIUM  
**Complexity:** HIGH  
**Files:** `discourse_engine.h/cpp`

Current implementation generates placeholder arguments. Enhance to:

1. **Use Philosophy Profiles Intelligently:**
   - Parse main_theses and apply to dilemma
   - Use decision_framework rules
   - Consider strengths/weaknesses

2. **Generate Multiple Argument Types:**
   - PRO arguments based on principles
   - CONTRA arguments from weaknesses
   - REBUTTALS to counter-arguments
   - SYNTHESIS combining viewpoints

3. **Argument Quality Assessment:**
   - Score arguments based on principle adherence
   - Assess logical consistency
   - Rate strength appropriately

**Example Enhanced Generation:**

```cpp
EthicalArgument EthicalDiscourseEngine::generateArgument(
    const PhilosophyProfile& profile,
    const std::string& dilemma,
    ArgumentType type) {
    
    EthicalArgument argument;
    argument.id = generateUUID();
    argument.philosophy_school = profile.school_id;
    argument.argument_type = type;
    argument.created_at = std::chrono::system_clock::now();
    
    // Extract key concepts from dilemma
    auto concepts = extractConcepts(dilemma);
    
    // Find relevant theses
    std::vector<std::string> relevant_theses;
    for (const auto& thesis : profile.main_theses) {
        if (hasOverlap(concepts, extractConcepts(thesis))) {
            relevant_theses.push_back(thesis);
        }
    }
    
    // Generate content based on type
    std::stringstream content;
    if (type == ArgumentType::PRO) {
        content << "From " << profile.name << " perspective:\n\n";
        for (const auto& thesis : relevant_theses) {
            content << "Applying the principle: " << thesis << "\n";
        }
        content << "\nThis principle suggests that...";
        // Use decision_framework to generate specific recommendation
    }
    
    argument.content = content.str();
    argument.principle_basis = relevant_theses;
    argument.strength = assessArgumentStrength(argument, profile);
    
    return argument;
}
```

### Task 4: Testing Suite

**Priority:** HIGH  
**Complexity:** MEDIUM  
**Files:** `tests/test_ethics_ai_*.cpp`

Create comprehensive tests:

1. **Unit Tests:**
   - test_ethics_ai_types.cpp ✅ (exists)
   - test_philosophy_loader.cpp
   - test_argument_store.cpp
   - test_rag_context_engine.cpp
   - test_discourse_engine.cpp
   - test_ethics_evaluator.cpp

2. **Integration Tests:**
   - test_ethics_ai_plugin_integration.cpp
   - test_storage_integration.cpp
   - test_rag_patterns.cpp

3. **Performance Tests:**
   - bench_argument_storage.cpp
   - bench_rag_retrieval.cpp
   - bench_decision_making.cpp

**Example Test Structure:**

```cpp
#include <gtest/gtest.h>
#include "plugins/ethics_ai/philosophy_loader.h"

class PhilosophyLoaderTest : public ::testing::Test {
protected:
    void SetUp() override {
        loader_ = std::make_unique<PhilosophyLoader>();
        // Create temp YAML file for testing
    }
    
    void TearDown() override {
        // Clean up temp files
    }
    
    std::unique_ptr<PhilosophyLoader> loader_;
};

TEST_F(PhilosophyLoaderTest, LoadFromDirectory) {
    auto result = loader_->loadFromDirectory("test_data/philosophies");
    ASSERT_TRUE(std::holds_alternative<size_t>(result));
    EXPECT_GT(std::get<size_t>(result), 0);
}

TEST_F(PhilosophyLoaderTest, GetProfile) {
    loader_->loadFromDirectory("test_data/philosophies");
    auto result = loader_->getProfile("kant");
    ASSERT_TRUE(std::holds_alternative<PhilosophyProfile>(result));
    
    auto profile = std::get<PhilosophyProfile>(result);
    EXPECT_EQ("kant", profile.school_id);
    EXPECT_FALSE(profile.name.empty());
}
```

### Task 5: Documentation Completion

**Priority:** MEDIUM  
**Complexity:** LOW  
**Files:** Various

1. **API Documentation:**
   - Generate Doxygen documentation
   - Add detailed examples for each method
   - Document error codes and exceptions

2. **Integration Guide:**
   - How to integrate with existing ThemisDB instance
   - Configuration options explained
   - Deployment best practices

3. **Performance Guide:**
   - Optimization tips
   - Caching strategies
   - Scaling considerations

## Future Enhancements

### Phase 2: Advanced Features

1. **Prompt Optimization Framework**
   - Iterative prompt improvement
   - Test-case-driven refinement
   - Version control for prompts

2. **LoRa Training Integration**
   - Dataset generation from decisions
   - Fine-tuning on successful outcomes
   - Philosophy-balanced training

3. **Advanced Evaluation**
   - More sophisticated metrics
   - Historical comparison
   - Trend analysis

4. **Real-time Monitoring**
   - WebSocket dashboard
   - Live metrics streaming
   - Alert system

### Phase 3: Production Features

1. **Distributed Operation**
   - Multi-node debate coordination
   - Consensus across instances
   - Federated learning

2. **API Gateway Integration**
   - REST endpoints
   - GraphQL schema
   - gRPC services

3. **Security & Compliance**
   - Audit logging
   - Decision traceability
   - Regulatory compliance tools

## Contributing

When contributing to the Ethics AI Plugin:

1. **Follow C++17 standards**
2. **Add tests for new functionality**
3. **Update documentation**
4. **No Python dependencies**
5. **Maintain thread safety**
6. **Use existing ThemisDB patterns**

## Getting Help

- Review the main README.md
- Check example code in examples/
- Consult ThemisDB plugin documentation
- Open issues on GitHub

## License

MIT License - See LICENSE file
