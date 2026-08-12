/**
 * @file shard_capabilities.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <vector>
#include <map>
#include <set>

namespace themis::sharding {

/**
 * Domain Capability - Describes shard specialization
 * 
 * Each shard can advertise its specializations to enable
 * intelligent query routing based on relevance scoring.
 */
struct DomainCapability {
    /**
     * Domain-specific fields (e.g., "law", "medicine", "construction")
     */
    std::vector<std::string> domains;
    
    /**
     * Organization/tenant specialization (e.g., "hamburg_bauamt", "berlin_health")
     */
    std::vector<std::string> organizations;
    
    /**
     * Geographic regions (e.g., "hamburg", "germany", "eu")
     */
    std::vector<std::string> regions;
    
    /**
     * Data types handled (e.g., "building_permits", "legal_documents", "medical_records")
     */
    std::vector<std::string> data_types;
    
    /**
     * Keywords for simple text matching (extracted from domain knowledge)
     */
    std::vector<std::string> keywords;
    
    /**
     * Optional embedding vector for semantic similarity matching
     * (e.g., 384-dim sentence-transformers embedding)
     */
    std::vector<float> embedding;
    
    /**
     * Custom metadata (key-value pairs for extensibility)
     */
    std::map<std::string, std::string> metadata;
    
    /**
     * Check if capability has any specialization defined
     */
    bool isEmpty() const {
        return domains.empty() && organizations.empty() && 
               regions.empty() && data_types.empty() && keywords.empty();
    }
    
    /**
     * Get all keywords including derived from domains, orgs, regions, data_types
     */
    std::set<std::string> getAllKeywords() const {
        std::set<std::string> all_keywords(keywords.begin(), keywords.end());
        all_keywords.insert(domains.begin(), domains.end());
        all_keywords.insert(organizations.begin(), organizations.end());
        all_keywords.insert(regions.begin(), regions.end());
        all_keywords.insert(data_types.begin(), data_types.end());
        return all_keywords;
    }
};

/**
 * Capability Match Result
 * 
 * Represents the relevance score between a query and a shard's capabilities
 */
struct CapabilityMatchResult {
    std::string shard_id;
    double score;                    // Overall relevance score [0.0, 1.0]
    double keyword_score;            // Keyword-based score
    double semantic_score;           // Embedding-based semantic score
    double domain_score;             // Domain match score
    double organization_score;       // Organization match score
    double region_score;             // Region match score
    double data_type_score;          // Data type match score
    
    /**
     * Detailed match information for debugging/metrics
     */
    std::vector<std::string> matched_keywords;
    std::vector<std::string> matched_domains;
    std::vector<std::string> matched_organizations;
    std::vector<std::string> matched_regions;
    std::vector<std::string> matched_data_types;
    
    /**
     * Constructor
     */
    CapabilityMatchResult() 
        : score(0.0), keyword_score(0.0), semantic_score(0.0),
          domain_score(0.0), organization_score(0.0), 
          region_score(0.0), data_type_score(0.0) {}
    
    /**
     * Comparison operator for sorting by score (descending)
     * Note: Returns true if this score is GREATER than other's score
     * to enable descending sort when used with std::sort
     */
    bool operator<(const CapabilityMatchResult& other) const {
        return score > other.score;  // Reversed for descending sort
    }
    
    /**
     * Explicit descending comparison
     */
    bool operator>(const CapabilityMatchResult& other) const {
        return score < other.score;
    }
};

} // namespace themis::sharding
