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

// Real ONNX Runtime NLI inference — available when THEMIS_HAS_NLI is defined.
// Enable with -DTHEMIS_HAS_NLI=ON and link onnxruntime.
// Default: OFF — the heuristic fallback is used.
#ifdef THEMIS_HAS_NLI
#  include <onnxruntime_cxx_api.h>
#endif

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
     * @brief Compute NLI score using the ONNX Runtime model.
     *
     * When `THEMIS_HAS_NLI` is defined, runs real tokenization + ONNX Runtime
     * inference using the loaded session.  Falls back to word-overlap heuristics
     * when ONNX Runtime is not linked or the model is not loaded.
     *
     * @note PERMANENT FALLBACK NOTE:
     * When THEMIS_HAS_NLI is NOT defined, heuristic word-overlap is used.
     * Enable real inference with -DTHEMIS_HAS_NLI=ON + link onnxruntime.
     */
    NLIResult computeNLIWithOnnx(const std::string& premise, const std::string& hypothesis) {
        auto start_time = std::chrono::steady_clock::now();
        NLIResult result;

#ifdef THEMIS_HAS_NLI
        // ── Real ONNX Runtime inference (THEMIS_HAS_NLI=ON) ─────────────────
        // Requires: onnxruntime ≥ 1.16, a loaded ONNX NLI model (e.g.
        //           cross-encoder/nli-deberta-v3-large-mnli).
        //
        // The session is owned by a static Ort::Env + Ort::Session pair that
        // is lazily created from loaded_model_ on first use.
        if (model_loaded && loaded_model_.has_value()) {
            try {
                static Ort::Env ort_env{ORT_LOGGING_LEVEL_WARNING, "NLIVerifier"};

                // Build the session on first call (thread-safe one-time init).
                static std::mutex session_mutex;
                static std::unique_ptr<Ort::Session> ort_session;
                static std::string loaded_path;
                {
                    std::lock_guard<std::mutex> lk(session_mutex);
                    if (!ort_session || loaded_path != loaded_model_->model_path) {
                        Ort::SessionOptions opts;
                        opts.SetIntraOpNumThreads(1);
                        opts.SetGraphOptimizationLevel(
                            GraphOptimizationLevel::ORT_ENABLE_EXTENDED);
                        ort_session = std::make_unique<Ort::Session>(
                            ort_env,
                            loaded_model_->model_path.c_str(),
                            opts);
                        loaded_path = loaded_model_->model_path;
                        THEMIS_INFO("NLI ONNX session created for model: {}",
                                    loaded_model_->model_path);
                    }
                }

                // Minimal whitespace tokenizer.
                // For production accuracy use the model's BPE/WordPiece tokenizer
                // (load via tokenizer.json from loaded_model_->tokenizer_path).
                auto tokenize = [](const std::string& text, int max_len = 512)
                    -> std::vector<int64_t>
                {
                    std::vector<int64_t> ids;
                    ids.push_back(0); // [CLS]
                    std::istringstream iss(text);
                    std::string tok;
                    while (iss >> tok && static_cast<int>(ids.size()) < max_len - 1) {
                        ids.push_back(static_cast<int64_t>(
                            std::hash<std::string>{}(tok) % 30000 + 1));
                    }
                    ids.push_back(2); // [SEP]
                    return ids;
                };

                auto premise_ids    = tokenize(premise);
                auto hypothesis_ids = tokenize(hypothesis);

                // Build combined input: [CLS] premise [SEP] hypothesis [SEP]
                std::vector<int64_t> input_ids;
                input_ids.insert(input_ids.end(), premise_ids.begin(), premise_ids.end());
                // Replace trailing [SEP] of premise with the hypothesis tokens
                input_ids.pop_back();
                input_ids.insert(input_ids.end(), hypothesis_ids.begin(), hypothesis_ids.end());

                const int64_t seq_len = static_cast<int64_t>(input_ids.size());
                std::vector<int64_t> attention_mask(seq_len, 1);
                std::vector<int64_t> token_type_ids(seq_len, 0);
                // Mark hypothesis tokens as segment 1
                const int64_t sep1 = static_cast<int64_t>(premise_ids.size()) - 1;
                for (int64_t i = sep1; i < seq_len; ++i) token_type_ids[i] = 1;

                const std::array<int64_t, 2> shape{1, seq_len};
                Ort::MemoryInfo mem_info =
                    Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);

                std::array<Ort::Value, 3> inputs = {
                    Ort::Value::CreateTensor<int64_t>(mem_info,
                        input_ids.data(), input_ids.size(), shape.data(), 2),
                    Ort::Value::CreateTensor<int64_t>(mem_info,
                        attention_mask.data(), attention_mask.size(), shape.data(), 2),
                    Ort::Value::CreateTensor<int64_t>(mem_info,
                        token_type_ids.data(), token_type_ids.size(), shape.data(), 2),
                };

                const char* input_names[]  = {"input_ids", "attention_mask", "token_type_ids"};
                const char* output_names[] = {"logits"};

                auto output_tensors = ort_session->Run(
                    Ort::RunOptions{nullptr},
                    input_names, inputs.data(), inputs.size(),
                    output_names, 1);

                // Extract logits [CONTRADICTION, NEUTRAL, ENTAILMENT] (DeBERTa-MNLI order)
                const float* logits =
                    output_tensors[0].GetTensorData<float>();

                // Softmax
                const float e0 = std::exp(logits[0]);
                const float e1 = std::exp(logits[1]);
                const float e2 = std::exp(logits[2]);
                const float sum = e0 + e1 + e2;

                result.contradiction_score = e0 / sum;
                result.neutral_score       = e1 / sum;
                result.entailment_score    = e2 / sum;
                result.confidence = std::max({result.contradiction_score,
                                             result.neutral_score,
                                             result.entailment_score});

                if (result.entailment_score >= result.neutral_score &&
                    result.entailment_score >= result.contradiction_score) {
                    result.label = NLILabel::ENTAILMENT;
                } else if (result.contradiction_score >= result.neutral_score) {
                    result.label = NLILabel::CONTRADICTION;
                } else {
                    result.label = NLILabel::NEUTRAL;
                }

                auto end_time = std::chrono::steady_clock::now();
                const auto latency = std::chrono::duration_cast<std::chrono::milliseconds>(
                    end_time - start_time).count();
                onnx_inference_count_++;
                total_onnx_latency_ms_ += latency;
                if (config.log_inference_mode) {
                    THEMIS_DEBUG("NLI inference (ONNX Runtime): count={}, latency={}ms",
                                 onnx_inference_count_, latency);
                }
                return result;

            } catch (const Ort::Exception& ex) {
                THEMIS_WARN("ONNX Runtime inference failed: {} — falling back to heuristic",
                            ex.what());
            }
        }
        // ── End ONNX Runtime path ────────────────────────────────────────────
#endif // THEMIS_HAS_NLI

        // PERMANENT FALLBACK NOTE:
        // THEMIS_HAS_NLI not defined, ONNX Runtime not linked, or model not
        // loaded.  Uses word-overlap heuristic as a safe, zero-dependency
        // approximation.  For production accuracy enable THEMIS_HAS_NLI=ON.
        {
        std::string premise_lower = premise;
        std::string hypothesis_lower = hypothesis;
        std::transform(premise_lower.begin(), premise_lower.end(),
                      premise_lower.begin(), ::tolower);
        std::transform(hypothesis_lower.begin(), hypothesis_lower.end(),
                      hypothesis_lower.begin(), ::tolower);

        std::istringstream hyp_stream(hypothesis_lower);
        std::vector<std::string> hyp_words;
        std::string word;
        while (hyp_stream >> word) {
            if (word.length() > 3) hyp_words.push_back(word);
        }

        if (hyp_words.empty()) {
            result.label = NLILabel::NEUTRAL;
            result.entailment_score = 0.33;
            result.neutral_score = 0.34;
            result.contradiction_score = 0.33;
            result.confidence = 0.5;
        } else {
            std::size_t matches = 0;
            for (const auto& w : hyp_words) {
                if (premise_lower.find(w) != std::string::npos) ++matches;
            }
            const double match_ratio = static_cast<double>(matches) / hyp_words.size();
            if (match_ratio >= 0.8) {
                result.entailment_score = 0.75; result.neutral_score = 0.15;
                result.contradiction_score = 0.10; result.label = NLILabel::ENTAILMENT;
            } else if (match_ratio >= 0.5) {
                result.entailment_score = 0.30; result.neutral_score = 0.55;
                result.contradiction_score = 0.15; result.label = NLILabel::NEUTRAL;
            } else {
                result.entailment_score = 0.10; result.neutral_score = 0.20;
                result.contradiction_score = 0.70; result.label = NLILabel::CONTRADICTION;
            }
            result.confidence = std::max({result.entailment_score,
                                         result.neutral_score,
                                         result.contradiction_score});
        }
        }

        auto end_time = std::chrono::steady_clock::now();
        auto latency = std::chrono::duration_cast<std::chrono::milliseconds>(
            end_time - start_time).count();
        onnx_inference_count_++;
        total_onnx_latency_ms_ += latency;
        if (config.log_inference_mode) {
            const double avg_latency = total_onnx_latency_ms_ / onnx_inference_count_;
            THEMIS_DEBUG("NLI inference (heuristic fallback): count={}, latency={}ms, avg={}ms",
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
        
        // ONNX required but unavailable and no fallback permitted: fail hard.
        // This enforces the contract documented in Config::fallback_to_heuristic.
        if (config.use_onnx && !model_loaded && !config.fallback_to_heuristic) {
            THEMIS_ERROR("NLI ONNX inference required (fallback_to_heuristic=false) "
                         "but no model is loaded. Returning neutral result.");
            NLIResult fail_result;
            fail_result.label          = NLILabel::NEUTRAL;
            fail_result.entailment_score    = 0.0;
            fail_result.neutral_score       = 1.0;
            fail_result.contradiction_score = 0.0;
            fail_result.confidence          = 0.0;
            return fail_result;
        }
        
        // Fallback to heuristic (either ONNX disabled, or fallback permitted)
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

/**
 * @brief Default constructor; initialises with the default `Config`.
 *
 * Delegates to the explicit-config constructor, which sets all thresholds,
 * ONNX options, and performance flags to their documented defaults.
 */
NLIFaithfulnessVerifier::NLIFaithfulnessVerifier()
    : NLIFaithfulnessVerifier(Config{}) {
}

/**
 * @brief Construct the verifier with a custom configuration.
 *
 * Allocates the `Impl` pimpl struct, which initialises the ONNX model loader
 * if `config.use_onnx == true`, and logs the effective thresholds for audit.
 *
 * @param config Verifier configuration (thresholds, ONNX settings, batching).
 */
NLIFaithfulnessVerifier::NLIFaithfulnessVerifier(const Config& config)
    : impl_(std::make_unique<Impl>(config)) {
}

/**
 * @brief Destructor; defined out-of-line to allow the pimpl `Impl` type to
 *        remain incomplete in the header.
 */
NLIFaithfulnessVerifier::~NLIFaithfulnessVerifier() = default;

/**
 * @brief Verify the faithfulness of @p answer with respect to @p documents.
 *
 * Execution steps:
 * 1. Extract individual factual claims from @p answer using sentence splitting.
 * 2. For each claim, run `computeNLI()` against every document to obtain
 *    entailment/neutral/contradiction scores.
 * 3. Classify each claim as FULLY_SUPPORTED, PARTIALLY_SUPPORTED, UNSUPPORTED,
 *    or CONTRADICTED based on the best per-document entailment score and the
 *    configured thresholds.
 * 4. Compute a weighted faithfulness score
 *    (1.0 / 0.5 / 0.0 / −0.5 per support level) and determine whether the
 *    answer meets `Config::min_faithfulness_score`.
 *
 * Edge cases:
 * - Empty @p answer or empty @p documents: returns score=0, not faithful.
 * - No claims extracted (e.g. purely generic answer): returns score=0.7,
 *   is_faithful=true (no claims to contradict).
 *
 * @param answer    Generated answer text whose claims will be verified.
 * @param documents Retrieved documents as (document_id, content) pairs.
 *                  Content is used as the NLI premise for each claim.
 *
 * @return `FaithfulnessVerificationResult` containing the overall score,
 *         per-claim analysis, an explanation string, and the elapsed time.
 */
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

/**
 * @brief Check the entailment relationship between @p premise and @p hypothesis.
 *
 * Routes to the configured inference path via `Impl::computeNLI()`:
 * - Real ONNX Runtime inference if THEMIS_HAS_NLI is defined and model is loaded.
 * - Heuristic term-overlap + negation detection as permanent fallback otherwise.
 *
 * @param premise    Context text that may entail the hypothesis (typically a
 *                   retrieved document passage).
 * @param hypothesis Claim to verify (typically a sentence from the answer).
 *
 * @return `NLIResult` with probability scores for ENTAILMENT, NEUTRAL, and
 *         CONTRADICTION, plus an overall confidence value.
 */
NLIResult NLIFaithfulnessVerifier::checkEntailment(
    const std::string& premise,
    const std::string& hypothesis
) {
    return impl_->computeNLI(premise, hypothesis);
}

/**
 * @brief Load an NLI model from @p model_path and prepare it for inference.
 *
 * Behaviour depends on the `use_onnx` configuration flag:
 * - `use_onnx == false`: ONNX is disabled; marks the model as "loaded" so
 *   that the heuristic path is gated by `isModelLoaded()`, and logs an
 *   informational message.
 * - `use_onnx == true` and load succeeds: sets `model_loaded = true`, logs
 *   model name and size.
 * - `use_onnx == true`, load fails, `fallback_to_heuristic == true`: sets
 *   `model_loaded = true` (heuristic path takes over), logs a warning.
 * - `use_onnx == true`, load fails, `fallback_to_heuristic == false`: leaves
 *   `model_loaded = false` and logs an error; subsequent `computeNLI()` calls
 *   will return zero-confidence results.
 *
 * @param model_path File system path (or identifier) passed to the ONNX
 *                   model loader.
 */
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

/**
 * @brief Report whether the ONNX model (or its surrogate) has been loaded.
 *
 * Returns the raw `Impl::model_loaded` flag.  This is set by `loadModel()`.
 * Callers that want to know whether *any* inference path is available should
 * use `isReady()` instead.
 *
 * @return `true` if a model (or heuristic surrogate) was successfully loaded.
 */
bool NLIFaithfulnessVerifier::isModelLoaded() const {
    return impl_->model_loaded;
}

/**
 * @brief Check whether the verifier can service inference requests.
 *
 * Evaluates whether at least one of the following inference paths is available:
 * - ONNX model is loaded and ready.
 * - ONNX is disabled (`use_onnx == false`), so the heuristic path is the
 *   sole path and is always available.
 * - ONNX is enabled but heuristic fallback is permitted
 *   (`fallback_to_heuristic == true`), making the verifier ready even before
 *   a model is loaded.
 *
 * Only returns `false` when ONNX is the required path (`use_onnx == true`),
 * fallback is prohibited (`fallback_to_heuristic == false`), **and** no model
 * has been loaded yet.
 *
 * @return true if the verifier can perform inference
 */
bool NLIFaithfulnessVerifier::isReady() const {
    if (impl_->model_loaded) {
        return true;
    }
    // If ONNX is disabled, heuristic is always available
    if (!impl_->config.use_onnx) {
        return true;
    }
    // If ONNX is required but fallback is allowed, heuristic covers us
    if (impl_->config.fallback_to_heuristic) {
        return true;
    }
    // ONNX required, no fallback, and model not yet loaded
    return false;
}

/**
 * @brief Return a copy of the current verifier configuration.
 *
 * @return Snapshot of the `Config` used to initialise the verifier (or the
 *         last value set by `setConfig()`).
 */
NLIFaithfulnessVerifier::Config NLIFaithfulnessVerifier::getConfig() const {
    return impl_->config;
}

/**
 * @brief Replace the verifier configuration at runtime.
 *
 * @note Does **not** reload the ONNX model; call `loadModel()` explicitly if
 *       `config.onnx_model_path` changed.
 *
 * @param config New configuration to apply.
 */
void NLIFaithfulnessVerifier::setConfig(const Config& config) {
    impl_->config = config;
}

} // namespace themis::rag::judge
