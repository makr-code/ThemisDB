### Context

This issue implements the roadmap item 'Modular Build System' for the themis domain. It is sourced from the consolidated roadmap under 🔴 Critical Priority and targets milestone v1.7.0.

Primary detail section: Modular Build System

### Goal

Deliver the scoped changes for Modular Build System in src/themis/ and complete the linked detail section in a release-ready state for v1.7.0.

### Detailed Scope

### Modular Build System
**Priority:** Critical  
**Target Version:** v1.7.0

Migrate from monolithic to modular build architecture.

**Tasks:**
1. Extract core implementations to `src/themis/`
2. Create `libthemis-base.so` / `themis-base.dll`
3. Update CMakeLists.txt for modular builds
4. Add export macros to all public APIs
5. Test on Windows and Linux

**Migration Steps:**
```bash
# Step 1: Extract implementations
git mv src/core/build_info.cpp src/themis/build_info.cpp
git mv src/security/license_manager.cpp src/themis/license_info.cpp

# Step 2: Update CMakeLists.txt
add_library(themis-base SHARED
    src/themis/build_info.cpp
    src/themis/license_info.cpp
    src/themis/module_loader.cpp
)

# Step 3: Update other modules
target_link_libraries(themis-storage themis-base)
target_link_libraries(themis-query themis-base)
```

---

### Acceptance Criteria

- [ ] Extract core implementations to `src/themis/`
- [ ] Create `libthemis-base.so` / `themis-base.dll`
- [ ] Update CMakeLists.txt for modular builds
- [ ] Add export macros to all public APIs
- [ ] Test on Windows and Linux

### Relationships

- Roadmap row: #3 (🔴 Critical Priority)
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/themis/FUTURE_ENHANCEMENTS.md#modular-build-system
- Source key: roadmap:3:themis:v1.7.0:modular-build-system

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:3:themis:v1.7.0:modular-build-system -->
<!-- roadmap-ref: row=3;module=themis;target=v1.7.0 -->
<!-- roadmap-detail: src/themis/FUTURE_ENHANCEMENTS.md#modular-build-system -->
