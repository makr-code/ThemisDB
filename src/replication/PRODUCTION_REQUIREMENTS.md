> **Status:** 2026-08-18 – mit aktuellem Replication-Code (`raft_v2.cpp`, Wave A Block 2 Hardening) abgeglichen.

# ThemisDB Replication Module - Production Requirements

## Zweck und Geltungsbereich

Dieses Dokument ist der **kanonische Referenzpunkt für produktive Mindestanforderungen** des Replication-Moduls.
Es definiert verbindliche Betriebs- und Sicherheitsanforderungen für Raft-v2-Consensus, Replication-Policy, Replication-Manager, Lock-Ordering und Timeout-Hardening.

## Dokumentabgrenzung (Canonical Split)

- **`src/replication/PRODUCTION_REQUIREMENTS.md` (dieses Dokument):** verpflichtende Produktionsanforderungen (MUST/MUST NOT), Sicherheitsannahmen, Betriebsgrenzen, Lock-Hierarchie, Timeout-Konfiguration.
- **`src/replication/README.md`:** Funktionsübersicht, Architekturkontext, API- und Nutzungsbeispiele.
- **`src/replication/ROADMAP.md`:** Lieferphasen, offene/abgeschlossene Features, Readiness-Planung.
- **`src/replication/FUTURE_ENHANCEMENTS.md`:** mittelfristige/langfristige Erweiterungen und Forschungsfelder.

## Verbindliche Produktionsanforderungen

- **MUST:** Raft-Leader-Election mit konfigurierten Election-Timeouts; kein Split-Brain ohne definierte Resolution-Policy.
- **MUST:** Replication-Policy aktiv; Replikationskonflikte werden mit Metadaten persistiert (Fail-Closed, keine Silent-Drops).
- **MUST:** Alle sicherheitsrelevanten Konfigurationswerte beim Start validiert; fehlende oder ungültige Werte führen zu Fail-Closed-Verhalten.
- **MUST NOT:** Sicherheits- oder Autorisierungs-Checks in Produktionspfaden deaktivieren.

## Verbindliche Lock-Ordering und Timeout-Anforderungen (Wave A Block 2)

### Lock Hierarchy Enforcement (MUST)
- **MUST:** Alle Replication-Module respektieren die 3-Level Lock Hierarchy (Manager → Resource → I/O)
- **MUST:** Keine I/O-Operationen (File, Network, WAL append) unter Level 1 oder Level 2 Locks
- **MUST:** State Copying Pattern: Mutable state wird unter Lock kopiert, Lock freigegeben, dann auf Kopie operiert
- **MUST:** Deadlock Detection: Tests verifizieren kein Circular Wait (siehe test_replication_lock_ordering_focused.cpp)

### Timeout Configuration (MUST)

The following configuration keys MUST be set in production deployments:

```yaml
replication:
  # Lock timeout for background worker thread condition variables
  timeout_ms: 5000  # Default: 5 seconds; adjust based on I/O performance
  
  # Specific timeouts for critical operations
  io_timeout_ms: 10000        # File I/O operations (default: 10 sec)
  wal_append_timeout_ms: 5000 # WAL append operations (default: 5 sec)
  
  # AsyncWalShipper configuration
  wal_shipping:
    max_lag_ms: 1000           # Maximum acceptable replication lag (1 sec)
    max_queue_depth: 4096      # Queue size before backpressure
    timeout_ms: 5000           # Worker loop wake-up timeout
```

### Timeout Guarantee (MUST)
- **MUST:** Condition variable waits use `cv.wait_for(lock, timeout, predicate)` NOT bare `cv.wait()`
- **MUST:** All background worker threads respond to stop signals within timeout window
- **MUST:** Operations abort cleanly on timeout rather than hang indefinitely
- **MUST:** Timeout values are configurable per deployment environment

## Verbindliche Sicherheitsanforderungen

- Sicherheitsrelevante Operationen werden über dedizierte Kontroll-Surfaces geleitet.
- Fehler in sicherheitskritischen Pfaden werden als explizite Fehlercodes propagiert; kein Silent-Permit.
- Audit-Logging für sicherheitsrelevante Operationen aktiv in Produktionsdeployments.

## Betriebsgrenzen

- Konfigurationswerte müssen deployment-spezifisch gesetzt sein; Default-Werte gelten nicht als produktionssicher.
- Ressourcen-Limits (Größen, Counts, Timeouts) müssen mit den Deployment-Anforderungen übereinstimmen.
- Externe Abhängigkeiten müssen mit expliziten Verbindungs-Timeouts und Retry-Policies konfiguriert sein.
- Lock Hold Time Limits (Überschreitungen führen zu Warnings):
  - Level 1 locks: < 1 ms typical (map operations only)
  - Level 2 locks: < 1 ms typical (state copy only)
  - Level 3 locks: < 100 ms typical (queue operations, no I/O)

## Minimaler Produktions-Check (Audit-fähig)

- [ ] Modul-Konfiguration vollständig und beim Start validiert
- [ ] Sicherheits- und Autorisierungs-Checks aktiv
- [ ] Ressourcen-Limits explizit konfiguriert (keine Unlimited-Defaults)
- [ ] Lock-Timeout-Werte explizit gesetzt (replication.timeout_ms, etc.)
- [ ] Deadlock-Detection Tests erfolgreich (test_replication_lock_ordering_focused.cpp)
- [ ] Audit-Logging aktiv
- [ ] Externe Abhängigkeiten mit Timeout und Retry konfiguriert
- [ ] Produktionsmodus via `THEMIS_PRODUCTION_MODE` oder `THEMIS_ENVIRONMENT` gesetzt
- [ ] Lock hierarchy documentation present in all 6 target files (verified)
- [ ] No raw `mutex.lock()` or `cv.wait()` patterns (timeout-guarded only)

## Review / Sourcecode-Audit-Nachweis

### Betroffene Dateien im Review (Wave A Block 2)

- `src/replication/PRODUCTION_REQUIREMENTS.md` (dieses Dokument)
- `src/replication/ARCHITECTURE.md` (Lock Hierarchy Diagramm hinzugefügt)
- `src/replication/raft_v2.cpp` (Lock-Ordering-Violation behoben)
- `src/replication/replication_slot.cpp` (Lock Hierarchy dokumentiert)
- `src/replication/event_stream.cpp` (Lock Hierarchy dokumentiert)
- `src/replication/async_wal_shipper.cpp` (Lock Hierarchy + Timeout Guards hinzugefügt)
- `src/replication/logical_replication.cpp` (Lock Hierarchy dokumentiert)
- `src/replication/multi_tier_replication.cpp` (Lock Hierarchy + Scope Optimization hinzugefügt)
- `tests/test_replication_lock_ordering_focused.cpp` (10 Focused Tests, ≥8 für Lock-Ordering & Timeouts)

### Verifikationsergebnisse

**Circular Lock Ordering Gaps (Wave A Block 2):**
- [x] raft_v2.cpp: writeEntry() WAL append lock violation FIXED (moved outside lock)
- [x] replication_slot.cpp: lag() external call lock-free pattern verified
- [x] event_stream.cpp: callbacks outside locks verified
- [x] async_wal_shipper.cpp: handler invocation lock-free pattern verified
- [x] logical_replication.cpp: Level 1→2 hierarchy documented
- [x] multi_tier_replication.cpp: Level 1→2 hierarchy scope-optimized

**No Timeout Gaps (Wave A Block 2):**
- [x] async_wal_shipper.cpp: cv.wait_for() with 1-second timeout implemented
- [x] All blocking I/O wrapped: No bare cv.wait() patterns found
- [x] Configuration keys documented: replication.timeout_ms, wal_shipping.timeout_ms
- [x] Test coverage: 10 tests including timeout validation (test_replication_lock_ordering_focused.cpp)

