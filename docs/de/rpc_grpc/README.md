# RPC-gRPC-Plugin

<!-- Status: current | validated: 2026-05-13 -->
<!-- Links: PRIMARY_SOURCES.md · ../../../src/rpc_grpc/README.md · ../../../include/rpc_grpc/README.md -->

**Stand:** 13. Mai 2026<br>
**Version:** 0.3.0<br>
**Kategorie:** RPC / gRPC-Plugin<br>
**Status:** 🟢 Production-Ready

---

## Übersicht

Das Modul `rpc_grpc` stellt den gRPC-basierten RPC-Transport für ThemisDB bereit.
Es liefert den Plugin-Einstiegspunkt (`GRPCPlugin`), den Server-Lifecycle
(`GRPCServer`), TLS/mTLS-Konfiguration (fail-closed), optionale Keepalive- und
Admin-Port-Optionen sowie Observability-Hooks (Metriken und strukturierte Access-Logs).

**Primäre Quelle:** [`src/rpc_grpc/`](../../../src/rpc_grpc/)

---

## Hauptkomponenten

| Komponente | Source | Beschreibung |
|---|---|---|
| `GRPCPlugin` | `src/rpc_grpc/grpc_plugin.h/.cpp` | Plugin-Implementierung von `IRPCPlugin` inkl. Server-Erzeugung |
| `GRPCServer` | `src/rpc_grpc/grpc_plugin.h/.cpp` | Server-Lifecycle, Service-Registrierung, TLS/mTLS, Metriken, Logs |
| `BidiStreamAdapter<Req,Resp>` | `src/rpc_grpc/bidi_stream_adapter.h` | Typisierter Helper für bidirektionale gRPC-Streams mit Backpressure |
| RPC-Vertrag | `include/plugins/rpc_plugin_interface.h` | `RPCServerConfig`, `IRPCServer`, `IRPCPlugin` |

---

## Public API & Konfiguration

- Public-API-Guide: [`include/rpc_grpc/README.md`](../../../include/rpc_grpc/README.md)
- Wichtiger Einstiegspunkt: `rpc_grpc/grpc_plugin.h`
- Relevante Konfigurationsschlüssel:
  - `host`, `port`
  - `tls_enabled`, `tls_cert_path`, `tls_key_path`, `tls_ca_cert_path`, `auth_required`
  - `extra_config["keepalive_time_ms"]`, `extra_config["keepalive_timeout_ms"]`, `extra_config["admin_port"]`

---

## Laufzeitverhalten, Fehlerfälle und Grenzen

- `start()` liefert `false`, wenn der Server bereits läuft.
- TLS-Start ist fail-closed: ungültige oder fehlende Zertifikatsdateien brechen den Start ab.
- Ohne TLS wird explizit auf unsichere Credentials (`InsecureServerCredentials`) gewechselt und geloggt.
- Message-Limits sind auf 100 MiB (Send/Receive) gesetzt.
- `reloadTls()` gilt nur für laufende TLS-Server; bei Fehlern bleiben alte Credentials aktiv.
- `setServiceHealth()`/`isServiceHealthy()` verwalten derzeit nur internen Zustand (kein standardisierter `grpc.health.v1`-Service-Endpunkt).
- `getMetricsText()` bleibt leer, bis `recordRPC()` mindestens einmal aufgerufen wurde.

---

## Usage (Kurzbeispiel)

```cpp
#include "rpc_grpc/grpc_plugin.h"

using namespace themis::plugins::rpc::grpc_plugin;

GRPCPlugin plugin;
plugin.initialize(nullptr);
auto server = plugin.createServer();

RPCServerConfig cfg;
cfg.host = "0.0.0.0";
cfg.port = plugin.getDefaultPort();
cfg.tls_enabled = false; // nur lokal/dev

server->initialize(cfg);
if (server->start()) {
    server->stop();
}
```

## Installation

Das Modul wird mit ThemisDB gebaut. In-tree-Targets brauchen `include/` und `src/` auf dem Include-Pfad:

```cmake
target_include_directories(your_target PRIVATE
    ${THEMISDB_INCLUDE_DIR}
    ${THEMISDB_SOURCE_DIR}/src
)
```

## Troubleshooting

- **Start schlägt mit TLS fehl:** Zertifikat-/Key-/CA-Dateien prüfen (existieren, lesbar, korrektes PEM-Format).
- **Admin-Port ist nicht aktiv:** `extra_config["admin_port"]` mit gültigem numerischem Port (1..65535) setzen.
- **Keine Prometheus-Ausgabe:** erst `recordRPC()`-Ereignisse erzeugen, dann `getMetricsText()` abfragen.

---

## Primäre Dokumentation

| Dokument | Beschreibung |
|---|---|
| [`src/rpc_grpc/README.md`](../../../src/rpc_grpc/README.md) | Modulübersicht, Laufzeitverhalten, Beispiele |
| [`include/rpc_grpc/README.md`](../../../include/rpc_grpc/README.md) | Public API, Header-Entry-Points, Konfiguration |
| [`src/rpc_grpc/ROADMAP.md`](../../../src/rpc_grpc/ROADMAP.md) | Phasen, Status, offene Punkte |
| [`src/rpc_grpc/FUTURE_ENHANCEMENTS.md`](../../../src/rpc_grpc/FUTURE_ENHANCEMENTS.md) | Konkrete Folge-Erweiterungen |
| [`src/rpc_grpc/ARCHITECTURE.md`](../../../src/rpc_grpc/ARCHITECTURE.md) | Architektur und Komponentenfluss |
| [`src/rpc_grpc/SECURITY.md`](../../../src/rpc_grpc/SECURITY.md) | Sicherheitsmodell und Risiken |
| [`../../../plugins/rpc/README.md`](../../../plugins/rpc/README.md) | Modulkontext im RPC-Backend-Verzeichnis |
