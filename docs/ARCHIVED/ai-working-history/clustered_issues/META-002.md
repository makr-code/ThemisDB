# Audit STUB/MOCK/SIMULATION markers: add expiration & removal plans

**Issue Type:** Implementation Gap Audit  
**Priority:** HIGH  
**Affected Modules:** 49  
**Total Gaps:** 384  

## Summary


This meta-issue ensures all STUB/MOCK/SIMULATION markers follow the documentation standard from COPILOT_INSTRUCTIONS.md.

**Standard Template (from .github/copilot-instructions.md § 8):**
```cpp
// STUB/SIMULATION NOTE:
// Purpose: <why this non-production path exists>
// Activation: <build flag/runtime condition/test-only gate>
// Production Delta: <how behavior differs from production>
// Removal Plan: <when/how this path will be removed>
```

**Scope:** 384 stub markers across 49 modules.
**Target:** 100% of STUB/MOCK markers comply with standard.


## Gap Breakdown

- **Stub:** 384

## Example Gaps

- `src\acceleration\ai_hardware_dispatcher.cpp:606` — // STUB/SIMULATION NOTE:
- `src\acceleration\graphics_backends.cpp:894` — // STUB/SIMULATION NOTE:
- `src\acceleration\graphics_backends.cpp:913` — // STUB/SIMULATION NOTE:
- ... and 2 more

## Acceptance Criteria

1. All STUB/MOCK/SIMULATION markers include 4-line template
2. Each marker has documented expiration date or condition
3. Removal plan is specific and tracked (e.g., 'Remove after feature X ships')
4. Tests explicitly verify stub behavior vs production behavior
5. No stubs without clear purpose or removal plan remain

## Related

- [Gap Scan Results](ai_working/) — Full scan reports
- [ROADMAP.md](ROADMAP.md) — Project roadmap
