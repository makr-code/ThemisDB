#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <mutex>

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
        int priority;
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
     * @return Aggregated detection result
     */
    DetectionResult detectEthicalContextInRAG(
        const std::vector<std::string>& documents,
        const std::string& query
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
