# GPU Impact Analysis - Architektur-Abgrenzung

**Version:** 1.0.0  
**Datum:** 7. Dezember 2025  
**Thema:** Abgrenzung Kernaufgabe Datenbank vs. Verarbeitungsschicht

---

## 1. Kernfrage: Wo ist die Grenze?

**Frage:** Wo grenzen wir zur externen Programmatik ab (Kernaufgabe Datenbank gegen Verarbeitungsschicht → Analyse-Fähigkeiten)?

**Antwort:** Die GPU Impact Analysis ist als **Enterprise Plugin** implementiert, das klar von der Datenbank-Kernfunktionalität getrennt ist, aber über definierte Schnittstellen integriert wird.

---

## 2. Architektur-Ebenen

### 2.1 ThemisDB Core (Datenbank-Kern)

**Verantwortlichkeiten:**
- ✅ Datenhaltung (RocksDB LSM-Tree)
- ✅ Indizierung (Secondary, Graph, Vector, Spatial)
- ✅ Transaktionen (MVCC, Snapshot Isolation)
- ✅ Query Engine (AQL Parsing, Optimization, Execution)
- ✅ CRUD Operations (GET, PUT, DELETE, UPDATE)
- ✅ Basic Graph Operations (Traversal, Shortest Path)
- ✅ Basic Analytics (COUNT, SUM, AVG, etc.)

**Schnittstellen:**
- HTTP REST API (Port 8765)
- AQL Query Language
- Index APIs (GraphIndexManager, VectorIndexManager)
- Plugin Interface (`IThemisPlugin`)

**Implementierung:**
```
src/
├── storage/          # RocksDB wrapper
├── index/            # Graph, Vector, Secondary indexes
├── query/            # Query parser, optimizer, executor
├── server/           # HTTP server (Boost.Beast)
└── transaction/      # MVCC, locking
```

---

### 2.2 Analytics Layer (Datenbank-erweitert)

**Verantwortlichkeiten:**
- ✅ OLAP-Aggregationen (GROUP BY, HAVING)
- ✅ Process Mining (Event Logs, Process Discovery)
- ✅ Time Series Analysis (Windowing, Forecasting)
- ✅ Graph Analytics (Centrality, Community Detection)

**Implementierung:**
```
include/analytics/
├── olap.h              # OLAP operations
├── process_mining.h    # Process discovery
└── cep_engine.h        # Complex Event Processing
```

**Abgrenzung:**
- Diese Funktionen sind **noch Teil der Datenbank**, da sie:
  - Direkt auf Index-Strukturen zugreifen
  - In AQL integriert sind (`FOR event IN events WINDOW ...`)
  - Mit Datenbank-Transaktionen arbeiten
  - Keine externe Datenhaltung benötigen

---

### 2.3 Plugin Layer (Externe Verarbeitungsschicht)

**Verantwortlichkeiten:**
- ⚠️ Spezialisierte Analyse-Algorithmen
- ⚠️ GPU-beschleunigte Berechnungen
- ⚠️ Domain-spezifische Logik (Legal, Finance, etc.)
- ⚠️ Machine Learning Integration
- ⚠️ Externe Service-Integration

**Implementierung:**
```
plugins/
├── enterprise/
│   ├── gpu_impact_analysis/       # GPU FEM Analysis
│   ├── legal_compliance/          # Legal analysis
│   └── financial_risk/            # Risk models
├── acceleration/
│   ├── cuda_backend/              # CUDA GPU
│   └── vulkan_backend/            # Vulkan GPU
└── importers/
    ├── postgresql/                # Data import
    └── mysql/                     # Data import
```

**Abgrenzung:**
- Diese Funktionen sind **NICHT Teil des Datenbank-Kerns**, da sie:
  - Als separate DLLs/Shared Libraries geladen werden
  - Eigene Lizenzierung haben können (Enterprise)
  - Optional sind (Datenbank funktioniert ohne sie)
  - Spezialisierte Hardware benötigen können (GPU)
  - Externe Abhängigkeiten haben (ML-Libraries)

---

## 3. GPU Impact Analysis Plugin - Architektur

### 3.1 Positionierung in der Architektur

```
┌─────────────────────────────────────────────────────────────────┐
│                          Client Layer                            │
│  (Python SDK, JavaScript SDK, Direct HTTP, AQL Console)          │
└────────────────────────────┬────────────────────────────────────┘
                             │
                             │ HTTP REST API / AQL
                             ▼
┌─────────────────────────────────────────────────────────────────┐
│                      ThemisDB Core (C++)                         │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │  Query Engine                                            │   │
│  │  - AQL Parser                                            │   │
│  │  - Query Optimizer                                       │   │
│  │  - Execution Engine                                      │   │
│  │  - Function Registry  ← GPU_ANALYZE_IMPACT registriert  │   │
│  └──────────────────────────────────────────────────────────┘   │
│                             │                                    │
│  ┌──────────────────────────┼─────────────────────────────────┐ │
│  │  Index Layer             │                                 │ │
│  │  ┌──────────┐  ┌─────────────┐  ┌──────────┐             │ │
│  │  │ Graph    │  │ Vector      │  │ Secondary│             │ │
│  │  │ Index    │  │ Index       │  │ Index    │             │ │
│  │  └──────────┘  └─────────────┘  └──────────┘             │ │
│  └────────────────────────────────────────────────────────────┘ │
│                             │                                    │
│  ┌──────────────────────────┼─────────────────────────────────┐ │
│  │  Plugin Manager          │                                 │ │
│  │  - Plugin Discovery      │                                 │ │
│  │  - DLL Loading           │                                 │ │
│  │  - Lifecycle Management  │                                 │ │
│  └──────────────────────────┼─────────────────────────────────┘ │
└─────────────────────────────┼────────────────────────────────────┘
                              │
                              │ IThemisPlugin Interface
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│           GPU Impact Analysis Plugin (Separate DLL)             │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │  IGPUImpactAnalysisPlugin (Enterprise Interface)         │   │
│  │  - analyzeImpact()                                       │   │
│  │  - monteCarloRisk()                                      │   │
│  │  - temporalImpact()                                      │   │
│  │  - detectPatterns()                                      │   │
│  └──────────────────────────────────────────────────────────┘   │
│                             │                                    │
│  ┌──────────────────────────┼─────────────────────────────────┐ │
│  │  FEM Algorithm Layer     │                                 │ │
│  │  - propagateImpactFEM()  (CPU Fallback)                   │ │
│  │  - GPU Acceleration Hooks                                 │ │
│  └──────────────────────────┼─────────────────────────────────┘ │
│                             │                                    │
│                             ▼                                    │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │  GPU Backend Abstraction                                 │   │
│  │  ┌──────────┐  ┌──────────┐  ┌──────────┐               │   │
│  │  │  CUDA    │  │ Vulkan   │  │   HIP    │               │   │
│  │  │ Backend  │  │ Backend  │  │ Backend  │               │   │
│  │  └──────────┘  └──────────┘  └──────────┘               │   │
│  └──────────────────────────────────────────────────────────┘   │
│                             │                                    │
│  ┌──────────────────────────┼─────────────────────────────────┐ │
│  │  Data Access Layer       │                                 │ │
│  │  - Query ThemisDB via REST API                           │ │
│  │  - Cache results locally                                  │ │
│  │  - No direct RocksDB access                              │ │
│  └──────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
                        GPU Hardware
                     (NVIDIA, AMD, Intel)
```

### 3.2 Klare Trennung

| Aspekt | ThemisDB Core | GPU Impact Plugin |
|--------|---------------|-------------------|
| **Kompilierung** | Monolithischer Binary | Separate DLL/SO |
| **Deployment** | Immer vorhanden | Optional (Enterprise) |
| **Dependencies** | Minimal (RocksDB, Boost) | GPU Libraries (CUDA, Vulkan) |
| **Lizenz** | Open Source (geplant) | Enterprise (kommerziell) |
| **Datenzugriff** | Direkter RocksDB Zugriff | Nur über REST API |
| **Startup** | Beim Server-Start | On-Demand Plugin Load |
| **Fehlertoleranz** | Crash = DB down | Crash = Plugin disabled, DB läuft weiter |

---

## 4. Datenfluss: Query mit GPU Impact Analysis

### 4.1 Beispiel-Query

```sql
LET impact = GPU_ANALYZE_IMPACT(
  {document_id: 'docs/api/payment.md', magnitude: 0.9},
  {max_depth: 10, use_fem_metadata: true}
)

FOR node IN impact.affected_nodes
  FILTER node.impact_score > 0.5
  RETURN {
    document: node.node_id,
    impact: node.impact_score
  }
```

### 4.2 Ausführungsablauf

```
1. Client sendet Query
   │
   ▼
2. ThemisDB Query Engine
   ├─ Parse AQL
   ├─ Erkennt GPU_ANALYZE_IMPACT() Funktion
   └─ Prüft: Ist Plugin geladen?
   │
   ▼
3. Plugin Manager
   ├─ Load GPU Impact Plugin (falls nicht geladen)
   ├─ Validiere Lizenz
   └─ Hole Plugin-Instanz
   │
   ▼
4. GPU Impact Plugin
   ├─ Empfängt analyzeImpact() Call
   ├─ Benötigt Graph-Daten
   └─ Macht REST API Call zu ThemisDB:
       GET /graph/neighbors?node_id=docs/api/payment.md
   │
   ▼
5. ThemisDB Core (separater Request)
   ├─ GraphIndexManager.getNeighbors()
   ├─ Liest aus RocksDB
   └─ Gibt JSON zurück
   │
   ▼
6. GPU Impact Plugin
   ├─ Empfängt Graph-Daten
   ├─ Führt FEM-Algorithmus aus (GPU)
   ├─ Berechnet Impact-Scores
   └─ Gibt Ergebnis als JSON zurück
   │
   ▼
7. ThemisDB Query Engine
   ├─ Empfängt Impact-Ergebnis
   ├─ Führt FOR-Loop aus
   ├─ Filtert nach impact_score > 0.5
   └─ Formatiert Final Response
   │
   ▼
8. Client erhält Ergebnis
```

**Wichtig:** Das Plugin macht **keine direkten RocksDB-Zugriffe**. Es kommuniziert nur über die REST API mit der Datenbank.

---

## 5. Integration Points

### 5.1 AQL Function Registration

**ThemisDB Core (`query/functions/function_registry.cpp`):**

```cpp
void FunctionRegistry::registerEnterprisePluginFunctions() {
    // Wenn GPU Impact Plugin geladen ist
    auto plugin = PluginManager::instance().getPlugin("gpu_impact_analysis");
    if (!plugin) return;
    
    auto gpu_plugin = static_cast<IGPUImpactAnalysisPlugin*>(plugin->getInstance());
    
    // Registriere AQL-Funktionen
    registerFunction("GPU_ANALYZE_IMPACT", [gpu_plugin](const nlohmann::json& args) {
        return gpu_plugin->analyzeImpact(
            args["change"],
            args["config"]
        );
    });
    
    registerFunction("GPU_MONTE_CARLO_RISK", [gpu_plugin](const nlohmann::json& args) {
        return gpu_plugin->monteCarloRisk(
            args["change"],
            args["config"]
        );
    });
    
    // ... weitere Funktionen
}
```

### 5.2 Plugin Discovery & Loading

**Server Startup (`src/server/main.cpp`):**

```cpp
int main() {
    // 1. Initialize ThemisDB Core
    ThemisDB db;
    db.initialize();
    
    // 2. Load Plugins (optional)
    PluginManager::instance().discoverPlugins("./plugins/");
    PluginManager::instance().loadPlugin("gpu_impact_analysis"); // Falls konfiguriert
    
    // 3. Start HTTP Server
    HttpServer server(8765);
    server.start();
}
```

**Plugin Manifest (`plugins/enterprise/gpu_impact_analysis/plugin.json`):**

```json
{
  "name": "gpu_impact_analysis",
  "version": "1.0.0",
  "type": "CUSTOM",
  "description": "GPU-accelerated FEM impact analysis",
  "binary_linux": "libgpu_impact_analysis.so",
  "binary_windows": "gpu_impact_analysis.dll",
  "binary_macos": "libgpu_impact_analysis.dylib",
  "dependencies": [],
  "capabilities": {
    "gpu_accelerated": true,
    "thread_safe": true
  },
  "auto_load": false,
  "license_required": true,
  "enterprise_only": true
}
```

### 5.3 Data Access API für Plugins

**Plugin → ThemisDB Communication:**

Das Plugin nutzt **nur** die öffentliche REST API:

```cpp
// Im GPU Impact Plugin
class PluginDataAccess {
public:
    nlohmann::json getNode(const std::string& node_id) {
        auto response = httpClient_.get(
            "http://localhost:8765/entities/" + node_id
        );
        return nlohmann::json::parse(response.body);
    }
    
    nlohmann::json getNeighbors(const std::string& node_id, int max_depth) {
        auto response = httpClient_.post(
            "http://localhost:8765/graph/neighbors",
            {{"node_id", node_id}, {"max_depth", max_depth}}
        );
        return nlohmann::json::parse(response.body);
    }
    
    std::vector<nlohmann::json> query(const std::string& aql) {
        auto response = httpClient_.post(
            "http://localhost:8765/query",
            {{"query", aql}}
        );
        return nlohmann::json::parse(response.body)["result"];
    }
    
private:
    HttpClient httpClient_;  // Interner HTTP Client
};
```

**Vorteile dieser Architektur:**
- ✅ Plugin ist unabhängig vom Datenbank-Internals
- ✅ Versionierung: Plugin kann mit verschiedenen DB-Versionen arbeiten
- ✅ Sicherheit: Plugin hat keine direkten RocksDB-Rechte
- ✅ Isolation: Plugin-Crash beeinflusst DB nicht
- ✅ Deployment: Plugin kann separat aktualisiert werden

---

## 6. Vergleich: Analytics Layer vs. Plugin Layer

### 6.1 Analytics Layer (Teil der DB)

**Beispiel: Process Mining**

```cpp
// In include/analytics/process_mining.h
class ProcessMiningEngine {
public:
    // Direkter Zugriff auf Datenbank-Internals
    ProcessGraph discoverProcess(
        const std::vector<BaseEntity>& events,
        GraphIndexManager& graph_index
    ) {
        // Nutzt interne Index-Strukturen
        auto adjacencies = graph_index.getAdjacencyList();
        
        // Berechnung in-process
        return buildProcessGraph(events, adjacencies);
    }
};
```

**AQL Integration:**

```sql
FOR event IN events
  COLLECT process_id = event.case_id
  INTO case_events
  RETURN DISCOVER_PROCESS(case_events)  -- Built-in Funktion
```

**Begründung warum Teil der DB:**
- Nutzt interne Index-Strukturen direkt
- Keine GPU-Beschleunigung nötig
- Relativ kleiner Algorithmus (~500 LOC)
- Generisch verwendbar (nicht domain-spezifisch)

---

### 6.2 Plugin Layer (Externe Verarbeitung)

**Beispiel: GPU Impact Analysis**

```cpp
// In plugins/enterprise/gpu_impact_analysis/
class GPUImpactAnalysisPlugin : public IGPUImpactAnalysisPlugin {
public:
    nlohmann::json analyzeImpact(
        const DocumentChange& change,
        const AnalysisConfig& config
    ) override {
        // 1. Hole Daten via REST API
        auto graph = dataAccess_.getNeighbors(change.document_id, config.max_depth);
        
        // 2. GPU-Beschleunigung
        auto gpu_result = gpuBackend_->propagateImpactFEM(graph, config);
        
        // 3. Gib Ergebnis zurück
        return gpu_result;
    }
    
private:
    PluginDataAccess dataAccess_;  // REST API Client
    std::unique_ptr<IGPUBackend> gpuBackend_;  // CUDA/Vulkan/HIP
};
```

**AQL Integration:**

```sql
LET impact = GPU_ANALYZE_IMPACT(...)  -- Plugin-Funktion (registriert wenn Plugin geladen)
```

**Begründung warum Plugin:**
- ⚠️ Benötigt GPU-Hardware (optional)
- ⚠️ Große externe Abhängigkeiten (CUDA Toolkit, Vulkan SDK)
- ⚠️ Enterprise-Feature (Lizenzierung)
- ⚠️ Domain-spezifisch (FEM für Impact-Analyse)
- ⚠️ ~5,000 LOC (zu groß für Core)

---

## 7. Deployment-Szenarien

### 7.1 Minimal Deployment (Open Source)

```
/opt/themisdb/
├── bin/
│   └── themis_server           # Core binary (20 MB)
├── config/
│   └── themis.yaml
└── data/
    └── rocksdb/
```

**Features:**
- ✅ CRUD, AQL Queries
- ✅ Graph, Vector, Spatial Indexing
- ✅ OLAP Analytics (GROUP BY, etc.)
- ✅ Process Mining
- ❌ GPU Impact Analysis (nicht verfügbar)

---

### 7.2 Enterprise Deployment (mit GPU Plugin)

```
/opt/themisdb/
├── bin/
│   └── themis_server           # Core binary (20 MB)
├── plugins/
│   └── enterprise/
│       ├── gpu_impact_analysis/
│       │   ├── libgpu_impact_analysis.so  # Plugin binary (50 MB)
│       │   ├── plugin.json
│       │   └── config.yaml
│       └── licenses/
│           └── gpu_impact_analysis.lic
├── config/
│   └── themis.yaml
│       plugins:
│         auto_load:
│           - gpu_impact_analysis
└── data/
    └── rocksdb/
```

**Features:**
- ✅ Alles aus Minimal Deployment
- ✅ GPU Impact Analysis
- ✅ FEM-basierte Propagierung
- ✅ Monte Carlo Risk
- ✅ Temporal Forecasting

**Hardware-Anforderungen:**
- NVIDIA GPU mit CUDA 11+ (empfohlen)
- Oder AMD GPU mit ROCm
- Oder Vulkan-fähige GPU (Intel, NVIDIA, AMD)

---

## 8. Vorteile dieser Architektur

### 8.1 Modularität

**Problem:** Monolithische Datenbank mit allen Features
- ❌ Binary-Größe: 500+ MB
- ❌ Startup-Zeit: 10+ Sekunden
- ❌ Dependencies: CUDA, Vulkan, ML-Libraries
- ❌ Lizenzierung: Alles oder nichts

**Lösung:** Plugin-Architektur
- ✅ Core binary: 20 MB
- ✅ Startup-Zeit: <1 Sekunde
- ✅ Dependencies: Nur was gebraucht wird
- ✅ Lizenzierung: Granular (Plugin-Level)

---

### 8.2 Unabhängige Entwicklung

**ThemisDB Core:**
- Release-Zyklus: 3 Monate
- Breaking Changes: Selten
- Stabilität: Hoch

**GPU Impact Plugin:**
- Release-Zyklus: Wöchentlich
- Breaking Changes: Häufig (neue FEM-Algorithmen)
- Stabilität: Medium (experimentell)

→ Plugin kann unabhängig entwickelt und released werden

---

### 8.3 Fehler-Isolation

**Szenario:** GPU-Treiber-Crash

**Ohne Plugin:**
```
ThemisDB Server CRASHED (gesamte DB down)
→ Alle Clients betroffen
→ Datenbank muss neu gestartet werden
```

**Mit Plugin:**
```
GPU Impact Plugin CRASHED
→ Plugin wird disabled
→ ThemisDB Core läuft weiter
→ Nur GPU-Funktionen nicht verfügbar
→ Client kann weiterarbeiten (ohne GPU Features)
```

---

### 8.4 Lizenzierung

**Szenario:** Kunde möchte nur Core-Features

**Ohne Plugin:**
- Muss Enterprise-Lizenz kaufen (auch wenn GPU nicht genutzt)
- Teuer

**Mit Plugin:**
- Kauft nur Core-Lizenz (Open Source oder günstiger)
- Kann später GPU-Plugin hinzufügen
- Pay-as-you-grow Modell

---

## 9. Implementierungs-Roadmap

### Phase 1: Plugin Framework (✅ Existiert bereits)

- ✅ `IThemisPlugin` Interface
- ✅ `PluginManager`
- ✅ DLL Loading (Windows/Linux/macOS)
- ✅ Plugin Discovery

**Files:**
- `include/plugins/plugin_interface.h`
- `include/plugins/plugin_manager.h`

---

### Phase 2: GPU Impact Plugin (✅ In diesem PR)

- ✅ Plugin Interface (`IGPUImpactAnalysisPlugin`)
- ✅ Reference Implementation (CPU Fallback)
- ✅ Configuration (YAML)
- ✅ Build System (CMake)
- ✅ Documentation

**Files:**
- `include/enterprise/gpu_impact_analysis_plugin.h`
- `plugins/enterprise/gpu_impact_analysis/`
- `docs/enterprise/gpu_impact_analysis_*.md`

---

### Phase 3: AQL Integration (TODO)

- [ ] Function Registration in ThemisDB Core
- [ ] REST API Endpoints für Plugin-Daten
- [ ] Error Handling (Plugin not loaded)
- [ ] Performance Optimization (Caching)

**Implementation:**

```cpp
// In src/query/functions/function_registry.cpp

void FunctionRegistry::initializePluginFunctions() {
    auto& pm = PluginManager::instance();
    
    // GPU Impact Analysis Plugin
    if (auto plugin = pm.getPlugin("gpu_impact_analysis")) {
        auto gpu = static_cast<IGPUImpactAnalysisPlugin*>(plugin->getInstance());
        
        registerFunction("GPU_ANALYZE_IMPACT", 
            [gpu](const std::vector<Value>& args) -> Value {
                // Convert AQL args to JSON
                nlohmann::json change = args[0].toJson();
                nlohmann::json config = args[1].toJson();
                
                // Call plugin
                auto result = gpu->analyzeImpact(change, config);
                
                // Convert back to AQL Value
                return Value::fromJson(result);
            },
            /* min_args */ 2,
            /* max_args */ 2
        );
        
        // ... weitere Funktionen
    }
}
```

---

### Phase 4: GPU Backend Implementation (TODO)

- [ ] CUDA Backend
- [ ] Vulkan Backend
- [ ] HIP Backend (AMD)
- [ ] Performance Benchmarks

**Architecture:**

```cpp
// In plugins/enterprise/gpu_impact_analysis/backends/

class IGPUBackend {
public:
    virtual ~IGPUBackend() = default;
    virtual bool initialize() = 0;
    virtual nlohmann::json propagateImpactFEM(...) = 0;
};

class CUDABackend : public IGPUBackend { /* CUDA implementation */ };
class VulkanBackend : public IGPUBackend { /* Vulkan implementation */ };
class HIPBackend : public IGPUBackend { /* HIP implementation */ };
```

---

## 10. Best Practices

### 10.1 Wann gehört etwas in ThemisDB Core?

**✅ Gehört in Core wenn:**
- Grundlegende Datenbank-Operationen (CRUD)
- Indizierung (Secondary, Graph, Vector)
- Generische Analytics (COUNT, SUM, GROUP BY)
- Kein spezialisierte Hardware (GPU, TPU)
- Klein (<1,000 LOC)
- Keine externen Dependencies außer Boost, RocksDB
- Open Source freundlich

**Beispiele:**
- Process Mining (generisch, klein, keine GPU)
- OLAP Aggregations (Standard SQL-ähnlich)
- Graph Traversal (Basisoperation)

---

### 10.2 Wann gehört etwas in ein Plugin?

**✅ Gehört in Plugin wenn:**
- Domain-spezifische Logik (Legal, Finance, Healthcare)
- GPU/Hardware-Beschleunigung erforderlich
- Große externe Dependencies (ML-Libraries, GPU SDKs)
- Enterprise-Feature (Lizenzierung)
- Groß (>1,000 LOC)
- Experimentell/instabil
- Nicht jeder Kunde braucht es

**Beispiele:**
- GPU Impact Analysis (GPU, FEM-spezifisch, Enterprise)
- Legal Compliance Checker (Domain-spezifisch)
- Real-time Fraud Detection (ML, spezialisiert)
- Advanced Risk Modeling (Monte Carlo, GPU)

---

## 11. Zusammenfassung

### 11.1 Architektur-Grenzen

| Layer | Verantwortung | Implementierung | Deployment |
|-------|---------------|-----------------|------------|
| **ThemisDB Core** | Daten, Indizes, Queries | Monolith (themis_server) | Immer |
| **Analytics Layer** | Generische Analytics | Teil von Core | Immer |
| **Plugin Layer** | Spezialisierte Verarbeitung | Separate DLLs | Optional |

### 11.2 GPU Impact Analysis Positionierung

**✅ Richtig positioniert als Plugin weil:**
1. GPU-Hardware erforderlich (optional)
2. Große Dependencies (CUDA, Vulkan)
3. Enterprise-Feature (Lizenzierung)
4. Domain-spezifisch (FEM für Impact-Analyse)
5. Experimentell (neue Algorithmen)
6. Groß (~5,000 LOC)

**✅ Klare Abgrenzung durch:**
1. Separate DLL/Shared Library
2. Nur REST API Zugriff (keine RocksDB-Internals)
3. Eigene Lizenzierung
4. Optional beim Deployment
5. Unabhängige Versionierung

### 11.3 Vorteile

- ✅ **Modularität:** Kunden zahlen nur für was sie nutzen
- ✅ **Stabilität:** Plugin-Crash ≠ DB-Crash
- ✅ **Flexibilität:** Plugins können unabhängig entwickelt werden
- ✅ **Skalierung:** Neue Features als Plugins ohne Core zu ändern
- ✅ **Open Source:** Core kann Open Source sein, Plugins kommerziell

---

**Erstellt:** 7. Dezember 2025  
**Version:** 1.0.0  
**Autor:** ThemisDB Architecture Team
