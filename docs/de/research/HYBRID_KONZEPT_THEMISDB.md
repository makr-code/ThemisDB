# Hybrid-Konzept für ThemisDB – Kombination von Regelbasierten und ML-basierten Ansätzen

**Stand:** 6. April 2026  
**Version:** 1.0  
**Status:** 🔬 Konzeptionelles Design  
**Kategorie:** Hybrid Adaptive Learning Architecture

---

## 📋 Inhaltsverzeichnis

- [Übersicht](#übersicht)
- [1. Hybrid-Architektur Grundlagen](#1-hybrid-architektur-grundlagen)
- [2. ThemisDB Hybrid Optimizer Architektur](#2-themisdb-hybrid-optimizer-architektur)
- [3. Komponenten-spezifische Hybrid-Konzepte](#3-komponenten-spezifische-hybrid-konzepte)
- [4. Entscheidungslogik & Fallback-Strategien](#4-entscheidungslogik--fallback-strategien)
- [5. Implementierung in ThemisDB](#5-implementierung-in-themisdb)
- [6. Konfiguration & Betrieb](#6-konfiguration--betrieb)
- [7. Monitoring & Feedback Loop](#7-monitoring--feedback-loop)
- [8. Evaluierung & Benchmarks](#8-evaluierung--benchmarks)
- [9. Roadmap & Migration](#9-roadmap--migration)

---

## Übersicht

### Motivation

Das Hybrid-Konzept kombiniert die **Zuverlässigkeit und Vorhersagbarkeit** regelbasierter Systeme mit der **Adaptivität und Leistungsfähigkeit** ML-basierter Ansätze. Für ThemisDB bedeutet dies:

- **Regelbasierte Komponenten** liefern garantierte Baseline-Performance
- **ML-Komponenten** optimieren darüber hinaus für spezifische Workloads
- **Intelligente Fallbacks** sichern Stabilität bei ML-Fehlschlägen
- **Kontinuierliches Lernen** verbessert Performance über Zeit

### Kernprinzipien

1. **Safety First:** Regelbasierte Systeme als Sicherheitsnetz
2. **Gradual Adoption:** ML-Features schrittweise aktivieren
3. **Explainability:** Transparente Entscheidungen (warum ML vs. Regel?)
4. **Performance Guarantees:** Nie schlechter als regelbasierte Baseline
5. **Adaptive Confidence:** ML-Nutzung basierend auf Konfidenz

### Zielgruppe

- ThemisDB Core-Entwickler
- Performance Engineers
- Database Administrators
- System Architects

---

## 1. Hybrid-Architektur Grundlagen

### 1.1 Architektur-Pattern

**Layered Hybrid Architecture:**

```
┌─────────────────────────────────────────────────────────┐
│                   Decision Layer                         │
│  (Wählt zwischen Regel/ML basierend auf Kontext)        │
└────────────┬──────────────────────────┬─────────────────┘
             │                          │
    ┌────────▼────────┐        ┌───────▼────────┐
    │  Rule-Based     │        │   ML-Based      │
    │  Optimizer      │        │   Optimizer     │
    │                 │        │                 │
    │  • Deterministisch      │  • Learned       │
    │  • Schnell       │        │  • Adaptiv      │
    │  • Garantierte   │        │  • Optimiert    │
    │    Baseline      │        │    für Workload │
    └────────┬────────┘        └───────┬────────┘
             │                          │
             └──────────┬───────────────┘
                        │
                 ┌──────▼──────┐
                 │  Execution   │
                 │  Engine      │
                 └──────────────┘
```

### 1.2 Hybrid-Modi

ThemisDB unterstützt verschiedene Hybrid-Modi:

**1. Conservative Mode (Standard)**
- Regel-basiert als Default
- ML nur bei hoher Confidence (>90%)
- Minimales Risiko

**2. Balanced Mode**
- ML für 50% der Queries (basierend auf Heuristiken)
- Regel-basiert für kritische/komplexe Queries
- Optimal für Production

**3. Aggressive Mode**
- ML als Default
- Regel-basiert nur als Fallback
- Maximale Performance-Gains

**4. Shadow Mode**
- Beide Ansätze parallel ausführen
- Nur Regel-basiert für Production
- ML-Ergebnisse für Training

### 1.3 Confidence-basierte Entscheidung

```cpp
enum class OptimizerMode {
    RULE_BASED,
    ML_BASED,
    HYBRID
};

class HybridDecisionEngine {
public:
    OptimizerMode DecideOptimizer(const Query& query) {
        // 1. Blacklist-Check: Immer regelbasiert
        if (IsBlacklisted(query)) {
            return OptimizerMode::RULE_BASED;
        }
        
        // 2. ML-Confidence berechnen
        float ml_confidence = ml_model_->GetConfidence(query);
        
        // 3. Workload-spezifische Heuristiken
        if (IsSimpleQuery(query) || IsCriticalQuery(query)) {
            return OptimizerMode::RULE_BASED;
        }
        
        // 4. Confidence-basierte Entscheidung
        if (ml_confidence > config_.high_confidence_threshold) {
            return OptimizerMode::ML_BASED;
        } else if (ml_confidence > config_.low_confidence_threshold) {
            // Hybrid: ML mit Regel-Validation
            return OptimizerMode::HYBRID;
        } else {
            return OptimizerMode::RULE_BASED;
        }
    }
    
private:
    bool IsBlacklisted(const Query& query) const {
        // Queries mit bekannten ML-Problemen
        return blacklist_.contains(query.pattern_id);
    }
    
    bool IsSimpleQuery(const Query& query) const {
        // Einfache Queries: Regel-based ist schneller
        return query.num_tables <= 2 && 
               query.num_joins == 0;
    }
    
    bool IsCriticalQuery(const Query& query) const {
        // Kritische Queries: Garantierte Performance
        return query.is_transactional || 
               query.max_latency_ms < 10;
    }
};
```

---

## 2. ThemisDB Hybrid Optimizer Architektur

### 2.1 Gesamtarchitektur

```cpp
namespace themis::query {

class HybridQueryOptimizer {
public:
    HybridQueryOptimizer(
        std::unique_ptr<RuleBasedOptimizer> rule_optimizer,
        std::unique_ptr<MLBasedOptimizer> ml_optimizer,
        std::shared_ptr<HybridConfig> config
    ) : rule_optimizer_(std::move(rule_optimizer)),
        ml_optimizer_(std::move(ml_optimizer)),
        config_(config),
        decision_engine_(config),
        fallback_tracker_(config) {}
    
    QueryPlan Optimize(const Query& query) {
        // 1. Entscheide Optimizer-Modus
        OptimizerMode mode = decision_engine_.DecideOptimizer(query);
        
        // 2. Führe Optimization aus
        QueryPlan plan;
        try {
            switch (mode) {
                case OptimizerMode::RULE_BASED:
                    plan = OptimizeWithRules(query);
                    break;
                    
                case OptimizerMode::ML_BASED:
                    plan = OptimizeWithML(query);
                    break;
                    
                case OptimizerMode::HYBRID:
                    plan = OptimizeHybrid(query);
                    break;
            }
        } catch (const MLOptimizationException& e) {
            // Fallback zu regelbasiert
            LOG(WARNING) << "ML optimization failed, falling back to rules: " 
                         << e.what();
            plan = OptimizeWithRules(query);
            fallback_tracker_.RecordFallback(query, e);
        }
        
        // 3. Validiere Plan
        if (!ValidatePlan(plan)) {
            // Fallback bei invaliden Plänen
            plan = OptimizeWithRules(query);
        }
        
        // 4. Track für Feedback
        TrackOptimization(query, plan, mode);
        
        return plan;
    }
    
private:
    QueryPlan OptimizeWithRules(const Query& query) {
        auto start = Clock::now();
        auto plan = rule_optimizer_->Optimize(query);
        metrics_.RecordRuleOptimization(Clock::now() - start);
        return plan;
    }
    
    QueryPlan OptimizeWithML(const Query& query) {
        auto start = Clock::now();
        
        // Timeout für ML-Optimization
        auto future = std::async(std::launch::async, [&]() {
            return ml_optimizer_->Optimize(query);
        });
        
        if (future.wait_for(config_->ml_timeout) == 
            std::future_status::timeout) {
            // Timeout: Fallback zu Regeln
            LOG(WARNING) << "ML optimization timeout";
            return OptimizeWithRules(query);
        }
        
        auto plan = future.get();
        metrics_.RecordMLOptimization(Clock::now() - start);
        return plan;
    }
    
    QueryPlan OptimizeHybrid(const Query& query) {
        // Beide Optimierer ausführen
        auto rule_plan = OptimizeWithRules(query);
        auto ml_plan = OptimizeWithML(query);
        
        // Wähle besseren Plan (basierend auf geschätzten Kosten)
        if (ml_plan.estimated_cost < rule_plan.estimated_cost &&
            ValidateMLPlan(ml_plan, rule_plan)) {
            metrics_.RecordHybridChoice("ml");
            return ml_plan;
        } else {
            metrics_.RecordHybridChoice("rule");
            return rule_plan;
        }
    }
    
    bool ValidateMLPlan(
        const QueryPlan& ml_plan, 
        const QueryPlan& rule_plan
    ) const {
        // ML-Plan darf nicht zu stark von Regel-Plan abweichen
        float cost_ratio = ml_plan.estimated_cost / rule_plan.estimated_cost;
        return cost_ratio > config_->min_cost_ratio &&
               cost_ratio < config_->max_cost_ratio;
    }
    
    std::unique_ptr<RuleBasedOptimizer> rule_optimizer_;
    std::unique_ptr<MLBasedOptimizer> ml_optimizer_;
    std::shared_ptr<HybridConfig> config_;
    HybridDecisionEngine decision_engine_;
    FallbackTracker fallback_tracker_;
    HybridMetrics metrics_;
};

} // namespace themis::query
```

### 2.2 Regelbasierter Optimizer (Baseline)

```cpp
class RuleBasedOptimizer {
public:
    QueryPlan Optimize(const Query& query) {
        QueryPlan plan;
        
        // 1. Join-Order mit klassischen Heuristiken
        plan.join_order = OptimizeJoinOrder(query);
        
        // 2. Index-Selection basierend auf Statistiken
        plan.indexes = SelectIndexes(query);
        
        // 3. Access Methods (Scan vs. Index)
        plan.access_methods = ChooseAccessMethods(query, plan.indexes);
        
        // 4. Aggregation Strategy
        plan.aggregation = ChooseAggregationStrategy(query);
        
        return plan;
    }
    
private:
    JoinOrder OptimizeJoinOrder(const Query& query) {
        // Klassische Dynamic Programming für Join-Order
        // Oder Greedy-Heuristik für >10 Tables
        if (query.num_tables <= 10) {
            return DynamicProgrammingJoinOrder(query);
        } else {
            return GreedyJoinOrder(query);
        }
    }
};
```

### 2.3 ML-basierter Optimizer (Adaptive)

```cpp
class MLBasedOptimizer {
public:
    QueryPlan Optimize(const Query& query) {
        // 1. Feature Extraction
        auto features = ExtractFeatures(query);
        
        // 2. ML Model Inference
        auto ml_output = model_->Predict(features);
        
        // 3. Post-Processing & Validation
        QueryPlan plan = DecodePlan(ml_output, query);
        
        // 4. Sanity Checks
        if (!IsFeasiblePlan(plan)) {
            throw MLOptimizationException("Infeasible plan generated");
        }
        
        return plan;
    }
    
    float GetConfidence(const Query& query) const {
        auto features = ExtractFeatures(query);
        return model_->GetConfidence(features);
    }
    
private:
    std::vector<float> ExtractFeatures(const Query& query) const {
        std::vector<float> features;
        
        // Query Structure
        features.push_back(query.num_tables);
        features.push_back(query.num_joins);
        features.push_back(query.num_predicates);
        
        // Cardinality Estimates
        for (const auto& table : query.tables) {
            features.push_back(stats_->GetCardinality(table));
        }
        
        // Index Availability
        for (const auto& column : query.referenced_columns) {
            features.push_back(stats_->HasIndex(column) ? 1.0f : 0.0f);
        }
        
        // Historical Performance
        auto hist = history_->GetSimilarQueries(query);
        if (!hist.empty()) {
            features.push_back(hist.avg_latency_ms);
        }
        
        return features;
    }
    
    std::unique_ptr<NeuralNetwork> model_;
    std::shared_ptr<Statistics> stats_;
    std::shared_ptr<QueryHistory> history_;
};
```

---

## 3. Komponenten-spezifische Hybrid-Konzepte

### 3.1 Hybrid Cache Management

Integration mit bestehender `AdaptiveQueryCache`:

```cpp
// In src/cache/adaptive_query_cache.cpp erweitern

class HybridCacheManager {
public:
    struct CacheDecision {
        bool should_cache;
        CacheStrategy strategy;
        std::chrono::milliseconds ttl;
    };
    
    CacheDecision DecideCaching(const Query& query, 
                                 const QueryResult& result) {
        // 1. Regelbasierte Heuristiken
        auto rule_decision = ApplyRuleBasedCaching(query, result);
        
        // 2. ML-basierte Vorhersage
        if (ml_enabled_ && ml_model_ready_) {
            auto ml_decision = ApplyMLBasedCaching(query, result);
            
            // 3. Hybrid-Entscheidung
            if (ml_decision.confidence > 0.8) {
                return ml_decision.decision;
            }
        }
        
        // Fallback zu Regeln
        return rule_decision;
    }
    
private:
    CacheDecision ApplyRuleBasedCaching(
        const Query& query, 
        const QueryResult& result
    ) {
        CacheDecision decision;
        
        // Regel 1: Große Results nicht cachen
        if (result.size_bytes > max_cacheable_size_) {
            decision.should_cache = false;
            return decision;
        }
        
        // Regel 2: Frequently accessed queries cachen
        uint64_t access_count = GetAccessCount(query.pattern);
        if (access_count > frequent_threshold_) {
            decision.should_cache = true;
            decision.strategy = CacheStrategy::LRU;
            decision.ttl = std::chrono::hours(1);
            return decision;
        }
        
        // Regel 3: Expensive queries cachen
        if (query.execution_time_ms > expensive_threshold_) {
            decision.should_cache = true;
            decision.strategy = CacheStrategy::LFU;
            decision.ttl = std::chrono::minutes(30);
            return decision;
        }
        
        decision.should_cache = false;
        return decision;
    }
    
    struct MLCacheDecision {
        CacheDecision decision;
        float confidence;
    };
    
    MLCacheDecision ApplyMLBasedCaching(
        const Query& query,
        const QueryResult& result
    ) {
        // Feature Extraction
        std::vector<float> features = {
            static_cast<float>(result.size_bytes),
            static_cast<float>(query.execution_time_ms),
            static_cast<float>(GetAccessCount(query.pattern)),
            static_cast<float>(GetHitRate(query.pattern)),
            static_cast<float>(result.num_rows)
        };
        
        // ML Prediction
        auto prediction = cache_ml_model_->Predict(features);
        
        MLCacheDecision ml_decision;
        ml_decision.confidence = prediction.confidence;
        ml_decision.decision.should_cache = prediction.should_cache;
        ml_decision.decision.ttl = 
            std::chrono::milliseconds(prediction.ttl_ms);
        
        return ml_decision;
    }
    
    bool ml_enabled_ = false;
    bool ml_model_ready_ = false;
    std::unique_ptr<CacheMLModel> cache_ml_model_;
};
```

### 3.2 Hybrid Index Selection

```cpp
// Neue Komponente in src/index/

class HybridIndexAdvisor {
public:
    struct IndexRecommendation {
        std::string column;
        IndexType type;  // BTREE, HASH, HNSW, etc.
        float expected_improvement;
        float confidence;
        std::string reasoning;
    };
    
    std::vector<IndexRecommendation> RecommendIndexes(
        const WorkloadStats& workload
    ) {
        std::vector<IndexRecommendation> recommendations;
        
        // 1. Regelbasierte Index-Empfehlungen
        auto rule_recs = GenerateRuleBasedRecommendations(workload);
        
        // 2. ML-basierte Index-Empfehlungen
        if (ml_enabled_) {
            auto ml_recs = GenerateMLBasedRecommendations(workload);
            
            // 3. Merge und Deduplizierung
            recommendations = MergeRecommendations(rule_recs, ml_recs);
        } else {
            recommendations = rule_recs;
        }
        
        // 4. Sortiere nach expected_improvement
        std::sort(recommendations.begin(), recommendations.end(),
            [](const auto& a, const auto& b) {
                return a.expected_improvement > b.expected_improvement;
            });
        
        return recommendations;
    }
    
private:
    std::vector<IndexRecommendation> GenerateRuleBasedRecommendations(
        const WorkloadStats& workload
    ) {
        std::vector<IndexRecommendation> recs;
        
        // Regel 1: Häufig gefilterte Spalten
        for (const auto& [column, count] : workload.filter_counts) {
            if (count > filter_threshold_ && !HasIndex(column)) {
                IndexRecommendation rec;
                rec.column = column;
                rec.type = DetermineIndexType(column);
                rec.expected_improvement = EstimateImprovement(column, workload);
                rec.confidence = 1.0f;  // Regelbasiert = hohe Confidence
                rec.reasoning = "Häufig in WHERE-Klauseln verwendet";
                recs.push_back(rec);
            }
        }
        
        // Regel 2: Join-Spalten
        for (const auto& [column, count] : workload.join_counts) {
            if (count > join_threshold_ && !HasIndex(column)) {
                IndexRecommendation rec;
                rec.column = column;
                rec.type = IndexType::BTREE;
                rec.expected_improvement = EstimateJoinImprovement(column);
                rec.confidence = 1.0f;
                rec.reasoning = "Häufig in JOINs verwendet";
                recs.push_back(rec);
            }
        }
        
        return recs;
    }
    
    std::vector<IndexRecommendation> GenerateMLBasedRecommendations(
        const WorkloadStats& workload
    ) {
        // Feature Extraction für gesamten Workload
        auto features = ExtractWorkloadFeatures(workload);
        
        // ML Model Prediction
        auto predictions = index_ml_model_->PredictIndexes(features);
        
        std::vector<IndexRecommendation> recs;
        for (const auto& pred : predictions) {
            if (pred.score > ml_threshold_) {
                IndexRecommendation rec;
                rec.column = pred.column;
                rec.type = pred.index_type;
                rec.expected_improvement = pred.improvement;
                rec.confidence = pred.confidence;
                rec.reasoning = "ML-Vorhersage: " + pred.explanation;
                recs.push_back(rec);
            }
        }
        
        return recs;
    }
    
    bool ml_enabled_ = false;
    std::unique_ptr<IndexMLModel> index_ml_model_;
};
```

### 3.3 Hybrid Compaction Strategy

```cpp
// Integration mit RocksDB Compaction

class HybridCompactionScheduler {
public:
    rocksdb::CompactionStyle DecideCompactionStyle(
        const LSMTreeStats& stats
    ) {
        // 1. Regelbasierte Entscheidung
        auto rule_style = ApplyCompactionRules(stats);
        
        // 2. ML-basierte Optimierung (falls trainiert)
        if (ml_model_trained_ && stats.num_samples > min_samples_) {
            auto ml_style = ml_model_->PredictCompactionStyle(stats);
            float ml_confidence = ml_model_->GetConfidence(stats);
            
            if (ml_confidence > 0.9) {
                LOG(INFO) << "Using ML compaction style";
                return ml_style;
            }
        }
        
        // Fallback zu Regeln
        LOG(INFO) << "Using rule-based compaction style";
        return rule_style;
    }
    
private:
    rocksdb::CompactionStyle ApplyCompactionRules(
        const LSMTreeStats& stats
    ) {
        // Regel 1: Write-Heavy Workload → Universal Compaction
        if (stats.write_amplification > 10.0) {
            return rocksdb::kCompactionStyleUniversal;
        }
        
        // Regel 2: Read-Heavy Workload → Leveled Compaction
        if (stats.read_write_ratio > 10.0) {
            return rocksdb::kCompactionStyleLevel;
        }
        
        // Regel 3: Mixed Workload → FIFO
        return rocksdb::kCompactionStyleFIFO;
    }
    
    bool ml_model_trained_ = false;
    std::unique_ptr<CompactionMLModel> ml_model_;
};
```

---

## 4. Entscheidungslogik & Fallback-Strategien

### 4.1 Decision Tree für Optimizer-Auswahl

```
                        Start Query
                            |
                            v
                  ┌─────────────────┐
                  │ Is Blacklisted? │
                  └────┬────────┬───┘
                  YES  │        │ NO
                       v        v
                  [RULE]   ┌──────────────┐
                           │ Is Critical? │
                           └──┬───────┬───┘
                         YES  │       │ NO
                              v       v
                         [RULE]  ┌────────────────┐
                                 │ ML Available?  │
                                 └──┬────────┬────┘
                               YES  │        │ NO
                                    v        v
                            ┌──────────┐  [RULE]
                            │ML Conf?  │
                            └──┬───┬───┘
                          High │   │ Low
                               v   v
                          [ML] [HYBRID]
```

### 4.2 Fallback-Kaskade

```cpp
class FallbackCascade {
public:
    QueryPlan OptimizeWithFallback(const Query& query) {
        // Level 1: ML Optimizer (falls verfügbar und confident)
        if (TryMLOptimization(query, plan)) {
            return plan;
        }
        
        // Level 2: Hybrid Approach (ML + Rule Validation)
        if (TryHybridOptimization(query, plan)) {
            return plan;
        }
        
        // Level 3: Rule-Based Optimizer (garantiert erfolgreich)
        return rule_optimizer_->Optimize(query);
    }
    
private:
    bool TryMLOptimization(const Query& query, QueryPlan& plan) {
        try {
            if (!ml_optimizer_->IsAvailable()) {
                return false;
            }
            
            float confidence = ml_optimizer_->GetConfidence(query);
            if (confidence < high_confidence_threshold_) {
                return false;
            }
            
            plan = ml_optimizer_->Optimize(query);
            
            // Post-Validation
            if (!ValidatePlan(plan)) {
                LOG(WARNING) << "ML plan validation failed";
                return false;
            }
            
            return true;
            
        } catch (const std::exception& e) {
            LOG(ERROR) << "ML optimization exception: " << e.what();
            return false;
        }
    }
    
    bool TryHybridOptimization(const Query& query, QueryPlan& plan) {
        try {
            auto rule_plan = rule_optimizer_->Optimize(query);
            auto ml_plan = ml_optimizer_->Optimize(query);
            
            // Wähle den besseren Plan
            if (ComparePlans(ml_plan, rule_plan) > 0) {
                plan = ml_plan;
            } else {
                plan = rule_plan;
            }
            
            return true;
            
        } catch (const std::exception& e) {
            LOG(ERROR) << "Hybrid optimization exception: " << e.what();
            return false;
        }
    }
};
```

### 4.3 Circuit Breaker für ML-Komponenten

```cpp
class MLCircuitBreaker {
public:
    enum class State {
        CLOSED,    // ML funktioniert normal
        OPEN,      // ML ist deaktiviert (zu viele Fehler)
        HALF_OPEN  // ML wird getestet
    };
    
    bool AllowMLExecution() {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto now = Clock::now();
        
        switch (state_) {
            case State::CLOSED:
                return true;
                
            case State::OPEN:
                // Check ob Timeout abgelaufen
                if (now - last_failure_time_ > recovery_timeout_) {
                    state_ = State::HALF_OPEN;
                    LOG(INFO) << "Circuit breaker entering HALF_OPEN state";
                    return true;
                }
                return false;
                
            case State::HALF_OPEN:
                // Nur 1 Request durchlassen zum Testen
                return !test_request_active_;
        }
        
        return false;
    }
    
    void RecordSuccess() {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (state_ == State::HALF_OPEN) {
            // Test erfolgreich → zurück zu CLOSED
            state_ = State::CLOSED;
            failure_count_ = 0;
            LOG(INFO) << "Circuit breaker CLOSED (test successful)";
        }
        
        test_request_active_ = false;
    }
    
    void RecordFailure() {
        std::lock_guard<std::mutex> lock(mutex_);
        
        failure_count_++;
        last_failure_time_ = Clock::now();
        test_request_active_ = false;
        
        if (state_ == State::CLOSED && 
            failure_count_ >= failure_threshold_) {
            // Zu viele Fehler → OPEN
            state_ = State::OPEN;
            LOG(WARNING) << "Circuit breaker OPEN (too many failures)";
        } else if (state_ == State::HALF_OPEN) {
            // Test fehlgeschlagen → zurück zu OPEN
            state_ = State::OPEN;
            LOG(WARNING) << "Circuit breaker OPEN (test failed)";
        }
    }
    
private:
    State state_ = State::CLOSED;
    uint32_t failure_count_ = 0;
    TimePoint last_failure_time_;
    bool test_request_active_ = false;
    
    const uint32_t failure_threshold_ = 5;
    const Duration recovery_timeout_ = std::chrono::minutes(5);
    
    std::mutex mutex_;
};
```

---
## 5. Implementierung in ThemisDB

### 5.1 Integration mit bestehenden Komponenten

**Query Component (`src/query/`)**

```cpp
// Erweitere QueryEngine mit Hybrid Support

namespace themis::query {

class QueryEngine {
public:
    void EnableHybridOptimization(const HybridConfig& config) {
        if (!hybrid_optimizer_) {
            hybrid_optimizer_ = std::make_unique<HybridQueryOptimizer>(
                std::make_unique<RuleBasedOptimizer>(stats_),
                std::make_unique<MLBasedOptimizer>(ml_model_path_),
                std::make_shared<HybridConfig>(config)
            );
        }
        use_hybrid_ = true;
    }
    
    QueryResult Execute(const Query& query) {
        // Use hybrid optimizer if enabled
        QueryPlan plan;
        if (use_hybrid_ && hybrid_optimizer_) {
            plan = hybrid_optimizer_->Optimize(query);
        } else {
            plan = traditional_optimizer_->Optimize(query);
        }
        
        return execution_engine_->Execute(plan);
    }
    
private:
    std::unique_ptr<HybridQueryOptimizer> hybrid_optimizer_;
    std::unique_ptr<TraditionalOptimizer> traditional_optimizer_;
    bool use_hybrid_ = false;
};

} // namespace themis::query
```

**Cache Component (`src/cache/adaptive_query_cache.cpp`)**

```cpp
// Erweitere AdaptiveQueryCache

class AdaptiveQueryCache {
public:
    void EnableHybridCaching(bool enable) {
        if (enable && !hybrid_manager_) {
            hybrid_manager_ = std::make_unique<HybridCacheManager>(
                cache_ml_model_path_
            );
        }
        use_hybrid_caching_ = enable;
    }
    
    void MaybeCacheResult(const Query& query, const QueryResult& result) {
        if (use_hybrid_caching_ && hybrid_manager_) {
            auto decision = hybrid_manager_->DecideCaching(query, result);
            if (decision.should_cache) {
                Cache(query, result, decision.ttl);
            }
        } else {
            // Traditional caching logic
            if (ShouldCache(query, result)) {
                Cache(query, result, default_ttl_);
            }
        }
    }
    
private:
    std::unique_ptr<HybridCacheManager> hybrid_manager_;
    bool use_hybrid_caching_ = false;
};
```

**Index Component (`src/index/`)**

```cpp
// Neue Datei: src/index/hybrid_index_advisor.cpp

namespace themis::index {

class HybridIndexAdvisor {
public:
    HybridIndexAdvisor(
        std::shared_ptr<Statistics> stats,
        const std::string& ml_model_path
    ) : stats_(stats) {
        if (!ml_model_path.empty()) {
            LoadMLModel(ml_model_path);
        }
    }
    
    std::vector<IndexRecommendation> AnalyzeWorkload(
        const WorkloadStats& workload
    ) {
        return RecommendIndexes(workload);
    }
    
    void ExportRecommendations(
        const std::vector<IndexRecommendation>& recs,
        const std::string& output_path
    ) {
        // Export als SQL Script
        std::ofstream out(output_path);
        out << "-- Generated Index Recommendations\n";
        out << "-- Date: " << CurrentTimestamp() << "\n\n";
        
        for (const auto& rec : recs) {
            out << "-- Reason: " << rec.reasoning << "\n";
            out << "-- Expected Improvement: " 
                << rec.expected_improvement * 100 << "%\n";
            out << "CREATE INDEX IF NOT EXISTS idx_" 
                << rec.column << " ON " << rec.table 
                << "(" << rec.column << ");\n\n";
        }
    }
};

} // namespace themis::index
```

### 5.2 Phasenweise Einführung

**Phase 1: Shadow Mode (Monate 1-2)**

```cpp
// Aktiviere Shadow Mode in Config
HybridConfig config;
config.mode = HybridMode::SHADOW;
config.log_comparisons = true;

query_engine_->EnableHybridOptimization(config);

// Beide Optimizer laufen parallel
// Nur regelbasierte Pläne werden ausgeführt
// ML-Pläne werden geloggt für Analyse
```

**Phase 2: Conservative Mode (Monate 3-4)**

```cpp
// Wechsel zu Conservative Mode
config.mode = HybridMode::CONSERVATIVE;
config.high_confidence_threshold = 0.95;  // Sehr konservativ
config.enable_circuit_breaker = true;

query_engine_->EnableHybridOptimization(config);

// ML wird nur bei sehr hoher Confidence verwendet
// Regelbasiert als Standard
```

**Phase 3: Balanced Mode (Monate 5-6)**

```cpp
// Wechsel zu Balanced Mode
config.mode = HybridMode::BALANCED;
config.high_confidence_threshold = 0.80;
config.low_confidence_threshold = 0.50;

query_engine_->EnableHybridOptimization(config);

// ML für ~50% der Queries
// Hybrid Mode für mittlere Confidence
```

**Phase 4: Aggressive Mode (Optional, ab Monat 7)**

```cpp
// Nur für Performance-kritische Workloads
config.mode = HybridMode::AGGRESSIVE;
config.high_confidence_threshold = 0.60;
config.prefer_ml_for_complex_queries = true;

query_engine_->EnableHybridOptimization(config);
```

### 5.3 Training Pipeline

```cpp
class HybridTrainingPipeline {
public:
    void TrainModels() {
        // 1. Sammle Trainingsdaten
        LOG(INFO) << "Collecting training data...";
        auto training_data = CollectTrainingData();
        
        // 2. Trainiere Query Optimizer Model
        LOG(INFO) << "Training query optimizer model...";
        TrainQueryOptimizerModel(training_data.query_data);
        
        // 3. Trainiere Cache Manager Model
        LOG(INFO) << "Training cache manager model...";
        TrainCacheManagerModel(training_data.cache_data);
        
        // 4. Trainiere Index Advisor Model
        LOG(INFO) << "Training index advisor model...";
        TrainIndexAdvisorModel(training_data.index_data);
        
        // 5. Evaluiere Models
        LOG(INFO) << "Evaluating models...";
        auto metrics = EvaluateModels();
        
        // 6. Deploy Models (falls Verbesserung)
        if (metrics.query_opt_improvement > 0.05) {  // >5% Verbesserung
            DeployModel("query_optimizer", metrics.query_opt_version);
        }
        
        LOG(INFO) << "Training pipeline completed";
    }
    
private:
    TrainingData CollectTrainingData() {
        TrainingData data;
        
        // Query Logs der letzten 7 Tage
        data.query_data = query_logger_->GetLogs(Days(7));
        
        // Cache Hit/Miss Statistics
        data.cache_data = cache_tracker_->GetStatistics(Days(7));
        
        // Index Usage Statistics
        data.index_data = index_tracker_->GetUsageStats(Days(7));
        
        return data;
    }
    
    void TrainQueryOptimizerModel(const QueryLogData& data) {
        // Feature Engineering
        auto features = ExtractQueryFeatures(data);
        auto labels = ExtractQueryLabels(data);
        
        // Model Training
        auto model = std::make_unique<QueryOptimizerNN>();
        model->Train(features, labels, training_config_);
        
        // Save Model
        model->Save(model_dir_ + "/query_optimizer_latest.pb");
    }
};
```

---

## 6. Konfiguration & Betrieb

### 6.1 Konfigurations-Schema

```yaml
# config/hybrid_learning.yaml

hybrid_learning:
  # Global Settings
  enabled: true
  mode: "balanced"  # shadow | conservative | balanced | aggressive
  
  # Query Optimizer
  query_optimizer:
    enabled: true
    ml_enabled: true
    ml_model_path: "models/query_optimizer.onnx"
    
    # Confidence Thresholds
    high_confidence_threshold: 0.80
    low_confidence_threshold: 0.50
    
    # Timeouts
    ml_optimization_timeout_ms: 100
    
    # Fallback Behavior
    fallback_on_timeout: true
    fallback_on_error: true
    
    # Circuit Breaker
    circuit_breaker:
      enabled: true
      failure_threshold: 5
      recovery_timeout_sec: 300  # 5 Minuten
    
    # Blacklist
    blacklisted_patterns:
      - "SELECT * FROM large_table"  # Example
  
  # Cache Manager
  cache_manager:
    hybrid_enabled: true
    ml_model_path: "models/cache_manager.onnx"
    
    # Rule-Based Thresholds
    max_cacheable_size_mb: 10
    frequent_access_threshold: 100
    expensive_query_threshold_ms: 1000
    
    # ML Thresholds
    ml_confidence_threshold: 0.80
  
  # Index Advisor
  index_advisor:
    hybrid_enabled: true
    ml_model_path: "models/index_advisor.onnx"
    
    # Rule-Based Thresholds
    filter_count_threshold: 1000
    join_count_threshold: 500
    
    # ML Thresholds
    ml_recommendation_threshold: 0.70
  
  # Training Pipeline
  training:
    auto_training_enabled: false  # Manual triggering initially
    training_schedule: "0 2 * * 0"  # Sunday 2 AM
    training_data_days: 7
    min_samples_required: 10000
    
    # Model Versioning
    keep_last_n_models: 5
    auto_deploy_if_improvement: true
    min_improvement_threshold: 0.05  # 5%
  
  # Monitoring
  monitoring:
    log_optimizer_decisions: true
    log_fallbacks: true
    log_ml_predictions: false  # High volume
    
    # Metrics Export
    prometheus_enabled: true
    metrics_port: 9090
    
    # Performance Tracking
    track_query_performance: true
    track_cache_hit_rate: true
    track_index_usage: true
  
  # Shadow Mode Settings
  shadow_mode:
    compare_all_queries: false
    sample_rate: 0.10  # 10% der Queries
    log_comparisons: true
    comparison_log_path: "logs/shadow_comparisons.log"
```

### 6.2 Runtime Configuration via API

```cpp
// REST API für Runtime Configuration

// GET /api/v1/config/hybrid_learning
// Returns: Current hybrid learning configuration

// PUT /api/v1/config/hybrid_learning/mode
// Body: { "mode": "balanced" }
// Updates: Optimizer mode

// POST /api/v1/config/hybrid_learning/reload
// Reloads: Configuration from file

// Example Usage:
curl -X PUT http://localhost:8529/api/v1/config/hybrid_learning/mode \
     -H "Content-Type: application/json" \
     -d '{"mode": "conservative"}'
```

### 6.3 CLI Tools

```bash
# ThemisDB Hybrid Learning CLI

# Check Status
themisdb-hybrid status

# Output:
# Hybrid Learning Status:
#   Mode: balanced
#   Query Optimizer: enabled (ML: active, confidence: 0.82)
#   Cache Manager: enabled (ML: active)
#   Index Advisor: enabled (ML: active)
#   Circuit Breaker: closed
#   Last Training: 2026-02-08 02:00:00

# Change Mode
themisdb-hybrid set-mode aggressive

# Trigger Training
themisdb-hybrid train --component query_optimizer

# View Recommendations
themisdb-hybrid index-advisor --analyze-workload --days 7

# Output:
# Index Recommendations (Last 7 Days):
#   1. CREATE INDEX idx_user_email ON users(email)
#      Reason: Frequently used in filters
#      Expected Improvement: 45%
#      Confidence: 0.92
#
#   2. CREATE INDEX idx_order_date ON orders(date)
#      Reason: Range queries on date column
#      Expected Improvement: 38%
#      Confidence: 0.88

# Export SQL
themisdb-hybrid index-advisor --export-sql recommendations.sql
```

---

## 7. Monitoring & Feedback Loop

### 7.1 Metriken

**Query Optimizer Metrics:**

```cpp
class HybridOptimizerMetrics {
public:
    // Decision Metrics
    uint64_t rule_based_count = 0;
    uint64_t ml_based_count = 0;
    uint64_t hybrid_count = 0;
    uint64_t fallback_count = 0;
    
    // Performance Metrics
    HdrHistogram rule_optimization_latency;
    HdrHistogram ml_optimization_latency;
    HdrHistogram hybrid_optimization_latency;
    
    // Quality Metrics
    HdrHistogram rule_query_latency;
    HdrHistogram ml_query_latency;
    HdrHistogram hybrid_query_latency;
    
    // ML Confidence Distribution
    HdrHistogram ml_confidence;
    
    // Circuit Breaker State
    std::atomic<uint32_t> circuit_breaker_open_count{0};
    
    void ExportPrometheus(PrometheusExporter& exporter) {
        // Counter Metrics
        exporter.AddCounter("themis_optimizer_decisions_total",
                           {{"type", "rule"}}, rule_based_count);
        exporter.AddCounter("themis_optimizer_decisions_total",
                           {{"type", "ml"}}, ml_based_count);
        exporter.AddCounter("themis_optimizer_decisions_total",
                           {{"type", "hybrid"}}, hybrid_count);
        exporter.AddCounter("themis_optimizer_fallbacks_total",
                           {}, fallback_count);
        
        // Histogram Metrics
        exporter.AddHistogram("themis_optimization_latency_ms",
                             {{"type", "rule"}}, 
                             rule_optimization_latency);
        exporter.AddHistogram("themis_optimization_latency_ms",
                             {{"type", "ml"}}, 
                             ml_optimization_latency);
        
        exporter.AddHistogram("themis_query_latency_ms",
                             {{"optimizer", "rule"}}, 
                             rule_query_latency);
        exporter.AddHistogram("themis_query_latency_ms",
                             {{"optimizer", "ml"}}, 
                             ml_query_latency);
        
        // Gauge Metrics
        exporter.AddGauge("themis_ml_confidence_avg",
                         {}, ml_confidence.Mean());
        exporter.AddGauge("themis_circuit_breaker_open",
                         {}, circuit_breaker_open_count.load());
    }
};
```

**Grafana Dashboard:**

```json
{
  "dashboard": {
    "title": "ThemisDB Hybrid Learning",
    "panels": [
      {
        "title": "Optimizer Decision Distribution",
        "type": "piechart",
        "targets": [
          {
            "expr": "sum(themis_optimizer_decisions_total) by (type)"
          }
        ]
      },
      {
        "title": "Query Latency by Optimizer",
        "type": "graph",
        "targets": [
          {
            "expr": "histogram_quantile(0.95, themis_query_latency_ms)"
          }
        ]
      },
      {
        "title": "ML Confidence Distribution",
        "type": "heatmap",
        "targets": [
          {
            "expr": "themis_ml_confidence_avg"
          }
        ]
      },
      {
        "title": "Fallback Rate",
        "type": "graph",
        "targets": [
          {
            "expr": "rate(themis_optimizer_fallbacks_total[5m])"
          }
        ]
      }
    ]
  }
}
```

### 7.2 Feedback Loop

```cpp
class FeedbackLoop {
public:
    void RecordQueryExecution(
        const Query& query,
        const QueryPlan& plan,
        OptimizerMode mode,
        const QueryStats& stats
    ) {
        // 1. Store für Training
        training_buffer_.Add({
            .query = query,
            .plan = plan,
            .mode = mode,
            .latency_ms = stats.execution_time_ms,
            .rows_scanned = stats.rows_scanned,
            .timestamp = Clock::now()
        });
        
        // 2. Update Online-Statistiken
        if (mode == OptimizerMode::ML_BASED) {
            ml_performance_tracker_.Update(stats);
        } else {
            rule_performance_tracker_.Update(stats);
        }
        
        // 3. Detect Regressions
        if (DetectRegression(query, stats)) {
            LOG(WARNING) << "Performance regression detected for query: " 
                         << query.id;
            regression_handler_.Handle(query, stats);
        }
        
        // 4. Periodic Training Trigger
        if (training_buffer_.Size() > training_batch_size_) {
            TriggerIncrementalTraining();
        }
    }
    
private:
    bool DetectRegression(const Query& query, const QueryStats& stats) {
        // Vergleiche mit historischer Performance
        auto historical = GetHistoricalStats(query.pattern);
        if (!historical) return false;
        
        // Regression = >50% langsamer als P95 historical
        return stats.execution_time_ms > historical->p95_latency_ms * 1.5;
    }
    
    void TriggerIncrementalTraining() {
        // Async Training mit neuesten Daten
        training_executor_->Submit([this]() {
            auto batch = training_buffer_.Flush();
            model_trainer_->IncrementalTrain(batch);
        });
    }
};
```

---

## 8. Evaluierung & Benchmarks

### 8.1 Benchmark Suite

```cpp
class HybridBenchmarkSuite {
public:
    struct BenchmarkResult {
        std::string workload_name;
        double rule_throughput;
        double ml_throughput;
        double hybrid_throughput;
        double rule_p95_latency;
        double ml_p95_latency;
        double hybrid_p95_latency;
        double improvement_percent;
    };
    
    std::vector<BenchmarkResult> RunBenchmarks() {
        std::vector<BenchmarkResult> results;
        
        // Workload 1: OLTP (Read-Heavy)
        results.push_back(BenchmarkOLTPReadHeavy());
        
        // Workload 2: OLTP (Write-Heavy)
        results.push_back(BenchmarkOLTPWriteHeavy());
        
        // Workload 3: OLAP (Complex Queries)
        results.push_back(BenchmarkOLAPComplex());
        
        // Workload 4: Mixed (50/50)
        results.push_back(BenchmarkMixed());
        
        return results;
    }
    
private:
    BenchmarkResult BenchmarkOLTPReadHeavy() {
        BenchmarkResult result;
        result.workload_name = "OLTP Read-Heavy";
        
        // Configure Workload
        YCSBConfig config;
        config.workload = "workloada";  // 50% reads, 50% updates
        config.record_count = 1000000;
        config.operation_count = 100000;
        
        // Run with Rule-Based
        SetOptimizerMode(OptimizerMode::RULE_BASED);
        auto rule_stats = RunYCSB(config);
        result.rule_throughput = rule_stats.throughput;
        result.rule_p95_latency = rule_stats.p95_latency_ms;
        
        // Run with ML-Based
        SetOptimizerMode(OptimizerMode::ML_BASED);
        auto ml_stats = RunYCSB(config);
        result.ml_throughput = ml_stats.throughput;
        result.ml_p95_latency = ml_stats.p95_latency_ms;
        
        // Run with Hybrid
        SetOptimizerMode(OptimizerMode::HYBRID);
        auto hybrid_stats = RunYCSB(config);
        result.hybrid_throughput = hybrid_stats.throughput;
        result.hybrid_p95_latency = hybrid_stats.p95_latency_ms;
        
        // Calculate Improvement
        result.improvement_percent = 
            (hybrid_throughput - rule_throughput) / rule_throughput * 100.0;
        
        return result;
    }
};
```

### 8.2 Erwartete Performance-Gewinne

**Baseline (Regelbasiert):**
- OLTP Read-Heavy: 120K ops/s, P95: 5ms
- OLTP Write-Heavy: 45K ops/s, P95: 10ms
- OLAP Complex: 100 queries/s, P95: 500ms

**Mit Hybrid Approach (Konservative Schätzung):**

| Workload | Throughput Improvement | Latency Improvement |
|----------|----------------------|-------------------|
| OLTP Read-Heavy | +10-20% | -5-10% |
| OLTP Write-Heavy | +5-15% | -5-8% |
| OLAP Complex | +30-50% | -20-40% |
| Mixed | +15-25% | -10-20% |

**Optimistische Schätzung (nach 6+ Monaten Training):**

| Workload | Throughput Improvement | Latency Improvement |
|----------|----------------------|-------------------|
| OLTP Read-Heavy | +20-35% | -10-20% |
| OLTP Write-Heavy | +15-25% | -10-15% |
| OLAP Complex | +50-80% | -30-50% |
| Mixed | +25-40% | -15-30% |

---

## 9. Roadmap & Migration

### 9.1 Implementierungs-Roadmap

**Q1 2026 (Monate 1-3): Foundation & Shadow Mode**

Deliverables:
- ✅ Hybrid-Architektur Design finalisiert
- ✅ `HybridQueryOptimizer` Implementierung
- ✅ Shadow Mode aktiviert für Query Optimizer
- ✅ Monitoring Dashboard aufgesetzt
- ✅ Erste Trainingsdaten gesammelt (30 Tage)

Tasks:
```bash
# Week 1-2: Architecture & Design
- Finalize hybrid architecture
- Define interfaces for rule/ML components
- Setup development environment

# Week 3-6: Implementation
- Implement HybridQueryOptimizer
- Implement HybridDecisionEngine
- Implement FallbackCascade
- Add Circuit Breaker

# Week 7-9: Shadow Mode
- Deploy shadow mode to staging
- Collect comparison data
- Analyze ML vs Rule performance

# Week 10-12: Monitoring
- Setup Prometheus metrics
- Create Grafana dashboards
- Implement alerting rules
```

**Q2 2026 (Monate 4-6): Conservative & Balanced Modes**

Deliverables:
- ✅ ML Models trainiert (Query Optimizer v1.0)
- ✅ Conservative Mode in Production
- ✅ Balanced Mode aktiviert (50% Traffic)
- ✅ Hybrid Cache Manager implementiert
- ✅ Performance-Verbesserung: +15-20%

Tasks:
```bash
# Week 1-3: Model Training
- Collect training data from shadow mode
- Train initial ML models
- Validate model performance

# Week 4-6: Conservative Rollout
- Deploy to 1% production traffic
- Monitor for regressions
- Gradually increase to 10%

# Week 7-9: Balanced Mode
- Increase ML usage to 50%
- Implement adaptive confidence thresholds
- Fine-tune fallback logic

# Week 10-12: Cache Integration
- Implement HybridCacheManager
- Train cache ML model
- Deploy hybrid caching
```

**Q3 2026 (Monate 7-9): Full Hybrid Stack**

Deliverables:
- ✅ Hybrid Index Advisor deployed
- ✅ Hybrid Compaction Scheduler active
- ✅ Automatic model retraining pipeline
- ✅ Performance-Verbesserung: +25-35%

**Q4 2026 (Monate 10-12): Optimization & Advanced Features**

Deliverables:
- ✅ Multi-Armed Bandit für Exploration
- ✅ Transfer Learning zwischen Workloads
- ✅ Automated A/B Testing
- ✅ Performance-Verbesserung: +35-50%

### 9.2 Migration Path

**Für bestehende ThemisDB Installationen:**

```bash
# Step 1: Upgrade zu Version mit Hybrid Support
themisdb-upgrade --version 1.6.0

# Step 2: Enable Shadow Mode
themisdb-hybrid init --mode shadow

# Step 3: Collect Data (mindestens 7 Tage)
# Warten...

# Step 4: Train Initial Models
themisdb-hybrid train --all-components

# Step 5: Validate Models
themisdb-hybrid validate --benchmark

# Step 6: Enable Conservative Mode
themisdb-hybrid set-mode conservative

# Step 7: Monitor (mindestens 7 Tage)
themisdb-hybrid monitor --alerts

# Step 8: Gradually increase (wenn Performance gut)
themisdb-hybrid set-mode balanced
```

### 9.3 Rollback Plan

Falls Hybrid-Modus Probleme verursacht:

```bash
# Sofort-Rollback zu regelbasiert
themisdb-hybrid disable

# Oder nur ML deaktivieren, Hybrid-Logik behalten
themisdb-hybrid set-mode rule-only

# Oder zurück zu vorheriger ThemisDB Version
themisdb-downgrade --version 1.5.x
```

---

## Zusammenfassung

### Kernvorteile des Hybrid-Konzepts für ThemisDB

1. **Beste aus beiden Welten:**
   - Zuverlässigkeit regelbasierter Systeme
   - Adaptivität ML-basierter Optimierung

2. **Zero-Risk Adoption:**
   - Shadow Mode für sichere Evaluation
   - Conservative Mode mit hohen Confidence-Thresholds
   - Automatische Fallbacks bei ML-Fehlern

3. **Kontinuierliche Verbesserung:**
   - Feedback Loop für ständiges Lernen
   - Automatisches Retraining
   - Workload-spezifische Anpassung

4. **Production-Ready:**
   - Circuit Breaker für Fehlertoleranz
   - Umfassende Monitoring & Alerting
   - Klare Rollback-Strategien

5. **Flexibilität:**
   - Verschiedene Hybrid-Modi
   - Komponenten-spezifische Konfiguration
   - Schrittweise Aktivierung

### Empfohlener Startpunkt

Für ThemisDB empfehlen wir:

1. **Start mit Query Optimizer** (größter Impact)
2. **Shadow Mode für 30 Tage** (Datensammlung)
3. **Conservative Mode mit hohem Threshold** (95%+ Confidence)
4. **Gradual Expansion** zu anderen Komponenten
5. **Continuous Monitoring** & Adjustment

### Nächste Schritte

1. Review dieses Konzepts mit ThemisDB Core-Team
2. Proof-of-Concept Implementation (Query Optimizer)
3. Benchmark gegen aktuelle Baseline
4. Entscheidung über Production Rollout

---

**Erstellt von:** GitHub Copilot  
**Datum:** 10. Februar 2026  
**Version:** 1.0  
**Status:** 🔬 Konzeptionelles Design  
**Nächste Review:** März 2026

---

**Referenzen:**
- Siehe [ADAPTIVE_LEARNING_CORE_SELBSTOPTIMIERUNG.md](ADAPTIVE_LEARNING_CORE_SELBSTOPTIMIERUNG.md) für Details zu Einzelansätzen
- ThemisDB Architecture Documentation
- Bao Paper (SIGMOD 2021) - Practical Learned Query Optimization
- Neo Paper (VLDB 2019) - RL-based Query Optimizer
