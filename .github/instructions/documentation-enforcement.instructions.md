---
description: "Use when editing C/C++ files to enforce mandatory documentation quality without CI gating dependencies."
applyTo: "**/*.{c,cc,cpp,cxx,h,hh,hpp,hxx,ipp,tpp}"
---

# C++ Documentation Enforcement Instructions (Doxygen-aware, no CI gate dependency)

Documentation is part of the definition of done.

## Mandatory Rules

- Always add or update API-facing documentation for new or changed public interfaces.
- Each documented interface must include: purpose, parameter expectations, return behavior (if applicable), and failure/edge-case behavior.
- Prefer Doxygen-compatible API comments for in-code public interface docs (`@brief`, `@param`, `@return`, `@throws` as applicable).
- For templates and constrained APIs, document the semantic requirements and constraints in plain language; include Doxygen tags such as `@tparam`/`@requires` when used in-code.
- Comments must capture intent (**why/constraints/trade-offs**), not only restate implementation details.
- When refactoring behavior, update existing documentation in the same change.
- Document error/edge-case behavior explicitly (invalid input, empty state, cancellation, timeout).
- Public APIs that have ownership/lifetime semantics, thread-safety guarantees, or cancellation/timeout behavior must state those contracts in the API docs explicitly.
- If a function is deprecated or superseded, mark it in both source comments and Doxygen with `@deprecated` and the migration path.
- Simulation/Stub/Mockup paths are forbidden without explicit human approval and explicit human marking.

## Mandatory Doxygen Rules for Public C++ APIs

The repository requires Doxygen-compatible documentation for all public C++ interfaces and for any changed public API surface. This is a required quality gate, not a style preference.

- Public API comments must appear directly above the declaration and must be readable by Doxygen without requiring extra tooling.
- Use `@brief` for the summary of intent and behavior.
- Use `@param` for every input argument, including semantic constraints, valid ranges, empty-state expectations, ownership expectations, and failure modes.
- Use `@return` whenever the function returns a value, status, pointer, handle, or result object.
- Use `@throws` for exceptions or explicit failure modes that callers must handle; if the API uses `Result<T>` or status codes instead of exceptions, document the error conditions in `@return` and/or `@note`.
- Use `@note` for non-obvious invariants, thread-safety guarantees, performance expectations, ownership/lifetime constraints, and required caller behavior.
- Use `@deprecated` when a function, type, or overload is superseded; always include the recommended replacement.
- Use `@tparam` and `@requires` for templates and concepts when semantic constraints matter to callers.
- For interfaces with cancellation, timeout, retry, or concurrency semantics, document the contract explicitly, including default behavior and failure/timeout handling.
- For APIs with non-trivial ownership or lifetime semantics, document who owns the object, when it becomes invalid, and what caller responsibilities exist.
- Do not document only the implementation; document why the API exists, what its contract is, and what edge cases a caller must consider.

## Doxygen Review Checklist

Before merging a public C++ API or API refactor, verify all of the following:

- The API has a Doxygen block or equivalent API-facing comment.
- `@brief` explains the purpose and contract, not just the name.
- Every parameter is documented with `@param` and its valid/invalid expectations.
- Every return path is documented with `@return` or equivalent status semantics.
- Exceptions or failure modes are covered by `@throws`, `@return`, or `@note`.
- Edge cases are documented: null/empty inputs, invalid ranges, cancellation, timeout, thread-safety assumptions, and ownership/lifetime rules.
- Deprecated or replaced APIs include `@deprecated` and a migration hint.
- Template or concept constraints are documented with `@tparam`/`@requires` where relevant.
- The documentation reflects the current behavior, not stale pre-refactor assumptions.

## Good and Bad Examples

Good example:

```cpp
/// @brief Validates and stores a batch of user records.
/// @param records Input records to validate; each entry must be non-null and have a
///         stable unique key. Empty records are ignored without error.
/// @param max_batch_size Maximum number of records processed in one call; must be > 0.
/// @return Number of records accepted and stored, or 0 if the input is empty.
/// @throws std::invalid_argument if max_batch_size is zero or negative.
/// @note This API is not thread-safe; callers must serialize access to the writer.
/// @note The returned handle remains valid until the underlying writer is destroyed.
[[nodiscard]] std::size_t storeRecords(std::span<const Record> records, std::size_t max_batch_size);
```

Bad example:

```cpp
// Stores records.
std::size_t storeRecords(std::span<const Record> records, std::size_t max_batch_size);
```

This is not acceptable because it omits contract details, failure behavior, ownership/lifetime expectations, and thread-safety semantics.

## Mandatory Human Marking For Approved Non-Production Paths

If a human explicitly approves a Simulation/Stub/Mockup path, documentation and code comments must include:

```cpp
// NON-PRODUCTION PATH (Simulation/Stub/Mockup)
// Reason: <why needed>
// Activation: <when active>
// Production Delta: <difference to primary path>
// Approved By: <human name/role + ticket/PR reference>
// Removal Target: <date or milestone>
```

## Review Expectations

- Missing or stale API documentation is a quality defect.
- PR review must confirm documentation synchronization for behavior changes.
- Keep examples and module-level docs aligned with API behavior changes.
