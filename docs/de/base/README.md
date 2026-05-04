# Base-Modul – Überblick

**Stand:** 6. April 2026  
**Version:** v1.1.0  
**Kategorie:** 🔌 Plugin-Infrastruktur  
**Status:** 🟢 Production-Ready

---

## 📑 Inhaltsverzeichnis

- [Überblick](#überblick)
- [Komponenten](#komponenten)
- [Schnellstart](#schnellstart)
- [Sicherheitskonzept](#sicherheitskonzept)
- [Konfiguration](#konfiguration)
- [Bekannte Einschränkungen](#bekannte-einschränkungen)
- [Weiterführende Dokumentation](#weiterführende-dokumentation)

---

## Überblick

Das **Base-Modul** stellt die grundlegende Plugin- und Modul-Ladeinfrastruktur für ThemisDB bereit. Es ermöglicht das sichere Laden dynamischer Bibliotheken (`.so`, `.dll`, `.dylib`) zur Laufzeit, verifiziert deren Authentizität per digitaler Signatur und SHA-256-Hash und verwaltet den vollständigen Plugin-Lebenszyklus (Init → Execute → Shutdown).

Alle anderen Plugin-fähigen Module (`src/acceleration/`, `src/plugins/`, `src/storage/`) bauen auf diesem Modul auf.

### Kernfunktionen

| Funktion | Status | Beschreibung |
|----------|--------|--------------|
| Sicheres Laden von DLL/SO/DYLIB | ✅ Production | Windows, Linux, macOS |
| Digitale Signaturprüfung | ✅ Production | SHA-256 + Zertifikatsvalidierung |
| Vertrauensstufen | ✅ Production | TRUSTED / VERIFIED / UNTRUSTED |
| Plugin-Lebenszyklus | ✅ Production | Initialize, Execute, Shutdown |
| Hot-Reload | ✅ Production | Zero-Downtime-Reload mit Rollback |
| OS-Level-Sandboxing | ✅ Production | CPU- und Speicher-Limits pro Plugin |
| WASM-Plugin-Isolation | ✅ Production | Speichersichere Isolation für unsichere Plugins |
| Remote-Registry-Client | ✅ Production | Authentifizierter Download aus Marketplace |
| Plugin-Dependency-Graph | ✅ Production | Abhängigkeitsdeklaration und topologische Sortierung |
| A/B-Test-Manager | ✅ Production | Traffic-Split via Modul-Swapping |
| Per-Plugin-Audit-Trail | ✅ Production | Ladeprotokoll mit Timestamp und Ergebnis |
| Dependency-Ordered Loading | 🚧 In Progress | Abhängigkeitsgeordnetes Laden (Issue #1566, Target: Q2 2026) |

---

## Komponenten

### ModuleLoader (`module_loader.h/.cpp`)
Sicheres plattformübergreifendes Laden von Shared Libraries. Bietet:
- Auflösung und Validierung des Pfads
- SHA-256-Integritätsprüfung + Signaturverifizierung (Produktionsmodus)
- Zertifikats-Revokationsprüfung (OCSP/CRL)
- Zuweisung von Vertrauensstufen (TRUSTED / VERIFIED / UNTRUSTED)
- Registry geladener Module (verhindert doppeltes Laden)
- Per-Plugin-Audit-Trail (Laden, Entladen, Fehler)

### HotReloadManager (`hot_reload_manager.h/.cpp`)
Zero-Downtime-Modul-Austausch zur Laufzeit:
- Atomares Ersetzen des alten Moduls durch die neue Version
- Laufende Queries mit dem alten Plugin-Handle werden vollständig abgeschlossen
- Automatisches Rollback bei Fehler (≤ 500 ms)

### ModuleSandbox (`module_sandbox.h/.cpp`)
OS-seitige Ressourcenbegrenzung pro Plugin:
- CPU- und Speicher-Limits (Standard: max. 256 MB RAM)
- ABI-Kompatibilitätsprüfung vor dem Laden

### WasmPluginSandbox (`wasm_plugin_sandbox.h/.cpp`)
WASM-basierte Isolation für unsichere Drittanbieter-Plugins:
- Speichersichere Ausführungsumgebung
- Anforderung: Injektion eines konkreten WASM-Runtimes (z. B. Wasmtime, WasmEdge)

### RemoteRegistryClient (`remote_registry_client.h/.cpp`)
Authentifizierter Client für den ThemisDB-Plugin-Marketplace:
- TLS-gesicherter Download
- Signaturverifizierung vor der Installation
- SHA-256-Checksum im Audit-Log

### PluginDependencyGraph (`plugin_dependency_graph.h/.cpp`)
Abhängigkeitsverwaltung zwischen Plugins:
- Deklaration von Plugin-Abhängigkeiten
- Topologische Sortierung der Ladereihenfolge
- Erkennung zyklischer Abhängigkeiten

### ABTestManager (`ab_test_manager.h/.cpp`)
Traffic-Split für kontrollierte Plugin-Experimente:
- Prozentuales Traffic-Routing zwischen zwei Modul-Versionen
- Metriken pro Variante

---

## Schnellstart

### Plugin laden

```cpp
#include "themis/base/module_loader.h"
using namespace themis::modules;

ModuleLoader loader;

// Sicherheitsrichtlinie konfigurieren
ModuleSecurityPolicy policy;
policy.requireSignature = true;   // Pflicht in Produktion
loader.setSecurityPolicy(policy);

// Plugin laden
auto result = loader.loadModule("storage_plugin.so");
if (result.is_ok()) {
    auto module = result.value();
    auto iface = module->getInterface<IStoragePlugin>();
    iface->initialize();
    // Plugin verwenden ...
    loader.unloadModule(module);
} else {
    spdlog::error("Plugin-Ladefehler: {}", result.error());
}
```

### Hot-Reload durchführen

```cpp
#include "themis/base/hot_reload_manager.h"
using namespace themis::modules;

HotReloadManager mgr(loader);
auto res = mgr.reloadModule("my_plugin", "/path/to/new_plugin.so");
if (!res.is_ok()) {
    // Rollback wird automatisch ausgelöst
    spdlog::error("Hot-Reload fehlgeschlagen: {}", res.error());
}
```

### Export-Makros (für Plugin-Entwickler)

```cpp
#include "themis/base/export.h"

// Exportierte Funktion
THEMIS_BASE_API int myPluginEntry(int arg);

// Exportierte Klasse
class THEMIS_BASE_API MyPlugin {
public:
    void initialize();
};
```

---

## Sicherheitskonzept

| Modus | Signatur | Vertrauensstufe | Revokation |
|-------|----------|-----------------|------------|
| **Produktion** | Pflicht (Ed25519/SHA-256) | Minimum TRUSTED | OCSP/CRL |
| **Entwicklung** | Optional | UNTRUSTED erlaubt | Übersprungen |

> **Wichtig:** In der Produktionskonfiguration wird ein Plugin vor dem `dlopen`-Aufruf verifiziert. Bei ungültiger oder fehlender Signatur wird das Laden abgebrochen und ein Security-Alert geloggt.

---

## Konfiguration

| Parameter | Standard (Prod) | Beschreibung |
|-----------|-----------------|--------------|
| `base.security.require_signature` | `true` | Unsignierte Module ablehnen |
| `base.security.allow_unsigned` | `false` | Unsignierte Module in Dev-Modus erlauben |
| `base.security.min_trust_level` | `TRUSTED` | Mindest-Vertrauensstufe |
| `base.hot_reload.enabled` | `false` | Hot-Reload aktivieren |
| `base.sandbox.memory_limit_mb` | `256` | Maximaler RAM pro Plugin in MB |

---

## Bekannte Einschränkungen

- **Dependency-Ordered Loading** (Issue #1566, Target Q2 2026): Die Ladereihenfolge für Plugins mit deklarierten Abhängigkeiten muss aktuell noch manuell gesteuert werden.
- **WASM-Runtime**: `WasmPluginSandbox` benötigt die Injektion eines konkreten WASM-Runtimes (Wasmtime, WasmEdge o. ä.) für die vollständige Ausführungsunterstützung.
- **Auto-Restart**: Zustandsprüfungen (Health-Checks) beim Laden sind implementiert; automatischer Neustart bei fehlgeschlagener Zustandsprüfung ist noch nicht vorhanden (Issue #2373).
- **Key Pinning**: Der Remote-Registry-Client prüft TLS-Zertifikate (CA-basiert); Public-Key-Pinning ist noch nicht implementiert.
- **Test-Coverage**: Unit-Tests (> 80 %), Integrationstests und Performance-Benchmarks sind noch offen (Issues #1573, #1574, #1575, Target Q2 2026).

Detaillierter Report: [MISSING_IMPLEMENTATIONS.md](./MISSING_IMPLEMENTATIONS.md)

---

## Weiterführende Dokumentation

### Primary-Quellen (Entwickler-Dokumentation)

| Dokument | Inhalt |
|----------|--------|
| [src/base/README.md](../../../src/base/README.md) | Vollständige Modul-Übersicht, Konfiguration, Beispiele, Best Practices |
| [src/base/ARCHITECTURE.md](../../../src/base/ARCHITECTURE.md) | Architekturdetails, Datenfluss, Threading-Modell, Sicherheitsarchitektur |
| [src/base/ROADMAP.md](../../../src/base/ROADMAP.md) | Feature-Status mit Code-Evidence, offene Issues, Implementierungsphasen |
| [src/base/FUTURE_ENHANCEMENTS.md](../../../src/base/FUTURE_ENHANCEMENTS.md) | Geplante Features, Design-Constraints, Test-Strategie, Performance-Ziele, IEEE-Referenzen |
| [include/themis/base/README.md](../../../include/themis/base/README.md) | Öffentliche Header-Übersicht, DI-Interface-Dokumentation |

### Fehlende Implementierungen

| Dokument | Inhalt |
|----------|--------|
| [MISSING_IMPLEMENTATIONS.md](./MISSING_IMPLEMENTATIONS.md) | Detaillierter Report aller Diskrepanzen ROADMAP vs. Code (Markdown) |
| [missing-implementations.json](./missing-implementations.json) | Maschinenlesbare Fassung desselben Reports (JSON) |

### Verwandte Module

- [Plugin-Dokumentation](../plugins/README.md) — Übergeordnetes Plugin-System
- [Security-Modul](../security/README.md) — Signaturverifizierung
- [Architektur-Übersicht](../architecture/ARCHITECTURE_OVERVIEW.md) — Systemarchitektur
