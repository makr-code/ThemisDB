/**
 * @file ethical_guidelines_manager.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=6, M=17, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "llm/ethical_guidelines_manager.h"
#include "ethics_ai/ethics_ai_types.h"
#include "llm/llm_plugin_interface.h"
#include "utils/logger.h"
#include <yaml-cpp/yaml.h>
#include <nlohmann/json.hpp>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <regex>

// Map legacy logging calls to project-wide macros
#define LogInfo  THEMIS_INFO
#define LogWarning THEMIS_WARN
#define LogError THEMIS_ERROR

namespace themis {
namespace llm {

EthicalGuidelinesManager::EthicalGuidelinesManager(const std::string& config_path)
    : config_path_(config_path) {
    
    // Set default configuration
    config_.enabled = true;
    config_.detection_threshold = 0.6f;
    config_.enable_logging = true;
    config_.always_apply_default = true;
    config_.show_disclaimers = true;
    config_.language_mode = "both";
    
    // Load configuration
    if (!loadConfig(config_path)) {
        LogWarning("Failed to load ethical guidelines from {}, using defaults", config_path);
    }
}

EthicalGuidelinesManager::~EthicalGuidelinesManager() {
}

bool EthicalGuidelinesManager::loadConfig(const std::string& config_path) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    try {
        YAML::Node config = YAML::LoadFile(config_path);
        
        if (!config) {
            LogError("Failed to parse YAML file: {}", config_path);
            return false;
        }
        
        // Load configuration settings
        if (config["config"]) {
            auto cfg = config["config"];
            if (cfg["enabled"]) config_.enabled = cfg["enabled"].as<bool>();
            if (cfg["detection_threshold"]) config_.detection_threshold = cfg["detection_threshold"].as<float>();
            if (cfg["enable_logging"]) config_.enable_logging = cfg["enable_logging"].as<bool>();
            if (cfg["always_apply_default"]) config_.always_apply_default = cfg["always_apply_default"].as<bool>();
            if (cfg["show_disclaimers"]) config_.show_disclaimers = cfg["show_disclaimers"].as<bool>();
            if (cfg["language_mode"]) config_.language_mode = cfg["language_mode"].as<std::string>();
            
            // LLM-as-judge configuration
            if (cfg["use_llm_as_judge"]) config_.use_llm_as_judge = cfg["use_llm_as_judge"].as<bool>();
            if (cfg["llm_judge_threshold"]) config_.llm_judge_threshold = cfg["llm_judge_threshold"].as<float>();
            if (cfg["combine_with_keywords"]) config_.combine_with_keywords = cfg["combine_with_keywords"].as<bool>();
        }
        
        // Load core principles
        if (config["core_principles"]) {
            principles_.clear();
            for (const auto& p : config["core_principles"]) {
                Principle principle;
                principle.id = p["id"].as<std::string>();
                principle.name = p["name"].as<std::string>();
                principle.description = p["description"].as<std::string>();
                if (p["description_en"]) principle.description_en = p["description_en"].as<std::string>();
                principle.priority = p["priority"].as<int>();
                principles_.push_back(principle);
            }
        }
        
        // Load context detection keywords
        if (config["context_detection"]) {
            auto cd = config["context_detection"];
            
            if (cd["ethical_keywords"]) {
                if (cd["ethical_keywords"]["german"]) {
                    ethical_keywords_de_.clear();
                    for (const auto& kw : cd["ethical_keywords"]["german"]) {
                        ethical_keywords_de_.push_back(kw.as<std::string>());
                    }
                }
                if (cd["ethical_keywords"]["english"]) {
                    ethical_keywords_en_.clear();
                    for (const auto& kw : cd["ethical_keywords"]["english"]) {
                        ethical_keywords_en_.push_back(kw.as<std::string>());
                    }
                }
            }
            
            if (cd["high_autonomy_contexts"]) {
                high_autonomy_contexts_.clear();
                for (const auto& ctx : cd["high_autonomy_contexts"]) {
                    high_autonomy_contexts_.push_back(ctx.as<std::string>());
                }
            }
        }
        
        // Load augmentation templates
        if (config["prompt_augmentation"]) {
            auto pa = config["prompt_augmentation"];
            augmentation_templates_.clear();
            
            // Load each template type
            std::vector<std::string> template_names = {
                "default", "high_autonomy", "administrative", 
                "bias_prevention", "moral_imperatives"
            };
            
            for (const auto& name : template_names) {
                if (pa[name]) {
                    AugmentationTemplate tmpl;
                    if (pa[name]["system_prefix"]) {
                        tmpl.system_prefix = pa[name]["system_prefix"].as<std::string>();
                    }
                    if (pa[name]["response_suffix"]) {
                        tmpl.response_suffix = pa[name]["response_suffix"].as<std::string>();
                    }
                    augmentation_templates_[name] = tmpl;
                }
            }
        }
        
        // Load domain-specific guidelines
        if (config["domain_guidelines"]) {
            domain_guidelines_.clear();
            for (const auto& domain : config["domain_guidelines"]) {
                std::string domain_name = domain.first.as<std::string>();
                DomainGuideline guideline;
                
                if (domain.second["name"]) guideline.name = domain.second["name"].as<std::string>();
                if (domain.second["augmentation"]) guideline.augmentation = domain.second["augmentation"].as<std::string>();
                if (domain.second["additional_notes"]) guideline.additional_notes = domain.second["additional_notes"].as<std::string>();
                
                if (domain.second["applies_to"]) {
                    for (const auto& keyword : domain.second["applies_to"]) {
                        guideline.applies_to.push_back(keyword.as<std::string>());
                    }
                }
                
                domain_guidelines_[domain_name] = guideline;
            }
        }
        
        config_path_ = config_path;
        LogInfo("Loaded ethical guidelines from {}", config_path);
        LogInfo("  - {} core principles", principles_.size());
        LogInfo("  - {} German keywords, {} English keywords", 
                ethical_keywords_de_.size(), ethical_keywords_en_.size());
        LogInfo("  - {} augmentation templates", augmentation_templates_.size());
        LogInfo("  - {} domain-specific guidelines", domain_guidelines_.size());
        
        return true;
        
    } catch (const YAML::Exception& e) {
        LogError("YAML parsing error in {}: {}", config_path, e.what());
        return false;
    } catch (const std::exception& e) {
        LogError("Error loading ethical guidelines: {}", e.what());
        return false;
    }
}

bool EthicalGuidelinesManager::reloadConfig() {
    return loadConfig(config_path_);
}

void EthicalGuidelinesManager::setConfig(const Config& config) {
    std::lock_guard<std::mutex> lock(mutex_);
    config_ = config;
}

void EthicalGuidelinesManager::setEnabled(bool enabled) {
    std::lock_guard<std::mutex> lock(mutex_);
    config_.enabled = enabled;
}

EthicalGuidelinesManager::DetectionResult 
EthicalGuidelinesManager::detectEthicalContext(
    const std::string& text,
    const std::string& language) {
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!config_.enabled) {
        return DetectionResult{};
    }
    
    DetectionResult result;
    statistics_.total_detections++;
    
    // Detect language if not provided
    std::string lang = language.empty() ? detectLanguage(text) : language;
    
    // Convert text to lowercase for matching
    std::string text_lower = text;
    std::transform(text_lower.begin(), text_lower.end(), text_lower.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    
    // Check ethical keywords
    const auto& keywords = (lang == "de" || config_.language_mode == "both") 
                          ? ethical_keywords_de_ : ethical_keywords_en_;
    
    for (const auto& keyword : keywords) {
        std::string keyword_lower = keyword;
        std::transform(keyword_lower.begin(), keyword_lower.end(), keyword_lower.begin(),
                      [](unsigned char c) { return std::tolower(c); });
        
        if (text_lower.find(keyword_lower) != std::string::npos) {
            result.detected_keywords.push_back(keyword);
        }
    }
    
    // Also check English keywords if language mode is "both"
    if (config_.language_mode == "both" && lang == "de") {
        for (const auto& keyword : ethical_keywords_en_) {
            std::string keyword_lower = keyword;
            std::transform(keyword_lower.begin(), keyword_lower.end(), keyword_lower.begin(),
                          [](unsigned char c) { return std::tolower(c); });
            
            if (text_lower.find(keyword_lower) != std::string::npos) {
                result.detected_keywords.push_back(keyword);
            }
        }
    }
    
    // Detect domains
    result.detected_domains = detectDomains(text_lower);
    
    // Calculate confidence
    result.confidence = calculateConfidence(result.detected_keywords);
    result.has_ethical_context = result.confidence >= config_.detection_threshold;
    
    if (result.has_ethical_context) {
        statistics_.ethical_contexts_found++;
        result.recommended_augmentation = selectAugmentation(result);
        
        // Track domain statistics
        for (const auto& domain : result.detected_domains) {
            statistics_.domain_counts[domain]++;
        }
        
        if (config_.enable_logging) {
            logDetection(result, std::to_string(text.length()));
        }
    }
    
    return result;
}

EthicalGuidelinesManager::DetectionResult 
EthicalGuidelinesManager::detectEthicalContextInRAG(
    const std::vector<std::string>& documents,
    const std::string& query,
    const std::vector<std::string>& conversation_history) {
    
    if (!config_.enabled) {
        return DetectionResult{};
    }
    
    // First check the query itself
    DetectionResult result = detectEthicalContext(query);
    
    // Then check each document
    for (const auto& doc : documents) {
        auto doc_result = detectEthicalContext(doc);
        
        // Merge results
        result.detected_keywords.insert(
            result.detected_keywords.end(),
            doc_result.detected_keywords.begin(),
            doc_result.detected_keywords.end()
        );
        
        result.detected_domains.insert(
            result.detected_domains.end(),
            doc_result.detected_domains.begin(),
            doc_result.detected_domains.end()
        );
        
        // Update confidence (take maximum)
        result.confidence = std::max(result.confidence, doc_result.confidence);
    }
    
    // If conversation history provided and LLM judge enabled, analyze context
    if (!conversation_history.empty() && config_.use_llm_as_judge) {
        // Note: LLM judge would be called here if llm_wrapper is available
        // For now, we increase confidence if conversation history is present
        // as it provides additional context for ethical considerations
        if (!result.detected_keywords.empty()) {
            result.confidence = std::min(result.confidence * 1.2f, 1.0f);
        }
    }
    
    // Remove duplicates
    std::sort(result.detected_keywords.begin(), result.detected_keywords.end());
    result.detected_keywords.erase(
        std::unique(result.detected_keywords.begin(), result.detected_keywords.end()),
        result.detected_keywords.end()
    );
    
    std::sort(result.detected_domains.begin(), result.detected_domains.end());
    result.detected_domains.erase(
        std::unique(result.detected_domains.begin(), result.detected_domains.end()),
        result.detected_domains.end()
    );
    
    result.has_ethical_context = result.confidence >= config_.detection_threshold;
    if (result.has_ethical_context) {
        result.recommended_augmentation = selectAugmentation(result);
    }
    
    return result;
}

std::string EthicalGuidelinesManager::augmentPrompt(
    const std::string& original_prompt,
    const DetectionResult& detection_result) {
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!config_.enabled) {
        return original_prompt;
    }
    
    // Always apply default if configured, or if ethical context detected
    bool should_augment = config_.always_apply_default || detection_result.has_ethical_context;
    
    if (!should_augment) {
        return original_prompt;
    }
    
    statistics_.prompts_augmented++;
    
    // Select augmentation template
    std::string template_name = detection_result.has_ethical_context 
                               ? detection_result.recommended_augmentation 
                               : "default";
    
    const auto* tmpl = getAugmentationTemplate(template_name);
    if (!tmpl) {
        LogWarning("Augmentation template '{}' not found, using default", template_name);
        tmpl = getAugmentationTemplate("default");
    }
    
    if (!tmpl) {
        LogError("Default augmentation template not found!");
        return original_prompt;
    }
    
    // Construct augmented prompt
    std::stringstream ss;
    ss << tmpl->system_prefix << "\n\n";
    ss << original_prompt;
    
    return ss.str();
}

std::string EthicalGuidelinesManager::augmentResponse(
    const std::string& response,
    const DetectionResult& detection_result) {
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!config_.enabled || !config_.show_disclaimers) {
        return response;
    }
    
    if (!detection_result.has_ethical_context) {
        return response;
    }
    
    // Select augmentation template
    std::string template_name = detection_result.recommended_augmentation;
    const auto* tmpl = getAugmentationTemplate(template_name);
    
    if (!tmpl || tmpl->response_suffix.empty()) {
        return response;
    }
    
    // Append disclaimer
    return response + tmpl->response_suffix;
}

const EthicalGuidelinesManager::AugmentationTemplate* 
EthicalGuidelinesManager::getAugmentationTemplate(const std::string& name) const {
    auto it = augmentation_templates_.find(name);
    return (it != augmentation_templates_.end()) ? &it->second : nullptr;
}

// Private helper methods

float EthicalGuidelinesManager::calculateConfidence(
    const std::vector<std::string>& detected_keywords) const {
    
    if (detected_keywords.empty()) {
        return 0.0f;
    }
    
    // Simple confidence calculation based on number of keywords
    // More sophisticated approaches could use keyword weights, context, etc.
    float base_confidence = 0.3f;
    float per_keyword = 0.15f;
    
    float confidence = base_confidence + (detected_keywords.size() * per_keyword);
    return std::min(confidence, 1.0f);
}

std::string EthicalGuidelinesManager::detectLanguage(const std::string& text) const {
    // Simple language detection based on common words/characters
    // German indicators: ä, ö, ü, ß, common German words
    // English indicators: common English words
    
    int de_score = 0;
    int en_score = 0;
    
    // Check for German umlauts
    if (text.find("ä") != std::string::npos || 
        text.find("ö") != std::string::npos ||
        text.find("ü") != std::string::npos ||
        text.find("ß") != std::string::npos) {
        de_score += 2;
    }
    
    // Check for common German words
    std::vector<std::string> de_words = {"und", "der", "die", "das", "ist", "mit", "für"};
    for (const auto& word : de_words) {
        if (text.find(word) != std::string::npos) de_score++;
    }
    
    // Check for common English words
    std::vector<std::string> en_words = {"the", "and", "is", "are", "with", "for", "this"};
    for (const auto& word : en_words) {
        if (text.find(word) != std::string::npos) en_score++;
    }
    
    return (de_score > en_score) ? "de" : "en";
}

std::vector<std::string> EthicalGuidelinesManager::detectDomains(const std::string& text_lower) const {
    std::vector<std::string> detected_domains;
    
    for (const auto& [domain_name, guideline] : domain_guidelines_) {
        for (const auto& keyword : guideline.applies_to) {
            std::string keyword_lower = keyword;
            std::transform(keyword_lower.begin(), keyword_lower.end(), keyword_lower.begin(),
                          [](unsigned char c) { return std::tolower(c); });
            
            if (text_lower.find(keyword_lower) != std::string::npos) {
                detected_domains.push_back(domain_name);
                break;  // Domain detected, no need to check more keywords
            }
        }
    }
    
    return detected_domains;
}

std::string EthicalGuidelinesManager::selectAugmentation(const DetectionResult& result) const {
    // Priority order:
    // 1. Moral imperatives (if specific keywords detected)
    // 2. Domain-specific augmentation
    // 3. High autonomy (if confidence is very high)
    // 4. Default
    
    // Check for moral imperative keywords
    for (const auto& keyword : result.detected_keywords) {
        std::string kw_lower = keyword;
        std::transform(kw_lower.begin(), kw_lower.end(), kw_lower.begin(),
                      [](unsigned char c) { return std::tolower(c); });
        
        if (kw_lower.find("imperativ") != std::string::npos ||
            kw_lower.find("pflicht") != std::string::npos ||
            kw_lower.find("duty") != std::string::npos ||
            kw_lower.find("obligation") != std::string::npos) {
            return "moral_imperatives";
        }
    }
    
    // Check domain-specific augmentation
    for (const auto& domain : result.detected_domains) {
        auto it = domain_guidelines_.find(domain);
        if (it != domain_guidelines_.end()) {
            return it->second.augmentation;
        }
    }
    
    // High confidence suggests high-stakes decision
    if (result.confidence >= 0.8f) {
        return "high_autonomy";
    }
    
    return "default";
}

void EthicalGuidelinesManager::logDetection(const DetectionResult& result, const std::string& context) const {
    LogInfo("Ethical context detected:");
    LogInfo("  Confidence: {:.2f}", result.confidence);
    LogInfo("  Keywords: {}", result.detected_keywords.size());
    LogInfo("  Domains: {}", result.detected_domains.size());
    LogInfo("  Augmentation: {}", result.recommended_augmentation);
    LogInfo("  Context length: {}", context);
}

EthicalGuidelinesManager::Statistics EthicalGuidelinesManager::getStatistics() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return statistics_;
}

void EthicalGuidelinesManager::resetStatistics() {
    std::lock_guard<std::mutex> lock(mutex_);
    statistics_ = Statistics{};
}

EthicalGuidelinesManager::DetectionResult 
EthicalGuidelinesManager::detectWithLLMJudge(
    const std::string& text,
    const std::vector<std::string>& conversation_context,
    void* llm_wrapper_ptr) {
    
    DetectionResult result;
    result.used_llm_judge = true;
    
    if (!llm_wrapper_ptr) {
        LogWarning("LLM judge requested but no LLM wrapper provided");
        return result;
    }
    
    // Build context from conversation history
    std::stringstream context_builder;
    context_builder << "Conversation History:\n";
    for (size_t i = 0; i < conversation_context.size(); i++) {
        context_builder << (i + 1) << ". " << conversation_context[i] << "\n";
    }
    context_builder << "\nCurrent Text: " << text << "\n";
    
    // LLM-as-ethical-judge prompt
    std::string judge_prompt = R"(
You are an ethical analysis expert. Your task is to analyze text and conversation context to identify ethical and moral implications that may not be immediately obvious.

Consider:
1. **Implicit moral questions** - Is someone facing a decision with ethical implications?
2. **Power dynamics** - Are there issues of autonomy, coercion, or vulnerable parties?
3. **Harm potential** - Could actions discussed cause harm (physical, psychological, social)?
4. **Rights conflicts** - Are human rights, duties, or moral obligations in tension?
5. **Cultural/religious sensitivity** - Are there diverse moral perspectives to consider?

)";
    
    judge_prompt += context_builder.str();
    judge_prompt += R"(

Analyze the above text and context. Respond in JSON format:
{
  "has_ethical_implications": true/false,
  "confidence": 0.0-1.0,
  "reasoning": "Brief explanation of why this has ethical implications",
  "implicit_questions": ["list of implicit moral questions"],
  "recommended_approach": "default/high_autonomy/administrative/moral_imperatives"
}
)";
    
    // Call the LLM wrapper to run the ethical judge analysis.
    auto* llm = static_cast<ILLMPlugin*>(llm_wrapper_ptr);

    InferenceRequest req;
    req.prompt = judge_prompt;
    req.max_tokens = 512;
    req.temperature = 0.1f;  // Low temperature for deterministic ethical analysis

    InferenceResponse resp;
    try {
        resp = llm->generate(req);
    } catch (const std::exception& ex) {
        LogWarning("LLM judge inference failed: {}", ex.what());
        return result;
    }

    if (!resp.success || resp.text.empty()) {
        LogWarning("LLM judge returned empty or failed response: {}",
                   resp.error_message);
        return result;
    }

    // Parse JSON response from model output — the prompt asked for JSON only.
    // Strip any markdown code fences the model may have emitted.
    std::string json_text = resp.text;
    {
        static const std::regex kFence("```(?:json)?\\s*([\\s\\S]*?)```",
                                       std::regex_constants::icase);
        std::smatch m;
        if (std::regex_search(json_text, m, kFence)) {
            json_text = m[1].str();
        }
        // Trim leading/trailing whitespace
        auto ltrim = json_text.find('{');
        auto rtrim = json_text.rfind('}');
        if (ltrim != std::string::npos && rtrim != std::string::npos) {
            json_text = json_text.substr(ltrim, rtrim - ltrim + 1);
        }
    }

    try {
        auto j = nlohmann::json::parse(json_text);
        result.has_ethical_context =
            j.value("has_ethical_implications", false);
        result.llm_confidence = j.value("confidence", 0.0f);
        result.llm_reasoning  = j.value("reasoning", std::string{});
    } catch (const nlohmann::json::exception& je) {
        LogWarning("LLM judge JSON parse error: {} — raw output: {}",
                   je.what(), resp.text.substr(0, 200));
        // Treat as no ethical implications if we can't parse the response
    }

    return result;
}

// ═══════════════════════════════════════════════════════════
// Plugin Integration API Implementation
// ═══════════════════════════════════════════════════════════

bool EthicalGuidelinesManager::registerPhilosophy(
    const std::string& school_id,
    const themis::plugins::ethics::PhilosophyProfile& profile) {
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (school_id.empty()) {
        LogWarning("Cannot register philosophy with empty school_id");
        return false;
    }
    
    // Validate profile has minimum required fields
    if (profile.name.empty()) {
        LogWarning("Cannot register philosophy '{}' with empty name", school_id);
        return false;
    }
    
    // Check if already registered
    if (philosophy_profiles_.find(school_id) != philosophy_profiles_.end()) {
        LogInfo("Philosophy '{}' already registered, updating", school_id);
    }
    
    // Store the profile
    philosophy_profiles_[school_id] = profile;
    
    LogInfo("Registered philosophy profile: {} ({})", school_id, profile.name);
    return true;
}

size_t EthicalGuidelinesManager::mergePhilosophies(
    const std::map<std::string, themis::plugins::ethics::PhilosophyProfile>& profiles) {
    
    size_t registered_count = 0;
    
    for (const auto& [school_id, profile] : profiles) {
        if (registerPhilosophy(school_id, profile)) {
            registered_count++;
        }
    }
    
    LogInfo("Merged {} philosophy profiles into EthicalGuidelinesManager", registered_count);
    return registered_count;
}

std::vector<std::string> EthicalGuidelinesManager::getRegisteredPhilosophies() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<std::string> schools;
    schools.reserve(philosophy_profiles_.size());
    
    for (const auto& [school_id, profile] : philosophy_profiles_) {
        schools.push_back(school_id);
    }
    
    return schools;
}

} // namespace llm
} // namespace themis

