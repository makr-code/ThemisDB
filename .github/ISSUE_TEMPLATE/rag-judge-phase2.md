---
name: RAG LLM-as-Judge - Phase 2
about: Multi-Dimension Evaluation Implementation (2-3 Wochen)
title: '[RAG-JUDGE-P2] LLM-as-Judge - Phase 2: Multi-Dimension Evaluation'
labels: 'priority:P1, type:feature, area:llm, effort:large, phase:2'
assignees: ''
---

## 📋 Übersicht

Implementation der 4-dimensionalen RAG-Qualitätsbewertung mit spezialisierten Evaluatoren.

**Namespace:** `themis::rag::judge`  
**Dokumentation:** `docs/de/llm/RAG_LLM_AS_JUDGE_TODO.md` (Phase 2)  
**Voraussetzung:** Phase 1 muss abgeschlossen sein

## 🎯 Ziele dieser Phase

- ✅ Faithfulness-Evaluation (Faktentreue)
- ✅ Relevance-Evaluation (Antwortrelevanz)
- ✅ Completeness-Evaluation (Vollständigkeit)
- ✅ Coherence-Evaluation (Kohärenz)
- ✅ Integration Tests für alle Dimensionen

## 📦 Arbeitspakete

### 2.1 Faithfulness-Evaluation (5-6 Tage)

**Threshold: ≥ 0.80 (höchster Schwellenwert gegen Hallucinations)**

- [ ] Claim-Extraktion aus Antwort
  - [ ] LLM-basierte atomare Claim-Generierung
  - [ ] Strukturierte Claim-Liste (JSON-Array)
  - [ ] Claim-Kategorisierung (factual, opinion, reasoning)
  - [ ] Deduplication ähnlicher Claims

- [ ] Document-Entailment-Check
  - [ ] NLI-Modell-Integration (RoBERTa-large-MNLI)
  - [ ] Per-Claim-Verification gegen Documents
  - [ ] Support-Klassifikation:
    - FULLY_SUPPORTED (Score: 1.0)
    - PARTIALLY_SUPPORTED (Score: 0.5)
    - UNSUPPORTED (Score: 0.0)
    - CONTRADICTED (Score: -0.5)
  - [ ] Aggregierte Faithfulness-Score-Berechnung

- [ ] Citation-Prüfung
  - [ ] Explizite Quellen-Referenzen finden
  - [ ] Attribution-Mapping (Claim → Document)
  - [ ] Missing-Citation-Detection
  - [ ] Citation-Quality-Score (0-1)

**Acceptance Criteria:**
- Claims werden atomar und vollständig extrahiert
- NLI-basierte Verification mit 90%+ Accuracy
- Faithfulness-Score korreliert mit menschlichen Annotationen (r > 0.75)
- Citations werden korrekt zugeordnet

**Tests:**
- [ ] Unit Test: Claim-Extraktion (10+ Test-Cases)
- [ ] Unit Test: NLI-Entailment-Check (MNLI Test-Set)
- [ ] Unit Test: Aggregation-Logik
- [ ] Integration Test: End-to-End Faithfulness-Evaluation

**Performance-Target:** < 500ms für 10 Claims (mit NLI-Modell)

---

### 2.2 Relevance-Evaluation (4-5 Tage)

**Threshold: ≥ 0.65**

- [ ] Reverse-Question-Generation
  - [ ] LLM generiert Fragen zur Antwort
  - [ ] Semantic-Similarity zu Original-Query (SBERT)
  - [ ] Coverage-Score-Berechnung (Jaccard, BERTScore)
  - [ ] Multiple Questions pro Antwort (n=3-5)

- [ ] Query-Intent-Analysis
  - [ ] Intent-Klassifikation (informational, navigational, transactional)
  - [ ] Intent-Alignment-Check
  - [ ] Context-Awareness (Follow-up-Query-Detection)
  - [ ] Intent-Score (0-1)

- [ ] Noise-Detection
  - [ ] Irrelevante Informationen identifizieren
  - [ ] Signal-to-Noise-Ratio berechnen
  - [ ] Penalty für Irrelevanz
  - [ ] Relevance-Precision-Score

**Acceptance Criteria:**
- Reverse-Questions semantisch ähnlich zu Original-Query
- Intent-Klassifikation mit 85%+ Accuracy
- Noise-Detection identifiziert irrelevante Sätze
- Relevance-Score differenziert gut zwischen relevant/irrelevant

**Tests:**
- [ ] Unit Test: Reverse-Question-Generation
- [ ] Unit Test: Intent-Klassifikation
- [ ] Unit Test: Noise-Detection
- [ ] Integration Test: End-to-End Relevance-Evaluation

**Performance-Target:** < 400ms (inkl. SBERT)

---

### 2.3 Completeness-Evaluation (4-5 Tage)

**Threshold: ≥ 0.65**

- [ ] Aspect-Coverage-Analysis
  - [ ] Query-Aspekte extrahieren (aus Phase 1 GAP Detector)
  - [ ] Per-Aspekt-Coverage messen
  - [ ] Weighted-Coverage-Score
    - Required Aspects: 70% Gewicht
    - Optional Aspects: 30% Gewicht

- [ ] Depth-Assessment
  - [ ] Tiefe der Antwort bewerten (shallow/medium/deep)
  - [ ] Detail-Level pro Aspekt
  - [ ] Examples/Evidence vorhanden?
  - [ ] Depth-Score (0-1)

- [ ] Missing-Information-Detection
  - [ ] Fehlende Aspekte identifizieren
  - [ ] Priorisierung nach Wichtigkeit
  - [ ] Vorschläge für Verbesserungen
  - [ ] Completeness-Gap-Report

**Acceptance Criteria:**
- Alle Query-Aspekte werden erkannt
- Coverage-Score reflektiert tatsächliche Vollständigkeit
- Depth-Assessment ist konsistent
- Missing-Information wird actionable beschrieben

**Tests:**
- [ ] Unit Test: Aspect-Extraktion & Coverage
- [ ] Unit Test: Depth-Assessment
- [ ] Unit Test: Missing-Information-Detection
- [ ] Integration Test: End-to-End Completeness-Evaluation

**Performance-Target:** < 300ms

---

### 2.4 Coherence-Evaluation (3-4 Tage)

**Threshold: ≥ 0.65**

- [ ] Logical-Flow-Analysis
  - [ ] Argument-Struktur bewerten
  - [ ] Transition-Quality zwischen Sätzen
  - [ ] Conclusion-Alignment mit Premises
  - [ ] Logical-Consistency-Score (30%)

- [ ] Structural-Coherence
  - [ ] Intro-Body-Conclusion-Struktur
  - [ ] Topic-Sentence-Quality
  - [ ] Paragraph-Organization
  - [ ] Structure-Score (20%)

- [ ] Linguistic-Quality
  - [ ] Grammar & Syntax (via LanguageTool)
  - [ ] Clarity & Readability (Flesch-Score)
  - [ ] Professional Tone
  - [ ] Language-Score (20%)

- [ ] Internal-Consistency
  - [ ] Contradiction-Detection (NLI)
  - [ ] Claim-Alignment
  - [ ] Temporal-Consistency
  - [ ] Consistency-Score (30%)

**Acceptance Criteria:**
- Logical-Flow wird korrekt bewertet
- Structural-Coherence erkennt Organisationsprobleme
- Linguistic-Quality-Metriken sind valide
- Internal-Contradictions werden erkannt

**Tests:**
- [ ] Unit Test: Logical-Flow-Analysis
- [ ] Unit Test: Structural-Coherence
- [ ] Unit Test: Linguistic-Quality (mit LanguageTool)
- [ ] Unit Test: Contradiction-Detection
- [ ] Integration Test: End-to-End Coherence-Evaluation

**Performance-Target:** < 400ms

---

## 🔗 Abhängigkeiten

**Code-Abhängigkeiten:**
- `include/llm/inference_engine_enhanced.h` - LLM für Claim-Extraktion
- `include/llm/sentence_bert.h` - Semantic Similarity
- `include/nlp/nli_model.h` - Entailment & Contradiction
- `include/rag/knowledge_gap_detector.h` - Aspect-Extraktion

**Externe Libraries:**
- Sentence-BERT - Semantic Similarity
- RoBERTa-large-MNLI - NLI-Modell
- LanguageTool (optional) - Grammar/Syntax-Checking
- spaCy - NLP-Pipeline

**Externe Modelle:**
- `roberta-large-mnli` (~1.4GB) - für NLI
- `sentence-transformers/all-mpnet-base-v2` (~420MB) - für SBERT

**Voraussetzungen:**
- Phase 1 abgeschlossen
- NLI-Modell heruntergeladen
- SBERT-Modell verfügbar
- GPU empfohlen für Performance

## 📊 Erfolgskriterien

- [ ] Alle 16+ Unit Tests bestehen
- [ ] Integration Tests zeigen < 2s Gesamtlatenz
- [ ] Faithfulness-Score korreliert mit Human-Judgments (r > 0.75)
- [ ] Relevance/Completeness/Coherence konsistent (r > 0.65)
- [ ] Dokumentation aktualisiert (Evaluation-Guide)
- [ ] Code Review abgeschlossen
- [ ] Performance-Profiling durchgeführt

## 📝 Implementation Notes

**Performance-Targets:**
- Faithfulness: < 500ms (10 Claims + NLI)
- Relevance: < 400ms (inkl. SBERT)
- Completeness: < 300ms
- Coherence: < 400ms
- **Gesamt (4D): < 2s** (parallel execution)

**Parallelisierung:**
- Alle 4 Dimensionen parallel ausführen (Thread-Pool)
- NLI-Batch-Inferenz für Claims
- SBERT-Batch-Embedding für Reverse-Questions

**Monitoring:**
- Track score-distributions per dimension
- Log correlation mit human-judgments
- Monitor latency per dimension
- Alert bei score-Anomalien

**Weighted Overall-Score:**
```cpp
overall_score = 
    0.30 * faithfulness_score +  // Highest weight (anti-hallucination)
    0.25 * relevance_score +
    0.25 * completeness_score +
    0.20 * coherence_score;
```

**Konfiguration:**
```yaml
rag_judge:
  dimensions:
    faithfulness:
      threshold: 0.80
      nli_model: "roberta-large-mnli"
      claim_extraction_llm: true
    relevance:
      threshold: 0.65
      reverse_questions: 5
      sbert_model: "all-mpnet-base-v2"
    completeness:
      threshold: 0.65
      required_aspect_weight: 0.7
    coherence:
      threshold: 0.65
      weights:
        logical_flow: 0.30
        structure: 0.20
        language: 0.20
        consistency: 0.30
  execution:
    parallel: true
    timeout_ms: 3000
```

## 🔗 Related Issues

- Requires: Phase 1 completed
- Blocks: Phase 3 (Ethical Compliance Dimension)
- Related: `docs/de/llm/RAG_DECISION_CRITERIA.md`
- Related: Knowledge Gap Detector (Aspect-Extraktion)

## 📚 Referenzen

- [9] Liu et al., "G-Eval," arXiv:2303.16634, 2023
- [11] Es et al., "RAGAS," arXiv:2309.15217, 2023
- [15] Wang et al., "Self-Consistency," ICLR 2023
- [20] Zhang et al., "BERTScore," ICLR 2020
- Siehe: `docs/de/llm/RAG_BIBLIOGRAPHY.md`

---

**Labels:** `priority:P1`, `type:feature`, `area:llm`, `effort:large`, `phase:2`  
**Estimated Effort:** 2-3 Wochen (1 Developer)  
**Created:** 2026-01-18
