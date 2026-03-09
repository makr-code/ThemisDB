# Plugins Module

**Stand:** 9. März 2026
**Version:** v1.5.0
**Kategorie:** 🔌 Plugins
**Validated:** 2026-03-09 (09f7c55)
**Status:** current

---

## 📑 Inhaltsverzeichnis

- [Übersicht](#übersicht)
- [Source-Code Referenz](#source-code-referenz)
- [Features](#features)
- [Dokumentation in diesem Ordner](#dokumentation-in-diesem-ordner)
- [Verwandte Dokumentation](#verwandte-dokumentation)

---

## Übersicht

Das Plugins-Modul implementiert die vollständige Plugin-Infrastruktur von ThemisDB: dynamisches Laden von Shared Libraries, Ed25519-Signaturverifizierung, Manifest-Validierung (JSON-Schema v2), Fähigkeitsverhandlung, Dependency-Resolution, Hot-Plug-Monitoring, Health-Monitoring mit Auto-Restart, Metriken und OCI-Registry-Integration.

**Primäre Dokumentation:** [`src/plugins/README.md`](../../../src/plugins/README.md)
**Architektur:** [`src/plugins/ARCHITECTURE.md`](../../../src/plugins/ARCHITECTURE.md)
**Roadmap:** [`src/plugins/ROADMAP.md`](../../../src/plugins/ROADMAP.md)
**Geplante Erweiterungen:** [`src/plugins/FUTURE_ENHANCEMENTS.md`](../../../src/plugins/FUTURE_ENHANCEMENTS.md)

---

## Source-Code Referenz

### Headers (`include/plugins/`)

| Komponente | Header | Source | Beschreibung |
|------------|--------|--------|--------------|
| IThemisPlugin | `plugin_interface.h` | *(in plugin_manager.cpp)* | Core-Plugin-Basisklasse, `PluginManifest`, `MarketplaceManifest`, `PluginCapabilityNegotiator`, `PluginVersionRange`, `ManifestSchemaValidator` |
| PluginManager | `plugin_manager.h` | `plugin_manager.cpp` | Laden/Entladen/Reload; `verifyPlugin()` (Ed25519, NDEBUG); `reloadPlugin()` (TOCTOU-safe); `negotiateCapabilities()` |
| PluginAPI | `plugin_api.h` | *(header-only)* | Top-Level-API-Hilfsfunktionen |
| PluginRegistry | `plugin_registry.h` | `plugin_registry.cpp` | Plugin-Registrierungs-Katalog |
| PluginMetrics | `plugin_metrics.h` | `plugin_metrics.cpp` | Aufruf-Latenz und Fehlerrate pro Plugin |
| PluginHealthMonitor | `plugin_health_monitor.h` | `plugin_health_monitor.cpp` | Absturzerkennung und automatischer Neustart |
| PluginHotPlugMonitor | `plugin_hot_plug_monitor.h` | `plugin_hot_plug_monitor.cpp` | Dateisystem-Watcher für Hot-Plug-Events |
| PluginDependencyResolver | `plugin_dependency_resolver.h` | *(header-only)* | DAG-basierte Dependency-Resolution, Zykluserkennung |
| SignedPluginRepository | `signed_plugin_repository.h` | `signed_plugin_repository.cpp` | Ed25519-signierter Plugin-Katalog mit Key-Pinning |
| OciRegistryClient | `oci_registry_client.h` | `oci_registry_client.cpp` | OCI-Registry-Pull-Client (Remote-Plugin-Loading) |
| RpcPluginInterface | `rpc_plugin_interface.h` | `rpc_service_registry.cpp` | RPC-Plugin-Interface und Service-Registry |
| ImageAnalysisInterface | `image_analysis_interface.h` | *(header-only)* | Multi-Backend-Bildanalyse-Plugin-Interface |
| ImageAnalysisManager | `image_analysis_manager.h` | *(header-only)* | Bildanalyse-Plugin-Manager |
| SelfHealingPlugin | `self_healing_plugin.h` | *(header-only)* | Self-Healing-Plugin-Helfer |
| HuggingFaceIngestionPlugin | `huggingface_ingestion_plugin.h` | `huggingface_ingestion_plugin.cpp` | HuggingFace-Dataset-Ingestion-Plugin |
| ManifestSchema | `manifest_schema_v2.json` | — | JSON-Schema für Marketplace-Manifest-Validierung |
| PluginSystemEdition | — | `plugin_system_edition.cpp` | Edition-gesteuertes Plugin-System-Initialisierung |

**Gesamt:** 15 Header + 1 JSON-Schema in `include/plugins/`, 10 Source-Dateien in `src/plugins/`

---

## Features

| Feature | Status | Beschreibung | Implementierung |
|---------|--------|--------------|-----------------|
| Dynamisches Laden | ✅ Production | `.so`/`.dll` via `dlopen`/`LoadLibrary` | `plugin_manager.cpp` |
| Ed25519 Signaturverifizierung | ✅ Production | Key-Pinning, NDEBUG-Enforcement | `signed_plugin_repository.cpp` |
| Manifest-Validierung | ✅ Production | JSON-Schema v2 (`ManifestSchemaValidator`) | `plugin_interface.h` |
| Fähigkeitsverhandlung | ✅ Production | `PluginCapabilityNegotiator`, Versionsbereiche | `plugin_interface.h`, `plugin_manager.cpp` |
| Dependency-Resolution | ✅ Production | DAG, Zykluserkennung | `plugin_dependency_resolver.h`, `plugin_manager.cpp` |
| Hot-Plug-Monitoring | ✅ Production | Dateisystem-Watcher | `plugin_hot_plug_monitor.cpp` |
| Health-Monitoring | ✅ Production | Auto-Restart bei Absturz | `plugin_health_monitor.cpp` |
| Plugin-Metriken | ✅ Production | Latenz, Fehlerrate | `plugin_metrics.cpp` |
| OCI-Registry-Integration | ✅ Production | Remote-Plugin-Pull | `oci_registry_client.cpp` |
| Manifest Signatures | ✅ Production | Plugin-Signatur-Verifikation | `signed_plugin_repository.cpp` |
| RPC-Framework | ✅ Production | RPC-Plugin-Architektur | `rpc_service_registry.cpp` |
| Image-Analysis-Plugins | ✅ Production | Multi-Backend-KI für Bildanalyse | `image_analysis_interface.h` |
| WASM-Sandbox | 🔲 Geplant | Isolierte Ausführung via Wasmtime | `FUTURE_ENHANCEMENTS.md` |

---

## Dokumentation in diesem Ordner

| Datei | Beschreibung |
|-------|--------------|
| [MANIFEST_SIGNATURES.md](MANIFEST_SIGNATURES.md) | Plugin Manifest Signing |
| [PLUGIN_MIGRATION.md](PLUGIN_MIGRATION.md) | Plugin Migration Guide |
| [RPC_PLUGIN_ARCHITECTURE.md](RPC_PLUGIN_ARCHITECTURE.md) | RPC Framework & Plugin-Integration |
| [missing-implementations.md](missing-implementations.md) | Bekannte Lücken und Reality-Check-Ergebnisse |

---

## Verwandte Dokumentation

- [Primary Source Docs](../../../src/plugins/README.md) — vollständige API-Referenz
- [Architecture](../../../src/plugins/ARCHITECTURE.md) — Komponenten-Diagramm, Datenfluss
- [ROADMAP](../../../src/plugins/ROADMAP.md) — Implementierungsstand und geplante Features
- [Enterprise Features](../enterprise/README.md) — Enterprise Plugin System
- [Security Module](../security/README.md) — Signaturverifizierung
