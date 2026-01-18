---
name: RAG Knowledge Gap Detector - Phase 2
about: LLM-basierte Konfidenzmetriken (3-4 Wochen)
title: '[RAG-GAP-P2] Knowledge Gap Detector - Phase 2: LLM-basierte Konfidenz'
labels: 'priority:P1, type:feature, area:llm, effort:x-large, phase:2'
assignees: ''
---

## 📋 Übersicht

Implementation von LLM-basierten Konfidenzmetriken für präzisere Knowledge-Gap-Detection.

**Namespace:** `themis::rag::knowledge_gap`  
**Dokumentation:** `docs/de/llm/RAG_KNOWLEDGE_GAP_DETECTOR_TODO.md` (Phase 2)  
**Voraussetzung:** Phase 1 muss abgeschlossen sein

## 🎯 Ziele dieser Phase

- ✅ Token-Probability Tracking während LLM-Generation
- ✅ Perplexity-Berechnung und Anomalie-Erkennung
- ✅ Self-Consistency Check via Multiple Sampling
- ✅ FLARE-Style Active Retrieval
- ✅ Integration Tests mit realen LLM-Modellen

## 📦 Arbeitspakete

### 2.1 Token-Probability Tracking (5-6 Tage)

- [ ] Integration mit inference_engine_enhanced.cpp
  - [ ] Hook für Token-Probability-Callbacks implementieren
  - [ ] Streaming-Mode-Unterstützung (per-token tracking)
  - [ ] Per-Token-Probability-Sammlung
  - [ ] Thread-safe Accumulation

- [ ] Perplexity-Berechnung
  - [ ] Real-time Perplexity während Generation
  - [ ] Sliding-Window-Analyse (window_size=10)
  - [ ] Anomalie-Erkennung bei hoher Perplexity (> 100)
  - [ ] Moving-Average-Smoothing

- [ ] Confidence-Score-Aggregation
  - [ ] Gewichtete Durchschnittsbildung über Tokens
  - [ ] Outlier-Token-Behandlung (z-score > 3)
  - [ ] Kalibrierung gegen Ground-Truth-Daten
  - [ ] Confidence-Intervall-Berechnung

**Acceptance Criteria:**
- Token-Probabilities werden bei jedem LLM-Call gesammelt
- Perplexity-Berechnung erfolgt in < 5ms Overhead
- Anomalie-Erkennung triggert bei Perplexity > 100
- Confidence-Score korreliert mit tatsächlicher Qualität (Pearson r > 0.7)

**Tests:**
- [ ] Unit Test: Token-Probability-Callback
- [ ] Unit Test: Perplexity-Berechnung (bekannte Test-Sequences)
- [ ] Unit Test: Anomalie-Detection
- [ ] Integration Test: End-to-End mit llama.cpp

**Performance-Target:** < 10ms Overhead pro LLM-Response

---

### 2.2 Self-Consistency Check (4-5 Tage)

- [ ] Multiple Sampling implementieren
  - [ ] Parallele Generation mit verschiedenen Seeds (n=3-5)
  - [ ] Temperature-Variation für Diversität (T=0.7, 0.8, 0.9)
  - [ ] GPU-Batch-Inferenz für Performance
  - [ ] Timeout-Handling (max 10s pro Sample)

- [ ] Consistency-Metriken
  - [ ] Semantic Similarity zwischen Antworten (SBERT)
  - [ ] Entailment-Check (NLI-basiert)
  - [ ] Contradiction-Detection
  - [ ] Aggregierte Consistency-Score (0.0-1.0)

- [ ] Threshold-Tuning
  - [ ] A/B-Testing verschiedener Schwellenwerte
  - [ ] Domänen-spezifische Kalibrierung
  - [ ] User-Feedback-basierte Anpassung
  - [ ] Adaptive Threshold-Learning

**Acceptance Criteria:**
- Multiple Sampling generiert 3-5 diverse Antworten
- Consistency-Score misst semantische Übereinstimmung
- Contradiction-Detection erkennt widersprüchliche Aussagen
- Threshold-Tuning verbessert Precision/Recall-Balance

**Tests:**
- [ ] Unit Test: Multiple Sampling mit verschiedenen Seeds
- [ ] Unit Test: Semantic-Similarity-Berechnung
- [ ] Unit Test: Entailment & Contradiction Detection
- [ ] Integration Test: End-to-End Self-Consistency-Check

**Performance-Target:** < 2s für 5x Sampling (mit GPU-Batch)

---

### 2.3 FLARE-Style Active Retrieval (4-5 Tage)

- [ ] Forward-Looking-Generation
  - [ ] Sentence-by-Sentence-Generierung
  - [ ] Confidence-Monitoring pro Satz
  - [ ] Trigger für Nachladen (confidence < 0.5)
  - [ ] Iterative Generation Loop

- [ ] Dynamic Re-Retrieval
  - [ ] Automatische Query-Reformulierung
  - [ ] Iterative Dokumenten-Ergänzung
  - [ ] Cost-Benefit-Analyse für zusätzliche Abrufe
  - [ ] Max-Retrieval-Rounds-Limit (default: 3)

- [ ] Integration mit VectorIndexManager
  - [ ] Dynamic Query Expansion
  - [ ] Incremental Document Addition
  - [ ] Deduplication von Dokumenten
  - [ ] Ranking-Update nach Re-Retrieval

**Acceptance Criteria:**
- Sentence-by-Sentence-Generation funktioniert
- Confidence-Monitoring triggert Re-Retrieval bei < 0.5
- Query-Reformulierung verbessert Retrieval-Qualität
- Max 3 Re-Retrieval-Rounds pro Query

**Tests:**
- [ ] Unit Test: Sentence-Splitting & Confidence-Monitoring
- [ ] Unit Test: Query-Reformulierung
- [ ] Integration Test: FLARE-Loop mit realen Queries
- [ ] Performance Test: Latenz bei 3x Re-Retrieval

**Performance-Target:** < 500ms pro Re-Retrieval-Runde

---

## 🔗 Abhängigkeiten

**Code-Abhängigkeiten:**
- `include/llm/inference_engine_enhanced.h` - LLM-Callbacks
- `include/llm/llama_wrapper.h` - Token-Probability-API
- `include/vector_index/vector_index.h` - Dynamic Re-Retrieval
- `include/llm/sentence_bert.h` - Semantic Similarity

**Externe Libraries:**
- llama.cpp - Token-Probability-Zugriff
- Sentence-BERT (optional) - für Semantic Similarity
- NLI-Modell (RoBERTa-large-MNLI) - für Entailment

**Voraussetzungen:**
- Phase 1 abgeschlossen
- LLM Inference Engine unterstützt Token-Probability-Callbacks
- GPU verfügbar für Batch-Inferenz

## 📊 Erfolgskriterien

- [ ] Alle 11 Unit Tests bestehen
- [ ] Integration Tests zeigen < 500ms Overhead
- [ ] Self-Consistency verbessert Precision um ≥ 15%
- [ ] FLARE reduziert Hallucinations um ≥ 20%
- [ ] Dokumentation aktualisiert
- [ ] Code Review abgeschlossen
- [ ] Performance-Profiling durchgeführt

## 📝 Implementation Notes

**Performance-Targets:**
- Token-Probability-Tracking: < 10ms Overhead
- Self-Consistency-Check: < 2s (5x Sampling)
- FLARE Re-Retrieval: < 500ms pro Runde
- Gesamt-Overhead: < 3s für komplexe Queries

**Monitoring:**
- Track perplexity-Verteilung
- Log self-consistency-scores
- Monitor re-retrieval-frequency
- Alert bei Anomalien (perplexity > 150)

**Konfiguration:**
```yaml
knowledge_gap_detector:
  llm_confidence:
    enable_token_probability: true
    perplexity_threshold: 100
    self_consistency:
      num_samples: 5
      temperature_range: [0.7, 0.9]
      consistency_threshold: 0.6
    flare:
      enable: true
      max_retrieval_rounds: 3
      confidence_threshold: 0.5
```

## 🔗 Related Issues

- Requires: Phase 1 completed
- Blocks: Phase 3 (Claim-Verification)
- Related: `docs/de/llm/RAG_DECISION_CRITERIA.md`

## 📚 Referenzen

- [1] Asai et al., "Self-RAG," arXiv:2310.11511, 2023
- [2] Jiang et al., "Active RAG," EMNLP 2023 (FLARE)
- [15] Wang et al., "Self-Consistency," ICLR 2023
- Siehe: `docs/de/llm/RAG_BIBLIOGRAPHY.md`

---

**Labels:** `priority:P1`, `type:feature`, `area:llm`, `effort:x-large`, `phase:2`  
**Estimated Effort:** 3-4 Wochen (1 Developer)  
**Created:** 2026-01-18
