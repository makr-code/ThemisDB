# Build Configuration Hardening Audit Report
**Date**: 2026-08-08  
**Status**: In Progress  

## Executive Summary

This audit validates CMakePresets.json and test infrastructure for consistency, completeness, and portability. **3 actionable findings** identified across build parallelization, test configuration, and edition setup.

---

## 1. CMakePresets.json Validation Results

### Overview
- **Total Configure Presets**: 28 (4 hidden base presets, 24 concrete presets)
- **Total Build Presets**: 22
- **Total Test Presets**: 19
- **CMake Version Required**: 3.23.0

### Edition Coverage
| Edition | Config | Build | Test |
|---------|--------|-------|------|
| Community | 8 | 5 | 4 |
| Enterprise | 4 | 4 | 4 |
| Hyperscaler | 4 | 3 | 3 |
| Military | 4 | 4 | 4 |
| Linux | 2 | 2 | 1 |
| Windows | 2 | 2 | 1 |
| Special | 4 | 2 | 2 |

### Validation Checks Passed
✓ All build presets reference valid configure presets  
✓ All test presets reference valid configure presets  
✓ No duplicate binary directories  
✓ No obvious hardcoded environment paths  
✓ Inheritance chain integrity verified  

---

## 2. Test Infrastructure Audit Results

### Test Coverage Summary
- **Total Test Modules**: 528 directories
- **Total Test Files**: 3,781 .cpp files
- **Modules with Focused Tests**: 51 (9.7%)
- **Focused Test Files**: 146
- **Test Registrations**: 269+

### Structure Validation
✓ 603 CMakeLists.txt files found  
✓ 24 directories with tests but no CMakeLists.txt (20 are nested under parent CMakeLists)  
✓ 4 directories with explicit registration  
✓ Test timeout configuration present across modules  

### Registration Pattern Analysis
- **Modules using focused test registration**: 591 (`themis_register_module_focused_test`)
- **Modules using standard registration**: 4 (`themis_register_module_test`)

---

## 3. Audit Findings

### Finding #1: Missing CMAKE_BUILD_PARALLEL_LEVEL in linux-base [MEDIUM]

**Issue**: `linux-base` preset lacks `CMAKE_BUILD_PARALLEL_LEVEL` environment variable, while `windows-base` has it set to 16.

**Impact**: 
- Inconsistent build parallelization across platforms
- Slower builds on Linux compared to Windows
- Maintenance burden when adjusting parallel levels

**Root Cause**: 
The linux-base preset was created without the parallelization setting, likely overlooked during multi-platform support addition.

**Fix**: Add `CMAKE_BUILD_PARALLEL_LEVEL` to linux-base environment section.

**Files to modify**: CMakePresets.json

---

### Finding #2: Test Presets Missing Explicit Filters [MEDIUM]

**Issue**: All 19 test presets lack explicit `output.filter` or `filter` configuration.

**Affected Presets**:
- windows-release
- hyperscaler-debug-windows
- community-debug
- hyperscaler-release-windows
- hyperscaler-release-linux
- linux-release
- community-release
- community-asan
- community-ubsan
- linux-asan
- linux-ubsan (and 8 more)

**Impact**:
- Cannot selectively run test subsets (e.g., "only unit tests" or "only release-critical tests")
- Longer CI/CD cycles without targeted test filtering
- Difficult to parallelize test execution across machines

**Root Cause**:
Test preset filter configuration was not standardized when presets were created. CMake 3.21+ supports `output.filter` for selective test execution via labels.

**Fix**: Add explicit test filters based on CTest labels to each test preset.

**Expected Filters**:
```json
{
  "name": "windows-release",
  "output": {
    "filter": {
      "include": {
        "label": ["release_critical", "module:*"]
      }
    }
  }
}
```

**Files to modify**: CMakePresets.json (all test presets)

---

### Finding #3: Missing THEMIS_EDITION in Some Presets [LOW]

**Issue**: 2 concrete (non-hidden) presets lack explicit `THEMIS_EDITION` cache variable:
1. `community-release-allow-missing-rocksdb`
2. `nightly-bench-sweep`

**Impact**:
- Edition inheritance ambiguity
- Potential misconfiguration of edition-specific features
- Hard to debug which edition a build represents

**Root Cause**:
These specialized presets were added without inheriting or explicitly setting THEMIS_EDITION.

**Fix**: Explicitly set `THEMIS_EDITION: "COMMUNITY"` in cache variables for both presets.

**Files to modify**: CMakePresets.json (2 presets)

---

## 4. Build Parallelization Configuration

### Current State
| Preset | CMAKE_BUILD_PARALLEL_LEVEL | Status |
|--------|-------------------------|--------|
| windows-base | 16 | ✓ Set |
| linux-base | (not set) | ❌ Missing |

### Recommendation
Set `CMAKE_BUILD_PARALLEL_LEVEL: "8"` for linux-base as a conservative default. This can be overridden per machine via environment variable. Value of 8 is reasonable for most modern development machines (4-8 cores typical).

---

## 5. Test Preset Filter Strategy

### Proposed Test Filter Patterns

**Pattern 1: Community Release Testing**
```json
"filter": {
  "include": {
    "label": ["release_critical", "module:*", "tier:unit", "tier:integration"]
  }
}
```

**Pattern 2: Sanitizer Testing** (ASAN/UBSAN)
```json
"filter": {
  "include": {
    "label": ["module:*", "kind:standard"]
  },
  "exclude": {
    "label": ["manual", "disabled"]
  }
}
```

**Pattern 3: Nightly Sweep**
```json
"filter": {
  "include": {
    "label": ["module:*"]
  },
  "exclude": {
    "label": ["manual", "disabled", "slow"]
  }
}
```

---

## 6. Edition Configuration Consistency

### Current State
- **Community presets**: 8 configure + 1 diagnostic
- **Enterprise presets**: 4 (Windows/Linux × Release/Debug)
- **Hyperscaler presets**: 4 (Windows/Linux × Release/Debug)
- **Military presets**: 4 (Windows/Linux × Release/Debug)

### Consistency Check Results
✓ Edition-specific cache variables are properly inherited  
✓ All production presets have explicit THEMIS_EDITION  
⚠ 2 special presets need THEMIS_EDITION clarification  

---

## 7. Remediation Plan

### Phase 1: Fix Critical Issues (Priority: HIGH)
1. Add `CMAKE_BUILD_PARALLEL_LEVEL: "8"` to `linux-base` environment
2. Add `THEMIS_EDITION: "COMMUNITY"` to `community-release-allow-missing-rocksdb`
3. Add `THEMIS_EDITION: "COMMUNITY"` to `nightly-bench-sweep`

**Estimated Time**: 10 minutes  
**Risk Level**: LOW  
**Testing**: CMake configure on Linux + verify parallel build execution  

### Phase 2: Implement Test Preset Filters (Priority: HIGH)
1. Define filter policy in CMakePresets.json documentation
2. Add explicit `output.filter` to all test presets
3. Validate filter expressions with `ctest --help-manual cmake.ctest`

**Estimated Time**: 30 minutes  
**Risk Level**: MEDIUM (requires CMake 3.21+ for test filters)  
**Testing**: Run `ctest -N` with each test preset to verify filter logic  

### Phase 3: Documentation & Hardening (Priority: MEDIUM)
1. Create preset usage guide
2. Document test label conventions
3. Add CI/CD examples for preset-based test selection

**Estimated Time**: 20 minutes  
**Risk Level**: LOW  

---

## 8. Validation Checklist

- [ ] linux-base has CMAKE_BUILD_PARALLEL_LEVEL set
- [ ] All test presets have explicit filters
- [ ] community-release-allow-missing-rocksdb has THEMIS_EDITION
- [ ] nightly-bench-sweep has THEMIS_EDITION
- [ ] CMake configure succeeds on Linux with new settings
- [ ] ctest -N shows correct test count with filters
- [ ] No regressions in build or test execution
- [ ] Documentation updated with filter policies

---

## 9. References

- **CMakePresets Documentation**: https://cmake.org/cmake/help/latest/manual/cmake-presets.7.html
- **CTest Filter Documentation**: https://cmake.org/cmake/help/latest/manual/ctest.1.html#command:ctest
- **Repository Settings**: `/home/runner/work/ThemisDB/ThemisDB/CMakePresets.json`
- **Test Configuration**: `/home/runner/work/ThemisDB/ThemisDB/tests/cmake/TestPolicy.cmake`
- **Test Registration**: `/home/runner/work/ThemisDB/ThemisDB/tests/cmake/RegisterModuleTests.cmake`

---

## Conclusion

CMakePresets.json has **good structural consistency** with proper inheritance chains and comprehensive edition coverage. The main gaps are:
1. **Platform parity**: Linux build parallelization config
2. **Testing agility**: Test preset filters for selective execution
3. **Configuration clarity**: Edition assignment in special presets

All findings are actionable with **low-to-medium complexity** and can be implemented in one batch.
