# Phase 2: 6-Week Hardening Roadmap (Full Blitz)
## Parallel Security + Server + LLM + Sharding Hardening

**Status:** Ready to execute  
**Team Size:** 10-12 developers (2-3 per workstream + QA)  
**Goal:** Reduce CRITICAL gaps from 2,261 to <500 across 4 modules  
**Timeline:** May 20 - July 1, 2026

---

## Executive Summary

### Current State (Gap Counts)
```
server:   2,722 gaps (303 CRITICAL)  🚨 NO TIMEOUT (60%+)
llm:      2,255 gaps (765 CRITICAL)  🚨 EXCEPTION SAFETY (45%+)
sharding: 1,336 gaps (453 CRITICAL)  🚨 CONSISTENCY (40%+)
security:   669 gaps (227 CRITICAL)  🚨 HARDCODED SECRETS (30%+)
────────────────────────────────────
TOTAL:    6,982 gaps (1,748 CRITICAL)
```

### Target State (End of Week 6)
```
server:   <400 gaps (<50 CRITICAL)   ✅ All handlers have timeouts + health checks
llm:      <300 gaps (<40 CRITICAL)   ✅ Full exception safety + RAII patterns
sharding: <200 gaps (<25 CRITICAL)   ✅ Consistency tested + failover works
security: <100 gaps (<15 CRITICAL)   ✅ No hardcoded secrets + validation complete
────────────────────────────────────
TOTAL:    <1,000 gaps (<150 CRITICAL) = 85% gap reduction in 6 weeks
```

---

## Team Organization (Parallel Workstreams)

### 🔒 **Security Workstream (Team A: 3 devs)** — WEEKS 1-2, then ONGOING
**Lead:** Security architect  
**Focus:** Module `security` + cross-module hardcoded secrets  
**Deliverables:** Secret rotation, validation framework, audit report

### ⚙️ **Server Workstream (Team B: 3 devs)** — WEEKS 2-3, then HARDENING
**Lead:** Backend lead  
**Focus:** Module `server` HTTP handlers + reliability  
**Deliverables:** Timeout patterns, health checks, retry logic

### 🤖 **LLM Workstream (Team C: 2 devs)** — WEEKS 4-5
**Lead:** AI systems engineer  
**Focus:** Module `llm` exception safety + resource management  
**Deliverables:** RAII patterns, exception handlers, memory audit

### 📊 **Sharding Workstream (Team D: 2 devs)** — WEEKS 5-6
**Lead:** Distributed systems engineer  
**Focus:** Module `sharding` consistency + failover  
**Deliverables:** Consistency proofs, failover tests, rebalancing logic

### 🧪 **QA/Testing (Team E: 2 devs)** — WEEKS 1-6 (CONTINUOUS)
**Lead:** QA engineer  
**Focus:** Test coverage for each fix, regression prevention  
**Deliverables:** Test suites, CI integration, gap validation

---

## WEEK 1: Security Audit + Initial Fixes

### 🎯 Objectives
- [ ] Identify all hardcoded secrets in codebase
- [ ] Create secret rotation plan
- [ ] Implement input validation framework
- [ ] Remove 50+ CRITICAL secrets

### Team A: Security Module Deep Dive

#### 1.1 Hardcoded Secrets Scan (Days 1-2)
**Files to audit:**
- `src/security/auth_manager.cpp` — API keys, DB passwords
- `src/security/credential_store.cpp` — Session tokens, JWTs
- `src/security/encryption.cpp` — Master keys, derivation seeds
- `src/server/config_handler.cpp` — Default credentials
- All `*.json` config files with embedded credentials

**Pattern Detection (use Gitleaks + manual review):**
```bash
# Already integrated; run before each commit
gitleaks detect --source . --verbose
```

**High-Confidence Secrets to Find:**
```cpp
// Pattern 1: API Keys
"api_key": "sk-...",  "secret_key": "sk_...",  "AWS_SECRET_ACCESS_KEY": "..."

// Pattern 2: Passwords
"password": "admin123",  "db_password": "...",  "root_pwd": "..."

// Pattern 3: Tokens
"access_token": "eyJ...",  "jwt_secret": "...",  "bearer_token": "..."

// Pattern 4: Keys
"private_key": "-----BEGIN RSA...",  "encryption_key": "0x...",  "master_key": "..."
```

**Output:** `ai_working/SECRETS_AUDIT_WEEK1.md` (detailed inventory)

---

#### 1.2 Input Validation Framework (Days 2-3)
**Create reusable validation patterns:**

File: `include/security/input_validator.hpp`
```cpp
namespace themis::security {

class InputValidator {
public:
  // Validates user-supplied strings (SQL injection, XSS, etc.)
  static Status validateUserInput(std::string_view input, 
                                  ValidationContext ctx);
  
  // Validates API request payloads
  static Status validateJsonPayload(const json& payload, 
                                    const std::string& schema_name);
  
  // Sanitizes output for safe display
  static std::string sanitizeForDisplay(std::string_view raw);
};

} // themis::security
```

**Implementation file:** `src/security/input_validator.cpp` (~200 lines)

**Critical APIs to validate (from security gaps):**
- All HTTP endpoint handlers in `src/server/api_handler_*.cpp`
- Database query builders in `src/storage/query_builder.cpp`
- Message processors in `src/network/message_processor.cpp`

---

#### 1.3 Security Gaps - Top 20 CRITICAL (Days 3-5)
**From gap_scan_v3_security.json, focus on:**

| Gap ID | File | Line | Type | Severity | Fix Strategy |
|--------|------|------|------|----------|--------------|
| SEC-001 | src/security/credential_store.cpp | 145 | hardcoded_password | CRITICAL | Remove; use environment variable + encrypted vault |
| SEC-002 | src/security/auth_manager.cpp | 87 | api_key_in_code | CRITICAL | Extract to `THEMIS_API_KEY` env var; document in `.env.example` |
| SEC-003 | src/server/config_handler.cpp | 234 | sql_injection_risk | CRITICAL | Add parameterized query + validation framework |
| SEC-004 | src/security/encryption.cpp | 56 | hardcoded_master_key | CRITICAL | Use AWS KMS / HashiCorp Vault for key management |
| SEC-005 | src/network/api_endpoint.cpp | 189 | missing_input_validation | CRITICAL | Call `InputValidator::validateUserInput()` on all inputs |
| SEC-006 | src/server/request_parser.cpp | 112 | buffer_overflow_risk | CRITICAL | Use bounded string operations (`std::string_view`, bounds check) |
| SEC-007 | src/security/jwt_handler.cpp | 78 | weak_jwt_secret | CRITICAL | Enforce min 32-byte secret; rotate on startup |
| SEC-008 | src/auth/oauth_handler.cpp | 234 | missing_csrf_token_validation | CRITICAL | Validate CSRF token on POST/PUT/DELETE |
| SEC-009 | src/server/session_manager.cpp | 156 | session_fixation_risk | CRITICAL | Regenerate session ID on login; invalidate old tokens |
| SEC-010 | src/content/file_upload.cpp | 445 | arbitrary_file_upload | CRITICAL | Whitelist allowed file types; verify MIME type + magic bytes |

**Effort per fix:** 30 mins - 2 hours (average 1 hour)  
**Total for TOP 20:** ~20 hours = 2.5 days  
**Team A assignment:** 1 dev = full week commitment + code review

---

#### 1.4 Secret Rotation Plan (Days 4-5)
**Document in:** `ai_working/SECRET_ROTATION_PLAN.md`

```markdown
# Secret Rotation Procedure

## Secrets Identified
- 47 hardcoded API keys (AWS, Azure, GCP)
- 12 database passwords
- 8 JWT secrets
- 3 encryption master keys
- 23 OAuth tokens

## Rotation Steps
1. **Generate new secrets** (AWS Secrets Manager / HashiCorp Vault)
2. **Update environment variables** in CI/CD (GitHub Actions)
3. **Update .env.example** (public; no real values)
4. **Deploy to staging** (test with new secrets)
5. **Rollout to production** (blue-green deployment)
6. **Invalidate old secrets** (revoke API keys, reset passwords)
7. **Audit logs** (verify no old secrets in git history)

## Timeline
- Days 1-2: Generate all new secrets
- Days 3-4: Update deployment configs
- Days 5-6: Staging validation
- Days 7-8: Production rollout (phased)

## Owners
- AWS secrets: @security-lead
- DB passwords: @database-ops
- JWT/OAuth: @auth-team
```

---

### Team E (QA): Security Test Suite (Days 1-5)

**Create test file:** `tests/security/test_input_validation.cpp`
```cpp
#include <gtest/gtest.h>
#include "security/input_validator.hpp"

class InputValidationTest : public ::testing::Test { };

TEST_F(InputValidationTest, RejectsXSSPayload) {
  std::string xss_input = "<script>alert('xss')</script>";
  EXPECT_THAT(InputValidator::validateUserInput(xss_input, {}),
              testing::Not(IsOk()));
}

TEST_F(InputValidationTest, RejectsSQLInjection) {
  std::string sql_inject = "'; DROP TABLE users; --";
  EXPECT_THAT(InputValidator::validateUserInput(sql_inject, {}),
              testing::Not(IsOk()));
}

TEST_F(InputValidationTest, AcceptsCleanInput) {
  std::string clean = "John Doe (123) 456-7890";
  EXPECT_OK(InputValidator::validateUserInput(clean, {}));
}
```

**Test coverage:** 15-20 test cases covering XSS, SQL injection, buffer overflow, path traversal

---

## WEEK 2: Server Timeout Patterns + Health Checks

### 🎯 Objectives
- [ ] Add timeout patterns to ALL HTTP handlers (50+ functions)
- [ ] Implement health check endpoints
- [ ] Add circuit breaker for cascading failures
- [ ] Reduce server CRITICAL gaps from 303 to <100

### Team B: Server Module Refactoring

#### 2.1 Timeout Pattern Framework (Days 6-8)

**File:** `include/server/request_handler_base.hpp`
```cpp
namespace themis::server {

class RequestHandlerBase {
protected:
  // Timeout configuration
  static constexpr std::chrono::milliseconds DEFAULT_TIMEOUT{5000};  // 5 sec
  static constexpr std::chrono::milliseconds DEFAULT_CONNECT_TIMEOUT{2000};
  static constexpr std::chrono::milliseconds DEFAULT_READ_TIMEOUT{3000};
  
  // Execute handler with timeout
  template<typename Func>
  Status executeWithTimeout(Func func, 
                           std::chrono::milliseconds timeout = DEFAULT_TIMEOUT) {
    std::promise<Status> result;
    std::future<Status> future = result.get_future();
    
    // Execute in thread with timeout
    std::thread([&] {
      try {
        result.set_value(func());
      } catch (const std::exception& e) {
        result.set_value(Status::InternalError(e.what()));
      }
    }).detach();
    
    // Wait with timeout
    if (future.wait_for(timeout) == std::future_status::timeout) {
      return Status::DeadlineExceeded("Request timeout");
    }
    return future.get();
  }
};

}
```

**All HTTP Handler Files to Update:**
```
src/server/async_job_api_handler.cpp        → Add timeout to 12 handlers
src/server/blob_api_handler.cpp             → Add timeout to 8 handlers
src/server/entity_api_handler.cpp           → Add timeout to 15 handlers
src/server/graph_api_handler.cpp            → Add timeout to 10 handlers
src/server/query_api_handler.cpp            → Add timeout to 20 handlers
src/server/vector_api_handler.cpp           → Add timeout to 14 handlers
src/server/websocket_handler.cpp            → Add timeout to 6 handlers
src/server/health_check_handler.cpp         → New file (health endpoints)
```

**Pattern (for each handler):**
```cpp
// BEFORE:
Status AsyncJobApiHandler::handleGetJobStatus(const Request& req) {
  auto job_id = extractJobId(req);
  auto job = jobs_.findById(job_id);  // Can block indefinitely
  return job ? serializeJob(job) : Status::NotFound();
}

// AFTER:
Status AsyncJobApiHandler::handleGetJobStatus(const Request& req) {
  return executeWithTimeout([this, &req]() {
    auto job_id = extractJobId(req);
    auto job = jobs_.findById(job_id);  // Now has 5-second timeout
    return job ? serializeJob(job) : Status::NotFound();
  }, std::chrono::milliseconds(3000));  // Custom timeout: 3 sec
}
```

**Effort:** 50 handlers × 15 mins = ~12.5 hours = 2 days

---

#### 2.2 Health Check Endpoints (Days 8-9)

**File:** `src/server/health_check_handler.cpp`
```cpp
namespace themis::server {

class HealthCheckHandler {
public:
  // GET /health — basic liveness check
  Status handleHealthCheck(const Request& req) {
    json response = {
      {"status", "ok"},
      {"timestamp", getCurrentTimestamp()},
      {"uptime_seconds", getUptimeSeconds()},
      {"version", THEMIS_VERSION}
    };
    return Response::Ok(response);
  }
  
  // GET /health/ready — readiness check (all dependencies up)
  Status handleReadinessCheck(const Request& req) {
    json readiness = {
      {"db_ready", isDatabaseConnected()},
      {"cache_ready", isCacheConnected()},
      {"index_ready", isIndexReady()},
      {"queue_ready", isQueueConnected()},
      {"overall", isReadyForTraffic()}
    };
    return readiness["overall"] ? Response::Ok(readiness) : 
           Response::ServiceUnavailable(readiness);
  }
  
  // GET /health/deep — deep diagnostics (for ops)
  Status handleDeepCheck(const Request& req) {
    json diagnostics = {
      {"memory_usage_mb", getMemoryUsageMb()},
      {"goroutine_count", getGoroutineCount()},
      {"open_connections", getOpenConnections()},
      {"timed_out_requests", getTimeoutCount()},
      {"error_rate_1m", getErrorRate(60)},
      {"latency_p99_ms", getLatencyP99()}
    };
    return Response::Ok(diagnostics);
  }
};

}
```

**Route registration in `main_server.cpp`:**
```cpp
router.get("/health", [&](const Request& r) { return hc.handleHealthCheck(r); });
router.get("/health/ready", [&](const Request& r) { return hc.handleReadinessCheck(r); });
router.get("/health/deep", [&](const Request& r) { return hc.handleDeepCheck(r); });
```

---

#### 2.3 Mutex Lock Timeouts (Days 9-10)

**All mutex.lock() calls → lock_with_timeout()**

Files to update:
```
src/server/async_job_api_handler.cpp — 15 mutex locks
src/server/job_queue_manager.cpp      — 12 mutex locks
src/server/session_manager.cpp        — 18 mutex locks
src/server/rate_limiter.cpp           — 8 mutex locks
```

**Pattern:**
```cpp
// BEFORE (can deadlock indefinitely):
std::unique_lock<std::mutex> lock(mutex_);
auto job = findJob(job_id);

// AFTER (deadlock-proof):
std::unique_lock<std::mutex> lock(mutex_, std::defer_lock);
if (!lock.try_lock_for(std::chrono::milliseconds(1000))) {
  return Status::DeadlineExceeded("Lock acquisition timeout");
}
auto job = findJob(job_id);
```

**Effort:** 53 locks × 5 mins = ~4.5 hours = 1 day

---

### Team E (QA): Server Integration Tests (Days 6-10)

**File:** `tests/server/test_timeout_handling.cpp`
```cpp
TEST_F(ServerTimeoutTest, HandlerReturnsDeadlineExceeded) {
  Request slow_req = createRequest("GET /slow?delay=10s");
  auto response = handler.handleRequest(slow_req);
  EXPECT_EQ(response.status_code, StatusCode::DEADLINE_EXCEEDED);
}

TEST_F(ServerTimeoutTest, HealthCheckRespondsQuickly) {
  auto start = Clock::now();
  auto response = handler.handleHealthCheck(Request{});
  auto elapsed = Clock::now() - start;
  EXPECT_LT(elapsed, std::chrono::milliseconds(100));
}

TEST_F(ServerTimeoutTest, ReadinessCheckDetectsDatabaseFailure) {
  simulateDatabaseFailure();
  auto response = handler.handleReadinessCheck(Request{});
  EXPECT_EQ(response.status_code, StatusCode::SERVICE_UNAVAILABLE);
  EXPECT_FALSE(response.json()["db_ready"]);
}
```

---

## WEEK 3: Server Completion + LLM Preview

### 🎯 Objectives (Server)
- [ ] Finish timeout implementation (remaining 20%)
- [ ] Complete health check framework
- [ ] Full integration testing
- [ ] Security audit on timeout logic

### 🎯 Objectives (LLM Preview)
- [ ] Identify exception-unsafe code paths (analysis phase)
- [ ] Plan exception safety strategy

### Team B: Week 3 Wrap-up
- Code review all timeout implementations
- Performance testing (verify timeouts don't impact latency)
- Stress testing (cascading failures under load)
- Target: Server CRITICAL gaps < 100

### Team C: Exception Safety Analysis (Days 11-14)

**File:** `src/llm/exception_safety_analysis.md`

**Code paths to analyze:**
```cpp
// HIGH RISK: Resource leaks on exception
src/llm/model_loader.cpp:
  - loadModelWeights() — allocates large GPU memory, no cleanup on error
  - loadTokenizer() — file I/O, buffer alloc, no exception handler
  - buildComputeGraph() — node allocation, DAG construction

src/llm/inference_engine.cpp:
  - preprocessInput() — tensor creation, no RAII
  - runInference() — GPU kernel launch, memory sync, exception unsafe
  - postprocessOutput() — result assembly, pointer arithmetic

src/llm/lora_trainer.cpp:
  - allocateGradients() — memory, no cleanup
  - backpropagate() — numerical errors, NaN propagation
  - updateWeights() — in-place ops, no rollback
```

**Exception safety classification:**
- 🔴 **No exception safety** (45% of llm gaps) → immediate risk
- 🟡 **Weak exception safety** (30% of gaps) → some leaks possible
- 🟢 **Strong/basic exception safety** (25% of gaps) → acceptable

---

## WEEK 4-5: LLM Exception Safety Hardening

### 🎯 Objectives
- [ ] Convert all manual `new`/`delete` to smart pointers
- [ ] Add try-catch to exception-unsafe paths
- [ ] Implement cleanup guards (RAII patterns)
- [ ] Reduce LLM CRITICAL gaps from 765 to <100

### Team C: Exception Safety Implementation

#### 4.1 Smart Pointer Migration (Days 15-18)

**Pattern 1: Single ownership → `std::unique_ptr`**
```cpp
// BEFORE:
class ModelLoader {
  ort::Session* session_;  // Manual cleanup required
  
  ~ModelLoader() { delete session_; }  // Easy to forget
};

// AFTER:
class ModelLoader {
  std::unique_ptr<ort::Session> session_;  // Automatic cleanup
  
  // No destructor needed; RAII handles it
};
```

**Pattern 2: Shared ownership → `std::shared_ptr`**
```cpp
// BEFORE:
class InferenceEngine {
  std::vector<Tensor*> tensors_;  // Who owns cleanup?
};

// AFTER:
class InferenceEngine {
  std::vector<std::shared_ptr<Tensor>> tensors_;  // Clear ownership
};
```

**Files to convert (critical):**
```
src/llm/model_loader.cpp        → 30+ manual new/delete
src/llm/inference_engine.cpp    → 25+ raw pointers
src/llm/lora_trainer.cpp        → 18+ unmanaged allocations
src/llm/tokenizer.cpp           → 12+ buffer pointers
src/llm/kv_cache.cpp            → 22+ cache allocations
```

**Effort:** ~150 conversions × 10 mins = ~25 hours = 3 days

---

#### 4.2 Exception Handlers (Days 18-20)

**File:** `src/llm/exception_handlers.hpp`
```cpp
namespace themis::llm {

// RAII guard for GPU memory
class GpuMemoryGuard {
  std::vector<void*> allocations_;
public:
  void* allocate(size_t bytes) {
    void* ptr = cudaMalloc(...);
    allocations_.push_back(ptr);
    return ptr;
  }
  
  ~GpuMemoryGuard() {
    // Automatic cleanup on exception or scope exit
    for (auto ptr : allocations_) {
      cudaFree(ptr);
    }
  }
};

// Exception-safe model loading
class ModelLoaderSafe {
  Status loadModel(std::string_view path) {
    try {
      GpuMemoryGuard gpu_guard;  // Auto-cleanup on exception
      
      auto weights = gpu_guard.allocate(weights_size);
      loadWeightsFromDisk(path, weights);  // May throw
      
      auto graph = buildComputeGraph(weights);  // May throw
      
      session_ = std::make_unique<Session>(graph);  // Exception-safe
      return Status::Ok();
    } catch (const std::exception& e) {
      return Status::InternalError(e.what());
      // GPU memory auto-freed by guard destructor
    }
  }
};

}
```

---

#### 4.3 Tensor & Buffer Management (Days 20-22)

**File:** `src/llm/tensor_safe.hpp`
```cpp
namespace themis::llm {

class SafeTensor {
  std::unique_ptr<float[]> data_;
  size_t size_;
  
public:
  SafeTensor(size_t size) : size_(size) {
    data_ = std::make_unique<float[]>(size);  // Exception-safe alloc
  }
  
  // Bounds-safe access
  float& operator[](size_t i) {
    if (i >= size_) throw std::out_of_range("Tensor index out of bounds");
    return data_[i];
  }
  
  // No manual cleanup needed
  ~SafeTensor() = default;  // RAII takes care of it
};

}
```

---

## WEEK 5: LLM Completion + Sharding Kickoff

### 🎯 Objectives (LLM)
- [ ] Finish exception safety (remaining 15%)
- [ ] Memory leak testing (AddressSanitizer)
- [ ] Performance validation (no regression)
- [ ] Target: LLM CRITICAL gaps < 50

### 🎯 Objectives (Sharding)
- [ ] Analyze consistency guarantees
- [ ] Plan failover logic
- [ ] Design rebalancing algorithm

### Team C: LLM Final Push (Days 22-28)
- Complete remaining smart pointer migrations
- AddressSanitizer testing to catch leaks
- Code review & security audit
- Performance benchmarks

### Team D: Sharding Analysis (Days 22-28)

**File:** `src/sharding/consistency_analysis.md`

**Consistency gaps identified:**
```
SHA-001: No write-ahead logging (WAL)           → Data loss on crash
SHA-002: Replica divergence possible            → Inconsistent replicas
SHA-003: No distributed transaction coordination → Partial commits
SHA-004: Shard rebalancing not atomic           → Split-brain scenarios
SHA-005: Failover doesn't validate state        → Invalid replica takeover
```

**Sharding consistency model needed:**
- **Strong Consistency:** Serializable, no anomalies (expensive)
- **Sequential Consistency:** Total order on writes, reads see latest
- **Eventual Consistency:** Replicas eventually sync (partition tolerant)

**Recommendation:** Sequential Consistency (good balance)

---

## WEEK 6: Sharding Hardening + Final Testing

### 🎯 Objectives
- [ ] Implement write-ahead logging for durability
- [ ] Add failover validation logic
- [ ] Test rebalancing under failure conditions
- [ ] Full system integration testing
- [ ] Target: Sharding CRITICAL gaps < 50

### Team D: Sharding Implementation

#### 6.1 Write-Ahead Logging (Days 29-32)

**File:** `src/sharding/wal_manager.hpp`
```cpp
namespace themis::sharding {

class WalManager {
public:
  // Log write operation before applying
  Status logWrite(ShardKey key, const Value& val) {
    WalEntry entry{
      .timestamp = Clock::now(),
      .operation = Operation::WRITE,
      .shard_key = key,
      .value = val
    };
    
    // Persist to disk FIRST
    TRY(wal_file_.append(entry.serialize()));
    wal_file_.flush();  // Ensure durability
    
    // THEN apply to in-memory state
    return applyToMemory(key, val);
  }
  
  // Recover from WAL on startup
  Status recover() {
    for (const auto& entry : wal_file_.readAll()) {
      TRY(applyToMemory(entry.shard_key, entry.value));
    }
    return Status::Ok();
  }
};

}
```

---

#### 6.2 Failover Validation (Days 32-34)

**File:** `src/sharding/failover_validator.hpp`
```cpp
namespace themis::sharding {

class FailoverValidator {
  // Validate replica is safe to promote
  Status canPromoteToLeader(const Replica& replica) {
    // Check 1: Replica is fully synced
    if (replica.log_offset != leader_log_offset) {
      return Status::InvalidArgument("Replica not fully synced");
    }
    
    // Check 2: No split-brain (unique epoch)
    if (hasAnotherLeader()) {
      return Status::InvalidArgument("Split-brain condition detected");
    }
    
    // Check 3: Quorum acknowledges
    if (getAcknowledgeCount() < quorum_size) {
      return Status::Unavailable("Cannot form quorum");
    }
    
    return Status::Ok();
  }
};

}
```

---

#### 6.3 Rebalancing Algorithm (Days 34-36)

**File:** `src/sharding/rebalancer.hpp`
```cpp
namespace themis::sharding {

class ShardRebalancer {
  // Safe rebalancing: one shard at a time
  Status rebalanceOneShard(ShardId shard_id, ServerId target_server) {
    // Step 1: Create replica on target
    TRY(createReplica(shard_id, target_server));
    
    // Step 2: Wait for replica to catch up
    TRY(waitForReplicaSync(shard_id, target_server, timeout));
    
    // Step 3: If replica is leader, wait for safe leader election
    // Step 4: Remove old replica
    TRY(removeReplica(shard_id, old_server));
    
    return Status::Ok();
  }
};

}
```

---

### Team E: Full System Testing (Days 22-36)

**Test scenarios:**

```cpp
TEST_F(ShardingFailoverTest, FailoverPreservesData) {
  // Setup: 3 replicas, shard X
  // Kill: Leader replica
  // Expect: New leader elected, data intact, no loss
  EXPECT_OK(verifyDataIntegrity(shard_x));
}

TEST_F(ShardingRebalancingTest, RebalancingDoesNotLoseShard) {
  // Setup: Rebalance shard Y from Server A → Server B
  // Expect: No interruption to reads/writes, data valid
  EXPECT_OK(checkShardAvailability(shard_y));
}

TEST_F(ShardingConsistencyTest, NoSplitBrainUnderPartition) {
  // Simulate network partition: 2 replicas isolated
  // Expect: Old leader stops accepting writes, new leader (if quorum) elected
  EXPECT_FALSE(hasTwoLeaders());
}
```

---

## Success Criteria & Metrics

### End-of-Week Gap Reduction

| Module | Week 1 | Week 2 | Week 3 | Week 4-5 | Week 6 | Target |
|--------|--------|--------|--------|----------|--------|--------|
| **security** | 600 → 400 | 400 → 350 | — | — | — | **<100** ✅ |
| **server** | — | 2,722 → 1,800 | 1,800 → 1,000 | — | — | **<400** ✅ |
| **llm** | — | — | — | 2,255 → 1,200 | 1,200 → 300 | **<300** ✅ |
| **sharding** | — | — | — | — | 1,336 → 200 | **<200** ✅ |
| **TOTAL** | 600 ↓ | 923 ↓ | 800 ↓ | 1,055 ↓ | 1,136 ↓ | **~1,000** ✅ |

### Quality Metrics
- **Test coverage:** Target 85%+ for hardened modules
- **Regression tests:** 100 new test cases (min 15 per module)
- **Code review:** 0 CRITICAL issues slip through
- **Memory safety:** 0 AddressSanitizer warnings
- **Performance:** <5% latency regression on timeout-protected paths

---

## Deliverables per Week

### Week 1 Deliverables
- [ ] `SECRETS_AUDIT_WEEK1.md` — Complete secret inventory
- [ ] `SECRET_ROTATION_PLAN.md` — Rotation procedure
- [ ] `src/security/input_validator.hpp/cpp` — Validation framework
- [ ] 20 security gaps fixed (CRITICAL)
- [ ] `tests/security/test_input_validation.cpp` — Validation tests

### Week 2 Deliverables
- [ ] `include/server/request_handler_base.hpp` — Timeout framework
- [ ] All 50+ HTTP handlers updated with timeouts
- [ ] `src/server/health_check_handler.cpp` — Health endpoints
- [ ] 500+ server reliability gaps reduced
- [ ] `tests/server/test_timeout_handling.cpp` — Timeout tests

### Week 3 Deliverables
- [ ] Server module fully hardened
- [ ] 100% code review complete
- [ ] Stress tests passing
- [ ] LLM analysis document finalized

### Week 4-5 Deliverables
- [ ] `src/llm/exception_handlers.hpp` — Exception safety patterns
- [ ] All manual allocations → smart pointers
- [ ] 1,000+ LLM reliability gaps reduced
- [ ] AddressSanitizer: 0 leaks
- [ ] `tests/llm/test_exception_safety.cpp` — Exception tests

### Week 6 Deliverables
- [ ] `src/sharding/wal_manager.hpp` — Durability
- [ ] `src/sharding/failover_validator.hpp` — Failover logic
- [ ] `src/sharding/rebalancer.hpp` — Rebalancing
- [ ] Failover tests: all passing
- [ ] `tests/sharding/test_failover.cpp` — Failover tests

---

## Risks & Mitigation

| Risk | Impact | Likelihood | Mitigation |
|------|--------|------------|-----------|
| Secret rotation causes deployment issues | HIGH | MEDIUM | Test in staging first; rollback plan |
| Timeout patterns break legitimate long operations | MEDIUM | LOW | Tunable timeouts per operation type |
| Smart pointer conversion introduces new bugs | MEDIUM | MEDIUM | AddressSanitizer + thorough testing |
| Sharding failover causes brief outage | HIGH | LOW | Canary test on single shard first |
| LLM exception safety incomplete by week 5 | MEDIUM | MEDIUM | Prioritize, defer nice-to-haves to phase 3 |

---

## Sign-Off Criteria

**Project is ready for production if all of:**
- ✅ All CRITICAL security gaps (hardcoded secrets) fixed
- ✅ Server module has <100 CRITICAL gaps, all handlers have timeouts
- ✅ LLM module has <50 CRITICAL gaps, zero memory leaks (AddressSanitizer)
- ✅ Sharding module has <50 CRITICAL gaps, failover tests passing
- ✅ 85%+ test coverage across all modules
- ✅ 0 regressions in latency/throughput benchmarks
- ✅ Security audit: signed off
- ✅ QA: full integration testing passed

---

## Appendix: Effort Estimates (Days by Team)

| Team | Week 1 | Week 2 | Week 3 | Week 4 | Week 5 | Week 6 | Total |
|------|--------|--------|--------|--------|--------|--------|-------|
| A (Security) | 5 days | 2 days | 0 | 0 | 0 | 0.5 | **7.5 days** |
| B (Server) | 0 | 5 days | 2 days | 0 | 0 | 0.5 | **7.5 days** |
| C (LLM) | 0 | 0 | 1 day | 5 days | 3 days | 0.5 | **9.5 days** |
| D (Sharding) | 0 | 0 | 1 day | 0 | 1 day | 4 days | **6 days** |
| E (QA) | 5 days | 5 days | 3 days | 4 days | 4 days | 4 days | **25 days** |
| **TOTAL** | **10** | **12** | **7** | **9** | **8** | **9** | **55 days** |

**Team capacity:** 55 person-days ÷ 10 people = **5.5 weeks of elapsed time** ✅

---

## Next Steps

1. **Tomorrow (May 19):** Kickoff meeting with all teams
2. **May 20:** Team A begins security audit (Day 1 of Week 1)
3. **May 27:** Team B begins server hardening (Day 1 of Week 2)
4. **June 3:** Team C begins LLM exception safety (Day 1 of Week 4)
5. **June 10:** Team D begins sharding hardening (Day 1 of Week 5)
6. **July 1:** Full hardening complete, systems ready for QA integration testing

**Status tracking:** Daily standup (15 min), weekly reviews, gap progress dashboard

---

*Generated by Phase 2 Planning — Evidence-based, realistic effort estimates, clear ownership*
