# Implementierungsstrategie: Prompt-Enhancement Priorities

**Projekt:** ThemisDB  
**Kategorie:** Implementation Strategy  
**Basierend auf:** PROMPT_ENHANCEMENT_ENGINE_OPTIMIZATION_RESEARCH.md  
**Status:** 📋 Planning  
**Datum:** 10. Februar 2026  
**Version:** 1.0

---

## 📋 Executive Summary

Dieses Dokument definiert die Implementierungsstrategie für die drei Prioritäten aus der Prompt-Enhancement-Engine-Forschung. Basierend auf der Analyse des ThemisDB-Sourcecodes werden konkrete Implementierungsschritte, Architekturentscheidungen und Zeitpläne definiert.

### Bestehende Infrastruktur (Analyse-Ergebnis)

**✅ Vollständig implementiert:**
- `PromptManager` - Template-Verwaltung
- `PromptOptimizer` - Iterative Verbesserung
- `MetaPromptGenerator` - Meta-Prompting
- `PromptEvaluator` - Qualitätsmetriken (Jaccard-Similarity baseline)
- `PromptPerformanceTracker` - Execution Tracking
- `FeedbackCollector` - Strukturiertes Feedback
- `PromptVersionControl` - Git-like Versionierung
- `SelfImprovementOrchestrator` - A/B Testing & Rollback
- `PromptEngineeringIntegration` - Unified Integration Layer

**Namespace:** `themis::prompt_engineering`  
**Storage:** RocksDB persistence  
**LLM Integration:** `llama_wrapper.h`, `llamacpp_inference_engine.h`

---

## 🎯 Priorität 1: LLM-as-a-Judge Integration

### Zielsetzung

Erweitere den bestehenden `PromptEvaluator` um LLM-basierte Bewertungen zur automatischen Qualitätsbewertung von Prompt-Outputs.

### Motivation

**Aktueller Zustand:**
- `PromptEvaluator` nutzt Jaccard-Similarity (word overlap) als Baseline
- Gut für syntaktische Ähnlichkeit, aber limitiert für semantische Qualität
- Kommentar im Code: "This can be extended with embedding-based similarity"

**Ziel:**
- LLM-as-a-Judge für semantische Qualitätsbewertung
- Strukturierte Bewertungen (Relevance, Accuracy, Completeness, Coherence)
- Grammar-Constrained Generation für JSON-Outputs
- Automatische Integration in Feedback-Loop

### Architektur-Design

#### 1.1 Neue Komponente: `LLMJudge`

```cpp
// include/prompt_engineering/llm_judge.h

namespace themis {
namespace prompt_engineering {

/**
 * @brief Bewertungskriterien für LLM-as-a-Judge
 */
struct JudgeCriteria {
    std::string name;                    // z.B. "relevance", "accuracy"
    std::string description;             // Beschreibung des Kriteriums
    double weight = 1.0;                 // Gewichtung (0.0-1.0)
    int min_score = 1;                   // Minimum Score
    int max_score = 5;                   // Maximum Score
};

/**
 * @brief Ergebnis einer LLM-Judge-Bewertung
 */
struct JudgeResult {
    std::unordered_map<std::string, double> scores;  // Score pro Kriterium
    double overall_score = 0.0;                      // Gewichteter Gesamtscore
    std::string reasoning;                           // LLM-Begründung
    nlohmann::json raw_response;                     // Raw LLM Response
    std::chrono::milliseconds latency;               // Inferenz-Zeit
    
    nlohmann::json toJson() const;
};

/**
 * @brief Konfiguration für LLM Judge
 */
struct LLMJudgeConfig {
    std::vector<JudgeCriteria> criteria;
    std::string judge_prompt_template;
    bool use_grammar_constraint = true;
    std::string grammar_schema;
    double temperature = 0.0;  // Deterministisch
    int max_tokens = 500;
    std::string model_name = "default";  // Falls mehrere Modelle verfügbar
    
    // Defaults für Standard-Bewertung
    static LLMJudgeConfig createDefault();
};

/**
 * @brief LLM-as-a-Judge Evaluator
 * 
 * Nutzt ein LLM zur Bewertung von Prompt-Outputs.
 * Integriert sich nahtlos in bestehenden PromptEvaluator.
 */
class LLMJudge {
public:
    /**
     * @brief Konstruktor
     * @param llm_engine LLM Inference Engine
     * @param config Judge-Konfiguration
     */
    LLMJudge(
        std::shared_ptr<llm::IInferenceEngine> llm_engine,
        const LLMJudgeConfig& config = LLMJudgeConfig::createDefault()
    );
    
    /**
     * @brief Bewerte einen Output
     * @param query Original Query
     * @param output LLM Output zu bewerten
     * @param expected Optionale erwartete Antwort (für Kontext)
     * @return Judge-Ergebnis
     */
    JudgeResult evaluate(
        const std::string& query,
        const std::string& output,
        const std::string& expected = ""
    );
    
    /**
     * @brief Batch-Bewertung
     * @param queries Liste von Queries
     * @param outputs Liste von Outputs
     * @param expected Liste erwarteter Antworten (optional)
     * @return Liste von Judge-Ergebnissen
     */
    std::vector<JudgeResult> evaluateBatch(
        const std::vector<std::string>& queries,
        const std::vector<std::string>& outputs,
        const std::vector<std::string>& expected = {}
    );
    
    /**
     * @brief Generiere Judge Prompt
     * @param query Query
     * @param output Output
     * @param expected Erwartete Antwort
     * @return Generierter Prompt für Judge
     */
    std::string generateJudgePrompt(
        const std::string& query,
        const std::string& output,
        const std::string& expected = ""
    ) const;
    
    /**
     * @brief Parse LLM Response mit Grammar Constraint
     * @param response Raw LLM Response
     * @return Geparste Scores
     */
    JudgeResult parseResponse(const std::string& response) const;
    
    /**
     * @brief Get/Set Config
     */
    const LLMJudgeConfig& getConfig() const { return config_; }
    void setConfig(const LLMJudgeConfig& config);

private:
    std::shared_ptr<llm::IInferenceEngine> llm_engine_;
    LLMJudgeConfig config_;
    mutable std::mutex mutex_;
    
    // Grammar für JSON-constrained generation
    std::string json_grammar_;
    
    void initializeGrammar();
    double computeWeightedScore(const std::unordered_map<std::string, double>& scores) const;
};

} // namespace prompt_engineering
} // namespace themis
```

#### 1.2 Integration in `PromptEvaluator`

**Erweitere bestehenden `PromptEvaluator`:**

```cpp
// Ergänzungen zu include/prompt_engineering/prompt_evaluator.h

class PromptEvaluator {
public:
    // ... bestehende Methoden ...
    
    /**
     * @brief Set LLM Judge (optional)
     * Wenn gesetzt, wird LLM-basierte Bewertung verwendet
     */
    void setLLMJudge(std::shared_ptr<LLMJudge> llm_judge);
    
    /**
     * @brief Evaluate mit LLM Judge
     * Nutzt LLM-basierte Bewertung statt Jaccard-Similarity
     * @param query Original Query
     * @param output Actual output
     * @param expected Expected output (optional für Kontext)
     * @return Enhanced evaluation metrics
     */
    EvaluationMetrics evaluateSingleWithLLM(
        const std::string& query,
        const std::string& output,
        const std::string& expected = ""
    ) const;
    
    /**
     * @brief Hybrid Evaluation: Jaccard + LLM Judge
     * Kombiniert schnelle Jaccard-Baseline mit LLM-Qualität
     * @param query Query
     * @param output Output
     * @param expected Expected
     * @return Combined metrics
     */
    EvaluationMetrics evaluateHybrid(
        const std::string& query,
        const std::string& output,
        const std::string& expected
    ) const;

private:
    std::shared_ptr<LLMJudge> llm_judge_; // Optional LLM Judge
};
```

#### 1.3 Integration in Feedback-Loop

**Automatische LLM-Judge-Bewertung bei jedem Execution:**

```cpp
// Erweiterung in PromptEngineeringIntegration

void PromptEngineeringIntegration::postExecutionHook(
    const std::string& prompt_id,
    const std::string& query,
    const std::string& output,
    bool success,
    double latency_ms,
    double user_feedback
) {
    // ... bestehender Code ...
    
    // Optional: LLM Judge Evaluation
    if (config_.enable_llm_judge && llm_judge_) {
        try {
            auto judge_result = llm_judge_->evaluate(query, output);
            
            // Record als strukturiertes Feedback
            feedback_collector_->recordFeedback(
                prompt_id,
                FeedbackType::LLM_JUDGE_EVALUATION,
                judge_result.reasoning,
                FeedbackSeverity::INFO,
                judge_result.toJson()
            );
            
            // Update performance metrics mit LLM-Score
            if (config_.use_llm_judge_score_in_metrics) {
                performance_tracker_->recordExecution(
                    prompt_id,
                    success,
                    latency_ms,
                    judge_result.overall_score  // LLM Score statt User Feedback
                );
            }
        } catch (const std::exception& e) {
            // Graceful degradation: Log error, aber fail nicht
            spdlog::warn("LLM Judge evaluation failed: {}", e.what());
        }
    }
}
```

### Implementierungsplan

#### Phase 1: LLMJudge Core (Woche 1)

**Aufgaben:**
1. ✅ Create `include/prompt_engineering/llm_judge.h`
2. ✅ Create `src/prompt_engineering/llm_judge.cpp`
3. ✅ Implement `LLMJudge` class mit Basic Functionality
4. ✅ Implement Grammar-Constrained Generation für JSON
5. ✅ Default Judge Prompt Template erstellen

**Deliverables:**
- `LLMJudge` class mit evaluate() und evaluateBatch()
- JSON Grammar für Score-Output
- Unit Tests für LLMJudge

**Geschätzte Zeit:** 5-7 Tage

---

#### Phase 2: PromptEvaluator Integration (Woche 2)

**Aufgaben:**
1. ✅ Extend `PromptEvaluator` mit LLM Judge Support
2. ✅ Implement `evaluateSingleWithLLM()`
3. ✅ Implement `evaluateHybrid()` (Jaccard + LLM)
4. ✅ Add configuration options
5. ✅ Update EvaluationMetrics struct

**Deliverables:**
- Enhanced `PromptEvaluator` mit LLM Judge
- Backward-compatible API (opt-in)
- Integration Tests

**Geschätzte Zeit:** 3-5 Tage

---

#### Phase 3: Integration & Testing (Woche 2-3)

**Aufgaben:**
1. ✅ Integrate LLMJudge in `PromptEngineeringIntegration`
2. ✅ Add Configuration Options zu `IntegrationConfig`
3. ✅ Implement Graceful Degradation (fallback zu Jaccard)
4. ✅ Add Metrics für LLM Judge Performance
5. ✅ End-to-End Testing mit Real LLM
6. ✅ Performance Benchmarks

**Deliverables:**
- Vollständig integrierter LLM Judge
- Configuration Guide
- Performance Report
- Example Code

**Geschätzte Zeit:** 5-7 Tage

---

### API-Beispiele

#### Verwendung von LLMJudge

```cpp
// Setup
auto llm_engine = std::make_shared<LlamaCppInferenceEngine>(model_path);
auto config = LLMJudgeConfig::createDefault();
auto llm_judge = std::make_shared<LLMJudge>(llm_engine, config);

// Single Evaluation
std::string query = "Erkläre Photosynthese";
std::string output = "Photosynthese ist der Prozess, bei dem Pflanzen...";
auto result = llm_judge->evaluate(query, output);

std::cout << "Overall Score: " << result.overall_score << "\n";
std::cout << "Relevance: " << result.scores["relevance"] << "\n";
std::cout << "Accuracy: " << result.scores["accuracy"] << "\n";
std::cout << "Reasoning: " << result.reasoning << "\n";

// Integration in PromptEvaluator
auto evaluator = std::make_shared<PromptEvaluator>();
evaluator->setLLMJudge(llm_judge);

// Hybrid Evaluation
auto metrics = evaluator->evaluateHybrid(query, output, expected);
```

#### Integration in PromptEngineeringIntegration

```cpp
// Configuration
IntegrationConfig config;
config.enable_llm_judge = true;
config.use_llm_judge_score_in_metrics = true;
config.llm_judge_config = LLMJudgeConfig::createDefault();

auto integration = std::make_shared<PromptEngineeringIntegration>(
    /* dependencies */, config
);

// LLM Judge wird automatisch bei postExecutionHook() verwendet
```

### REST API Endpoints

```
# LLM Judge Evaluation
POST /api/v1/prompts/:id/judge-evaluate
Body: {
  "query": "...",
  "output": "...",
  "expected": "..." (optional)
}
Response: {
  "scores": { "relevance": 4.5, "accuracy": 4.0, ... },
  "overall_score": 4.2,
  "reasoning": "The response adequately explains..."
}

# Batch Evaluation
POST /api/v1/prompts/judge-evaluate-batch
Body: {
  "evaluations": [
    { "query": "...", "output": "..." },
    ...
  ]
}

# LLM Judge Configuration
GET /api/v1/judge/config
PUT /api/v1/judge/config
```

### Risiken & Mitigationen

| Risiko | Wahrscheinlichkeit | Impact | Mitigation |
|--------|-------------------|--------|------------|
| LLM-Latenz zu hoch | Mittel | Hoch | Async evaluation, caching, batch processing |
| LLM-Inferenz-Fehler | Niedrig | Mittel | Graceful degradation zu Jaccard, retry logic |
| Inkonsistente Scores | Mittel | Mittel | Temperature=0, grammar constraint, averaging |
| Hohe LLM-Kosten | Hoch | Mittel | Sampling (nicht jeden Request), rate limiting |

### Erfolgskriterien

✅ **Must-Have:**
- LLMJudge evaluiert Prompt-Outputs mit 4+ Kriterien
- Grammar-Constrained JSON Output
- Integration in PromptEvaluator (opt-in)
- Backward compatibility

✅ **Should-Have:**
- <1s Latenz für single evaluation
- Batch evaluation support
- Async evaluation option
- REST API endpoints

✅ **Nice-to-Have:**
- Caching von Judge-Results
- Multiple Judge Models
- Custom Criteria per Prompt

---

## 🎯 Priorität 2: Shadow Testing Framework

### Zielsetzung

Implementiere Shadow Testing für risikofreies Testen neuer Prompt-Versionen in Production.

### Motivation

**Aktueller Zustand:**
- A/B Testing vorhanden (SelfImprovementOrchestrator)
- User sieht Test-Varianten → potentiell schlechtere Erfahrung bei Version B

**Ziel:**
- Shadow Execution: Version B läuft parallel, aber User sieht nur Version A
- Zero User-Impact bei Tests
- Real-world Daten für Evaluation
- Vergleich beider Versionen ohne Risiko

### Architektur-Design

#### 2.1 Neue Komponente: `ShadowTestingManager`

```cpp
// include/prompt_engineering/shadow_testing_manager.h

namespace themis {
namespace prompt_engineering {

/**
 * @brief Konfiguration für Shadow Test
 */
struct ShadowTestConfig {
    std::string test_id;
    std::string prompt_id;
    std::string primary_version;   // Production version (user sieht diese)
    std::string shadow_version;    // Test version (läuft parallel)
    
    double sample_rate = 1.0;      // Anteil Requests für Shadow (0.0-1.0)
    size_t max_samples = 1000;     // Maximum samples zu sammeln
    bool async_shadow = true;      // Shadow async ausführen
    std::chrono::seconds timeout{5}; // Timeout für Shadow execution
    
    // Comparison
    bool enable_comparison = true;
    bool log_differences = true;
    double significant_diff_threshold = 0.1; // 10% Unterschied
    
    nlohmann::json toJson() const;
    static ShadowTestConfig fromJson(const nlohmann::json& j);
};

/**
 * @brief Ergebnis einer Shadow Execution
 */
struct ShadowExecutionResult {
    std::string test_id;
    std::string execution_id;
    
    // Primary (Production)
    std::string primary_output;
    bool primary_success = true;
    double primary_latency_ms = 0.0;
    
    // Shadow (Test)
    std::string shadow_output;
    bool shadow_success = true;
    double shadow_latency_ms = 0.0;
    bool shadow_timeout = false;
    
    // Comparison
    double similarity_score = 0.0;  // Semantic similarity
    bool outputs_differ = false;
    nlohmann::json diff_details;
    
    std::chrono::system_clock::time_point timestamp;
    
    nlohmann::json toJson() const;
};

/**
 * @brief Shadow Test Status
 */
struct ShadowTestStatus {
    std::string test_id;
    ShadowTestConfig config;
    
    size_t samples_collected = 0;
    size_t primary_successes = 0;
    size_t shadow_successes = 0;
    size_t shadow_timeouts = 0;
    
    double mean_similarity = 0.0;
    double primary_mean_latency = 0.0;
    double shadow_mean_latency = 0.0;
    
    std::chrono::system_clock::time_point started_at;
    bool completed = false;
    
    nlohmann::json toJson() const;
};

/**
 * @brief Shadow Testing Manager
 * 
 * Führt Shadow Tests durch:
 * - Primary version läuft normal (user sieht diese)
 * - Shadow version läuft parallel (async, logged)
 * - Results werden verglichen und gespeichert
 */
class ShadowTestingManager {
public:
    /**
     * @brief Konstruktor
     * @param prompt_manager Prompt Manager
     * @param llm_engine LLM Inference Engine
     * @param storage Storage für Results
     */
    ShadowTestingManager(
        std::shared_ptr<PromptManager> prompt_manager,
        std::shared_ptr<llm::IInferenceEngine> llm_engine,
        std::shared_ptr<RocksDBStorage> storage
    );
    
    /**
     * @brief Starte Shadow Test
     * @param config Test-Konfiguration
     * @return Test ID
     */
    std::string startShadowTest(const ShadowTestConfig& config);
    
    /**
     * @brief Führe Shadow Execution durch
     * 
     * Primary wird synchron ausgeführt und returned.
     * Shadow wird async ausgeführt und geloggt.
     * 
     * @param test_id Shadow Test ID
     * @param query User Query
     * @param context Execution Context
     * @return Primary Output (user-facing)
     */
    std::string executeShadow(
        const std::string& test_id,
        const std::string& query,
        const nlohmann::json& context = {}
    );
    
    /**
     * @brief Stop Shadow Test
     * @param test_id Test ID
     */
    void stopShadowTest(const std::string& test_id);
    
    /**
     * @brief Get Shadow Test Status
     * @param test_id Test ID
     * @return Test Status
     */
    ShadowTestStatus getStatus(const std::string& test_id) const;
    
    /**
     * @brief Get Shadow Test Results
     * @param test_id Test ID
     * @param limit Maximum results
     * @return Liste von Execution Results
     */
    std::vector<ShadowExecutionResult> getResults(
        const std::string& test_id,
        size_t limit = 100
    ) const;
    
    /**
     * @brief Analyze Shadow Test
     * 
     * Statistischer Vergleich Primary vs. Shadow
     * 
     * @param test_id Test ID
     * @return Analysis Results
     */
    nlohmann::json analyzeShadowTest(const std::string& test_id) const;
    
    /**
     * @brief List Active Tests
     */
    std::vector<std::string> listActiveTests() const;

private:
    std::shared_ptr<PromptManager> prompt_manager_;
    std::shared_ptr<llm::IInferenceEngine> llm_engine_;
    std::shared_ptr<RocksDBStorage> storage_;
    
    mutable std::mutex mutex_;
    std::unordered_map<std::string, ShadowTestConfig> active_tests_;
    std::unordered_map<std::string, ShadowTestStatus> test_status_;
    
    // Thread pool für async shadow executions
    std::unique_ptr<ThreadPool> thread_pool_;
    
    /**
     * @brief Execute Shadow asynchronously
     */
    void executeShadowAsync(
        const std::string& test_id,
        const std::string& query,
        const std::string& primary_output,
        const nlohmann::json& context
    );
    
    /**
     * @brief Compare outputs
     */
    ShadowExecutionResult compareOutputs(
        const std::string& primary,
        const std::string& shadow
    ) const;
    
    /**
     * @brief Store shadow result
     */
    void storeResult(const ShadowExecutionResult& result);
    
    /**
     * @brief Load test status from storage
     */
    ShadowTestStatus loadStatus(const std::string& test_id) const;
    
    /**
     * @brief Update test status
     */
    void updateStatus(const std::string& test_id, const ShadowExecutionResult& result);
};

} // namespace prompt_engineering
} // namespace themis
```

#### 2.2 Integration in `PromptEngineeringIntegration`

```cpp
// Erweiterung von PromptEngineeringIntegration

class PromptEngineeringIntegration {
public:
    // ... bestehende Methoden ...
    
    /**
     * @brief Start Shadow Test für Prompt
     * @param prompt_id Prompt ID
     * @param shadow_version Version ID für Shadow
     * @param config Shadow Test Config
     * @return Test ID
     */
    std::string startShadowTest(
        const std::string& prompt_id,
        const std::string& shadow_version,
        const ShadowTestConfig& config
    );
    
    /**
     * @brief Stop Shadow Test
     */
    void stopShadowTest(const std::string& test_id);
    
    /**
     * @brief Get Shadow Test Status
     */
    ShadowTestStatus getShadowTestStatus(const std::string& test_id) const;
    
private:
    std::shared_ptr<ShadowTestingManager> shadow_testing_manager_;
};

// Modified preExecutionHook mit Shadow Support
std::string PromptEngineeringIntegration::preExecutionHook(
    const std::string& prompt_id,
    const std::string& query,
    const nlohmann::json& context
) {
    // Check if Shadow Test is active for this prompt
    auto active_shadow_test = findActiveShadowTest(prompt_id);
    
    if (active_shadow_test.has_value()) {
        // Shadow Test Mode: Execute both versions
        return shadow_testing_manager_->executeShadow(
            active_shadow_test->test_id,
            query,
            context
        );
    }
    
    // Normal Mode: Regular prompt enhancement
    return /* ... bestehender Code ... */;
}
```

### Implementierungsplan

#### Phase 1: ShadowTestingManager Core (Woche 1-2)

**Aufgaben:**
1. ✅ Create `include/prompt_engineering/shadow_testing_manager.h`
2. ✅ Create `src/prompt_engineering/shadow_testing_manager.cpp`
3. ✅ Implement Core Shadow Execution Logic
4. ✅ Implement Async Thread Pool
5. ✅ Implement Comparison Logic

**Deliverables:**
- `ShadowTestingManager` class
- Async execution framework
- Output comparison logic
- Unit Tests

**Geschätzte Zeit:** 10-12 Tage

---

#### Phase 2: Storage & Persistence (Woche 2)

**Aufgaben:**
1. ✅ RocksDB Schema für Shadow Results
2. ✅ Implement storeResult() & loadStatus()
3. ✅ Implement result querying
4. ✅ Implement status tracking

**Deliverables:**
- Persistent shadow test storage
- Query API für results
- Status tracking

**Geschätzte Zeit:** 3-5 Tage

---

#### Phase 3: Integration & Analysis (Woche 3-4)

**Aufgaben:**
1. ✅ Integrate in PromptEngineeringIntegration
2. ✅ Implement analyzeShadowTest()
3. ✅ REST API Endpoints
4. ✅ Dashboard/Visualization Support
5. ✅ End-to-End Testing
6. ✅ Performance Benchmarks

**Deliverables:**
- Vollständig integriertes Shadow Testing
- Analysis & Reporting
- REST API
- Documentation

**Geschätzte Zeit:** 10-14 Tage

---

### API-Beispiele

#### Shadow Test Start

```cpp
// Setup
auto shadow_manager = std::make_shared<ShadowTestingManager>(
    prompt_manager, llm_engine, storage
);

// Configure Shadow Test
ShadowTestConfig config;
config.prompt_id = "rag_summarization";
config.primary_version = "v1.0";  // Current production
config.shadow_version = "v1.1";   // New optimized version
config.sample_rate = 1.0;         // Test 100% of requests
config.max_samples = 1000;
config.async_shadow = true;

// Start Test
auto test_id = shadow_manager->startShadowTest(config);

// Execute (automatically uses shadow if active)
auto output = integration->preExecutionHook(prompt_id, query, context);
// User sees primary output, shadow runs in background

// Check Status
auto status = shadow_manager->getStatus(test_id);
std::cout << "Samples: " << status.samples_collected << "/" << config.max_samples << "\n";
std::cout << "Mean Similarity: " << status.mean_similarity << "\n";

// Analyze Results
auto analysis = shadow_manager->analyzeShadowTest(test_id);
std::cout << analysis.dump(2) << "\n";

// Stop Test (when satisfied)
shadow_manager->stopShadowTest(test_id);
```

### REST API Endpoints

```
# Start Shadow Test
POST /api/v1/shadow-tests
Body: {
  "prompt_id": "...",
  "primary_version": "v1.0",
  "shadow_version": "v1.1",
  "sample_rate": 1.0,
  "max_samples": 1000
}

# Get Test Status
GET /api/v1/shadow-tests/:test_id

# Get Test Results
GET /api/v1/shadow-tests/:test_id/results?limit=100

# Analyze Test
GET /api/v1/shadow-tests/:test_id/analyze

# Stop Test
POST /api/v1/shadow-tests/:test_id/stop

# List Active Tests
GET /api/v1/shadow-tests
```

### Risiken & Mitigationen

| Risiko | Wahrscheinlichkeit | Impact | Mitigation |
|--------|-------------------|--------|------------|
| Doppelte LLM-Kosten | Hoch | Hoch | Sample rate < 1.0, timeout enforcement |
| Shadow-Latenz blockiert Primary | Mittel | Hoch | Async execution, timeout |
| Speicher-Overhead | Mittel | Mittel | Result compression, automatic cleanup |
| Thread Pool Exhaustion | Niedrig | Mittel | Pool size limits, queue management |

### Erfolgskriterien

✅ **Must-Have:**
- Shadow execution ohne User-Impact
- Async execution < 100ms overhead
- Output comparison & storage
- Statistical analysis

✅ **Should-Have:**
- Configurable sample rate
- Automatic test completion
- REST API für Test Management
- Real-time status monitoring

✅ **Nice-to-Have:**
- Dashboard visualization
- Alert bei großen Unterschieden
- Auto-promotion zu A/B Test

---

## 🎯 Priorität 3: Canary Deployment Strategy

### Zielsetzung

Erweitere das bestehende A/B Testing Framework zu gestuftem Canary Deployment mit automatischer Promotion/Rollback.

### Motivation

**Aktueller Zustand:**
- A/B Testing vorhanden (SelfImprovementOrchestrator)
- Binary: 50/50 Split zwischen Version A und B
- Deployment nach statistical significance

**Ziel:**
- Graduelles Rollout: 5% → 20% → 50% → 100%
- Automatic Promotion bei Success
- Automatic Rollback bei Degradation
- Safety Guards pro Stage

### Architektur-Design

#### 3.1 Erweiterung von `SelfImprovementOrchestrator`

```cpp
// Ergänzungen zu include/prompt_engineering/self_improvement_orchestrator.h

/**
 * @brief Canary Deployment Stage
 */
enum class CanaryStage {
    STAGE_0 = 0,      // 0% - Not started
    STAGE_1 = 5,      // 5% Canary
    STAGE_2 = 20,     // 20% Canary
    STAGE_3 = 50,     // 50% Canary
    STAGE_4 = 100,    // 100% Full Rollout
    ROLLED_BACK = -1  // Rolled back
};

/**
 * @brief Canary Deployment Configuration
 */
struct CanaryDeploymentConfig {
    // Stages
    std::vector<int> stages = {5, 20, 50, 100};  // Percentage per stage
    
    // Stage Durations (Beobachtungszeitraum pro Stage)
    std::vector<std::chrono::hours> stage_durations = {
        std::chrono::hours(24),  // Stage 1: 24h
        std::chrono::hours(12),  // Stage 2: 12h
        std::chrono::hours(6),   // Stage 3: 6h
        std::chrono::hours(0)    // Stage 4: Immediate (full rollout)
    };
    
    // Minimum Samples pro Stage
    std::vector<size_t> min_samples_per_stage = {100, 200, 500, 0};
    
    // Safety Thresholds
    double min_success_rate = 0.8;           // Rollback wenn < 80%
    double max_error_rate_increase = 0.1;    // Rollback wenn +10% errors
    double max_latency_increase = 0.2;       // Rollback wenn +20% latency
    
    // Auto-Promotion
    bool enable_auto_promotion = true;
    double promotion_confidence = 0.95;      // 95% confidence für promotion
    
    // Auto-Rollback
    bool enable_auto_rollback = true;
    size_t rollback_grace_samples = 50;     // Samples before rollback
    
    nlohmann::json toJson() const;
    static CanaryDeploymentConfig fromJson(const nlohmann::json& j);
};

/**
 * @brief Canary Deployment Status
 */
struct CanaryDeploymentStatus {
    std::string deployment_id;
    std::string prompt_id;
    std::string canary_version;
    std::string baseline_version;
    
    CanaryStage current_stage = CanaryStage::STAGE_0;
    int current_percentage = 0;
    
    // Stage Metrics
    size_t samples_in_current_stage = 0;
    size_t canary_successes = 0;
    size_t canary_failures = 0;
    double canary_mean_latency = 0.0;
    
    size_t baseline_successes = 0;
    size_t baseline_failures = 0;
    double baseline_mean_latency = 0.0;
    
    // Overall Health
    bool is_healthy = true;
    std::string health_reason;
    
    // Timing
    std::chrono::system_clock::time_point stage_started_at;
    std::chrono::system_clock::time_point next_stage_eligible_at;
    
    bool completed = false;
    bool rolled_back = false;
    
    nlohmann::json toJson() const;
};

/**
 * @brief Canary Deployment Decision
 */
struct CanaryDecision {
    enum class Action {
        CONTINUE,      // Continue current stage
        PROMOTE,       // Promote to next stage
        ROLLBACK       // Rollback deployment
    };
    
    Action action;
    std::string reason;
    nlohmann::json metrics;
    
    nlohmann::json toJson() const;
};

class SelfImprovementOrchestrator {
public:
    // ... bestehende Methoden ...
    
    /**
     * @brief Start Canary Deployment
     * 
     * Startet gestuftes Rollout einer neuen Prompt-Version
     * 
     * @param prompt_id Prompt ID
     * @param canary_version Neue Version (Canary)
     * @param config Canary Config
     * @return Deployment ID
     */
    std::string startCanaryDeployment(
        const std::string& prompt_id,
        const std::string& canary_version,
        const CanaryDeploymentConfig& config = CanaryDeploymentConfig{}
    );
    
    /**
     * @brief Record Canary Observation
     * 
     * Wird automatisch bei jeder Execution aufgerufen
     * 
     * @param deployment_id Deployment ID
     * @param is_canary true wenn Canary version verwendet
     * @param success Execution success
     * @param latency_ms Latency
     */
    void recordCanaryObservation(
        const std::string& deployment_id,
        bool is_canary,
        bool success,
        double latency_ms
    );
    
    /**
     * @brief Check Canary Health & Decide Next Action
     * 
     * Prüft ob Canary healthy ist und entscheidet über:
     * - Continue: Weiter sammeln
     * - Promote: Nächste Stage
     * - Rollback: Zurück zu Baseline
     * 
     * @param deployment_id Deployment ID
     * @return Decision (Continue/Promote/Rollback)
     */
    CanaryDecision checkCanaryHealth(const std::string& deployment_id);
    
    /**
     * @brief Promote Canary to Next Stage
     * @param deployment_id Deployment ID
     * @return New stage
     */
    CanaryStage promoteCanary(const std::string& deployment_id);
    
    /**
     * @brief Rollback Canary Deployment
     * @param deployment_id Deployment ID
     * @param reason Rollback reason
     */
    void rollbackCanary(const std::string& deployment_id, const std::string& reason);
    
    /**
     * @brief Get Canary Deployment Status
     */
    CanaryDeploymentStatus getCanaryStatus(const std::string& deployment_id) const;
    
    /**
     * @brief List Active Canary Deployments
     */
    std::vector<std::string> listActiveCanaries() const;
    
    /**
     * @brief Select Version for Execution (Canary Routing)
     * 
     * Entscheidet basierend auf user_id hash ob Canary oder Baseline
     * 
     * @param prompt_id Prompt ID
     * @param user_id User ID (für consistent hashing)
     * @return Version ID zu verwenden
     */
    std::string selectCanaryVersion(
        const std::string& prompt_id,
        const std::string& user_id
    ) const;

private:
    std::unordered_map<std::string, CanaryDeploymentConfig> canary_configs_;
    std::unordered_map<std::string, CanaryDeploymentStatus> canary_status_;
    std::unordered_map<std::string, std::string> prompt_to_canary_; // prompt_id -> deployment_id
    
    /**
     * @brief Background Worker für Canary Monitoring
     */
    void canaryMonitoringLoop();
    
    /**
     * @brief Statistical Comparison Canary vs. Baseline
     */
    bool isCanaryBetter(const CanaryDeploymentStatus& status, double confidence) const;
    
    /**
     * @brief Check Safety Thresholds
     */
    bool isCanarySafe(const CanaryDeploymentStatus& status) const;
};
```

#### 3.2 Background Canary Monitor

```cpp
// Auto-Promotion & Auto-Rollback Worker

void SelfImprovementOrchestrator::canaryMonitoringLoop() {
    while (background_worker_running_) {
        std::this_thread::sleep_for(std::chrono::minutes(5));
        
        std::lock_guard<std::mutex> lock(mutex_);
        
        for (const auto& [deployment_id, status] : canary_status_) {
            if (status.completed || status.rolled_back) {
                continue;
            }
            
            // Check health & decide
            auto decision = checkCanaryHealth(deployment_id);
            
            switch (decision.action) {
                case CanaryDecision::Action::PROMOTE:
                    spdlog::info("Canary {} promoting to next stage: {}",
                                 deployment_id, decision.reason);
                    promoteCanary(deployment_id);
                    break;
                    
                case CanaryDecision::Action::ROLLBACK:
                    spdlog::warn("Canary {} rolling back: {}",
                                 deployment_id, decision.reason);
                    rollbackCanary(deployment_id, decision.reason);
                    break;
                    
                case CanaryDecision::Action::CONTINUE:
                    // Continue monitoring
                    break;
            }
        }
    }
}
```

### Implementierungsplan

#### Phase 1: Canary Core Extensions (Woche 1)

**Aufgaben:**
1. ✅ Extend `SelfImprovementOrchestrator` mit Canary Structs
2. ✅ Implement `startCanaryDeployment()`
3. ✅ Implement `recordCanaryObservation()`
4. ✅ Implement `selectCanaryVersion()` (Consistent Hashing)
5. ✅ Storage Schema für Canary Status

**Deliverables:**
- Canary deployment structures
- Basic canary routing
- Observation recording
- Unit Tests

**Geschätzte Zeit:** 5-7 Tage

---

#### Phase 2: Auto-Promotion & Rollback (Woche 2)

**Aufgaben:**
1. ✅ Implement `checkCanaryHealth()`
2. ✅ Implement Statistical Comparison
3. ✅ Implement Safety Threshold Checks
4. ✅ Implement `promoteCanary()`
5. ✅ Implement `rollbackCanary()`
6. ✅ Background Monitoring Worker

**Deliverables:**
- Health checking logic
- Auto-promotion logic
- Auto-rollback logic
- Background worker
- Integration Tests

**Geschätzte Zeit:** 5-7 Tage

---

#### Phase 3: Integration & UI (Woche 3)

**Aufgaben:**
1. ✅ Integrate in PromptEngineeringIntegration
2. ✅ REST API Endpoints
3. ✅ Monitoring Dashboard Support
4. ✅ Alerting Integration
5. ✅ Documentation
6. ✅ End-to-End Testing

**Deliverables:**
- REST API
- Monitoring integration
- Complete documentation
- Example deployments

**Geschätzte Zeit:** 5-7 Tage

---

### API-Beispiele

#### Canary Deployment Start

```cpp
// Configure Canary Deployment
CanaryDeploymentConfig config;
config.stages = {5, 20, 50, 100};
config.stage_durations = {
    std::chrono::hours(24),
    std::chrono::hours(12),
    std::chrono::hours(6),
    std::chrono::hours(0)
};
config.enable_auto_promotion = true;
config.enable_auto_rollback = true;

// Start Deployment
auto deployment_id = orchestrator->startCanaryDeployment(
    "rag_summarization",
    "v1.2_optimized",
    config
);

// Automatic Routing (in PromptEngineeringIntegration)
auto version = orchestrator->selectCanaryVersion(prompt_id, user_id);
// Returns "v1.2_optimized" für 5% der User (Stage 1)

// Automatic Observation Recording
orchestrator->recordCanaryObservation(deployment_id, is_canary, success, latency);

// Monitor Status
auto status = orchestrator->getCanaryStatus(deployment_id);
std::cout << "Stage: " << static_cast<int>(status.current_stage) << "%\n";
std::cout << "Canary Success Rate: " 
          << (double)status.canary_successes / 
             (status.canary_successes + status.canary_failures) << "\n";

// Background worker automatically promotes/rollbacks
```

### REST API Endpoints

```
# Start Canary Deployment
POST /api/v1/canary-deployments
Body: {
  "prompt_id": "...",
  "canary_version": "v1.2",
  "config": { ... }
}

# Get Deployment Status
GET /api/v1/canary-deployments/:deployment_id

# Manual Promotion
POST /api/v1/canary-deployments/:deployment_id/promote

# Manual Rollback
POST /api/v1/canary-deployments/:deployment_id/rollback

# List Active Deployments
GET /api/v1/canary-deployments

# Get Health Check Result
GET /api/v1/canary-deployments/:deployment_id/health
```

### Risiken & Mitigationen

| Risiko | Wahrscheinlichkeit | Impact | Mitigation |
|--------|-------------------|--------|------------|
| Zu schnelle Promotion | Mittel | Hoch | Min samples per stage, time gates |
| Zu langsame Rollback | Niedrig | Hoch | Real-time monitoring, aggressive thresholds |
| User Assignment Drift | Niedrig | Mittel | Consistent hashing auf user_id |
| Monitoring Overhead | Niedrig | Niedrig | Efficient metrics aggregation |

### Erfolgskriterien

✅ **Must-Have:**
- Staged rollout (min. 3 stages)
- Auto-promotion bei Success
- Auto-rollback bei Degradation
- Consistent user assignment

✅ **Should-Have:**
- Configurable stages & durations
- Real-time health monitoring
- REST API
- Alert integration

✅ **Nice-to-Have:**
- Dashboard visualization
- Manual override controls
- A/B → Canary migration path

---

## 📊 Gesamtüberblick: Implementierungsplan

### Zeitplan

| Priorität | Feature | Wochen | Start | Ende |
|-----------|---------|--------|-------|------|
| **P1** | LLM-as-a-Judge | 2-3 | Woche 1 | Woche 3 |
| **P2** | Shadow Testing | 3-4 | Woche 4 | Woche 7 |
| **P3** | Canary Deployment | 2-3 | Woche 8 | Woche 10 |
| **Total** | **All Priorities** | **7-10** | - | - |

### Meilensteine

**M1: LLM-as-a-Judge Core (Ende Woche 2)**
- ✅ LLMJudge implementiert
- ✅ PromptEvaluator Integration
- ✅ Unit Tests

**M2: LLM-as-a-Judge Production (Ende Woche 3)**
- ✅ PromptEngineeringIntegration
- ✅ REST API
- ✅ Documentation

**M3: Shadow Testing Core (Ende Woche 5)**
- ✅ ShadowTestingManager implementiert
- ✅ Async execution
- ✅ Storage

**M4: Shadow Testing Production (Ende Woche 7)**
- ✅ Integration complete
- ✅ Analysis tools
- ✅ REST API

**M5: Canary Deployment Core (Ende Woche 9)**
- ✅ Canary routing
- ✅ Auto-promotion/rollback
- ✅ Monitoring

**M6: Canary Deployment Production (Ende Woche 10)**
- ✅ REST API
- ✅ Dashboard support
- ✅ Complete testing

---

## 🔧 Technische Dependencies

### Bestehende Komponenten (Wiederverwenden)

✅ **Bereits vorhanden:**
- `PromptManager` - Template management
- `PromptOptimizer` - Optimization logic
- `PromptPerformanceTracker` - Metrics
- `SelfImprovementOrchestrator` - A/B testing baseline
- `PromptEngineeringIntegration` - Integration layer
- RocksDB Storage
- llama.cpp LLM Integration
- Thread Pool Infrastructure

### Neue Dependencies

**P1 (LLM-as-a-Judge):**
- Keine neuen Dependencies (verwendet bestehende LLM-Engine)

**P2 (Shadow Testing):**
- Thread Pool (bereits vorhanden)
- Async execution framework (std::async oder existing thread pool)

**P3 (Canary Deployment):**
- Consistent Hashing (std::hash, bereits vorhanden)
- Background monitoring thread (bereits vorhanden in Integration)

---

## 🧪 Testing-Strategie

### Unit Tests

**Pro Priority:**
- LLMJudge: 10-15 Tests
- ShadowTestingManager: 15-20 Tests
- Canary Extensions: 10-15 Tests

**Total:** ~40-50 neue Unit Tests

### Integration Tests

**End-to-End Scenarios:**
- LLM Judge → Feedback Loop → Optimization
- Shadow Test → Analysis → A/B Test
- Canary Deployment → Auto-Promotion → Full Rollout

**Total:** ~10-15 Integration Tests

### Performance Tests

**Benchmarks:**
- LLM Judge latency (<1s target)
- Shadow overhead (<100ms target)
- Canary routing latency (<1ms target)

---

## 📚 Dokumentation

### Developer Documentation

- **Architecture Guide:** Neue Komponenten und Integration Points
- **API Reference:** REST Endpoints und C++ APIs
- **Configuration Guide:** Alle Config-Optionen
- **Migration Guide:** Upgrade Path für bestehende Installationen

### User Documentation

- **LLM Judge Guide:** Verwendung und Best Practices
- **Shadow Testing Guide:** Setup und Analysis
- **Canary Deployment Guide:** Rollout-Strategien

### Examples

- **Complete Examples:** Code-Beispiele für jede Priority
- **Integration Examples:** End-to-End Workflows
- **Configuration Examples:** Verschiedene Use Cases

---

## 🎯 Erfolgskriterien (Gesamt)

### Must-Have (Alle Priorities)

✅ **P1: LLM-as-a-Judge**
- LLM-basierte Evaluation funktional
- Grammar-constrained JSON Output
- Integration in Feedback Loop
- <1s Latenz für single evaluation

✅ **P2: Shadow Testing**
- Zero User-Impact Testing
- Async execution functional
- Output comparison & analysis
- Storage & querying

✅ **P3: Canary Deployment**
- Staged rollout (min. 3 stages)
- Auto-promotion functional
- Auto-rollback functional
- Monitoring integration

### Should-Have

- REST APIs für alle Features
- Real-time monitoring
- Comprehensive documentation
- Performance benchmarks

### Nice-to-Have

- Dashboard visualizations
- Advanced analytics
- Alert integrations
- Custom configurations

---

## 🚀 Go-Live Strategie

### Phase 1: Internal Testing (Woche 11)

- Deploy auf Staging Environment
- Internal Team Testing
- Performance Validation
- Bug Fixes

### Phase 2: Beta Release (Woche 12)

- Selected Beta Users
- Feedback Collection
- Documentation Refinement
- Edge Case Fixes

### Phase 3: Production Rollout (Woche 13)

- Canary Deployment zu Production (meta!)
- Monitoring & Alerting Setup
- Documentation Release
- Blog Post / Release Notes

---

## 📝 Changelog

| Datum | Version | Änderungen |
|-------|---------|------------|
| 2026-02-10 | 1.0 | Initial Implementation Strategy |

---

**Erstellt:** 10. Februar 2026  
**Autor:** Development Team  
**Version:** 1.0  
**Status:** 📋 Planning Complete

---

*Dieses Dokument definiert die Implementierungsstrategie für die drei Prioritäten aus PROMPT_ENHANCEMENT_ENGINE_OPTIMIZATION_RESEARCH.md. Es dient als Blueprint für die Entwicklung und enthält alle notwendigen technischen Details, Zeitpläne und Erfolgskriterien.*
