[docs](../../README.md) > [de](../INDEX.md) > [failover](./README.md) > [overview](./README.md)
**Datum:** 2026-05-12
**Status:** review
**Primary (Quelle der Wahrheit):**
- `src/failover/README.md`
- `src/failover/ARCHITECTURE.md`
- `src/failover/ROADMAP.md`
- `src/failover/FUTURE_ENHANCEMENTS.md`
- `include/failover/README.md`

**Bezug / Reference:**
- Issue: `[MODULE] failover`
- Kontext: Reality-Check und modulweise Secondary-Doku-Synchronisierung für das Failover-Modul.

---

# Failover-Modul

## TL;DR

Der Kern des Moduls ist implementiert (`AutoFailoverManager`, `DisasterRecoveryManager`) und durch Tests abgedeckt.
Offene Punkte betreffen primär Roadmap-/Enhancement-Themen (z. B. Cross-Region-Integration, ABI/Soak-Tests), nicht fehlende Basisfunktionalität.

## Task 1 — Reality-Check gegen Sourcecode

### Verifiziert implementiert

- `AutoFailoverManager` mit Queue, Worker-Loop, Zustandsmaschine und Queue-Pressure-/Retry-Telemetrie
  (`include/failover/auto_failover_manager.h`, `src/failover/auto_failover_manager.cpp`)
- `DisasterRecoveryManager` mit Plan-Validierung und 7-stufiger Recovery-Pipeline
  (`include/failover/disaster_recovery_manager.h`, `src/failover/disaster_recovery_manager.cpp`)
- Zentrale Source-Komponenten:
  - Monitoring-Loop + Failover-Worker in `src/failover/auto_failover_manager.cpp`
  - Recovery-Phasen PRECHECKS → SNAPSHOT_VALIDATION → EPOCH_FENCING → RESTORE → REPLICA_CATCHUP → TRAFFIC_SHIFT → VERIFICATION
- Testabdeckung vorhanden:
  - `tests/test_auto_failover_manager.cpp` (39 Tests)
  - `tests/test_disaster_recovery_manager.cpp` (8 Tests)
  - `tests/test_failover_chaos_scenarios.cpp` (17 Tests)

### Dokumentierte Abweichungen / offene Lücken

Siehe detailliert: [`MISSING_IMPLEMENTATIONS.md`](./MISSING_IMPLEMENTATIONS.md)

## Installation

Das Modul ist Teil des regulären ThemisDB-Builds; keine separate Installation erforderlich.

## Usage

- Laufzeitsteuerung über `AutoFailoverManager::start/stop/triggerManualFailover`
- DR-Ausführung über `DisasterRecoveryManager::validatePlan/executePlan`
- Öffentliche Header-/API-Übersicht: [`../../../include/failover/README.md`](../../../include/failover/README.md)

```cpp
#include "failover/auto_failover_manager.h"

themis::failover::AutoFailoverConfig cfg;
cfg.max_concurrent_failovers = 4;
cfg.queue_pressure_threshold = 0.5f;

themis::failover::AutoFailoverManager mgr(cfg, replication_mgr, health_monitor, spare_mgr, fencing_mgr);
mgr.start();
mgr.triggerManualFailover("node-a");
```

```cpp
#include "failover/disaster_recovery_manager.h"

themis::failover::DisasterRecoveryPlan plan;
plan.plan_id = "dr-plan-1";
plan.primary_site = "dc-a";
plan.recovery_site = "dc-b";
plan.snapshot_id = "snapshot-42";

auto result = dr_mgr.executePlan(plan);
```
- Details zu offenen Lücken: [`MISSING_IMPLEMENTATIONS.md`](./MISSING_IMPLEMENTATIONS.md)

## Task 2 — ROADMAP / FUTURE_ENHANCEMENTS Verifikation

### ROADMAP-Statusabgleich

- `src/failover/ROADMAP.md` ist strukturell konform (Current Status, In Progress, 6 Phasen, Readiness, Known Issues, Breaking Changes).
- Implementierte Phasenpunkte (z. B. Queue-/Retry-Telemetrie, Chaos-Matrix) sind im Code/Test nachvollziehbar.
- Offene Punkte in Phase 4/5 und „In Progress“ sind weiterhin offen und konsistent dokumentiert.

### FUTURE_ENHANCEMENTS-Prüfung

- `src/failover/FUTURE_ENHANCEMENTS.md` folgt der geforderten Struktur:
  Scope, Design Constraints, Required Interfaces, Implementation Notes, Test Strategy, Performance Targets, Security/Reliability.
- Hinweise sind implementierbar formuliert; relevante offene Punkte wurden in den Missing-Implementations-Report übernommen.
- `include/failover/` enthält in diesem Stand öffentliche Header plus `README.md`; zusätzliche Include-seitige Roadmap-/Future-Dokumente sind nicht vorhanden und werden daher nicht als Quelle behauptet.

## Public API, Konfigurationsoptionen und Grenzen

### `AutoFailoverManager`

- Lifecycle: `start()`, `stop()`, `isRunning()`
- Manueller Eingriff: `triggerManualFailover(failed_node_id, target_promote_id)`
- Beobachtbarkeit: `getStatistics()`, `getLastFailoverResult()`, `registerEventCallback()`
- Wichtige Konfigurationsfelder:
  - `consecutive_failures_before_action`
  - `max_concurrent_failovers`
  - `queue_pressure_threshold`
  - `enable_network_partition_detection`
  - `enable_split_brain_prevention`
  - `enable_automatic_recovery`, `recovery_retry_interval`, `max_recovery_attempts`

### `DisasterRecoveryManager`

- Plan-Pfad: `validatePlan()`, `executePlan()`
- Test-/Integrations-Hooks: `setStepHook()`, `clearStepHooks()`
- Wichtige Konfigurationsfelder:
  - `require_quorum`
  - `enforce_epoch_fencing`
  - `catchup_timeout`
  - `verification_timeout`
  - `max_verification_retries`
  - `allow_dry_run_without_managers` (im Public API vorhanden; die aktuelle Implementierung erlaubt Dry-Run ohne Manager bereits unabhängig von diesem Flag)

### Laufzeitverhalten / Fehlerfälle

- `triggerManualFailover()` liefert `false`, wenn der Manager nicht läuft oder die Queue voll ist.
- Queue-Pressure-Telemetrie greift ab `queue_pressure_threshold`.
- Netzwerkpartitionserkennung kann Split-Brain-Schutz und Statistik-Updates auslösen.
- Nicht-Dry-Run-DR-Pläne benötigen `snapshot_id`.
- Fehlende Replikations-/Fencing-Manager führen bei aktivierten Anforderungen (`require_quorum`, `enforce_epoch_fencing`) zu Fehlern.
- Verifikation ist retry-gebunden; Catchup und Verifikation enden bei Timeout.

## Task 3 — Research-Hinweise

- Priorität für nächste Umsetzungsrunde:
  1. Cross-Region-Traffic-Manager-Integration (`src/failover/ROADMAP.md`)
  2. API/ABI-Grenztests für `include/failover/*` (Header-/Test-Analyse, siehe `MISSING_IMPLEMENTATIONS.md`)
  3. Metrik-Export pro DR-Schritt (`src/failover/FUTURE_ENHANCEMENTS.md`)
- Constraint: bestehende öffentliche Signaturen der Manager stabil halten (siehe FUTURE_ENHANCEMENTS).

## Troubleshooting

- `triggerManualFailover()` schlägt fehl: prüfen, ob `start()` erfolgreich war und `max_concurrent_failovers` ausreicht.
- Häufige `QUEUE_PRESSURE`-Events: Queue-Limit, Enqueue-Rate und Abarbeitungsdauer prüfen.
- DR-Fehler `snapshot_id must not be empty for non-dry-run recovery`: Snapshot setzen oder bewusst `dry_run=true` nutzen.
- DR-Fehler zu Quorum/Fencing: benötigte Manager verdrahten oder Konfiguration gezielt für Lab-/Dry-Run-Szenarien lockern.

## Verwandte Dokumente

- [PRIMARY_SOURCES.md](./PRIMARY_SOURCES.md)
- [Public API (`include/failover/README.md`)](../../../include/failover/README.md)
- [Implementierungsübersicht (`src/failover/README.md`)](../../../src/failover/README.md)
- [Architektur](../../../src/failover/ARCHITECTURE.md)
- [Roadmap](../../../src/failover/ROADMAP.md)
- [Future Enhancements](../../../src/failover/FUTURE_ENHANCEMENTS.md)
- [Fehlende Implementierungen](./MISSING_IMPLEMENTATIONS.md)
