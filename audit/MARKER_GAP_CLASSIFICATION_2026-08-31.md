# Marker-Klassifikation 2026-08-31 (Reale Gaps vs Doku-Leaks)

- Quelle: `audit/MARKER_LOCATIONS_2026-08-31.md`
- Methode: Jede Fundstelle wurde gegen den realen Quelltext an der gemeldeten Zeile verifiziert.
- Klassifikation:
  - **Doku-Leak** = Marker stammt nur aus auto-generierter Doxygen-Metadatenzeile (`@note Gap Summary: ... TODO=..., Stub=..., Mock=...`)
  - **Realer Gap** = Marker steht im fachlichen/technischen Code-Kommentar oder Codepfad

## Gesamtbild

- Reale Gaps: **257**
- Doku-Leaks: **1473**
- Gesamt geprüft: **1730**

## Modul-Liste

| Modul | Reale Gaps | Doku-Leaks |
|---|---:|---:|
| `security` | 41 | 46 |
| `chimera` | 38 | 1 |
| `llama_cpp` | 26 | 3 |
| `tensor` | 25 | 14 |
| `analytics` | 18 | 26 |
| `llm` | 18 | 166 |
| `acceleration` | 16 | 26 |
| `storage` | 12 | 61 |
| `server` | 7 | 121 |
| `ingestion` | 6 | 34 |
| `geo` | 5 | 21 |
| `rag` | 5 | 65 |
| `training` | 4 | 16 |
| `transaction` | 4 | 17 |
| `voice` | 4 | 19 |
| `governance` | 3 | 25 |
| `performance` | 3 | 31 |
| `cache` | 2 | 12 |
| `ethics_ai` | 2 | 27 |
| `index` | 2 | 41 |
| `onnx_clip` | 2 | 2 |
| `plugins` | 2 | 10 |
| `themis` | 2 | 11 |
| `utils` | 2 | 44 |
| `api` | 1 | 9 |
| `cdc` | 1 | 12 |
| `gpu` | 1 | 30 |
| `graph` | 1 | 14 |
| `network` | 1 | 24 |
| `observability` | 1 | 22 |
| `process` | 1 | 17 |
| `ai` | 0 | 2 |
| `aql` | 0 | 22 |
| `auth` | 0 | 35 |
| `base` | 0 | 8 |
| `chaos` | 0 | 1 |
| `config` | 0 | 6 |
| `content` | 0 | 42 |
| `core` | 0 | 12 |
| `distributed_knowledge` | 0 | 5 |
| `document` | 0 | 1 |
| `exporters` | 0 | 16 |
| `failover` | 0 | 2 |
| `importers` | 0 | 32 |
| `maintenance` | 0 | 2 |
| `metadata` | 0 | 12 |
| `projects` | 0 | 7 |
| `prompt_engineering` | 0 | 32 |
| `query` | 0 | 53 |
| `replication` | 0 | 10 |
| `rpc_grpc` | 0 | 3 |
| `scheduler` | 0 | 9 |
| `scraper` | 0 | 8 |
| `search` | 0 | 19 |
| `sharding` | 0 | 86 |
| `stable_diffusion` | 0 | 6 |
| `temporal` | 0 | 15 |
| `timeseries` | 0 | 18 |
| `toolbox` | 0 | 11 |
| `updates` | 0 | 21 |
| `user_storage_encrypted` | 0 | 4 |
| `whisper` | 0 | 1 |

## Verifizierte Beispiele

### `security`
- Reale Gaps: **41**
- Doku-Leaks: **46**
- Real-Beispiel: `GAP-0102` → `src/security/hsm_key_provider_adapter.cpp:28`
  - `const char* allow_stub = std::getenv("THEMIS_ALLOW_HSM_STUB");`
- Doku-Leak-Beispiel: `GAP-1213` → `src/security/access_control.cpp:7`
  - `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=1, M=3, L=0`

### `chimera`
- Reale Gaps: **38**
- Doku-Leaks: **1**
- Real-Beispiel: `GAP-0364` → `src/chimera/mongodb_adapter.cpp:67`
  - `// TODO: Actual mongocxx client creation`
- Doku-Leak-Beispiel: `GAP-0402` → `src/chimera/themisdb_adapter.cpp:7`
  - `* @note Gap Summary: total=19; TODO=1, Stub=6, Unimpl=0, Mock=1, Sim=11, Debt=0, C=6, H=18, M=28, L=0`

### `llama_cpp`
- Reale Gaps: **26**
- Doku-Leaks: **3**
- Real-Beispiel: `GAP-0050` → `src/llama_cpp/llama_cpp_plugin.cpp:347`
  - `// STUB/SIMULATION NOTE:`
- Doku-Leak-Beispiel: `GAP-0715` → `src/llama_cpp/llama_cpp_plugin.cpp:7`
  - `* @note Gap Summary: total=19; TODO=1, Stub=15, Unimpl=0, Mock=1, Sim=2, Debt=0, C=4, H=11, M=11, L=0`

### `tensor`
- Reale Gaps: **25**
- Doku-Leaks: **14**
- Real-Beispiel: `GAP-1551` → `src/tensor/compression_strategy.cpp:40`
  - `// TODO: Wire to actual TensorTrainDecomposer`
- Doku-Leak-Beispiel: `GAP-1550` → `src/tensor/adapter_repository.cpp:7`
  - `* @note Gap Summary: total=19; TODO=1, Stub=13, Unimpl=0, Mock=1, Sim=4, Debt=0, C=0, H=6, M=3, L=0`

### `analytics`
- Reale Gaps: **18**
- Doku-Leaks: **26**
- Real-Beispiel: `GAP-0253` → `src/analytics/streaming_window.cpp:50`
  - `* Open TODOs (tracked here per code-review requirements; see also`
- Doku-Leak-Beispiel: `GAP-0229` → `src/analytics/analytics_export.cpp:7`
  - `* @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=5, L=0`

### `llm`
- Reale Gaps: **18**
- Doku-Leaks: **166**
- Real-Beispiel: `GAP-0810` → `src/llm/ssm_state_rocksdb_store.cpp:261`
  - `// TODO: Use protobuf or binary serialization for efficiency`
- Doku-Leak-Beispiel: `GAP-0718` → `src/llm/active_vram_allocator.cpp:7`
  - `* @note Gap Summary: total=5; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=2, Debt=0, C=0, H=13, M=5, L=0`

### `acceleration`
- Reale Gaps: **16**
- Doku-Leaks: **26**
- Real-Beispiel: `GAP-0200` → `src/acceleration/break_even_validator.cc:184`
  - `// Export metrics (TODO: integrate with Prometheus)`
- Doku-Leak-Beispiel: `GAP-0198` → `src/acceleration/ai_hardware_dispatcher.cpp:7`
  - `* @note Gap Summary: total=7; TODO=1, Stub=4, Unimpl=0, Mock=1, Sim=1, Debt=0, C=28, H=49, M=2, L=0`

### `storage`
- Reale Gaps: **12**
- Doku-Leaks: **61**
- Real-Beispiel: `GAP-0145` → `src/storage/backup_manager.cpp:1613`
  - `THEMIS_WARN("BackupManager::decompressPath: STUB — files copied without decompression "`
- Doku-Leak-Beispiel: `GAP-1474` → `src/storage/adaptive_compaction.cpp:7`
  - `* @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=4, M=0, L=0`

