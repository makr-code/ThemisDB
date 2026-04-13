/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            direct_execution_engine.h                          ║
  Version:         0.0.38                                             ║
  Last Modified:   2026-04-13 04:22:46                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     415                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#ifndef THEMIS_DIRECT_EXECUTION_ENGINE_H
#define THEMIS_DIRECT_EXECUTION_ENGINE_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <optional>
#include <nlohmann/json.hpp>

namespace rocksdb {
    class TransactionDB;
}

namespace themis::direct_execution {

/**
 * @brief Execution Plan - Intermediate Representation (NO CODE!)
 * 
 * This is a declarative description of the operation to execute.
 * It is NOT code that needs to be compiled.
 */
struct ExecutionPlan {
    enum class OperationType {
        QUERY,           // Retrieve data
        AGGREGATE,       // Calculate aggregations
        TRANSFORM,       // Transform data
        JOIN,            // Join data sources
        GRAPH_TRAVERSE,  // Graph traversal
        VECTOR_SEARCH,   // Similarity search
        TIME_SERIES,     // Time-series analysis
        MUTATION         // Insert/Update/Delete
    };

    OperationType operation;
    nlohmann::json parameters;
    std::vector<ExecutionPlan> sub_plans;  // For complex operations
    
    // Convert to/from JSON
    nlohmann::json toJSON() const;
    static ExecutionPlan fromJSON(const nlohmann::json& j);
};

/**
 * @brief Result of direct execution
 */
struct ExecutionResult {
    bool success = true;
    std::string error;
    nlohmann::json data;           // Result data
    int64_t duration_ms = 0;       // Execution time
    size_t rows_affected = 0;      // Number of rows
    
    // Performance metrics
    struct Metrics {
        bool used_index = false;
        std::string index_name;
        size_t rows_scanned = 0;
        size_t rows_filtered = 0;
        int64_t db_time_ms = 0;
        int64_t processing_time_ms = 0;
    } metrics;
};

/**
 * @brief Translates natural language prompts to execution plans
 * 
 * This is the LLM interface that generates structured execution plans
 * instead of code.
 */
class PromptToExecutionPlan {
public:
    struct Config {
        std::string llm_endpoint = "http://localhost:8000";
        std::string llm_model = "codellama/CodeLlama-13b-Instruct-hf";
        double temperature = 0.1;  // Very low for deterministic plans
        int max_tokens = 2048;
    };

    explicit PromptToExecutionPlan(const Config& config);

    /**
     * @brief Translate user prompt to execution plan
     * @param user_prompt Natural language description
     * @return Structured execution plan (JSON-based, NOT code)
     */
    ExecutionPlan translate(const std::string& user_prompt);

    /**
     * @brief Translate with additional context
     * @param user_prompt Natural language description
     * @param context Additional context (schema, available tables, etc.)
     * @return Structured execution plan
     */
    ExecutionPlan translateWithContext(
        const std::string& user_prompt,
        const std::map<std::string, std::string>& context
    );

private:
    Config config_;
    
    std::string buildPrompt(
        const std::string& user_prompt,
        const std::map<std::string, std::string>& context
    );
    
    ExecutionPlan parseResponse(const std::string& llm_response);
};

/**
 * @brief Validates execution plans for security and correctness
 */
class PlanValidator {
public:
    struct ValidationResult {
        bool valid = false;
        std::vector<std::string> errors;
        std::vector<std::string> warnings;
    };

    /**
     * @brief Validate execution plan
     * @param plan Plan to validate
     * @return Validation result
     */
    ValidationResult validate(const ExecutionPlan& plan);

    /**
     * @brief Check if operation is allowed
     * @param operation Operation type
     * @return true if allowed
     */
    bool isAllowedOperation(ExecutionPlan::OperationType operation);

    /**
     * @brief Estimate resource usage
     * @param plan Execution plan
     * @return Estimated memory usage in bytes
     */
    size_t estimateResourceUsage(const ExecutionPlan& plan);

private:
    bool validateQueryPlan(const nlohmann::json& params);
    bool validateAggregatePlan(const nlohmann::json& params);
    bool validateGraphTraversalPlan(const nlohmann::json& params);
    bool validateVectorSearchPlan(const nlohmann::json& params);
};

/**
 * @brief Executes plans directly without code generation/compilation
 * 
 * This is the core of the "Prompt as Language" concept.
 * No intermediate code is generated or compiled.
 */
class DirectExecutor {
public:
    explicit DirectExecutor(rocksdb::TransactionDB* db);

    /**
     * @brief Execute plan directly
     * @param plan Execution plan to execute
     * @return Execution result
     */
    ExecutionResult execute(const ExecutionPlan& plan);

    /**
     * @brief Explain how plan would be executed (dry-run)
     * @param plan Execution plan
     * @return Explanation of execution strategy
     */
    nlohmann::json explain(const ExecutionPlan& plan);

private:
    rocksdb::TransactionDB* db_;
    
    // Operation executors
    ExecutionResult executeQuery(const nlohmann::json& params);
    ExecutionResult executeAggregate(const nlohmann::json& params);
    ExecutionResult executeTransform(const nlohmann::json& params);
    ExecutionResult executeJoin(const nlohmann::json& params);
    ExecutionResult executeGraphTraversal(const nlohmann::json& params);
    ExecutionResult executeVectorSearch(const nlohmann::json& params);
    ExecutionResult executeTimeSeries(const nlohmann::json& params);
    ExecutionResult executeMutation(const nlohmann::json& params);
    
    // Helper methods
    bool hasIndex(const std::string& table, const std::string& field);
    std::vector<nlohmann::json> scanWithFilter(
        const std::string& table,
        const std::vector<nlohmann::json>& filters
    );
    void applySort(
        std::vector<nlohmann::json>& results,
        const nlohmann::json& sort_spec
    );
};

/**
 * @brief Cache for execution plans
 */
class PlanCache {
public:
    struct CacheConfig {
        bool enabled = true;
        size_t max_size = 1000;
        int64_t ttl_seconds = 3600;  // 1 hour
    };

    explicit PlanCache(const CacheConfig& config = CacheConfig{});

    /**
     * @brief Get cached plan
     * @param prompt_hash Hash of user prompt
     * @return Cached plan if found
     */
    std::optional<ExecutionPlan> get(const std::string& prompt_hash);

    /**
     * @brief Store plan in cache
     * @param prompt_hash Hash of user prompt
     * @param plan Execution plan
     */
    void put(const std::string& prompt_hash, const ExecutionPlan& plan);

    /**
     * @brief Get cache statistics
     */
    struct CacheStats {
        size_t hits = 0;
        size_t misses = 0;
        size_t size = 0;
        double hit_rate = 0.0;
    };
    CacheStats getStats() const;

private:
    CacheConfig config_;
    std::map<std::string, ExecutionPlan> cache_;
    std::map<std::string, int64_t> timestamps_;
    size_t hits_ = 0;
    size_t misses_ = 0;
    
    void evictExpired();
    void evictLRU();
    std::string hashPrompt(const std::string& prompt);
};

/**
 * @brief Main Direct Execution Engine
 * 
 * This is the primary interface for executing natural language prompts
 * directly without code generation/compilation.
 * 
 * Key Concept: Prompt is the language!
 */
class DirectExecutionEngine {
public:
    struct Config {
        std::string llm_endpoint = "http://localhost:8000";
        std::string llm_model = "codellama/CodeLlama-13b-Instruct-hf";
        bool enable_caching = true;
        bool enable_plan_validation = true;
        bool enable_metrics = true;
        
        PlanCache::CacheConfig cache_config;
        PromptToExecutionPlan::Config translator_config;
    };

    /**
     * @brief Construct DirectExecutionEngine
     * @param db ThemisDB instance
     * @param config Configuration
     */
    explicit DirectExecutionEngine(
        rocksdb::TransactionDB* db,
        const Config& config
    );

    ~DirectExecutionEngine() = default;

    /**
     * @brief Execute user prompt directly
     * 
     * This is the main entry point. It translates the prompt to an
     * execution plan and executes it directly without generating or
     * compiling any code.
     * 
     * Example:
     * ```cpp
     * auto result = engine.executePrompt(
     *     "Find all users active in last 7 days"
     * );
     * ```
     * 
     * @param user_prompt Natural language description
     * @return Execution result
     */
    ExecutionResult executePrompt(const std::string& user_prompt);

    /**
     * @brief Execute prompt with additional context
     * @param user_prompt Natural language description
     * @param context Additional context (schema, etc.)
     * @return Execution result
     */
    ExecutionResult executePromptWithContext(
        const std::string& user_prompt,
        const std::map<std::string, std::string>& context
    );

    /**
     * @brief Show execution plan without executing (dry-run)
     * @param user_prompt Natural language description
     * @return Execution plan that would be used
     */
    ExecutionPlan explainPrompt(const std::string& user_prompt);

    /**
     * @brief Execute pre-built plan
     * @param plan Execution plan
     * @return Execution result
     */
    ExecutionResult executePlan(const ExecutionPlan& plan);

    /**
     * @brief Get engine statistics
     */
    struct EngineStats {
        size_t total_executions = 0;
        size_t successful_executions = 0;
        size_t failed_executions = 0;
        size_t cached_executions = 0;
        double avg_execution_time_ms = 0.0;
        double avg_translation_time_ms = 0.0;
        PlanCache::CacheStats cache_stats;
    };
    EngineStats getStats() const;

private:
    rocksdb::TransactionDB* db_;
    Config config_;
    
    std::unique_ptr<PromptToExecutionPlan> translator_;
    std::unique_ptr<DirectExecutor> executor_;
    std::unique_ptr<PlanValidator> validator_;
    std::unique_ptr<PlanCache> cache_;
    
    // Statistics
    mutable EngineStats stats_;
    
    void updateStats(const ExecutionResult& result, bool from_cache);
    void logExecution(const std::string& prompt, 
                     const ExecutionPlan& plan,
                     const ExecutionResult& result);
};

/**
 * @brief Fluent API for building execution plans programmatically
 * 
 * This is useful for testing or when you want to bypass the LLM
 * and build plans directly.
 */
class ExecutionPlanBuilder {
public:
    ExecutionPlanBuilder& query(const std::string& table);
    
    ExecutionPlanBuilder& filter(const std::string& field, 
                                const std::string& op, 
                                const nlohmann::json& value);
    
    ExecutionPlanBuilder& groupBy(const std::vector<std::string>& fields);
    
    ExecutionPlanBuilder& aggregate(const std::string& func,
                                   const std::string& field,
                                   const std::string& as);
    
    ExecutionPlanBuilder& sort(const std::string& field, 
                              bool descending = false);
    
    ExecutionPlanBuilder& limit(size_t n);
    
    ExecutionPlanBuilder& returnType(const std::string& type);
    
    ExecutionPlan build();

private:
    ExecutionPlan plan_;
};

} // namespace themis::direct_execution

#endif // THEMIS_DIRECT_EXECUTION_ENGINE_H
