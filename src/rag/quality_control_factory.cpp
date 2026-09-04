/**
 * @file quality_control_factory.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=5, M=0, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "rag/quality_control_factory.h"
#include "utils/logger.h"

namespace themis::rag::judge {

namespace {

QualityControlPipeline::Config makePipelineConfigForMode(QCMode mode) {
    QualityControlPipeline::Config config;
    switch (mode) {
        case QCMode::FAST:
            config.enable_fast_stage = true;
            config.enable_balanced_stage = false;
            config.enable_thorough_stage = false;
            break;
        case QCMode::BALANCED:
            config.enable_fast_stage = true;
            config.enable_balanced_stage = true;
            config.enable_thorough_stage = false;
            break;
        case QCMode::THOROUGH:
            config.enable_fast_stage = true;
            config.enable_balanced_stage = true;
            config.enable_thorough_stage = true;
            break;
    }
    return config;
}

} // namespace

// ═══════════════════════════════════════════════════════════
// QualityControlFactory Implementation
// ═══════════════════════════════════════════════════════════

std::unique_ptr<QualityControlPipeline> QualityControlFactory::createBasic(QCMode mode) {
    auto config = makePipelineConfigForMode(mode);
    
    THEMIS_INFO("Creating basic quality control pipeline (mode: {})", static_cast<int>(mode));
    
    return std::make_unique<QualityControlPipeline>(config);
}

std::unique_ptr<QualityControlPipeline> QualityControlFactory::createProduction(
    const SetupConfig& setup_config
) {
    THEMIS_INFO("Creating production quality control pipeline");
    
    // Create NLI verifier
    auto nli_verifier = createNLIVerifier(setup_config);
    
    // Create G-Eval evaluator if enabled
    std::shared_ptr<GEvalEvaluator> geval_evaluator = {};

    if (setup_config.enable_geval) {
        geval_evaluator = createGEvalEvaluator();
    }
    
    // Create LLM Judge Client if inference engine available
    std::shared_ptr<LLMJudgeClient> llm_judge_client = {};

    if (setup_config.enable_llm_judge && setup_config.inference_engine) {
        llm_judge_client = createLLMJudgeClient(setup_config.inference_engine);
    } else if (setup_config.enable_llm_judge) {
        THEMIS_WARN("LLM Judge requested but no inference engine provided");
    }
    
    auto config = makePipelineConfigForMode(setup_config.default_mode);
    config.enable_learning_feedback = setup_config.log_to_continuous_learning;
    config.learning_orchestrator_url = setup_config.cl_endpoint;

    auto pipeline = std::make_unique<QualityControlPipeline>(config);
    if (nli_verifier && setup_config.enable_nli) {
        pipeline->setNLIVerifier(nli_verifier);
    }
    if (geval_evaluator && setup_config.enable_geval) {
        pipeline->setGEvalEvaluator(geval_evaluator);
    }
    if (llm_judge_client && setup_config.enable_llm_judge) {
        pipeline->setLLMJudgeClient(llm_judge_client);
    }

    return pipeline;
}

std::unique_ptr<QualityControlPipeline> QualityControlFactory::createLightweight() {
    THEMIS_INFO("Creating lightweight quality control pipeline (Fast mode)");
    
    auto config = makePipelineConfigForMode(QCMode::FAST);
    config.fast_stage_timeout_ms = 50;
    
    return std::make_unique<QualityControlPipeline>(config);
}

std::unique_ptr<QualityControlPipeline> QualityControlFactory::createComprehensive(
    const SetupConfig& setup_config
) {
    THEMIS_INFO("Creating comprehensive quality control pipeline (Thorough mode)");
    
    SetupConfig comprehensive_config = setup_config;
    comprehensive_config.default_mode = QCMode::THOROUGH;
    comprehensive_config.enable_nli = true;
    comprehensive_config.enable_geval = true;
    comprehensive_config.enable_llm_judge = true;
    
    return createProduction(comprehensive_config);
}

std::shared_ptr<NLIFaithfulnessVerifier> QualityControlFactory::createNLIVerifier(
    const SetupConfig& config
) {
    NLIFaithfulnessVerifier::Config nli_config;
    
    if (!config.nli_model_path.empty()) {
        nli_config.model_path = config.nli_model_path;
        THEMIS_INFO("NLI Verifier configured with model: {}", config.nli_model_path);
    } else {
        THEMIS_INFO("NLI Verifier using heuristic fallback (no model path provided)");
    }
    
    nli_config.use_gpu = config.use_gpu_for_nli;
    
    return std::make_shared<NLIFaithfulnessVerifier>(nli_config);
}

std::shared_ptr<GEvalEvaluator> QualityControlFactory::createGEvalEvaluator() {
    GEvalEvaluator::Config config;
    config.num_samples = 3;
    config.aggregation = AggregationMethod::MEAN;
    config.temperature = 0.7;
    config.extract_reasoning = true;
    
    THEMIS_INFO("G-Eval Evaluator created with {} samples", config.num_samples);
    
    return std::make_shared<GEvalEvaluator>(config);
}

std::shared_ptr<LLMJudgeClient> QualityControlFactory::createLLMJudgeClient(
    std::shared_ptr<llm::InferenceEngineEnhanced> inference_engine
) {
    if (!inference_engine) {
        THEMIS_ERROR("Cannot create LLM Judge Client: inference engine is null");
        return nullptr;
    }
    
    LLMJudgeClient::Config config;
    config.model_name = "default";
    config.temperature = 0.3;
    config.max_tokens = 1024;
    config.enable_caching = true;
    config.timeout_ms = 30000;
    
    auto client = std::make_shared<LLMJudgeClient>(config);
    client->setInferenceEngine(std::move(inference_engine));

    THEMIS_INFO("LLM Judge Client created with model: {}", config.model_name);
    
    return client;
}

// ═══════════════════════════════════════════════════════════
// RAGJudgeQCConfigurator Implementation
// ═══════════════════════════════════════════════════════════

RAGJudgeConfig RAGJudgeQCConfigurator::configure(
    bool enable_nli,
    bool enable_geval,
    bool enable_full_pipeline
) {
    RAGJudgeConfig config;
    
    // Enable quality control features
    config.use_nli_verifier = enable_nli;
    config.use_geval_scoring = enable_geval;
    config.use_quality_control_pipeline = enable_full_pipeline;
    
    // Basic RAG Judge settings
    config.mode = EvaluationMode::BALANCED;
    config.enable_claim_verification = true;
    config.enable_citation_check = true;
    
    THEMIS_INFO("RAG Judge configured with QC features: NLI={}, G-Eval={}, Pipeline={}",
                enable_nli, enable_geval, enable_full_pipeline);
    
    return config;
}

RAGJudgeConfig RAGJudgeQCConfigurator::getProductionConfig() {
    RAGJudgeConfig config;
    
    // Production mode - thorough evaluation
    config.mode = EvaluationMode::THOROUGH;
    
    // Enable all quality control features
    config.use_nli_verifier = true;
    config.use_geval_scoring = true;
    config.use_quality_control_pipeline = false;  // Use RAG Judge's own pipeline
    config.use_llm_judge_client = true;
    
    // Enable advanced features
    config.enable_claim_verification = true;
    config.enable_citation_check = true;
    config.enable_ethical_evaluation = true;
    
    // Production thresholds
    config.quality_threshold = 0.75;
    config.faithfulness_threshold = 0.80;
    config.ethical_compliance_threshold = 0.70;
    
    // Performance optimizations
    config.cache_evaluations = true;
    config.max_claims_to_verify = 10;
    
    THEMIS_INFO("RAG Judge production config created");
    
    return config;
}

RAGJudgeConfig RAGJudgeQCConfigurator::getDevelopmentConfig() {
    RAGJudgeConfig config;
    
    // Development mode - balanced evaluation
    config.mode = EvaluationMode::BALANCED;
    
    // Enable basic quality control features
    config.use_nli_verifier = true;
    config.use_geval_scoring = false;  // Skip G-Eval for speed
    config.use_quality_control_pipeline = false;
    config.use_llm_judge_client = false;
    
    // Enable basic features
    config.enable_claim_verification = true;
    config.enable_citation_check = false;  // Skip citation check for speed
    config.enable_ethical_evaluation = false;  // Skip ethical for speed
    
    // Lenient thresholds for development
    config.quality_threshold = 0.60;
    config.faithfulness_threshold = 0.70;
    
    // Performance optimizations
    config.cache_evaluations = true;
    config.max_claims_to_verify = 5;  // Fewer claims for speed
    
    THEMIS_INFO("RAG Judge development config created");
    
    return config;
}

} // namespace themis::rag::judge

