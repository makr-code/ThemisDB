# GitHub Issue Templates for ThemisDB

This directory contains GitHub issue templates for various components and migration efforts in ThemisDB.

## 📋 Template Categories

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
