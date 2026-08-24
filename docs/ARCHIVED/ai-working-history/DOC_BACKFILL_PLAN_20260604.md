# Doxygen Backfill Plan

## Goal
- Improve public C++ API documentation in core headers so Doxygen output becomes more complete and consistent.

## Initial Batch
- [ ] Enrich docs in `include/core/index_initialization.h`
- [ ] Enrich docs in `include/core/storage_initialization.h`
- [ ] Enrich docs in `include/core/config_validator.h`

## Acceptance Criteria
- Public builder APIs document purpose, parameters, return values, and failure behavior.
- Validation helpers document edge cases and production-mode differences.
- Changes remain documentation-only and do not alter runtime behavior.

## Validation
- Run a Doxygen syntax/build check after the edits.
- Review diff for comment-only changes.