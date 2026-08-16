# Phase 5 Blocker Remediation — Agent 2 (CMake Infrastructure)

**Date:** 2026-08-15 15:20 UTC  
**Target:** Blocker #5 (MEDIUM finding)  
**Agent Type:** task (build/infrastructure)  
**Timeline:** 2-4 hours (can run parallel with Agent 1)  
**Dependency:** None (independent from code fixes)

---

## Scope

Add 3 missing CMake presets to `CMakePresets.json`:
- `develop-strict` — Strict compilation with warnings-as-errors
- `develop-asan` — AddressSanitizer validation
- `develop-tsan` — ThreadSanitizer validation

These presets are required by CI/CD validation gates for Phase 2 blocker verification (ASan/TSan testing).

---

## File to Modify

**Primary:** `CMakePresets.json`  
**Related (reference only):** `CMakeLists.txt` (verify sanitizer flags)

---

## Preset Specifications

### Preset 1: `develop-strict`

**Purpose:** Strict compilation with warnings-as-errors (catch undefined behavior early)

**JSON Structure:**
```json
{
  "name": "develop-strict",
  "displayName": "Develop - Strict (Warnings-as-Errors)",
  "description": "Strict compilation with all warnings enabled and treated as errors",
  "inherits": "develop",
  "cacheVariables": {
    "CMAKE_CXX_COMPILE_FLAGS_INIT": "-Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Wshadow -Wunused -Wnull-dereference -Wformat=2 -Werror -Wno-deprecated-declarations",
    "CMAKE_C_COMPILE_FLAGS_INIT": "-Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Wshadow -Wunused -Wnull-dereference -Wformat=2 -Werror",
    "CMAKE_BUILD_TYPE": "Debug"
  }
}
```

**Flags Explained:**
- `-Wall -Wextra -Wpedantic` — All standard warnings
- `-Wconversion -Wsign-conversion` — Implicit type conversion warnings
- `-Wshadow` — Variable shadowing (error-prone)
- `-Wunused` — Unused variables/functions
- `-Wnull-dereference` — Null pointer dereference warnings
- `-Wformat=2` — String format checks
- `-Werror` — Treat all warnings as errors
- `-Wno-deprecated-declarations` — Suppress deprecated API warnings (exemption)

**CI Gate:** Used before PR submission to ensure no warnings slipping through.

---

### Preset 2: `develop-asan`

**Purpose:** AddressSanitizer (detects memory errors: leaks, use-after-free, buffer overflows)

**JSON Structure:**
```json
{
  "name": "develop-asan",
  "displayName": "Develop - AddressSanitizer (ASan)",
  "description": "Build with AddressSanitizer for memory error detection",
  "inherits": "develop",
  "cacheVariables": {
    "CMAKE_CXX_FLAGS_INIT": "-fsanitize=address -fsanitize-recover=address -g",
    "CMAKE_C_FLAGS_INIT": "-fsanitize=address -fsanitize-recover=address -g",
    "CMAKE_EXE_LINKER_FLAGS_INIT": "-fsanitize=address",
    "CMAKE_SHARED_LINKER_FLAGS_INIT": "-fsanitize=address",
    "CMAKE_BUILD_TYPE": "Debug"
  }
}
```

**Flags Explained:**
- `-fsanitize=address` — Enable AddressSanitizer
- `-fsanitize-recover=address` — Continue after finding memory errors (don't crash on first hit)
- `-g` — Include debug symbols for stack traces

**Expected Output:** Memory errors reported to STDERR/log with stack traces.

**CI Gate:** Run all tests with ASan; expect 0 memory errors.

---

### Preset 3: `develop-tsan`

**Purpose:** ThreadSanitizer (detects data races and thread synchronization bugs)

**JSON Structure:**
```json
{
  "name": "develop-tsan",
  "displayName": "Develop - ThreadSanitizer (TSan)",
  "description": "Build with ThreadSanitizer for race condition detection",
  "inherits": "develop",
  "cacheVariables": {
    "CMAKE_CXX_FLAGS_INIT": "-fsanitize=thread -g",
    "CMAKE_C_FLAGS_INIT": "-fsanitize=thread -g",
    "CMAKE_EXE_LINKER_FLAGS_INIT": "-fsanitize=thread",
    "CMAKE_SHARED_LINKER_FLAGS_INIT": "-fsanitize=thread",
    "CMAKE_BUILD_TYPE": "Debug"
  }
}
```

**Flags Explained:**
- `-fsanitize=thread` — Enable ThreadSanitizer
- `-g` — Include debug symbols for stack traces

**Expected Output:** Race conditions reported to STDERR/log with involved thread stack traces.

**CI Gate:** Run all tests with TSan; expect 0 data races.

---

## Implementation Steps

### Step 1: Locate CMakePresets.json

Find the presets structure:
```bash
grep -n "\"presets\":" CMakePresets.json | head -1
# Look for existing "develop" preset as reference
```

### Step 2: Add 3 New Presets

Insert after existing `develop` preset (around line ~100-150, adjust based on file structure):

```json
{
  "name": "develop-strict",
  "displayName": "Develop - Strict (Warnings-as-Errors)",
  "description": "Strict compilation with all warnings enabled and treated as errors",
  "inherits": "develop",
  "cacheVariables": {
    "CMAKE_CXX_FLAGS_INIT": "-Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Wshadow -Wunused -Wnull-dereference -Wformat=2 -Werror -Wno-deprecated-declarations",
    "CMAKE_C_FLAGS_INIT": "-Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Wshadow -Wunused -Wnull-dereference -Wformat=2 -Werror",
    "CMAKE_BUILD_TYPE": "Debug"
  }
},
{
  "name": "develop-asan",
  "displayName": "Develop - AddressSanitizer (ASan)",
  "description": "Build with AddressSanitizer for memory error detection",
  "inherits": "develop",
  "cacheVariables": {
    "CMAKE_CXX_FLAGS_INIT": "-fsanitize=address -fsanitize-recover=address -g",
    "CMAKE_C_FLAGS_INIT": "-fsanitize=address -fsanitize-recover=address -g",
    "CMAKE_EXE_LINKER_FLAGS_INIT": "-fsanitize=address",
    "CMAKE_SHARED_LINKER_FLAGS_INIT": "-fsanitize=address",
    "CMAKE_BUILD_TYPE": "Debug"
  }
},
{
  "name": "develop-tsan",
  "displayName": "Develop - ThreadSanitizer (TSan)",
  "description": "Build with ThreadSanitizer for race condition detection",
  "inherits": "develop",
  "cacheVariables": {
    "CMAKE_CXX_FLAGS_INIT": "-fsanitize=thread -g",
    "CMAKE_C_FLAGS_INIT": "-fsanitize=thread -g",
    "CMAKE_EXE_LINKER_FLAGS_INIT": "-fsanitize=thread",
    "CMAKE_SHARED_LINKER_FLAGS_INIT": "-fsanitize=thread",
    "CMAKE_BUILD_TYPE": "Debug"
  }
}
```

### Step 3: Validate JSON Syntax

```bash
python3 -m json.tool CMakePresets.json > /dev/null && echo "Valid JSON"
```

### Step 4: Test Presets

```bash
# Strict preset
cmake --preset develop-strict && cmake --build --preset develop-strict

# ASan preset
cmake --preset develop-asan && cmake --build --preset develop-asan

# TSan preset (may require libstdc++ or libc++)
cmake --preset develop-tsan && cmake --build --preset develop-tsan
```

---

## Acceptance Criteria

- [ ] 3 presets added to CMakePresets.json
- [ ] JSON syntax valid (python3 -m json.tool passes)
- [ ] All 3 presets configure successfully
- [ ] All 3 presets build without errors
- [ ] Sanitizer flags correctly applied (verify in cmake output)
- [ ] No conflicts with existing presets
- [ ] Documentation in CMakePresets.json comments (optional but recommended)

---

## Commit Message

```
build(cmake): Add missing sanitizer presets (develop-strict, develop-asan, develop-tsan)

- develop-strict: Strict compilation with warnings-as-errors (catch UB early)
- develop-asan: AddressSanitizer for memory error detection
- develop-tsan: ThreadSanitizer for race condition detection

Fixes Phase 5 blocker finding #5 (MEDIUM).
Enables CI/CD validation gates for Phase 2 blocker verification.

Timeline impact: Unblocks CP-1 validation infrastructure.
```

---

## Quality Gates

**Before Submission:**
1. Verify JSON syntax: `python3 -m json.tool CMakePresets.json > /dev/null`
2. Test strict preset: `cmake --preset develop-strict && cmake --build --preset develop-strict`
3. Test ASan preset: `cmake --preset develop-asan && cmake --build --preset develop-asan`
4. Test TSan preset: `cmake --preset develop-tsan && cmake --build --preset develop-tsan` (skip if not supported)

---

**Estimated Duration:** 2-4 hours  
**Target Submission:** 2026-08-15 18:00 UTC (parallel with Agent 1)  
**Dependency Chain:** None (independent; Agent 3 can start immediately after this)

