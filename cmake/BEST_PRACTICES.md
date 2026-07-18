# CMake Best Practices for ThemisDB

**Document Version:** Phase 1 (2026-07-18)  
**Status:** Foundation for contributor onboarding and consistency

---

## Core Principles

### 1. Use Modular Architecture

- **One CMake concern per file** — split edition, feature, platform, packaging, and validation logic
- **Root CMakeLists.txt** — orchestration only (includes, no business logic)
- **Separate module CMakeLists.txt** — handle target creation and dependencies

**Example:**
```cmake
# ✅ GOOD (root CMakeLists.txt)
include(cmake/editions/EditionDefaults.cmake)
include(cmake/features/FeatureDefaults.cmake)

# ❌ BAD (root CMakeLists.txt)
option(THEMIS_ENABLE_HTTP3 "..." ON)  # Should be in cmake/features/NetworkFeatures.cmake
```

---

### 2. Use Helpers for Repetitive Patterns

ThemisDB provides **consolidated macros** to reduce duplication:

#### `themis_register_edition_option()`
For registering edition-specific options with consistent naming and defaults.

**Usage:**
```cmake
# In cmake/editions/COMMUNITY.cmake
themis_register_edition_option(
    OPTION_NAME THEMIS_ENABLE_HTTP3
    DESCRIPTION "Enable HTTP/3 (QUIC) support for Community edition"
    DEFAULT ON
)
```

**Benefits:**
- Reduces boilerplate (~3 lines → 1 line)
- Enforces naming consistency
- Centralizes future enhancements (e.g., auto-doc generation)

#### `themis_add_test()`
For creating CTest tests with uniform properties (timeout, labels).

**Usage:**
```cmake
# In tests/*/CMakeLists.txt
themis_add_test(
    test_my_feature
    my_test_executable arg1 arg2
)
```

**Behavior:**
- Creates test with 60s timeout by default
- Applies 'unit' label (discoverable via `ctest -L unit`)

---

### 3. Follow Edition-Feature Governance

**Edition hierarchy:**
```
MILITARY (most restrictive)
  ↓
ENTERPRISE
  ↓
HYPERSCALER
  ↓
COMMUNITY
  ↓
MINIMAL (most permissive)
```

**Each edition** should:
1. Include defaults from below
2. Add edition-specific options
3. Enable features appropriate to its use case

**Example:**
```cmake
# ✅ GOOD (cmake/editions/ENTERPRISE.cmake)
include(cmake/editions/COMMUNITY.cmake)  # Inherit from Community

# Add Enterprise-specific options
themis_register_edition_option(
    OPTION_NAME THEMIS_ENABLE_ADVANCED_AUDITING
    DESCRIPTION "Enable enterprise-grade audit logging"
    DEFAULT ON
    ADVANCED
)
```

---

### 4. Namespace and Scope Variables Carefully

**Global cache variables** (visible to all modules):
- Use `set(...CACHE STRING/BOOL/PATH)` for user-settable options
- Prefix with `THEMIS_` to avoid collisions
- Document at point of definition

**Local/function variables:**
- Use `set()` without `CACHE` for intermediate computations
- Prefix with `_` to indicate local scope (optional but recommended)
- Clean up after function returns

**Example:**
```cmake
# ✅ GOOD
set(_themis_temp_var "${CMAKE_BINARY_DIR}/temp")  # Local, temporary
set(THEMIS_CUSTOM_FLAG OFF CACHE BOOL "User-settable option")  # Global, documented

# ❌ BAD
set(temp_var "${CMAKE_BINARY_DIR}/temp")  # Unclear if global or local
set(CUSTOM_FLAG OFF)  # No THEMIS_ prefix, unintended global scope
```

---

### 5. Validation Before Use

Always validate:
- **File existence** — `if(NOT EXISTS <path>)` before including/processing
- **Required variables** — `if(NOT DEFINED <var>)` before use
- **CMake version** — `cmake_minimum_required(VERSION X.Y.Z)` at start
- **External commands** — `find_program()` before `execute_process()`

**Example:**
```cmake
# ✅ GOOD
find_program(THEMIS_GIT_EXECUTABLE git)
if(NOT THEMIS_GIT_EXECUTABLE)
    message(FATAL_ERROR "git not found in PATH")
endif()

# ❌ BAD
execute_process(COMMAND git clone ...)  # May fail silently if git not found
```

---

### 6. Document Complex Logic

Use **in-line comments** and **CMake code blocks** to explain:
- Why a particular configuration is needed
- What assumptions are being made
- Edge cases or platform-specific quirks

**Example:**
```cmake
# ✅ GOOD
# On Windows, separate runtime/lib output directories to avoid parallel
# linker/file-lock collisions during large builds with hundreds of targets.
# See: cmake/CPackConfig.cmake for packaging layout expectations.
if(WIN32)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
endif()

# ❌ BAD
if(WIN32)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
endif()
```

---

### 7. Test Your CMake Changes

Before committing:
1. **Syntax check** — `cmake --syntax-only <file>`
2. **Configure test** — `cmake --preset <name> --fresh`
3. **Build test** — `cmake --build --preset <name>`
4. **Sanity check** — Verify expected targets/options exist

**Quick verify script:**
```bash
#!/bin/bash
set -e
cmake --preset community-release --fresh
cmake --build --preset community-release --target help | head -20
ctest --preset community-release --show-only | head -20
```

---

## Adding New Options

**Checklist:**

- [ ] Create option in appropriate `cmake/editions/*.cmake` or `cmake/features/*.cmake`
- [ ] Use `themis_register_edition_option()` macro
- [ ] Document in module header (why, when, edge cases)
- [ ] Add validation check if dependencies are required
- [ ] Test with `cmake --preset <name> --fresh -D<OPTION>=ON/OFF`

---

## Adding New Targets

**Checklist:**

- [ ] Place `add_library()` / `add_executable()` in module CMakeLists.txt
- [ ] Set `target_include_directories(... PUBLIC)` for public API
- [ ] Set `target_link_libraries()` with transitive dependencies
- [ ] For tests: use `themis_add_test()` macro for consistency
- [ ] Document via Doxygen comments in C++ header (not CMake)

---

## Troubleshooting

### "CMake Error: option() not found"
**Cause:** Option variable collision across modules.  
**Fix:** Ensure option is defined in exactly one CMake file; use `CACHE FORCE` if redefining.

### "Target not found" linking error
**Cause:** Forgot to `link_directories()` or `target_link_libraries()`.  
**Fix:** Check that all dependencies are linked in correct order (transitive vs. public vs. private).

### Slow CMake configure
**Cause:** Too many `find_package()` or `execute_process()` calls in root.  
**Fix:** Defer expensive operations to module CMakeLists or use ccache / sccache.

---

## See Also

- `cmake/helpers.cmake` — Available macros and functions
- `CMakePresets.json` — Preset definitions for common build scenarios
- `.github/workflows/` — CI/CD usage of CMake commands
- `INSTALLATION.md` — End-user installation via CPack

---

**Last Updated:** 2026-07-18 (Phase 1)
