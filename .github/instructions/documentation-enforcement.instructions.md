---
description: "Use when editing C/C++ files to enforce mandatory documentation quality without CI gating dependencies."
applyTo: "**/*.{c,cc,cpp,cxx,h,hh,hpp,hxx,ipp,tpp}"
---

# C++ Documentation Enforcement Instructions (Doxygen-aware, no CI gate dependency)

Documentation is part of the definition of done.

## Mandatory Rules

- ALWAYS add or update API-facing documentation for new or changed public interfaces.
- Each documented interface must include: purpose, parameter expectations, return behavior (if applicable), and failure/edge-case behavior.
- Prefer Doxygen-compatible API comments for in-code public interface docs (`@brief`, `@param`, `@return`, `@throws` as applicable).
- For templates and constrained APIs, document the semantic requirements and constraints in plain language; include Doxygen tags such as `@tparam`/`@requires` when used in-code.
- Comments must capture intent (**why/constraints/trade-offs**), not only restate implementation details.
- When refactoring behavior, update existing documentation in the same change.
- Document error/edge-case behavior explicitly (invalid input, empty state, cancellation, timeout).

## Review Expectations

- Missing or stale API documentation is a quality defect.
- PR review must confirm documentation synchronization for behavior changes.
- Keep examples and module-level docs aligned with API behavior changes.
