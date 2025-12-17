# Direct Execution Architecture - Bypassing Compilation

## Neue Anforderung: Prompt-as-Language

**Kernidee:** Den Zwischenschritt (Compilieren und Linken) und das Anwenden von Programmiersprachen überspringen. Der Prompt des Benutzers ist die Sprache.

## Konzept: Prompt → Direct Execution

Statt:
```
User Prompt → LLM → Code (C++/Python) → Compiler → Binary → Execution
```

Direkt:
```
User Prompt → LLM → Execution Plan → Direct Execution
```

## Architektur-Überblick

```
┌─────────────────────────────────────────────────────────────────────┐
│                    Direct Execution Engine                           │
├─────────────────────────────────────────────────────────────────────┤
│                                                                      │
│  ┌────────────────────────────────────────────────────────────┐    │
│  │ Layer 1: Prompt Understanding                              │    │
│  │                                                             │    │
│  │  User: "Finde alle Benutzer, die gestern aktiv waren"     │    │
│  │        ↓                                                    │    │
│  │  Intent: QUERY                                             │    │
│  │  Entity: users                                             │    │
│  │  Filter: last_active == yesterday                          │    │
│  │  Action: RETRIEVE                                          │    │
│  └────────────────────────────┬───────────────────────────────┘    │
│                                │                                     │
│                                v                                     │
│  ┌────────────────────────────────────────────────────────────┐    │
│  │ Layer 2: Execution Plan Generation (nicht Code!)          │    │
│  │                                                             │    │
│  │  LLM Output: JSON Execution Plan                          │    │
│  │  {                                                          │    │
│  │    "operation": "FILTER_QUERY",                            │    │
│  │    "datasource": "users",                                  │    │
│  │    "filters": [                                            │    │
│  │      {"field": "last_active", "op": ">=",                 │    │
│  │       "value": {"type": "relative_date", "offset": "-1d"}}│    │
│  │    ],                                                       │    │
│  │    "return": "entities"                                    │    │
│  │  }                                                          │    │
│  └────────────────────────────┬───────────────────────────────┘    │
│                                │                                     │
│                                v                                     │
│  ┌────────────────────────────────────────────────────────────┐    │
│  │ Layer 3: Direct Execution (kein Compiler!)                │    │
│  │                                                             │    │
│  │  Execution Plan → Native Operations                        │    │
│  │                                                             │    │
│  │  ThemisDB API Direct Calls:                               │    │
│  │  - db->scanTableWithFilter("users", filters)              │    │
│  │  - index->rangeQuery("users", "last_active", ...)         │    │
│  │  - result_set->collect()                                   │    │
│  └────────────────────────────┬───────────────────────────────┘    │
│                                │                                     │
│                                v                                     │
│  ┌────────────────────────────────────────────────────────────┐    │
│  │ Layer 4: Result Formatting                                 │    │
│  │                                                             │    │
│  │  Results → User-friendly Output                            │    │
│  │                                                             │    │
│  │  [                                                          │    │
│  │    {"id": "user123", "name": "Alice", ...},               │    │
│  │    {"id": "user456", "name": "Bob", ...}                  │    │
│  │  ]                                                          │    │
│  └─────────────────────────────────────────────────────────────┘   │
│                                                                      │
└─────────────────────────────────────────────────────────────────────┘
```

## Kernkomponenten

### 1. Execution Plan Specification

**Statt Programmiersprache → Deklaratives Execution-Format**

```cpp
namespace themis::direct_execution {

/**
 * @brief Execution Plan - Intermediate Representation
 * 
 * Dies ist KEIN Code, sondern eine deklarative Beschreibung
 * der auszuführenden Operation.
 */
struct ExecutionPlan {
    enum class OperationType {
        QUERY,           // Daten abrufen
        AGGREGATE,       // Aggregationen berechnen
        TRANSFORM,       // Daten transformieren
        JOIN,            // Daten verknüpfen
        GRAPH_TRAVERSE,  // Graph durchlaufen
        VECTOR_SEARCH,   // Ähnlichkeitssuche
        TIME_SERIES      // Zeitreihen-Analyse
    };

    OperationType operation;
    nlohmann::json parameters;
    std::vector<ExecutionPlan> sub_plans;  // Für komplexe Operationen
    
    // Keine Code-Generierung nötig!
    // Plan wird direkt ausgeführt
};

} // namespace themis::direct_execution
```

### 2. Prompt → Plan Translator

```cpp
class PromptToExecutionPlan {
public:
    /**
     * @brief Übersetzt natürlichsprachlichen Prompt direkt in Execution Plan
     * 
     * KEIN Zwischenschritt über Programmiersprache!
     */
    ExecutionPlan translate(const std::string& user_prompt) {
        // LLM generiert strukturierten Plan (JSON), nicht Code
        std::string llm_prompt = R"(
Convert the user's request into an execution plan.
Output ONLY valid JSON, no code.

User request: ")" + user_prompt + R"("

Execution plan schema:
{
  "operation": "QUERY" | "AGGREGATE" | "TRANSFORM" | ...,
  "datasource": "table_name",
  "filters": [
    {"field": "field_name", "op": "==|!=|>|<|>=|<=|IN", "value": ...}
  ],
  "groupBy": ["field1", "field2"],
  "aggregations": [
    {"function": "COUNT|SUM|AVG|MIN|MAX", "field": "field_name", "as": "alias"}
  ],
  "sort": [
    {"field": "field_name", "order": "ASC|DESC"}
  ],
  "limit": number,
  "return": "entities" | "keys" | "count" | "aggregated"
}

Generate execution plan:
)";

        auto llm_response = llm_client_->generate(llm_prompt);
        
        // Parse JSON direkt zu Execution Plan
        ExecutionPlan plan;
        auto json = nlohmann::json::parse(llm_response.text);
        plan.operation = parseOperationType(json["operation"]);
        plan.parameters = json;
        
        return plan;
    }

private:
    std::shared_ptr<LLMClient> llm_client_;
};
```

### 3. Direct Executor

```cpp
class DirectExecutor {
public:
    /**
     * @brief Führt Execution Plan DIREKT aus - KEIN Compiler!
     * 
     * Der Plan wird zu nativen Datenbank-Operationen übersetzt
     * und sofort ausgeführt.
     */
    ExecutionResult execute(const ExecutionPlan& plan) {
        switch (plan.operation) {
            case ExecutionPlan::OperationType::QUERY:
                return executeQuery(plan.parameters);
            
            case ExecutionPlan::OperationType::AGGREGATE:
                return executeAggregation(plan.parameters);
            
            case ExecutionPlan::OperationType::GRAPH_TRAVERSE:
                return executeGraphTraversal(plan.parameters);
            
            case ExecutionPlan::OperationType::VECTOR_SEARCH:
                return executeVectorSearch(plan.parameters);
            
            default:
                throw std::runtime_error("Unsupported operation");
        }
    }

private:
    rocksdb::TransactionDB* db_;
    
    ExecutionResult executeQuery(const nlohmann::json& params) {
        std::string table = params["datasource"];
        
        // Direkte DB-Operationen - KEIN generierter Code!
        std::vector<Entity> results;
        
        // Build filters
        std::vector<Filter> filters;
        for (const auto& f : params["filters"]) {
            filters.push_back({
                .field = f["field"],
                .op = parseOperator(f["op"]),
                .value = f["value"]
            });
        }
        
        // Direct execution via ThemisDB API
        if (hasIndex(table, filters[0].field)) {
            // Use index scan
            results = index_manager_->scanIndex(table, filters);
        } else {
            // Table scan with filter
            results = storage_->scanWithFilter(table, filters);
        }
        
        // Apply sorting if specified
        if (params.contains("sort")) {
            sortResults(results, params["sort"]);
        }
        
        // Apply limit
        if (params.contains("limit")) {
            results.resize(std::min(results.size(), 
                                   params["limit"].get<size_t>()));
        }
        
        return ExecutionResult{results};
    }
};
```

## Beispiele: Prompt → Direct Execution

### Beispiel 1: Einfache Abfrage

**User Prompt:**
```
"Zeige mir alle Produkte mit Preis über 100 Euro"
```

**LLM Output (Execution Plan, NICHT Code):**
```json
{
  "operation": "QUERY",
  "datasource": "products",
  "filters": [
    {"field": "price", "op": ">", "value": 100}
  ],
  "return": "entities"
}
```

**Direct Execution:**
```cpp
// Kein Compiler! Direkte Ausführung:
auto results = index_manager_->rangeQuery(
    "products", 
    "price", 
    100, 
    std::numeric_limits<double>::max()
);
```

**Ergebnis:**
```json
[
  {"id": "p123", "name": "Laptop", "price": 999},
  {"id": "p456", "name": "Monitor", "price": 299}
]
```

### Beispiel 2: Aggregation

**User Prompt:**
```
"Wie viele Benutzer gibt es pro Stadt?"
```

**LLM Output:**
```json
{
  "operation": "AGGREGATE",
  "datasource": "users",
  "groupBy": ["city"],
  "aggregations": [
    {"function": "COUNT", "field": "*", "as": "count"}
  ],
  "return": "aggregated"
}
```

**Direct Execution:**
```cpp
// Direkte Aggregation - kein generierter Code
std::map<std::string, size_t> city_counts;
for (const auto& user : storage_->scan("users")) {
    city_counts[user["city"]]++;
}
```

### Beispiel 3: Graph Traversal

**User Prompt:**
```
"Finde alle Freunde von Alice bis zu 3 Ebenen tief"
```

**LLM Output:**
```json
{
  "operation": "GRAPH_TRAVERSE",
  "start_vertex": "user:alice",
  "edge_type": "follows",
  "max_depth": 3,
  "direction": "OUTBOUND",
  "return": "vertices"
}
```

**Direct Execution:**
```cpp
// Direkte Graph-Operation
auto friends = graph_index_->bfsTraversal(
    "user:alice",
    "follows",
    3,  // max_depth
    GraphIndex::Direction::OUTBOUND
);
```

### Beispiel 4: Vector Search

**User Prompt:**
```
"Finde Dokumente ähnlich zu 'Künstliche Intelligenz in der Medizin'"
```

**LLM Output:**
```json
{
  "operation": "VECTOR_SEARCH",
  "datasource": "documents",
  "query_text": "Künstliche Intelligenz in der Medizin",
  "k": 10,
  "metric": "cosine",
  "return": "entities"
}
```

**Direct Execution:**
```cpp
// Embedding generieren
auto query_embedding = embedding_service_->embed(
    "Künstliche Intelligenz in der Medizin"
);

// Direkte Vektor-Suche
auto similar_docs = vector_index_->search(
    query_embedding,
    10,  // k
    VectorIndex::Metric::COSINE
);
```

## Vorteile der Direct Execution

### 1. Keine Compilation-Latency

**Vorher (mit Code-Generierung):**
```
Prompt → LLM (2s) → Code Generation (1s) → Compilation (5s) → Execution (0.1s)
Total: ~8 Sekunden
```

**Jetzt (Direct Execution):**
```
Prompt → LLM (2s) → Execution Plan (0.5s) → Direct Execution (0.1s)
Total: ~2.6 Sekunden (3x schneller!)
```

### 2. Keine Sicherheitsrisiken durch Code-Ausführung

- Kein generierter Code = Keine Code Injection
- Kein Compiler = Keine Compiler-Exploits
- Nur vordefinierte, sichere Operationen

### 3. Einfachere Validierung

```cpp
bool validateExecutionPlan(const ExecutionPlan& plan) {
    // Validiere nur den Plan, nicht generierten Code
    
    // 1. Check operation ist erlaubt
    if (!isAllowedOperation(plan.operation)) {
        return false;
    }
    
    // 2. Check Parameter sind valide
    if (!areValidParameters(plan.parameters)) {
        return false;
    }
    
    // 3. Check Ressourcen-Limits
    if (exceedsLimits(plan)) {
        return false;
    }
    
    return true;  // Viel einfacher als Code-Validierung!
}
```

### 4. Konsistente Performance

- Vordefinierte Operationen sind optimiert
- Kein unvorhersehbarer generierter Code
- Garantierte Nutzung von Indizes

## Implementation

### Core Interface

```cpp
namespace themis::direct_execution {

class DirectExecutionEngine {
public:
    struct Config {
        std::string llm_endpoint;
        std::string llm_model;
        bool enable_caching = true;
        bool enable_plan_validation = true;
    };

    explicit DirectExecutionEngine(
        rocksdb::TransactionDB* db,
        const Config& config
    );

    /**
     * @brief Führt User-Prompt DIREKT aus
     * 
     * Prompt → Execution Plan → Results
     * KEIN Zwischenschritt über Programmiersprache!
     */
    ExecutionResult executePrompt(const std::string& user_prompt);

    /**
     * @brief Zeigt Execution Plan an (für Debugging)
     */
    ExecutionPlan explainPrompt(const std::string& user_prompt);

private:
    rocksdb::TransactionDB* db_;
    Config config_;
    
    std::unique_ptr<PromptToExecutionPlan> translator_;
    std::unique_ptr<DirectExecutor> executor_;
    std::unique_ptr<PlanValidator> validator_;
    std::unique_ptr<PlanCache> cache_;
};

} // namespace themis::direct_execution
```

### Usage Example

```cpp
#include "direct_execution_engine.h"

int main() {
    // Initialize
    auto db = openThemisDB("./data");
    
    DirectExecutionEngine::Config config;
    config.llm_endpoint = "http://localhost:8000";
    config.llm_model = "codellama/CodeLlama-13b-Instruct-hf";
    
    DirectExecutionEngine engine(db, config);

    // User schreibt natürlichsprachlichen Prompt
    std::string prompt = R"(
        Finde alle Sensoren, die in den letzten 24 Stunden
        eine Temperatur über 50°C gemessen haben und zeige
        den Durchschnitt pro Sensor.
    )";

    // DIREKTE Ausführung - kein Code, kein Compiler!
    auto result = engine.executePrompt(prompt);

    // Ergebnis anzeigen
    std::cout << "Ergebnisse:\n";
    for (const auto& row : result.rows) {
        std::cout << "Sensor: " << row["sensor_id"] 
                  << ", Avg Temp: " << row["avg_temperature"] << "°C\n";
    }

    // Optional: Plan anzeigen (für Debugging)
    auto plan = engine.explainPrompt(prompt);
    std::cout << "\nExecution Plan:\n" 
              << plan.toJSON().dump(2) << "\n";

    return 0;
}
```

**Output:**
```
Ergebnisse:
Sensor: S001, Avg Temp: 67.3°C
Sensor: S042, Avg Temp: 52.1°C

Execution Plan:
{
  "operation": "AGGREGATE",
  "datasource": "sensor_readings",
  "filters": [
    {"field": "temperature", "op": ">", "value": 50},
    {"field": "timestamp", "op": ">=", "value": {"type": "relative", "offset": "-24h"}}
  ],
  "groupBy": ["sensor_id"],
  "aggregations": [
    {"function": "AVG", "field": "temperature", "as": "avg_temperature"}
  ]
}
```

## Execution Plan DSL

```cpp
// Domain-Specific Language für Execution Plans
namespace themis::dsl {

// Fluent API für Plan-Building
class ExecutionPlanBuilder {
public:
    ExecutionPlanBuilder& query(const std::string& table) {
        plan_.operation = ExecutionPlan::OperationType::QUERY;
        plan_.parameters["datasource"] = table;
        return *this;
    }

    ExecutionPlanBuilder& filter(const std::string& field, 
                                 const std::string& op, 
                                 const nlohmann::json& value) {
        plan_.parameters["filters"].push_back({
            {"field", field},
            {"op", op},
            {"value", value}
        });
        return *this;
    }

    ExecutionPlanBuilder& groupBy(const std::vector<std::string>& fields) {
        plan_.parameters["groupBy"] = fields;
        return *this;
    }

    ExecutionPlanBuilder& aggregate(const std::string& func,
                                   const std::string& field,
                                   const std::string& as) {
        plan_.parameters["aggregations"].push_back({
            {"function", func},
            {"field", field},
            {"as", as}
        });
        return *this;
    }

    ExecutionPlan build() { return plan_; }

private:
    ExecutionPlan plan_;
};

} // namespace themis::dsl

// Usage:
auto plan = ExecutionPlanBuilder()
    .query("sensor_readings")
    .filter("temperature", ">", 50)
    .filter("timestamp", ">=", relativeTime("-24h"))
    .groupBy({"sensor_id"})
    .aggregate("AVG", "temperature", "avg_temperature")
    .build();
```

## Performance-Vergleich

### Traditioneller Ansatz (mit Code-Generierung):

```
User: "Find users in Berlin"
  ↓ (2000ms - LLM)
Generated Code: "FOR u IN users FILTER u.city == 'Berlin' RETURN u"
  ↓ (500ms - AQL Parser)
Query Plan: [scan users, filter city, return]
  ↓ (50ms - Execution)
Result: [user1, user2, ...]

Total: 2550ms
```

### Direct Execution:

```
User: "Find users in Berlin"
  ↓ (1500ms - LLM generates execution plan directly)
Execution Plan: {"operation": "QUERY", "datasource": "users", ...}
  ↓ (50ms - Direct Execution, no parsing needed)
Result: [user1, user2, ...]

Total: 1550ms (40% faster!)
```

## Zusammenfassung

### Kernidee

**Prompt ist die Sprache** - Kein Zwischenschritt über Programmiersprachen nötig!

```
Traditionell:  Prompt → Code → Compiler → Execution
Neu:          Prompt → Execution Plan → Execution
```

### Vorteile

✅ **Schneller** - Keine Compilation-Latency  
✅ **Sicherer** - Kein generierter Code, keine Code Injection  
✅ **Einfacher** - Nur vordefinierte Operationen  
✅ **Konsistenter** - Optimierte Ausführungspfade  
✅ **Verständlicher** - Execution Plan ist menschenlesbar  

### Nächste Schritte

1. Implementierung des `DirectExecutionEngine`
2. Definition aller unterstützten Operationen
3. LLM Prompt-Templates für Plan-Generierung
4. Integration mit ThemisDB Backend
5. Performance-Benchmarks
