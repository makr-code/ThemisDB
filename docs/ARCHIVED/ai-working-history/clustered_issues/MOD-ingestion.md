# [ingestion] Fix 153 unimplemented paths + 178 total gaps

**Issue Type:** Implementation Gap Audit  
**Priority:** CRITICAL  
**Affected Modules:** 1  
**Total Gaps:** 178  

## Summary

Implementation gap audit for `ingestion` module.

## Gap Breakdown

- **Unimplemented:** 153
- **Stub:** 24
- **Todo:** 1

## Example Gaps

- `src\ingestion\entity_assembler.cpp:79` — if (!std::regex_search(text, m, re_law)) return {};
- `src\ingestion\ingestion_coordinator.cpp:197` — return {};
- `src\ingestion\cdc_connector.cpp:567` — // STUB/SIMULATION NOTE:
- ... and 2 more

## Acceptance Criteria

1. Reduce gaps from 178 to <35
2. All critical unimplemented paths resolved or documented
3. STUB markers updated with expiration criteria

## Related

- [Gap Scan Results](ai_working/) — Full scan reports
- [ROADMAP.md](ROADMAP.md) — Project roadmap
