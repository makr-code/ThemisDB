/*
 * ThemisDB | File: constitutional_reasoning_engine.h | Version: 0.0.47 | Last Modified: 2026-06-01 07:40:07
 * Author: copilot-swe-agent[bot] | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 360
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * PR History (last 5): #836 Implement production-ready ... (2026-03-11)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

/**
 * @file constitutional_reasoning_engine.h
 * @brief Constitutional AI-style self-critique and revision engine
 * 
 * Implements self-critique and revision pattern inspired by Anthropic's
 * Constitutional AI (Bai et al., 2022). Uses universal ethical principles
 * to critique and revise LLM outputs without domain-specific rules.
 * 
 * Key features:
 * - Self-critique prompts based on universal principles
 * - Self-revision based on detected issues
 * - Principle tracking and application logging
 * - Domain-agnostic ethical reasoning
 * 
 * Scientific foundation:
 * - Bai et al. (2022): Constitutional AI - Harmlessness from AI Feedback
 * - UN Human Rights (1948): Universal ethical foundation
 * - Asimov (1942): Three Laws of Robotics (adapted)
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

/**
 * @brief Constitutional principle for self-critique
 */
struct ConstitutionalPrinciple {
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

/**
 * @brief Result of constitutional reasoning
 */
struct ConstitutionalReasoningResult {
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

/**
 * @brief Configuration for constitutional reasoning
 */
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

/**
 * @brief Constitutional Reasoning Engine
 * 
 * Applies constitutional AI principles to critique and revise LLM outputs.
 * Uses universal ethical principles (UN Human Rights, Asimov's Laws) to
 * ensure outputs respect human autonomy, acknowledge uncertainty, and
 * avoid harmful content.
 */
class ConstitutionalReasoningEngine {
public:
    using PromptRunner = std::function<std::string(const std::string&)>;

    /**
     * @brief Constructor with configuration
     * @param config Configuration for reasoning
     */
    explicit ConstitutionalReasoningEngine(
        const ConstitutionalReasoningConfig& config = {}
    );
    
    /**
     * @brief Destructor
     */
    ~ConstitutionalReasoningEngine();
    
    // ═══════════════════════════════════════════════════════════
    // Core functionality
    // ═══════════════════════════════════════════════════════════
    
    /**
     * @brief Apply constitutional reasoning to critique and revise response
     * @param response Original LLM response
     * @param query Original user query (for context)
     * @param llm_wrapper Optional pointer to a PromptRunner used for
     *        critique/revision completions; nullptr falls back to the
     *        deterministic rule-based path.
     * @return Reasoning result with critiques and revised response
     */
    ConstitutionalReasoningResult reason(
        const std::string& response,
        const std::string& query,
        void* llm_wrapper  // LlamaWrapper* - forward declared
    );
    
    /**
     * @brief Generate self-critique for response
     * @param response Response to critique
     * @param query Original query
     * @param principle Principle to apply
     * @param llm_wrapper Optional pointer to a PromptRunner for critique generation
     * @return Critique text
     */
    std::string generateCritique(
        const std::string& response,
        const std::string& query,
        const ConstitutionalPrinciple& principle,
        void* llm_wrapper
    );
    
    /**
     * @brief Generate revised response based on critiques
     * @param response Original response
     * @param critiques Generated critiques
     * @param query Original query
     * @param llm_wrapper Optional pointer to a PromptRunner for revision generation
     * @return Revised response
     */
    std::string generateRevision(
        const std::string& response,
        const std::vector<std::string>& critiques,
        const std::string& query,
        void* llm_wrapper
    );
    
    /**
     * @brief Check if response violates constitutional principles
     * @param response Response to check
     * @return Vector of violated principle IDs
     */
    std::vector<std::string> checkViolations(const std::string& response);
    
    /**
     * @brief Score response against constitutional principles
     * @param response Response to score
     * @return Score (0-1) indicating compliance
     */
    float scoreResponse(const std::string& response);
    
    // ═══════════════════════════════════════════════════════════
    // Principle management
    // ═══════════════════════════════════════════════════════════
    
    /**
     * @brief Add constitutional principle
     * @param principle Principle to add
     */
    void addPrinciple(const ConstitutionalPrinciple& principle);
    
    /**
     * @brief Remove principle by ID
     * @param principle_id Principle ID to remove
     */
    void removePrinciple(const std::string& principle_id);
    
    /**
     * @brief Get all principles
     * @return Vector of all principles
     */
    std::vector<ConstitutionalPrinciple> getPrinciples() const;
    
    /**
     * @brief Load default constitutional principles
     * 
     * Loads universal principles based on:
     * - UN Human Rights (1948)
     * - Asimov's Laws (adapted for AI)
     * - Core ethical guidelines
     */
    void loadDefaultPrinciples();
    
    // ═══════════════════════════════════════════════════════════
    // Configuration
    // ═══════════════════════════════════════════════════════════
    
    /**
     * @brief Update configuration
     * @param config New configuration
     */
    void setConfig(const ConstitutionalReasoningConfig& config);
    
    /**
     * @brief Get current configuration
     * @return Current configuration
     */
    ConstitutionalReasoningConfig getConfig() const;
    
    /**
     * @brief Clear critique cache
     */
    void clearCache();
    
    // ═══════════════════════════════════════════════════════════
    // Statistics and monitoring
    // ═══════════════════════════════════════════════════════════
    
    /**
     * @brief Statistics for monitoring
     */
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
     * @brief Get statistics
     * @return Current statistics
     */
    Statistics getStatistics() const;
    
    /**
     * @brief Reset statistics
     */
    void resetStatistics();
    
    /**
     * @brief Set callback for reasoning completion
     * @param callback Function to call after each reasoning
     */
    void setReasoningCallback(
        std::function<void(const ConstitutionalReasoningResult&)> callback
    );

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    
    // Helper methods
    std::string buildCritiquePrompt(
        const std::string& response,
        const std::string& query,
        const ConstitutionalPrinciple& principle
    );
    
    std::string buildRevisionPrompt(
        const std::string& response,
        const std::vector<std::string>& critiques,
        const std::string& query
    );
    
    bool shouldContinueIterating(
        const ConstitutionalReasoningResult& result,
        int iteration
    );
    
    void updateStatistics(const ConstitutionalReasoningResult& result);
    
    // Violation detection helpers
    bool checkAutonomyRespect(const std::string& response);
    bool checkTransparency(const std::string& response);
    bool checkNonHarmfulness(const std::string& response);
    bool checkFairness(const std::string& response);
};

/**
 * @brief Factory for creating constitutional reasoning engines
 */
class ConstitutionalReasoningFactory {
public:
    /**
     * @brief Create engine with default UN/Asimov principles
     */
    static std::unique_ptr<ConstitutionalReasoningEngine> createDefault();
    
    /**
     * @brief Create engine with strict principles
     */
    static std::unique_ptr<ConstitutionalReasoningEngine> createStrict();
    
    /**
     * @brief Create engine with lenient principles
     */
    static std::unique_ptr<ConstitutionalReasoningEngine> createLenient();
    
    /**
     * @brief Create engine with custom configuration
     */
    static std::unique_ptr<ConstitutionalReasoningEngine> create(
        const ConstitutionalReasoningConfig& config
    );
};

} // namespace llm
} // namespace themis
