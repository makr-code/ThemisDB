# RAG Security Guardrail Pack

**Status:** Design & Implementation  
**Created:** 2026-09-24  
**Version:** 1.0.0 (Specification)  
**Target Completion:** Q4 2026 – Q1 2027  
**Scope:** Tenant isolation, deny-by-default retrieval policies, and audit trails  

---

## Purpose

This document defines the security boundary enforcement for multi-tenant RAG systems. The guardrail pack ensures:

1. **Tenant Isolation:** Only authorized tenants can retrieve documents from their assigned index ranges
2. **Deny-by-Default:** All retrieval operations default to DENIED unless explicit policy allows
3. **Audit Trail:** All retrieval access is logged with tenant context, query content, and decision rationale
4. **Policy Enforcement:** Malformed or missing policy contexts result in hard failures, not silent fallback

---

## Architectural Design

### Tenant-Isolated Retrieval Data Flow

```
┌──────────────────────────────────────────────────────────────────┐
│ User Query (Tenant A, API Key abc123)                           │
└──────────────────┬───────────────────────────────────────────────┘
                   │
                   ▼
┌──────────────────────────────────────────────────────────────────┐
│ Policy Context Gate (src/security/policy_context_gate.cpp)      │
│ ├─ Extract tenant_id from API credentials                       │
│ ├─ Fetch tenant policy from AuthzStore                          │
│ ├─ Validate policy is present and non-null                      │
│ └─ Forward (policy_context, tenant_id) or DENY                  │
└──────────────────┬───────────────────────────────────────────────┘
                   │
       ┌───────────┴───────────┐
       │ Policy Match?         │
       ▼                       ▼
    ✓ ALLOW              ✗ POLICY_MISMATCH
       │                       │
       ▼                       ▼
┌──────────────────────────────────────────────────────────────────┐
│ Retrieval Policy Enforcer (src/rag/retrieval_policy_enforcer.cpp)│
│ ├─ Filter index ranges by tenant_policy.allowed_ranges          │
│ ├─ Apply masking rules (column-level privacy)                   │
│ ├─ Log access (OTLP event: retrieval.access)                    │
│ └─ Return masked results or DENY                                │
└──────────────────┬───────────────────────────────────────────────┘
                   │
       ┌───────────┴───────────┐
       │ Range Check Pass?     │
       ▼                       ▼
    ✓ RETRIEVE          ✗ RANGE_DENIED
       │                       │
       ▼                       ▼
┌──────────────────────────────────────────────────────────────────┐
│ Hybrid Retriever (src/rag/hybrid_retriever.cpp)                 │
│ ├─ Query BM25 + HNSW over allowed ranges only                   │
│ ├─ Apply result masking (PII removal if configured)             │
│ └─ Log result count and latency                                 │
└──────────────────┬───────────────────────────────────────────────┘
                   │
                   ▼
┌──────────────────────────────────────────────────────────────────┐
│ Audit Trail (src/observability/opentelemetry_tracer.cpp)        │
│ ├─ Emit OTLP trace: retrieval.access                            │
│ │  ├─ tenant_id, policy_id, query_hash, result_count            │
│ │  ├─ allowed_ranges, applied_masks, decision                   │
│ │  └─ timestamp, latency_ms, cost_usd                           │
│ └─ Send to observability backend (e.g., Jaeger)                 │
└──────────────────┬───────────────────────────────────────────────┘
                   │
                   ▼
           Return Results to User
```

---

## Policy Context Definition

### AuthzPolicy Schema

```cpp
// In src/security/authz_policy.hpp

struct TenantRetrievalPolicy {
  // Identity
  std::string policy_id;              // "policy-tenant-a-v1"
  std::string tenant_id;              // "tenant-a"
  
  // Authorization scope
  std::vector<std::string> allowed_index_ids;  // ["wiki", "code"]
  std::vector<IndexRange> allowed_ranges;      // Document ranges in index
  std::vector<std::string> allowed_document_types;  // ["article", "documentation"]
  
  // Data masking & privacy
  struct MaskingRule {
    std::string field_name;           // "email", "phone", "credit_card"
    std::string masking_type;         // "HASH", "PII_REDACT", "TRUNCATE"
    std::string replacement_text;     // "[REDACTED]", "***-****"
  };
  std::vector<MaskingRule> masking_rules;
  
  // Rate limiting & SLA
  int max_queries_per_minute;         // 100
  int max_results_per_query;          // 50
  int max_total_tokens_per_query;     // 10000
  
  // Versioning & audit
  std::string policy_version;         // "1.0.0"
  std::chrono::system_clock::time_point valid_from;
  std::chrono::system_clock::time_point valid_until;
  std::string approved_by;            // "security-team@themis.local"
  std::string approval_id;            // "PR#12345"
};

// Policy validation
bool isValidPolicy(const TenantRetrievalPolicy& policy) {
  if (policy.tenant_id.empty()) return false;
  if (policy.allowed_ranges.empty()) return false;
  if (policy.masking_rules.empty() && policy.allowed_document_types.size() > 1)
    return false;  // Multi-type access requires explicit masking rules
  return true;
}
```

### Policy Storage (AuthzStore)

```cpp
// src/security/authz_store.hpp

class AuthzStore {
 public:
  // Load tenant policy by tenant_id
  std::optional<TenantRetrievalPolicy> getTenantPolicy(
      const std::string& tenant_id) const;
  
  // Validate policy is current and non-expired
  bool isPolicyValid(const TenantRetrievalPolicy& policy) const;
  
  // Audit access
  void logAccessDecision(
      const std::string& tenant_id,
      const std::string& decision,       // "ALLOW", "DENY", "POLICY_MISMATCH"
      const std::string& reason) const;
};
```

---

## Component 4.1: Tenant-Isolated Retrieval Enforcement

### Spec: Retrieval Policy Enforcer

```cpp
// src/rag/retrieval_policy_enforcer.cpp

class RetrievalPolicyEnforcer {
 public:
  struct EnforceResult {
    bool allowed;
    std::string reason;           // "ALLOWED", "DENIED", "POLICY_MISMATCH"
    std::vector<IndexRange> allowed_ranges;
    std::vector<MaskingRule> masking_rules;
  };
  
  // Main enforcement entry point
  EnforceResult enforceRetrievalPolicy(
      const QueryRequest& query,
      const TenantRetrievalPolicy& policy,
      const std::string& tenant_id) const;
  
  // Apply masking rules to retrieved documents
  std::vector<Document> applyMasking(
      const std::vector<Document>& documents,
      const std::vector<MaskingRule>& rules) const;
  
 private:
  // Range validation
  bool validateIndexRanges(
      const std::vector<IndexRange>& requested_ranges,
      const std::vector<IndexRange>& allowed_ranges) const;
};

// Implementation logic:
RetrievalPolicyEnforcer::EnforceResult 
RetrievalPolicyEnforcer::enforceRetrievalPolicy(
    const QueryRequest& query,
    const TenantRetrievalPolicy& policy,
    const std::string& tenant_id) const {
  
  // 1. Validate policy is current
  if (!authz_store_->isPolicyValid(policy)) {
    return {
      .allowed = false,
      .reason = "POLICY_EXPIRED_OR_INVALID",
      .allowed_ranges = {},
      .masking_rules = {}
    };
  }
  
  // 2. Extract requested ranges from query
  auto requested_ranges = extractIndexRangesFromQuery(query);
  
  // 3. Intersect with allowed ranges
  if (!validateIndexRanges(requested_ranges, policy.allowed_ranges)) {
    logAccessDecision(tenant_id, "DENY", "index_range_violation");
    return {
      .allowed = false,
      .reason = "INDEX_RANGE_DENIED",
      .allowed_ranges = policy.allowed_ranges,
      .masking_rules = policy.masking_rules
    };
  }
  
  // 4. Emit OTLP event
  emitAccessDecisionEvent(
      tenant_id, policy.policy_id, "ALLOW",
      policy.allowed_ranges);
  
  // 5. Return enforcement decision
  return {
    .allowed = true,
    .reason = "POLICY_MATCHED",
    .allowed_ranges = policy.allowed_ranges,
    .masking_rules = policy.masking_rules
  };
}

std::vector<Document> RetrievalPolicyEnforcer::applyMasking(
    const std::vector<Document>& documents,
    const std::vector<MaskingRule>& rules) const {
  std::vector<Document> result = documents;
  for (auto& doc : result) {
    for (const auto& rule : rules) {
      if (doc.fields.count(rule.field_name)) {
        std::string& field_value = doc.fields[rule.field_name];
        field_value = applyMaskingRule(field_value, rule);
      }
    }
  }
  return result;
}
```

### Test Suite: Retrieval Tenant Isolation

```cpp
// tests/security/test_retrieval_tenant_isolation.cpp

TEST(RetrievalTenantIsolation, TenantA_CannotAccessTenantB_Index) {
  RetrievalPolicyEnforcer enforcer;
  
  TenantRetrievalPolicy tenant_a_policy = {
    .tenant_id = "tenant-a",
    .allowed_ranges = {IndexRange(0, 1000)},
    .allowed_index_ids = {"wiki"}
  };
  
  QueryRequest query = {
    .tenant_id = "tenant-a",
    .query = "test",
    .index_id = "wiki"
  };
  
  auto result = enforcer.enforceRetrievalPolicy(
      query, tenant_a_policy, "tenant-a");
  
  EXPECT_TRUE(result.allowed);
}

TEST(RetrievalTenantIsolation, TenantA_Cannot_QueryTenantB_IndexRange) {
  RetrievalPolicyEnforcer enforcer;
  
  TenantRetrievalPolicy tenant_a_policy = {
    .tenant_id = "tenant-a",
    .allowed_ranges = {IndexRange(0, 1000)},
    .allowed_index_ids = {"wiki"}
  };
  
  QueryRequest query = {
    .tenant_id = "tenant-a",
    .query = "test",
    .index_id = "wiki",
    .forced_range = IndexRange(5000, 6000)  // Outside allowed range
  };
  
  auto result = enforcer.enforceRetrievalPolicy(
      query, tenant_a_policy, "tenant-a");
  
  EXPECT_FALSE(result.allowed);
  EXPECT_EQ(result.reason, "INDEX_RANGE_DENIED");
}

TEST(RetrievalTenantIsolation, MaskingRules_Applied_Correctly) {
  RetrievalPolicyEnforcer enforcer;
  
  std::vector<MaskingRule> rules = {
    {.field_name = "email", .masking_type = "PII_REDACT", .replacement_text = "[REDACTED]"},
    {.field_name = "phone", .masking_type = "TRUNCATE", .replacement_text = "***-****"}
  };
  
  TenantRetrievalPolicy policy = {
    .tenant_id = "tenant-a",
    .allowed_ranges = {IndexRange(0, 1000)},
    .masking_rules = rules
  };
  
  std::vector<Document> docs = {
    Document{.fields = {{"email", "user@example.com"}, {"phone", "555-1234"}}}
  };
  
  docs = enforcer.applyMasking(docs, rules);
  
  EXPECT_EQ(docs[0].fields["email"], "[REDACTED]");
  EXPECT_TRUE(docs[0].fields["phone"].find("***") != std::string::npos);
}

TEST(RetrievalTenantIsolation, ExpiredPolicy_DeniesAccess) {
  RetrievalPolicyEnforcer enforcer;
  
  auto now = std::chrono::system_clock::now();
  TenantRetrievalPolicy expired_policy = {
    .tenant_id = "tenant-a",
    .allowed_ranges = {IndexRange(0, 1000)},
    .valid_until = now - std::chrono::seconds(3600)  // Expired 1 hour ago
  };
  
  QueryRequest query = {.tenant_id = "tenant-a"};
  
  auto result = enforcer.enforceRetrievalPolicy(
      query, expired_policy, "tenant-a");
  
  EXPECT_FALSE(result.allowed);
  EXPECT_EQ(result.reason, "POLICY_EXPIRED_OR_INVALID");
}

TEST(RetrievalTenantIsolation, 
     NoPolicy_DeniesAccess_DenyByDefault) {
  RetrievalPolicyEnforcer enforcer;
  
  TenantRetrievalPolicy null_policy = {};  // Empty/invalid policy
  QueryRequest query = {.tenant_id = "tenant-a"};
  
  auto result = enforcer.enforceRetrievalPolicy(
      query, null_policy, "tenant-a");
  
  EXPECT_FALSE(result.allowed);
  EXPECT_EQ(result.reason, "POLICY_EXPIRED_OR_INVALID");
}
```

---

## Component 4.2: Deny-by-Default & Policy Context Gate

### Spec: Policy Context Gate

```cpp
// src/security/policy_context_gate.cpp

class PolicyContextGate {
 public:
  enum class GateDecision {
    ALLOW,              // Policy matched, proceed to retrieval
    DENY,               // Policy mismatch or missing
    POLICY_ERROR,       // Unrecoverable error (log and escalate)
  };
  
  struct GateResult {
    GateDecision decision;
    std::string reason;
    TenantRetrievalPolicy policy;  // Only set if decision == ALLOW
    std::string trace_id;          // For audit trail
  };
  
  // Main gate entry point (deny-by-default)
  GateResult checkPolicyContext(
      const QueryRequest& query,
      const AuthzStore& authz_store) const;
};

// Implementation: deny-by-default logic
PolicyContextGate::GateResult 
PolicyContextGate::checkPolicyContext(
    const QueryRequest& query,
    const AuthzStore& authz_store) const {
  
  // 1. Extract tenant_id from credentials
  std::optional<std::string> tenant_id = 
      extractTenantIdFromApiKey(query.api_key);
  
  if (!tenant_id.has_value()) {
    logAccessDecision("UNKNOWN", "DENY", "INVALID_CREDENTIALS");
    return {
      .decision = GateDecision::DENY,
      .reason = "INVALID_CREDENTIALS",
      .trace_id = generateTraceId()
    };
  }
  
  // 2. Fetch policy from AuthzStore (deny if missing)
  auto policy = authz_store.getTenantPolicy(tenant_id.value());
  
  if (!policy.has_value()) {
    logAccessDecision(tenant_id.value(), "DENY", "NO_POLICY_FOUND");
    return {
      .decision = GateDecision::DENY,
      .reason = "NO_POLICY_FOUND",
      .trace_id = generateTraceId()
    };
  }
  
  // 3. Validate policy is current and non-expired
  if (!authz_store.isPolicyValid(policy.value())) {
    logAccessDecision(tenant_id.value(), "DENY", "POLICY_EXPIRED");
    return {
      .decision = GateDecision::DENY,
      .reason = "POLICY_EXPIRED",
      .trace_id = generateTraceId()
    };
  }
  
  // 4. Validate policy structure
  if (!isValidPolicy(policy.value())) {
    logAccessDecision(tenant_id.value(), "POLICY_ERROR", "MALFORMED_POLICY");
    return {
      .decision = GateDecision::POLICY_ERROR,
      .reason = "MALFORMED_POLICY",
      .trace_id = generateTraceId()
    };
  }
  
  // 5. All checks passed → ALLOW with policy context
  logAccessDecision(tenant_id.value(), "ALLOW", "POLICY_MATCHED");
  return {
    .decision = GateDecision::ALLOW,
    .reason = "POLICY_MATCHED",
    .policy = policy.value(),
    .trace_id = generateTraceId()
  };
}
```

### Spec: Null Retrieval Backend (Fallback)

```cpp
// src/security/null_retrieval_backend.cpp

class NullRetrievalBackend {
 public:
  std::vector<RetrievedDoc> query(
      const QueryRequest& query) const override {
    // Deterministic rejection only
    throw SecurityPolicyViolationException(
        "Retrieval denied by security policy",
        query.tenant_id);
  }
};

// Usage in policy-denied case:
RetrievalBackend* selectRetrievalBackend(
    const PolicyContextGate::GateResult& gate_result) {
  
  if (gate_result.decision == PolicyContextGate::GateDecision::ALLOW) {
    return hybrid_retriever_.get();  // Normal hybrid retrieval
  } else if (gate_result.decision == PolicyContextGate::GateDecision::DENY) {
    return null_backend_.get();      // Deny-by-default backend
  } else {
    // POLICY_ERROR: escalate to security team
    alertSecurityTeam("Policy context gate error", gate_result);
    return null_backend_.get();
  }
}
```

### Test Suite: Deny-by-Default Gate

```cpp
// tests/security/test_policy_context_gate.cpp

TEST(PolicyContextGate, ValidCredentials_MatchingPolicy_ALLOW) {
  PolicyContextGate gate;
  AuthzStore authz_store;
  
  QueryRequest query = {.api_key = "valid_key_tenant_a"};
  auto result = gate.checkPolicyContext(query, authz_store);
  
  EXPECT_EQ(result.decision, PolicyContextGate::GateDecision::ALLOW);
  EXPECT_EQ(result.reason, "POLICY_MATCHED");
}

TEST(PolicyContextGate, InvalidCredentials_DENY) {
  PolicyContextGate gate;
  AuthzStore authz_store;
  
  QueryRequest query = {.api_key = "invalid_key"};
  auto result = gate.checkPolicyContext(query, authz_store);
  
  EXPECT_EQ(result.decision, PolicyContextGate::GateDecision::DENY);
  EXPECT_EQ(result.reason, "INVALID_CREDENTIALS");
}

TEST(PolicyContextGate, NoPolicy_DenyByDefault) {
  PolicyContextGate gate;
  AuthzStore authz_store;  // Empty AuthzStore
  
  QueryRequest query = {.api_key = "key_for_unknown_tenant"};
  auto result = gate.checkPolicyContext(query, authz_store);
  
  EXPECT_EQ(result.decision, PolicyContextGate::GateDecision::DENY);
  EXPECT_EQ(result.reason, "NO_POLICY_FOUND");
}

TEST(PolicyContextGate, MissingPolicy_NOT_Silent_Fallback) {
  PolicyContextGate gate;
  
  // This test verifies that missing policy is NOT silently ignored
  // Instead, it raises an explicit DENY decision
  
  auto result = gate.checkPolicyContext(query, authz_store);
  EXPECT_NE(result.decision, PolicyContextGate::GateDecision::ALLOW);
}

TEST(PolicyContextGate, ExpiredPolicy_DENY) {
  PolicyContextGate gate;
  
  auto now = std::chrono::system_clock::now();
  authz_store.addPolicy({
    .policy_id = "expired-policy",
    .tenant_id = "tenant-a",
    .valid_until = now - std::chrono::seconds(3600)
  });
  
  QueryRequest query = {.api_key = "key_tenant_a"};
  auto result = gate.checkPolicyContext(query, authz_store);
  
  EXPECT_EQ(result.decision, PolicyContextGate::GateDecision::DENY);
  EXPECT_EQ(result.reason, "POLICY_EXPIRED");
}

TEST(PolicyContextGate, MalformedPolicy_POLICY_ERROR) {
  PolicyContextGate gate;
  
  // Add policy with missing required fields
  authz_store.addPolicy({
    .policy_id = "malformed",
    .tenant_id = "tenant-a"
    // .allowed_ranges is empty (required field missing)
  });
  
  QueryRequest query = {.api_key = "key_tenant_a"};
  auto result = gate.checkPolicyContext(query, authz_store);
  
  EXPECT_EQ(result.decision, PolicyContextGate::GateDecision::POLICY_ERROR);
  EXPECT_EQ(result.reason, "MALFORMED_POLICY");
}

TEST(SecurityPolicy, NullBackendThrowsException) {
  NullRetrievalBackend null_backend;
  QueryRequest query = {.tenant_id = "tenant-a"};
  
  EXPECT_THROW(
      null_backend.query(query),
      SecurityPolicyViolationException);
}
```

---

## OTLP Observability Integration

### Retrieval Access Event Schema

```cpp
// In src/observability/opentelemetry_tracer.cpp

void emitRetrievalAccessEvent(
    const std::string& tenant_id,
    const std::string& policy_id,
    const std::string& decision,  // "ALLOW", "DENY"
    const std::vector<IndexRange>& ranges,
    int result_count,
    int64_t latency_ms,
    double cost_usd) {
  
  auto span = tracer_->StartSpan("retrieval.access");
  
  // Tenant context
  span->SetAttribute("tenant.id", tenant_id);
  span->SetAttribute("policy.id", policy_id);
  
  // Access decision
  span->SetAttribute("access.decision", decision);
  span->SetAttribute("access.reason", reason);
  
  // Retrieval details
  span->SetAttribute("retrieval.allowed_ranges", 
      formatRanges(ranges));
  span->SetAttribute("retrieval.result_count", result_count);
  span->SetAttribute("retrieval.latency_ms", latency_ms);
  
  // Cost tracking
  span->SetAttribute("cost.usd", cost_usd);
  
  // Timestamp
  span->SetAttribute("timestamp", getCurrentTimestamp());
  
  span->End();
}

// OTLP Payload Example (JSON):
{
  "resourceSpans": [
    {
      "scopeSpans": [
        {
          "spans": [
            {
              "traceId": "0af7651916cd43dd",
              "spanId": "b7ad6b7169203331",
              "name": "retrieval.access",
              "attributes": {
                "tenant.id": "tenant-a",
                "policy.id": "policy-tenant-a-v1",
                "access.decision": "ALLOW",
                "access.reason": "POLICY_MATCHED",
                "retrieval.allowed_ranges": "[0:1000]",
                "retrieval.result_count": 25,
                "retrieval.latency_ms": 145,
                "cost.usd": 0.045,
                "timestamp": "2026-09-24T06:30:15.123Z"
              }
            }
          ]
        }
      ]
    }
  ]
}
```

---

## CI/Gate Integration

### Release Critical Test Suite

```cmake
# In tests/security/CMakeLists.txt

add_executable(test_retrieval_tenant_isolation
  test_retrieval_tenant_isolation.cpp)
target_link_libraries(test_retrieval_tenant_isolation
  gtest themis_security themis_rag)

add_test(NAME rag_tenant_isolation_gate
         COMMAND test_retrieval_tenant_isolation)

set_tests_properties(rag_tenant_isolation_gate 
  PROPERTIES
  LABELS "release_critical;security"
  TIMEOUT 60)

add_executable(test_policy_context_gate
  test_policy_context_gate.cpp)
target_link_libraries(test_policy_context_gate
  gtest themis_security)

add_test(NAME rag_policy_context_gate
         COMMAND test_policy_context_gate)

set_tests_properties(rag_policy_context_gate
  PROPERTIES
  LABELS "release_critical;security"
  TIMEOUT 60)
```

### GitHub Actions Gate

```yaml
# In .github/workflows/gate-pr-rag-security.yml (new)

name: RAG Security Guardrails Gate

on:
  pull_request:
    paths:
      - 'src/rag/**'
      - 'src/security/**'
      - 'src/llm/**'
      - 'tests/security/**'
      - 'tests/rag/**'

jobs:
  security_gate:
    runs-on: ubuntu-22.04
    steps:
      - uses: actions/checkout@v4
      
      - name: Build & Test Retrieval Tenant Isolation
        run: |
          cmake -B build -DTHEMIS_BUILD_TESTS=ON -DTHEMIS_ALLOW_MISSING_ROCKSDB=ON
          ctest -L "security" -V
      
      - name: Verify Deny-by-Default Semantics
        run: |
          ./tests/security/verify_deny_by_default.sh
      
      - name: OTLP Audit Trail Validation
        run: |
          python3 ./scripts/validate_otlp_audit_trail.py \
            --test-suite rag_security \
            --require-fields tenant_id,policy_id,access.decision
      
      - name: Report
        if: failure()
        run: |
          echo "## ❌ RAG Security Guardrails Test Failed" >> $GITHUB_STEP_SUMMARY
          echo "See test output above for details" >> $GITHUB_STEP_SUMMARY
```

---

## Governance & Rollout

### Q4 2026 (Immediate)

- [ ] Implement `src/security/policy_context_gate.cpp` (deny-by-default logic)
- [ ] Implement `src/rag/retrieval_policy_enforcer.cpp` (tenant range isolation)
- [ ] Implement `src/security/null_retrieval_backend.cpp` (fallback)
- [ ] Create test suite with `release_critical` labels
- [ ] Integrate with `.github/workflows/gate-pr-rag-security.yml`

### Q1 2027 (Production Deployment)

- [ ] Deploy policy context gate with audit logging
- [ ] Roll out tenant policies via AuthzStore
- [ ] Monitor OTLP audit events for access patterns
- [ ] Create security dashboard for policy compliance
- [ ] Update operator runbooks with incident response procedures

---

## References

- `audit/RAG_READINESS_AUDIT_2026-09-23.md` § 4 (Security Guardrails)
- `src/security/QueryMaskingPolicy` (existing masking infrastructure)
- `src/observability/opentelemetry_tracer.cpp` (OTLP instrumentation)
- `src/rag/hybrid_retriever.cpp` (retrieval entry point)
