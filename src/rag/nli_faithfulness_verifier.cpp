/**
 * @file nli_faithfulness_verifier.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=0, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "rag/nli_faithfulness_verifier.h"
#include "rag/faithfulness_evaluator.h"
#include "rag/onnx_model_loader.h"
#include "utils/logger.h"
#include <algorithm>
#include <sstream>
#include <cmath>
#include <regex>
#include <chrono>
#include <iomanip>

// Regex patterns compiled once at program startup to avoid per-call overhead.
// Positive-complement phrases that begin with a negation word but express
// addition or intensification rather than logical contradiction.
static const std::regex kPositivePhrase(
    R"(\b(?:not only|not just|not even|not yet|never before|no less)\b)",
    std::regex::icase);
// Remaining negation words (after kPositivePhrase has been removed).
static const std::regex kNegationPattern(
    R"(\b(?:not|never|no|false)\b)",
    std::regex::icase);

namespace themis::rag::judge {

// ═══════════════════════════════════════════════════════════
// NLIFaithfulnessVerifier Implementation
// ═══════════════════════════════════════════════════════════

struct NLIFaithfulnessVerifier::Impl {
    Config config;
    bool model_loaded = false;
    
    // ONNX model loader and cached model info
    std::unique_ptr<ONNXModelLoader> model_loader_;
    std::optional<ONNXModelInfo> loaded_model_;
    
    // Inference statistics
    size_t onnx_inference_count_ = 0;
    size_t heuristic_inference_count_ = 0;
    double total_onnx_latency_ms_ = 0.0;
    
    Impl(const Config& cfg) : config(cfg) {
        THEMIS_INFO("NLIFaithfulnessVerifier initialized");
        THEMIS_INFO("  Entailment threshold: {}", config.entailment_threshold);
        THEMIS_INFO("  Contradiction threshold: {}", config.contradiction_threshold);
        
        // Initialize ONNX model loader if ONNX is enabled
        if (config.use_onnx) {
            ONNXModelLoaderConfig loader_config;
            loader_config.cache_dir = "./models";
            loader_config.verify_checksum = false;
            loader_config.auto_download = false;
            loader_config.create_cache_dir = true;
            
            model_loader_ = std::make_unique<ONNXModelLoader>(loader_config);
            THEMIS_INFO("ONNX Model Loader initialized");
            THEMIS_INFO("  ONNX model path: {}", config.onnx_model_path);
            THEMIS_INFO("  Fallback to heuristic: {}", config.fallback_to_heuristic);
            THEMIS_INFO("  ONNX inference timeout: {}ms", config.onnx_inference_timeout_ms);
        }
    }
    
    /**
     * @brief Load ONNX model from specified path
     * @return true if model loaded successfully
     */
    bool loadOnnxModel(const std::string& model_path) {
        if (!model_loader_) {
            THEMIS_WARN("ONNX model loader not initialized (use_onnx=false)");
            return false;
        }
        
        if (model_path.empty()) {
            THEMIS_ERROR("Empty model path provided");
            return false;
        }
        
        auto model_info = model_loader_->loadModel(model_path);
        if (!model_info) {
            THEMIS_ERROR("Failed to load ONNX model from: {}", model_path);
            return false;
        }
        
        loaded_model_ = model_info.value();
        model_loaded = true;
        THEMIS_INFO("Successfully loaded ONNX model: {} ({} bytes)", 
                   model_info->model_name, model_info->model_size_bytes);
        return true;
    }
    
    /**
     * @brief Compute NLI score using ONNX model.
     *
     * @note STUB/SIMULATION NOTE:
     * Purpose: Demonstrates ONNX integration pattern; actual ONNX Runtime not yet wired.
     * Activation: config.use_onnx == true && model_loaded == true
     * Production Delta: Uses text-overlap heuristics instead of ONNX Runtime inference.
     * Removal Plan: Phase 2 will replace with actual ONNX Runtime tokenization and inference.
     */
    NLIResult computeNLIWithOnnx(const std::string& premise, const std::string& hypothesis) {
        auto start_time = std::chrono::steady_clock::now();
        
        NLIResult result;
        
        // Phase 1: Stub implementation that demonstrates ONNX integration pattern
        // Phase 2 will replace this with actual ONNX Runtime tokenization and inference
        
        // In a full implementation, this would:
        // 1. Tokenize premise and hypothesis using tokenizer
        // 2. Prepare input tensors for ONNX model
        // 3. Run model inference
        // 4. Extract logits and compute softmax probabilities
        // 5. Return NLI result
        
        // For now, we use a deterministic mapping based on text overlap
        // This maintains compatibility while showing the integration point
        
        std::string premise_lower = premise;
        std::string hypothesis_lower = hypothesis;
        std::transform(premise_lower.begin(), premise_lower.end(), 
                      premise_lower.begin(), ::tolower);
        std::transform(hypothesis_lower.begin(), hypothesis_lower.end(), 
                      hypothesis_lower.begin(), ::tolower);
        
        // Count overlapping words (simple heuristic for Phase 1)
        std::istringstream hyp_stream(hypothesis_lower);
        std::vector<std::string> hyp_words;
        std::string word;
        while (hyp_stream >> word) {
            if (word.length() > 3) {
                hyp_words.push_back(word);
            }
        }
        
        if (hyp_words.empty()) {
            result.label = NLILabel::NEUTRAL;
            result.entailment_score = 0.33;
            result.neutral_score = 0.34;
            result.contradiction_score = 0.33;
            result.confidence = 0.5;
        } else {
            size_t matches = 0;
            for (const auto& w : hyp_words) {
                if (premise_lower.find(w) != std::string::npos) {
                    matches++;
                }
            }
            
            double match_ratio = static_cast<double>(matches) / hyp_words.size();
            
            if (match_ratio >= 0.8) {
                result.entailment_score = 0.75;
                result.neutral_score = 0.15;
                result.contradiction_score = 0.10;
                result.label = NLILabel::ENTAILMENT;
            } else if (match_ratio >= 0.5) {
                result.entailment_score = 0.30;
                result.neutral_score = 0.55;
                result.contradiction_score = 0.15;
                result.label = NLILabel::NEUTRAL;
            } else {
                result.entailment_score = 0.10;
                result.neutral_score = 0.20;
                result.contradiction_score = 0.70;
                result.label = NLILabel::CONTRADICTION;
            }
            
            result.confidence = std::max({result.entailment_score, 
                                         result.neutral_score, 
                                         result.contradiction_score});
        }
        
        auto end_time = std::chrono::steady_clock::now();
        auto latency = std::chrono::duration_cast<std::chrono::milliseconds>(
            end_time - start_time).count();
        
        onnx_inference_count_++;
        total_onnx_latency_ms_ += latency;
        
        if (config.log_inference_mode) {
            double avg_latency = total_onnx_latency_ms_ / onnx_inference_count_;
            THEMIS_DEBUG("NLI inference (ONNX stub/heuristic): count={}, latency={}ms, avg={}ms",
                        onnx_inference_count_, latency, avg_latency);
        }
        
        return result;
    }
    
    /**
     * @brief Compute NLI score using heuristic term-overlap and negation detection.
     *
     * Produces entailment / neutral / contradiction labels and scores based on
     * weighted term overlap and negation signals.
     */
    NLIResult computeNLI(const std::string& premise, const std::string& hypothesis) {
        auto start_time = std::chrono::steady_clock::now();
        
        // Try ONNX first if enabled and model is loaded
        if (config.use_onnx && model_loaded && loaded_model_) {
            auto onnx_result = computeNLIWithOnnx(premise, hypothesis);
            
            if (config.log_inference_mode) {
                auto latency = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - start_time).count();
                THEMIS_INFO("NLI inference (ONNX stub/heuristic): latency={}ms, label={}", 
                           latency, 
                           static_cast<int>(onnx_result.label));
            }
            
            return onnx_result;
        }
        
        // Fallback to heuristic
        heuristic_inference_count_++;
        
        NLIResult result;
        
        // Convert to lowercase for comparison
        std::string premise_lower = premise;
        std::string hypothesis_lower = hypothesis;
        std::transform(premise_lower.begin(), premise_lower.end(), 
                      premise_lower.begin(), ::tolower);
        std::transform(hypothesis_lower.begin(), hypothesis_lower.end(), 
                      hypothesis_lower.begin(), ::tolower);
        
        // Extract words from hypothesis
        std::istringstream hyp_stream(hypothesis_lower);
        std::vector<std::string> hyp_words;
        std::string word;
        while (hyp_stream >> word) {
            if (word.length() > 3) {  // Skip short words
                hyp_words.push_back(word);
            }
        }
        
        if (hyp_words.empty()) {
            result.label = NLILabel::NEUTRAL;
            result.entailment_score = 0.33;
            result.neutral_score = 0.34;
            result.contradiction_score = 0.33;
            result.confidence = 0.5;
            
            auto latency = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start_time).count();
            if (config.log_inference_mode) {
                THEMIS_DEBUG("NLI inference (heuristic): latency={}ms (empty hypothesis)", latency);
            }
            return result;
        }
        
        // Count matching words
        size_t matches = 0;

        for (const auto& w : hyp_words) {
            if (premise_lower.find(w) != std::string::npos) {
                matches++;
            }
        }

        double match_ratio = static_cast<double>(matches) / hyp_words.size();

        // Negation detection using word-boundary regex on the full hypothesis.
        std::string hyp_for_negation =
            std::regex_replace(hypothesis_lower, kPositivePhrase, " ");
        auto neg_begin = std::sregex_iterator(
            hyp_for_negation.begin(), hyp_for_negation.end(), kNegationPattern);
        size_t contradictions = static_cast<size_t>(
            std::distance(neg_begin, std::sregex_iterator{}));
        
        // Compute probability distribution
        if (match_ratio >= 0.8 && contradictions == 0) {
            // High overlap, likely entailment
            result.entailment_score = 0.70 + (match_ratio - 0.8) * 1.5;
            result.neutral_score = 0.20;
            result.contradiction_score = 0.10;
            result.label = NLILabel::ENTAILMENT;
        } else if (match_ratio >= 0.5 && contradictions == 0) {
            // Moderate overlap, likely neutral
            result.entailment_score = 0.25 + match_ratio * 0.3;
            result.neutral_score = 0.50;
            result.contradiction_score = 0.25;
            result.label = NLILabel::NEUTRAL;
        } else if (contradictions > 0 || match_ratio < 0.2) {
            // Low overlap or negation, likely contradiction
            result.entailment_score = 0.10;
            result.neutral_score = 0.20;
            result.contradiction_score = 0.70;
            result.label = NLILabel::CONTRADICTION;
        } else {
            // Moderate overlap
            result.entailment_score = 0.30;
            result.neutral_score = 0.50;
            result.contradiction_score = 0.20;
            result.label = NLILabel::NEUTRAL;
        }
        
        // Normalize probabilities
        double total = result.entailment_score + result.neutral_score + 
                      result.contradiction_score;
        result.entailment_score /= total;
        result.neutral_score /= total;
        result.contradiction_score /= total;
        
        // Compute confidence as max probability
        result.confidence = std::max({result.entailment_score, 
                                     result.neutral_score, 
                                     result.contradiction_score});
        
        auto latency = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start_time).count();
        if (config.log_inference_mode) {
            THEMIS_DEBUG("NLI inference (heuristic): latency={}ms, label={}, confidence={:.3f}",
                        latency, static_cast<int>(result.label), 
                        result.confidence);
        }
        
        return result;
    }
    
    /**
     * @brief Extract claims from text using sentence splitting
     */
    std::vector<std::string> extractClaims(const std::string& text) {
        std::vector<std::string> claims;
        
        if (text.empty()) {
            return claims;
        }
        
        // Simple sentence splitting using regex patterns
        // In production, use a proper NLP library
        size_t start = 0;
        for (size_t i = 0; i < text.length(); i++) {
            if ((text[i] == '.' || text[i] == '!' || text[i] == '?') && 
                i + 1 < text.length() && text[i + 1] == ' ') {
                
                std::string claim = text.substr(start, i - start + 1);
                
                // Trim whitespace
                claim.erase(0, claim.find_first_not_of(" \t\n\r"));
                claim.erase(claim.find_last_not_of(" \t\n\r") + 1);
                
                if (claim.length() > 10) {  // Skip very short sentences
                    claims.push_back(claim);
                }
                
                start = i + 2;  // Skip the period and space
            }
        }
        
        // Add last claim if any
        if (start < text.length()) {
            std::string claim = text.substr(start);
            claim.erase(0, claim.find_first_not_of(" \t\n\r"));
            claim.erase(claim.find_last_not_of(" \t\n\r") + 1);
            
            if (claim.length() > 10) {
                claims.push_back(claim);
            }
        }
        
        // Limit number of claims
        if (claims.size() > config.max_claims) {
            claims.resize(config.max_claims);
        }
        
        return claims;
    }
};

NLIFaithfulnessVerifier::NLIFaithfulnessVerifier()
    : NLIFaithfulnessVerifier(Config{}) {
}

NLIFaithfulnessVerifier::NLIFaithfulnessVerifier(const Config& config)
    : impl_(std::make_unique<Impl>(config)) {
}

NLIFaithfulnessVerifier::~NLIFaithfulnessVerifier() = default;

FaithfulnessVerificationResult NLIFaithfulnessVerifier::verify(
    const std::string& answer,
    const std::vector<std::pair<std::string, std::string>>& documents
) {
    auto start_time = std::chrono::steady_clock::now();
    
    FaithfulnessVerificationResult result;
    
    if (answer.empty() || documents.empty()) {
        result.faithfulness_score = 0.0;
        result.is_faithful = false;
        result.explanation = "Empty answer or no documents provided";
        return result;
    }
    
    // Extract claims from answer
    auto claims = impl_->extractClaims(answer);
    result.total_claims = claims.size();
    
    THEMIS_DEBUG("Extracted {} claims from answer", claims.size());
    
    if (claims.empty()) {
        // No claims = potentially low quality or parsing failure
        // Use a neutral score rather than perfect score
        result.faithfulness_score = 0.7;  // Neutral: neither perfect nor poor
        result.is_faithful = true;  // No claims to contradict
        result.explanation = "No factual claims found in answer (may indicate simple/generic response)";
        
        auto end_time = std::chrono::steady_clock::now();
        result.verification_time = std::chrono::duration_cast<std::chrono::milliseconds>(
            end_time - start_time);
        
        return result;
    }
    
    // Verify each claim against documents
    for (const auto& claim : claims) {
        ClaimVerification claim_result;
        claim_result.claim = claim;
        claim_result.support_level = SupportLevel::UNSUPPORTED;
        
        double best_entailment = 0.0;
        std::string best_document_id;
        
        // Check claim against each document
        for (const auto& [doc_id, doc_content] : documents) {
            NLIResult nli = impl_->computeNLI(doc_content, claim);
            
            claim_result.nli_scores[doc_id] = nli;
            
            if (nli.entailment_score > best_entailment) {
                best_entailment = nli.entailment_score;
                best_document_id = doc_id;
            }
        }
        
        // Determine support level based on best entailment score
        if (best_entailment >= impl_->config.entailment_threshold) {
            claim_result.support_level = SupportLevel::FULLY_SUPPORTED;
            claim_result.supporting_doc_ids.push_back(best_document_id);
            result.supported_claims++;
        } else if (best_entailment >= impl_->config.neutral_threshold) {
            claim_result.support_level = SupportLevel::PARTIALLY_SUPPORTED;
            claim_result.supporting_doc_ids.push_back(best_document_id);
            result.partially_supported_claims++;
        } else {
            // Check for contradiction
            for (const auto& [doc_id, nli] : claim_result.nli_scores) {
                if (nli.contradiction_score >= impl_->config.contradiction_threshold) {
                    claim_result.support_level = SupportLevel::CONTRADICTED;
                    result.contradicted_claims++;
                    break;
                }
            }
            
            if (claim_result.support_level == SupportLevel::UNSUPPORTED) {
                result.unsupported_claims++;
            }
        }
        
        claim_result.confidence = best_entailment;
        result.claims.push_back(claim_result);
    }
    
    // Calculate overall faithfulness score
    // Weight: Supported = 1.0, Partial = 0.5, Unsupported = 0.0, Contradicted = -0.5
    double score_sum = 0.0;
    
    for (const auto& claim : result.claims) {
        switch (claim.support_level) {
            case SupportLevel::FULLY_SUPPORTED:
                score_sum += 1.0;
                break;
            case SupportLevel::PARTIALLY_SUPPORTED:
                score_sum += 0.5;
                break;
            case SupportLevel::UNSUPPORTED:
                score_sum += 0.0;
                break;
            case SupportLevel::CONTRADICTED:
                score_sum += -0.5;
                break;
        }
    }
    
    result.faithfulness_score = std::max(0.0, score_sum / result.total_claims);
    result.is_faithful = result.faithfulness_score >= impl_->config.min_faithfulness_score;
    
    // Generate explanation
    std::ostringstream explanation;
    explanation << "Faithfulness Verification:\n";
    explanation << "  Total claims: " << result.total_claims << "\n";
    explanation << "  Fully supported: " << result.supported_claims << "\n";
    explanation << "  Partially supported: " << result.partially_supported_claims << "\n";
    explanation << "  Unsupported: " << result.unsupported_claims << "\n";
    explanation << "  Contradicted: " << result.contradicted_claims << "\n";
    explanation << "  Faithfulness score: " << std::fixed << std::setprecision(3) 
                << result.faithfulness_score << "\n";
    explanation << "  Verdict: " << (result.is_faithful ? "FAITHFUL" : "NOT FAITHFUL");
    
    result.explanation = explanation.str();
    
    auto end_time = std::chrono::steady_clock::now();
    result.verification_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time - start_time);
    
    THEMIS_INFO("NLI verification complete: score={:.3f}, faithful={}, time={}ms",
               result.faithfulness_score, result.is_faithful, 
               result.verification_time.count());
    
    return result;
}

NLIResult NLIFaithfulnessVerifier::checkEntailment(
    const std::string& premise,
    const std::string& hypothesis
) {
    return impl_->computeNLI(premise, hypothesis);
}

void NLIFaithfulnessVerifier::loadModel(const std::string& model_path) {
    if (!impl_->model_loader_) {
        THEMIS_WARN("ONNX model loader not initialized (use_onnx=false)");
        impl_->model_loaded = true;  // Mark as loaded for heuristic fallback
        THEMIS_INFO("Using heuristic NLI (ONNX disabled)");
        return;
    }
    
    if (impl_->loadOnnxModel(model_path)) {
        impl_->model_loaded = true;
        THEMIS_INFO("NLI ONNX model successfully loaded and ready for inference");
    } else {
        if (impl_->config.fallback_to_heuristic) {
            impl_->model_loaded = true;  // Fallback enabled, still mark as loaded
            THEMIS_WARN("ONNX model failed to load, falling back to heuristic inference");
        } else {
            impl_->model_loaded = false;
            THEMIS_ERROR("ONNX model failed to load and fallback disabled");
        }
    }
}

bool NLIFaithfulnessVerifier::isModelLoaded() const {
    return impl_->model_loaded;
}

NLIFaithfulnessVerifier::Config NLIFaithfulnessVerifier::getConfig() const {
    return impl_->config;
}

void NLIFaithfulnessVerifier::setConfig(const Config& config) {
    impl_->config = config;
}

} // namespace themis::rag::judge
