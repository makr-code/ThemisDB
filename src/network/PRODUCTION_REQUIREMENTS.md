---
Author: "ThemisDB Contributors"
Created: "unknown (pre-existing document; creation date not recorded)"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

> Exact model backend not verifiable from final document text alone.
>
> **Source review (2026-10-31):** `NetworkAuditLog` is compiled into `themis_core`
> and owned by `WireProtocolServer`. The server records connection lifecycle,
> authentication outcomes, and rate-limit rejections. Events are retained in a
> bounded in-memory buffer; durable storage/export requires a configured callback.

# ThemisDB Network Module - Production Requirements

## Zweck und Geltungsbereich

Dieses Dokument ist der **kanonische Referenzpunkt für produktive Mindestanforderungen** des Network-Moduls.
Es definiert verbindliche Anforderungen für Transportabsicherung, Authentifizierung, Connection-Limits, Timeout-Verhalten und Protokoll-Härtung.

## Dokumentabgrenzung (Canonical Split)

- **`src/network/PRODUCTION_REQUIREMENTS.md` (dieses Dokument):** verpflichtende Produktionsanforderungen (MUST/MUST NOT), Sicherheitsannahmen, Betriebsgrenzen.
- **`src/network/README.md`:** Funktionsübersicht, Architekturkontext, API- und Nutzungsbeispiele.
- **`src/network/ROADMAP.md`:** Lieferphasen, offene/abgeschlossene Features, Readiness-Planung.
- **`src/network/FUTURE_ENHANCEMENTS.md`:** mittelfristige/langfristige Erweiterungen und Forschungsfelder.

## Verbindliche Transportanforderungen

- **MUST:** `validateTransportSecurity(...)` vor `start()` aufrufen; `start()` prüft die Transportsicherheit ebenfalls als Fail-Closed-Gate.
- **MUST:** `auth_token` nicht leer/blank; Startup-Validierung des Wire-Protocol-Servers weist leere Tokens ab.
- **MUST:** Constant-time Token-Compare (`CRYPTO_memcmp`) in Auth-Validierungspfaden aktiv; kein String-Compare mit Early-Return.
- **MUST NOT:** Protokoll-Downgrade zu Klartextverbindungen in Produktionsdeployments erlauben.

## Verbindliche Sicherheitsanforderungen

### 1) Session-Authentication und Request-Gating

- **MUST:** Auth-Checks im `wire_protocol_server.cpp` laufen vor sensitiven Operation-Handlern; `401`-Pfad aktiv.
- **MUST:** Frame- und Payload-Size-Validierung aktiv; oversized Payloads werden mit explizitem Fehlercode abgewiesen.
- **MUST NOT:** Sessions ohne Auth-Check zu sensiblen Operationen weiterleiten.

### 2) Connection- und Rate-Limits

- **MUST:** Connection-Limits pro IP konfiguriert; unbegrenzte Verbindungen von einer Quelle sind nicht zulässig.
- **MUST:** `socket_timeout_manager.cpp` aktiv; inaktive Verbindungen werden nach konfiguriertem Timeout geschlossen.
- **MUST:** `adaptive_circuit_breaker.cpp` konfiguriert; offene Breaker-Zustände werden beobachtet.

### 3) Netzwerk-Audit-Logging

- **MUST:** `NetworkAuditLog` in Produktionspfade integrieren, damit sicherheitsrelevante Verbindungsereignisse protokolliert werden.
- **Status:** Im `WireProtocolServer` integriert; `getAuditLog()` bietet Zugriff auf die servereigene, begrenzte Ereignis-Queue und den optionalen Callback.
- **Betriebsgrenze:** Ohne Callback verbleiben Ereignisse ausschließlich im Arbeitsspeicher (Standardkapazität: 10.000); Persistenz/Export muss der Betreiber über einen nicht-blockierenden Sink konfigurieren.

## Betriebsgrenzen (aktuelles Network-Verhalten)

- `io_uring_batcher.cpp` und `kernel_bypass.cpp` für High-Throughput-Szenarien; Kernel-Bypass-Pfade erfordern ausdrückliche Betriebsfreigabe.
- `geo_topology_router.cpp` und `envoy_xds.cpp` benötigen valide Topology-Konfiguration; falsche Routing-Tabellen führen zu Fehlrouting.
- `connection_compression.cpp` kann aktiviert werden; Kompression-Level muss gegen Latenz-Anforderungen abgewogen sein.

## Minimaler Produktions-Check (Audit-fähig)

- [ ] Transport-Security-Validierung beim Start aktiv
- [ ] auth_token nicht leer/blank
- [ ] Constant-time Token-Compare aktiv (kein String-Compare)
- [ ] Frame/Payload-Size-Validierung aktiv
- [ ] Connection-Limits pro IP konfiguriert
- [ ] Socket-Timeout-Manager aktiv
- [ ] Circuit-Breaker konfiguriert und beobachtbar
- [x] Network-Audit-Log in Wire-Protocol-Produktionspfaden integriert und mit fokussiertem Integrationstest belegt
- [ ] Produktionsmodus durch den aufrufenden Prozess gemäß `TransportSecurityChecker` konfiguriert; konkrete Startparameter außerhalb dieses Moduls verifizieren

## Review / Sourcecode-Audit-Nachweis

### Betroffene Dateien im Review

- `src/network/PRODUCTION_REQUIREMENTS.md`
- `src/network/wire_protocol_server.cpp`
- `src/network/wire_protocol_server_ws.cpp`
- `src/network/socket_timeout_manager.cpp`
- `src/network/adaptive_circuit_breaker.cpp`
- `src/network/network_audit_log.cpp`
