---
Author: "ThemisDB Contributors"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

Source level: 1; SOT domain: module-behavior. Canonical references: `include/transaction/transaction_manager.h`, `include/transaction/distributed_transaction_manager.h`, `include/transaction/lock_manager.h`, `src/transaction/distributed_transaction_manager.cpp`, and `src/transaction/transaction_manager.cpp`. Related issue: #7390.

> **Scope:** These are proposed deployment requirements, not proof that the defaults or production wiring enforce them. The 2026-10-31 source audit did not execute builds or tests.

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

- **MUST (deployment policy):** Enable and configure recovery explicitly where required. `TransactionManager` exposes `enableCrashRecovery(...)`; `DistributedTxnManagerConfig::enable_recovery_log` defaults to `false`. Startup recovery is not inferred to be enabled by default.
- **MUST (deployment policy):** Set/verify lock wait and transaction timeout policy. The lock API's default wait is 5000 ms; manager timeout settings are separate.
- **MUST (deployment policy):** Enable audit recording in the actual production path if required. The presence of `TransactionAuditor` does not prove that all transaction paths record to it.
- **MUST NOT:** SAGA-Kompensationsschritte als non-idempotent oder side-effect-unsafe definieren; Produktion setzt idempotente Compensation-Steps voraus.

## Verbindliche Sicherheitsanforderungen

### 1) Distributed Transaction Coordinator

- **MUST:** Prepare/Commit/Abort-Übergänge mit explizitem Outcome-Logging; keine stillen Zustandsübergänge.
- **MUST (deployment policy):** Define a recovery procedure for unresolved participant states. TODO: establish a single default recovery timeout across the concrete coordinator implementations.
- **MUST NOT (deployment policy):** Assume participant liveness or restart recovery without validating the selected transport/configuration path.

### 2) SAGA-Orchestrierung

- **MUST:** Alle SAGA-Kompensationsschritte sind definiert und testbar; keine partial-Rollback-Pfade ohne definiertes Compensation-Set.
- **MUST (application contract):** Compensation callbacks should be designed for the application's required retry/order semantics; TODO: verify each distributed-saga path before stating a universal reverse-order guarantee.
- **MUST NOT:** Long-running SAGA Chains ohne Timeout-Guardrail betreiben.

### 3) Lock Management

- **MUST:** `lock_manager.cpp` mit konfiguriertem Deadlock-Timeout; Timeout-Wert muss dokumentiert und deployment-spezifisch gesetzt sein.
- **TODO:** No requirement that the deadlock predictor is enabled by default was established from the inspected configuration/API.

## Betriebsgrenzen (aktuelles Transaction-Verhalten)

- `global_transaction_manager.cpp` koordiniert über Shards hinweg; verteilte Koordination hängt von Liveness der beteiligten Knoten ab.
- `branch_manager.cpp` contains branch-management code. TODO: verify any recovery-manager cleanup of unfinished branches before documenting it.
- Audit-Logging über `transaction_auditor.cpp` verursacht I/O-Overhead; in Hochlast-Szenarien dedizierte Audit-Log-Kapazität einplanen.

## Minimaler Produktions-Check (Audit-fähig)

- [ ] Crash-recovery explicitly enabled/configured where required
- [ ] Deadlock-Timeout explizit gesetzt (kein Unlimited-Default)
- [ ] Transaktionsaudit-Logging aktiviert
- [ ] SAGA-Kompensationsschritte idempotent und getestet
- [ ] Verify participant transport/liveness behavior for the selected deployment
- [ ] In-doubt-Timeout dokumentiert und deployment-spezifisch konfiguriert
- [ ] TODO: verify the supported production-mode configuration key in current runtime configuration

## RPC Transport Injection Requirement (Wave 4C T1 — STUB #279)

- **MUST:** RPC transport must be injected before production deployment; the built-in defaults now fail closed instead of silently acting as no-op transport.
  - The `DistributedTransactionManager` Phase-1 and Phase-2 RPC bridges are pure injection points.
  - The manager configuration exposes `remote_phase1_dispatch` and `remote_phase2_dispatch` callbacks. TODO: verify the exact no-transport outcome and constructor guard against the current implementation before using the historical wording below as an operational contract.
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
