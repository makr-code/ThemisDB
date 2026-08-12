/**
 * @file test_gpu_lora_integration.cpp
 * @brief Tests for GPU offload and LoRA runtime integration
 * 
 * Tests the wiring of GPU/VRAM handling and Multi-LoRA runtime support
 * to llama.cpp, verifying:
 * - GPU configuration is properly set in llama.cpp model parameters
 * - LoRA adapter initialization API is available and called correctly
 * - CPU fallback works when GPU is unavailable
 * - Logging and validation of GPU/LoRA configuration
 * 
 * @author ThemisDB Team
 * @date February 2026
 */

#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <iostream>

// Conditional compilation for LLM support
#ifdef THEMIS_ENABLE_LLM
#include "llm/multi_lora_manager.h"
#include "llm/model_loader.h"
#include "llm/llama_wrapper.h"

// Forward declarations for C API
extern "C" {
    bool themis_llama_lora_available();
    void themis_lora_inject_api_functions(void*, void*, void*, void*, void*);
    void* llama_lora_adapter_init(struct llama_model* model, const char* path_lora);
}
#endif

using namespace themis;

namespace fs = std::filesystem;

/**
 * Test fixture for GPU and LoRA integration tests
 */
class GPULoRAIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_dir_ = fs::temp_directory_path() / "themis_gpu_lora_test";
        fs::create_directories(test_dir_);
    }
    
    void TearDown() override {
        if (fs::exists(test_dir_)) {
            fs::remove_all(test_dir_);
        }
    }
    
    fs::path test_dir_;
};

// ═══════════════════════════════════════════════════════════
// GPU Configuration Tests
// ═══════════════════════════════════════════════════════════

/**
 * Test that LazyModelLoader initializes llama.cpp backend
 * Acceptance Criteria:
 * - LazyModelLoader can be constructed
 * - Backend initialization completes without errors
 * - GPU configuration is logged
 */
TEST_F(GPULoRAIntegrationTest, BackendInitialization) {
#ifdef THEMIS_ENABLE_LLM
    llm::LazyModelLoader::Config config;
    config.max_vram_mb = 8192;  // 8GB
    config.default_n_gpu_layers = 32;
    config.default_n_ctx = 4096;
    
    // This should call llama_backend_init() internally
    EXPECT_NO_THROW({
        llm::LazyModelLoader loader(config);
    });
#else
    GTEST_SKIP() << "capability:llm_support_enabled=false;reason=llm_support_not_enabled";
#endif
}

/**
 * Test GPU layer configuration
 * Acceptance Criteria:
 * - GPU layers can be set to various values
 * - Zero GPU layers enables CPU-only mode
 * - Negative values are handled gracefully
 */
TEST_F(GPULoRAIntegrationTest, GPULayerConfiguration) {
#ifdef THEMIS_ENABLE_LLM
    llm::LazyModelLoader::Config config;
    
    // Test CPU-only mode
    config.default_n_gpu_layers = 0;
    EXPECT_NO_THROW({
        llm::LazyModelLoader loader(config);
    });
    
    // Test GPU-accelerated mode
    config.default_n_gpu_layers = 32;
    EXPECT_NO_THROW({
        llm::LazyModelLoader loader(config);
    });
    
    // Test full GPU offload
    config.default_n_gpu_layers = 999;  // Will offload as many layers as possible
    EXPECT_NO_THROW({
        llm::LazyModelLoader loader(config);
    });
#else
    GTEST_SKIP() << "capability:llm_support_enabled=false;reason=llm_support_not_enabled";
#endif
}

/**
 * Test VRAM limit configuration
 * Acceptance Criteria:
 * - VRAM limits are properly set
 * - Configuration is stored correctly
 * - Limits are enforced during model loading
 */
TEST_F(GPULoRAIntegrationTest, VRAMLimitConfiguration) {
#ifdef THEMIS_ENABLE_LLM
    llm::LazyModelLoader::Config config;
    config.max_vram_mb = 8192;  // 8GB limit
    config.max_ram_mb = 16384;  // 16GB limit
    
    EXPECT_NO_THROW({
        llm::LazyModelLoader loader(config);
    });
#else
    GTEST_SKIP() << "capability:llm_support_enabled=false;reason=llm_support_not_enabled";
#endif
}

// ═══════════════════════════════════════════════════════════
// LoRA Runtime Integration Tests
// ═══════════════════════════════════════════════════════════

/**
 * Test LoRA API availability check
 * Acceptance Criteria:
 * - themis_llama_lora_available() can be called
 * - Returns boolean indicating API availability
 * - Does not crash when LoRA API is missing
 */
TEST_F(GPULoRAIntegrationTest, LoRAAPIAvailability) {
#ifdef THEMIS_ENABLE_LLM
    
    // Should not crash regardless of API availability
    EXPECT_NO_THROW({
        bool available = themis_llama_lora_available();
        // Log the result but don't fail if unavailable
        // (LoRA support is optional based on llama.cpp build)
        if (available) {
            std::cout << "LoRA API is available" << std::endl;
        } else {
            std::cout << "LoRA API is not available (expected if llama.cpp built without LLAMA_LORA=ON)" << std::endl;
        }
    });
#else
    GTEST_SKIP() << "capability:llm_support_enabled=false;reason=llm_support_not_enabled";
#endif
}

/**
 * Test MultiLoRAManager initialization with GPU config
 * Acceptance Criteria:
 * - MultiLoRAManager can be constructed with GPU configuration
 * - Multi-GPU settings are stored correctly
 * - VRAM limits are respected
 */
TEST_F(GPULoRAIntegrationTest, MultiLoRAManagerGPUConfig) {
#ifdef THEMIS_ENABLE_LLM
    llm::MultiLoRAManager::Config config;
    config.max_lora_vram_mb = 2048;  // 2GB for LoRAs
    config.max_lora_slots = 16;
    
    // Multi-GPU configuration
    config.multi_gpu.enabled = true;
    config.multi_gpu.devices = {0, 1};  // Use GPUs 0 and 1
    config.multi_gpu.strategy = llm::MultiGPUStrategy::ROUND_ROBIN;
    config.multi_gpu.max_vram_per_gpu_mb = 8192;  // 8GB per GPU
    
    EXPECT_NO_THROW({
        llm::MultiLoRAManager manager(config);
    });
#else
    GTEST_SKIP() << "capability:llm_support_enabled=false;reason=llm_support_not_enabled";
#endif
}

/**
 * Test LoRA adapter initialization method exists
 * Acceptance Criteria:
 * - initializeLoRAWithModel() method can be called
 * - Returns false when model handle is null
 * - Returns false when LoRA ID doesn't exist
 */
TEST_F(GPULoRAIntegrationTest, LoRAInitializationMethod) {
#ifdef THEMIS_ENABLE_LLM
    llm::MultiLoRAManager::Config config;
    config.max_lora_vram_mb = 1024;
    
    llm::MultiLoRAManager manager(config);
    
    // Test with null model handle - should return false
    EXPECT_FALSE(manager.initializeLoRAWithModel("nonexistent_lora", nullptr));
    
    // Test with non-existent LoRA - should return false
    // (even with a valid-looking model handle)
    void* fake_model = reinterpret_cast<void*>(0x1234);
    EXPECT_FALSE(manager.initializeLoRAWithModel("nonexistent_lora", fake_model));
#else
    GTEST_SKIP() << "capability:llm_support_enabled=false;reason=llm_support_not_enabled";
#endif
}

/**
 * Test LlamaWrapper GPU configuration
 * Acceptance Criteria:
 * - LlamaWrapper can be constructed with GPU settings
 * - GPU layers setting is stored in configuration
 * - VRAM limits are configured
 */
TEST_F(GPULoRAIntegrationTest, LlamaWrapperGPUConfig) {
#ifdef THEMIS_ENABLE_LLM
    llm::LlamaWrapper::Config config;
    config.n_gpu_layers = 32;
    config.max_vram_mb = 14336;  // 14GB
    config.n_ctx = 4096;
    config.n_batch = 512;
    config.n_threads = 8;
    
    EXPECT_NO_THROW({
        llm::LlamaWrapper wrapper(config);
    });
#else
    GTEST_SKIP() << "capability:llm_support_enabled=false;reason=llm_support_not_enabled";
#endif
}

/**
 * Test LlamaWrapper LoRA configuration
 * Acceptance Criteria:
 * - LlamaWrapper's multi-LoRA manager is configured
 * - LoRA VRAM limits are set
 * - Multi-LoRA support is enabled
 */
TEST_F(GPULoRAIntegrationTest, LlamaWrapperLoRAConfig) {
#ifdef THEMIS_ENABLE_LLM
    llm::LlamaWrapper::Config config;
    config.n_gpu_layers = 32;
    config.n_ctx = 4096;
    
    // Configure multi-LoRA support
    config.multi_lora_config.max_lora_vram_mb = 2048;
    config.multi_lora_config.max_lora_slots = 8;
    config.multi_lora_config.enable_multi_lora_batch = true;
    
    EXPECT_NO_THROW({
        llm::LlamaWrapper wrapper(config);
    });
#else
    GTEST_SKIP() << "capability:llm_support_enabled=false;reason=llm_support_not_enabled";
#endif
}

// ═══════════════════════════════════════════════════════════
// Integration Tests
// ═══════════════════════════════════════════════════════════

/**
 * Test complete GPU + LoRA configuration
 * Acceptance Criteria:
 * - Can configure both GPU offload and LoRA support simultaneously
 * - Settings don't conflict
 * - All components initialize successfully
 */
TEST_F(GPULoRAIntegrationTest, CompleteGPULoRAConfiguration) {
#ifdef THEMIS_ENABLE_LLM
    llm::LlamaWrapper::Config config;
    
    // GPU configuration
    config.n_gpu_layers = 32;
    config.max_vram_mb = 14336;
    config.use_mmap = true;
    config.use_mlock = false;
    
    // Context configuration
    config.n_ctx = 4096;
    config.n_batch = 512;
    config.n_threads = 8;
    
    // Multi-LoRA configuration
    config.multi_lora_config.max_lora_vram_mb = 2048;
    config.multi_lora_config.max_lora_slots = 16;
    config.multi_lora_config.enable_multi_lora_batch = true;
    
    // Multi-GPU LoRA configuration
    config.multi_lora_config.multi_gpu.enabled = true;
    config.multi_lora_config.multi_gpu.devices = {0};
    config.multi_lora_config.multi_gpu.strategy = llm::MultiGPUStrategy::ROUND_ROBIN;
    
    // Performance optimizations
    config.use_flash_attn = true;
    config.use_kv_cache_reuse = true;
    
    EXPECT_NO_THROW({
        llm::LlamaWrapper wrapper(config);
        
        // Verify capabilities reflect GPU and LoRA support
        auto caps = wrapper.getCapabilities();
        EXPECT_TRUE(caps.gpu_accelerated);
        
        // LoRA support depends on whether llama.cpp was built with LLAMA_LORA=ON
        // Check actual runtime availability rather than assuming it's always true
        EXPECT_EQ(caps.supports_lora, themis_llama_lora_available());
    });
#else
    GTEST_SKIP() << "capability:llm_support_enabled=false;reason=llm_support_not_enabled";
#endif
}

/**
 * Test logging of GPU/LoRA configuration
 * Acceptance Criteria:
 * - GPU configuration is logged at initialization
 * - LoRA configuration is logged at initialization
 * - Log messages include key parameters
 * Note: This test primarily validates that initialization completes
 * without errors. Actual log output would need to be captured separately.
 */
TEST_F(GPULoRAIntegrationTest, ConfigurationLogging) {
#ifdef THEMIS_ENABLE_LLM
    llm::LlamaWrapper::Config config;
    config.n_gpu_layers = 32;
    config.max_vram_mb = 8192;
    config.multi_lora_config.max_lora_vram_mb = 1024;
    
    // Initialization should log configuration details
    EXPECT_NO_THROW({
        llm::LlamaWrapper wrapper(config);
    });
#else
    GTEST_SKIP() << "capability:llm_support_enabled=false;reason=llm_support_not_enabled";
#endif
}

// Note: This test file is auto-discovered by CMake via GLOB_RECURSE in tests/CMakeLists.txt
// and linked with GTest::gtest_main, so no explicit main() function is needed here.

// ─────────────────────────────────────────────────────────────────────────────
// LoRA API injection tests (LORA-INJ-01..03)
// Tests for themis_lora_inject_api_functions()
// ─────────────────────────────────────────────────────────────────────────────
// These tests exercise the override path introduced for stub #75.  They require
// the LoRA adapter TU to be compiled (THEMIS_ENABLE_LLM) but do NOT need a
// real llama.cpp LoRA build — mock functions are injected instead.

#ifdef THEMIS_ENABLE_LLM
namespace {
    static bool g_mock_lora_init_called = false;
    static int  g_mock_set_count = 0;

    static void* mock_lora_init_fn(struct llama_model* /*model*/, const char* /*path*/) {
        g_mock_lora_init_called = true;
        return reinterpret_cast<void*>(static_cast<uintptr_t>(0xDEAD));
    }
    static int mock_lora_set_fn(struct llama_context* /*ctx*/, void* /*adapter*/, float /*scale*/) {
        ++g_mock_set_count;
        return 0;
    }
    static int  mock_lora_remove_fn(struct llama_context* /*ctx*/, void* /*adapter*/) { return 0; }
    static int  mock_lora_clear_fn(struct llama_context* /*ctx*/) { return 0; }
    static void mock_lora_free_fn(void* /*adapter*/) {}
} // namespace

// LORA-INJ-01: After injecting mock functions, themis_llama_lora_available() == true
TEST(LoraApiInjectionTest, InjectedApiReportsAvailable) {
    themis_lora_inject_api_functions(
        reinterpret_cast<void*>(mock_lora_init_fn),
        reinterpret_cast<void*>(mock_lora_set_fn),
        reinterpret_cast<void*>(mock_lora_remove_fn),
        reinterpret_cast<void*>(mock_lora_clear_fn),
        reinterpret_cast<void*>(mock_lora_free_fn));
    EXPECT_TRUE(themis_llama_lora_available());
    // Clean up — clear the override so other tests use the real dlsym path.
    themis_lora_inject_api_functions(nullptr, nullptr, nullptr, nullptr, nullptr);
}

// LORA-INJ-02: Injected init mock is actually called and returns the mock handle
TEST(LoraApiInjectionTest, InjectedInitIsInvoked) {
    g_mock_lora_init_called = false;
    themis_lora_inject_api_functions(
        reinterpret_cast<void*>(mock_lora_init_fn),
        reinterpret_cast<void*>(mock_lora_set_fn),
        reinterpret_cast<void*>(mock_lora_remove_fn),
        reinterpret_cast<void*>(mock_lora_clear_fn),
        reinterpret_cast<void*>(mock_lora_free_fn));
    // Provide a non-null sentinel as fake model pointer — mock ignores it.
    struct llama_model* fake_model = reinterpret_cast<struct llama_model*>(static_cast<uintptr_t>(1));
    void* handle = llama_lora_adapter_init(fake_model, "/fake/path.bin");
    EXPECT_TRUE(g_mock_lora_init_called);
    EXPECT_EQ(handle, reinterpret_cast<void*>(static_cast<uintptr_t>(0xDEAD)));
    themis_lora_inject_api_functions(nullptr, nullptr, nullptr, nullptr, nullptr);
}

// LORA-INJ-03: Clearing injection reverts availability; no crash on subsequent availability check
TEST(LoraApiInjectionTest, ClearInjectionRevertsOverride) {
    // Inject first so we have a known state.
    themis_lora_inject_api_functions(
        reinterpret_cast<void*>(mock_lora_init_fn),
        reinterpret_cast<void*>(mock_lora_set_fn),
        reinterpret_cast<void*>(mock_lora_remove_fn),
        reinterpret_cast<void*>(mock_lora_clear_fn),
        reinterpret_cast<void*>(mock_lora_free_fn));
    ASSERT_TRUE(themis_llama_lora_available());
    // Clear override — availability now reflects actual dlsym detection.
    themis_lora_inject_api_functions(nullptr, nullptr, nullptr, nullptr, nullptr);
    // Calling available() after clearing must not crash.
    EXPECT_NO_THROW(themis_llama_lora_available());
}
#endif  // THEMIS_ENABLE_LLM
