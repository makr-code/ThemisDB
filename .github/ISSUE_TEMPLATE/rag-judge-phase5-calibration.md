---
name: RAG Judge - CalibrationManager Implementation
about: Implementierung der Kalibrierungs-Pipeline für RAG Judge (3-4 Tage)
title: '[RAG-JUDGE-P5-CAL] CalibrationManager Implementation - Human Alignment & ECE'
labels: 'priority:P2, type:feature, area:llm, area:rag, effort:medium, phase:5'
assignees: ''
---

## 📋 Übersicht

Implementierung des `CalibrationManager` für die Kalibrierung von Judge-Scores gegen menschliche Bewertungen.

**Namespace:** `themis::rag::judge`  
**Header:** `include/rag/calibration_manager.h` (✅ bereits vorhanden)  
**Implementation:** `src/rag/calibration_manager.cpp` (🚧 zu implementieren)  
**Dokumentation:** `docs/implementation-history/IMPLEMENTATION_PROGRESS_RAG_JUDGE_P5_P6.md`

## 🎯 Ziele

- ✅ API-Header definiert (195 LOC)
- 🚧 Implementierung der Calibration-Pipeline (~400 LOC)
- 🚧 Ground-Truth-Daten-Management
- 🚧 Calibration-Methoden (Temperature, Platt, Isotonic)
- 🚧 Metriken (ECE, Brier Score, IAA)
- 🚧 Unit Tests (6 Tests)

## 📦 Arbeitspakete

### 1. Ground-Truth-Management (1 Tag)

**Zu implementieren:**
- [ ] `loadGroundTruth(filepath)` - JSON/YAML Parsing
  - [ ] JSON-Schema-Validierung
  - [ ] Multi-Annotator-Support
  - [ ] Fehlerbehandlung bei invaliden Daten
- [ ] `addGroundTruth(annotation)` - Einzelne Annotation hinzufügen
- [ ] Interne Datenspeicherung (`ground_truth_` Vector)
- [ ] Validierung von Annotations (Scores 0-1, IDs unique)

**Acceptance Criteria:**
- Lädt JSON/YAML Ground-Truth-Dateien
- Validiert Annotation-Format
- Speichert Multi-Annotator-Daten
- Fehlerbehandlung bei fehlerhaften Dateien

**Code-Referenz:**
```cpp
struct GroundTruthAnnotation {
    std::string test_id;
    std::string query;
    std::string answer;
    std::vector<RetrievedDocument> documents;
    double faithfulness_score;  // 0-1
    double relevance_score;
    // ... weitere Scores
    std::vector<std::string> annotators;
    double inter_annotator_agreement;
};
```

---

### 2. Calibration-Methoden (1.5 Tage)

**Zu implementieren:**

#### A) Temperature Scaling
- [ ] `train()` - Optimierung von Temperature-Parameter
  - [ ] Grid-Search oder Gradient-Descent
  - [ ] Cross-Validation für Overfitting-Prevention
  - [ ] Per-Dimension Temperature-Parameter
- [ ] `applyTemperatureScaling(score, temperature)`
  - [ ] Logit-Transformation: `score / temperature`
  - [ ] Sigmoid-Rücktransformation

#### B) Platt Scaling
- [ ] Logistische Regression für Calibration
  - [ ] A und B Parameter-Optimierung
  - [ ] MLE (Maximum Likelihood Estimation)
- [ ] `applyPlattScaling(score, params)`
  - [ ] Sigmoid-Transformation mit A, B

#### C) Isotonic Regression
- [ ] Non-parametrische Monoton-Transformation
- [ ] `buildIsotonicModel(predictions, ground_truth)`
  - [ ] Pair-Adjacent-Violators (PAV) Algorithmus
  - [ ] Stückweise konstante/lineare Funktion
- [ ] Interpolation für neue Scores

**Acceptance Criteria:**
- Temperature Scaling funktioniert
- Platt Scaling implementiert
- Isotonic Regression trainierbar
- Calibration verbessert ECE messbar

---

### 3. Calibration-Metriken (1 Tag)

**Zu implementieren:**
- [ ] `calculateECE(predictions, ground_truth, confidences)`
  - [ ] Binning in N Bins (default: 10)
  - [ ] Berechnung pro Bin: |accuracy - confidence|
  - [ ] Gewichteter Durchschnitt über Bins
- [ ] `calculateBrierScore(predictions, ground_truth)`
  - [ ] Mean Squared Error für Probability-Predictions
  - [ ] `(1/N) * sum((pred_i - truth_i)^2)`
- [ ] `calculateInterAnnotatorAgreement(annotations)`
  - [ ] Cohen's Kappa (2 Annotators)
  - [ ] Fleiss' Kappa (3+ Annotators)
  - [ ] Krippendorff's Alpha (general-purpose)
- [ ] `calculateMetrics()` - Aggregiert alle Metriken
  - [ ] Pre- und Post-Calibration-Vergleich
  - [ ] Per-Dimension-Metriken

**Acceptance Criteria:**
- ECE korrekt berechnet (0-1 Skala)
- Brier Score implementiert
- Inter-Annotator-Agreement funktioniert
- Metriken-Objekt vollständig gefüllt

**Formeln:**
```
ECE = sum(|B_i| / N * |acc(B_i) - conf(B_i)|)
Brier = (1/N) * sum((p_i - y_i)^2)
Cohen's Kappa = (p_o - p_e) / (1 - p_e)
```

---

### 4. Calibration-Pipeline (0.5 Tage)

**Zu implementieren:**
- [ ] `train(judge)` - Haupt-Training-Methode
  - [ ] Judge evaluiert alle Ground-Truth-Cases
  - [ ] Calibration-Methode wird trainiert
  - [ ] Metriken vor/nach Calibration berechnen
  - [ ] Return: pair<pre_metrics, post_metrics>
- [ ] `calibrate(result)` - Apply Calibration
  - [ ] Wendet trainierte Parameter auf EvaluationResult an
  - [ ] Kalibriert alle Dimension-Scores
  - [ ] Aktualisiert overall_score

**Acceptance Criteria:**
- Training-Pipeline vollständig
- Calibration-Anwendung funktioniert
- Pre/Post-Metriken verfügbar
- Integration mit RAGJudge möglich

---

### 5. Persistence & Configuration (0.5 Tage)

**Zu implementieren:**
- [ ] `saveModel(filepath)` - Speichert Calibration-Parameter
  - [ ] JSON-Serialisierung von Parameters
  - [ ] Temperature-Werte, Platt A/B, Isotonic-Modell
- [ ] `loadModel(filepath)` - Lädt gespeicherte Parameter
  - [ ] JSON-Deserialisierung
  - [ ] Validierung der geladenen Werte
- [ ] `setConfig(config)` / `getConfig()` - Configuration-Management

**Acceptance Criteria:**
- Model-Speicherung funktioniert
- Model-Laden funktioniert
- Config-Updates zur Runtime möglich

---

### 6. Unit Tests (0.5 Tage)

**Test-Suite:** `tests/test_calibration_manager.cpp`

**Zu implementieren (6 Tests):**
1. [ ] `TEST(CalibrationManager, LoadGroundTruth)`
   - JSON-Datei laden, Annotations validieren
2. [ ] `TEST(CalibrationManager, TemperatureScaling)`
   - Training, Anwendung, ECE-Verbesserung prüfen
3. [ ] `TEST(CalibrationManager, CalculateECE)`
   - ECE-Berechnung mit verschiedenen Bins
4. [ ] `TEST(CalibrationManager, BrierScore)`
   - Brier Score für perfekte/schlechte Predictions
5. [ ] `TEST(CalibrationManager, InterAnnotatorAgreement)`
   - Kappa für 2/3+ Annotators
6. [ ] `TEST(CalibrationManager, SaveLoadModel)`
   - Persistence-Round-Trip

**Acceptance Criteria:**
- Alle 6 Tests bestehen
- Code-Coverage > 80%
- Edge-Cases abgedeckt (empty data, single sample, etc.)

---

## 🔗 Abhängigkeiten

**Code:**
- `include/rag/rag_judge.h` - EvaluationResult, RAGJudge
- `include/utils/logger.h` - Logging
- `nlohmann/json.hpp` - JSON-Serialisierung

**External (Optional):**
- `scipy` (Python bindings) - Für robuste Isotonic Regression
- Statistical library - Für bessere Kappa-Berechnung

**Voraussetzungen:**
- Phase 1-4 Judge-Implementation
- Ground-Truth-Annotationen (mindestens 20-30 Samples für Training)

---

## 📊 Erfolgskriterien

- [ ] Alle Methoden in `calibration_manager.h` implementiert
- [ ] 6 Unit Tests bestehen
- [ ] ECE-Verbesserung nachweisbar (< 0.1 Pre → < 0.05 Post)
- [ ] Dokumentation aktualisiert
- [ ] Code Review abgeschlossen
- [ ] Keine Compiler-Warnings
- [ ] Integration mit RAGJudge getestet

---

## 📝 Implementation Notes

**Performance-Targets:**
- Ground-Truth-Loading: < 100ms für 100 Samples
- Training (Temperature): < 500ms
- Calibration-Anwendung: < 1ms per Result
- Gesamt: < 1s für vollständiges Training

**Best Practices:**
- Verwende numerisch stabile Implementierungen (log-space für Likelihoods)
- Cross-Validation für Hyperparameter-Tuning
- Dokumentiere Calibration-Methoden-Wahl
- Logging für Training-Progress

**Konfiguration:**
```yaml
calibration:
  method: temperature_scaling  # temperature_scaling, platt_scaling, isotonic_regression
  num_bins: 10                 # Für ECE-Berechnung
  confidence_threshold: 0.5    # Min-Confidence für Predictions
  ground_truth_file: "annotations/judge_ground_truth.json"
```

---

## 📚 Referenzen

- [Guo et al., "On Calibration of Modern Neural Networks"](https://arxiv.org/abs/1706.04599)
- [Platt, "Probabilistic Outputs for SVMs"](https://www.cs.colorado.edu/~mozer/Teaching/syllabi/6622/papers/Platt1999.pdf)
- [Isotonic Regression](https://en.wikipedia.org/wiki/Isotonic_regression)
- Phase 5 Progress: `docs/implementation-history/IMPLEMENTATION_PROGRESS_RAG_JUDGE_P5_P6.md`

---

**Labels:** `priority:P2`, `type:feature`, `area:llm`, `area:rag`, `effort:medium`, `phase:5`  
**Estimated Effort:** 3-4 Tage (1 Developer)  
**Dependencies:** Phase 1-4 Complete, BiasDetector Implemented  
**Follow-up:** Phase 6 (EvaluationCache, BatchEvaluator)
