# Synergien: Ethics/Moral Modul & RAG Enhancement (Gap Detector + LLM-as-Judge)

## Zusammenfassung

Das bestehende **Ethical Guidelines Manager** Modul und die neuen **RAG Enhancement** Komponenten (Knowledge Gap Detector und LLM-as-Judge) haben erhebliche Synergien und Paralellen. Eine Integration würde die Qualität und Sicherheit beider Systeme deutlich verbessern.

## Entdeckte Parallelen

### 1. LLM-as-Judge Pattern

**Ethical Guidelines Manager:**
- Hat bereits `detectWithLLMJudge()` Methode (Zeile 132-146)
- Implementiert "LLM-as-ethical-judge" Pattern
- Verwendet LLM zur Erkennung ethischer Implikationen
- Config: `use_llm_as_judge`, `llm_judge_threshold` (Zeile 87-89, 544-546)

**RAG Judge:**
- Implementiert allgemeines "LLM-as-judge" Pattern
- Bewertet RAG-Antworten auf Faithfulness, Relevance, etc.
- Verwendet LLM für strukturierte Evaluation

**Synergie:** Beide nutzen das gleiche Grundpattern - LLM zur Meta-Analyse von Texten.

### 2. Detection & Confidence Scoring

**Ethical Guidelines Manager:**
```cpp
struct DetectionResult {
    bool has_ethical_context = false;
    std::vector<std::string> detected_keywords;
    float confidence = 0.0f;
    bool used_llm_judge = false;
    std::string llm_reasoning = "";
    float llm_confidence = 0.0f;
}
```

**Knowledge Gap Detector:**
```cpp
struct DetectionResult {
    bool gap_detected;
    double confidence_score;
    GapType gap_type;
    std::vector<std::string> missing_aspects;
    std::string explanation;
}
```

**Synergie:** Beide verwenden ähnliche Detection-Result-Strukturen mit Confidence-Scores.

### 3. Multi-Level Detection

**Ethical Guidelines Manager:**
- Keyword-basierte Detection
- LLM-basierte Detection (optional)
- Domain-spezifische Detection
- Combination Strategy (`combine_with_keywords`)

**Knowledge Gap Detector:**
- Level 1: Pre-generation (Similarity, Document Count)
- Level 2: During generation (Token Probabilities)
- Level 3: Post-generation (Claim Verification)
- Multi-level combination

**Synergie:** Beide nutzen mehrschichtige Detection-Strategien.

### 4. Context-Aware Analysis

**Ethical Guidelines Manager:**
```cpp
DetectionResult detectEthicalContextInRAG(
    const std::vector<std::string>& documents,
    const std::string& query,
    const std::vector<std::string>& conversation_history
);
```

**RAG Enhancement:**
- Analysiert Query + Documents + Generated Answer
- Berücksichtigt Retrieval-Kontext
- Multi-dimensional assessment

**Synergie:** Beide analysieren RAG-spezifische Kontexte.

## Kritische Synergien & Integration-Punkte

### 1. Ethical Gaps als spezielle Knowledge Gaps

**Problem:** 
Wenn das Ethics-Modul ethische Implikationen erkennt, aber die abgerufenen Dokumente keine ethische Vielfalt bieten, entsteht eine **ethische Wissenslücke**.

**Lösung - Neue Gap-Kategorie:**
```cpp
// In knowledge_gap_detector.h
enum class GapType {
    LOW_SIMILARITY,
    INSUFFICIENT_DOCS,
    UNCERTAIN_GENERATION,
    MISSING_ASPECTS,
    CONFLICTING_INFO,
    OUTDATED_INFO,
    ETHICAL_PERSPECTIVE_GAP,  // NEU!
    NONE
};
```

**Integration:**
```cpp
// In knowledge_gap_detector.cpp
DetectionResult KnowledgeGapDetector::detectEthicalPerspectiveGap(
    const std::string& query,
    const std::vector<RetrievedDocument>& documents,
    const themis::llm::EthicalGuidelinesManager& ethics_manager
) {
    // 1. Prüfe ob ethischer Kontext vorliegt
    auto ethical_result = ethics_manager.detectEthicalContext(query);
    
    if (!ethical_result.has_ethical_context) {
        return DetectionResult{false, 0.0, GapType::NONE};
    }
    
    // 2. Prüfe ob Dokumente diverse ethische Perspektiven abdecken
    std::vector<std::string> perspectives = {
        "utilitaristisch", "deontologisch", "tugendethisch",
        "religiös", "kulturrelativ"
    };
    
    int perspectives_found = 0;
    for (const auto& doc : documents) {
        for (const auto& perspective : perspectives) {
            if (containsPerspective(doc.content, perspective)) {
                perspectives_found++;
                break;
            }
        }
    }
    
    // 3. Gap wenn < 2 Perspektiven in Dokumenten
    if (perspectives_found < 2) {
        DetectionResult result;
        result.gap_detected = true;
        result.gap_type = GapType::ETHICAL_PERSPECTIVE_GAP;
        result.confidence_score = ethical_result.confidence;
        result.recommendation = FallbackStrategy::EXPAND_SEARCH;
        result.explanation = "Ethischer Kontext erkannt, aber unzureichende "
                           "Perspektivenvielfalt in abgerufenen Dokumenten";
        result.missing_aspects = {"Diverse moralphilosophische Perspektiven"};
        return result;
    }
    
    return DetectionResult{false, 0.0, GapType::NONE};
}
```

### 2. Ethics-Aware LLM-as-Judge

**Problem:**
Der RAG Judge sollte auch bewerten, ob ethische Richtlinien eingehalten wurden.

**Lösung - Neue Evaluation-Dimension:**
```cpp
// In rag_judge.h
enum class EvaluationDimension {
    FAITHFULNESS,
    RELEVANCE,
    COMPLETENESS,
    COHERENCE,
    ETHICAL_COMPLIANCE,  // NEU!
    OVERALL
};

struct EvaluationResult {
    // ... existing fields ...
    double ethical_compliance_score;  // NEU!
    std::vector<std::string> ethical_violations;  // NEU!
    bool respects_human_autonomy;  // NEU!
    bool shows_moral_diversity;    // NEU!
};
```

**Integration:**
```cpp
// In rag_judge.cpp
double RAGJudge::evaluateEthicalCompliance(
    const EvaluationInput& input,
    const themis::llm::EthicalGuidelinesManager& ethics_manager
) {
    auto ethical_detection = ethics_manager.detectEthicalContext(input.query);
    
    if (!ethical_detection.has_ethical_context) {
        return 1.0;  // Keine ethischen Aspekte, daher OK
    }
    
    double score = 1.0;
    
    // Check 1: Respektiert die Antwort menschliche Autonomie?
    if (containsPatronizing(input.generated_answer)) {
        score -= 0.3;
        result.ethical_violations.push_back("Bevormundende Sprache");
    }
    
    // Check 2: Zeigt die Antwort moralische Vielfalt?
    if (ethical_detection.detected_domains.contains("moral_imperatives")) {
        int perspectives = countMoralPerspectives(input.generated_answer);
        if (perspectives < 2) {
            score -= 0.3;
            result.ethical_violations.push_back(
                "Unzureichende Darstellung moralischer Vielfalt"
            );
        }
    }
    
    // Check 3: Sind Quellen genannt bei ethischen Claims?
    auto claims = extractClaims(input.generated_answer);
    for (const auto& claim : claims) {
        if (isEthicalClaim(claim) && !hasCitation(claim)) {
            score -= 0.2;
            result.ethical_violations.push_back(
                "Ethischer Claim ohne Quellenangabe"
            );
        }
    }
    
    return std::max(0.0, score);
}
```

### 3. Unified Confidence & Threshold System

**Aktuell:**
- Ethics Manager: `detection_threshold = 0.6`, `llm_judge_threshold = 0.7`
- Gap Detector: `confidence_threshold = 0.7`, `similarity_threshold = 0.75`
- RAG Judge: `quality_threshold = 0.7`, `faithfulness_threshold = 0.8`

**Problem:** Inkonsistente Thresholds können zu widersprüchlichen Entscheidungen führen.

**Lösung - Unified Configuration:**
```yaml
# config/rag_quality_config.yaml
rag_quality_system:
  # Global thresholds
  confidence_thresholds:
    low: 0.6      # Warning level
    medium: 0.7   # Standard threshold
    high: 0.8     # Critical contexts
    
  # Component-specific overrides
  knowledge_gap_detector:
    similarity_threshold: 0.75
    min_documents: 3
    use_global_confidence: true
    
  llm_as_judge:
    quality_threshold_mode: "medium"  # Uses 0.7
    faithfulness_critical: true       # Uses high threshold (0.8)
    
  ethical_guidelines:
    detection_threshold_mode: "low"   # Uses 0.6 (more sensitive)
    llm_judge_threshold_mode: "medium"
    
  # Integration rules
  integration:
    # Wenn Ethics-Modul ethischen Kontext mit > 0.7 erkennt,
    # erhöhe Judge quality_threshold auf "high"
    ethical_context_raises_threshold: true
    
    # Wenn Gap-Detector ETHICAL_PERSPECTIVE_GAP findet,
    # triggere erweiterte Suche
    ethical_gap_triggers_expansion: true
```

### 4. Shared LLM-as-Judge Infrastructure

**Problem:** 
Beide Systeme nutzen LLM für Meta-Analyse, aber duplicate Code.

**Lösung - Shared Base Class:**
```cpp
// In include/rag/llm_meta_analyzer.h
namespace themis::rag {

/**
 * @brief Base class for LLM-based meta-analysis
 * 
 * Provides shared infrastructure for:
 * - Ethics Judge (detect ethical implications)
 * - Quality Judge (evaluate RAG quality)
 * - Gap Judge (detect knowledge gaps)
 */
class LLMMetaAnalyzer {
public:
    struct AnalysisResult {
        bool detection_positive;
        double confidence;
        std::string reasoning;
        std::unordered_map<std::string, double> dimension_scores;
    };
    
    struct AnalysisConfig {
        std::string judge_model;
        bool use_chain_of_thought;
        int max_retries;
        double min_confidence;
    };
    
protected:
    // Shared prompt engineering
    std::string buildPrompt(
        const std::string& task_description,
        const std::string& input_text,
        const std::vector<std::string>& criteria
    );
    
    // Shared response parsing
    AnalysisResult parseResponse(const std::string& llm_response);
    
    // Shared LLM inference
    std::string callLLM(const std::string& prompt);
};

// Spezialisierungen
class EthicalContextAnalyzer : public LLMMetaAnalyzer { ... };
class QualityJudge : public LLMMetaAnalyzer { ... };
class GapAnalyzer : public LLMMetaAnalyzer { ... };

} // namespace themis::rag
```

## Implementation TODO

### Phase 1: Ethics-Gap Integration (2 Wochen)

- [ ] `ETHICAL_PERSPECTIVE_GAP` zu `GapType` enum hinzufügen
- [ ] `detectEthicalPerspectiveGap()` implementieren
- [ ] Ethics Manager in Gap Detector integrieren
- [ ] Tests für ethical gap detection
- [ ] Fallback-Strategie für ethical gaps
  - [ ] Erweiterte Suche mit "diverse perspectives" Query
  - [ ] Explizite Warnung "Ethisch sensibel, bitte konsultieren Sie Experten"

### Phase 2: Ethics-Aware Judge (2 Wochen)

- [ ] `ETHICAL_COMPLIANCE` Dimension hinzufügen
- [ ] `evaluateEthicalCompliance()` implementieren
- [ ] Patronizing-Language-Detection
- [ ] Moral-Diversity-Checker
- [ ] Citation-Checker für ethische Claims
- [ ] Integration mit Ethics Manager
- [ ] Tests für ethical compliance evaluation

### Phase 3: Unified Configuration (1 Woche)

- [ ] Unified Config-Schema entwerfen
- [ ] Config-Loader implementieren
- [ ] Threshold-Mapping für alle Komponenten
- [ ] Config-Validation
- [ ] Migration-Guide für bestehende Configs
- [ ] Tests für Config-System

### Phase 4: Shared Infrastructure (2 Wochen)

- [ ] `LLMMetaAnalyzer` Base Class implementieren
- [ ] Prompt-Engineering-Utilities
- [ ] Response-Parsing-Utilities
- [ ] LLM-Caching-Layer
- [ ] Refactoring: Ethics Manager nutzt Base Class
- [ ] Refactoring: RAG Judge nutzt Base Class
- [ ] Refactoring: Gap Detector nutzt Base Class
- [ ] Performance-Tests

### Phase 5: End-to-End Integration (1 Woche)

- [ ] Integrated RAG Pipeline mit allen Komponenten
- [ ] Ethics-aware Retrieval
- [ ] Ethics-aware Generation
- [ ] Ethics-aware Evaluation
- [ ] Monitoring & Metrics
- [ ] Documentation

**Gesamt: ~8 Wochen**

## Architektur-Diagramm: Integriertes System

```
┌─────────────────────────────────────────────────────────────┐
│                       User Query                             │
└─────────────────┬───────────────────────────────────────────┘
                  │
                  ▼
┌─────────────────────────────────────────────────────────────┐
│              Ethics Context Detection                        │
│         (EthicalGuidelinesManager)                          │
│  • Keyword-based detection                                  │
│  • LLM-as-ethical-judge                                     │
│  • Domain classification                                     │
└─────────────────┬───────────────────────────────────────────┘
                  │
                  ├─── Ethical Context? ───┐
                  │                        │
                  ▼                        ▼
         ┌────────────────┐      ┌───────────────────────┐
         │  Standard RAG  │      │  Ethics-Aware RAG     │
         └────────┬───────┘      └─────────┬─────────────┘
                  │                        │
                  │                        │
                  ▼                        ▼
┌─────────────────────────────────────────────────────────────┐
│                   Vector Search                              │
└─────────────────┬───────────────────────────────────────────┘
                  │
                  ▼
┌─────────────────────────────────────────────────────────────┐
│           Knowledge Gap Detection                            │
│  • Pre-generation: Similarity, Doc Count                    │
│  • Ethical Perspective Gap (NEW!)                           │
│  • Missing Aspect Detection                                 │
└─────────────────┬───────────────────────────────────────────┘
                  │
                  ├─── Gap Detected? ───┐
                  │                     │
         No Gap   │            Yes Gap  │
                  ▼                     ▼
         ┌────────────────┐    ┌──────────────────┐
         │   Generate     │    │  Apply Fallback  │
         └────────┬───────┘    │  • Expand Search │
                  │             │  • Reformulate   │
                  │             │  • Add Ethics    │
                  │             │    Perspectives  │
                  │             └────────┬─────────┘
                  │                      │
                  │◄─────────────────────┘
                  │
                  ▼
┌─────────────────────────────────────────────────────────────┐
│              LLM Generation                                  │
│  • Prompt augmented with ethics guidelines                  │
│  • Token probability monitoring                             │
└─────────────────┬───────────────────────────────────────────┘
                  │
                  ▼
┌─────────────────────────────────────────────────────────────┐
│           LLM-as-Judge Evaluation                           │
│  • Faithfulness                                             │
│  • Relevance                                                │
│  • Completeness                                             │
│  • Coherence                                                │
│  • Ethical Compliance (NEW!)                                │
│    - Respects autonomy?                                     │
│    - Shows moral diversity?                                 │
│    - Has proper citations?                                  │
└─────────────────┬───────────────────────────────────────────┘
                  │
                  ├─── Pass Quality Threshold? ───┐
                  │                                │
           Pass   │                         Fail   │
                  ▼                                ▼
         ┌────────────────┐              ┌──────────────────┐
         │ Return Answer  │              │  Reject/Revise   │
         │ + Disclaimer   │              │  or Flag Issue   │
         └────────────────┘              └──────────────────┘
```

## Nutzen der Integration

### 1. Verbesserte Ethik-Compliance

**Vorher:**
- Ethics Manager erkennt ethischen Kontext
- Aber keine Garantie, dass Dokumente diverse Perspektiven haben
- Antwort könnte einseitig sein

**Nachher:**
- Gap Detector erkennt fehlende ethische Perspektiven
- Automatische Erweiterung der Suche
- Judge bewertet ethische Compliance der Antwort
- **Resultat:** Garantiert diverse moralphilosophische Perspektiven

### 2. Höhere Antwortqualität bei ethischen Fragen

**Vorher:**
- RAG könnte gute Faithfulness haben
- Aber gegen ethische Richtlinien verstoßen (z.B. bevormundend)

**Nachher:**
- Judge bewertet auch ethical compliance
- Antworten müssen BEIDE Kriterien erfüllen:
  - Faktentreu (Faithfulness)
  - Ethisch korrekt (Autonomy, Diversity)

### 3. Präventive Gap-Erkennung

**Vorher:**
- Gap Detector findet fehlende Informationen
- Aber nicht spezifisch für ethische Perspektiven

**Nachher:**
- Spezialisierte Erkennung ethischer Perspektiven-Gaps
- Proaktive Erweiterung der Suche
- Vermeidung einseitiger ethischer Darstellungen

### 4. Konsistente Qualitätsstandards

**Vorher:**
- Verschiedene Thresholds in verschiedenen Modulen
- Inkonsistente Entscheidungen möglich

**Nachher:**
- Unified threshold system
- Context-aware threshold adjustment
- Konsistente Quality Gates

## Empfehlung

**Priorität: HOCH**

Die Integration sollte **parallel** zur Basis-Implementation von Gap Detector und Judge erfolgen:

1. **Sofort:** Unified Config-System designen
2. **Phase 1 (parallel):** Ethics-Gap-Detection während Gap Detector Impl
3. **Phase 2 (parallel):** Ethics-Compliance-Dimension während Judge Impl
4. **Phase 3-4:** Shared Infrastructure & Refactoring
5. **Phase 5:** End-to-End Integration & Testing

**Grund:** Die Synergien sind so erheblich, dass ein späteres Refactoring viel aufwändiger wäre als eine integrierte Entwicklung von Anfang an.

---

*Erstellt: 2026-01-18*  
*Version: 1.0*  
*Status: Design & Planning*
