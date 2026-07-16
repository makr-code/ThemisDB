# ML, AutoML & Model Serving Guide

**Version:** v1.7.0
**Status:** 🟢 Production-Ready
**Last Updated:** 2026-04-06

---

## Übersicht

ThemisDB bietet eine vollständige ML-Pipeline: automatische Modellauswahl (AutoML),
externe Inferenz über ONNX Runtime und TensorFlow Serving, sowie ein internes
versioniertes Modell-Registry für Online- und Batch-Inferenz.

| Komponente | Header | Implementierung |
| --- | --- | --- |
| AutoML | `include/analytics/automl.h` | `src/analytics/automl.cpp` |
| ML Serving (extern) | `include/analytics/ml_serving.h` | `src/analytics/ml_serving.cpp` |
| Model Registry | `include/analytics/model_serving.h` | `src/analytics/model_serving.cpp` |

---

## AutoML — Automatische Modellauswahl

AutoML (`automl.cpp`) wählt automatisch den besten ML-Algorithmus für einen Datensatz
durch Hyperparameter-Suche, Feature Engineering und Ensemble-Generierung.

### Unterstützte Algorithmen

| Algorithmus | Beschreibung |
| --- | --- |
| Logistic Regression | Lineare binäre/multi-class Klassifikation |
| Decision Tree | Baumbasierte Klassifikation/Regression |
| Random Forest | Ensemble aus Entscheidungsbäumen |
| Gradient Boosting | Sequentielle Boosting-Ensemble |
| K-Nearest Neighbors | Instanzbasiertes Lernen |
| Linear Regression | Lineare Regression |

### Verwendungsbeispiel

```cpp
#include "analytics/automl.h"
using namespace themisdb::analytics;

// Trainingsdaten als DataPoint-Vektor aufbauen
// Jeder DataPoint enthält named features + das Zielfeld (label)
struct Sample { double feat_a, feat_b, feat_c; std::string label; };
std::vector<Sample> raw_data = {
    {1.0, 2.5, 0.3, "cat"},
    {2.1, 1.8, 0.7, "dog"},
    /* ... */
};

std::vector<DataPoint> data;
data.reserve(raw_data.size());
for (const auto& s : raw_data) {
    DataPoint dp;
    dp.set("feature_a", s.feat_a);
    dp.set("feature_b", s.feat_b);
    dp.set("feature_c", s.feat_c);
    dp.set("label", s.label);   // Zielfeld — muss dem AutoMLConfig::target entsprechen
    data.push_back(dp);
}

// AutoML konfigurieren
AutoMLConfig cfg;
cfg.target           = "label";
cfg.task             = AutoMLTask::CLASSIFICATION;
cfg.max_time_minutes = 1;                   // Max. Trainingszeit in Minuten
cfg.cv_folds         = 5;                   // Cross-Validation Faltungen
cfg.metric           = AutoMLMetric::F1;    // Optimierungsmetrik
cfg.ensemble         = true;                // Ensemble aus Top-Modellen
cfg.max_trials       = 20;                  // Max. auszuprobierender Hyperparameter-Sets

// Training starten — gibt das beste Modell zurück
AutoML automl;
AutoMLModel best_model = automl.trainClassifier(data, cfg);

std::cout << "Bestes Modell: " << modelAlgorithmName(best_model.algorithm()) << "\n";
std::cout << "F1-Score: " << best_model.metrics().f1 << "\n";

// Top-Kandidaten einsehen
for (const auto& c : best_model.candidateModels()) {
    std::cout << c.name << " cv_score=" << c.cv_score << "\n";
}

// SHAP-Erklärungen für einen einzelnen Datenpunkt
DataPoint sample;
sample.set("feature_a", 1.5); sample.set("feature_b", 2.0); sample.set("feature_c", 0.5);

auto exp = best_model.explainOne(sample);
std::cout << "Vorhergesagte Klasse: " << exp.predicted_label
          << " (Konfidenz: " << exp.confidence << ")\n";
for (const auto& [feat, contrib] : exp.feature_contributions)
    std::cout << feat << ": " << contrib << "\n";

// Einzelinferenz
std::string label = best_model.predictOne(sample);

// Batch-Inferenz
auto predictions = best_model.predict(data);
```

### AutoMLConfig Referenz

| Feld | Standard | Beschreibung |
| --- | --- | --- |
| `target` | `""` | Name des Zielfeldes in `DataPoint` |
| `task` | `CLASSIFICATION` | `CLASSIFICATION` oder `REGRESSION` |
| `max_time_minutes` | 5 | Maximale Trainingszeit in **Minuten** |
| `cv_folds` | 3 | Anzahl Cross-Validation-Faltungen |
| `metric` | `F1` | Optimierungsmetrik (ACCURACY, F1, AUC\_ROC, RMSE, …) |
| `ensemble` | true | Ensemble aus Top-`ensemble_top_k`-Modellen erstellen |
| `ensemble_top_k` | 3 | Anzahl Modelle im Ensemble |
| `max_trials` | 50 | Maximale Anzahl Hyperparameter-Versuche |
| `feature_engineering` | true | Automatisches Feature Engineering aktivieren |

---

## ML Serving — Externe Inferenz (ONNX & TensorFlow)

`MLServingClient` bietet eine einheitliche Abstraktion für externe ML-Backends:

### ONNX Runtime (lokale Inferenz)

```cpp
#include "analytics/ml_serving.h"
using namespace themisdb::analytics;

// ONNX-Backend konfigurieren
MLServingConfig cfg;
cfg.backend                       = MLBackendType::ONNX_RUNTIME;
cfg.onnx_config.model_directory   = "/models";   // Verzeichnis mit *.onnx-Dateien
cfg.onnx_config.enable_cuda       = false;        // CUDA-Provider deaktiviert

MLServingClient client(cfg);

// Inferenz über DataPoint (Felder werden alphabetisch sortiert → float32)
DataPoint dp;
dp.set("feat_a", 1.0); dp.set("feat_b", 2.5); dp.set("feat_c", 0.3);

auto resp = client.inferFromDataPoint("classifier", dp);
if (resp.ok()) {
    // resp.outputs[0].data enthält den rohen float32-Ausgabe-Tensor
    for (float v : resp.outputs[0].data)
        std::cout << v << " ";
} else {
    std::cerr << "Fehler: " << resp.error_message << "\n";
}
```

### TensorFlow Serving (REST-API)

```cpp
MLServingConfig cfg;
cfg.backend                 = MLBackendType::TF_SERVING;
cfg.tf_config.base_url      = "http://tf-serving:8501";  // Basis-URL (ohne Modelpfad)
cfg.tf_config.timeout_ms    = 5000;
cfg.tf_config.api_key       = "bearer-token";  // optional

MLServingClient client(cfg);
```

### Graceful Degradation

Wenn ein ML-Backend nicht verfügbar ist, gibt `MLServingClient` einen strukturierten
Fehler zurück:

```cpp
auto resp = client.inferFromDataPoint("model", dp);
if (!resp.ok()) {
    if (resp.status == MLServingStatus::UNAVAILABLE) {
        // Fallback-Logik, z.B. regelbasiertes System
    }
}
```

---

## Model Registry — Internes Modell-Management

`ModelServingEngine` verwaltet ein versioniertes, thread-sicheres Modell-Registry
für Online- und Batch-Inferenz:

```cpp
#include "analytics/model_serving.h"
using namespace themisdb::analytics;

ModelServingEngine engine;

// Trainiertes AutoMLModel registrieren (Move-Semantik)
AutoML automl;
AutoMLModel model = automl.trainClassifier(training_data, cfg);
engine.registerModel("fraud_detector", "v2.1", std::move(model));

// Online-Inferenz (einzelner Datenpunkt) — gibt Label als string zurück
DataPoint dp;
dp.set("amount",     100.0);
dp.set("n_items",    3.0);
dp.set("risk_score", 0.5);

std::string label = engine.predict("fraud_detector", "v2.1", dp);

// Klassen-Wahrscheinlichkeiten (Classification)
auto probas = engine.predictProba("fraud_detector", "v2.1", {dp});
// probas[0] = map<string,double>: {"fraud": 0.87, "legit": 0.13}

// Batch-Inferenz
std::vector<DataPoint> batch = {dp1, dp2, dp3};
auto batch_labels = engine.predictBatch("fraud_detector", "v2.1", batch);

// Modell-Health-Metriken
auto metrics = engine.healthMetrics("fraud_detector", "v2.1");
if (metrics) {
    std::cout << "Anfragen gesamt: " << metrics->total_predictions << "\n";
    std::cout << "Ø Latenz (ms): "   << metrics->avg_latency_ms << "\n";
}

// Modell serialisieren/deserialisieren
std::string serialized = engine.serializeModel("fraud_detector", "v2.1");
engine.loadModel("fraud_detector", "v2.1-restored", serialized);
```

### Modell-Versionierung

```cpp
// Alle registrierten Modelle auflisten
for (const auto& info : engine.listModels()) {
    std::cout << info.name << " " << info.version
              << " active=" << info.is_active << "\n";
}

// Modell aus der Registry entfernen (deaktivieren)
engine.unregisterModel("fraud_detector", "v1.0");

// Prüfen ob ein Modell registriert ist
if (engine.isRegistered("fraud_detector", "v2.1")) { /* ... */ }
```

---

## Thread-Sicherheit

| Komponente | Methode | Thread-Sicher? |
| --- | --- | --- |
| `AutoML` | `trainClassifier()` / `trainRegressor()` | Nein — ein Thread pro Training |
| `AutoMLModel` | `predict()`, `predictOne()`, `explain()` | Ja — read-only nach Training |
| `MLServingClient` | `infer()`, `inferFromDataPoint()` | Ja |
| `ModelServingEngine` | `predict()`, `predictBatch()`, `predictProba()` | Ja (shared lock) |
| `ModelServingEngine` | `registerModel()`, `unregisterModel()`, `loadModel()` | Ja (exclusive lock) |

---

## Verwandte Dokumentation

- [Analytics Docs Hub](./README.md)
- [Anomaly Detection Guide](./anomaly_detection_guide.md)
- [Forecasting Guide](./forecasting_guide.md)
- [API Reference AutoML](../../../include/analytics/automl.h)
- [API Reference ML Serving](../../../include/analytics/ml_serving.h)
- [API Reference Model Serving](../../../include/analytics/model_serving.h)
- [Implementierungs-Roadmap](../../../src/analytics/ROADMAP.md)

---

**Last Updated:** 2026-04-06
**Version:** v1.7.0
**Status:** 🟢 Production-Ready
