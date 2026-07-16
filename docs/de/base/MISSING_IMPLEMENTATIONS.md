# Base-Modul — Fehlende / Unvollständige Implementierungen

**Geprüft:** 2026-03-09 (aktualisiert: 2026-03-11)  
**Geprüft von:** Source-Code-Audit (commit 0091524); Follow-up-Audit (PR copilot/sync-developer-docs-module); Watchdog-Implementierung (PR copilot/add-watchdog-plugin-restart)  
**Modul:** `src/base/` + `include/themis/base/`

---

## Übersicht

Bei der Reality-Check-Prüfung (ROADMAP vs. Sourcecode) wurden folgende Diskrepanzen zwischen dokumentierten Ansprüchen und tatsächlicher Implementierung gefunden. Einträge 1, 3 und 4 wurden inzwischen implementiert und als ✅ geschlossen.

| # | Feature | ROADMAP-Status (alt) | Beobachteter Stand | Schwere | Aktueller Status |
|---|---------|---------------------|-------------------|---------|-----------------|
| 1 | Plugin health monitoring & automatic restart | `[x]` erledigt | Nur Health-Checks beim Laden; kein Auto-Restart | Medium | ✅ Gelöst (2026-03-11) |
| 2 | WASM-based plugin isolation | `[x]` erledigt | Infrastruktur vollständig; Runtime-Injection fehlt | Medium | 🔴 Offen (Issue #1572) |
| 3 | Signed plugin repository with key pinning | `[x]` erledigt | TLS-Zertifikat-Verifikation vorhanden; Public-Key-Pinning fehlt | Low | ✅ Gelöst (2026-03-10) |
| 4 | Runtime plugin capability negotiation (version ranges) | `[x]` erledigt | Version-Ranges in Dependency-Graph gespeichert; kein aktives Negotiation-Protokoll | Low | ✅ Gelöst (2026-03-10) |

---

## Eintrag 1: Plugin Health Monitoring & Automatic Restart — ✅ Gelöst

**Lösung (2026-03-11):** Plugin-Watchdog in `src/base/module_loader.cpp` implementiert.

- `WatchdogConfig` struct (in `include/themis/base/module_loader.h`): konfigurierbare Parameter
  `check_interval_ms`, `max_restart_attempts`, `initial_backoff_ms`, `backoff_multiplier`,
  `max_backoff_ms`, `enabled`.
- `WatchdogModuleStats` struct: per-Modul-Statistiken (`restart_count`, `consecutive_failures`,
  `last_health_check_ms`, `last_failure_ms`, `last_restart_ms`, `next_retry_ms`,
  `permanently_failed`, `last_error`).
- Neue öffentliche API in `ModuleLoader`:
  - `configureWatchdog(WatchdogConfig)` — Konfiguration setzen
  - `startWatchdog()` / `stopWatchdog()` — Lifecycle (idempotent)
  - `isWatchdogRunning() const` — Status-Abfrage
  - `getWatchdogStats(name)` / `getAllWatchdogStats()` — Metriken je Modul
  - `resetWatchdogStats()` — Counters zurücksetzen
- Hintergrund-Thread (`watchdogLoop`) führt periodische Health-Checks auf alle geladenen Module aus.
- Bei Fehler: exponentieller Backoff, max. Restart-Versuche, Markierung als `permanently_failed`.
- Destruktor stoppt den Watchdog-Thread korrekt (kein Race Condition bei Shutdown).
- `PluginWatchdogFocusedTests` in `tests/CMakeLists.txt` (18 Unit-Tests).

**Claim-Quelle:** `src/base/ROADMAP.md`, Abschnitt "Completed ✅"  
> `[x] Plugin health monitoring and automatic restart (Issue: #2373)`

**Ursprünglich beobachtet:**
- Health-Check-Infrastruktur war in `src/base/module_loader.cpp` implementiert: Activation Stage, `runHealthChecks()`, `healthChecks_` Map.
- **Kein Restart-Mechanismus** gefunden: weder in `module_loader.cpp`, `hot_reload_manager.cpp` noch in `module_sandbox.cpp`.

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

## Eintrag 3: Signed Plugin Repository with Key Pinning — ✅ Gelöst

**Lösung (2026-03-10):** `RegistryConfig::pinned_public_key` (Format: `sha256//base64`) wurde in
`include/themis/base/remote_registry_client.h` (Zeile 93) ergänzt. `CURLOPT_PINNEDPUBLICKEY`
wird in beiden HTTP-Pfaden (`httpGet` und `httpGetBinary`) in
`src/base/remote_registry_client.cpp` (Zeilen 336–338, 428–430) gesetzt.

**Ursprüngliche Beobachtung:**
- `RemoteRegistryClient` in `include/themis/base/remote_registry_client.h` bot `RegistryConfig::verify_ssl = true` für TLS-Zertifikatsvalidierung (Zeile 70–71).
- **Kein Public-Key-Pinning**: Kein `pinned_pubkey`-, `cert_pin`- oder `pubkey_fingerprint`-Feld in `remote_registry_client.h` oder `remote_registry_client.cpp`.
- TLS-Zertifikat-Verifikation (CA-basiert) war implementiert; echtes Key-Pinning (unabhängig von CA-Vertrauen) fehlte.

---

## Eintrag 4: Runtime Plugin Capability Negotiation (Version Ranges) — ✅ Gelöst

**Lösung (2026-03-10):** `ModuleDependencyResolver::isVersionCompatible()` und
`topologicalSort()` erzwingen Versionsbeschränkungen beim Auflösen der Ladereihenfolge in
`src/base/module_loader.cpp` (Zeilen 1277–1414). Runtime-Capability-Negotiation auf
höherer Ebene übernimmt `PluginCapabilityNegotiator` im Plugins-Modul (Issue: #1984). Zwei
Bugs in `isVersionCompatible()` (blanket-catch reset) und `resolveFor()` (unregistrierte
transitive Deps bypassten die Pflicht-Abhängigkeitsvalidierung) wurden behoben und durch
Regressionstests in `tests/test_module_loader.cpp` abgedeckt.

**Ursprüngliche Beobachtung:**
- `PluginDependencyGraph` speicherte `minVersion`/`maxVersion` pro Abhängigkeits-Kante (Zeilen 109–110).
- Kein aktives Negotiation-Protokoll: Keine `negotiate()`-, `checkCompatibility()`- oder ähnliche Methode.
- Version-Ranges wurden deklariert und in DOT-Visualisierung angezeigt, aber nicht zur Laufzeit durchgesetzt.
- Tatsächliche Durchsetzung war Teil von Issue #1566 (Plugin dependency resolution).

---

## Zusammenfassung

| # | Issue-Titelvorschlag | Priorität | Labels | Status |
|---|---------------------|-----------|--------|--------|
| 1 | `feat(base): implement automatic plugin restart on health-check failure` | Medium | `enhancement`, `module:base`, `priority:medium` | ✅ Gelöst (2026-03-11) |
| 2 | `feat(base): integrate concrete WasmRuntime (Wasmtime or WasmEdge) into WasmPluginSandbox` | Medium | `enhancement`, `module:base`, `priority:medium` | 🔴 Offen |
| 3 | `feat(base): implement public-key pinning for remote plugin registry` | Low | `enhancement`, `module:base`, `security`, `priority:low` | ✅ Gelöst (2026-03-10) |
| 4 | Teil von Issue #1566: Runtime version compatibility enforcement | Medium | `enhancement`, `module:base`, `priority:medium` | ✅ Gelöst (2026-03-10) |

---

*Generiert: 2026-03-09 | Aktualisiert: 2026-03-11 | Modul: base | Validierung: Source-Code-Audit commit 0091524 + PR copilot/sync-developer-docs-module + Watchdog PR copilot/add-watchdog-plugin-restart*
