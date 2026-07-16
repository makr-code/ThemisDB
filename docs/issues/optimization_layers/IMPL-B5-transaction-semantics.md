---
type: enhancement
labels: ["type:enhancement", "module:transaction", "module:training", "priority:medium", "status:open", "queue/copilot"]
milestone: v2.0.0
parent: IMPL-EPIC-B
session: S-6
layer: 5
---

# [IMPL-B5] Layer 5: TransactionSemanticAdvisor

## Aufgabe

`LLM_OPTIMIZATION_LAYERS_MATRIX.md` §Layer-5 definiert die semantische Ergänzung
des bestehenden `DeadlockPredictor`: ein `TransactionSemanticAdvisor` der
Batch-Affinität und optimale Commit-Reihenfolge aus dem **semantischen Inhalt**
der Transaktion ableitet — nicht nur aus dem Konflikt-Graph.

## Abgrenzung

| In Scope | Out of Scope |
|---|---|
| `TransactionSemanticAdvisor` Klasse | Änderungen am `DeadlockPredictor` |
| `BatchAffinityHint` Struct | Neue Transaktions-Engine |
| Entity-basierte Konflikt-Schätzung via LLM-Embeddings | SQL-Parser (bestehend nutzen) |
| `AIDecisionAuditor`-Integration | DBA-Dialog-UI |
| Advisory-only (kein blockierendes Verhalten) | Cross-Shard-Tx-Hints (→ DK-4/Layer 11) |

## Idee / Konzept

Der `DeadlockPredictor` erkennt Deadlocks auf Basis des Lock-Graphen.
Der `TransactionSemanticAdvisor` ergänzt ihn **vor** der Transaktion:
Er schaut auf die Entity-Map des Transaction-Inhalts (z.B. `{orders.user_id=42,
inventory.product_id=7}`) und berechnet Batch-Affinität zu anderen laufenden Transaktionen:

```
Tx A: {entity: "user:42", action: "UPDATE orders"}
Tx B: {entity: "user:42", action: "UPDATE loyalty_points"}
→ BatchAffinityHint: batch Tx A + Tx B (gleicher user:42, kein Deadlock-Risiko)

Tx C: {entity: "inventory:7", action: "DECREMENT stock"}
Tx D: {entity: "inventory:7", action: "UPDATE reorder_threshold"}
→ BatchAffinityHint: sequence C → D (stock first, then threshold — semantisch korrekt)
```

## Technische Details

```cpp
// Neue Datei: include/transaction/transaction_semantic_advisor.h
class TransactionSemanticAdvisor {
public:
    struct BatchAffinityHint {
        std::string primary_tx_id;
        std::vector<std::string> affine_tx_ids;  // empfohlene Batch-Partner
        double conflict_probability;              // [0.0, 1.0]
        std::string reason;                       // "same_entity_different_action"
    };

    // Schätze Batch-Affinität für gegebene Tx-Menge
    std::vector<BatchAffinityHint> analyzeBatch(
        const std::vector<TransactionContext>& pending_txs
    ) const;

    // Advisory für eine einzelne Tx: empfohlene Ausführungszeit
    std::chrono::milliseconds suggestDeferral(
        const TransactionContext& tx,
        const std::vector<TransactionContext>& concurrent_txs
    ) const;
};
```

**`TransactionContext`** enthält: `tx_id`, `entity_map` (entity-type → entity-id),
`operation_type` (READ/WRITE/DELETE), `estimated_duration_ms`.

**Entity-Ähnlichkeit** via Embedding-Kosinus-Distanz (bestehender `EmbeddingModel`).
Schwellenwert: `similarity > 0.8 → batch candidate`.

## Erfolgskriterien

- [ ] `TransactionSemanticAdvisor` Klasse vorhanden
- [ ] `analyzeBatch()` gruppiert Transaktionen mit gleichem Entity-Typ korrekt
- [ ] `conflict_probability` für disjunkte Entities < 0.1
- [ ] `conflict_probability` für identische Entity + konkurrierende Writes > 0.7
- [ ] Advisory ist **non-blocking** — gibt immer sofort zurück (≤ 10 ms)
- [ ] Schreibt `DecisionRecord{decision_type="TX_SEMANTIC_HINT"}` in `AIDecisionAuditor`
- [ ] `suggestDeferral()` gibt 0ms zurück wenn keine Konflikte erkennbar
- [ ] 8 neue Tests in `tests/test_transaction_semantic_advisor.cpp`
- [ ] Layer-5-Performance-Target: `analyzeBatch(100 Tx)` ≤ 10 ms

## Definition of Done

10 Transaktionen mit 3 verschiedenen Entities: `analyzeBatch()` schlägt
die korrekte Batch-Gruppierung vor (gleiche Entities zusammen). Alle 8
Tests grün.
