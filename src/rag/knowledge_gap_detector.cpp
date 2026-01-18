/**
 * @file knowledge_gap_detector.cpp
 * @brief Implementation of Knowledge Gap Detector for RAG Systems
 */

#include "rag/knowledge_gap_detector.h"
#include "utils/logger.h"
#include <algorithm>
#include <numeric>
#include <cmath>
#include <chrono>
#include <ctime>

namespace themis::rag::knowledge_gap {

// Private implementation details
struct KnowledgeGapDetector::Impl {
    KnowledgeGapConfig config;
    std::function<void(const DetectionResult&)> gap_callback;
    
    // Cache for performance
    std::unordered_map<std::string, DetectionResult> cache;
};

KnowledgeGapDetector::KnowledgeGapDetector(const KnowledgeGapConfig& config)
    : impl_(std::make_unique<Impl>()) {
    impl_->config = config;
    THEMIS_INFO("Knowledge Gap Detector initialized with mode: {}", 
                static_cast<int>(config.mode));
}

KnowledgeGapDetector::~KnowledgeGapDetector() = default;

DetectionResult KnowledgeGapDetector::detectPreGeneration(
    const std::string& query,
    const std::vector<RetrievedDocument>& documents
) {
    THEMIS_DEBUG("Pre-generation gap detection for query: {}", query);
    
    DetectionResult result;
    result.num_retrieved_docs = documents.size();
    
    // Check document count threshold
    if (documents.size() < impl_->config.min_documents) {
        result.gap_detected = true;
        result.gap_type = GapType::INSUFFICIENT_DOCS;
        result.confidence_score = 0.9;
        result.recommendation = FallbackStrategy::EXPAND_SEARCH;
        result.explanation = "Insufficient number of documents retrieved (found " + 
                            std::to_string(documents.size()) + ", need " + 
                            std::to_string(impl_->config.min_documents) + ")";
        return result;
    }
    
    // Check for outdated information in metadata
    size_t outdated_count = 0;
    for (const auto& doc : documents) {
        auto timestamp_it = doc.metadata.find("timestamp");
        if (timestamp_it != doc.metadata.end()) {
            try {
                // Parse timestamp (assuming ISO 8601 or epoch format)
                // Simplified: check if year < current_year - 2
                std::string ts = timestamp_it->second;
                if (!ts.empty() && ts.length() >= 4) {
                    int year = std::stoi(ts.substr(0, 4));
                    auto now = std::chrono::system_clock::now();
                    auto now_time = std::chrono::system_clock::to_time_t(now);
                    std::tm* now_tm = std::localtime(&now_time);
                    int current_year = now_tm->tm_year + 1900;
                    
                    if (year < current_year - 2) {
                        outdated_count++;
                    }
                }
            } catch (...) {
                // Ignore parsing errors
            }
        }
    }
    
    // If most documents are outdated, flag it
    if (documents.size() > 0 && 
        static_cast<double>(outdated_count) / documents.size() > 0.5) {
        result.gap_detected = true;
        result.gap_type = GapType::OUTDATED_INFO;
        result.confidence_score = 0.75;
        result.recommendation = FallbackStrategy::EXPAND_SEARCH;
        result.explanation = "Most retrieved documents contain outdated information (>2 years old)";
        return result;
    }
    
    // Calculate average similarity
    result.avg_similarity_score = calculateAverageSimilarity(documents);
    
    if (result.avg_similarity_score < impl_->config.similarity_threshold) {
        result.gap_detected = true;
        result.gap_type = GapType::LOW_SIMILARITY;
        result.confidence_score = 0.85;
        result.recommendation = FallbackStrategy::REFORMULATE_QUERY;
        result.explanation = "Retrieved documents have low semantic similarity to query (avg: " +
                            std::to_string(result.avg_similarity_score) + ", threshold: " +
                            std::to_string(impl_->config.similarity_threshold) + ")";
        return result;
    }
    
    // Calculate query coverage
    if (impl_->config.enable_query_aspect_analysis) {
        result.coverage_score = calculateQueryCoverage(query, documents);
        
        if (result.coverage_score < impl_->config.coverage_threshold) {
            result.gap_detected = true;
            result.gap_type = GapType::MISSING_ASPECTS;
            result.confidence_score = 0.75;
            result.recommendation = FallbackStrategy::MULTI_HOP_RETRIEVAL;
            result.missing_aspects = findMissingAspects(query, documents);
            result.explanation = "Query aspects not fully covered by documents (coverage: " +
                                std::to_string(result.coverage_score) + ", threshold: " +
                                std::to_string(impl_->config.coverage_threshold) + ")";
            return result;
        }
    }
    
    // No gap detected in pre-generation phase
    result.gap_detected = false;
    result.gap_type = GapType::NONE;
    result.confidence_score = 0.8;
    result.recommendation = FallbackStrategy::NONE;
    
    return result;
}

DetectionResult KnowledgeGapDetector::detectDuringGeneration(
    const std::string& query,
    const std::vector<RetrievedDocument>& documents,
    const GenerationContext& context
) {
    THEMIS_DEBUG("During-generation gap detection");
    
    DetectionResult result;
    result.num_retrieved_docs = documents.size();
    result.avg_similarity_score = calculateAverageSimilarity(documents);
    
    // Check token probability
    if (context.token_probability_avg < impl_->config.confidence_threshold) {
        result.gap_detected = true;
        result.gap_type = GapType::UNCERTAIN_GENERATION;
        result.confidence_score = 0.85;
        result.recommendation = FallbackStrategy::EXPAND_SEARCH;
        result.explanation = "Low confidence in generation based on token probabilities";
        return result;
    }
    
    // Check perplexity
    if (context.perplexity > 100.0) {  // High perplexity indicates uncertainty
        result.gap_detected = true;
        result.gap_type = GapType::UNCERTAIN_GENERATION;
        result.confidence_score = 0.8;
        result.recommendation = FallbackStrategy::MULTI_HOP_RETRIEVAL;
        result.explanation = "High perplexity indicates uncertain generation";
        return result;
    }
    
    result.gap_detected = false;
    result.gap_type = GapType::NONE;
    result.confidence_score = 0.75;
    result.recommendation = FallbackStrategy::NONE;
    
    return result;
}

DetectionResult KnowledgeGapDetector::detectPostGeneration(
    const std::string& query,
    const std::vector<RetrievedDocument>& documents,
    const std::string& generated_answer
) {
    THEMIS_DEBUG("Post-generation gap detection");
    
    DetectionResult result;
    result.num_retrieved_docs = documents.size();
    result.avg_similarity_score = calculateAverageSimilarity(documents);
    
    // TODO: Implement claim verification
    if (impl_->config.enable_claim_verification) {
        auto claims = extractClaims(generated_answer);
        size_t unverified_count = 0;
        
        for (const auto& claim : claims) {
            if (!verifyClaim(claim, documents)) {
                unverified_count++;
            }
        }
        
        if (claims.size() > 0 && 
            static_cast<double>(unverified_count) / claims.size() > 0.3) {
            result.gap_detected = true;
            result.gap_type = GapType::UNCERTAIN_GENERATION;
            result.confidence_score = 0.8;
            result.recommendation = FallbackStrategy::EXPAND_SEARCH;
            result.explanation = "Significant claims cannot be verified against documents";
            return result;
        }
    }
    
    // TODO: Implement self-consistency check
    if (impl_->config.enable_self_consistency_check) {
        bool is_consistent = checkSelfConsistency(query, documents);
        if (!is_consistent) {
            result.gap_detected = true;
            result.gap_type = GapType::CONFLICTING_INFO;
            result.confidence_score = 0.75;
            result.recommendation = FallbackStrategy::REFORMULATE_QUERY;
            result.explanation = "Generated answers lack self-consistency";
            return result;
        }
    }
    
    result.gap_detected = false;
    result.gap_type = GapType::NONE;
    result.confidence_score = 0.85;
    result.recommendation = FallbackStrategy::NONE;
    
    return result;
}

DetectionResult KnowledgeGapDetector::detectGap(
    const std::string& query,
    const std::vector<RetrievedDocument>& documents,
    const std::string& generated_answer,
    const GenerationContext& context
) {
    // Comprehensive detection based on mode
    switch (impl_->config.mode) {
        case DetectionMode::FAST:
            return detectPreGeneration(query, documents);
            
        case DetectionMode::BALANCED: {
            auto pre_result = detectPreGeneration(query, documents);
            if (pre_result.gap_detected) {
                return pre_result;
            }
            
            if (context.generation_started) {
                auto during_result = detectDuringGeneration(query, documents, context);
                if (during_result.gap_detected) {
                    return during_result;
                }
            }
            
            return pre_result;
        }
        
        case DetectionMode::THOROUGH: {
            auto pre_result = detectPreGeneration(query, documents);
            if (pre_result.gap_detected) {
                if (impl_->gap_callback) {
                    impl_->gap_callback(pre_result);
                }
                return pre_result;
            }
            
            if (context.generation_started) {
                auto during_result = detectDuringGeneration(query, documents, context);
                if (during_result.gap_detected) {
                    if (impl_->gap_callback) {
                        impl_->gap_callback(during_result);
                    }
                    return during_result;
                }
            }
            
            if (!generated_answer.empty()) {
                auto post_result = detectPostGeneration(query, documents, generated_answer);
                if (impl_->gap_callback && post_result.gap_detected) {
                    impl_->gap_callback(post_result);
                }
                return post_result;
            }
            
            return pre_result;
        }
    }
    
    return DetectionResult{};
}

void KnowledgeGapDetector::setConfig(const KnowledgeGapConfig& config) {
    impl_->config = config;
}

KnowledgeGapConfig KnowledgeGapDetector::getConfig() const {
    return impl_->config;
}

void KnowledgeGapDetector::setGapDetectionCallback(
    std::function<void(const DetectionResult&)> callback
) {
    impl_->gap_callback = std::move(callback);
}

// Private helper methods

double KnowledgeGapDetector::calculateAverageSimilarity(
    const std::vector<RetrievedDocument>& docs
) {
    if (docs.empty()) {
        return 0.0;
    }
    
    double sum = std::accumulate(docs.begin(), docs.end(), 0.0,
        [](double acc, const RetrievedDocument& doc) {
            return acc + doc.similarity_score;
        });
    
    double avg = sum / docs.size();
    
    // Ensure normalized to 0.0-1.0 range
    // If using cosine distance (1 - cosine_similarity), convert to similarity
    // Most vector stores return distance where smaller is better
    // For cosine: distance = 1 - similarity, so similarity = 1 - distance
    return std::clamp(avg, 0.0, 1.0);
}

double KnowledgeGapDetector::calculateQueryCoverage(
    const std::string& query,
    const std::vector<RetrievedDocument>& docs
) {
    // Basic coverage calculation based on:
    // 1. Average similarity score
    // 2. Document count factor
    // 3. Content diversity
    
    if (docs.empty()) {
        return 0.0;
    }
    
    double avg_similarity = calculateAverageSimilarity(docs);
    
    // Document count factor: more documents generally means better coverage
    // Asymptotic to 1.0, reaches 0.8 at 5 docs, 0.9 at 10 docs
    double doc_factor = 1.0 - std::exp(-0.3 * docs.size());
    
    // Content diversity: check if documents have varied content
    // Simple heuristic: check length variance
    double diversity_score = 1.0;
    if (docs.size() > 1) {
        double avg_length = 0.0;
        for (const auto& doc : docs) {
            avg_length += doc.content.length();
        }
        avg_length /= docs.size();
        
        double variance = 0.0;
        for (const auto& doc : docs) {
            double diff = doc.content.length() - avg_length;
            variance += diff * diff;
        }
        variance /= docs.size();
        
        // Normalize diversity: higher variance = more diverse
        // Cap at reasonable levels
        diversity_score = std::min(1.0, 0.7 + variance / (avg_length * avg_length) * 0.3);
    }
    
    // Combine factors with weights
    return avg_similarity * 0.6 + doc_factor * 0.3 + diversity_score * 0.1;
}

std::vector<std::string> KnowledgeGapDetector::extractQueryAspects(
    const std::string& query
) {
    // Basic query aspect extraction using simple heuristics
    // In a full implementation, this would use NLP/NER
    
    std::vector<std::string> aspects;
    
    if (query.empty()) {
        return aspects;
    }
    
    // Split by common delimiters and extract key phrases
    std::string current_aspect;
    bool in_word = false;
    
    for (char c : query) {
        if (std::isalnum(c) || c == '_' || c == '-') {
            current_aspect += c;
            in_word = true;
        } else {
            if (in_word && current_aspect.length() > 2) {
                // Only add meaningful words (length > 2)
                aspects.push_back(current_aspect);
            }
            current_aspect.clear();
            in_word = false;
        }
    }
    
    // Add last aspect if any
    if (!current_aspect.empty() && current_aspect.length() > 2) {
        aspects.push_back(current_aspect);
    }
    
    // Remove duplicates
    std::sort(aspects.begin(), aspects.end());
    aspects.erase(std::unique(aspects.begin(), aspects.end()), aspects.end());
    
    // If no aspects found, use whole query
    if (aspects.empty()) {
        aspects.push_back(query);
    }
    
    return aspects;
}

std::vector<std::string> KnowledgeGapDetector::findMissingAspects(
    const std::string& query,
    const std::vector<RetrievedDocument>& docs
) {
    // Basic missing aspect detection
    // Checks which query aspects are not well-covered by documents
    
    std::vector<std::string> missing;
    auto query_aspects = extractQueryAspects(query);
    
    if (docs.empty()) {
        return query_aspects; // All aspects are missing
    }
    
    // Concatenate all document content for searching
    std::string all_content;
    for (const auto& doc : docs) {
        all_content += doc.content + " ";
    }
    
    // Convert to lowercase for case-insensitive matching
    std::transform(all_content.begin(), all_content.end(), 
                   all_content.begin(), ::tolower);
    
    // Check each aspect
    for (const auto& aspect : query_aspects) {
        std::string aspect_lower = aspect;
        std::transform(aspect_lower.begin(), aspect_lower.end(),
                      aspect_lower.begin(), ::tolower);
        
        // If aspect not found in any document, mark as missing
        if (all_content.find(aspect_lower) == std::string::npos) {
            missing.push_back(aspect);
        }
    }
    
    return missing;
}

bool KnowledgeGapDetector::checkSelfConsistency(
    const std::string& query,
    const std::vector<RetrievedDocument>& docs
) {
    // TODO: Implement self-consistency check
    // Placeholder implementation
    return true;
}

std::vector<std::string> KnowledgeGapDetector::extractClaims(
    const std::string& answer
) {
    // Basic claim extraction by splitting on sentence boundaries
    // In a full implementation, this would use NLP for proper claim extraction
    
    std::vector<std::string> claims;
    
    if (answer.empty()) {
        return claims;
    }
    
    std::string current_claim;
    
    for (size_t i = 0; i < answer.length(); ++i) {
        char c = answer[i];
        current_claim += c;
        
        // Check for sentence endings
        if (c == '.' || c == '!' || c == '?') {
            // Look ahead to avoid abbreviations (e.g., "Dr.")
            if (i + 1 < answer.length() && std::isspace(answer[i + 1])) {
                // Trim whitespace
                size_t start = current_claim.find_first_not_of(" \t\n\r");
                size_t end = current_claim.find_last_not_of(" \t\n\r");
                
                if (start != std::string::npos && end != std::string::npos) {
                    std::string claim = current_claim.substr(start, end - start + 1);
                    if (claim.length() > 10) { // Only meaningful claims
                        claims.push_back(claim);
                    }
                }
                current_claim.clear();
            }
        }
    }
    
    // Add last claim if any
    if (!current_claim.empty()) {
        size_t start = current_claim.find_first_not_of(" \t\n\r");
        size_t end = current_claim.find_last_not_of(" \t\n\r");
        if (start != std::string::npos && end != std::string::npos) {
            std::string claim = current_claim.substr(start, end - start + 1);
            if (claim.length() > 10) {
                claims.push_back(claim);
            }
        }
    }
    
    return claims;
}

bool KnowledgeGapDetector::verifyClaim(
    const std::string& claim,
    const std::vector<RetrievedDocument>& docs
) {
    // Basic claim verification using substring matching
    // In a full implementation, this would use semantic similarity
    
    if (claim.empty() || docs.empty()) {
        return false;
    }
    
    // Extract key terms from claim (simple word extraction)
    std::vector<std::string> claim_terms;
    std::string current_term;
    
    for (char c : claim) {
        if (std::isalnum(c) || c == '_') {
            current_term += std::tolower(c);
        } else if (!current_term.empty()) {
            if (current_term.length() > 3) { // Only significant terms
                claim_terms.push_back(current_term);
            }
            current_term.clear();
        }
    }
    
    if (!current_term.empty() && current_term.length() > 3) {
        claim_terms.push_back(current_term);
    }
    
    if (claim_terms.empty()) {
        return true; // No specific claims to verify
    }
    
    // Check how many terms are found in documents
    size_t terms_found = 0;
    
    for (const auto& doc : docs) {
        std::string content_lower = doc.content;
        std::transform(content_lower.begin(), content_lower.end(),
                      content_lower.begin(), ::tolower);
        
        for (const auto& term : claim_terms) {
            if (content_lower.find(term) != std::string::npos) {
                terms_found++;
                break; // Count each term only once per document
            }
        }
    }
    
    // Verify if at least 60% of claim terms are found
    double verification_ratio = static_cast<double>(terms_found) / claim_terms.size();
    return verification_ratio >= 0.6;
}

// Factory implementations

std::unique_ptr<KnowledgeGapDetector> KnowledgeGapDetectorFactory::createFast() {
    KnowledgeGapConfig config;
    config.mode = DetectionMode::FAST;
    config.enable_self_consistency_check = false;
    config.enable_claim_verification = false;
    config.enable_query_aspect_analysis = false;
    return std::make_unique<KnowledgeGapDetector>(config);
}

std::unique_ptr<KnowledgeGapDetector> KnowledgeGapDetectorFactory::createBalanced() {
    KnowledgeGapConfig config;
    config.mode = DetectionMode::BALANCED;
    config.enable_query_aspect_analysis = true;
    return std::make_unique<KnowledgeGapDetector>(config);
}

std::unique_ptr<KnowledgeGapDetector> KnowledgeGapDetectorFactory::createThorough() {
    KnowledgeGapConfig config;
    config.mode = DetectionMode::THOROUGH;
    config.enable_self_consistency_check = true;
    config.enable_claim_verification = true;
    config.enable_query_aspect_analysis = true;
    return std::make_unique<KnowledgeGapDetector>(config);
}

std::unique_ptr<KnowledgeGapDetector> KnowledgeGapDetectorFactory::create(
    const KnowledgeGapConfig& config
) {
    return std::make_unique<KnowledgeGapDetector>(config);
}

} // namespace themis::rag::knowledge_gap
