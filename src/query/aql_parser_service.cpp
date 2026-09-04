/**
 * @file aql_parser_service.cpp
 * @brief Implementation of AQL parser service interface
 *
 * Wraps existing AQLParser to expose clean service interface for SOC/OOP.
 *
 * @author ThemisDB Team
 * @date 2026-06-18
 */

#include "query/aql_parser_service.h"
#include "query/aql_parser.h"

#include <spdlog/spdlog.h>
#include <utility>

namespace themis::query {

/** @brief Implementation detail. */
class AQLParserServiceImpl::Impl {
public:
    explicit Impl(
        bool enable_mutations = false,
        bool enable_ddl = false,
        bool enable_geospatial = true
    )
        : parser_(),
          enable_mutations_(enable_mutations),
          enable_ddl_(enable_ddl),
          enable_geospatial_(enable_geospatial) {}
    
    AQLParser parser_;
    bool enable_mutations_;
    bool enable_ddl_;
    bool enable_geospatial_;
};

// ============================================================================
// AQLParserServiceImpl Implementation
// ============================================================================

AQLParserServiceImpl::AQLParserServiceImpl(
    bool enable_mutations,
    bool enable_ddl,
    bool enable_geospatial)
    : impl_(std::make_unique<Impl>(enable_mutations, enable_ddl, enable_geospatial)) {}

AQLParserServiceImpl::~AQLParserServiceImpl() = default;

ParseResult AQLParserServiceImpl::parse(const std::string& aql_query) {
    ParseResult result = {};
    
    if (aql_query.empty()) {
        result.success = false;
        result.diagnostics.error_message = "Empty query";
        result.diagnostics.error_category = "EMPTY_QUERY";
        return result;
    }
    
    try {
        // Use existing AQLParser to parse query
        auto ast = impl_->parser_.parse(aql_query);
        
        if (!ast) {
            result.success = false;
            
            // Extract diagnostic info from parser state
            // NOTE: Assumes AQLParser has diagnostics() method or similar
            // For now, provide generic error; will be enhanced when parser
            // exposes detailed diagnostic info
            result.diagnostics.error_message = "Parse failed (details pending parser enhancement)";
            result.diagnostics.error_category = "SYNTAX_ERROR";
            result.diagnostics.error_context = aql_query;
            
            return result;
        }
        
        result.success = true;
        // PHASE GATE: AST is not exposed in ParseResult until Phase 1.
        // Phase 0.3 limitation: Full AST access deferred pending ASTNode exposure.
        
    } catch (const std::exception& e) {
        result.success = false;
        result.diagnostics.error_message = std::string("Parse exception: ") + e.what();
        result.diagnostics.error_category = "PARSE_EXCEPTION";
        
        spdlog::debug("AQLParser threw exception: {}", e.what());
    }
    
    return result;
}

std::string AQLParserServiceImpl::version() const {
    std::string base = "AQL v1.3 (read-only)";
    
    if (impl_->enable_mutations_) {
        base += " +mutations";
    }
    if (impl_->enable_ddl_) {
        base += " +ddl";
    }
    
    return base;
}

bool AQLParserServiceImpl::supportsFeature(const std::string& feature) const {
    if (feature == "mutations" || feature == "dml") {
        return impl_->enable_mutations_;
    }
    if (feature == "ddl") {
        return impl_->enable_ddl_;
    }
    if (feature == "geospatial") {
        return impl_->enable_geospatial_;
    }
    if (feature == "for" || feature == "filter" || feature == "sort" || 
        feature == "limit" || feature == "return") {
        return true;  // Core features always supported
    }
    
    return false;
}

// ============================================================================
// AQLParserServiceFactory Implementation
// ============================================================================

std::shared_ptr<AQLParserService> AQLParserServiceFactory::create() {
    return std::make_shared<AQLParserServiceImpl>();
}

std::shared_ptr<AQLParserService> AQLParserServiceFactory::createWithFeatures(
    bool enable_mutations,
    bool enable_ddl,
    bool enable_geospatial) {
    return std::make_shared<AQLParserServiceImpl>(
        enable_mutations,
        enable_ddl,
        enable_geospatial
    );
}

} // namespace themis::query
