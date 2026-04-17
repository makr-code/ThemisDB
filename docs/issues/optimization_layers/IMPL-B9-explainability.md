---
type: enhancement
labels: ["type:enhancement", "module:rag", "module:llm", "priority:medium", "status:open", "queue/copilot"]
milestone: v2.0.0
parent: IMPL-EPIC-B
session: S-8
layer: 9
---

# [IMPL-B9] Layer 9: ExplainabilityReasonBuilder

## Aufgabe

`LLM_OPTIMIZATION_LAYERS_MATRIX.md` §Layer-9 definiert den `ExplainabilityReasonBuilder`:
**Jede autonome Entscheidung** des ThemisDB-Systems muss für den DBA in natürlicher
Sprache erklärbar sein — Kausalkette von Signal → Analyse → Entscheidung → Konfidenz.
Der `ExplainabilityReasonBuilder` erzeugt diese Erklärungen aus `DecisionRecord`-Einträgen.

## Abgrenzung

| In Scope | Out of Scope |
|---|---|
| `ExplainabilityReasonBuilder` Klasse | DBA-Chat-Interface (liegt in API) |
| `CausalChain` Struct (Signal → Analyse → Entscheidung → Konfidenz) | Visuelle Erklärungen / Diagramme |
| `AIDecisionAuditor`-Integration: enriche DecisionRecords mit Causal Chain | Neue LLM-Modelle |
| Template-basierte Erklärungsgenerierung (kein LLM-Aufruf nötig) | Mehrsprachige Erklärungen (EN only) |
| 100 %-Coverage: alle Entscheidungstypen haben Template | Vollständige NL-Generierung via LLM |
| `FederatedAIDecisionAuditor`-Erweiterung: cross-shard Timeline | Separate Audit-UI |

## Idee / Konzept

Jede autonome Aktion produziert einen `DecisionRecord` (bereits implementiert im
`AIDecisionAuditor`). Der `ExplainabilityReasonBuilder` ergänzt diesen Record um
eine strukturierte kausale Erklärung:

```
DecisionRecord{decision_type="HNSW_PARAMS_UPDATED", ef_construction=200, M=16, recall=0.89}
→ ExplainabilityReasonBuilder.build(record) →
CausalChain{
  signal:   "HNSW recall@10 dropped to 0.89 (threshold: 0.93) over last 7 days",
  analysis: "Index distribution drift detected: new documents in 'legal_contracts'
             collection shifted embedding centroid by 0.18 cosine units",
  decision: "Retrained LoRA adapter (Loop 1); updated ef_construction from 128→200,
             M from 8→16 to restore recall",
  confidence: 0.91,
  impact:   "Estimated +8 % recall improvement; +12 ms index build time (acceptable)",
  dba_action_required: false
}
```

**Template-Bibliothek:** Ein Template pro `decision_type` — 15+ Templates
decken alle Entscheidungstypen der Layers 1–11 ab.

## Technische Details

```cpp
// Neue Datei: include/rag/explainability_reason_builder.h
class ExplainabilityReasonBuilder {
public:
    struct CausalChain {
        std::string signal;              // Was hat ausgelöst
        std::string analysis;            // Warum ist das relevant
        std::string decision;            // Was wurde entschieden
        double      confidence;          // Wie sicher ist die Entscheidung
        std::string impact;              // Was ist die erwartete Wirkung
        bool        dba_action_required; // Muss der DBA aktiv werden
        std::string decision_type;       // Original decision_type aus AIDecisionAuditor
    };

    // Baue kausale Erklärung aus DecisionRecord
    CausalChain build(const AIDecisionRecord& record) const;

    // Formatiere als lesbarer Text (für REST-API, CLI, DBA-Dialog)
    std::string toNaturalLanguage(const CausalChain& chain) const;

    // Batch: alle unkomentierten Records im Auditor enrichen
    size_t enrichAuditor(AIDecisionAuditor& auditor) const;
};
```

### Template-Coverage (Pflicht für alle Layers 1–11)

| decision_type | Layer | Template-Status |
|---|---|---|
| `HNSW_PARAMS_UPDATED` | L1 | required |
| `BAO_PLAN_SELECTED` | L1 | required |
| `TX_SEMANTIC_HINT` | L5 | required |
| `SCHEMA_DEAD_WEIGHT_REPORT` | L6 | required |
| `INTENT_ALERT` | L7 | required |
| `WORKLOAD_FINGERPRINT` | L8 | required |
| `FEDERATED_ROUND` | L11B | required |
| `FEDERATED_DELTA_APPLIED` | L11B | required |
| `INTENT_ALERT` (cross-shard) | L7+L11A | required |
| *(+ alle anderen bestehenden types)* | | required |

## Abhängigkeiten

- **Vorbedingung:** IMPL-A2 (alle LoopPhase-Typen bekannt)
- **Parallel möglich mit:** IMPL-B5…B8

## Erfolgskriterien

- [ ] `ExplainabilityReasonBuilder` Klasse vorhanden
- [ ] `build()` gibt `CausalChain` mit nicht-leerem `signal` + `decision` für alle definierten Types
- [ ] `build()` mit unbekanntem `decision_type` → fallback-Template (kein Crash)
- [ ] `toNaturalLanguage()` gibt Text ≥ 50 und ≤ 500 Wörter zurück
- [ ] `dba_action_required = true` für `INTENT_ALERT` mit confidence > 0.9
- [ ] `dba_action_required = false` für `HNSW_PARAMS_UPDATED` mit guardrail_passed=true
- [ ] `enrichAuditor()` gibt Anzahl enrichter Records zurück
- [ ] 100 % der definierten decision_types haben Templates (Pflicht: keine leeren Templates)
- [ ] `FederatedAIDecisionAuditor`-Stub: cross-shard `DecisionRecord`-Timeline (einfaches Merge nach Timestamp)
- [ ] 10 neue Tests in `tests/test_explainability_reason_builder.cpp`

## Definition of Done

Für jeden der 10 definierten `decision_type`-Werte produziert `build()` eine
`CausalChain` mit `signal.size() > 0` und `decision.size() > 0`.
`toNaturalLanguage()` gibt lesbaren Text zurück.
