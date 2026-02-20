# LoRA Adapter Manager Migration Guide

**Migrating from `LoRAAdapterManager` to `MultiLoRAManager`**

---

## Summary

`LoRAAdapterManager` (declared in `include/llm/lora_framework/lora_adapter_manager.h`) is **deprecated** as of ThemisDB v1.3.0. It will be removed in **v2.0.0**.

The replacement is `MultiLoRAManager` (declared in `include/llm/multi_lora_manager.h`), which provides:

- **vLLM-style multi-LoRA batching** — multiple adapters active per inference batch
- **GPU-aware placement** — spreads adapters across multiple GPUs
- **Adapter fusion** — merge several LoRAs at inference time
- **Lazy loading** — on-demand loading without blocking the main thread
- **Quantization support** (v1.4.0) — FP16 / INT8 / INT4 LoRA weights
- **Thread-safe API** — all public methods are safe to call concurrently

---

## Deprecation Timeline

| Version | Status |
|---------|--------|
| v1.3.0 | `LoRAAdapterManager` marked `@deprecated`; `MultiLoRAManager` fully functional |
| v1.4.0 | `LoRAAdapterManager` header emits `#pragma message` deprecation warning at compile time |
| v1.5.0 | `LoRAAdapterManager` moved to separate `lora_compat` CMake target (opt-in) |
| **v2.0.0** | `LoRAAdapterManager` **removed** from the main build; `lora_compat` target removed |

---

## Quick Migration Checklist

1. Replace `#include "llm/lora_framework/lora_adapter_manager.h"` with
   `#include "llm/multi_lora_manager.h"`
2. Change `LoRAAdapterManager` → `MultiLoRAManager` (or use the temporary
   `LoRAAdapterManager` typedef from `lora_adapter_manager_compat.h`)
3. Translate `Config` fields (see table below)
4. Translate method calls (see table below)
5. Remove calls to `switchAdapterWithFusion()` — the new API handles fusion
   automatically via `MultiLoRAManager::Config::enable_adapter_fusion`

---

## Config Migration

### Old (`LoRAAdapterManager::Config`)

```cpp
LoRAAdapterManager::Config old_config;
old_config.max_cache_size     = 5;     // max cached adapters
old_config.cache_ttl          = std::chrono::seconds{3600};
old_config.enable_auto_unload = true;
old_config.max_memory_mb      = 2048;  // MB
```

### New (`MultiLoRAManager::Config`)

```cpp
MultiLoRAManager::Config new_config;
new_config.max_lora_slots    = 16;    // max concurrent LoRAs (was max_cache_size)
new_config.lora_ttl          = std::chrono::seconds{1800};  // 30 min default
new_config.enable_lazy_load  = true;  // replaces enable_auto_unload semantics
new_config.max_lora_vram_mb  = 2048;  // VRAM limit in MB (was max_memory_mb)
// New in MultiLoRAManager — no equivalent in old API:
new_config.enable_multi_lora_batch = false;
new_config.enable_adapter_fusion   = false;
```

| Old field | New field | Notes |
|-----------|-----------|-------|
| `max_cache_size` | `max_lora_slots` | Renamed; semantics identical |
| `cache_ttl` | `lora_ttl` | Renamed; same type |
| `enable_auto_unload` | `enable_lazy_load` | Different semantics: `auto_unload=true` meant "evict unused adapters"; `lazy_load=true` means "load on first use" — both default to `true` |
| `max_memory_mb` | `max_lora_vram_mb` | Renamed; measures VRAM specifically |

---

## Method Migration

| Old method | New method | Notes |
|------------|------------|-------|
| `loadAdapter(id, path, base_model, scale)` | `loadLoRA(id, path, base_model, scale)` | Direct rename; same parameters |
| `unloadAdapter(id, force)` | `unloadLoRA(id, force)` | Direct rename |
| `switchAdapter(from_id, to_id)` | `getLoRA(to_id)` + `applyLoRA(to_id, ctx)` | No atomic switch — apply the new LoRA directly |
| `applyAdapter(id, ctx, scale)` | `applyLoRA(id, ctx)` | Scale is now set at load time; pass `scale` to `loadLoRA()` |
| `deactivateAdapter(ctx)` | `removeLoRA(id, ctx)` | Must specify the adapter ID explicitly |
| `switchAdapterWithFusion(ids, ctx)` | Set `config.enable_adapter_fusion = true` then call `applyLoRA()` for each | Fusion is handled automatically when enabled in Config |
| `getAdapterInfo(id)` | `getLoRAInfo(id)` | Returns `std::optional<LoRAInfo>` — same structure |
| *(no equivalent)* | `listLoRAs()` | Returns all loaded LoRAs — useful for health/admin APIs |
| *(no equivalent)* | `initializeLoRAWithModel(id, model)` | Required on multi-GPU setups before first apply |

---

## Code Examples

### Before

```cpp
#include "llm/lora_framework/lora_adapter_manager.h"

using namespace themis::llm::lora;

// Create
LoRAAdapterManager::Config config;
config.max_cache_size = 8;
config.max_memory_mb  = 4096;
auto manager = std::make_unique<LoRAAdapterManager>(config);

// Load
if (!manager->loadAdapter("sql-expert", "/models/sql-expert.bin", "llama-3b", 0.8f)) {
    spdlog::error("Failed to load adapter");
}

// Apply
manager->applyAdapter("sql-expert", ctx, /*scale=*/0.8f);

// Switch
manager->switchAdapter("sql-expert", "code-assistant");

// Unload
manager->unloadAdapter("sql-expert");
```

### After

```cpp
#include "llm/multi_lora_manager.h"

using namespace themis::llm;

// Create
MultiLoRAManager::Config config;
config.max_lora_slots   = 8;
config.max_lora_vram_mb = 4096;
auto manager = std::make_unique<MultiLoRAManager>(config);

// Load (scale passed at load time)
if (!manager->loadLoRA("sql-expert", "/models/sql-expert.bin", "llama-3b", /*scale=*/0.8f)) {
    spdlog::error("Failed to load LoRA");
}

// Apply (scale already set)
manager->applyLoRA("sql-expert", ctx);

// "Switch": remove the old adapter, then load and apply the new one
if (!manager->loadLoRA("code-assistant", "/models/code-assistant.bin", "llama-3b")) {
    spdlog::error("Failed to load LoRA");
}
manager->removeLoRA("sql-expert", ctx);
manager->applyLoRA("code-assistant", ctx);

// Unload
manager->unloadLoRA("sql-expert");
```

---

## Temporary Compatibility Shim

If you cannot migrate immediately, you can include the compatibility header which
typedef's `LoRAAdapterManager` to `MultiLoRAManager`:

```cpp
// Temporary: use compat shim (will be removed in v2.0.0)
#include "llm/lora_framework/lora_adapter_manager_compat.h"
using namespace themis::llm::lora;

LoRAAdapterManager::Config config;  // actually MultiLoRAManager::Config
auto manager = std::make_unique<LoRAAdapterManager>(config);  // actually MultiLoRAManager
```

> **Warning:** The `LoRAAdapterManager` typedef is a compile-time alias only.
> The old *method names* (`loadAdapter`, `switchAdapter`, etc.) are NOT forwarded.
> You still need to update the method calls even when using the compat shim.

---

## See Also

- `include/llm/multi_lora_manager.h` — full `MultiLoRAManager` API reference
- `include/llm/lora_framework/lora_adapter_manager_compat.h` — compatibility shim
- `docs/llm/MULTI_GPU_LORA_ADVANCED.md` — multi-GPU LoRA placement guide
- `docs/llm/DISTRIBUTED_LORA_TRAINING.md` — distributed LoRA training
- `docs/llm_roadmap.md` — Q4 cleanup section
