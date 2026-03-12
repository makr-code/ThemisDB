### Context

This issue implements the roadmap item 'Module Loader Implementation' for the themis domain. It is sourced from the consolidated roadmap under 🔴 Critical Priority and targets milestone v1.7.0.

Primary detail section: Module Loader Implementation

### Goal

Deliver the scoped changes for Module Loader Implementation in src/themis/ and complete the linked detail section in a release-ready state for v1.7.0.

### Detailed Scope

### Module Loader Implementation
**Priority:** Critical  
**Target Version:** v1.7.0

Implement secure module loading with signature verification.

**Components:**
- `ModuleLoader` class implementation
- `ModuleSecurityVerifier` class implementation
- Platform-specific loading (Windows/Linux)
- Hash and signature verification
- Audit logging

**Implementation Files:**
- `src/themis/module_loader.cpp` - Main implementation
- `src/themis/module_loader_win32.cpp` - Windows-specific
- `src/themis/module_loader_linux.cpp` - Linux-specific
- `src/themis/module_security.cpp` - Security verification

**Security Features:**
```cpp
// Implementation priorities
1. SHA-256 hash verification ✓
2. Signature verification (X.509/GPG) ✓
3. Whitelist/blacklist support ✓
4. Zone.Identifier detection (Windows) ✓
5. Authenticode verification (Windows) ✓
6. Audit logging ✓
```

---

### Acceptance Criteria

- [ ] `ModuleLoader` class implementation
- [ ] `ModuleSecurityVerifier` class implementation
- [ ] Platform-specific loading (Windows/Linux)
- [ ] Hash and signature verification
- [ ] Audit logging
- [ ] `src/themis/module_loader.cpp` - Main implementation
- [ ] `src/themis/module_loader_win32.cpp` - Windows-specific
- [ ] `src/themis/module_loader_linux.cpp` - Linux-specific
- [ ] `src/themis/module_security.cpp` - Security verification

### Relationships

- Roadmap row: #4 (🔴 Critical Priority)
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/themis/FUTURE_ENHANCEMENTS.md#module-loader-implementation
- Source key: roadmap:4:themis:v1.7.0:module-loader-implementation

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:4:themis:v1.7.0:module-loader-implementation -->
<!-- roadmap-ref: row=4;module=themis;target=v1.7.0 -->
<!-- roadmap-detail: src/themis/FUTURE_ENHANCEMENTS.md#module-loader-implementation -->
