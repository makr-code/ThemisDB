> **Aktueller Build-Flow:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# C++ Ethics Integration Guide: Multi-Model Storage & Audit Trail

> **Historischer Stand:** 2026-01-31 — Inhalte nicht gegen aktuelle Quellen geprüft.

## Overview

The C++ ethical AI implementation provides a **thin orchestration layer** (~1,200 lines) that coordinates existing ThemisDB components. It stores ethical decisions across **multiple storage models** (Graph, Vector, Relational) with **comprehensive audit trail logging** for compliance and transparency.

**Core Design Principle**: **Reuse ThemisDB infrastructure, minimal new code**

## Multi-Model Storage Architecture

### Decision Storage Across Four Models

Each ethical decision is stored across ThemisDB's multi-model architecture:

#### 1. Graph Storage (PropertyGraphManager)
**Purpose**: Full reasoning chains, transparent decision structure

**Stored Data**:
- Decision nodes with all metadata
- Scenario nodes with context
- Stakeholder nodes with impact assessments
- Principle nodes from philosophical frameworks
- Action and outcome nodes
- Edges representing relationships (based_on, involves, applies_to, leads_to)

**Example**:
```cpp
// Graph stores complete reasoning structure
BaseEntity decision_entity(decision.decision_id);
decision_entity.setField("type", "decision");
decision_entity.setField("recommended_action", decision.recommended_action);
decision_entity.setField("confidence", decision.confidence);
decision_entity.setField("principle_citations", decision.principle_citations);
decision_entity.setField("alternative_perspectives", alt_perspectives_json);
decision_entity.setField("metric_consistency", decision.metrics.consistency);
decision_entity.setField("metric_fairness", decision.metrics.fairness);
// ... all metrics and reasoning data

graph_manager_->addNode(decision_entity, decision.graph_id);
```

#### 2. Vector Storage (VectorIndexManager)
**Purpose**: Similarity search for precedent cases

**Stored Data**:
- Scenario embeddings (high-dimensional vectors)
- Decision metadata for retrieval
- Domain tags for filtering

**Example**:
```cpp
// Vector stores embeddings for similarity search
BaseEntity vector_entity(decision.scenario_id + "_embedding");
vector_entity.setField("embedding", scenario_embedding);  // Vector<float>
vector_entity.setField("scenario_id", decision.scenario_id);
vector_entity.setField("decision_id", decision.decision_id);
vector_entity.setField("philosophy", decision.philosophy);

vector_index_->addEntity(vector_entity, "embedding");

// Later: Find similar cases
auto similar = vector_index_->searchKnn("ethics_scenarios", query_embedding, k=5);
```

#### 3. Relational Storage (RocksDB Key-Value)
**Purpose**: Structured queries, keywords, metadata

**Stored Data**:
- Keywords extracted from principles and reasoning
- Timestamps for temporal queries
- Aggregate metrics (avg score, principle count)
- Indexed metadata for fast lookup

**Example**:
```cpp
// Relational stores structured metadata and keywords
BaseEntity metadata_entity("decision_metadata_" + decision.decision_id);
metadata_entity.setField("decision_id", decision.decision_id);
metadata_entity.setField("philosophy", decision.philosophy);
metadata_entity.setField("confidence", decision.confidence);
metadata_entity.setField("keywords", keywords);  // ["kant", "duty", "autonomy"]
metadata_entity.setField("timestamp", timestamp);
metadata_entity.setField("metrics_avg", avg_score);

db_.put("ethics_metadata:" + decision.decision_id, metadata_entity.serialize());

// Later: Query by keywords or time range
```

#### 4. Audit Trail (AIDecisionAuditor)
**Purpose**: Compliance logging, explainability, regulatory requirements

**Stored Data**:
- Complete decision context
- Model parameters and version
- Confidence scores and alternatives
- Human-readable explanations
- Step-by-step reasoning
- Key factors influencing decision
- Human review flags for low-confidence cases
- Cryptographic signatures for integrity

**Example**:
```cpp
// Audit trail logs complete decision for compliance
AIDecisionAudit audit;
audit.decision_id = decision.decision_id;
audit.user_id = "ethics_system";
audit.timestamp = std::chrono::system_clock::now();
audit.query = "Ethical scenario: " + decision.scenario_id;
audit.response = decision.recommended_action;
audit.confidence_score = decision.confidence;
audit.explanation = decision.reasoning;
audit.reasoning_steps = decision.principle_citations;
audit.requires_human_review = (decision.confidence < 0.7);

decision_auditor_->logDecision(audit);
```

## Complete Usage Example

```cpp
#include "llm/moral_analyzer.h"
#include "index/vector_index.h"
#include "llm/ai_decision_auditor.h"

// 1. Initialize ThemisDB components
RocksDBWrapper db;
db.open("/path/to/themisdb");

// 2. Create storage managers
auto guidelines_mgr = std::make_shared<EthicalGuidelinesManager>();
guidelines_mgr->loadFromYAML("philosophies.yaml");

auto vector_index = std::make_shared<VectorIndexManager>(db);
vector_index->init("ethics_scenarios", 768);  // 768-dim embeddings

auto decision_auditor = std::make_shared<AIDecisionAuditor>(db);

// 3. Create analyzer with multi-model storage
MoralAnalyzer analyzer(db, guidelines_mgr, vector_index, decision_auditor);

// 4. Define ethical scenario
MoralAnalyzer::EthicalScenario scenario;
scenario.id = "autonomous_vehicle_001";
scenario.description = "Autonomous vehicle must choose between...";
scenario.domain = "autonomous_systems";
scenario.stakeholders = {{"passenger", 1}, {"pedestrian", 5}};
scenario.possible_actions = {"brake", "swerve_left", "continue"};

// 5. Analyze with multi-philosophy reasoning
auto [status, decision] = analyzer.analyzeMultiPhilosophy(
    scenario, {"kant", "utilitarian", "virtue"}
);

// 6. Generate or obtain embedding (in production: use LLM)
std::vector<float> embedding(768);
// ... generate embedding from scenario description using LLM

// 7. Store across all models with audit trail
auto store_status = analyzer.storeDecision(decision, embedding);

// Decision is now stored in:
// - Graph: Full reasoning chains
// - Vector: Embedding for similarity search
// - Relational: Keywords and metadata
// - Audit: Complete compliance log
```

## Query Examples

### Query by Graph Traversal
```cpp
// Find all decisions based on a specific scenario
auto decisions = graph_manager_->findNodesConnectedBy(
    "scenario_" + scenario_id, 
    "based_on", 
    Direction::INCOMING
);
```

### Query by Vector Similarity
```cpp
// Find similar past scenarios
auto similar_cases = vector_index_->searchKnn(
    "ethics_scenarios",
    current_scenario_embedding,
    5,  // k: number of neighbors
    VectorIndexManager::Metric::COSINE
);
```

### Query by Keywords (Relational)
```cpp
// Find decisions involving specific principles
auto keyword_results = db_.scan("ethics_metadata:", 
    [](const std::string& key, const std::string& value) {
        auto entity = BaseEntity::deserialize(value);
        auto keywords = entity.getField<std::vector<std::string>>("keywords");
        return std::find(keywords.begin(), keywords.end(), "autonomy") != keywords.end();
    }
);
```

### Query Audit Trail
```cpp
// Retrieve audit log for compliance review
auto audit_log = decision_auditor_->getDecisionAudit(decision_id);

// Export for regulatory submission
auto audit_json = audit_log.toJson();
```

## Benefits of Multi-Model Architecture

### 1. **Comprehensive Storage**
- Graph: Complete reasoning transparency
- Vector: Efficient precedent search
- Relational: Fast keyword/metadata queries
- Audit: Regulatory compliance

### 2. **Query Flexibility**
- Graph traversal for reasoning chains
- Vector similarity for case-based reasoning
- Relational lookup for specific attributes
- Audit trail for explainability

### 3. **Performance Optimization**
- Graph: O(N+E) for reasoning paths
- Vector: O(log N) for similarity search
- Relational: O(1) for key-value lookup
- Audit: Sequential append-only log

### 4. **Regulatory Compliance**
- EU AI Act: Audit trail with explainability
- GDPR Article 22: Transparent automated decisions
- eIDAS: Cryptographic signatures
- FDA (medical AI): Complete decision logs

## Integration Points

### Adding Vector Embeddings
```cpp
// In production: Connect to LLM for embeddings
auto embedding = llm_backend->embed(scenario.description);
analyzer.storeDecision(decision, embedding);
```

### Enabling Audit Signatures
```cpp
// Add cryptographic signing
decision_auditor->setSigningKey(pki_client);
// Audit logs now include signatures for tamper-evidence
```

### Historical Data Analysis
```cpp
// Use stored decisions for improvement
auto past_decisions = analyzer.findSimilarScenarios(scenario, 10);
// Analyze outcomes for better predictions
```

## ThemisDB Components Used

### Existing Infrastructure (Reused)
1. **PropertyGraphManager** - Graph storage and traversal
2. **VectorIndexManager** - HNSW-based similarity search
3. **RocksDBWrapper** - Key-value storage
4. **AIDecisionAuditor** - Compliance logging
5. **EthicalGuidelinesManager** - Philosophy profiles

### New Orchestration Layer
- **MoralAnalyzer** (~1,200 lines) - Coordinates above components for ethics workflows

## Summary

The MoralAnalyzer serves as a **thin orchestration layer** that:
1. **Coordinates** existing ThemisDB components
2. **Stores** decisions across multiple models (Graph + Vector + Relational)
3. **Logs** complete audit trail for compliance
4. **Provides** flexible querying across all storage types

**Minimal new code** (~1,200 lines) leverages **100K+ lines** of proven ThemisDB infrastructure for production-grade ethical AI with:
- ✅ Graph reasoning chains
- ✅ Vector similarity search
- ✅ Relational keyword queries
- ✅ Complete audit trail
- ✅ Regulatory compliance (EU AI Act, GDPR, eIDAS)
