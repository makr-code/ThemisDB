/**
 * @file test_error_registry.cpp
 * @brief Tests for the Error Registry system
 */

#include <gtest/gtest.h>
#include "utils/error_registry.h"
#include <atomic>
#include <thread>
#include <vector>

using namespace themis::errors;

class ErrorRegistryTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Registry is initialized with default errors
    }
};

// ============================================================================
// Error Registry Tests
// ============================================================================

TEST_F(ErrorRegistryTest, GetErrorByCode) {
    auto& registry = ErrorRegistry::getInstance();
    
    // Test getting a known error
    auto metadata = registry.getError(ErrorCode::ERR_LLM_MODEL_NOT_FOUND);
    
    EXPECT_EQ(metadata.code, ErrorCode::ERR_LLM_MODEL_NOT_FOUND);
    EXPECT_EQ(metadata.category, "LLM");
    EXPECT_EQ(metadata.severity, "Error");
    EXPECT_FALSE(metadata.message_template.empty());
    EXPECT_FALSE(metadata.cause.empty());
    EXPECT_FALSE(metadata.solution.empty());
}

TEST_F(ErrorRegistryTest, GetUnknownError) {
    auto& registry = ErrorRegistry::getInstance();
    
    // Test getting an unknown error code (using 9998 which is not in the defined range)
    auto metadata = registry.getError(static_cast<ErrorCode>(9998));
    
    EXPECT_EQ(metadata.code, ErrorCode::ERR_UNKNOWN);
    EXPECT_EQ(metadata.category, "Unknown");
}

TEST_F(ErrorRegistryTest, GetErrorsByCategory) {
    auto& registry = ErrorRegistry::getInstance();
    
    // Test getting errors by category
    auto llm_errors = registry.getErrorsByCategory("LLM");
    
    EXPECT_GT(llm_errors.size(), 0);
    
    // Verify all returned errors are in the LLM category
    for (const auto& error : llm_errors) {
        EXPECT_EQ(error.category, "LLM");
    }
}

TEST_F(ErrorRegistryTest, SearchErrors) {
    auto& registry = ErrorRegistry::getInstance();
    
    // Test searching for GPU-related errors
    auto gpu_errors = registry.searchErrors("gpu");
    
    EXPECT_GT(gpu_errors.size(), 0);
    
    // Verify results contain GPU keyword
    bool found_gpu = false;
    for (const auto& error : gpu_errors) {
        for (const auto& keyword : error.keywords) {
            if (keyword.find("gpu") != std::string::npos) {
                found_gpu = true;
                break;
            }
        }
        if (found_gpu) break;
    }
    EXPECT_TRUE(found_gpu);
}

TEST_F(ErrorRegistryTest, SearchErrorsLoRA) {
    auto& registry = ErrorRegistry::getInstance();
    
    // Test searching for LoRA-related errors
    auto lora_errors = registry.searchErrors("lora");
    
    EXPECT_GT(lora_errors.size(), 0);
    
    // Verify all results are in LoRA category
    for (const auto& error : lora_errors) {
        EXPECT_EQ(error.category, "LoRA");
    }
}

TEST_F(ErrorRegistryTest, GetAllCategories) {
    auto& registry = ErrorRegistry::getInstance();
    
    auto categories = registry.getAllCategories();
    
    EXPECT_GT(categories.size(), 0);
    
    // Verify expected categories exist
    std::vector<std::string> expected = {"Storage", "LLM", "LoRA", "MCP", "Schema", "Network"};
    for (const auto& expected_cat : expected) {
        bool found = false;
        for (const auto& cat : categories) {
            if (cat == expected_cat) {
                found = true;
                break;
            }
        }
        EXPECT_TRUE(found) << "Expected category not found: " << expected_cat;
    }
}

TEST_F(ErrorRegistryTest, ToJSON) {
    auto& registry = ErrorRegistry::getInstance();
    
    auto json_data = registry.toJSON();
    
    EXPECT_TRUE(json_data.contains("total_errors"));
    EXPECT_TRUE(json_data.contains("categories"));
    EXPECT_TRUE(json_data.contains("errors"));
    
    EXPECT_GT(json_data["total_errors"].get<int>(), 0);
    EXPECT_GT(json_data["categories"].size(), 0);
    EXPECT_GT(json_data["errors"].size(), 0);
}

TEST_F(ErrorRegistryTest, ErrorMetadataToJSON) {
    auto& registry = ErrorRegistry::getInstance();
    
    auto metadata = registry.getError(ErrorCode::ERR_LLM_GPU_OOM);
    auto json_data = metadata.toJSON();
    
    EXPECT_TRUE(json_data.contains("code"));
    EXPECT_TRUE(json_data.contains("category"));
    EXPECT_TRUE(json_data.contains("severity"));
    EXPECT_TRUE(json_data.contains("message_template"));
    EXPECT_TRUE(json_data.contains("cause"));
    EXPECT_TRUE(json_data.contains("solution"));
    EXPECT_TRUE(json_data.contains("related_docs"));
    EXPECT_TRUE(json_data.contains("keywords"));
    
    EXPECT_EQ(json_data["code"].get<int>(), 2004);
    EXPECT_EQ(json_data["category"].get<std::string>(), "LLM");
    EXPECT_EQ(json_data["severity"].get<std::string>(), "Critical");
}

TEST_F(ErrorRegistryTest, StorageErrors) {
    auto& registry = ErrorRegistry::getInstance();
    
    // Test storage-related errors
    auto storage_errors = registry.getErrorsByCategory("Storage");
    
    EXPECT_GT(storage_errors.size(), 0);
    
    // Verify expected storage errors exist
    std::vector<ErrorCode> expected_codes = {
        ErrorCode::ERR_STORAGE_FILE_NOT_FOUND,
        ErrorCode::ERR_STORAGE_DISK_FULL,
        ErrorCode::ERR_STORAGE_PERMISSION_DENIED,
        ErrorCode::ERR_STORAGE_CORRUPTION
    };
    
    for (const auto& expected_code : expected_codes) {
        bool found = false;
        for (const auto& error : storage_errors) {
            if (error.code == expected_code) {
                found = true;
                break;
            }
        }
        EXPECT_TRUE(found) << "Expected storage error not found: " << static_cast<int>(expected_code);
    }
}

TEST_F(ErrorRegistryTest, MCPErrors) {
    auto& registry = ErrorRegistry::getInstance();
    
    // Test MCP-related errors
    auto mcp_errors = registry.getErrorsByCategory("MCP");
    
    EXPECT_GT(mcp_errors.size(), 0);
    
    // Verify MCP schema unavailable error exists
    auto schema_error = registry.getError(ErrorCode::ERR_MCP_SCHEMA_UNAVAILABLE);
    EXPECT_EQ(schema_error.code, ErrorCode::ERR_MCP_SCHEMA_UNAVAILABLE);
    EXPECT_EQ(schema_error.category, "MCP");
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST_F(ErrorRegistryTest, SearchMultipleKeywords) {
    auto& registry = ErrorRegistry::getInstance();
    
    // Test that searching works with different keywords
    auto oom_errors = registry.searchErrors("out of memory");
    auto vram_errors = registry.searchErrors("vram");
    
    // Both should find GPU OOM error
    EXPECT_GT(oom_errors.size(), 0);
    EXPECT_GT(vram_errors.size(), 0);
}

TEST_F(ErrorRegistryTest, ErrorCodeRanges) {
    auto& registry = ErrorRegistry::getInstance();
    
    // Verify error codes are in expected ranges
    auto storage_errors = registry.getErrorsByCategory("Storage");
    for (const auto& error : storage_errors) {
        int code = static_cast<int>(error.code);
        EXPECT_GE(code, 1000);
        EXPECT_LT(code, 2000);
    }
    
    auto llm_errors = registry.getErrorsByCategory("LLM");
    for (const auto& error : llm_errors) {
        int code = static_cast<int>(error.code);
        EXPECT_GE(code, 2000);
        EXPECT_LT(code, 2100);
    }
    
    auto lora_errors = registry.getErrorsByCategory("LoRA");
    for (const auto& error : lora_errors) {
        int code = static_cast<int>(error.code);
        EXPECT_GE(code, 2100);
        EXPECT_LT(code, 2200);
    }
}

// ============================================================================
// New Error Code Tests
// ============================================================================

TEST_F(ErrorRegistryTest, NewLLMErrorCodes) {
    auto& registry = ErrorRegistry::getInstance();
    
    // Test ERR_LLM_INVALID_HANDLE
    auto invalid_handle = registry.getError(ErrorCode::ERR_LLM_INVALID_HANDLE);
    EXPECT_EQ(invalid_handle.code, ErrorCode::ERR_LLM_INVALID_HANDLE);
    EXPECT_EQ(invalid_handle.category, "LLM");
    EXPECT_EQ(invalid_handle.severity, "Error");
    EXPECT_FALSE(invalid_handle.message_template.empty());
    
    // Test ERR_LLM_VISION_INFERENCE_FAILED
    auto vision_failed = registry.getError(ErrorCode::ERR_LLM_VISION_INFERENCE_FAILED);
    EXPECT_EQ(vision_failed.code, ErrorCode::ERR_LLM_VISION_INFERENCE_FAILED);
    EXPECT_EQ(vision_failed.category, "LLM");
    EXPECT_EQ(vision_failed.severity, "Error");
    
    // Test ERR_LLM_DRAFT_MODEL_LOAD_FAILED
    auto draft_failed = registry.getError(ErrorCode::ERR_LLM_DRAFT_MODEL_LOAD_FAILED);
    EXPECT_EQ(draft_failed.code, ErrorCode::ERR_LLM_DRAFT_MODEL_LOAD_FAILED);
    EXPECT_EQ(draft_failed.category, "LLM");
    EXPECT_EQ(draft_failed.severity, "Error");
    
    // Test ERR_LLM_RAM_OOM
    auto ram_oom = registry.getError(ErrorCode::ERR_LLM_RAM_OOM);
    EXPECT_EQ(ram_oom.code, ErrorCode::ERR_LLM_RAM_OOM);
    EXPECT_EQ(ram_oom.category, "LLM");
    EXPECT_EQ(ram_oom.severity, "Critical");
    
    // Test ERR_LLM_GPU_NOT_AVAILABLE
    auto gpu_not_available = registry.getError(ErrorCode::ERR_LLM_GPU_NOT_AVAILABLE);
    EXPECT_EQ(gpu_not_available.code, ErrorCode::ERR_LLM_GPU_NOT_AVAILABLE);
    EXPECT_EQ(gpu_not_available.category, "LLM");
    EXPECT_EQ(gpu_not_available.severity, "Error");
    
    // Test ERR_LLM_GPU_ALLOC_FAILED
    auto gpu_alloc_failed = registry.getError(ErrorCode::ERR_LLM_GPU_ALLOC_FAILED);
    EXPECT_EQ(gpu_alloc_failed.code, ErrorCode::ERR_LLM_GPU_ALLOC_FAILED);
    EXPECT_EQ(gpu_alloc_failed.category, "LLM");
    EXPECT_EQ(gpu_alloc_failed.severity, "Critical");
    
    // Test ERR_LLM_GPU_PEER_ACCESS_FAILED
    auto peer_access_failed = registry.getError(ErrorCode::ERR_LLM_GPU_PEER_ACCESS_FAILED);
    EXPECT_EQ(peer_access_failed.code, ErrorCode::ERR_LLM_GPU_PEER_ACCESS_FAILED);
    EXPECT_EQ(peer_access_failed.category, "LLM");
    EXPECT_EQ(peer_access_failed.severity, "Warning");
}

TEST_F(ErrorRegistryTest, NewLoRAErrorCodes) {
    auto& registry = ErrorRegistry::getInstance();
    
    // Test ERR_LORA_MODEL_MISMATCH
    auto model_mismatch = registry.getError(ErrorCode::ERR_LORA_MODEL_MISMATCH);
    EXPECT_EQ(model_mismatch.code, ErrorCode::ERR_LORA_MODEL_MISMATCH);
    EXPECT_EQ(model_mismatch.category, "LoRA");
    EXPECT_EQ(model_mismatch.severity, "Error");
    EXPECT_FALSE(model_mismatch.message_template.empty());
    
    // Test ERR_LORA_GPU_LOAD_FAILED
    auto gpu_load_failed = registry.getError(ErrorCode::ERR_LORA_GPU_LOAD_FAILED);
    EXPECT_EQ(gpu_load_failed.code, ErrorCode::ERR_LORA_GPU_LOAD_FAILED);
    EXPECT_EQ(gpu_load_failed.category, "LoRA");
    EXPECT_EQ(gpu_load_failed.severity, "Error");
}

TEST_F(ErrorRegistryTest, NewMCPErrorCodes) {
    auto& registry = ErrorRegistry::getInstance();
    
    // Test ERR_MCP_STDIO_INIT_FAILED
    auto stdio_init_failed = registry.getError(ErrorCode::ERR_MCP_STDIO_INIT_FAILED);
    EXPECT_EQ(stdio_init_failed.code, ErrorCode::ERR_MCP_STDIO_INIT_FAILED);
    EXPECT_EQ(stdio_init_failed.category, "MCP");
    EXPECT_EQ(stdio_init_failed.severity, "Error");
    EXPECT_FALSE(stdio_init_failed.message_template.empty());
}

TEST_F(ErrorRegistryTest, NewErrorCodesInJSON) {
    auto& registry = ErrorRegistry::getInstance();
    
    // Test that new error codes are included in JSON output
    auto json_data = registry.toJSON();
    
    EXPECT_TRUE(json_data.contains("errors"));
    auto& errors = json_data["errors"];
    
    // Check for presence of new error codes
    bool found_invalid_handle = false;
    bool found_vision_failed = false;
    bool found_lora_mismatch = false;
    bool found_stdio_failed = false;
    
    for (const auto& error : errors) {
        int code = error["code"].get<int>();
        if (code == static_cast<int>(ErrorCode::ERR_LLM_INVALID_HANDLE)) found_invalid_handle = true;
        if (code == static_cast<int>(ErrorCode::ERR_LLM_VISION_INFERENCE_FAILED)) found_vision_failed = true;
        if (code == static_cast<int>(ErrorCode::ERR_LORA_MODEL_MISMATCH)) found_lora_mismatch = true;
        if (code == static_cast<int>(ErrorCode::ERR_MCP_STDIO_INIT_FAILED)) found_stdio_failed = true;
    }
    
    EXPECT_TRUE(found_invalid_handle) << "ERR_LLM_INVALID_HANDLE not found in JSON";
    EXPECT_TRUE(found_vision_failed) << "ERR_LLM_VISION_INFERENCE_FAILED not found in JSON";
    EXPECT_TRUE(found_lora_mismatch) << "ERR_LORA_MODEL_MISMATCH not found in JSON";
    EXPECT_TRUE(found_stdio_failed) << "ERR_MCP_STDIO_INIT_FAILED not found in JSON";
}

TEST_F(ErrorRegistryTest, SearchNewErrorCodes) {
    auto& registry = ErrorRegistry::getInstance();
    
    // Test searching for vision-related errors
    auto vision_errors = registry.searchErrors("vision");
    EXPECT_GT(vision_errors.size(), 0);
    
    bool found_vision_error = false;
    for (const auto& error : vision_errors) {
        if (error.code == ErrorCode::ERR_LLM_VISION_INFERENCE_FAILED) {
            found_vision_error = true;
            break;
        }
    }
    EXPECT_TRUE(found_vision_error) << "ERR_LLM_VISION_INFERENCE_FAILED not found via search";
    
    // Test searching for RAM OOM errors
    auto ram_errors = registry.searchErrors("ram");
    bool found_ram_oom = false;
    for (const auto& error : ram_errors) {
        if (error.code == ErrorCode::ERR_LLM_RAM_OOM) {
            found_ram_oom = true;
            break;
        }
    }
    EXPECT_TRUE(found_ram_oom) << "ERR_LLM_RAM_OOM not found via search";
    
    // Test searching for stdio errors
    auto stdio_errors = registry.searchErrors("stdio");
    bool found_stdio_error = false;
    for (const auto& error : stdio_errors) {
        if (error.code == ErrorCode::ERR_MCP_STDIO_INIT_FAILED) {
            found_stdio_error = true;
            break;
        }
    }
    EXPECT_TRUE(found_stdio_error) << "ERR_MCP_STDIO_INIT_FAILED not found via search";
}

TEST_F(ErrorRegistryTest, VerifyLLMCategoryCount) {
    auto& registry = ErrorRegistry::getInstance();
    
    // Verify LLM category now has at least 12 errors (5 original + 7 new)
    auto llm_errors = registry.getErrorsByCategory("LLM");
    EXPECT_GE(llm_errors.size(), 12);
}

TEST_F(ErrorRegistryTest, VerifyLoRACategoryCount) {
    auto& registry = ErrorRegistry::getInstance();
    
    // Verify LoRA category now has at least 7 errors (5 original + 2 new)
    auto lora_errors = registry.getErrorsByCategory("LoRA");
    EXPECT_GE(lora_errors.size(), 7);
}

TEST_F(ErrorRegistryTest, VerifyMCPCategoryCount) {
    auto& registry = ErrorRegistry::getInstance();
    
    // Verify MCP category now has at least 5 errors (4 original + 1 new)
    auto mcp_errors = registry.getErrorsByCategory("MCP");
    EXPECT_GE(mcp_errors.size(), 5);
}

TEST_F(ErrorRegistryTest, ConcurrentReadWriteAccessDoesNotCorruptRegistry) {
    auto& registry = ErrorRegistry::getInstance();

    std::atomic<bool> failed{false};
    constexpr int kIterations = 200;
    std::vector<std::thread> workers;
    workers.reserve(4);

    workers.emplace_back([&]() {
        for (int i = 0; i < kIterations; ++i) {
            ErrorMetadata custom{
                static_cast<ErrorCode>(9800 + (i % 50)),
                "TestConcurrent",
                "Warning",
                "Concurrent test error {}",
                "Synthetic concurrent registration",
                "No action required",
                {},
                {"concurrency", "test"}
            };
            registry.registerError(custom);
        }
    });

    workers.emplace_back([&]() {
        for (int i = 0; i < kIterations; ++i) {
            auto error = registry.getError(ErrorCode::ERR_LLM_MODEL_NOT_FOUND);
            if (error.category.empty()) {
                failed.store(true);
            }
        }
    });

    workers.emplace_back([&]() {
        for (int i = 0; i < kIterations; ++i) {
            auto by_category = registry.getErrorsByCategory("LLM");
            if (by_category.empty()) {
                failed.store(true);
            }
        }
    });

    workers.emplace_back([&]() {
        for (int i = 0; i < kIterations; ++i) {
            auto dump = registry.toJSON();
            if (!dump.contains("errors")) {
                failed.store(true);
            }
        }
    });

    for (auto& worker : workers) {
        worker.join();
    }

    EXPECT_FALSE(failed.load());
}
