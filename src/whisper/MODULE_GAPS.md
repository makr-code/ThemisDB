# whisper Module — Implementation Gap Analysis

**Status:** ✅ Remediated (2026-05-19)  
**Last Updated:** 2026-05-19  
**Scan Reference:** `ai_working/gap_scan_whisper.json` + `ai_working/gap_scan_v3_whisper.json`

---

## 📊 Gap Summary

| Severity | Count | Status |
|----------|-------|--------|
| 🔴 CRITICAL | 1 | ✅ Fixed |
| 🟠 HIGH | 3 | ✅ Fixed |
| 🟡 MEDIUM | 2 | ✅ Fixed |
| **TOTAL** | **6** | ✅ All remediated |

---

## ✅ Remediated Gaps

### CRITICAL — Data Race on `vad_` / `vad_cfg_` (whisper_plugin.cpp)

**Root cause:** `setVoiceActivityDetector()` and `applyVad()` accessed `vad_` and `vad_cfg_`
from multiple threads without synchronization, leading to a data race.

**Fix:** Added `vad_mutex_` (declared `mutable std::mutex` in `whisper_plugin.h`) and
locked it in both `setVoiceActivityDetector()` and `applyVad()`. The external `vad_`
null-check in `transcribeStream()` was moved inside `applyVad()` to keep the entire
read/use sequence atomic.

**Files changed:**
- `include/whisper/whisper_plugin.h` — added `vad_mutex_`
- `src/whisper/whisper_plugin.cpp` — locked `vad_mutex_` in `setVoiceActivityDetector()` and `applyVad()`

---

### HIGH — Division-by-Zero when `num_channels == 0` in `parseWav()` (audio_chunk_reader.cpp)

**Root cause:** `parseWav()` used `num_channels` as a divisor and loop bound immediately
after reading it from the fmt chunk without checking it was non-zero.

**Fix:** Added explicit `num_channels == 0` check that throws `std::runtime_error` before
the data chunk processing begins. Also added an upper-bound guard (`num_channels > 64`)
to reject unreasonably large values that could cause runaway memory allocation.

**Files changed:**
- `src/whisper/audio_chunk_reader.cpp` — bounds guard before the decode loops

---

### HIGH — Pointer Arithmetic Without Validated Bounds in `parseWav()` (audio_chunk_reader.cpp)

**Root cause:** The `memcpy` expressions `data[data_start + (i * num_channels + ch) * N]`
could access beyond the buffer if `num_channels` was 0 or excessively large.

**Fix:** Covered by the `num_channels` range guard above.

**Files changed:**
- `src/whisper/audio_chunk_reader.cpp` (same fix as above)

---

### HIGH — Raw `delete` Without Null-Safety Note in C-API Destroy Function (whisper_plugin.cpp)

**Root cause:** The C ABI `themis_audio_destroy()` called `delete p` without any
indication that null-safety was considered.

**Fix:** Added a clarifying comment documenting that `delete nullptr` is well-defined in
C++ and that ownership is transferred to this function.

**Files changed:**
- `src/whisper/whisper_plugin.cpp` — added ownership-transfer comment

---

### MEDIUM — Explicit `f.close()` RAII Violation in `WavAudioChunkReader::readFile()` (audio_chunk_reader.cpp)

**Root cause:** `std::ifstream::close()` was called manually, which is redundant and
potentially inconsistent (the destructor already closes the file, so the explicit call
only matters if exception safety was intended but not achieved).

**Fix:** Removed the explicit `f.close()` call. The stream now closes automatically when
it goes out of scope, which is the correct RAII pattern.

**Files changed:**
- `src/whisper/audio_chunk_reader.cpp`

---

### MEDIUM — O(n²) String Concatenation in `shellEscape()` (audio_chunk_reader.cpp)

**Root cause:** `shellEscape()` built the escaped string with `operator+=` in a loop
starting from a string-literal `"'"`, triggering reallocation on every append when many
single-quote characters were present.

**Fix:** Pre-computed the worst-case capacity and called `reserve()` before the loop,
bounding the memory cost to O(n) instead of O(n²).

**Files changed:**
- `src/whisper/audio_chunk_reader.cpp`

---

## 🧪 Regression Tests Added

Test groups added to `src/whisper/tests/test_whisper_plugin.cpp`:

| Group | Tests | Coverage |
|-------|-------|----------|
| R | R1, R2 | VAD thread-safety: concurrent set+transcribe, post-transcribe set |
| S | S1, S2, S3 | WAV parser: zero channels throws, excessive channels throws, 64-channel boundary accepted |

---

## 📚 Files Changed

| File | Change |
|------|--------|
| `include/whisper/whisper_plugin.h` | Added `vad_mutex_` |
| `src/whisper/whisper_plugin.cpp` | VAD locking, RAII-aware VAD call, destroy comment |
| `src/whisper/audio_chunk_reader.cpp` | Channel guard, RAII f.close removal, reserve() |
| `src/whisper/tests/test_whisper_plugin.cpp` | Groups R (2 tests) and S (3 tests) |

---

## 🔄 How to Re-scan

```bash
python tools/gap_audit_pipeline_v2.py
python tools/auto_gap_categorizer.py ai_working/gap_scan_v3_aggregate.json --module whisper
```

---

**Format:** THEMIS_MODULE_GAPS_v1  
**Last Remediation:** 2026-05-19 by copilot/gap-remediation-whisper
