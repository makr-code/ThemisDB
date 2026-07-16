---
type: enhancement
labels: ["type:enhancement", "module:storage", "module:training", "priority:medium", "status:open", "queue/copilot"]
milestone: v2.0.0
parent: IMPL-EPIC-B
session: S-8
layer: 10
---

# [IMPL-B10] Layer 10: StorageLayoutAdvisor

## Aufgabe

`LLM_OPTIMIZATION_LAYERS_MATRIX.md` §Layer-10 definiert den `StorageLayoutAdvisor`:
Er analysiert Collections und empfiehlt das optimale physische Speicherlayout
(Row vs. Columnar vs. Hybrid) basierend auf dem semantischen Datentyp und
dem Zugriffsmuster — mit besonderem Fokus auf Zeitreihen und Kompressions-Potential.

## Abgrenzung

| In Scope | Out of Scope |
|---|---|
| `StorageLayoutAdvisor` Klasse | Automatische Datenmigration |
| `LayoutRecommendation` Struct mit Kompressions-Ratio-Schätzung | Neue Storage-Engine |
| GDPR-Schutz: kein Layout-Wechsel für GDPR-Felder ohne DBA-Approval | Layout-Execution |
| `AIDecisionAuditor`-Integration | Cross-Shard-Layout-Propagation (→ DK-2 Layer 11A) |
| Zeitreihen-Erkennung: ARIMA + Fourier (analog IMPL-B6) | Neue Zeitreihen-Datenbank |

## Idee / Konzept

```
Collection "sensor_readings":
  schema: {timestamp: DateTime, sensor_id: String, temperature: Float, humidity: Float}
  access: 95% range-scan auf timestamp + aggregation
  → StorageLayoutAdvisor: COLUMNAR_COMPRESSED (Parquet-ähnlich)
  → estimated_compression_ratio: 8.5x (Float-Spalten komprimieren gut)
  → estimated_query_speedup: +340%

Collection "user_sessions":
  schema: {session_id: UUID, user_id: Int, data: JSON, created_at: DateTime}
  access: 80% point-lookup auf session_id
  → StorageLayoutAdvisor: ROW_ORIENTED (UUID-Lookup braucht Zeilen)
  → estimated_compression_ratio: 1.2x (JSON komprimiert mäßig)

Collection "financial_transactions":
  schema: gemischt: Metadaten + BLOB-Payload
  access: 60% Metadaten-Only, 40% Full-Row
  → StorageLayoutAdvisor: HYBRID (Metadaten columnar, BLOB row)
```

**Performance-Target aus Paper §Layer-10:**
≥ +50 % Kompressionsverbesserung nach Advisory für Zeitreihen-Collections.

## Technische Details

```cpp
// Neue Datei: include/storage/storage_layout_advisor.h
class StorageLayoutAdvisor {
public:
    enum class LayoutType {
        ROW_ORIENTED,          // Standard — gut für Point-Lookups
        COLUMNAR_COMPRESSED,   // Gut für Aggregationen, Zeitreihen
        HYBRID,                // Metadaten columnar, Payload row
        TIERED                 // Hot/Warm/Cold Tiering-Empfehlung
    };

    struct LayoutRecommendation {
        std::string  collection_name;
        LayoutType   recommended_layout;
        LayoutType   current_layout;       // ROW_ORIENTED als Default
        double       estimated_compression_ratio;  // > 1.0 = Verbesserung
        double       estimated_query_speedup;      // Faktor, z.B. 3.4 = +240%
        double       confidence;                   // [0.0, 1.0]
        bool         gdpr_approval_required;       // wenn GDPR-Felder betroffen
        std::string  rationale;                    // Begründung in 1 Satz
    };

    // Analysiere eine Collection
    LayoutRecommendation analyze(
        const std::string& collection_name,
        const CollectionAccessStats& stats,  // query_types, access_patterns
        const SchemaInfo& schema,            // Feldtypen, GDPR-Tags
        const GdprFieldRegistry& gdpr_fields
    ) const;

    // Erkennt ob Collection Zeitreihenmuster zeigt (Fourier — analog IMPL-B6)
    bool isTimeSeries(const CollectionAccessStats& stats) const;
};
```

### Entscheidungslogik

```
if isTimeSeries(stats) AND aggregation_ratio > 0.7:
    → COLUMNAR_COMPRESSED
elif point_lookup_ratio > 0.8:
    → ROW_ORIENTED
elif blob_field_exists AND metadata_only_access_ratio > 0.5:
    → HYBRID
else:
    → ROW_ORIENTED (safe default)
```

## Abhängigkeiten

- **Vorbedingung:** keine
- **Verbessert durch:** IMPL-B6 (teilt Zeitreihen-Erkennungslogik)
- **Parallel möglich mit:** IMPL-B5, IMPL-B7, IMPL-B8, IMPL-B9

## Erfolgskriterien

- [ ] `StorageLayoutAdvisor` Klasse vorhanden
- [ ] `analyze()` empfiehlt `COLUMNAR_COMPRESSED` für Zeitreihen mit hohem Aggregations-Anteil
- [ ] `analyze()` empfiehlt `ROW_ORIENTED` für UUID-Point-Lookup-Pattern
- [ ] `estimated_compression_ratio >= 5.0` für Float-Zeitreihen (physikalisch plausibel)
- [ ] GDPR-Felder: `gdpr_approval_required = true` wenn Layout-Wechsel GDPR-Felder betrifft
- [ ] `isTimeSeries()` gibt true für monoton steigende Timestamp + Float-Payload
- [ ] Schreibt `DecisionRecord{decision_type="LAYOUT_RECOMMENDATION"}` in `AIDecisionAuditor`
- [ ] Layer-10-Performance-Target: `analyze()` ≤ 10 s Batch, nicht auf kritischem Pfad
- [ ] 10 neue Tests in `tests/test_storage_layout_advisor.cpp`
- [ ] `LayoutRecommendation::rationale` nicht leer (DBA-lesbarer Satz)

## Definition of Done

Zeitreihen-Collection mit 5 Float-Feldern → `COLUMNAR_COMPRESSED` mit
`estimated_compression_ratio >= 5.0`. UUID-Collection → `ROW_ORIENTED`.
Alle 10 Tests grün.
