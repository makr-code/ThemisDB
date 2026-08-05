# Prompt Engineering Module - Backward Compatibility & Feature Gating (Phase 1)

## Document Status
- **Maturity:** 🟡 DESIGN/CONTRACT
- **Status:** Phase 1 frozen agreement (Q3 2026)
- **Version:** 1.0.0
- **Target Release:** v2.4.0-rc1 and later

---

## Backward Compatibility Guarantee

### Core API Stability
All public prompt engineering APIs remain backward-compatible within the major release line (v2.x):

| API Surface | Stability | Notes |
|-------------|-----------|-------|
| `PromptManager` lifecycle (create/get/list/delete) | ✅ STABLE | No signature changes planned |
| `PromptVersionControl` commit/history/rollback | ✅ STABLE | No signature changes planned |
| `PromptOptimizer` optimization loop | ✅ STABLE | Configuration may expand, not break |
| `PromptEvaluator` evaluation interface | ✅ STABLE | No breaking signature changes |
| `FeedbackCollector` feedback ingestion | ✅ STABLE | New fields are optional |
| `PromptEngineeringMetrics` metrics recording | ✅ STABLE | New metrics are additive |

### Error Code Stability
Error codes in the range 9500–9899 are frozen for this release line:
- Existing codes will never change meaning
- New errors will use un-assigned codes
- All error codes map to `PromptEngineeringErrorCode` enum (see `prompt_engineering_errors.h`)

### RewriteEngine Stability (NEW in Phase 2)
RewriteEngine interfaces are new in Phase 2 (Q4 2026) and will stabilize at v2.4.0:
- Public interfaces: `IRewriteEngine`, `IRewriteRule`
- Support types: `RewriteDocument`, `RewriteContext`, `RewriteResult`, `RewriteTrace`
- After v2.4.0, signature changes to these types will go through deprecation cycles

---

## Feature Gating Strategy

### Phase 1 (Q3 2026): Interface Contracts Only
- **Enabled by default:** No (interfaces only, no implementation)
- **Feature gate:** Not applicable yet

### Phase 2 (Q4 2026): RewriteEngine Implementation
- **Feature gate:** `THEMISDB_ENABLE_REWRITE_ENGINE` (CMake flag)
- **Default:** OFF (opt-in until Phase 3 sign-off)
- **Impact:** Controls RewriteEngine implementation availability
- **Fallback:** If disabled, no rewrite operations are available; template manager operates unchanged

### Phase 3 (Q4 2026): Production Hardening
- **Feature gate:** `THEMISDB_ENABLE_REWRITE_ENGINE_STRICT_VALIDATION` (CMake flag)
- **Default:** OFF (strict mode is opt-in)
- **Impact:** Enables fail-fast validation and stricter phase isolation
- **Recommendation:** Enable in production after Phase 3 sign-off

### Phase 4 (Q1 2027): Full Production Rollout
- **Feature gate:** May remain for backward compat but recommended default is ON
- **Deprecation:** Legacy non-rewrite paths deprecated (marked `[[deprecated]]`)
- **Timeline:** Full removal in v3.0.0

---

## Contract Violation Handling

### What Constitutes a Breaking Change
- Removing an existing public method signature
- Changing a public method's parameter types or return type
- Changing error code meanings
- Removing error codes from enum
- Removing public fields from structures

### What Does NOT Constitute Breaking
- Adding new optional parameters to methods (with defaults)
- Adding new error codes
- Adding new public methods
- Making library behavior more strict (if properly versioned)
- Refactoring private implementation

### Migration Path for Any Breaking Changes
If a breaking change is absolutely necessary:
1. Deprecate old API for 1–2 minor versions
2. Mark deprecated API with `[[deprecated("use X instead")]]`
3. Provide migration guide in changelog
4. Only remove deprecated API in next major version (v3.0.0)

---

## Concurrent Mutation Contract

The prompt engineering module guarantees:
- ✅ Template create/delete operations are atomic
- ✅ Version control commits are atomic
- ✅ Optimizer state changes are isolated
- ✅ No silent data corruption under concurrent access

The module does NOT guarantee:
- ❌ Lock-free performance (uses locking where needed)
- ❌ Stale read protection for unsynced clients
- ❌ Automatic conflict resolution (returns explicit error)

---

## Error Taxonomy Stability

### Error Code Ranges (Q3 2026 — Frozen)
| Range | Category | Status |
|-------|----------|--------|
| 9500–9549 | Template errors | Frozen |
| 9550–9599 | Injection errors | Frozen |
| 9600–9649 | Version errors | Frozen |
| 9650–9699 | Optimization errors | Frozen |
| 9700–9749 | Evaluator errors | Frozen |
| 9750–9799 | Rewrite errors | Frozen (Phase 2+) |
| 9800–9849 | Concurrency errors | Frozen |
| 9850–9899 | Configuration errors | Frozen |

### Recovery Strategies
All errors in `PromptEngineeringErrorCode` include a `remediation_hint` in the `PromptEngineeringErrorContext` struct. Operators MUST check this field for recovery suggestions.

---

## Configuration File Contracts

### YAML Rule Files (Phase 2+)
- YAML rule files use schema version `rewrite_rules_schema_v1`
- Schema is frozen for this release
- Breaking schema changes go to `rewrite_rules_schema_v2` in v3.0.0
- Parser rejects unknown versions with `CONFIG_SCHEMA_MISMATCH` error

### Configuration Merging
- Low-risk lexical rules loaded from YAML can be combined with C++ policy rules
- Later rules override earlier ones (deterministic priority ordering applies)
- No silent conflicts or undefined behavior

---

## Testing Contracts

All tests MUST:
- ✅ Use error codes from `PromptEngineeringErrorCode` enum
- ✅ Verify deterministic behavior (same input → same output)
- ✅ Handle concurrent mutations correctly
- ✅ Use `PromptEngineeringErrorContext` for error diagnostics
- ✅ Document test assumptions about resource limits and timeouts

---

## Documentation Contracts

### Versioning
All major version boundaries (v1.x → v2.0, v2.x → v3.0) will include:
- Comprehensive migration guide in `docs/MIGRATION.md`
- Mapping of old APIs to new APIs
- Changelog documenting all breaking changes
- Deprecation timeline for affected features

### Doxygen API Docs
- All public APIs have mandatory `@brief`, `@param`, `@return` documentation
- All error cases have documented return codes or exceptions
- All thread-safety assumptions are documented
- All performance contracts (time/space bounds) are documented

---

## Release Gate Criteria

For Phase 1 (Design Contract) to be considered COMPLETE:
- ✅ All interface headers (`rewrite_engine.h`, `prompt_engineering_errors.h`) compile without errors
- ✅ All error codes are documented in `PromptEngineeringErrorCode` enum
- ✅ Backward compatibility guarantee is documented and reviewed
- ✅ Feature gating strategy is documented and approved
- ✅ No implementation code exists (interfaces and documentation only)

---

## Approval & Sign-Off

**Phase 1 Contract Frozen By:**
- [ ] Architecture review (AI agent)
- [ ] Security review (human)
- [ ] Product management sign-off (human)
- [ ] Release engineering sign-off (human)

**Expected Sign-Off Date:** End of Q3 2026

---

## References

- [Prompt Engineering ROADMAP](./ROADMAP.md)
- [FUTURE_ENHANCEMENTS](./FUTURE_ENHANCEMENTS.md)
- [Error Taxonomy Header](../include/prompt_engineering/prompt_engineering_errors.h)
- [RewriteEngine Interface Header](../include/prompt_engineering/rewrite_engine.h)
