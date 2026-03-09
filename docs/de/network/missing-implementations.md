# Network-Modul — Fehlende / Unvollständige Implementierungen

<!-- Status: current | validated: 2026-03-09 -->
<!-- Primärdokumentation: ../../../src/network/ -->

Dieser Report dokumentiert Funktionen, die in `src/network/ROADMAP.md` oder anderen
Primary-Docs als implementiert oder abgeschlossen beschrieben werden, jedoch beim
Reality-Check als **nicht vollständig umgesetzt** befunden wurden.

Prüfstand: 2026-03-09 | Branch: `develop`

---

## 1. Wire-Protocol-V1 Opcode-Handler sind Stubs

| Feld | Wert |
|---|---|
| **ID** | NETWORK-MISSING-001 |
| **Claim-Quelle** | `src/network/ROADMAP.md` §"Completed ✅" ("WireProtocolServer – high-performance binary TCP server") |
| **Erwartet** | Vollständig implementierte Dispatch-Handler für alle Wire-Protocol-V1-Opcodes: HELLO, AUTH, GET, PUT, DELETE, QUERY, VECTOR_SEARCH, GEO_QUERY |
| **Beobachtet** | 8 Handler in `wire_protocol_server.cpp` (Zeilen 866–904) sind Stubs, die nur `sendError(0x0003, "... not yet implemented")` zurückgeben. Kein Storage-/Index-Dispatch erfolgt. |
| **Evidence (geprüfte Pfade)** | `src/network/wire_protocol_server.cpp` Zeilen 866–904 (9 TODO-Kommentare; Datei-Header meldet `TODOs: 9`); Dispatch-Tabelle Zeilen 714–735 leitet korrekt weiter, Handler-Body ist Stub |
| **ROADMAP-Status** | Fälschlicherweise `[x]` — korrigiert auf `[~]` (authentication) im ROADMAP-Update 2026-03-09 |
| **Betroffene Opcodes** | `0x01 HELLO`, `0x03 AUTH_REQUEST`, `0x04 GET`, `0x05 PUT`, `0x06 DELETE`, `0x07 QUERY`, `0x08 VECTOR_SEARCH`, `0x09 GEO_QUERY` |
| **Kritikalität** | Hoch — ohne diese Handler ist das binäre Wire-Protocol für Client-Datenbankoperationen nicht nutzbar |
| **Issue-Titelvorschlag** | `[network] Implement WireProtocolServer V1 opcode handlers (HELLO, AUTH, GET, PUT, DELETE, QUERY, VECTOR_SEARCH, GEO_QUERY)` |
| **Label-Vorschläge** | `type:feature`, `priority:high`, `network`, `status:open` |

---

## 2. Authentifizierung nicht verdrahtet

| Feld | Wert |
|---|---|
| **ID** | NETWORK-MISSING-002 |
| **Claim-Quelle** | `src/network/ROADMAP.md` §"Completed ✅" ("Authentication (token-based) with configurable auth timeout") |
| **Erwartet** | Token-basierte Authentifizierung; nach erfolgreicher AUTH-Sequenz wird `authenticated_` auf `true` gesetzt; geschützte Handler (BPMN, etc.) akzeptieren authentifizierte Sessions |
| **Beobachtet** | `handleAuthRequest()` in `wire_protocol_server.cpp` (Zeile 871–873) ist ein Stub, der nur `sendError(0x0003, "AUTH not yet implemented")` zurückgibt. `authenticated_` (deklariert in `include/network/wire_protocol_server.h:412` als `std::atomic<bool> authenticated_{false}`) wird nirgendwo im Source auf `true` gesetzt. BPMN-Handler, die auf `authenticated_.load()` prüfen (Zeilen 1179, 1277, 1364), geben immer 401 zurück. |
| **Evidence (geprüfte Pfade)** | `src/network/wire_protocol_server.cpp:871–873` (Stub); `include/network/wire_protocol_server.h:412` (Deklaration); grep nach `authenticated_.store\|authenticated_ =\|authenticated_.exchange` liefert keine Treffer in `.cpp` |
| **ROADMAP-Status** | Fälschlicherweise `[x]` — korrigiert auf `[~]` im ROADMAP-Update 2026-03-09 |
| **Kritikalität** | Hoch — alle per-authenticated-Guard geschützten Operationen (BPMN) sind effektiv nicht nutzbar |
| **Issue-Titelvorschlag** | `[network] Implement token-based authentication in WireProtocolServer (handleAuthRequest, set authenticated_ flag)` |
| **Label-Vorschläge** | `type:feature`, `priority:high`, `network`, `status:open` |

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
| NETWORK-MISSING-001 | Wire-Protocol-V1-Opcode-Handler sind Stubs | Hoch | 🔴 Offen |
| NETWORK-MISSING-002 | Authentifizierung nicht verdrahtet | Hoch | 🔴 Offen |
| NETWORK-MISSING-003 | `grpc_transport.cpp` fehlte in CMakeLists | Mittel | ✅ Behoben (2026-03-09) |
| NETWORK-MISSING-004 | WebSocket-Binary-Frame-Dispatch fehlt | Niedrig | 🟡 Dokumentiert / Offen |

**Korrekte ROADMAP-Einträge:** Alle anderen ROADMAP-Einträge (V2-Protokoll, WebSocket-Upgrade,
UDP-Fast-Path, QUIC, gRPC-Transport, Geo-Topology-Router, Service-Mesh, QoS-Manager,
Connection-Pool, Socket-Timeout-Manager) sind durch vorhandene Implementierungsdateien
und Unit-Tests auf dem `develop`-Branch belegt.
