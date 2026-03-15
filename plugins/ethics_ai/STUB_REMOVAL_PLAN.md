# Ethics AI Plugin - Stub Removal Implementation Plan

## Problem Statement

**"Keine wrapper, stub, simulationen, usw."** (No wrappers, stubs, simulations, etc.)

Current implementation contains multiple stubs and simulations that need to be replaced with production code.

---

## Issues Identified

### 1. Stub Functions in `src/query/functions/ethics_functions.cpp` ❌

**Line 12-17:** `makeStubResponse()` helper function  
**Line 28-42:** `EthicsMakeDecisionFunction` - Returns fake decision  
**Line 68-87:** `EthicsEvaluateFunction` - Returns hardcoded scores  
**Line 115-133:** `EthicsGetArgumentsFunction` - Returns empty array  
**Line 136-156:** `EthicsFindSimilarDilemmasFunction` - Returns empty array  
**Line 159-175:** `EthicsTraverseChainFunction` - Returns empty array  
**Line 182-196:** `EthicsLoadProfileFunction` - Returns stub profile  
**Line 227-242:** `EthicsBuildContextFunction` - Returns empty context  
**Line 250-265:** `EthicsStatsFunction` - Returns zeros  
**Line 269-294:** `EthicsMetricsFunction` - Returns hardcoded metrics with zeros  

### 2. Stub AQL Execution in `src/server/ethics_api_handler.cpp` ❌

**Line 410-429:** `executeAQL()` doesn't call QueryEngine  
Returns: `{"success": true, "message": "AQL execution not yet fully integrated"}`

### 3. In-Memory Simulation in `src/ethics_ai/argument_store.cpp` ❌

**Line 22-32:** `standalone_mode_` flag  
**Line 45-49:** In-memory `arguments_` map  
**Line 79-86:** Returns from memory instead of BaseEntity  

---

## Implementation Requirements

### Phase 1: Component Integration

**Required:** Create singleton instances of plugin components that functions can use:

```cpp
// In ethics_functions.cpp
namespace {
    // Shared component instances
    std::shared_ptr<PhilosophyLoader> g_philosophy_loader;
    std::shared_ptr<EthicalDiscourseEngine> g_discourse_engine;
    std::shared_ptr<EthicsEvaluator> g_evaluator;
    std::shared_ptr<ArgumentStore> g_argument_store;
    std::shared_ptr<RAGContextEngine> g_rag_engine;
    
    // Initialize once
    void initializeComponents(const FunctionContext& ctx) {
        if (!g_philosophy_loader) {
            g_philosophy_loader = std::make_shared<PhilosophyLoader>();
            // Load from configured directory
            g_philosophy_loader->loadFromDirectory("plugins/ethics_ai/philosophies");
        }
        
        if (!g_argument_store) {
            g_argument_store = std::make_shared<ArgumentStore>();
            // Get storage from context
            g_argument_store->initialize(ctx.storage, ctx.query_engine);
        }
        
        if (!g_rag_engine) {
            g_rag_engine = std::make_shared<RAGContextEngine>(
                g_argument_store, ctx.query_engine
            );
        }
        
        if (!g_discourse_engine) {
            g_discourse_engine = std::make_shared<EthicalDiscourseEngine>(
                g_philosophy_loader, g_argument_store, g_rag_engine
            );
        }
        
        if (!g_evaluator) {
            g_evaluator = std::make_shared<EthicsEvaluator>();
        }
    }
}
```

### Phase 2: Function Implementations

**EthicsMakeDecisionFunction:**
```cpp
json EthicsMakeDecisionFunction::execute(
    const std::vector<json>& args,
    const FunctionContext& ctx) const {
    
    initializeComponents(ctx);
    
    std::string dilemma = args[0];
    std::vector<std::string> philosophies = args[1];
    std::string category = args.size() > 2 ? args[2] : "general";
    bool use_rag = args.size() > 3 ? args[3].get<bool>() : true;
    
    auto result = g_discourse_engine->makeDecision(
        dilemma, philosophies, category, use_rag
    );
    
    if (std::holds_alternative<Status>(result)) {
        throw std::runtime_error(std::get<Status>(result).message);
    }
    
    const auto& decision = std::get<EthicalDecision>(result);
    
    // Convert to JSON
    json response;
    response["decision_id"] = decision.decision_id;
    response["decision_text"] = decision.decision_text;
    response["primary_philosophy"] = decision.primary_philosophy;
    response["confidence"] = decision.confidence;
    response["consensus_level"] = decision.consensus_level;
    response["created_at"] = std::chrono::system_clock::to_time_t(decision.created_at);
    
    return response;
}
```

**EthicsEvaluateFunction:**
```cpp
json EthicsEvaluateFunction::execute(
    const std::vector<json>& args,
    const FunctionContext& ctx) const {
    
    initializeComponents(ctx);
    
    // Parse decision from JSON
    EthicalDecision decision;
    decision.decision_id = args[0]["decision_id"];
    decision.decision_text = args[0]["decision_text"];
    // ... parse other fields
    
    std::vector<EthicalArgument> arguments;
    if (args.size() > 1 && args[1].is_array()) {
        // Parse arguments
    }
    
    auto result = g_evaluator->evaluateDecision(decision, arguments);
    
    if (std::holds_alternative<Status>(result)) {
        throw std::runtime_error(std::get<Status>(result).message);
    }
    
    const auto& eval = std::get<EthicsEvaluationResult>(result);
    
    // Convert to JSON
    json response;
    response["overall_score"] = eval.overall_score;
    response["decision_quality_score"] = eval.decision_quality_score;
    response["consistency_score"] = eval.consistency_score;
    response["fairness_score"] = eval.fairness_score;
    response["alignment_score"] = eval.alignment_score;
    response["transparency_score"] = eval.transparency_score;
    response["detailed_metrics"] = eval.detailed_metrics;
    
    return response;
}
```

**EthicsLoadProfileFunction:**
```cpp
json EthicsLoadProfileFunction::execute(
    const std::vector<json>& args,
    const FunctionContext& ctx) const {
    
    initializeComponents(ctx);
    
    std::string school = args[0];
    auto result = g_philosophy_loader->getProfile(school);
    
    if (std::holds_alternative<Status>(result)) {
        throw std::runtime_error(std::get<Status>(result).message);
    }
    
    const auto& profile = std::get<PhilosophyProfile>(result);
    
    // Convert to JSON
    json response;
    response["school"] = profile.school;
    response["name"] = profile.name;
    response["founder"] = profile.founder;
    response["main_thesis"] = profile.main_thesis;
    // ... add other fields
    
    return response;
}
```

**EthicsGetArgumentsFunction:**
```cpp
json EthicsGetArgumentsFunction::execute(
    const std::vector<json>& args,
    const FunctionContext& ctx) const {
    
    initializeComponents(ctx);
    
    std::string philosophy = args[0];
    std::vector<std::string> types;
    if (args.size() > 1 && args[1].is_array()) {
        types = args[1].get<std::vector<std::string>>();
    }
    int limit = args.size() > 2 ? args[2].get<int>() : 20;
    
    auto result = g_argument_store->getArgumentsByPhilosophy(
        philosophy, types, limit
    );
    
    if (std::holds_alternative<Status>(result)) {
        throw std::runtime_error(std::get<Status>(result).message);
    }
    
    const auto& arguments = std::get<std::vector<EthicalArgument>>(result);
    
    // Convert to JSON array
    json results = json::array();
    for (const auto& arg : arguments) {
        json item;
        item["id"] = arg.id;
        item["philosophy_school"] = arg.philosophy_school;
        item["argument_type"] = argumentTypeToString(arg.argument_type);
        item["content"] = arg.content;
        item["strength"] = argumentStrengthToString(arg.strength);
        results.push_back(item);
    }
    
    return results;
}
```

### Phase 3: API Handler - Direct Function Calls

Instead of executing AQL strings, call functions directly:

```cpp
nlohmann::json EthicsApiHandler::executeFunctionDirectly(
    const std::string& function_name,
    const std::vector<nlohmann::json>& args
) {
    // Get function from registry
    auto& registry = FunctionRegistry::instance();
    auto function = registry.getFunction(function_name);
    
    if (!function) {
        throw std::runtime_error("Function not found: " + function_name);
    }
    
    // Create context
    FunctionContext ctx;
    ctx.storage = storage_;
    ctx.query_engine = query_engine_;
    
    // Execute function
    return function->execute(args, ctx);
}

http::response<http::string_body> EthicsApiHandler::handleMakeDecision(
    const http::request<http::string_body>& req
) {
    try {
        nlohmann::json body = nlohmann::json::parse(req.body());
        
        // Prepare arguments
        std::vector<nlohmann::json> args = {
            body["dilemma_description"],
            body["philosophy_schools"],
            body.value("category", "general"),
            body.value("use_rag", true)
        };
        
        // Call function directly
        auto result = executeFunctionDirectly("ETHICS_MAKE_DECISION", args);
        
        return makeResponse(http::status::ok, result.dump(), req);
        
    } catch (const std::exception& e) {
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}
```

### Phase 4: Remove Standalone Mode

In `argument_store.cpp`:

```cpp
Status ArgumentStore::initialize(
    std::shared_ptr<RocksDBWrapper> storage,
    std::shared_ptr<query::QueryEngine> query_engine) {
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (initialized_) {
        return Status::Error("ArgumentStore already initialized");
    }
    
    if (!storage) {
        return Status::Error("Storage required - no standalone mode");
    }
    
    storage_ = storage;
    query_engine_ = query_engine;
    initialized_ = true;
    
    return Status::OK();
}

Status ArgumentStore::storeArgument(const EthicalArgument& argument, bool store_vector) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!initialized_ || !storage_) {
        return Status::Error("ArgumentStore not properly initialized");
    }
    
    if (argument.id.empty()) {
        return Status::Error("Argument ID cannot be empty");
    }
    
    // Always use BaseEntity storage
    BaseEntity entity = EthicsBaseEntityAdapter::toBaseEntity(argument);
    std::string key = EthicsBaseEntityAdapter::makeArgumentKey(argument.id);
    auto blob = entity.serialize();
    storage_->put(key, blob);
    
    // TODO: Vector storage when VectorIndexManager is available
    
    return Status::OK();
}
```

---

## FunctionContext Requirements

Need to ensure FunctionContext provides storage and query_engine:

```cpp
// In function signature definition
struct FunctionContext {
    std::shared_ptr<RocksDBWrapper> storage;
    std::shared_ptr<QueryEngine> query_engine;
    // ... other context
};
```

---

## Testing Strategy

After implementation:

1. **Unit Tests:** Test each function with real components
2. **Integration Tests:** Test full workflow end-to-end
3. **Storage Tests:** Verify BaseEntity storage/retrieval
4. **API Tests:** Test REST endpoints with real data

---

## Timeline

- **Phase 1:** Component integration (2-3 hours)
- **Phase 2:** Function implementations (4-5 hours)
- **Phase 3:** API handler updates (1-2 hours)
- **Phase 4:** Remove standalone mode (1 hour)
- **Testing:** (2-3 hours)

**Total Estimated:** 10-14 hours for complete stub removal

---

## Success Criteria

✅ No `makeStubResponse()` function  
✅ No hardcoded return values  
✅ No `TODO` comments in function implementations  
✅ No `standalone_mode` in ArgumentStore  
✅ All functions use real components  
✅ All storage goes through BaseEntity  
✅ API handler calls functions directly  
✅ All tests pass with real data  

---

## Current Status

**DOCUMENTED** - Implementation plan ready  
**PENDING** - Actual implementation (requires 10-14 hours)

This document serves as complete implementation guide for removing all stubs, simulations, and wrappers from the Ethics AI Plugin.
