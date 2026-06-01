> **Status:** 2026-06-01 – mit aktuellem LLM-Code (`prompt_policy.cpp`, `llm_security_utils.cpp`, `production_validator.cpp`, `token_quota_manager.cpp`) abgeglichen.

# ThemisDB LLM Module - Production Requirements

## Zweck und Geltungsbereich

Dieses Dokument ist der **kanonische Referenzpunkt für produktive Mindestanforderungen** des LLM-Moduls.
Es definiert verbindliche Anforderungen für Prompt-Policy, Model-Lifecycle, Token-Quotas, GPU-Ressourcenmanagement und LLM-Safety.

## Dokumentabgrenzung (Canonical Split)

- **`src/llm/PRODUCTION_REQUIREMENTS.md` (dieses Dokument):** verpflichtende Produktionsanforderungen (MUST/MUST NOT), Sicherheitsannahmen, Betriebsgrenzen.
- **`src/llm/README.md`:** Funktionsübersicht, Architekturkontext, API- und Nutzungsbeispiele.
- **`src/llm/ROADMAP.md`:** Lieferphasen, offene/abgeschlossene Features, Readiness-Planung.
- **`src/llm/FUTURE_ENHANCEMENTS.md`:** mittelfristige/langfristige Erweiterungen und Forschungsfelder.

## Verbindliche LLM-Safety-Anforderungen

- **MUST:** `prompt_policy.cpp` mit aktivem Safety-Ruleset konfiguriert; kein Pass-Through ohne Policy-Check.
- **MUST:** `production_validator.cpp` Validierung vor Backend-Inference-Submission; invalide Requests werden abgewiesen.
- **MUST:** Safety-Classifier (`safety/classifier.cpp`) und Guardian (`safety/guardian.cpp`) aktiv in Produktionspfaden.
- **MUST NOT:** LLM-Requests ohne Prompt-Policy-Check an Backend weiterleiten.

## Verbindliche Ressourcen-Anforderungen

### 1) Token-Quota-Management

- **MUST:** `token_quota_manager.cpp` mit definierten Token-Limits pro Session/User konfiguriert; kein Unlimited-Default.
- **MUST:** Token-Budget-Überschreitungen werden mit explizitem Fehler abgewiesen.

### 2) GPU-Speicherverwaltung

- **MUST:** GPU-Speicherverwaltung (`gpu_memory_manager.cpp`) mit expliziten VRAM-Budgets konfiguriert.
- **MUST:** `cudaDeviceSynchronize`-Fehler in FlashLoRA Forward/Backward propagiert (REL-49..51 geschlossen).
- **MUST:** `getMemoryStats()` prüft `cudaMemGetInfo` und wirft bei Fehler; kein Silence-on-Error.
- **MUST NOT:** GPU-Gesundheitsprüfung deaktivieren; unhealthy GPUs bleiben `is_available=true` aber mit `is_healthy=false` und `last_error` gemeldet.

### 3) Adapter-Lifecycle

- **MUST:** `multi_lora_manager.cpp` mit explizitem Adapter-Lifecycle (Load/Unload/Validation); kein Silent-Adapter-Leak.
- **MUST:** Adapter-Registry (`adapter_registry.cpp`) nur mit validierten Adaptern befüllen.

## Betriebsgrenzen (aktuelles LLM-Verhalten)

- Safety-Monitoring (`safety/monitoring.cpp`) benötigt konfigurierten Monitoring-Adapter; kein impliziter Default-Sink.
- Flash-Attention-CUDA (`attention/cuda/flash_attention_cuda.cu`) CUDA-Backward-Fallback aktiv; Build ohne CUDA fällt auf CPU-Pfad zurück.
- Streaming-Handler und Cancellation-Aware-Execution-Flow müssen getestet sein; partial-Response-Hazards können auftreten.

## Minimaler Produktions-Check (Audit-fähig)

- [ ] Prompt-Policy aktiviert (kein Pass-Through)
- [ ] Production-Validator vor Backend-Inference aktiv
- [ ] Safety-Classifier und Guardian aktiv
- [ ] Token-Quota konfiguriert (kein Unlimited-Default)
- [ ] GPU-VRAM-Budget konfiguriert
- [ ] cudaDeviceSynchronize-Fehler propagiert
- [ ] Adapter-Lifecycle explizit (kein Silent-Leak)
- [ ] Safety-Monitoring-Adapter konfiguriert
- [ ] Produktionsmodus via `THEMIS_PRODUCTION_MODE` oder `THEMIS_ENVIRONMENT` gesetzt

## Review / Sourcecode-Audit-Nachweis

### Betroffene Dateien im Review

- `src/llm/PRODUCTION_REQUIREMENTS.md`
- `src/llm/prompt_policy.cpp`
- `src/llm/llm_security_utils.cpp`
- `src/llm/production_validator.cpp`
- `src/llm/token_quota_manager.cpp`
- `src/llm/multi_lora_manager.cpp`
- `src/llm/adapter_registry.cpp`
- `src/llm/safety/classifier.cpp`
- `src/llm/safety/guardian.cpp`
- `src/llm/safety/monitoring.cpp`
- `src/llm/attention/cuda/flash_attention_cuda.cu`
