# Unity Build Validator - Fehlerklassifikation

## Fehlertyp: "Unity Build Namespace Closure Imbalance"

**ID**: UNITY_NAMESPACE_IMBALANCE  
**Severity**: 🔴 CRITICAL  
**Detection**: Kumulative Brace-Analyse in CMake-Konfiguration ODER Compile-Zeit-Fehler  
**Affected Components**: themis_graph Module (20+ .cpp files)

---

## Fehlerbeschreibung

### Root Cause
Wenn CMake mit `MSVC_UNITY_BUILD=ON` konfiguriert wird, werden mehrere C++-Dateien in eine single Compilation Unit (`.cxx` file) concateniert:

```cmake
# Unity-File: build/ninja/cmake/CMakeFiles/themis_graph.dir/Unity/unity_0_cxx.cxx
#include "../../../src/acceleration/ai_hardware_dispatcher.cpp"
#include "../../../src/index/graph_auto_buffer.cpp"
...
#include "../../../src/graph/explain_plan.cpp"           # File 11 - opens namespaces, closes them
#include "../../../src/graph/ontology_manager.cpp"       # File 12 - REOPENS namespaces, closes them
...
```

**Problem**: Wenn File 11 (`explain_plan.cpp`) mit `} // namespace themis` **ENDET**, dann kann File 12 (`ontology_manager.cpp`) nicht mit `} // namespace themis` **STARTEN** - die Klammer wurde bereits geschlossen!

### Symptome

**Compile-Fehler (MSVC)**:
```
src\graph\cep_engine.h:461: error C2143: syntax error: missing '}' before 'constant'
src\graph\cep_engine.h:461: error C2065: 'EventStream': undeclared identifier
src\graph\cep_engine.h:464: error C2923: 'result': 'EventStream' is not a valid argument...
(33+ cascading errors)
```

**Wurzelsymptome**:
- Fehler tritt in SPÄTER Dateien auf (nicht in der problematischen Datei)
- Fehler manifestiert sich als Namespace-Corruption (Symbole "verschwinden")
- IntelliSense/Single-File-Analyse zeigt KEINE Fehler
- Build ohne Unity (`MSVC_UNITY_BUILD=OFF`) erfolgreich
- Original-Dateien sind syntaktisch korrekt

---

## Diagnostische Befunde (ThemisDB 2026-06-19)

**File**: `src/graph/ontology_manager.cpp`  
**Line**: 38-39 (namespace open), 701-702 (namespace close)  
**Issue**: 161 `{` but 162 `}` (extra closing brace)

**Cumulative Brace Analysis**:
| File # | Name | Opens | Closes | Balance | Cumulative |
|--------|------|-------|--------|---------|------------|
| 1-11 | (acceleration/index/graph files) | 2752 | 2752 | 0 | **0** |
| 12 | ontology_manager.cpp | 161 | 162 | **-1** | **-1** ⚠️ |
| 13-21 | (remaining files) | 361 | 361 | 0 | **-1** (persists) |

**Root Cause**: When Unity build parser reaches File 12, it has:
```
// [From File 11 / explain_plan.cpp]
} // namespace graph
} // namespace themis

// [File 12 / ontology_manager.cpp starts]
namespace themis {
namespace graph {
  ...
  [702 lines later]
  ...
} // namespace graph
} // namespace themis   <-- THIS IS EXTRA! (no opening was consumed for it)
```

---

## Lösungsansätze

### Option A: Remove redundant namespace closure in File 12 (RECOMMENDED)
**File**: `src/graph/ontology_manager.cpp`  
**Action**: Delete lines 701-702 (closing namespace blocks)  
**Rationale**: File 12 is middle of Unity sequence, should NOT close namespaces opened before it

**Before**:
```cpp
    return true;
}

} // namespace graph
} // namespace themis
```

**After**:
```cpp
    return true;
}
// (namespace closed by later file, if needed)
```

### Option B: Remove namespace re-opening in File 12 (INVASIVE)
**File**: `src/graph/ontology_manager.cpp`  
**Action**: Delete lines 38-39 (namespace opens) AND 701-702 (namespace closes)  
**Rationale**: Rely on File 11's namespaces remaining open  
**Cost**: More refactoring, potential ODR issues

### Option C: Non-Unity builds only (NOT RECOMMENDED)
**Action**: Disable Unity in CMakeUserPresets.json  
**Rationale**: Avoids root cause but loses 30-50% build time performance  
**Cost**: Major build infrastructure regression

---

## Prevention Strategy

### Detection Phase
1. **CMake Pre-Configure Check** (new):
   - Run `tools/unity_build_validator.py --module themis_graph` before build attempt
   - Block build if ERROR status detected
   - Integration point: `cmake/CMakeLists.txt` (add_custom_target for validation)

2. **CI/CD Gate** (new):
   - Add validation step to GitHub Actions / local test pipeline
   - Run validator against all ENABLED modules before linking

3. **Developer Documentation**:
   - **DO**: Ensure last line of .cpp file that ends Unity sequence closes namespaces
   - **DO**: Ensure first line of .cpp file that starts Unity sequence opens same namespaces OR reopens explicitly
   - **DON'T**: Reopen+close namespaces in middle of Unity sequence without closing all previous files

### Code Quality Checks
- ESLint-style rule: "No unclosed namespaces in single-file analysis"
- Linter rule: "Cumulative brace balance must be 0 per file group"

---

## Implementation Status

**✅ DETECTION**: tools/unity_build_validator.py created (120+ lines)
- Parses CMakeLists.txt for module source lists
- Analyzes individual files for brace balance
- Simulates Unity concatenation
- Reports exact file/line for imbalances
- JSON output format for CI/tooling integration

**🔴 FIX**: Pending - requires File 12 modification

**⏳ PREVENTION**: To be integrated into CMake workflow

---

## References

- **Report**: `ai_working/unity_build_analysis.json`
- **Validator**: `tools/unity_build_validator.py`
- **MSVC Docs**: Unity builds / incremental compilation
- **CMake**: `MSVC_UNITY_BUILD` documentation

---

## Related Issues

- ISSUE: Build fails with C2143/C2065/C2923 in themis_graph module
- TRACKED: Session conversation summary (14750 lines)
- RELATED: Gap Scanner v3 orchestration
