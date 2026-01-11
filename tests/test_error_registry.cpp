/**
 * @file test_error_registry.cpp
 * @brief Tests for the Error Registry system
 */

#include <gtest/gtest.h>
#include "utils/error_registry.h"

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
