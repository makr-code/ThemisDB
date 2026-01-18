/**
 * @file knowledge_gap_detector.cpp
 * @brief Implementation of Knowledge Gap Detector for RAG Systems
 */

#include "rag/knowledge_gap_detector.h"
#include "utils/logger.h"
#include <algorithm>
#include <numeric>
#include <cmath>

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
        result.explanation = "Insufficient number of documents retrieved";
        return result;
    }
    
    // Calculate average similarity
    result.avg_similarity_score = calculateAverageSimilarity(documents);
    
    if (result.avg_similarity_score < impl_->config.similarity_threshold) {
        result.gap_detected = true;
        result.gap_type = GapType::LOW_SIMILARITY;
        result.confidence_score = 0.85;
        result.recommendation = FallbackStrategy::REFORMULATE_QUERY;
        result.explanation = "Retrieved documents have low semantic similarity to query";
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
            result.explanation = "Query aspects not fully covered by documents";
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
    // Check for ethical perspective gap first if enabled
    if (impl_->config.enable_ethical_gap_detection) {
        auto ethical_result = detectEthicalPerspectiveGap(query, documents);
        if (ethical_result.gap_detected) {
            if (impl_->gap_callback) {
                impl_->gap_callback(ethical_result);
            }
            return ethical_result;
        }
    }
    
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
    
    return sum / docs.size();
}

double KnowledgeGapDetector::calculateQueryCoverage(
    const std::string& query,
    const std::vector<RetrievedDocument>& docs
) {
    // TODO: Implement semantic coverage analysis
    // For now, return a placeholder based on document count and similarity
    if (docs.empty()) {
        return 0.0;
    }
    
    double avg_similarity = calculateAverageSimilarity(docs);
    double doc_factor = std::min(1.0, docs.size() / 5.0);
    
    return avg_similarity * doc_factor;
}

std::vector<std::string> KnowledgeGapDetector::extractQueryAspects(
    const std::string& query
) {
    // TODO: Implement proper query aspect extraction
    // Placeholder implementation
    return {query};
}

std::vector<std::string> KnowledgeGapDetector::findMissingAspects(
    const std::string& query,
    const std::vector<RetrievedDocument>& docs
) {
    // TODO: Implement proper missing aspect detection
    // Placeholder implementation
    return {};
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
    // TODO: Implement claim extraction
    // Placeholder implementation
    return {};
}

bool KnowledgeGapDetector::verifyClaim(
    const std::string& claim,
    const std::vector<RetrievedDocument>& docs
) {
    // TODO: Implement claim verification
    // Placeholder implementation
    return true;
}

DetectionResult KnowledgeGapDetector::detectEthicalPerspectiveGap(
    const std::string& query,
    const std::vector<RetrievedDocument>& documents
) {
    THEMIS_DEBUG("Detecting ethical perspective gap for query: {}", query);
    
    DetectionResult result;
    result.num_retrieved_docs = documents.size();
    result.avg_similarity_score = calculateAverageSimilarity(documents);
    
    // Check if query has ethical context
    if (!isEthicalQuery(query)) {
        // Not an ethical query, no gap
        result.gap_detected = false;
        result.gap_type = GapType::NONE;
        result.confidence_score = 0.9;
        result.recommendation = FallbackStrategy::NONE;
        return result;
    }
    
    THEMIS_DEBUG("Ethical context detected in query");
    
    // Count ethical perspectives in documents
    int perspectives_found = countEthicalPerspectives(documents);
    
    // Calculate perspective diversity
    double diversity = calculatePerspectiveDiversity(documents);
    
    // Check if we have minimum required perspectives
    if (perspectives_found < static_cast<int>(impl_->config.min_ethical_perspectives) ||
        diversity < impl_->config.ethical_diversity_threshold) {
        
        result.gap_detected = true;
        result.gap_type = GapType::ETHICAL_PERSPECTIVE_GAP;
        result.confidence_score = 0.85;
        result.recommendation = FallbackStrategy::EXPAND_SEARCH;
        result.coverage_score = diversity;
        
        std::ostringstream explanation;
        explanation << "Ethical context detected in query, but insufficient "
                   << "perspective diversity in documents. Found " 
                   << perspectives_found << " perspectives (minimum: "
                   << impl_->config.min_ethical_perspectives << "), "
                   << "diversity score: " << diversity;
        result.explanation = explanation.str();
        
        result.missing_aspects.push_back("Diverse moral philosophical perspectives");
        result.missing_aspects.push_back("Multiple ethical frameworks representation");
        
        THEMIS_INFO("Ethical perspective gap detected: {} perspectives, diversity={}",
                   perspectives_found, diversity);
        
        return result;
    }
    
    // Sufficient ethical perspectives found
    result.gap_detected = false;
    result.gap_type = GapType::NONE;
    result.confidence_score = 0.8;
    result.recommendation = FallbackStrategy::NONE;
    result.coverage_score = diversity;
    
    return result;
}

bool KnowledgeGapDetector::isEthicalQuery(const std::string& query) {
    // Keywords that indicate ethical/moral queries
    std::vector<std::string> ethical_keywords = {
        "should", "ought", "moral", "ethical", "ethics",
        "right", "wrong", "good", "bad", "justice",
        "fair", "unfair", "virtue", "duty", "obligation",
        "value", "principle", "conscience", "responsibility"
    };
    
    std::string lower_query = query;
    std::transform(lower_query.begin(), lower_query.end(), 
                  lower_query.begin(), ::tolower);
    
    int keyword_count = 0;
    for (const auto& keyword : ethical_keywords) {
        if (lower_query.find(keyword) != std::string::npos) {
            keyword_count++;
        }
    }
    
    // Query is ethical if it contains N+ ethical keywords (configurable)
    return keyword_count >= impl_->config.ethical_keyword_threshold;
}

int KnowledgeGapDetector::countEthicalPerspectives(
    const std::vector<RetrievedDocument>& docs
) {
    // Moral frameworks to look for
    std::vector<std::string> frameworks = {
        "utilitarian", "consequentialist", "utility",
        "deontological", "kant", "duty", "categorical imperative",
        "virtue", "aristotle", "character",
        "rights", "human rights", "natural rights",
        "care ethics", "feminist ethics",
        "religious", "divine", "faith",
        "cultural", "relativism"
    };
    
    std::unordered_set<std::string> found_frameworks;
    
    for (const auto& doc : docs) {
        std::string lower_content = doc.content;
        std::transform(lower_content.begin(), lower_content.end(),
                      lower_content.begin(), ::tolower);
        
        for (const auto& framework : frameworks) {
            if (lower_content.find(framework) != std::string::npos) {
                // Group similar frameworks
                if (framework.find("utilitarian") != std::string::npos ||
                    framework.find("consequentialist") != std::string::npos ||
                    framework.find("utility") != std::string::npos) {
                    found_frameworks.insert("utilitarian");
                } else if (framework.find("deontological") != std::string::npos ||
                          framework.find("kant") != std::string::npos ||
                          framework.find("duty") != std::string::npos) {
                    found_frameworks.insert("deontological");
                } else if (framework.find("virtue") != std::string::npos ||
                          framework.find("aristotle") != std::string::npos ||
                          framework.find("character") != std::string::npos) {
                    found_frameworks.insert("virtue");
                } else if (framework.find("rights") != std::string::npos) {
                    found_frameworks.insert("rights-based");
                } else if (framework.find("care") != std::string::npos ||
                          framework.find("feminist") != std::string::npos) {
                    found_frameworks.insert("care-ethics");
                } else if (framework.find("religious") != std::string::npos ||
                          framework.find("divine") != std::string::npos ||
                          framework.find("faith") != std::string::npos) {
                    found_frameworks.insert("religious");
                } else if (framework.find("cultural") != std::string::npos ||
                          framework.find("relativism") != std::string::npos) {
                    found_frameworks.insert("cultural");
                }
            }
        }
    }
    
    return static_cast<int>(found_frameworks.size());
}

double KnowledgeGapDetector::calculatePerspectiveDiversity(
    const std::vector<RetrievedDocument>& docs
) {
    if (docs.empty()) {
        return 0.0;
    }
    
    // Count perspectives
    int perspectives = countEthicalPerspectives(docs);
    
    // Calculate diversity score based on number of perspectives
    // and their distribution across documents
    double base_score = std::min(1.0, perspectives / 3.0);
    
    // Bonus if perspectives are well-distributed across documents
    // (not all in one document)
    int docs_with_perspectives = 0;
    for (const auto& doc : docs) {
        std::string lower_content = doc.content;
        std::transform(lower_content.begin(), lower_content.end(),
                      lower_content.begin(), ::tolower);
        
        if (lower_content.find("ethic") != std::string::npos ||
            lower_content.find("moral") != std::string::npos ||
            lower_content.find("right") != std::string::npos ||
            lower_content.find("duty") != std::string::npos) {
            docs_with_perspectives++;
        }
    }
    
    double distribution_bonus = 
        std::min(0.2, docs_with_perspectives / static_cast<double>(docs.size()) * 0.2);
    
    return std::min(1.0, base_score + distribution_bonus);
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
