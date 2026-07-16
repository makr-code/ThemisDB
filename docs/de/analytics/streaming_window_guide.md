# Streaming Windows Guide

**Version:** v1.7.0
**Status:** 🟢 Production-Ready
**Last Updated:** 2026-04-06

---

## Übersicht

ThemisDB's Streaming Window Engine (`streaming_window.cpp`) bietet vier Fenstertypen für
Streaming-Aggregationen mit Watermark-Unterstützung. Streaming Windows sind die Grundlage
für CEP, Echtzeit-Aggregationen und inkrementelle Materialized Views.

**Header:** `include/analytics/streaming_window.h`
**Implementierung:** `src/analytics/streaming_window.cpp`

---

## Fenstertypen

| Typ | Klasse | Beschreibung |
| --- | --- | --- |
| Tumbling Window | `TumblingWindow` | Nicht-überlappende Zeitfenster gleicher Größe |
| Sliding Window | `SlidingWindow` | Überlappende Fenster mit konfigurierbarem Schritt |
| Session Window | `SessionWindow` | Aktivitätsbasierte Fenster mit Timeout |
| Hopping Window | `HoppingWindow` | Regelmäßig vorwärts rückende Fenster |

---

## Tumbling Window

Ereignisse werden in **nicht-überlappende Zeitabschnitte** gleicher Länge gruppiert.
Gut geeignet für periodische Aggregationen (z.B. stündliche Zusammenfassungen).

```text
Zeit:    0----60----120----180----240
         [  Fenster 1  ][  Fenster 2  ]
```

```cpp
#include "analytics/streaming_window.h"
using namespace themisdb::analytics;

// Konfiguration: 60s Fenstergröße
TumblingWindowConfig cfg;
cfg.size = std::chrono::seconds(60);

TumblingWindow window(cfg);

// Aggregation registrieren: COUNT aller Ereignisse
window.addAggregation(AggregateSpec("event_count", AggFunc::COUNT, ""));

// Callback wenn ein Fenster schließt
window.setResultCallback([](WindowResult result) {
    std::cout << "Fenster geschlossen: " << result.record_count
              << " Ereignisse\n";
});

// Ereignisse als StreamRecord einspeisen
StreamRecord rec;
rec.event_time  = std::chrono::system_clock::now();
rec.ingest_time = rec.event_time;
rec.fields["level"] = std::string("ERROR");
window.ingest(rec);

// Am Ende alle offenen Fenster schließen
window.flush();
```

---

## Sliding Window

Ereignisse werden in **überlappende Zeitfenster** gruppiert, die mit einem konfigurierbaren
Schritt vorwärts rücken. Gut für gleitende Durchschnitte.

```text
Zeit:    0----30----60----90----120
         [  Fenster 1 (60s) ]
               [  Fenster 2 (60s) ]
                     [  Fenster 3 (60s) ]
```

```cpp
SlidingWindowConfig cfg;
cfg.size  = std::chrono::seconds(60);   // Fenstergröße
cfg.slide = std::chrono::seconds(30);   // Schrittweite (advance)

SlidingWindow window(cfg);
window.addAggregation(AggregateSpec("avg_latency", AggFunc::AVG, "latency_ms"));
window.setResultCallback([](WindowResult result) { /* ... */ });

StreamRecord rec;
rec.event_time = std::chrono::system_clock::now();
rec.fields["latency_ms"] = 42.5;
window.ingest(rec);
```

---

## Session Window

Ereignisse werden in **aktivitätsbasierte Sitzungen** gruppiert. Ein neues Fenster beginnt,
wenn die Inaktivitätszeit (Timeout) überschritten wird.

```text
Ereignisse: E1 E2  E3          E4 E5
Zeit:       |--|--|--|---------|--|--|
                     Timeout      Neues Session-Fenster
```

```cpp
SessionWindowConfig cfg;
cfg.gap = std::chrono::seconds(30);   // Session-Timeout

SessionWindow window(cfg);
window.addAggregation(AggregateSpec("total", AggFunc::SUM, "amount"));
window.setResultCallback([](WindowResult result) { /* ... */ });

StreamRecord rec;
rec.event_time     = std::chrono::system_clock::now();
rec.partition_key  = "user_alice";  // Sessions werden pro partition_key getrennt
rec.fields["amount"] = 100.0;
window.ingest(rec);

// Session wird automatisch durch den internen Expiry-Thread geschlossen.
// Am Shutdown alle Sessions schließen:
window.flush();
```

---

## Hopping Window

Regelmäßig vorwärts rückende Fenster mit fester Größe und Schrittweite.
Ähnlich wie Sliding Window, aber mit explizit getrennter `hop`-Konfiguration.

```cpp
HoppingWindowConfig cfg;
cfg.size = std::chrono::minutes(10);  // Fenstergröße
cfg.hop  = std::chrono::minutes(1);   // Vorwärtsschritt

HoppingWindow window(cfg);
window.addAggregation(AggregateSpec("peak", AggFunc::MAX, "cpu_usage"));
window.setResultCallback([](WindowResult result) { /* ... */ });
```

---

## Watermark-Unterstützung

Watermarks ermöglichen die Verarbeitung von Ereignissen mit Verzögerung (late events).
Die Konfiguration erfolgt über `WatermarkConfig` im jeweiligen Fenster-Config-Struct:

```cpp
TumblingWindowConfig cfg;
cfg.size = std::chrono::seconds(60);
// Ereignisse bis zu 5s verspätet erlaubt
cfg.watermark.max_out_of_orderness = std::chrono::seconds(5);
// Nach 60s ohne Ereignisse Watermark auf processing-time vorrücken
cfg.watermark.idle_timeout = std::chrono::seconds(60);

TumblingWindow window(cfg);
window.addAggregation(AggregateSpec("cnt", AggFunc::COUNT, ""));
window.setResultCallback([](WindowResult result) { /* ... */ });

// Verspätete Ereignisse werden dem richtigen Fenster zugeordnet,
// solange sie innerhalb von max_out_of_orderness ankommen.
StreamRecord late_rec;
late_rec.event_time = std::chrono::system_clock::now() - std::chrono::seconds(3);
window.ingest(late_rec);
```

---

## Aggregationstypen (`AggFunc`)

| Typ | Enum | Beschreibung |
| --- | --- | --- |
| Anzahl | `AggFunc::COUNT` | Ereignisanzahl im Fenster |
| Summe | `AggFunc::SUM` | Summe eines numerischen Feldes |
| Durchschnitt | `AggFunc::AVG` | Mittelwert eines Feldes |
| Minimum | `AggFunc::MIN` | Kleinstes Wert |
| Maximum | `AggFunc::MAX` | Größtes Wert |
| Standardabweichung | `AggFunc::STDDEV` | Populationsstandardabweichung |
| Varianz | `AggFunc::VARIANCE` | Populationsvarianz |
| Perzentil | `AggFunc::PERCENTILE` | p-tes Perzentil (p via `AggregateSpec::percentile_p`) |

---

## Konfigurationsreferenz

Jeder Fenstertyp hat ein eigenes Konfigurations-Struct:

```cpp
// Tumbling Window
TumblingWindowConfig tcfg;
tcfg.size                         = std::chrono::milliseconds(60000); // Fenstergröße
tcfg.watermark.max_out_of_orderness = std::chrono::milliseconds(5000); // Late-event-Toleranz
tcfg.emit_empty_windows            = false; // Leere Fenster nicht emittieren

// Sliding Window
SlidingWindowConfig scfg;
scfg.size  = std::chrono::milliseconds(60000); // Fenstergröße
scfg.slide = std::chrono::milliseconds(10000); // Schrittgröße

// Session Window
SessionWindowConfig sessioncfg;
sessioncfg.gap = std::chrono::milliseconds(30000); // Inaktivitäts-Timeout

// Hopping Window
HoppingWindowConfig hcfg;
hcfg.size = std::chrono::milliseconds(600000); // Fenstergröße
hcfg.hop  = std::chrono::milliseconds(60000);  // Vorwärtsschritt
```

---

## Integration mit CEP Engine

Streaming Windows sind direkt in die CEP Engine integriert:

```sql
-- EPL-Regel verwendet automatisch einen TumblingWindow
CREATE RULE error_rate AS
SELECT COUNT(*) AS cnt
FROM events
WINDOW TUMBLING 60s
WHERE level = 'ERROR'
HAVING cnt > 50
ACTION alert("errors")
```

→ Weitere EPL-Beispiele: [CEP Guide](./cep_guide.md)

---

## Thread-Sicherheit

- `ingest()` — **thread-safe**; Events können von mehreren Threads eingefügt werden
- `flush()` — **thread-safe**
- `getStats()` — **thread-safe**
- Fensterabschluss und Callback-Aufruf erfolgen innerhalb der `ingest()`-Sperre

---

## Performance

- Tumbling/Hopping: O(1) pro Event, O(w) pro Window-Flush (w = Events im Fenster)
- Sliding: O(log w) pro Event durch geordnete Ereignisliste
- Session: O(1) amortisiert pro Event
- Speicher: ≤ 512 MB pro aktivem Fenstergruppe (konfigurierbar)

---

## Verwandte Dokumentation

- [Analytics Docs Hub](./README.md)
- [CEP Guide](./cep_guide.md)
- [Anomaly Detection Guide](./anomaly_detection_guide.md)
- [API Reference](../../../include/analytics/streaming_window.h)
- [Implementierung](../../../src/analytics/streaming_window.cpp)
- [Roadmap](../../../src/analytics/ROADMAP.md)

---

**Last Updated:** 2026-04-06
**Version:** v1.7.0
**Status:** 🟢 Production-Ready
