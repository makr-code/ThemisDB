# Gap Scanner v3 — Extended Categories Roadmap

**Status:** Strategic Planning  
**Date:** 2026-05-18  
**Current Scope:** v2 scans 7,500 gaps (unimplemented, stubs, TODOs)  
**Proposed:** 8 new categories targeting Security, Memory, Performance, Audit

---

## 📊 Current Scanner Coverage (v2)

| Category | Count | Pattern | Severity |
|----------|-------|---------|----------|
| Unimplemented | 2,565 | `throw std::*not_implemented*` | CRITICAL |
| STUB Documented | 164 | 4-line STUB/SIMULATION note | INTENTIONAL |
| STUB Undocumented | ? | `return {}; // STUB` | HIGH |
| Mock Frameworks | ? | `EXPECT_CALL`, `MOCK_METHOD` | LOW |
| Test-Only Code | ? | `TEST_F`, `#ifdef TEST` | LOW |
| Platform Fallback | ? | `#ifdef THEMIS_ENABLE_*` | INTENTIONAL |
| Disabled Code | ? | `#if 0 ... #endif` | LOW |
| TODO/FIXME | 15 | `// TODO:`, `// FIXME:` | MEDIUM |
| Technical Debt | ? | `// HACK:`, `// DEBT:` | LOW |
| **Total v2** | **7,500** | — | — |

---

## 🔒 Category 1: SECURITY GAPS

### Rationale
ThemisDB handles sensitive data (customer records, PII, encryption keys). Security gaps are **CRITICAL**.

### Patterns to Detect

#### 1.1 Unsafe Functions
```cpp
// 🔴 CRITICAL — Buffer overflow risk
strcpy(buffer, user_input);           // Use strncpy/std::string
sprintf(buf, "%s", data);             // Use snprintf/format
gets(input);                          // NEVER USE
scanf("%s", buffer);                  // Use scanf_s or std::cin
strcpyn_s(dst, src, -1);              // Unbounded copy
```

**Regex:** `strcpy|sprintf\s*\(|scanf\s*\(%[^n]|gets\s*\(`

#### 1.2 Missing Input Validation
```cpp
// 🔴 CRITICAL — No bounds check
void process_packet(const uint8_t* data, size_t len) {
    int offset = data[0];              // No validation
    return data[offset];               // Out-of-bounds read!
}
```

**Heuristic:** Function takes `size_t len` parameter but never uses it → HIGH

#### 1.3 Hardcoded Secrets
```cpp
// 🔴 CRITICAL — Exposed key
const char* API_KEY = "sk-abc123xyz456";
const char* DB_PASSWORD = "admin123";
```

**Regex:** `(API_KEY|PASSWORD|SECRET|TOKEN)\s*=\s*["'].*["']`

#### 1.4 Missing Null Checks on Pointer Operations
```cpp
// 🔴 HIGH — Null dereference risk
void process(FileHandle* fh) {
    fh->seek(0);                      // No null check!
    fh->read(buffer);
}
```

**Pattern:** `var->method()` without prior `if (var)` check → HIGH

#### 1.5 SQL/Command Injection Risk
```cpp
// 🔴 CRITICAL — String concatenation
std::string query = "SELECT * FROM users WHERE id=" + user_id;
db.execute(query);                     // Injection risk!
```

**Pattern:** `std::string` + concatenation with user input, then `execute/sql` → CRITICAL

#### 1.6 Unchecked Error Returns
```cpp
// 🔴 HIGH — Ignoring error status
status_code = validate_input(data);    // Returned but not checked
process_data(data);                    // May be invalid!
```

**Pattern:** Function call returns `status_code/error/bool` but is immediately discarded → HIGH

#### 1.7 Missing Crypto/Hashing
```cpp
// 🟠 HIGH — Storing plaintext password
void store_password(const std::string& pwd) {
    db.insert("passwords", {pwd});     // Should be hashed!
}
```

**Heuristic:** Field name contains `password|secret|key` but no hash function in same block → HIGH

### Implementation Effort
- **v3-security-gaps.py**: 200 lines
- **Patterns:** 7 high-confidence patterns + 3 heuristic checks
- **False-Positive Rate:** ~15% (needs domain context)
- **ThemisDB Impact:** 🔴 **CRITICAL** — will find 50-100 security gaps

### Estimated Gaps in ThemisDB
- Hardcoded credentials: 2-5
- Missing validation: 20-30
- Unsafe functions: 5-10
- Null dereference risks: 15-25
- SQL injection risks: 3-8
- **Total:** ~50-78 SECURITY gaps

---

## 💾 Category 2: MEMORY SAFETY GAPS

### Rationale
C++ memory management is error-prone. Gaps here cause crashes, leaks, UAF.

### Patterns to Detect

#### 2.1 Raw `new/delete` Without RAII
```cpp
// 🔴 CRITICAL — Memory leak risk
DataBuffer* buf = new DataBuffer(size);
process(buf);
// delete buf;  ← Forgotten!
```

**Pattern:** `new Type(` without corresponding `delete` in same scope or destructor → CRITICAL

#### 2.2 Pointer Arithmetic Without Bounds
```cpp
// 🟠 HIGH — No bounds check
uint8_t* data_ptr = buffer;
for (int i = 0; i < unknown_size; i++) {
    process(data_ptr[i]);              // Unchecked read!
}
```

**Pattern:** Pointer arithmetic (`ptr + i`, `ptr[i]`) without prior bounds validation → HIGH

#### 2.3 `delete` Without `nullptr` Assignment
```cpp
// 🟠 HIGH — Double-free or use-after-free risk
delete obj;
// obj still points to freed memory!
if (obj) { /* ... */ }                 // Danger!
```

**Pattern:** `delete` followed by usage of same pointer → HIGH

#### 2.4 `std::move` on Non-Movable Types
```cpp
// 🟡 MEDIUM — May not compile or leak
std::unique_ptr<T> src;
std::unique_ptr<T> dst = std::move(src);
// But if type doesn't support move, fallback to copy?
```

**Pattern:** `std::move` on non-movable type or unique_ptr without checking move semantics → MEDIUM

#### 2.5 Unchecked `malloc/realloc`
```cpp
// 🟠 HIGH — No null check
void* buf = malloc(size);
memcpy(buf, src, size);                // buf might be nullptr!
```

**Pattern:** `malloc|calloc|realloc` result not checked before use → HIGH

#### 2.6 Array Out-of-Bounds (Static)
```cpp
// 🔴 CRITICAL — Buffer overflow
uint8_t buffer[256];
for (int i = 0; i < 500; i++) {
    buffer[i] = data[i];               // Overflow!
}
```

**Heuristic:** Loop condition `i < N` with array `[M]` where N > M → CRITICAL

#### 2.7 Shared Pointer Cycles
```cpp
// 🟡 MEDIUM — Memory leak
struct Node {
    std::shared_ptr<Node> next;
    std::shared_ptr<Node> prev;        // Cycle! Leak!
};
```

**Pattern:** Two `shared_ptr` pointing to each other in same struct → MEDIUM

### Implementation Effort
- **v3-memory-gaps.py**: 250 lines
- **Patterns:** 5 high-confidence + 2 flow-analysis heuristics
- **False-Positive Rate:** ~20% (needs control-flow tracking)
- **ThemisDB Impact:** 🔴 **HIGH** — distributed system, lots of memory ops

### Estimated Gaps in ThemisDB
- `new` without `delete`: 15-30
- Pointer arithmetic unvalidated: 10-20
- `delete` without nullptr: 5-10
- Array bounds issues: 8-15
- `malloc` unchecked: 3-8
- **Total:** ~41-83 MEMORY gaps

---

## ⚡ Category 3: PERFORMANCE GAPS

### Rationale
ThemisDB targets <50ms latency for queries. Performance gaps block SLOs.

### Patterns to Detect

#### 3.1 O(n²) Nested Loops
```cpp
// 🟠 HIGH — Performance risk
for (auto& item1 : container) {
    for (auto& item2 : container) {     // Nested loop!
        if (match(item1, item2)) { /* ... */ }
    }
}
```

**Pattern:** Nested loops on same/similar containers → HIGH (with size heuristic)

#### 3.2 String Concatenation in Loop
```cpp
// 🟠 HIGH — Repeated allocations
std::string result;
for (int i = 0; i < 1000; i++) {
    result += get_chunk(i);             // Reallocates each time!
}
```

**Pattern:** `string += expr` inside loop → HIGH

#### 3.3 `find()` in Loop (Repeated Searches)
```cpp
// 🟠 HIGH — Repeated linear search
for (auto& item : items) {
    if (container.find(item) != end) {  // O(n) per iteration!
        // O(n²) total
    }
}
```

**Pattern:** `find|count|contains` inside loop over another collection → HIGH

#### 3.4 Lock Contention (Coarse-Grained Locks)
```cpp
// 🟠 HIGH — Scalability bottleneck
{
    std::lock_guard<std::mutex> lock(global_mutex);
    for (int i = 0; i < 1000; i++) {    // Lock held too long!
        process_item(i);
    }
}
```

**Pattern:** Large loop body inside critical section → HIGH (needs heuristic)

#### 3.5 Missing `constexpr`/`inline` on Utility Functions
```cpp
// 🟡 MEDIUM — Runtime cost for compile-time computable
int get_config_value(int type) {
    if (type == CONFIG_TIMEOUT) return 30000;
    if (type == CONFIG_RETRIES) return 3;
}
```

**Pattern:** Small function with only literals/constants returned → MEDIUM

#### 3.6 Memory Allocation in Hot Path
```cpp
// 🟠 HIGH — Repeated allocation
void hot_function() {
    std::vector<T> temp;               // Allocated every call!
    temp.reserve(size);
    // ... use temp ...
}
```

**Pattern:** `new|malloc|vector()` inside frequently-called function → HIGH

#### 3.7 Unbounded Recursion (No Base Case Guard)
```cpp
// 🔴 CRITICAL — Stack overflow risk
void recursive_process(T node) {
    for (auto child : node.children) {
        recursive_process(child);       // No depth limit!
    }
}
```

**Pattern:** Recursion without `depth > MAX_DEPTH` check → CRITICAL

### Implementation Effort
- **v3-performance-gaps.py**: 300 lines
- **Patterns:** 5 medium-confidence + 2 heuristics (loop analysis)
- **False-Positive Rate:** ~25% (need control-flow analysis)
- **ThemisDB Impact:** 🔴 **HIGH** — real-time constraints

### Estimated Gaps in ThemisDB
- O(n²) patterns: 8-15
- String concat in loops: 5-10
- Repeated finds: 10-20
- Hot-path allocations: 5-10
- Missing constexpr: 10-15
- **Total:** ~38-70 PERFORMANCE gaps

---

## 📋 Category 4: AUDIT & COMPLIANCE GAPS

### Rationale
ThemisDB is GDPR/DSGVO-relevant (data storage, deletion, consent). Audit gaps = compliance risk.

### Patterns to Detect

#### 4.1 Data Access Without Logging
```cpp
// 🟠 HIGH — Compliance gap
PII* customer_data = db.retrieve(customer_id);
// No audit_log() call!
return customer_data;
```

**Heuristic:** Function accesses fields containing `customer|user|personal|pii|email|ssn` without `audit_log|log_access` → HIGH

#### 4.2 Data Deletion Without Audit Trail
```cpp
// 🔴 CRITICAL — GDPR Art. 17 risk
db.delete_record(user_id);              // No audit trail!
// Should: audit_log("DELETE", user_id, timestamp)
```

**Pattern:** `delete|erase|purge|wipe` operation on PII table without preceding audit call → CRITICAL

#### 4.3 Encryption Operations Not Logged
```cpp
// 🟠 HIGH — Compliance gap
crypto.encrypt(sensitive_data, key);    // No log!
// Should: audit_log("ENCRYPT", purpose, key_id)
```

**Pattern:** Crypto operations (`encrypt|decrypt|sign|verify`) without adjacent `log_*` call → HIGH

#### 4.4 Unencrypted Sensitive Field Storage
```cpp
// 🔴 CRITICAL — Data protection gap
struct User {
    std::string ssn;                    // Plaintext!
    std::string email;                  // Should be encrypted
};
db.insert(user);
```

**Heuristic:** Struct member named `ssn|credit_card|password|api_key|secret` without `encrypted_` prefix → CRITICAL

#### 4.5 Missing Data Classification
```cpp
// 🟡 MEDIUM — No retention policy
class CustomerData {
    std::string name;
    std::string address;
    // No retention_days, classification, or deletion marker
};
```

**Pattern:** Class/struct with PII fields but no `// DATA_CLASS:` or retention comment → MEDIUM

#### 4.6 No Consent Check Before Data Access
```cpp
// 🟠 HIGH — Consent gap
void process_marketing_email(User user) {
    send_email(user.email);             // No consent check!
}
```

**Heuristic:** Function sends/stores PII without prior `check_consent()` or `has_consent` guard → HIGH

### Implementation Effort
- **v3-audit-gaps.py**: 200 lines
- **Patterns:** 3 high-confidence + 3 heuristic (field naming, function context)
- **False-Positive Rate:** ~30% (needs business context)
- **ThemisDB Impact:** 🔴 **CRITICAL** — regulatory requirement

### Estimated Gaps in ThemisDB
- Unlogged data access: 15-25
- Unencrypted sensitive fields: 5-10
- Missing audit trails on delete: 3-8
- No consent checks: 5-10
- Missing data classification: 10-20
- **Total:** ~38-73 AUDIT gaps

---

## 🛡️ Category 5: RELIABILITY GAPS

### Rationale
Distributed system needs high availability. Gaps here cause cascading failures.

### Patterns to Detect

#### 5.1 Unhandled Exception Cases
```cpp
// 🟠 HIGH — Exception not caught
try {
    db.query(sql);
    process_result();
} catch (std::exception& e) {
    // Logs but doesn't recover!
    logger.error(e.what());
    // Returns error to caller without cleanup
}
```

**Pattern:** `catch(...)` block that doesn't implement recovery (no retry, fallback, or cleanup) → HIGH

#### 5.2 No Retry Logic on Network Failures
```cpp
// 🟠 HIGH — No resilience
Response resp = remote_service.call(request);
if (!resp.ok()) {
    return error;                       // Should retry with backoff!
}
```

**Heuristic:** Function making RPC/HTTP call without `retry_count|backoff|circuit_breaker` guard → HIGH

#### 5.3 No Circuit Breaker on Dependent Service
```cpp
// 🟠 HIGH — Cascading failure risk
for (int i = 0; i < 10000; i++) {
    Result r = failing_service.call();  // No circuit breaker!
    process(r);                         // Hammers service
}
```

**Pattern:** Loop with external service call without breaker/fallback → HIGH

#### 5.4 Missing Graceful Degradation
```cpp
// 🟡 MEDIUM — Feature not optional
if (!cache.is_available()) {
    return error;                       // Should degrade!
    // Could still serve from DB, just slower
}
```

**Pattern:** Feature returns error instead of degraded mode → MEDIUM

#### 5.5 No Timeout on Blocking Operations
```cpp
// 🟠 HIGH — Hang risk
result = queue.pop();                  // Blocks forever if queue is empty!
// Should: queue.pop_with_timeout(1s)
```

**Pattern:** `wait|pop|read|recv` without timeout parameter → HIGH

#### 5.6 Missing Resource Limits
```cpp
// 🟠 HIGH — Resource exhaustion
while (true) {
    connection = accept();              // No connection limit!
    spawn_thread(connection);           // Unbounded threads!
}
```

**Pattern:** Resource allocation in loop without `if (count >= MAX)` check → HIGH

### Implementation Effort
- **v3-reliability-gaps.py**: 220 lines
- **Patterns:** 4 medium-confidence + 2 heuristics
- **False-Positive Rate:** ~20%
- **ThemisDB Impact:** 🔴 **HIGH** — distributed system SLA

### Estimated Gaps in ThemisDB
- No retry logic: 15-25
- Missing timeouts: 8-15
- No circuit breakers: 5-10
- No graceful degradation: 5-10
- Resource limits missing: 8-12
- **Total:** ~41-72 RELIABILITY gaps

---

## ⚙️ Category 6: CONFIGURATION GAPS

### Rationale
Hardcoded values are inflexible and error-prone. Configuration gaps limit deployment flexibility.

### Patterns to Detect

#### 6.1 Hardcoded Values (Not in Config)
```cpp
// 🟠 HIGH — Not configurable
const int QUERY_TIMEOUT_MS = 5000;      // Should be from config!
const std::string DB_HOST = "localhost";// Magic value!
```

**Pattern:** `const` with literal value (not `std::getenv|config.get`) → HIGH (if > 10 bytes or numeric)

#### 6.2 No Config Validation
```cpp
// 🟠 HIGH — Bad config not detected
int timeout = config.get("timeout_ms");
// No validation! Could be negative, zero, or insane value
executor.set_timeout(timeout);
```

**Pattern:** Config value retrieved but not validated with bounds check → HIGH

#### 6.3 Missing Default Config Values
```cpp
// 🟡 MEDIUM — Config may be missing
std::string api_key = config.get("api_key");
if (api_key.empty()) {
    // Should have sensible default!
}
```

**Pattern:** Config value retrieved with no fallback/default → MEDIUM

#### 6.4 Environment Variable Not Validated
```cpp
// 🟠 HIGH — Trusts external input
const char* db_url = std::getenv("DB_URL");
db.connect(db_url);                     // No validation!
```

**Pattern:** `getenv` or environment variable used directly without validation → HIGH

### Implementation Effort
- **v3-config-gaps.py**: 150 lines
- **Patterns:** 4 patterns
- **False-Positive Rate:** ~15%
- **ThemisDB Impact:** 🟡 **MEDIUM** — deployment flexibility

### Estimated Gaps in ThemisDB
- Hardcoded values: 20-40
- No config validation: 10-20
- Missing defaults: 5-10
- **Total:** ~35-70 CONFIG gaps

---

## 📋 Category 7: THREADING & CONCURRENCY GAPS

### Rationale
ThemisDB is multi-threaded. Concurrency bugs are subtle and hard to reproduce.

### Patterns to Detect

#### 7.1 Unprotected Shared State Access
```cpp
// 🔴 CRITICAL — Race condition
int global_counter = 0;                 // Shared, no lock!
global_counter++;                       // Racy!
```

**Heuristic:** Global/static variable modified without preceding `mutex::lock|atomic` → CRITICAL

#### 7.2 Inconsistent Locking (Sometimes Locked)
```cpp
// 🔴 CRITICAL — Race condition
void process_item(Item item) {
    {
        std::lock_guard<std::mutex> lock(mutex);
        state.add(item);
    }
}

void read_state() {
    return state.get();                 // No lock! Race!
}
```

**Pattern:** Same object accessed locked in one place, unlocked in another → CRITICAL

#### 7.3 Lock Held Too Long
```cpp
// 🟠 HIGH — Scalability bottleneck
{
    std::lock_guard<std::mutex> lock(big_lock);
    for (int i = 0; i < 1000; i++) {
        expensive_operation(i);         // Lock held entire time!
    }
}
```

**Pattern:** Large block (> 10 lines) inside lock_guard → HIGH

#### 7.4 Potential Deadlock (Lock Ordering)
```cpp
// 🔴 CRITICAL — Deadlock risk
void func_a() {
    lock(mutex1);
    lock(mutex2);                       // Order 1→2
}

void func_b() {
    lock(mutex2);
    lock(mutex1);                       // Order 2→1 — DEADLOCK!
}
```

**Pattern:** Two functions lock same mutexes in different order → CRITICAL (heuristic)

#### 7.5 Condition Variable Without Spurious Wakeup Guard
```cpp
// 🟠 HIGH — Spurious wakeup bug
cv.wait(lock);                          // Woke up, but condition false!
// Should: while (!condition) cv.wait(lock);
process();
```

**Pattern:** `condition_variable::wait()` not in `while` loop → HIGH

#### 7.6 Data Race on `std::vector`
```cpp
// 🔴 CRITICAL — Race condition
std::vector<T> shared_vec;
// Thread 1: shared_vec.push_back(item);
// Thread 2: for (auto& x : shared_vec) { /* Race! */ }
```

**Heuristic:** `std::vector|std::string|std::list` without `mutex` guard + multi-threaded context → CRITICAL

### Implementation Effort
- **v3-threading-gaps.py**: 280 lines
- **Patterns:** 3 high-confidence + 3 heuristic (needs thread-safety tracking)
- **False-Positive Rate:** ~30% (hard to detect without alias analysis)
- **ThemisDB Impact:** 🔴 **CRITICAL** — distributed/multi-threaded

### Estimated Gaps in ThemisDB
- Unprotected shared access: 10-20
- Inconsistent locking: 5-10
- Lock ordering issues: 3-8
- Condition variable bugs: 5-10
- Data races on containers: 5-10
- **Total:** ~28-58 THREADING gaps

---

## 📡 Category 8: API DESIGN GAPS

### Rationale
Poor API design causes integration friction, misuse, and bugs.

### Patterns to Detect

#### 8.1 No Error Code Documentation
```cpp
// 🟡 MEDIUM — Unclear contract
error_code process(const Data& d) {
    // Caller doesn't know what errors are possible!
    // Should: ///< @return 0=OK, -1=invalid, -2=timeout
}
```

**Pattern:** Function returning `error_code|status|bool` without doc comment listing possible values → MEDIUM

#### 8.2 No Version/Deprecation Marker
```cpp
// 🟡 MEDIUM — API stability unclear
void process_old_format(const Buffer& buf) {
    // Is this deprecated? When will it be removed?
    // Should: ///< @deprecated Since v0.5, use process_new_format()
}
```

**Pattern:** Public function with no version/deprecation comment → MEDIUM (older APIs)

#### 8.3 Non-Const Reference Parameter
```cpp
// 🟠 HIGH — Unclear ownership/intent
void process(std::vector<T>& items) {
    // Is this modified? Cleared? Takes ownership?
    // Should: (const std::vector<T>&) or (std::vector<T>&&)
}
```

**Pattern:** Public function parameter is non-const reference without clear ownership semantics → HIGH

#### 8.4 Missing Null Check Documentation
```cpp
// 🟠 HIGH — Caller confused
void process(const Obj* obj) {
    // Can obj be nullptr? Unclear!
    // Should: ///< @param obj Non-null pointer to...
}
```

**Pattern:** Public function takes pointer without doc on null behavior → HIGH

#### 8.5 Inconsistent Naming
```cpp
// 🟡 MEDIUM — Confusing API
class DataManager {
    void fetch_data();      // Sync or async?
    void retrieve();        // Same as fetch?
    void load();           // What's the difference?
    void get_items();      // Why different verb?
};
```

**Heuristic:** Class with 3+ similar methods (`get_*`, `fetch_*`, `retrieve`, `load`) → MEDIUM

### Implementation Effort
- **v3-api-gaps.py**: 180 lines
- **Patterns:** 3 medium-confidence + 2 heuristics
- **False-Positive Rate:** ~25% (subjective)
- **ThemisDB Impact:** 🟡 **MEDIUM** — internal API quality

### Estimated Gaps in ThemisDB
- Missing error documentation: 20-40
- Missing deprecation markers: 5-10
- Unclear ownership semantics: 10-20
- Inconsistent naming: 5-10
- **Total:** ~40-80 API gaps

---

## 🎯 Implementation Roadmap

### Phase 1: High-Value Additions (Next Month)
**Effort:** ~2-3 weeks  
**Expected New Gaps:** ~200-300

1. ✅ Security Gaps (50-80 gaps)
   - Unsafe functions, hardcoded secrets, missing validation
2. ✅ Memory Gaps (40-80 gaps)
   - `new/delete` without RAII, pointer arithmetic
3. ✅ Reliability Gaps (40-70 gaps)
   - No retry logic, missing timeouts, circuit breakers

**Files to Create:**
- `tools/gap_scanner_v3_security.py`
- `tools/gap_scanner_v3_memory.py`
- `tools/gap_scanner_v3_reliability.py`
- `tools/gap_scanner_v3_unified.py` (combines all)

### Phase 2: Medium-Value (Months 2-3)
**Effort:** ~2 weeks  
**Expected New Gaps:** ~100-200

4. Performance Gaps (40-70 gaps)
5. Audit/Compliance Gaps (40-75 gaps)

### Phase 3: Lower-Priority (Q3)
**Effort:** ~1-2 weeks  
**Expected New Gaps:** ~100-150

6. Configuration Gaps (35-70 gaps)
7. Threading Gaps (30-60 gaps — complex!)
8. API Design Gaps (40-80 gaps)

---

## 📈 Impact Projection

### Current State (v2)
- **Total Gaps:** 7,500
- **Breakdown:** Unimplemented (2,565), Intentional (3,940), TODO (15)

### After v3 Phase 1
- **Security:** +50-80
- **Memory:** +40-80
- **Reliability:** +40-70
- **New Total:** ~7,770-7,830 gaps

### After Full v3
- **All Categories:** +200-450 new gaps identified
- **Final Total:** ~7,700-7,950 gaps
- **Categories:** 17 instead of 9
- **Actionability:** 🚀 Much higher (security/memory/reliability gaps are immediate action items)

---

## 🚀 Recommended Next Action

**Start with Phase 1** (Security + Memory + Reliability):
1. These categories align with **ROADMAP.md** priorities (Security, Performance, Reliability)
2. High confidence patterns (low false-positive rate)
3. Clear remediation steps
4. **Biggest immediate impact** on code quality

**Command to Execute Phase 1:**
```bash
python tools/gap_scanner_v3_unified.py --categories security,memory,reliability
```

Expected output: Additional ~200-300 actionable gaps to address.

---

## 📚 References

- **ROADMAP.md** → `## Security & Compliance` section
- **FUTURE_ENHANCEMENTS.md** → Memory safety, performance targets
- **gap_scanner_v2.py** → Reference implementation for patterns
- **C++ Best Practices Instructions** → RAII, exception safety, thread safety
