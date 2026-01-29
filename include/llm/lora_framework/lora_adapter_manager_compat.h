#pragma once

/**
 * @file lora_adapter_manager_compat.h
 * @brief Compatibility layer - LoRAAdapterManager is now deprecated
 * 
 * Use MultiLoRAManager instead! This file provides a deprecation wrapper.
 * 
 * Migration path:
 * - OLD: LoRAAdapterManager::Config config; manager = std::make_unique<LoRAAdapterManager>(config);
 * - NEW: MultiLoRAManager::Config config; manager = std::make_unique<MultiLoRAManager>(config);
 * 
 * API mapping:
 * - loadAdapter() → loadLoRA()
 * - unloadAdapter() → unloadLoRA()
 * - switchAdapter() → getLoRA() + manual apply
 * - applyAdapter() → applyLoRA() (now actually works!)
 * - deactivateAdapter() → removeLoRA()
 */

#include "llm/multi_lora_manager.h"
#include <memory>
#include <string>
#include <vector>
#include <optional>

namespace themis {
namespace llm {
namespace lora {

/**
 * @deprecated Use MultiLoRAManager instead
 * 
 * This is a compatibility typedef for easier migration.
 * Simply replace LoRAAdapterManager with MultiLoRAManager in your code.
 */
using LoRAAdapterManager = MultiLoRAManager;

/**
 * Helper class to convert old LoRAAdapterManager::Config to MultiLoRAManager::Config
 * 
 * Usage:
 *   LoRAAdapterManager::Config old_config = {...};
 *   auto multi_config = ConfigConverter::convert(old_config);
 *   auto manager = std::make_unique<MultiLoRAManager>(multi_config);
 */
struct ConfigConverter {
    static MultiLoRAManager::Config convert(const MultiLoRAManager::Config& config) {
        return config;  // Already compatible
    }
};

}  // namespace lora
}  // namespace llm
}  // namespace themis
