# LLM-Modul – Primäres Inventar

<!-- Status: current | validated: 2026-04-09 -->
<!-- Primärdokumentation: ../../../src/llm/ | ../../../include/llm/ -->

**Datum:** 9. April 2026  
**Modul:** `llm`  
**Modulpfad:** `src/llm/` + `include/llm/`

---

## 1. Dokumentationsdateien im Modul

| Datei | Beschreibung |
|---|---|
| `src/llm/README.md` | Modulübersicht, Architekturüberblick, Delivery-Status, Komponenten, Wissenschaftliche Referenzen |
| `src/llm/ARCHITECTURE.md` | Detaillierte Architektur: Design-Prinzipien, Komponententabelle, Datenfluß, Threading, Performance, Sicherheit, Konfiguration, Fehlerbehandlung |
| `src/llm/ROADMAP.md` | Implementierungsstatus, abgeschlossene Features, geplante Features (v1.16.0), Phasenmodell, Production-Readiness-Checkliste, Breaking Changes |
| `src/llm/FUTURE_ENHANCEMENTS.md` | Geplante Erweiterungen (Scope: federated inference), Design-Constraints, IEEE-Referenzen |
| `src/llm/gguf_loader_README.md` | GGUF-Modelllader: Format, Validierung, Fehlerbehandlung |
| `src/llm/llama_lora_adapter_README.md` | LoRA-Adapter-Integration mit llama.cpp: API-Erkennung, Lifecycle |
| `include/llm/README.md` | Übersicht der öffentlichen Header-Dateien; Architekturhinweis |
| `include/llm/FUTURE_ENHANCEMENTS.md` | Header-Interface-Enhancements (Streaming, OpenAI-Adapter, LoRA Hot-Swap – alle implementiert) |

---

## 2. Quellcode-Dateien (`src/llm/`)

### Inference Engines

| Quelldatei | Header | Beschreibung |
|---|---|---|
| `async_inference_engine.cpp` | `include/llm/async_inference_engine.h` | Leichtgewichtige Async-Wrapper für Single-Model-Inferenz; Priority-Queue, Worker-Thread-Pool |
| `inference_engine_enhanced.cpp` | `include/llm/inference_engine_enhanced.h` | Enterprise Multi-Model-Engine: KV-Cache, Batching, Load-Balancing, LoRA-Hot-Loading, Per-Modell-Quoten, Hot-Swap |
| `inference_handle.cpp` | `include/llm/inference_handle.h` | Geteilter Async-Request-Handle (`get()`, `ready()`, `cancel()`) |
| `llamacpp_inference_engine.cpp` | `include/llm/llamacpp_inference_engine.h` | llama.cpp Backend-Integration |
| `embedded_llm.cpp` | `include/llm/embedded_llm.h` | In-Process Embedded LLM Server |
| `embedded_llm_stub.cpp` | `include/llm/embedded_llm.h` | Stub/default constructors für `EmbeddedLLM` (no-op impl für Builds ohne llama.cpp) |
| `ai_orchestrator.cpp` | `include/llm/ai_orchestrator.h` | Multi-Model-Orchestrierung und Routing; Mode-Spec-Loading |
| `mode_spec_loader.cpp` | *(in ai_orchestrator.h)* | YAML Loader für LLM-Orchestrierungsmodi |

### llama.cpp Integration

| Quelldatei | Header | Beschreibung |
|---|---|---|
| `llama_wrapper.cpp` | `include/llm/llama_wrapper.h` | Direkte llama.cpp C-API-Wrapper; Tokenisierung, Inferenz, Sampling, Vision |
| `llama_grammar_adapter.cpp` | *(extern "C")* | Dynamische llama.cpp Grammar-API-Erkennung via dlsym/GetProcAddress |
| `llama_lora_adapter.cpp` | *(extern "C")* | Dynamische llama.cpp LoRA-API-Erkennung (init/set/remove/clear/free) |
| `llama_resource_manager.cpp` | `include/llm/llama_resource_manager.h` | RAII-basiertes Resource-Management für llama.cpp Modelle und Contexts |
| `gguf_loader.cpp` | `include/llm/gguf_loader.h` | GGUF-Modell-Datei-Laden und -Validierung |

### Modell-Management

| Quelldatei | Header | Beschreibung |
|---|---|---|
| `model_loader.cpp` | `include/llm/model_loader.h` | Modell-Lifecycle: Laden, Cachen, Validieren |
| `model_downloader.cpp` | `include/llm/model_downloader.h` | Modell-Download (HuggingFace, URL) |
| `llm_model_storage.cpp` | `include/llm/llm_model_storage.h` | Modell-Gewichte-Storage-Integration; `LLMModelMetadata` als BaseEntity |
| `model_metadata_cache.cpp` | `include/llm/model_metadata_cache.h` | Gecachete Modell-Metadaten |
| `ml_model_manager.cpp` | `include/llm/ml_model_manager.h` | Allgemeines ML-Modell-Management (Non-LLM) |
| `model_router.cpp` | `include/llm/model_router.h` | Multi-Model-Routing auf Basis von Prompt-Inhalt oder Metadaten-Tags (ECMAScript-Regex) |
| `model_quantization_pipeline.cpp` | `include/llm/model_quantization_pipeline.h` | Unified Loading von GGUF/AWQ/GPTQ quantisierten Modellen |

### KV-Cache & Batching

| Quelldatei | Header | Beschreibung |
|---|---|---|
| `paged_kv_cache.cpp` | `include/llm/paged_kv_cache.h` | Paged KV-Cache (vLLM-inspiriert) |
| `paged_kv_cache_manager.cpp` | `include/llm/paged_kv_cache_manager.h` | KV-Cache-Seiten-Allokation/Deallokation |
| `paged_block_manager.cpp` | `include/llm/paged_block_manager.h` | KV-Cache-Block-Allokation |
| `kv_cache_buffer.cpp` | `include/llm/kv_cache_buffer.h` | Rohpuffer-Verwaltung für KV-Cache |
| `block_table.cpp` | `include/llm/block_table.h` | Seitentabelle für KV-Cache-Blöcke |
| `llm_prefix_cache.cpp` | `include/llm/llm_prefix_cache.h` | Prompt-Präfix-Caching für Wiederverwendung |
| `llm_response_cache.cpp` | `include/llm/llm_response_cache.h` | Response-Level-Caching (Dedup-Cache) |
| `continuous_batch_scheduler.cpp` | `include/llm/continuous_batch_scheduler.h` | Continuous Batching für Durchsatz |

### LoRA & Adapter

| Quelldatei | Header | Beschreibung |
|---|---|---|
| `lora_router.cpp` | `include/llm/lora_router.h` | Per-Request-LoRA-Adapter-Auswahl (Domain, Tenant) |
| `multi_lora_manager.cpp` | `include/llm/multi_lora_manager.h` | Concurrent-Multi-LoRA-Management |
| `lora_metadata_cache.cpp` | `include/llm/lora_metadata_cache.h` | Gecachete LoRA-Adapter-Metadaten |
| `lora_security_validator.cpp` | `include/llm/lora_security_validator.h` | LoRA-Adapter-Sicherheitsvalidierung vor dem Laden |
| `lora_certificate_store.cpp` | `include/llm/lora_certificate_store.h` | `LoRACertificateStore`: Filesystem-basierter Zertifikatsspeicher für LoRA-Signatur-Verifikation; Fallback auf System-Zertifikatsspeicher (`/etc/ssl/certs`) |
| `adapter_registry.cpp` | `include/llm/adapter_registry.h` | Registry für verfügbare LLM-Adapter |
| `adapter_load_balancer.cpp` | `include/llm/adapter_load_balancer.h` | Load-Balancing über Adapter-Instanzen |

### Grammar & Sampling

| Quelldatei | Header | Beschreibung |
|---|---|---|
| `grammar.cpp` | `include/llm/grammar.h` | Grammar-constrained Generation (EBNF/GBNF); llama.cpp Grammar-API via `llama_grammar_adapter.cpp` |
| `grammar_cache.cpp` | `include/llm/grammar_cache.h` | LRU-Cache für kompilierte Grammatiken |
| `json_schema_converter.cpp` | `include/llm/json_schema_converter.h` | JSON-Schema → EBNF-Konvertierung; Tool-Calling-Support |
| `sampling_strategy.cpp` | `include/llm/sampling_strategy.h` | Token-Sampling: Temperature, Top-P, Top-K, Beam-Search |
| `speculative_decoder.cpp` | `include/llm/speculative_decoder.h` | Speculative Decoding (Draft-Model + Target-Model) |

### GPU & Memory

| Quelldatei | Header | Beschreibung |
|---|---|---|
| `adaptive_vram_allocator.cpp` | `include/llm/adaptive_vram_allocator.h` | Dynamische VRAM-Allokation; `allocateWithFragmentation()` und `handleOutOfMemory()` als Stubs (GPU-Hardware erforderlich) |
| `gpu_memory_manager.cpp` | `include/llm/gpu_memory_manager.h` | GPU-Memory-Lifecycle für LLM-Inferenz |
| `gpu_safe_fail.cpp` | `include/llm/gpu_safe_fail.h` | LLM-spezifischer GPU-Safe-Fail (GPU → CPU Fallback) |
| `multi_gpu_memory_coordinator.cpp` | `include/llm/multi_gpu_memory_coordinator.h` | Multi-GPU-Memory-Koordination |
| `mixed_precision_inference.cpp` | `include/llm/mixed_precision_inference.h` | FP16/BF16/INT8/INT4-Inferenz |
| `kernel_fusion.cpp` / `kernel_fusion.cu` | `include/llm/kernel_fusion.h` | GPU-Kernel-Fusion für Inferenz |

### Vision

| Quelldatei | Header | Beschreibung |
|---|---|---|
| `vision_encoder.cpp` | `include/llm/vision_encoder.h` | Vision-Input-Kodierung (Bilder); experimentell |
| `vision_config.cpp` | `include/llm/vision_config.h` | Vision-Modell-Konfiguration |
| `vision_resource_monitor.cpp` | `include/llm/vision_resource_monitor.h` | Vision-Inferenz-Ressourcenüberwachung |

### Sicherheit & Ethics

| Quelldatei | Header | Beschreibung |
|---|---|---|
| `ai_decision_auditor.cpp` | `include/llm/ai_decision_auditor.h` | KI-Entscheidungs-Audit für Compliance |
| `constitutional_reasoning_engine.cpp` | `include/llm/constitutional_reasoning_engine.h` | Constitutional AI Reasoning (Sicherheitseinschränkungen) |
| `ethical_guidelines_manager.cpp` | `include/llm/ethical_guidelines_manager.h` | Ethics-Regeln und Durchsetzung |
| `ethics_aware_confidence_detector.cpp` | `include/llm/ethics_aware_confidence_detector.h` | Konfidenz-Scoring mit Ethics-Integration |
| `moral_analyzer.cpp` | `include/llm/moral_analyzer.h` | Moralanalyse von LLM-Ausgaben |
| `llm_security_utils.cpp` | `include/llm/llm_security_utils.h` | Prompt-Injection-Erkennung, PII-Filterung |
| `prompt_policy.cpp` | `include/llm/prompt_policy.h` | Prompt-Policy-Durchsetzung (Content-Filter) |
| `lora_security_validator.cpp` | `include/llm/lora_security_validator.h` | LoRA-Sicherheitsvalidierung |

### Observability & Metriken

| Quelldatei | Header | Beschreibung |
|---|---|---|
| `grafana_metrics.cpp` | `include/llm/grafana_metrics.h` | Grafana/Prometheus-Metriken für LLM-Operationen |
| `token_quota_manager.cpp` | `include/llm/token_quota_manager.h` | Per-Tenant-Token-Quota-Durchsetzung |
| `llm_interaction_store.cpp` | `include/llm/llm_interaction_store.h` | LLM-Interaktionen persistieren (Audit/Analyse) |
| `llm_model_audit_logger.cpp` | `include/llm/llm_model_audit_logger.h` | Strukturiertes Audit-Log für Modell-Nutzung |

### Streaming & Worker

| Quelldatei | Header | Beschreibung |
|---|---|---|
| `streaming_handler.cpp` | `include/llm/streaming_handler.h` | Token-Streaming (SSE / WebSocket) |
| `shared_worker_pool.cpp` | `include/llm/shared_worker_pool.h` | Geteilter Work-Stealing-Thread-Pool für beide Engines |
| `inference_handle.cpp` | `include/llm/inference_handle.h` | Async-Request-Handle (get/ready/cancel) |

### Prompt-Management

| Quelldatei | Header | Beschreibung |
|---|---|---|
| `prompt_manager.cpp` | `include/llm/prompt_manager.h` | Prompt-Verwaltung und Optimierung |
| `prompt_optimizer.cpp` | `include/llm/prompt_optimizer.h` | Prompt-Optimierung |
| `prompt_evaluator.cpp` | `include/llm/prompt_evaluator.h` | Prompt-Policy-Evaluierung |
| `meta_prompt_generator.cpp` | `include/llm/meta_prompt_generator.h` | Meta-Prompt-Generierung |
| `fewshot_optimizer.cpp` | `include/llm/fewshot_optimizer.h` | Few-Shot-Beispiel-Auswahl |
| `explanation_generator.cpp` | `include/llm/explanation_generator.h` | Natürlichsprachige Erklärung von KI-Entscheidungen |

### OpenAI-Kompatibilität & Plugins

| Quelldatei | Header | Beschreibung |
|---|---|---|
| `openai_compat_adapter.cpp` | `include/llm/openai_compat_adapter.h` | OpenAI-kompatibler `/v1/chat/completions`-Adapter |
| `llm_deployment_plugin.cpp` | `include/llm/llm_deployment_plugin.h` | LLM-Deployment-Plugin |
| `llm_plugin_manager.cpp` | `include/llm/llm_plugin_manager.h` | Dynamisches LLM-Backend-Plugin-Loading |
| `mcp_tool_bridge.cpp` | *(mcp_tool_bridge.h)* | MCP (Model Context Protocol) Tool-Bridge |

### Sonstige

| Quelldatei | Header | Beschreibung |
|---|---|---|
| `docs_assistant.cpp` | *(docs_assistant.h)* | Dokumentations-bewusster Assistent; `generateAnswer()` nutzt `THEMIS_LLM_GENERATE` wenn `THEMIS_ENABLE_LLM` gesetzt |
| `feedback_store.cpp` | `include/llm/feedback_store.h` | User-Feedback-Sammlung |
| `feedback_plugin_basic.cpp` | *(i_feedback_plugin.h)* | Basis-Feedback-Plugin |
| `byzantine_detector.cpp` | `include/llm/byzantine_detector.h` | Byzantine-Fault-Erkennung in verteilter Inferenz |
| `multi_perspective_generator.cpp` | `include/llm/multi_perspective_generator.h` | `MultiPerspectiveGenerator`: Generiert Antworten aus mehreren Perspektiven (z. B. faktisch, ethisch, rechtlich); 🟢 Production-ready |
| `production_validator.cpp` | `include/llm/production_validator.h` | `ProductionValidator`: Validiert Produktionsbedingungen (Engine-Status, GPU-Verfügbarkeit, Quota); 🟠 Beta (14 Test-TODOs offen) |
| `distributed_training_coordinator.cpp` | *(distributed_training_coordinator.h)* | Verteilte Trainings-Koordination |
| `aql_train_parser.cpp` | *(aql_train_parser.h)* | AQL-Trainingsdaten-Parser |

---

## 3. Sekundäre Dokumentation (`docs/de/llm/`)

| Datei | Beschreibung |
|---|---|
| `docs/de/llm/README.md` | Deutsche LLM-Übersicht; Quicklinks auf englische Feature-Docs |
| `docs/de/llm/inventory.md` | Dieses Inventardokument |
| `docs/de/llm/missing-implementations.md` | Report fehlender/unvollständiger Implementierungen |
| `docs/de/llm/README_PLUGINS.md` | Plugin-Entwicklung: Quickstart & Beispiele |
| `docs/de/llm/README_QLORA.md` | QLoRA-Integration |
| `docs/de/llm/LORA_DOKUMENTATIONS_HUB.md` | LoRA-Dokumentationshub |
| `docs/de/llm/LLM_PLUGIN_DEVELOPMENT_GUIDE.md` | Plugin-Entwicklungs-Leitfaden |
| `docs/de/llm/LLM_LOADER_GUIDE.md` | vLLM-ähnlicher LLM-Loader-Leitfaden |
| `docs/en/llm/` | Englische Feature-Docs (Grammar, Speculative Decoding, Vision, etc.) |

---

## 4. Weiterführende Dokumentation (Root-Ebene)

| Datei | Beschreibung |
|---|---|
| `docs/llm_roadmap.md` | Produktionsreife-Bewertung und Roadmap (älterer Stand) |
| `docs/en/llm/LLM_IMPLEMENTATION_PLAN_100_PERCENT.md` | Implementierungsplan (historisch) |
| `ROADMAP.md` (Root) | Aggregierte Roadmap aller Module; LLM-Status: v1.16.0 |

---

## 5. Reality-Check-Ergebnis (Stand: April 2026)

### ✅ Korrekt dokumentiert
- Alle Inference-Engine-Komponenten (`AsyncInferenceEngine`, `InferenceEngineEnhanced`) sind implementiert und korrekt dokumentiert
- ROADMAP-Status v1.16.0 spiegelt den Implementierungsstand korrekt wider (alle kurz- und langfristigen Features abgeschlossen außer Federated Inference)
- Grammar-API via `llama_grammar_adapter.cpp` dynamisch erkannt; Fallback korrekt dokumentiert
- LoRA-API via `llama_lora_adapter.cpp` dynamisch erkannt; Fallback korrekt dokumentiert
- OpenAI-kompatibler Adapter (`openai_compat_adapter.h/.cpp`) implementiert
- Speculative Decoding, Shared Worker Pool, Streaming-Handler implementiert

### 🔧 Korrigiert (in diesem PR, 2026-04-09)
- Inventar: 4 bisher nicht dokumentierte Quelldateien ergänzt (`embedded_llm_stub.cpp`, `lora_certificate_store.cpp`, `multi_perspective_generator.cpp`, `production_validator.cpp`)
- `docs_assistant.cpp`-Eintrag korrigiert: LLM-Completion-Macro ist implementiert (nicht mehr fehlend)
- `missing-implementations.json`: LLM-MISSING-002, 003, 004 als gelöst markiert (waren bereits in md als gelöst dokumentiert)

### 🔧 Korrigiert (frühere PRs, März 2026)
- `src/llm/README.md` Delivery-Status: "🟡 Beta / in progress" → "🟢 Production-ready (v1.16.0)"
- `src/llm/ARCHITECTURE.md` Version und Datum aktualisiert; Abschnitt 11 (Known Limitations) stale Einträge entfernt
- `src/llm/ROADMAP.md` Phase 3 als Completed markiert; alle implementierten Features auf `[x]` gesetzt
- `include/llm/FUTURE_ENHANCEMENTS.md` Alle abgeschlossenen Features auf `[x]`; Planungs-Sektion in Implementations-Sektion umbenannt
- `src/llm/FUTURE_ENHANCEMENTS.md` IEEE-Referenzen ergänzt (13 Quellen)

### ⚠️ Bekannte Einschränkungen / Stubs
- `adaptive_vram_allocator.cpp`: `allocateWithFragmentation()` und `handleOutOfMemory()` delegieren an `ActiveVRAMAllocator`; echter GPU-VRAM-Support erfordert `cudaMalloc`-Build
- Vision-Support: Experimentell; nur bestimmte Modellarchitekturen unterstützt
- Federated Inference: Nicht implementiert (Issue: #1928)
- `production_validator.cpp`: 🟠 Beta — 14 Test-TODOs; Produktionslogik implementiert, Integration-Tests fehlen noch

Detaillierter Report: [`missing-implementations.md`](missing-implementations.md)
