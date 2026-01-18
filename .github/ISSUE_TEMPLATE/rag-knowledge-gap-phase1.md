---
name: RAG Knowledge Gap Detector - Phase 1
about: Grundlegende Implementierung des Knowledge Gap Detectors (2-3 Wochen)
title: '[RAG-GAP-P1] Knowledge Gap Detector - Phase 1: Grundlegende Implementierung'
labels: 'priority:P1, type:feature, area:llm, area:api, effort:large, phase:1'
assignees: ''
---

## 📋 Übersicht

Implementation der grundlegenden Funktionalität des Knowledge Gap Detectors im ThemisDB RAG-System.

**Namespace:** `themis::rag::knowledge_gap`  
**Dokumentation:** `docs/de/llm/RAG_KNOWLEDGE_GAP_DETECTOR_TODO.md`  
**Analyse:** `docs/de/llm/RAG_KNOWLEDGE_GAP_DETECTOR_ANALYSE.md`

## 🎯 Ziele dieser Phase

- ✅ Similarity-basierte Gap-Erkennung implementieren
- ✅ Integration mit VectorIndexManager
- ✅ Query-Aspekt-Analyse
- ✅ Document Count & Basic Metrics
- ✅ Unit Tests für alle Komponenten

## 📦 Arbeitspakete

### 1.1 Similarity-basierte Erkennung (3-4 Tage)

**Status:** Basis vorhanden, Integration erforderlich

- [ ] Integration mit VectorIndexManager
  - [ ] Retrieval-Scores aus `vector_index.cpp` abrufen
  - [ ] Similarity-Metriken normalisieren (0.0-1.0)
  - [ ] GPU-beschleunigte Batch-Berechnung
  - [ ] Error-Handling für fehlende Embeddings

**Acceptance Criteria:**
- Similarity-Scores können aus VectorIndexManager abgerufen werden
- Scores sind auf 0.0-1.0 normalisiert
- GPU-Batch-Berechnung funktioniert mit >100 Dokumenten
- Graceful Degradation bei fehlenden Embeddings

**Tests:**
- [ ] Unit Test: Niedrige Similarity-Scores (< 0.75) triggern Gap
- [ ] Unit Test: Unzureichende Dokumentenanzahl (< 3) triggert Gap
- [ ] Unit Test: Schwellenwert-Konfiguration funktioniert
- [ ] Integration Test: End-to-End mit VectorIndexManager

---

### 1.2 Query-Aspekt-Analyse (4-5 Tage)

- [ ] Query-Parser implementieren
  - [ ] Named Entity Recognition für Schlüsselbegriffe
  - [ ] Dependency Parsing für Aspekt-Extraktion
  - [ ] Integration mit bestehendem NLP-Pipeline
  - [ ] Multi-Language-Support (DE/EN)

- [ ] Coverage-Berechnung
  - [ ] Semantic Matching zwischen Query und Documents
  - [ ] Embedding-basierte Aspekt-Abdeckung
  - [ ] Gewichtung nach Aspekt-Wichtigkeit
  - [ ] Missing-Aspect-Score (0.0 = keine Abdeckung, 1.0 = vollständig)

- [ ] Missing-Aspect-Detection
  - [ ] Identifikation fehlender Informationen
  - [ ] Priorisierung nach Relevanz
  - [ ] Vorschläge für erweiterte Suche
  - [ ] Strukturierte Missing-Info-Liste

**Acceptance Criteria:**
- Query-Aspekte werden korrekt extrahiert (≥ 90% Precision)
- Coverage-Score reflektiert tatsächliche Abdeckung
- Missing-Aspects sind actionable für Query-Reformulierung
- Multi-Language-Support für DE & EN

**Tests:**
- [ ] Unit Test: Aspekt-Extraktion aus einfachen Queries
- [ ] Unit Test: Aspekt-Extraktion aus komplexen Multi-Aspekt-Queries
- [ ] Unit Test: Coverage-Berechnung mit verschiedenen Abdeckungsgraden
- [ ] Integration Test: End-to-End mit realen Dokumenten

---

### 1.3 Document Count & Basic Metrics (2-3 Tage)

- [ ] Konfigurierbare Schwellenwerte
  - [ ] `min_documents` Validierung
  - [ ] Dynamic threshold adjustment basierend auf Query-Typ
  - [ ] Domain-spezifische Kalibrierung
  - [ ] Config-Loader (YAML/JSON)

- [ ] Metadata-basierte Filterung
  - [ ] Zeitstempel-Validierung (Outdated-Check)
  - [ ] Vertrauenswürdigkeit-Scores
  - [ ] Quellen-Diversität-Analyse
  - [ ] Domain-Authority-Assessment

**Acceptance Criteria:**
- `min_documents` threshold ist konfigurierbar (default: 3)
- Dynamic thresholds passen sich an Query-Kontext an
- Metadata-Filterung erkennt veraltete Dokumente (> 2 Jahre)
- Quellen-Diversität wird gemessen (mindestens 2 verschiedene Quellen)

**Tests:**
- [ ] Unit Test: Config-Loading & Validation
- [ ] Unit Test: Dynamic Threshold Adjustment
- [ ] Unit Test: Metadata Filtering (Zeitstempel, Authority)
- [ ] Unit Test: Diversität-Berechnung

---

## 🔗 Abhängigkeiten

**Code-Abhängigkeiten:**
- `include/vector_index/vector_index.h` - Similarity-Scores
- `include/llm/inference_engine_enhanced.h` - NLP-Pipeline
- `include/config/config_manager.h` - Configuration-System

**Externe Libraries:**
- SpaCy/StanfordNLP (optional) - für NLP-Parsing
- Eigen/Armadillo - für Matrix-Operationen
- RapidJSON - für Config-Parsing

**Voraussetzungen:**
- VectorIndexManager muss Similarity-Scores exportieren
- Config-System muss YAML/JSON laden können
- Logging-Framework verfügbar

## 📊 Erfolgskriterien

- [ ] Alle 12 Unit Tests bestehen
- [ ] Integration Tests zeigen < 50ms Latenz für Similarity-Checks
- [ ] Coverage-Berechnung mit < 100ms pro Query
- [ ] Dokumentation aktualisiert (API docs, README)
- [ ] Code Review abgeschlossen
- [ ] Keine Compiler-Warnings

## 📝 Implementation Notes

**Performance-Targets:**
- Similarity-Check: < 50ms für 100 Dokumente
- Aspekt-Analyse: < 100ms pro Query
- Gesamt-Overhead: < 200ms pro RAG-Request

**Monitoring:**
- Metriken exportieren via Prometheus
- Log gap-detection-rate
- Track false-positive/false-negative rates

## 🔗 Related Issues

- Blocked by: #XXX (falls VectorIndexManager-Änderungen nötig)
- Blocks: Phase 2 LLM-basierte Konfidenzmetriken
- Related: `docs/de/llm/RAG_DECISION_CRITERIA.md` (Schwellenwerte)

## 📚 Referenzen

- [1] Asai et al., "Self-RAG," arXiv:2310.11511, 2023
- [2] Jiang et al., "Active RAG," EMNLP 2023
- [8] Izacard & Grave, "Passage Retrieval," EACL 2021
- Siehe: `docs/de/llm/RAG_BIBLIOGRAPHY.md`

---

**Labels:** `priority:P1`, `type:feature`, `area:llm`, `area:api`, `effort:large`, `phase:1`  
**Estimated Effort:** 2-3 Wochen (1 Developer)  
**Created:** 2026-01-18
