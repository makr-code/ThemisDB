/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            multi_perspective_generator.h                      ║
  Version:         0.0.39                                             ║
  Last Modified:   2026-04-13 20:23:15                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     403                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file multi_perspective_generator.h
 * @brief Multi-perspective generation for ethical queries
 * 
 * For ethical and moral queries, generates multiple viewpoints to ensure
 * balanced presentation of diverse ethical frameworks. Prevents single-
 * perspective bias and respects moral diversity.
 * 
 * Scientific foundation:
 * - Wang et al. (2023): Self-consistency - Multi-perspective reasoning
 * - UN Human Rights Art. 18, 19: Freedom of thought, opinion
 * - Ethical pluralism: Recognition of multiple valid moral frameworks
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>
#include <chrono>

namespace themis {
namespace llm {

// Forward declarations
class EthicalGuidelinesManager;

/**
 * @brief Ethical perspective/framework
 */
struct EthicalPerspective {
    std::string id;
    std::string name;
    std::string description;
    std::string tradition;         ///< e.g., "Utilitarian", "Deontological", "Virtue Ethics"
    std::vector<std::string> key_principles;
    std::string prompt_template;   ///< Template for generating perspective
};

/**
 * @brief Single perspective response
 */
struct PerspectiveResponse {
    EthicalPerspective perspective;
    std::string response;
    float confidence;
    std::vector<std::string> key_points;
    std::string reasoning;
};

/**
 * @brief Multi-perspective generation result
 */
struct MultiPerspectiveResult {
    // Original query
    std::string query;
    
    // Individual perspectives
    std::vector<PerspectiveResponse> perspectives;
    
    // Synthesized response
    std::string synthesized_response;
    std::string synthesis_reasoning;
    
    // Diversity metrics
    int unique_perspectives_count;
    float perspective_diversity_score;  ///< 0-1, higher = more diverse
    bool shows_balanced_view;
    
    // Quality metrics
    bool meets_diversity_requirement;
    std::vector<std::string> common_themes;
    std::vector<std::string> disagreements;
    
    // Metadata
    std::chrono::milliseconds generation_time;
};

/**
 * @brief Configuration for multi-perspective generation
 */
struct MultiPerspectiveConfig {
    // Perspective selection
    int min_perspectives = 2;              ///< Minimum perspectives to generate
    int max_perspectives = 4;              ///< Maximum perspectives to generate
    bool auto_select_perspectives = true;  ///< Auto-select based on query
    std::vector<std::string> required_perspectives; ///< Must include these
    
    // Diversity requirements
    float min_diversity_score = 0.6f;      ///< Minimum diversity score
    bool require_contrasting_views = true;  ///< Must include opposing views
    
    // Synthesis settings
    bool enable_synthesis = true;           ///< Generate synthesized response
    bool preserve_all_perspectives = true;  ///< Include all perspectives in synthesis
    bool highlight_disagreements = true;    ///< Explicitly note disagreements
    
    // Integration with existing systems
    bool use_ethical_guidelines_manager = true; ///< Integrate with EthicalGuidelinesManager
    
    // Performance
    bool cache_perspectives = true;
    size_t max_cache_size = 500;
    bool async_generation = false;
};

/**
 * @brief Multi-perspective generator
 * 
 * Generates multiple ethical/moral perspectives for queries to ensure
 * balanced and diverse presentation. Prevents bias towards single moral
 * framework and respects moral pluralism.
 */
class MultiPerspectiveGenerator {
public:
    /**
     * @brief Constructor
     * @param config Configuration
     * @param guidelines_manager Optional ethical guidelines manager
     */
    explicit MultiPerspectiveGenerator(
        const MultiPerspectiveConfig& config = {},
        EthicalGuidelinesManager* guidelines_manager = nullptr
    );
    
    /**
     * @brief Destructor
     */
    ~MultiPerspectiveGenerator();
    
    // ═══════════════════════════════════════════════════════════
    // Core functionality
    // ═══════════════════════════════════════════════════════════
    
    /**
     * @brief Generate multiple perspectives for query
     * @param query User query
     * @param llm_wrapper LLM wrapper for generation
     * @param context Optional conversation context
     * @return Multi-perspective result
     */
    MultiPerspectiveResult generatePerspectives(
        const std::string& query,
        void* llm_wrapper,  // LlamaWrapper* - forward declared
        const std::vector<std::string>& context = {}
    );
    
    /**
     * @brief Generate single perspective response
     * @param query User query
     * @param perspective Perspective to apply
     * @param llm_wrapper LLM wrapper for generation
     * @return Perspective response
     */
    PerspectiveResponse generateSinglePerspective(
        const std::string& query,
        const EthicalPerspective& perspective,
        void* llm_wrapper
    );
    
    /**
     * @brief Synthesize multiple perspectives into balanced response
     * @param perspectives Vector of perspective responses
     * @param query Original query
     * @return Synthesized response
     */
    std::string synthesizePerspectives(
        const std::vector<PerspectiveResponse>& perspectives,
        const std::string& query
    );
    
    /**
     * @brief Detect if query requires multi-perspective analysis
     * @param query User query
     * @return true if multi-perspective analysis recommended
     */
    bool requiresMultiPerspective(const std::string& query);
    
    /**
     * @brief Select appropriate perspectives for query
     * @param query User query
     * @return Vector of perspectives to generate
     */
    std::vector<EthicalPerspective> selectPerspectives(const std::string& query);
    
    // ═══════════════════════════════════════════════════════════
    // Perspective management
    // ═══════════════════════════════════════════════════════════
    
    /**
     * @brief Add ethical perspective
     * @param perspective Perspective to add
     */
    void addPerspective(const EthicalPerspective& perspective);
    
    /**
     * @brief Remove perspective by ID
     * @param perspective_id Perspective ID to remove
     */
    void removePerspective(const std::string& perspective_id);
    
    /**
     * @brief Get all available perspectives
     * @return Vector of all perspectives
     */
    std::vector<EthicalPerspective> getAvailablePerspectives() const;
    
    /**
     * @brief Load default ethical perspectives
     * 
     * Loads standard ethical frameworks:
     * - Utilitarian (consequentialist)
     * - Deontological (duty-based)
     * - Virtue ethics (character-based)
     * - Care ethics (relationship-based)
     * - Rights-based ethics
     * - Justice-based ethics
     */
    void loadDefaultPerspectives();
    
    // ═══════════════════════════════════════════════════════════
    // Diversity analysis
    // ═══════════════════════════════════════════════════════════
    
    /**
     * @brief Calculate diversity score for perspectives
     * @param perspectives Vector of perspective responses
     * @return Diversity score (0-1)
     */
    float calculateDiversityScore(
        const std::vector<PerspectiveResponse>& perspectives
    );
    
    /**
     * @brief Find common themes across perspectives
     * @param perspectives Vector of perspective responses
     * @return Vector of common themes
     */
    std::vector<std::string> findCommonThemes(
        const std::vector<PerspectiveResponse>& perspectives
    );
    
    /**
     * @brief Find disagreements between perspectives
     * @param perspectives Vector of perspective responses
     * @return Vector of disagreement areas
     */
    std::vector<std::string> findDisagreements(
        const std::vector<PerspectiveResponse>& perspectives
    );
    
    // ═══════════════════════════════════════════════════════════
    // Configuration
    // ═══════════════════════════════════════════════════════════
    
    /**
     * @brief Update configuration
     * @param config New configuration
     */
    void setConfig(const MultiPerspectiveConfig& config);
    
    /**
     * @brief Get current configuration
     * @return Current configuration
     */
    MultiPerspectiveConfig getConfig() const;
    
    /**
     * @brief Set ethical guidelines manager
     * @param manager Guidelines manager to use
     */
    void setEthicalGuidelinesManager(EthicalGuidelinesManager* manager);
    
    /**
     * @brief Clear perspective cache
     */
    void clearCache();
    
    // ═══════════════════════════════════════════════════════════
    // Statistics
    // ═══════════════════════════════════════════════════════════
    
    /**
     * @brief Statistics for monitoring
     */
    struct Statistics {
        uint64_t total_generations = 0;
        uint64_t multi_perspective_generated = 0;
        uint64_t cache_hits = 0;
        uint64_t cache_misses = 0;
        
        // Perspective usage
        std::unordered_map<std::string, uint64_t> perspective_usage;
        
        // Diversity metrics
        float avg_diversity_score = 0.0f;
        float avg_perspectives_per_query = 0.0f;
        
        // Timing
        std::chrono::milliseconds avg_generation_time{0};
    };
    
    /**
     * @brief Get statistics
     * @return Current statistics
     */
    Statistics getStatistics() const;
    
    /**
     * @brief Reset statistics
     */
    void resetStatistics();
    
    /**
     * @brief Set callback for generation completion
     * @param callback Function to call after each generation
     */
    void setGenerationCallback(
        std::function<void(const MultiPerspectiveResult&)> callback
    );

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    
    // Helper methods
    std::string buildPerspectivePrompt(
        const std::string& query,
        const EthicalPerspective& perspective
    );
    
    std::string buildSynthesisPrompt(
        const std::vector<PerspectiveResponse>& perspectives,
        const std::string& query
    );
    
    bool detectEthicalQuery(const std::string& query);
    
    std::vector<std::string> extractKeyPoints(
        const std::string& response,
        const EthicalPerspective& perspective
    );
    
    void updateStatistics(const MultiPerspectiveResult& result);
};

/**
 * @brief Factory for creating multi-perspective generators
 */
class MultiPerspectiveGeneratorFactory {
public:
    /**
     * @brief Create generator with default configuration
     */
    static std::unique_ptr<MultiPerspectiveGenerator> createDefault();
    
    /**
     * @brief Create generator requiring high diversity
     */
    static std::unique_ptr<MultiPerspectiveGenerator> createHighDiversity();
    
    /**
     * @brief Create generator with specific perspectives
     * @param required_perspectives Vector of perspective IDs to require
     */
    static std::unique_ptr<MultiPerspectiveGenerator> createWithPerspectives(
        const std::vector<std::string>& required_perspectives
    );
    
    /**
     * @brief Create generator with custom configuration
     */
    static std::unique_ptr<MultiPerspectiveGenerator> create(
        const MultiPerspectiveConfig& config
    );
};

} // namespace llm
} // namespace themis
