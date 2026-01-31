# C++ Ethical AI Integration Guide for ThemisDB

## Architecture Overview

The C++ ethical AI implementation is **fully integrated with ThemisDB's existing infrastructure** and operates **completely independently** of the Python examples. Python serves only as a showcase/demonstration.

### Core Design Principles

1. **Leverage Existing ThemisDB Components**: Use existing managers, engines, and storage systems
2. **Minimal New Implementation**: Only add what's absolutely necessary for ethics-specific workflows
3. **Full Independence**: No Python dependencies - pure C++ with ThemisDB APIs
4. **Production-Ready**: Built on battle-tested ThemisDB infrastructure

## ThemisDB Components Used

### 1. **PropertyGraphManager** (Already Exists)
- **Purpose**: Stores ethical decision graphs with full reasoning chains
- **Location**: `include/index/property_graph.h`
- **Usage in Ethics**:
  - Scenario nodes with descriptions and context
  - Stakeholder nodes with risk levels
  - Principle nodes from ethical frameworks
  - Action nodes for possible choices
  - Outcome nodes for predicted consequences
  - Edges representing relationships (involves, applies_to, leads_to, etc.)

```cpp
PropertyGraphManager graph_mgr(db);

// Create scenario node
BaseEntity scenario("trolley_001");
scenario.setField("_labels", std::vector<std::string>{"Scenario"});
scenario.setField("description", "A runaway trolley...");
graph_mgr.addNode(scenario, "ethics_graph");

// Create action node
BaseEntity action("pull_lever");
action.setField("_labels", std::vector<std::string>{"Action"});
action.setField("description", "Pull the lever to divert trolley");
graph_mgr.addNode(action, "ethics_graph");

// Connect with edge
BaseEntity edge("scenario_action_1");
edge.setField("_from", "trolley_001");
edge.setField("_to", "pull_lever");
edge.setField("_type", "considers");
graph_mgr.addEdge(edge, "ethics_graph");
```

### 2. **EthicalGuidelinesManager** (Already Exists)
- **Purpose**: Manages ethical principles and philosophy profiles
- **Location**: `include/llm/ethical_guidelines_manager.h`
- **Usage in Ethics**:
  - Loads philosophy profiles from YAML (Kant, Utilitarian, Virtue, Care Ethics, Rawls)
  - Provides ethical principles per philosophy
  - Detects ethical context in scenarios
  - Augments reasoning with appropriate guidelines

```cpp
auto guidelines_mgr = std::make_shared<EthicalGuidelinesManager>();
guidelines_mgr->loadFromYAML("philosophies.yaml");

// Get principles for a philosophy
auto kant_principles = guidelines_mgr->getPrinciplesFor("kant");
// Result: ["categorical_imperative", "respect_for_persons", ...]

// Detect ethical context
auto detection = guidelines_mgr->detectEthicalContext(
    "Should we sacrifice one to save many?"
);
// Result: has_ethical_context=true, detected_keywords=["sacrifice", "save"]
```

### 3. **RocksDBWrapper** (Already Exists)
- **Purpose**: All persistence for ethical decisions
- **Location**: `include/storage/rocksdb_wrapper.h`
- **Usage in Ethics**:
  - Stores decision graphs via PropertyGraphManager
  - Persists metadata via relational operations
  - Enables timeline queries for outcome tracking

```cpp
RocksDBWrapper db;
db.open("/path/to/themisdb");

// All graph operations persist through RocksDB
// PropertyGraphManager, VectorIndexManager, etc. use this
```

### 4. **LlamaCppInferenceEngine** (Already Exists)
- **Purpose**: LLM integration for sophisticated reasoning
- **Location**: `include/llm/llamacpp_inference_engine.h`
- **Usage in Ethics**:
  - Generate ethical arguments
  - Predict outcomes based on context
  - Semantic analysis of scenarios
  - Cross-philosophy critique generation

```cpp
auto llm_engine = std::make_shared<LlamaCppInferenceEngine>();
llm_engine->loadModel("llama-3-8b-ethics.gguf");

// Generate ethical argument
std::string prompt = "From a Kantian perspective, analyze: " + scenario;
auto argument = llm_engine->generate(prompt);
```

### 5. **VectorIndexManager** (Already Exists)
- **Purpose**: Similarity search for precedent cases
- **Location**: `include/index/vector_index.h`
- **Usage in Ethics**:
  - Find similar past ethical scenarios
  - Retrieve relevant precedents
  - Enable case-based reasoning

```cpp
VectorIndexManager vector_mgr(db);

// Create embedding for scenario
std::vector<float> embedding = llm_engine->embed(scenario.description);

// Store scenario vector
vector_mgr.addVector("scenario:trolley_001", embedding);

// Find similar scenarios
auto similar = vector_mgr.searchKNN(embedding, 5);
// Result: ["scenario:trolley_002", "scenario:autonomous_vehicle_001", ...]
```

## New Implementation: MoralAnalyzer

**Only one new class** is added as a thin orchestration layer connecting existing components:

### Purpose
Coordinates ethical reasoning workflows using ThemisDB's existing infrastructure

### What It Does
1. **Builds Decision Graphs**: Uses PropertyGraphManager to create ethical scenario graphs
2. **Applies Philosophies**: Uses EthicalGuidelinesManager to load principles
3. **Generates Arguments**: Uses LlamaCppInferenceEngine for sophisticated reasoning
4. **Finds Precedents**: Uses VectorIndexManager for similarity search
5. **Stores Decisions**: Uses RocksDBWrapper via PropertyGraphManager

### What It Does NOT Do
- ❌ Implement its own storage (uses PropertyGraphManager)
- ❌ Parse YAML directly (uses EthicalGuidelinesManager)
- ❌ Generate LLM responses (uses LlamaCppInferenceEngine)
- ❌ Manage vectors (uses VectorIndexManager)
- ❌ Create new database abstractions (uses RocksDBWrapper)

### Example Usage

```cpp
#include "llm/moral_analyzer.h"
#include "llm/ethical_guidelines_manager.h"
#include "storage/rocksdb_wrapper.h"

// Initialize ThemisDB components
RocksDBWrapper db;
db.open("/path/to/themisdb");

auto guidelines_mgr = std::make_shared<EthicalGuidelinesManager>();
guidelines_mgr->loadFromYAML("philosophies.yaml");

// Create analyzer (thin orchestration layer)
MoralAnalyzer analyzer(db, guidelines_mgr);

// Define scenario
MoralAnalyzer::EthicalScenario scenario;
scenario.id = "trolley_001";
scenario.description = "A runaway trolley is heading towards five people...";
scenario.domain = "classic_dilemma";
scenario.stakeholders = {{"people_on_main_track", 5}, {"person_on_side_track", 1}};
scenario.possible_actions = {"pull_lever", "do_nothing"};
scenario.relevant_principles = {"minimize_harm", "do_not_kill"};

// Analyze with specific philosophy (uses EthicalGuidelinesManager)
auto [status, decision] = analyzer.analyzeWithPhilosophy(scenario, "kant");

// Multi-philosophy ensemble (uses all ThemisDB components)
auto [status2, ensemble] = analyzer.analyzeMultiPhilosophy(
    scenario,
    {"kant", "utilitarian", "virtue"}
);

// Decision is stored in PropertyGraph via PropertyGraphManager
// Can be queried later using graph operations
```

## Multi-Model Storage Architecture

Ethical decisions are stored across ThemisDB's multi-model architecture:

### 1. **Graph Storage** (PropertyGraph)
- **What**: Decision reasoning chains
- **How**: Scenario → Stakeholders → Actions → Outcomes → Principles
- **Query**: `graph_mgr.traversePath("trolley_001", "pull_lever")`

### 2. **Vector Storage** (VectorIndex)
- **What**: Scenario embeddings for similarity search
- **How**: LLM-generated embeddings of scenario descriptions
- **Query**: `vector_mgr.searchKNN(embedding, 5)` for similar cases

### 3. **Relational Storage** (RocksDB KV)
- **What**: Structured metadata, metrics, outcomes
- **How**: Key-value pairs for quick lookups
- **Query**: `db.get("decision:metadata:trolley_001")`

### 4. **Timeline Storage** (Timeseries)
- **What**: Outcome tracking over time, decision evolution
- **How**: Time-stamped decision events
- **Query**: `timeseries_mgr.queryRange("trolley_001", start_time, end_time)`

## Production Integration Path

### Step 1: LLM Integration
```cpp
// Connect MoralAnalyzer with actual LLM for argument generation
auto llm_engine = std::make_shared<LlamaCppInferenceEngine>();
llm_engine->loadModel("llama-3-8b-ethics.gguf");

// Pass to analyzer
MoralAnalyzer analyzer(db, guidelines_mgr, llm_engine);

// Now analyzer uses LLM for:
// - generateArguments()
// - predictOutcomes()
// - semantic analysis
```

### Step 2: Vector Search Integration
```cpp
// Enable similarity search for precedent retrieval
auto vector_mgr = std::make_shared<VectorIndexManager>(db);

// Pass to analyzer
MoralAnalyzer analyzer(db, guidelines_mgr, llm_engine, vector_mgr);

// Now retrieveSimilarScenarios() uses vector search
auto similar = analyzer.retrieveSimilarScenarios(scenario, 5);
```

### Step 3: Historical Data Integration
```cpp
// Connect to outcome database for prediction models
auto outcomes_db = std::make_shared<TimeseriesManager>(db);

// Analyzer uses historical data for:
// - predictOutcomes() - real data instead of heuristics
// - calculateExpectedUtility() - actual success rates
// - checkConsistency() - compare with past decisions
```

### Step 4: LoRa Training Integration
```cpp
// Use LoRa framework for philosophy-specific fine-tuning
auto lora_mgr = std::make_shared<LoRaTrainingService>();

// Train philosophy adapters
lora_mgr->trainAdapter("kant_ethics", training_data);
lora_mgr->trainAdapter("utilitarian_ethics", training_data);

// Analyzer uses adapters for:
// - evaluateDeontological() - Kant adapter
// - evaluateConsequentialist() - Utilitarian adapter
// - evaluateVirtueEthics() - Virtue adapter
```

## Comparison with Python Example

| Feature | Python (Showcase) | C++ (Production) |
|---------|------------------|------------------|
| **Purpose** | Demonstration | Production deployment |
| **Storage** | Mock/in-memory | ThemisDB (RocksDB, PropertyGraph) |
| **LLM** | OpenAI/Anthropic APIs | llama.cpp (local inference) |
| **Philosophy Profiles** | Hardcoded | EthicalGuidelinesManager (YAML) |
| **Vector Search** | Not implemented | VectorIndexManager |
| **Graph Operations** | Simulated | PropertyGraphManager (persistent) |
| **Performance** | N/A (demo) | Production-optimized |
| **Dependencies** | Python + external APIs | C++ + ThemisDB only |
| **Independence** | N/A | Fully standalone |

## Benefits of ThemisDB Integration

### 1. **Proven Infrastructure**
- Uses battle-tested PropertyGraphManager, RocksDBWrapper, etc.
- No reinventing the wheel

### 2. **Multi-Model Power**
- Graph for reasoning chains
- Vector for similarity search
- Relational for metadata
- Timeline for outcome tracking

### 3. **Production-Ready**
- Built on production-grade components
- Performance-optimized
- ACID guarantees via RocksDB

### 4. **Minimal Code Footprint**
- MoralAnalyzer: ~900 lines (orchestration only)
- No duplicate implementations
- Leverage existing 100K+ lines of ThemisDB code

### 5. **Full Independence**
- No Python dependencies
- No external API calls required
- Pure C++ with ThemisDB

## Documentation References

### ThemisDB Core Documentation
- **PropertyGraph**: See `include/index/property_graph.h` for graph operations
- **EthicalGuidelines**: See `include/llm/ethical_guidelines_manager.h` for philosophy management
- **LLM Integration**: See `include/llm/llamacpp_inference_engine.h` for inference
- **Vector Search**: See `include/index/vector_index.h` for similarity search
- **Storage**: See `include/storage/rocksdb_wrapper.h` for persistence

### Ethics-Specific Documentation
- **Literature Review**: `ETHICS_AI_LITERATURE_REVIEW.md` - Research foundation
- **LoRa Training**: `LORA_ETHICAL_ALIGNMENT_BEST_PRACTICES.md` - Training methodology
- **Best Practices**: `ETHICS_AI_BEST_PRACTICES.md` - Production deployment
- **API Reference**: `include/llm/moral_analyzer.h` - Complete API documentation

## Example Compilation

```bash
# With CMake (recommended)
mkdir -p build && cd build
cmake .. -DTHEMIS_BUILD_EXAMPLES=ON
make moral_analyzer_example
./moral_analyzer_example

# With g++ directly
g++ -std=c++17 -I./include \
    examples/moral_analyzer_example.cpp \
    src/llm/moral_analyzer.cpp \
    -lrocksdb -lyaml-cpp \
    -o moral_analyzer_example
./moral_analyzer_example
```

## Summary

The C++ ethical AI implementation is **NOT a port of the Python code**. Instead, it:

1. ✅ **Leverages existing ThemisDB infrastructure** (PropertyGraph, EthicalGuidelines, LLM, Vector, Storage)
2. ✅ **Adds minimal new code** (only MoralAnalyzer orchestration layer)
3. ✅ **Operates completely independently** (no Python dependencies)
4. ✅ **Production-ready from day one** (built on battle-tested components)
5. ✅ **Python is just a showcase** (C++ is the real implementation)

This design ensures maintainability, performance, and seamless integration with ThemisDB's existing ecosystem.
