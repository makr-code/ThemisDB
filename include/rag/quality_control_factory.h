/**
 * @file quality_control_factory.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include "rag/quality_control_pipeline.h"
#include "rag/continuous_learning_client.h"
#include "rag/llm_judge_client.h"
#include "rag/geval_evaluator.h"
#include "rag/nli_faithfulness_verifier.h"
#include "llm/inference_engine_enhanced.h"
#include <memory>
#include <string>

namespace themis::rag::judge {

/**
 * @brief Quality Control Factory
 * 
 * Provides convenience methods for creating fully-configured quality
 * control pipelines with sensible defaults.
 */
class QualityControlFactory {
public:
    /**
     * @brief Configuration for quality control setup
     */
    struct SetupConfig {
        // Model paths (optional - uses heuristic fallback if not provided)
        std::string nli_model_path;
        std::string nli_tokenizer_path;
        
        // LLM inference engine (optional - required for LLM Judge Client)
        std::shared_ptr<llm::InferenceEngineEnhanced> inference_engine;
        
        // Quality control mode
        QCMode default_mode = QCMode::BALANCED;
        
        // Feature toggles
        bool enable_nli = true;
        bool enable_geval = false;  // Optional, adds latency
        bool enable_llm_judge = false;  // Requires inference_engine
        
        // Performance tuning
        bool enable_caching = true;
        bool use_gpu_for_nli = false;
        int num_threads = 4;
        
        // Continuous learning
        bool log_to_continuous_learning = false;
        std::string cl_endpoint;
    };
    
    /**
     * @brief Create a basic quality control pipeline
     * 
     * Uses heuristic fallbacks for all components. Good for testing
     * and development without requiring model files.
     * 
     * @param mode Default evaluation mode
     * @return Configured quality control pipeline
     */
    static std::unique_ptr<QualityControlPipeline> createBasic(
        QCMode mode = QCMode::BALANCED
    );
    
    /**
     * @brief Create a production quality control pipeline
     * 
     * Fully configured with real models and inference engines.
     * Requires model paths and inference engine.
     * 
     * @param config Setup configuration with model paths
     * @return Configured quality control pipeline
     */
    static std::unique_ptr<QualityControlPipeline> createProduction(
        const SetupConfig& config
    );
    
    /**
     * @brief Create a lightweight pipeline for real-time use
     * 
     * Fast mode only, minimal features, optimized for latency.
     * 
     * @return Fast-mode quality control pipeline
     */
    static std::unique_ptr<QualityControlPipeline> createLightweight();
    
    /**
     * @brief Create a comprehensive pipeline for batch processing
     * 
     * Thorough mode, all features enabled, optimized for accuracy.
     * 
     * @param config Setup configuration
     * @return Thorough-mode quality control pipeline
     */
    static std::unique_ptr<QualityControlPipeline> createComprehensive(
        const SetupConfig& config
    );
    
    /**
     * @brief Create NLI verifier with automatic configuration
     * 
     * @param config Setup configuration
     * @return Configured NLI verifier
     */
    static std::shared_ptr<NLIFaithfulnessVerifier> createNLIVerifier(
        const SetupConfig& config
    );
    
    /**
     * @brief Create G-Eval evaluator with default configuration
     * 
     * @return Configured G-Eval evaluator
     */
    static std::shared_ptr<GEvalEvaluator> createGEvalEvaluator();
    
    /**
     * @brief Create LLM Judge Client if inference engine available
     * 
     * @param inference_engine Shared inference engine
     * @return Configured LLM Judge Client, or nullptr if engine not available
     */
    static std::shared_ptr<LLMJudgeClient> createLLMJudgeClient(
        std::shared_ptr<llm::InferenceEngineEnhanced> inference_engine
    );
};

/**
 * @brief Helper class for configuring quality control in RAG Judge
 */
class RAGJudgeQCConfigurator {
public:
    /**
     * @brief Configure RAG Judge with quality control features
     * 
     * @param enable_nli Enable NLI verification
     * @param enable_geval Enable G-Eval scoring
     * @param enable_full_pipeline Use full QC pipeline instead of basic judge
     * @return Configured RAG Judge config
     */
    static RAGJudgeConfig configure(
        bool enable_nli = true,
        bool enable_geval = false,
        bool enable_full_pipeline = false
    );
    
    /**
     * @brief Get recommended configuration for production
     * 
     * @return Production-ready RAG Judge config
     */
    static RAGJudgeConfig getProductionConfig();
    
    /**
     * @brief Get recommended configuration for development/testing
     * 
     * @return Development-friendly RAG Judge config
     */
    static RAGJudgeConfig getDevelopmentConfig();
};

} // namespace themis::rag::judge
