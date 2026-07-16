### Context

This issue implements the roadmap item 'Build Reproducibility Implementation' for the themis domain. It is sourced from the consolidated roadmap under 🟠 High Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.7.0.

Primary detail section: Build Reproducibility Implementation

### Goal

Deliver the scoped changes for Build Reproducibility Implementation in src/themis/ and complete the linked detail section in a release-ready state for v1.7.0.

### Detailed Scope

### Build Reproducibility Implementation
**Priority:** High  
**Target Version:** v1.7.0

Ensure all builds are reproducible for security auditing.

**Implementation:**
```cpp
// build_info.cpp additions
namespace themis::build_info {

struct ReproducibilityInfo {
    std::string git_commit;
    std::string git_commit_date;
    std::string build_host;
    std::string build_user;
    std::map<std::string, std::string> dependencies;
    std::string expected_hash;
};

ReproducibilityInfo getReproducibilityInfo();
bool verifyBuildHash(const std::string& expected_hash);
void exportBuildManifest(const std::string& output_path);

} // namespace themis::build_info
```

**CMake Integration:**
```cmake
# Capture build metadata
execute_process(
    COMMAND git log -1 --format=%H
    OUTPUT_VARIABLE GIT_COMMIT
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

execute_process(
    COMMAND git log -1 --format=%ci
    OUTPUT_VARIABLE GIT_COMMIT_DATE
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Embed in build
target_compile_definitions(themis-base PRIVATE
    THEMIS_GIT_COMMIT="${GIT_COMMIT}"
    THEMIS_GIT_COMMIT_DATE="${GIT_COMMIT_DATE}"
    THEMIS_BUILD_HOST="${CMAKE_HOST_SYSTEM_NAME}"
)
```

---

### Acceptance Criteria

- [ ] Implement the scoped changes described in the linked detail section.
- [ ] Add or update tests that verify the intended behaviour.

### Relationships

- Roadmap row: #114 (🟠 High Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/themis/FUTURE_ENHANCEMENTS.md#build-reproducibility-implementation
- Source key: roadmap:114:themis:v1.7.0:build-reproducibility-implementation

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:114:themis:v1.7.0:build-reproducibility-implementation -->
<!-- roadmap-ref: row=114;module=themis;target=v1.7.0 -->
<!-- roadmap-detail: src/themis/FUTURE_ENHANCEMENTS.md#build-reproducibility-implementation -->
