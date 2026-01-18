---
name: RAG Ethics Integration
about: Ethical Compliance Dimension & Ethics Module Integration (8 Wochen)
title: '[RAG-ETHICS] RAG Ethics Integration - Ethical Compliance Evaluation'
labels: 'priority:P1, type:feature, area:llm, area:security, effort:x-large, phase:integration'
assignees: ''
---

## 📋 Übersicht

Integration der ethischen Bewertungsdimension in LLM-as-Judge und Knowledge Gap Detector.

**Namespaces:** 
- `themis::rag::judge` (Ethical Compliance Dimension)
- `themis::rag::knowledge_gap` (Ethical Perspective Gap)

**Dokumentation:** `docs/de/llm/RAG_ETHICS_INTEGRATION_ANALYSIS.md`  
**Voraussetzung:** Judge Phase 2 & Gap Detector Phase 1 abgeschlossen

## 🎯 Ziele

- ✅ ETHICAL_COMPLIANCE als 5. Bewertungsdimension (VETO-POWER)
- ✅ ETHICAL_PERSPECTIVE_GAP Detection
- ✅ Integration mit Ethical Guidelines Manager
- ✅ Shared LLMMetaAnalyzer Base Class
- ✅ UN Human Rights Compliance

## 📦 Arbeitspakete

### Phase 1: Ethical Compliance Dimension (2-3 Wochen)

#### 1.1 Autonomy Respect Assessment (40% Gewicht)

- [ ] Patronizing-Language-Detection
  - [ ] Pattern-basierte Erkennung (Regex + NLP)
  - [ ] Sentiment-Analysis für herablassenden Ton
  - [ ] Penalty bei patronizing language (Score: -0.3)

- [ ] Choice-Preservation-Check
  - [ ] Prüfung auf aufgezwungene Meinungen
  - [ ] "Must", "Should", "Only"-Pattern-Detection
  - [ ] User-Agency-Erhaltung (Score: 0-1)

- [ ] Balanced-Perspective-Requirement
  - [ ] Mindestens 2 verschiedene Perspektiven erforderlich
  - [ ] Pro/Con-Balance-Analyse
  - [ ] Diversity-Score (0-1)

**Acceptance Criteria:**
- Patronizing-Language wird mit 85%+ Precision erkannt
- Choice-Preservation funktioniert bei moralischen Fragen
- Mindestens 2 Perspektiven werden durchgesetzt

**Tests:**
- [ ] Unit Test: Patronizing-Detection (10+ Beispiele)
- [ ] Unit Test: Choice-Preservation-Check
- [ ] Unit Test: Perspective-Counting
- [ ] Integration Test: End-to-End Autonomy Assessment

---

#### 1.2 Moral Diversity Assessment (30% Gewicht)

- [ ] Multi-Perspective-Representation
  - [ ] Integration mit Ethical Guidelines Manager
  - [ ] Diversitäts-Frameworks prüfen:
    - Asimov's Laws
    - UN Declaration of Human Rights (UDHR)
    - Constitutional AI Principles
  - [ ] Perspective-Coverage-Score (0-1)

- [ ] Bias-Detection
  - [ ] Cultural Bias-Erkennung
  - [ ] Moral-Framework-Bias-Detection
  - [ ] Geographic/Religious Bias-Check
  - [ ] Bias-Penalty bei einseitiger Darstellung

- [ ] Conflict-Resolution-Quality
  - [ ] Bei widersprüchlichen moralischen Prinzipien
  - [ ] Transparent-Communication-Check
  - [ ] Nuance-Preservation (nicht Schwarz/Weiß)

**Acceptance Criteria:**
- Mindestens 2 moralische Frameworks repräsentiert
- Bias-Detection erkennt einseitige Darstellung
- Konflikte werden transparent kommuniziert

**Tests:**
- [ ] Unit Test: Framework-Coverage (UDHR, Asimov, etc.)
- [ ] Unit Test: Bias-Detection
- [ ] Integration Test: Multi-Perspective-Scenarios

---

#### 1.3 Citation Quality (30% Gewicht)

- [ ] Source-Attribution für moralische Claims
  - [ ] Explizite Quellen-Nennung erforderlich
  - [ ] Framework-Attribution (z.B. "nach UDHR Artikel 19")
  - [ ] Missing-Attribution-Penalty

- [ ] Authority-Check
  - [ ] Quellen-Vertrauenswürdigkeit bewerten
  - [ ] Academic/Legal/Religious-Source-Validation
  - [ ] Domain-Authority-Score

- [ ] Citation-Completeness
  - [ ] Alle moralischen Claims zitiert?
  - [ ] Citation-Coverage-Score (0-1)

**Acceptance Criteria:**
- Moralische Claims erfordern Quellen-Nennung
- Authority-Check funktioniert für verschiedene Quellen-Typen
- Citation-Coverage wird gemessen

**Tests:**
- [ ] Unit Test: Citation-Extraktion
- [ ] Unit Test: Authority-Assessment
- [ ] Unit Test: Coverage-Berechnung

---

### Phase 2: Ethical Perspective Gap Detection (1-2 Wochen)

#### 2.1 ETHICAL_PERSPECTIVE_GAP Type

- [ ] Gap-Type-Erweiterung in Knowledge Gap Detector
  ```cpp
  enum class GapType {
      // ... existing types
      ETHICAL_PERSPECTIVE_GAP,  // NEW
  };
  ```

- [ ] Ethical-Query-Classification
  - [ ] Moral-Question-Detection (keyword + context)
  - [ ] Ethics-Domain-Classifier
  - [ ] Ethical-Intensity-Score (0-1)

- [ ] Perspective-Diversity-Check
  - [ ] Mindestens 2 verschiedene Frameworks in Documents?
  - [ ] Geographic/Cultural-Diversity
  - [ ] Trigger bei < 2 Perspektiven

**Acceptance Criteria:**
- Moralische Fragen werden erkannt (90%+ Recall)
- Perspektiven-Diversität wird gemessen
- Gap wird bei < 2 Perspektiven getriggert

**Tests:**
- [ ] Unit Test: Moral-Question-Detection
- [ ] Unit Test: Perspective-Counting
- [ ] Integration Test: End-to-End Ethical-Gap-Detection

---

### Phase 3: Integration & Shared Infrastructure (2-3 Wochen)

#### 3.1 LLMMetaAnalyzer Base Class

- [ ] Shared Base Class erstellen
  ```cpp
  namespace themis::rag {
  class LLMMetaAnalyzer {
  public:
      virtual ~LLMMetaAnalyzer() = default;
      
      // Shared configuration
      void loadConfig(const Config& config);
      
      // Shared LLM access
      std::string callLLM(const std::string& prompt);
      
      // Shared metrics
      void exportMetrics(MetricsCollector& collector);
  };
  }
  ```

- [ ] Refactor Judge & Gap Detector to inherit
  - [ ] `RAGJudge : public LLMMetaAnalyzer`
  - [ ] `KnowledgeGapDetector : public LLMMetaAnalyzer`

- [ ] Unified Configuration-System
  - [ ] Shared YAML-Schema
  - [ ] Consistent naming conventions
  - [ ] Cross-component config-sharing

#### 3.2 Ethical Guidelines Manager Integration

- [ ] Connection zu existing Ethics-Module
  - [ ] Load ethical frameworks from database
  - [ ] Query ethical guidelines
  - [ ] Cache frequently used principles

- [ ] Framework-Selection-API
  - [ ] User-specified frameworks (UDHR, Asimov, etc.)
  - [ ] Domain-specific framework-recommendation
  - [ ] Multi-framework-evaluation

**Acceptance Criteria:**
- LLMMetaAnalyzer Base Class implementiert
- Judge & Gap Detector nutzen shared infrastructure
- Ethical Guidelines Manager ist integriert

**Tests:**
- [ ] Unit Test: LLMMetaAnalyzer-Interface
- [ ] Integration Test: Cross-Component-Config
- [ ] Integration Test: Ethics-Module-Connection

---

### Phase 4: VETO-Power Implementation (1 Woche)

#### 4.1 Veto-Mechanismus

- [ ] Ethical Compliance Threshold: 0.70
- [ ] VETO-Logic:
  ```cpp
  if (ethical_compliance_score < 0.70) {
      return QUALITY_THRESHOLD_FAILED;  // VETO!
  }
  ```

- [ ] User-Facing-Communication
  - [ ] Klare Erklärung warum VETO
  - [ ] Vorschläge zur Verbesserung
  - [ ] Alternative Antworten anbieten?

- [ ] Audit-Logging
  - [ ] Alle VETO-Entscheidungen loggen
  - [ ] Reasoning & Score-Details
  - [ ] Compliance-Report generieren

**Acceptance Criteria:**
- VETO wird bei Score < 0.70 ausgelöst
- User erhält klare Erklärung
- Alle VETOs werden auditiert

**Tests:**
- [ ] Unit Test: VETO-Trigger-Logic
- [ ] Integration Test: VETO-Flow-End-to-End
- [ ] Manual Test: User-Communication-Quality

---

## 🔗 Abhängigkeiten

**Code-Abhängigkeiten:**
- `include/ethics/ethical_guidelines_manager.h` - Ethics Module
- `include/rag/rag_judge.h` - Judge Framework
- `include/rag/knowledge_gap_detector.h` - Gap Detector
- `include/config/config_manager.h` - Config System

**Externe Resources:**
- UN Declaration of Human Rights (UDHR) - Text-Datenbank
- Asimov's Laws - Framework-Definition
- Constitutional AI Principles (Anthropic) - Guidelines

**Voraussetzungen:**
- Judge Phase 2 abgeschlossen
- Gap Detector Phase 1 abgeschlossen
- Ethical Guidelines Manager operational

## 📊 Erfolgskriterien

- [ ] Alle 20+ Tests bestehen
- [ ] Ethical Compliance Dimension mit 80%+ Agreement mit Human-Annotators
- [ ] VETO-Mechanismus funktioniert wie spezifiziert
- [ ] Performance < 800ms für Ethical-Evaluation
- [ ] Dokumentation aktualisiert (Ethics-Integration-Guide)
- [ ] Code Review abgeschlossen
- [ ] Compliance-Audit durchgeführt

## 📝 Implementation Notes

**Performance-Targets:**
- Autonomy-Assessment: < 200ms
- Moral-Diversity-Check: < 300ms
- Citation-Quality: < 200ms
- Perspective-Gap-Detection: < 100ms
- **Gesamt: < 800ms**

**Ethical Frameworks-Support:**
1. **UDHR (Universal Declaration of Human Rights)**
   - 30 Artikel als Basis
   - Fokus auf Artikel 18 (Gedanken), 19 (Meinungsfreiheit)

2. **Asimov's Laws of Robotics**
   - Harm-Prevention
   - Command-Obedience (with caveats)
   - Self-Preservation

3. **Constitutional AI (Anthropic)**
   - Helpfulness
   - Harmlessness
   - Honesty

**Monitoring:**
- Track ethical-veto-rate
- Log framework-coverage-distribution
- Monitor bias-detection-accuracy
- Alert bei ungewöhnlich hoher VETO-Rate

**Konfiguration:**
```yaml
rag_judge:
  dimensions:
    ethical_compliance:
      enable: true
      threshold: 0.70
      veto_power: true
      weights:
        autonomy_respect: 0.40
        moral_diversity: 0.30
        citation_quality: 0.30
      frameworks:
        - udhr
        - asimov
        - constitutional_ai
      bias_detection:
        cultural: true
        geographic: true
        religious: true

knowledge_gap_detector:
  ethical_perspective_gap:
    enable: true
    min_perspectives: 2
    diversity_threshold: 0.60
```

## 🔗 Related Issues

- Requires: Judge Phase 2, Gap Detector Phase 1
- Integrates: Ethical Guidelines Manager
- Related: `docs/de/llm/RAG_ETHICS_INTEGRATION_ANALYSIS.md`
- Related: UN Human Rights compliance

## 📚 Referenzen

- [13] Bai et al., "Constitutional AI," Anthropic 2022
- [23] Isaac Asimov, "I, Robot," 1950
- [24] UN General Assembly, "Universal Declaration of Human Rights," 1948
- [25] EU AI Act, "High-Risk AI Systems," 2024
- Siehe: `docs/de/llm/RAG_BIBLIOGRAPHY.md`

---

**Labels:** `priority:P1`, `type:feature`, `area:llm`, `area:security`, `effort:x-large`, `phase:integration`  
**Estimated Effort:** 6-8 Wochen (1 Developer + Ethics Expert Review)  
**Created:** 2026-01-18
