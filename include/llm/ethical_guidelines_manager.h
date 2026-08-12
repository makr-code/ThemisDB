/**
 * @file ethical_guidelines_manager.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

// Forward declare PhilosophyProfile from ethics AI plugin
namespace themis {
namespace plugins {
namespace ethics {
    struct PhilosophyProfile;
    struct Status;
}
}
}

namespace themis {
namespace llm {

/**
 * @brief Manages ethical and moral guidelines for LLM responses
 * 
 * This class loads ethical guidelines from YAML configuration and applies
 * them during RAG retrieval and LLM inference to ensure the AI never
 * patronizes humans and respects human autonomy.
 * 
 * Key features:
 * - Detects ethical/moral contexts in queries and retrieved documents
 * - Augments LLM prompts with appropriate ethical guidelines
 * - Supports multiple languages (German and English)
 * - Domain-specific guidelines (medical, legal, administrative, etc.)
 * - Configurable detection thresholds and logging
 * 
 * Thread-safe for concurrent use.
 */
class EthicalGuidelinesManager {
public:
    /**
     * @brief Core ethical principle
     */
    struct Principle {
        std::string id;
        std::string name;
        std::string description;
        std::string description_en;
        int priority = 0;
    };
    
    /**
     * @brief Augmentation template for prompts
     */
    struct AugmentationTemplate {
        std::string system_prefix;
        std::string response_suffix;
    };
    
    /**
     * @brief Domain-specific guideline configuration
     */
    struct DomainGuideline {
        std::string name;
        std::vector<std::string> applies_to;
        std::string augmentation;
        std::string additional_notes;
    };
    
    /**
     * @brief Context detection result
     */
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
    
    /**
     * @brief Configuration
     */
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
    
    /**
     * @brief Constructor
     * @param config_path Path to ethical_guidelines.yaml file
     */
    explicit EthicalGuidelinesManager(const std::string& config_path = "config/ethical_guidelines.yaml");
    
    /**
     * @brief Destructor
     */
    ~EthicalGuidelinesManager();
    
    // ═══════════════════════════════════════════════════════════
    // Core functionality
    // ═══════════════════════════════════════════════════════════
    
    /**
     * @brief Detect ethical/moral context in text
     * @param text Input text (query or document)
     * @param language Language hint ("de", "en", or "" for auto-detect)
     * @return Detection result with confidence and recommendations
     */
    DetectionResult detectEthicalContext(
        const std::string& text,
        const std::string& language = ""
    );
    
    /**
     * @brief Detect ethical context in multiple documents (RAG retrieval)
     * @param documents Vector of retrieved document texts
     * @param query Original user query
     * @param conversation_history Optional conversation history for context
     * @return Aggregated detection result
     */
    DetectionResult detectEthicalContextInRAG(
        const std::vector<std::string>& documents,
        const std::string& query,
        const std::vector<std::string>& conversation_history = {}
    );
    
    /**
     * @brief Use LLM as ethical judge to detect context-aware implications
     * @param text Text to analyze
     * @param conversation_context Optional conversation history
     * @param llm_wrapper Pointer to LLM wrapper for inference
     * @return Detection result with LLM reasoning
     * 
     * This implements "LLM-as-ethical-judge" pattern similar to "LLM-as-judge".
     * The LLM analyzes the text and conversation context to identify
     * ethical/moral implications that may not be obvious from keywords alone.
     */
    DetectionResult detectWithLLMJudge(
        const std::string& text,
        const std::vector<std::string>& conversation_context,
        void* llm_wrapper  // LlamaWrapper* - forward declared to avoid circular dependency
    );
    
    /**
     * @brief Augment LLM prompt with ethical guidelines
     * @param original_prompt Original system prompt
     * @param detection_result Detection result from detectEthicalContext()
     * @return Augmented prompt with ethical guidelines prepended
     */
    std::string augmentPrompt(
        const std::string& original_prompt,
        const DetectionResult& detection_result
    );
    
    /**
     * @brief Augment LLM response with ethical disclaimer
     * @param response Original LLM response
     * @param detection_result Detection result
     * @return Response with disclaimer appended (if applicable)
     */
    std::string augmentResponse(
        const std::string& response,
        const DetectionResult& detection_result
    );
    
    /**
     * @brief Get augmentation template by name
     * @param name Template name (default, high_autonomy, administrative, etc.)
     * @return Template or nullptr if not found
     */
    const AugmentationTemplate* getAugmentationTemplate(const std::string& name) const;
    
    // ═══════════════════════════════════════════════════════════
    // Configuration management
    // ═══════════════════════════════════════════════════════════
    
    /**
     * @brief Load configuration from file
     * @param config_path Path to YAML file
     * @return true if successful, false otherwise
     */
    bool loadConfig(const std::string& config_path);
    
    /**
     * @brief Reload configuration (hot reload)
     * @return true if successful, false otherwise
     */
    bool reloadConfig();
    
    /**
     * @brief Get current configuration
     */
    const Config& getConfig() const { return config_; }
    
    /**
     * @brief Update configuration
     */
    void setConfig(const Config& config);
    
    /**
     * @brief Check if system is enabled
     */
    bool isEnabled() const { return config_.enabled; }
    
    /**
     * @brief Enable/disable system
     */
    void setEnabled(bool enabled);
    
    // ═══════════════════════════════════════════════════════════
    // Introspection
    // ═══════════════════════════════════════════════════════════
    
    /**
     * @brief Get all core principles
     */
    const std::vector<Principle>& getPrinciples() const { return principles_; }
    
    /**
     * @brief Get all domain guidelines
     */
    const std::unordered_map<std::string, DomainGuideline>& getDomainGuidelines() const {
        return domain_guidelines_;
    }
    
    /**
     * @brief Get statistics (for monitoring)
     */
    struct Statistics {
        uint64_t total_detections = 0;
        uint64_t ethical_contexts_found = 0;
        uint64_t prompts_augmented = 0;
        std::unordered_map<std::string, uint64_t> domain_counts;
    };
    
    Statistics getStatistics() const;
    void resetStatistics();
    
    // ═══════════════════════════════════════════════════════════
    // Plugin Integration API
    // ═══════════════════════════════════════════════════════════
    
    /**
     * @brief Register a philosophy profile from external source (e.g., plugin)
     * @param school_id Unique identifier for the philosophy
     * @param profile Philosophy profile structure
     * @return Status indicating success/failure
     * 
     * This method allows plugins to extend the base ethical guidelines system
     * by registering additional philosophy profiles. The manager will validate
     * the profile and make it available for ethical context detection.
     * 
     * Thread-safe.
     */
    bool registerPhilosophy(
        const std::string& school_id,
        const themis::plugins::ethics::PhilosophyProfile& profile
    );
    
    /**
     * @brief Merge multiple philosophy profiles from plugin
     * @param profiles Map of school_id -> PhilosophyProfile
     * @return Number of profiles successfully registered
     * 
     * Convenience method for bulk registration of philosophy profiles.
     * Typically called by plugins during initialization to register all
     * their philosophy profiles at once.
     * 
     * Thread-safe.
     */
    size_t mergePhilosophies(
        const std::map<std::string, themis::plugins::ethics::PhilosophyProfile>& profiles
    );
    
    /**
     * @brief Get all registered philosophy schools
     * @return Vector of school IDs (base + plugin-registered)
     * 
     * Returns the complete list of philosophy schools available to the
     * ethical guidelines system, including both base philosophies loaded
     * from YAML and those registered by plugins.
     * 
     * Thread-safe.
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
    bool loadFromYAML(const std::string& yaml_content);
    float calculateConfidence(const std::vector<std::string>& detected_keywords) const;
    std::string detectLanguage(const std::string& text) const;
    std::vector<std::string> detectDomains(const std::string& text) const;
    std::string selectAugmentation(const DetectionResult& result) const;
    void logDetection(const DetectionResult& result, const std::string& context) const;
};

} // namespace llm
} // namespace themis
