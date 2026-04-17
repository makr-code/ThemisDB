---
type: enhancement
labels: ["type:enhancement", "module:distributed_knowledge", "module:rag", "module:prompt_engineering", "priority:medium", "status:open", "queue/copilot"]
milestone: v2.0.0
parent: DK-0-EPIC
session: 6
---

# [DK-5] distributed_knowledge: Layer D — Federated RLAIF

## Aufgabe

DBA-Feedback das auf einem Shard gegeben wird, soll alle Shards verbessern —
**anonymisiert, ohne Klartext-Propagation**. `FeedbackCollector` erhält einen
optionalen `CrossShardFeedbackSync`-Hook, der das Feedback als Embedding
(384-dim, kein Originaltext) via Gossip verteilt. Alle empfangenden Shards
fügen ein synthetisches RLAIF-Präferenzpaar in ihren `RLAIFTrainer` ein.

## Abgrenzung

| In Scope | Out of Scope |
|---|---|
| `FeedbackCollector::setCrossShardSync()` DI-Setter | Änderungen am RLAIF-Algorithmus |
| Embedding-Extraktion via injiziertem `EmbeddingModel` | Training eines dedizierten Feedback-Embedding-Modells (→ Future, RQ-DK-3) |
| `RLAIFTrainer::addCrossShardSummary()` | Nearest-Neighbour-Lookup-Implementierung (Caller-Verantwortung) |
| Dedup-Schutz gegen Gossip-Echos | Klartext-Übertragung (explizit verboten) |
| `ZeroTrustPolicyEnforcer`-Check auf eingehende Summaries | Neue ZeroTrust-Regeln |
| 6 neue Tests (Feedback: 2, RLAIF: 3, ZeroTrust-Integration: 1) | Performance (→ DK-8) |

## Idee / Konzept

DBA-Feedback enthält domänenspezifisches Wissen das auf einem Shard entsteht,
aber für alle Shards relevant ist. Das Problem: GDPR und Zero-Knowledge-Constraint
verbieten Klartext-Propagation.

**Lösung:** Nur der Embedding-Vektor des Queries wird übertragen — ein 384-dimensionaler
Float-Vektor kodiert die semantische Bedeutung, enthält aber keine rekonstruierbaren
Texte (bei korrektem Embedding-Modell).

```
DBA: "Ablehnung — Denormalisierung bei orders.status ist falsch"
  ↓ FeedbackCollector.recordFeedback(entry)    [lokal, unverändert]
  ↓ [optional, wenn sync gesetzt]
  ↓ reason_embedding = embeddingModel.embed(entry.query)   [384-dim]
  ↓ CrossShardFeedbackSync.publishFeedback({
        feedback_type_label: "USER_NEGATIVE",
        reason_embedding: [0.12, -0.34, ...],   // kein Klartext
        shard_origin: "ANON"                     // erzwungen
    })
  ↓ GossipProtocol broadcastet FeedbackSummary
  ↓
  Empfangender Shard (nach Dedup + ZeroTrust-Check):
  ↓ CrossShardFeedbackSync.handleInboundSummary(summary)
  ↓ callback → RLAIFTrainer.addCrossShardSummary(summary)
```

### Synthetisches Präferenzpaar aus Embedding

Da kein Klartext vorliegt, konstruiert `addCrossShardSummary()` ein
synthetisches Präferenzpaar. Die genaue Nearest-Neighbour-Implementierung
liegt beim Caller (z.B. `HybridRetriever`) — `addCrossShardSummary()` akzeptiert
das fertige `PreferencePair` oder überlässt die Konstruktion dem Caller.

**Pragmatische Variante für v1.0:** Caller übergibt `PreferencePair` direkt,
`addCrossShardSummary()` ist nur ein typsicherer Wrapper.

## Technische Details

### Sub-Issue 5a — FeedbackCollector Hook

**Datei:** `include/prompt_engineering/feedback_collector.h` + `.cpp`

```cpp
// Neu: optionaler DI-Setter
void setCrossShardSync(std::shared_ptr<CrossShardFeedbackSync> sync);

// Optionaler EmbeddingModel-Setter (für Embedding-Extraktion)
void setEmbeddingModel(std::shared_ptr<IEmbeddingModel> model);
```

**Verhalten in `recordFeedback(entry)`:**
1. Lokale Aufzeichnung — **unverändert** (Bestehende Logik bleibt erhalten)
2. `if (sync_ && embedding_model_)`:
   - `embedding = model_->embed(entry.query)` — nur Query, kein Feedback-Text
   - Wenn `embedding.size() != sync_config.max_embedding_dim` → Log-Warning, Skip
   - `sync_->publishFeedback(summary)` mit `shard_origin = "ANON"` (vom Sync erzwungen)
3. Bei fehlendem Embedding-Modell: Log-Warning `"Cross-shard feedback skipped: no embedding model"`, kein Fehler

### Sub-Issue 5b — RLAIFTrainer Erweiterung

**Datei:** `include/rag/rlaif_trainer.h` + `.cpp`

```cpp
// Neu: Empfange synthetisches Präferenzpaar aus Cross-Shard-Feedback
void addCrossShardSummary(const FeedbackSummary& summary,
                          const PreferencePair& synthetic_pair);

// Stats-Abfrage
struct CrossShardStats {
    size_t received_summaries;
    size_t applied_pairs;
    size_t skipped_summaries; // embedding lookup fehlgeschlagen
};
CrossShardStats getCrossShardStats() const;
```

### Sub-Issue 5c — ZeroTrust-Integration

Vor `handleInboundSummary()` in `CrossShardFeedbackSync`:
```cpp
// Bestehende ZeroTrustPolicyEnforcer-Instanz injizierbar
void setZeroTrustEnforcer(std::shared_ptr<ZeroTrustPolicyEnforcer> enforcer);

// In handleInboundSummary():
if (enforcer_) {
    ZeroTrustRequest req{.source_id = summary.shard_origin,
                         .resource  = "federated_feedback",
                         .action    = "ingest"};
    if (!enforcer_->evaluateRequest(req).allowed) {
        stats_.rejected_by_policy++;
        return; // silent drop, kein Fehler
    }
}
```

### Neue Tests

**`tests/test_feedback_collector.cpp`:**
- `FC-CSS-01` recordFeedback() mit gesetztem Sync: publishFeedback() wird aufgerufen
- `FC-CSS-02` recordFeedback() ohne Sync: kein Crash, lokale Aufzeichnung vollständig

**`tests/test_rlaif_trainer.cpp`:**
- `RLAIF-CSS-01` addCrossShardSummary() erhöht `applied_pairs`-Counter
- `RLAIF-CSS-02` addCrossShardSummary() fügt PreferencePair in internen Pool ein
- `RLAIF-CSS-03` getCrossShardStats() gibt konsistente Zähler zurück

**Integration ZeroTrust:**
- `ZT-FED-01` handleInboundSummary() mit rejectendem Enforcer → callback nicht aufgerufen

## Abhängigkeiten

- **Vorbedingung:** DK-1 (Tests laufen)
- **Parallel möglich mit:** DK-3 (Layer B), DK-4 (Layer C)
- **Optionale Verbesserung:** DK-2 (Gossip-Transport — `publishFeedback()` nutzt
  denselben Handler-Mechanismus)
- **Blockiert:** DK-6 (Layer-D-Szenario im Integrationstest)

## Erfolgskriterien

- [ ] `FeedbackCollector::setCrossShardSync()` DI-Setter vorhanden
- [ ] `FeedbackCollector::setEmbeddingModel()` DI-Setter vorhanden
- [ ] `recordFeedback()` ruft `publishFeedback()` auf wenn Sync gesetzt
- [ ] `recordFeedback()` schlägt nicht fehl wenn kein Sync gesetzt (backward-compatible)
- [ ] `shard_origin` ist immer `"ANON"` in publizierten Summaries (erzwungen)
- [ ] Keine Klartexte in `FeedbackSummary::reason_embedding` (384-dim Floats)
- [ ] `RLAIFTrainer::addCrossShardSummary()` vorhanden und erhöht `applied_pairs`
- [ ] `getCrossShardStats()` gibt korrekte Zähler zurück
- [ ] ZeroTrust-Check blockt inakzeptable Inbound-Summaries
- [ ] 6 neue Tests grün
- [ ] `recordFeedback()` bestehende Tests unverändert grün (kein Regression)

## Definition of Done

In-Process-Test: DBA-Feedback auf Shard 1 → `FeedbackSummary` mit `shard_origin="ANON"`
propagiert via Mock-Gossip → Shard 2 + 3 zeigen `applied_pairs >= 1` in
`getCrossShardStats()`. Der `FeedbackSummary.reason_embedding` enthält
keinen Teilstring des originalen Queries.
