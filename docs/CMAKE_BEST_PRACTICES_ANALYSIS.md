# CMake Best-Practice Analyse für ThemisDB

> **Datum:** 2026-06-23 · **Basis:** CMake Official Tutorial (Steps 0-11)  
> **Scope:** Vergleich aktueller ThemisDB-CMake-Struktur mit Best-Practices

---

## Zusammenfassung

ThemisDB hat eine **modular gut strukturierte CMake-Konfiguration**, aber es gibt mehrere **Best-Practice Gaps** im Vergleich zum CMake Tutorial und modernen Empfehlungen.

**Gesamtnote: 7/10 (Gut, aber Verbesserungen möglich)**

| Kategorie | Score | Status |
|---|---|---|
| **Step 1: Basic Structure** | 8/10 | ✅ Gute Modularität, aber einige Redundanzen |
| **Step 2: Language Fundamentals** | 7/10 | ⚠️ Macros vs. Functions — inkonsistent |
| **Step 3: Configuration & Cache** | 8/10 | ✅ CMakePresets.json vorhanden, aber nicht alle OPTIONS dokumentiert |
| **Step 4: Target Commands** | 7/10 | ⚠️ PUBLIC/PRIVATE Interfaces nicht konsistent |
| **Step 5: Library Concepts** | 6/10 | ❌ Object Libraries nicht genutzt, Interface Libraries selektiv |
| **Step 6: System Introspection** | 5/10 | ❌ Check-Module kaum genutzt (z.B. CheckIncludeFile) |
| **Step 7: Custom Commands** | 6/10 | ⚠️ Einige Code-Generierung, aber nicht systematisch dokumentiert |
| **Step 8: Testing & CTest** | 7/10 | ✅ CTest/GTest integriert, aber Test-Komponenten-Struktur komplex |
| **Step 9: Installation** | 7/10 | ⚠️ install() Rules vorhanden, aber keine CPack-Integration |
| **Step 10: Finding Dependencies** | 7/10 | ✅ find_package() mit Fallbacks, aber Mixed CONFIG/MODULE Mode |
| **Step 11: Miscellaneous** | 5/10 | ❌ Generator Expressions minimal, Target Aliases selektiv |

---

## 1. Step 1: Basic Structure & Getting Started

### Was das Tutorial empfiehlt

```cmake
# Minimal structure:
cmake_minimum_required(VERSION 3.20)
project(MyProject VERSION 1.0.0 LANGUAGES CXX)

add_executable(myapp main.cpp)
add_library(mylib src.cpp)
target_link_libraries(myapp PRIVATE mylib)

install(TARGETS myapp DESTINATION bin)
```

### Was ThemisDB macht

✅ **Gut:**
- `cmake_minimum_required(VERSION 3.20)` — aktuell
- `project(Themis ...)` mit VERSION, DESCRIPTION, LANGUAGES — vollständig
- Modular (70+ CMake-Dateien in `cmake/`)
- Build-Ordnung dokumentiert (Kommentar in Root CMakeLists.txt)

❌ **Verbesserungsbedarf:**
- **Zu viele separate `include(cmake/...)` Dateien** — führt zu Komplexität und versteckten Abhängigkeiten
  - Root CMakeLists.txt: 150+ Zeilen nur für includes
  - Keine klare Abhängigkeits-Visualisierung zwischen Modulen
  - Schwer zu verstehen, welche Variable in welchem Modul gesetzt wird
  
- **Unklare Initialisierungs-Reihenfolge** — Kommentar sagt "Build order (critical):", aber nicht enforced
  ```cmake
  # 1. Version management
  include(cmake/Versions.cmake)
  # 2. Compiler setup
  include(cmake/CompilerOptions.cmake)
  # ... 15 weitere includes
  ```
  Problem: Wenn jemand das falsch ordnet, kann es zu Silent Failures führen.

- **CMAKE_PROJECT_INCLUDE Hack** — nicht Best-Practice
  ```cmake
  set(CMAKE_PROJECT_INCLUDE "${CMAKE_CURRENT_SOURCE_DIR}/cmake/msvc_includes_fix.cmake")
  ```
  Besser: `include()` direkt nach `project()` oder in CompilerOptions.cmake.

### Empfehlung

**Mittelfristig (3-6 Monate):**
- Reduziere Root CMakeLists.txt auf **max. 10-15 logische Includes**
- Nutze `include(cmake/Init.cmake)` als Single Entry Point, das die Reihenfolge garantiert
- Dokumentiere Abhängigkeiten zwischen Modulen (z.B. als Diagramm)

**Beispiel-Refactor:**
```cmake
# Root CMakeLists.txt (simplified)
cmake_minimum_required(VERSION 3.20)
project(Themis VERSION ... LANGUAGES CXX)

# Single initialization module handles all ordering
include(cmake/ThemisInit.cmake)
```

---

## 2. Step 2: CMake Language Fundamentals

### Was das Tutorial empfiehlt

- **Prefer Functions over Macros** (Functions have local scope)
- **Use foreach(), if(), while() for control flow**
- **Use list() for list operations** (not string manipulation)
- **Avoid string(TOUPPER ...)** in loops — use list functions instead

### Was ThemisDB macht

✅ **Gut:**
- Nutzt Funktionen in mehreren Modulen
- Nutzt `foreach()` korrekt
- `list(APPEND ...)` für Sammlungen

❌ **Verbesserungsbedarf:**
- **Macros werden häufig statt Functions verwendet**
  - `macro(...)` in mehreren Dateien, z.B. `cmake/CopyRuntimeDlls.cmake`
  - Macros haben keine lokale Scope → können zu Variablen-Namenskollisionen führen
  
- **String-Manipulation statt list operations**
  ```cmake
  # Anti-pattern (found in cmake/CMakeLists.txt):
  string(REPLACE ";" "\\;" _CMAKE_LIB_PATH_STR "${_CMAKE_LIB_PATH_LIST}")
  ```
  Besser: Nutze `list(JOIN ...)` oder `target_link_options()`

- **Keine Helper-Functions dokumentiert**
  - Viele lokale Helper-Functions in verschiedenen Dateien
  - Keine zentrale `cmake/helpers.cmake` für Wiederverwendung

### Empfehlung

**Kurz (1-2 Monate):**
- Audit: Alle `macro()` zu `function()` konvertieren (außer wenn Scope-Escape beabsichtigt)
- Erstelle `cmake/helpers.cmake` mit dokumentierten Helper-Funktionen
- Schreibe Unit-Tests für CMake-Funktionen (mit cmake --trace-expand)

**Beispiel:**
```cmake
# cmake/helpers.cmake
function(themis_add_executable target_name)
    # Dokumentierte, wiederverwendbare Logik
    add_executable(${target_name} ${ARGN})
    # Apply common properties
    target_compile_options(${target_name} PRIVATE ${THEMIS_CXX_FLAGS})
endfunction()
```

---

## 3. Step 3: Configuration & Cache Variables

### Was das Tutorial empfiehlt

- `option(MYOPTION "Description" ON/OFF)` für Boolean Options
- `set(MYCACHE "default" CACHE STRING "Description")`
- `CMakePresets.json` für Konfigurations-Templates
- `message(STATUS/WARNING/FATAL_ERROR)` für Benutzer-Feedback

### Was ThemisDB macht

✅ **Sehr gut:**
- `CMakePresets.json` mit mehreren Konfigurationen (`windows-release`, `windows-debug-hyperscaler`, etc.)
- `CMakeUserPresets.json` für lokale Overrides
- `option()` für boolean Cache-Variablen (z.B. `THEMIS_AUTO_BOOTSTRAP_DEPS`)
- Klare `message(STATUS)` für Build-Ausgaben

❌ **Verbesserungsbedarf:**
- **OPTIONS sind über 70+ CMake-Dateien verteilt**
  - Keine zentrale `cmake/FeatureOptions.cmake` Dokumentation
  - Benutzer wissen nicht, welche OPTIONS verfügbar sind
  - Keine `cmake --help-variable` Integration

- **Preset-Dokumentation unvollständig**
  ```json
  // CMakePresets.json: Was ist "windows-release"? 
  // - Compiler? MSVC/Clang/GCC?
  // - Optimierungen? -O2/-O3?
  // - Features? LLM/GPU/Network?
  // Keine README in Presets
  ```

- **Keine Schema-Validierung für Custom OPTIONS**
  - OPTIONS wie `THEMIS_EDITION` sind nicht in `CMakePresets.json` als `cacheVariables` dokumentiert

### Empfehlung

**Kurz (2-3 Wochen):**
- Erstelle `cmake/FeatureOptions.cmake` als Single Source of Truth
  ```cmake
  option(THEMIS_ENABLE_LLM "Enable LLM support" ON)
  option(THEMIS_ENABLE_GPU "Enable GPU acceleration" OFF)
  # ... mit Dokumentation
  ```

- Update `CMakePresets.json` um `cacheVariables`:
  ```json
  "cacheVariables": {
    "THEMIS_ENABLE_LLM": "ON",
    "THEMIS_ENABLE_GPU": "OFF"
  }
  ```

- Schreibe README.md für Presets mit Tabelle aller verfügbaren Optionen

---

## 4. Step 4: In-Depth Target Commands

### Was das Tutorial empfiehlt

- `target_compile_features(mylib PUBLIC cxx_std_17)` für C++-Standard
- `target_compile_options(mylib PRIVATE -Wall -Wextra)` für Compiler-Flags
- `target_compile_definitions(mylib PRIVATE NDEBUG)` für Präprocessor
- **Always use PUBLIC/PRIVATE/INTERFACE** — never global `add_definitions()`

### Was ThemisDB macht

✅ **Gut:**
- Nutzt `target_compile_options()` mit PRIVATE/PUBLIC
- Nutzt `target_compile_definitions()` für Feature-Flags (z.B. `THEMIS_ENABLE_LLM`)
- Nutzt `target_include_directories()` mit Scope

❌ **Verbesserungsbedarf:**
- **Inkonsistente Scope-Nutzung**
  ```cmake
  # Sometimes PUBLIC is used when PRIVATE would suffice
  target_compile_definitions(themis_core PUBLIC DTHEMIS_VERSION="${THEMIS_VERSION}")
  # → This leaks to consumers; PRIVATE is usually better
  ```

- **Keine standardisierte `target_compile_features()`**
  - Kein expliziter `cxx_std_17` oder `cxx_std_20` für Targets
  - C++-Standard wird implizit über Compiler-Flags gesetzt, nicht via `target_compile_features()`

- **Mix von OLD and NEW style**
  ```cmake
  # Old style (found in some places):
  add_definitions(-DTHEMIS_EDITION_COMMUNITY)
  
  # New style (preferred):
  target_compile_definitions(themis_core PRIVATE THEMIS_EDITION_COMMUNITY)
  ```

### Empfehlung

**Kurz (2-3 Wochen):**
- Audit: Ersetze alle `add_definitions()` durch `target_compile_definitions()`
- Audit: Ersetze globale Compiler-Flags durch `target_compile_options()` mit Scope
- Standardisiere C++-Standard:
  ```cmake
  set(CMAKE_CXX_STANDARD 20)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  target_compile_features(themis_core PUBLIC cxx_std_20)
  ```

---

## 5. Step 5: In-Depth Library Concepts

### Was das Tutorial empfiehlt

- **Static Libraries:** `add_library(mylib STATIC src.cpp)` — faster linking, larger binaries
- **Shared Libraries:** `add_library(mylib SHARED src.cpp)` — slower linking, smaller binaries, DLL hell
- **Interface Libraries:** `add_library(mylib INTERFACE)` — header-only, no compilation
- **Object Libraries:** `add_library(mylib OBJECT src.cpp)` — intermediate compilation, flexible linking

### Was ThemisDB macht

✅ **Gut:**
- Nutzt Shared Libraries für modular Build (`themis_core.dll`, `themis_storage.dll`)
- Nutzt Interface Libraries für Header-only Dependencies

❌ **Verbesserungsbedarf:**
- **Keine Object Libraries**
  - Große Monolithic Libraries (`themis_core`, `themis_tests`)
  - Könnten in Object-Libraries zerlegt werden für flexibler Linking
  - Beispiel: `themis_tests` linkt 2000+ Quellen; könnte in Object-Libs pro Modul zerlegt werden

- **Mixed Static/Shared für Dependencies nicht dokumentiert**
  - RocksDB wird als SHARED oder STATIC linkt, aber keine Dokumentation
  - Kann zu ABI-Instabilität führen

- **Keine EXPORT/Import Target Handling für Windows DLLs**
  - Manuelle `DTHEMIS_BASE_EXPORTS` Definitionen statt `generate_export_header()`

### Empfehlung

**Mittelfristig (6-12 Monate):**
- Pilot: Zerlege `themis_tests` in Object-Libraries pro Test-Modul
  ```cmake
  add_library(themis_tests_obj_auth OBJECT tests/auth/*.cpp)
  add_library(themis_tests_obj_storage OBJECT tests/storage/*.cpp)
  
  add_executable(themis_tests
    $<TARGET_OBJECTS:themis_tests_obj_auth>
    $<TARGET_OBJECTS:themis_tests_obj_storage>
  )
  ```

- Nutze `generate_export_header()` statt manuelle `#ifdef` für DLL-Exporte
  ```cmake
  include(GenerateExportHeader)
  generate_export_header(themis_base
    BASE_NAME THEMIS_BASE
    EXPORT_FILE_NAME themis_base_export.h)
  ```

---

## 6. Step 6: System Introspection

### Was das Tutorial empfiehlt

- `check_include_file()` — Prüfe ob Header vorhanden
- `check_source_compiles()` — Prüfe ob Code compiliert
- `check_cxx_compiler_flag()` — Prüfe ob Compiler-Flag unterstützt

### Was ThemisDB macht

✅ **Minimal vorhanden:**
- Einige `find_package()` Fallbacks
- `CMAKE_LIBRARY_PATH` Lookup für MSVC-Libs

❌ **Hauptproblem: Kaum System-Introspection genutzt**
- Keine `check_include_file()` für optionale Headers (z.B. `<cuda.h>`, `<vulkan/vulkan.h>`)
- Keine `check_source_compiles()` für Feature-Detection
- Keine AVX2/NEON-Compiler-Flag-Checks

### Empfehlung

**Mittelfristig (3-6 Monate):**
- Importiere `CheckIncludeFile`, `CheckCXXCompilerFlag` in `cmake/features/`
- Nutze für GPU-Feature-Detection:
  ```cmake
  include(CheckIncludeFile)
  check_include_file("cuda.h" HAS_CUDA)
  check_include_file("vulkan/vulkan.h" HAS_VULKAN)
  
  if(HAS_CUDA AND HAS_VULKAN)
      set(THEMIS_ENABLE_GPU ON)
  endif()
  ```

- Compiler-Flag-Tests für AVX2, NEON:
  ```cmake
  include(CheckCXXCompilerFlag)
  check_cxx_compiler_flag("-mavx2" COMPILER_SUPPORTS_AVX2)
  if(COMPILER_SUPPORTS_AVX2)
      target_compile_options(themis_core PRIVATE -mavx2)
  endif()
  ```

---

## 7. Step 7: Custom Commands and Generated Files

### Was das Tutorial empfiehlt

- `add_custom_command(OUTPUT ...)` — Generiere Dateien
- `add_custom_target(ALL DEPENDS ...)` — Target für generated outputs
- Nutze `file(GENERATE ...)` für Konfigurationsdateien

### Was ThemisDB macht

✅ **Teilweise vorhanden:**
- Proto-Generierung für gRPC (via `add_custom_command`)
- Config-File-Generierung

❌ **Verbesserungsbedarf:**
- **Keine zentrale Dokumentation** von Custom Commands
- **Fehlerbehandlung**: Keine Checks ob Generators erfolgreich waren
- **Rebuilds**: Custom Commands triggern oft unnötige Rebuilds

### Empfehlung

**Kurz (2-3 Wochen):**
- Erstelle `cmake/generators.cmake` mit dokumentierten Custom-Command Helper
- Nutze `add_custom_command()` mit explicittem Dependency Tracking
- Beispiel:
  ```cmake
  add_custom_command(
    OUTPUT ${generated_pb_cpp}
    COMMAND protoc --cpp_out=. ${proto_file}
    DEPENDS ${proto_file} protoc
    COMMENT "Generating Protocol Buffer files"
    VERBATIM)
  ```

---

## 8. Step 8: Testing and CTest

### Was das Tutorial empfiehlt

- `enable_testing()` in Root CMakeLists.txt
- `add_test(NAME mytest COMMAND mytest)` für jeden Test
- CTest Integration: `ctest --output-on-failure`
- Properties: `set_tests_properties(mytest PROPERTIES TIMEOUT 60)`

### Was ThemisDB macht

✅ **Sehr gut:**
- CTest voll integriert
- `enable_testing()` vorhanden
- Test-Presets in `CMakePresets.json`
- `add_test()` mit explicittem Filter (`--gtest_filter`)
- `CTEST.md` dokumentiert komplette Test-Inventur

❌ **Verbesserungsbedarf:**
- **Komplexe Test-Komponenten-Struktur**
  - 600+ Tests über viele Modules verteilt
  - Schwer nachzuvollziehen welche Tests wo sind
  - Keine klare Test-Hierarchie

- **Test-Properties nicht konsistent**
  - Manche Tests haben TIMEOUT, manche nicht
  - Keine LABELS für Test-Kategorisierung (UNIT, INTEGRATION, PERFORMANCE)

- **Keine CTest Custom Dashboards**
  - Keine `cdash` oder `ctest --submit` Integration

### Empfehlung

**Kurz (1-2 Wochen):**
- Standardisiere Test-Properties:
  ```cmake
  function(themis_add_test name)
    add_test(NAME ${name} COMMAND ${ARGN})
    set_tests_properties(${name} PROPERTIES
      TIMEOUT 60
      LABELS "UNIT"
    )
  endfunction()
  ```

- Füge Test-LABELS für Kategorisierung:
  ```cmake
  set_tests_properties(AuthenticationTests PROPERTIES LABELS "SECURITY;UNIT")
  set_tests_properties(ReplicationTests PROPERTIES LABELS "INTEGRATION;SLOW")
  ```

- Ermögliche Filtered Runs:
  ```bash
  ctest -L "SECURITY" --output-on-failure
  ctest -L "UNIT" -L "!SLOW" --parallel 16
  ```

---

## 9. Step 9: Installation Commands and Concepts

### Was das Tutorial empfiehlt

- `install(TARGETS mylib DESTINATION lib)` — Install built artifacts
- `install(DIRECTORY include/ DESTINATION include)` — Install headers
- `install(FILES config.txt DESTINATION etc)` — Install configuration
- `install(EXPORT mylib-targets ...)` — Install CMake export files

### Was ThemisDB macht

✅ **Gut:**
- `install()` Rules für Targets, Headers, Docs vorhanden
- `CMAKE_INSTALL_DEFAULT_COMPONENT_NAME` gesetzt für modulares Packaging

❌ **Verbesserungsbedarf:**
- **Kein Export-Handling**
  - `install(EXPORT ...)` nicht vorhanden
  - Keine `themisdb-config.cmake` für Consumers
  - Externe Projekte können `find_package(ThemisDB)` nicht nutzen

- **Keine Installation von Configuration Files**
  - `config/` und `data/` werden manuell gepackt (siehe `package-*.ps1`)
  - Sollten via `install()` integriert sein

- **Dokumentation-Installation komplex**
  - Mehrere `install(DIRECTORY docs ...)` mit Conditionals
  - Besser: Zentrale `cmake/InstallDocumentation.cmake`

### Empfehlung

**Mittelfristig (6-12 Wochen):**
- Implementiere Export-Handling:
  ```cmake
  install(EXPORT ThemisDBTargets
    FILE themisdb-targets.cmake
    NAMESPACE ThemisDB::
    DESTINATION cmake)
  
  # themisdb-config.cmake template
  install(FILES cmake/themisdb-config.cmake.in
    DESTINATION cmake)
  ```

- Zentralisiere Installation:
  ```cmake
  # cmake/InstallConfig.cmake
  function(themis_install_configs)
    install(DIRECTORY ${THEMIS_ROOT_DIR}/config/
      DESTINATION etc/themis
      COMPONENT runtime)
  endfunction()
  ```

---

## 10. Step 10: Finding Dependencies

### Was das Tutorial empfiehlt

- `find_package(Pkg REQUIRED)` — Obligatorische Dependencies
- `find_package(Pkg QUIET)` — Optionale Dependencies
- `find_package(Pkg CONFIG)` vs. `MODULE` — Prefer CONFIG (vcpkg)
- Transitive Dependencies via EXPORT files

### Was ThemisDB macht

✅ **Gut:**
- `find_package()` mit CONFIG/MODULE Fallbacks
- Alias-Targets für Kompatibilität (`zstd::zstd`, `RocksDB::rocksdb`)
- Handelt optionale Dependencies mit QUIET

❌ **Verbesserungsbedarf:**
- **Mixed CONFIG/MODULE Mode kann zu Instabilität führen**
  ```cmake
  find_package(OpenSSL CONFIG QUIET)
  if(NOT OpenSSL_FOUND)
    find_package(OpenSSL MODULE QUIET)
  endif()
  ```
  Problem: Verschiedene Versions können geladen werden → ABI-Inkompatibilität

- **Keine Versionsprüfung**
  - `find_package(Boost ...)` hat keine Versions-Check
  - Sollte `find_package(Boost 1.70 REQUIRED)` sein

- **Transitive Dependencies nicht dokumentiert**
  - Welche Dependencies braucht `themis_core` wirklich?
  - Keine PUBLIC vs. PRIVATE Separation

### Empfehlung

**Kurz (2-3 Wochen):**
- Standardisiere `find_package()` Pattern:
  ```cmake
  # Immer CONFIG first, kein Fallback zu MODULE
  find_package(OpenSSL 3.0 REQUIRED CONFIG)
  find_package(Boost 1.80 REQUIRED CONFIG COMPONENTS system)
  ```

- Dokumentiere Abhängigkeits-Graph:
  ```
  themis_core
    ├── OpenSSL (PUBLIC)
    ├── RocksDB (PRIVATE)
    └── fmt (PRIVATE)
  
  themis_server
    ├── themis_core (PUBLIC)
    └── gRPC (PRIVATE)
  ```

---

## 11. Step 11: Miscellaneous Features

### Was das Tutorial empfiehlt

- `add_library(mylib ALIAS mylib_real)` — Namespace Aliases
- `$<CONFIG:Debug>` — Generator Expressions für Config-specific Logic
- `$<TARGET_PROPERTY:...>` — Query Target Properties at generation time

### Was ThemisDB macht

✅ **Minimal vorhanden:**
- Einige Aliases (`zstd::zstd` als Alias von `zstd::libzstd_shared`)

❌ **Hauptproblem: Minimal Generator Expression Nutzung**
- Kaum `$<CONFIG:...>`, `$<TARGET_PROPERTY:...>` verwendet
- Stattdessen Conditionals via `if(CMAKE_BUILD_TYPE STREQUAL Debug)`

### Empfehlung

**Kurz (1 Woche):**
- Einführung von Generator Expressions für Config-specific Logic:
  ```cmake
  target_compile_options(themis_core PRIVATE
    $<$<CONFIG:Debug>:-O0 -g3>
    $<$<CONFIG:Release>:-O3 -DNDEBUG>
  )
  ```

- Nutze Aliases für Public API:
  ```cmake
  add_library(ThemisDB::Core ALIAS themis_core)  # Public namespace
  ```

---

## 12. Cross-Cutting Best-Practices

### 12.1 CMakeLists.txt Organization

**Current:**
- Root: 150+ Zeilen, viele includes
- cmake/CMakeLists.txt: 5000+ Zeilen monolithic

**Recommended:**
```
CMakeLists.txt (50 Zeilen, nur essentials)
├── cmake/
│   ├── Init.cmake (Orchestrierung)
│   ├── CompilerOptions.cmake
│   ├── Platform.cmake
│   ├── Dependencies.cmake
│   └── ...
├── src/CMakeLists.txt (Targets)
└── tests/CMakeLists.txt (Tests)
```

### 12.2 Documentation

**Current:**
- Keine Inline-Dokumentation von Custom Functions
- Keine README für CMake Modules

**Recommended:**
```cmake
# cmake/helpers.cmake
##
# themis_add_target_with_flags()
# 
# Adds a target with standard ThemisDB compile flags and features.
# 
# Arguments:
#   TARGET_NAME: Name of the target
#   TYPE: EXECUTABLE or LIBRARY
#   SOURCES: Source files (variadic)
#
# Usage:
#   themis_add_target_with_flags(my_app EXECUTABLE src/main.cpp src/util.cpp)
##
function(themis_add_target_with_flags target_name type)
  ...
endfunction()
```

### 12.3 Error Handling

**Current:**
- Silente Fehler bei fehlenden Dependencies (QUIET mode)
- Keine Validation am Ende der Config

**Recommended:**
```cmake
# At end of CMakeLists.txt
message(STATUS "ThemisDB Build Configuration Summary:")
message(STATUS "  Edition: ${THEMIS_EDITION}")
message(STATUS "  Enable LLM: ${THEMIS_ENABLE_LLM}")
message(STATUS "  Enable GPU: ${THEMIS_ENABLE_GPU}")

if(NOT THEMIS_ENABLE_LLM AND THEMIS_ENABLE_GPU)
  message(FATAL_ERROR "GPU acceleration requires LLM support to be enabled")
endif()
```

---

## 13. Prioritized Action Plan

### Phase 1: Quick Wins (Weeks 1-2)
- [ ] Replace all `macro()` with `function()`
- [ ] Standardize `target_compile_features()` usage (C++20)
- [ ] Add Test LABELS for filtering
- [ ] Audit and remove `add_definitions()`

**Effort:** 4-8 hours

### Phase 2: Structure Improvements (Weeks 3-4)
- [ ] Create `cmake/FeatureOptions.cmake` (Single Source of Truth)
- [ ] Reduce Root CMakeLists.txt to 20 lines via `cmake/Init.cmake`
- [ ] Add `cmake/helpers.cmake` with documented functions
- [ ] Create `cmake/generators.cmake` for Code Generation

**Effort:** 12-16 hours

### Phase 3: Best-Practice Integration (Weeks 5-8)
- [ ] Implement Export/Import Targets (find_package support)
- [ ] Add System Introspection (check_include_file, check_cxx_compiler_flag)
- [ ] Object Libraries Pilot (decompose themis_tests)
- [ ] Generator Expressions for Config-specific Logic

**Effort:** 16-24 hours

### Phase 4: CPack Integration (Weeks 9-12)
- [ ] Implement CPack configuration (see separate CPACK_ANALYSIS.md)
- [ ] Multi-Edition Packaging
- [ ] Installation of Configuration Files

**Effort:** 16-24 hours

---

## 14. Summary: Scoring Rationale

| Step | ThemisDB | Ideal | Gap | Reason |
|---|---|---|---|---|
| 1: Structure | 8 | 10 | 2 | Modular but too many includes, ordering not enforced |
| 2: Language | 7 | 10 | 3 | Macros vs. Functions inconsistent, no helper docs |
| 3: Configuration | 8 | 10 | 2 | Presets good, but OPTIONS scattered, no central docs |
| 4: Targets | 7 | 10 | 3 | PUBLIC/PRIVATE scope not consistent, no C++ feature declaration |
| 5: Libraries | 6 | 10 | 4 | No Object Libraries, no Export Headers, mixed static/shared |
| 6: Introspection | 5 | 10 | 5 | Minimal check_* usage, no Feature Detection |
| 7: Custom Commands | 6 | 10 | 4 | Works but scattered, no error handling, no rebuild optimization |
| 8: Testing | 7 | 10 | 3 | CTest good, but Test Properties inconsistent, no Dashboard |
| 9: Installation | 7 | 10 | 3 | Works for Packaging, but no Export Targets, no config install |
| 10: Dependencies | 7 | 10 | 3 | CONFIG/MODULE mix risky, no transitive deps doc, no version checks |
| 11: Miscellaneous | 5 | 10 | 5 | Minimal Generator Expressions, few Aliases |

**Overall: 7.0 / 10.0** ✅ Solid foundation, modern C++ build system, but room for Best-Practice alignment.

---

## Conclusion

ThemisDB hat eine **funktionale und modular durchdachte CMake-Konfiguration**. Die größten Verbesserungspotenziale liegen in:

1. **Strukturelle Vereinfachung** — Zu viele Module, Reihenfolge nicht erzwungen
2. **Konsistenz** — PUBLIC/PRIVATE Scope, Macros vs. Functions, CONFIG/MODULE Fallbacks
3. **Best-Practice Gaps** — Fehlende Export Targets, System Introspection, Generator Expressions
4. **Documentation** — Optionen und Module nicht zentral dokumentiert

Mit dem priorisierten 4-Phasen-Plan (12 Wochen, ~70 Stunden) können diese Gaps geschlossen werden, ohne die aktuelle Funktionalität zu unterbrechen.

**Empfehlung: Mit Phase 1 & 2 beginnen (Quick Wins + Structure), dann Phase 3 & 4 später.**
