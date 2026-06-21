# Gap Scanner V3 - File Naming Standardization

**Goal**: Consistent naming convention across all 47 scanners  
**Pattern**: `gs3_step<N>_<category>_<name>.py`

## Current Issues

### Inconsistent Prefixes
- `ai_*` (5 files) - AI-Vibe specific
- `classic_*` (9 files) - Classic C++ 
- None (33 files) - Mixed purposes

### Inconsistent Suffixes
- `_improved` (8 files)
- No suffix (39 files)

---

## Proposed Standard Categories

### Step 0 - Meta
```
gs3_step00_orchestrator.py
```

### Step 1 - Baseline Detection
**Category Prefixes**:
- `ai_` - Production logic, stubs, prompts, documentation (5)
- `core_` - Memory, RAII, concurrency (9)
- `check_` - Syntactic checks, parsing (1)

**Files to Rename**:
```
KEEP (ai_*):
  gs3_step01_ai_error_handling_consistency.py
  gs3_step01_ai_header_drift.py
  gs3_step01_ai_llm_prompt_injection.py
  gs3_step01_ai_simulation_stub_leak.py
  gs3_step01_ai_todo_productionlogic.py

RENAME (classic_* → core_*):
  gs3_step01_classic_concurrency.py → gs3_step01_core_concurrency.py
  gs3_step01_classic_container.py → gs3_step01_core_container.py
  gs3_step01_classic_memory_improved.py → gs3_step01_core_memory.py
  gs3_step01_classic_performance.py → gs3_step01_core_performance.py
  gs3_step01_classic_platform.py → gs3_step01_core_platform.py
  gs3_step01_classic_raii.py → gs3_step01_core_raii.py
  gs3_step01_classic_reliability.py → gs3_step01_core_reliability.py
  gs3_step01_classic_security.py → gs3_step01_core_security.py
  
RENAME (no prefix → core_*):
  gs3_step01_error_handling.py → gs3_step01_core_error_handling.py
  gs3_step01_memory_safety_improved.py → gs3_step01_core_memory_safety.py
  gs3_step01_namespace_unity_check.py → gs3_step01_check_namespace_unity.py
  gs3_step01_raii.py → gs3_step01_core_raii_patterns.py (consolidate with _classic_raii)
  gs3_step01_thread_safety_improved.py → gs3_step01_core_thread_safety.py
  gs3_step01_braces_check.py → gs3_step01_check_braces.py
```

### Step 2 - Context-Aware Analysis
**Category Prefix**: `safety_`

```
RENAME (all to safety_*):
  gs3_step02_exception_safety_improved.py → gs3_step02_safety_exception.py
  gs3_step02_input_validation.py → gs3_step02_safety_input_validation.py
  gs3_step02_type_conversion.py → gs3_step02_safety_type_conversion.py
  gs3_step02_uninitialized_improved.py → gs3_step02_safety_uninitialized.py
  gs3_step02_virtual_oop.py → gs3_step02_safety_virtual_oop.py
```

### Step 3 - Security & Cryptography
**Category Prefix**: `security_`

```
RENAME (all to security_*):
  gs3_step03_attack_vectors.py → gs3_step03_security_attack_vectors.py
  gs3_step03_data_leak_improved.py → gs3_step03_security_data_leak.py
  gs3_step03_e2e_encryption.py → gs3_step03_security_e2e_encryption.py
  gs3_step03_encryption_leak_improved.py → gs3_step03_security_encryption_leak.py
  gs3_step03_key_failure_improved.py → gs3_step03_security_key_failure.py
  gs3_step03_legacy_duplication_improved.py → gs3_step03_security_legacy_duplication.py
  gs3_step03_military_hardening.py → gs3_step03_security_military_hardening.py
```

### Step 4 - Design & Architecture Rules
**Category Prefixes**:
- `design_` - Architecture, governance, consistency (12)
- `quality_` - Documentation, API standards (5)

```
RENAME (design_*):
  gs3_step04_architecture_rules.py → gs3_step04_design_architecture.py
  gs3_step04_bridge_interface_rules.py → gs3_step04_design_bridge_interface.py
  gs3_step04_deprecated_apis.py → gs3_step04_design_deprecated_apis.py
  gs3_step04_design_error_rules_improved.py → gs3_step04_design_error_rules.py
  gs3_step04_determinism_improved.py → gs3_step04_design_determinism.py
  gs3_step04_distributed_consistency_improved.py → gs3_step04_design_distributed_consistency.py
  gs3_step04_gpu_memory.py → gs3_step04_design_gpu_memory.py
  gs3_step04_llm_ai_safety.py → gs3_step04_design_llm_ai_safety.py
  gs3_step04_module_governance_rules.py → gs3_step04_design_module_governance.py
  gs3_step04_observability_improved.py → gs3_step04_design_observability.py
  gs3_step04_performance_patterns_improved.py → gs3_step04_design_performance_patterns.py
  gs3_step04_query_correctness.py → gs3_step04_design_query_correctness.py

RENAME (quality_*):
  gs3_step04_audit_logging_improved.py → gs3_step04_quality_audit_logging.py
  gs3_step04_cpp_doxygen_policy_rules.py → gs3_step04_quality_cpp_doxygen.py
  gs3_step04_doc_freshness_rules.py → gs3_step04_quality_doc_freshness.py
  gs3_step04_docs_markdown_rules.py → gs3_step04_quality_docs_markdown.py
```

---

## Summary of Changes

| Step | Category | Count | From | To |
|------|----------|-------|------|-----|
| 1 | ai_ | 5 | ai_* | ai_* (keep) |
| 1 | core_ | 9 | classic_* | core_* |
| 1 | check_ | 2 | (none) | check_* |
| 1 | core_ | 5 | (none) | core_* |
| 2 | safety_ | 5 | (none) | safety_* |
| 3 | security_ | 7 | (none) | security_* |
| 4 | design_ | 12 | (none) | design_* |
| 4 | quality_ | 4 | (none) | quality_* |

**Total Changes**: 44 renames (keep gs3_step00 + 5 ai_* unchanged)

---

## Execution Plan

1. Rename all files using consistent pattern
2. Update imports in `gs3_orchestrator.py` if needed
3. Update documentation (GS3_COMPLETE_GUIDE.md, GS3_INTEGRATION_STATUS.md)
4. Test scanner discovery
5. Verify all scanners still load correctly

---

## Standardized Naming Convention

```
gs3_step<N>_<category>_<name>.py

Where:
  N = 0-4 (execution step)
  <category> = ai | core | check | safety | security | design | quality
  <name> = descriptive snake_case name (no "_improved" suffix)
```

**Benefits**:
- ✅ Consistent pattern across all 47 scanners
- ✅ Easy to identify scanner purpose by category
- ✅ No suffix duplication (_improved appears 8 times)
- ✅ Better filesystem organization
- ✅ Clearer for auto-discovery and documentation
