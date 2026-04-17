---
type: enhancement
labels: ["type:enhancement", "module:distributed_knowledge", "module:api", "module:security", "priority:medium", "status:open", "queue/copilot"]
milestone: v2.0.0
parent: DK-0-EPIC
session: 8
---

# [DK-7] distributed_knowledge: Admin-API, Privacy-Audit & GDPR-Integration

## Aufgabe

Die Federation für Betreiber sichtbar und steuerbar machen: Admin-REST-Endpunkte
für Federation-Stats und manuelle Trigger, `SphincsPlus`-gesichertes Audit-Log
pro Runde, `CrossBorderTransferPolicy`-Check vor jedem Runden-Start und
`AIDecisionAuditor`-Integration für shard-übergreifende Entscheidungs-Timeline.

## Abgrenzung

| In Scope | Out of Scope |
|---|---|
| `GET /admin/federation/stats` | Grafana-Dashboard (liegt in observability/) |
| `GET /admin/federation/rag-stats` | WebSocket-basiertes Streaming der Stats |
| `POST /admin/federation/trigger` | Automatische Shard-Rebalancing-Trigger |
| SphincsPlus-Audit-Record pro Federation-Runde | Neue SphincsPlus-Varianten (liegt in security/) |
| `CrossBorderTransferPolicy`-Check | Neue GDPR-Regeln (liegt in governance/) |
| `AIDecisionAuditor`-Eintrag nach jedem `applyGlobalDelta()` | FederatedAIDecisionAuditor UI (→ Future) |
| 6 neue Tests (Admin: 3, Audit: 2, GDPR: 1) | Load-Tests der Admin-API |

## Idee / Konzept

Drei Sicherheitsschichten schützen die Federation:

**Schicht 1 — GDPR-Wächter (vor der Runde):**
`CrossBorderTransferPolicy::checkTransfer()` prüft ob beteiligte Shard-Standorte
EU-Adequacy-Anforderungen erfüllen. Falls ein Non-EU-Shard ohne Adequacy-Beschluss
beteiligt ist, wird die Runde blockiert.

**Schicht 2 — DP + Audit (während der Runde):**
Nach `triggerAggregation()` schreibt der Coordinator einen
`SphincsPlus`-signierten `DecisionRecord`:
```json
{
  "decision_type": "FEDERATED_ROUND",
  "round": 42,
  "participants": 3,
  "epsilon_spent": 0.1,
  "total_epsilon": 4.2,
  "algorithm": "FedAvg",
  "timestamp": "2026-04-17T04:24:07Z",
  "sphincs_signature": "..."
}
```

**Schicht 3 — Operator-Sichtbarkeit (nach der Runde):**
Admin-API gibt Betreibern vollständige Kontrolle und Sichtbarkeit ohne
direkt in die Datenbank schauen zu müssen.

### Admin-API Endpunkte

```
GET  /admin/federation/stats
     → {"current_round": 42, "pending_gradients": 1, "total_participants_ever": 126,
        "last_round_at": "...", "dp_epsilon_total": 4.2, "dp_epsilon_budget": 5.0}

GET  /admin/federation/rag-stats
     → {"last_merge_shard_count": 3, "last_merge_doc_count": 47,
        "last_merge_strategy": "RECIPROCAL_RANK_FUSION", "avg_merge_ms": 12.4}

POST /admin/federation/trigger
     Body: {"algorithm": "FedAvg"}  (optional, überschreibt Config)
     → {"round": 43, "participants": 3, "delta_version": "global-v43",
        "epsilon_spent": 0.1, "status": "success"}
     → 400 wenn min_participants nicht erfüllt
     → 403 wenn DP-Budget erschöpft
     → 503 wenn CrossBorderTransferPolicy blockiert
```

## Technische Details

### Sub-Issue 7a — LoRAFederationCoordinator: GDPR + Audit Hooks

**Datei:** `include/distributed_knowledge/lora_federation_coordinator.h`

```cpp
// Neu: optionale DI-Setter
void setCrossBorderPolicy(std::shared_ptr<CrossBorderTransferPolicy> policy);
void setDecisionAuditor(std::shared_ptr<AIDecisionAuditor> auditor);
void setSphincsPlus(std::shared_ptr<SphincsPlus> sphincs);
```

**In `doAggregation()` — Reihenfolge:**
1. `CrossBorderTransferPolicy::checkTransfer(participant_locations)` → bei Block: throw
2. `verifyPrivacyBudget()` → bei Überschreitung: throw mit Meldung "DP budget exhausted"
3. FedAvg/FedProx Aggregation
4. `addDifferentialPrivacy()`
5. `AIDecisionAuditor::record(DecisionRecord{...})`
6. `SphincsPlus::sign(audit_record)` → in `AIDecisionAuditor` gespeichert

### Sub-Issue 7b — Admin API Handler

**Datei:** `src/api/` (bestehende Admin-Handler-Struktur)

```cpp
// Registrierung in bestehender API-Routing-Tabelle
router.GET("/admin/federation/stats",    FederationAdminHandler::getStats);
router.GET("/admin/federation/rag-stats", FederationAdminHandler::getRagStats);
router.POST("/admin/federation/trigger", FederationAdminHandler::triggerRound);
```

`FederationAdminHandler` erhält `LoRAFederationCoordinator` und
`FederatedRAGMerger` via DI (bestehender DI-Container).

### Sub-Issue 7c — AIDecisionAuditor: Cross-Shard Timeline

In `IncrementalLoRATrainer::applyGlobalDelta()` (DK-3):
```cpp
// Bereits in DK-3 vorgesehen — Auditierung hier vervollständigen
auditor_->record({
    .decision_type = "FEDERATED_DELTA_APPLIED",
    .adapter_version = delta.version,
    .shard_id        = local_shard_id_,
    .participants    = delta.participants,
    .timestamp       = now()
});
```

### Neue Tests

**Admin-Handler-Tests:**
- `ADM-FED-01` GET /admin/federation/stats gibt JSON mit `current_round`-Feld zurück
- `ADM-FED-02` POST /admin/federation/trigger gibt `GlobalAdapterDelta`-JSON zurück
- `ADM-FED-03` POST /admin/federation/trigger gibt 403 zurück wenn DP-Budget erschöpft

**Audit-Tests:**
- `AUD-FED-01` Nach Aggregation existiert `DecisionRecord` mit `decision_type="FEDERATED_ROUND"`
- `AUD-FED-02` `DecisionRecord` enthält gültige SphincsPlus-Signatur

**GDPR-Test:**
- `GDPR-FED-01` `CrossBorderTransferPolicy` blockiert → `triggerAggregation()` wirft `std::runtime_error`

## Abhängigkeiten

- **Vorbedingung:** DK-3 (Koordinator vollständig), DK-6 (Integration getestet)
- **Parallel möglich mit:** DK-8 (Performance)

## Erfolgskriterien

- [ ] `LoRAFederationCoordinator::setCrossBorderPolicy()` vorhanden
- [ ] `LoRAFederationCoordinator::setDecisionAuditor()` vorhanden
- [ ] `LoRAFederationCoordinator::setSphincsPlus()` vorhanden
- [ ] `CrossBorderTransferPolicy`-Block → `triggerAggregation()` wirft bevor Aggregation startet
- [ ] DP-Budget erschöpft → `triggerAggregation()` wirft mit Meldung "DP budget exhausted"
- [ ] `DecisionRecord` mit `decision_type="FEDERATED_ROUND"` nach jeder Runde vorhanden
- [ ] `DecisionRecord` trägt gültige SphincsPlus-Signatur
- [ ] `GET /admin/federation/stats` gibt JSON mit `current_round` zurück
- [ ] `POST /admin/federation/trigger` gibt `delta_version` zurück
- [ ] `POST /admin/federation/trigger` gibt 403 bei erschöpftem DP-Budget
- [ ] `applyGlobalDelta()` schreibt `FEDERATED_DELTA_APPLIED`-Record in `AIDecisionAuditor`
- [ ] 6 neue Tests grün

## Definition of Done

Betreiber kann via `curl POST /admin/federation/trigger` eine Federation-Runde
manuell anstoßen und erhält JSON-Antwort mit Round-Number, Participant-Count und
Delta-Version. Im `AIDecisionAuditor` ist die Runde mit SphincsPlus-Signatur
nachvollziehbar.
