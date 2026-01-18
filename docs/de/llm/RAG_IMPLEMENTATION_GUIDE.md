# RAG Enhancements - Implementation Guide

## Übersicht

Diese Guide bietet einen praktischen Einstieg in die Nutzung und Weiterentwicklung der neuen RAG-Enhancement-Komponenten in ThemisDB.

## Was wurde implementiert?

### 1. Knowledge Gap Detector

**Namespace:** `themis::rag::knowledge_gap`

**Zweck:** Erkennt, wann abgerufene Dokumente nicht ausreichend sind, um eine Benutzeranfrage zuverlässig zu beantworten.

**Status:**
- ✅ API-Design komplett
- ✅ Basis-Implementierung (Similarity, Document Count)
- ⏳ Erweiterte Features (LLM-Konfidenz, Claim-Verification) - siehe TODO

### 2. LLM-as-Judge

**Namespace:** `themis::rag::judge`

**Zweck:** Bewertet die Qualität von RAG-Antworten auf mehreren Dimensionen.

**Status:**
- ✅ API-Design komplett
- ✅ Framework-Implementierung (Scoring, Ensemble, Caching)
- ⏳ LLM-Integration und Prompts - siehe TODO

## Quick Start

### Knowledge Gap Detector verwenden

```cpp
#include "llm/rag/knowledge_gap_detector.h"

using namespace themis::rag::knowledge_gap;

// 1. Detector erstellen
auto detector = KnowledgeGapDetectorFactory::createBalanced();

// 2. Vor der Generation prüfen
std::vector<RetrievedDocument> documents = /* ... von Vector Search ... */;
std::string query = "Was ist die Hauptstadt von Frankreich?";

auto result = detector->detectPreGeneration(query, documents);

if (result.gap_detected) {
    std::cout << "Gap detected: " << result.explanation << std::endl;
    std::cout << "Recommendation: " << static_cast<int>(result.recommendation) << std::endl;
    
    // Fallback-Strategie anwenden
    switch (result.recommendation) {
        case FallbackStrategy::EXPAND_SEARCH:
            // Suche mit relaxierten Constraints erweitern
            break;
        case FallbackStrategy::REFORMULATE_QUERY:
            // Query umformulieren und erneut suchen
            break;
        case FallbackStrategy::INSUFFICIENT_DATA_RESPONSE:
            // Explizit "nicht genug Informationen" zurückgeben
            break;
    }
}
```

### LLM-as-Judge verwenden

```cpp
#include "llm/rag/rag_judge.h"

using namespace themis::rag::judge;

// 1. Judge erstellen
auto judge = RAGJudgeFactory::createBalanced();

// 2. Antwort bewerten
std::string query = "Was ist die Hauptstadt von Frankreich?";
std::vector<RetrievedDocument> documents = /* ... */;
std::string answer = "Die Hauptstadt von Frankreich ist Paris.";

auto evaluation = judge->evaluate(query, documents, answer);

std::cout << "Overall Score: " << evaluation.overall_score << std::endl;
std::cout << "Faithfulness: " << evaluation.faithfulness_score << std::endl;
std::cout << "Relevance: " << evaluation.relevance_score << std::endl;
std::cout << "Passed Threshold: " << evaluation.passed_quality_threshold << std::endl;

if (!evaluation.passed_quality_threshold) {
    std::cout << "Unverified claims:" << std::endl;
    for (const auto& claim : evaluation.unverified_claims) {
        std::cout << "  - " << claim << std::endl;
    }
}

// 3. Zwei Antworten vergleichen
std::string answer_a = "Paris ist die Hauptstadt.";
std::string answer_b = "Die Hauptstadt von Frankreich ist Paris, eine Stadt mit reicher Geschichte.";

auto comparison = judge->compare(query, documents, answer_a, answer_b);

switch (comparison.winner) {
    case ComparisonResult::Winner::ANSWER_A:
        std::cout << "Answer A is better" << std::endl;
        break;
    case ComparisonResult::Winner::ANSWER_B:
        std::cout << "Answer B is better" << std::endl;
        break;
    case ComparisonResult::Winner::TIE:
        std::cout << "Both answers are equally good" << std::endl;
        break;
}
```

### Integration in RAG-Pipeline

```cpp
#include "llm/rag/knowledge_gap_detector.h"
#include "llm/rag/rag_judge.h"

// Setup
auto gap_detector = KnowledgeGapDetectorFactory::createBalanced();
auto judge = RAGJudgeFactory::createBalanced();

// RAG Pipeline
std::string processQuery(const std::string& query) {
    // 1. Retrieval
    auto documents = vectorIndex->search(query, 10);
    
    // 2. Gap Detection (Pre-Generation)
    auto gap_result = gap_detector->detectPreGeneration(query, documents);
    
    if (gap_result.gap_detected && 
        gap_result.confidence_score > 0.8) {
        // Fallback anwenden
        if (gap_result.recommendation == FallbackStrategy::EXPAND_SEARCH) {
            documents = vectorIndex->search(query, 20); // Mehr Dokumente
        } else if (gap_result.recommendation == FallbackStrategy::INSUFFICIENT_DATA_RESPONSE) {
            return "Ich habe nicht genügend Informationen, um diese Frage zuverlässig zu beantworten.";
        }
    }
    
    // 3. Generation
    std::string answer = llm->generate(query, documents);
    
    // 4. Post-Generation Gap Check
    auto final_gap = gap_detector->detectPostGeneration(query, documents, answer);
    
    // 5. Quality Evaluation
    auto eval = judge->evaluate(query, documents, answer);
    
    // 6. Entscheidung
    if (eval.passed_quality_threshold && !final_gap.gap_detected) {
        return answer;
    } else if (eval.faithfulness_score < 0.7) {
        return "Ich bin mir nicht sicher, ob diese Antwort vollständig korrekt ist: " + answer;
    } else {
        return "Ich kann diese Frage nicht zuverlässig beantworten.";
    }
}
```

## Konfiguration

### Über Config-Struct

```cpp
// Knowledge Gap Detector konfigurieren
KnowledgeGapConfig config;
config.mode = DetectionMode::THOROUGH;
config.similarity_threshold = 0.8;
config.min_documents = 5;
config.enable_claim_verification = true;
config.enable_self_consistency_check = true;

auto detector = std::make_unique<KnowledgeGapDetector>(config);

// LLM-as-Judge konfigurieren
RAGJudgeConfig judge_config;
judge_config.mode = EvaluationMode::THOROUGH;
judge_config.faithfulness_weight = 0.5;  // Höhere Gewichtung für Faktentreue
judge_config.quality_threshold = 0.75;
judge_config.enable_claim_verification = true;

auto judge = std::make_unique<RAGJudge>(judge_config);
```

### Callbacks für Monitoring

```cpp
// Gap Detection Callback
detector->setGapDetectionCallback([](const DetectionResult& result) {
    if (result.gap_detected) {
        THEMIS_WARN("Knowledge gap detected: {}", result.explanation);
        // Metrics aktualisieren
        metrics_->incrementCounter("knowledge_gaps_detected");
        metrics_->recordValue("gap_confidence", result.confidence_score);
    }
});

// Evaluation Callback
judge->setEvaluationCallback([](const EvaluationResult& result) {
    metrics_->recordValue("evaluation_score", result.overall_score);
    metrics_->recordValue("faithfulness", result.faithfulness_score);
    
    if (!result.passed_quality_threshold) {
        THEMIS_WARN("Answer quality below threshold: {}", result.overall_score);
    }
});
```

## Testing

### Unit Tests schreiben

```cpp
#include <gtest/gtest.h>
#include "llm/rag/knowledge_gap_detector.h"

TEST(KnowledgeGapDetectorTest, DetectsLowSimilarity) {
    KnowledgeGapConfig config;
    config.similarity_threshold = 0.8;
    
    KnowledgeGapDetector detector(config);
    
    std::vector<RetrievedDocument> docs = {
        {"doc1", "Content about Berlin", 0.5},
        {"doc2", "Content about Hamburg", 0.6}
    };
    
    auto result = detector.detectPreGeneration(
        "What is the capital of France?", 
        docs
    );
    
    EXPECT_TRUE(result.gap_detected);
    EXPECT_EQ(result.gap_type, GapType::LOW_SIMILARITY);
    EXPECT_LT(result.avg_similarity_score, 0.8);
}

TEST(KnowledgeGapDetectorTest, PassesGoodDocuments) {
    KnowledgeGapDetector detector;
    
    std::vector<RetrievedDocument> docs = {
        {"doc1", "Paris is the capital of France", 0.95},
        {"doc2", "The French capital Paris", 0.92},
        {"doc3", "France's capital city is Paris", 0.90}
    };
    
    auto result = detector.detectPreGeneration(
        "What is the capital of France?", 
        docs
    );
    
    EXPECT_FALSE(result.gap_detected);
    EXPECT_EQ(result.gap_type, GapType::NONE);
}
```

### Integration Tests

```cpp
TEST(RAGIntegrationTest, EndToEndPipeline) {
    // Setup
    auto gap_detector = KnowledgeGapDetectorFactory::createBalanced();
    auto judge = RAGJudgeFactory::createBalanced();
    
    // Test-Daten
    std::string query = "What is ThemisDB?";
    std::vector<RetrievedDocument> docs = loadTestDocuments();
    
    // 1. Gap Detection
    auto gap = gap_detector->detectPreGeneration(query, docs);
    ASSERT_FALSE(gap.gap_detected);
    
    // 2. Generation (Mock)
    std::string answer = "ThemisDB is a multi-model database.";
    
    // 3. Evaluation
    auto eval = judge->evaluate(query, docs, answer);
    EXPECT_GT(eval.overall_score, 0.7);
    EXPECT_TRUE(eval.passed_quality_threshold);
}
```

## Nächste Schritte für Entwickler

### Phase 1: LLM-Integration (Priorität: HOCH)

Beide Komponenten benötigen LLM-Integration für volle Funktionalität:

**Knowledge Gap Detector:**
1. Token-Probability-Tracking implementieren
2. Self-Consistency-Check mit LLM
3. Claim-Extraction und Verification

**LLM-as-Judge:**
1. Prompt-Templates erstellen
2. LLM-Inferenz für Scoring
3. Response-Parsing implementieren

**Integration-Punkt:**
```cpp
// In inference_engine_enhanced.cpp
class InferenceEngineEnhanced {
    // Neuer Hook für Token-Probabilities
    void registerTokenProbabilityCallback(
        std::function<void(const std::vector<double>&)> callback
    );
    
    // Für Judge-Integration
    std::string evaluateWithPrompt(const std::string& prompt);
};
```

### Phase 2: Testing & Validation (Priorität: HOCH)

1. **Test-Dataset erstellen**
   - Ground-Truth-Annotationen sammeln
   - Diverse Query-Typen abdecken
   - Edge-Cases dokumentieren

2. **Benchmarks entwickeln**
   - Latency-Tests
   - Accuracy-Tests (Precision/Recall/F1)
   - Comparison mit Baseline

3. **Metrics implementieren**
   - Prometheus-Metriken exportieren
   - Grafana-Dashboards erstellen
   - Alert-Rules definieren

### Phase 3: Performance-Optimierung (Priorität: MITTEL)

1. **Caching optimieren**
   - Cache-Warming-Strategien
   - TTL-Tuning
   - Hit-Rate-Monitoring

2. **GPU-Acceleration**
   - Batch-Inferenz
   - CUDA-Kernels für Similarity
   - Parallel Claim-Verification

3. **Async-Processing**
   - Thread-Pool-basierte Verarbeitung
   - Queue-basiertes Processing
   - Progressive Results

### Phase 4: Production Deployment (Priorität: MITTEL)

1. **Configuration-Management**
   - YAML-Config-Loader
   - Environment-spezifische Configs
   - Feature-Flags

2. **Monitoring & Alerting**
   - Dashboards einrichten
   - Anomaly-Detection
   - On-Call-Runbooks

3. **Documentation**
   - User-Guide vervollständigen
   - API-Docs finalisieren
   - Troubleshooting-Guide

## Hilfreiche Ressourcen

### Code-Beispiele
- `src/rag/README.md` - Übersicht und Quick Start
- Unit-Test-Templates (siehe oben)
- Integration-Beispiele (siehe oben)

### Wissenschaftliche Grundlagen
- `docs/de/llm/RAG_KNOWLEDGE_GAP_DETECTOR_ANALYSE.md` - Theorie & Papers
- `docs/de/llm/RAG_LLM_AS_JUDGE_ANALYSE.md` - Evaluation-Methoden

### Implementation Roadmaps
- `docs/de/llm/RAG_KNOWLEDGE_GAP_DETECTOR_TODO.md` - 7 Phasen
- `docs/de/llm/RAG_LLM_AS_JUDGE_TODO.md` - 8 Phasen

### Executive Summary
- `docs/de/llm/RAG_ENHANCEMENTS_SUMMARY.md` - Gesamtübersicht

## Kontakt & Support

Bei Fragen oder Problemen:
1. GitHub Issues: https://github.com/makr-code/ThemisDB/issues
2. Dokumentation: https://makr-code.github.io/ThemisDB/
3. Code-Reviews via Pull Requests

## Contributing

Beiträge sind willkommen! Siehe:
- `CONTRIBUTING.md` für allgemeine Guidelines
- TODO-Dokumente für offene Tasks
- GitHub Issues für geplante Features

---

*Erstellt: 2026-01-18*  
*Version: 1.0*  
*Next Review: Bei Completion von Phase 1 (LLM-Integration)*
