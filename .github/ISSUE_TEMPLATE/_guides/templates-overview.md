# GitHub Issue Templates for ThemisDB

This directory contains GitHub issue templates for various components and migration efforts in ThemisDB.

## 📋 Template Categories

### 🔄 Systematic Component Review Templates (NEW - v1.4.1)

**Purpose:** Repeatable templates for systematic review of ThemisDB components covering best practices, state-of-the-art research, documentation, roadmap, security, and compliance.

**Master Template:**
- **`SYSTEMATIC_COMPONENT_REVIEW_TEMPLATE.md`** - Comprehensive template for any ThemisDB component with all review sections

**Specialized Templates:**
- **`core_database_component_review.md`** - For core database components (Storage, Transaction, Query, Index, AQL)
- **`ai_llm_component_review.md`** - For AI/LLM components (LLM Engine, Embeddings, RAG, Voice, Ethics)
- **`distributed_systems_component_review.md`** - For distributed systems (Sharding, Replication, Consensus, CDC)
- **`network_api_component_review.md`** - For network & API components (HTTP, gRPC, WebSocket, MQTT, PostgreSQL Wire, GraphQL)

**Key Features of Systematic Review Templates:**
- ✅ **Best Practices Analysis** - Code quality, design patterns, modern standards
- ✅ **State of the Art** - Research papers, competitive analysis, technology trends
- ✅ **Documentation Review** - Code docs, user docs, developer docs, gaps identification
- ✅ **Developer Roadmap** - Current state, technical debt, short/medium/long-term plans
- ✅ **Security & Compliance** - Threat modeling, vulnerability assessment, regulatory compliance (BSI C5, ISO 27001, DSGVO, NIS2)
- ✅ **Performance Analysis** - Current metrics, bottlenecks, optimization opportunities
- ✅ **Testing & Quality** - Test coverage, test types, testing gaps
- ✅ **Dependencies & Integration** - External/internal dependencies, integration points
- ✅ **Metrics & KPIs** - Code metrics, quality metrics, operational metrics
- ✅ **Action Items** - Prioritized action items with owners and due dates

**Usage:**
1. Choose the appropriate specialized template for your component area
2. Fill in all relevant sections thoroughly
3. Conduct research on state-of-the-art solutions
4. Document findings, gaps, and recommendations
5. Create prioritized action items
6. Schedule next review date (recommended: quarterly)

**Related Documentation:**
- `docs/de/compliance/compliance_full_checklist.md` - Full compliance checklist
- `docs/de/security/ANGRIFFSVEKTOREN_ANALYSE_RUNBOOK.md` - Security analysis runbook

### 🔒 Security Attack Vector Templates (NEW - v1.4.1)

**Purpose:** Track findings from systematic attack vector analysis framework

Specialized templates for security attack vectors organized by category:

- **`security_network_attack.md`** - Network-level attack vectors (HTTP smuggling, SSRF, WebSocket, gRPC, MQTT)
- **`security_authentication_attack.md`** - Authentication/authorization attack vectors (JWT, session, RBAC, IDOR)
- **`security_injection_attack.md`** - Injection attack vectors (AQL, NoSQL, command, LLM prompt injection)
- **`security_crypto_attack.md`** - Cryptographic attack vectors (weak ciphers, key management, padding oracle)
- **`security_distributed_attack.md`** - Distributed system attack vectors (consensus, MVCC, shard attacks)

**Related Workflows:**
- `.github/workflows/security.yml` - Comprehensive security scanning (CodeQL, Trivy, Gitleaks, cppcheck)
- `.github/workflows/tests-specialized.yml` - Fuzzing tests for injection vectors

**Documentation:**
- `docs/de/security/ANGRIFFSVEKTOREN_ANALYSE_RUNBOOK.md` - Analysis runbook
- `docs/de/security/ATTACK_VECTOR_ANALYSIS_FRAMEWORK.md` - Framework documentation

### 🌐 Distributed Sharding Templates (NEW - v1.4)

**See:** `SHARDING_TEMPLATES_README.md` for complete documentation

Specialized templates for distributed sharding, consensus, and transaction coordination:

- **`sharding_bug.md`** - Report bugs in Raft/Paxos/Gossip consensus or cross-shard transactions
- **`sharding_feature.md`** - Request new distributed systems features
- **`sharding_performance.md`** - Report performance issues in consensus or transactions
- **`consensus_implementation.md`** - Track consensus module stub completions
- **`transaction_implementation.md`** - Track transaction protocol stub completions

### 🐛 General Templates

- **`bug_report.md`** - Standard bug report template
- **`feature_request.md`** - Standard feature request template

### 🔄 Query Engine Migration Templates

Templates for the remaining phases of the Query Engine error handling migration:

### Phase 3: AQL Translator Migration
**File:** `phase3_aql_translator_migration.md`  
**Priority:** P0 (Critical)  
**Effort:** 2-3 weeks  
**Points:** 96

Migrate AQL Translator error handling from Status returns to Result<T> pattern.

### Phase 4: Query Engine Core Migration
**File:** `phase4_query_engine_core_migration.md`  
**Priority:** P0 (Critical)  
**Effort:** 2 weeks  
**Points:** 72

Migrate Query Engine Core from legacy patterns (nullptr, Status) to Result<T>.

### Phase 5: Other Query Components Migration
**File:** `phase5_other_components_migration.md`  
**Priority:** P1 (High)  
**Effort:** 3 weeks  
**Points:** ~62

Migrate remaining query components (Expression Evaluator, Join Operations, Query Planner).

### Phase 6: Testing & Validation
**File:** `phase6_testing_validation.md`  
**Priority:** P0 (Critical)  
**Effort:** 1 week

Comprehensive testing and validation of all migrations.

## 🚀 How to Use

1. **Navigate to Issues** in your GitHub repository
2. **Click "New Issue"**
3. **Select the appropriate template** for the phase you're working on
4. **Fill in any additional details**
5. **Create the issue**

## 📊 Migration Progress

| Phase | Status | Points | Effort |
|-------|--------|--------|--------|
| Phase 1-2 | ✅ Complete | 35 | 2 weeks |
| Phase 3 | ⏳ Ready | 96 | 2-3 weeks |
| Phase 4 | ⏳ Ready | 72 | 2 weeks |
| Phase 5 | ⏳ Ready | ~62 | 3 weeks |
| Phase 6 | ⏳ Ready | - | 1 week |
| **Total** | **13.2%** | **265** | **~10 weeks** |

## 📚 Resources

- **Migration Roadmap:** `MIGRATION_ROADMAP.md`
- **Current Status:** `CURRENT_STATUS.md`
- **Code Review:** `CODE_REVIEW.md`
- **Migration Guide:** `docs/error_handling/phase4_query_engine_migration_example.md`
- **Phase 1 Summary:** `PHASE1_COMPLETE_SUMMARY.md`
- **Phase 2 Summary:** `PHASE2_COMPLETE_SUMMARY.md`

## 🎯 Success Criteria

All phases complete when:
- ✅ All 265 migration points converted to Result<T>
- ✅ All tests passing (100%)
- ✅ Performance regression < 5%
- ✅ Documentation complete
- ✅ Code review approved

---

**Created:** 2026-01-20  
**Status:** Ready for use
