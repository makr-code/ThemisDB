---
type: enhancement
labels: ["type:enhancement", "module:distributed_knowledge", "module:rag", "module:query", "priority:medium", "status:open", "queue/copilot"]
milestone: v2.0.0
parent: DK-0-EPIC
session: 5
---

# [DK-4] distributed_knowledge: Layer C — Federated RAG Merge

## Aufgabe

Erweitere `QueryFederation` um eine RAG-bewusste Merge-Strategie via
`FederatedRAGMerger`: Bei einer AQL-Anfrage ruft `QueryFederation` die
`RAGIngestionBridge` auf **allen Shards parallel** auf und kombiniert die
Ergebnisse mittels Reciprocal Rank Fusion (RRF) zu einem globalen Kontext —
das LLM sieht damit Wissen aus allen Shards bei jeder Anfrage.

## Abgrenzung

| In Scope | Out of Scope |
|---|---|
| `QueryFederation::setRAGMerger(shared_ptr<FederatedRAGMerger>)` DI-Setter | Neues Retrieval-Modell |
| `QueryFederation::merge()` → RAG-aware Pfad wenn Merger gesetzt | Hybrid BM25+Vector Retrieval (liegt in RAG) |
| `ShardRetrievalResult::adapter_accuracy_delta` aus Gossip-Scores | Streaming RAG (→ Future) |
| `AdaptiveShardRouter::getAdapterAccuracyDelta()` (→ DK-2) | Änderungen an RRF-Algorithmus |
| 5 neue Tests in `tests/test_query_federation.cpp` | Multi-modal RAG (→ Future) |
| Graceful Handling eines Timeout-Shards (ok=false) | Cross-shard Transaktionen (andere Schicht) |

## Idee / Konzept

`QueryFederation` führt bereits Fan-out und Merge durch — aber für generische
Query-Ergebnisse, nicht für RAG-Dokument-Listen. Der neue Pfad:

```
AQL: "Häufigste Transaktionsfehler"
  ↓ QueryFederation.fanOut(queryPlan)   [parallel, bestehend]
  ↓
  Shard 1: RAGIngestionBridge.enrichRetrievedDocuments(docs_1) → ShardRetrievalResult
  Shard 2: RAGIngestionBridge.enrichRetrievedDocuments(docs_2) → ShardRetrievalResult
  Shard N: RAGIngestionBridge.enrichRetrievedDocuments(docs_N) → ShardRetrievalResult
  ↓
  [Shard 7 hat SECURITY_MONITOR-Adapter → adapter_accuracy_delta = +0.12
   aus AdaptiveShardRouter.getAdapterAccuracyDelta("shard-7", domain)]
  ↓ FederatedRAGMerger.merge(shard_results)
    → RRF + Spezialisierungs-Boost 1.2× für Shard 7
    → dedup + top_k=20
  ↓ MergedRAGContext.buildPromptContext(max_docs=10, max_chars=4000)
  ↓
LLM-Prompt: "[Shard: shard-7] ... [Shard: shard-3] ..."
```

**Schlüsselunterschied zu heute:** Das LLM erhält Kontext aus allen Shards,
nicht nur vom lokalen. Die `adapter_accuracy_delta`-Information aus DK-2
veredelt die Shard-Gewichtung im RRF.

### RRF-Korrektheit

```
score(doc d über alle Shards S) = Σ_{s∈S} boost_s / (k + rank_s(d))
boost_s = 1.2 wenn accuracy_delta_s > 0 (Spezialisierungs-Boost)
k = 60   (Cormack 2009 — empirisch optimiert)
```

Ein Dokument das in Shard 7 auf Rang 1 steht und in Shard 3 auf Rang 5:
```
score = 1.2/(60+1) + 1.0/(60+5) = 0.0197 + 0.0154 = 0.035
```
vs. ein Dokument nur auf Shard 3 Rang 1:
```
score = 1.0/(60+1) = 0.016
```
→ das shard-übergreifend relevante Dokument dominiert.

## Technische Details

### Sub-Issue 4a — QueryFederation RAG-aware Merge

**Datei:** `include/query/query_federation.h` + `.cpp`

```cpp
// Neu: optionaler DI-Setter
void setRAGMerger(std::shared_ptr<FederatedRAGMerger> merger);

// Intern: merge() prüft ob RAGMerger gesetzt UND result_type == RAG_CONTEXT
// Falls ja: konvertiert per-Shard-Ergebnisse → ShardRetrievalResult → merger.merge()
```

**Konvertierung `QueryResult → ShardRetrievalResult`:**
```cpp
ShardRetrievalResult toShardResult(const QueryResult& r, const std::string& shard_id) {
    ShardRetrievalResult sr;
    sr.shard_id = shard_id;
    sr.ok       = !r.hasError();
    sr.adapter_accuracy_delta =
        router_->getAdapterAccuracyDelta(shard_id, r.domainType());
    for (size_t i = 0; i < r.docs.size(); ++i) {
        sr.documents.push_back({r.docs[i].id, r.docs[i].content,
                                r.docs[i].score, i+1 /*rank*/, shard_id});
    }
    return sr;
}
```

**Timeout-Handling:** Shard antwortet nicht innerhalb `query_timeout_ms` →
`sr.ok = false`, Shard wird in Merge übersprungen. Merge läuft mit verbleibenden Shards.

### Sub-Issue 4b — ShardRetrievalResult Befüllung

`adapter_accuracy_delta` aus DK-2 (`AdaptiveShardRouter::getAdapterAccuracyDelta()`).
Das setzt voraus, dass `QueryFederation` eine Referenz auf `AdaptiveShardRouter` hält
(via DI-Setter, analog zu `setRAGMerger()`):

```cpp
void setShardRouter(std::shared_ptr<AdaptiveShardRouter> router);
```

### Neue Tests in `tests/test_query_federation.cpp`

- `QF-RAG-01` Fan-out zu 3 Mock-Shards → merged top-5 enthält Docs aus allen 3 Shards
- `QF-RAG-02` Ein Shard timeout (`ok=false`) → Merge liefert trotzdem Ergebnis mit 2 Shards
- `QF-RAG-03` Spezialisierter Shard (accuracy_delta=+0.15) → seine Top-Docs dominieren Merger-Ergebnis
- `QF-RAG-04` Wenn kein RAGMerger gesetzt → bestehender Merge-Pfad unverändert (kein Regression)
- `QF-RAG-05` `buildPromptContext()` enthält Shard-ID-Präfixe im Output

## Abhängigkeiten

- **Vorbedingung:** DK-1 (Tests), DK-2 (`getAdapterAccuracyDelta()` vorhanden)
- **Parallel möglich mit:** DK-3 (Layer B), DK-5 (Layer D)
- **Blockiert:** DK-6 (Layer-C-Szenario im Integrationstest)

## Erfolgskriterien

- [ ] `QueryFederation::setRAGMerger()` DI-Setter vorhanden
- [ ] `QueryFederation::setShardRouter()` DI-Setter vorhanden
- [ ] `merge()` ruft `FederatedRAGMerger::merge()` auf wenn Merger gesetzt und RAG-Kontext
- [ ] Timeout-Shard wird als `ok=false` markiert und Merge läuft mit Rest weiter
- [ ] `ShardRetrievalResult::adapter_accuracy_delta` wird aus Router befüllt
- [ ] RRF-Ergebnis: Dokument aus 3 Shards rankt höher als Dokument nur aus 1 Shard
- [ ] Spezialisierungs-Boost: Shard mit `accuracy_delta > 0` liefert höher gerankte Docs
- [ ] Kein RAGMerger gesetzt → `merge()` verhält sich identisch zu heute (kein Regression)
- [ ] 5 neue Query-Federation-Tests grün
- [ ] Keine Regressions in bestehenden `test_query_federation.cpp`-Tests

## Definition of Done

In einem In-Process-Test mit 3 Mock-Shards: das LLM-Prompt-Context enthält
`[Shard: ...]`-Präfixe aus mindestens 2 verschiedenen Shards. Recall@10 im
Test ist ≥ +15 % gegenüber Single-Shard-Baseline.
