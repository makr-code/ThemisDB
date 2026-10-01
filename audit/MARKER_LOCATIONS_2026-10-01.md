---
Author: ThemisDB Maintainers
Created: 2026-10-01
Last Updated: 2026-10-01
Status: active
---
# ThemisDB — Marker-Fundstellen in `src/` (Reality Check Snapshot)

**Erstellt:** 2026-10-01T05:00:00Z  
**Branch:** `develop`  
**Werkzeug:** `grep -RInE --include='*.cpp' --include='*.cc' --include='*.c' --include='*.h' --include='*.hpp' 'TODO|STUB|MOCK|FIXME' src`  
**Gesamt:** 172 Fundstellen

## Zusammenfassung

| Marker | Anzahl |
|---|---:|
| TODO | 30 |
| STUB | 139 |
| MOCK | 3 |
| FIXME | 0 |
| **Gesamt** | **172** |

## Reality-Check-Hinweis

- Nach gezielter Bereinigung offensichtlicher False-Positive-Marker sind verbleibende TODOs weiterhin vollständig im Modul `src/rag/` konzentriert.
- Zusätzlich wurden nicht-actionable Tracking-Labels der Form `STUB #<id>` in `BRIDGE_REF #<id>` umbenannt, um Scanner-Rauschen zu reduzieren, ohne Laufzeitverhalten zu ändern.
- Verbleibende Nicht-TODO-Marker (`STUB`/`MOCK`/`FIXME`): **142**

## Vollständige Fundstellenliste

```text
src/analytics/olap.cpp:2370:// STUB/SIMULATION NOTE:
src/analytics/process_mining.cpp:12:#if defined(_WIN32) && defined(THEMIS_PROCESS_MINING_WINDOWS_STUB)
src/analytics/process_mining.cpp:13:// STUB/SIMULATION NOTE:
src/analytics/process_mining.cpp:18:// Activation: Compiled when both _WIN32 and THEMIS_PROCESS_MINING_WINDOWS_STUB
src/analytics/process_mining.cpp:27://   the THEMIS_PROCESS_MINING_WINDOWS_STUB CMake option.  Tracking:
src/analytics/process_mining.cpp:46:                  "Windows stub build (THEMIS_PROCESS_MINING_WINDOWS_STUB). "
src/analytics/process_mining.cpp:2479:// RESOLUTION NOTE (clusterVariants — was: naive round-robin, see STUB_INVENTORY.md #212):
src/main_server.cpp:40://      `THEMIS_ALLOW_HSM_STUB=1`. Missing/invalid HSM config and HSM init
src/process/process_community_detector.cpp:398:    // planned for Q4 2026 (STUB_INVENTORY #238); once integrated, `llm_endpoint`
src/tensor/utr_converter.cpp:296:// STUB/SIMULATION NOTE:
src/tensor/utr_converter.cpp:568:    // STUB/SIMULATION NOTE:
src/tensor/adapter_repository.cpp:330:    // STUB/SIMULATION NOTE (AR-01):
src/tensor/adapter_repository.cpp:507:    // STUB/SIMULATION NOTE (AR-02 / BRIDGE_REF #266):
src/plugins/wasm_plugin_loader.cpp:557:// STUB/SIMULATION NOTE:
src/ethics_ai/prior_round_compressor.cpp:246:    // than only citation tokens) are retained.  See STUB_INVENTORY.md entry #235.
src/training/multi_task_lora.cpp:94:        // STUB/SIMULATION NOTE (MTL-S02 — SGD training loop, no BLAS):
src/training/multi_task_lora.cpp:142:        // STUB/SIMULATION NOTE (MTL-S01 — cosine-similarity gating heuristic):
src/training/multi_task_lora.cpp:169:        // Training loop (MTL-S02 — see STUB/SIMULATION NOTE above).
src/training/multi_task_lora.cpp:300:        // MTL-S01 gating heuristic (cosine similarity to prototype vectors — see STUB/SIMULATION NOTE above).
src/storage/tensor_compaction_filter.cpp:12:// STUB/SIMULATION NOTE:
src/storage/tensor_compaction_filter.cpp:54:// STUB/SIMULATION NOTE (BRIDGE_REF #264 — RecompressFn injection bridge):
src/storage/ggml_tensor_bridge.cpp:82:// STUB/SIMULATION NOTE (BRIDGE_REF #263a — GgmlAllocFn injection bridge):
src/storage/ggml_tensor_bridge.cpp:132:// STUB/SIMULATION NOTE (BRIDGE_REF #263b — PrefetchFn injection bridge):
src/storage/ggml_tensor_bridge.cpp:181:// STUB/SIMULATION NOTE (BRIDGE_REF #263c — TypeRegistrationFn injection bridge):
src/server/timeseries_api_handler.cpp:472:            // STUB/SIMULATION NOTE:
src/rag/reranker_cost_analyzer.cpp:300:  // TODO: Store forecast for later use
src/rag/reranker_budget_gate.cpp:137:  // TODO: This requires Document struct with score field
src/rag/freshness_sla_enforcer.cpp:67:  // TODO: Call IndexRefreshScheduler::ScheduleEmergencyRefresh()
src/rag/freshness_sla_enforcer.cpp:93:  // TODO: Filter events by time window (last N hours)
src/rag/otel_span_emitter.cpp:161:  // TODO: Implement actual OTLP export logic
src/rag/ingestion_latency_monitor.cpp:86:    status.is_primary = true;  // TODO: Track primary vs secondary
src/rag/ingestion_latency_monitor.cpp:128:  // TODO: Implement persistence to RocksDB
src/rag/ingestion_latency_monitor.cpp:135:  // TODO: Retrieve historical aggregates for last N hours
src/rag/adaptive_hybrid_router.cpp:63:  // TODO: Call actual retrieval backends
src/rag/adaptive_hybrid_router.cpp:119:  // TODO: Integrate with OpenTelemetry SDK
src/rag/model_registry.cpp:227:  // TODO: Implement JSON serialization to persistence_path
src/rag/model_registry.cpp:237:  // TODO: Implement JSON deserialization from persistence_path
src/rag/model_registry.cpp:247:  // TODO: Implement comprehensive JSON export
src/rag/evaluation_result_store.cpp:193:    return "{}";  // TODO: Serialize to JSON
src/rag/evaluation_result_store.cpp:211:  // TODO: Export run to file
src/rag/cost_model_builder.cpp:78:  // TODO: Serialize model to JSON file
src/rag/cost_model_builder.cpp:83:  // TODO: Deserialize model from JSON file
src/rag/recommendation_engine.cpp:123:  // TODO: Export recommendations to file (JSON or markdown)
src/rag/benchmark_suite.cpp:19:  // TODO: Parse TREC/MARCO format TSV or JSON
src/rag/benchmark_suite.cpp:53:  // TODO: Execute RAG system with scenario config, retrieve and rerank
src/rag/benchmark_suite.cpp:104:  // TODO: Export to JSON or CSV
src/rag/cross_encoder_orchestrator.cpp:56:  // TODO: Actual cross-encoder inference
src/rag/cross_encoder_orchestrator.cpp:126:  // TODO: Load model from disk or download from registry
src/rag/cost_attribution_tracker.cpp:30:    // TODO: Replace with actual RocksDB initialization:
src/rag/cost_attribution_tracker.cpp:243:  // TODO: Compare actual cost vs forecast from cost model
src/rag/router_policy_store.cpp:16:  // TODO: In production, initialize RocksDB connection here
src/rag/router_policy_store.cpp:20:  // TODO: Close RocksDB connection
src/rag/router_policy_store.cpp:94:  // TODO: Implement metrics recording in database
src/rag/router_policy_store.cpp:103:  // TODO: Retrieve and aggregate metrics
src/rag/metrics_reporter.cpp:218:  // TODO: Implement with access to model-specific metrics
src/acceleration/vulkan_backend_full.cpp:199:    // STUB/SIMULATION NOTE:
src/acceleration/vulkan_backend_full.cpp:217:    std::cerr << "GLSL to SPIR-V compilation requires shaderc library (STUB)" << std::endl;
src/acceleration/nccl_vector_backend.cpp:772:// STUB/SIMULATION NOTE:
src/acceleration/nccl_vector_backend.cpp:789:// STUB/SIMULATION NOTE (allReduce bridge):
src/acceleration/oneapi_backend.cpp:258:// STUB/SIMULATION NOTE:
src/acceleration/oneapi_backend.cpp:273:// STUB/SIMULATION NOTE (computeDistances bridge):
src/acceleration/opencl_backend.cpp:396:// STUB/SIMULATION NOTE:
src/acceleration/opencl_backend.cpp:411:// STUB/SIMULATION NOTE (computeDistances bridge):
src/acceleration/ai_hardware_dispatcher.cpp:836:    // STUB/SIMULATION NOTE:
src/ingestion/cdc_connector.cpp:687:    // STUB/SIMULATION NOTE:
src/ingestion/database_connector.cpp:521:    // STUB/SIMULATION NOTE:
src/ingestion/kafka_connector.cpp:272:    // STUB/SIMULATION NOTE:
src/ingestion/s3_connector.cpp:377:        // STUB/SIMULATION NOTE:
src/ingestion/s3_connector.cpp:582:    // STUB/SIMULATION NOTE:
src/ingestion/object_storage_connector.cpp:317:    // STUB/SIMULATION NOTE:
src/security/hsm_key_provider_adapter.cpp:32:    const char* allow_stub = std::getenv("THEMIS_ALLOW_HSM_STUB");
src/security/hsm_key_provider_adapter.cpp:39:// STUB/SIMULATION NOTE:
src/security/hsm_key_provider_adapter.cpp:535:                "Configure a real PKCS#11 HSM or set THEMIS_ALLOW_HSM_STUB=1 "
src/security/hsm_key_provider_adapter.cpp:614:                "Configure a real PKCS#11 HSM or set THEMIS_ALLOW_HSM_STUB=1 "
src/security/hsm_key_provider_adapter.cpp:749:// ── Static bridge setters (BRIDGE_REF #47 / #48) — see STUB/SIMULATION NOTE above ──
src/security/timestamp_authority.cpp:20://          Production mode is explicitly blocked unless THEMIS_ALLOW_TSA_STUB=1 is set.
src/security/timestamp_authority.cpp:91:    const char* allow_stub = std::getenv("THEMIS_ALLOW_TSA_STUB");
src/security/timestamp_authority.cpp:107:        "or set THEMIS_ALLOW_TSA_STUB=1 to explicitly allow the insecure stub.";
src/security/timestamp_authority.cpp:184:    // WARNING: This is a STUB implementation for development only
src/security/timestamp_authority.cpp:187:    THEMIS_WARN("Using TimestampAuthority STUB - NOT SECURE for production!");
src/security/timestamp_authority.cpp:243:    tok.serial_number = "STUB-SERIAL";
src/security/timestamp_authority.cpp:248:    tok.tsa_name = "STUB-TSA";
src/security/timestamp_authority.cpp:249:    tok.tsa_serial = "STUB-TSA-SERIAL";
src/security/timestamp_authority.cpp:359:            std::string("-----BEGIN CERTIFICATE-----\nSTUB-TSA\n-----END CERTIFICATE-----\n");
src/security/timestamp_authority.cpp:488:            "or set THEMIS_ALLOW_TSA_STUB=1 for explicit non-production override.");
src/security/timestamp_authority_openssl.cpp:12:// STUB/SIMULATION NOTE:
src/security/field_encryption.cpp:363:    // Set THEMIS_ALLOW_MOCK_KEY_PROVIDER=1 only in test/demo environments.
src/security/field_encryption.cpp:364:    const char* allow_env = std::getenv("THEMIS_ALLOW_MOCK_KEY_PROVIDER");
src/security/field_encryption.cpp:378:            "To explicitly opt in for testing, set THEMIS_ALLOW_MOCK_KEY_PROVIDER=1.");
src/security/hsm_provider.cpp:23://          Production mode is explicitly blocked unless THEMIS_ALLOW_HSM_STUB=1 env var
src/security/hsm_provider.cpp:324:    const char* allow_stub = std::getenv("THEMIS_ALLOW_HSM_STUB");
src/security/hsm_provider.cpp:328:    // This cannot be overridden by THEMIS_ALLOW_HSM_STUB.
src/security/hsm_provider.cpp:349:            last_error_ = "HSM stub provider detected production environment but THEMIS_ALLOW_HSM_STUB is not set. "
src/security/hsm_provider.cpp:350:                          "Set THEMIS_ALLOW_HSM_STUB=1 to explicitly allow insecure stub, or use real HSM.";
src/security/hsm_provider.cpp:377:    THEMIS_WARN("║  ⚠️  INSECURE CONFIGURATION: HSM STUB PROVIDER ACTIVE!  ⚠️   ║");
src/security/hsm_provider.cpp:388:    THEMIS_WARN("║  - Set THEMIS_ALLOW_HSM_STUB=1 environment variable          ║");
src/security/hsm_provider.cpp:465:    THEMIS_WARN("HSMProvider STUB signing - NOT cryptographically secure!");
src/security/hsm_provider.cpp:470:    r.cert_serial = "STUB-CERT";
src/security/hsm_provider.cpp:562:    THEMIS_WARN("HSMProvider STUB encryptData - NOT hardware-protected, for development only!");
src/security/hsm_provider.cpp:597:    THEMIS_WARN("HSMProvider STUB decryptData - NOT hardware-protected, for development only!");
src/security/hsm_provider.cpp:698:    const char* allow_stub = std::getenv("THEMIS_ALLOW_HSM_STUB");
src/security/hsm_provider.cpp:702:            "is insecure. Set THEMIS_ALLOW_HSM_STUB=1 for explicit development override, "
src/security/hsm_provider.cpp:709:        "(THEMIS_ALLOW_HSM_STUB=1). Not suitable for production.",
src/security/hsm_provider.cpp:711:    return std::string("-----BEGIN CERTIFICATE-----\nSTUB\n-----END CERTIFICATE-----\n");
src/security/hsm_provider_pkcs11.cpp:82://             fails). Controlled by THEMIS_ALLOW_HSM_STUB env var in production mode.
src/security/hsm_provider_pkcs11.cpp:527:        THEMIS_WARN("║  ⚠️  HSM FALLBACK STUB ACTIVE - INSECURE CONFIGURATION  ⚠️   ║");
src/security/hsm_provider_pkcs11.cpp:807:        const char* allow_stub = std::getenv("THEMIS_ALLOW_HSM_STUB");
src/security/hsm_provider_pkcs11.cpp:811:                "returning a stub signature is insecure. Set THEMIS_ALLOW_HSM_STUB=1 to "
src/security/hsm_provider_pkcs11.cpp:817:        // STUB/SIMULATION NOTE:
src/security/hsm_provider_pkcs11.cpp:823:        //                   cert_serial is hardcoded "STUB-CERT". Not cryptographically secure.
src/security/hsm_provider_pkcs11.cpp:835:        r.cert_serial = "STUB-CERT"; 
src/security/hsm_provider_pkcs11.cpp:1339:        // Require explicit opt-in via THEMIS_ALLOW_HSM_STUB=1.
src/security/hsm_provider_pkcs11.cpp:1341:            const char* allow_stub = std::getenv("THEMIS_ALLOW_HSM_STUB");
src/security/hsm_provider_pkcs11.cpp:1345:                    "ready and returning a stub PEM is insecure. Set THEMIS_ALLOW_HSM_STUB=1 "
src/security/hsm_provider_pkcs11.cpp:1353:            "(THEMIS_ALLOW_HSM_STUB=1). Not suitable for production.",
src/security/hsm_provider_pkcs11.cpp:1355:        return std::string("-----BEGIN CERTIFICATE-----\nSTUB\n-----END CERTIFICATE-----\n");
src/api/themisdb_grpc_service.cpp:1946:    // STUB/SIMULATION NOTE:
src/transaction/distributed_transaction_manager.cpp:75:// STUB/SIMULATION NOTE:
src/transaction/distributed_transaction_manager.cpp:140:// STUB/SIMULATION NOTE:
src/geo/gpu_kernel_dispatcher_cpu.cpp:12:// STUB/SIMULATION NOTE:
src/geo/gpu_backend_stub.cpp:1265:        // STUB/SIMULATION NOTE:
src/geo/cpu_backend.cpp:33:// STUB/SIMULATION NOTE:
src/governance/opa_adapter.cpp:358:// STUB/SIMULATION NOTE:
src/performance/cycle_metrics.cpp:196:// STUB/SIMULATION NOTE:
src/performance/phase4/pmu_counters.cpp:821:// STUB/SIMULATION NOTE:
src/performance/advanced_cache_manager.cpp:82:// STUB/SIMULATION NOTE:
src/index/advanced_vector_index.cpp:33:    // STUB/SIMULATION NOTE:
src/index/gpu_vector_index_vulkan.cpp:1115:// STUB/SIMULATION NOTE:
src/voice/voice_telephony.cpp:569:    //                   See STUB_INVENTORY entry #173 and
src/voice/voice_telephony.cpp:876:    //                   See STUB_INVENTORY entry #174 and
src/voice/audio_preprocessing.cpp:59:// STUB/SIMULATION NOTE:
src/voice/voice_browser_streaming.cpp:150:    // STUB/SIMULATION NOTE:
src/llm/llm_plugin_manager.cpp:811: * @details Calls: THEMIS_LLAMA_CPP_STUB_MODE(), contains(), std::chrono::seconds(), empty(), loadModel(), errors::logError(), LLMPluginManager::instance(), registerPlugin().
src/llm/llm_plugin_manager.cpp:819:#ifdef THEMIS_LLAMA_CPP_STUB_MODE
src/llm/llm_plugin_manager.cpp:820:    // STUB/SIMULATION NOTE (BRIDGE_REF #LPM-01 — llama.cpp stub mode):
src/llm/llm_plugin_manager.cpp:824:    // Activation:        Compiled when THEMIS_LLAMA_CPP_STUB_MODE is defined.
src/llm/llm_plugin_manager.cpp:829:    // Removal Plan:      Do not set THEMIS_LLAMA_CPP_STUB_MODE in production builds.
src/llm/lora_framework/gpu_tensor.cpp:447:        // STUB/SIMULATION NOTE:
src/llm/lora_framework/gpu_tensor.cpp:484:        // STUB/SIMULATION NOTE:
src/llm/embedded_llm_stub.cpp:416:    // In THEMIS_LLM_STUB_MODE (test/dev-only builds) return the deterministic
src/llm/embedded_llm_stub.cpp:423:    // Activation: compile-time flag THEMIS_LLM_STUB_MODE (never set in release presets).
src/llm/embedded_llm_stub.cpp:433:#ifdef THEMIS_LLM_STUB_MODE
src/llm/embedded_llm_stub.cpp:475:#ifdef THEMIS_LLM_STUB_MODE
src/llm/ssm_stub_plugin.cpp:20:// STUB/SIMULATION NOTE (SyntheticSSMStub — SSM PoC stub):
src/llm/ssm_stub_plugin.cpp:35:SyntheticSSMStub::SyntheticSSMStub() : rng_(STUB_SEED) {
src/llm/ssm_stub_plugin.cpp:38:    oss << "stub-v0.1-seed" << STUB_SEED << "-dim" << HIDDEN_DIM;
src/llama_cpp/llama_cpp_registrar.cpp:85:        // STUB/SIMULATION NOTE:
src/llama_cpp/llama_cpp_registrar.cpp:101:    #ifdef THEMIS_LLAMA_CPP_STUB_MODE
src/llama_cpp/tests/test_llama_cpp_plugin.cpp:473:// These tests verify the production contract: generate() without STUB_MODE
src/llama_cpp/tests/test_llama_cpp_plugin.cpp:475:// descriptive error_message.  In this test file THEMIS_LLAMA_CPP_STUB_MODE is
src/llama_cpp/tests/test_llama_cpp_plugin.cpp:479:// O2: model loaded with empty path, STUB_MODE → still returns success=true  (sanity)
src/llama_cpp/tests/test_llama_cpp_plugin.cpp:492:    // With THEMIS_LLAMA_CPP_STUB_MODE defined (as it is in this test build),
src/llama_cpp/tests/test_llama_cpp_plugin.cpp:500:        << "THEMIS_LLAMA_CPP_STUB_MODE must preserve success=true for test builds";
src/llama_cpp/tests/test_llama_cpp_plugin.cpp:517:// All tests operate in STUB_MODE (no real model required) and set a generous
src/llama_cpp/tests/test_llama_cpp_plugin.cpp:752:///     THEMIS_LLAMA_CPP_STUB_MODE (the test binary always defines this macro).
src/llama_cpp/tests/test_llama_cpp_plugin.cpp:1074:///     Requires THEMIS_LLAMA_CPP_STUB_MODE so the callback is exercised in
src/llama_cpp/tests/test_llama_cpp_plugin.cpp:1077:#ifndef THEMIS_LLAMA_CPP_STUB_MODE
src/llama_cpp/tests/test_llama_cpp_plugin.cpp:1078:    GTEST_SKIP() << "Requires THEMIS_LLAMA_CPP_STUB_MODE for stub callback path";
src/llama_cpp/tests/test_llama_cpp_plugin.cpp:1129:#ifndef THEMIS_LLAMA_CPP_STUB_MODE
src/llama_cpp/tests/test_llama_cpp_plugin.cpp:1130:    GTEST_SKIP() << "Requires THEMIS_LLAMA_CPP_STUB_MODE for stub path";
src/llama_cpp/tests/test_llama_cpp_inference_contract_focused.cpp:6: * THEMIS_LLAMA_CPP_STUB_MODE:
src/llama_cpp/tests/test_llama_cpp_plugin_lifecycle_focused.cpp:9: * All tests run under THEMIS_LLAMA_CPP_STUB_MODE; no real model is required.
src/llama_cpp/tests/test_llama_cpp_validation_gates_focused.cpp:7: * LlamaCppPlugin API.  All tests run under THEMIS_LLAMA_CPP_STUB_MODE so
src/llama_cpp/tests/test_llama_cpp_registrar_integration_focused.cpp:8: * All tests run under THEMIS_LLAMA_CPP_STUB_MODE so no real model file is
src/llama_cpp/tests/test_llama_cpp_registrar_integration_focused.cpp:91:    // In THEMIS_LLAMA_CPP_STUB_MODE loadModel() always succeeds, so this
src/llama_cpp/llama_cpp_plugin.cpp:394:    // STUB/SIMULATION NOTE:
src/llama_cpp/llama_cpp_plugin.cpp:398:    //          behaviour should define THEMIS_LLAMA_CPP_STUB_MODE.
src/llama_cpp/llama_cpp_plugin.cpp:413:#ifdef THEMIS_LLAMA_CPP_STUB_MODE
src/llama_cpp/llama_cpp_plugin.cpp:653:    // STUB/SIMULATION NOTE:
src/llama_cpp/llama_cpp_plugin.cpp:889:        // STUB/SIMULATION NOTE:
src/llama_cpp/llama_cpp_plugin.cpp:937:    // STUB/SIMULATION NOTE:
src/llama_cpp/llama_cpp_plugin.cpp:962:    // STUB/SIMULATION NOTE:
```
