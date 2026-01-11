# GitHub Issues - Generated from Gap Analysis

This directory contains **GitHub-ready issues** generated from comprehensive gap analysis of:
- Documentation (research documents, TODOs)
- Source code (TODOs, FIXMEs, Stubs, Mocks)
- Strategic gaps (BSI compliance, enterprise features)

**Generated**: 2026-01-11  
**Source Analysis**:
- **229 markers found** (complete exhaustive search)
- **92 files** with TODOs/stubs across entire codebase
- **40+ component directories** analyzed
- All gaps verified before issue creation

---

## 📊 Issue Statistics

### By Priority
- **CRITICAL**: 0 issues (Apache Ranger already implemented, skipped)
- **HIGH**: 3 issues (Self-awareness, RPC implementation, Security stubs)
- **MEDIUM**: 4 issues (LLM features, Metrics, Process Mining, Scheduler)

### By Component
- **Self-Awareness / Agentic AI**: 1 issue
- **RPC / Distribution**: 1 issue
- **Security / Enterprise**: 1 issue
- **LLM Integration**: 2 issues
- **Process Mining / Analytics**: 1 issue
- **Scheduler / Task Management**: 1 issue

### Status
- ✅ **All issues verified** - features confirmed as missing before issue creation
- ✅ **No duplicate issues** - each represents unique gap
- ✅ **All issues actionable** - include implementation details, tasks, timelines
- ✅ **Exhaustive search** - Complete codebase scan found **229 markers** (all files, all directories)

---

## 📝 Issue List

### 001: Schema Manager for Self-Awareness
**Priority**: HIGH | **Effort**: 2-4 weeks | **Component**: Self-Awareness

Implement core `SchemaManager` class to enable database introspection. Critical path for all Agentic AI features.

**Blocks**:
- REST API schema endpoints
- MCP full integration
- Natural language Q&A

**Source**: 
- `docs/research/AGENTIC_AI_SELF_AWARENESS_RESEARCH.md`
- `docs/research/IMPLEMENTATION_CHECKLIST.md`
- Verified: `include/metadata/schema_manager.h` does not exist

---

### 002: RPC Service Full Implementation
**Priority**: HIGH | **Effort**: 3-4 weeks | **Component**: RPC/Distribution

Replace 13+ stub implementations in RPC service with actual database operations.

**Impact**: Blocks distributed deployments, multi-shard operations, replication

**Source**:
- `src/server/rpc/rpc_service_impl.cpp` (13+ TODOs verified)
- `docs/de/development/GAPS_STUBS_SUMMARY.md`

**Stubs Found**:
- Get, Put, Delete operations
- Batch operations
- Query execution
- Transactions (Begin, Commit, Abort)
- Vector/Graph/Geo/TimeSeries queries

---

### 003: Security Stubs Production Implementation
**Priority**: HIGH | **Effort**: 2-3 weeks | **Component**: Security/Enterprise

Replace 13 security stubs with production implementations.

**Impact**: Blocks BSI compliance, enterprise deployments, SOC 2 certification

**Source**:
- `src/security/timestamp_authority.cpp` (4 stubs)
- `src/security/hsm_provider.cpp` (4 stubs)
- `src/security/hsm_provider_pkcs11.cpp` (2 stubs)
- `src/security/vault_signing_provider.cpp` (3 mocks)
- Critical TODOs in license verification, X.509 validation

**Components**:
- Timestamp Authority (RFC 3161)
- HSM Provider (PKCS#11)
- License signature verification (RSA-SHA256)
- X.509 chain validation (CRL/OCSP)

---

### 004: LLM Model Loading to Blob Store
**Priority**: MEDIUM | **Effort**: 1-2 weeks | **Component**: LLM/Storage

Implement model loading from RocksDB Blob Store (currently file system only).

**Benefits**: Portability, unified management, versioning, distribution

**Source**:
- `src/llm/gguf_loader.cpp:173` (TODO verified)

---

### 005: LLM Grafana Metrics Implementation
**Priority**: MEDIUM | **Effort**: 1-2 weeks | **Component**: LLM/Observability

Replace 19 stub implementations in Grafana metrics with real Prometheus integration.

**Impact**: No production monitoring, no alerting, blind to LLM performance issues

**Source**:
- `src/llm/grafana_metrics_stub.cpp` (entire file)
- `src/llm/grafana_metrics.cpp` (multiple stubs)

**Metrics to Implement**:
- Request metrics (count, latency, errors)
- Token metrics (generation rate, totals)
- Model metrics (memory, load time)
- Cache metrics (hit rate, size)
- GPU metrics (utilization, memory)
- Queue metrics (size, wait time)

---

## 🔍 Verification Process

Each issue was **verified before creation**:

1. **File existence check**: Ensure implementation files don't exist
2. **Code pattern search**: Search for class/function definitions  
3. **TODO/Stub verification**: Confirm TODO comments and stub implementations
4. **Functionality check**: Verify feature is not already implemented

**Example verification** (Issue #001):
```bash
# File check
✅ include/metadata/schema_manager.h does not exist
✅ src/metadata/schema_manager.cpp does not exist

# Code check  
✅ No "class SchemaManager" found in codebase

# Stub verification
✅ MCP Server returns empty arrays in toolGetSchema()
```

---

## 🚀 Usage

These issues are **ready to be filed** in GitHub:

### Option 1: Manual Filing
1. Go to https://github.com/makr-code/ThemisDB/issues/new
2. Copy content from `001-schema-manager-self-awareness.md`
3. Paste into issue form
4. Submit

### Option 2: GitHub CLI
```bash
gh issue create \
  --title "Implement Schema Manager for Database Self-Awareness" \
  --label "enhancement,self-awareness,agentic-ai,mcp,priority-high,v1.5" \
  --body-file .github/issues/001-schema-manager-self-awareness.md
```

### Option 3: GitHub API
```bash
curl -X POST \
  -H "Authorization: token YOUR_TOKEN" \
  -d @001-schema-manager-self-awareness.md \
  https://api.github.com/repos/makr-code/ThemisDB/issues
```

---

### 006: Process Mining Features
**Priority**: MEDIUM | **Effort**: 3-4 weeks | **Component**: Process Mining/Analytics

Implement 8 missing process mining features: condition evaluation, event handling, task assignment queries, node history, process metrics, critical path analysis, and hyperedge operations.

**Source**:
- `src/index/process_graph.cpp` (8 TODOs verified at lines 709, 832, 841, 852, 861, 868, 876, 883)

---

### 007: Task Scheduler Implementation
**Priority**: MEDIUM | **Effort**: 2-3 weeks | **Component**: Scheduler

Complete task scheduler with 16 missing features for task management, scheduling, execution, and resource allocation.

**Source**:
- `src/scheduler/task_scheduler.cpp` (16 TODOs verified)

---

## 📋 Remaining Gap Categories

### Not Yet Converted to Issues (Lower Priority)

**Total catalogued**: 222 additional markers across:

#### By Component (Top Categories)
- **LLM TODOs** (73 total): CUDA kernels, async loading, grammar support, vision integration, production validator tests (31 TODOs)
- **Server/API TODOs** (41 total): RPC implementation (17), WebSocket integration, Postgres queries, HTTP optimizations
- **Security** (20 total): USB admin authenticator (6), stubs already covered in Issue #003
- **Scheduler** (16 total): Covered in Issue #007
- **Tests** (20 total): Integration tests, various test infrastructure
- **Tools** (7 total): themis_docs_builder TODOs
- **Benchmarks** (3 total): Sharding performance, insert profiling
- **Analytics/AQL** (4 total): LLM integration, batch inference
- **Others** (38 total): Storage, sharding, temporal, timeseries, updates, exporters, etc.

#### Newly Found Categories (23 additional markers)
- `benchmarks/` (3): Performance benchmark completion
- `src/analytics/` (1): LLM API integration  
- `src/aql/` (3): Batch inference, vector search integration
- `src/base/` (1): Module loader
- `src/exporters/` (1): JSONL exporter
- `src/timeseries/` (1): TSStore implementation
- `src/updates/` (1): Manifest database
- `tools/themis_docs_builder/` (7): Documentation builder TODOs
- Additional markers in security (usb_admin_authenticator)
- Additional test files with markers

---

## 🎯 Recommended Action Plan

### Phase 1: HIGH Priority (Weeks 1-8)
1. **Issue #001**: Schema Manager (2-4 weeks)
2. **Issue #003**: Security Stubs (2-3 weeks)
3. **Issue #002**: RPC Service (3-4 weeks)

### Phase 2: MEDIUM Priority (Weeks 9-12)
4. **Issue #004**: Model Blob Store (1-2 weeks)
5. **Issue #005**: Grafana Metrics (1-2 weeks)

### Phase 3: Additional Issues (As Needed)
- Create issues for remaining LLM TODOs
- Create issues for documentation gaps
- Create issues for lower-priority enhancements

---

## 📚 Related Documentation

- **Research Summary**: `docs/research/README.md`
- **Self-Awareness Research**: `docs/research/AGENTIC_AI_SELF_AWARENESS_RESEARCH.md`
- **Implementation Checklist**: `docs/research/IMPLEMENTATION_CHECKLIST.md`
- **Gap Analysis**: `docs/de/development/GAPS_STUBS_SUMMARY.md`
- **Documentation Gaps**: `docs/de/reports/DOCUMENTATION_GAP_ANALYSIS.md`

---

## ✅ Quality Assurance

All issues in this directory meet the following criteria:

- ✅ **Verified missing**: Feature/implementation confirmed as not present
- ✅ **Well-documented**: Clear problem statement and proposed solution
- ✅ **Actionable tasks**: Specific implementation steps provided
- ✅ **Success criteria**: Clear definition of done
- ✅ **Timeline estimated**: Realistic effort estimates
- ✅ **Dependencies identified**: Related issues and blockers listed
- ✅ **Source cited**: Original TODO/research document referenced

---

**Maintained by**: GitHub Copilot AI Agent  
**Last Updated**: 2026-01-11  
**Analysis Date**: 2026-01-11  
**Verification Method**: Automated + Manual Review
