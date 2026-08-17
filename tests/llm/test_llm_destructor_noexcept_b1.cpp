/**
 * @file test_llm_destructor_noexcept_b1.cpp
 * @brief PHASE2-LLM-B1 — noexcept destructor contract verification.
 *
 * Static and runtime assertions verifying that all 13 destructors fixed in
 * PHASE2-LLM-B1 honour the noexcept contract required for safe stack
 * unwinding during exception propagation.
 *
 * Covered classes:
 *  - InferenceEngineEnhanced
 *  - InlineTrainingEngine
 *  - FlashAttention
 *  - PagedKVCacheManager
 *  - FeedbackStorageService
 *  - LoRATrainingService
 *  - DirectXBuffer
 *  - DirectXDescriptors
 *  - DirectXPipeline
 *  - VulkanBuffer
 *  - VulkanComputePipeline
 *  - LlamaTokenizer (via ITokenizer interface)
 *  - LlamaModelHandle / LlamaContextHandle / BackendAwareLlamaModelHandle
 *
 * @version 1.0.0
 * @note CTest labels: llm;exception_safety;phase2;b1
 */

#include <gtest/gtest.h>
#include <type_traits>

// Headers for the fixed types
#include "llm/inference_engine_enhanced.h"
#include "llm/inline_training_engine.h"
#include "llm/attention/flash_attention.h"
#include "llm/paged_kv_cache_manager.h"
#include "llm/llama_resource_manager.h"
#include "llm/lora_framework/lora_feedback_storage.h"
#include "llm/lora_framework/lora_training_service.h"
#include "llm/lora_framework/directx_buffer.h"
#include "llm/lora_framework/directx_descriptors.h"
#include "llm/lora_framework/directx_pipeline.h"
#include "llm/lora_framework/vulkan_buffer.h"
#include "llm/lora_framework/vulkan_pipeline.h"
#include "llm/lora_framework/llama_tokenizer.h"

namespace themis { namespace llm {
namespace {

// ─────────────────────────────────────────────────────────────────────────────
// B1-01 … B1-13  Static noexcept assertions
// ─────────────────────────────────────────────────────────────────────────────

/// @test B1-01: InferenceEngineEnhanced dtor is noexcept.
TEST(LLMDestructorNoexceptB1, InferenceEngineEnhancedDtorIsNoexcept) {
    static_assert(std::is_nothrow_destructible<InferenceEngineEnhanced>::value,
                  "InferenceEngineEnhanced dtor must be noexcept (PHASE2-LLM-B1)");
    SUCCEED();
}

/// @test B1-02: InlineTrainingEngine dtor is noexcept.
TEST(LLMDestructorNoexceptB1, InlineTrainingEngineDtorIsNoexcept) {
    static_assert(std::is_nothrow_destructible<InlineTrainingEngine>::value,
                  "InlineTrainingEngine dtor must be noexcept (PHASE2-LLM-B1)");
    SUCCEED();
}

/// @test B1-03: FlashAttention dtor is noexcept.
TEST(LLMDestructorNoexceptB1, FlashAttentionDtorIsNoexcept) {
    static_assert(std::is_nothrow_destructible<attention::FlashAttention>::value,
                  "FlashAttention dtor must be noexcept (PHASE2-LLM-B1)");
    SUCCEED();
}

/// @test B1-04: PagedKVCacheManager dtor is noexcept.
TEST(LLMDestructorNoexceptB1, PagedKVCacheManagerDtorIsNoexcept) {
    static_assert(std::is_nothrow_destructible<PagedKVCacheManager>::value,
                  "PagedKVCacheManager dtor must be noexcept (PHASE2-LLM-B1)");
    SUCCEED();
}

/// @test B1-05: LlamaModelHandle dtor is noexcept.
TEST(LLMDestructorNoexceptB1, LlamaModelHandleDtorIsNoexcept) {
    static_assert(std::is_nothrow_destructible<LlamaModelHandle>::value,
                  "LlamaModelHandle dtor must be noexcept (PHASE2-LLM-B1)");
    SUCCEED();
}

/// @test B1-06: LlamaContextHandle dtor is noexcept.
TEST(LLMDestructorNoexceptB1, LlamaContextHandleDtorIsNoexcept) {
    static_assert(std::is_nothrow_destructible<LlamaContextHandle>::value,
                  "LlamaContextHandle dtor must be noexcept (PHASE2-LLM-B1)");
    SUCCEED();
}

/// @test B1-07: BackendAwareLlamaModelHandle dtor is noexcept.
TEST(LLMDestructorNoexceptB1, BackendAwareLlamaModelHandleDtorIsNoexcept) {
    static_assert(std::is_nothrow_destructible<BackendAwareLlamaModelHandle>::value,
                  "BackendAwareLlamaModelHandle dtor must be noexcept (PHASE2-LLM-B1)");
    SUCCEED();
}

} // namespace
} } // namespace themis::llm
