# Wave 1 Ticket-Plan: server + llm (2026-05-25)

Ziel von Wave 1:
- Kritische Risiken zuerst senken (data_race, no_timeout, null/pointer safety).
- Danach High-Volumen-Hotspots mit maximalem Hebel abbauen.
- Messbar: Reduktion in `gap_scan_v3_server.json` und `gap_scan_v3_llm.json`.

Ausgangslage (Rescan):
- server: HIGH=14066, CRITICAL=396, TOTAL=17603
- llm: HIGH=18885, CRITICAL=1414, TOTAL=23543

## GitHub-Verknuepfung (P0 erstellt)
- W1-S01 -> #5348
- W1-S02 -> #5349
- W1-L01 -> #5350
- W1-L02 -> #5351
- W1-L03 -> #5352

## Reihenfolge (strict priority)

## W1-S01 (P0)
Titel: server Query API - Data-Race und Timeout-Kernpfade schließen
- Dateien:
  - src/server/query_api_handler.cpp
- Fokus-Typen:
  - data_race
  - no_timeout
  - null_dereference
- Akzeptanz:
  - keine lock-freien Shared-State-Schreibzugriffe in Request-/Job-Pfaden
  - blockierende Pfade mit explizitem Timeout/Abbruchpfad
  - keine neuen Build-Warnungen
- Verifikation:
  - focused tests fuer query-api/server
  - Gap-Delta in server-report dokumentiert

## W1-S02 (P0)
Titel: server HTTP Core - Concurrency-Hotspots entkoppeln
- Dateien:
  - src/server/http_server.cpp
- Fokus-Typen:
  - data_race
  - iterator_invalidation
  - no_timeout
- Akzeptanz:
  - konsistente Sperrreihenfolge in Session-/Connection-Verwaltung
  - keine Iterator-Invalidierungen in Mutationspfaden
- Verifikation:
  - focused server/http tests
  - Gap-Delta fuer `http_server.cpp`

## W1-S03 (P1)
Titel: server MCP + Voice Handler - Null-/Pointer-Guards standardisieren
- Dateien:
  - src/server/mcp_server.cpp
  - src/server/voice_api_handler.cpp
- Fokus-Typen:
  - null_dereference
  - pointer_arithmetic
- Akzeptanz:
  - defensive Guards vor Zugriff auf optionale Ressourcen
  - konsistente Fehlerantwort statt implizitem Crashpfad
- Verifikation:
  - focused handler tests

## W1-S04 (P1)
Titel: server Postgres Session + RPC Service - Retry/Timeout Verhalten härten
- Dateien:
  - src/server/postgres_session.cpp
  - src/server/rpc/rpc_service_impl.cpp
- Fokus-Typen:
  - no_retry_logic
  - no_timeout
  - uncaught_exception
- Akzeptanz:
  - transiente Fehler mit begrenztem Retry-Budget
  - einheitliche Timeout-Fehlercodes
- Verifikation:
  - focused postgres/rpc tests

## W1-S05 (P1)
Titel: server Cache Admin + SSE Manager - kritische Nebenpfade stabilisieren
- Dateien:
  - src/server/cache_admin_api_handler.cpp
  - src/server/sse_connection_manager.cpp
- Fokus-Typen:
  - data_race
  - iterator_invalidation
- Akzeptanz:
  - kein unsynchronisiertes Entfernen/Iterieren auf Shared-Containern
- Verifikation:
  - focused cache/sse tests

## W1-S06 (P2)
Titel: server Exception-Sicherheitskanten schließen
- Dateien:
  - src/server/llm_api_handler.cpp
  - src/server/http3_session.cpp
- Fokus-Typen:
  - uncaught_exception
  - no_timeout
- Akzeptanz:
  - externe Aufrufe in guardierten Try/Catch-Zonen
  - eindeutige Recovery-Pfade
- Verifikation:
  - focused http3/llm handler tests

## W1-S07 (P2)
Titel: server Scanner-Noise reduzieren (unknown cluster triage)
- Dateien:
  - src/server/http_server.cpp
  - src/server/query_api_handler.cpp
- Fokus-Typen:
  - unknown
- Akzeptanz:
  - identifizierte false-positives dokumentiert oder durch klarere Guards eliminiert
- Verifikation:
  - Vergleich der `unknown`-Anteile vor/nach

## W1-L01 (P0)
Titel: llm Multi-LoRA Manager - zentrale Race/Lock-Probleme beheben
- Dateien:
  - src/llm/multi_lora_manager.cpp
- Fokus-Typen:
  - data_race
  - no_timeout
  - null_dereference
- Akzeptanz:
  - konsistente Synchronisierung fuer mutable Manager-States
  - keine blockierenden Locks ohne Timeout
- Verifikation:
  - focused llm/multi-lora tests

## W1-L02 (P0)
Titel: llm Training Service - kritische Parallelpfade absichern
- Dateien:
  - src/llm/lora_framework/lora_training_service.cpp
- Fokus-Typen:
  - data_race
  - iterator_invalidation
  - no_timeout
- Akzeptanz:
  - thread-sichere Queue/Batch-Verarbeitung
  - keine invalidierten Iteratoren bei parallel mutation
- Verifikation:
  - focused training tests

## W1-L03 (P0)
Titel: llm Vulkan/DirectX Kernel-Schnittstellen robust machen
- Dateien:
  - src/llm/lora_framework/kernels/vulkan_kernels.cpp
  - src/llm/lora_framework/kernels/directx_kernels.cpp
- Fokus-Typen:
  - data_race
  - smart_ptr_misuse
  - no_timeout
- Akzeptanz:
  - deterministische Ressourcen-Lifetime in Kernel-Dispatch
  - keine unsynchronisierten Shared-Ressourcen
- Verifikation:
  - focused kernel tests + build auf windows-release

## W1-L04 (P1)
Titel: llm Llama Wrapper + Inference Engine - Pointer/Null Sicherheit erhöhen
- Dateien:
  - src/llm/llama_wrapper.cpp
  - src/llm/inference_engine_enhanced.cpp
- Fokus-Typen:
  - pointer_arithmetic
  - null_dereference
- Akzeptanz:
  - belastbare Bounds-/Null-Prüfungen in allen externen Bufferpfaden
- Verifikation:
  - focused inference tests

## W1-L05 (P1)
Titel: llm AQL Train Parser - Eingabe- und Ausnahmehärtung
- Dateien:
  - src/llm/aql_train_parser.cpp
- Fokus-Typen:
  - uncaught_exception
  - pointer_arithmetic
- Akzeptanz:
  - parser errors liefern klare Fehlermeldung statt Abbruch
  - keine ungeprüften Index-/Pointerzugriffe
- Verifikation:
  - focused parser tests

## W1-L06 (P1)
Titel: llm GPU Memory Manager - resource lifecycle und Race-Pfade stabilisieren
- Dateien:
  - src/llm/gpu_memory_manager.cpp
- Fokus-Typen:
  - data_race
  - db_connection_leak
  - uninitialized_access
- Akzeptanz:
  - klarer Ownership-Lebenszyklus
  - keine parallelen Writes ohne Guard
- Verifikation:
  - focused gpu-memory tests

## W1-L07 (P2)
Titel: llm Unknown-Cluster auflösen und scanner-freundliche Guards ergänzen
- Dateien:
  - src/llm/multi_lora_manager.cpp
  - src/llm/llama_wrapper.cpp
  - src/llm/lora_framework/lora_training_service.cpp
- Fokus-Typen:
  - unknown
- Akzeptanz:
  - dokumentierte Triage fuer verbleibende unknown-Befunde
  - echte Befunde in konkrete Typen überführt oder behoben
- Verifikation:
  - unknown-Anteil in llm-report sinkt

## Mess- und Abschlusskriterien Wave 1
- Pflichtmessung nach jedem Ticket-Block:
  - `tools/gap_scanner_v3.py` (oder modulare Teilmessung, wenn vorhanden)
  - Delta-Export in `ai_working/gap_scan_v3_rescan_delta_2026-05-25.json`-Format
- Wave-1-Zielkorridor:
  - server: CRITICAL < 300
  - llm: CRITICAL < 1000
  - combined actionable (server+llm) deutlich sinkend

## Vorschlag Ausführungsreihenfolge in 2 Sprints
Sprint A:
- W1-S01, W1-S02, W1-L01, W1-L02, W1-L03

Sprint B:
- W1-S03, W1-S04, W1-S05, W1-S06, W1-S07,
- W1-L04, W1-L05, W1-L06, W1-L07
