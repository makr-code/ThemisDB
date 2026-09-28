> **Status:** 2026-09-28 – mit aktuellem llama_cpp-Code (`llama_cpp_plugin.cpp`, `llama_cpp_registrar.cpp`, v2.4.0) abgeglichen.

# ThemisDB llama_cpp Plugin — Production Requirements

## Zweck und Geltungsbereich

Dieses Dokument ist der **kanonische Referenzpunkt für produktive Mindestanforderungen** des `llama_cpp`-Moduls.
Es definiert verbindliche Betriebs- und Sicherheitsanforderungen für den `LlamaCppPlugin`-LLM-Backend, LoRA-Lifecycle-Management und Registrar-Infrastruktur.

## Dokumentabgrenzung (Canonical Split)

- **`src/llama_cpp/PRODUCTION_REQUIREMENTS.md` (dieses Dokument):** verpflichtende Produktionsanforderungen (MUST/MUST NOT), Sicherheitsannahmen, Betriebsgrenzen.
- **`src/llama_cpp/README.md`:** Funktionsübersicht, Architekturkontext, API- und Nutzungsbeispiele.
- **`src/llama_cpp/ARCHITECTURE.md`:** Komponentenstruktur, Datenfluss, Interfaces, Failure-Paths.
- **`src/llama_cpp/ROADMAP.md`:** Lieferphasen, offene/abgeschlossene Features, Readiness-Planung.
- **`src/llama_cpp/SECURITY.md`:** Threat Model, Security Controls, Sicherheits-Checkliste.
- **`src/llama_cpp/FUTURE_ENHANCEMENTS.md`:** mittelfristige/langfristige Erweiterungen und Forschungsfelder.

## Verbindliche Produktionsanforderungen

- **MUST:** `loadModel()` muss vor `generate()`, `embed()` oder `generateRAG()` aufgerufen werden; kein automatisches Laden aus `generate()`.
- **MUST:** Alle öffentlichen Methoden sind durch `std::mutex mutex_` geschützt; kein ungeschützter Zugriff auf `model_loaded_`, `model_id_` oder `loras_` in Produktionspfaden.
- **MUST:** `generate()` gibt bei nicht geladenem Modell ein strukturiertes Fehlerobjekt zurück (`success=false`); kein Silent-Permit oder Null-Pointer-Dereferenzierung.
- **MUST:** In Produktionsbuilds muss `THEMIS_LLAMA_CPP_STUB_MODE` deaktiviert sein; Stub-Antworten sind ausschließlich für CI-Umgebungen ohne Modelldatei zulässig.
- **MUST:** Modellpfade werden über Konfiguration gesetzt (`loadModel(model_path, config)`); keine hartkodierten Modellpfade in produktivem Code.
- **MUST NOT:** `THEMIS_LLM_PLUGIN()`-Einstiegspunkte (`themis_llm_create`, `themis_llm_destroy`) mit ungültigem Zustand zurückgeben; `themis_llm_create` darf `nullptr` nur zurückgeben, wenn eine Ausnahme verhindert wird.
- **MUST NOT:** Sicherheits- oder Autorisierungs-Checks in Produktionspfaden deaktivieren.

## Verbindliche Sicherheitsanforderungen

- Sicherheitsrelevante Operationen (Modell-Loading, LoRA-Registration) werden atomar und mutex-geschützt ausgeführt.
- Fehler in sicherheitskritischen Pfaden (Modell-Loading, LoRA-Deserialisierung) werden als explizite Fehlercodes propagiert; kein Silent-Permit.
- `getPerformanceStats()` / `getMemoryStats()` dürfen keine Credentials, Prompts oder benutzerspezifische Daten enthalten.
- **[COMPLETED v2.4.0]** Modell-Integritätsprüfung (SHA-256-Digest): opt-in via `"verify_model_digest": true` + `"expected_model_digest"` config keys; fail-closed (B3).
- **[COMPLETED v2.4.0]** Upstream-Prompt-Policy-Check via `setPolicyFn`-Hook integriert; `generate()` / `generateRAG()` gate on denial (B4).

## Betriebsgrenzen

- Konfigurationswerte (`n_ctx`, `n_gpu_layers`, `n_batch`, `n_threads`) müssen deployment-spezifisch gesetzt sein; Default-Werte gelten nicht als produktionssicher.
- `generateBatch()` ist in v2.0.0 sequenziell; parallele Batch-Inferenz ist für künftige Releases geplant — Throughput-Limits bei hoher Last beachten.
- `embed()` gibt bei fehlendem Backend einen 384-dimensionalen Null-Vektor zurück; in Produktionsumgebungen muss ein reales Backend oder eine injizierte `EmbedFn` konfiguriert sein.
- Rate-Limiting für `generate()`-Aufrufe liegt in der Verantwortung der API-Schicht; `LlamaCppPlugin` hat keinen eingebauten Rate-Limiter.
- Externe Abhängigkeiten (LlamaWrapper, LoRA-Loader) müssen mit expliziten Timeout- und Retry-Policies konfiguriert sein.

## Minimaler Produktions-Check (Audit-fähig)

- [x] Modul-Konfiguration vollständig und beim Start validiert (`model_path`, `n_ctx`, `n_gpu_layers`) — v2.4.0 complete
- [x] `THEMIS_LLAMA_CPP_STUB_MODE` deaktiviert in Produktionsbuild — CI/Test-only mode gated correctly
- [x] Sicherheits- und Autorisierungs-Checks aktiv (Mutex-Schutz, Null-Guard in `generate()`) — Phase 7 hardening complete
- [x] `embed()` mit realem Backend via `LlamaWrapper` oder injizierter `EmbedFn` konfiguriert — v2.2.0 real inference wired
- [x] Kein hartcodierter Modellpfad im deployten Build — model path via config only
- [x] Audit-Logging für Modell-Loading und LoRA-Lifecycle verfügbar (optional, per deployment config)
- [x] Modell-Integritätsprüfung aktiviert (v2.4.0 B3): opt-in via `"verify_model_digest": true` + `"expected_model_digest"` config keys
- [x] LoRA-Adapter-Integritätsprüfung aktiviert (v2.4.0 B2): GGUF magic bytes + 2 GB size bound validated in `importLoRA`
- [x] Inference-Policy-Gate aktiviert (v2.4.0 B4): `setPolicyFn(fn)` pluggable hook; denial returns `success=false`
- [x] Rate-Limiting auf API-Schicht konfiguriert (verantwortlich der aufrufenden API)
- [x] Produktionsmodus via `THEMIS_PRODUCTION_MODE` oder `THEMIS_ENVIRONMENT` gesetzt (optional per deployment)

## Review / Sourcecode-Audit-Nachweis

### Betroffene Dateien im Review

- `src/llama_cpp/PRODUCTION_REQUIREMENTS.md`
- `src/llama_cpp/llama_cpp_plugin.cpp`
- `src/llama_cpp/llama_cpp_registrar.cpp`
- `include/llama_cpp/llama_cpp_plugin.h`
- `include/llama_cpp/llama_cpp_registrar.h`

### Verwandte Governance-Dokumente

- [`SECURITY.md`](SECURITY.md)
- [`ARCHITECTURE.md`](ARCHITECTURE.md)
- [`ROADMAP.md`](ROADMAP.md)
- [`PERFORMANCE_EXPECTATIONS.md`](PERFORMANCE_EXPECTATIONS.md)
