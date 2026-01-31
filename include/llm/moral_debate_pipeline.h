#pragma once

#include "llm/moral_analyzer.h"
#include "llm/ethical_guidelines_manager.h"
#include "llm/llamacpp_inference_engine.h"
#include "storage/rocksdb_wrapper.h"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>

namespace themis {
namespace llm {

/**
 * @brief Complete pipeline for moral philosophy debates
 * 
 * This class provides a complete workflow similar to the Python example:
 * 1. Load ethical scenarios from YAML
 * 2. Extract ethical questions
 * 3. Generate multi-philosophy debates
 * 4. Store results in ThemisDB (Graph, Vector, Relational, Timeline)
 * 5. Synthesize AI recommendations
 * 6. Track outcomes and enable self-improvement
 * 
 * Example Usage:
 * ```cpp
 * MoralDebatePipeline pipeline(db_path, llm_engine);
 * 
 * // Load scenario from YAML
 * auto scenario = pipeline.loadScenarioFromYAML("trolley_001");
 * 
 * // Run complete debate
 * auto debate = pipeline.conductDebate(scenario, 
 *     {"kant", "utilitarian", "virtue"});
 * 
 * // Get AI synthesis
 * auto recommendation = pipeline.synthesizeRecommendation(debate);
 * 
 * // Store in ThemisDB
 * pipeline.storeDebateResults(debate);
 * ```
 */
class MoralDebatePipeline {
public:
    /**
     * @brief Philosophy perspective in debate
     */
    struct PhilosophyPerspective {
        std::string philosophy_name;
        std::string recommended_action;
        std::string reasoning;
        std::vector<std::string> supporting_principles;
        std::vector<std::string> objecting_principles;
        double confidence;
        std::vector<MoralAnalyzer::EthicalArgument> arguments;
    };
    
    /**
     * @brief Complete debate session
     */
    struct DebateSession {
        std::string session_id;
        std::string scenario_id;
        MoralAnalyzer::EthicalScenario scenario;
        std::vector<PhilosophyPerspective> perspectives;
        std::map<std::string, std::string> cross_philosophy_critiques;
        std::string ai_synthesis;
        std::string final_recommendation;
        double consensus_score;
        std::string graph_id;
        int64_t timestamp_ms;
        
        // Metadata
        std::map<std::string, double> metrics;
        std::vector<std::string> conflicting_principles;
        std::vector<std::string> common_ground;
    };
    
    /**
     * @brief Pipeline configuration
     */
    struct PipelineConfig {
        std::string yaml_scenarios_path = "ethical_scenarios.yaml";
        std::string philosophies_yaml_path = "philosophies.yaml";
        bool enable_llm_integration = true;
        bool enable_vector_search = true;
        bool enable_outcome_tracking = true;
        bool enable_self_improvement = true;
        int max_debate_rounds = 3;
        double consensus_threshold = 0.7;
        std::vector<std::string> default_philosophies = {
            "kant", "utilitarian", "virtue", "care_ethics", "rawls"
        };
    };
    
    /**
     * @brief Status codes for pipeline operations
     */
    enum class Status {
        OK,
        ERROR_FILE_NOT_FOUND,
        ERROR_YAML_PARSE,
        ERROR_INVALID_SCENARIO,
        ERROR_LLM_FAILURE,
        ERROR_DB_FAILURE,
        ERROR_INVALID_PHILOSOPHY
    };
    
    /**
     * @brief Constructor
     * 
     * @param db Database wrapper
     * @param guidelines_mgr Ethical guidelines manager
     * @param llm_engine LLM inference engine (optional)
     * @param config Pipeline configuration
     */
    MoralDebatePipeline(
        std::shared_ptr<storage::RocksDBWrapper> db,
        std::shared_ptr<EthicalGuidelinesManager> guidelines_mgr,
        std::shared_ptr<LlamaCppInferenceEngine> llm_engine = nullptr,
        const PipelineConfig& config = PipelineConfig()
    );
    
    /**
     * @brief Load scenario from YAML configuration
     * 
     * @param scenario_id Scenario identifier
     * @return Pair of status and ethical scenario
     */
    std::pair<Status, MoralAnalyzer::EthicalScenario> 
    loadScenarioFromYAML(const std::string& scenario_id);
    
    /**
     * @brief Load all scenarios from YAML
     * 
     * @return Pair of status and vector of scenarios
     */
    std::pair<Status, std::vector<MoralAnalyzer::EthicalScenario>>
    loadAllScenariosFromYAML();
    
    /**
     * @brief Conduct complete multi-philosophy debate
     * 
     * @param scenario Ethical scenario to debate
     * @param philosophies List of philosophies to include
     * @return Pair of status and debate session
     */
    std::pair<Status, DebateSession>
    conductDebate(
        const MoralAnalyzer::EthicalScenario& scenario,
        const std::vector<std::string>& philosophies
    );
    
    /**
     * @brief Generate cross-philosophy critiques
     * 
     * Each philosophy critiques others' arguments
     * 
     * @param debate Debate session to add critiques to
     * @return Status code
     */
    Status generateCrossPhilosophyCritiques(DebateSession& debate);
    
    /**
     * @brief Synthesize AI recommendation from debate
     * 
     * Uses multi-philosophy synthesis to create final recommendation
     * 
     * @param debate Debate session
     * @return Pair of status and recommendation string
     */
    std::pair<Status, std::string>
    synthesizeRecommendation(const DebateSession& debate);
    
    /**
     * @brief Store debate results in ThemisDB
     * 
     * Stores in multiple models:
     * - Graph: Decision chains and reasoning paths
     * - Vector: For similarity search of precedent cases
     * - Relational: Structured metadata and analytics
     * - Timeline: Temporal evolution of decisions
     * 
     * @param debate Debate session to store
     * @return Status code
     */
    Status storeDebateResults(const DebateSession& debate);
    
    /**
     * @brief Search for similar past debates using vector search
     * 
     * @param scenario Current scenario
     * @param top_k Number of similar debates to retrieve
     * @return Vector of similar debate session IDs
     */
    std::vector<std::string> 
    searchSimilarDebates(
        const MoralAnalyzer::EthicalScenario& scenario,
        int top_k = 5
    );
    
    /**
     * @brief Track outcome of a decision
     * 
     * Used for self-improvement loop
     * 
     * @param session_id Debate session ID
     * @param actual_outcome Description of what happened
     * @param success_score 0.0 to 1.0 indicating success
     * @param stakeholder_feedback Optional feedback from stakeholders
     * @return Status code
     */
    Status trackOutcome(
        const std::string& session_id,
        const std::string& actual_outcome,
        double success_score,
        const std::map<std::string, std::string>& stakeholder_feedback = {}
    );
    
    /**
     * @brief Retrieve debate session by ID
     * 
     * @param session_id Session identifier
     * @return Pair of status and debate session
     */
    std::pair<Status, DebateSession>
    getDebateSession(const std::string& session_id);
    
    /**
     * @brief Export debate to DOT format for visualization
     * 
     * @param debate Debate session
     * @param output_path Path to write DOT file
     * @return Status code
     */
    Status exportDebateGraph(
        const DebateSession& debate,
        const std::string& output_path
    );
    
    /**
     * @brief Run benchmarks on scenarios
     * 
     * @param scenario_ids List of scenario IDs to benchmark
     * @param philosophies Philosophies to test
     * @return Map of scenario_id -> metrics
     */
    std::map<std::string, std::map<std::string, double>>
    runBenchmarks(
        const std::vector<std::string>& scenario_ids,
        const std::vector<std::string>& philosophies
    );
    
    /**
     * @brief Get pipeline statistics
     * 
     * @return Map of statistic name to value
     */
    std::map<std::string, double> getStatistics() const;
    
private:
    std::shared_ptr<storage::RocksDBWrapper> db_;
    std::shared_ptr<EthicalGuidelinesManager> guidelines_mgr_;
    std::shared_ptr<LlamaCppInferenceEngine> llm_engine_;
    std::shared_ptr<MoralAnalyzer> analyzer_;
    PipelineConfig config_;
    
    // Cached scenarios and philosophies
    std::map<std::string, MoralAnalyzer::EthicalScenario> scenarios_cache_;
    std::map<std::string, std::string> philosophies_cache_;
    
    // Statistics
    mutable int total_debates_ = 0;
    mutable int total_scenarios_ = 0;
    mutable double avg_consensus_score_ = 0.0;
    
    /**
     * @brief Parse YAML file
     */
    Status parseYAMLFile(const std::string& path, void* data);
    
    /**
     * @brief Generate argument using LLM
     */
    std::pair<Status, std::string>
    generateArgumentWithLLM(
        const MoralAnalyzer::EthicalScenario& scenario,
        const std::string& philosophy,
        const std::string& action
    );
    
    /**
     * @brief Calculate consensus score
     */
    double calculateConsensusScore(const DebateSession& debate) const;
    
    /**
     * @brief Find common ground between philosophies
     */
    std::vector<std::string>
    findCommonGround(const DebateSession& debate) const;
    
    /**
     * @brief Identify conflicting principles
     */
    std::vector<std::string>
    identifyConflicts(const DebateSession& debate) const;
    
    /**
     * @brief Store in graph database
     */
    Status storeInGraphDB(const DebateSession& debate);
    
    /**
     * @brief Store in vector database
     */
    Status storeInVectorDB(const DebateSession& debate);
    
    /**
     * @brief Store in relational database
     */
    Status storeInRelationalDB(const DebateSession& debate);
    
    /**
     * @brief Store in timeline database
     */
    Status storeInTimelineDB(const DebateSession& debate);
};

} // namespace llm
} // namespace themis
