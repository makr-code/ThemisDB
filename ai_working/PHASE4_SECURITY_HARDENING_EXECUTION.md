# Phase 4 — Security & Compliance Hardening Execution Plan

**Date:** 2026-07-28  
**Target Completion:** 2026-10-31  
**Status:** 🟡 PLANNING & EXECUTION  
**Scope:** Burn down security-relevant gaps and achieve zero CRITICAL findings in release-critical paths (server, llm, sharding)

---

## Executive Summary

Phase 4 focuses on systematically hardening the release-critical modules (server, llm, sharding) against CRITICAL and HIGH security findings. Work is organized into 5 concurrent focus areas, each with explicit acceptance criteria and test gates.

### Current State (2026-07-28)

- **Baseline findings:** 22,085 deduplicated gaps across all modules
- **Release-critical modules:** server (P5-S01/S02 delivered), llm (P5-L01/L02 delivered), sharding (P6 complete 2026-07-22)
- **Existing gates:** Wave 7 baseline PASS, release_critical CI active, sanitizer evidence bundle ready, pentest evidence bundle ready
- **Residual risk:** Phase 4 exit criterion requires **0 new CRITICAL findings** in server/llm/sharding + 50+ security tests

---

## Focus Areas & Burndown Strategy

### Focus Area 1: Input Validation (Server, Query, LLM) — CRITICAL
**Owner:** Security & Input Validation Team  
**Target:** 2026-09-15

#### 1.1 Server-Side Input Validation (SEC-IV-01..05)

**Scope:** All HTTP request handlers in `include/server/http_server.h` and `src/server/http_server.cpp`

**Tasks:**
- [ ] Audit HTTP request handler input validation patterns
  - Query parameters validation (size, type, encoding)
  - Request body size limits
  - Header value validation
  - Status: PENDING → IN_PROGRESS

- [ ] Implement strict allowlist validation (reject-by-default)
  - SQL injection prevention (parameterized queries)
  - Command injection prevention (no shell evaluation)
  - Buffer overflow prevention (strict size limits)
  - Status: PENDING

- [ ] Create security tests (SEC-IV-01..05)
  - Test oversized requests
  - Test malformed parameters
  - Test injection payloads
  - Status: PENDING

#### 1.2 Query Parsing & Optimization (SEC-IV-06..10)

**Scope:** Query parser in `src/query/parser/` and `include/query/parser.h`

**Tasks:**
- [ ] Audit query parser for security gaps
  - Malformed SQL handling
  - Parameter binding validation
  - Literal value type checking
  - Integer overflow detection
  - Status: PENDING

- [ ] Implement parser hardening
  - Reject malformed queries
  - Validate type inference
  - Detect overflow in numeric literals
  - Status: PENDING

- [ ] Create security tests (SEC-IV-06..10)
  - Fuzzing with malformed queries
  - Type confusion payloads
  - Integer overflow inputs
  - Status: PENDING

#### 1.3 LLM Input Handling (SEC-IV-11..15)

**Scope:** Prompt and model input in `src/llm/` and `include/llm/`

**Tasks:**
- [ ] Audit LLM prompt input validation
  - Prompt length limits
  - Special character handling
  - Character encoding validation
  - Status: PENDING

- [ ] Implement LLM input hardening
  - Reject oversized prompts
  - Sanitize special characters
  - Validate UTF-8 encoding
  - Detect prompt injection patterns
  - Status: PENDING

- [ ] Create security tests (SEC-IV-11..15)
  - Prompt injection payloads
  - Model confusion tests
  - Resource exhaustion scenarios
  - Status: PENDING

#### 1.4 Fuzz Testing Campaign (SEC-FUZZ-01..20)

**Scope:** libFuzzer targets for critical parsers

**Tasks:**
- [ ] Create libFuzzer targets
  - Query parser fuzzer
  - HTTP request parser fuzzer
  - Protocol deserialization fuzzer
  - Status: PENDING

- [ ] Run fuzzing campaign
  - 1M+ inputs per fuzzer
  - 24+ hour continuous runs
  - Corpus minimization
  - Status: PENDING

- [ ] Remediate fuzzing findings
  - Triage and categorize all crashes/hangs
  - Fix root causes
  - Add regression tests
  - Status: PENDING

---

### Focus Area 2: Transport & Certificate Hardening (Network, Server) — HIGH
**Owner:** Network Security Team  
**Target:** 2026-09-30

#### 2.1 TLS 1.3+ Enforcement (SEC-TLS-01..05)

**Scope:** All TLS configurations in `include/network/` and `src/network/`

**Tasks:**
- [ ] Audit TLS configurations
  - Current minimum version
  - Cipher suite strength
  - Legacy protocol usage
  - Status: PENDING

- [ ] Remove TLS 1.0/1.1/1.2 support
  - Update OpenSSL/BoringSSL configuration
  - Remove legacy cipher suites
  - Enforce TLS 1.3+ minimum
  - Status: PENDING

- [ ] Implement strongest cipher suites
  - Enable ChaCha20-Poly1305
  - Enable AES-256-GCM
  - Disable legacy algorithms
  - Status: PENDING

- [ ] Create TLS tests (SEC-TLS-01..05)
  - Version enforcement tests
  - Cipher suite validation
  - Handshake verification
  - Status: PENDING

#### 2.2 Certificate Pinning (SEC-TLS-06..10)

**Scope:** Inter-node communication in sharding and replication modules

**Tasks:**
- [ ] Implement public-key pinning
  - Store trusted certificate hashes
  - Verify certificates on connection
  - Handle pin rotation
  - Status: PENDING

- [ ] Test certificate pinning
  - Valid certificate acceptance
  - Invalid certificate rejection
  - Rotation ceremony validation
  - Status: PENDING

#### 2.3 Certificate Rotation Automation

**Scope:** Certificate lifecycle management

**Tasks:**
- [ ] Implement expiration monitoring
  - Track certificate expiration dates
  - Alert 30 days before expiration
  - Status: PENDING

- [ ] Automate certificate renewal
  - Automated renewal 30 days before expiration
  - Zero-downtime certificate swap
  - Status: PENDING

- [ ] Test rotation process
  - Renewal without downtime
  - Cluster-wide synchronization
  - Status: PENDING

---

### Focus Area 3: Ownership & Memory Safety (All Modules) — CRITICAL
**Owner:** Memory Safety Team  
**Target:** 2026-09-15

#### 3.1 Run Full Sanitizer Suite (SEC-MEM-01..15)

**Scope:** All release-critical paths in server, llm, sharding

**Command:** `cmake --preset community-asan && ctest -L release_critical --output-on-failure`

**Tasks:**
- [ ] Run AddressSanitizer (ASan)
  - Heap buffer overflow detection
  - Stack buffer overflow detection
  - Use-after-free detection
  - Double-free detection
  - Status: PENDING → RUNNING

- [ ] Run MemorySanitizer (MSan)
  - Uninitialized memory read detection
  - Status: PENDING

- [ ] Run UndefinedBehaviorSanitizer (UBSan)
  - Undefined behavior detection
  - Integer overflow detection
  - Type confusion detection
  - Status: PENDING

#### 3.2 Fix All Memory-Safety Issues

**Tasks:**
- [ ] Inventory all ASan findings
  - Categorize by module
  - Categorize by finding type
  - Prioritize by severity
  - Status: PENDING

- [ ] Fix CRITICAL findings
  - Heap/stack overflows
  - Use-after-free bugs
  - Status: PENDING

- [ ] Fix HIGH findings
  - Memory leaks
  - Invalid memory access
  - Status: PENDING

- [ ] Verify clean sanitizer runs
  - Re-run full suite
  - Document clean passes
  - Status: PENDING

#### 3.3 RAII Audit (SEC-MEM-01..15)

**Scope:** All resource management in server, llm, sharding

**Tasks:**
- [ ] Verify RAII patterns
  - All malloc → delete
  - All open → close
  - All lock → unlock
  - All new → delete
  - Status: PENDING

- [ ] Implement RAII wrappers
  - Smart pointer adoption
  - Resource guard patterns
  - Exception-safe cleanup
  - Status: PENDING

- [ ] Document ownership contracts
  - Function-level ownership rules
  - Lifetime guarantees
  - Exception safety levels
  - Status: PENDING

---

### Focus Area 4: Concurrency & Race Conditions (All Modules) — HIGH
**Owner:** Concurrency Safety Team  
**Target:** 2026-09-30

#### 4.1 ThreadSanitizer (TSan) Analysis (SEC-RACE-01..10)

**Scope:** Concurrent LLM, server, sharding paths

**Tasks:**
- [ ] Run ThreadSanitizer on all modules
  - LLM concurrent inference
  - Server request handling
  - Sharding partition operations
  - Status: PENDING

- [ ] Analyze TSan data-race reports
  - Categorize by module
  - Identify synchronization gaps
  - Prioritize by risk
  - Status: PENDING

#### 4.2 Fix Race Conditions

**Tasks:**
- [ ] Fix all data races
  - Add missing locks
  - Add atomic operations
  - Add memory ordering constraints
  - Status: PENDING

- [ ] Verify TSan clean passes
  - Re-run full suite
  - Document clean passes
  - Status: PENDING

#### 4.3 Lock-Free Audit

**Scope:** All lock-free code (CAS loops, atomics)

**Tasks:**
- [ ] Audit lock-free implementations
  - Validate ordering semantics
  - Check seq-cst vs. acquire-release
  - Document assumptions
  - Status: PENDING

- [ ] Document synchronization contracts
  - Memory ordering assumptions
  - Causality guarantees
  - Failure semantics
  - Status: PENDING

---

### Focus Area 5: Error Path Security (All Modules) — HIGH
**Owner:** Error Handling & Audit Team  
**Target:** 2026-09-15

#### 5.1 Audit Error Messages (SEC-ERR-01..05)

**Scope:** All error messages and exception text in server, llm, sharding

**Tasks:**
- [ ] Inventory all error messages
  - Check for PII leakage (passwords, API keys, internal paths)
  - Check for stack traces in production builds
  - Categorize by sensitivity level
  - Status: PENDING

- [ ] Sanitize error messages
  - Remove sensitive data
  - Replace stack traces with error codes
  - Implement generic user-facing messages
  - Status: PENDING

- [ ] Create tests (SEC-ERR-01..05)
  - Verify no PII in error messages
  - Verify stack traces stripped in release builds
  - Status: PENDING

#### 5.2 Fail-Closed Defaults (SEC-FAIL-01..05)

**Scope:** All permission checks and security-relevant state

**Tasks:**
- [ ] Audit permission checks
  - Verify default is DENY
  - Check explicit ALLOW paths
  - Status: PENDING

- [ ] Audit security-relevant state defaults
  - Encryption state defaults to FAIL-SAFE
  - Authentication state defaults to DENY
  - Authorization state defaults to DENY
  - Status: PENDING

- [ ] Create tests (SEC-FAIL-01..05)
  - Verify fail-closed defaults
  - Test authorization boundaries
  - Status: PENDING

#### 5.3 Audit Logging (SEC-AUD-01..05)

**Scope:** Security-relevant operation logging

**Tasks:**
- [ ] Implement audit logging
  - Log all authentication operations
  - Log all privilege escalations
  - Log all configuration changes
  - Status: PENDING

- [ ] Secure audit logs
  - Prevent truncation by users
  - Implement tamper-evident storage
  - Test log integrity
  - Status: PENDING

---

## Phase 4 Exit Criteria

- [ ] **0 new CRITICAL findings** in server, llm, sharding modules (CodeQL + custom scanners)
- [ ] **Memory-safety:** ASan/MSan/UBSan clean (0 findings in critical paths)
- [ ] **Race conditions:** TSan clean (0 data-races in critical paths)
- [ ] **Input validation:** Fuzz testing complete, no new injection vectors found
- [ ] **TLS/Certificates:** 1.3+ enforced, pinning implemented, rotation automated
- [ ] **50+ security tests created and passing** (SEC-IV/TLS/MEM/RACE/ERR)
- [ ] **Security gap burndown:** All CRITICAL/HIGH findings resolved
- [ ] **Evidence:** All sanitizer, fuzz, and pentest reports archived in `docs/security/`

---

## Implementation Timeline

### Week 1 (2026-07-28 – 2026-08-03): Audit & Inventory
- [ ] Audit all security domains (IV, TLS, Memory, Concurrency, Error)
- [ ] Inventory all findings by module and severity
- [ ] Prioritize by risk and effort
- [ ] Document current state

### Week 2 (2026-08-04 – 2026-08-10): Input Validation & TLS Hardening
- [ ] Implement input validation hardening
- [ ] Enforce TLS 1.3+
- [ ] Create initial security tests

### Week 3 (2026-08-11 – 2026-08-17): Memory Safety Remediation
- [ ] Run full sanitizer suite
- [ ] Fix all CRITICAL/HIGH memory issues
- [ ] Implement RAII audit

### Week 4 (2026-08-18 – 2026-08-24): Concurrency & Error Path Hardening
- [ ] Run ThreadSanitizer
- [ ] Fix all data races
- [ ] Audit error messages and permissions

### Week 5 (2026-08-25 – 2026-08-31): Fuzz Testing & Certificate Automation
- [ ] Create libFuzzer targets
- [ ] Run fuzzing campaign (1M+ inputs)
- [ ] Implement certificate automation

### Week 6 (2026-09-01 – 2026-09-07): Security Test Suite
- [ ] Create 50+ security tests
- [ ] Integrate with release_critical CI
- [ ] Verify all tests passing

### Week 7-8 (2026-09-08 – 2026-09-30): Gap Burndown & Sign-Off
- [ ] Complete gap burndown
- [ ] Archive evidence
- [ ] Prepare for GA promotion

---

## Deliverables

1. **Security Gap Burndown Scoreboard** — by module, by finding type
2. **Input Validation Hardening Report** — fuzz testing results, injection vectors fixed
3. **TLS/Certificate Security Report** — 1.3+ enforcement, pinning implementation, rotation evidence
4. **Memory-Safety & Race-Condition Report** — ASan/TSan/UBSan evidence, fixes applied
5. **50+ Security Tests** — SEC-IV/TLS/MEM/RACE/ERR test suite
6. **Updated Security Documentation** — `docs/security/GA_SANITIZER_EVIDENCE_BUNDLE.md`
7. **GA Sign-Off Checklist** — comprehensive security readiness validation

---

## Tracking & Monitoring

### Key Metrics
- **CRITICAL findings:** Target 0 (from baseline 22,085 → all resolved)
- **HIGH findings:** Target < 5 (residual low-risk items acceptable)
- **Memory-safety:** Target ASan/MSan/UBSan clean
- **Race conditions:** Target TSan clean
- **Test coverage:** Target 50+ security tests passing
- **Fuzz coverage:** Target 1M+ inputs without crashes/hangs

### Risk Register
- **Residual TLS cipher suite weakness:** Mitigate with pinning + rotation
- **Legacy code paths:** Mark and document for removal
- **Concurrent access patterns:** Verify with stress testing

---

## Sign-Off Criteria (Human Approval Required)

Phase 4 completion requires explicit human approval for:
1. ✅ Zero CRITICAL findings (verified by CodeQL + custom scanners)
2. ✅ ASan/MSan/UBSan clean (verified by sanitizer runs)
3. ✅ TSan clean (verified by concurrency analysis)
4. ✅ Fuzz testing campaign complete (verified by crash/hang report)
5. ✅ 50+ security tests created and passing (verified by test suite)
6. ✅ Evidence archived and reviewed (verified by documentation review)
7. ✅ GA promotion sign-off ready (verified by human sign-off)

---

## References

- **Root Roadmap:** ROADMAP.md § Phase 4
- **Branch Governance:** BRANCHING_STRATEGY.md
- **Release Strategy:** RELEASE_STRATEGY.md
- **Sanitizer Evidence:** docs/security/GA_SANITIZER_EVIDENCE_BUNDLE.md
- **Pentest Evidence:** security/pentest/GA_PENTEST_EVIDENCE_BUNDLE.md
- **GA Sign-Off:** docs/governance/GA_PROMOTION_SIGN_OFF.md
