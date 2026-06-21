/**
 * @file aql_parser_service.h
 * @brief Abstract parser service interface for clean SOC/OOP separation
 *
 * Exposes AQL parsing capabilities as a service interface for external consumers
 * (particularly src/aql/ LLM integration layer).
 *
 * ARCHITECTURAL PRINCIPLE: src/aql/ depends on src/query/ interfaces ONLY.
 * No reverse dependency (src/query/ does NOT depend on src/aql/).
 *
 * @author ThemisDB Team
 * @date 2026-06-18
 */

#pragma once

#include <memory>
#include <string>
#include <vector>
#include <cstdint>

namespace themis::query {

/**
 * @brief Diagnostic information from parser when syntax error detected
 *
 * Provides detailed error context to enable intelligent retry feedback.
 */
struct ParserDiagnostics {
    /// Line number where error detected (1-indexed)
    uint32_t line_number = 0;
    
    /// Column number in line (1-indexed)
    uint32_t column_number = 0;
    
    /// Primary error message (e.g., "expected RETURN clause")
    std::string error_message;
    
    /// Code context snippet showing offending line + surrounding lines
    std::string error_context;
    
    /// Suggestions for fixing the error (e.g., "Use FOR...IN syntax")
    std::vector<std::string> suggestions;
    
    /// Error category for classification (e.g., "UNKNOWN_KEYWORD", "SYNTAX_ERROR")
    std::string error_category;
};

/**
 * @brief Result of AQL parsing operation
 *
 * On success: `success` is true; diagnostics empty.
 * On failure: `success` is false; `diagnostics` contains error details.
 * 
 * Phase 0.3 note: AST is not returned here (to avoid incomplete type issues with unique_ptr).
 * Full AST access will be added in Phase 1 when ASTNode is fully exposed.
 */
struct ParseResult {
    /// True if parsing succeeded
    bool success = false;
    
    /// Diagnostic information (populated only on parse failure)
    ParserDiagnostics diagnostics;
};

/**
 * @brief Abstract AQL parser service interface
 *
 * Encapsulates AQL parsing logic behind a clean interface suitable for
 * dependency injection in LLM integration layers.
 *
 * USAGE (in src/aql/):
 * ```cpp
 * auto parser_service = AQLParserServiceFactory::create();
 * auto result = parser_service->parse(generated_aql);
 * if (!result.success) {
 *     // Use result.diagnostics to inform LLM retry logic
 *     std::string feedback = formatFeedback(result.diagnostics);
 *     // LLM retry with: "Fix the error: " + feedback
 * }
 * ```
 */
class AQLParserService {
public:
    virtual ~AQLParserService() = default;
    
    /**
     * @brief Parse and validate AQL query string
     *
     * Performs complete syntax validation:
     * - Tokenization (lexical analysis)
     * - Recursive descent parsing (syntactic analysis)
     * - AST construction
     * - Structural validation (e.g., all queries must have RETURN)
     *
     * @param aql_query The AQL query string to parse
     * @return ParseResult with:
     *   - success=true, ast=<tree> on valid syntax
     *   - success=false, diagnostics=<error details> on invalid syntax
     *
     * @note Thread-safe; can be called concurrently
     * @note No side effects on parser state
     */
    virtual ParseResult parse(const std::string& aql_query) = 0;
    
    /**
     * @brief Get parser version and capability info
     *
     * Used for logging, metrics, and compatibility checks.
     *
     * @return Version string, e.g. "AQL v1.3.0 (mutations disabled)"
     */
    virtual std::string version() const = 0;
    
    /**
     * @brief Check if specific feature is supported by this parser
     *
     * Allows consumer code to detect feature availability at runtime.
     *
     * @param feature Feature name, e.g. "mutations", "ddl", "geospatial"
     * @return true if feature is enabled/supported
     */
    virtual bool supportsFeature(const std::string& feature) const = 0;
};

/**
 * @brief Concrete implementation of AQL parser service
 *
 * Wraps the existing AQLParser class to expose it as a service interface.
 * Handles conversion between internal AST representation and ParseResult struct.
 */
class AQLParserServiceImpl : public AQLParserService {
public:
    /// @brief Create parser service with custom feature flags
    /// @param enable_mutations Enable INSERT/UPDATE/DELETE/REPLACE/REMOVE/UPSERT
    /// @param enable_ddl Enable CREATE/DROP/ALTER
    /// @param enable_geospatial Enable ST_* functions in parser context
    explicit AQLParserServiceImpl(
        bool enable_mutations = false,
        bool enable_ddl = false,
        bool enable_geospatial = true
    );
    
    ~AQLParserServiceImpl() override;
    
    ParseResult parse(const std::string& aql_query) override;
    
    std::string version() const override;
    
    bool supportsFeature(const std::string& feature) const override;
    
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

/**
 * @brief Factory for creating AQL parser service instances
 *
 * Centralizes parser service creation to enable future customization
 * (e.g., mock implementations for testing).
 */
class AQLParserServiceFactory {
public:
    /// @brief Create default parser service
    static std::shared_ptr<AQLParserService> create();
    
    /// @brief Create parser with feature configuration
    static std::shared_ptr<AQLParserService> createWithFeatures(
        bool enable_mutations,
        bool enable_ddl,
        bool enable_geospatial
    );
};

} // namespace themis::query
