# Wire Protocol Server Compile Fix Plan (2026-06-21)

## Scope
- Ziel: Erste Parse-/Type-Kaskade in src/themis/wire_protocol_server.cpp beseitigen.
- Fokus: json-Typ/Include und Protobuf-abhängige JSON-Helper absichern.

## Affected Files
- src/themis/wire_protocol_server.cpp

## Acceptance
- Fokustarget test_themis_wire_protocol_server_focused baut mindestens bis zum nächsten echten Fehlercluster.
- Keine API-Signaturänderung an public Headern.

## Verification
- cmake --build --preset windows-release --target test_themis_wire_protocol_server_focused --parallel 1
