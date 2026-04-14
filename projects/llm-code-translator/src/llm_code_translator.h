/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            llm_code_translator.h                              ║
  Version:         0.0.42                                             ║
  Last Modified:   2026-04-14 18:45:37                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     270                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#ifndef LLM_CODE_TRANSLATOR_H
#define LLM_CODE_TRANSLATOR_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <optional>
#include <nlohmann/json.hpp>

namespace themis {

// Forward declarations
class LLMInteractionStore;
namespace rocksdb {
    class TransactionDB;
}

/**
 * @brief LLM Code Translator - Generates executable code from natural language descriptions
 * 
 * This class provides functionality to translate user descriptions of architectural solutions
 * and data processing requirements into machine-readable code using LLM prompting.
 * 
 * Features:
 * - Multi-language support (C++, Python, AQL)
 * - Security validation before execution
 * - Runtime compilation and execution
 * - Audit logging of all generations
 * - Context-aware code generation
 * 
 * Example:
 * ```cpp
 * LLMCodeTranslator translator(db, llm_client);
 * auto result = translator.generateCode(
 *     "Create a query to find all users active in last 7 days", 
 *     "aql"
 * );
 * if (result.success && result.security_approved) {
 *     auto exec_result = translator.executeCode(result.code, "aql");
 * }
 * ```
 */
class LLMCodeTranslator {
public:
    /**
     * @brief Supported programming languages for code generation
     */
    enum class Language {
        CPP,      // C++ code
        PYTHON,   // Python scripts
        AQL,      // ThemisDB AQL queries
        SQL       // SQL queries (for compatibility)
    };

    /**
     * @brief Result of code generation
     */
    struct GenerationResult {
        bool success = false;
        std::string code;                    // Generated code
        std::string language;                // Target language
        std::string error;                   // Error message if failed
        bool security_approved = false;      // Passed security review?
        double quality_score = 0.0;          // Quality score 0-1
        std::vector<std::string> warnings;   // Non-critical warnings
        std::vector<std::string> suggestions; // Improvement suggestions
        nlohmann::json metadata;             // Additional metadata
    };

    /**
     * @brief Result of code execution
     */
    struct ExecutionResult {
        bool success = false;
        std::string output;                  // Execution output/result
        std::string error;                   // Error message if failed
        int64_t duration_ms = 0;             // Execution time
        nlohmann::json result_data;          // Structured result data
        size_t memory_used_kb = 0;           // Memory usage
    };

    /**
     * @brief Execution limits for sandboxed code
     */
    struct ExecutionLimits {
        int64_t max_execution_time_ms = 30000;  // 30 seconds default
        size_t max_memory_mb = 512;             // 512 MB default
        int max_cpu_percent = 80;               // 80% CPU default
        bool allow_network = false;              // Network access disabled by default
        bool allow_file_writes = false;          // File writes disabled by default

        static ExecutionLimits default_limits() {
            return ExecutionLimits{};
        }
    };

    /**
     * @brief Code review result
     */
    struct ReviewResult {
        bool approved = false;
        double quality_score = 0.0;
        std::vector<std::string> issues;      // Blocking issues
        std::vector<std::string> warnings;    // Non-blocking warnings
        std::vector<std::string> suggestions; // Improvement suggestions
    };

    /**
     * @brief Configuration for LLM code translator
     */
    struct Config {
        std::string llm_endpoint = "http://localhost:8000";  // vLLM or OpenAI API endpoint
        std::string llm_model = "codellama/CodeLlama-13b-Instruct-hf";
        std::string api_key = "";                            // Optional API key
        double temperature = 0.2;                            // Low temp for deterministic code
        int max_tokens = 4096;
        bool enable_auto_execution = false;                  // Auto-execute generated code?
        bool enable_security_review = true;                  // Auto security review?
        bool log_all_generations = true;                     // Log to LLMInteractionStore?
    };

    /**
     * @brief Construct LLMCodeTranslator
     * @param db ThemisDB instance for logging and data access
     * @param config Configuration
     */
    explicit LLMCodeTranslator(
        rocksdb::TransactionDB* db,
        const Config& config
    );

    ~LLMCodeTranslator() = default;

    /**
     * @brief Generate code from natural language description
     * @param user_description User's description of desired functionality
     * @param target_language Target programming language
     * @param context Additional context (tables, schemas, existing functions)
     * @return Generation result with code and metadata
     */
    GenerationResult generateCode(
        const std::string& user_description,
        const std::string& target_language,
        const std::map<std::string, std::string>& context = {}
    );

    /**
     * @brief Generate code with automatic security review
     * @param user_description User's description
     * @param target_language Target language
     * @param context Additional context
     * @return Generation result (includes security approval status)
     */
    GenerationResult generateAndReview(
        const std::string& user_description,
        const std::string& target_language,
        const std::map<std::string, std::string>& context = {}
    );

    /**
     * @brief Review generated code for security and quality
     * @param code Code to review
     * @param language Programming language
     * @return Review result
     */
    ReviewResult reviewCode(
        const std::string& code,
        const std::string& language
    );

    /**
     * @brief Execute generated code in sandboxed environment
     * @param code Code to execute
     * @param language Programming language
     * @param limits Execution limits
     * @return Execution result
     */
    ExecutionResult executeCode(
        const std::string& code,
        const std::string& language,
        const ExecutionLimits& limits = ExecutionLimits::default_limits()
    );

    /**
     * @brief Regenerate code with user feedback
     * @param previous_result Previous generation result
     * @param feedback User feedback on the code
     * @return New generation result
     */
    GenerationResult regenerateWithFeedback(
        const GenerationResult& previous_result,
        const std::string& feedback
    );

    /**
     * @brief Get available prompt templates
     * @return Map of template names to templates
     */
    std::map<std::string, std::string> getAvailableTemplates() const;

    /**
     * @brief Set custom prompt template
     * @param name Template name
     * @param template_text Template text (with placeholders like {user_description})
     */
    void setPromptTemplate(const std::string& name, const std::string& template_text);

private:
    rocksdb::TransactionDB* db_;
    Config config_;
    std::shared_ptr<LLMInteractionStore> interaction_store_;
    std::map<std::string, std::string> prompt_templates_;

    // Internal helper methods
    std::string buildPrompt(
        const std::string& user_description,
        const std::string& target_language,
        const std::map<std::string, std::string>& context
    );

    std::string callLLM(const std::string& prompt);

    bool validateInput(const std::string& user_input);

    bool checkSyntax(const std::string& code, const std::string& language);

    std::vector<std::string> scanSecurityIssues(const std::string& code);

    double calculateQualityScore(const std::string& code, const std::string& language);

    void logGeneration(
        const std::string& user_description,
        const GenerationResult& result
    );

    // Language-specific executors
    ExecutionResult executePython(const std::string& code, const ExecutionLimits& limits);
    ExecutionResult executeCpp(const std::string& code, const ExecutionLimits& limits);
    ExecutionResult executeAQL(const std::string& code, const ExecutionLimits& limits);

    // Initialize default templates
    void initializeDefaultTemplates();
};

} // namespace themis

#endif // LLM_CODE_TRANSLATOR_H
