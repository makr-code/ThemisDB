# Copilot Instructions for C++ Projects

## Goals

- Keep code production-ready.
- Keep behavior explicit and testable.
- Keep docs synchronized with public APIs.

## Mandatory C++ rules

- Use RAII for resource ownership.
- Avoid raw pointers in public APIs.
- Prefer modern C++ (for example `std::string_view`, `std::span`, `std::unique_ptr`).
- Keep functions focused and side effects explicit.

## Documentation rules

- Every new or changed public API must include Doxygen comments.
- Document `@brief`, `@param`, `@return`, and failure behavior.
- Update docs in the same PR as code changes.

## Refactoring rules

- For C/C++ symbol changes, use semantic symbol tools instead of text-only search.
- Check references and call hierarchy before signature updates.

## Simplicity contract

- Prefer readable loops and straightforward control flow.
- Reject unnecessary abstractions.
- Optimize only after profiling evidence.
