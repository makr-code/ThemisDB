# chaos — MODULE_GAPS.md (Phase 5 Verified)

This file documents all documentation and code quality gaps in the **chaos** module, as identified by the gap scanner (Phase 5 with external submodule filtering).

## Summary

- **Total Gaps**: 66
- **Status**: Verified (Phase 1: file existence, Phase 2: classification, Phase 5: external module filtering)
- **Last Updated**: C:\Projects\ThemisDB (L0 full scan with Phase 5)

### By Severity

- **CRITICAL**: 1
- **HIGH**: 2
- **MEDIUM**: 61
- **LOW**: 2

### By Type

- copy_overhead: 1
- exception_in_destructor: 1
- lock_contention: 1
- module_doc_linkset_drift: 2
- scope_mismatch: 59
- todo_as_productionlogic: 2

## Top 20 Gaps

- [exception_in_destructor] chaos_framework.cpp:207 (CRITICAL)
- [lock_contention] chaos_framework.cpp:258 (HIGH)
- [scope_mismatch] chaos_framework.cpp:276 (HIGH)
- [todo_as_productionlogic] chaos_framework.cpp:7 (MEDIUM)
- [todo_as_productionlogic] chaos_framework.cpp:15 (MEDIUM)
- [scope_mismatch] chaos_framework.cpp:57 (MEDIUM)
- [scope_mismatch] chaos_framework.cpp:59 (MEDIUM)
- [scope_mismatch] chaos_framework.cpp:63 (MEDIUM)
- [scope_mismatch] chaos_framework.cpp:67 (MEDIUM)
- [scope_mismatch] chaos_framework.cpp:71 (MEDIUM)
- [scope_mismatch] chaos_framework.cpp:76 (MEDIUM)
- [scope_mismatch] chaos_framework.cpp:79 (MEDIUM)
- [scope_mismatch] chaos_framework.cpp:89 (MEDIUM)
- [scope_mismatch] chaos_framework.cpp:91 (MEDIUM)
- [scope_mismatch] chaos_framework.cpp:99 (MEDIUM)
- [scope_mismatch] chaos_framework.cpp:100 (MEDIUM)
- [scope_mismatch] chaos_framework.cpp:104 (MEDIUM)
- [scope_mismatch] chaos_framework.cpp:106 (MEDIUM)
- [scope_mismatch] chaos_framework.cpp:116 (MEDIUM)
- [scope_mismatch] chaos_framework.cpp:119 (MEDIUM)

... and 46 more gaps.

---

**Phase 5 Verification Notes**: External GitHub submodules (llama.cpp, whisper.cpp, vcpkg, etc.) are explicitly excluded from this analysis via Phase 5 filtering. This ensures all gaps are from themis_core (100% scope accuracy).
