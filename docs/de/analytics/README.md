# Analytics Module — Dokumentation

**Version:** 1.9.0
**Status:** 🟢 Production-Ready
**Last Updated:** 2026-04-06

---

## Übersicht

Das Analytics-Modul verwandelt ThemisDB von einer transaktionalen Datenbank in eine vollständige
Analyseplattform. Es bietet OLAP-Abfrageverarbeitung, Process Mining, NLP-Textanalyse,
Zeitreihenprognose, ML-Integration, Anomalieerkennung und Complex Event Processing.

**Primäre Quellen:** [`src/analytics/`](../../../src/analytics/) · [`include/analytics/`](../../../include/analytics/)

---

## Inhalt dieser Dokumentation

| Dokument | Beschreibung | Status |
| --- | --- | --- |
| [OLAP Guide](./olap_guide.md) | GROUP BY, CUBE, ROLLUP, Window-Funktionen | 🟢 Fertig |
| [Process Mining Guide](./process_mining_guide.md) | Prozessentdeckung, Konformitätsprüfung | 🟢 Fertig |
| [Forecasting Guide](./forecasting_guide.md) | Zeitreihenprognose (ARIMA, Holt-Winters, ENSEMBLE, Batch-Prognose, inkrementelles Update) | 🟢 Fertig |
| [CEP Guide](./cep_guide.md) | Complex Event Processing, EPL-Regeln, Alerting | 🟢 Fertig |
| [Anomaly Detection Guide](./anomaly_detection_guide.md) | Z-Score, IQR, LOF, Isolation Forest, Ensemble | 🟢 Fertig |
| [Streaming Windows Guide](./streaming_window_guide.md) | Tumbling, Sliding, Session, Hopping Windows | 🟢 Fertig |
| [ML & AutoML Guide](./ml_guide.md) | AutoML, ONNX, TensorFlow Serving, Model Registry | 🟢 Fertig |
| [NLP Text Analyzer](./NLP_TEXT_ANALYZER.md) | Tokenisierung, TF-IDF, NER, Sentiment | 🟢 Fertig |
| [Process Mining AQL Examples](./PROCESS_MINING_AQL_EXAMPLES.md) | AQL-Abfragebeispiele | 🟢 Fertig |
| [Process Mining Research & Roadmap](./PROCESS_MINING_RESEARCH_AND_ROADMAP.md) | Forschungsgrundlagen | 📋 Referenz |
| [BPMN Future Enhancements](./bpmn_future_enhancements.md) | Geplante BPMN-Erweiterungen | 📋 Geplant |

---

## Schnellstart

### 1. OLAP-Abfragen

```cpp
#include "analytics/olap.h"
using namespace themis::analytics;

OLAPQuery query;
query.collection = "sales";
query.dimensions.push_back({"region", "", true});
query.measures.push_back({"revenue", "amount", Measure::Function::Sum});

OLAPEngine engine;
auto result = engine.execute(query);
```

→ [Vollständiger OLAP Guide](./olap_guide.md)

### 2. Zeitreihenprognose

```cpp
#include "analytics/forecasting.h"
using namespace themisdb::analytics;

TimeSeries ts;
// ... Datenpunkte hinzufügen ...
ForecastModel model(ForecastMethod::HOLT_WINTERS);
model.fit(ts);
auto forecast = model.predict(7);
```

> **Neu in v1.9.0:** `predictBatch()` prognostiziert N Zeitreihen in einem einzigen Aufruf
> und reduziert den Overhead für Batch-Szenarien erheblich.

→ [Vollständiger Forecasting Guide](./forecasting_guide.md)

### 3. Complex Event Processing

```cpp
// EPL-Regel erstellen
std::string epl = R"(
  CREATE RULE high_error_rate AS
  SELECT COUNT(*) AS cnt
  FROM events
  WINDOW TUMBLING 60s
  WHERE level = 'ERROR'
  HAVING cnt > 100
  ACTION alert("high-error-rate")
)";
CEPEngine& engine = CEPEngine::getInstance();
engine.initialize(CEPConfig{});
engine.addRuleFromEPL(epl);
```

→ [Vollständiger CEP Guide](./cep_guide.md)

---

## Modulstruktur

```text
src/analytics/           ← Implementierungsdateien
  README.md              ← Implementierungsübersicht
  ARCHITECTURE.md        ← Architekturleitfaden
  ROADMAP.md             ← Roadmap & Status
  FUTURE_ENHANCEMENTS.md ← Geplante Erweiterungen
  olap.cpp               ← OLAP Engine
  cep_engine.cpp         ← CEP Engine
  forecasting.cpp        ← Zeitreihenprognose
  anomaly_detection.cpp  ← Anomalieerkennung
  streaming_window.cpp   ← Streaming Windows
  process_mining.cpp     ← Process Mining
  nlp_text_analyzer.cpp  ← NLP Textanalyse
  automl.cpp             ← AutoML
  model_serving.cpp      ← Modell-Registry & Inferenz
  ml_serving.cpp         ← ONNX/TF-Serving Integration
  ...

include/analytics/       ← Öffentliche Header
  README.md              ← API-Übersicht
  FUTURE_ENHANCEMENTS.md ← API-Erweiterungen
  olap.h
  cep_engine.h
  forecasting.h
  anomaly_detection.h
  streaming_window.h
  ...
```

---

## Primäre Dokumentation

| Dokument | Pfad | Beschreibung |
| --- | --- | --- |
| Implementierungsübersicht | [`src/analytics/README.md`](../../../src/analytics/README.md) | Alle Implementierungsdateien, Komponenten |
| Architektur | [`src/analytics/ARCHITECTURE.md`](../../../src/analytics/ARCHITECTURE.md) | Design-Prinzipien, Datenfluss, Integrationen |
| Roadmap | [`src/analytics/ROADMAP.md`](../../../src/analytics/ROADMAP.md) | Status, Phasen, geplante Features |
| Impl. Future Enhancements | [`src/analytics/FUTURE_ENHANCEMENTS.md`](../../../src/analytics/FUTURE_ENHANCEMENTS.md) | Implementierungsspezifische Erweiterungen |
| API-Übersicht | [`include/analytics/README.md`](../../../include/analytics/README.md) | Öffentliche C++-Interfaces |
| API Future Enhancements | [`include/analytics/FUTURE_ENHANCEMENTS.md`](../../../include/analytics/FUTURE_ENHANCEMENTS.md) | Geplante API-Erweiterungen |

---

**Last Updated:** 2026-04-06
**Version:** v1.9.0
