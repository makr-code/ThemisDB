# Compiler Warnings Report
Generated: /home/runner/work/ThemisDB/ThemisDB
---

## Source Code Analysis

### Static Cast Size T
Found **315** instances

**Top affected files:**
- `query/query_engine.cpp`: 82 issues
- `server/query_api_handler.cpp`: 38 issues
- `security/pki_key_provider.cpp`: 12 issues
- `content/content_manager.cpp`: 9 issues
- `llm/llama_wrapper.cpp`: 7 issues
- `llm/paged_kv_cache_manager.cpp`: 5 issues
- `server/vector_api_handler.cpp`: 5 issues
- `index/multi_gpu_vector_index.cpp`: 5 issues
- `acceleration/plugin_security.cpp`: 4 issues
- `storage/storage_engine.cpp`: 4 issues
- ... and 80 more files

### Float From Double
Found **359** instances

**Top affected files:**
- `index/vector_index.cpp`: 17 issues
- `llm/multi_lora_manager.cpp`: 15 issues
- `index/multi_vector_search.cpp`: 15 issues
- `llm/lora_framework/gpu_training_loop.cpp`: 13 issues
- `llm/distributed_training_coordinator.cpp`: 12 issues
- `llm/lora_framework/kernels/hip_fused_kernels.cpp`: 12 issues
- `llm/lora_framework/lora_training_service.cpp`: 11 issues
- `llm/kernel_fusion.cpp`: 10 issues
- `llm/llama_wrapper.cpp`: 10 issues
- `llm/lora_router.cpp`: 9 issues
- ... and 72 more files

### Pragma Suppressions
Found **9** instances

**Top affected files:**
- `utils/pki_client.cpp`: 2 issues
- `query/window_evaluator.cpp`: 2 issues
- `server/audit_api_handler.cpp`: 2 issues
- `query/cte_subquery.cpp`: 1 issues
- `auth/jwt_validator.cpp`: 1 issues
- `content/content_manager.cpp`: 1 issues

### Signed Unsigned Loops
Found **3** instances

**Top affected files:**
- `sharding/cross_shard_transaction.cpp`: 1 issues
- `replication/replication_manager.cpp`: 1 issues
- `llm/attention/kv_cache_manager.cpp`: 1 issues

## Recommendations

### Priority 1: Critical Type Conversions
- Migrate `static_cast<int>(size_t)` to `safe_size_to_int()`
- Fix `double → float` with `safe_double_to_float()`
- Review all pragma warning suppressions

### Priority 2: Code Quality
- Mark unused parameters with `[[maybe_unused]]`
- Remove genuinely unused local variables
- Use `size_t` for container iteration

### Priority 3: Best Practices
- Adopt range-based for loops where possible
- Document why parameters must remain unused
- Add CI checks to prevent regression

## Migration Guide

See `docs/de/guides/TYPE_CONVERSION_GUIDE.md` for detailed examples.

### Quick Reference

```cpp
// BEFORE (warnings)
int count = vector.size();              // C4267
float val = some_double;                // C4244
for (int i = 0; i < vec.size(); ++i)   // C4018

// AFTER (safe)
#include "utils/type_conversion.h"
using themis::utils::conversion::safe_size_to_int;
using themis::utils::conversion::safe_double_to_float;

int count = safe_size_to_int(vector.size());
float val = safe_double_to_float(some_double, true);
for (size_t i = 0; i < vec.size(); ++i)
```
