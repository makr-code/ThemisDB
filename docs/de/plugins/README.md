# Plugins Modul
<!-- Status: current | validated: 2026-04-06 -->
<!-- Primärdokumentation: ../../../src/plugins/ -->

**Stand:** 6. April 2026  
**Version:** 1.1  
**Kategorie:** 🔌 Plugin-System  
**Validated:** 2026-03-09 (Reality-Check gegen Sourcecode; siehe [MISSING_IMPLEMENTATIONS.md](MISSING_IMPLEMENTATIONS.md))

---

## Übersicht

Das Plugins-Modul stellt die **Erweiterbarkeitsinfrastruktur** für ThemisDB bereit. Es umfasst:

- **Dynamisches Laden** — `.so`/`.dll` über `dlopen`/`LoadLibrary` mit RAII-Lifecycle-Guards
- **Manifest-Validierung** — JSON-Schema-v2-Prüfung bei jeder Plugin-Registrierung
- **Ed25519-Signierung** — kryptografische Signaturprüfung vor jedem `dlopen`
- **Capability-basierte Berechtigungen** — Plugins deklarieren benötigte Fähigkeiten; der Manager gewährt nur deklarierte Capabilities
- **Plugin-Dependency-Resolution** — Topologische Ladereihenfolge + Zykluserkennung (Kahn-Algorithmus)
- **Hot-Reload ohne Serverneustart** — `PluginManager::reloadPlugin()` — TOCTOU-sicheres 3-Phasen-Reload mit State-Preserve
- **Health-Monitoring & Auto-Restart** — `PluginHealthMonitor` mit konfigurierbaren Liveness-Probes
- **Per-Plugin-Metriken** — Call-Count, Latenzen, Error-Rate via `PluginMetrics`
- **OCI-Registry-Integration** — Remote-Plugin-Laden von OCI-kompatiblen Registries
- **Signed Plugin Repository** — Pinned-Key-Store für vertrauenswürdige Publisher-Schlüssel
- **RPC-Service-Registry** — Plugins können gRPC-/RPC-Endpunkte registrieren

---

## Source-Code Referenz

### Implementierung (`src/plugins/`)

| Datei / Komponente | Rolle |
|---|---|
| `plugin_manager.cpp` | Kern-Lifecycle: load, unload, **hot-reload**, capability negotiation, autoLoad |
| `plugin_registry.cpp` | Registrierung, Manifest-Validierung, Ed25519-Signaturprüfung |
| `plugin_system_edition.cpp` | Edition-Gates (CORE / Professional / Enterprise) |
| `plugin_metrics.cpp` | Per-Plugin-Metriken: Call-Count, Latenz, Error-Rate |
| `plugin_health_monitor.cpp` | Liveness-Probes, automatischer Neustart bei aufeinanderfolgenden Fehlern |
| `plugin_hot_plug_monitor.cpp` | Verzeichnis-Watcher: erkennt neue/aktualisierte Plugins und löst Reload aus |
| `signed_plugin_repository.cpp` | Signed-Plugin-Repository: Pinned-Key-Store, Manifest-Fetch |
| `oci_registry_client.cpp` | OCI-Registry-Client: remote Plugin-Laden (libcurl, Distribution Spec) |
| `rpc_service_registry.cpp` | Registry für Plugins mit gRPC-/RPC-Endpunkten |
| `huggingface_ingestion_plugin.cpp` | First-Party-Plugin: HuggingFace-Modell-Ingestion |

### Öffentliche Header (`include/plugins/`)

| Header | Rolle |
|---|---|
| `plugin_interface.h` | `IThemisPlugin`, `IStatefulPlugin`, `ISelfHealingPlugin`, `PluginCapabilityNegotiator` |
| `plugin_manager.h` | `PluginManager` — Haupt-Einstiegspunkt für Host-Code |
| `plugin_registry.h` | `PluginRegistry` — Registrierung, Validierung, Verifikation |
| `plugin_api.h` | Öffentliche Plugin-API-Typen und Versionskonstanten |
| `plugin_dependency_resolver.h` | `PluginDependencyResolver` — header-only topologische Sortierung / Zykluserkennung |
| `plugin_health_monitor.h` | `PluginHealthMonitor` — Liveness-Probe-API |
| `plugin_hot_plug_monitor.h` | `PluginHotPlugMonitor` — Filesystem-Watch-API |
| `plugin_metrics.h` | `PluginMetrics` — per-Plugin-Telemetrie |
| `signed_plugin_repository.h` | `SignedPluginRepository` — Key-Store und Eintrags-Management |
| `oci_registry_client.h` | `OciRegistryClient` — Remote-Registry-Fetch |
| `rpc_plugin_interface.h` | `RpcPlugin`, `RpcServiceRegistry` — RPC-Plugin-Basistypen |
| `self_healing_plugin.h` | `ISelfHealingPlugin` — Heartbeat und Auto-Restart-Vertrag |
| `manifest_schema_v2.json` | JSON-Schema v2 für capability-aware Manifeste |
| `image_analysis_interface.h` | `IImageAnalysisPlugin` — Bildanalyse-Plugin-Basis |
| `image_analysis_manager.h` | `ImageAnalysisManager` — Multi-Backend-Plugin-Manager |
| `huggingface_ingestion_plugin.h` | `HuggingFaceIngestionPlugin` — HuggingFace First-Party-Plugin |

---

## Komponentenarchitektur

```
PluginManager  (Kern-Lifecycle + Hot-Reload + Capability-Negotiation)
    │
    ├─ PluginRegistry          ──► JSON-Schema-v2-Validierung
    │      └─ Ed25519-Signaturprüfung (vor jedem dlopen)
    │
    ├─ PluginDependencyResolver ──► Topologische Ladereihenfolge (Kahn)
    │                               Zykluserkennung (reject on cycle)
    │
    ├─ PluginHealthMonitor     ──► Liveness-Probes, Auto-Restart
    ├─ PluginHotPlugMonitor    ──► inotify/FSEvents Verzeichnis-Watch
    │      └─ löst PluginManager::reloadPlugin() aus
    │
    ├─ PluginMetrics           ──► Per-Plugin-Telemetrie
    │
    ├─ SignedPluginRepository  ──► Pinned-Key-Store
    ├─ OciRegistryClient       ──► Remote-Plugin-Fetch (OCI Distribution Spec)
    └─ RpcServiceRegistry      ──► gRPC-Endpunkt-Registry
```

---

## Schnellstart

```cpp
#include "plugins/plugin_manager.h"

using namespace themis::plugins;

// Server-Start: Plugin laden
PluginManager& pm = PluginManager::instance();
auto result = pm.loadPlugin("/plugins/my_plugin.so");
if (!result) {
    THEMIS_ERROR("Plugin load failed: {}", result.error().message());
}

// Hot-Reload ohne Neustart
auto reload = pm.reloadPlugin("my_plugin");  // TOCTOU-sicher, 3-Phasen

// Auto-Load aller registrierten Plugins (mit dep-Sortierung)
auto count = pm.autoLoadPlugins();

// Capability-Negotiation
PluginCapabilityRequirements reqs;
reqs.add({"storage:read", "1.0"});
auto negotiation = pm.negotiateCapabilities("my_plugin", reqs);
```

---

## Feature-Status

| Feature | Status | Evidence |
|---|---|---|
| Dynamisches Laden (.so/.dll) | ✅ Produktionsreif | `plugin_manager.cpp` |
| Manifest-Validierung (JSON Schema v2) | ✅ Produktionsreif | `plugin_registry.cpp`, `manifest_schema_v2.json` |
| Ed25519-Signierung & Verifikation | ✅ Produktionsreif | `plugin_registry.cpp`, `signed_plugin_repository.cpp` |
| Capability-Negotiation | ✅ Produktionsreif | `plugin_manager.cpp:1288`, `plugin_interface.h` |
| Plugin-Dependency-Resolution | ✅ Produktionsreif | `plugin_dependency_resolver.h` (header-only Kahn) |
| Hot-Reload ohne Neustart | ✅ Produktionsreif | `plugin_manager.cpp:953` |
| Health-Monitoring & Auto-Restart | ✅ Produktionsreif | `plugin_health_monitor.cpp` |
| Per-Plugin-Metriken | ✅ Produktionsreif | `plugin_metrics.cpp` |
| OCI-Registry-Integration | ✅ Produktionsreif | `oci_registry_client.cpp` |
| Signed Plugin Repository | ✅ Produktionsreif | `signed_plugin_repository.cpp` |
| HuggingFace Ingestion Plugin | ✅ Produktionsreif | `huggingface_ingestion_plugin.cpp` |
| WASM-Sandbox-Isolation | 🔜 Geplant | `include/plugins/FUTURE_ENHANCEMENTS.md` |
| Manifest-Signing mit Key-Rotation | 🚧 In Bearbeitung | `ROADMAP.md` Phase 2 |
| Community-Plugin-Marketplace | 🔜 Geplant | `ROADMAP.md` Phase 3 |

---

## Tests

| Testdatei | Abgedeckte Komponente |
|---|---|
| `test_plugin_manager.cpp` | Core lifecycle: load, initialize, unload |
| `test_plugin_manager_comprehensive.cpp` | Edge Cases, Fehlerbehandlung, autoLoad |
| `test_plugin_dependency_resolver.cpp` | Dependency Resolution, Kahn-Sort, Zykluserkennung |
| `test_plugin_hot_reload_enhanced.cpp` | Hot-Reload (3-Phasen, State-Preserve) |
| `test_plugin_hot_plug.cpp` | Hot-Plug-Monitor: Verzeichnis-Watch |
| `test_plugin_security_audit.cpp` | Ed25519-Signaturprüfung |
| `test_plugin_security_implementation.cpp` | Sicherheits-Implementation |
| `test_plugin_metrics_integration.cpp` | Per-Plugin-Metriken |
| `test_generic_plugin_registry.cpp` | Registry-Operationen |
| `test_plugin_marketplace_manifest.cpp` | Manifest-Schema-Validierung |
| `test_huggingface_plugin.cpp` | HuggingFace-Ingestion-Plugin |
| `test_llm_plugin.cpp` | LLM-Plugin-Integration |
| `test_wasm_plugin_sandbox.cpp` | WASM-Sandbox (geplant, Test-Scaffold) |
| `test_ethics_plugin_integration.cpp` | Ethics-AI-Plugin |
| `test_importer_plugin_api.cpp` | Importer-Plugin-API |

---

## Verwandte Dokumentation

### Primärdokumentation (Quellcode)

- [README (src/plugins)](../../../src/plugins/README.md) — Modulübersicht und Entwicklerleitfaden
- [ARCHITECTURE (src/plugins)](../../../src/plugins/ARCHITECTURE.md) — Architektur-Leitfaden
- [ROADMAP (src/plugins)](../../../src/plugins/ROADMAP.md) — Entwicklungs-Roadmap, verifiziert gegen Sourcecode
- [FUTURE_ENHANCEMENTS (src/plugins)](../../../src/plugins/FUTURE_ENHANCEMENTS.md) — Geplante Features mit IEEE-Referenzen
- [FUTURE_ENHANCEMENTS (include/plugins)](../../../include/plugins/FUTURE_ENHANCEMENTS.md) — Header-API-Enhancements

### Detaildokumentation in diesem Verzeichnis

| Datei | Beschreibung |
|---|---|
| [MANIFEST_SIGNATURES.md](MANIFEST_SIGNATURES.md) | Plugin-Manifest-Signing und Ed25519-Workflow |
| [PLUGIN_MIGRATION.md](PLUGIN_MIGRATION.md) | Plugin-Migrations-Guide |
| [RPC_PLUGIN_ARCHITECTURE.md](RPC_PLUGIN_ARCHITECTURE.md) | RPC-Framework & Plugin-Integration |
| [HOT_RELOAD_GUIDE.md](HOT_RELOAD_GUIDE.md) | Hot-Reload ohne Serverneustart |
| [DEPENDENCY_RESOLVER_USAGE.md](DEPENDENCY_RESOLVER_USAGE.md) | Dependency-Resolution-Usage-Guide |
| [SIGNED_PLUGIN_REPOSITORY.md](SIGNED_PLUGIN_REPOSITORY.md) | Signed Plugin Repository |
| [MISSING_IMPLEMENTATIONS.md](MISSING_IMPLEMENTATIONS.md) | Reality-Check: offene Implementierungen mit Evidence |
| [missing-implementations.json](missing-implementations.json) | Maschinenlesbares Format |

### Verwandte Module

- [Security-Modul](../security/README.md) — Signature-Verification
- [Enterprise Features](../enterprise/README.md) — Enterprise Plugin System
- [LLM-Modul](../llm/README.md) — LLM Plugin Architecture

