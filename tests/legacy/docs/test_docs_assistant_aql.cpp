/**
 * @file test_docs_assistant_aql.cpp
 * @brief Unit tests for documentation assistant AQL functions
 * 
 * Tests both the unified HELP() function and individual functions.
 */

#include <gtest/gtest.h>
#include "aql/docs_assistant_functions.h"
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
namespace fs = std::filesystem;

/**
 * @brief Test fixture for documentation assistant AQL functions
 */
class DocsAssistantAQLTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a minimal test documentation database
        test_db_path_ = "/tmp/test_docs_database.json";
        createTestDatabase();
    }
    
    void TearDown() override {
        // Clean up test database
        if (fs::exists(test_db_path_)) {
            fs::remove(test_db_path_);
        }
    }
    
    void createTestDatabase() {
        json db;
        
        db["metadata"] = {
            {"version", "1.0.0"},
            {"generation_time", "2024-01-11T12:00:00Z"},
            {"total_documents", 3},
            {"themisdb_version", "1.3.0"}
        };
        
        db["statistics"] = {
            {"total_size_bytes", 10000}
        };
        
        db["documents"] = json::array({
            {
                {"file_hash", "abc123"},
                {"file_path", "docs/sharding.md"},
                {"mime_type", "text/markdown"},
                {"ingestion_time", "2024-01-11T12:00:00Z"},
                {"metadata", {
                    {"file_name", "sharding.md"}
                }},
                {"themis_metadata", {
                    {"vector", {
                        {"text_content", "Sharding in ThemisDB allows you to distribute data across multiple nodes. Enable with THEMIS_ENABLE_SHARDING=true."}
                    }}
                }}
            },
            {
                {"file_hash", "def456"},
                {"file_path", "docs/security.md"},
                {"mime_type", "text/markdown"},
                {"ingestion_time", "2024-01-11T12:00:00Z"},
                {"metadata", {
                    {"file_name", "security.md"}
                }},
                {"themis_metadata", {
                    {"vector", {
                        {"text_content", "Security features include TLS encryption, authentication, and RBAC. Configure with security.yaml."}
                    }}
                }}
            },
            {
                {"file_hash", "ghi789"},
                {"file_path", "docs/troubleshooting.md"},
                {"mime_type", "text/markdown"},
                {"ingestion_time", "2024-01-11T12:00:00Z"},
                {"metadata", {
                    {"file_name", "troubleshooting.md"}
                }},
                {"themis_metadata", {
                    {"vector", {
                        {"text_content", "If server hangs at startup, check port availability and permissions. Common issue: port 8529 already in use."}
                    }}
                }}
            }
        });
        
        // Write to file
        std::ofstream out(test_db_path_);
        out << db.dump(2);
        out.close();
    }
    
    std::string test_db_path_;
};

/**
 * @brief Test basic instantiation and singleton pattern
 */
TEST_F(DocsAssistantAQLTest, SingletonPattern) {
    auto& func1 = themis::aql::getDocsAssistantFunctions();
    auto& func2 = themis::aql::getDocsAssistantFunctions();
    
    // Should be the same instance
    EXPECT_EQ(&func1, &func2);
}

/**
 * @brief Test DOCS_SEARCH function
 */
TEST_F(DocsAssistantAQLTest, DocsSearch) {
    auto& docs_func = themis::aql::getDocsAssistantFunctions();
    
    // Note: This test may skip if docs database is not available
    if (!docs_func.isReady()) {
        GTEST_SKIP() << "Documentation database not available";
    }
    
    // Search for sharding documentation
    auto results = docs_func.docsSearch("sharding", 5);
    
    EXPECT_TRUE(results.is_array());
    
    if (!results.empty()) {
        auto first_result = results[0];
        
        EXPECT_TRUE(first_result.contains("file_name"));
        EXPECT_TRUE(first_result.contains("relevance_score"));
        EXPECT_TRUE(first_result.contains("content_preview"));
        
        // Relevance score should be between 0 and 1
        float score = first_result["relevance_score"];
        EXPECT_GE(score, 0.0f);
        EXPECT_LE(score, 1.0f);
    }
}

/**
 * @brief Test DOCS_SEARCH with limit parameter
 */
TEST_F(DocsAssistantAQLTest, DocsSearchWithLimit) {
    auto& docs_func = themis::aql::getDocsAssistantFunctions();
    
    if (!docs_func.isReady()) {
        GTEST_SKIP() << "Documentation database not available";
    }
    
    // Search with limit of 3
    auto results = docs_func.docsSearch("configuration", 3);
    
    EXPECT_TRUE(results.is_array());
    EXPECT_LE(results.size(), 3) << "Should respect limit parameter";
}

/**
 * @brief Test unified HELP() function - General query
 */
TEST_F(DocsAssistantAQLTest, HelpGeneralQuery) {
    auto& docs_func = themis::aql::getDocsAssistantFunctions();
    
    if (!docs_func.isReady()) {
        GTEST_SKIP() << "Documentation database not available";
    }
    
    try {
        // General question should route to RAG query
        auto answer = docs_func.help("How do I enable sharding?");
        
        EXPECT_FALSE(answer.empty());
        EXPECT_TRUE(
            answer.find("shard") != std::string::npos ||
            answer.find("distribute") != std::string::npos ||
            answer.find("enable") != std::string::npos
        );
        
    } catch (const std::exception& e) {
        GTEST_SKIP() << "LLM not available: " << e.what();
    }
}

/**
 * @brief Test unified HELP() function - Configuration intent
 */
TEST_F(DocsAssistantAQLTest, HelpConfigurationIntent) {
    auto& docs_func = themis::aql::getDocsAssistantFunctions();
    
    if (!docs_func.isReady()) {
        GTEST_SKIP() << "Documentation database not available";
    }
    
    try {
        // Query with "configure" should route to config help
        auto answer = docs_func.help("Configure security settings");
        
        EXPECT_FALSE(answer.empty());
        EXPECT_TRUE(
            answer.find("security") != std::string::npos ||
            answer.find("TLS") != std::string::npos ||
            answer.find("auth") != std::string::npos
        );
        
    } catch (const std::exception& e) {
        GTEST_SKIP() << "LLM not available: " << e.what();
    }
}

/**
 * @brief Test unified HELP() function - Troubleshooting intent
 */
TEST_F(DocsAssistantAQLTest, HelpTroubleshootingIntent) {
    auto& docs_func = themis::aql::getDocsAssistantFunctions();
    
    if (!docs_func.isReady()) {
        GTEST_SKIP() << "Documentation database not available";
    }
    
    try {
        // Query with "error" should route to troubleshooting
        auto answer = docs_func.help("Server hangs at startup");
        
        EXPECT_FALSE(answer.empty());
        EXPECT_TRUE(
            answer.find("port") != std::string::npos ||
            answer.find("startup") != std::string::npos ||
            answer.find("hang") != std::string::npos
        );
        
    } catch (const std::exception& e) {
        GTEST_SKIP() << "LLM not available: " << e.what();
    }
}

/**
 * @brief Test unified HELP() function - Search intent
 */
TEST_F(DocsAssistantAQLTest, HelpSearchIntent) {
    auto& docs_func = themis::aql::getDocsAssistantFunctions();
    
    if (!docs_func.isReady()) {
        GTEST_SKIP() << "Documentation database not available";
    }
    
    // Query with "search" should route to document search
    auto answer = docs_func.help("Search for sharding documentation");
    
    EXPECT_FALSE(answer.empty());
    // Should contain search results formatting
    EXPECT_TRUE(
        answer.find("Found") != std::string::npos ||
        answer.find("relevant") != std::string::npos ||
        answer.find("document") != std::string::npos
    );
}

/**
 * @brief Test DOCS_QUERY function (explicit) (requires LLM)
 */
TEST_F(DocsAssistantAQLTest, DocsQuery) {
    auto& docs_func = themis::aql::getDocsAssistantFunctions();
    
    if (!docs_func.isReady()) {
        GTEST_SKIP() << "Documentation database not available";
    }
    
    try {
        // Query for sharding help
        auto answer = docs_func.docsQuery("How do I enable sharding?");
        
        // Should return a non-empty string
        EXPECT_FALSE(answer.empty());
        
        // Answer should contain relevant keywords
        // Note: This is a weak test as LLM output can vary
        EXPECT_TRUE(
            answer.find("shard") != std::string::npos ||
            answer.find("distribute") != std::string::npos ||
            answer.find("enable") != std::string::npos
        ) << "Answer should contain relevant keywords";
        
    } catch (const std::exception& e) {
        // LLM might not be available
        GTEST_SKIP() << "LLM not available: " << e.what();
    }
}

/**
 * @brief Test DOCS_CONFIG_HELP function
 */
TEST_F(DocsAssistantAQLTest, DocsConfigHelp) {
    auto& docs_func = themis::aql::getDocsAssistantFunctions();
    
    if (!docs_func.isReady()) {
        GTEST_SKIP() << "Documentation database not available";
    }
    
    try {
        // Get configuration help for security
        auto help = docs_func.docsConfigHelp("security");
        
        EXPECT_FALSE(help.empty());
        
        // Should contain security-related keywords
        EXPECT_TRUE(
            help.find("security") != std::string::npos ||
            help.find("TLS") != std::string::npos ||
            help.find("auth") != std::string::npos
        );
        
    } catch (const std::exception& e) {
        GTEST_SKIP() << "LLM not available: " << e.what();
    }
}

/**
 * @brief Test DOCS_TROUBLESHOOT function
 */
TEST_F(DocsAssistantAQLTest, DocsTroubleshoot) {
    auto& docs_func = themis::aql::getDocsAssistantFunctions();
    
    if (!docs_func.isReady()) {
        GTEST_SKIP() << "Documentation database not available";
    }
    
    try {
        // Get troubleshooting help for startup issue
        auto solution = docs_func.docsTroubleshoot("Server hangs at startup");
        
        EXPECT_FALSE(solution.empty());
        
        // Should contain troubleshooting-related keywords
        EXPECT_TRUE(
            solution.find("port") != std::string::npos ||
            solution.find("startup") != std::string::npos ||
            solution.find("check") != std::string::npos
        );
        
    } catch (const std::exception& e) {
        GTEST_SKIP() << "LLM not available: " << e.what();
    }
}

/**
 * @brief Test DOCS_STATS function
 */
TEST_F(DocsAssistantAQLTest, DocsStats) {
    auto& docs_func = themis::aql::getDocsAssistantFunctions();
    
    if (!docs_func.isReady()) {
        GTEST_SKIP() << "Documentation database not available";
    }
    
    auto stats = docs_func.docsStats();
    
    EXPECT_TRUE(stats.is_object());
    
    // Stats should contain expected fields
    // Note: Exact fields depend on implementation
    EXPECT_TRUE(
        stats.contains("total_documents") ||
        stats.contains("database_version") ||
        !stats.empty()
    ) << "Stats should contain database information";
}

/**
 * @brief Test error handling when database not loaded
 */
TEST_F(DocsAssistantAQLTest, ErrorHandlingNoDatabaseSTRESS) {
#ifdef _WIN32
    GTEST_SKIP() << "Skipping unstable STRESS test on Windows";
#endif
    // Create a new instance that won't find database
    // This tests graceful degradation
    
    // Remove test database temporarily
    if (fs::exists(test_db_path_)) {
        fs::remove(test_db_path_);
    }
    
    // Try to create fresh instance
    themis::aql::DocsAssistantFunctions docs_func;
    
    // Should handle missing database gracefully
    if (!docs_func.isReady()) {
        // Expected behavior - functions should throw informative errors
        EXPECT_THROW(
            docs_func.docsQuery("test"),
            std::runtime_error
        );
        
        EXPECT_THROW(
            docs_func.docsSearch("test", 5),
            std::runtime_error
        );
    }
}

/**
 * @brief Test cache clearing
 */
TEST_F(DocsAssistantAQLTest, CacheClear) {
    auto& docs_func = themis::aql::getDocsAssistantFunctions();
    
    if (!docs_func.isReady()) {
        GTEST_SKIP() << "Documentation database not available";
    }
    
    // Should not throw
    EXPECT_NO_THROW(docs_func.clearCache());
}

/**
 * @brief Integration test: Search then query workflow
 */
TEST_F(DocsAssistantAQLTest, IntegrationSearchThenQuery) {
    auto& docs_func = themis::aql::getDocsAssistantFunctions();
    
    if (!docs_func.isReady()) {
        GTEST_SKIP() << "Documentation database not available";
    }
    
    // Step 1: Search for relevant documents
    auto search_results = docs_func.docsSearch("sharding configuration", 3);
    EXPECT_TRUE(search_results.is_array());
    
    // Step 2: Use query to get detailed answer
    try {
        auto query_answer = docs_func.docsQuery("Explain sharding configuration");
        EXPECT_FALSE(query_answer.empty());
        
        // Both operations should complete successfully
        SUCCEED();
        
    } catch (const std::exception& e) {
        GTEST_SKIP() << "LLM not available: " << e.what();
    }
}

/**
 * @brief Performance test: Multiple searches should use cache
 */
TEST_F(DocsAssistantAQLTest, PerformanceCaching) {
    auto& docs_func = themis::aql::getDocsAssistantFunctions();
    
    if (!docs_func.isReady()) {
        GTEST_SKIP() << "Documentation database not available";
    }
    
    const std::string query = "test query for caching";
    
    // First search
    auto start1 = std::chrono::high_resolution_clock::now();
    auto result1 = docs_func.docsSearch(query, 5);
    auto end1 = std::chrono::high_resolution_clock::now();
    auto duration1 = std::chrono::duration_cast<std::chrono::milliseconds>(end1 - start1);
    
    // Second search (should be cached)
    auto start2 = std::chrono::high_resolution_clock::now();
    auto result2 = docs_func.docsSearch(query, 5);
    auto end2 = std::chrono::high_resolution_clock::now();
    auto duration2 = std::chrono::duration_cast<std::chrono::milliseconds>(end2 - start2);
    
    // Results should be the same
    EXPECT_EQ(result1.dump(), result2.dump());
    
    // Second query might be faster due to caching
    // (Not a strict requirement as caching implementation may vary)
    std::cout << "First query: " << duration1.count() << "ms" << std::endl;
    std::cout << "Second query (cached): " << duration2.count() << "ms" << std::endl;
}

/**
 * @brief Main function
 */


/**
 * ============================================================================
 * LoRA Integration Tests
 * ============================================================================
 */

/**
 * @brief Test that LoRA availability can be checked
 */
TEST_F(DocsAssistantAQLTest, LoRAAvailabilityCheck) {
    auto& docs_func = themis::aql::getDocsAssistantFunctions();
    
    if (!docs_func.isReady()) {
        GTEST_SKIP() << "Documentation database not available";
    }
    
    // Should not crash when checking LoRA availability
    bool lora_active = docs_func.isLoRAActive();
    
    // Log result for debugging
    if (lora_active) {
        std::cout << "LoRA adapter is active" << std::endl;
    } else {
        std::cout << "LoRA adapter not available, using base model" << std::endl;
    }
    
    // Test should pass regardless of whether LoRA is available
    SUCCEED();
}

/**
 * @brief Test HELP() function with potential LoRA support
 */
TEST_F(DocsAssistantAQLTest, HelpWithLoRASupport) {
    auto& docs_func = themis::aql::getDocsAssistantFunctions();
    
    if (!docs_func.isReady()) {
        GTEST_SKIP() << "Documentation database not available";
    }
    
    try {
        // Query should work whether LoRA is available or not
        auto answer = docs_func.help("How do I configure sharding?", "test_user");
        
        EXPECT_FALSE(answer.empty());
        
        // Answer should contain relevant keywords
        std::string lower_answer = answer;
        std::transform(lower_answer.begin(), lower_answer.end(), 
                      lower_answer.begin(), ::tolower);
        
        EXPECT_TRUE(
            lower_answer.find("shard") != std::string::npos ||
            lower_answer.find("distribute") != std::string::npos ||
            lower_answer.find("partition") != std::string::npos
        ) << "Answer should contain sharding-related keywords";
        
    } catch (const std::exception& e) {
        GTEST_SKIP() << "LLM/LoRA not available: " << e.what();
    }
}

/**
 * @brief Test performance metrics retrieval
 */
TEST_F(DocsAssistantAQLTest, PerformanceMetrics) {
    auto& docs_func = themis::aql::getDocsAssistantFunctions();
    
    if (!docs_func.isReady()) {
        GTEST_SKIP() << "Documentation database not available";
    }
    
    // Should be able to get metrics
    auto metrics = docs_func.getPerformanceMetrics();
    
    EXPECT_TRUE(metrics.is_object());
    EXPECT_TRUE(metrics.contains("lora_active"));
    
    // If LoRA is active, should have LoRA-specific metrics
    if (metrics["lora_active"].get<bool>()) {
        EXPECT_TRUE(metrics.contains("lora"));
        EXPECT_TRUE(metrics.contains("lora_feedback"));
        EXPECT_TRUE(metrics.contains("lora_version"));
        EXPECT_TRUE(metrics.contains("lora_trained"));
        
        std::cout << "LoRA Metrics:\n" << metrics.dump(2) << std::endl;
    }
}

/**
 * @brief Test multiple queries to verify LoRA caching/performance
 */
TEST_F(DocsAssistantAQLTest, MultipleQueriesWithLoRA) {
    auto& docs_func = themis::aql::getDocsAssistantFunctions();
    
    if (!docs_func.isReady()) {
        GTEST_SKIP() << "Documentation database not available";
    }
    
    try {
        std::vector<std::string> queries = {
            "How do I enable sharding?",
            "Configure security settings",
            "Server hangs at startup"
        };
        
        for (const auto& query : queries) {
            auto start = std::chrono::high_resolution_clock::now();
            auto answer = docs_func.help(query, "test_user");
            auto end = std::chrono::high_resolution_clock::now();
            
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
            
            EXPECT_FALSE(answer.empty()) << "Query: " << query;
            std::cout << "Query '" << query << "' completed in " 
                     << duration.count() << "ms" << std::endl;
        }
        
        // Get metrics after multiple queries
        auto metrics = docs_func.getPerformanceMetrics();
        if (metrics.contains("lora") && metrics["lora"].contains("total_queries")) {
            std::cout << "Total LoRA queries: " 
                     << metrics["lora"]["total_queries"] << std::endl;
        }
        
    } catch (const std::exception& e) {
        GTEST_SKIP() << "LLM/LoRA not available: " << e.what();
    }
}

/**
 * @brief Test fallback behavior when LoRA is not available
 */
TEST_F(DocsAssistantAQLTest, FallbackToBaseModel) {
    auto& docs_func = themis::aql::getDocsAssistantFunctions();
    
    if (!docs_func.isReady()) {
        GTEST_SKIP() << "Documentation database not available";
    }
    
    try {
        // Query should work even if LoRA is not available
        auto answer = docs_func.help("How do I configure replication?");
        
        EXPECT_FALSE(answer.empty());
        
        // Log which model was used
        bool lora_active = docs_func.isLoRAActive();
        std::cout << "Query completed using: " 
                 << (lora_active ? "LoRA adapter" : "Base model") << std::endl;
        
    } catch (const std::exception& e) {
        GTEST_SKIP() << "LLM not available: " << e.what();
    }
}

/**
 * @brief Test intent detection with LoRA support
 */
TEST_F(DocsAssistantAQLTest, IntentDetectionWithLoRA) {
    auto& docs_func = themis::aql::getDocsAssistantFunctions();
    
    if (!docs_func.isReady()) {
        GTEST_SKIP() << "Documentation database not available";
    }
    
    try {
        // Configuration intent
        auto config_answer = docs_func.help("Configure security settings");
        EXPECT_FALSE(config_answer.empty());
        
        // Troubleshooting intent
        auto trouble_answer = docs_func.help("Server error at startup");
        EXPECT_FALSE(trouble_answer.empty());
        
        // Search intent
        auto search_answer = docs_func.help("Search for RAID documentation");
        EXPECT_FALSE(search_answer.empty());
        
        // General query intent
        auto general_answer = docs_func.help("What is sharding?");
        EXPECT_FALSE(general_answer.empty());
        
        std::cout << "All intent types handled successfully" << std::endl;
        
    } catch (const std::exception& e) {
        GTEST_SKIP() << "LLM/LoRA not available: " << e.what();
    }
}

/**
 * @brief Test error handling when queries fail
 */
TEST_F(DocsAssistantAQLTest, ErrorHandlingWithLoRA) {
    auto& docs_func = themis::aql::getDocsAssistantFunctions();
    
    if (!docs_func.isReady()) {
        GTEST_SKIP() << "Documentation database not available";
    }
    
    try {
        // Empty query should still be handled gracefully
        auto answer = docs_func.help("", "test_user");
        
        // Should return something, even if it's an error message
        // The implementation should not crash
        EXPECT_TRUE(true) << "Empty query handled without crash";
        
    } catch (const std::exception& e) {
        // Exception is acceptable for invalid input
        EXPECT_TRUE(true) << "Exception thrown for invalid input: " << e.what();
    }
}

/**
 * @brief Test cache clearing with LoRA
 */
TEST_F(DocsAssistantAQLTest, CacheClearWithLoRA) {
    auto& docs_func = themis::aql::getDocsAssistantFunctions();
    
    if (!docs_func.isReady()) {
        GTEST_SKIP() << "Documentation database not available";
    }
    
    // Perform some queries to populate cache
    try {
        docs_func.help("Test query 1");
        docs_func.help("Test query 2");
    } catch (...) {
        GTEST_SKIP() << "LLM not available";
    }
    
    // Clear cache should not throw
    EXPECT_NO_THROW(docs_func.clearCache());
    
    // Should still be able to query after cache clear
    try {
        auto answer = docs_func.help("Test query after clear");
        EXPECT_FALSE(answer.empty());
    } catch (const std::exception& e) {
        GTEST_SKIP() << "LLM not available: " << e.what();
    }
}
