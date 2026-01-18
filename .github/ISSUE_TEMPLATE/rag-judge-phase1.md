---
name: RAG LLM-as-Judge - Phase 1
about: Grundlegende Judge-Implementierung (2-3 Wochen)
title: '[RAG-JUDGE-P1] LLM-as-Judge - Phase 1: Core Framework & Prompt Engineering'
labels: 'priority:P1, type:feature, area:llm, area:api, effort:large, phase:1'
assignees: ''
---

## 📋 Übersicht

Implementation des grundlegenden LLM-as-Judge Frameworks für RAG-Qualitätsbewertung.

**Namespace:** `themis::rag::judge`  
**Dokumentation:** `docs/de/llm/RAG_LLM_AS_JUDGE_TODO.md`  
**Analyse:** `docs/de/llm/RAG_LLM_AS_JUDGE_ANALYSE.md`

## 🎯 Ziele dieser Phase

- ✅ Core Judge Framework implementieren
- ✅ Prompt-Engineering für 4 Evaluation-Dimensionen
- ✅ Response-Parsing (JSON & Fallback)
- ✅ Configuration-System
- ✅ Unit Tests für alle Komponenten

## 📦 Arbeitspakete

### 1.1 Core Judge Framework (3-4 Tage)

**Status:** Basis vorhanden, Integration erforderlich

- [ ] Integration mit LLM Inference Engine
  - [ ] Verbindung zu `inference_engine_enhanced.cpp`
  - [ ] Prompt-Template-Management-System
  - [ ] Response-Parsing-Pipeline
  - [ ] Error-Handling & Retry-Logic

- [ ] Configuration-System
  - [ ] YAML/JSON-Config-Loader
  - [ ] Runtime-Config-Updates (ohne Neustart)
  - [ ] Validation von Config-Parametern
  - [ ] Schema-Definition (JSON Schema)

- [ ] Factory-Pattern-Verbesserung
  - [ ] Judge-Modi: SINGLE_DIM, MULTI_DIM, PAIRWISE
  - [ ] Lazy Loading von Judge-Komponenten
  - [ ] Dependency Injection für Testbarkeit

**Acceptance Criteria:**
- Judge kann LLM-Inference-Engine nutzen
- Config wird aus YAML/JSON geladen
- Runtime-Config-Updates funktionieren
- Factory erstellt verschiedene Judge-Modi

**Tests:**
- [ ] Unit Test: Config-Loading & Validation
- [ ] Unit Test: LLM-Integration (Mocked)
- [ ] Unit Test: Factory-Pattern verschiedene Modi
- [ ] Integration Test: End-to-End mit Mock-LLM

---

### 1.2 Prompt-Engineering (5-6 Tage)

**Kritischer Erfolgsf aktor für Judge-Qualität!**

#### A) Faithfulness-Prompt-Template

- [ ] Chain-of-Thought-Anweisung
  - [ ] Step-by-step reasoning template
  - [ ] Claim-Extraktion-Prompt
  - [ ] Document-Entailment-Check-Prompt

- [ ] Few-Shot-Examples
  - [ ] 3-5 annotierte Beispiele (high/medium/low faithfulness)
  - [ ] Diverse Domänen (tech, medical, legal)
  - [ ] Erklärungen für Scores

- [ ] Output-Format-Spezifikation
  ```json
  {
    "score": 0.85,
    "confidence": 0.9,
    "reasoning": "...",
    "supporting_claims": [...],
    "unsupported_claims": [...]
  }
  ```

#### B) Relevance-Prompt-Template

- [ ] Query-Aspekt-Identifikation
  - [ ] Aspekte aus Query extrahieren
  - [ ] Coverage-Assessment pro Aspekt
  - [ ] Noise-Detection (irrelevante Info)

- [ ] Intent-Alignment-Check
  - [ ] Informational/Navigational/Transactional
  - [ ] Direct-Answer vs. Explanatory
  - [ ] Context-Awareness

#### C) Completeness-Prompt-Template

- [ ] Aspekt-Vollständigkeit
  - [ ] Alle Query-Aspekte adressiert?
  - [ ] Tiefe der Antwort ausreichend?
  - [ ] Missing-Information-Identifikation

- [ ] Weighted-Coverage
  - [ ] Wichtige Aspekte höher gewichtet
  - [ ] Optional vs. Required Information
  - [ ] Depth-vs-Breadth-Balance

#### D) Coherence-Prompt-Template

- [ ] Logischer Fluss
  - [ ] Argument-Struktur
  - [ ] Transition-Quality
  - [ ] Internal-Consistency

- [ ] Linguistic-Quality
  - [ ] Grammar & Syntax
  - [ ] Clarity & Readability
  - [ ] Professional Tone

**Acceptance Criteria:**
- Alle 4 Prompt-Templates vollständig
- Few-Shot-Examples für jede Dimension
- JSON-Output-Format spezifiziert
- Chain-of-Thought führt zu besseren Scores

**Tests:**
- [ ] Prompt-Validation-Tests (Format, Length)
- [ ] Few-Shot-Example-Coverage-Tests
- [ ] Manual Review: 10 Test-Cases pro Dimension

---

### 1.3 Response-Parsing (3-4 Tage)

- [ ] JSON-Parser für strukturierte Outputs
  - [ ] Robustes Parsing mit RapidJSON/nlohmann::json
  - [ ] Fehlerbehandlung für malformed JSON
  - [ ] Fallback auf Regex bei fehlerhaftem JSON
  - [ ] Schema-Validierung gegen definiertes Format

- [ ] Score-Extraktion
  - [ ] Numerische Werte extrahieren (1-5 oder 0-1)
  - [ ] Normalisierung auf 0-1 Skala
  - [ ] Confidence-Score-Parsing
  - [ ] Validation (0 ≤ score ≤ 1)

- [ ] Explanation-Extraktion
  - [ ] Reasoning-Text extrahieren
  - [ ] Strukturierung für Logging
  - [ ] User-Facing-Formatting (Markdown)
  - [ ] Truncation für lange Explanations

**Acceptance Criteria:**
- JSON-Parsing funktioniert für well-formed responses
- Fallback-Regex funktioniert bei fehlerhaftem JSON
- Scores werden korrekt normalisiert (0-1)
- Explanations werden strukturiert extrahiert

**Tests:**
- [ ] Unit Test: JSON-Parsing (valid & invalid)
- [ ] Unit Test: Regex-Fallback
- [ ] Unit Test: Score-Normalisierung (verschiedene Skalen)
- [ ] Unit Test: Explanation-Extraktion

---

## 🔗 Abhängigkeiten

**Code-Abhängigkeiten:**
- `include/llm/inference_engine_enhanced.h` - LLM-Integration
- `include/config/config_manager.h` - Configuration
- `include/logging/logger.h` - Logging
- `include/utils/json_utils.h` - JSON-Parsing

**Externe Libraries:**
- RapidJSON oder nlohmann::json - JSON-Parsing
- spdlog - Logging
- yaml-cpp - YAML-Config-Loading

**Voraussetzungen:**
- LLM Inference Engine operational
- Config-System verfügbar
- Logging-Framework eingerichtet

## 📊 Erfolgskriterien

- [ ] Alle 10+ Unit Tests bestehen
- [ ] Prompt-Templates manuell reviewt (2+ Reviewer)
- [ ] Response-Parsing mit 95%+ Success-Rate
- [ ] Integration Tests zeigen < 500ms Overhead
- [ ] Dokumentation aktualisiert (API docs, Prompt-Guide)
- [ ] Code Review abgeschlossen
- [ ] Keine Compiler-Warnings

## 📝 Implementation Notes

**Performance-Targets:**
- Config-Loading: < 10ms
- Prompt-Rendering: < 5ms
- Response-Parsing: < 20ms
- Gesamt-Overhead (ohne LLM-Call): < 50ms

**Prompt-Engineering Best Practices:**
- Clear & specific instructions
- Few-shot examples from diverse domains
- Explicit output format specification
- Chain-of-thought reasoning encouraged
- Avoid bias-inducing language

**Monitoring:**
- Track parsing-success-rate
- Log malformed responses
- Monitor score-distributions per dimension
- Alert bei häufigen Parsing-Failures

**Konfiguration:**
```yaml
rag_judge:
  llm:
    model: "llama-3-70b-instruct"
    temperature: 0.3
    max_tokens: 1024
  prompts:
    faithfulness: "prompts/faithfulness_v1.txt"
    relevance: "prompts/relevance_v1.txt"
    completeness: "prompts/completeness_v1.txt"
    coherence: "prompts/coherence_v1.txt"
  parsing:
    fallback_regex: true
    strict_validation: true
```

## 🔗 Related Issues

- Blocks: Phase 2 (Multi-Dimension Evaluation)
- Related: `docs/de/llm/RAG_DECISION_CRITERIA.md`
- Related: G-Eval paper implementation

## 📚 Referenzen

- [9] Liu et al., "G-Eval," arXiv:2303.16634, 2023
- [10] Zheng et al., "MT-Bench," NeurIPS 2023
- [11] Es et al., "RAGAS," arXiv:2309.15217, 2023
- Siehe: `docs/de/llm/RAG_BIBLIOGRAPHY.md`

---

**Labels:** `priority:P1`, `type:feature`, `area:llm`, `area:api`, `effort:large`, `phase:1`  
**Estimated Effort:** 2-3 Wochen (1 Developer)  
**Created:** 2026-01-18
