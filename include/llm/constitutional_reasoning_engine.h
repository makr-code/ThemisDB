/**
 * @file constitutional_reasoning_engine.h
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
#include <chrono>
#include <functional>

namespace themis {
namespace llm {

struct ConstitutionalPrinciple {
    /**
     * @brief Constitutional Principle.
     * @return Return value.
     */
    virtual ~ConstitutionalPrinciple() = default;
    std::string id;
    std::string name;
    std::string description;
    int priority = 0;                  ///< Higher = more important
    std::string critique_prompt;       ///< Prompt for self-critique
    std::string revision_prompt;       ///< Prompt for self-revision
    
    // Metadata
    std::string source;                ///< e.g., "UN Human Rights Art. 1"
    bool domain_agnostic = false;              ///< true if universal principle
};

struct ConstitutionalReasoningResult {
    /**
     * @brief Constitutional Reasoning Result.
     * @return Return value.
     */
    virtual ~ConstitutionalReasoningResult() = default;
    // Original response
    std::string original_response;
    
    // Critique results
    std::vector<std::string> critiques;           ///< Self-critiques generated
    std::vector<std::string> violated_principles; ///< Principles violated
    std::vector<std::string> applied_principles;  ///< Principles successfully applied
    
    // Revision results
    std::string revised_response;                 ///< Revised output
    bool was_revised = false;                             ///< true if revision occurred
    std::string revision_reasoning;               ///< Explanation of revision
    
    // Quality metrics
    float original_score = 0.0f;                         ///< Score before revision (0-1)
    float revised_score = 0.0f;                          ///< Score after revision (0-1)
    float improvement = 0.0f;                            ///< Improvement delta
    
    // Metadata
    std::chrono::milliseconds critique_time;
    std::chrono::milliseconds revision_time;
    int iterations = 0;                               ///< Number of critique-revision cycles
};

struct ConstitutionalReasoningConfig {
    // Principles to apply
    std::vector<ConstitutionalPrinciple> principles;
    std::vector<std::string> domain_constraints; ///< Domain-specific additions
    
    // Reasoning settings
    bool enable_self_critique = true;
    bool enable_self_revision = true;
    int max_iterations = 3;                       ///< Max critique-revision cycles
    float improvement_threshold = 0.05f;          ///< Min improvement to continue
    
    // Quality thresholds
    float min_acceptable_score = 0.7f;
    bool require_all_principles = false;          ///< Must satisfy all principles
    
    // Performance
    bool cache_critiques = true;
    size_t max_cache_size = 500;
    bool async_processing = false;
};

class ConstitutionalReasoningEngine {
public:
    using PromptRunner = std::function<std::string(const std::string&)>;

    explicit ConstitutionalReasoningEngine(
        const ConstitutionalReasoningConfig& config = {}
    );
    
    ~ConstitutionalReasoningEngine();
    
    /**
     * @brief ═══════════════════════════════════════════════════════════ Core functionality ═══════════════════════════════════════════════════════════
     * @param[in] response Input parameter.
     * @param[in] query Input parameter.
     * @param[in,out] llm_wrapper Input/output parameter.
     * @return Return value.
     */
    
    ConstitutionalReasoningResult reason(
        const std::string& response,
        const std::string& query,
        void* llm_wrapper  // LlamaWrapper* - forward declared
    );
    
    /**
     * @brief Generate Critique.
     * @param[in] response Input parameter.
     * @param[in] query Input parameter.
     * @param[in] principle Input parameter.
     * @param[in,out] llm_wrapper Input/output parameter.
     * @return Return value.
     */
    std::string generateCritique(
        const std::string& response,
        const std::string& query,
        const ConstitutionalPrinciple& principle,
        void* llm_wrapper
    );
    
    /**
     * @brief Generate Revision.
     * @param[in] response Input parameter.
     * @param[in] critiques Input parameter.
     * @param[in] query Input parameter.
     * @param[in,out] llm_wrapper Input/output parameter.
     * @return Return value.
     */
    std::string generateRevision(
        const std::string& response,
        const std::vector<std::string>& critiques,
        const std::string& query,
        void* llm_wrapper
    );
    
    /**
     * @brief Check Violations.
     * @param[in] response Input parameter.
     * @return Return value.
     */
    std::vector<std::string> checkViolations(const std::string& response);
    
    /**
     * @brief Score Response.
     * @param[in] response Input parameter.
     * @return Return value.
     */
    float scoreResponse(const std::string& response);
    
    /**
     * @brief ═══════════════════════════════════════════════════════════ Principle management ═══════════════════════════════════════════════════════════
     * @param[in] principle Input parameter.
     */
    
    void addPrinciple(const ConstitutionalPrinciple& principle);
    
    /**
     * @brief Remove Principle.
     * @param[in] principle_id Identifier of the principle.
     */
    void removePrinciple(const std::string& principle_id);
    
    /**
     * @brief Get Principles.
     * @return Return value.
     */
    std::vector<ConstitutionalPrinciple> getPrinciples() const;
    
    /**
     * @brief Load Default Principles.
     */
    void loadDefaultPrinciples();
    
    /**
     * @brief ═══════════════════════════════════════════════════════════ Configuration ═══════════════════════════════════════════════════════════
     * @param[in] config Input parameter.
     */
    
    void setConfig(const ConstitutionalReasoningConfig& config);
    
    /**
     * @brief Get Config.
     * @return Return value.
     */
    ConstitutionalReasoningConfig getConfig() const;
    
    /**
     * @brief Clear Cache.
     */
    void clearCache();
    
    // ═══════════════════════════════════════════════════════════
    // Statistics and monitoring
    // ═══════════════════════════════════════════════════════════
    
    struct Statistics {
        uint64_t total_reasonings = 0;
        uint64_t revisions_performed = 0;
        uint64_t violations_detected = 0;
        uint64_t cache_hits = 0;
        uint64_t cache_misses = 0;
        
        // Score metrics
        float avg_original_score = 0.0f;
        float avg_revised_score = 0.0f;
        float avg_improvement = 0.0f;
        
        // Timing
        std::chrono::milliseconds avg_critique_time{0};
        std::chrono::milliseconds avg_revision_time{0};
        
        // Principle tracking
        std::unordered_map<std::string, uint64_t> principle_violations;
        std::unordered_map<std::string, uint64_t> principle_applications;
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
    
    void setReasoningCallback(
        std::function<void(const ConstitutionalReasoningResult&)> callback
    );

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    
    // Helper methods
    /**
     * @brief Build Critique Prompt.
     * @param[in] response Input parameter.
     * @param[in] query Input parameter.
     * @param[in] principle Input parameter.
     * @return Return value.
     */
    std::string buildCritiquePrompt(
        const std::string& response,
        const std::string& query,
        const ConstitutionalPrinciple& principle
    );
    
    /**
     * @brief Build Revision Prompt.
     * @param[in] response Input parameter.
     * @param[in] critiques Input parameter.
     * @param[in] query Input parameter.
     * @return Return value.
     */
    std::string buildRevisionPrompt(
        const std::string& response,
        const std::vector<std::string>& critiques,
        const std::string& query
    );
    
    /**
     * @brief Should Continue Iterating.
     * @param[in] result Input parameter.
     * @param[in] iteration Input parameter.
     * @return True when the operation succeeds.
     */
    bool shouldContinueIterating(
        const ConstitutionalReasoningResult& result,
        int iteration
    );
    
    /**
     * @brief Update Statistics.
     * @param[in] result Input parameter.
     */
    void updateStatistics(const ConstitutionalReasoningResult& result);
    
    // Violation detection helpers
    /**
     * @brief Check Autonomy Respect.
     * @param[in] response Input parameter.
     * @return True when the operation succeeds.
     */
    bool checkAutonomyRespect(const std::string& response);
    /**
     * @brief Check Transparency.
     * @param[in] response Input parameter.
     * @return True when the operation succeeds.
     */
    bool checkTransparency(const std::string& response);
    /**
     * @brief Check Non Harmfulness.
     * @param[in] response Input parameter.
     * @return True when the operation succeeds.
     */
    bool checkNonHarmfulness(const std::string& response);
    /**
     * @brief Check Fairness.
     * @param[in] response Input parameter.
     * @return True when the operation succeeds.
     */
    bool checkFairness(const std::string& response);
};

class ConstitutionalReasoningFactory {
public:
    /**
     * @brief Create Default.
     * @return Return value.
     */
    static std::unique_ptr<ConstitutionalReasoningEngine> createDefault();
    
    /**
     * @brief Create Strict.
     * @return Return value.
     */
    static std::unique_ptr<ConstitutionalReasoningEngine> createStrict();
    
    /**
     * @brief Create Lenient.
     * @return Return value.
     */
    static std::unique_ptr<ConstitutionalReasoningEngine> createLenient();
    
    /**
     * @brief Create.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    static std::unique_ptr<ConstitutionalReasoningEngine> create(
        const ConstitutionalReasoningConfig& config
    );
};

} // namespace llm
} // namespace themis
