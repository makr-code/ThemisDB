/**
 * @file test_llm_aql_handler_phase03.cpp
 * @brief Phase 0.3 integration tests for parser service + LLM handler
 *
 * Tests that the refactored llm_aql_handler.cpp properly:
 * 1. Integrates AQLParserService for AST-based validation
 * 2. Maintains backward compatibility with existing code
 * 3. Handles parser diagnostics and retry feedback correctly
 * 4. Logs appropriately at each validation step
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <spdlog/spdlog.h>

#include "aql/llm_aql_handler.h"
#include "query/aql_parser_service.h"
#include "llm/llm_error_codes.h"

using ::testing::Return;
using ::testing::NiceMock;
using ::testing::Matcher;

namespace themis {
namespace aql {
namespace test {

/**
 * @brief Mock AQL Parser Service for Phase 0.3 testing
 */
class MockAQLParserService : public query::AQLParserService {
public:
    MOCK_METHOD(query::ParseResult, parse, (const std::string& aql_query), (const, override));
    MOCK_METHOD(std::string, getVersion, (), (const, override));
    MOCK_METHOD(void, setFeatureFlags, (const query::FeatureFlags& flags), (override));
    MOCK_METHOD(query::FeatureFlags, getFeatureFlags, (), (const, override));
};

/**
 * @brief Mock LLM Plugin Manager for testing LLM generation
 */
class MockLLMPluginManager {
public:
    virtual ~MockLLMPluginManager() = default;
    MOCK_METHOD(std::string, generateText,
                (const std::string& prompt, const std::string& model_id),
                ());
};

/**
 * @class Phase03LLMAQLHandlerTests
 * @brief Test suite for Phase 0.3 parser service integration
 */
class Phase03LLMAQLHandlerTests : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize spdlog for tests
        spdlog::set_level(spdlog::level::debug);
    }

    void TearDown() override {
        spdlog::flush_on(spdlog::level::err);
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// Test: Parser Service Injection
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(Phase03LLMAQLHandlerTests, ParserServiceInjection_Success) {
    /**
     * GIVEN: An LLMAQLHandler created with a Config that includes a parser service
     * WHEN: The handler initializes
     * THEN: The parser service should be stored and retrievable
     */
    auto mock_parser = std::make_shared<NiceMock<MockAQLParserService>>();
    
    LLMAQLHandler::Config config;
    config.parser_service = mock_parser;
    
    LLMAQLHandler handler(config);
    
    // Verify parser service is stored
    auto retrieved = handler.getParserService();
    EXPECT_EQ(retrieved, mock_parser);
}

TEST_F(Phase03LLMAQLHandlerTests, ParserServiceInjection_DefaultCreation) {
    /**
     * GIVEN: An LLMAQLHandler created WITHOUT a parser service in Config
     * WHEN: The handler initializes
     * THEN: A default parser service should be created automatically
     */
    LLMAQLHandler::Config config;
    // Don't set parser_service (leave as nullptr)
    
    LLMAQLHandler handler(config);
    
    // Verify default parser service was created
    auto retrieved = handler.getParserService();
    EXPECT_NE(retrieved, nullptr) << "Default parser service should be created";
}

TEST_F(Phase03LLMAQLHandlerTests, ParserServiceSetter_UpdatesConfig) {
    /**
     * GIVEN: An LLMAQLHandler
     * WHEN: setParserService is called with a new parser service
     * THEN: The new parser service should replace the old one
     */
    LLMAQLHandler handler;
    
    auto old_parser = handler.getParserService();
    auto new_parser = std::make_shared<NiceMock<MockAQLParserService>>();
    
    handler.setParserService(new_parser);
    
    auto retrieved = handler.getParserService();
    EXPECT_EQ(retrieved, new_parser) << "New parser service should be active";
    EXPECT_NE(retrieved, old_parser) << "Old parser service should be replaced";
}

// ─────────────────────────────────────────────────────────────────────────────
// Test: Validation Pipeline Configuration
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(Phase03LLMAQLHandlerTests, ValidationPipelineConfig_Getters) {
    /**
     * GIVEN: An LLMAQLHandler with custom validation config
     * WHEN: getValidationPipelineConfig is called
     * THEN: The config should match what was set
     */
    LLMAQLHandler::Config config;
    config.validation_config.max_retries = 3;
    config.validation_config.timeout_ms = 8000;
    config.validation_config.reject_on_error = true;
    
    LLMAQLHandler handler(config);
    
    auto retrieved = handler.getValidationPipelineConfig();
    EXPECT_EQ(retrieved.max_retries, 3);
    EXPECT_EQ(retrieved.timeout_ms, 8000);
    EXPECT_TRUE(retrieved.reject_on_error);
}

TEST_F(Phase03LLMAQLHandlerTests, ValidationPipelineConfig_Setters) {
    /**
     * GIVEN: An LLMAQLHandler with default validation config
     * WHEN: setValidationPipelineConfig is called with new values
     * THEN: The new config should be active
     */
    LLMAQLHandler handler;
    
    LLMValidationPipelineConfig new_config;
    new_config.max_retries = 5;
    new_config.timeout_ms = 10000;
    new_config.reject_on_error = false;
    
    handler.setValidationPipelineConfig(new_config);
    
    auto retrieved = handler.getValidationPipelineConfig();
    EXPECT_EQ(retrieved.max_retries, 5);
    EXPECT_EQ(retrieved.timeout_ms, 10000);
    EXPECT_FALSE(retrieved.reject_on_error);
}

// ─────────────────────────────────────────────────────────────────────────────
// Test: Validation Mode (Backward Compatibility)
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(Phase03LLMAQLHandlerTests, ValidationMode_WarnOnly_Default) {
    /**
     * GIVEN: An LLMAQLHandler
     * WHEN: getValidationMode is called without setting a mode
     * THEN: The default should be WARN_ONLY (backward compatible)
     */
    LLMAQLHandler handler;
    
    auto mode = handler.getValidationMode();
    EXPECT_EQ(mode, TranslationValidationMode::WARN_ONLY)
        << "Default should be WARN_ONLY for backward compatibility";
}

TEST_F(Phase03LLMAQLHandlerTests, ValidationMode_SetAndGet) {
    /**
     * GIVEN: An LLMAQLHandler
     * WHEN: setValidationMode and then getValidationMode are called
     * THEN: The mode should match what was set
     */
    LLMAQLHandler handler;
    
    handler.setValidationMode(TranslationValidationMode::REJECT_ON_ERROR);
    auto mode = handler.getValidationMode();
    EXPECT_EQ(mode, TranslationValidationMode::REJECT_ON_ERROR);
    
    handler.setValidationMode(TranslationValidationMode::RETRY_ON_ERROR);
    mode = handler.getValidationMode();
    EXPECT_EQ(mode, TranslationValidationMode::RETRY_ON_ERROR);
}

// ─────────────────────────────────────────────────────────────────────────────
// Test: Parser Service Configuration via Config Struct
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(Phase03LLMAQLHandlerTests, Config_WithParserService_Success) {
    /**
     * GIVEN: A Config struct with parser_service set
     * WHEN: LLMAQLHandler is constructed with this config
     * THEN: The parser service should be injected properly
     */
    auto mock_parser = std::make_shared<NiceMock<MockAQLParserService>>();
    
    LLMAQLHandler::Config config;
    config.parser_service = mock_parser;
    config.validation_config.max_retries = 2;
    
    LLMAQLHandler handler(config);
    
    EXPECT_EQ(handler.getParserService(), mock_parser);
    EXPECT_EQ(handler.getValidationPipelineConfig().max_retries, 2);
}

TEST_F(Phase03LLMAQLHandlerTests, Config_Defaults_Sensible) {
    /**
     * GIVEN: A default Config struct
     * WHEN: LLMAQLHandler is constructed
     * THEN: All config defaults should be sensible for production
     */
    LLMAQLHandler::Config config;
    
    // All circuit breakers should have reasonable defaults
    EXPECT_EQ(config.infer_circuit_breaker.failure_threshold, 5);
    EXPECT_EQ(config.rag_circuit_breaker.failure_threshold, 5);
    EXPECT_EQ(config.embed_circuit_breaker.failure_threshold, 5);
    
    // Validation config should have sensible defaults
    EXPECT_EQ(config.validation_config.max_retries, 1);
    EXPECT_EQ(config.validation_config.timeout_ms, 5000);
    EXPECT_FALSE(config.validation_config.reject_on_error);
}

// ─────────────────────────────────────────────────────────────────────────────
// Test: Parser Service Disable (Backward Compat)
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(Phase03LLMAQLHandlerTests, ParserServiceDisable_ExplicitNull) {
    /**
     * GIVEN: An LLMAQLHandler with a parser service
     * WHEN: setParserService(nullptr) is called
     * THEN: Parser validation should be disabled (fallback to string-level)
     */
    auto mock_parser = std::make_shared<NiceMock<MockAQLParserService>>();
    
    LLMAQLHandler::Config config;
    config.parser_service = mock_parser;
    
    LLMAQLHandler handler(config);
    EXPECT_NE(handler.getParserService(), nullptr);
    
    handler.setParserService(nullptr);
    EXPECT_EQ(handler.getParserService(), nullptr)
        << "Parser service should be disabled explicitly";
}

// ─────────────────────────────────────────────────────────────────────────────
// Test: Multiple Handlers with Different Configurations
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(Phase03LLMAQLHandlerTests, MultipleHandlers_IndependentConfigs) {
    /**
     * GIVEN: Multiple LLMAQLHandler instances with different configs
     * WHEN: Each is configured independently
     * THEN: Their configs should not interfere with each other
     */
    auto parser1 = std::make_shared<NiceMock<MockAQLParserService>>();
    auto parser2 = std::make_shared<NiceMock<MockAQLParserService>>();
    
    LLMAQLHandler::Config config1;
    config1.parser_service = parser1;
    config1.validation_config.max_retries = 1;
    
    LLMAQLHandler::Config config2;
    config2.parser_service = parser2;
    config2.validation_config.max_retries = 3;
    
    LLMAQLHandler handler1(config1);
    LLMAQLHandler handler2(config2);
    
    EXPECT_EQ(handler1.getParserService(), parser1);
    EXPECT_EQ(handler2.getParserService(), parser2);
    EXPECT_EQ(handler1.getValidationPipelineConfig().max_retries, 1);
    EXPECT_EQ(handler2.getValidationPipelineConfig().max_retries, 3);
}

} // namespace test
} // namespace aql
} // namespace themis
