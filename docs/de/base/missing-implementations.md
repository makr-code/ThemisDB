# Base-Modul — Fehlende / Unvollständige Implementierungen

**Geprüft:** 2026-03-09  
**Geprüft von:** Source-Code-Audit (commit 0091524)  
**Modul:** `src/base/` + `include/themis/base/`

---

## Übersicht

Bei der Reality-Check-Prüfung (ROADMAP vs. Sourcecode) wurden folgende Diskrepanzen zwischen dokumentierten Ansprüchen und tatsächlicher Implementierung gefunden:

| # | Feature | ROADMAP-Status (alt) | Beobachteter Stand | Schwere |
|---|---------|---------------------|-------------------|---------|
| 1 | Plugin health monitoring & automatic restart | `[x]` erledigt | Nur Health-Checks beim Laden; kein Auto-Restart | Medium |
| 2 | WASM-based plugin isolation | `[x]` erledigt | Infrastruktur vollständig; Runtime-Injection fehlt | Medium |
| 3 | Signed plugin repository with key pinning | `[x]` erledigt | TLS-Zertifikat-Verifikation vorhanden; Public-Key-Pinning fehlt | Low |
| 4 | Runtime plugin capability negotiation (version ranges) | `[x]` erledigt | Version-Ranges in Dependency-Graph gespeichert; kein aktives Negotiation-Protokoll | Low |

---

## Eintrag 1: Plugin Health Monitoring & Automatic Restart

**Claim-Quelle:** `src/base/ROADMAP.md`, Abschnitt "Completed ✅"  
> `[x] Plugin health monitoring and automatic restart (Issue: #2373)`

**Erwartet:** Plugins werden nach einem Health-Check-Fehler automatisch neugestartet (Watchdog-Mechanismus).

**Beobachtet:**
- Health-Check-Infrastruktur ist in `src/base/module_loader.cpp` implementiert: Activation Stage (Zeilen 452–476), `runHealthChecks()` (Zeilen 1148–1190), `healthChecks_` Map.
- Health-Checks werden beim Modullade-Vorgang ausgeführt (Activation Stage, `module_loader.cpp` Zeilen 452–476 und `runHealthChecks()` Zeilen 1148–1190).
- **Kein Restart-Mechanismus** gefunden: weder in `module_loader.cpp`, `hot_reload_manager.cpp` noch in `module_sandbox.cpp`. Keine `restart_count`-, `watchdog`- oder `auto_recover`-Symbole in einem der Base-Quellcode-Dateien.

**Evidence (geprüfte Pfade):**
- `src/base/module_loader.cpp` — `healthChecks_` Map; Activation Stage (Zeilen 452–476); `runHealthChecks()` (Zeilen 1148–1190)
- `include/themis/base/module_loader.h` — `registerHealthCheck()` API
- `src/base/hot_reload_manager.cpp` — kein `restart`/`watchdog` Symbol
- `include/themis/base/module_sandbox.h` — kein `restart`/`watchdog` Symbol

**Issue-Titelvorschlag:** `feat(base): implement automatic plugin restart on health-check failure`  
**Label-Vorschläge:** `enhancement`, `module:base`, `priority:medium`

---

## Eintrag 2: WASM-Based Plugin Isolation (Runtime-Injection erforderlich)

**Claim-Quelle:** `src/base/ROADMAP.md`, Abschnitt "Completed ✅"  
> `[x] WASM-based plugin isolation for untrusted code (Issue: #1572)`

**Erwartet:** Vollständig funktionale WASM-Isolation für unsichere Plugins.

**Beobachtet:**
- `WasmPluginSandbox` in `src/base/wasm_plugin_sandbox.cpp` ist vollständig implementiert (Sandbox-Infrastruktur, Memory-Mapping, Capability-Limits).
- `setRuntime(std::unique_ptr<WasmRuntime>)` in `wasm_plugin_sandbox.cpp` (Zeile 288) muss vor dem ersten `callExport()` aufgerufen werden.
- Ohne Runtime-Injection schlägt `callExport()` mit dem Fehler `"inject a WasmRuntime before calling callExport()"` fehl (Zeile 457).
- Keine konkrete `WasmRuntime`-Implementierung (Wasmtime, WasmEdge) im Repository gefunden.

**Evidence (geprüfte Pfade):**
- `src/base/wasm_plugin_sandbox.cpp` — `WasmPluginSandbox::setRuntime()` (Zeile 288), Fehlermeldung (Zeile 457)
- `include/themis/base/wasm_plugin_sandbox.h` — `WasmRuntime` Interface-Anforderung
- Kein `wasmtime_*`- oder `wasmedge_*`-Symbol im Repository

**Issue-Titelvorschlag:** `feat(base): integrate concrete WasmRuntime (Wasmtime or WasmEdge) into WasmPluginSandbox`  
**Label-Vorschläge:** `enhancement`, `module:base`, `priority:medium`

---

## Eintrag 3: Signed Plugin Repository with Key Pinning

**Claim-Quelle:** `src/base/ROADMAP.md`, Phase 3  
> `[x] Signed plugin repository with key pinning`

**Erwartet:** Öffentlicher Schlüssel des Plugin-Repositories ist im Server gepinnt; Downloads werden gegen diesen festen Schlüssel verifiziert.

**Beobachtet:**
- `RemoteRegistryClient` in `include/themis/base/remote_registry_client.h` bietet `RegistryConfig::verify_ssl = true` für TLS-Zertifikatsvalidierung (Zeile 70–71).
- **Kein Public-Key-Pinning**: Kein `pinned_pubkey`-, `cert_pin`- oder `pubkey_fingerprint`-Feld in `remote_registry_client.h` oder `remote_registry_client.cpp`.
- TLS-Zertifikat-Verifikation (CA-basiert) ist implementiert; echtes Key-Pinning (unabhängig von CA-Vertrauen) fehlt.

**Evidence (geprüfte Pfade):**
- `include/themis/base/remote_registry_client.h` — `RegistryConfig::verify_ssl` (Zeile 71), kein `pin_*` Feld
- `src/base/remote_registry_client.cpp` — kein `pin`-Symbol

**Issue-Titelvorschlag:** `feat(base): implement public-key pinning for remote plugin registry`  
**Label-Vorschläge:** `enhancement`, `module:base`, `security`, `priority:low`

---

## Eintrag 4: Runtime Plugin Capability Negotiation (Version Ranges)

**Claim-Quelle:** `src/base/ROADMAP.md`, Phase 3  
> `[x] Runtime plugin capability negotiation (version ranges)`

**Erwartet:** Plugins verhandeln aktiv ihre Version-Kompatibilität zur Laufzeit.

**Beobachtet:**
- `PluginDependencyGraph` in `include/themis/base/plugin_dependency_graph.h` speichert `minVersion`/`maxVersion` pro Abhängigkeits-Kante (Zeilen 109–110).
- **Kein aktives Negotiation-Protokoll**: Keine `negotiate()`-, `checkCompatibility()`- oder ähnliche Methode in `plugin_dependency_graph.h/.cpp` oder `module_loader.h/.cpp`.
- Version-Ranges werden deklariert und in DOT-Visualisierung angezeigt, aber nicht zur Laufzeit durchgesetzt.
- Tatsächliche Durchsetzung ist Teil von Issue #1566 (Plugin dependency resolution).

**Evidence (geprüfte Pfade):**
- `include/themis/base/plugin_dependency_graph.h` — `Edge::minVersion`/`maxVersion` (Zeilen 109–110)
- `src/base/plugin_dependency_graph.cpp` — `topologicalOrder()`, keine Versionsverhandlung
- `include/themis/base/module_loader.h` — kein `negotiate`-Symbol

**Issue-Titelvorschlag:** `feat(base): implement runtime version compatibility enforcement in PluginDependencyGraph` (Teil von Issue #1566)  
**Label-Vorschläge:** `enhancement`, `module:base`, `priority:medium`

---

## Zusammenfassung

| # | Issue-Titelvorschlag | Priorität | Labels |
|---|---------------------|-----------|--------|
| 1 | `feat(base): implement automatic plugin restart on health-check failure` | Medium | `enhancement`, `module:base`, `priority:medium` |
| 2 | `feat(base): integrate concrete WasmRuntime (Wasmtime or WasmEdge) into WasmPluginSandbox` | Medium | `enhancement`, `module:base`, `priority:medium` |
| 3 | `feat(base): implement public-key pinning for remote plugin registry` | Low | `enhancement`, `module:base`, `security`, `priority:low` |
| 4 | Teil von Issue #1566: Runtime version compatibility enforcement | Medium | `enhancement`, `module:base`, `priority:medium` |

---

*Generiert: 2026-03-09 | Modul: base | Validierung: Source-Code-Audit commit 0091524*
