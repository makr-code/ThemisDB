# Phase 4 Security & Compliance Hardening — Audit & Implementation Guide

**Date:** 2026-07-28  
**Version:** 1.0 (Execution Ready)  
**Status:** 🟡 IN EXECUTION  
**Scope:** Systematic security gap burndown across release-critical modules  
**Target:** Zero CRITICAL findings by 2026-10-31

---

## Table of Contents

1. [Audit Methodology](#audit-methodology)
2. [Focus Area 1: Input Validation](#focus-area-1-input-validation)
3. [Focus Area 2: Transport Security](#focus-area-2-transport-security)
4. [Focus Area 3: Memory Safety](#focus-area-3-memory-safety)
5. [Focus Area 4: Concurrency](#focus-area-4-concurrency)
6. [Focus Area 5: Error Path Security](#focus-area-5-error-path-security)
7. [Test Execution Plan](#test-execution-plan)
8. [Evidence Archival](#evidence-archival)

---

## Audit Methodology

### Phases

1. **Inventory Phase** — Identify all security-relevant code paths
2. **Analysis Phase** — Detect vulnerability patterns and anti-patterns
3. **Remediation Phase** — Fix identified issues with priority ordering
4. **Verification Phase** — Validate fixes with targeted tests
5. **Evidence Phase** — Archive proof of security posture

### Tools & Techniques

- **Code Analysis:** Manual review, grep/AST analysis, semantic search
- **Vulnerability Scanning:** CodeQL, custom scanners, fuzzing
- **Memory Safety:** AddressSanitizer (ASan), MemorySanitizer (MSan), UndefinedBehaviorSanitizer (UBSan)
- **Concurrency:** ThreadSanitizer (TSan), lockfree analysis, stress testing
- **Testing:** Unit tests, integration tests, fuzzing, property-based testing

---

## Focus Area 1: Input Validation

### Objective

Ensure all external input (HTTP, SQL, LLM prompts) is validated with strict reject-by-default semantics, protecting against:
- SQL injection
- Command injection
- Path traversal
- Buffer overflow
- Prompt injection
- Resource exhaustion

### Audit Scope

#### 1.1 HTTP Request Handler Validation

**File:** `include/server/http_server.h`, `src/server/http_server.cpp`

**Validation Points:**
- Query parameter validation
- Request body size limits (100MB default)
- Header value validation
- Request path validation
- Content-Type validation
- Content-Length validation

**Implementation Pattern (Reject-by-Default):**

```cpp
bool ValidateHttpRequest(const HttpRequest& req) {
  // 1. Check size limits
  if (req.body.size() > MAX_REQUEST_SIZE) return false;
  
  // 2. Validate parameters (whitelist)
  for (const auto& [key, value] : req.parameters) {
    if (!IsValidParameterName(key)) return false;
    if (!IsValidParameterValue(value)) return false;
  }
  
  // 3. Validate headers
  for (const auto& [name, value] : req.headers) {
    if (!IsValidHeaderName(name)) return false;
    if (!IsValidHeaderValue(value)) return false;
  }
  
  // 4. Validate path
  if (!IsValidPath(req.path)) return false;
  
  // 5. Only allow after all checks pass
  return true;
}
```

**Test Coverage:**
- SEC-IV-01: Oversized request rejection
- SEC-IV-02: Normal request acceptance
- SEC-IV-03: Malformed parameter rejection
- SEC-IV-04: SQL injection detection
- SEC-IV-05: Command injection detection

#### 1.2 Query Parser Validation

**File:** `src/query/parser/parser.cpp`, `include/query/parser.h`

**Validation Points:**
- SQL syntax validation
- Parameter type checking
- Literal value bounds checking
- Integer overflow detection
- Operator precedence verification

**Implementation Pattern:**

```cpp
ParseResult ValidateAndParseQuery(std::string_view query) {
  // 1. Syntax validation (reject invalid SQL)
  if (!HasValidSyntax(query)) return ParseError("Invalid SQL syntax");
  
  // 2. Parameter binding validation
  for (const auto& param : ExtractParameters(query)) {
    if (!IsValidParameterBinding(param)) 
      return ParseError("Invalid parameter binding");
  }
  
  // 3. Type inference validation
  auto types = InferTypes(query);
  for (const auto& [param, type] : types) {
    if (!IsValidType(type))
      return ParseError("Type confusion detected");
  }
  
  // 4. Literal value validation
  for (const auto& literal : ExtractLiterals(query)) {
    if (!IsValidLiteral(literal))
      return ParseError("Invalid literal value");
  }
  
  return ParseSuccess(parsed_query);
}
```

**Test Coverage:**
- SEC-IV-06: Malformed query rejection
- SEC-IV-07: Parameter binding validation
- SEC-IV-08: Type confusion detection
- SEC-IV-09: Integer overflow detection
- SEC-IV-10: Operator precedence verification

#### 1.3 LLM Input Validation

**File:** `src/llm/prompt_processor.cpp`, `include/llm/prompt_processor.h`

**Validation Points:**
- Prompt length limits (4096 chars default)
- Special character filtering
- Character encoding validation (UTF-8)
- Prompt injection pattern detection
- Token count validation

**Implementation Pattern:**

```cpp
ValidateResult ValidateLLMInput(std::string_view prompt) {
  // 1. Length check
  if (prompt.length() > MAX_PROMPT_LENGTH)
    return {false, "Prompt exceeds maximum length"};
  
  // 2. Encoding validation
  if (!IsValidUTF8(prompt))
    return {false, "Invalid UTF-8 encoding"};
  
  // 3. Character set validation
  for (char c : prompt) {
    if (!IsAllowedCharacter(c))
      return {false, "Invalid character in prompt"};
  }
  
  // 4. Injection pattern detection
  for (const auto& pattern : INJECTION_PATTERNS) {
    if (prompt.find(pattern) != std::string::npos)
      return {false, "Prompt injection pattern detected"};
  }
  
  // 5. Token count validation
  auto tokens = Tokenize(prompt);
  if (tokens.size() > MAX_TOKENS)
    return {false, "Exceeds token limit"};
  
  return {true, ""};
}
```

**Test Coverage:**
- SEC-IV-11: Oversized prompt rejection
- SEC-IV-12: Invalid character rejection
- SEC-IV-13: Prompt injection detection
- SEC-IV-14: Token count validation
- SEC-IV-15: Encoding validation

#### 1.4 Fuzz Testing Campaign

**Scope:** libFuzzer targets for critical parsers

**Targets:**
1. `fuzz_query_parser` — Query parser with malformed inputs
2. `fuzz_http_parser` — HTTP request parser
3. `fuzz_protocol_deserializer` — Protocol message deserialization

**Execution:**
```bash
# Create fuzzer targets
clang++ -fsanitize=fuzzer,address src/fuzz/fuzz_query_parser.cpp -o fuzz_qp
clang++ -fsanitize=fuzzer,address src/fuzz/fuzz_http_parser.cpp -o fuzz_http

# Run campaign
./fuzz_qp corpus/ -max_len=10000 -rss_limit_mb=2048 -timeout=5 &
./fuzz_http corpus/ -max_len=10000 -rss_limit_mb=2048 -timeout=5 &
wait

# Analyze results
for crash in crash-*; do
  echo "Crash: $crash"
  ./fuzz_qp "$crash" 2>&1
done
```

**Expected Outcomes:**
- 1M+ fuzz inputs without crashes in stable code paths
- All crashes analyzed and root causes documented
- Regression tests added for each crash scenario

---

## Focus Area 2: Transport Security

### Objective

Enforce TLS 1.3+ with strongest cipher suites, implement certificate pinning, and automate certificate rotation.

### Audit Scope

#### 2.1 TLS Configuration Audit

**File:** `include/network/tls_config.h`, `src/network/tls_config.cpp`

**Current State Check:**

```bash
# Check minimum TLS version
grep -r "TLS_1_0\|TLS_1_1\|TLS_1_2" include/network/ src/network/

# Check cipher suites
grep -r "DES\|RC4\|MD5\|SHA1\|ECB" include/network/ src/network/

# Check certificate verification
grep -r "verify\|certificate\|cert" include/network/ src/network/ | grep -i skip
```

#### 2.2 TLS 1.3+ Enforcement

**Implementation:**

```cpp
class TLSConfig {
 public:
  // Enforce TLS 1.3 minimum
  static constexpr int MIN_TLS_VERSION = TLS_1_3;
  static constexpr int MAX_TLS_VERSION = TLS_1_3;
  
  // Approved cipher suites (AEAD only)
  static constexpr std::array<std::string_view, 2> APPROVED_CIPHERS = {
    "TLS_AES_256_GCM_SHA384",      // AES-256-GCM
    "TLS_CHACHA20_POLY1305_SHA256", // ChaCha20-Poly1305
  };
  
  // No deprecated algorithms
  static constexpr std::array<std::string_view, 0> DEPRECATED_CIPHERS = {};
  
  static bool IsApprovedCipherSuite(std::string_view cipher) {
    return std::find(APPROVED_CIPHERS.begin(), APPROVED_CIPHERS.end(), cipher)
        != APPROVED_CIPHERS.end();
  }
};
```

**Test Coverage:**
- SEC-TLS-01: TLS 1.3 minimum enforcement
- SEC-TLS-02: Weak cipher rejection
- SEC-TLS-03: Certificate validation
- SEC-TLS-04: Handshake verification
- SEC-TLS-05: Forward secrecy validation

#### 2.3 Certificate Pinning

**Implementation:**

```cpp
class CertificatePinning {
 private:
  std::map<std::string, std::vector<std::string>> pinned_hashes_;
  
 public:
  bool VerifyCertificate(const X509* cert, const std::string& hostname) {
    // Get certificate hash
    auto cert_hash = ComputeCertificateHash(cert);
    
    // Check if hash is pinned
    auto pinned = pinned_hashes_.find(hostname);
    if (pinned == pinned_hashes_.end()) {
      return false; // Fail closed if no pins found
    }
    
    // Verify hash matches one of pinned hashes
    return std::find(pinned->second.begin(), pinned->second.end(), cert_hash)
        != pinned->second.end();
  }
  
  void AddPinnedCertificate(const std::string& hostname, 
                           const std::string& cert_hash) {
    pinned_hashes_[hostname].push_back(cert_hash);
  }
};
```

**Test Coverage:**
- SEC-TLS-06: Valid certificate acceptance
- SEC-TLS-07: Invalid certificate rejection
- SEC-TLS-08: Pin rotation ceremony
- SEC-TLS-09: Fallback mechanism
- SEC-TLS-10: Pin backup validation

#### 2.4 Certificate Rotation Automation

**Implementation:**

```cpp
class CertificateRotationManager {
 private:
  std::chrono::system_clock::time_point expiration_;
  std::chrono::hours renewal_threshold_{24 * 30}; // 30 days
  
 public:
  bool ShouldRenew() {
    auto now = std::chrono::system_clock::now();
    return (expiration_ - now) <= renewal_threshold_;
  }
  
  RenewalResult AutoRenew() {
    // 1. Request new certificate
    auto new_cert = RequestNewCertificate();
    if (!new_cert) return RenewalResult::Failed();
    
    // 2. Validate new certificate
    if (!ValidateCertificate(new_cert)) return RenewalResult::Failed();
    
    // 3. Install new certificate (zero-downtime)
    if (!InstallCertificateZeroDowntime(new_cert)) 
      return RenewalResult::Failed();
    
    // 4. Notify cluster members
    BroadcastCertificateUpdate(new_cert);
    
    return RenewalResult::Success();
  }
};
```

---

## Focus Area 3: Memory Safety

### Objective

Ensure zero memory-safety violations (buffer overflow, use-after-free, uninitialized reads, undefined behavior) in release-critical paths.

### Audit Scope

#### 3.1 Sanitizer Suite Execution

**Prerequisite:** Build with sanitizers enabled

```bash
# Build with AddressSanitizer
cmake --preset community-asan
ctest -L release_critical --output-on-failure

# Build with MemorySanitizer
cmake --preset community-msan
ctest -L release_critical --output-on-failure

# Build with UndefinedBehaviorSanitizer
cmake --preset community-ubsan
ctest -L release_critical --output-on-failure

# Build with ThreadSanitizer
cmake --preset community-tsan
ctest -L release_critical --output-on-failure
```

#### 3.2 RAII Audit Pattern

**Required Pattern:**

```cpp
// ✅ CORRECT: RAII with unique_ptr
class ConnectionPool {
  std::vector<std::unique_ptr<Connection>> connections_;
  
 public:
  void AddConnection(std::unique_ptr<Connection> conn) {
    connections_.push_back(std::move(conn));
  }
  // Automatically cleaned up on destruction
};

// ❌ WRONG: Manual new/delete
class BadConnectionPool {
  std::vector<Connection*> connections_; // Manual management!
  
 public:
  ~BadConnectionPool() {
    for (auto* conn : connections_) {
      delete conn; // Leaks if exception thrown!
    }
  }
};
```

**Audit Commands:**

```bash
# Find manual new usage
grep -r "new " include/server src/server | grep -v "new_" | grep -v "new\." | grep -v comment

# Find manual delete usage
grep -r "delete " include/server src/server | grep -v "delete_" | grep -v comment

# Find raw pointers in public APIs
grep -r "\*" include/server/http_server.h | grep -v "**" | grep -v comment

# Find missing RAII patterns
grep -r "malloc\|calloc\|realloc" include/server src/server
```

#### 3.3 Memory Safety Fixes

**Priority Ordering:**
1. **CRITICAL:** Heap buffer overflow, stack buffer overflow, use-after-free, double-free
2. **HIGH:** Memory leaks > 1MB, uninitialized memory reads, integer overflow to allocation
3. **MEDIUM:** Memory leaks < 1MB, buffer underflow, type confusion

**Fix Template:**

```cpp
// BEFORE: Unsafe manual management
void* buffer = malloc(size);
if (!buffer) return error;
// ... use buffer ...
free(buffer); // Leaks if exception thrown!

// AFTER: Safe RAII wrapper
std::unique_ptr<uint8_t[]> buffer(new uint8_t[size]);
if (!buffer) return error;
// ... use buffer ...
// Automatically freed on scope exit or exception
```

#### 3.4 Memory Safety Test Coverage

**Test Cases (SEC-MEM-01..15):**

| Test ID | Scenario | Expected Result |
|---------|----------|-----------------|
| SEC-MEM-01 | Buffer overflow attempt | Crash/error detected |
| SEC-MEM-02 | Use-after-free | Crash/error detected |
| SEC-MEM-03 | RAII compliance | All patterns approved |
| SEC-MEM-04 | Secret memory zeroing | Memory cleared after use |
| SEC-MEM-05 | Stack overflow prevention | Safe bounds enforced |
| SEC-MEM-06 | Double-free prevention | Only one delete |
| SEC-MEM-07 | Uninitialized read | Error detected |
| SEC-MEM-08 | Integer overflow in alloc | Safe size check |
| SEC-MEM-09 | Type confusion | Type mismatch detected |
| SEC-MEM-10 | Exception safety | No leaks on throw |
| SEC-MEM-11 | Move semantics correctness | Moved-from object safe |
| SEC-MEM-12 | Copy-on-write safety | Proper refcounting |
| SEC-MEM-13 | Pointer arithmetic bounds | Bounds checked |
| SEC-MEM-14 | String buffer overrun | Max length enforced |
| SEC-MEM-15 | Array bounds | Index validated |

---

## Focus Area 4: Concurrency

### Objective

Ensure thread-safety through proper synchronization, atomic operations, and lock-free algorithms validated with ThreadSanitizer.

### Audit Scope

#### 4.1 ThreadSanitizer Analysis

**Execution:**

```bash
# Build with ThreadSanitizer
cmake --preset community-tsan
ctest -L release_critical --output-on-failure 2>&1 | tee tsan_output.log

# Extract data races
grep "WARNING: ThreadSanitizer" tsan_output.log | wc -l
grep "data race" tsan_output.log | cut -d: -f1 | sort -u | wc -l
```

#### 4.2 Concurrency Test Coverage

**Test Cases (SEC-RACE-01..10):**

| Test ID | Scenario | Expected Result |
|---------|----------|-----------------|
| SEC-RACE-01 | Mutex protects shared state | No race detected |
| SEC-RACE-02 | Atomic operations correct | Correct value at end |
| SEC-RACE-03 | Lock acquisition order consistent | No deadlock |
| SEC-RACE-04 | No concurrent data races | No data races |
| SEC-RACE-05 | Exception-safe locking | Lock released on throw |
| SEC-RACE-06 | Read-write lock fairness | No starvation |
| SEC-RACE-07 | Condition variable waits | Proper spurious wake-up handling |
| SEC-RACE-08 | Lock-free queue correctness | All items processed |
| SEC-RACE-09 | Memory ordering validation | Acquire-release correct |
| SEC-RACE-10 | Deadlock detection | No circular waits |

#### 4.3 Lock-Free Code Audit

**Pattern Validation:**

```cpp
// ✅ CORRECT: Proper memory ordering
std::atomic<bool> flag(false);
std::vector<int> data(100);

// Writer: populate data, then signal
for (int i = 0; i < 100; i++) data[i] = i;
flag.store(true, std::memory_order_release); // Release semantics

// Reader: read flag, then access data
if (flag.load(std::memory_order_acquire)) { // Acquire semantics
  // Data is guaranteed to be visible here
}

// ❌ WRONG: Missing proper ordering
std::atomic<bool> flag(false);
flag.store(true); // Relaxed (default) - race condition!
// Reader doesn't wait for data to be visible
```

---

## Focus Area 5: Error Path Security

### Objective

Ensure error messages don't leak sensitive information, permission checks default to DENY, and all security-relevant operations are audited.

### Audit Scope

#### 5.1 Error Message Sanitization

**Patterns to Remove:**

```
✗ Sensitive Information:
  - Passwords / API keys / tokens
  - Internal file paths (/home/username/..., /usr/bin/...)
  - Stack traces with memory addresses (0x7ffexxxx)
  - Database connection strings
  - Usernames and email addresses
  - System internals (/proc, /sys, /dev)

✓ Safe Information:
  - Generic error codes (ERR-1001)
  - Business-level messages ("Unable to process request")
  - Non-identifying context ("Database unavailable")
```

**Sanitization Function:**

```cpp
std::string SanitizeErrorMessage(std::string_view raw_error) {
  // 1. Check for sensitive patterns
  static constexpr std::array<std::string_view, 15> SENSITIVE_PATTERNS = {
    "password", "secret", "token", "api_key", "credentials",
    "/home/", "/root/", "/usr/", "0x", "stack trace",
    "at line ", "at function", "Database:", "Connection:",
    "user:", "root:",
  };
  
  // 2. Replace with generic message if sensitive data found
  for (const auto& pattern : SENSITIVE_PATTERNS) {
    if (raw_error.find(pattern) != std::string::npos) {
      return "An error occurred"; // Generic message
    }
  }
  
  // 3. Strip stack traces
  if (raw_error.find("Traceback") != std::string::npos ||
      raw_error.find("at 0x") != std::string::npos) {
    return "An error occurred";
  }
  
  return std::string(raw_error);
}
```

#### 5.2 Fail-Closed Defaults

**Pattern:**

```cpp
// ✅ CORRECT: Default DENY
bool IsAuthorized(const Request& req) {
  // Default: unauthorized
  bool authorized = false;
  
  // Only set to true if ALL checks pass
  if (IsAuthenticatedUser(req) &&
      HasRequiredRole(req.user, req.resource) &&
      IsResourceAvailable(req.resource)) {
    authorized = true;
  }
  
  return authorized; // Safe default: false
}

// ❌ WRONG: Default ALLOW
bool IsAuthorized(const Request& req) {
  bool authorized = true; // DANGER!
  
  // Try to find reason to deny
  if (!IsAuthenticatedUser(req)) {
    authorized = false;
  }
  // ... but other checks might not run!
  
  return authorized;
}
```

#### 5.3 Audit Logging

**Events to Log:**

```
Security Events:
- Authentication attempts (success/failure)
- Privilege escalation
- Configuration changes
- Permission denials
- Encryption state changes
- Certificate operations
- Access to sensitive data
- Administrative operations

Log Format:
TIMESTAMP|EVENT_TYPE|USER|RESOURCE|RESULT|DETAILS

Example:
2026-07-28T11:42:38Z|AUTH_ATTEMPT|admin|api|success|IP=192.168.1.1
2026-07-28T11:42:39Z|PRIVILEGE_CHANGE|alice|database|success|from=user to=admin
2026-07-28T11:42:40Z|CONFIG_CHANGE|admin|tls_config|success|enforce_tls_1_3=true
```

**Implementation:**

```cpp
class AuditLogger {
 private:
  std::ofstream audit_log_;
  std::mutex log_mutex_;
  
 public:
  void LogSecurityEvent(std::string_view event_type,
                       std::string_view user,
                       std::string_view resource,
                       bool success,
                       std::string_view details) {
    std::lock_guard<std::mutex> lock(log_mutex_);
    
    auto timestamp = GetCurrentTimestamp();
    auto result = success ? "success" : "failure";
    
    audit_log_ << timestamp << "|" << event_type << "|"
               << user << "|" << resource << "|" << result
               << "|" << details << "\n";
    audit_log_.flush(); // Ensure immediate persistence
  }
};
```

---

## Test Execution Plan

### Build Configuration

```bash
# Build all security test targets
cmake --preset community-release
cmake --build --preset community-release --target test_phase4_security_hardening_focused

# Run Phase 4 tests
ctest -L phase4 --output-on-failure --timeout 300

# Run memory-safety tests
cmake --preset community-asan
ctest -L release_critical --output-on-failure
```

### Test Registration

All Phase 4 tests are registered in the release_critical label group:

```bash
# List Phase 4 tests
ctest -L phase4 --verbose

# Run with detailed output
ctest -L phase4 -V --output-on-failure

# Generate coverage report
ctest -L phase4 --coverage --output-on-failure
```

### Success Criteria

- ✅ All 50+ tests passing
- ✅ ASan/MSan/UBSan clean (0 findings)
- ✅ TSan clean (0 data races)
- ✅ Fuzz campaign complete (1M+ inputs)
- ✅ Zero CRITICAL findings
- ✅ Evidence archived

---

## Evidence Archival

### Test Execution Evidence

```bash
# Save test results
ctest -L phase4 --output-on-failure -T Test > phase4_tests_results.log

# Save sanitizer runs
cmake --preset community-asan
ctest -L release_critical -T Test > asan_results.log

# Save fuzzer outputs
ls -la fuzz_*/crash-* fuzz_*/leak-* 2>/dev/null > fuzz_findings.log
```

### Documentation Deliverables

1. **Security Gap Burndown Scoreboard**
   - By module: server, llm, sharding
   - By finding type: injection, buffer overflow, race condition, memory leak
   - Status: OPEN → IN_PROGRESS → RESOLVED

2. **Test Suite Summary**
   - Test count: 50+ tests (SEC-IV/TLS/MEM/RACE/ERR)
   - Pass rate: 100%
   - Execution time: < 300 seconds

3. **Sanitizer Evidence Bundle**
   - ASan: Clean (0 findings)
   - MSan: Clean (0 findings)
   - UBSan: Clean (0 findings)
   - TSan: Clean (0 data races)

4. **Fuzz Campaign Results**
   - Total inputs: 1M+
   - Crashes found: X (fixed)
   - Hangs found: Y (analyzed)
   - Coverage: Z%

5. **GA Sign-Off Checklist**
   - [ ] Zero CRITICAL findings verified
   - [ ] 50+ tests passing
   - [ ] All sanitizers clean
   - [ ] Fuzz campaign complete
   - [ ] Evidence archived
   - [ ] Human review complete

---

## Next Steps

1. **Immediate (Week 1):**
   - [ ] Run baseline sanitizer suite
   - [ ] Audit all three modules (server, llm, sharding)
   - [ ] Prioritize findings by severity

2. **Short-term (Weeks 2-4):**
   - [ ] Implement input validation hardening
   - [ ] Fix all CRITICAL memory-safety issues
   - [ ] Enforce TLS 1.3+

3. **Medium-term (Weeks 5-6):**
   - [ ] Run fuzz testing campaign
   - [ ] Fix all TSan data races
   - [ ] Create 50+ security tests

4. **Long-term (Week 7-8):**
   - [ ] Complete gap burndown
   - [ ] Archive evidence
   - [ ] Prepare for GA sign-off

---

## References

- Root Roadmap: `ROADMAP.md` § Phase 4
- Branch Governance: `BRANCHING_STRATEGY.md`
- Security Policy: `SECURITY.md`
- Sanitizer Evidence: `docs/security/GA_SANITIZER_EVIDENCE_BUNDLE.md`
- Pentest Evidence: `security/pentest/GA_PENTEST_EVIDENCE_BUNDLE.md`
