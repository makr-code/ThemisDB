# Voice Module Duplicate Symbol Analysis (LNK2005/LNK4006)

**Date:** 18. Mai 2026  
**Status:** Identified root cause and duplication points  
**Target Resolution:** Consolidate voice source files to eliminate dual compilation

---

## Executive Summary

The `themis_tests` executable is experiencing **LNK2005/LNK4006 duplicate symbol warnings** because voice-related source files (`voice_error_handler.cpp`, `voice_intent_detector.cpp`, and 16+ others) are being compiled into **both**:

1. **themis_llm module library** (via `THEMIS_LLM_SOURCES` in `cmake/ModularBuild.cmake`)
2. **themis_tests executable directly** (via `target_sources()` in `tests/CMakeLists.txt`)

Then `themis_tests` **links against themis_llm**, creating symbol conflicts.

---

## Root Cause Analysis

### Voice Files in THEMIS_LLM_SOURCES

**File:** [cmake/ModularBuild.cmake](cmake/ModularBuild.cmake#L1288-L1303)  
**Lines:** 1288-1303  
**Condition:** `$<$<BOOL:${THEMIS_ENABLE_LLM}>:...>`

Voice files are added to `THEMIS_LLM_SOURCES` with generator expressions gating on `THEMIS_ENABLE_LLM`:

```cmake
set(THEMIS_LLM_SOURCES
    # ... other LLM files ...
    $<$<BOOL:${THEMIS_ENABLE_LLM}>:../src/voice/voice_error_handler.cpp>
    $<$<BOOL:${THEMIS_ENABLE_LLM}>:../src/voice/voice_intent_detector.cpp>
    $<$<BOOL:${THEMIS_ENABLE_LLM}>:../src/voice/voice_assistant_llm.cpp>
    $<$<BOOL:${THEMIS_ENABLE_LLM}>:../src/voice/audio_preprocessing.cpp>
    $<$<BOOL:${THEMIS_ENABLE_LLM}>:../src/voice/emotion_analyzer.cpp>
    $<$<BOOL:${THEMIS_ENABLE_LLM}>:../src/voice/voice_accessibility.cpp>
    $<$<BOOL:${THEMIS_ENABLE_LLM}>:../src/voice/voice_audio_storage.cpp>
    $<$<BOOL:${THEMIS_ENABLE_LLM}>:../src/voice/voice_authenticator.cpp>
    $<$<BOOL:${THEMIS_ENABLE_LLM}>:../src/voice/voice_batch_processor.cpp>
    $<$<BOOL:${THEMIS_ENABLE_LLM}>:../src/voice/voice_browser_streaming.cpp>
    $<$<BOOL:${THEMIS_ENABLE_LLM}>:../src/voice/voice_macro_manager.cpp>
    $<$<BOOL:${THEMIS_ENABLE_LLM}>:../src/voice/voice_meeting_support.cpp>
    $<$<BOOL:${THEMIS_ENABLE_LLM}>:../src/voice/voice_model_cache.cpp>
    $<$<BOOL:${THEMIS_ENABLE_LLM}>:../src/voice/voice_security.cpp>
    $<$<BOOL:${THEMIS_ENABLE_LLM}>:../src/voice/voice_session_manager.cpp>
    $<$<BOOL:${THEMIS_ENABLE_LLM}>:../src/voice/voice_telephony.cpp>
    $<$<BOOL:${THEMIS_ENABLE_LLM}>:../src/voice/voice_tts_customizer.cpp>
    $<$<BOOL:${THEMIS_ENABLE_LLM}>:../src/voice/wake_word_detector.cpp>
)
```

**Result:** These 18 voice files are compiled into `themis_llm.lib` (or `.dll` depending on build config).

---

### Voice Files in themis_tests Direct Sources

**File:** [tests/CMakeLists.txt](tests/CMakeLists.txt#L1140-L1163)  
**Lines:** 1140-1163  
**Condition:** `if(NOT THEMIS_ENABLE_VOICE_ASSISTANT)`

When `THEMIS_ENABLE_VOICE_ASSISTANT=OFF`, voice production files are added as direct test sources:

```cmake
if(NOT THEMIS_ENABLE_VOICE_ASSISTANT)
    target_sources(themis_tests PRIVATE
        ../src/content/stt_processor.cpp
        ../src/content/tts_processor.cpp
        ../src/voice/audio_preprocessing.cpp
        ../src/voice/wake_word_detector.cpp
        ../src/voice/voice_intent_detector.cpp
        ../src/voice/voice_session_manager.cpp
        ../src/voice/voice_security.cpp
        ../src/voice/voice_error_handler.cpp
        ../src/voice/voice_tts_customizer.cpp
        ../src/voice/voice_meeting_support.cpp
        ../src/voice/voice_audio_storage.cpp
        ../src/voice/voice_accessibility.cpp
        ../src/voice/voice_model_cache.cpp
        ../src/voice/voice_batch_processor.cpp
        ../src/voice/voice_authenticator.cpp
        ../src/voice/voice_macro_manager.cpp
        ../src/voice/emotion_analyzer.cpp
        ../src/voice/voice_browser_streaming.cpp
        ../src/voice/voice_telephony.cpp
    )
endif()
```

**Intent:** These files are added when the `VOICE_ASSISTANT` feature is disabled, assuming themis_llm is not available to provide them.

**Problem:** This assumption is **violated** when:
- `THEMIS_BUILD_MODULAR=ON` (modular build enabled)
- `THEMIS_ENABLE_LLM=ON` (LLM module is built)
- `THEMIS_ENABLE_VOICE_ASSISTANT=OFF` (voice assistant feature is disabled)
- `TARGET themis_llm` exists (linked to themis_tests)

In this scenario, **both** compilations happen → duplicate symbols.

---

### themis_tests Links to themis_llm

**File:** [tests/CMakeLists.txt](tests/CMakeLists.txt#L1527-L1550)  
**Lines:** 1527-1550

themis_llm module is unconditionally added to `_themis_test_core_libs` when it exists:

```cmake
if(THEMIS_BUILD_MODULAR)
    if(DEFINED THEMIS_CORE_SOURCES AND THEMIS_CORE_SOURCES)
        # ... deduplication logic ...
    else()
        foreach(_mod
            themis_base
            themis_storage
            themis_security
            themis_transaction
            themis_sharding
            themis_llm              # <-- themis_llm is added here
            themis_query
            themis_timeseries
            themis_graph
            themis_geo
            themis_content
            themis_network
        )
            if(TARGET ${_mod})
                list(APPEND _themis_test_core_libs ${_mod})
            endif()
        endforeach()
    endif()
endif()
```

**Result:** [tests/CMakeLists.txt](tests/CMakeLists.txt#L1707)

```cmake
target_link_libraries(themis_tests PRIVATE
    GTest::gtest
    GTest::gtest_main
    ${_themis_test_core_libs}    # <-- includes themis_llm
    # ... other deps ...
)
```

---

## Linking Chain That Causes Duplicates

```
1. THEMIS_LLM_SOURCES contains voice files
   ↓
2. themis_llm library is built from THEMIS_LLM_SOURCES
   ↓
3. themis_tests conditionally adds same voice files as direct sources
   ↓
4. themis_tests links against themis_llm
   ↓
5. LINKER SEES:
   - voice_error_handler.cpp compiled in themis_llm
   - voice_error_handler.cpp ALSO compiled directly in themis_tests
   ↓
6. LNK2005/LNK4006: duplicate symbol definition
```

---

## Affected Voice Files (18 Total)

These files appear in **both** `THEMIS_LLM_SOURCES` and the conditional `target_sources()` block:

| Filename | THEMIS_LLM_SOURCES (ModularBuild.cmake) | test Direct Add (tests/CMakeLists.txt) |
|----------|------------------------------------------|---------------------------------------|
| voice_error_handler.cpp | L1288 ✓ | L1154 ✓ |
| voice_intent_detector.cpp | L1289 ✓ | L1153 ✓ |
| voice_assistant_llm.cpp | L1287 ✓ | — |
| audio_preprocessing.cpp | L1290 ✓ | L1144 ✓ |
| emotion_analyzer.cpp | L1291 ✓ | L1166 ✓ |
| voice_accessibility.cpp | L1292 ✓ | L1152 ✓ |
| voice_audio_storage.cpp | L1293 ✓ | L1151 ✓ |
| voice_authenticator.cpp | L1294 ✓ | L1158 ✓ |
| voice_batch_processor.cpp | L1295 ✓ | L1159 ✓ |
| voice_browser_streaming.cpp | L1296 ✓ | L1167 ✓ |
| voice_macro_manager.cpp | L1298 ✓ | L1161 ✓ |
| voice_meeting_support.cpp | L1299 ✓ | L1150 ✓ |
| voice_model_cache.cpp | L1300 ✓ | L1156 ✓ |
| voice_security.cpp | L1301 ✓ | L1148 ✓ |
| voice_session_manager.cpp | L1302 ✓ | L1147 ✓ |
| voice_telephony.cpp | L1303 ✓ | L1168 ✓ |
| voice_tts_customizer.cpp | L1297 ✓ | L1149 ✓ |
| wake_word_detector.cpp | L1304 ✓ | L1145 ✓ |

---

## Current Build Configuration Scenarios

### Scenario A: Monolithic Build (Current Default)
- `THEMIS_BUILD_MODULAR=OFF` (default)
- Result: **NO DUPLICATE SYMBOLS**
  - themis_llm module does NOT exist
  - Voice files are added directly to themis_tests only
  - No linking conflict

### Scenario B: Modular Build WITHOUT LLM
- `THEMIS_BUILD_MODULAR=ON`
- `THEMIS_ENABLE_LLM=OFF`
- Result: **NO DUPLICATE SYMBOLS**
  - Voice files are added directly to themis_tests
  - themis_llm does not exist, so no linking conflict

### Scenario C: Modular Build WITH LLM + Voice Assistant Enabled
- `THEMIS_BUILD_MODULAR=ON`
- `THEMIS_ENABLE_LLM=ON`
- `THEMIS_ENABLE_VOICE_ASSISTANT=ON`
- Result: **NO DUPLICATE SYMBOLS**
  - Voice files compiled into themis_llm only
  - Conditional block `if(NOT THEMIS_ENABLE_VOICE_ASSISTANT)` is FALSE
  - Voice files NOT added to themis_tests
  - Link succeeds

### Scenario D: Modular Build WITH LLM + Voice Assistant Disabled ← **PROBLEMATIC**
- `THEMIS_BUILD_MODULAR=ON`
- `THEMIS_ENABLE_LLM=ON`
- `THEMIS_ENABLE_VOICE_ASSISTANT=OFF`
- Result: **LNK2005/LNK4006 DUPLICATE SYMBOLS**
  - Voice files compiled into themis_llm (via THEMIS_LLM_SOURCES)
  - Voice files ALSO added to themis_tests (conditional block is TRUE)
  - themis_tests links themis_llm
  - Linker sees duplicate definitions for all 18 voice files

---

## Root Cause Summary

**The conditional logic in tests/CMakeLists.txt is insufficient:**

```cmake
if(NOT THEMIS_ENABLE_VOICE_ASSISTANT)
    target_sources(themis_tests PRIVATE
        # ... voice files ...
    )
endif()
```

This condition **assumes** that when `THEMIS_ENABLE_VOICE_ASSISTANT=OFF`, the voice files are NOT already compiled elsewhere.

**This assumption breaks when:**
- themis_llm module exists AND
- themis_llm contains voice files via `THEMIS_LLM_SOURCES` (which it always does when `THEMIS_ENABLE_LLM=ON`)

---

## Proposed Fix

The conditional must be updated to:

```cmake
if(NOT THEMIS_ENABLE_VOICE_ASSISTANT AND NOT TARGET themis_llm)
    target_sources(themis_tests PRIVATE
        # ... voice files ...
    )
endif()
```

**Or** filter out voice files when themis_llm exists and is linked:

```cmake
if(NOT THEMIS_ENABLE_VOICE_ASSISTANT)
    set(_voice_sources
        ../src/content/stt_processor.cpp
        ../src/content/tts_processor.cpp
        ../src/voice/audio_preprocessing.cpp
        # ... all 18 voice files ...
    )
    
    if(NOT TARGET themis_llm)
        target_sources(themis_tests PRIVATE ${_voice_sources})
    endif()
endif()
```

---

## Verification Points

To confirm the fix works:

1. **Scenario D rebuild (modular + LLM + no voice assistant):**
   - Should NOT produce LNK2005/LNK4006 for voice symbols
   - Link should complete successfully

2. **Scenario A rebuild (monolithic default):**
   - Should still work (voice files added to themis_tests)

3. **Scenario B rebuild (modular + no LLM):**
   - Should still work (voice files added to themis_tests)

4. **Scenario C rebuild (modular + LLM + voice assistant):**
   - Should still work (voice files in themis_llm only)

---

## Files to Modify

- [tests/CMakeLists.txt](tests/CMakeLists.txt#L1140-L1163) — Update conditional to exclude when themis_llm exists

---

## References

- **THEMIS_LLM_SOURCES definition:** [cmake/ModularBuild.cmake](cmake/ModularBuild.cmake#L1021)
- **Voice file inclusion in ModularBuild:** [cmake/ModularBuild.cmake](cmake/ModularBuild.cmake#L1288-L1303)
- **Voice file inclusion in tests:** [tests/CMakeLists.txt](tests/CMakeLists.txt#L1140-L1163)
- **themis_llm module target creation:** [cmake/ModularBuild.cmake](cmake/ModularBuild.cmake#L2199-L2230)
- **themis_tests linking:** [tests/CMakeLists.txt](tests/CMakeLists.txt#L1706-1720)
