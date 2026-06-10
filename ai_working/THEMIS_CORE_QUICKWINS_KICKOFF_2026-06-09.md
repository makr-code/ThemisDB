# Themis Core Quickwins Kickoff (2026-06-09)

## Ziel

Schneller, risikoarmer Abbau von Critical/High-Gaps im themis_core-Scope mit direkt validierbaren Änderungen.

## Datenbasis

- ai_working/gap_scan_results.json
- ai_working/gap_scope_breakdown_20260604.md
- ai_working/gap_scan_v3_preflight_actionable_queue.json

## Priorisierte Module (Kickoff)

1. network (`src/network/wire_protocol_server.cpp`)
2. replication (`src/replication/replication_manager.cpp`)
3. server (`src/server/http_server.cpp`)

## Quickwins

- [x] QW-core-01: Bounded retry/backoff beim Wire-Listener-Setup (opweiten/bind/listen) in `wire_protocol_server.cpp`.
  - Wirkung: Reduziert harte Startfehler bei transienten Bind/Listen-Problemen; adressiert `no_retry_logic`-Klasse im Startpfad.
  - Validierung: `cmake --build --preset windows-release --target themis_network --parallel 16` erfolgreich (nach transientem Windows-Dateilock-Retry).

- [x] QW-core-02: Replication-Stop-Pfad hardenen (interruptible waits im Stream-Loop), Fokus auf `thread_join_no_timeout`.
  - Umsetzung: `ReplicationStream::stop()` signalisiert jetzt `wait_cv_`; Backoff- und Batch-Waits in `streamLoop()` sind per `condition_variable::wait_for` unterbrechbar.
  - Validierung: `cmake --build --preset windows-release --target test_replication_manager_addReplica_simple --parallel 16` erfolgreich.

- [x] QW-core-03: HTTP-Handler-Guardrails fuer Null-/Auth-Pfade vereinheitlichen, Fokus auf `null_dereference`/`auth_bypass`-Risiken in Hot-Endpoints.
  - Umsetzung: Zentraler fail-closed Guard (`ensure_handler_ready`) im Route-Switch; Null-Checks fuer WAL/Admin/Entity/Query/Index/Spatial Hot-Routen vor Handler-Aufruf.
  - Validierung: `cmake --build --preset windows-release --target themis_server --parallel 16` erfolgreich.

- [x] QW-core-04: Zero-Copy-Logger Hot-Path defensiv hardenen und leichte Allokationslast senken.
  - Umsetzung: In `zero_copy_logger.cpp` konservatives `reserve()` im JSON-Escape-Pfad, defensiv initialisierte Stack-Buffer (`char[8]{}`, `char[128]{}`) und sign-sichere Zeichennormalisierung.
  - Validierung: `Build_CMakeTools` fuer Target `themis_server` erfolgreich (inkl. Rebuild von `zero_copy_logger.cpp`).

## Akzeptanzkriterien pro Quickwin

- Kompiliert im betroffenen Modul-/Target-Build.
- Keine neuen Editor-/Compile-Fehler im geänderten Bereich.
- Änderung bleibt klein und lokal (kein Architektur-Branching, keine Legacy-Pfade).
