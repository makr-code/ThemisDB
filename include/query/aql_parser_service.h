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

struct ParserDiagnostics {
    uint32_t line_number = 0;
    
    uint32_t column_number = 0;
    
    std::string error_message;
    
    std::string error_context;
    
    std::vector<std::string> suggestions;
    
    std::string error_category;
};

struct ParseResult {
    bool success = false;
    
    ParserDiagnostics diagnostics;
};

class AQLParserService {
public:
    /**
     * @brief AQLParser Service.
     * @return Return value.
     */
    virtual ~AQLParserService() = default;
    
    /**
     * @brief Parse.
     * @param[in] aql_query Input parameter.
     * @return Return value.
     */
    virtual ParseResult parse(const std::string& aql_query) = 0;
    
    /**
     * @brief Version.
     * @return Return value.
     */
    virtual std::string version() const = 0;
    
    /**
     * @brief Supports Feature.
     * @param[in] feature Input parameter.
     * @return True when the operation succeeds.
     */
    virtual bool supportsFeature(const std::string& feature) const = 0;
};

class AQLParserServiceImpl : public AQLParserService {
public:
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

class AQLParserServiceFactory {
public:
    /**
     * @brief Create.
     * @return Return value.
     */
    static std::shared_ptr<AQLParserService> create();
    
    /**
     * @brief Create With Features.
     * @param[in] enable_mutations Input parameter.
     * @param[in] enable_ddl Input parameter.
     * @param[in] enable_geospatial Input parameter.
     * @return Return value.
     */
    static std::shared_ptr<AQLParserService> createWithFeatures(
        bool enable_mutations,
        bool enable_ddl,
        bool enable_geospatial
    );
};

} // namespace themis::query
