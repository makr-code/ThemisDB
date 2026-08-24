# ThemisDB Gap Scanner v3 — Comprehensive Analysis & Enhancement Roadmap

**Date:** May 19, 2026  
**Analysis Scope:** 8 existing scanners (Phase 1-4), 31,720 gaps detected across 60 modules  
**Objective:** Identify coverage gaps, propose improvements, and roadmap Phase 5+ enhancements

---

## Executive Summary

The Gap Scanner v3 suite provides broad coverage across 8 critical C++ quality domains (security, memory, reliability, concurrency, RAII, containers, platform, performance). However, analysis reveals **significant false-negative rates** and **missing CERT/CWE/MISRA patterns** that can be addressed through:

1. **Existing Scanner Improvements** (Higher precision, better control-flow analysis)
2. **5-7 New Scanner Tasks** (Phase 5-6) targeting uncovered vulnerability classes
3. **Framework Enhancements** (Context preservation, multi-line pattern matching)

**Estimated 40-60% gap closure** with Phase 5 improvements; **Phase 6** targets remaining edge cases and MISRA compliance.

---

## Part 1: Existing Scanner Review & Coverage Assessment

### 1.1 Security Scanner (210 lines, 9 patterns)

**Current Coverage:**
- Unsafe functions: `strcpy`, `sprintf`, `gets`, `scanf`
- Hardcoded secrets: API keys, passwords, tokens, encryption keys
- SQL/command injection: String concatenation with user input
- Null dereference: Heuristic check for `->` without preceding `if`
- Error code checks: `status = func()` without validation
- Format string vulnerabilities

**Gaps & False Negatives:**
- ✗ No detection of **secondary/derived unsafe functions** (`strcat`, `sscanf`, `getenv`)
- ✗ Hardcoded secrets pattern is string-literal-only; misses `const char*` global arrays
- ✗ SQL injection requires explicit string concatenation (`+`); misses `fmt::format` with user input
- ✗ Null dereference heuristic fails on:
  - Complex control flow (if/else chains, nested conditions)
  - Pointer assignments from function returns without explicit check
  - Pointers checked in different scope (e.g., helper function)
- ✗ No detection of **off-by-one errors** in bounds checking
- ✗ No detection of **information disclosure** (logging secrets)
- ✗ No **regex DoS** pattern detection
- ✗ No **integer overflow** in size calculations

**Best-Practice Gaps:**
- CWE-200: Information Exposure (logging/debug output)
- CWE-327: Use of Broken Cryptography (hardcoded IV, weak hash)
- CWE-326: Inadequate Encryption Strength
- MISRA C++ 2008 Rule 0-1-10: NULL pointer dereferencing

---

### 1.2 Memory Safety Scanner (230 lines, 8 patterns)

**Current Coverage:**
- Raw `new`/`delete` without RAII
- Unchecked `malloc`/`calloc`/`realloc`
- Pointer arithmetic without bounds
- Array bounds violations (static analysis)
- `delete` without `nullptr` assignment
- Bidirectional `shared_ptr` reference cycles
- Manual memory cleanup instead of RAII

**Gaps & False Negatives:**
- ✗ **Smart pointer initialization** not validated (e.g., `unique_ptr<T> p; ... p = new T()` — should use `make_unique`)
- ✗ **Memory leaks from exceptions** not caught (RAII in constructor before full initialization)
- ✗ **Use-after-free** only detects `delete` without `nullptr`; misses:
  - Function return of stack address
  - Return of freed pointer from library call
  - Pointer passed to async function, freed before callback
- ✗ **Double-free** not detected (two paths leading to same `delete`)
- ✗ **Ownership transfer** not tracked (when passing raw pointer, unclear if callee takes ownership)
- ✗ **Scoping issues** with stack objects passed to async contexts
- ✗ **Alloca stack overflow** not detected
- ✗ No detection of **buffer overread** (off-by-one in `memcpy(dst, src, len+1)`)

**Best-Practice Gaps:**
- CWE-190: Integer overflow in size calculations
- CWE-415: Double free
- CWE-416: Use-after-free
- CWE-762: Mismatched new/delete (placement new, array new/delete)
- CERT C++ DCL: Declare objects at appropriate scope
- MISRA A11-3-1: Member data must be private/protected

---

### 1.3 Reliability Scanner (210 lines, 7 patterns)

**Current Coverage:**
- Network calls without retry logic
- Blocking operations without timeout
- No circuit breaker pattern
- No graceful degradation fallback
- Uncaught exceptions
- No health checks
- No backoff strategy

**Gaps & False Negatives:**
- ✗ **Retry detection** is keyword-based ("retry", "loop"); misses:
  - Labeled loop with `goto` for retry
  - Functional retry (higher-order function)
  - Async retry framework calls
- ✗ **Timeout detection** looks for "timeout"/"ms"/"seconds"; misses:
  - Timeouts in function signatures (e.g., `wait_for(Duration)`)
  - Timeout from config/constant (no string match)
  - Timeouts in gRPC metadata/context
- ✗ **Circuit breaker** not detected (pattern is "circuit", "breaker" keywords)
- ✗ **Graceful degradation** only looks for exceptions; misses:
  - Error code return paths
  - Fallback service calls
  - Feature flags
- ✗ **Cascading failures** not modeled (failure A triggers failure B)
- ✗ **Health check** requires explicit keyword; misses:
  - Implicit health via status code
  - Ping/heartbeat patterns
- ✗ **Bulkhead isolation** not detected (thread pool, resource limits)

**Best-Practice Gaps:**
- CWE-391: Unchecked error condition
- CWE-231: Improper handling of extra parameters
- CERT C++: ERR30-C Temporary file race conditions
- MISRA C++ 8.0-8: Exception handling completeness

---

### 1.4 Concurrency Scanner (380 lines, 8 patterns)

**Current Coverage:**
- Data races (unprotected shared data)
- Lock ordering violations (nested locks → deadlock)
- Missing lock guards around mutex
- Race conditions in callbacks/async
- Thread-unsafe singleton access
- Memory ordering issues (`std::memory_order`)
- Double-lock patterns
- Condition variable races

**Gaps & False Negatives:**
- ✗ **Shared data detection** is keyword-based (mutable, static, global_, etc.); misses:
  - Class members accessed from multiple threads without synchronization
  - Non-obvious shared state (e.g., captured by lambda)
  - Thread-local storage not marked `thread_local`
- ✗ **Lock guard detection** requires explicit `lock_guard`/`unique_lock`; misses:
  - RAII wrapper classes around `pthread_mutex`
  - Custom lock classes
  - `std::scoped_lock` (C++17)
- ✗ **Deadlock detection** heuristic-based on nested locks; misses:
  - Non-nested but inconsistent lock order (A→B in one thread, B→A in another)
  - Locks acquired in function call chains (no local nesting visible)
- ✗ **Data race** from async function capturing mutable references not detected
- ✗ **Memory ordering** (acquire/release/seq_cst) patterns not validated
- ✗ **Spurious wakeup** loops for `condition_variable` not checked
- ✗ **Stale closure** in lambda/callback (captured by value but uses shared state)

**Best-Practice Gaps:**
- CWE-366: Race condition
- CWE-667: Improper locking
- CERT C++ CON: Concurrency
- MISRA C++ 5.2: Synchronization of threads
- C++ std: `std::scoped_lock` (C++17) preferred over nested `lock_guard`

---

### 1.5 RAII & Resource Management Scanner (350 lines, 8 patterns)

**Current Coverage:**
- File handle leaks (`FILE*` without `fclose`)
- Socket descriptor leaks without `close()`
- Database connection leaks
- Smart pointer misuse
- Exception-unsafe resource allocation
- Missing destructor
- Manual cleanup instead of RAII
- Resource scope issues

**Gaps & False Negatives:**
- ✗ **File handle detection** requires `fopen`/`fclose` keywords; misses:
  - `open()/close()` POSIX APIs
  - Stream-based file I/O without explicit close (implicit in destructor)
  - File handles returned from library calls (unclear ownership)
- ✗ **Socket leaks** detected via `socket()/close()` keywords; misses:
  - Platform-specific socket APIs (WSASocket on Windows)
  - Sockets created/managed by framework (e.g., listener threads)
  - Socket ownership transfer between objects
- ✗ **Database connection** detection is keyword-based; misses:
  - Custom connection pool implementations
  - Connection returned from factory (unclear lifetime)
  - Async operations (connection lifetime spans callback)
- ✗ **Smart pointer misuse** doesn't detect:
  - `get()` used incorrectly (lifetime assumptions)
  - `unique_ptr` move semantics not followed
  - `shared_ptr` aliasing without `make_shared` (ref count mismatch)
- ✗ **Exception-unsafe construction** not detected (RAII not validated on exception path)
- ✗ **Copy/move semantics** of resource types not checked
- ✗ **Scope issues** with temporary resources (e.g., `Resource().use()` doesn't persist)

**Best-Practice Gaps:**
- CWE-404: Improper resource validation
- CWE-459: Incomplete cleanup
- CERT C++: RAII and related patterns
- MISRA C++ 12.4.4: Resource management

---

### 1.6 STL Container Misuse Scanner (380 lines, 8 patterns)

**Current Coverage:**
- O(n²) patterns (nested iterations, repeated lookups)
- Inefficient `find()` in vector
- Iterator invalidation after container modification
- Wrong container type for use case
- Repeated lookups in loop
- Copy overhead
- Uninitialized container access
- Range-for on temporary container

**Gaps & False Negatives:**
- ✗ **O(n²) detection** looks for nested loops with `find()`; misses:
  - O(n²) from repeated allocations without nesting visibility
  - O(n²) from callback chains (allocates in loop)
  - O(n²) from algorithm misuse (e.g., `partition` called repeatedly)
- ✗ **Iterator invalidation** doesn't track:
  - Iterators returned from functions (lifetime unclear)
  - Iterators stored in data structures
  - Invalidation from indirect container modifications (through reference parameter)
- ✗ **Wrong container** detection requires keyword patterns; misses:
  - Subtle performance gaps (vector iteration over linked list, etc.)
  - Locality of reference issues
  - Cache-unfriendly access patterns
- ✗ **Copy overhead** not detected for large types (relies on keyword matching)
- ✗ **Reserve/capacity** not checked (frequent reallocations)
- ✗ **Range-for on temporary** heuristic-based; misses complex expressions
- ✗ **Erase-remove idiom** not recognized as optimal pattern

**Best-Practice Gaps:**
- CWE-399: Resource exhaustion
- CWE-407: Inefficient algorithmic complexity
- MISRA C++ A6-2-2: Prefer standard library over custom containers
- CERT C++: STL containers and algorithms

---

### 1.7 Platform Portability Scanner (280 lines, 7 patterns)

**Current Coverage:**
- Missing `#ifdef` guards around platform-specific code
- POSIX-only APIs without Windows guards
- Windows-only APIs without POSIX guards
- Hardcoded path separators
- Endianness assumptions
- Integer/pointer size assumptions
- Unportable pragmas

**Gaps & False Negatives:**
- ✗ **Platform guard tracking** is naive (ifdef stack); misses:
  - Nested/conditional ifdef chains with complex logic
  - `#if defined(...) && defined(...)` compound conditions
  - Macros controlling guards (indirection)
- ✗ **POSIX/Windows API detection** is keyword-based; misses:
  - Vendor-specific extensions (e.g., `O_DIRECT` on Linux, `FILE_FLAG_NO_BUFFERING` on Windows)
  - Wrapper functions that abstract platform differences
  - Cross-platform libraries (e.g., Boost.Asio) hiding platform details
- ✗ **Path separator** assumes hardcoded `/` or `\\`; misses:
  - Paths passed as config/constant
  - UNC paths on Windows (`\\server\share`)
  - Path composition via `std::filesystem` (safe, not detected as portability issue)
- ✗ **Endianness** not checked in serialization/network code
- ✗ **Type size** assumptions (e.g., `int` for 32-bit) not validated
- ✗ **Floating-point** precision assumptions across platforms
- ✗ **Atomic/volatile** platform behavior differences not checked

**Best-Practice Gaps:**
- CWE-392: Incorrect setting of access control
- CWE-1104: Use of unmaintained third-party components (platform-specific)
- MISRA C++ Portability (2nd edition)
- CERT C++: Portability and compatibility

---

### 1.8 Performance Anti-Patterns Scanner (330 lines, 8 patterns)

**Current Coverage:**
- String concatenation in loops (StringBuilder pattern)
- Repeated allocations in loops
- Inefficient sorting/searching
- Unnecessary copies
- Lock contention in hot paths
- Expensive operations in inner loops
- Missing caching/memoization
- Double processing

**Gaps & False Negatives:**
- ✗ **String concatenation** detection requires `+=` with quoted strings; misses:
  - `std::string(a) + b + c` (temporary accumulation)
  - Function-based concatenation (e.g., `str.append(...)` in loop)
  - StringBuilder pattern using `fmt::format` or ranges
- ✗ **Allocation in loop** doesn't distinguish:
  - Necessary allocations (unavoidable)
  - Avoidable allocations (could pre-allocate)
  - Allocation via function call (not visible as `new`)
- ✗ **Expensive inner ops** requires hardcoded keyword list; misses:
  - Custom expensive functions (library calls unknown)
  - O(n) operations not in keyword list
- ✗ **Lock contention** detection looks for mutex in loop; misses:
  - Lock acquired in function call chain
  - Contention from shared state accessed in hot path
- ✗ **Cache efficiency** not analyzed (stride, alignment, prefetching)
- ✗ **Branch prediction** issues not detected
- ✗ **SIMD missed optimization** not detected
- ✗ **Unnecessary copies** from implicit conversions not caught

**Best-Practice Gaps:**
- CWE-407: Inefficient algorithmic complexity
- CWE-399: Resource exhaustion (CPU/memory)
- MISRA C++ Performance (guidelines, not rules)
- CERT C++: Performance (not formalized as CWE)

---

## Part 2: Best-Practice Gap Analysis

### 2.1 CERT C++ Issues Not Covered

| CWE/CERT ID | Category | Current Status | Impact |
|---|---|---|---|
| CERT C++ ENV | Environment | ✗ Not covered | Environment variable injection, unsafe `getenv` |
| CERT C++ FIO | I/O | ✗ Partial (RAII only) | File pointer leaks, format string in I/O |
| CERT C++ MEM | Memory | ✓ Good (Memory scanner) | Some edge cases (see 1.2) |
| CERT C++ OOP | OOP | ✗ Not covered | Virtual destructor, slicing, PIMPL issues |
| CERT C++ DCL | Declarations | ✗ Not covered | External linkage abuse, unnamed namespaces |
| CERT C++ EXP | Expressions | ✗ Not covered | Side effects in condition, uninitialized vars |
| CERT C++ INT | Integers | ✗ Not covered | Overflow, truncation, sign mismatches |
| CERT C++ FLP | Floating-point | ✗ Not covered | Comparison, precision, NaN/Inf handling |
| CERT C++ ARR | Arrays | ✗ Minimal | Only static bounds checks |
| CERT C++ STR | Strings | ✗ Minimal | Only unsafe functions, not buffer overruns |
| CERT C++ CON | Concurrency | ✓ Good (Concurrency scanner) | Some edge cases (see 1.4) |
| CERT C++ ERR | Error Handling | ✗ Minimal | Only unchecked return codes |

### 2.2 CWE Top 25 (2023) Not Covered

| CWE | Title | Current Status |
|---|---|---|
| CWE-787 | Out-of-bounds Write | ✗ Not covered (only static array bounds) |
| CWE-79 | Improper Neutralization (XSS) | ✗ Not covered (Web-specific) |
| CWE-89 | SQL Injection | ✓ Partial (Security scanner) |
| CWE-1021 | Improper Restriction of Rendered UI | ✗ Not covered (UI-specific) |
| CWE-78 | Improper Neutralization (OS Command) | ✗ Not covered (command injection, not SQL) |
| CWE-434 | Unrestricted Upload of File | ✗ Not covered |
| CWE-94 | Improper Control of Generation (Code Injection) | ✗ Not covered |
| CWE-190 | Integer Overflow | ✗ **HIGH PRIORITY** |
| CWE-352 | Cross-Site Request Forgery (CSRF) | ✗ Not covered (Web-specific) |
| CWE-22 | Improper Limitation of Pathname | ✗ Not covered (path traversal) |
| CWE-200 | Exposure of Sensitive Information | ✗ Minimal |
| CWE-401 | Missing Release of Memory | ✓ Partial (Memory scanner) |
| CWE-426 | Untrusted Search Path | ✗ Not covered |
| CWE-502 | Deserialization of Untrusted Data | ✗ Not covered |
| CWE-611 | Improper Restriction of XML (XXE) | ✗ Not covered |

### 2.3 MISRA C++ 2008/2020 Rules Not Covered

**Notable Missing Rules:**
- Rule 5-2-10: Literals shall not use suffixes (L, U, F)
- Rule 5-3-1: Each operand of the ! operator shall have bool type
- Rule 5-0-1: Conversions shall not be performed (implicit casts)
- Rule 6-4-1: Array indexing shall be the only form of pointer arithmetic
- Rule 8-0-1: Functions shall not return references or pointers to local data
- Rule 9-3-1: Const member functions shall not return non-const references
- Rule 12-1-1: An rvalue reference shall not be bound to an lvalue
- Rule 14-5-1: A non-member generic function declaration shall have only a single parameter

---

## Part 3: Improvement Proposals for Existing Scanners

### 3.1 Security Scanner Improvements

**Priority 1 — Integer Overflow Detection**
- **Pattern:** Size calculations before allocation/copy
  - `new T[size + 1]`, `memcpy(dst, src, len + offset)`, `malloc(width * height * depth)`
- **Implementation:** Track integer arithmetic in allocation context, check for overflow risks
- **Complexity:** Medium (requires type inference for size_t vs int)
- **Est. code:** ~80 lines

**Priority 2 — Secondary Unsafe Functions**
- **Pattern:** `strcat`, `strncat`, `sscanf`, `atoi` (implicit overflow)
- **Implementation:** Extend UNSAFE_PATTERNS dict with secondary functions
- **Complexity:** Low (pattern matching)
- **Est. code:** ~20 lines

**Priority 3 — Hardcoded Secret Improvements**
- **Pattern:** Global `const char* array` with suspicious content, environment variable names
- **Implementation:** Regex for `const char* ARRAY[] = {...}` with secret-like strings
- **Complexity:** Medium (multi-line pattern)
- **Est. code:** ~40 lines

**Priority 4 — Information Disclosure**
- **Pattern:** `LOG(...)`, `printf(...)`, `std::cout`, `syslog(...)` containing secret keywords
- **Implementation:** Check log statements for secret keywords (API, password, token, key)
- **Complexity:** Medium
- **Est. code:** ~50 lines

---

### 3.2 Memory Safety Scanner Improvements

**Priority 1 — Double-Free Detection**
- **Pattern:** Multiple `delete` paths in control flow (if/else)
- **Implementation:** Build control flow graph (CFG) of function, track `delete` on all paths
- **Complexity:** High (requires CFG construction)
- **Est. code:** ~150 lines

**Priority 2 — Use-After-Free (Enhanced)**
- **Pattern:** Pointer used after `delete`, pointer returned from freed stack object
- **Implementation:** Track variable lifetimes, check uses after scope exit or `delete`
- **Complexity:** High (data flow analysis)
- **Est. code:** ~120 lines

**Priority 3 — Mismatched new/delete**
- **Pattern:** `new[]` with `delete`, array `new` with scalar `delete`, placement new issues
- **Implementation:** Track `new`/`new[]` source, match against `delete`/`delete[]`
- **Complexity:** Medium (requires symbol tracking)
- **Est. code:** ~70 lines

**Priority 4 — Smart Pointer Factory Calls**
- **Pattern:** `unique_ptr<T> p; p = new T()` (should use `make_unique`)
- **Implementation:** Detect `assignment to smart pointer from raw new`
- **Complexity:** Low-Medium (pattern + symbol matching)
- **Est. code:** ~50 lines

---

### 3.3 Concurrency Scanner Improvements

**Priority 1 — Class Member Data Race Detection**
- **Pattern:** Shared class members accessed from multiple threads without synchronization
- **Implementation:** Track class layout, identify mutable members, check accesses from thread/callback contexts
- **Complexity:** High (requires class scope analysis)
- **Est. code:** ~180 lines

**Priority 2 — Lock Ordering Violation (Enhanced)**
- **Pattern:** Inconsistent lock order across different code paths (A→B in one, B→A in another)
- **Implementation:** Build lock dependency graph, detect cycles
- **Complexity:** High (graph construction and cycle detection)
- **Est. code:** ~150 lines

**Priority 3 — Stale Closure Detection**
- **Pattern:** Lambda/callback capturing mutable reference/pointer to local variable, called asynchronously
- **Implementation:** Track lambda captures, check for async invocation (thread spawn, async task, callback registration)
- **Complexity:** High (requires lifetime analysis)
- **Est. code:** ~140 lines

**Priority 4 — Spurious Wakeup Loops**
- **Pattern:** `condition_variable::wait()` without loop, notified without checking predicate
- **Implementation:** Detect bare `wait()` calls (should be in loop), check `notify` usage
- **Complexity:** Medium
- **Est. code:** ~80 lines

---

### 3.4 Container Scanner Improvements

**Priority 1 — Iterator Lifetime Validation**
- **Pattern:** Iterator used after container modification, or returned from function scope
- **Implementation:** Track iterator creation, container modifications, scope boundaries
- **Complexity:** High (lifetime analysis)
- **Est. code:** ~150 lines

**Priority 2 — Reserve/Capacity Optimization**
- **Pattern:** `push_back()` in loop without prior `reserve()` (frequent reallocations)
- **Implementation:** Detect loop over container with `push_back()`, check for prior `reserve()`
- **Complexity:** Medium
- **Est. code:** ~70 lines

**Priority 3 — Erase-Remove Idiom Recognition**
- **Pattern:** `container.erase(remove_if(...), container.end())` — optimize or flag as correct usage
- **Implementation:** Recognize idiom as optimal pattern (avoid false positive)
- **Complexity:** Low-Medium (pattern recognition)
- **Est. code:** ~50 lines

---

### 3.5 RAII Scanner Improvements

**Priority 1 — Ownership Transfer Validation**
- **Pattern:** Raw pointer passed to function where ownership transfer is unclear or incorrect
- **Implementation:** Track function signatures for `unique_ptr` parameters (explicit ownership), detect `get()` misuse
- **Complexity:** Medium (requires function signature analysis)
- **Est. code:** ~100 lines

**Priority 2 — Async Lifetime Validation**
- **Pattern:** Resource lifetime spans callback/async invocation; resource freed before callback executes
- **Implementation:** Track async function calls, identify captured resources, verify resource lifetime
- **Complexity:** High (async context tracking)
- **Est. code:** ~140 lines

**Priority 3 — Exception-Safe Construction**
- **Pattern:** Member initialization in constructor where exception in constructor can leak
- **Implementation:** Build initialization dependency graph, detect resources acquired before full object initialization
- **Complexity:** High (exception path analysis)
- **Est. code:** ~130 lines

---

### 3.6 Reliability Scanner Improvements

**Priority 1 — Retry Loop Recognition (Enhanced)**
- **Pattern:** Function-based retry (e.g., `RetryPolicy::execute(func)`, `async::retry(func, attempts)`)
- **Implementation:** Recognize retry framework patterns beyond keyword matching
- **Complexity:** Medium (pattern library)
- **Est. code:** ~90 lines

**Priority 2 — Timeout Context Tracking**
- **Pattern:** Timeout from const/config variable, timeout in context metadata (gRPC, REST)
- **Implementation:** Track timeout values from variables, context objects
- **Complexity:** Medium (symbol + context tracking)
- **Est. code:** ~100 lines

**Priority 3 — Cascading Failure Modeling**
- **Pattern:** Failure in component A causes secondary failure in B (e.g., DB down → cache miss → OOM)
- **Implementation:** Build dependency graph of component calls, detect failure chains
- **Complexity:** High (system-level graph)
- **Est. code:** ~150 lines

---

## Part 4: New Scanner Proposals (Phase 5-6)

### Phase 5: Critical Vulnerabilities & Best Practices

#### **Phase 5.1: Exception Safety & Move Semantics Scanner**

**Title:** Exception-Safe C++ & Rvalue Reference Patterns Detection  
**Purpose & Impact:**  
- Detect **exception-unsafe code** (resources acquired but not exception-safe)
- Detect **missing move semantics** (expensive copies when moves available)
- Detect **noexcept violations** (functions marked noexcept but can throw)
- Reduce production crashes from unexpected exceptions; improve performance (avoid unnecessary copies)
- **Impact:** ~500-1000 gaps/module (estimated 5-7 high-severity in storage/concurrency modules)

**Expected Patterns to Detect:**
1. Constructor body acquires resources before member initialization completes (exception leak)
2. Function marked `noexcept` but contains throwing operations (`std::make_shared`, `std::vector::push_back`)
3. Copy constructor defined but move constructor missing (expensive return by value)
4. Pass-by-const-reference for large types in performance-critical code (should be move or std::optional)
5. `std::move` on rvalue (redundant), or missing `std::move` on expensive temporary

**Complexity:** High (requires data flow + exception path analysis)  
**Est. Lines of Code:** ~280-320 lines  
**Priority:** Phase 5 (High — affects reliability & performance)

---

#### **Phase 5.2: Type Conversion & Narrowing Detection**

**Title:** Unsafe Type Conversions, Integer Overflow, Narrowing Conversions  
**Purpose & Impact:**  
- Detect **narrowing conversions** in initializers (`int x = 3.14;`)
- Detect **integer overflow** in arithmetic (size calculations, bitwise ops)
- Detect **sign mismatch** comparisons (`if (size_t x < -1)` always true)
- Detect **unsafe casts** (`reinterpret_cast` without alignment checks, `const_cast` away from thread-safety)
- **Impact:** Eliminate silent truncations, integer overflows, security mismatches
- **Estimated gaps:** ~300-600/module (especially storage, query, index modules)

**Expected Patterns to Detect:**
1. Implicit narrowing in initializer: `int x {large_uint64_t};`, `char c = large_int;`
2. Integer arithmetic that can overflow: `uint32_t idx = i * sizeof(T)` (multiply before bounds check)
3. Signed/unsigned comparison: `for (int i = 0; i < (unsigned)size; ++i)`, `if (error_code < 0)`
4. Unsafe `reinterpret_cast`: No alignment validation, type mismatch
5. `const_cast` in non-const context (thread-unsafe mutation of shared const data)

**Complexity:** Medium-High (requires type inference)  
**Est. Lines of Code:** ~240-280 lines  
**Priority:** Phase 5 (Critical — CWE-190 integer overflow is top-10 CWE)

---

#### **Phase 5.3: Uninitialized Variables & Data Flow Detection**

**Title:** Uninitialized Variable Detection, Data Flow Anomalies  
**Purpose & Impact:**  
- Detect **uninitialized variables** used without assignment
- Detect **uninitialized pointers** dereferenced before allocation
- Detect **member initialization** gaps in constructors
- **Impact:** Eliminate UB from uninitialized reads, improve determinism
- **Estimated gaps:** ~200-400/module (especially in query engine, graph modules)

**Expected Patterns to Detect:**
1. Variable declared but used without initialization: `int x; if (x > 0)` (use without init)
2. Member variable not initialized in any constructor branch
3. Pointer allocated conditionally, but used unconditionally later: `if (x) p = new T(); p->use();` (UB)
4. Output parameter not assigned before return in all branches
5. Function parameter used before being assigned: `void f(int x) { y = x; x = new_val; ... }` — use before init

**Complexity:** High (requires data flow analysis, multiple branches)  
**Est. Lines of Code:** ~280-320 lines  
**Priority:** Phase 5 (High — common source of UB)

---

#### **Phase 5.4: Virtual Function & OOP Correctness Detection**

**Title:** Virtual Destructor, Slicing, PIMPL Pattern, Pure Virtual Implementation  
**Purpose & Impact:**  
- Detect **missing virtual destructor** in polymorphic classes (resource leak)
- Detect **object slicing** (derived → base by value assignment)
- Detect **PIMPL idiom violations** (pimpl member public, not private)
- Detect **pure virtual not implemented** in concrete classes
- **Impact:** Eliminate object slicing bugs, resource leaks from polymorphism, enforce design patterns
- **Estimated gaps:** ~150-300/module

**Expected Patterns to Detect:**
1. Derived class without `virtual` destructor in inheritance hierarchy
2. Assignment/passing derived class by-value to base class: `Base b = derived;`
3. PIMPL member (`impl`, `pimpl`, `_pimpl`) not private in public class
4. Pure virtual method (`= 0`) not implemented in derived class (template specialization check)
5. Virtual function call in constructor/destructor on wrong vptr (undefined behavior)

**Complexity:** Medium (requires class hierarchy analysis)  
**Est. Lines of Code:** ~220-260 lines  
**Priority:** Phase 5 (Medium — good OOP correctness)

---

#### **Phase 5.5: Input Validation & Boundary Checking**

**Title:** Missing Input Validation, Boundary Checks, Range Validation  
**Purpose & Impact:**  
- Detect **missing bounds checks** on array indexing, loops
- Detect **unchecked input parameters** in public APIs
- Detect **off-by-one errors** in loop conditions, array access
- **Impact:** Eliminate buffer overflows, DoS from invalid input, off-by-one crashes
- **Estimated gaps:** ~400-700/module

**Expected Patterns to Detect:**
1. Array/pointer access without preceding size check: `arr[i]` without `if (i < size)`
2. Loop condition off-by-one: `for (int i = 0; i <= size; ++i) arr[i]` (out-of-bounds)
3. Division by zero risk: `result = a / b` without `if (b != 0)`
4. Public API parameter not validated: `void setSize(int s) { this->size = s; }` (no range check)
5. Unsafe pointer arithmetic: `ptr + offset` without bounds validation

**Complexity:** Medium-High (control flow + bounds analysis)  
**Est. Lines of Code:** ~250-290 lines  
**Priority:** Phase 5 (Critical — CWE-787 out-of-bounds write)

---

### Phase 6: Advanced Analysis & Optimization

#### **Phase 6.1: Const-Correctness & Mutation Audit**

**Title:** Missing `const` Qualifiers, Logical Constness Violations, Mutable Abuse  
**Purpose & Impact:**  
- Detect **methods not marked const** when they should be (design issue)
- Detect **const method mutating state** via mutable member (logical const violation)
- Detect **mutable member abuse** (should use non-const path instead)
- **Impact:** Enforce const-safety, catch mutation bugs, improve code clarity
- **Estimated gaps:** ~300-500/module

**Expected Patterns to Detect:**
1. Non-const method that doesn't modify state (`return member;`, getter logic)
2. Const method modifying `mutable` member (cache, logger — OK in moderation, but flag excessive use)
3. `mutable` member that persists state (not just cache — logic error)
4. Parameter passed non-const when could be const-ref (allows accidental mutation)
5. Return non-const reference from const method (const violation escape hatch)

**Complexity:** High (requires method body analysis, type system awareness)  
**Est. Lines of Code:** ~280-320 lines  
**Priority:** Phase 6 (Design improvement, not correctness)

---

#### **Phase 6.2: Template Instantiation & Generic Code Correctness**

**Title:** Template Error Detection, Generic Type Preconditions, Concept Violations  
**Purpose & Impact:**  
- Detect **template misuse** (missing operator<, lack of copy/move semantics)
- Detect **generic algorithm preconditions** (e.g., `std::binary_search` requires sorted range)
- Detect **concept violations** in C++20 code (missing required operations)
- **Impact:** Catch template compilation errors early, enforce generic code contracts
- **Estimated gaps:** ~200-400/module (especially template-heavy modules like query engine)

**Expected Patterns to Detect:**
1. `std::sort` on range without `operator<` defined for element type
2. `std::binary_search` on unsorted range (precondition violation)
3. Generic function `template<T> void f()` using `T::size()` without concept check
4. `std::hash<T>` not specialized, used in unordered_map (UB)
5. Container element type doesn't support copy/move but container is copied (UB)

**Complexity:** Very High (template analysis, concept checking)  
**Est. Lines of Code:** ~350-400 lines  
**Priority:** Phase 6 (Advanced — aids template-heavy codebases)

---

#### **Phase 6.3: Undefined Behavior Detection (UB Catalog)**

**Title:** Comprehensive Undefined Behavior Detection  
**Purpose & Impact:**  
- Detect **common UB patterns** (signed overflow, shift amount out of range, etc.)
- Detect **data race UB** (non-atomic shared state access)
- Detect **use of moved-from object** (destructors may fail, state invalid)
- **Impact:** Eliminate UB that causes non-deterministic bugs, improve code reliability
- **Estimated gaps:** ~500-1000/module

**Expected Patterns to Detect:**
1. Signed integer overflow: `int x = INT_MAX + 1;`
2. Shift amount out of range: `x << (sizeof(x) * 8)` (UB if >= width)
3. Array indexing out of bounds: `arr[-1]`, `arr[size]`
4. Pointer to freed memory dereferenced
5. Use of object after calling `std::move` without reassignment
6. Data race on shared variable (non-atomic, multiple threads)

**Complexity:** Very High (requires UB catalog, taint analysis)  
**Est. Lines of Code:** ~400-450 lines  
**Priority:** Phase 6 (Comprehensive safety)

---

#### **Phase 6.4: Macro Safety & Preprocessor Correctness**

**Title:** Macro Hygiene, Macro Substitution Risks, Preprocessor Issues  
**Purpose & Impact:**  
- Detect **macro hygiene violations** (unguarded parameters, missing parentheses)
- Detect **macro name collisions** with standard library/keywords
- Detect **macro substitution risks** (argument evaluation multiple times)
- **Impact:** Eliminate macro-related subtle bugs, improve code maintainability
- **Estimated gaps:** ~100-200/module

**Expected Patterns to Detect:**
1. Macro parameter not parenthesized: `#define MAX(a, b) a > b ? a : b` (use in `MAX(1+1, 2)` breaks)
2. Macro argument evaluated multiple times (side effects): `#define ARRAY_LEN(a) (sizeof(a)/sizeof(a[0]))`
3. Macro name conflicts with std keywords: `#define min std::min` (OK), but `#define min(...) ...` (not OK)
4. Macro containing statements without `do { ... } while(0)` guard
5. Macro with unguarded `#else` or `#endif` causing preprocessor errors

**Complexity:** Medium (preprocessor parsing)  
**Est. Lines of Code:** ~180-220 lines  
**Priority:** Phase 6 (Code quality, not critical)

---

#### **Phase 6.5: API Contract & Precondition Validation**

**Title:** API Preconditions, Postconditions, Invariant Violations  
**Purpose & Impact:**  
- Detect **precondition violations** (function assumes sorted input, but unsorted passed)
- Detect **postcondition gaps** (function doesn't establish promised state)
- Detect **class invariant violations** (state becomes inconsistent)
- **Impact:** Enforce API contracts, catch misuse at call sites
- **Estimated gaps:** ~300-600/module (high-value for library APIs)

**Expected Patterns to Detect:**
1. Function documented to require sorted input, called on unsorted data
2. Function returning pointer/reference, but caller doesn't validate (null/dangling)
3. Class invariant broken after public method call (e.g., `size()` doesn't match internal count)
4. Function precondition not validated (e.g., `require(x > 0)` missing)
5. Postcondition not established (e.g., `append()` doesn't increase `size()`)

**Complexity:** Very High (requires contract annotations, semantic understanding)  
**Est. Lines of Code:** ~350-400 lines  
**Priority:** Phase 6 (Design enforcement, high ROI for complex modules)

---

## Part 5: Feasibility & Priority Matrix

### Effort vs. Impact Assessment

| Phase | Scanner | Complexity | Est. Lines | Effort | Impact | ROI | Priority |
|---|---|---|---|---|---|---|---|
| 5 | Exception Safety | High | 280-320 | 8 days | High | 4/5 | Phase 5-1 |
| 5 | Type Conversion | Med-High | 240-280 | 6 days | Critical | 5/5 | **Phase 5-2** |
| 5 | Uninitialized Vars | High | 280-320 | 8 days | High | 4/5 | Phase 5-3 |
| 5 | OOP Correctness | Medium | 220-260 | 5 days | Medium | 3/5 | Phase 5-4 |
| 5 | Input Validation | Med-High | 250-290 | 7 days | Critical | 5/5 | **Phase 5-5** |
| 6 | Const-Correctness | High | 280-320 | 8 days | Medium | 3/5 | Phase 6-1 |
| 6 | Template Safety | Very High | 350-400 | 12 days | Medium-High | 3/5 | Phase 6-2 |
| 6 | UB Catalog | Very High | 400-450 | 14 days | High | 4/5 | Phase 6-3 |
| 6 | Macro Safety | Medium | 180-220 | 4 days | Low-Medium | 2/5 | Phase 6-4 |
| 6 | API Contracts | Very High | 350-400 | 12 days | Very High | 5/5 | **Phase 6-5** |

### Recommended Phase 5 Roadmap

**Month 1 (Target: Q3 2026):**
1. **Type Conversion & Narrowing** (5-2) — 6 days — Critical CWE-190
2. **Input Validation** (5-5) — 7 days — Critical CWE-787

**Month 2:**
3. **Exception Safety & Move Semantics** (5-1) — 8 days — Reliability
4. **Uninitialized Variables** (5-3) — 8 days — Correctness

**Fallback/Parallel:**
5. **OOP Correctness** (5-4) — 5 days — Design patterns (lower priority)

**Phase 6 Priority:** Phase 6-5 (API Contracts) → Phase 6-3 (UB Catalog) → Phase 6-2 (Template Safety)

---

## Part 6: Implementation Guidelines & Patterns

### 6.1 Framework Enhancements Required

**Multi-line Pattern Matching:**
```python
class MultilinePatternMatcher:
    def match_across_lines(self, lines: List[str], start_idx: int, pattern: str) -> Optional[Match]:
        """Match pattern that spans multiple lines (e.g., resource allocation spanning assignment & initialization)"""
        # Collect context (e.g., 10 lines before and after)
        context = '\n'.join(lines[max(0, start_idx-10):min(len(lines), start_idx+20)])
        # Apply regex with DOTALL flag
        return re.search(pattern, context, re.DOTALL | re.MULTILINE)
```

**Control Flow Graph (CFG) for UB Detection:**
```python
class ControlFlowAnalyzer:
    def build_cfg(self, ast: FunctionAST) -> ControlFlowGraph:
        """Build CFG to track reachability, data flow, exception paths"""
        # Build nodes for basic blocks
        # Identify edges (if/else, loops, exception handlers)
        # Compute reachability, dominance relationships
        pass
```

**Type Inference System:**
```python
class TypeInferenceEngine:
    def infer_type(self, expr: Expression, context: ScopeContext) -> Type:
        """Infer type of expression (needed for narrowing, overflow detection)"""
        # Track variable declarations, assignments, operations
        # Return Type(base, width, signedness, qualifiers)
        pass
```

### 6.2 Recommended Pattern Library Additions

**Retry Patterns Library:**
```python
RETRY_PATTERNS = {
    'loop_retry': re.compile(r'for\s*\(\s*int\s+\w+\s*=\s*0\s*;\s*\w+\s*<\s*\d+\s*;\s*\w+\+\+\s*\)'),
    'while_retry': re.compile(r'while\s*\(\s*--\w+\s*>\s*0\s*\)'),
    'async_retry': re.compile(r'(RetryPolicy|AsyncRetry|RetryExecutor)::\w+'),
    'timer_retry': re.compile(r'(setTimeout|setInterval|timer)\s*\('),
}
```

**Timeout Patterns Library:**
```python
TIMEOUT_PATTERNS = {
    'explicit_timeout': re.compile(r'(wait_for|wait_until|with_timeout|timeout)\s*\([^)]*\)'),
    'deadline_timeout': re.compile(r'(deadline|expires_at|expires_in)\s*\('),
    'grpc_timeout': re.compile(r'context\.(deadline|timeout|set_deadline)'),
    'timeout_var': re.compile(r'(\w*timeout\w*|duration)\s*[=:]'),
}
```

### 6.3 Scanner Integration Points

**Phase 5 Timeline:**
- Weeks 1-2: Enhance existing Security/Memory/Concurrency scanners (low-effort improvements)
- Weeks 3-6: Implement Phase 5 new scanners (Type Conversion, Input Validation, Exception Safety, Uninitialized Vars)
- Weeks 7-8: Integration, framework enhancements, orchestrator updates

**Phase 6 Timeline:**
- Months 4-5: Template Safety, UB Catalog
- Months 6-7: API Contracts, Const-Correctness
- Month 8: Macro Safety, refinements

---

## Part 7: Recommendations & Next Steps

### 7.1 Quick Wins (Existing Scanner Improvements)

**Implement in Week 1:**
1. **Integer Overflow in Security scanner** — 80 lines, high impact
2. **Smart Pointer Factory Detection in Memory scanner** — 50 lines, easy
3. **Lock Ordering in Concurrency scanner** — extend with CFG analysis

**Est. ROI:** +500-1000 gaps detected, minimal effort

### 7.2 Phase 5 Starting Point

**Recommend starting with Phase 5-2 (Type Conversion):**
- Highest CWE impact (CWE-190 in top-10)
- Clear pattern signatures (narrowing, overflow)
- Foundational for Phase 5-3 (Uninitialized Vars)

**Second priority: Phase 5-5 (Input Validation)**
- CWE-787 (out-of-bounds write) also top-10
- Complements existing Memory scanner
- Direct security impact

### 7.3 Orchestrator Enhancements

**Update gap_scanner_v3.py:**
```python
# Phase 5 imports
from gap_scanner_v3_type_conversion import TypeConversionGapScanner
from gap_scanner_v3_input_validation import InputValidationGapScanner
from gap_scanner_v3_exception_safety import ExceptionSafetyGapScanner

# Phase 5 execution
phase5_scanners = [
    ('type_conversion', TypeConversionGapScanner),
    ('input_validation', InputValidationGapScanner),
    ('exception_safety', ExceptionSafetyGapScanner),
]
```

### 7.4 Tracking & Validation

**Establish baseline metrics:**
- Current: 31,720 gaps across 60 modules
- Phase 5 target: +40-60% (12,000-19,000 additional gaps)
- Phase 6 target: +60-80% closure rate (validation)

**Implement automated testing:**
- Unit tests for each scanner pattern (positive/negative cases)
- Regression tests (existing gaps still detected)
- False-positive rate tracking (target: <5%)

---

## Conclusion

The Gap Scanner v3 provides solid foundational coverage across 8 domains. **Phase 5** addresses critical gaps (Type Conversion, Input Validation, Exception Safety) with estimated 40-60% coverage improvement. **Phase 6** targets advanced analysis (API Contracts, UB Catalog, Template Safety) for long-tail vulnerability elimination.

**Recommended approach:**
1. ✅ Implement quick wins in existing scanners (1 week)
2. ✅ Start Phase 5-2 (Type Conversion) immediately (6 days)
3. ✅ Parallel track Phase 5-5 (Input Validation) (7 days)
4. ✅ Complete remaining Phase 5 by Q3 2026
5. ✅ Prioritize Phase 6-5 (API Contracts) for highest-ROI phase 6 work

**Estimated total effort:** ~50 dev-days for full Phase 5+6 implementation

---

**Document Version:** 1.0  
**Last Updated:** May 19, 2026  
**Confidence Level:** High (based on comprehensive codebase analysis)
