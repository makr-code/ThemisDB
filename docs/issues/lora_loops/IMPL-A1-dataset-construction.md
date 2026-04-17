---
type: enhancement
labels: ["type:enhancement", "module:training", "priority:high", "status:open", "queue/copilot"]
milestone: v2.0.0
parent: IMPL-EPIC-A
session: S-2
---

# [IMPL-A1] LoRA Foundation: Dataset Construction & Database-Domain AutoLabeler

## Aufgabe

Das `THEMISDB_LORA_RESEARCH_PAPER.md` §4 definiert Phase 1 der Implementierung als
**Dataset Construction** — das Fundament ohne das keine Trainingsschleife laufen kann.
Benötigt wird: ein CLI-Werkzeug zum Aufbau des goldenen Trainings-Datasets aus
historischen Query-Logs, und ein `DatabaseDomainAutoLabeler` der Query-Plans mit
`DomainType` und Konfidenz-Scores annotiert.

## Abgrenzung

| In Scope | Out of Scope |
|---|---|
| `themisdb-cli golden-dataset build` Subcommand | Vollständige CLI-Erweiterung (liegt in api/) |
| `DatabaseDomainAutoLabeler` Klasse | `LegalAutoLabeler` (existiert bereits) |
| `DomainType::DATABASE_OPTIMIZER` Enum-Wert | Neue DomainTypes für andere Branchen |
| `LoRADataSelector::selectDatabaseDomainSamples()` | Änderungen am bestehenden Selector |
| Konfidenz-Funktion: Δp99 → [0.0, 1.0] | ML-basierte Konfidenz-Kalibrierung |
| Goldenes Dataset-Format: JSONL `{query, plan, label, confidence, source}` | Produktions-Trainingspipeline starten |

## Idee / Konzept

Das Paper definiert drei Quellen für Trainingssamples:

```
Quelle 1: Historische Query-Logs  (BaoOptimizer-Decisions + Latenz-Delta)
Quelle 2: DBA-Annotationen        (FeedbackCollector-Einträge)
Quelle 3: Synthetische Queries    (aus bekannten Opt-Patterns generiert)
```

Der `DatabaseDomainAutoLabeler` kombiniert alle drei via:
```
confidence = f(Δp99, sample_age, source_weight)
label      = argmax(DeadlockPredictor | BaoOptimizer | IndexSuggestionEngine signal)
```

**Mindest-Dataset-Größe laut Paper §4.1:** 5.000 Samples pro Domain-Typ
vor Start von Loop 1.

## Technische Details

### DatabaseDomainAutoLabeler

**Neue Datei:** `include/training/database_domain_auto_labeler.h`

```cpp
class DatabaseDomainAutoLabeler {
public:
    struct LabeledSample {
        std::string query_text;
        std::string plan_json;
        DomainType  label;
        double      confidence;  // [0.0, 1.0]
        std::string source;      // "bao_log" | "dba_feedback" | "synthetic"
    };

    // Label einen Query-Plan aus BaoOptimizer-Log
    LabeledSample labelFromBaoDecision(
        const std::string& query,
        const nlohmann::json& bao_plan,
        double delta_p99_ms
    ) const;

    // Label aus FeedbackCollector-Eintrag
    LabeledSample labelFromDBAFeedback(
        const FeedbackEntry& entry
    ) const;

    // Batch-Export aus Query-Log-Datei
    std::vector<LabeledSample> labelFromLogFile(
        const std::string& log_path,
        size_t max_samples = 50000
    ) const;

    // Konfidenz-Funktion: Δp99 → [0.0, 1.0]
    // confidence = sigmoid(|delta_p99_ms| / sensitivity_ms)
    // sensitivity_ms = 10.0 (konfigurierbar)
    double computeConfidence(double delta_p99_ms) const;

private:
    double sensitivity_ms_ = 10.0;
};
```

### CLI-Subcommand

**Datei:** `src/cli/themisctl/` (bestehende CLI-Struktur)

```
themisctl golden-dataset build \
  --log-path /var/log/themis/query.log \
  --feedback-db /var/themis/feedback.db \
  --output golden_dataset.jsonl \
  --min-samples 5000 \
  --min-confidence 0.7
```

**Output-Format (JSONL):**
```json
{"query": "SELECT ...", "plan": {...}, "label": "DATABASE_OPTIMIZER",
 "confidence": 0.87, "source": "bao_log", "delta_p99_ms": -12.4}
```

### DomainType Extension

**Datei:** `include/llm/adapter_registry.h` (oder wo `DomainType` definiert ist)

```cpp
enum class DomainType {
    // ... bestehende Werte ...
    DATABASE_OPTIMIZER,    // neu: Query-Plan-Optimierung
    INDEX_ADVISOR,         // neu: Index-Empfehlungen
    SCHEMA_ADVISOR,        // neu: Schema-Evolution
    SECURITY_MONITOR,      // neu: Anomalie-Erkennung
};
```

## Abhängigkeiten

- **Vorbedingung:** keine (kann sofort starten)
- **Blockiert:** IMPL-A2 (Loop-Orchestration benötigt befülltes Dataset)

## Erfolgskriterien

- [ ] `DatabaseDomainAutoLabeler` Klasse in `include/training/database_domain_auto_labeler.h`
- [ ] `labelFromBaoDecision()` erzeugt `confidence > 0` für |Δp99| > 5ms
- [ ] `labelFromDBAFeedback()` übernimmt Negativ-Feedback als `confidence ≥ 0.9`
- [ ] `computeConfidence(0.0)` gibt `0.5` zurück (sigmoid bei 0)
- [ ] `DomainType::DATABASE_OPTIMIZER` Enum-Wert vorhanden und serialisierbar
- [ ] `themisctl golden-dataset build` Subcommand vorhanden (auch wenn Log-Datei leer)
- [ ] Output-JSONL ist valides JSON (jede Zeile), Felder `query, label, confidence` vorhanden
- [ ] Minimum-Confidence-Filter: Samples mit `confidence < min_confidence` werden gefiltert
- [ ] `labelFromLogFile()` verarbeitet 50k-Zeilen-Log in ≤ 30 s
- [ ] 8 neue Unit-Tests in `tests/test_database_domain_auto_labeler.cpp`

## Definition of Done

`themisctl golden-dataset build --log-path sample.log --output out.jsonl` erzeugt
eine valide JSONL-Datei. 5 von 5 manell verifizierten Testqueries erhalten
das korrekte `DomainType`-Label.
