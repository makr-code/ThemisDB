/**
 * @file capability_matcher.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=0, M=4, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "sharding/capability_matcher.h"
#include <algorithm>
#include <cmath>
#include <cctype>
#include <sstream>
#include <set>
#include <nlohmann/json.hpp>

namespace themis::sharding {

namespace {

// Common stopwords for German and English
const std::set<std::string> STOPWORDS = {
    // German
    "der", "die", "das", "den", "dem", "des", "ein", "eine", "einer", "eines",
    "und", "oder", "aber", "von", "zu", "im", "am", "auf", "für", "mit",
    "ist", "sind", "war", "waren", "wird", "werden", "wurde", "wurden",
    // English
    "the", "a", "an", "and", "or", "but", "of", "to", "in", "on", "for", "with",
    "is", "are", "was", "were", "be", "been", "being", "have", "has", "had"
};

} // anonymous namespace

CapabilityMatcher::CapabilityMatcher(const Config& config)
    : config_(config) {
    if (!config_.isValid()) {
        throw std::invalid_argument("Invalid CapabilityMatcher configuration: weights must sum to 1.0");
    }
}

CapabilityMatcher::CapabilityMatcher()
    : CapabilityMatcher(Config{})
{
}

std::vector<CapabilityMatchResult> CapabilityMatcher::match(
    const QueryContext& query,
    const std::vector<ShardInfo>& shards
) {
    total_matches_++;
    
    std::vector<CapabilityMatchResult> results = {};

    results.reserve(shards.size());
    
    // Build IDF if using TF-IDF and not already built
    if (config_.use_tfidf && (total_shards_ != shards.size())) {
        buildIDF(shards);
    }
    
    // Match each shard
    for (const auto& shard : shards) {
        auto match_result = matchShard(query, shard);
        
        // Only include if above minimum score threshold
        if (match_result.score >= config_.min_score) {
            results.push_back(match_result);
        }
    }
    
    // Sort by score (descending)
    std::sort(results.begin(), results.end());
    
    // Limit to max results
    if (static_cast<int>(results.size()) > config_.max_results) {
        results.resize(config_.max_results);
    }
    
    return results;
}

CapabilityMatchResult CapabilityMatcher::matchShard(
    const QueryContext& query,
    const ShardInfo& shard
) {
    CapabilityMatchResult result;
    result.shard_id = shard.shard_id;
    
    // Get shard's domain capability
    const DomainCapability& shard_capability = shard.domain_capability;
    
    // If shard has no capability defined, fall back to using existing capabilities as keywords
    DomainCapability effective_capability = shard_capability;
    if (shard_capability.isEmpty() && !shard.capabilities.empty()) {
        effective_capability.keywords = shard.capabilities;
    }
    
    auto shard_keywords = effective_capability.getAllKeywords();
    
    // Calculate keyword score
    if (!query.keywords.empty() && !shard_keywords.empty()) {
        result.keyword_score = calculateKeywordScore(
            query.keywords, shard_keywords, result.matched_keywords);
        keyword_matches_++;
    }
    
    // Calculate semantic score if embeddings available
    if (config_.enable_embeddings && 
        !query.embedding.empty() && 
        !effective_capability.embedding.empty()) {
        result.semantic_score = calculateSemanticScore(
            query.embedding, effective_capability.embedding);
        semantic_matches_++;
    }
    
    // Calculate domain score
    if (!query.domains.empty() && !effective_capability.domains.empty()) {
        result.domain_score = calculateDomainScore(
            query.domains, effective_capability.domains, result.matched_domains);
    }
    
    // Calculate organization score
    if (!query.organizations.empty() && !effective_capability.organizations.empty()) {
        result.organization_score = calculateOrganizationScore(
            query.organizations, effective_capability.organizations, result.matched_organizations);
    }
    
    // Calculate region score
    if (!query.regions.empty() && !effective_capability.regions.empty()) {
        result.region_score = calculateRegionScore(
            query.regions, effective_capability.regions, result.matched_regions);
    }
    
    // Calculate data type score
    if (!query.data_types.empty() && !effective_capability.data_types.empty()) {
        result.data_type_score = calculateDataTypeScore(
            query.data_types, effective_capability.data_types, result.matched_data_types);
    }
    
    // Calculate overall score using weighted sum
    result.score = 
        config_.keyword_weight * result.keyword_score +
        config_.semantic_weight * result.semantic_score +
        config_.domain_weight * result.domain_score +
        config_.organization_weight * result.organization_score +
        config_.region_weight * result.region_score +
        config_.data_type_weight * result.data_type_score;
    
    return result;
}

std::vector<std::string> CapabilityMatcher::extractKeywords(const std::string& query_text) {
    std::vector<std::string> keywords;
    std::string word = {};
    std::istringstream stream(query_text);
    
    while (stream >> word) {
        // Normalize word
        std::string normalized = normalize(word);
        
        // Remove punctuation
        normalized.erase(
            std::remove_if(normalized.begin(), normalized.end(),
                          [](char c) { return std::ispunct(c); }),
            normalized.end());
        
        // Skip if empty or stopword
        if (!normalized.empty() && 
            STOPWORDS.find(normalized) == STOPWORDS.end() &&
            normalized.length() > 2) {  // Skip very short words
            keywords.push_back(normalized);
        }
    }
    
    return keywords;
}

void CapabilityMatcher::buildIDF(const std::vector<ShardInfo>& shards) {
    idf_cache_.clear();
    total_shards_ = shards.size();
    
    if (total_shards_ == 0) {
      return;
    }
    
    // Count document frequency for each term
    std::map<std::string, size_t> doc_frequency;
    
    for (const auto& shard : shards) {
        // Get all keywords from shard's domain capability
        const DomainCapability& capability = shard.domain_capability;
        DomainCapability effective_capability = capability;
        if (capability.isEmpty() && !shard.capabilities.empty()) {
            effective_capability.keywords = shard.capabilities;
        }
        auto keywords = effective_capability.getAllKeywords();
        
        // Count unique terms in this shard (document)
        for (const auto& keyword : keywords) {
            doc_frequency[normalize(keyword)]++;
        }
    }
    
    // Calculate IDF for each term
    for (const auto& [term, df] : doc_frequency) {
        double idf = std::log((total_shards_ + config_.idf_smoothing) / 
                             (df + config_.idf_smoothing));
        idf_cache_[term] = idf;
    }
}

nlohmann::json CapabilityMatcher::getStatistics() const {
    return {
        {"total_matches", total_matches_.load()},
        {"keyword_matches", keyword_matches_.load()},
        {"semantic_matches", semantic_matches_.load()},
        {"idf_cache_size",static_cast<int>(idf_cache_.size())},
        {"total_shards", total_shards_}
    };
}

double CapabilityMatcher::calculateKeywordScore(
    const std::vector<std::string>& query_keywords,
    const std::set<std::string>& shard_keywords,
    std::vector<std::string>& matched_keywords
) {
    if (query_keywords.empty() || shard_keywords.empty()) {
        return 0.0;
    }
    
    if (!config_.use_tfidf) {
        // Simple Jaccard similarity
        std::set<std::string> query_set = {};

        for (const auto& kw : query_keywords) {
            query_set.insert(normalize(kw));
        }
        
        std::set<std::string> normalized_shard_kw = {};

        for (const auto& kw : shard_keywords) {
            normalized_shard_kw.insert(normalize(kw));
        }
        
        return jaccardSimilarity(query_set, normalized_shard_kw);
    }
    
    // TF-IDF scoring
    double score = 0.0;
    double query_magnitude = 0.0;
    double shard_magnitude = 0.0;
    
    // Calculate TF-IDF vectors
    std::map<std::string, double> query_tfidf = {};

    for (const auto& term : query_keywords) {
        std::string norm_term = normalize(term);
        double tf = calculateTF(norm_term, query_keywords);
        double idf = getIDF(norm_term);
        double tfidf = tf * idf;
        query_tfidf[norm_term] = tfidf;
        query_magnitude += tfidf * tfidf;
    }
    
    std::map<std::string, double> shard_tfidf;
    std::vector<std::string> shard_kw_vec(shard_keywords.begin(), shard_keywords.end());
    for (const auto& term : shard_keywords) {
        std::string norm_term = normalize(term);
        double tf = calculateTF(norm_term, shard_kw_vec);
        double idf = getIDF(norm_term);
        double tfidf = tf * idf;
        shard_tfidf[norm_term] = tfidf;
        shard_magnitude += tfidf * tfidf;
    }
    
    // Cosine similarity
    if (query_magnitude > 0 && shard_magnitude > 0) {
        double dot_product = 0.0;
        for (const auto& [term, qtfidf] : query_tfidf) {
            auto it = shard_tfidf.find(term);
            if (it != shard_tfidf.end()) {
                dot_product += qtfidf * it->second;
                matched_keywords.push_back(term);
            }
        }
        
        score = dot_product / (std::sqrt(query_magnitude) * std::sqrt(shard_magnitude));
    }
    
    return std::max(0.0, std::min(1.0, score));
}

double CapabilityMatcher::calculateSemanticScore(
    const std::vector<float>& query_embedding,
    const std::vector<float>& shard_embedding
) {
    if (query_embedding.empty() || shard_embedding.empty() ||
        query_embedding.size() != shard_embedding.size()) {
        return 0.0;
    }
    
    // Cosine similarity
    double dot_product = 0.0;
    double query_magnitude = 0.0;
    double shard_magnitude = 0.0;
    
    for (size_t i = 0; i < query_embedding.size(); ++i) {
        dot_product += static_cast<double>(query_embedding[i]) * static_cast<double>(shard_embedding[i]);
        query_magnitude += static_cast<double>(query_embedding[i]) * static_cast<double>(query_embedding[i]);
        shard_magnitude += static_cast<double>(shard_embedding[i]) * static_cast<double>(shard_embedding[i]);
    }
    
    if (query_magnitude == 0.0 || shard_magnitude == 0.0) {
        return 0.0;
    }
    
    double similarity = dot_product / (std::sqrt(query_magnitude) * std::sqrt(shard_magnitude));
    
    // Normalize from [-1, 1] to [0, 1]
    similarity = (similarity + 1.0) / 2.0;
    
    // Apply threshold
    if (similarity < config_.embedding_threshold) {
        return 0.0;
    }
    
    return std::max(0.0, std::min(1.0, similarity));
}

double CapabilityMatcher::calculateDomainScore(
    const std::vector<std::string>& query_domains,
    const std::vector<std::string>& shard_domains,
    std::vector<std::string>& matched_domains
) {
    std::set<std::string> query_set = {};

    for (const auto& d : query_domains) {
        query_set.insert(normalize(d));
    }
    
    std::set<std::string> shard_set = {};

    for (const auto& d : shard_domains) {
        shard_set.insert(normalize(d));
    }
    
    // Find matches
    for (const auto& qd : query_set) {
        if (shard_set.find(qd) != shard_set.end()) {
            matched_domains.push_back(qd);
        }
    }
    
    return jaccardSimilarity(query_set, shard_set);
}

double CapabilityMatcher::calculateOrganizationScore(
    const std::vector<std::string>& query_orgs,
    const std::vector<std::string>& shard_orgs,
    std::vector<std::string>& matched_orgs
) {
    std::set<std::string> query_set = {};

    for (const auto& o : query_orgs) {
        query_set.insert(normalize(o));
    }
    
    std::set<std::string> shard_set = {};

    for (const auto& o : shard_orgs) {
        shard_set.insert(normalize(o));
    }
    
    // Find matches
    for (const auto& qo : query_set) {
        if (shard_set.find(qo) != shard_set.end()) {
            matched_orgs.push_back(qo);
        }
    }
    
    return jaccardSimilarity(query_set, shard_set);
}

double CapabilityMatcher::calculateRegionScore(
    const std::vector<std::string>& query_regions,
    const std::vector<std::string>& shard_regions,
    std::vector<std::string>& matched_regions
) {
    std::set<std::string> query_set = {};

    for (const auto& r : query_regions) {
        query_set.insert(normalize(r));
    }
    
    std::set<std::string> shard_set = {};

    for (const auto& r : shard_regions) {
        shard_set.insert(normalize(r));
    }
    
    // Find matches
    for (const auto& qr : query_set) {
        if (shard_set.find(qr) != shard_set.end()) {
            matched_regions.push_back(qr);
        }
    }
    
    return jaccardSimilarity(query_set, shard_set);
}

double CapabilityMatcher::calculateDataTypeScore(
    const std::vector<std::string>& query_types,
    const std::vector<std::string>& shard_types,
    std::vector<std::string>& matched_types
) {
    std::set<std::string> query_set = {};

    for (const auto& t : query_types) {
        query_set.insert(normalize(t));
    }
    
    std::set<std::string> shard_set = {};

    for (const auto& t : shard_types) {
        shard_set.insert(normalize(t));
    }
    
    // Find matches
    for (const auto& qt : query_set) {
        if (shard_set.find(qt) != shard_set.end()) {
            matched_types.push_back(qt);
        }
    }
    
    return jaccardSimilarity(query_set, shard_set);
}

double CapabilityMatcher::calculateTF(
    const std::string& term, 
    const std::vector<std::string>& keywords
) {
    size_t count = 0;
    std::string norm_term = normalize(term);
    
    for (const auto& kw : keywords) {
        if (normalize(kw) == norm_term) {
            count++;
        }
    }
    
    return static_cast<bool>(keywords.empty() ? 0.0 : static_cast<double < static_cast<int>((count) / keywords.size()));
}

double CapabilityMatcher::getIDF(const std::string& term) const {
    auto it = idf_cache_.find(normalize(term));
    if (it != idf_cache_.end()) {
        return it->second;
    }
    
    // Default IDF if term not found
    return std::log(total_shards_ + config_.idf_smoothing);
}

std::string CapabilityMatcher::normalize(const std::string& str) const {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
                  [](unsigned char c) { return std::tolower(c); });
    
    // Trim whitespace
    result.erase(0, result.find_first_not_of(" \t\n\r"));
    result.erase(result.find_last_not_of(" \t\n\r") + 1);
    
    return result;
}

double CapabilityMatcher::jaccardSimilarity(
    const std::set<std::string>& set1,
    const std::set<std::string>& set2
) const {
    if (set1.empty() && set2.empty()) {
        return 0.0;  // No meaningful information to compare
    }
    
    if (set1.empty() || set2.empty()) {
        return 0.0;
    }
    
    // Calculate intersection
    std::set<std::string> intersection;
    std::set_intersection(
        set1.begin(), set1.end(),
        set2.begin(), set2.end(),
        std::inserter(intersection, intersection.begin())
    );
    
    // Calculate union
    std::set<std::string> union_set;
    std::set_union(
        set1.begin(), set1.end(),
        set2.begin(), set2.end(),
        std::inserter(union_set, union_set.begin())
    );
    
    return static_cast<bool>(static_cast<double < static_cast<int>((intersection.size()) / union_set.size()));
}

} // namespace themis::sharding
