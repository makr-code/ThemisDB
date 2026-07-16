---
type: enhancement
labels: ["type:enhancement", "module:storage", "module:training", "priority:medium", "status:open", "queue/copilot"]
milestone: v2.0.0
parent: IMPL-EPIC-B
session: S-6
layer: 6
---

# [IMPL-B6] Layer 6: SchemaDeadWeightDetector

## Aufgabe

`LLM_OPTIMIZATION_LAYERS_MATRIX.md` §Layer-6 definiert den `SchemaDeadWeightDetector`:
Er identifiziert Felder, Collections und Indizes die über einen rollierenden
180-Tage-Zeitraum nie gelesen werden — unter Berücksichtigung von Saisonalität
und (ab Layer 11) shard-übergreifender Nutzungsaggregation. Das Ergebnis ist
ein DBA-Advisory-Report, kein automatisches DROP.

## Abgrenzung

| In Scope | Out of Scope |
|---|---|
| `SchemaDeadWeightDetector` Klasse | Automatische DDL-Execution |
| 180-Tage-Rolling-Window mit ARIMA-Saisonalitäts-Erkennung | Neue ARIMA-Bibliothek — einfache Fourier-Approximation reicht |
| GDPR-Tag-Schutz: GDPR-Felder niemals als Dead-Weight | Neue GDPR-Infrastruktur |
| `DeadWeightReport` mit Konfidenz-Score | Shard-übergreifende Aggregation (→ DK-4 Layer 11) |
| `AIDecisionAuditor`-Integration | Schema-Migration-Execution (liegt in OnlineSchemaMigration) |

## Idee / Konzept

```
Feld f: letzte Leseoperation vor 240 Tagen
→ Naiver Detector: Dead-Weight (schläge DROP vor)

Aber: Feld f ist das GDPR-Export-Feld (nur jährlich abgerufen)
→ GDPR-Tag verhindert Dead-Weight-Einstufung ✓

Feld g: letzte Leseoperation vor 200 Tagen
→ ARIMA-Saisonalitätsprüfung: Spitzenlast alle 90 Tage (Quartalsabschluss)
→ nächste Spitze in 70 Tagen erwartet → KEIN Dead-Weight ✓

Feld h: letzte Leseoperation vor 365 Tagen, keine GDPR, keine Saisonalität
→ Dead-Weight mit confidence=0.95 → im Report
```

**Konfidenz-Formel:**
```
confidence = (days_since_access / 180) * (1 - seasonality_score) * (1 - gdpr_protection)
```

## Technische Details

```cpp
// Neue Datei: include/storage/schema_dead_weight_detector.h
class SchemaDeadWeightDetector {
public:
    struct DeadWeightCandidate {
        std::string field_path;       // "collection.field_name"
        double      confidence;       // [0.0, 1.0]
        uint32_t    days_since_access;
        bool        gdpr_protected;   // niemals true wenn confidence > 0
        double      seasonality_score;// [0.0, 1.0] — hohe Saisonalität → niedriger confidence
        std::string recommendation;   // "archive" | "drop_index" | "deprecate"
    };

    struct DeadWeightReport {
        std::vector<DeadWeightCandidate> candidates;
        std::chrono::system_clock::time_point generated_at;
        uint32_t analysis_window_days = 180;
        size_t   total_fields_analyzed;
        size_t   gdpr_protected_skipped;
    };

    DeadWeightReport analyze(
        const SchemaAccessStats& stats,       // Zugriffs-Zeitreihen pro Feld
        const GdprFieldRegistry& gdpr_fields  // GDPR-geschützte Felder
    ) const;

    // Saisonalitätserkennung via Fourier-Koeffizienten (k=3 harmonics)
    double computeSeasonalityScore(
        const std::vector<std::pair<std::chrono::system_clock::time_point, uint64_t>>& access_series
    ) const;
};
```

## Abhängigkeiten

- **Vorbedingung:** IMPL-A2 (Loop-3 ruft `SchemaDeadWeightDetector` auf)
- **Parallel möglich mit:** IMPL-B5, IMPL-B7, IMPL-B8

## Erfolgskriterien

- [ ] `SchemaDeadWeightDetector` Klasse vorhanden
- [ ] `analyze()` gibt 0 Kandidaten für GDPR-geschützte Felder (0-Fehler-Invariante)
- [ ] `analyze()` gibt 0 Kandidaten für saisonale Felder mit `seasonality_score > 0.7`
- [ ] `confidence > 0.8` für Felder ohne Zugriff seit 365 Tagen
- [ ] `recommendation = "archive"` vs `"drop_index"` korrekt nach Feldtyp unterschieden
- [ ] `DeadWeightReport` schreibt `DecisionRecord` in `AIDecisionAuditor`
- [ ] `computeSeasonalityScore()` gibt > 0.5 für monatliche Zugriffsmuster
- [ ] 10 neue Tests in `tests/test_schema_dead_weight_detector.cpp`
- [ ] Layer-6-Performance: `analyze(1000 Felder)` ≤ 10 s (Batch-Job, nicht kritischer Pfad)

## Definition of Done

Test-Dataset mit 10 Feldern (3 GDPR, 2 saisonal, 5 echte Dead-Weights):
`analyze()` findet genau 5 Kandidaten. 0 GDPR-Felder im Report.
