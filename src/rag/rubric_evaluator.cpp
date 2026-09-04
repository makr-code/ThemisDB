/**
 * @file rubric_evaluator.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "rag/rubric_evaluator.h"
#include "rag/llm_judge_integration.h"
#include "rag/response_parser.h"
#include "utils/logger.h"
#include <nlohmann/json.hpp>
#include <mutex>
#include <sstream>
#include <fstream>

namespace themis::rag::judge {

using json = nlohmann::json;

struct RubricEvaluator::Impl {
    Config config;
    EvaluationRubric active_rubric;
    std::unique_ptr<LLMJudgeIntegration> llm_integration;
    ResponseParser parser;
    mutable std::mutex state_mutex;  // Protect shared state access
    
    // Generate rubric-based prompt
    std::string generateRubricPrompt(
        const std::string& query,
        const std::string& answer,
        const std::vector<std::pair<std::string, std::string>>& documents,
        const DimensionRubric& dimension_rubric
    ) {
        std::ostringstream prompt = {};
        
        prompt << "Evaluate the following answer using this rubric for " 
               << dimension_rubric.dimension_name << ":\n\n";
        
        // Add rubric levels
        prompt << "Rubric:\n";
        for (const auto& level : dimension_rubric.levels) {
            prompt << "Level " << level.score << ": " << level.description << "\n";
            if (!level.criteria.empty()) {
                prompt << "  Criteria:\n";
                for (const auto& criterion : level.criteria) {
                    prompt << "  - " << criterion << "\n";
                }
            }
        }
        prompt << "\n";
        
        // Add query and answer
        prompt << "Query: " << query << "\n\n";
        
        if (!documents.empty()) {
            prompt << "Retrieved Documents:\n";
            for (size_t i = 0; i < std::min(documents.size(), size_t(3)); ++i) {
                prompt << "Doc " << (i+1) << ": " << documents[i].second << "\n";
            }
            prompt << "\n";
        }
        
        prompt << "Answer: " << answer << "\n\n";
        
        prompt << R"(Evaluate the answer and provide:
{
  "level": 1-5,
  "score": 0.0-1.0,
  "reasoning": "Brief explanation of why this level was assigned"
}

Evaluation:)";
        
        return prompt.str();
    }
};

RubricEvaluator::RubricEvaluator()
    : RubricEvaluator(Config{}) {
}

RubricEvaluator::RubricEvaluator(const Config& config)
    : impl_(std::make_unique<Impl>()) {
    impl_->config = config;
    
    // Initialize LLM integration
    LLMJudgeIntegration::Config llm_config;
    llm_config.model_name = "default";
    llm_config.temperature = 0.2;  // Low temperature for consistent rubric application
    llm_config.max_tokens = 512;
    impl_->llm_integration = std::make_unique<LLMJudgeIntegration>(llm_config);
    
    // Load default rubric
    impl_->active_rubric = createDefaultRubric();
    
    THEMIS_DEBUG("RubricEvaluator initialized");
}

RubricEvaluator::~RubricEvaluator() = default;

bool RubricEvaluator::loadRubricFromYAML(const std::string& yaml_content) {
    // Note: In production, this would use a YAML parser like yaml-cpp
    // For now, we'll use a simplified JSON-based approach
    
    try {
        json j = json::parse(yaml_content);
        
        EvaluationRubric rubric;
        rubric.name = j.value("name", "custom_rubric");
        rubric.description = j.value("description", "");
        rubric.domain = j.value("domain", "general");
        rubric.version = j.value("version", "1.0");
        rubric.author = j.value("author", "");
        
        if (j.contains("dimensions")) {
            for (const auto& dim_json : j["dimensions"]) {
                DimensionRubric dim;
                dim.dimension_name = dim_json.value("name", "");
                dim.description = dim_json.value("description", "");
                dim.weight = dim_json.value("weight", 0.25);
                
                if (dim_json.contains("levels")) {
                    for (const auto& level_json : dim_json["levels"]) {
                        RubricLevel level;
                        level.score = level_json.value("score", 1);
                        level.description = level_json.value("description", "");
                        
                        if (level_json.contains("criteria")) {
                            level.criteria = level_json["criteria"].get<std::vector<std::string>>();
                        }
                        
                        dim.levels.push_back(level);
                    }
                }
                
                rubric.dimensions.push_back(dim);
            }
        }
        
        if (validateRubric(rubric)) {
            impl_->active_rubric = rubric;
            THEMIS_INFO("Loaded rubric: {}", rubric.name);
            return true;
        }
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("Failed to load rubric from YAML: {}", e.what());
    }
    
    return false;
}

bool RubricEvaluator::loadRubricFromFile(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        THEMIS_ERROR("Failed to open rubric file: {}", filepath);
        return false;
    }
    
    std::stringstream buffer = {};
    buffer << file.rdbuf();
    
    return loadRubricFromYAML(buffer.str());
}

void RubricEvaluator::setRubric(const EvaluationRubric& rubric) {
    if (validateRubric(rubric)) {
        impl_->active_rubric = rubric;
        THEMIS_INFO("Set active rubric: {}", rubric.name);
    } else {
        THEMIS_ERROR("Invalid rubric provided");
    }
}

const EvaluationRubric& RubricEvaluator::getRubric() const {
    return impl_->active_rubric;
}

RubricEvaluationResult RubricEvaluator::evaluate(
    const std::string& query,
    const std::string& answer,
    const std::vector<std::pair<std::string, std::string>>& documents
) {
    RubricEvaluationResult result;
    result.rubric_name = impl_->active_rubric.name;
    
    THEMIS_DEBUG("Evaluating with rubric: {}", impl_->active_rubric.name);
    
    // Evaluate each dimension
    double total_weighted_score = 0.0;
    double total_weight = 0.0;
    
    for (const auto& dimension : impl_->active_rubric.dimensions) {
        std::string prompt = impl_->generateRubricPrompt(query, answer, documents, dimension);
        
        try {
            std::string response = impl_->llm_integration->evaluateDimension(
                prompt, EvaluationDimension::OVERALL
            );
            
            json result_json = impl_->parser.parseJSONResponse(response);
            
            int level = result_json.value("level", 3);
            double score = result_json.value("score", normalizeScore(level));
            std::string reasoning = result_json.value("reasoning", "");
            
            result.dimension_levels[dimension.dimension_name] = level;
            result.dimension_scores[dimension.dimension_name] = score;
            result.dimension_reasoning[dimension.dimension_name] = reasoning;
            
            total_weighted_score += score * dimension.weight;
            total_weight += dimension.weight;
            
        } catch (const std::exception& e) {
            THEMIS_WARN("Dimension evaluation failed for {}: {}", 
                       dimension.dimension_name, e.what());
            
            // Fallback to mid-level score
            result.dimension_levels[dimension.dimension_name] = 3;
            result.dimension_scores[dimension.dimension_name] = normalizeScore(3);
            total_weighted_score += normalizeScore(3) * dimension.weight;
            total_weight += dimension.weight;
        }
    }
    
    // Calculate overall score
    result.overall_score = total_weight > 0 ? total_weighted_score / total_weight : 0.5;
    
    // Generate overall reasoning
    std::ostringstream reasoning = {};
    reasoning << "Rubric: " << impl_->active_rubric.name << "\n";
    reasoning << "Overall Score: " << result.overall_score << "\n";
    reasoning << "Dimension Scores:\n";
    
    for (const auto& dimension : impl_->active_rubric.dimensions) {
        const std::string& name = dimension.dimension_name;
        reasoning << "- " << name << ": Level " << result.dimension_levels[name]
                  << " (score: " << result.dimension_scores[name] << ")\n";
        if (!result.dimension_reasoning[name].empty()) {
            reasoning << "  " << result.dimension_reasoning[name] << "\n";
        }
    }
    
    result.overall_reasoning = reasoning.str();
    
    THEMIS_INFO("Rubric evaluation complete: overall={:.2f}", result.overall_score);
    
    return result;
}

EvaluationRubric RubricEvaluator::createDefaultRubric() {
    EvaluationRubric rubric;
    rubric.name = "default_rag_rubric";
    rubric.description = "Default rubric for RAG answer evaluation";
    rubric.domain = "general";
    rubric.version = "1.0";
    rubric.author = "ThemisDB";
    
    // Faithfulness dimension
    {
        DimensionRubric dim;
        dim.dimension_name = "Faithfulness";
        dim.description = "Factual accuracy and support from documents";
        dim.weight = 0.35;
        
        dim.levels = {
            {5, "Fully supported - All claims directly supported by documents", {}, 
             {"All claims verifiable", "No hallucinations", "Strong evidence"}},
            {4, "Mostly supported - Most claims supported with minor gaps", {}, 
             {"Most claims verifiable", "Minor unsupported details"}},
            {3, "Partially supported - Some claims supported", {}, 
             {"Mixed support", "Some unsupported claims"}},
            {2, "Weakly supported - Few claims supported", {}, 
             {"Mostly unsupported", "Significant gaps"}},
            {1, "Not supported - Claims contradict or absent from documents", {}, 
             {"Contradictions present", "Hallucinations"}}
        };
        
        rubric.dimensions.push_back(dim);
    }
    
    // Relevance dimension
    {
        DimensionRubric dim;
        dim.dimension_name = "Relevance";
        dim.description = "How well the answer addresses the query";
        dim.weight = 0.25;
        
        dim.levels = {
            {5, "Directly addresses all aspects of the query", {}, {}},
            {4, "Addresses most aspects with minor tangents", {}, {}},
            {3, "Partially addresses query with some irrelevant content", {}, {}},
            {2, "Mostly irrelevant or off-topic", {}, {}},
            {1, "Completely irrelevant to query", {}, {}}
        };
        
        rubric.dimensions.push_back(dim);
    }
    
    // Completeness dimension
    {
        DimensionRubric dim;
        dim.dimension_name = "Completeness";
        dim.description = "Coverage of all query aspects and depth";
        dim.weight = 0.25;
        
        dim.levels = {
            {5, "Comprehensive coverage with appropriate depth", {}, {}},
            {4, "Good coverage with minor gaps", {}, {}},
            {3, "Partial coverage, some aspects missing", {}, {}},
            {2, "Incomplete, major gaps", {}, {}},
            {1, "Minimal coverage, severely incomplete", {}, {}}
        };
        
        rubric.dimensions.push_back(dim);
    }
    
    // Coherence dimension
    {
        DimensionRubric dim;
        dim.dimension_name = "Coherence";
        dim.description = "Logical structure and clarity";
        dim.weight = 0.15;
        
        dim.levels = {
            {5, "Excellent structure, clear and logical", {}, {}},
            {4, "Good structure with minor issues", {}, {}},
            {3, "Adequate structure, some confusion", {}, {}},
            {2, "Poor structure, hard to follow", {}, {}},
            {1, "Incoherent, no clear structure", {}, {}}
        };
        
        rubric.dimensions.push_back(dim);
    }
    
    return rubric;
}

bool RubricEvaluator::validateRubric(const EvaluationRubric& rubric) {
    if (rubric.name.empty()) {
        THEMIS_ERROR("Rubric name is empty");
        return false;
    }
    
    if (rubric.dimensions.empty()) {
        THEMIS_ERROR("Rubric has no dimensions");
        return false;
    }
    
    double total_weight = 0.0;
    for (const auto& dim : rubric.dimensions) {
        if (dim.dimension_name.empty()) {
            THEMIS_ERROR("Dimension name is empty");
            return false;
        }
        
        if (dim.levels.empty()) {
            THEMIS_ERROR("Dimension {} has no levels", dim.dimension_name);
            return false;
        }
        
        total_weight += dim.weight;
    }
    
    // Check if weights sum to approximately 1.0
    if (std::abs(total_weight - 1.0) > 0.01) {
        THEMIS_WARN("Dimension weights sum to {}, not 1.0", total_weight);
    }
    
    return true;
}

double RubricEvaluator::normalizeScore([[maybe_unused]] int level_score) {
    // Convert 1-5 to 0-1 scale
    return (level_score - 1.0) / 4.0;
}

} // namespace themis::rag::judge
