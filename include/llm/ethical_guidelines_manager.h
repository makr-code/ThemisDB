/**
 * @file ethical_guidelines_manager.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <map>
#include <memory>
#include <mutex>
#include "ethics_ai/ethics_ai_types.h"

namespace themis {
namespace llm {

class EthicalGuidelinesManager {
public:
    struct Principle {
        std::string id;
        std::string name;
        std::string description;
        std::string description_en;
        int priority = 0;
    };
    
    struct AugmentationTemplate {
        std::string system_prefix;
        std::string response_suffix;
    };
    
    struct DomainGuideline {
        std::string name;
        std::vector<std::string> applies_to;
        std::string augmentation;
        std::string additional_notes;
    };
    
    struct DetectionResult {
        bool has_ethical_context = false;
        std::vector<std::string> detected_keywords;
        std::vector<std::string> detected_domains;
        float confidence = 0.0f;
        std::string recommended_augmentation;
        
        // LLM-as-judge results
        bool used_llm_judge = false;
        std::string llm_reasoning = "";      // Why LLM detected ethical context
        float llm_confidence = 0.0f;         // LLM's confidence score
    };
    
    struct Config {
        bool enabled = true;
        float detection_threshold = 0.6f;
        bool enable_logging = true;
        bool always_apply_default = true;
        bool show_disclaimers = true;
        std::string language_mode = "both";  // de, en, both
        
        // LLM-as-ethical-judge configuration
        bool use_llm_as_judge = false;        // Use LLM for context-aware detection
        float llm_judge_threshold = 0.7f;     // Confidence threshold for LLM judge
        bool combine_with_keywords = true;    // Combine LLM judge with keyword matching
    };
    
    explicit EthicalGuidelinesManager(const std::string& config_path = "config/ethical_guidelines.yaml");
    
    ~EthicalGuidelinesManager() = default;
    
    // ═══════════════════════════════════════════════════════════
    // Core functionality
    // ═══════════════════════════════════════════════════════════
    
    DetectionResult detectEthicalContext(
        const std::string& text,
        const std::string& language = ""
    );
    
    DetectionResult detectEthicalContextInRAG(
        const std::vector<std::string>& documents,
        const std::string& query,
        const std::vector<std::string>& conversation_history = {}
    );
    
    /**
     * @brief Detect With LLMJudge.
     * @param[in] text Input parameter.
     * @param[in] conversation_context Input parameter.
     * @param[in,out] llm_wrapper Input/output parameter.
     * @return Return value.
     */
    DetectionResult detectWithLLMJudge(
        const std::string& text,
        const std::vector<std::string>& conversation_context,
        void* llm_wrapper  // LlamaWrapper* - forward declared to avoid circular dependency
    );
    
    /**
     * @brief Augment Prompt.
     * @param[in] original_prompt Input parameter.
     * @param[in] detection_result Input parameter.
     * @return Return value.
     */
    std::string augmentPrompt(
        const std::string& original_prompt,
        const DetectionResult& detection_result
    );
    
    /**
     * @brief Augment Response.
     * @param[in] response Input parameter.
     * @param[in] detection_result Input parameter.
     * @return Return value.
     */
    std::string augmentResponse(
        const std::string& response,
        const DetectionResult& detection_result
    );
    
    /**
     * @brief Get Augmentation Template.
     * @param[in] name Input parameter.
     * @return Pointer to the result.
     */
    const AugmentationTemplate* getAugmentationTemplate(const std::string& name) const;
    
    /**
     * @brief ═══════════════════════════════════════════════════════════ Configuration management ═══════════════════════════════════════════════════════════
     * @param[in] config_path Path to the retention policy configuration file.
     * @return True when the operation succeeds.
     */
    
    bool loadConfig(const std::string& config_path);
    
    /**
     * @brief Reload Config.
     * @return True when the operation succeeds.
     */
    bool reloadConfig();
    
    const Config& getConfig() const { return config_; }
    
    /**
     * @brief Set Config.
     * @param[in] config Input parameter.
     */
    void setConfig(const Config& config);
    
    bool isEnabled() const { return config_.enabled; }
    
    /**
     * @brief Set Enabled.
     * @param[in] enabled Input parameter.
     */
    void setEnabled(bool enabled);
    
    // ═══════════════════════════════════════════════════════════
    // Introspection
    // ═══════════════════════════════════════════════════════════
    
    const std::vector<Principle>& getPrinciples() const { return principles_; }
    
    const std::unordered_map<std::string, DomainGuideline>& getDomainGuidelines() const {
        return domain_guidelines_;
    }
    
    struct Statistics {
        uint64_t total_detections = 0;
        uint64_t ethical_contexts_found = 0;
        uint64_t prompts_augmented = 0;
        std::unordered_map<std::string, uint64_t> domain_counts;
    };
    
    /**
     * @brief Return access control statistics.
     * @return Access control statistics.
     */
    Statistics getStatistics() const;
    /**
     * @brief Reset Statistics.
     */
    void resetStatistics();
    
    /**
     * @brief ═══════════════════════════════════════════════════════════ Plugin Integration API ═══════════════════════════════════════════════════════════
     * @param[in] school_id Identifier of the school.
     * @param[in] profile Input parameter.
     * @return True when the operation succeeds.
     */
    
    bool registerPhilosophy(
        const std::string& school_id,
        const themis::plugins::ethics::PhilosophyProfile& profile
    );
    
    size_t mergePhilosophies(
        const std::map<std::string, themis::plugins::ethics::PhilosophyProfile>& profiles
    );
    
    /**
     * @brief Get Registered Philosophies.
     * @return Return value.
     */
    std::vector<std::string> getRegisteredPhilosophies() const;
    
private:
    // Configuration
    Config config_;
    std::string config_path_;
    
    // Core principles
    std::vector<Principle> principles_;
    
    // Context detection keywords
    std::vector<std::string> ethical_keywords_de_;
    std::vector<std::string> ethical_keywords_en_;
    std::vector<std::string> high_autonomy_contexts_;
    
    // Augmentation templates
    std::unordered_map<std::string, AugmentationTemplate> augmentation_templates_;
    
    // Domain-specific guidelines
    std::unordered_map<std::string, DomainGuideline> domain_guidelines_;
    
    // Plugin-registered philosophy profiles
    std::map<std::string, themis::plugins::ethics::PhilosophyProfile> philosophy_profiles_;
    
    // Statistics
    mutable Statistics statistics_;
    
    // Thread safety
    mutable std::mutex mutex_;
    
    // Helper methods
    /**
     * @brief Load From YAML.
     * @param[in] yaml_content Input parameter.
     * @return True when the operation succeeds.
     */
    bool loadFromYAML(const std::string& yaml_content);
    /**
     * @brief Calculate Confidence.
     * @param[in] detected_keywords Input parameter.
     * @return Return value.
     */
    float calculateConfidence(const std::vector<std::string>& detected_keywords) const;
    /**
     * @brief Detect Language.
     * @param[in] text Input parameter.
     * @return Return value.
     */
    std::string detectLanguage(const std::string& text) const;
    /**
     * @brief Detect Domains.
     * @param[in] text Input parameter.
     * @return Return value.
     */
    std::vector<std::string> detectDomains(const std::string& text) const;
    /**
     * @brief Select Augmentation.
     * @param[in] result Input parameter.
     * @return Return value.
     */
    std::string selectAugmentation(const DetectionResult& result) const;
    /**
     * @brief Log Detection.
     * @param[in] result Input parameter.
     * @param[in] context Input parameter.
     */
    void logDetection(const DetectionResult& result, const std::string& context) const;
};

} // namespace llm
} // namespace themis
