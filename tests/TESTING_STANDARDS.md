# ThemisDB Testing Standards

Status: Canonical
Scope: All C++ test files under tests/ and related test registration in CMake.

## 1. Purpose

This document defines binding standards for:
- CTest registration and execution behavior
- GoogleTest file structure and naming
- Focused test targets and stability requirements

If another tests document conflicts with this file, this file wins for test
structure and execution rules.

## 2. CTest Standard (Binding)

### 2.1 Registration

- Every actively maintained test binary must be registered in CTest.
- Each CTest test name must be unique and deterministic.
- No active CTest entry may reference a missing executable.
- Tests intended for regular runs must not rely on EXCLUDE_FROM_ALL.

### 2.2 Naming

- Use a consistent CTest naming scheme by module and purpose.
- Focused tests must be clearly identifiable by name and mapped target.
- Avoid ambiguous aliases that hide the real binary under test.

### 2.3 Properties

- Set explicit timeout values for all non-trivial tests.
- Use labels consistently (module, phase, risk class, release_critical if needed).
- Enable output-on-failure behavior in standard run flows.

### 2.4 Validation

Minimum validation gates for structural health:
- ctest --output-on-failure must run without registration errors.
- No "Could not find executable" for active tests.
- Focused test mappings must resolve to fresh binaries, not stale artifacts.

## 3. GoogleTest Standard (Binding)

### 3.1 File Structure

- Include order must be consistent within each test file.
- Use TEST, TEST_F, and TEST_P according to the fixture/parameterization need.
- Shared helpers must be in a local namespace (or anonymous namespace).
- Avoid global helper symbol collisions across multiple test files.

### 3.2 Naming and Semantics

- Test suite names describe the unit or behavior under test.
- Test case names describe expected behavior, not implementation details.
- Keep behavior assertions stable across platforms where contract allows.

### 3.3 Assertion Quality

- Prefer assertions with actionable failure diagnostics.
- Avoid fragile temporary-lifetime or undefined-behavior patterns.
- Do not hide failures through silent retries inside test logic.

### 3.4 Focused Binaries

- Focused binaries must have explicit scope and corresponding CTest mapping.
- If a CTest case points to a focused binary, that binary must be built before run.
- Focused filters should target the intended subset only.

## 4. CMake Registration Policy

- Module tests should be registered in module-local CMake files where possible.
- Root-level registrations are allowed only when target ownership is clear.
- Registration helpers should be preferred over ad-hoc per-test boilerplate.

## 5. Migration Rules for Existing Tests

- When touching an existing test file, migrate it to this standard in the same PR.
- Resolve ODR-prone helper patterns during migration.
- Keep migration behavior-preserving unless a defect fix is intentional.

## 6. Legacy and Historical Documents

Older analysis or summary documents in tests/ remain useful evidence, but are
non-canonical for current standards unless explicitly aligned with this file.

## 7. Verification Checklist

- [ ] CTest registration is complete and executable-resolved
- [ ] Naming follows module-consistent scheme
- [ ] Timeout and labels are set consistently
- [ ] GoogleTest file structure follows this standard
- [ ] Helper symbols are scoped to avoid collisions
- [ ] Focused test mappings are correct and reproducible
