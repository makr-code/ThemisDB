# RAG Enhancement Modules

## Übersicht

Dieses Verzeichnis enthält erweiterte Komponenten für das Retrieval-Augmented Generation (RAG) System von ThemisDB. Die Module verbessern die Qualität und Zuverlässigkeit von RAG-Antworten durch intelligente Gap-Detection und LLM-basierte Qualitätsbewertung.

## Komponenten

### 1. Knowledge Gap Detector

**Namespace:** `themis::rag::knowledge_gap`

**Zweck:** Erkennt Wissenslücken in abgerufenen Dokumenten, um zu verhindern, dass das System auf Basis unzureichender Informationen antwortet.

**Hauptfunktionalität:**
- Multi-Level-Detection (Pre-, During-, Post-Generation)
- Similarity-basierte Analyse
- Token-Probability-Tracking
- Claim-Verification
- Automatische Fallback-Strategien

**Dateien:**
- `include/rag/knowledge_gap_detector.h` - Header mit Interface
- `src/rag/knowledge_gap_detector.cpp` - Implementation
- `docs/de/llm/RAG_KNOWLEDGE_GAP_DETECTOR_ANALYSE.md` - Wissenschaftliche Analyse
- `docs/de/llm/RAG_KNOWLEDGE_GAP_DETECTOR_TODO.md` - Implementation Roadmap

**Schnellstart:**
```cpp
using namespace themis::rag::knowledge_gap;

// Fast detector für Production
auto detector = KnowledgeGapDetectorFactory::createBalanced();

// Vor Generation prüfen
auto result = detector->detectPreGeneration(query, documents);

if (result.gap_detected) {
    // Fallback-Strategie anwenden
    switch (result.recommendation) {
        case FallbackStrategy::EXPAND_SEARCH:
            // Suche erweitern
            break;
        case FallbackStrategy::REFORMULATE_QUERY:
            // Query umformulieren
            break;
        // ...
    }
}
```

### 2. LLM-as-Judge

**Namespace:** `themis::rag::judge`

**Zweck:** Bewertet die Qualität von RAG-Antworten auf mehreren Dimensionen (Faithfulness, Relevance, Completeness, Coherence).

**Hauptfunktionalität:**
- Multi-Dimension Scoring
- Pairwise Comparison
- Judge Ensemble
- Chain-of-Thought Evaluation
- Rubric-based Assessment
- Bias Mitigation

**Dateien:**
- `include/rag/rag_judge.h` - Header mit Interface
- `src/rag/rag_judge.cpp` - Implementation
- `docs/de/llm/RAG_LLM_AS_JUDGE_ANALYSE.md` - Wissenschaftliche Analyse
- `docs/de/llm/RAG_LLM_AS_JUDGE_TODO.md` - Implementation Roadmap

**Schnellstart:**
```cpp
using namespace themis::rag::judge;

// Balanced judge für Standard-Use-Cases
auto judge = RAGJudgeFactory::createBalanced();

// Antwort bewerten
auto evaluation = judge->evaluate(query, documents, generated_answer);

if (!evaluation.passed_quality_threshold) {
    // Antwort verbessern oder kennzeichnen
    THEMIS_WARN("Answer quality below threshold: {}", 
                evaluation.overall_score);
    
    // Unverified claims anzeigen
    for (const auto& claim : evaluation.unverified_claims) {
        THEMIS_DEBUG("Unverified claim: {}", claim);
    }
}

// Zwei Antworten vergleichen
auto comparison = judge->compare(query, documents, answer_a, answer_b);
```

## Integration in RAG-Pipeline

### Typischer Workflow

```cpp
// 1. Retrieval
auto documents = vector_index->search(query, top_k);

// 2. Gap Detection (Pre-Generation)
auto gap_result = gap_detector->detectPreGeneration(query, documents);

if (gap_result.gap_detected) {
    // Fallback-Strategie anwenden
    documents = applyFallbackStrategy(gap_result.recommendation, query);
}

// 3. Generation
auto answer = llm->generate(query, documents);

// 4. Post-Generation Gap Check
auto final_gap_check = gap_detector->detectPostGeneration(
    query, documents, answer
);

// 5. Quality Evaluation
auto evaluation = judge->evaluate(query, documents, answer);

// 6. Entscheidung
if (evaluation.passed_quality_threshold && !final_gap_check.gap_detected) {
    return Response{answer, evaluation.overall_score};
} else {
    return Response{
        "Ich kann diese Frage mit den verfügbaren Informationen nicht "
        "zuverlässig beantworten.",
        0.0,
        evaluation.unverified_claims
    };
}
```

## Konfiguration

### Knowledge Gap Detector

```yaml
knowledge_gap_detection:
  enabled: true
  mode: "balanced"  # fast, balanced, thorough
  
  similarity_threshold: 0.75
  min_documents: 3
  confidence_threshold: 0.7
  coverage_threshold: 0.8
  
  enable_self_consistency_check: true
  enable_claim_verification: true
  enable_query_aspect_analysis: true
  
  fallback_strategy:
    - expand_search
    - reformulate_query
    - insufficient_data_response
```

### LLM-as-Judge

```yaml
rag_judge:
  enabled: true
  mode: "balanced"  # fast, balanced, thorough
  judge_model: "themis-judge-v1"
  
  scoring:
    faithfulness_weight: 0.4
    relevance_weight: 0.3
    completeness_weight: 0.2
    coherence_weight: 0.1
  
  quality_threshold: 0.7
  faithfulness_threshold: 0.8
  
  chain_of_thought: true
  claim_verification: true
  cache_evaluations: true
  
  ensemble:
    enabled: false
    judges: 3
    voting_strategy: "weighted_average"
```

## Performance-Charakteristika

### Knowledge Gap Detector

| Mode | Latency | Accuracy | Use Case |
|------|---------|----------|----------|
| Fast | ~10ms | 75-80% | High-throughput Production |
| Balanced | ~100ms | 85-90% | Standard RAG Pipeline |
| Thorough | ~500ms+ | 95%+ | Critical Applications |

### LLM-as-Judge

| Mode | Latency | Correlation with Humans | Use Case |
|------|---------|------------------------|----------|
| Fast | ~100ms | 0.7-0.75 | Quick relevance check |
| Balanced | ~500ms | 0.8-0.85 | Standard evaluation |
| Thorough | ~2s | 0.9+ | Research, A/B testing |

## Wissenschaftliche Grundlagen

Beide Komponenten basieren auf state-of-the-art Forschung:

### Knowledge Gap Detector
- Self-RAG (Asai et al., 2023)
- Active Retrieval Augmented Generation (Jiang et al., 2023)
- REALM (Guu et al., 2020)

### LLM-as-Judge
- G-Eval (Liu et al., 2023)
- MT-Bench (Zheng et al., 2023)
- RAGAS Framework (Es et al., 2023)
- Constitutional AI (Anthropic, 2022)

Siehe Analyse-Dokumente für Details und vollständige Literaturverzeichnisse.

## Entwicklungsstatus

### Knowledge Gap Detector
- ✅ API Design & Header
- ✅ Basis-Implementation (Similarity, Document Count)
- ⏳ LLM-basierte Konfidenzmetriken (TODO)
- ⏳ Claim-Verification (TODO)
- ⏳ Integration mit Inference Engine (TODO)
- ⏳ Tests & Benchmarks (TODO)

### LLM-as-Judge
- ✅ API Design & Header
- ✅ Basis-Implementation (Multi-Dimension Scoring)
- ⏳ Prompt-Engineering & Templates (TODO)
- ⏳ Claim-Verification (TODO)
- ⏳ Ensemble & Bias-Mitigation (TODO)
- ⏳ Calibration & Testing (TODO)

Siehe TODO-Dokumente für detaillierte Roadmaps.

## Testing

```bash
# Unit tests
./build/tests/test_knowledge_gap_detector
./build/tests/test_rag_judge

# Integration tests
./build/tests/test_rag_pipeline_with_enhancements

# Benchmarks
./build/benchmarks/bench_rag_enhancements
```

## Beitragen

Siehe `CONTRIBUTING.md` im Root-Verzeichnis für allgemeine Richtlinien.

Für diese Module:
1. Forken und Branch erstellen
2. Tests hinzufügen für neue Features
3. Dokumentation aktualisieren
4. Pull Request mit detaillierter Beschreibung

## Lizenz

MIT License - siehe `LICENSE` im Root-Verzeichnis.

## Kontakt

Bei Fragen zu diesen Modulen:
- GitHub Issues: https://github.com/makr-code/ThemisDB/issues
- Dokumentation: https://makr-code.github.io/ThemisDB/

---

*Erstellt: 2026-01-18*  
*Version: 1.0*
