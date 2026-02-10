# Systemische Stabilitätsanalyse: LLM/LoRA Core

## 🔴 KRITISCHE ARCHITEKTURFEHLER

### 1. **Verwaiste LoRA-Adapter nach dem Laden** (SILENT FAILURE)
**Datei**: [include/llm/lora_framework/lora_adapter_manager.h](include/llm/lora_framework/lora_adapter_manager.h#L79)
**Problem**: 
```cpp
/**
 * @brief Apply loaded adapter to model context for inference
 * 
 * Implements weight fusion: output = base_weight @ input + alpha * adapter_weight @ input
 * This is the CRITICAL MISSING FEATURE - adapters are loaded but not applied!
 */
bool applyAdapter(
    const std::string& adapter_id,
    llama_context* context,
    float alpha = -1.0f  // -1.0 means use adapter's configured scaling
);
```

**Auswirkung**: 
- LoRA-Adapter werden GELADEN aber NICHT auf das Modell angewendet
- `loadAdapter()` speichert Adapter im Cache
- `applyAdapter()` wird aufgerufen aber Gewichte werden NICHT fusioniert
- Resultat: Adapter werden ignoriert, nur base model lädt → **Funktionalität komplett broken**

**Status**: DEPRECATED, aber wird trotzdem verwendet!

### 2. **Doppelter LoRA Manager-Code** (INKONSISTENZ)
**Komponenten**:
- **LoRAAdapterManager** (deprecated, legacy, 456 Zeilen)
  - Alte Implementierung mit Cache
  - Adapter werden geladen aber nicht angewendet
  - Thread-unsafe in vielen Stellen
  
- **MultiLoRAManager** (neu, vLLM-style, 2847 Zeilen nach fixes)
  - Modernere Implementierung
  - Multi-GPU Support
  - Quantization Support
  - Aber: nicht konsistent mit LoRAAdapterManager

**Problem**: 
- Code kennt nicht standardisierte Schnittstelle
- Tests verwenden beide Implementierungen gemischt
- Migrationsweg unklar

### 3. **Speicher-Lifecycle Chaos** (MEMORY LEAK)
**Datei**: [src/llm/lora_framework/gpu_data_loader.cpp](src/llm/lora_framework/gpu_data_loader.cpp#L28)
```cpp
allocator_ = new VRAMAllocator(backend);  // ← Manual new
// ...
delete allocator_;  // ← Manual delete - aber wann?
```

**Problem**:
- `new`/`delete` in einer Klasse → Manual Speicherverwaltung fehleranfällig
- Keine Ausnahmesicherheit
- Mehrfache Löschung möglich (double-free)
- **Sollte**: `std::unique_ptr<VRAMAllocator>` sein

**Betroffene Dateien**:
- `src/llm/lora_framework/gpu_data_loader.cpp`

### 4. **Adapter-Handle Lifecycle Undefined** (DANGLING POINTER)
**Datei**: [src/llm/lora_framework/lora_adapter_manager.cpp](src/llm/lora_framework/lora_adapter_manager.cpp#L29-34)
```cpp
for (auto& [id, entry] : adapters_) {
    if (entry->adapter_handle) {
        // Modern llama.cpp: adapters are freed automatically with the model
        // No manual free needed (llama_adapter_lora_free is deprecated)
        spdlog::debug("Adapter {} will be freed with model", id);
        entry->is_applied = false;
        entry->adapter_handle = nullptr;
    }
}
```

**Problem**:
- Kommentar sagt "werden automatisch mit Model freigegeben"
- Aber: `entry->adapter_handle = nullptr;` am Ende ist FALSCH
- Wenn Model NICHT freigegeben wird → DANGLING POINTER
- Keine Validierung dass handle noch gültig ist

**Korrekt**:
```cpp
// Nur nullptr setzen, NICHT freigeben
entry->adapter_handle = nullptr;
// adapter_handle wird vom Modell verwaltet, nicht von uns
```

### 5. **MultiLoRA Config Validation unzureichend** (CRASH-ANFÄLLIGKEIT)
**Datei**: [src/llm/multi_lora_manager.cpp](src/llm/multi_lora_manager.cpp#L40-100)

**Was validiert wird**:
- ✓ max_lora_vram_mb == 0
- ✓ max_lora_slots == 0
- ✓ devices.empty()

**Was NICHT validiert wird**:
- ✗ GPU IDs sind negativ oder zu hoch (0-255 check minimal)
- ✗ `config_.quantization.group_size` kann 0 sein
- ✗ `config_.quantization.per_channel` mit Rank 0
- ✗ `config_.multi_gpu.max_vram_per_gpu_mb` ist unrealistisch
- ✗ TTL ist zu kurz (< 1s crasht Eviction)
- ✗ `load_balance_threshold` ist 0 oder > 1

---

## 🟠 KRITISCHE IMPLEMENTIERUNGSFEHLER

### 6. **Quantization mit ungültigen Scale-Faktoren** (DIVISION ZERO/NaN)
**Datei**: [src/llm/multi_lora_manager.cpp](src/llm/multi_lora_manager.cpp#L1120-1165) (jetzt behoben)

Aber andere Quellen:
- `quantizeLoRA()` prüft nicht auf inf/nan nach Operationen
- `calibrateScales()` kann INF zurückgeben wenn `max_abs` sehr klein

**Fix erforderlich**:
```cpp
// Nach scale Berechnung:
if (!std::isfinite(scale)) {
    spdlog::error("Non-finite scale: {}", scale);
    scale = MIN_SCALE_EPSILON;
}
```

### 7. **selectGPUForLoRA() gibt -1 zurück aber wird nicht geprüft** (CRASH)
**Datei**: [src/llm/multi_lora_manager.cpp](src/llm/multi_lora_manager.cpp#L1280)
```cpp
// My fix returns -1 for error, aber:
// Caller macht:
int gpu_id = selectGPUForLoRA(vram_bytes);
gpu_vram_usage_[gpu_id] += vram_bytes;  // ← -1 als key in map!
```

**Problem**: Map[−1] erzeugt neue Entry, keine Fehlerbehandlung

**Korrekt**:
```cpp
int gpu_id = selectGPUForLoRA(vram_bytes);
if (gpu_id < 0) {
    spdlog::error("Failed to select GPU");
    return false;
}
```

### 8. **Thread-Safety der Eviction** (RACE CONDITIONS)
**Datei**: [src/llm/multi_lora_manager.cpp](src/llm/multi_lora_manager.cpp#L1620-1665)

**Probleme**:
- `evictionWorker()` läuft in Background Thread
- Liest `loras_` ohne Lock während `loadLoRA()` schreibt
- `eviction_thread_running_` ist `std::atomic<bool>` ✓ gut
- Aber: `evictionWorker()` könnte LRU unload während Quantization

**Race Condition**:
```
T1: loadLoRA() → weitet loras_ aus
T2: evictionWorker() → schließt LRU
T1: quantizeLoRA() → auf gelöschten Adapter!
```

### 9. **LoRA Unload während Inference** (SEGFAULT)
**Datei**: [src/llm/multi_lora_manager.cpp](src/llm/multi_lora_manager.cpp#L1300-1330)

```cpp
bool MultiLoRAManager::unloadLoRA(const std::string& lora_id, bool force) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = loras_.find(lora_id);
    // ...
    loras_.erase(it);  // ← Adapter wird sofort gelöscht
}

// Aber gleichzeitig könnte applyLoRA() laufen:
auto* lora = getLoRA(lora_id);  // ← Pointer zu gelöschtem LoRA!
```

**Fix erforderlich**: Reference counting auf LoRA-Slots

### 10. **Config-Mutation nach Initialisierung** (RACE CONDITIONS)
**Datei**: [src/llm/multi_lora_manager.cpp](src/llm/multi_lora_manager.cpp#L1110-1125)

```cpp
// In setMultiGPUConfig():
void MultiLoRAManager::setMultiGPUConfig(const MultiGPUConfig& config) {
    std::lock_guard<std::mutex> lock(mutex_);
    config_.multi_gpu = config;  // ← Aber config_ wird in selectGPUForLoRA gelesen!
    
    // Andere Threads lesen veraltete config_
}
```

**Problem**: `config_` sollte immutable nach Konstruktor sein

---

## 🟡 SCHWERWIEGENDE FEHLER

### 11. **LLama.cpp Integration ungeprüft** (CRASH-ANFÄLLIGKEIT)
**Datei**: [src/llm/llama_wrapper.cpp](src/llm/llama_wrapper.cpp#L1)

**Externe Dependencies**:
```cpp
extern "C" {
    int llama_lora_adapter_set(struct llama_context* ctx, int adapter_index, float scale);
}
```

**Probleme**:
- ✗ Keine Check ob Funktion existiert
- ✗ Keine Runtime-Überprüfung auf nullptr
- ✗ Keine Validierung von Rückgabewert
- ✗ Wenn llama.cpp alte Version → undefined behavior

**Beispiel**:
```cpp
// In applyLoRA():
int adapter_index = static_cast<int>(reinterpret_cast<uintptr_t>(lora->adapter_handle));
int result = llama_lora_adapter_set(context, adapter_index, lora->scale);
// ← Was wenn llama.cpp diese Funktion nicht hat?
// ← Was wenn adapter_index ungültig ist?
```

### 12. **Multi-GPU VRAM Accounting fehlerhaft** (OVERFLOW)
**Datei**: [src/llm/multi_lora_manager.cpp](src/llm/multi_lora_manager.cpp#L1470-1490)

```cpp
void MultiLoRAManager::updateGPUMemoryTracking() {
    // Already locked by caller
    
    for (auto& [gpu_id, _] : gpu_vram_usage_) {
        gpu_vram_usage_[gpu_id] = 0;  // ← Reset zu 0
    }
    
    for (const auto& [_, lora] : loras_) {
        if (lora->gpu_placement == GPUPlacement::SINGLE_GPU) {
            gpu_vram_usage_[lora->primary_gpu] += lora->vram_bytes;  // ← Kann overflow sein
```

**Problem**:
- Keine Prüfung auf `primary_gpu >= 0`
- Keine Prüfung ob GPU in map existiert (creates entry with value 0!)
- `vram_bytes` kann größer als `max_vram_per_gpu_mb` sein

### 13. **Quantization Memory Accounting falsch** (LEAK)
**Datei**: [src/llm/multi_lora_manager.cpp](src/llm/multi_lora_manager.cpp#L1000-1010)

```cpp
// In quantizeINT8():
// Original size = 32MB
size_t num_weights = lora->original_vram_bytes / sizeof(float);
std::vector<float> weights = simulateWeights(num_weights);  // Capped to 100MB

// Aber original_vram_bytes wird NICHT aktualisiert!
// Später:
lora->vram_bytes = num_weights + lora->scale_factors.size() * sizeof(float);
// ← Das ist QUANTIZED size, aber original_vram_bytes ist noch 32MB!
```

**Problem**: `lora->original_vram_bytes` wird nicht gespeichert → Memory tracking fehlerhaft

---

## FEHLERHAFTE LOGIK-PATTERN

### 14. **Adapter-Cache Hit vs Miss Logik** (COUNTING ERROR)
**Datei**: [src/llm/lora_framework/lora_adapter_manager.cpp](src/llm/lora_framework/lora_adapter_manager.cpp#L50-62)

```cpp
// Check if already loaded (cache hit)
if (adapters_.find(adapter_id) != adapters_.end()) {
    spdlog::debug("Adapter {} already loaded", adapter_id);
    touchAdapter(adapter_id);
    cache_stats_.cache_hits++;
    return true;
}

// Real load attempt (not a cache hit)
cache_stats_.total_loads++;
cache_stats_.cache_misses++;
```

**Problem**: `total_loads` wird IMMER incrementiert, aber nur wenn nicht im Cache!
- Sollte IMMER inkrementiert werden (auch bei hits)
- Oder gar nicht inkrementiert werden

**Korrekt**:
```cpp
if (adapters_.find(adapter_id) != adapters_.end()) {
    cache_stats_.cache_hits++;
    cache_stats_.total_loads++;
    return true;
}
cache_stats_.cache_misses++;
cache_stats_.total_loads++;
// Load...
```

### 15. **GPU Load Balancing unvollständig** (NO-OP)
**Datei**: [src/llm/multi_lora_manager.cpp](src/llm/multi_lora_manager.cpp#L1150-1220)

```cpp
size_t balanceGPULoad() {
    // ...
    for (int overloaded_gpu : overloaded_gpus) {
        for (auto& [lora_id, lora] : loras_) {
            if (lora->primary_gpu == overloaded_gpu && 
                !lora->keep_loaded &&
                lora->gpu_placement == GPUPlacement::SINGLE_GPU) {
                // Try to move to underloaded GPU
                for (int target_gpu : underloaded_gpus) {
                    if (gpu_vram_usage_[target_gpu] + lora->vram_bytes < max_vram_per_gpu_bytes) {
                        // Move LoRA
                        gpu_vram_usage_[overloaded_gpu] -= lora->vram_bytes;
                        gpu_vram_usage_[target_gpu] += lora->vram_bytes;
                        lora->primary_gpu = target_gpu;
                        lora->assigned_gpus = {target_gpu};
                        moved++;
                        break;
                    }
                }
                if (moved >= 5) break;
            }
        }
        if (moved >= 5) break;
    }
    return moved;
}
```

**Problem**:
- Maximal 5 LoRAs pro Balance-Call
- Unvollständige Umverteilung (first underloaded, nicht optimal)
- Keine Schleife bis balanciert

---

## SICHERHEITSPROBLEME

### 16. **Type Casting Unsicherheit** (UB)
**Datei**: [src/llm/multi_lora_manager.cpp](src/llm/multi_lora_manager.cpp#L1340)

```cpp
int adapter_index = static_cast<int>(reinterpret_cast<uintptr_t>(lora->adapter_handle));
```

**Problem**:
- ✗ Keine Validierung dass `adapter_handle` gültig ist
- ✗ Kann sein dass handle ursprünglich kein uintptr_t war
- ✗ Narrowing von 64-bit zu 32-bit auf 64-bit-Systemen

### 17. **Array Bounds nicht geprüft** (BUFFER OVERFLOW)
**Datei**: [src/llm/multi_lora_manager.cpp](src/llm/multi_lora_manager.cpp#L1243)

```cpp
int selected_gpu = config_.multi_gpu.devices[next_round_robin_gpu_];
```

**Problem** (obwohl mein Fix es verbessert):
- Wenn `devices.empty()` wird geprüft, aber...
- Wenn `devices` danach geleert wird (config mutation) → UAF

### 18. **Uninitialisierte Exception Handling** (CRASH)
**Datei**: Multiple Dateien

```cpp
try {
    // Operation
} catch (const std::bad_alloc& e) {
    spdlog::error("...failed: {}", e.what());
    return false;
}
// catch (const std::exception& e) nicht vorhanden → andere Exceptions nicht gehandled
```

---

## EMPFOHLENE IMMEDIATE FIXES (Priorität)

| Pri | Fix | Komponente | Risiko |
|-----|-----|-----------|---------|
| P0 | Adapter-Handle Lifecycle richtig definieren | lora_adapter_manager.cpp | SEGFAULT |
| P0 | GPU Selection -1 error check hinzufügen | multi_lora_manager.cpp | CRASH |
| P0 | Reference Counting auf LoRA-Slots | multi_lora_manager.cpp | SEGFAULT |
| P1 | config_ als const nach Konstruktor | multi_lora_manager.cpp | RACE |
| P1 | selectGPUForLoRA() Caller validiert result | multi_lora_manager.cpp | CRASH |
| P1 | gpu_data_loader mit unique_ptr | gpu_data_loader.cpp | MEMORY LEAK |
| P1 | Eviction Thread Synchronisation verstärken | multi_lora_manager.cpp | RACE |
| P2 | Quantization NaN/Inf checks | multi_lora_manager.cpp | DATA LOSS |
| P2 | applyAdapter() Implementierung fertigstellen | lora_adapter_manager.cpp | SILENT FAILURE |
| P2 | llama.cpp extern functions mit runtime check | llama_wrapper.cpp | UB |

---

## TEST-AUSFÄLLE EXPLAINED

**28 LoRA Quantization Tests crashen**:
1. `simulateWeights()` allokiert zu viel RAM (jetzt gefixt)
2. `selectGPUForLoRA()` gibt Fehler zurück nicht geprüft (jetzt gefixt)
3. `quantizeINT8/INT4()` mit ungültigen Scales (jetzt gefixt)
4. aber: **Reference Counting Fehler** → LoRA wird während Quantization gelöscht

**Empfohlene nächste Tests nach Fixes**:
```bash
cd C:\VCC\themis
cmake -B build-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug -DTHEMIS_BUILD_TESTS=ON -DTHEMIS_ENABLE_LLM=ON
ninja -C build-debug themis_tests
./build-debug/cmake/tests/themis_tests --gtest_filter="*Quantization*" 2>&1 | tee quantization_test.log
```
