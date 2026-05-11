---
description: "Use when editing C/C++ files to enforce documentation quality and Doxygen completeness."
applyTo: "**/*.{c,cc,cpp,cxx,h,hh,hpp,hxx,ipp,tpp}"
---

# C++ Documentation Enforcement Instructions

Documentation is part of the definition of done.

## Mandatory Rules

- ALWAYS generate/update Doxygen comments for new or changed public APIs.
- Required tags for public APIs: `@brief`, `@param` (all params), `@return` (if non-void), `@throws` (if applicable).
- Use `@tparam` for template parameters.
- Use `@requires` when C++20 concepts or semantic preconditions exist.
- Comments must capture intent (**why/constraints/trade-offs**), not only restate implementation details.
- When refactoring behavior, update existing comments in the same change.
- Document error/edge-case behavior explicitly (invalid input, empty state, cancellation, timeout).

## Review Gate Expectations

- Missing or stale API documentation is a quality defect.
- PR checklist must confirm documentation synchronization for behavior changes.
- Keep examples and module-level docs aligned with API behavior changes.
