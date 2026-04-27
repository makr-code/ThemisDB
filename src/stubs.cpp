/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            stubs.cpp                                          ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:51:08                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   85.0/100                                       ║
    • Total Lines:     127                                            ║
    • Open Issues:     TODOs: 0, Stubs: 3                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 7c2cc11ffb  2026-04-14  refactor: replace (void)var; suppressions with C++17 [[ma... ║
    • ad6e8f172c  2026-04-14  refactor: replace (void)var; suppressions with C++17 [[ma... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// Stub implementations for linking purposes
// These stubs allow themis_tests to link successfully
// They are minimal implementations that prevent linker errors
//
// STUB/SIMULATION NOTE:
// Purpose: Provides minimal link-time stubs for `themis::llm::lora` types that
//   are referenced by the test executable but whose real implementations
//   (llm/lora_framework/*) require optional GPU/training dependencies.
// Activation: Included in the build by default via stubs.cpp being part of
//   themis_tests sources.  The real headers exist under include/llm/lora_framework/
//   and include/llm/feedback_store.h but are not unconditionally linkable.
// Production Delta: Struct layouts are minimal (no fields, no real logic).
//   `loadFromFile()` returns empty defaults instead of parsing YAML/JSON config.
//   `getFeedbackForAdapter()` always returns empty vector.
// Roadmap ref: src/ROADMAP.md § "Consolidation Phase — Stub/Simulation Lifecycle"
//              src/llm/FUTURE_ENHANCEMENTS.md § "LoRA Training Integration"
// Sync check (2026-04-27): Struct layouts and function signatures verified against
//   include/llm/lora_framework/{lora_feedback.h,lora_feedback_storage.h,
//   lora_training_config.h,feedback_plugin.h}.
//   `getFeedbackForAdapter` signature updated: unsigned __int64 → std::size_t
//   to match the real `FeedbackStorageService::getFeedbackForAdapter(const std::string&, size_t)`.
// Removal Plan: Replace stubs with proper conditional compilation (THEMIS_ENABLE_LORA_TRAINING)
//   and forward-declare only; or link real implementations once dependency
//   tree is resolved (Target: v1.5.0).

#include <memory>
#include <map>
#include <string>
#include <vector>
#include <cstddef>

namespace themis::llm::lora {

struct Feedback {
    // Stub
};

struct TrainingTriggerPlugin {
    virtual ~TrainingTriggerPlugin() = default;
};

struct CacheAwareWeightingPlugin {
    virtual ~CacheAwareWeightingPlugin() = default;
    virtual void process(Feedback&) {}
};

struct LoRATrainingConfig {
    static LoRATrainingConfig loadFromFile(const std::string&) {
        return {};
    }
    
    std::shared_ptr<TrainingTriggerPlugin> createTrainingTriggerPlugin(const std::string&) const {
        return std::make_shared<TrainingTriggerPlugin>();
    }
    
    std::shared_ptr<CacheAwareWeightingPlugin> createCacheWeightingPlugin(const std::string&) const {
        return std::make_shared<CacheAwareWeightingPlugin>();
    }
};

struct FeedbackStorageService {
    std::vector<Feedback> getFeedbackForAdapter(const std::string&, std::size_t) const {
        return {};
    }
};

} // namespace themis::llm::lora


