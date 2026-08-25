> **Status:** 2026-08-24 – enhanced with failover scenarios, fencing strategy, and recovery procedures.

# ThemisDB Failover Module - Production Requirements

<!-- Links: ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md · README.md · ROADMAP.md -->

Dieses Dokument ist der **kanonische Referenzpunkt für produktive Mindestanforderungen** des Failover-Moduls.
Es definiert verbindliche Betriebs- und Sicherheitsanforderungen für Auto-Failover-Manager, Disaster-Recovery-Manager.

## Dokumentabgrenzung (Canonical Split)

- **`src/failover/PRODUCTION_REQUIREMENTS.md` (dieses Dokument):** verpflichtende Produktionsanforderungen (MUST/MUST NOT), Sicherheitsannahmen, Betriebsgrenzen.
- **`src/failover/README.md`:** Funktionsübersicht, Architekturkontext, API- und Nutzungsbeispiele.
- **`src/failover/ROADMAP.md`:** Lieferphasen, offene/abgeschlossene Features, Readiness-Planung.
- **`src/failover/FUTURE_ENHANCEMENTS.md`:** mittelfristige/langfristige Erweiterungen und Forschungsfelder.

## Verbindliche Produktionsanforderungen

- **MUST:** Failover-Trigger-Thresholds explizit konfiguriert; kein automatischer Failover ohne definierte Schwellwerte.
- **MUST:** Disaster-Recovery-Manager mit geprüftem Recovery-Playbook konfiguriert; Recovery-Prozeduren müssen getestet sein.
- **MUST:** Alle sicherheitsrelevanten Konfigurationswerte beim Start validiert; fehlende oder ungültige Werte führen zu Fail-Closed-Verhalten.
- **MUST NOT:** Sicherheits- oder Autorisierungs-Checks in Produktionspfaden deaktivieren.
- **MUST:** `EpochFencingManager` konfiguriert sein, wenn `enable_split_brain_prevention=true`; fehlt er, schlägt `preventSplitBrain()` fail-closed mit `QUORUM_UNAVAILABLE`-Diagnostic.
- **MUST:** `quorum_log_path` auf persistenten Speicher zeigen; QuorumLog-Schreibfehler blockieren Promotions.
- **MUST:** `health_check_call_timeout_ms` explizit gesetzt (empfohlen ≤5 s); kein unbegrenztes Health-Check-Blocking in Produktionspfaden.
- **MUST NOT:** `auto_failover_manager.cpp` ohne Quorum-Log in Multi-Node-Setups einsetzen (durable Consensus erforderlich).

## Verbindliche Sicherheitsanforderungen

- Sicherheitsrelevante Operationen werden über dedizierte Kontroll-Surfaces geleitet.
- Fehler in sicherheitskritischen Pfaden werden als explizite Fehlercodes propagiert; kein Silent-Permit.
- Audit-Logging für sicherheitsrelevante Operationen aktiv in Produktionsdeployments.
- Epoch-Fencing vor jeder Promotion: `bumpEpoch()` muss epoch > 0 zurückgeben; epoch == 0 blockiert die Promotion.
- Idempotente Plan-Ausführung: wiederholte `executePlan(plan_id)` geben das gecachte Ergebnis zurück (keine doppelte Ausführung).

## Betriebsgrenzen

- Konfigurationswerte müssen deployment-spezifisch gesetzt sein; Default-Werte gelten nicht als produktionssicher.
- Ressourcen-Limits (Größen, Counts, Timeouts) müssen mit den Deployment-Anforderungen übereinstimmen.
- Externe Abhängigkeiten müssen mit expliziten Verbindungs-Timeouts und Retry-Policies konfiguriert sein.

## Failover-Szenarien und Anforderungen

### Szenario 1: Network Partition

**Beschreibung:** Ein oder mehrere Nodes sind vom Quorum isoliert.

| Anforderung | Verhalten |
|---|---|
| Quorum-Check schlägt fehl | `checkAndWaitForQuorum()` gibt `false` zurück nach `quorum_timeout_ms` |
| Promotion blockiert | kein `selectAndPromoteReplica()` ohne positiven Quorum-Check |
| Fail-closed Logging | `QUORUM_UNAVAILABLE` diagnostic emittiert via `emitDiagnostic()` |
| Recovery | Manuell via `failover_runbook_split_brain.md` oder automatisch wenn Quorum wiederkehrt |

**Konfigurationsanforderungen:**
- `quorum_timeout_ms` ≥ typische Netzwerk-Recovery-Zeit des Deployments
- `enable_split_brain_prevention = true`
- Runbook: `docs/operability/failover_runbook_split_brain.md`

### Szenario 2: Stale-Replica-Promotion

**Beschreibung:** Eine Replica mit veralteter Log-Position soll fälschlicherweise promoviert werden.

| Anforderung | Verhalten |
|---|---|
| Health-Status-Prüfung | nur `HealthStatus::HEALTHY` Replicas kommen als Kandidaten in Frage |
| Epoch-Fencing | `bumpEpoch()` muss vor Promotion aufgerufen werden; Fehler blockiert |
| QuorumLog | erfolgreiche Promotion wird in QuorumLog persistiert |
| Tie-Breaking | deterministisch (kleinster lexikographischer `node_id` gewinnt) |

**Konfigurationsanforderungen:**
- `EpochFencingManager` mit persistentem Epoch-Store konfiguriert
- `quorum_log_path` auf persistenten Storage
- Runbook: `docs/operability/failover_runbook_fencing_override.md`

### Szenario 3: Cascading Failover (Multi-Node-Ausfall)

**Beschreibung:** Mehrere Nodes fallen in kurzer Zeit aus.

| Anforderung | Verhalten |
|---|---|
| Queue-Begrenzung | `max_concurrent_failovers` begrenzt parallele Failover-Tasks |
| Queue-Saturation-Metrik | `tasks_dropped_queue_full` und `queue_pressure_events` werden gezählt |
| GC-Grace-Period | kurze Ausfall-Bursts werden als GC-Pause toleriert; kein unbeabsichtigter Failover |
| Adaptive Health-Check | bei hohen Check-Latenzen verlängert `updateAdaptiveInterval()` das Intervall |

**Konfigurationsanforderungen:**
- `max_concurrent_failovers` ≤ Anzahl verbleibender Healthy-Replicas im worst case
- `gc_grace_period` und `gc_grace_failure_count` deployment-spezifisch tunen
- Runbook: `docs/operability/failover_runbook_manual_recovery.md`

### Szenario 4: Disaster Recovery (DR) Plan

**Beschreibung:** Kompletter Site-Ausfall, DR-Plan wird ausgeführt.

| Anforderung | Verhalten |
|---|---|
| Plan-Validierung | `validatePlan()` erzwingt `plan_id ≠ ""`, `recovery_site ≠ ""`, `snapshot_id ≠ ""` (non-dry-run) |
| Concurrent-Guard | zweite parallele `executePlan()`-Aufruf wird sofort abgewiesen |
| Idempotenz | wiederholte Aufrufe mit gleichem `plan_id` geben gecachtes Ergebnis zurück |
| Fencing | `applyEpochFencing()` MUSS vor Restore ausgeführt werden; epoch == 0 blockiert |
| Quorum-Wait | `waitForCatchup()` wartet bis Quorum erreicht oder `catchup_timeout_ms` abgelaufen |

**Konfigurationsanforderungen:**
- `enforce_epoch_fencing = true` in Produktions-DR-Plänen
- `require_quorum = true`; nur in Test/Dry-Run deaktivierbar
- `max_verification_retries` ≥ 3

## Fencing-Strategie

### Epoch-Fencing (Mandatory)

Die Epoch-Fencing-Strategie verhindert Dual-Master und Stale-Replica-Promotion:

1. **Vor jeder automatischen Promotion** (`selectAndPromoteReplica()`): `bumpEpoch()` mit Kontext-String aufrufen.
2. **Vor jeder DR-Recovery** (`applyEpochFencing()`): `bumpEpoch()` für den DR-Plan aufrufen.
3. **Epoch-Validierung**: epoch == 0 wird als invalid behandelt und blockiert die Promotion fail-closed.
4. **Fencing-Manager fehlt**: `preventSplitBrain()` returned `false` + emittiert `QUORUM_UNAVAILABLE` diagnostic.

### Fencing-Override (Notfall)

Nur mit explizitem Operator-Approval; Prozedur in `docs/operability/failover_runbook_fencing_override.md`.

## Recovery-Prozeduren

### Automatische Recovery (`attemptRecovery`)

- Versucht `max_recovery_attempts` Mal den Node wiederherzustellen.
- Stats werden batch-weise (einmal pro Call, nicht per Iteration) aktualisiert.
- Bei Exhaustion: `NODE_REJOIN_FAILED` diagnostic + Event emittiert.

### Manuelle Recovery

Procedure in `docs/operability/failover_runbook_manual_recovery.md`:
1. Health-Status prüfen: `getStatistics()` auswerten.
2. Failover manuell auslösen: `triggerManualFailover(failed_node_id, target_promote_id)`.
3. Quorum verifizieren: `replication_mgr_->hasQuorum()`.
4. Topology-Version prüfen: `getTopologySnapshot()` auswerten.

## Minimaler Produktions-Check (Audit-fähig)

- [ ] Modul-Konfiguration vollständig und beim Start validiert
- [ ] Sicherheits- und Autorisierungs-Checks aktiv
- [ ] `EpochFencingManager` konfiguriert wenn `enable_split_brain_prevention=true`
- [ ] `quorum_log_path` auf persistenten Speicher gesetzt
- [ ] `health_check_call_timeout_ms` explizit konfiguriert (≤5 s empfohlen)
- [ ] Ressourcen-Limits explizit konfiguriert (keine Unlimited-Defaults)
- [ ] Audit-Logging aktiv
- [ ] Externe Abhängigkeiten mit Timeout und Retry konfiguriert
- [ ] Produktionsmodus via `THEMIS_PRODUCTION_MODE` oder `THEMIS_ENVIRONMENT` gesetzt
- [ ] Operator-Runbooks bekannt und aktuell (`docs/operability/failover_runbook_*.md`)

## Review / Sourcecode-Audit-Nachweis

### Betroffene Dateien im Review

- `src/failover/PRODUCTION_REQUIREMENTS.md`
- `src/failover/auto_failover_manager.cpp`
- `src/failover/disaster_recovery_manager.cpp`
