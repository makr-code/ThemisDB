> **Status:** 2026-06-01 – mit aktuellem Transaction-Code (`distributed_transaction_manager.cpp`, `lock_manager.cpp`, `saga_orchestrator.cpp`, `transaction_auditor.cpp`) abgeglichen.

# ThemisDB Transaction Module - Production Requirements

## Zweck und Geltungsbereich

Dieses Dokument ist der **kanonische Referenzpunkt für produktive Mindestanforderungen** des Transaction-Moduls.
Es definiert verbindliche Anforderungen für Transaktionslifecycle, verteilte Koordination, SAGA-Kompensation und Auditing.

## Dokumentabgrenzung (Canonical Split)

- **`src/transaction/PRODUCTION_REQUIREMENTS.md` (dieses Dokument):** verpflichtende Produktionsanforderungen (MUST/MUST NOT), Sicherheitsannahmen, Betriebsgrenzen.
- **`src/transaction/README.md`:** Funktionsübersicht, Architekturkontext, API- und Nutzungsbeispiele.
- **`src/transaction/ROADMAP.md`:** Lieferphasen, offene/abgeschlossene Features, Readiness-Planung.
- **`src/transaction/FUTURE_ENHANCEMENTS.md`:** mittelfristige/langfristige Erweiterungen und Forschungsfelder.

## Verbindliche Transaktionsanforderungen

- **MUST:** Distributed-Coordinator-Recovery (`crash_recovery_manager.cpp`) bei Start aktiviert; in-doubt Transaktionen werden nach Neustart ausgerollt oder abgebrochen.
- **MUST:** Deadlock-Timeout im `lock_manager.cpp` konfiguriert (kein unbegrenzt wartendes Lock).
- **MUST:** Transaktionsaudit-Logging (`transaction_auditor.cpp`) in Produktionspfaden aktiv.
- **MUST NOT:** SAGA-Kompensationsschritte als non-idempotent oder side-effect-unsafe definieren; Produktion setzt idempotente Compensation-Steps voraus.

## Verbindliche Sicherheitsanforderungen

### 1) Distributed Transaction Coordinator

- **MUST:** Prepare/Commit/Abort-Übergänge mit explizitem Outcome-Logging; keine stillen Zustandsübergänge.
- **MUST:** In-doubt-Windows haben definierten Recovery-Timeout; Coordinator-Restart muss Pending-State auflösen.
- **MUST NOT:** Koordinatorpfade ohne Liveness-Checks betreiben; Liveness-Hooks in `distributed_transaction_manager.cpp` müssen aktiv sein.

### 2) SAGA-Orchestrierung

- **MUST:** Alle SAGA-Kompensationsschritte sind definiert und testbar; keine partial-Rollback-Pfade ohne definiertes Compensation-Set.
- **MUST:** `distributed_saga.cpp` Kompensationsschritte deterministisch in umgekehrter Reihenfolge ausführen.
- **MUST NOT:** Long-running SAGA Chains ohne Timeout-Guardrail betreiben.

### 3) Lock Management

- **MUST:** `lock_manager.cpp` mit konfiguriertem Deadlock-Timeout; Timeout-Wert muss dokumentiert und deployment-spezifisch gesetzt sein.
- **MUST:** Deadlock-Predictor (`deadlock_predictor.cpp`) aktiv für verteilte Wait-For-Graphen.

## Betriebsgrenzen (aktuelles Transaction-Verhalten)

- `global_transaction_manager.cpp` koordiniert über Shards hinweg; verteilte Koordination hängt von Liveness der beteiligten Knoten ab.
- `branch_manager.cpp` verwaltet Transaction-Branches; Branches ohne expliziten Commit/Abort-Abschluss werden vom Recovery-Manager aufgeräumt.
- Audit-Logging über `transaction_auditor.cpp` verursacht I/O-Overhead; in Hochlast-Szenarien dedizierte Audit-Log-Kapazität einplanen.

## Minimaler Produktions-Check (Audit-fähig)

- [ ] Crash-Recovery-Manager aktiv und konfiguriert
- [ ] Deadlock-Timeout explizit gesetzt (kein Unlimited-Default)
- [ ] Transaktionsaudit-Logging aktiviert
- [ ] SAGA-Kompensationsschritte idempotent und getestet
- [ ] Distributed-Coordinator-Liveness-Hooks aktiv
- [ ] In-doubt-Timeout dokumentiert und deployment-spezifisch konfiguriert
- [ ] Produktionsmodus via `THEMIS_PRODUCTION_MODE` oder `THEMIS_ENVIRONMENT` gesetzt

## RPC Transport Injection Requirement (Wave 4C T1 — STUB #279)

- **MUST:** RPC transport must be injected before production deployment; stub injection causes silent no-op delivery.
  - The `DistributedTransactionManager` Phase-1 and Phase-2 RPC bridges are pure injection points.
  - Without a concrete `RpcTransport` implementation bound via `setRpcPhase1Fn()` / `setRpcPhase2Fn()` (or constructor config), distributed 2PC delivers no-op calls to remote participants.
  - A fail-fast guard prevents construction when `remote_phase1_dispatch` is set but no Phase-2 bridge is injected.
  - **Activation trigger:** Any deployment using remote participants (non-null `endpoint`, null `callback`).
  - **Removal Plan:** Q4 2026 — bind gRPC transport in `ThemisServer::initialize()`; remove stub after integration tests pass (issue #279).

## Review / Sourcecode-Audit-Nachweis

### Betroffene Dateien im Review

- `src/transaction/PRODUCTION_REQUIREMENTS.md`
- `src/transaction/distributed_transaction_manager.cpp`
- `src/transaction/lock_manager.cpp`
- `src/transaction/crash_recovery_manager.cpp`
- `src/transaction/saga_orchestrator.cpp`
- `src/transaction/distributed_saga.cpp`
- `src/transaction/transaction_auditor.cpp`
- `src/transaction/deadlock_predictor.cpp`
