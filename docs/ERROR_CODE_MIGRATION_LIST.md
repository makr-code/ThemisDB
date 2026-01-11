# Error Code Migration List

**Erstellt:** 11. Januar 2026  
**Status:** Migration Pending  
**Zweck:** Liste aller bestehenden Error-Logging-Stellen, die auf das neue Error Code Management umgestellt werden müssen

---

## 📊 Zusammenfassung

| Kategorie | Anzahl Stellen | Priorität | Status |
|-----------|----------------|-----------|--------|
| **LLM Errors (2000-2099)** | ~21 | HOCH | ⏳ Ausstehend |
| **LoRA Errors (2100-2199)** | ~15 | HOCH | ⏳ Ausstehend |
| **GPU/Memory Errors** | ~14 | KRITISCH | ⏳ Ausstehend |
| **MCP Errors (3000-3999)** | ~3 | MITTEL | ⏳ Ausstehend |
| **Storage Errors (1000-1999)** | 0 | NIEDRIG | ✅ Keine gefunden |
| **Network Errors (5000-5999)** | 0 | NIEDRIG | ✅ Keine gefunden |
| **Schema Errors (4000-4999)** | 0 | NIEDRIG | ✅ Keine gefunden |
| **Gesamt** | **~53** | | |

**Hinweis:** Es gibt insgesamt 117 `spdlog::error` Aufrufe, 111 `spdlog::warn` Aufrufe und 1 `spdlog::critical` Aufruf im gesamten Codebase. Diese Liste konzentriert sich auf die Fehler, die den definierten Error Code Kategorien entsprechen.

---

## 🔴 Category: LLM Errors (2000-2099)

### ✅ ERR_LLM_MODEL_NOT_FOUND (2000) - BEREITS DEFINIERT

**Migration erforderlich:**

| Datei | Zeile | Aktueller Code | Ziel Error Code |
|-------|-------|----------------|-----------------|
| `src/llm/model_loader.cpp` | 307 | `spdlog::error("Model file not found: {}", model_path);` | `errors::logError(ErrorCode::ERR_LLM_MODEL_NOT_FOUND, model_path);` |

### ✅ ERR_LLM_MODEL_LOAD_FAILED (2001) - BEREITS DEFINIERT

**Migration erforderlich:**

| Datei | Zeile | Aktueller Code | Ziel Error Code |
|-------|-------|----------------|-----------------|
| `src/llm/model_loader.cpp` | 341 | `spdlog::error("Failed to load model from file: {}", model_path);` | `errors::logError(ErrorCode::ERR_LLM_MODEL_LOAD_FAILED, model_path);` |
| `src/llm/llama_wrapper.cpp` | 292 | `spdlog::error("Failed to load model: {}", model_path);` | `errors::logError(ErrorCode::ERR_LLM_MODEL_LOAD_FAILED, model_path);` |
| `src/llm/embedded_llm.cpp` | 41 | `spdlog::error("Failed to load model: {}", config.model_path);` | `errors::logError(ErrorCode::ERR_LLM_MODEL_LOAD_FAILED, config.model_path);` |
| `src/llm/llm_plugin_manager.cpp` | 426 | `spdlog::error("Failed to load model: {}", model_path);` | `errors::logError(ErrorCode::ERR_LLM_MODEL_LOAD_FAILED, model_path);` |

### ✅ ERR_LLM_CONTEXT_CREATION_FAILED (2002) - BEREITS DEFINIERT

**Migration erforderlich:**

| Datei | Zeile | Aktueller Code | Ziel Error Code |
|-------|-------|----------------|-----------------|
| `src/llm/model_loader.cpp` | 415 | `spdlog::error("Failed to create context for model: {}", model_id);` | `errors::logError(ErrorCode::ERR_LLM_CONTEXT_CREATION_FAILED, model_id);` |
| `src/llm/llama_wrapper.cpp` | 1391 | `spdlog::error("Failed to create context for draft model");` | `errors::logError(ErrorCode::ERR_LLM_CONTEXT_CREATION_FAILED, "draft model");` |

### ❌ ERR_LLM_INFERENCE_TIMEOUT (2003) - NOCH ZU DEFINIEREN

**Potenzielle Kandidaten:**
- Keine direkt gefunden, aber möglicherweise in anderen Bereichen vorhanden

### ✅ ERR_LLM_GPU_OOM (2004) - BEREITS DEFINIERT

**Migration erforderlich:**

| Datei | Zeile | Aktueller Code | Ziel Error Code |
|-------|-------|----------------|-----------------|
| `src/llm/gpu_memory_manager.cpp` | 229 | `spdlog::error("Cannot allocate {} bytes VRAM for model {}: insufficient memory", ...)` | `errors::logError(ErrorCode::ERR_LLM_GPU_OOM, bytes, available_memory);` |
| `src/llm/gpu_memory_manager.cpp` | 968 | `spdlog::error("Cannot allocate {} bytes on GPU {}: insufficient memory (used: {}, max: {})", ...)` | `errors::logError(ErrorCode::ERR_LLM_GPU_OOM, bytes, available_memory);` |

### ❌ NEUE ERROR CODES ERFORDERLICH

**Weitere LLM-spezifische Fehler ohne passenden Error Code:**

| Datei | Zeile | Beschreibung | Vorgeschlagener Code |
|-------|-------|--------------|---------------------|
| `src/llm/llama_wrapper.cpp` | 476-477 | Model/context handle is null | `ERR_LLM_INVALID_HANDLE` (2005) |
| `src/llm/llama_wrapper.cpp` | 2154 | Vision inference error | `ERR_LLM_VISION_INFERENCE_FAILED` (2006) |
| `src/llm/llama_wrapper.cpp` | 1344 | Failed to load draft model | `ERR_LLM_DRAFT_MODEL_LOAD_FAILED` (2007) |
| `src/llm/multi_lora_manager.cpp` | 370 | Cannot fuse LoRAs from different base models | `ERR_LORA_MODEL_MISMATCH` (2105) |

---

## 🟡 Category: LoRA Errors (2100-2199)

### ✅ ERR_LORA_NOT_LOADED (2100) - BEREITS DEFINIERT

**Migration erforderlich:**

| Datei | Zeile | Aktueller Code | Ziel Error Code |
|-------|-------|----------------|-----------------|
| `src/llm/multi_lora_manager.cpp` | 206 | `spdlog::error("LoRA not loaded: {}", lora_id);` | `errors::logError(ErrorCode::ERR_LORA_NOT_LOADED, lora_id);` |
| `src/llm/multi_lora_manager.cpp` | 359 | `spdlog::error("LoRA {} not loaded", lora_ids[i]);` | `errors::logError(ErrorCode::ERR_LORA_NOT_LOADED, lora_ids[i]);` |
| `src/llm/multi_lora_manager.cpp` | 655 | `spdlog::error("Cannot export LoRA: {} not loaded", lora_id);` | `errors::logError(ErrorCode::ERR_LORA_NOT_LOADED, lora_id);` |
| `src/llm/llama_wrapper.cpp` | 378 | `spdlog::error("Cannot load LoRA: no model loaded");` | `errors::logError(ErrorCode::ERR_LORA_NOT_LOADED, "no model");` |
| `src/llm/llama_wrapper.cpp` | 853 | `spdlog::error("Cannot import LoRA: no model loaded");` | `errors::logError(ErrorCode::ERR_LORA_NOT_LOADED, "no model");` |

### ✅ ERR_LORA_BATCHING_DISABLED (2101) - BEREITS DEFINIERT

**Migration erforderlich:**

| Datei | Zeile | Aktueller Code | Ziel Error Code |
|-------|-------|----------------|-----------------|
| `src/llm/multi_lora_manager.cpp` | 257 | `spdlog::error("Multi-LoRA batching is disabled");` | `errors::logError(ErrorCode::ERR_LORA_BATCHING_DISABLED);` |

### ✅ ERR_LORA_WEIGHT_MISMATCH (2102) - BEREITS DEFINIERT

**Migration erforderlich:**

| Datei | Zeile | Aktueller Code | Ziel Error Code |
|-------|-------|----------------|-----------------|
| `src/llm/multi_lora_manager.cpp` | 341 | `spdlog::error("Number of LoRAs ({}) doesn't match number of weights ({})", ...)` | `errors::logError(ErrorCode::ERR_LORA_WEIGHT_MISMATCH, num_loras, num_weights);` |

### ✅ ERR_LORA_FUSION_FAILED (2103) - BEREITS DEFINIERT

**Migration erforderlich:**

| Datei | Zeile | Aktueller Code | Ziel Error Code |
|-------|-------|----------------|-----------------|
| `src/llm/multi_lora_manager.cpp` | 336 | `spdlog::error("No LoRAs provided for fusion");` | `errors::logError(ErrorCode::ERR_LORA_FUSION_FAILED, "no LoRAs provided");` |

### ✅ ERR_LORA_INVALID_DATA (2104) - BEREITS DEFINIERT

**Migration erforderlich:**

| Datei | Zeile | Aktueller Code | Ziel Error Code |
|-------|-------|----------------|-----------------|
| `src/llm/multi_lora_manager.cpp` | 707 | `spdlog::error("Empty LoRA data");` | `errors::logError(ErrorCode::ERR_LORA_INVALID_DATA, "empty data");` |
| `src/llm/multi_lora_manager.cpp` | 718 | `spdlog::error("Invalid LoRA data: too small");` | `errors::logError(ErrorCode::ERR_LORA_INVALID_DATA, "too small");` |
| `src/llm/multi_lora_manager.cpp` | 726 | `spdlog::error("Invalid LoRA data: invalid id_len");` | `errors::logError(ErrorCode::ERR_LORA_INVALID_DATA, "invalid id_len");` |
| `src/llm/multi_lora_manager.cpp` | 737 | `spdlog::error("Invalid LoRA data: invalid path_len");` | `errors::logError(ErrorCode::ERR_LORA_INVALID_DATA, "invalid path_len");` |

### ❌ NEUE ERROR CODES ERFORDERLICH

| Datei | Zeile | Beschreibung | Vorgeschlagener Code |
|-------|-------|--------------|---------------------|
| `src/llm/multi_lora_manager.cpp` | 1393 | Failed to load LoRA on multiple GPUs | `ERR_LORA_GPU_LOAD_FAILED` (2106) |
| `src/llm/multi_lora_manager.cpp` | 1400 | Failed to load LoRA on specific GPU | `ERR_LORA_GPU_LOAD_FAILED` (2106) |

---

## 🔵 Category: MCP Errors (3000-3999)

### ✅ ERR_MCP_TRANSPORT_FAILED (3000) - BEREITS DEFINIERT

**Migration erforderlich:**

| Datei | Zeile | Aktueller Code | Ziel Error Code |
|-------|-------|----------------|-----------------|
| `src/server/mcp_server.cpp` | 193 | `spdlog::error("Error handling MCP request: {}", e.what());` | `errors::logError(ErrorCode::ERR_MCP_TRANSPORT_FAILED, e.what());` |
| `src/server/mcp_server.cpp` | 1656 | `spdlog::error("Error handling WebSocket message from session {}: {}", session_id, e.what());` | `errors::logError(ErrorCode::ERR_MCP_TRANSPORT_FAILED, e.what());` |

### ❌ NEUE ERROR CODES ERFORDERLICH

| Datei | Zeile | Beschreibung | Vorgeschlagener Code |
|-------|-------|--------------|---------------------|
| `src/server/mcp_server.cpp` | 1312 | Failed to get stdin handle | `ERR_MCP_STDIO_INIT_FAILED` (3004) |

---

## 🟢 Category: GPU/Memory Errors (Cross-Category)

**Hinweis:** Diese Fehler betreffen mehrere Kategorien (hauptsächlich LLM und LoRA)

### Speicher-Allokierungsfehler

| Datei | Zeile | Beschreibung | Zuordnung |
|-------|-------|--------------|-----------|
| `src/llm/gpu_memory_manager.cpp` | 229 | Cannot allocate VRAM for model | `ERR_LLM_GPU_OOM` (2004) |
| `src/llm/gpu_memory_manager.cpp` | 248 | Failed to allocate (simulation) | `ERR_LLM_GPU_OOM` (2004) |
| `src/llm/gpu_memory_manager.cpp` | 256 | Failed to allocate (simulation) | `ERR_LLM_GPU_OOM` (2004) |
| `src/llm/gpu_memory_manager.cpp` | 282 | Cannot allocate RAM for model | Neu: `ERR_LLM_RAM_OOM` (2008) |
| `src/llm/gpu_memory_manager.cpp` | 310 | Failed to allocate RAM | Neu: `ERR_LLM_RAM_OOM` (2008) |
| `src/llm/gpu_memory_manager.cpp` | 968 | Cannot allocate on GPU | `ERR_LLM_GPU_OOM` (2004) |
| `src/llm/gpu_memory_manager.cpp` | 990 | Failed to allocate on GPU | `ERR_LLM_GPU_OOM` (2004) |
| `src/llm/gpu_memory_manager.cpp` | 999 | Failed to allocate on GPU | `ERR_LLM_GPU_OOM` (2004) |

### GPU-Verfügbarkeit und Konfiguration

| Datei | Zeile | Beschreibung | Vorgeschlagener Code |
|-------|-------|--------------|---------------------|
| `src/llm/gpu_memory_manager.cpp` | 961 | GPU is not available | `ERR_LLM_GPU_NOT_AVAILABLE` (2009) |
| `src/llm/gpu_memory_manager.cpp` | 983 | cudaMalloc failed | `ERR_LLM_GPU_ALLOC_FAILED` (2010) |
| `src/llm/gpu_memory_manager.cpp` | 1128 | Cannot enable peer access | `ERR_LLM_GPU_PEER_ACCESS_FAILED` (2011) |
| `src/llm/gpu_memory_manager.cpp` | 1147 | Failed to enable peer access | `ERR_LLM_GPU_PEER_ACCESS_FAILED` (2011) |

---

## 📋 Migration-Prioritäten

### 🔴 KRITISCH (Sofort)
1. **GPU OOM Errors** - Häufigste Fehlerquelle bei LLM-Operationen
2. **Model Load Failed** - Blockiert grundlegende Funktionalität
3. **LoRA Not Loaded** - Wichtig für Multi-LoRA Features

### 🟡 HOCH (Kurzfristig)
4. **Context Creation Failed** - Beeinträchtigt Inferenz
5. **LoRA Weight Mismatch** - Verhindert korrekte Fusion
6. **LoRA Invalid Data** - Datenkonsistenz

### 🟢 MITTEL (Mittelfristig)
7. **MCP Transport Errors** - Wichtig für MCP-Funktionalität
8. **Alle verbleibenden LLM/LoRA Errors**

### 🔵 NIEDRIG (Langfristig)
9. **Neue Error Codes definieren** - Für aktuell nicht abgedeckte Fehler
10. **Andere Kategorien** - Storage, Network, Schema (falls gefunden)

---

## 🔧 Migrations-Workflow

### Schritt 1: Error Code Definition erweitern

Für die neu identifizierten Fehler müssen folgende Error Codes in `include/utils/error_registry.h` hinzugefügt werden:

```cpp
// Additional LLM Errors
ERR_LLM_INVALID_HANDLE = 2005,
ERR_LLM_VISION_INFERENCE_FAILED = 2006,
ERR_LLM_DRAFT_MODEL_LOAD_FAILED = 2007,
ERR_LLM_RAM_OOM = 2008,
ERR_LLM_GPU_NOT_AVAILABLE = 2009,
ERR_LLM_GPU_ALLOC_FAILED = 2010,
ERR_LLM_GPU_PEER_ACCESS_FAILED = 2011,

// Additional LoRA Errors
ERR_LORA_MODEL_MISMATCH = 2105,
ERR_LORA_GPU_LOAD_FAILED = 2106,

// Additional MCP Errors
ERR_MCP_STDIO_INIT_FAILED = 3004,
```

### Schritt 2: Metadata registrieren

In `src/utils/error_registry.cpp` im `registerDefaultErrors()` die Metadata für neue Codes hinzufügen.

### Schritt 3: Code Migration durchführen

**Beispiel:**

**Vorher:**
```cpp
spdlog::error("Model file not found: {}", model_path);
```

**Nachher:**
```cpp
#include "utils/error_registry.h"
// ...
errors::logError(errors::ErrorCode::ERR_LLM_MODEL_NOT_FOUND, model_path);
```

### Schritt 4: Tests aktualisieren

Sicherstellen, dass Tests die neuen Error Codes erwarten und testen.

---

## 📊 Statistik nach Migration

**Geschätzte Reduzierung:**
- 53+ spezifische Fehler werden strukturiert erfasst
- Konsistente Fehlerbehandlung über alle Module
- Verbesserte Fehlersuche und Diagnose
- AI-gestützte Fehlererklärungen für alle migrierten Fehler

---

## 📝 Notizen

1. **Backward Compatibility:** Während der Migration sollten beide Logging-Methoden parallel unterstützt werden
2. **Testing:** Jede Kategorie sollte nach Migration getestet werden
3. **Documentation:** User-facing Dokumentation sollte aktualisiert werden, wenn Fehlercodes sichtbar werden
4. **Monitoring:** Nach Migration sollten Error Code Metriken erfasst werden

---

**Erstellt von:** @copilot  
**Letzte Aktualisierung:** 11. Januar 2026  
**Version:** 1.0
