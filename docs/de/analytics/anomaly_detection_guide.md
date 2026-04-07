# Anomaly Detection Guide

**Version:** v1.7.0
**Status:** 🟢 Production-Ready
**Last Updated:** 2026-04-06

---

## Übersicht

ThemisDB's Anomaly Detection Engine bietet statistische und ML-basierte Algorithmen zur
Erkennung von Ausreißern in Zeitreihendaten und mehrdimensionalen Datensätzen.
Die Engine unterstützt adaptives Lernen und Ensemble-Methoden.

**Header:** `include/analytics/anomaly_detection.h`
**Implementierung:** `src/analytics/anomaly_detection.cpp`

---

## Unterstützte Algorithmen

| Algorithmus | Enum | Beschreibung | Geeignet für |
| --- | --- | --- | --- |
| Z-Score | `Z_SCORE` | Normalverteilungsbasierte Ausreißererkennung | Normalverteilte Daten |
| Modified Z-Score (MAD) | `MODIFIED_Z_SCORE` | Robuster Z-Score via Median Absolute Deviation | Robuster gegen Ausreißer |
| Interquartile Range | `IQR` | Boxplot-basierte Ausreißererkennung | Nicht-normalverteilte Daten |
| Isolation Forest | `ISOLATION_FOREST` | Baum-basierte Isolation von Ausreißern | Hochdimensionale Daten |
| Local Outlier Factor | `LOF` | Dichte-basierte Nachbarschaftsanalyse | Cluster-basierte Anomalien |
| Ensemble | `ENSEMBLE` | Gewichtete Kombination aller Methoden | Allgemeiner Einsatz |

---

## Verwendungsbeispiele

### Z-Score Anomalieerkennung

```cpp
#include "analytics/anomaly_detection.h"
using namespace themisdb::analytics;

AnomalyDetector detector(AnomalyMethod::Z_SCORE);

// Trainingsdaten (Normalbetrieb) — als DataPoint-Vektor
std::vector<DataPoint> baseline;
for (double v : {10.1, 9.8, 10.3, 10.0, 9.9, 10.2}) {
    DataPoint dp;
    dp.set("value", v);
    baseline.push_back(dp);
}
detector.train(baseline);

// Neue Datenpunkte prüfen
std::vector<DataPoint> observations;
for (double v : {10.1, 10.3, 45.7, 9.8}) {
    DataPoint dp;
    dp.set("value", v);
    observations.push_back(dp);
}
auto results = detector.predictBatch(observations);

for (size_t i = 0; i < results.size(); ++i) {
    if (results[i].is_anomaly) {
        std::cout << "Anomalie bei Index " << i
                  << " (Score: " << results[i].score << ")\n";
    }
}
```

### Isolation Forest für mehrdimensionale Daten

```cpp
DetectorConfig cfg;
cfg.method        = AnomalyMethod::ISOLATION_FOREST;
cfg.n_estimators  = 100;     // Anzahl Bäume
cfg.contamination = 0.05;    // Erwarteter Ausreißer-Anteil (5%)
cfg.max_samples   = 256;

AnomalyDetector detector(cfg);

// Trainingsdaten: DataPoints mit named features
std::vector<DataPoint> training_data;
for (auto& [a, b, c] : std::vector<std::tuple<double,double,double>>{
    {1.0, 2.0, 3.0}, {1.1, 1.9, 3.1}  // ... viele normale Datenpunkte ...
}) {
    DataPoint dp;
    dp.set("feat_a", a); dp.set("feat_b", b); dp.set("feat_c", c);
    training_data.push_back(dp);
}
detector.train(training_data);

// Einzelnen Testpunkt prüfen
DataPoint test_point;
test_point.set("feat_a", 100.0); test_point.set("feat_b", 200.0); test_point.set("feat_c", 300.0);
auto result = detector.predict(test_point);
if (result.is_anomaly) {
    std::cout << "Ausreißer erkannt (Score: " << result.score << ")\n";
    auto exp = detector.explain(test_point);
    // exp.feature_contributions enthält den Feature-Beitrag je Feld
}
```

### LOF (Local Outlier Factor)

```cpp
DetectorConfig cfg;
cfg.method       = AnomalyMethod::LOF;
cfg.k_neighbors  = 20;        // k-Nachbarn für Dichteberechnung
cfg.contamination = 0.1;      // 10% Ausreißer erwartet

AnomalyDetector detector(cfg);
detector.train(training_data);

auto results = detector.predictBatch(test_data);
```

### Ensemble-Methode

```cpp
DetectorConfig cfg;
cfg.method = AnomalyMethod::ENSEMBLE;
// Methoden und deren Gewichte (gleiche Reihenfolge)
cfg.ensemble_methods  = {AnomalyMethod::Z_SCORE, AnomalyMethod::IQR,
                         AnomalyMethod::ISOLATION_FOREST, AnomalyMethod::LOF};
cfg.ensemble_weights  = {0.25, 0.25, 0.25, 0.25};
cfg.adaptive          = true;  // Online-Update via detector.update() aktivieren

AnomalyDetector detector(cfg);
detector.train(baseline_data);

auto results = detector.predictBatch(new_observations);
```

### Adaptives Lernen (Online-Update)

Der Detektor kann mit `update()` inkrementell neue Beobachtungen einfließen lassen,
wenn `DetectorConfig::adaptive == true` gesetzt ist:

```cpp
// Neuen Datenpunkt in das Modell einarbeiten
DataPoint new_point;
new_point.set("value", 10.5);
detector.update(new_point);
// Die internen Sliding-Window-Statistiken werden aktualisiert
```

---

## DetectorConfig Referenz

| Feld | Standard | Beschreibung |
| --- | --- | --- |
| `method` | `Z_SCORE` | Algorithmus (wird im Konstruktor oder in der Config gesetzt) |
| `threshold` | 0.6 | Score-Schwellenwert für Anomalie-Flag (score ≥ threshold → is_anomaly) |
| `k_neighbors` | 5 | LOF: Anzahl der k-Nachbarn |
| `n_estimators` | 100 | Isolation Forest: Anzahl der Bäume |
| `max_samples` | 256 | Isolation Forest: Max. Samples pro Baum |
| `contamination` | 0.1 | Erwarteter Anteil von Ausreißern (0.0–0.5) |
| `adaptive` | false | Online-Update via `update()` aktivieren |
| `ensemble_methods` | {} | Ensemble: Liste der verwendeten Methoden |
| `ensemble_weights` | {} | Ensemble: Gewichte pro Methode (uniform wenn leer) |

---

## AnomalyResult

```cpp
struct AnomalyResult {
    std::string   id;           // DataPoint-ID
    bool          is_anomaly;   // true wenn Ausreißer (score >= threshold)
    double        score;        // Anomalie-Score: 0.0 = normal, 1.0 = definitiv anomal
    AnomalyMethod method;       // Verwendeter Algorithmus
    int64_t       timestamp_ms; // Zeitstempel des Datenpunkts
    std::string   description;  // Lesbare Zusammenfassung
};
```

---

## Thread-Sicherheit

- `train()` — **nicht** thread-safe; vor paralleler Nutzung abschließen
- `predict()`, `predictBatch()`, `explain()` — **thread-safe** (read-only nach Training)
- `update()` — **nicht** thread-safe; Synchronisation durch den Aufrufer erforderlich

---

## Performance-Hinweise

| Algorithmus | Trainingszeit | Erkennungszeit | Speicher |
| --- | --- | --- | --- |
| Z-Score / MAD | O(n) | O(1) | O(1) |
| IQR | O(n log n) | O(1) | O(1) |
| Isolation Forest | O(n · t) | O(t · log n) | O(t · n) |
| LOF | O(n²) | O(n · k) | O(n) |
| Ensemble | Summe der Methoden | Summe der Methoden | Summe |

n = Trainingsdatenpunkte, t = Anzahl Bäume, k = Nachbarn

---

## Verwandte Dokumentation

- [Analytics Docs Hub](./README.md)
- [Forecasting Guide](./forecasting_guide.md)
- [CEP Guide](./cep_guide.md)
- [Streaming Windows Guide](./streaming_window_guide.md)
- [API Reference](../../../include/analytics/anomaly_detection.h)
- [Implementierung](../../../src/analytics/anomaly_detection.cpp)
- [Roadmap](../../../src/analytics/ROADMAP.md)

---

**Last Updated:** 2026-04-06
**Version:** v1.7.0
**Status:** 🟢 Production-Ready
