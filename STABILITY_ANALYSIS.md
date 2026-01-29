# Kritische Stabilitätsprobleme - multi_lora_manager.cpp

## 🔴 KRITISCHE FEHLER (Systemzusammenbruch)

### 1. **GPU-Array-Zugriff ohne Grenzenprüfung** (CRASH)
**Datei**: [src/llm/multi_lora_manager.cpp](src/llm/multi_lora_manager.cpp#L1243)
**Problem**: `int selected_gpu = config_.multi_gpu.devices[next_round_robin_gpu_]` 
- `next_round_robin_gpu_` wird never initialized in Constructor
- `config_.multi_gpu.devices` könnte leer sein
- **Ergebnis**: Array-Zugriff außerhalb Grenzen → Crash/UB

**Fix erforderlich**:
```cpp
// Konstruktor muss initialisieren:
next_round_robin_gpu_ = 0;

// Und vor Zugriff prüfen:
if (config_.multi_gpu.devices.empty()) {
    return 0;  // Fallback to default GPU
}
```

### 2. **GPU-VRAM-Tracking mit nicht initialisiertem GPU** (SEGFAULT)
**Datei**: [src/llm/multi_lora_manager.cpp](src/llm/multi_lora_manager.cpp#L1247)
**Problem**: `gpu_vram_usage_[selected_gpu]` 
- GPU existiert möglicherweise nicht in `gpu_vram_usage_` map
- Zugriff erzeugt NEW entry mit 0-Wert (falsch!)
- Später wird aus map gelesen aber nie geschrieben

**Szenario**:
```
config_.multi_gpu.devices = [0, 1, 2]  // 3 GPUs definiert
gpu_vram_usage_ = {0: 100MB}            // Nur GPU 0 initialisiert?!
next_round_robin_gpu_ % 3 = 1
gpu_vram_usage_[1] // ← Erzeugt neue Entry mit 0!
```

**Fix erforderlich**: 
- All definierten GPUs im Constructor initialisieren
- Vor Zugriff validieren, dass GPU in map existiert

### 3. **Kein Hardware-Validation** (STILLE FEHLER)
**Datei**: [src/llm/multi_lora_manager.cpp](src/llm/multi_lora_manager.cpp#L50-70)
**Problem**: 
- Constructor initialisiert GPUs aus config aber validiert NICHT, dass Hardware existiert
- GPU-ID 0, 1, 2 in config → aber reale Machine hat nur GPU 0!
- Laden erfolgt auf nicht-existierende GPU → Crash im llama.cpp Layer

**Keine Funktion**:
- `detectAvailableGPUs()` existiert nicht
- `getGPUCapabilities()` existiert nicht  
- Keine CUDA/HIP Runtime-Abfrage

**Fix erforderlich**: Implementiere Hardware-Erkennung

### 4. **Kapazitätsprüfung Fehlgeschlagen** (OVERFLOW)
**Datei**: [src/llm/multi_lora_manager.cpp](src/llm/multi_lora_manager.cpp#L1172)
**Problem**: 
```cpp
float usage_ratio = static_cast<float>(usage) / config_.multi_gpu.max_vram_per_gpu_mb;
```
- Integer zu Float Konvertierung kann Präzision verlieren
- `config_.multi_gpu.max_vram_per_gpu_mb = 0` würde Division durch Null sein

### 5. **Nicht initialisierte Member** (UNDEF. BEHAVIOR)
**Konstruktor fehlende Initialisierung**:
- `next_round_robin_gpu_` ← **NICHT INITIALISIERT**
- `eviction_thread_` ← möglich nicht initialisiert
- `stop_eviction_` ← möglich nicht initialisiert

**Problem**: Wilde Zugriffe auf diese Variablen führen zu UB

### 6. **GPU-Kapazität wird IGNORIERT bei Fehler** (CRASH)
**Datei**: [src/llm/multi_lora_manager.cpp](src/llm/multi_lora_manager.cpp#L1258)
```cpp
spdlog::warn("All GPUs are near capacity, using GPU {} anyway", selected_gpu);
return selected_gpu;  // ← CONTINUE TROTZDEM!
```
- Laden wird trotzdem versucht
- GPU überlaufen → OOM → Crash
- **Sollte**: Fehler zurückgeben statt zu crashen

## 🟠 KRITISCHE MÄNGEL (Speicher/Ressourcen)

### 7. **Speicherleck in quantizeLoRA()** (MEMORY LEAK)
**Datei**: [src/llm/multi_lora_manager.cpp](src/llm/multi_lora_manager.cpp#L1418)
**Problem**:
```cpp
std::vector<float> weights = simulateWeights(num_weights);
if (weights.empty()) {
    return false;  // ← weights vector wird automatic freigegeben ✓
}
// aber:
quantizeINT8(lora, weights);  // Kopie? oder Move?
```

Größer Problem: Original LoRA weights nicht freigegeben nach Quantization!

### 8. **simulateWeights() zu viel Speicher reserviert** (OOM)
**Datei**: [src/llm/multi_lora_manager.cpp](src/llm/multi_lora_manager.cpp#L1587)
```cpp
const size_t MAX_ALLOCATION_SIZE = 100 * 1024 * 1024 / sizeof(float);
weights.reserve(actual_count);  // ← reserve aber nicht im Loop allocieren!
for (size_t i = 0; i < actual_count; ++i) {
    weights.push_back(dist(gen));  // ← Mehrere reallocations!
}
```
**Problem**: 
- `reserve()` allokiert Platz aber `push_back()` in Loop reallokiert trotzdem
- Ineffizient
- Bei 100MB × mehrere Tests = RAM-Druck

**Fix**: Direkt konstruieren mit `std::vector<float>(actual_count)` und dann mit Werten füllen

### 9. **GPU Speicher nicht tracken** (LEAK)
**Datei**: [src/llm/multi_lora_manager.cpp](src/llm/multi_lora_manager.cpp#L1200-1210)
**Problem**: 
```cpp
lora->assigned_gpus = {target_gpu};
// ← assigned_gpus wird aktualisiert aber:
// 1. Alte GPU list nicht gelöscht
// 2. Speicher-tracking nicht aktualisiert pro GPU
// 3. Mehrfach-GPU LoRAs werden falsch tracked
```

### 10. **Thread-Safety Mängel** (RACE CONDITIONS)
**Destruktor**:
```cpp
stopEvictionThread();
std::lock_guard<std::mutex> lock(mutex_);  // ← Reihenfolge falsch!
```
**Problem**: 
- `stopEvictionThread()` wartet auf eviction_thread_, die evtl noch `mutex_` hält
- **Deadlock möglich**
- Thread könnte `loras_` mit unvollständigem Lock modifizieren

**Fix**: Lock ZUERST nehmen, dann thread stoppen

## 🟡 SCHWERWIEGENDE FEHLER (Funktionalität)

### 11. **Quantization mit unzureichender Error-Handling**
- `quantizeINT8()` und `quantizeINT4()` können bei Edge-Cases crashen
- Keine Validierung von `lora->scale_factors.size()`
- Keine Prüfung für zero-scales vor Division

### 12. **VRAM-Accounting ist inkonsistent**
- `total_vram_bytes_` wird manuell verwaltet (fehleranfällig)
- `gpu_vram_usage_[gpu_id]` wird asynchron aktualisiert
- Eviction könnte während Laden stattfinden → Race Condition

### 13. **Keine Validierung von config_.multi_gpu.devices**
- Könnte leer sein: `devices.empty()` wird geprüft aber zu spät
- Könnte negative GPU-IDs enthalten: `-1, -2, 999`
- Keine Bereichsprüfung gegen maximale GPU-Zahl

---

## SOFORT-FIXES ERFORDERLICH (vor Tests)

| Priorität | Fix | Datei | Lines |
|-----------|-----|-------|-------|
| 🔴 P0 | Initialisiere `next_round_robin_gpu_ = 0` in Constructor | multi_lora_manager.cpp | ~50 |
| 🔴 P0 | Validiere GPUs existieren vor Zugriff | multi_lora_manager.cpp | 1243, 1247, 1253 |
| 🔴 P0 | Ändere Fehlerbehandlung bei GPU-Kapazität von WARN zu ERROR+RETURN | multi_lora_manager.cpp | 1258 |
| 🔴 P0 | Implementiere `detectAvailableGPUs()` mit CUDA/HIP Query | multi_lora_manager.cpp | ~ |
| 🟠 P1 | Beheche simulateWeights() reserve/push_back Ineffizienz | multi_lora_manager.cpp | 1587 |
| 🟠 P1 | Fixe Reihenfolge in Destruktor: lock zuerst, dann stopEvictionThread | multi_lora_manager.cpp | ~100 |
| 🟠 P1 | Validiere alle config_.multi_gpu.* Werte in Constructor | multi_lora_manager.cpp | ~50 |
| 🟡 P2 | Füge Bounds-Prüfung zu quantizeINT8/INT4 hinzu | multi_lora_manager.cpp | 1480, 1520 |
