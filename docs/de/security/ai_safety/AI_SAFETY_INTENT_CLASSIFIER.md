# 🔎 IntentClassifier — AQL-Erweiterung (Schicht 4)

> **Erweiterung von** `include/security/intent_classifier.h` und
> `src/security/intent_classifier.cpp`
>
> Bestehende Klasse: Schicht-7 LLM-Sicherheitsanalyse (SQL-Injection, Datenexfiltration,
> Privilege-Escalation). Neue Klassen: `DATA_DESTRUCTION` und `SCHEMA_MUTATION` für
> AQL-native destruktive Muster.

---

## Hintergrund

Der bestehende `IntentClassifier` erkennt SQL-Injection-Muster (`; DROP`, `UNION SELECT`)
und klassische Datenbankangriffspatterns. AQL-native destruktive Operationen wie
`FOR x IN users REMOVE x IN users` oder `DROP COLLECTION` werden heute als `LEGITIMATE`
klassifiziert — weil sie keine SQL-Injection-Syntaxmerkmale haben.

Diese Lücke schließt Schicht 4.

---

## Neue IntentType-Werte

```cpp
// include/security/intent_classifier.h

enum class IntentType {
    LEGITIMATE,              // Normale, gutartige Anfrage
    SQL_INJECTION,           // SQL-Injection-Muster erkannt
    DATA_EXFILTRATION,       // Massendaten-Extraktionsversuch
    PRIVILEGE_ESCALATION,    // Zugriff auf Out-of-Scope-Ressourcen
    ANOMALOUS_PATTERN,       // Ungewöhnliche Struktur
    DATA_DESTRUCTION,        // NEU: AQL REMOVE / Bulk-Delete
    SCHEMA_MUTATION,         // NEU: DROP COLLECTION / TRUNCATE / DDL
};
```

---

## Feature-Gewichte (AQL-Destruktion)

### DATA_DESTRUCTION Features

| Pattern (case-insensitive) | Gewicht | Rationale |
|---|---|---|
| `" REMOVE "` | 0.45 | REMOVE-Keyword (kontextuell) |
| `"FOR "` + `" REMOVE "` | 0.70 | Batch-Delete-Pattern |
| `FOR...IN...REMOVE` ohne `FILTER` | 0.90 | Vollbereichs-Delete (maximal destruktiv) |
| `"REMOVE @"` (Bind-Var) | 0.40 | Parametrisiertes Delete (oft legitim) |
| `"DELETE "` | 0.30 | SQL-artiges DELETE (in Mixed-Workloads) |

### SCHEMA_MUTATION Features

| Pattern | Gewicht | Rationale |
|---|---|---|
| `"DROP COLLECTION"` | 0.95 | Irreversibler Schema-Verlust |
| `"DROP INDEX"` | 0.55 | Performance-Auswirkung |
| `"TRUNCATE"` | 0.80 | Irreversibler Datenverlust |
| `"CREATE COLLECTION"` | 0.15 | Meist legitim, leicht erhöhtes Risiko |
| `"RENAME COLLECTION"` | 0.35 | Potenziell disruptiv |

---

## Erkennungslogik (Pseudocode)

```cpp
// Ergänzung in src/security/intent_classifier.cpp — scoreFeatures()

// Bestehende Feature-Tabellen:
// kSqlInjectionFeatures, kExfiltrationFeatures, kPrivEscFeatures

// NEU:
static const Feature kDataDestructionFeatures[] = {
    {" REMOVE ",                          0.45},
    {"FOR \" + \" REMOVE ",               0.70},
    {"DROP COLLECTION",                   0.10},  // handled by SCHEMA_MUTATION
    {"DELETE ",                           0.30},
    {"REMOVE @",                          0.40},
};

static const Feature kSchemaMutationFeatures[] = {
    {"DROP COLLECTION",                   0.95},
    {"DROP INDEX",                        0.55},
    {"TRUNCATE",                          0.80},
    {"CREATE COLLECTION",                 0.15},
    {"RENAME COLLECTION",                 0.35},
};

// Spezialregel: FOR...REMOVE ohne FILTER
double checkBulkRemove(const std::string& upperQuery) {
    bool hasFor    = upperQuery.find("FOR ") != std::string::npos;
    bool hasRemove = upperQuery.find(" REMOVE ") != std::string::npos;
    bool hasFilter = upperQuery.find(" FILTER ") != std::string::npos;
    if (hasFor && hasRemove && !hasFilter) {
        return 0.90;  // Vollbereichs-Delete — maximal destruktiv
    }
    return 0.0;
}
```

---

## Blockierungsschwellenwert

| Klasse | Schwellenwert | Aktion |
|---|---|---|
| `DATA_DESTRUCTION` | Confidence ≥ 0.65 | Operation blockiert, Audit-Event erzeugt |
| `SCHEMA_MUTATION` | Confidence ≥ 0.65 | Operation blockiert, Audit-Event erzeugt |
| `SQL_INJECTION` | Confidence ≥ 0.85 | (bestehend) blockiert |
| `DATA_EXFILTRATION` | Confidence ≥ 0.85 | (bestehend) blockiert |

Der niedrigere Schwellenwert (0.65 statt 0.85) für destruktive AQL-Operationen ist bewusst
gewählt: Destruktive Operationen sind irreversibel, daher ist ein höherer False-Positive-Anteil
akzeptabel gegenüber einem False-Negative.

---

## Integration mit DOG + HILG

Der IntentClassifier ist eine **zusätzliche Sicherheitsschicht** zum DOG, nicht dessen Ersatz:

```
Query kommt an
     │
     ├──> IntentClassifier.classify()
     │    • Confidence DATA_DESTRUCTION = 0.82 ≥ 0.65 → blockieren
     │    • → Audit-Event AI_INTENT_BLOCKED erzeugen
     │
     ├──> AiOperationGuard.evaluate()
     │    • Unabhängige Klassifikation → DESTRUCTIVE
     │    • → HILG Approval-Flow
     │
     └──> Wenn beide bestehen → Ausführung (nach Approval + Snapshot)
```

**Redundanz ist gewollt:** Kein einzelner Guard soll ein Single Point of Failure sein.

---

## Neue riskDelta-Werte

```cpp
// src/security/intent_classifier.cpp — riskDelta()
double IntentClassifier::riskDelta(IntentType t) noexcept {
    switch (t) {
        case IntentType::SQL_INJECTION:        return 0.35;
        case IntentType::DATA_EXFILTRATION:    return 0.25;
        case IntentType::PRIVILEGE_ESCALATION: return 0.30;
        case IntentType::ANOMALOUS_PATTERN:    return 0.10;
        case IntentType::DATA_DESTRUCTION:     return 0.50;  // NEU: höchster Delta
        case IntentType::SCHEMA_MUTATION:      return 0.45;  // NEU
        default:                               return 0.0;
    }
}
```

---

## Neue intentName()-Werte

```cpp
// src/security/intent_classifier.cpp — intentName()
std::string IntentClassifier::intentName(IntentType t) {
    switch (t) {
        case IntentType::LEGITIMATE:            return "LEGITIMATE";
        case IntentType::SQL_INJECTION:         return "SQL_INJECTION";
        case IntentType::DATA_EXFILTRATION:     return "DATA_EXFILTRATION";
        case IntentType::PRIVILEGE_ESCALATION:  return "PRIVILEGE_ESCALATION";
        case IntentType::ANOMALOUS_PATTERN:     return "ANOMALOUS_PATTERN";
        case IntentType::DATA_DESTRUCTION:      return "DATA_DESTRUCTION";  // NEU
        case IntentType::SCHEMA_MUTATION:       return "SCHEMA_MUTATION";   // NEU
        default:                                return "UNKNOWN";
    }
}
```

---

## Testfälle (Geplant: `tests/security/test_intent_classifier_aql.cpp`)

| Test-ID | Query | Erwarteter IntentType | Min. Confidence |
|---|---|---|---|
| IC-AQL-01 | `FOR u IN users RETURN u` | `LEGITIMATE` | — |
| IC-AQL-02 | `FOR u IN users REMOVE u IN users` | `DATA_DESTRUCTION` | 0.85 |
| IC-AQL-03 | `FOR u IN users FILTER u.id==1 REMOVE u IN users` | `DATA_DESTRUCTION` | 0.65 |
| IC-AQL-04 | `DROP COLLECTION users` | `SCHEMA_MUTATION` | 0.90 |
| IC-AQL-05 | `TRUNCATE users` | `SCHEMA_MUTATION` | 0.75 |
| IC-AQL-06 | `DROP INDEX users.email` | `SCHEMA_MUTATION` | 0.50 |
| IC-AQL-07 | `REMOVE @key IN users` | `DATA_DESTRUCTION` | 0.35 (unter Schwelle!) |
| IC-AQL-08 | `FOR u IN users FILTER u.name LIKE "%REMOVE%" RETURN u` | `LEGITIMATE` | — |
| IC-AQL-09 | `INSERT {name:"x"} INTO users` | `LEGITIMATE` (kein Destruktionsmuster) | — |

> **Hinweis zu IC-AQL-07:** `REMOVE @key` liegt mit Confidence ~0.38 unter dem
> Blockierungsschwellenwert von 0.65 → wird durch den DOG (Schicht 1) abgefangen,
> nicht durch den IntentClassifier.

---

## Zukunft: LoRA-Adapter (IMPL-A2)

Die aktuelle regelbasierte Klassifikation ist ein Placeholder (STUB/SIMULATION NOTE vorhanden).
Phase 4 ersetzt die internen `scoreFeatures()`-Aufrufe durch einen LoRA-Adapter:

```
Ziel-Precision: ≥ 92% (vs. ~80% regelbasiert)
Trainingsdaten: Annotierte AQL-Queries (destruktiv / legitim)
Modell: LoRA-finetuned auf ThemisDB-spezifische AQL-Patterns
Aktivierung: IMPL-A2 Loop-1 (Q4 2026)
```

Roadmap-Referenz: `src/security/FUTURE_ENHANCEMENTS.md` — IMPL-A2

---

## Roadmap-Verknüpfung

- **ASL-1:** AQL-Patterns im IntentClassifier → Q2 2026 (Phase 1 — Kritisch)
- **ASL-13:** LoRA-Adapter für IntentClassifier → Q4 2026 (Phase 4)
