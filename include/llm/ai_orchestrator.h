/**
 * @file ai_orchestrator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include "llm/llm_plugin_interface.h"
#include "llm/themis_tool_interface.h"
#include "plugins/plugin_manager.h"
#include "utils/expected.h"
#include <nlohmann/json.hpp>
#include <chrono>
#include <functional>
#include <memory>
#include <optional>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace themis::llm {

using json = nlohmann::json;

// ============================================================================
// Mode Spec – YAML schema types
// ============================================================================

enum class ModeId {
    Ask,        ///< Plain question-answering (no retrieval)
    Edit,       ///< Instruction-following / text editing
    Rag,        ///< Retrieval-Augmented Generation
    Agentic,    ///< Single-agent with tool use
    MultiAgent, ///< Multi-agent message-passing (skeleton / extensible)
    Ethics,     ///< Ethics-AI plugin mode with constitutional reasoning
    Custom      ///< User-defined id
};

/**
 * @brief Mode Id From String.
 * @param[in] s Input parameter.
 * @return Return value.
 */
ModeId modeIdFromString(const std::string& s);
/**
 * @brief Mode Id To String.
 * @param[in] id Input parameter.
 * @return Return value.
 */
std::string modeIdToString(ModeId id);

// ----------------------------------------------------------------------------
// Sub-specs
// ----------------------------------------------------------------------------

struct RetrievalSpec {
    bool        enabled   = false;
    std::string strategy  = "hybrid";  ///< "vector", "fulltext", "hybrid"
    int         top_k     = 5;
    float       threshold = 0.5f;
    bool        rerank    = false;

    struct ChunkingSpec {
        int    size    = 512;
        int    overlap = 64;
        std::string strategy = "fixed"; ///< "fixed", "sentence", "paragraph"
    } chunking;

    std::string read_ts_semantics = "latest"; ///< "latest", "snapshot:<ts>"
    std::string locality;
};

struct OutputSpec {
    std::string format = "text";     ///< "text", "json", "markdown"
    std::optional<std::string> json_schema; ///< JSON Schema for structured output
    std::optional<std::string> grammar;     ///< EBNF grammar name or inline grammar
};

struct BudgetSpec {
    int    max_tokens   = 512;
    int    timeout_ms   = 30000;
    int    max_retries  = 1;
    float  temperature  = 0.7f;
    float  top_p        = 0.9f;
    int    top_k        = 40;
};

struct ObservabilitySpec {
    bool log_requests   = true;
    bool log_responses  = false;    ///< disabled by default (privacy)
    bool metrics        = true;     ///< Prometheus metrics
    bool trace          = false;    ///< OpenTelemetry tracing
};

struct ToolSpec {
    std::string name;
    std::string description;
    json        args_schema;    ///< JSON Schema for arguments
    int         timeout_ms = 5000;
};

struct ModeSpec {
    std::string      id;          ///< Unique mode identifier, e.g. "rag"
    ModeId           mode_id = ModeId::Custom;
    std::string      description;
    std::string      model_id = "default";
    std::string      lora_adapter_id;

    std::vector<std::string> tools_allowed;
    std::vector<std::string> tools_denied;

    RetrievalSpec   retrieval;
    OutputSpec      output;
    BudgetSpec      budgets;
    ObservabilitySpec observability;

    struct JudgeSpec {
        bool        enabled = false;
        std::string model_id;
        float       min_score = 0.6f;
    } judge;

    struct SafetySpec {
        bool        enabled = false;
        std::string ethics_profile; ///< path or id of ethics YAML profile
    } safety;

    json extensions;
};

struct ModelEntry {
    /**
     * @brief Model Entry.
     * @return Return value.
     */
    virtual ~ModelEntry() = default;
    std::string id;
    std::string path;
    int         gpu_layers = 0;
    int         n_ctx      = 4096;
};

struct ModePack {
    std::string apiVersion; ///< Expected: "themis.ai/v1"
    std::string kind;       ///< Expected: "ThemisModePack" or "ThemisAIPolicy"
    std::string name;
    std::string version = "1.0.0";

    std::vector<ModelEntry> models;
    std::vector<ToolSpec>   tools;
    std::vector<ModeSpec>   modes;

    std::string default_mode = "ask";
};

// ============================================================================
// Mode Spec Loader + Validator
// ============================================================================

struct ValidationResult {
    bool                     ok = true;
    std::vector<std::string> errors;
    std::vector<std::string> warnings;

    explicit operator bool() const { return ok; }
};

class ModeSpecLoader {
public:
    static ModePack loadFromFile(const std::string& path,
                                 ValidationResult*  result_out = nullptr);

    static ModePack loadFromString(const std::string& yaml_text,
                                   ValidationResult*  result_out = nullptr);

    /**
     * @brief Validate.
     * @param[in] pack Input parameter.
     * @return Return value.
     */
    static ValidationResult validate(const ModePack& pack);
};

// ============================================================================
// Tool Registry
// ============================================================================

using ToolHandler = std::function<json(const json& args, const ModeSpec& mode)>;

class ToolRegistry {
public:
    ToolRegistry();
    ~ToolRegistry();

    /**
     * @brief ── Static / built-in tool registration ──────────────────────────────────
     * @param[in] spec Input parameter.
     * @param[in] handler Input parameter.
     */

    void registerTool(const ToolSpec& spec, ToolHandler handler);

    // ── Dynamic tool loading via PluginManager ────────────────────────────────

    Result<void> loadToolPlugin(const std::string& path,
                                const std::string& config = "{}");

    /**
     * @brief Load Tools From Directory.
     * @param[in] directory Input parameter.
     * @return Return value.
     */
    Result<size_t> loadToolsFromDirectory(const std::string& directory);

    /**
     * @brief Reload Tool.
     * @param[in] name Input parameter.
     * @return Return value.
     */
    Result<void> reloadTool(const std::string& name);

    /**
     * @brief Unload Tool.
     * @param[in] name Input parameter.
     * @return Return value.
     */
    Result<void> unloadTool(const std::string& name);

    /**
     * @brief ── Dispatch ──────────────────────────────────────────────────────────────
     * @param[in] tool_name Name of the tool.
     * @param[in] args Input parameter.
     * @param[in] mode Input parameter.
     * @return Return value.
     */

    json invokeTool(const std::string& tool_name,
                    const json&        args,
                    const ModeSpec&    mode) const;

    /**
     * @brief Is Allowed.
     * @param[in] tool_name Name of the tool.
     * @param[in] mode Input parameter.
     * @return True when the operation succeeds.
     */
    bool isAllowed(const std::string& tool_name,
                   const ModeSpec&    mode) const;

    /**
     * @brief List Tools.
     * @return Return value.
     */
    std::vector<std::string> listTools() const;

    /**
     * @brief Get Spec.
     * @param[in] tool_name Name of the tool.
     * @return Return value.
     */
    std::optional<ToolSpec> getSpec(const std::string& tool_name) const;

    /**
     * @brief Is Plugin Tool.
     * @param[in] name Input parameter.
     * @return True when the operation succeeds.
     */
    bool isPluginTool(const std::string& name) const;

private:
    struct Entry {
        ToolSpec    spec;
        ToolHandler handler;
        bool        is_plugin = false;  ///< true when backed by a DLL plugin
    };

    /**
     * @brief Register Plugin Tool.
     * @param[in,out] tool Input/output parameter.
     */
    void registerPluginTool(IThemisTool* tool);

    std::unordered_map<std::string, Entry>      tools_;
    mutable std::shared_mutex                   tools_mutex_;   ///< guards tools_
    std::unique_ptr<plugins::PluginManager>     plugin_manager_; ///< DLL lifecycle
};

// ============================================================================
// Orchestrator – run metadata and result
// ============================================================================

struct RunLatency {
    /**
     * @brief Run Latency.
     * @return Return value.
     */
    virtual ~RunLatency() = default;
    int64_t retrieval_ms    = 0;
    int64_t llm_ms          = 0;
    int64_t tool_calls_ms   = 0;
    int64_t total_ms        = 0;
};

struct RunMetadata {
    /**
     * @brief Run Metadata.
     * @return Return value.
     */
    virtual ~RunMetadata() = default;
    std::string mode_id;
    std::string model_id;
    std::string request_id;

    // Retrieval stats
    int   retrieved_docs   = 0;
    float avg_relevance    = 0.0f;

    // Adapter candidate selection stats (optional PR-1 path)
    int   adapter_candidates = 0;
    std::optional<std::string> selected_adapter_id;
    std::optional<std::string> adapter_selection_reason;

    // Tool usage
    std::vector<std::string> tool_calls_made;

    // Token usage
    int tokens_prompt    = 0;
    int tokens_generated = 0;

    // Latency
    RunLatency latency;

    // Quality (when judge enabled)
    std::optional<float> judge_score;

    // Extension data
    json extra;
};

struct OrchestratorResult {
    bool         success = false;
    std::string  text;          ///< Generated text
    std::string  error;         ///< Set on failure
    RunMetadata  metadata;
    json         raw_response;  ///< Full InferenceResponse as JSON
};

// ============================================================================
// Orchestrator context (per-request)
// ============================================================================

struct OrchestratorContext {
    std::string query;
    std::string mode_id;        ///< Mode to use; empty = ModePack default
    std::string request_id;

    // Pre-populated context documents (for externally supplied RAG)
    std::vector<RAGContext::Document> documents;

    // Optional overrides
    std::optional<int>   max_tokens;
    std::optional<float> temperature;
    std::optional<std::string> system_prompt;

    // Multi-agent: sender identity (for future multi-agent extension)
    std::string sender_agent_id;

    json extra;
};

// ============================================================================
// Optional adapter candidate selection (PR-1)
// ============================================================================

struct AdapterCandidate {
    std::string adapter_id;
    float       similarity = 0.0f;
    std::string source_layer;
    std::string tenant;
};

struct AdapterSelectionInput {
    std::string session_id;
    std::string tenant;
    std::vector<float> query_embedding;
    std::size_t top_k = 0;
    std::string domain_hint;
};

struct AdapterSelectionResult {
    std::optional<std::string> selected_adapter_id;
    std::vector<AdapterCandidate> candidates;
    std::string reason;
};

struct AdapterSwitchPolicy {
    int   min_switch_interval_ms = 500;
    float min_similarity_gain = 0.0f;
    int   max_switches_per_request = 1;
    int   max_retry_attempts = 0;   ///< Additional retries after first apply attempt.
    int   retry_backoff_ms = 0;     ///< Backoff between retries for retryable failures.
    bool  enable_cost_budget_gate = false; ///< Enable pre-apply cost budget guard.
    double max_total_cost = 0.0;           ///< Max projected total RAG cost.
    bool  enable_cost_top_k_adaptation = false; ///< Adapt retrieval top_k to fit budget.
    int   min_top_k_under_budget = 1;           ///< Lower bound for budget-driven top_k reduction.
};

using AdapterPathResolverFn = std::function<std::optional<std::string>(
    const std::string& adapter_id,
    const std::string& tenant)>;

class IAdapterCandidateProvider {
public:
    /**
     * @brief IAdapter Candidate Provider.
     * @return Return value.
     */
    virtual ~IAdapterCandidateProvider() = default;

    [[nodiscard]] virtual AdapterSelectionResult
    selectCandidates(const AdapterSelectionInput& input) const = 0;
};

class IAdapterApplyService {
public:
    /**
     * @brief IAdapter Apply Service.
     * @return Return value.
     */
    virtual ~IAdapterApplyService() = default;

    [[nodiscard]] virtual bool applyAdapter(const std::string& adapter_id,
                                            const std::string& tenant,
                                            float              scale) = 0;

    [[nodiscard]] virtual std::string currentAdapter() const = 0;

    [[nodiscard]] virtual bool canSwitch() const = 0;
};

struct RagCostModelInput {
    std::size_t retrieved_docs = 0;
    int64_t retrieval_latency_ms = 0;
    int64_t llm_latency_ms = 0;
    int tokens_prompt = 0;
    int tokens_generated = 0;
    bool adapter_apply_attempted = false;
    bool adapter_apply_success = false;
    int adapter_apply_attempts = 0;
    int64_t adapter_apply_latency_ms = 0;
    std::string tenant;
    json extra;
};

struct RagCostEstimate {
    double total_cost = 0.0;
    double retrieval_cost = 0.0;
    double inference_cost = 0.0;
    double adapter_cost = 0.0;
    std::string model = "";
    std::string unit = "cost_units";
    json extra;
};

class IRagCostModelService {
public:
    /**
     * @brief IRag Cost Model Service.
     * @return Return value.
     */
    virtual ~IRagCostModelService() = default;

    [[nodiscard]] virtual std::optional<RagCostEstimate>
    estimate(const RagCostModelInput& input) const = 0;
};

// ============================================================================
// AIOrchestrator
// ============================================================================

class AIOrchestrator {
public:
    /**
     * @brief AIOrchestrator.
     * @param[in] pack Input parameter.
     * @return Return value.
     */
    explicit AIOrchestrator(const ModePack& pack);
    ~AIOrchestrator();

    // No copy
    AIOrchestrator(const AIOrchestrator&) = delete;
    AIOrchestrator& operator=(const AIOrchestrator&) = delete;

    /**
     * @brief ── Configuration ────────────────────────────────────────────────────────
     * @param[in] plugin Input parameter.
     */

    void setLLMPlugin(std::shared_ptr<ILLMPlugin> plugin);

    /**
     * @brief Set Adapter Candidate Provider.
     * @param[in] provider Input parameter.
     */
    void setAdapterCandidateProvider(std::shared_ptr<IAdapterCandidateProvider> provider);

    /**
     * @brief Set Adapter Apply Service.
     * @param[in] service Input parameter.
     */
    void setAdapterApplyService(std::shared_ptr<IAdapterApplyService> service);

    /**
     * @brief Set Adapter Switch Policy.
     * @param[in] policy Input parameter.
     */
    void setAdapterSwitchPolicy(const AdapterSwitchPolicy& policy);

    /**
     * @brief Set Adapter Path Resolver.
     * @param[in] resolver Input parameter.
     */
    void setAdapterPathResolver(AdapterPathResolverFn resolver);

    /**
     * @brief Set Rag Cost Model Service.
     * @param[in] service Input parameter.
     */
    void setRagCostModelService(std::shared_ptr<IRagCostModelService> service);

    /**
     * @brief Tool Registry.
     * @return Return value.
     */
    ToolRegistry& toolRegistry();

    /**
     * @brief ── Execution ────────────────────────────────────────────────────────────
     * @param[in] ctx Input parameter.
     * @return Return value.
     */

    OrchestratorResult run(const OrchestratorContext& ctx) const;

    /**
     * @brief ── Introspection ─────────────────────────────────────────────────────────
     * @return Return value.
     */

    const ModePack& modePack() const;

    /**
     * @brief Find Mode.
     * @param[in] id Input parameter.
     * @return Pointer to the result.
     */
    const ModeSpec* findMode(const std::string& id) const;

    /**
     * @brief Default Mode.
     * @return Pointer to the result.
     */
    const ModeSpec* defaultMode() const;

    /**
     * @brief Stats.
     * @return Return value.
     */
    json stats() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;

    /**
     * @brief ── Internal pipeline steps ───────────────────────────────────────────────
     * @param[in] ctx Input parameter.
     * @param[in] mode Input parameter.
     * @return Return value.
     */

    OrchestratorResult runAsk(const OrchestratorContext& ctx,
                               const ModeSpec&            mode) const;

    /**
     * @brief Run Rag.
     * @param[in] ctx Input parameter.
     * @param[in] mode Input parameter.
     * @return Return value.
     */
    OrchestratorResult runRag(const OrchestratorContext& ctx,
                               const ModeSpec&            mode) const;

    /**
     * @brief Run Agentic.
     * @param[in] ctx Input parameter.
     * @param[in] mode Input parameter.
     * @return Return value.
     */
    OrchestratorResult runAgentic(const OrchestratorContext& ctx,
                                   const ModeSpec&            mode) const;

    /**
     * @brief Run Ethics.
     * @param[in] ctx Input parameter.
     * @param[in] mode Input parameter.
     * @return Return value.
     */
    OrchestratorResult runEthics(const OrchestratorContext& ctx,
                                  const ModeSpec&            mode) const;

    /**
     * @brief Run Multi Agent.
     * @param[in] ctx Input parameter.
     * @param[in] mode Input parameter.
     * @return Return value.
     */
    OrchestratorResult runMultiAgent(const OrchestratorContext& ctx,
                                      const ModeSpec&            mode) const;

    /**
     * @brief Build Request.
     * @param[in] ctx Input parameter.
     * @param[in] mode Input parameter.
     * @return Return value.
     */
    InferenceRequest buildRequest(const OrchestratorContext& ctx,
                                   const ModeSpec&            mode) const;

    /**
     * @brief Assemble Prompt.
     * @param[in] query Input parameter.
     * @param[in] docs Input parameter.
     * @param[in] param Input parameter.
     * @return Return value.
     */
    std::string assemblePrompt(const std::string&                         query,
                                const std::vector<RAGContext::Document>&   docs,
                                const ModeSpec&                            /*mode*/) const;

    /**
     * @brief Emit Observability.
     * @param[in] meta Input parameter.
     * @param[in] mode Input parameter.
     */
    void emitObservability(const RunMetadata& meta,
                            const ModeSpec&    mode) const;
};

// ============================================================================
// McpToolBridge – connect MCP server tools into the ToolRegistry
// ============================================================================

#ifdef THEMIS_ENABLE_MCP
// Forward-declare McpServer to avoid a circular include between
// llm/ai_orchestrator.h and server/mcp_server.h.
namespace themis::server { class McpServer; }

class McpToolBridge {
public:
    static void bridgeTools(themis::server::McpServer& mcp,
                            ToolRegistry&               registry,
                            const std::string&          prefix = "");

    static void bridgeTool(themis::server::McpServer& mcp,
                            const std::string&         tool_name,
                            ToolRegistry&               registry,
                            const std::string&          alias = "");
};
#endif // THEMIS_ENABLE_MCP

} // namespace themis::llm

