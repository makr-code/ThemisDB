# Plugins Modul — Fehlende / Unvollständige Implementierungen

<!-- Status: updated | validated: 2026-04-06 | updated: 2026-03-10 -->
<!-- Primärdokumentation: ../../../src/plugins/ -->

Dieser Report dokumentiert Funktionen, die in `src/plugins/ROADMAP.md`,
`src/plugins/FUTURE_ENHANCEMENTS.md` oder zugehörigen Docs als implementiert oder geplant
beschrieben werden, jedoch bei der Reality-Check-Prüfung als **nicht vollständig umgesetzt**
oder als **fehlerhaft dokumentiert** befunden wurden.

Prüfstand: 2026-03-09 | Branch: `develop` | Aktualisiert: 2026-03-10

---

## 1. README: Falsche Dateinamen (bereits korrigiert)

| Feld | Wert |
|---|---|
| **Claim-Quelle** | `src/plugins/README.md` §"Relevant Interfaces" (Stand vor 2026-03-09) |
| **Erwartet** | `plugin_loader.cpp`, `plugin_api.cpp`, `manifest_validator.cpp`, `plugin_signer.cpp` |
| **Beobachtet** | Diese Dateien existieren **nicht** in `src/plugins/`; die tatsächliche Implementierung liegt in `plugin_manager.cpp`, `plugin_registry.cpp`, `signed_plugin_repository.cpp` usw. |
| **Evidence** | `ls src/plugins/*.cpp` — keine der 4 genannten Dateien vorhanden |
| **Status** | ✅ Korrigiert in diesem PR — README wurde auf tatsächliche Dateinamen aktualisiert |
| **Issue-Titelvorschlag** | (kein Issue nötig — dokumentarische Korrektur) |

---

## 2. ROADMAP: Hot-Reload als `[ ]` markiert, obwohl vollständig implementiert (bereits korrigiert)

| Feld | Wert |
|---|---|
| **Claim-Quelle** | `src/plugins/ROADMAP.md` Phase 2, Zeile 56: `- [ ] Plugin hot-reload without server restart` |
| **Erwartet** | Marker `[ ]` (nicht begonnen) — ROADMAP-Beschreibung "In Progress (Issue: #2223)" ist ebenfalls inkonsistent |
| **Beobachtet** | `PluginManager::reloadPlugin()` ist **vollständig implementiert** in `src/plugins/plugin_manager.cpp:953`; ~250 Zeilen TOCTOU-sicheres 3-Phasen-Reload (Phase 1: pre-validation, Phase 2: load new binary + verify signature, Phase 3: atomic swap + cleanup) |
| **Evidence** | `src/plugins/plugin_manager.cpp:953–1200`; `tests/test_plugin_hot_reload_enhanced.cpp` |
| **Status** | ✅ Korrigiert in diesem PR — ROADMAP auf `[x]` aktualisiert mit Evidence-Referenz |
| **Issue-Titelvorschlag** | (kein Issue nötig — ROADMAP-Korrektheit) |

---

## 3. Ed25519 Manifest-Signing mit Key-Rotation — ✅ Gelöst

| Feld | Wert |
|---|---|
| **Claim-Quelle** | `src/plugins/ROADMAP.md` Phase 2: `[x] Ed25519 manifest signing workflow with key-rotation support` |
| **Erwartet** | Ed25519-Signierung mit Key-Rotation-Unterstützung |
| **Beobachtet (alt)** | `manifest_signer.cpp` existierte nicht; nur manuelles Key-Management via `addPinnedKey`/`removePinnedKey` |
| **Lösung (2026-03-10)** | `signed_plugin_repository.cpp` enthält die vollständige Ed25519-Signierlogik (`verifyEd25519Signature()` via OpenSSL EVP_PKEY). ROADMAP Phase 2 markiert `[x]`. Feature wurde direkt in `signed_plugin_repository.cpp` implementiert statt in einer separaten `manifest_signer.cpp`-Datei. `Stubs: 0` im Datei-Header bestätigt. |

---

## 4. Capability-basiertes Berechtigungsmodell — nicht vollständig implementiert

| Feld | Wert |
|---|---|
| **Claim-Quelle** | `src/plugins/ROADMAP.md` Phase 2: `[?] Capability-based permission model (fine-grained access control per plugin)` |
| **Erwartet** | Feinkörniges Capability-Modell: Plugins können nur deklarierte Capabilities nutzen; Runtime-Durchsetzung verhindert Capability-Eskalation |
| **Beobachtet** | `PluginCapabilityNegotiator` in `include/plugins/plugin_interface.h` ist implementiert und ermöglicht Capability-Negotiation; jedoch: **Runtime-Durchsetzung** (verhindert Capability-Eskalation nach Aktivierung) ist nicht implementiert — `plugin_system_edition.cpp` enthält `CapabilityChecker`, aber kein enforcement auf dem Hot-Call-Pfad |
| **Evidence** | `src/plugins/plugin_manager.cpp:1288` (Negotiation implementiert); `src/plugins/plugin_system_edition.cpp` (CapabilityChecker vorhanden, aber kein Runtime-Gate auf Call-Pfad) |
| **ROADMAP-Status** | `[?]` – teilweise (Negotiation: ✅; Runtime-Enforcement: ✗) |
| **Issue-Titelvorschlag** | `[plugins] Implement runtime capability enforcement to prevent post-activation capability escalation` |
| **Label-Vorschläge** | `type:security`, `priority:high`, `plugins`, `status:planned` |

---

## 5. WASM-Sandbox-Isolation — nicht implementiert

| Feld | Wert |
|---|---|
| **Claim-Quelle** | `src/plugins/ROADMAP.md` Phase 3: `[?] WebAssembly (WASM) plugin runtime for sandboxed execution` |
| **Erwartet** | WASM-Runtime (Wasmtime oder WasmEdge) als Alternative zu `dlopen`; Memory-sichere Isolation; `wasm_plugin_loader.cpp`; `IWASMSandbox`-Interface |
| **Beobachtet** | Kein WASM-Code in `src/plugins/`; `include/plugins/FUTURE_ENHANCEMENTS.md` beschreibt das Interface als komplett geplant (`[ ]`); `tests/test_wasm_plugin_sandbox.cpp` existiert als Test-Scaffold |
| **Evidence** | `ls src/plugins/*.cpp` — kein WASM-Code; `include/plugins/FUTURE_ENHANCEMENTS.md` §"WebAssembly Sandbox Plugin Interface" (alle `[ ]`) |
| **ROADMAP-Status** | `[?]` – geplant (Phase 3) |
| **Issue-Titelvorschlag** | `[plugins] Implement WASM sandbox runtime via Wasmtime for plugin isolation` |
| **Label-Vorschläge** | `type:feature`, `priority:medium`, `plugins`, `status:planned` |

---

## 6. Plugin-Metriken-Dashboard — ✅ Gelöst

| Feld | Wert |
|---|---|
| **Claim-Quelle** | `src/plugins/FUTURE_ENHANCEMENTS.md` §"Plugin Metrics Dashboard Integration"; `src/plugins/ROADMAP.md` Phase 3 |
| **Erwartet** | `PluginMetricsCollector` mit Prometheus-Scrape-Endpunkt; `plugin_health_score` Gauge; Grafana-Dashboard |
| **Beobachtet (alt)** | `plugin_health_monitor.cpp` emittierte keinen `plugin_health_score` Gauge; kein Grafana-Dashboard |
| **Lösung (2026-03-10)** | `plugin_health_monitor.cpp` (Zeile 661) emittiert `setGauge("plugin_health_score", score, {{"plugin", plugin.name}})` über optionalen `IMetrics`-Sink. Grafana-Dashboard unter `grafana/dashboards/plugins.json`. ROADMAP Phase 3 markiert `[x]`. `Stubs: 0` im Datei-Header bestätigt. |

---

## Nicht als Issues betroffene Befunde (Hinweise)

### Test-Coverage unbekannt

`ROADMAP.md` §"Production Readiness Checklist" markiert "Unit tests coverage > 80%" als `[?]`. Angesichts von 15+ Testdateien für 10 Quelldateien ist eine Coverage > 80% wahrscheinlich, aber nicht gemessen.

### README Maturity-Label

Das README war auf "Alpha" gesetzt. Nach Reality-Check (Hot-Reload, OCI, SignedRepo, HealthMonitor alle implementiert) wurde es auf "Beta" korrigiert. Der Status bleibt Beta wegen der offenen WASM-Sandbox und Capability-Enforcement.

---

## Zusammenfassung

| # | Feature | Quelle | Kritikalität | Status |
|---|---|---|---|---|
| 1 | Falsche Dateinamen im README | README (alt) | Niedrig | ✅ Korrigiert |
| 2 | Hot-Reload ROADMAP-Marker falsch | ROADMAP Phase 2 | Niedrig | ✅ Korrigiert |
| 3 | Ed25519 Key-Rotation (`manifest_signer.cpp`) | ROADMAP Phase 2 | Hoch | ✅ Gelöst (2026-03-10) |
| 4 | Runtime Capability Enforcement | ROADMAP Phase 2 | Hoch | `[?]` teilweise (Negotiation ✅; Enforcement ✗) |
| 5 | WASM-Sandbox-Isolation | ROADMAP Phase 3 | Mittel | `[?]` geplant |
| 6 | Plugin-Metriken-Dashboard + Health-Gauge | FUTURE_ENHANCEMENTS | Mittel | ✅ Gelöst (2026-03-10) |

*Alle Phase-1-ROADMAP-Einträge (`[x]`) sind durch vorhandene Implementierungsdateien auf
`develop` belegt. Die neu verifizierten Phase-2-Einträge (Hot-Reload, Dependency-Resolution,
OCI, Health-Monitor) wurden ebenfalls als `[x]` markiert.*
