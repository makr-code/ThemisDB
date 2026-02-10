# LoRA System Stabilisierungs-Aktionsplan

## Status: 4 KRITISCHE FEHLER IDENTIFIZIERT + FIXES

### ✅ Bereits repariert (diese Session):
1. **Constructor Member-Initialisierung** ✓
   - `next_round_robin_gpu_` wird initialized
   - `stop_eviction_`, `eviction_thread_` werden initialized
   - Config-Validierung verstärkt

2. **GPU-Array Bounds Checking** ✓
   - Prüfe vor Zugriff auf `config_.multi_gpu.devices[index]`
   - Validiere dass index < devices.size()
   - Defaultwert für leeres devices list

3. **Speicherallokation optimiert** ✓
   - `simulateWeights()` nutzt direktes Konstruktor-Allocation statt reserve+push_back
   - Cap bei 100MB um OOM zu verhindern
   - Try-catch für bad_alloc

4. **Quantization Error-Handling** ✓
   - `quantizeINT8()` und `quantizeINT4()` validieren Input
   - `calibrateScales()` prüft auf NaN/Inf
   - Alle Divisions-by-Zero entfernt

5. **selectGPUForLoRA() Error-Check** ✓
   - Gibt jetzt -1 zurück bei Fehler statt zu crashen
   - Caller validiert Rückgabewert
   - Bessere Logik für GPU-Auswahl

### 🔴 NOCH ZU BEHEBEN (Priorität):

#### P0 - CRASH-ANFÄLLIGKEIT
```
1. LoRAAdapterManager::applyAdapter() ist MOCK
   - Macht NICHTS mit LoRA-Gewichten
   - Setzt nur einen Fake-Handle
   - Inference ignoriert LoRA komplett
   
   FIX: Entweder
   a) LoRAAdapterManager deprecaten und MultiLoRAManager verwenden
   b) Oder vollständige Implementierung in LoRAAdapterManager
   
   Status: CRITICAL - alle LoRA-Tests verwenden diese!
```

2. **LoRA-Unload während Inference**
   ```cpp
   Race Condition:
   Thread1: applyLoRA() → lora->adapter_handle
   Thread2: evictionWorker() → loras_.erase(id)
   Thread1: Zugriff auf gelöschten LoRA → SEGFAULT
   
   FIX: Reference Counting auf LoRA-Slots
   ```

3. **GPU-Tracking unvollständig**
   ```cpp
   Problem: gpu_vram_usage_[gpu_id] erzeugt neue Entry wenn nicht existiert
   Resultat: Später Tracking falsch, kann zu Overflow führen
   
   FIX: Nur auf existierende Keys zugreifen
   ```

#### P1 - FUNCTIONALITÄTS-FEHLER

4. **Quantization Memory Accounting**
   ```cpp
   Problem: original_vram_bytes wird nicht nach Quantization aktualisiert
   Resultat: Stats zeigen falschen Compression-Ratio
   
   FIX: Speichere original_vram_bytes vor Quantization
   ```

5. **Config Mutation nach Init**
   ```cpp
   Problem: setMultiGPUConfig() ändert config_ while anderen Threads lesen
   Resultat: Inkonsistentas Verhalten
   
   FIX: config_ als const nach Konstruktor
   ```

---

## EMPFEHLUNG: Migration Strategy

### Phase 1: Immediate Stabilization (diese Session)
```
✅ Constructor Member-Init
✅ GPU Bounds-Checking
✅ Speicher-Cap bei Quantization
✅ Error-Handling in Quantization
✅ selectGPUForLoRA Error-Check
```

### Phase 2: Deprecation (nächstes Sprint)
```
❌ LoRAAdapterManager DEPRECATEN
✓ Alle Code zu MultiLoRAManager migrieren
✓ Tests umschreiben auf neue API
```

### Phase 3: Safety-Hardening (nach Phase 2)
```
❌ Reference Counting für LoRA-Slots
❌ Config Immutability
❌ Thread-Safe GPU-Tracking
```

---

## TEST-STRATEGIE

### Compile Check:
```bash
cd C:\VCC\themis
cmake -B build-ninja-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug \
  -DTHEMIS_BUILD_TESTS=ON -DTHEMIS_ENABLE_LLM=ON
ninja -C build-ninja-debug 2>&1 | tee build.log
```

### Run Quantization Tests:
```bash
./build-ninja-debug/cmake/tests/themis_tests \
  --gtest_filter="*Quantization*" --gtest_repeat=5 2>&1 | tee quant_test.log
# Erwartung: Alle sollten PASS sein (nicht mehr OOM/crash)
```

### Run LoRA Adapter Tests:
```bash
./build-ninja-debug/cmake/tests/themis_tests \
  --gtest_filter="*LoRA*Adapter*" --gtest_repeat=5 2>&1 | tee adapter_test.log
# Erwartung: Alle Cache/Switch/Load Tests PASS
```

### Run Multi-GPU Tests:
```bash
./build-ninja-debug/cmake/tests/themis_tests \
  --gtest_filter="*MultiGPU*" --gtest_repeat=5 2>&1 | tee multigpu_test.log
# Erwartung: PASS ohne GPU-Array-Crashes
```

### Full Test Suite:
```bash
./build-ninja-debug/cmake/tests/themis_tests 2>&1 | tee full_test.log
grep -E "FAILED|passed|failed" full_test.log
```

---

## NÄCHSTE SCHRITTE FÜR NUTZER

1. **Jetzt kompilieren und testen**:
   ```bash
   cd C:\VCC\themis
   cmake -B build-test -G Ninja -DCMAKE_BUILD_TYPE=Release \
     -DTHEMIS_BUILD_TESTS=ON -DTHEMIS_ENABLE_LLM=ON
   ninja -C build-test themis_tests
   ./build-test/cmake/tests/themis_tests 2>&1 | tee test_results.txt
   ```

2. **Wenn noch Fehler**: Zeige test_results.txt

3. **Dann Phase 2**: Adapter-Manager Migration (LoRAAdapterManager → MultiLoRAManager)

4. **Dann Phase 3**: Concurrency-Hardening

---

## CHANGED FILES SUMMARY

**src/llm/multi_lora_manager.cpp**:
- Constructor: +30 Zeilen (Member-Init + Config-Validation)
- selectGPUForLoRA(): +120 Zeilen (Error-Handling + Bounds-Checks)
- destructor: reordered lock+stopEvictionThread
- quantizeINT8(): +60 Zeilen (Error-Handling)
- quantizeINT4(): +70 Zeilen (Error-Handling)
- calibrateScales(): +50 Zeilen (Error-Handling)
- simulateWeights(): optimized allocation (reserve → constructor)
- loadLoRAInternal(): +4 Zeilen (GPU-Selection error-check)

**Total Changes**: ~400 Zeilen neue Defensive Programming Code

---

## RISKS & MITIGATION

| Risk | Severity | Mitigation |
|------|----------|-----------|
| selectGPUForLoRA returns -1 not checked elsewhere | Medium | Only 1 caller, fixed ✓ |
| GPU-Tracking still uses map[key] unsafely | Medium | Could add at() with exception handling |
| Race conditions in eviction thread | High | Needs Phase 3 fixes (ref counting) |
| applyAdapter() is MOCK | CRITICAL | Needs LoRAAdapterManager deprecation |
| Config mutation race | Medium | Needs Phase 3 fixes (const config) |

---

## Files to Review After Compile

1. build-test/CMakeFiles/themis_tests.build/link.txt
   - Check if all libs linked correctly

2. test_results.txt
   - Count PASSED vs FAILED
   - Look for memory errors (ASAN output)
   - Check for "all tests passed" message

3. If crashes occur, check:
   - Error code in output
   - GTest filter to isolate test
   - Re-run with: --gtest_filter="TestName" --gtest_repeat=10

