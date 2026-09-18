/**
 * @file multi_perspective_generator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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

struct EthicalPerspective {
    std::string id;
    std::string name;
    std::string description;
    std::string tradition;         ///< e.g., "Utilitarian", "Deontological", "Virtue Ethics"
    std::vector<std::string> key_principles;
    std::string prompt_template;   ///< Template for generating perspective
};

struct PerspectiveResponse {
    EthicalPerspective perspective;
    std::string response;
    float confidence = 0.0f;
    std::vector<std::string> key_points;
    std::string reasoning;
};

struct MultiPerspectiveResult {
    /**
     * @brief Multi Perspective Result.
     * @return Return value.
     */
    virtual ~MultiPerspectiveResult() = default;
    // Original query
    std::string query;
    
    // Individual perspectives
    std::vector<PerspectiveResponse> perspectives;
    
    // Synthesized response
    std::string synthesized_response;
    std::string synthesis_reasoning;
    
    // Diversity metrics
    int unique_perspectives_count = 0;
    float perspective_diversity_score = 0.0f;  ///< 0-1, higher = more diverse
    bool shows_balanced_view = false;
    
    // Quality metrics
    bool meets_diversity_requirement = false;
    std::vector<std::string> common_themes;
    std::vector<std::string> disagreements;
    
    // Metadata
    std::chrono::milliseconds generation_time;
};

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

class MultiPerspectiveGenerator {
public:
    explicit MultiPerspectiveGenerator(
        const MultiPerspectiveConfig& config = {},
        EthicalGuidelinesManager* guidelines_manager = nullptr
    );
    
    ~MultiPerspectiveGenerator();
    
    // ═══════════════════════════════════════════════════════════
    // Core functionality
    // ═══════════════════════════════════════════════════════════
    
    MultiPerspectiveResult generatePerspectives(
        const std::string& query,
        void* llm_wrapper,  // LlamaWrapper* - forward declared
        const std::vector<std::string>& context = {}
    );
    
    /**
     * @brief Generate Single Perspective.
     * @param[in] query Input parameter.
     * @param[in] perspective Input parameter.
     * @param[in,out] llm_wrapper Input/output parameter.
     * @return Return value.
     */
    PerspectiveResponse generateSinglePerspective(
        const std::string& query,
        const EthicalPerspective& perspective,
        void* llm_wrapper
    );
    
    /**
     * @brief Synthesize Perspectives.
     * @param[in] perspectives Input parameter.
     * @param[in] query Input parameter.
     * @return Return value.
     */
    std::string synthesizePerspectives(
        const std::vector<PerspectiveResponse>& perspectives,
        const std::string& query
    );
    
    /**
     * @brief Requires Multi Perspective.
     * @param[in] query Input parameter.
     * @return True when the operation succeeds.
     */
    bool requiresMultiPerspective(const std::string& query);
    
    /**
     * @brief Select Perspectives.
     * @param[in] query Input parameter.
     * @return Return value.
     */
    std::vector<EthicalPerspective> selectPerspectives(const std::string& query);
    
    /**
     * @brief ═══════════════════════════════════════════════════════════ Perspective management ═══════════════════════════════════════════════════════════
     * @param[in] perspective Input parameter.
     */
    
    void addPerspective(const EthicalPerspective& perspective);
    
    /**
     * @brief Remove Perspective.
     * @param[in] perspective_id Identifier of the perspective.
     */
    void removePerspective(const std::string& perspective_id);
    
    /**
     * @brief Get Available Perspectives.
     * @return Return value.
     */
    std::vector<EthicalPerspective> getAvailablePerspectives() const;
    
    /**
     * @brief Load Default Perspectives.
     */
    void loadDefaultPerspectives();
    
    /**
     * @brief ═══════════════════════════════════════════════════════════ Diversity analysis ═══════════════════════════════════════════════════════════
     * @param[in] perspectives Input parameter.
     * @return Return value.
     */
    
    float calculateDiversityScore(
        const std::vector<PerspectiveResponse>& perspectives
    );
    
    /**
     * @brief Find Common Themes.
     * @param[in] perspectives Input parameter.
     * @return Return value.
     */
    std::vector<std::string> findCommonThemes(
        const std::vector<PerspectiveResponse>& perspectives
    );
    
    /**
     * @brief Find Disagreements.
     * @param[in] perspectives Input parameter.
     * @return Return value.
     */
    std::vector<std::string> findDisagreements(
        const std::vector<PerspectiveResponse>& perspectives
    );
    
    /**
     * @brief ═══════════════════════════════════════════════════════════ Configuration ═══════════════════════════════════════════════════════════
     * @param[in] config Input parameter.
     */
    
    void setConfig(const MultiPerspectiveConfig& config);
    
    /**
     * @brief Get Config.
     * @return Return value.
     */
    MultiPerspectiveConfig getConfig() const;
    
    /**
     * @brief Set Ethical Guidelines Manager.
     * @param[in,out] manager Input/output parameter.
     */
    void setEthicalGuidelinesManager(EthicalGuidelinesManager* manager);
    
    /**
     * @brief Clear Cache.
     */
    void clearCache();
    
    // ═══════════════════════════════════════════════════════════
    // Statistics
    // ═══════════════════════════════════════════════════════════
    
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
     * @brief Return access control statistics.
     * @return Access control statistics.
     */
    Statistics getStatistics() const;
    
    /**
     * @brief Reset Statistics.
     */
    void resetStatistics();
    
    void setGenerationCallback(
        std::function<void(const MultiPerspectiveResult&)> callback
    );

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    
    // Helper methods
    /**
     * @brief Build Perspective Prompt.
     * @param[in] query Input parameter.
     * @param[in] perspective Input parameter.
     * @return Return value.
     */
    std::string buildPerspectivePrompt(
        const std::string& query,
        const EthicalPerspective& perspective
    );
    
    /**
     * @brief Build Synthesis Prompt.
     * @param[in] perspectives Input parameter.
     * @param[in] query Input parameter.
     * @return Return value.
     */
    std::string buildSynthesisPrompt(
        const std::vector<PerspectiveResponse>& perspectives,
        const std::string& query
    );
    
    /**
     * @brief Detect Ethical Query.
     * @param[in] query Input parameter.
     * @return True when the operation succeeds.
     */
    bool detectEthicalQuery(const std::string& query);
    
    /**
     * @brief Extract Key Points.
     * @param[in] response Input parameter.
     * @param[in] perspective Input parameter.
     * @return Return value.
     */
    std::vector<std::string> extractKeyPoints(
        const std::string& response,
        const EthicalPerspective& perspective
    );
    
    /**
     * @brief Update Statistics.
     * @param[in] result Input parameter.
     */
    void updateStatistics(const MultiPerspectiveResult& result);
};

class MultiPerspectiveGeneratorFactory {
public:
    /**
     * @brief Create Default.
     * @return Return value.
     */
    static std::unique_ptr<MultiPerspectiveGenerator> createDefault();
    
    /**
     * @brief Create High Diversity.
     * @return Return value.
     */
    static std::unique_ptr<MultiPerspectiveGenerator> createHighDiversity();
    
    /**
     * @brief Create With Perspectives.
     * @param[in] required_perspectives Input parameter.
     * @return Return value.
     */
    static std::unique_ptr<MultiPerspectiveGenerator> createWithPerspectives(
        const std::vector<std::string>& required_perspectives
    );
    
    /**
     * @brief Create.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    static std::unique_ptr<MultiPerspectiveGenerator> create(
        const MultiPerspectiveConfig& config
    );
};

} // namespace llm
} // namespace themis
