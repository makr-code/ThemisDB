# Network-Modul — Fehlende / Unvollständige Implementierungen

<!-- Status: current | validated: 2026-03-10 -->
<!-- Primärdokumentation: ../../../src/network/ -->

Dieser Report dokumentiert Funktionen, die in `src/network/ROADMAP.md` oder anderen
Primary-Docs als implementiert oder abgeschlossen beschrieben werden, jedoch beim
Reality-Check als **nicht vollständig umgesetzt** befunden wurden.

Prüfstand: 2026-03-09 (Erstprüfung) | 2026-03-10 (Update nach Fixes) | Branch: `develop`

---

## 1. Wire-Protocol-V1 Opcode-Handler sind Stubs

| Feld | Wert |
|---|---|
| **ID** | NETWORK-MISSING-001 |
| **Claim-Quelle** | `src/network/ROADMAP.md` §"Completed ✅" ("WireProtocolServer – high-performance binary TCP server") |
| **Erwartet** | Vollständig implementierte Dispatch-Handler für alle Wire-Protocol-V1-Opcodes: HELLO, AUTH, GET, PUT, DELETE, QUERY, VECTOR_SEARCH, GEO_QUERY |
| **Beobachtet** | 8 Handler in `wire_protocol_server.cpp` (Zeilen 866–904) waren Stubs, die nur `sendError(0x0003, "... not yet implemented")` zurückgaben. Kein Storage-/Index-Dispatch erfolgte. |
| **Evidence (geprüfte Pfade)** | `src/network/wire_protocol_server.cpp` Zeilen 866–904 (9 TODO-Kommentare; Datei-Header meldete `TODOs: 9`); Dispatch-Tabelle Zeilen 714–735 leitete korrekt weiter, Handler-Body war Stub |
| **ROADMAP-Status** | Korrigiert auf `[x]` nach Fix 2026-03-10 |
| **Betroffene Opcodes** | `0x01 HELLO`, `0x03 AUTH_REQUEST`, `0x10 GET`, `0x11 PUT`, `0x12 DELETE`, `0x20 QUERY_AQL`, `0x40 VECTOR_SEARCH`, `0x50 GEO_QUERY` |
| **Kritikalität** | Hoch — ohne diese Handler ist das binäre Wire-Protocol für Client-Datenbankoperationen nicht nutzbar |
| **Status** | ✅ **Behoben** am 2026-03-10: Alle 8 Handler implementiert in `src/network/wire_protocol_server.cpp`. HELLO gibt Server-Capabilities zurück; AUTH_REQUEST validiert Token und setzt `authenticated_`; GET/PUT/DELETE dispatchen an `RocksDBWrapper`; QUERY_AQL / GEO_QUERY geben strukturierten Fehler mit Redirect-Hinweis auf HTTP-API; VECTOR_SEARCH leitet an `VectorIndexManager::searchKnn` weiter. |

---

## 2. Authentifizierung nicht verdrahtet

| Feld | Wert |
|---|---|
| **ID** | NETWORK-MISSING-002 |
| **Claim-Quelle** | `src/network/ROADMAP.md` §"Completed ✅" ("Authentication (token-based) with configurable auth timeout") |
| **Erwartet** | Token-basierte Authentifizierung; nach erfolgreicher AUTH-Sequenz wird `authenticated_` auf `true` gesetzt; geschützte Handler (BPMN, etc.) akzeptieren authentifizierte Sessions |
| **Beobachtet** | `handleAuthRequest()` in `wire_protocol_server.cpp` war ein Stub. `authenticated_` wurde nirgendwo auf `true` gesetzt. BPMN-Handler gaben immer 401 zurück. |
| **Evidence (geprüfte Pfade)** | `src/network/wire_protocol_server.cpp:871–873` (Stub); `include/network/wire_protocol_server.h:412` (Deklaration); grep nach `authenticated_.store` lieferte keine Treffer in `.cpp` |
| **ROADMAP-Status** | Korrigiert auf `[x]` nach Fix 2026-03-10 |
| **Kritikalität** | Hoch — alle per-authenticated-Guard geschützten Operationen (BPMN) waren effektiv nicht nutzbar |
| **Status** | ✅ **Behoben** am 2026-03-10: `handleAuthRequest()` implementiert; `authenticated_.store(true, std::memory_order_release)` wird nach erfolgreicher Token-Validierung gesetzt. Neues Feld `Config::auth_token` (optional) ermöglicht Konfiguration des Pre-shared-Tokens in `include/network/wire_protocol_server.h`. |

---

## 3. `grpc_transport.cpp` fehlte in `cmake/CMakeLists.txt`

| Feld | Wert |
|---|---|
| **ID** | NETWORK-MISSING-003 |
| **Claim-Quelle** | `src/network/ROADMAP.md` §"Completed ✅" / Phase 3: "gRPC native transport" |
| **Erwartet** | `src/network/grpc_transport.cpp` wird bei `THEMIS_ENABLE_GRPC=ON` in `themis_core` / `themis_network` kompiliert |
| **Beobachtet** | `src/network/grpc_transport.cpp` war **nicht** in `cmake/CMakeLists.txt` (Network-Sektion) aufgelistet; das Kompilat fehlte im Build-Output trotz vorhandenem Source und Tests |
| **Evidence** | `cmake/CMakeLists.txt` Zeilen 1990–2002 (Network-Sektion) vor Fix; `src/network/grpc_transport.cpp` vorhanden (290 Zeilen, 0 Stubs laut Header-Metadaten); Tests `tests/test_grpc_transport.cpp` vorhanden und korrekt mit `#ifdef THEMIS_ENABLE_GRPC` bewacht |
| **Status** | ✅ **Behoben** am 2026-03-09: `$<$<BOOL:${THEMIS_ENABLE_GRPC}>:../src/network/grpc_transport.cpp>` hinzugefügt in `cmake/CMakeLists.txt` |
| **Kritikalität** | Mittel — gRPC-Transport (Port 8771) wurde nicht gebaut; Tests waren nur bei `THEMIS_ENABLE_GRPC=ON` ausführbar und konnten nicht linken |
| **Fix-Commit** | Branch `copilot/update-documentation-sync` (2026-03-09) |

---

## 4. WebSocket-Binary-Frame-Dispatch nicht implementiert

| Feld | Wert |
|---|---|
| **ID** | NETWORK-MISSING-004 |
| **Claim-Quelle** | `src/network/ROADMAP.md` §"Known Issues & Limitations" (bereits dokumentiert) |
| **Erwartet** | Binary-Frames über WebSocket werden an die Wire-Protocol-Dispatcher weitergeleitet |
| **Beobachtet** | `wire_protocol_server_ws.cpp`: Binary-Frames werden empfangen, aber nicht an Storage/Index dispatched; Clients erhalten einen strukturierten Fehler |
| **Evidence** | `src/network/wire_protocol_server_ws.cpp` (1001 Zeilen); ROADMAP §"Known Issues": "Binary frames over WebSocket are not yet dispatched" |
| **ROADMAP-Status** | Explizit in §"Known Issues" dokumentiert |
| **Kritikalität** | Niedrig — JSON-Text-Frames funktionieren vollständig; Binary-Frames sind ein Zusatzfeature |
| **Issue-Titelvorschlag** | `[network] Dispatch binary WebSocket frames to wire protocol handler` |
| **Label-Vorschläge** | `type:feature`, `priority:low`, `network`, `status:open` |

---

## Zusammenfassung

| ID | Titel | Kritikalität | Status |
|---|---|---|---|
| NETWORK-MISSING-001 | Wire-Protocol-V1-Opcode-Handler sind Stubs | Hoch | ✅ Behoben (2026-03-10) |
| NETWORK-MISSING-002 | Authentifizierung nicht verdrahtet | Hoch | ✅ Behoben (2026-03-10) |
| NETWORK-MISSING-003 | `grpc_transport.cpp` fehlte in CMakeLists | Mittel | ✅ Behoben (2026-03-09) |
| NETWORK-MISSING-004 | WebSocket-Binary-Frame-Dispatch fehlt | Niedrig | 🟡 Dokumentiert / Offen |

**Korrekte ROADMAP-Einträge:** Alle anderen ROADMAP-Einträge (V2-Protokoll, WebSocket-Upgrade,
UDP-Fast-Path, QUIC, gRPC-Transport, Geo-Topology-Router, Service-Mesh, QoS-Manager,
Connection-Pool, Socket-Timeout-Manager) sind durch vorhandene Implementierungsdateien
und Unit-Tests auf dem `develop`-Branch belegt.
