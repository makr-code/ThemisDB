# Streaming Windows Guide

**Version:** v1.7.0
**Status:** 🟢 Production-Ready
**Last Updated:** 2026-03-09

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

TumblingWindow window(
    std::chrono::seconds(60),   // Fenstergröße: 60 Sekunden
    AggregationType::COUNT      // Aggregation: COUNT
);

// Ereignisse hinzufügen
window.add(event1);
window.add(event2);

// Aktuelle Fensterergebnisse abrufen
auto results = window.flush();
for (const auto& result : results) {
    std::cout << "Window [" << result.start_ms << ", " << result.end_ms << "]: "
              << result.value << "\n";
}
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
SlidingWindow window(
    std::chrono::seconds(60),   // Fenstergröße
    std::chrono::seconds(30),   // Schritt (advance)
    AggregationType::AVG        // Aggregation: Durchschnitt
);

window.add(event);
auto results = window.getActiveWindows();
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
SessionWindow window(
    std::chrono::seconds(30),   // Session-Timeout
    AggregationType::SUM        // Aggregation
);

window.add(event);

// Session wird abgeschlossen wenn kein Event für 30s kommt
// oder manuell via:
auto completed = window.closeExpiredSessions(currentTimeMs());
```

---

## Hopping Window

Regelmäßig vorwärts rückende Fenster mit fester Größe und Schrittweite.
Ähnlich wie Sliding Window, aber mit regelmäßigem Vorwärtsrücken.

```cpp
HoppingWindow window(
    std::chrono::minutes(10),   // Fenstergröße
    std::chrono::minutes(1),    // Vorwärtsschritt
    AggregationType::MAX        // Aggregation: Maximum
);
```

---

## Watermark-Unterstützung

Watermarks ermöglichen die Verarbeitung von Ereignissen mit Verzögerung (late events):

```cpp
TumblingWindow window(std::chrono::seconds(60), AggregationType::COUNT);

// Watermark setzen: Ereignisse bis zu 5s verspätet erlaubt
window.setWatermark(std::chrono::seconds(5));

// Verspätete Ereignisse werden noch dem richtigen Fenster zugeordnet,
// solange sie nicht mehr als watermark_delay nach Fensterabschluss ankommen
window.add(lateEvent);
```

---

## Aggregationstypen

| Typ | Enum | Beschreibung |
| --- | --- | --- |
| Anzahl | `COUNT` | Ereignisanzahl im Fenster |
| Summe | `SUM` | Summe eines numerischen Feldes |
| Durchschnitt | `AVG` | Mittelwert eines Feldes |
| Minimum | `MIN` | Kleinstes Wert |
| Maximum | `MAX` | Größtes Wert |
| Standardabweichung | `STDDEV` | Populationsstandardabweichung |
| Varianz | `VARIANCE` | Populationsvarianz |

---

## Konfigurationsreferenz

```cpp
WindowConfig cfg;
cfg.size_ms       = 60000;    // Fenstergröße in Millisekunden
cfg.advance_ms    = 10000;    // Schrittgröße (Sliding/Hopping)
cfg.timeout_ms    = 30000;    // Session-Timeout
cfg.watermark_ms  = 5000;     // Erlaubte Ereignisverzögerung
cfg.max_events    = 100000;   // Max. Events pro Fenster (Speicherschutz)
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

- `add()` — **thread-safe**; Events können von mehreren Threads eingefügt werden
- `flush()`, `getActiveWindows()` — **thread-safe**
- Fensterabschluss und Aggregation läuft in dediziertem Hintergrundthread

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

**Last Updated:** 2026-03-09
**Version:** v1.7.0
**Status:** 🟢 Production-Ready
