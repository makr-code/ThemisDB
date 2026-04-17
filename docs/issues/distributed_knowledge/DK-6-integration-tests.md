---
type: enhancement
labels: ["type:enhancement", "module:distributed_knowledge", "priority:high", "status:open", "queue/copilot"]
milestone: v2.0.0
parent: DK-0-EPIC
session: 7
---

# [DK-6] distributed_knowledge: End-to-End-Integration & Privacy-Validierung

## Aufgabe

Alle vier Verbindungsebenen (A–D) in einem gemeinsamen In-Process-Test mit
Mock-Gossip-Transport verifizieren. Der Fokus liegt auf zwei orthogonalen
Qualitätsachsen: **Korrektheit** (lernen alle Shards voneinander?) und
**Privacy** (verlässt kein Klartext den Shard?).

## Abgrenzung

| In Scope | Out of Scope |
|---|---|
| 6 Integrationsszenarien (A, B, C, D, Privacy, Fault-Tolerance) | Echter Netzwerk-Transport |
| Privacy-Budget-Test: 50 Runden → Runde 51 blockiert | Produktions-Lasttest (→ DK-8) |
| Property-based Privacy-Check: kein Klartext im GlobalDelta | Formal-Beweis der DP-Garantie |
| Fault-Tolerance: 1 von 3 Shards offline → Federation läuft weiter | Chaos-Engineering (liegt in chaos/) |
| `tests/test_distributed_knowledge_integration.cpp` (neu) | Admin-API-Tests (→ DK-7) |

## Idee / Konzept

Ein gutes Integrationssystem testet nicht nur Happy-Path, sondern auch die
**Invarianten die das System nie verletzen darf**. Für ein federated learning
System sind das:

1. **Korrektheit:** Globaler Delta verbessert oder hält Accuracy — nie Rückschritt
2. **Privacy:** Kein Wort aus einem Trainings-Dokument taucht im globalem Delta auf
3. **Robustheit:** Ausfall eines Shards stoppt nicht die Federation
4. **Budget:** DP-Budget wird akkurat verwaltet — nicht unbegrenzt verbrauchbar

```
In-Process Mock-Setup:
  gossip_bus = MockGossipBus()          // in-memory pub/sub
  shards[0..2] = MockShard(trainer, rag_bridge, feedback_collector)
  coordinator  = LoRAFederationCoordinator(config{min_participants: 2})
  merger       = FederatedRAGMerger(config{strategy: RRF, top_k: 5})
  feedback_sync = CrossShardFeedbackSync(config{})

  // Alles via DI verkabelt — kein echter Netzwerk-Code
```

### Privacy-Invarianten-Test (Scenario 5)

```python
# Pseudocode für Property-Based Check
for shard in shards:
    shard.train(documents=["CONFIDENTIAL-SHARD-" + shard.id + "-SECRET"])

delta = coordinator.triggerAggregation()

# Invariante: kein Token aus den Trainingsdokumenten im Delta
for key, value in delta.delta.items():
    assert "CONFIDENTIAL" not in str(value)
    assert "SECRET" not in str(value)
    assert isinstance(value, float)   # nur numerisch
```

## Technische Details

### Szenario 1 — Layer A: Domain-Routing

```
Setup:    Shard 0: SECURITY_MONITOR (accuracy_delta=+0.15)
          Shard 1: SCHEMA_ADVISOR   (accuracy_delta=+0.08)
          Shard 2: generisch        (accuracy_delta=0.0)
Test:     query(domain_hint=SECURITY_MONITOR) → routed to Shard 0
          query(domain_hint=SCHEMA_ADVISOR)   → routed to Shard 1
          query(domain_hint=GENERAL)          → default routing
Assertion: 100% der domain_hint-Queries landen auf korrektem Shard
```

### Szenario 2 — Layer B: Federated LoRA Round

```
Setup:    3 Shards, je 100 Trainings-Samples, gleiche Domäne
Test:     Alle 3 exportieren Gradienten → Aggregation + DP → applyGlobalDelta
Assertion: accuracy_after >= accuracy_before auf allen 3 Shards (kein Rückschritt)
           GlobalAdapterDelta.participants == 3
           GlobalAdapterDelta.round == 1
```

### Szenario 3 — Layer C: RAG-Recall

```
Setup:    Shard 0: 10 Docs zu "Transaktionsfehler" (Shard-0-spezifisch)
          Shard 1: 10 Docs zu "Schema-Migration"   (Shard-1-spezifisch)
          Shard 2: 5 Docs zu beiden Themen
Test:     query("Häufigste Transaktionsfehler") → FederatedRAGMerger top-5
Assertion: top-5 enthält mindestens 1 Doc aus Shard 0 UND 1 aus Shard 2
           (Recall@5 > Single-Shard-Baseline)
```

### Szenario 4 — Layer D: Cross-Shard RLAIF

```
Setup:    Shard 0 hat EmbeddingModel injiziert
Test:     DBA feedback auf Shard 0 → MockGossip propagiert
Assertion: Shard 1 + 2 zeigen applied_pairs >= 1 nach Gossip-Dispatch
           FeedbackSummary.shard_origin == "ANON"
           FeedbackSummary.reason_embedding hat korrekte Dimension (384)
```

### Szenario 5 — Privacy-Invariante

```
Setup:    Shard k trainiert auf Dokument "SHARD-K-GEHEIM-[k]"
Test:     Vollständiger Federation-Zyklus → GlobalAdapterDelta
Assertion: delta.delta Werte sind alle numerisch (float)
           Kein Schlüssel oder Wert enthält "GEHEIM" als Substring
           EncryptedGradient.data enthält kein "GEHEIM"
```

### Szenario 6 — Fault-Tolerance

```
Setup:    3 Shards, min_participants=2
          Shard 2 antwortet nicht (Timeout simuliert durch fehlendes submitGradient)
Test:     round_timeout abgewartet → triggerAggregation()
Assertion: Aggregation läuft durch (2 Participants ≥ min_participants)
           GlobalAdapterDelta.participants == 2
           Shard 0 + 1 erhalten applyGlobalDelta
           Shard 2 (offline) erhält kein applyGlobalDelta (kein Crash)
```

### Privacy-Budget-Test (separat)

```
Config:   dp_epsilon=0.1, dp_delta=1e-5, max_rounds=50
Test:     50 Runden mit je 2 Participants
Assertion: Runden 1–50 laufen durch
           coordinator.privacyBudgetRemaining() > 0 nach Runde 50
           Runde 51: verifyPrivacyBudget() → false → Runde übersprungen
           Log-Eintrag "DP budget exhausted" vorhanden
```

### Test-Datei-Struktur

```
tests/test_distributed_knowledge_integration.cpp
├── IntegrationSetup (gemeinsame Fixture)
├── Scenario_1_DomainRouting
├── Scenario_2_FederatedLoRA
├── Scenario_3_FederatedRAG
├── Scenario_4_CrossShardRLAIF
├── Scenario_5_PrivacyInvariant
├── Scenario_6_FaultTolerance
└── PrivacyBudgetExhaustion
```

## Abhängigkeiten

- **Vorbedingung:** DK-2, DK-3, DK-4, DK-5 alle abgeschlossen
- **Blockiert:** DK-7 (Admin-API setzt voraus, dass Integration getestet ist)

## Erfolgskriterien

- [ ] `tests/test_distributed_knowledge_integration.cpp` mit 7 Szenarien vorhanden
- [ ] Szenario 1: Domain-Routing Precision 100% (3 Queries, 3 korrekte Shards)
- [ ] Szenario 2: Accuracy nach Federation ≥ Accuracy vor Federation auf allen Shards
- [ ] Szenario 3: top-5-Ergebnis enthält Docs aus ≥ 2 verschiedenen Shards
- [ ] Szenario 4: `applied_pairs >= 1` auf Shard 1+2 nach Feedback auf Shard 0
- [ ] Szenario 5: Kein Substring aus Trainingsdokumenten in `GlobalAdapterDelta.delta`
- [ ] Szenario 6: Federation läuft mit 2 von 3 Shards (Shard 2 offline, kein Crash)
- [ ] Privacy-Budget-Test: Runde 51 wird abgelehnt (`verifyPrivacyBudget()` false)
- [ ] Alle 7 Integrationsszenarien grün
- [ ] Keine Regressions in bestehenden Test-Suites

## Definition of Done

Alle 7 Integrationsszenarien laufen in CI durch. Szenario 5 (Privacy-Invariante)
beweist nachweisbar, dass kein Trainingstext den Shard verlässt.
