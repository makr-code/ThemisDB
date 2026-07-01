# Wave 1 Gap Remediation Patterns Reference
**Edition:** Community / Enterprise / Military  
**Scope:** Phase 1-5 Scanner gaps for high-priority modules  
**Last Updated:** 2026-07-01

---

## Pattern Definitions & Remediation Strategies

This document defines the gap patterns used in Wave 1 remediation and provides remediation strategies for each.

---

## Security Patterns (S-1, S-2, S-3)

### S-1: Weak Cryptography (CWE-327, CWE-328, CWE-330)

**Detection:**
- Hardcoded crypto algorithms (MD5, DES, RC4, SHA-1)
- Weak key sizes (RSA <2048, EC <256)
- CSPRNG not seeded or predictable
- Deprecated OpenSSL APIs (MD5_*, DES_*, RC4_*)

**Remediation:**
1. Replace with modern algorithms: `SHA-256+` (not SHA-1), `AES-256-GCM` (not ECB/CBC)
2. Validate key sizes: RSA >=2048, EC >=P-256, AES >=256-bit
3. Use `std::random_device` + `std::mt19937_64` for CSPRNG, never hardcoded values
4. Update to OpenSSL 3.0+ APIs (`EVP_*` only)
5. Add unit tests validating algorithm strength

**Example Fix:**
```cpp
// ❌ BEFORE
unsigned char md5[MD5_DIGEST_LENGTH];
MD5((unsigned char*)data, len, md5);  // CWE-327: Weak hash

// ✅ AFTER
unsigned char sha256[EVP_MAX_MD_SIZE];
EVP_Digest(data, len, sha256, nullptr, EVP_sha256(), nullptr);
```

**Files in Wave 1:**
- LLM: `llama_adapter.cpp`, `inference_engine.cpp`
- Server: `auth_middleware.cpp`

---

### S-2: Hardcoded Credentials (CWE-798)

**Detection:**
- API keys, passwords in source code
- Database credentials hardcoded
- JWT secrets in binaries
- Default passwords not overridable

**Remediation:**
1. Move all credentials to environment variables or secure vaults (e.g., HashiCorp Vault)
2. Inject via constructor/factory with fail-closed default
3. Use injected validators (see Issue #5184 pattern: injected bridge)
4. Validate at initialization, fail fast if missing
5. Never log credentials (apply S-3 pattern)

**Example Fix:**
```cpp
// ❌ BEFORE
const std::string API_KEY = "sk-12345...";  // CWE-798

// ✅ AFTER
class LLMClient {
  std::string api_key_;
  std::function<std::string()> getApiKeyFn_;  // Injected
  
  LLMClient(std::function<std::string()> getKeyFn) 
    : getApiKeyFn_(getKeyFn) {
    if (!getApiKeyFn_) throw std::runtime_error("API key provider required");
  }
};
```

**Files in Wave 1:**
- LLM: `lora_training_service.cpp`, `embedding_service.cpp`
- Server: `api_handler.cpp`

---

### S-3: Information Disclosure (CWE-532, CWE-200)

**Detection:**
- Internal paths in error messages
- Database schema in exceptions
- Memory addresses in logs
- Sensitive parameters in debug output
- Stack traces with full paths

**Remediation:**
1. Sanitize all error messages (no paths, IPs, internal details)
2. Log sanitized message to customer, detailed to internal logs only
3. Use structured logging with redaction filters
4. Never dump raw exceptions to output
5. Implement error classification (user-facing vs internal)

**Example Fix:**
```cpp
// ❌ BEFORE
throw std::runtime_error("Failed to load /home/user/config.json: " + error);

// ✅ AFTER
THEMIS_ERROR("Failed to load config file: {}", error);
throw std::runtime_error("Configuration error (see logs)");  // User-safe message
```

**Files in Wave 1:**
- Server: `wire_protocol_server.cpp`, `error_mapping.cpp`
- Sharding: `shard_router.cpp`

---

## Memory Safety Patterns (M-1, M-2)

### M-1: Buffer Overflow (CWE-120, CWE-125)

**Detection:**
- Unbounded `strcpy`, `gets`, `scanf` without size
- Heap allocation mismatch (malloc size != usage)
- Array indexing without bounds check
- String operations without null terminator validation

**Remediation:**
1. Replace unsafe functions: `strcpy` → `strncpy/strlcpy`, `sprintf` → `snprintf`
2. Use `std::string` / `std::vector` (bounds-checked)
3. Add explicit bounds checks for manual indexing
4. Use `std::string_view` for read-only string parameters
5. Enable ASAN/UBSAN in debug builds

**Example Fix:**
```cpp
// ❌ BEFORE
char buffer[256];
strcpy(buffer, input);  // CWE-120: unbounded

// ✅ AFTER
std::string buffer = input;  // Bounds-safe
// OR
char buffer[256];
strncpy(buffer, input, sizeof(buffer)-1);
buffer[sizeof(buffer)-1] = '\0';
```

**Files in Wave 1:**
- LLM: `llama_adapter.cpp`, `onnx_clip_adapter.cpp`
- Query: `result_formatter.cpp`

---

### M-2: Integer Overflow (CWE-190, CWE-191)

**Detection:**
- Unvalidated size calculations (size_t overflow)
- Implicit narrowing casts (size_t → int)
- Multiplication without overflow check
- Array sizing from user input

**Remediation:**
1. Validate all size calculations before use
2. Use `std::make_signed_t` or explicit casts with bounds checks
3. Implement overflow-safe arithmetic: `if (a > MAX - b) throw;`
4. Use `std::numeric_limits` for safe conversions
5. Unit test boundary cases

**Example Fix:**
```cpp
// ❌ BEFORE
size_t total = count * item_size;  // CWE-190: overflow unchecked
auto* buf = malloc(total);

// ✅ AFTER
if (count > SIZE_MAX / item_size) throw std::overflow_error("...");
size_t total = count * item_size;
auto* buf = malloc(total);
```

**Files in Wave 1:**
- LLM: `embedding_service.cpp`, `inference_engine.cpp`
- Query: `aggregation_executor.cpp`, `join_executor.cpp`

---

## Concurrency Patterns (C-1)

### C-1: Data Race / Concurrency Issues (CWE-362, CWE-366, CWE-364)

**Detection:**
- Shared data without mutex
- Inconsistent lock ordering
- Read-write access without synchronization
- Double-checked locking anti-pattern
- TOCTOU (time-of-check-time-of-use) race

**Remediation:**
1. Protect shared data with `std::mutex` + `std::lock_guard` / `std::unique_lock`
2. Maintain consistent lock ordering (prevent deadlocks)
3. Use `std::atomic` for simple counters
4. Keep critical sections short (lock only needed data)
5. Use thread-safe data structures where appropriate

**Example Fix:**
```cpp
// ❌ BEFORE
shared_map[key] = value;  // CWE-362: race

// ✅ AFTER
{
  std::lock_guard<std::mutex> lock(map_mutex_);
  shared_map[key] = value;  // Protected by lock
}
```

**Files in Wave 1:**
- Sharding: `cross_shard_transaction.cpp`, `partition_manager.cpp`
- Index: `btree_index.cpp`, `index_coordinator.cpp`

---

## Reliability Patterns (R-1, R-2)

### R-1: Exception Safety & Error Handling

**Detection:**
- Uncaught exceptions leading to termination
- Resource leaks in error paths
- Missing error checks (return value ignored)
- Incomplete error recovery

**Remediation:**
1. Wrap all exception-throwing operations in try/catch
2. Use RAII to ensure cleanup on error
3. Check return values explicitly
4. Implement fail-fast validation at boundaries
5. Log all errors with context

**Example Fix:**
```cpp
// ❌ BEFORE
json_object_put(obj);  // May throw, not caught

// ✅ AFTER
try {
  json_object_put(obj);
} catch (const std::exception& e) {
  THEMIS_ERROR("JSON cleanup failed: {}", e.what());
  // Still safe to return (RAII cleanup already happened)
}
```

**Files in Wave 1:**
- Server: `grpc_error_mapping.cpp`, `streaming_protocol_handler.cpp`
- Query: `query_optimizer.cpp`

---

## Validation Strategy

### Per-Agent Validation

Each agent must verify:

1. **Pattern Coverage:**
   ```bash
   grep -r "CWE-327\|CWE-798\|CWE-532" src/<module> | wc -l
   # Before: X instances
   # After: ~0 instances (or documented exceptions)
   ```

2. **Test Coverage:**
   ```bash
   ctest --preset community-release --label-regex <module> -V
   # All tests PASS
   ```

3. **Security Validation:**
   ```bash
   codeql database analyze --format=csv | grep "CWE" | head -20
   # Zero new HIGH/CRITICAL issues
   ```

4. **Performance:**
   ```bash
   ctest --preset community-release --benchmark <module> 
   # <5% regression acceptable
   ```

---

## Integration Points

### Injected Bridge Pattern (See Issue #5184)

Many fixes use the "injected bridge" pattern for fail-closed behavior:

```cpp
class CryptoValidator {
  std::function<bool(const Data&)> validate_fn_;
  
public:
  CryptoValidator(std::function<bool(const Data&)> fn) 
    : validate_fn_(fn) {
    if (!validate_fn_) {
      throw std::runtime_error("Validator required (fail-closed)");
    }
  }
};
```

**Key Principle:** Default behavior is "fail" (reject), not "allow" (accept).

---

## Cross-Module Dependencies

**Watch for:**
- LLM/Server: Shared JWT validation logic → common interface
- Server/Sharding: 2PC error handling → unified error codes
- Sharding/Index: Cross-module FK validation → injected validators
- Query: Result formatting safety → shared buffer utilities

---

## Remediation Checklist Template

```markdown
## Remediation: [Module] [Pattern]

- [ ] Identify all instances (grep + manual verification)
- [ ] Implement fix per pattern guideline
- [ ] Add unit tests for fix validation
- [ ] Run full module test suite (100% PASS)
- [ ] CodeQL scan (zero new findings)
- [ ] Performance regression test (<5%)
- [ ] Document edge cases / assumptions
- [ ] Commit with detailed message
```

---

## Success Metrics per Pattern

| Pattern | Target Reduction | Validation |
|---------|------------------|-----------|
| S-1 (Weak crypto) | 95%+ | Crypto audit checklist |
| S-2 (Hardcoded creds) | 100% | Source scan + injection verify |
| S-3 (Info disclosure) | 90%+ | Log audit + error classification |
| M-1 (Buffer overflow) | 95%+ | ASAN/UBSAN clean run |
| M-2 (Integer overflow) | 90%+ | Boundary test cases |
| C-1 (Race conditions) | 90%+ | ThreadSanitizer clean |
| R-1 (Exception safety) | 85%+ | Exception path testing |

---

**Prepared by:** Claude Task Agent  
**Date:** 2026-07-01  
**Status:** Ready for Wave 1 agent deployment
