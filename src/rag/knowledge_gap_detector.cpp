/**
 * @file knowledge_gap_detector.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 87/100
 * @note Gap Summary: total=5; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=1, Debt=0, C=1, H=22, M=22, L=1
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "rag/knowledge_gap_detector.h"
#include <cctype>
#include <map>
#include <stdexcept>
#include "utils/logger.h"
#include <algorithm>
#include <numeric>
#include <cmath>
#include <chrono>
#include <ctime>
#include <set>
#include <shared_mutex>
#include <unordered_set>
#include <sstream>

namespace themis::rag::knowledge_gap {

// Private implementation details
struct KnowledgeGapDetector::Impl {
    KnowledgeGapConfig config;
    std::function<void(const DetectionResult&)> gap_callback;
    RetrievalCallback retrieval_fn;   ///< FLARE dynamic-retrieval callback (optional)
    LlmSampleFn       llm_sample_fn; ///< LLM-based self-consistency sample generator (optional)
    ClaimVerificationFn claim_verification_fn; ///< Runtime claim verifier for C1 checks (optional)

    // Cache for performance
    std::unordered_map<std::string, DetectionResult> cache;
    mutable std::shared_mutex mu;

    KnowledgeGapConfig snapshotConfig() const {
        std::shared_lock<std::shared_mutex> lock(mu);
        return config;
    }

    std::function<void([[maybe_unused]] const DetectionResult&)> snapshotGapCallback() const {
        std::shared_lock<std::shared_mutex> lock(mu);
        return gap_callback;
    }

    RetrievalCallback snapshotRetrievalCallback() const {
        std::shared_lock<std::shared_mutex> lock(mu);
        return retrieval_fn;
    }

    LlmSampleFn snapshotLlmSampleFn() const {
        std::shared_lock<std::shared_mutex> lock(mu);
        return llm_sample_fn;
    }

    ClaimVerificationFn snapshotClaimVerificationFn() const {
        std::shared_lock<std::shared_mutex> lock(mu);
        return claim_verification_fn;
    }

    void setConfig(const KnowledgeGapConfig& new_config) {
        std::unique_lock<std::shared_mutex> lock(mu);
        config = new_config;
    }

    void setGapCallback(std::function<void(const DetectionResult&)> cb) {
        std::unique_lock<std::shared_mutex> lock(mu);
        gap_callback = std::move([[maybe_unused]] cb);
    }

    void setRetrievalCallback([[maybe_unused]] RetrievalCallback fn) {
        std::unique_lock<std::shared_mutex> lock(mu);
        retrieval_fn = std::move(fn);
    }

    void setLlmSampleFn(LlmSampleFn fn) {
        std::unique_lock<std::shared_mutex> lock(mu);
        llm_sample_fn = std::move(fn);
    }

    void setClaimVerificationFn(ClaimVerificationFn fn) {
        std::unique_lock<std::shared_mutex> lock(mu);
        claim_verification_fn = std::move(fn);
    }
};

KnowledgeGapDetector::KnowledgeGapDetector()
    : KnowledgeGapDetector(KnowledgeGapConfig{}) {
}

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

    const auto config = impl_->snapshotConfig();
    DetectionResult result;
    result.num_retrieved_docs = documents.size();

    // Check document count threshold
    if (static_cast<int>(documents.size()) < config.min_documents) {
        result.gap_detected = true;
        result.gap_type = GapType::INSUFFICIENT_DOCS;
        result.confidence_score = 0.9;
        result.recommendation = FallbackStrategy::EXPAND_SEARCH;
        result.explanation = "Insufficient number of documents retrieved (found " +
                            std::to_string(documents.size()) + ", need " +
                            std::to_string(config.min_documents) + ")";
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

                    // Thread-safe time conversion
                    #if defined(_WIN32) || defined(_WIN64)
                        std::tm now_tm_storage = {};
                        std::tm* now_tm = &now_tm_storage;
                        localtime_s(now_tm, &now_time);
                    #else
                        std::tm now_tm_storage = {};
                        std::tm* now_tm = localtime_r(&now_time, &now_tm_storage);
                        if (!now_tm) {
                            continue; // Skip on error
                        }
                    #endif

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
    if (static_cast<int>(documents.size()) > 0 &&
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

    if (result.avg_similarity_score < config.similarity_threshold) {
        result.gap_detected = true;
        result.gap_type = GapType::LOW_SIMILARITY;
        result.confidence_score = 0.85;
        result.recommendation = FallbackStrategy::REFORMULATE_QUERY;
        result.explanation = "Retrieved documents have low semantic similarity to query (avg: " +
                            std::to_string(result.avg_similarity_score) + ", threshold: " +
                            std::to_string(config.similarity_threshold) + ")";
        return result;
    }

    // Calculate query coverage
    if (config.enable_query_aspect_analysis) {
        result.coverage_score = calculateQueryCoverage(query, documents);

        if (result.coverage_score < config.coverage_threshold) {
            result.gap_detected = true;
            result.gap_type = GapType::MISSING_ASPECTS;
            result.confidence_score = 0.75;
            result.recommendation = FallbackStrategy::MULTI_HOP_RETRIEVAL;
            result.missing_aspects = findMissingAspects(query, documents);
            result.explanation = "Query aspects not fully covered by documents (coverage: " +
                                std::to_string(result.coverage_score) + ", threshold: " +
                                std::to_string(config.coverage_threshold) + ")";
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
    [[maybe_unused]] const std::string& query,
    const std::vector<RetrievedDocument>& documents,
    const GenerationContext& context
) {
    THEMIS_DEBUG("During-generation gap detection");

    const auto config = impl_->snapshotConfig();
    DetectionResult result;
    result.num_retrieved_docs = documents.size();
    result.avg_similarity_score = calculateAverageSimilarity(documents);

    // Phase 2: Enhanced token probability tracking
    if (config.enable_token_probability && !context.token_probs.empty()) {
        // Calculate confidence score with outlier removal
        double confidence = calculateConfidenceScore(context.token_probs);

        if (confidence < config.confidence_threshold) {
            result.gap_detected = true;
            result.gap_type = GapType::UNCERTAIN_GENERATION;
            result.confidence_score = 0.85;
            result.recommendation = FallbackStrategy::EXPAND_SEARCH;
            result.explanation = "Low confidence in generation based on token probabilities (confidence: " +
                               std::to_string(confidence) + ")";
            return result;
        }

        // Phase 2: Sliding window perplexity analysis
        double sliding_perplexity = calculateSlidingWindowPerplexity(
            context.token_probs,
            config.perplexity_window_size
        );

        if (detectPerplexityAnomaly(sliding_perplexity, config.perplexity_threshold)) {
            result.gap_detected = true;
            result.gap_type = GapType::UNCERTAIN_GENERATION;
            result.confidence_score = 0.8;
            result.recommendation = FallbackStrategy::MULTI_HOP_RETRIEVAL;
            result.explanation = "High perplexity indicates uncertain generation (perplexity: " +
                               std::to_string(sliding_perplexity) + ", threshold: " +
                               std::to_string(config.perplexity_threshold) + ")";
            return result;
        }
    }

    // Fallback to legacy checks if token_probs not available
    if (context.token_probability_avg < config.confidence_threshold) {
        result.gap_detected = true;
        result.gap_type = GapType::UNCERTAIN_GENERATION;
        result.confidence_score = 0.85;
        result.recommendation = FallbackStrategy::EXPAND_SEARCH;
        result.explanation = "Low confidence in generation based on token probabilities";
        return result;
    }

    // Check perplexity (legacy)
    if (context.perplexity > config.perplexity_threshold) {
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

    const auto config = impl_->snapshotConfig();
    DetectionResult result;
    result.num_retrieved_docs = documents.size();
    result.avg_similarity_score = calculateAverageSimilarity(documents);

    // Claim verification: check whether significant claims in the answer
    // are supported by the retrieved documents using term-overlap heuristic.
    if (config.enable_claim_verification) {
        auto claims = extractClaims(generated_answer);
        size_t unverified_count = 0;

        for (const auto& claim : claims) {
            if (!verifyClaim(claim, documents)) {
                unverified_count++;
            }
        }

        if (static_cast<int>(claims.size()) > 0 &&
            static_cast<double>(unverified_count) / claims.size() > 0.3) {
            result.gap_detected = true;
            result.gap_type = GapType::UNCERTAIN_GENERATION;
            result.confidence_score = 0.8;
            result.recommendation = FallbackStrategy::EXPAND_SEARCH;
            result.explanation = "Significant claims cannot be verified against documents";
            return result;
        }
    }

    // Self-consistency check: detect conflicting information across
    // multiple candidate generations using the configured consistency threshold.
    if (config.enable_self_consistency_check) {
        if (!checkSelfConsistency(query, documents)) {
            result.gap_detected = true;
            result.gap_type = GapType::CONFLICTING_INFO;
            result.confidence_score = 0.75;
            result.recommendation = FallbackStrategy::EXPAND_SEARCH;
            result.explanation = "Heuristic self-consistency check detected low agreement "
                                 "across document-derived answer samples";
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
    const std::string& generated_answer
) {
    return detectGap(query, documents, generated_answer, GenerationContext{});
}

DetectionResult KnowledgeGapDetector::detectGap(
    const std::string& query,
    const std::vector<RetrievedDocument>& documents,
    const std::string& generated_answer,
    const GenerationContext& context
) {
    try {
        const auto config = impl_->snapshotConfig();
        const auto gap_callback = impl_->snapshotGapCallback();

        bool ethical_gap_detected = false;
        DetectionResult ethical_result = {};
        if (config.enable_ethical_gap_detection) {
            ethical_result = detectEthicalPerspectiveGap(query, documents);
            if (ethical_result.gap_detected) {
                ethical_gap_detected = true;
                if ([[maybe_unused]] gap_callback) {
                    gap_callback([[maybe_unused]] ethical_result);
                }
                // Continue with objective coverage/similarity checks below.
            }
        }

        switch (config.mode) {
            case DetectionMode::FAST: {
                auto pre_result = detectPreGeneration(query, documents);
                if (ethical_gap_detected && !pre_result.gap_detected) {
                    return ethical_result;
                }
                return pre_result;
            }

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

                if (ethical_gap_detected) {
                    return ethical_result;
                }

                return pre_result;
            }

            case DetectionMode::THOROUGH: {
                auto pre_result = detectPreGeneration(query, documents);
                if (pre_result.gap_detected) {
                    if ([[maybe_unused]] gap_callback) {
                        gap_callback([[maybe_unused]] pre_result);
                    }
                    return pre_result;
                }

                if (context.generation_started) {
                    auto during_result = detectDuringGeneration(query, documents, context);
                    if (during_result.gap_detected) {
                        if ([[maybe_unused]] gap_callback) {
                            gap_callback([[maybe_unused]] during_result);
                        }
                        return during_result;
                    }
                }

                if (!generated_answer.empty()) {
                    auto post_result = detectPostGeneration(query, documents, generated_answer);
                    if ([[maybe_unused]] gap_callback && post_result.gap_detected) {
                        gap_callback([[maybe_unused]] post_result);
                    }
                    return post_result;
                }

                if (ethical_gap_detected) {
                    return ethical_result;
                }

                return pre_result;
            }

            default:
                break;
        }

        return ethical_gap_detected ? ethical_result : DetectionResult{.gap_detected = false, .confidence_score = 0.0, .gap_type = GapType::NONE, .missing_aspects = {}, .recommendation = FallbackStrategy::NONE, .explanation = {}, .avg_similarity_score = 0.0, .num_retrieved_docs = 0, .coverage_score = 0.0};
    } catch (const std::exception& e) {
        // HIGH FIX: Exception guard on main detection path to prevent incomplete state
        THEMIS_WARN("detectGap: detection failed with exception: {}", e.what());
        // Return safe no-gap result rather than propagating exception
        return DetectionResult{.gap_detected = false, .confidence_score = 0.0, .gap_type = GapType::NONE, .missing_aspects = {}, .recommendation = FallbackStrategy::NONE, .explanation = {}, .avg_similarity_score = 0.0, .num_retrieved_docs = 0, .coverage_score = 0.0};
    } catch (...) {
        // HIGH FIX: Catch-all for unknown exceptions
        THEMIS_WARN("detectGap: detection failed with unknown exception");
        return DetectionResult{.gap_detected = false, .confidence_score = 0.0, .gap_type = GapType::NONE, .missing_aspects = {}, .recommendation = FallbackStrategy::NONE, .explanation = {}, .avg_similarity_score = 0.0, .num_retrieved_docs = 0, .coverage_score = 0.0};
    }
}

DetectionResult KnowledgeGapDetector::detectWithActiveRetrieval(
    const std::string& query,
    std::vector<RetrievedDocument>& initial_documents,
    const std::string& tenant_id
) {
    try {
        const auto config = impl_->snapshotConfig();

        if (!config.enable_flare) {
            return detectPreGeneration(query, initial_documents);
        }

        THEMIS_DEBUG("FLARE active retrieval for query: {}", query);

        DetectionResult result;
        result.num_retrieved_docs = initial_documents.size();

        auto current_documents = initial_documents;
        size_t retrieval_round = 0;

        while (retrieval_round < config.max_retrieval_rounds) {
            const double coverage = calculateQueryCoverage(query, current_documents);
            const double avg_similarity = calculateAverageSimilarity(current_documents);

            THEMIS_DEBUG("Round {}: coverage={}, similarity={}",
                        retrieval_round, coverage, avg_similarity);

            if (coverage >= config.coverage_threshold &&
                avg_similarity >= config.similarity_threshold) {
                result.gap_detected = false;
                result.gap_type = GapType::NONE;
                result.confidence_score = 0.9;
                result.coverage_score = coverage;
                result.avg_similarity_score = avg_similarity;
                result.num_retrieved_docs = current_documents.size();
                result.recommendation = FallbackStrategy::NONE;
                result.explanation = "Sufficient information after " +
                                    std::to_string(retrieval_round) +
                                    " retrieval rounds";

                initial_documents = current_documents;
                return result;
            }

            auto missing = findMissingAspects(query, current_documents);
            if (missing.empty()) {
                break;
            }

            const std::string reformulated = reformulateQuery(query, missing[0]);
            THEMIS_DEBUG("Reformulated query: {}", reformulated);

            auto new_documents = performDynamicRetrieval(reformulated, tenant_id);

            for (auto& new_doc : new_documents) {
                bool is_duplicate = false;
                for (const auto& existing : current_documents) {
                    if (existing.id == new_doc.id) {
                        is_duplicate = true;
                        break;
                    }
                }

                if (!is_duplicate) {
                    current_documents.push_back(new_doc);
                }
            }

            ++retrieval_round;

            if (new_documents.empty()) {
                THEMIS_DEBUG("No new documents retrieved, stopping");
                break;
            }
        }

        const double final_coverage = calculateQueryCoverage(query, current_documents);
        const double final_similarity = calculateAverageSimilarity(current_documents);

        result.coverage_score = final_coverage;
        result.avg_similarity_score = final_similarity;
        result.num_retrieved_docs = current_documents.size();

        if (final_coverage < config.coverage_threshold) {
            result.gap_detected = true;
            result.gap_type = GapType::MISSING_ASPECTS;
            result.confidence_score = 0.7;
            result.recommendation = FallbackStrategy::INSUFFICIENT_DATA_RESPONSE;
            result.missing_aspects = findMissingAspects(query, current_documents);
            result.explanation = "Insufficient coverage after " +
                               std::to_string(retrieval_round) +
                               " retrieval rounds (coverage: " +
                               std::to_string(final_coverage) + ")";
        } else if (final_similarity < config.similarity_threshold) {
            result.gap_detected = true;
            result.gap_type = GapType::LOW_SIMILARITY;
            result.confidence_score = 0.75;
            result.recommendation = FallbackStrategy::REFORMULATE_QUERY;
            result.explanation = "Low similarity after active retrieval (similarity: " +
                               std::to_string(final_similarity) + ")";
        } else {
            result.gap_detected = false;
            result.gap_type = GapType::NONE;
            result.confidence_score = 0.85;
            result.recommendation = FallbackStrategy::NONE;
            result.explanation = "Acceptable information after active retrieval";
        }

        initial_documents = current_documents;
        return result;
    } catch (const std::exception& e) {
        THEMIS_WARN("detectWithActiveRetrieval: active retrieval failed: {}", e.what());
        return DetectionResult{.gap_detected = false, .confidence_score = 0.0, .gap_type = GapType::NONE, .missing_aspects = {}, .recommendation = FallbackStrategy::NONE, .explanation = {}, .avg_similarity_score = 0.0, .num_retrieved_docs = 0, .coverage_score = 0.0};
    } catch (...) {
        THEMIS_WARN("detectWithActiveRetrieval: active retrieval failed with unknown exception");
        return DetectionResult{.gap_detected = false, .confidence_score = 0.0, .gap_type = GapType::NONE, .missing_aspects = {}, .recommendation = FallbackStrategy::NONE, .explanation = {}, .avg_similarity_score = 0.0, .num_retrieved_docs = 0, .coverage_score = 0.0};
    }
}

void KnowledgeGapDetector::setConfig(const KnowledgeGapConfig& config) {
    impl_->setConfig(config);
}

KnowledgeGapConfig KnowledgeGapDetector::getConfig() const {
    return impl_->snapshotConfig();
}

void KnowledgeGapDetector::setGapDetectionCallback(
    std::function<void(const DetectionResult&)> callback
) {
    impl_->setGapCallback([[maybe_unused]] std::move(callback));
}

void KnowledgeGapDetector::setRetrievalCallback([[maybe_unused]] RetrievalCallback fn) {
    impl_->setRetrievalCallback([[maybe_unused]] std::move(fn));
}

void KnowledgeGapDetector::setLlmSampleFn(LlmSampleFn fn) {
    impl_->setLlmSampleFn(std::move(fn));
}

void KnowledgeGapDetector::setClaimVerificationFn(ClaimVerificationFn fn) {
    impl_->setClaimVerificationFn(std::move(fn));
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
    // Note: Similarity scores should already be normalized when creating RetrievedDocument
    // This clamp is a safety measure for edge cases
    return std::clamp(avg, 0.0, 1.0);
}

double KnowledgeGapDetector::calculateQueryCoverage(
    [[maybe_unused]] const std::string& query,
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
    if (static_cast<int>(docs.size()) > 1) {
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
    std::string current_aspect = {};
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
    std::string all_content = {};
    for (const auto& doc : docs) {
        all_content += doc.content + " ";
    }

    // Convert to lowercase for case-insensitive matching
    std::transform(all_content.begin(), all_content.end(),
                   all_content.begin(),
                   [](unsigned char c){ return std::tolower(c); });

    // Check each aspect
    for (const auto& aspect : query_aspects) {
        std::string aspect_lower = aspect;
        std::transform(aspect_lower.begin(), aspect_lower.end(),
                      aspect_lower.begin(),
                      [](unsigned char c){ return std::tolower(c); });

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
    // Phase 2: Implement self-consistency check with multiple sampling
    const auto config = impl_->snapshotConfig();

    if (!config.enable_self_consistency_check) {
        // Self-consistency checking is disabled in configuration.
        // Return true to indicate consistency by default, allowing generation to proceed.
        return true;
    }

    // Generate multiple samples with different seeds/temperatures
    auto samples = generateMultipleSamples(
        query,
        docs,
        config.self_consistency_samples
    );

    if (static_cast<int>(samples.size()) < 2) {
        return true; // Not enough samples to check consistency
    }

    // Calculate consistency score
    double consistency_score = calculateConsistencyScore(samples);

    THEMIS_DEBUG("Self-consistency score: {}", consistency_score);

    // Check for contradictions
    for (size_t i = 0; i <static_cast<int>(samples.size()); ++i) {
        for (size_t j = i + 1; j <static_cast<int>(samples.size()); ++j) {
            if (detectContradiction(samples[i], samples[j])) {
                THEMIS_DEBUG("Contradiction detected between samples {} and {}", i, j);
                return false;
            }
        }
    }

    // Check if consistency meets threshold
    return consistency_score >= config.consistency_threshold;
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

    std::string current_claim = {};

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
    const auto claim_verification_fn = impl_->snapshotClaimVerificationFn();
    if (claim_verification_fn) {
        try {
            return claim_verification_fn(claim, docs);
        } catch (const std::exception& e) {
            THEMIS_WARN("ClaimVerificationFn threw exception, falling back to heuristic: {}", e.what());
        } catch (...) {
            THEMIS_WARN("ClaimVerificationFn threw unknown exception, falling back to heuristic");
        }
    }

    // Basic claim verification using substring matching
    // In a full implementation, this would use semantic similarity

    if (claim.empty() || docs.empty()) {
        return false;
    }

    // Extract key terms from claim (simple word extraction)
    std::vector<std::string> claim_terms;
    std::string current_term = {};

    for (char c : claim) {
        if (std::isalnum(static_cast<unsigned char>(c)) || c == '_') {
            current_term += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
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
        // F5-3 fix: fail-closed — empty term list (stop-word-only or very short claim)
        // cannot be verified against documents. Returning true here would pass any
        // short sentence through verification unconditionally.
        THEMIS_DEBUG("verifyClaim: empty term list after stop-word removal — "
                     "claim '{}' cannot be verified; treating as unverified.", claim);
        return false;
    }

    // Check how many terms are found in documents
    // Optimization: Convert to set for O(1) lookup and parse words once per document
    // Complexity: O(n_docs × n_content) instead of O(n_docs × n_terms × n_content)
    size_t terms_found = 0;
    
    // Pre-build set of claim terms for O(1) lookup
    std::set<std::string> term_set(claim_terms.begin(), claim_terms.end());

    for (const auto& doc : docs) {
        std::string content_lower = doc.content;
        std::transform(content_lower.begin(), content_lower.end(),
                      content_lower.begin(),
                      [](unsigned char c){ return std::tolower(c); });
        
        // Parse content into words and check membership in term set
        // Early exit when we find a matching term (O(log n_terms) per word)
        std::istringstream stream(content_lower);
        std::string word = {};
        bool found_any = false;
        
        while (stream >> word && !found_any) {
            // Remove punctuation from word end for better matching
            while (!word.empty() && (word.back() < 'a' || word.back() > 'z')) {
                word.pop_back();
            }
            if (term_set.count(word)) {
                terms_found++;
                found_any = true;
            }
        }
    }

    // Verify if at least 60% of claim terms are found
    double verification_ratio = static_cast<double>(terms_found) / claim_terms.size();
    return verification_ratio >= 0.6;
}

// ============================================================================
// Phase 2: Token Probability & Perplexity Methods
// ============================================================================

double KnowledgeGapDetector::calculatePerplexity(
    const std::vector<double>& token_probs
) {
    if (token_probs.empty()) {
        return 0.0;
    }

    // Perplexity = exp(-1/N * sum(log(p_i)))
    // where p_i is the probability of token i
    double log_sum = 0.0;
    size_t valid_tokens = 0;

    for (double prob : token_probs) {
        if (prob > 0.0 && prob <= 1.0) {
            log_sum += std::log(prob);
            valid_tokens++;
        }
    }

    if (valid_tokens == 0) {
        return 0.0;
    }

    double avg_log_prob = log_sum / valid_tokens;
    double perplexity = std::exp(-avg_log_prob);

    return perplexity;
}

double KnowledgeGapDetector::calculateSlidingWindowPerplexity(
    const std::vector<double>& token_probs,
    size_t window_size
) {
    if (token_probs.empty() || window_size == 0) {
        return 0.0;
    }

    // Calculate perplexity for sliding windows and return max
    double max_perplexity = 0.0;

    for (size_t i = 0; i + window_size <= token_probs.size(); ++i) {
        std::vector<double> window(
            token_probs.begin() + i,
            token_probs.begin() + i + window_size
        );

        double window_perplexity = calculatePerplexity(window);
        max_perplexity = std::max(max_perplexity, window_perplexity);
    }

    // If sequence shorter than window, use full sequence
    if (static_cast<int>(token_probs.size()) < window_size) {
        max_perplexity = calculatePerplexity(token_probs);
    }

    return max_perplexity;
}

bool KnowledgeGapDetector::detectPerplexityAnomaly(
    double perplexity,
    double threshold
) {
    return perplexity > threshold;
}

double KnowledgeGapDetector::calculateConfidenceScore(
    const std::vector<double>& token_probs
) {
    if (token_probs.empty()) {
        return 0.0;
    }

    const auto config = impl_->snapshotConfig();
    // Remove outliers before calculating confidence
    auto cleaned_probs = removeOutlierTokens(
        token_probs,
        config.outlier_zscore_threshold
    );

    if (cleaned_probs.empty()) {
        return 0.0;
    }

    // Calculate weighted average (geometric mean for probabilities)
    double log_sum = 0.0;
    for (double prob : cleaned_probs) {
        if (prob > 0.0) {
            log_sum += std::log(prob);
        }
    }

    double confidence = std::exp(log_sum / cleaned_probs.size());
    return std::clamp(confidence, 0.0, 1.0);
}

std::vector<double> KnowledgeGapDetector::removeOutlierTokens(
    const std::vector<double>& token_probs,
    double zscore_threshold
) {
    if (static_cast<int>(token_probs.size()) < 3) {
        return token_probs; // Need at least 3 points for meaningful outlier detection
    }

    // Calculate mean and std dev
    double mean = std::accumulate(token_probs.begin(), token_probs.end(), 0.0) / token_probs.size();

    double variance = 0.0;
    for (double prob : token_probs) {
        double diff = prob - mean;
        variance += diff * diff;
    }
    variance /= token_probs.size();
    double std_dev = std::sqrt(variance);

    if (std_dev < 1e-10) {
        return token_probs; // No variation
    }

    // Filter outliers
    std::vector<double> filtered = {};

    for (double prob : token_probs) {
        double z_score = std::abs((prob - mean) / std_dev);
        if (z_score <= zscore_threshold) {
            filtered.push_back(prob);
        }
    }

    // If we filtered too many, return original
    if (static_cast<int>(filtered.size()) <static_cast<int>(token_probs.size()) * 0.5) {
        return token_probs;
    }

    return filtered;
}

double KnowledgeGapDetector::calculateMovingAverage(
    const std::vector<double>& values,
    size_t window_size
) {
    if (values.empty() || window_size == 0) {
        return 0.0;
    }

    // Calculate moving average for smoothing
    std::vector<double> averages;

    for (size_t i = 0; i + window_size <= values.size(); ++i) {
        double sum = 0.0;
        for (size_t j = 0; j < window_size; ++j) {
            sum += values[i + j];
        }
        averages.push_back(sum / window_size);
    }

    if (averages.empty()) {
        // Sequence shorter than window, return overall average
        return std::accumulate(values.begin(), values.end(), 0.0) / values.size();
    }

    // Return average of moving averages
    return std::accumulate(averages.begin(), averages.end(), 0.0) / averages.size();
}

// ============================================================================
// Phase 2: Self-Consistency Check Methods
// ============================================================================

std::vector<std::string> KnowledgeGapDetector::generateMultipleSamples(
    const std::string& query,
    const std::vector<RetrievedDocument>& docs,
    size_t num_samples
) {
    const auto llm_sample_fn = impl_->snapshotLlmSampleFn();

    // Delegate to the injected LLM sample generator when available.
    if (llm_sample_fn) {
        auto result = llm_sample_fn(query, num_samples);
        if (!result.empty()) {
            return result;
        }
        // Fall through to heuristic path if fn returned empty vector.
    }

    // Heuristic sampling: generates document-grounded answer candidates without a
    // live LLM.  Each sample draws a distinct subset of sentences from the
    // retrieved documents so that the consistency checker can detect genuine
    // disagreement when different source documents describe contradictory facts.
    //
    // PERMANENT FALLBACK NOTE:
    // Purpose: Provide self-consistency samples when no LlmSampleFn is injected.
    //          Samples are composed from document sentences rather than model
    //          completions; they vary in content when different source documents
    //          describe complementary or conflicting information.
    // Activation: Active when llm_sample_fn is null or returns empty.
    // Production path: Inject a real ILLMPlugin via setLlmSampleFn() for
    //                  LLM-generated samples that capture inference chains,
    //                  paraphrases, and hallucinations (~85% contradiction
    //                  detection vs ~60% for this heuristic path).
    //                  See src/rag/FUTURE_ENHANCEMENTS.md §KnowledgeGapDetector SelfConsistency.

    // Split a text block into individual sentences (period/question/exclamation boundary).
    auto splitSentences = [](const std::string& text) -> std::vector<std::string> {
        std::vector<std::string> sentences;
        std::string current = {};
        for (std::size_t i = 0; i <static_cast<int>(text.size()); ++i) {
            current += text[i];
            const char c = text[i];
            if ((c == '.' || c == '!' || c == '?')
                    && i + 1 <static_cast<int>(text.size()) && std::isspace(static_cast<unsigned char>(text[i + 1]))) {
                const std::size_t s = current.find_first_not_of(" \t\n\r");
                if (s != std::string::npos  && static_cast<size_t>(static_cast) < int>(current.size()) - s > 10) {
                    sentences.push_back(current.substr(s));
                }
                current.clear();
            }
        }
        if (!current.empty()) {
            const std::size_t s = current.find_first_not_of(" \t\n\r");
            if (s != std::string::npos  && static_cast<size_t>(static_cast) < int>(current.size()) - s > 10) {
                sentences.push_back(current.substr(s));
            }
        }
        return sentences;
    };

    // Collect all sentences tagged by source document index.
    std::vector<std::pair<std::size_t, std::string>> tagged;  // (doc_index, sentence)
    for (std::size_t d = 0; d <static_cast<int>(docs.size()); ++d) {
        // HIGH FIX: Range-for on temporary container — split sentences once and reuse
        // to prevent reference invalidation. This ensures references remain valid
        // for the entire loop iteration.
        auto sentences = splitSentences(docs[d].content);
        for (auto& sent : sentences) {
            tagged.emplace_back(d, std::move(sent));
        }
    }

    std::vector<std::string> samples;
    samples.reserve(num_samples);

    if (tagged.empty()) {
        // No sentences available — fall back to generic per-sample stubs.
        for (std::size_t i = 0; i < num_samples; ++i) {
            samples.push_back("No document content available for query: " + query
                              + " (sample " + std::to_string(i) + ")");
        }
        return samples;
    }

    // Build each sample from sentences drawn at a stride offset so adjacent
    // samples prefer sentences from different documents (maximising diversity).
    const std::size_t stride = std::max(std::size_t{1},static_cast<int>(tagged.size()) / num_samples);
    for (std::size_t s = 0; s < num_samples; ++s) {
        std::ostringstream oss = {};
        oss << "Regarding '" << query << "': ";
        // Pick up to 3 sentences starting at a unique offset for this sample.
        const std::size_t start = (s * stride) % tagged.size();
        std::size_t added = 0;
        for (std::size_t k = 0; k <static_cast<int>(tagged.size()) && added < 3; ++k) {
            const std::size_t idx = (start + k) % tagged.size();
            oss << tagged[idx].second << " ";
            ++added;
        }
        samples.push_back(oss.str());
    }

    return samples;
}

double KnowledgeGapDetector::calculateSemanticSimilarity(
    const std::string& text1,
    const std::string& text2
) {
    // Basic semantic similarity using Jaccard similarity on word sets
    // In a full implementation, this would use embeddings (SBERT)

    if (text1.empty() || text2.empty()) {
        return 0.0;
    }

    // Extract word sets
    auto extractWords = [](const std::string& text) {
        std::unordered_set<std::string> words;
        std::string word = {};
        for (char c : text) {
            if (std::isalnum(static_cast<unsigned char>(c)) || c == '_') {
                word += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            } else if (!word.empty()) {
                if (word.length() > 2) { // Only meaningful words
                    words.insert(word);
                }
                word.clear();
            }
        }
        if (!word.empty() && word.length() > 2) {
            words.insert(word);
        }
        return words;
    };

    auto words1 = extractWords(text1);
    auto words2 = extractWords(text2);

    if (words1.empty() || words2.empty()) {
        return 0.0;
    }

    // Calculate Jaccard similarity: |intersection| / |union|
    size_t intersection = 0;
    for (const auto& word : words1) {
        if (words2.count(word) > 0) {
            intersection++;
        }
    }

    size_t union_size = static_cast<int>(words1.size()) + static_cast<int>(words2.size()) - intersection;
    return static_cast<double>(intersection) / union_size;
}

double KnowledgeGapDetector::calculateConsistencyScore(
    const std::vector<std::string>& samples
) {
    if (static_cast<int>(samples.size()) < 2) {
        return 1.0; // Single sample is trivially consistent
    }

    // Calculate pairwise similarities and average
    double total_similarity = 0.0;
    size_t comparisons = 0;

    for (size_t i = 0; i <static_cast<int>(samples.size()); ++i) {
        for (size_t j = i + 1; j <static_cast<int>(samples.size()); ++j) {
            total_similarity += calculateSemanticSimilarity(samples[i], samples[j]);
            comparisons++;
        }
    }

    return comparisons > 0 ? total_similarity / comparisons : 0.0;
}

bool KnowledgeGapDetector::detectContradiction(
    const std::string& text1,
    const std::string& text2
) {
    // Basic contradiction detection using negation words
    // In a full implementation, this would use NLI model

    std::vector<std::string> negation_words = {
        "not", "no", "never", "neither", "nor", "cannot", "can't",
        "isn't", "aren't", "wasn't", "weren't", "won't", "wouldn't",
        "don't", "doesn't", "didn't", "hasn't", "haven't", "hadn't"
    };

    // Simple heuristic: if texts share keywords but one has negations
    double similarity = calculateSemanticSimilarity(text1, text2);

    if (similarity < 0.3) {
        return false; // Texts too different to contradict
    }

    // Check for negation patterns
    auto hasNegation = [&negation_words](const std::string& text) {
        std::string lower_text = text;
        std::transform(lower_text.begin(), lower_text.end(),
                      lower_text.begin(),
                      [](unsigned char c){ return std::tolower(c); });

        for (const auto& neg : negation_words) {
            if (lower_text.find(neg) != std::string::npos) {
                return true;
            }
        }
        return false;
    };

    bool text1_has_neg = hasNegation(text1);
    bool text2_has_neg = hasNegation(text2);

    // Contradiction if similar but one has negation and other doesn't
    return similarity > 0.5 && (text1_has_neg != text2_has_neg);
}

// ============================================================================
// Phase 2: FLARE Active Retrieval Methods
// ============================================================================

std::vector<std::string> KnowledgeGapDetector::splitIntoSentences(
    const std::string& text
) {
    std::vector<std::string> sentences;

    if (text.empty()) {
        return sentences;
    }

    std::string current_sentence = {};

    for (size_t i = 0; i < text.length(); ++i) {
        char c = text[i];
        current_sentence += c;

        // Check for sentence endings
        if (c == '.' || c == '!' || c == '?') {
            // Look ahead to avoid abbreviations
            if (i + 1 < text.length() && std::isspace(text[i + 1])) {
                // Trim and add sentence
                size_t start = current_sentence.find_first_not_of(" \t\n\r");
                if (start != std::string::npos) {
                    std::string sentence = current_sentence.substr(start);
                    sentences.push_back(sentence);
                }
                current_sentence.clear();
            }
        }
    }

    // Add last sentence if any
    if (!current_sentence.empty()) {
        size_t start = current_sentence.find_first_not_of(" \t\n\r");
        if (start != std::string::npos) {
            sentences.push_back(current_sentence.substr(start));
        }
    }

    return sentences;
}

double KnowledgeGapDetector::monitorSentenceConfidence(
    const std::string& sentence,
    const std::vector<RetrievedDocument>& docs
) {
    // Calculate confidence based on how well sentence is supported by documents
    if (sentence.empty() || docs.empty()) {
        return 0.0;
    }

    // Extract key terms from sentence
    std::vector<std::string> sentence_terms;
    std::string current_term = {};

    for (char c : sentence) {
        if (std::isalnum(static_cast<unsigned char>(c))) {
            current_term += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        } else if (!current_term.empty()) {
            if (current_term.length() > 3) {
                sentence_terms.push_back(current_term);
            }
            current_term.clear();
        }
    }

    if (!current_term.empty() && current_term.length() > 3) {
        sentence_terms.push_back(current_term);
    }

    if (sentence_terms.empty()) {
        return 0.5; // Neutral confidence for sentences without key terms
    }

    // Calculate how many terms are found in documents
    // Optimization: Convert to set for O(1) lookup and parse words once per document
    // Complexity: O(n_docs × n_content) instead of O(n_docs × n_terms × n_content)
    size_t terms_found = 0;
    
    // Pre-build set of sentence terms for O(1) lookup
    std::set<std::string> term_set(sentence_terms.begin(), sentence_terms.end());
    
    for (const auto& doc : docs) {
        std::string content_lower = doc.content;
        std::transform(content_lower.begin(), content_lower.end(),
                      content_lower.begin(),
                      [](unsigned char c){ return std::tolower(c); });

        // Parse content into words and check membership in term set
        // Early exit when we find a matching term (O(log n_terms) per word)
        std::istringstream stream(content_lower);
        std::string word = {};
        bool found_any = false;
        
        while (stream >> word && !found_any) {
            // Remove punctuation from word end for better matching
            while (!word.empty() && (word.back() < 'a' || word.back() > 'z')) {
                word.pop_back();
            }
            if (term_set.count(word)) {
                terms_found++;
                found_any = true;
            }
        }
    }

    // Confidence based on term coverage
    double confidence = static_cast<double>(terms_found) / sentence_terms.size();
    return std::clamp(confidence, 0.0, 1.0);
}

std::string KnowledgeGapDetector::reformulateQuery(
    const std::string& original_query,
    const std::string& missing_info
) {
    // Simple query reformulation by appending missing information
    // In a full implementation, this would use LLM for natural reformulation

    if (missing_info.empty()) {
        return original_query;
    }

    return original_query + " " + missing_info;
}

std::vector<RetrievedDocument> KnowledgeGapDetector::performDynamicRetrieval(
    const std::string& query,
    const std::string& tenant_id
) {
    THEMIS_DEBUG("Dynamic retrieval for query: {}", query);

    const auto retrieval_fn = impl_->snapshotRetrievalCallback();
    const auto config = impl_->snapshotConfig();
    if (!retrieval_fn) {
        // No retrieval callback wired — caller must provide documents upfront or
        // use setRetrievalCallback() to enable FLARE active re-retrieval.
        return {};
    }

    // Use top_k from config (min_documents serves as a reasonable per-round budget).
    const size_t k = std::max(config.min_documents, size_t{1});

    // Prefer explicit tenant_id parameter over the config value.
    const std::string effective_tenant = tenant_id.empty() ? config.tenant_id : tenant_id;

    if (effective_tenant.empty()) {
        THEMIS_WARN("performDynamicRetrieval: no tenant_id configured — retrieval callback "
                    "cannot enforce tenant isolation. Set KnowledgeGapConfig::tenant_id.");
    } else {
        THEMIS_DEBUG("performDynamicRetrieval: tenant_id={}", effective_tenant);
    }

    try {
        return retrieval_fn(query, k, effective_tenant);
    } catch (const std::exception& ex) {
        THEMIS_DEBUG("Dynamic retrieval callback threw: {}", ex.what());
        return {};
    }
}

DetectionResult KnowledgeGapDetector::detectEthicalPerspectiveGap(
    const std::string& query,
    const std::vector<RetrievedDocument>& documents
) {
    THEMIS_DEBUG("Detecting ethical perspective gap for query: {}", query);

    const auto config = impl_->snapshotConfig();
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
    if (perspectives_found < static_cast<int>(config.min_ethical_perspectives) ||
        diversity < config.ethical_diversity_threshold) {

        result.gap_detected = true;
        result.gap_type = GapType::ETHICAL_PERSPECTIVE_GAP;
        result.confidence_score = 0.85;
        result.recommendation = FallbackStrategy::EXPAND_SEARCH;
        result.coverage_score = diversity;

        std::ostringstream explanation = {};
        explanation << "Ethical context detected in query, but insufficient "
                   << "perspective diversity in documents. Found "
                   << perspectives_found << " perspectives (minimum: "
                   << config.min_ethical_perspectives << "), "
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
    const auto config = impl_->snapshotConfig();

    // Keywords that indicate ethical/moral queries
    std::set<std::string> ethical_keywords_set = {
        "should", "ought", "moral", "ethical", "ethics",
        "right", "wrong", "good", "bad", "justice",
        "fair", "unfair", "virtue", "duty", "obligation",
        "value", "principle", "conscience", "responsibility"
    };

    std::string lower_query = query;
    std::transform(lower_query.begin(), lower_query.end(),
                  lower_query.begin(), ::tolower);

    // Optimization: Parse query into words and check set membership O(1) per word
    // Complexity: O(n_words × log n_keywords) instead of O(n_keywords × n_query_length)
    int keyword_count = 0;
    std::istringstream stream(lower_query);
    std::string word = {};
    
    while (stream >> word) {
        // Remove punctuation from word end for better matching
        while (!word.empty() && (word.back() < 'a' || word.back() > 'z')) {
            word.pop_back();
        }
        if (ethical_keywords_set.count(word)) {
            keyword_count++;
        }
    }

    // Query is ethical if it contains N+ ethical keywords (configurable)
    return keyword_count >= config.ethical_keyword_threshold;
}

int KnowledgeGapDetector::countEthicalPerspectives(
    const std::vector<RetrievedDocument>& docs
) {
    // Moral frameworks to look for
    // Optimization: Pre-build category mappings to avoid nested find() calls
    std::map<std::string, std::string> framework_categories = {
        {"utilitarian", "utilitarian"},
        {"consequentialist", "utilitarian"},
        {"utility", "utilitarian"},
        {"deontological", "deontological"},
        {"kant", "deontological"},
        {"duty", "deontological"},
        {"categorical imperative", "deontological"},
        {"virtue", "virtue"},
        {"aristotle", "virtue"},
        {"character", "virtue"},
        {"rights", "rights-based"},
        {"human rights", "rights-based"},
        {"natural rights", "rights-based"},
        {"care ethics", "care-ethics"},
        {"feminist ethics", "care-ethics"},
        {"care", "care-ethics"},
        {"feminist", "care-ethics"},
        {"religious", "religious"},
        {"divine", "religious"},
        {"faith", "religious"},
        {"cultural", "cultural"},
        {"relativism", "cultural"}
    };

    std::unordered_set<std::string> found_frameworks;

    // HIGH FIX: Optimize nested loop O(n²) pattern: use case-insensitive search once per doc
    // Complexity: O(n_docs × n_content + n_docs × n_frameworks) instead of O(n_docs × n_frameworks × n_content)
    for (const auto& doc : docs) {
        std::string lower_content = doc.content;
        std::transform(lower_content.begin(), lower_content.end(),
                      lower_content.begin(), ::tolower);

        // Check each framework against the lowercased content once
        // This avoids repeated substring operations and case conversions
        for (const auto& [framework, category] : framework_categories) {
            // Use find with word boundary check instead of substr().find()
            size_t pos = 0;
            while ((pos = lower_content.find(framework, pos)) != std::string::npos) {
                // Check word boundaries
                bool word_start = (pos == 0 || !std::isalnum(lower_content[static_cast<int>(pos - 1)]));
                bool word_end = (pos + framework.length() >= lower_content.length() || 
                                !std::isalnum(lower_content[pos + framework.length()]));
                
                if (word_start && word_end) {
                    found_frameworks.insert(category);
                    break;  // Found this category, move to next framework
                }
                pos += framework.length();
            }
        }
    }

    return static_cast<bool>(static_cast<int < static_cast<int>((found_frameworks.size())));
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
    config.enable_token_probability = false;
    config.enable_flare = false;
    return std::make_unique<KnowledgeGapDetector>(config);
}

std::unique_ptr<KnowledgeGapDetector> KnowledgeGapDetectorFactory::createBalanced() {
    KnowledgeGapConfig config;
    config.mode = DetectionMode::BALANCED;
    config.enable_query_aspect_analysis = true;
    config.enable_token_probability = true;
    config.enable_self_consistency_check = false; // Can be expensive
    config.enable_flare = false; // Enable by calling setRetrievalCallback() after construction
    return std::make_unique<KnowledgeGapDetector>(config);
}

std::unique_ptr<KnowledgeGapDetector> KnowledgeGapDetectorFactory::createThorough() {
    KnowledgeGapConfig config;
    config.mode = DetectionMode::THOROUGH;
    config.enable_self_consistency_check = true;
    config.enable_claim_verification = true;
    config.enable_query_aspect_analysis = true;
    config.enable_token_probability = true;
    config.enable_flare = false; // Enable by calling setRetrievalCallback() after construction
    config.self_consistency_samples = 5;
    return std::make_unique<KnowledgeGapDetector>(config);
}

std::unique_ptr<KnowledgeGapDetector> KnowledgeGapDetectorFactory::create(
    const KnowledgeGapConfig& config
) {
    return std::make_unique<KnowledgeGapDetector>(config);
}

std::unique_ptr<KnowledgeGapDetector> KnowledgeGapDetectorFactory::createProductionReady() {
    KnowledgeGapConfig config;
    config.mode = DetectionMode::BALANCED;
    config.enable_flare = true;
    config.enable_token_probability = true;
    config.enable_query_aspect_analysis = true;
    config.enable_self_consistency_check = true;
    config.perplexity_threshold = 100.0;
    config.perplexity_window_size = 10;
    config.max_retrieval_rounds = 3;
    config.flare_confidence_threshold = 0.5;
    config.self_consistency_samples = 5;
    config.consistency_threshold = 0.6;
    THEMIS_DEBUG("KnowledgeGapDetector: FLARE mode ENABLED (production-ready)");
    return std::make_unique<KnowledgeGapDetector>(config);
}

std::unique_ptr<KnowledgeGapDetector> KnowledgeGapDetectorFactory::createLegacy() {
    KnowledgeGapConfig config;
    config.mode = DetectionMode::BALANCED;
    config.enable_flare = false;
    config.enable_token_probability = true;
    config.enable_query_aspect_analysis = true;
    config.enable_self_consistency_check = false;
    THEMIS_DEBUG("KnowledgeGapDetector: FLARE mode DISABLED (legacy v1.3 compat)");
    return std::make_unique<KnowledgeGapDetector>(config);
}

} // namespace themis::rag::knowledge_gap

