# Anomaly Detection Guide

**Version:** v1.7.0
**Status:** 🟢 Production-Ready
**Last Updated:** 2026-03-09

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
| Z-Score | `ZSCORE` | Normalverteilungsbasierte Ausreißererkennung | Normalverteilte Daten |
| Modified Z-Score (MAD) | `MODIFIED_ZSCORE` | Robuster Z-Score via Median Absolute Deviation | Robuster gegen Ausreißer |
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

AnomalyDetector detector(AnomalyMethod::ZSCORE);

// Trainingsdaten (Normalbetrieb)
std::vector<double> baseline = {10.1, 9.8, 10.3, 10.0, 9.9, 10.2};
detector.fit(baseline);

// Neue Datenpunkte prüfen
std::vector<double> observations = {10.1, 10.3, 45.7, 9.8};
auto results = detector.detect(observations);

for (size_t i = 0; i < results.size(); ++i) {
    if (results[i].is_anomaly) {
        std::cout << "Anomalie bei Index " << i
                  << " (Score: " << results[i].score << ")\n";
    }
}
```

### Isolation Forest für mehrdimensionale Daten

```cpp
AnomalyConfig cfg;
cfg.n_estimators = 100;      // Anzahl Bäume
cfg.contamination = 0.05;    // Erwarteter Ausreißer-Anteil (5%)
cfg.max_samples = 256;

AnomalyDetector detector(AnomalyMethod::ISOLATION_FOREST, cfg);

// Trainingsdaten: Vektoren von Features
std::vector<std::vector<double>> training_data = {
    {1.0, 2.0, 3.0},
    {1.1, 1.9, 3.1},
    // ... viele normale Datenpunkte ...
};
detector.fit(training_data);

// Testpunkte
std::vector<double> test_point = {100.0, 200.0, 300.0};
auto result = detector.detectSingle(test_point);
if (result.is_anomaly) {
    std::cout << "Ausreißer erkannt (Score: " << result.score << ")\n";
}
```

### LOF (Local Outlier Factor)

```cpp
AnomalyConfig cfg;
cfg.n_neighbors = 20;         // k-Nachbarn für Dichteberechnung
cfg.contamination = 0.1;      // 10% Ausreißer erwartet

AnomalyDetector detector(AnomalyMethod::LOF, cfg);
detector.fit(training_data);

auto results = detector.detect(test_data);
```

### Ensemble-Methode

```cpp
AnomalyConfig cfg;
// Gewichte: [ZSCORE, MODIFIED_ZSCORE, IQR, ISOLATION_FOREST, LOF]
cfg.ensemble_weights = {0.2, 0.2, 0.2, 0.2, 0.2};
cfg.adaptive_learning = true;  // Gewichte automatisch anpassen

AnomalyDetector detector(AnomalyMethod::ENSEMBLE, cfg);
detector.fit(baseline_data);

auto results = detector.detect(new_observations);
```

### Adaptives Lernen

Der Ensemble-Detektor kann aus Feedback lernen und seine Gewichte anpassen:

```cpp
// Feedback: Index 5 war eine echte Anomalie, Index 7 war ein False Positive
detector.provideFeedback(5, true);    // true = echte Anomalie
detector.provideFeedback(7, false);   // false = kein Ausreißer
// Ensemble passt interne Gewichte an
```

---

## AnomalyConfig Referenz

| Feld | Standard | Beschreibung |
| --- | --- | --- |
| `threshold` | 3.0 | Z-Score/LOF-Schwellenwert für Anomalie-Flag |
| `n_neighbors` | 20 | LOF: Anzahl der k-Nachbarn |
| `n_estimators` | 100 | Isolation Forest: Anzahl der Bäume |
| `max_samples` | 256 | Isolation Forest: Max. Samples pro Baum |
| `contamination` | 0.1 | Erwarteter Anteil von Ausreißern (0.0–0.5) |
| `adaptive_learning` | false | Ensemble: Gewichte aus Feedback lernen |
| `ensemble_weights` | {} | Ensemble: Manuelle Gewichte pro Methode |

---

## AnomalyResult

```cpp
struct AnomalyResult {
    bool is_anomaly;    // true wenn Ausreißer
    double score;       // Anomalie-Score (höher = anomaler)
    double threshold;   // Verwendeter Schwellenwert
    std::string method; // Verwendeter Algorithmus
};
```

---

## Thread-Sicherheit

- `fit()` — **nicht** thread-safe; Initialisierung vor paralleler Nutzung
- `detect()`, `detectSingle()` — **thread-safe** (read-only nach fit)
- `provideFeedback()` — **thread-safe** (interne Synchronisation)

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

**Last Updated:** 2026-03-09
**Version:** v1.7.0
**Status:** 🟢 Production-Ready
