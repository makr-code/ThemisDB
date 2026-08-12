/**
 * @file capability_matcher.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "sharding/shard_capabilities.h"
#include "sharding/shard_topology.h"
#include <string>
#include <vector>
#include <memory>
#include <map>
#include <set>
#include <atomic>
#include <nlohmann/json.hpp>

namespace themis::sharding {

/**
 * Capability Matcher - Semantic matching engine for shard selection
 * 
 * Analyzes queries and matches them against shard capabilities using:
 * 1. Keyword-based matching (TF-IDF style)
 * 2. Semantic similarity via embeddings (cosine distance)
 * 3. Domain/organization/region/data-type specific scoring
 * 
 * Returns ranked list of shards with relevance scores.
 */
class CapabilityMatcher {
public:
    /**
     * Configuration for capability matching
     */
    struct Config {
        // Scoring weights (must sum to 1.0)
        double keyword_weight = 0.30;      // Weight for keyword matching
        double semantic_weight = 0.40;     // Weight for embedding similarity
        double domain_weight = 0.10;       // Weight for domain matching
        double organization_weight = 0.10; // Weight for organization matching
        double region_weight = 0.05;       // Weight for region matching
        double data_type_weight = 0.05;    // Weight for data type matching
        
        // TF-IDF parameters
        bool use_tfidf = true;             // Use TF-IDF weighting for keywords
        double idf_smoothing = 1.0;        // IDF smoothing factor
        
        // Embedding parameters
        bool enable_embeddings = true;     // Enable semantic matching
        double embedding_threshold = 0.5;  // Min cosine similarity threshold
        
        // Match thresholds
        double min_score = 0.0;            // Minimum match score to return
        size_t max_results = 50;           // Maximum number of results
        
        /**
         * Validate configuration
         */
        bool isValid() const {
            double sum = keyword_weight + semantic_weight + domain_weight +
                        organization_weight + region_weight + data_type_weight;
            return std::abs(sum - 1.0) < 0.001; // Allow small floating point errors
        }
    };
    
    /**
     * Query context for matching
     */
    struct QueryContext {
        std::string query_text;            // Original query text
        std::vector<std::string> keywords; // Extracted keywords
        std::vector<float> embedding;      // Query embedding (optional)
        std::vector<std::string> domains;  // Detected domains (optional)
        std::vector<std::string> organizations; // Detected organizations (optional)
        std::vector<std::string> regions;  // Detected regions (optional)
        std::vector<std::string> data_types; // Detected data types (optional)
    };
    
    /**
     * Construct capability matcher
     * @param config Configuration
     */
    explicit CapabilityMatcher(const Config& config);
    CapabilityMatcher();
    
    /**
     * Match query against shard capabilities
     * 
     * @param query Query context with text and optional metadata
     * @param shards List of shards with their capabilities
     * @return Ranked list of match results sorted by score (descending)
     */
    std::vector<CapabilityMatchResult> match(
        const QueryContext& query,
        const std::vector<ShardInfo>& shards
    );
    
    /**
     * Match query against single shard
     * 
     * @param query Query context
     * @param shard Shard info with capabilities
     * @return Match result with detailed scoring
     */
    CapabilityMatchResult matchShard(
        const QueryContext& query,
        const ShardInfo& shard
    );
    
    /**
     * Extract keywords from query text
     * Simple tokenization and stopword removal
     * 
     * @param query_text Query text
     * @return List of keywords
     */
    std::vector<std::string> extractKeywords(const std::string& query_text);
    
    /**
     * Build IDF (Inverse Document Frequency) map from shard capabilities
     * Used for TF-IDF scoring
     * 
     * @param shards List of shards with capabilities
     */
    void buildIDF(const std::vector<ShardInfo>& shards);
    
    /**
     * Get statistics about matching performance
     * @return JSON with match counts, avg scores, etc.
     */
    nlohmann::json getStatistics() const;

private:
    Config config_;
    
    // IDF cache for keyword scoring
    std::map<std::string, double> idf_cache_;
    size_t total_shards_ = 0;
    
    // Statistics
    mutable std::atomic<uint64_t> total_matches_{0};
    mutable std::atomic<uint64_t> keyword_matches_{0};
    mutable std::atomic<uint64_t> semantic_matches_{0};
    
    /**
     * Calculate keyword-based score using TF-IDF
     * @param query_keywords Query keywords
     * @param shard_keywords Shard keywords
     * @return Keyword score [0.0, 1.0]
     */
    double calculateKeywordScore(
        const std::vector<std::string>& query_keywords,
        const std::set<std::string>& shard_keywords,
        std::vector<std::string>& matched_keywords
    );
    
    /**
     * Calculate semantic similarity using cosine distance
     * @param query_embedding Query embedding vector
     * @param shard_embedding Shard embedding vector
     * @return Cosine similarity [0.0, 1.0]
     */
    double calculateSemanticScore(
        const std::vector<float>& query_embedding,
        const std::vector<float>& shard_embedding
    );
    
    /**
     * Calculate domain match score
     * @param query_domains Query domains
     * @param shard_domains Shard domains
     * @param matched_domains Output: matched domains
     * @return Domain score [0.0, 1.0]
     */
    double calculateDomainScore(
        const std::vector<std::string>& query_domains,
        const std::vector<std::string>& shard_domains,
        std::vector<std::string>& matched_domains
    );
    
    /**
     * Calculate organization match score
     */
    double calculateOrganizationScore(
        const std::vector<std::string>& query_orgs,
        const std::vector<std::string>& shard_orgs,
        std::vector<std::string>& matched_orgs
    );
    
    /**
     * Calculate region match score
     */
    double calculateRegionScore(
        const std::vector<std::string>& query_regions,
        const std::vector<std::string>& shard_regions,
        std::vector<std::string>& matched_regions
    );
    
    /**
     * Calculate data type match score
     */
    double calculateDataTypeScore(
        const std::vector<std::string>& query_types,
        const std::vector<std::string>& shard_types,
        std::vector<std::string>& matched_types
    );
    
    /**
     * Calculate TF (Term Frequency) for a term in keywords
     * @param term Term to count
     * @param keywords List of keywords
     * @return Term frequency
     */
    double calculateTF(const std::string& term, const std::vector<std::string>& keywords);
    
    /**
     * Get IDF (Inverse Document Frequency) for a term
     * @param term Term to lookup
     * @return IDF value
     */
    double getIDF(const std::string& term) const;
    
    /**
     * Normalize string for matching (lowercase, trim)
     * @param str Input string
     * @return Normalized string
     */
    std::string normalize(const std::string& str) const;
    
    /**
     * Calculate Jaccard similarity between two sets
     * @param set1 First set
     * @param set2 Second set
     * @return Jaccard similarity [0.0, 1.0]
     */
    double jaccardSimilarity(
        const std::set<std::string>& set1,
        const std::set<std::string>& set2
    ) const;
};

} // namespace themis::sharding
