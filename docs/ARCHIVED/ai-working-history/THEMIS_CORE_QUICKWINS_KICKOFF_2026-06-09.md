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

- [x] QW-core-05: LockFreeMetrics-Flush-Thread Shutdown interruptible machen (thread_join_no_timeout Hardening).
  - Umsetzung: In `lockfree_metrics` `condition_variable`-basiertes `wait_for` statt reinem `sleep_for` in `flushLoop()`, sofortiges Wake-up in `stopFlushThread()` via `notify_all()`, und kein Start des Flush-Threads bei `flush_interval<=0`.
  - Validierung: `Build_CMakeTools` fuer Target `themis_server` erfolgreich (result code 0; Rebuild inkl. `lockfree_metrics.cpp`).

- [x] QW-core-06: RedisCache-Subscriber-Shutdown interruptible machen (thread_join_no_timeout Hardening).
  - Umsetzung: In `redis_cache` ersetztes reconnect-slice-sleep durch `condition_variable::wait_for` mit Stop-Pruefung; `shutdown()` signalisiert jetzt aktiv per `notify_all()` vor `join()`.
  - Validierung: Isolierter Core-Build erfolgreich mit `cmake --build --preset windows-release --target themis_base --parallel 16`; dazu Editor-/Compile-Diagnostics fuer `redis_cache.{h,cpp}` sauber.

- [x] QW-core-07: Generated-Protobuf-Warnungsrauschen (C4267) auf Source-Ebene kapseln.
  - Umsetzung: In `cmake/CMakeLists.txt` gezielte MSVC-Source-Property `COMPILE_OPTIONS "/wd4267"` nur fuer generierte Protobuf-Translation-Units (`themis_wire_v1.pb.cc`, `themisdb.pb.cc`, `themisdb.grpc.pb.cc`), inkl. bestehender Unity-Excludes im Modularpfad.
  - Validierung: Re-Configure + Build (`CMake: Configure (windows-release)`, `CMake: Build (windows-release)`) ohne C4267-Treffer im Build-Log.

- [x] QW-core-08: MSVC D9025 Flag-Konflikt (`/O2` vs `/Od`) im Modular-Serverpfad entfernen.
  - Umsetzung: In `cmake/ModularBuild.cmake` die per-Source-Compile-Option fuer `monitoring_api_handler.cpp` und `index_api_handler.cpp` auf config-gated Debug-Only umgestellt (`/Od` nur fuer non-Release), bei Erhalt von `/bigobj` und `/Zm200`.
  - Validierung: Re-Configure + Build (`CMake: Configure (windows-release)`, `CMake: Build (windows-release)`) ohne D9025-Treffer im Build-Log.

- [x] QW-core-09: MSVC C1060 Heap-OOM im Monitoring-API-Compilepfad entschärfen.
  - Umsetzung: In `cmake/CMakeLists.txt` die monolithischen Server-TUs `monitoring_api_handler.cpp` und `index_api_handler.cpp` zusätzlich mit `COMPILE_OPTIONS "/bigobj;/Zm200"` versehen und gleichzeitig aus Unity ausgeschlossen belassen.
  - Validierung: `cmake --build --preset windows-release --target themis_network --parallel 16` erfolgreich; vorheriger C1060-Abbruch auf `monitoring_api_handler.cpp` trat nicht mehr auf.

- [x] QW-core-10: Drei produktive Warnungs-Hotspots lokal bereinigt (`C4456`, `C4189`).
  - Umsetzung: In `src/importers/federated_learning.cpp` inneres `sum` auf `trimmed_sum` umbenannt (kein Shadowing mehr), in `src/llm/lora_framework/lora_training_service.cpp` ungenutzte Variable `batches_per_epoch` entfernt, und in `src/index/vector_index.cpp` `vector_data` nur im tatsächlich benötigten Scope geführt.
  - Validierung: `cmake --build --preset windows-release --target themis_network --parallel 16` erfolgreich (inkrementell) ohne die zuvor gemeldeten Warnstellen.

- [x] QW-core-11: Signed/Unsigned-Vergleiche in Gossip-Update-Alterspfad bereinigt (`C4018`).
  - Umsetzung: In `src/sharding/gossip_config_manager.cpp` `now_ns` an beiden relevanten Stellen explizit als `uint64_t` geführt (statt implizitem signed `count()`-Typ), um Vergleiche mit `timestamp_ns` robust und warnfrei zu machen.
  - Validierung: `cmake --build --preset windows-release --target themis_network --parallel 16` ohne `C4018`-Treffer auf den zuvor gemeldeten Zeilen.

- [x] QW-core-12: Narrowing im Robustness-Scoring entschärft (`C4244`).
  - Umsetzung: In `src/rag/adversarial_tester.cpp` den `std::count_if`-Rückgabewert als `const auto` statt `long` geführt, damit keine implizite `__int64 -> long`-Konvertierung mehr erfolgt.
  - Validierung: `cmake --build --preset windows-release --target themis_network --parallel 16` ohne `C4244`-Treffer auf der gemeldeten Stelle.

- [x] QW-core-13: Generated shard-proto Headerwarnung (`C4267`) im Sharding-Modul gekapselt.
  - Umsetzung: In `cmake/ModularBuild.cmake` für `themis_sharding` (nur MSVC, nur bei aktivem `themis_shard_proto`) gezielt `/wd4267` ergänzt, damit `shard_rpc.pb.h`-Narrowing nicht in Consumer-TUs streut.
  - Validierung: `cmake --build --preset windows-release --target themis_network --parallel 16` ohne `C4267`-Treffer auf `shard_rpc.pb.h`.

- [x] QW-core-14: `[[nodiscard]]`-Rückgaben von `freeGPU`/`freeCPU` explizit behandelt (`C4834`).
  - Umsetzung: In `src/llm/active_vram_allocator.cpp` vier Best-Effort-Aufrufe von `gpu_mgr_->freeGPU/freeCPU` mit `static_cast<void>(...)` umschlossen; semantik ist bewusst fire-and-forget (Speicherfreigabe im VRAM-Allocator-Teardown).
  - Validierung: Inkrementeller Build erfolgreich ohne C4834-Zeilen im Output.

- [x] QW-core-15: Nicht referenzierte statische Helfer in WAL-Storage markiert (`C4505`).
  - Umsetzung: In `src/storage/wal_storage.cpp` die vier bounds-safe Overloads (`encode_u32/u64`, `decode_u32/u64`) mit `[[maybe_unused]]` versehen, da sie in bestimmten Unity-Builds nicht referenziert werden.
  - Validierung: Inkrementeller Build erfolgreich ohne C4505-Zeilen auf `wal_storage.cpp`.

## Akzeptanzkriterien pro Quickwin

- Kompiliert im betroffenen Modul-/Target-Build.
- Keine neuen Editor-/Compile-Fehler im geänderten Bereich.
- Änderung bleibt klein und lokal (kein Architektur-Branching, keine Legacy-Pfade).
