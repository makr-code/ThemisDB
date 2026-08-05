# AQL LLM Integration: Migration Guide for Users

**Version:** 1.0  
**Date:** 2026-08-05  
**Phase:** 3 (Documentation Consolidation)  
**Status:** ✅ COMPLETE  
**Parent Issue:** makr-code/ThemisDB#5664  
**Audience:** AQL application developers, data engineers

---

## Overview

This guide helps you adopt LLM-enhanced AQL queries in your ThemisDB applications. Whether you're new to LLM-assisted queries or migrating from existing applications, this guide walks you through the process step-by-step.

**Key Message**: Existing AQL queries continue to work unchanged. LLM features are opt-in enhancements you can adopt at your own pace.

---

## Table of Contents

1. [Section 1: What's New for Existing AQL Users](#section-1-whats-new-for-existing-aql-users)
2. [Section 2: Getting Started](#section-2-getting-started)
3. [Section 3: Best Practices](#section-3-best-practices)
4. [Section 4: Troubleshooting](#section-4-troubleshooting)
5. [Section 5: Advanced Scenarios](#section-5-advanced-scenarios)
6. [Section 6: Metrics Monitoring](#section-6-metrics-monitoring)

---

## Section 1: What's New for Existing AQL Users

### No Breaking Changes ✅

If you're using AQL today:
- **Your queries continue to work unchanged** ✅
- **No migrations required** ✅
- **No new dependencies** ✅
- **Performance is unaffected** ✅

### What You Gain

The AQL LLM Integration adds a new capability:

**Before** (Query as Code):
```
You → Write AQL manually → Parse → Execute
```

**After** (Natural Language + LLM):
```
You → Describe query in English → LLM → Validated AQL → Parse → Execute
```

**Benefits**:
- 📝 Write queries from natural language descriptions
- ⚡ Faster query generation for complex queries
- 🛡️ Automatic validation and safety checks
- 📊 Built-in quality monitoring
- 🔄 Retries with corrective feedback (LLM learns from failures)

### Backward Compatibility Guarantee

| Feature | Before (v2.0) | After (v2.1) | Status |
|---------|---|---|---|
| Standard AQL parsing | ✅ Works | ✅ Works unchanged | ✅ No breaking changes |
| QueryEngine.execute() | ✅ Works | ✅ Works unchanged | ✅ No breaking changes |
| Error codes | ✅ Defined | ✅ Unchanged | ✅ No breaking changes |
| New APIs | ❌ N/A | ✅ Available | ✅ Opt-in |
| LLM features | ❌ N/A | ✅ Optional | ✅ Opt-in |

---

## Section 2: Getting Started

### Step 1: Verify Your Version

Check that you're running Query Module v2.1 or later:

```bash
# Query module version
curl http://localhost:8080/api/version

# Expected response
{
  "query_module": {
    "version": "2.1.0",
    "llm_integration": "enabled"
  }
}
```

If your version is < 2.1.0, contact your administrator to upgrade.

### Step 2: Enable LLM Integration (Configuration)

**Option A: Environment Variable** (simplest):
```bash
export THEMIS_LLM_ENABLED=true
export THEMIS_LLM_MODEL=openai:gpt-4
export THEMIS_LLM_PROMPT_VERSION=gpt-4-turbo-v1.0

# Restart your application
systemctl restart my-themis-app
```

**Option B: Configuration File**:
```yaml
# config/themis.yaml
llm_integration:
  enabled: true
  model_id: "openai:gpt-4"
  prompt_version: "gpt-4-turbo-v1.0"
  strict_mode: true
  feature_flags:
    enforce_boundary_checks: true
    enforce_access_checks: true
    enforce_semantic_checks: true
```

**Option C: Programmatic** (SDK):
```python
from themis import QueryEngine, LLMContext

engine = QueryEngine()
engine.set_llm_context(LLMContext(
    model_id="openai:gpt-4",
    prompt_version="gpt-4-turbo-v1.0",
    strict_mode=True,
))
```

### Step 3: Test Your First LLM Query

**Test Case 1: Simple SELECT**

Write natural language:
```
"Find all users over age 30"
```

The LLM translates this to AQL:
```aql
SELECT * FROM users FILTER age > 30
```

Your application code:
```python
from themis import QueryEngine

engine = QueryEngine()

# Natural language query
nl_query = "Find all users over age 30"

# Translate via LLM
llm_result = engine.translate_nl_to_aql(nl_query)

if llm_result.success:
    print(f"Generated AQL: {llm_result.aql_query}")
    
    # Execute the query
    result = engine.execute_aql(llm_result.aql_query)
    print(f"Results: {result.entities}")
else:
    print(f"LLM translation failed: {llm_result.error}")
```

**Expected Output**:
```
Generated AQL: SELECT * FROM users FILTER age > 30
Results: [User(id=1, name=Alice, age=35), User(id=2, name=Bob, age=42), ...]
```

### Step 4: Monitor Quality with Metrics

Access Prometheus to see real-time LLM quality metrics:

```bash
# SSH to your monitoring cluster
ssh monitoring.example.com

# Query Prometheus API
curl http://localhost:9090/api/v1/query?query='parser_phase1_boundary_validation_ms_count{source="llm"}'

# Expected response (LLM queries processed)
{
  "status": "success",
  "data": {
    "resultType": "instant",
    "result": [
      {
        "metric": {"source": "llm"},
        "value": [1722873600, "2345"]  # 2,345 LLM queries processed
      }
    ]
  }
}
```

Or view pre-built dashboard:
```
Grafana → Dashboards → AQL LLM Integration — Phase 3 Monitoring
```

---

## Section 3: Best Practices

### Practice 1: Start with Simple Queries ✅

**Good**: Simple, single-operation queries
```
"Get users from New York"
"Find orders placed in the last 30 days"
"Count documents per collection"
```

**Less Ideal**: Complex, multi-operation queries (at first)
```
"Get the average order value by customer, including their top 5 products, 
 excluding orders from test accounts, and ranking by recency"
```

**Why**: LLM models are better at simple patterns. Start simple, then gradually use LLM for more complex queries.

**Progression**:
1. **Week 1**: Simple SELECT queries
2. **Week 2**: SELECT with joins
3. **Week 3**: SELECT with aggregations
4. **Week 4+**: Complex multi-stage queries

---

### Practice 2: Monitor Error Rate (parser.errors.llm_validation_fail_count)

**What to Monitor**: Percentage of LLM queries that fail validation

```promql
# Prometheus query for LLM failure rate
failure_rate = rate(parser_errors_llm_validation_fail_count[5m]) 
               / 
               rate(parser_phase1_boundary_validation_ms_count{source="llm"}[5m])
```

**Healthy Range**: 0-5% failure rate  
**Warning Range**: 5-10% failure rate  
**Critical Range**: > 10% failure rate

**What to Do if Rate is High**:

1. **Identify error pattern**:
```promql
sum(rate(parser_errors_llm_validation_fail_count[5m])) by (error_type)
```

2. **Common Issues**:

| Error Type | Cause | Fix |
|---|---|---|
| `missing_filter` | LLM doesn't add WHERE clause | Update prompt: "Always add a filter to limit results" |
| `syntax_error` | LLM generates invalid AQL syntax | Simplify prompt; show examples |
| `semantic_error` | LLM uses wrong field names or types | Verify schema hints in LLM prompt |
| `boundary_violation` | LLM generates unsafe patterns | Review access control policies |

3. **Iterate on prompt** (for your team):
```
Original prompt:
  "Generate AQL queries to answer the user's question"

Improved prompt:
  "Generate AQL queries to answer the user's question.
   - Always include a FILTER clause to limit results to at most 1000 rows
   - Use FROM collection_name syntax
   - Field names are lowercase with underscores (e.g., user_id, created_at)"
```

---

### Practice 3: Fine-Tune LLM Prompt Based on Rejection Patterns

**Process** (monthly):

1. **Collect failures** (automated):
```python
# Collect all LLM query failures from Prometheus
failed_queries = prometheus.query(
    "rate(parser_errors_llm_validation_fail_count[1d]) by (error_type)"
)
```

2. **Analyze patterns**:
```python
# Group by error type; find most common
most_common_errors = sorted(failed_queries, 
                            key=lambda x: x['rate'], 
                            reverse=True)[:5]

# Example output:
# - missing_filter: 45 failures
# - syntax_error: 23 failures
# - semantic_error: 12 failures
```

3. **Update prompt**:
```
If most errors are "missing_filter":
  → Add to prompt: "Always include WHERE or FILTER to limit results"

If most errors are "syntax_error":
  → Add to prompt: "Use AQL v2.1 syntax: FROM collection FILTER condition"

If most errors are "semantic_error":
  → Add to prompt: "Available fields are: user_id, name, email, created_at"
```

4. **Validate improvement**:
```promql
# Compare failure rate before/after prompt change
rate(parser_errors_llm_validation_fail_count[1d]) 
  offset 30d (old prompt) 
vs 
rate(parser_errors_llm_validation_fail_count[1d]) (new prompt)
```

---

### Practice 4: Use Access Control Metrics to Audit Permissions

**Metric**: `parser.access.llm_denial_count`

**What it means**: LLM tried to query a collection user doesn't have access to

**Healthy**: 0-2% denial rate  
**Warning**: 2-5% denial rate  
**Critical**: > 5% denial rate

**Use Case 1: Detect Permission Gaps**
```promql
# Which collections are being denied most?
sum(rate(parser_access_llm_denial_count[1d])) by (collection_name)

# Result:
#  customer_pii: 45 denials
#  user_credit_cards: 23 denials
#  audit_log: 12 denials
```

**Action**: Update LLM prompt to restrict queries to allowed collections

**Use Case 2: Verify Access Policy is Working**
```promql
# If denial rate is 0%, verify:
# 1. LLM prompt reflects user's actual permissions
# 2. Access control is enabled in parser
# 3. Schema hints in LLM prompt are accurate
```

---

## Section 4: Troubleshooting

### Problem 1: "LLM validation rejected my query"

**Error Message**:
```
QueryLLMValidationError: Fragment validation failed
  Stage: boundary
  Error: FILTER clause is required for SELECT *
```

**Cause**: LLM generated a query that doesn't follow safety rules (e.g., missing FILTER clause)

**Solution**:

```python
# Step 1: Check error details
try:
    result = engine.execute_llm_query(nl_query)
except QueryLLMValidationError as e:
    print(f"Error stage: {e.validation_stage}")
    print(f"Details: {e.error_details}")

# Step 2: Update LLM prompt
# Add guidance to avoid the error
engine.set_llm_context(LLMContext(
    prompt_hint="Always add WHERE or FILTER clause to limit results"
))

# Step 3: Retry
result = engine.execute_llm_query(nl_query)
```

**Prevention**:
- Simplify your natural language queries (LLM handles simple better than complex)
- Provide explicit schema hints to LLM
- Gradually increase query complexity

---

### Problem 2: "LLM queries are slow"

**Symptom**: Latency is high for LLM-generated queries

**Debug Steps**:

1. **Check validation latency**:
```promql
histogram_quantile(0.95, parser_phase1_boundary_validation_ms)
```

2. **Identify slow fragments**:
```python
# Collect slow fragments (validation takes > 10ms)
slow_queries = prometheus.query(
    'parser_phase1_boundary_validation_ms{le="+Inf"} > 10'
)

for query in slow_queries:
    print(f"Slow fragment: {query['raw_text']}")
    print(f"  Latency: {query['latency_ms']}ms")
    print(f"  Complexity: {query['fragment_complexity']}")
```

3. **Simplify LLM prompt**:
```
If fragments are too complex (deep nesting, many joins):

Original prompt:
  "Generate AQL queries"

Improved prompt:
  "Generate simple, flat AQL queries.
   Avoid deep nesting (more than 3 levels).
   Prefer multiple simple queries over one complex query."
```

4. **Monitor improvement**:
```promql
# p95 latency trend
histogram_quantile(0.95, parser_phase1_boundary_validation_ms)[1h:5m]
```

---

### Problem 3: "LLM denies access to collection I should have"

**Error Message**:
```
QueryLLMAccessDenied: Access denied for collection 'customer_data'
  Reason: parser_stage
  Message: "app_user_123 does not have permission to query customer_data"
```

**Cause 1**: Your access policy doesn't grant permission (correct behavior)

**Solution**: Contact your administrator to request access

**Cause 2**: Access policy changed but LLM prompt has stale info

**Solution**:
```python
# Step 1: Verify your actual permissions
perms = auth_service.get_permissions(user_id)
print(f"Collections you have access to: {perms.collections}")

# Step 2: Update LLM context with correct permissions
engine.set_llm_context(LLMContext(
    user_collections=perms.collections  # ← Must match actual permissions
))

# Step 3: Retry
result = engine.execute_llm_query(nl_query)
```

**Prevention**:
- Check permissions before querying: `auth_service.can_access(collection)`
- Include only allowed collections in LLM prompt
- Audit permission changes monthly

---

### Problem 4: "How do I enable LLM for my application?"

**Step-by-Step**:

1. **Check Query Module version**:
```bash
curl http://themis-server:8080/version
# Requires v2.1.0+
```

2. **Set LLM credentials** (if using external LLM):
```bash
export OPENAI_API_KEY="sk-..."
export THEMIS_LLM_MODEL="openai:gpt-4"
```

3. **Enable in your app config**:
```python
# Python SDK
from themis import QueryEngine, LLMContext

engine = QueryEngine()
engine.set_llm_context(LLMContext(
    model_id="openai:gpt-4",
    prompt_version="gpt-4-turbo-v1.0"
))
```

4. **Test**:
```python
result = engine.translate_nl_to_aql("Find users over age 30")
print(result.aql_query)  # Should output: SELECT * FROM users FILTER age > 30
```

---

## Section 5: Advanced Scenarios

### Scenario 1: Custom LLM Model Integration

If you have a custom/fine-tuned LLM model:

```python
from themis import QueryEngine, LLMContext

engine = QueryEngine()

# Point to your custom model
engine.set_llm_context(LLMContext(
    model_id="custom:themis-tuned-v2.0",
    prompt_version="custom-v2.0",
    strict_mode=False,  # Relax checks for trusted model
    feature_flags={
        "enforce_semantic_checks": True,  # Keep semantic validation
        "enforce_boundary_checks": False,  # Custom model handles boundaries
    }
))

# Use your model
result = engine.translate_nl_to_aql("Find top 10 customers by revenue")
```

**Benefits**: Custom models can be faster and cheaper than OpenAI

**Tradeoffs**: Requires fine-tuning on your schema and patterns

---

### Scenario 2: Federated Query Generation via LLM

If your data is spread across multiple shards:

```python
# LLM can generate federated queries automatically
nl_query = "Find orders across all regions in the last 30 days"

# LLM translates to:
aql_query = """
SELECT * FROM orders 
WHERE created_at > '2026-07-06' 
FEDERATE TO all_shards
"""

# QueryEngine handles federation transparently
result = engine.execute_aql(aql_query)
```

**What LLM Learns**: Query patterns, field names, partition schemes

**Monitoring**: Check federation metrics to ensure queries are well-distributed

---

### Scenario 3: Multi-Tenant LLM Query Isolation

If you have multiple tenants (SaaS):

```python
# Per-tenant LLM context with restricted schema
for tenant in tenants:
    # Each tenant gets a restricted schema
    tenant_schema = schema.filter_by_tenant(tenant.id)
    
    engine.set_llm_context(LLMContext(
        tenant_id=tenant.id,
        schema_hint=tenant_schema,
        user_collections=tenant.allowed_collections,
    ))
    
    # LLM can only see and query tenant's data
    result = engine.translate_nl_to_aql(nl_query, tenant_id=tenant.id)
```

**Security**: LLM can't generate cross-tenant queries

**Performance**: Smaller schema = faster LLM inference

---

## Section 6: Metrics Monitoring

### What to Monitor (Summary)

**Dashboard: AQL LLM Integration — Phase 3 Monitoring**

| Metric | What It Means | Healthy | Warning | Critical |
|---|---|---|---|---|
| `parser.phase1.boundary_validation_ms` (p95) | Validation speed | < 5ms | 5-10ms | > 10ms |
| `LLM Success Rate` | Percentage of valid queries | > 95% | 90-95% | < 90% |
| `LLM Access Denial Rate` | Access control compliance | < 2% | 2-5% | > 5% |
| `parser.aql.parse_latency_ms` (p99) | Total parse time | < 500ms | 500-1000ms | > 1000ms |

### Create Alerts

**For your team** (Prometheus AlertManager):

```yaml
groups:
  - name: llm_integration
    rules:
      - alert: LLMValidationFailureRateHigh
        expr: >
          rate(parser_errors_llm_validation_fail_count[5m]) 
          / 
          rate(parser_phase1_boundary_validation_ms_count{source="llm"}[5m])
          > 0.10
        for: 5m
        annotations:
          summary: "LLM validation failure rate > 10%"
          action: "Review LLM prompt; check for model/schema changes"

      - alert: LLMLatencyHigh
        expr: >
          histogram_quantile(0.95, parser_phase1_boundary_validation_ms) > 10
        for: 5m
        annotations:
          summary: "LLM validation p95 latency > 10ms"
          action: "Check query complexity; simplify LLM prompt"

      - alert: AccessDenialRateHigh
        expr: >
          rate(parser_access_llm_denial_count[5m]) 
          / 
          rate(parser_phase1_boundary_validation_ms_count{source="llm"}[5m])
          > 0.05
        for: 10m
        annotations:
          summary: "High rate of access denials (> 5%)"
          action: "Audit access policies and LLM permission metadata"
```

---

## Summary & Next Steps

### What You've Learned

✅ LLM-enhanced AQL queries are now available  
✅ Your existing queries continue to work unchanged  
✅ LLM features are opt-in (safe to try)  
✅ Quality is monitored via built-in metrics  
✅ Best practices and troubleshooting techniques  

### Next Steps

1. **Week 1**: Enable LLM integration on dev/staging
2. **Week 2**: Try simple queries; monitor error rate
3. **Week 3**: Optimize LLM prompt based on metrics
4. **Week 4**: Roll out to production with confidence

### Resources

- **Metrics Guide**: `src/query/AQL_LLM_INTEGRATION_PHASE3_METRICS.md`
- **API Contract**: `src/query/AQL_LLM_INTEGRATION_PHASE3_API_CONTRACT.md`
- **Parser Changes**: `src/query/AQL_LLM_INTEGRATION_PHASE3_PARSER_CHANGES.md`
- **Prometheus Dashboard**: `Grafana → Dashboards → AQL LLM Integration`
- **Support**: Contact #themis-llm Slack channel

---

**Document Status**: ✅ COMPLETE  
**Last Updated**: 2026-08-05  
**Author**: Query Module Phase 6A Documentation Agent  
**Review Status**: Pending core team review
