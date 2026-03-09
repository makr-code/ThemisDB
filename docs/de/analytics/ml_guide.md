# ML, AutoML & Model Serving Guide

**Version:** v1.7.0
**Status:** 🟢 Production-Ready
**Last Updated:** 2026-03-09

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

// Trainingsdaten vorbereiten
AutoMLDataset dataset;
dataset.features = {
    {1.0, 2.5, 0.3},
    {2.1, 1.8, 0.7},
    // ...
};
dataset.labels = {0, 1, 0, 1, /* ... */};
dataset.feature_names = {"feature_a", "feature_b", "feature_c"};

// AutoML konfigurieren
AutoMLConfig cfg;
cfg.max_time_seconds = 60;        // Max. Trainingszeit
cfg.cv_folds = 5;                 // Cross-Validation Faltungen
cfg.metric = AutoMLMetric::F1;    // Optimierungsmetrik
cfg.generate_ensemble = true;     // Ensemble aus Top-Modellen

AutoMLEngine automl(cfg);
automl.fit(dataset);

// Bestes Modell abrufen
auto best_model = automl.bestModel();
std::cout << "Bestes Modell: " << best_model.algorithm_name << "\n";
std::cout << "F1-Score: " << best_model.cv_score << "\n";

// SHAP-Erklärungen
auto shap = automl.computeSHAPValues(dataset.features[0]);
for (size_t i = 0; i < shap.size(); ++i) {
    std::cout << dataset.feature_names[i] << ": " << shap[i] << "\n";
}

// Inferenz
std::vector<double> new_sample = {1.5, 2.0, 0.5};
auto prediction = best_model.predict(new_sample);
std::cout << "Klasse: " << prediction.label
          << " (Konfidenz: " << prediction.confidence << ")\n";
```

### AutoMLConfig Referenz

| Feld | Standard | Beschreibung |
| --- | --- | --- |
| `max_time_seconds` | 60 | Maximale Trainingszeit in Sekunden |
| `cv_folds` | 5 | Anzahl Cross-Validation-Faltungen |
| `metric` | `ACCURACY` | Optimierungsmetrik (ACCURACY, F1, AUC, RMSE) |
| `generate_ensemble` | false | Ensemble aus Top-3-Modellen erstellen |
| `max_models` | 10 | Maximale Anzahl auszuprobierender Modelle |
| `feature_engineering` | true | Automatisches Feature Engineering aktivieren |

---

## ML Serving — Externe Inferenz (ONNX & TensorFlow)

`MLServingClient` bietet eine einheitliche Abstraktion für externe ML-Backends:

### ONNX Runtime (lokale Inferenz)

```cpp
#include "analytics/ml_serving.h"
using namespace themisdb::analytics;

// ONNX-Modell laden
MLServingConfig cfg;
cfg.backend = MLBackend::ONNX;
cfg.model_path = "/models/classifier.onnx";

MLServingClient client(cfg);

// Inferenz
DataPoint dp;
dp.features = {1.0, 2.5, 0.3, 4.1};

auto result = client.infer(dp);
std::cout << "Klasse: " << result.predicted_class << "\n";
std::cout << "Wahrscheinlichkeiten: ";
for (auto p : result.probabilities)
    std::cout << p << " ";
```

### TensorFlow Serving (REST-API)

```cpp
MLServingConfig cfg;
cfg.backend = MLBackend::TENSORFLOW_SERVING;
cfg.endpoint = "http://tf-serving:8501/v1/models/my_model:predict";
cfg.timeout_ms = 5000;

MLServingClient client(cfg);
// Graceful Degradation: Gibt leeres Ergebnis zurück wenn Backend nicht erreichbar
auto result = client.infer(dp);
```

### Graceful Degradation

Wenn ein ML-Backend nicht verfügbar ist, degradiert `MLServingClient` graceful:

```cpp
auto result = client.infer(dp);
if (result.is_degraded) {
    // Fallback-Logik, z.B. regelbasiertes System
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

// Modell registrieren
ModelConfig cfg;
cfg.name = "fraud_detector";
cfg.version = "v2.1";
cfg.algorithm = "gradient_boosting";
// ... Modellparameter ...
engine.registerModel(cfg);

// Online-Inferenz (einzelner Datenpunkt)
DataPoint dp;
dp.features = {100.0, 3, 0.5, /* ... */};

auto result = engine.infer("fraud_detector", "v2.1", dp);
std::cout << "Fraud-Wahrscheinlichkeit: " << result.probabilities[1] << "\n";

// Batch-Inferenz
std::vector<DataPoint> batch = {dp1, dp2, dp3};
auto batch_results = engine.inferBatch("fraud_detector", "v2.1", batch);

// Modell-Health-Metriken
auto metrics = engine.getModelMetrics("fraud_detector", "v2.1");
std::cout << "Anfragen gesamt: " << metrics.total_requests << "\n";
std::cout << "Ø Latenz (ms): " << metrics.avg_latency_ms << "\n";
std::cout << "Fehlerrate: " << metrics.error_rate << "\n";

// Modell serialisieren/deserialisiern
std::string serialized = engine.serializeModel("fraud_detector", "v2.1");
engine.loadModel(serialized);
```

### Modell-Versionierung

```cpp
// Modell-Versionen auflisten
auto versions = engine.listVersions("fraud_detector");
for (const auto& v : versions) {
    std::cout << v.version << " (" << v.status << ")\n";
}

// Version deaktivieren
engine.deactivateModel("fraud_detector", "v1.0");

// Aktuellste aktive Version abrufen
auto latest = engine.getLatestVersion("fraud_detector");
```

---

## Thread-Sicherheit

| Komponente | Methode | Thread-Sicher? |
| --- | --- | --- |
| AutoMLEngine | `fit()` | Nein — ein Thread pro Training |
| AutoMLEngine | `predict()` | Ja — read-only nach fit |
| MLServingClient | `infer()` | Ja |
| ModelServingEngine | `infer()`, `inferBatch()` | Ja |
| ModelServingEngine | `registerModel()` | Ja |

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

**Last Updated:** 2026-03-09
**Version:** v1.7.0
**Status:** 🟢 Production-Ready
