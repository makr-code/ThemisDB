# Legacy Gap Scanner V3 Files Mapping

**Date Archived**: 2026-06-21  
**Reason**: Consolidation into standardized `gs3_step<N>_<category>_<name>.py` naming convention

## Archive Contents

This directory contains 33 legacy gap scanner files that have been superseded by the standardized Gap Scanner V3 system.

These files are **no longer used** in production. All scanning functionality has been consolidated into the modern `tools/scanners/gs3_step*.py` files.

## Legacy File Listing

### Phase 1 Scanners (Legacy)
```
gap_scanner_v3_01_ai_error_handling.py
gap_scanner_v3_01_ai_header_drift.py
gap_scanner_v3_01_ai_llm_prompt_injection.py
gap_scanner_v3_01_ai_simulation_stub_leak.py
gap_scanner_v3_01_ai_todo_productionlogic.py
gap_scanner_v3_01_braces_check.py
gap_scanner_v3_01_classic_concurrency.py
gap_scanner_v3_01_classic_container.py
gap_scanner_v3_01_classic_memory_improved.py
gap_scanner_v3_01_classic_performance.py
gap_scanner_v3_01_classic_platform.py
gap_scanner_v3_01_classic_raii.py
gap_scanner_v3_01_classic_reliability.py
gap_scanner_v3_01_classic_security.py
gap_scanner_v3_01_error_handling.py
gap_scanner_v3_01_memory_safety_improved.py
gap_scanner_v3_01_namespace_unity_check.py
gap_scanner_v3_01_thread_safety_improved.py
```

### Phase 2 Scanners (Legacy)
```
gap_scanner_v3_02_exception_safety_improved.py
gap_scanner_v3_02_input_validation.py
gap_scanner_v3_02_type_conversion.py
gap_scanner_v3_02_uninitialized_improved.py
gap_scanner_v3_02_virtual_oop.py
```

### Phase 3 Scanners (Legacy)
```
gap_scanner_v3_03_attack_vectors.py
gap_scanner_v3_03_data_leak_improved.py
gap_scanner_v3_03_e2e_encryption.py
gap_scanner_v3_03_encryption_leak_improved.py
gap_scanner_v3_03_key_failure_improved.py
gap_scanner_v3_03_legacy_duplication_improved.py
gap_scanner_v3_03_military_hardening.py
```

### Phase 4 Scanners (Legacy)
```
gap_scanner_v3_04_architecture_rules.py
gap_scanner_v3_04_audit_logging_improved.py
gap_scanner_v3_04_bridge_interface_rules.py
gap_scanner_v3_04_cpp_doxygen_policy_rules.py
gap_scanner_v3_04_deprecated_apis.py
gap_scanner_v3_04_design_error_rules_improved.py
gap_scanner_v3_04_determinism_improved.py
gap_scanner_v3_04_distributed_consistency_improved.py
gap_scanner_v3_04_doc_freshness_rules.py
gap_scanner_v3_04_docs_markdown_rules.py
gap_scanner_v3_04_gpu_memory.py
gap_scanner_v3_04_llm_ai_safety.py
gap_scanner_v3_04_module_governance_rules.py
gap_scanner_v3_04_observability_improved.py
gap_scanner_v3_04_performance_patterns_improved.py
gap_scanner_v3_04_query_correctness.py
```

## Migration Guide

All legacy files have been consolidated and renamed to follow the modern convention:

### Old → New Naming Pattern

| Old Name | New Name | Status |
|----------|----------|--------|
| `gap_scanner_v3_01_*.py` | `gs3_step01_<category>_*.py` | ✅ Migrated |
| `gap_scanner_v3_02_*.py` | `gs3_step02_safety_*.py` | ✅ Migrated |
| `gap_scanner_v3_03_*.py` | `gs3_step03_security_*.py` | ✅ Migrated |
| `gap_scanner_v3_04_*.py` | `gs3_step04_<design\|quality>_*.py` | ✅ Migrated |

### Usage

These files should **NOT be imported or used**. Instead, use the modern CLI:

```bash
# DO THIS (modern):
python tools/gs3.py scan src include tests

# DON'T DO THIS (legacy):
python tools/gap_scanner_v3_01_classic_memory.py
```

## Why Archive?

1. **Naming inconsistency**: Old files mixed prefixes (gap_scanner_v3_ vs gs3_step)
2. **Unclear organization**: Categories mixed with numbers (01 vs 02 vs 03)
3. **No auto-discovery**: Manual import statements required
4. **Documentation scattered**: No central reference
5. **API inconsistency**: Not all inherited from BaseGapScanner

## Modern System

See [tools/GS3_CLI_GUIDE.md](../GS3_CLI_GUIDE.md) for complete documentation on the new system.

### Quick Reference

```bash
# List all 46 modern scanners
python tools/gs3.py list-scanners

# Run scan
python tools/gs3.py scan src include tests --scan-mode fast

# Generate report
python tools/gs3.py report results.json --format md

# Filter by phase
python tools/gs3.py list-scanners --step 1
```

## If You Need Legacy Files

1. **For reference**: Files are available in `tools/legacy/`
2. **For testing**: Import from legacy directory if needed
3. **For comparison**: Check git history for changes

```python
# Example: Import legacy scanner (if needed for testing)
from tools.legacy.gap_scanner_v3_01_classic_memory import ClassicMemoryScanner
```

## Removal Timeline

- **Date Archived**: 2026-06-21
- **Deprecation Period**: Until 2026-09-21 (3 months)
- **Planned Removal**: 2026-09-21 or after migration complete

During deprecation period, these files are maintained but not actively used.

---

**Status**: ✅ Archived  
**Modern System**: ✅ Production Ready  
**Migration**: ✅ 100% Complete  
**Removal**: Pending deprecation period
