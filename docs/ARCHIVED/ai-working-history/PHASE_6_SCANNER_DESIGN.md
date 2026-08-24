# Phase 6 Scanner Design — Extended Gap Analysis Architecture

**Date:** 2026-05-19  
**Status:** DESIGN PHASE  
**Target:** Q3 2026  
**Estimated LOC:** 1,480 total across 5 new scanners  
**Expected Gap Increase:** +6,000–10,000 (20–30% more than Phase 1-5)

---

## Overview

Phase 6 extends the gap scanning suite from 13 to 18 scanners, targeting advanced C++ patterns and system-level concerns not covered by Phase 1-5 detection.

### Phase 6 Scanners (5 new)
| ID | Name | Focus | Patterns | Est. LOC | Complexity |
|----|------|-------|----------|----------|------------|
| P6-1 | ABI Safety & Memory Layout | Padding, alignment, layout stability | 8-10 | 320 | HIGH |
| P6-2 | Const Correctness & API Design | const-correctness violations, mutable patterns | 12-15 | 380 | HIGH |
| P6-3 | Template Meta-Programming | SFINAE, concepts, template misuse | 10-12 | 350 | MEDIUM |
| P6-4 | Build System Hardening | CMake correctness, linker flags, dependencies | 6-8 | 280 | MEDIUM |
| P6-5 | Ownership & Lifetime Semantics | Move semantics edge cases, lifetime violations | 14-18 | 370 | CRITICAL |

---

## Detailed Scanner Specifications

### P6-1: ABI Safety & Memory Layout Scanner (320 LOC)

**Purpose:** Detect ABI-breaking changes, padding assumptions, and memory layout violations

**Patterns (8-10 detection rules):**
1. **Implicit Padding**: Struct members with different alignments causing hidden padding
   - Pattern: `struct { int x; char c; int y; }` → 7 bytes padding between c/y
   - Fix: Reorder fields by alignment or explicit padding documentation

2. **Virtual Base Class Offsets**: Diamond inheritance with virtual bases
   - Pattern: Multiple inheritance paths to same virtual base
   - Fix: Document layout guarantees or use composition

3. **Pack Directives**: `#pragma pack` usage affecting ABI across compilation units
   - Pattern: `#pragma pack(1)` inconsistency between headers
   - Fix: Use `alignas()` consistently, document packing rationale

4. **POD vs Non-POD Transitions**: Adding virtual functions to previously POD types
   - Pattern: Plain struct → adding `virtual ~Dtor()`
   - Fix: Version the type or use separate interface class

5. **Bitfield ABI**: Bitfield layout assumptions (undefined across compilers)
   - Pattern: Relying on bitfield packing across platforms
   - Fix: Use bitmasks or `std::bitset` instead

6. **std::vector Layout Assumptions**: Assuming contiguous layout in headers
   - Pattern: Direct memory access assuming std::vector internals
   - Fix: Use `.data()` and `.size()` explicitly

7. **Alignment Attribute Loss**: Cast-through misaligned pointers
   - Pattern: `int* ptr = (int*)misaligned_buffer;` accessing aligned data
   - Fix: Use `std::launder()` or byte-addressed access

8. **Hidden Offset Dependencies**: Assumptions about member offsets
   - Pattern: `offsetof()` assertions in tests or serialization code
   - Fix: Use reflection libraries or explicit serialization

**Expected Gaps:** 800–1,200  
**Estimated Complexity:** HIGH (struct layout analysis, compiler-specific ABI rules)

---

### P6-2: Const Correctness & API Design Scanner (380 LOC)

**Purpose:** Detect const-correctness violations and mutable state anti-patterns

**Patterns (12-15 detection rules):**
1. **Mutable Members in Const Methods**: Using `mutable` keyword for caching
   - Pattern: `mutable std::vector<> cache_; void refresh() const { cache_ = ...; }`
   - Gap: Thread-unsafe caching, const contract violation
   - Fix: Use `std::atomic<>` or move caching outside const APIs

2. **Logical Const Violations**: Modifying via `const_cast`
   - Pattern: `const_cast<T*>(this)->field = ...;` inside const method
   - Gap: Breaks const semantics, hides mutations
   - Fix: Use `mutable` or non-const method

3. **Non-const Reference Return**: Returning non-const ref from const method
   - Pattern: `int& ref() const { return field_; }`
   - Gap: Allows modification of logically const state
   - Fix: Return `const int&` or value

4. **Mutable Collection Returns**: Returning modifiable container from const method
   - Pattern: `std::vector<>& getItems() const { return items_; }`
   - Gap: Allows external mutation
   - Fix: Return `const std::vector<>&` or deep copy

5. **Pass-by-Const-Ref Failures**: Passing by value when const-ref is appropriate
   - Pattern: `void process(std::string s)` for read-only parameter
   - Gap: Unnecessary copies, performance regression
   - Fix: Use `std::string_view` or `const std::string&`

6. **Const Member Initialization**: Uninitialized const fields
   - Pattern: `const int value_;` without initialization
   - Gap: Undefined behavior
   - Fix: In-class initializer or member initializer list

7. **Bitwise vs Logical Const**: struct with non-const-propagating pointer member
   - Pattern: `int* ptr_; void modify() const { *ptr_ = ...; }`
   - Gap: Bitwise const but logically mutable
   - Fix: Use `std::unique_ptr<const int>` or explicit documentation

8. **Friend Access Violations**: Friends modifying private logically-const state
   - Pattern: Friend modifying mutable fields through const member functions
   - Gap: Const contract broken through friend access
   - Fix: Restrict friend access or redesign

9. **Const Correctness in Getters**: Duplicate const/non-const getters with different logic
   - Pattern: `int* get() { return ptr_; }` and `const int* get() const { return ptr_; }`
   - Gap: Inconsistent behavior, maintenance burden
   - Fix: Refactor to single template or SFINAE version

10. **Const Iterator Usage**: Returning non-const iterators from const methods
    - Pattern: `auto it = container_.begin()` in const method (non-const version)
    - Gap: Allows mutation through iterator
    - Fix: Use `auto it = container_.cbegin()`

11. **Volatile vs Const Interaction**: Mixing volatile and const incorrectly
    - Pattern: `const volatile int* ptr;` with unclear semantics
    - Gap: Confusing lifetime/access model
    - Fix: Document volatile semantics explicitly

12. **Method Chaining Const**: Returning non-const `*this` from const method
    - Pattern: `Builder& configure() const { return *this; }`
    - Gap: Allows mutation chain after const method
    - Fix: Use const-reference-qualified methods or redesign

**Expected Gaps:** 2,500–3,500  
**Estimated Complexity:** HIGH (semantic analysis, const propagation)

---

### P6-3: Template Meta-Programming Scanner (350 LOC)

**Purpose:** Detect template misuse, SFINAE errors, and concept violations

**Patterns (10-12 detection rules):**
1. **SFINAE Complexity**: Overly complex `std::enable_if` chains
   - Pattern: `typename = std::enable_if_t<std::is_integral_v<T> && ...>`
   - Gap: Reduced readability, C++20 concepts preferred
   - Fix: Use `requires` clauses or concepts

2. **Concept Violations**: Templates with implicit assumptions not enforced
   - Pattern: `template<typename T> void foo(T t) { t.size(); }` without concept
   - Gap: Compilation errors on types without `.size()`
   - Fix: Use `requires` or document concept

3. **Template Instantiation Explosion**: Parameterizing on too many types
   - Pattern: `template<typename A, typename B, typename C, ...>` 10+ parameters
   - Gap: Code bloat, compile times
   - Fix: Use type erasure or reduce template parameters

4. **Dependent Name Lookup**: Missing `typename` or `template` keywords
   - Pattern: `T::type x;` or `T::template Inner<U>`
   - Gap: Compilation errors with some compilers/instantiations
   - Fix: Add `typename` or `template` keywords

5. **Non-Type Template Parameter Misuse**: Incorrect NTTP constraints
   - Pattern: `template<int N>` used for values meant to be runtime
   - Gap: Explosion of template instantiations
   - Fix: Use runtime parameters or template specialization

6. **Template Specialization Ambiguity**: Partial specializations creating ambiguity
   - Pattern: Multiple partial specializations matching same types
   - Gap: Compilation errors or unexpected specialization selection
   - Fix: Order specializations by specificity or document precedence

7. **ADL (Argument-Dependent Lookup) Issues**: Unintended function discoveries
   - Pattern: `using std::swap; swap(a, b);` in namespace mixing code
   - Gap: Hidden dependencies on namespace pollution
   - Fix: Use explicit namespaces or CRTP

8. **Template Template Parameter Errors**: Incorrect template parameter requirements
   - Pattern: `template<template<typename> class>` but passing `template<typename, int>`
   - Gap: Compilation errors
   - Fix: Adjust template parameter or use concepts

9. **Type Traits Misuse**: Using removed/deprecated type traits
   - Pattern: `std::result_of<F(Args...)>` (deprecated in C++17, removed in C++20)
   - Gap: Compilation failures on new compilers
   - Fix: Use `std::invoke_result<>` instead

10. **Variadic Template Leaks**: Template parameter packs propagating incorrectly
    - Pattern: `template<typename... Args> void foo() { bar<Args...>(); }` with wrong count
    - Gap: Compilation errors or unexpected template instantiation
    - Fix: Use proper pack expansion or forwarding

11. **Recursive Template Instantiation**: Infinite template recursion
    - Pattern: `template<typename T> struct S : S<T> {};`
    - Gap: Compilation error: excessive template depth
    - Fix: Add base case specialization

12. **Constexpr Evaluation Limits**: Templates relying on constexpr beyond compiler limits
    - Pattern: Deep recursive constexpr templates exceeding `std::max_compile_time_recursion_depth`
    - Gap: Runtime fallback or compilation errors
    - Fix: Reduce recursion depth or use runtime evaluation

**Expected Gaps:** 600–1,000  
**Estimated Complexity:** MEDIUM (template analysis, requires compiler modeling)

---

### P6-4: Build System Hardening Scanner (280 LOC)

**Purpose:** Detect CMake errors, linker flag issues, and dependency problems

**Patterns (6-8 detection rules):**
1. **Missing Explicit Dependencies**: Implicit CMake dependency resolution
   - Pattern: `target_link_libraries(exe PRIVATE base)` without `base` being a declared target
   - Gap: Undefined linker behavior, missing libraries at link time
   - Fix: Explicitly declare and link all target dependencies

2. **Inconsistent Compiler Flags**: Different flags across build targets
   - Pattern: `add_compile_options(-O3)` in one file, `target_compile_options(exe PRIVATE -O2)` in another
   - Gap: Inconsistent optimization levels, performance variance
   - Fix: Centralize compiler flags in preset or toolchain

3. **Missing Debug Symbols**: Release builds without debug info
   - Pattern: No `-g` or `/Zi` in release CMake presets
   - Gap: Undebuggable production builds
   - Fix: Add `CMAKE_CXX_FLAGS_RELEASE` debug symbol configuration

4. **Undefined Symbol Visibility**: Missing visibility annotations
   - Pattern: No `set(CMAKE_CXX_VISIBILITY_PRESET hidden)` or symbols not marked
   - Gap: ABI instability, symbol conflicts in shared libraries
   - Fix: Set default visibility and mark public symbols explicitly

5. **Linker Script Issues**: Linker scripts not tracked or updated
   - Pattern: Custom linker scripts in build but not in source control or CMake
   - Gap: Build failures when scripts move or disappear
   - Fix: Add linker scripts to CMake with proper dependency tracking

6. **Unused Libraries Linked**: Unnecessary dependencies in link list
   - Pattern: `target_link_libraries(exe PRIVATE lib1 lib2 lib3)` but only lib1/lib2 used
   - Gap: Bloated binaries, potential license compliance issues
   - Fix: Remove unused libraries, audit dependencies

7. **Missing Sanitizer Flags**: Sanitizers not consistently applied
   - Pattern: `-fsanitize=address` in debug but not in test targets
   - Gap: Missed bugs in certain build configurations
   - Fix: Add sanitizer flags consistently in presets

8. **LTO (Link-Time Optimization) Misconfiguration**: LTO enabled inconsistently
   - Pattern: `set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)` for lib but not exe linking it
   - Gap: Linker errors or suboptimal optimization
   - Fix: Enable LTO consistently or disable globally

**Expected Gaps:** 200–400  
**Estimated Complexity:** MEDIUM (CMake parsing, build system understanding)

---

### P6-5: Ownership & Lifetime Semantics Scanner (370 LOC) — CRITICAL

**Purpose:** Detect lifetime violations, move semantics errors, and ownership transfer issues

**Patterns (14-18 detection rules):**
1. **Use-After-Move**: Using object after it's been moved from
   - Pattern: `T t; U u = std::move(t); use(t);`
   - Gap: Undefined behavior, logic error
   - Fix: Don't reuse moved-from objects or reset state

2. **Return of Local Reference**: Returning reference to local/temporary
   - Pattern: `const T& func() { T t; return t; }`
   - Gap: Dangling reference, UB
   - Fix: Return by value or use static/heap allocation

3. **Moved-From State Assumptions**: Assuming moved-from object is empty
   - Pattern: `T t = std::move(other); if (t.empty()) { ... }`
   - Gap: May not be empty (implementation-defined)
   - Fix: Document moved-from state guarantees

4. **Self-Move Assignment**: `a = std::move(a);` without guard
   - Pattern: Assignment without self-check
   - Gap: Data loss, resource leaks
   - Fix: Add `if (this != &other)` guard

5. **Move Constructor Not Noexcept**: Move constructor can throw
   - Pattern: `T(T&&) { ... }` without `noexcept`
   - Gap: std::vector/std::deque fallback to copy on exception
   - Fix: Make move constructor `noexcept` or document exception safety

6. **Lifetime Extension Failures**: Temporary lifetime not extended by binding
   - Pattern: `const T& ref = std::move(T());` then accessing outside immediate scope
   - Gap: ref becomes dangling
   - Fix: Bind to variable or ensure scope covers usage

7. **Copied Instead of Moved**: Copy when move is available/intended
   - Pattern: `T t; func(t);` instead of `func(std::move(t));`
   - Gap: Unnecessary copy, performance degradation
   - Fix: Use `std::move` for temporary forwarding

8. **RValue-Ref Member Storage**: Storing RValue reference as member
   - Pattern: `struct S { T&& ref_; }` storing rvalue reference
   - Gap: ref_ becomes dangling immediately after constructor
   - Fix: Store by value or unique_ptr<T>

9. **Returning Moved Parameter**: Returning moved parameter without moving
   - Pattern: `T func(T t) { return t; }` (no std::move)
   - Gap: Copy elision may fail on some compilers
   - Fix: Use `return std::move(t);` or rely on RVO

10. **Function Parameter Move**: Taking parameter by value then moving
    - Pattern: `void func(T t) { other = std::move(t); }`
    - Gap: Code intention unclear, missed optimization opportunity
    - Fix: Take by rvalue ref or document intentional move

11. **Const RValue Qualification**: Method taking const T&&
    - Pattern: `void method(const T&&) { ... }`
    - Gap: Can't move from const, method is useless
    - Fix: Remove const or take by const reference

12. **Lifetime Extension with Aggregates**: Initializer lists and lifetime
    - Pattern: `std::vector<int> v = {1, 2, 3}; int* ptr = v.data();` (ptr dangling after scope)
    - Gap: Dangling pointer
    - Fix: Extend lifetime or avoid raw pointers

13. **Move Assignment Operators**: Not handling moved-from state correctly
    - Pattern: `T& operator=(T&& other) { field_ = other.field_; }` (other.field_ still valid?)
    - Gap: Potential use-after-free if other is reused
    - Fix: Document moved-from state or reset other's fields

14. **Exception Safety in Move**: Move operations throwing exceptions
    - Pattern: `T(T&& other) noexcept { ptr_ = other.ptr_; other.ptr_ = acquire_new(); }` (acquire_new() throws)
    - Gap: Violates noexcept contract
    - Fix: Ensure all operations in noexcept move are exception-safe

15. **Forwarding Reference Lifetime**: Template forwarding references
    - Pattern: `template<typename T> void func(T&& t) { other = std::forward<T>(t); }`
    - Gap: Lifetime issues with temporaries
    - Fix: Document lifetime requirements or use concepts

16. **Returning Stack-Allocated Moved Object**: Return moved temporary
    - Pattern: `return std::move(local);` where local goes out of scope
    - Gap: RVO/NRVO may not apply, relying on move semantics
    - Fix: Ensure move semantics work correctly or use std::make_unique

17. **Moved Object in Containers**: Moving objects into containers with lifetime issues
    - Pattern: `std::vector<const T&> v; v.push_back(std::move(t));` (storing ref to moved obj)
    - Gap: References become invalid
    - Fix: Store by value or use smart pointers

18. **Default Move Behavior**: Relying on implicitly-generated move operations
    - Pattern: Class with non-movable member but implicit move generated
    - Gap: Compilation errors or unexpected behavior
    - Fix: Explicitly delete or define move operations

**Expected Gaps:** 3,500–5,000  
**Estimated Complexity:** CRITICAL (semantic lifetime analysis, hard to detect)

---

## Implementation Roadmap

### Sprint 1 (Week 1-2)
- [x] Implement P6-1 ABI Safety Scanner (320 LOC) — Done 2026-07-06
- [x] Implement P6-4 Build System Scanner (280 LOC) — Done 2026-07-06
- [x] Integrate P6-1 + P6-4 into gap_scanner_v3.py orchestrator
- Estimated gap increase: +500–800 gaps

### Sprint 2 (Week 3-4)
- [ ] Implement P6-2 Const Correctness Scanner (380 LOC)
- [ ] Estimated gap increase: +2,500–3,500 gaps

### Sprint 3 (Week 5-6)
- [ ] Implement P6-3 Template Meta-Programming Scanner (350 LOC)
- [ ] Estimated gap increase: +600–1,000 gaps

### Sprint 4 (Week 7-8)
- [ ] Implement P6-5 Ownership & Lifetime Semantics Scanner (370 LOC)
- [ ] Integration and testing
- [ ] Estimated gap increase: +3,500–5,000 gaps

### Phase 6 Completion
- [ ] Run complete Phase 1-6 pipeline (18 scanners)
- [ ] Generate aggregated report (expected: 165,000–185,000 total gaps)
- [ ] Update ROADMAP.md and GitHub issues
- [ ] Plan Phase 7 (optional: 5 more advanced scanners)

---

## Success Criteria

### Scanner Validation
- ✓ Each scanner produces deterministic output (same input → same gaps)
- ✓ False positive rate < 5% (validated on sample modules)
- ✓ Detection latency < 15 minutes for full codebase (65 modules)
- ✓ All patterns documented with examples and CWE mappings

### Gap Analysis
- ✓ Gap counts increase 20–30% from Phase 1-5 baseline (6,000–10,000 new gaps)
- ✓ Top gap producers identified by category
- ✓ Severity distribution estimated (CRITICAL, HIGH, MEDIUM)

### Documentation
- ✓ Phase 6 design finalized and approved
- ✓ Gap scanner suite documentation complete
- ✓ All 18 scanners integrated into orchestrator
- ✓ GitHub issues aggregated (1 Master + 18 Categories + 10 Modules = 29 total)

---

## Risk & Mitigation

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|-----------|
| P6-5 (Lifetime) Too Complex | HIGH | CRITICAL | Start with simple patterns; iterate on advanced ones |
| P6-1 (ABI) Compiler-Specific | HIGH | MEDIUM | Focus on portable patterns; flag compiler-specific |
| Phase 6 Gap Increase Exceeds Projections | MEDIUM | MEDIUM | Implement false positive filtering; tune pattern sensitivity |
| Integration Breaks Phase 1-5 Results | LOW | HIGH | Comprehensive regression testing before merge |

---

## Related Documents
- [PHASE_5_IMPLEMENTATION_COMPLETE.md](PHASE_5_IMPLEMENTATION_COMPLETE.md)
- [ROADMAP.md](../ROADMAP.md)
- [FUTURE_ENHANCEMENTS.md](../FUTURE_ENHANCEMENTS.md)

---

## Phase 1–4 Enhancement Patterns (Track 5, Q3–Q4 2026)

**Last Updated:** 2026-07-27 — Added as part of next-phase Track 5 planning.

These 12 patterns extend existing Phase 1–4 scanners with critical safety and security gaps
identified during post-GA review. Each maps to a CWE and has a projection of expected gap yield.

### Concurrency Patterns (C-series)

#### C-1: Data Race Detection

**Target scanner:** Phase 2 (Thread Safety Scanner) enhancement  
**CWE:** CWE-362 (Race Condition / Concurrent Execution with Shared Resource)  
**Expected gap yield:** 200–400 (server, llm, sharding are top-risk)

Patterns to detect:
1. **Unguarded shared write**: non-atomic write to shared state without lock in a function that is called from multiple threads based on class documentation
   - Pattern: member variable write without `std::lock_guard`/`std::unique_lock` where thread annotations exist
2. **Double-checked locking without `std::atomic`**: `if (!initialized_) { lock(); if (!initialized_) { ... } }` without atomic fence
3. **Callback-held lock**: holding a `std::mutex` across an external callback invocation (potential deadlock + race)

---

### Memory Safety Patterns (M-series)

#### M-1: Use-After-Free Detection

**Target scanner:** Phase 3 (Memory Safety Scanner) enhancement  
**CWE:** CWE-416 (Use After Free)  
**Expected gap yield:** 150–300

Patterns to detect:
1. **Shared pointer captured lambda escape**: `std::shared_ptr<T>` captured in a lambda that outlives the owning scope after `std::move` of the shared_ptr
2. **Raw pointer returned after container invalidation**: returning `.data()` of a `std::vector<>` and then modifying the vector in the same scope
3. **Erased iterator dereferenced**: iterator used after `erase()`/`clear()` without reassignment

#### M-2: Double-Free Detection

**Target scanner:** Phase 3 (Memory Safety Scanner) enhancement  
**CWE:** CWE-415 (Double Free)  
**Expected gap yield:** 50–100

Patterns to detect:
1. **Manual `delete` on shared resource**: calling `delete ptr` on a pointer that is also held by a `std::shared_ptr`
2. **Copy constructor deletes**: non-rule-of-five class with user-defined destructor calling `delete` but missing copy-assignment prohibition
3. **`free()` on `new`-allocated memory or vice versa**: mixing C and C++ allocation

---

### Security Patterns (S-series)

#### S-1: Hardcoded Secrets Detection

**Target scanner:** Phase 4 (Security Pattern Scanner) enhancement  
**CWE:** CWE-798 (Use of Hard-coded Credentials)  
**Expected gap yield:** 30–80

Patterns to detect:
1. **String literal matching secret heuristics**: variables named `password`, `secret`, `api_key`, `token` assigned a non-empty string literal
2. **Base64-looking literals in headers**: strings matching `[A-Za-z0-9+/]{40,}={0,2}` in `.h` files outside test directories
3. **Default credentials in config structs**: struct fields with `default_password`, `default_key` initialized to non-empty literals

#### S-2: Crypto Weakness Detection

**Target scanner:** Phase 4 (Security Pattern Scanner) enhancement  
**CWE:** CWE-327 (Use of Broken or Risky Cryptographic Algorithm)  
**Expected gap yield:** 20–50

Patterns to detect:
1. **MD5/SHA-1 for security**: calls to `MD5_Init`, `SHA1_Init`, `EVP_md5()`, `EVP_sha1()` outside checksum/non-security contexts
2. **ECB mode cipher**: `EVP_aes_128_ecb()` or equivalent ECB mode selection
3. **Static IV/nonce**: IV initialized from a constant or zero array for AES-GCM/CBC

#### S-3: Injection Vulnerability Detection

**Target scanner:** Phase 4 (Security Pattern Scanner) enhancement  
**CWE:** CWE-89 (SQL/Command Injection), CWE-78 (OS Command Injection)  
**Expected gap yield:** 40–100

Patterns to detect:
1. **String concatenation into query strings**: `query = "SELECT ... WHERE id = " + user_input` without parameterization
2. **`system()`/`popen()` with non-constant argument**: `system(cmd.c_str())` where `cmd` is built from external input
3. **`sprintf`/`snprintf` format string from external source**: format string is a variable rather than a string literal

---

### Deployment and Gate

| Pattern group | Target scanner phase | Expected yield | Q3 2026 target |
|---|---|---|---|
| C-1 (race conditions) | Phase 2 enhancement | 200–400 gaps | Q3 2026 |
| M-1 (use-after-free) | Phase 3 enhancement | 150–300 gaps | Q3 2026 |
| M-2 (double-free) | Phase 3 enhancement | 50–100 gaps | Q3 2026 |
| S-1 (hardcoded secrets) | Phase 4 enhancement | 30–80 gaps | Q3 2026 |
| S-2 (crypto weakness) | Phase 4 enhancement | 20–50 gaps | Q3 2026 |
| S-3 (injection) | Phase 4 enhancement | 40–100 gaps | Q3 2026 |

**Total projected new gaps from enhancements:** 490–1,030 additional on top of Phase 6 base (+6,000–10,000).

**Gate (Track 5):** All 12 pattern detectors deployed with ≤ 5% false-positive rate (validated on
Phase 1–4 known-good samples); `by_module.md` updated with scanner evidence per top-risk module.
