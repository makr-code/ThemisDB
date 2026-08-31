/**
 * @file ai_orchestrator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/**
 * @brief Supported LLM request mode identifiers.
 */
enum class ModeId {
    Ask,        ///< Plain question-answering (no retrieval)
    Edit,       ///< Instruction-following / text editing
    Rag,        ///< Retrieval-Augmented Generation
    Agentic,    ///< Single-agent with tool use
    MultiAgent, ///< Multi-agent message-passing (skeleton / extensible)
    Ethics,     ///< Ethics-AI plugin mode with constitutional reasoning
    Custom      ///< User-defined id
};

/** @brief Convert mode string to ModeId (case-insensitive). */
ModeId modeIdFromString(const std::string& s);
/** @brief Convert ModeId to canonical string. */
std::string modeIdToString(ModeId id);

// ----------------------------------------------------------------------------
// Sub-specs
// ----------------------------------------------------------------------------

/**
 * @brief Retrieval configuration for a mode.
 */
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

    /// Read timestamp semantics for shard-local consistency
    std::string read_ts_semantics = "latest"; ///< "latest", "snapshot:<ts>"
    /// Optional cluster/shard locality hint
    std::string locality;
};

/**
 * @brief Output constraints for a mode.
 */
struct OutputSpec {
    std::string format = "text";     ///< "text", "json", "markdown"
    std::optional<std::string> json_schema; ///< JSON Schema for structured output
    std::optional<std::string> grammar;     ///< EBNF grammar name or inline grammar
};

/**
 * @brief Resource budgets for a mode.
 */
struct BudgetSpec {
    int    max_tokens   = 512;
    int    timeout_ms   = 30000;
    int    max_retries  = 1;
    float  temperature  = 0.7f;
    float  top_p        = 0.9f;
    int    top_k        = 40;
};

/**
 * @brief Observability configuration for a mode.
 */
struct ObservabilitySpec {
    bool log_requests   = true;
    bool log_responses  = false;    ///< disabled by default (privacy)
    bool metrics        = true;     ///< Prometheus metrics
    bool trace          = false;    ///< OpenTelemetry tracing
};

/**
 * @brief MCP-style tool specification.
 */
struct ToolSpec {
    std::string name;
    std::string description;
    json        args_schema;    ///< JSON Schema for arguments
    int         timeout_ms = 5000;
};

/**
 * @brief A single mode specification.
 */
struct ModeSpec {
    std::string      id;          ///< Unique mode identifier, e.g. "rag"
    ModeId           mode_id = ModeId::Custom;
    std::string      description;
    std::string      model_id = "default";
    std::string      lora_adapter_id;

    /// Allowed tool names (empty = no tools).
    std::vector<std::string> tools_allowed;
    /// Explicitly blocked tools (overrides allowlist).
    std::vector<std::string> tools_denied;

    RetrievalSpec   retrieval;
    OutputSpec      output;
    BudgetSpec      budgets;
    ObservabilitySpec observability;

    /// Optional judge configuration for quality evaluation
    struct JudgeSpec {
        bool        enabled = false;
        std::string model_id;
        float       min_score = 0.6f;
    } judge;

    /// Optional safety/ethics guardrails
    struct SafetySpec {
        bool        enabled = false;
        std::string ethics_profile; ///< path or id of ethics YAML profile
    } safety;

    /// Arbitrary extension fields preserved from YAML
    json extensions;
};

/**
 * @brief Model entry in a ModePack.
 */
struct ModelEntry {
    virtual ~ModelEntry() = default;
    std::string id;
    std::string path;
    int         gpu_layers = 0;
    int         n_ctx      = 4096;
};

/**
 * @brief Top-level container parsed from a ThemisModePack YAML file.
 */
struct ModePack {
    std::string apiVersion; ///< Expected: "themis.ai/v1"
    std::string kind;       ///< Expected: "ThemisModePack" or "ThemisAIPolicy"
    std::string name;
    std::string version = "1.0.0";

    std::vector<ModelEntry> models;
    std::vector<ToolSpec>   tools;
    std::vector<ModeSpec>   modes;

    /// Default mode id when none specified in a request
    std::string default_mode = "ask";
};

// ============================================================================
// Mode Spec Loader + Validator
// ============================================================================

/**
 * @brief Result of a validation operation.
 */
struct ValidationResult {
    bool                     ok = true;
    std::vector<std::string> errors;
    std::vector<std::string> warnings;

    explicit operator bool() const { return ok; }
};

/**
 * @brief Loads and validates ThemisModePack YAML files.
 *
 * Supports:
 *   - apiVersion / kind checks
 *   - Default injection
 *   - Clear error messages referencing YAML key paths
 */
class ModeSpecLoader {
public:
    /**
     * @brief Load a ModePack from a YAML file.
     * @param path       Filesystem path to the YAML file.
     * @param result_out Optional validation result output (errors/warnings).
     * @return Loaded pack; empty on failure (check result_out).
     * @throws std::runtime_error when yaml-cpp cannot open the file.
     */
    static ModePack loadFromFile(const std::string& path,
                                 ValidationResult*  result_out = nullptr);

    /**
     * @brief Load a ModePack from an in-memory YAML string.
     * @param yaml_text  YAML content.
     * @param result_out Optional validation result output.
     * @return Loaded pack; empty on failure.
     */
    static ModePack loadFromString(const std::string& yaml_text,
                                   ValidationResult*  result_out = nullptr);

    /**
     * @brief Validate an already-loaded ModePack.
     * @return ValidationResult with errors/warnings.
     */
    static ValidationResult validate(const ModePack& pack);
};

// ============================================================================
// Tool Registry
// ============================================================================

/**
 * @brief Callable handler for a registered tool.
 *
 * @param args   Parsed JSON arguments (validated against ToolSpec::args_schema).
 * @param mode   Mode that invoked the tool (for permission checks).
 * @return JSON result that will be injected into the prompt context.
 */
using ToolHandler = std::function<json(const json& args, const ModeSpec& mode)>;

/**
 * @brief MCP-style tool registry with mode-based permission checking.
 *
 * Tools are registered globally; each ModeSpec's tools_allowed/tools_denied
 * lists are consulted before dispatch.
 *
 * ## Dynamic tool loading (DLL/SO)
 *
 * Tool plugins are shared libraries that export the standard
 * `createPlugin()`/`destroyPlugin()` C ABI and contain a class derived from
 * IThemisTool.  The plugin.json manifest must set `"type": "agentic_tool"`.
 *
 * Example workflow:
 * @code
 * ToolRegistry registry;
 * registry.loadToolsFromDirectory("plugins/tools");   // bulk load
 * registry.loadToolPlugin("plugins/tools/libtool_search.so"); // single load
 * registry.reloadTool("search_vector");               // hot-reload
 * registry.unloadTool("search_vector");               // unload + deregister
 * @endcode
 *
 * Statically registered tools (via registerTool()) and dynamically loaded
 * tools coexist in the same registry and are dispatched through the same
 * invokeTool() path.
 */
class ToolRegistry {
public:
    ToolRegistry();
    ~ToolRegistry();

    // ── Static / built-in tool registration ──────────────────────────────────

    /** @brief Register a tool handler. Overwrites any existing static registration. */
    void registerTool(const ToolSpec& spec, ToolHandler handler);

    // ── Dynamic tool loading via PluginManager ────────────────────────────────

    /**
     * @brief Load a single tool plugin from a shared library path.
     *
     * The library must export `createPlugin()`/`destroyPlugin()` and the
     * returned instance must implement IThemisTool.  On success the tool is
     * automatically registered in the registry using the name reported by
     * IThemisPlugin::getName().
     *
     * @param path    Absolute path to the .so / .dll.
     * @param config  Optional JSON configuration string passed to initialize().
     * @return Ok(void) on success, Err(ERR_TOOL_PLUGIN_NOT_A_TOOL) if the
     *         plugin does not implement IThemisTool, or a plugin-layer error
     *         from PluginManager on load failure.
     */
    Result<void> loadToolPlugin(const std::string& path,
                                const std::string& config = "{}");

    /**
     * @brief Scan a directory for tool plugins and load all AGENTIC_TOOL ones.
     *
     * Delegates directory scanning to PluginManager::scanPluginDirectory()
     * followed by loading every plugin with type == AGENTIC_TOOL.
     *
     * @param directory  Path to directory containing .so/.dll files.
     * @return Ok(n) where n is the number of tools successfully loaded.
     */
    Result<size_t> loadToolsFromDirectory(const std::string& directory);

    /**
     * @brief Hot-reload a named tool plugin without stopping the registry.
     *
     * Delegates to PluginManager::reloadPlugin() (atomic swap with rollback).
     * After the reload the tool handler is re-registered with the new instance.
     * Statically registered tools cannot be reloaded via this method.
     *
     * @param name  Tool name as reported by IThemisPlugin::getName().
     * @return Ok(void) on success, Err(ERR_TOOL_NOT_FOUND) if not a plugin tool.
     */
    Result<void> reloadTool(const std::string& name);

    /**
     * @brief Unload a dynamically loaded tool plugin and deregister it.
     *
     * Has no effect on statically registered tools.
     *
     * @param name  Tool name.
     * @return Ok(void) on success, or a plugin-layer error on unload failure.
     */
    Result<void> unloadTool(const std::string& name);

    // ── Dispatch ──────────────────────────────────────────────────────────────

    /** @brief Invoke a tool if permitted by mode's allowlist/denylist. */
    json invokeTool(const std::string& tool_name,
                    const json&        args,
                    const ModeSpec&    mode) const;

    /** @brief Check if a tool is permitted for a given mode. */
    bool isAllowed(const std::string& tool_name,
                   const ModeSpec&    mode) const;

    /** @brief List all registered tool names (static + dynamic). */
    std::vector<std::string> listTools() const;

    /** @brief Get spec for a named tool; nullopt if not found. */
    std::optional<ToolSpec> getSpec(const std::string& tool_name) const;

    /** @brief Returns true if the named tool was loaded from a plugin DLL. */
    bool isPluginTool(const std::string& name) const;

private:
    struct Entry {
        ToolSpec    spec;
        ToolHandler handler;
        bool        is_plugin = false;  ///< true when backed by a DLL plugin
    };

    /// Register a plugin-backed IThemisTool instance into tools_.
    void registerPluginTool(IThemisTool* tool);

    std::unordered_map<std::string, Entry>      tools_;
    mutable std::shared_mutex                   tools_mutex_;   ///< guards tools_
    std::unique_ptr<plugins::PluginManager>     plugin_manager_; ///< DLL lifecycle
};

// ============================================================================
// Orchestrator – run metadata and result
// ============================================================================

/**
 * @brief Latency breakdown for a single orchestrator run.
 */
struct RunLatency {
    virtual ~RunLatency() = default;
    int64_t retrieval_ms    = 0;
    int64_t llm_ms          = 0;
    int64_t tool_calls_ms   = 0;
    int64_t total_ms        = 0;
};

/**
 * @brief Run metadata emitted for every orchestrator execution.
 */
struct RunMetadata {
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

/**
 * @brief Result of a single orchestrator run.
 */
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

/**
 * @brief Input context for an orchestrator run.
 */
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

/**
 * @brief One adapter candidate returned by the selection provider.
 */
struct AdapterCandidate {
    std::string adapter_id;
    float       similarity = 0.0f;
    std::string source_layer;
    std::string tenant;
};

/**
 * @brief Input for adapter candidate selection.
 */
struct AdapterSelectionInput {
    std::string session_id;
    std::string tenant;
    std::vector<float> query_embedding;
    std::size_t top_k = 0;
    std::string domain_hint;
};

/**
 * @brief Output of adapter candidate selection.
 */
struct AdapterSelectionResult {
    std::optional<std::string> selected_adapter_id;
    std::vector<AdapterCandidate> candidates;
    std::string reason;
};

/**
 * @brief Policy guardrails for runtime adapter switching.
 */
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

/**
 * @brief Resolve adapter id + tenant to a concrete LoRA artifact path.
 *
 * Return std::nullopt or an empty string to signal that the adapter cannot
 * be resolved for the current request context.
 */
using AdapterPathResolverFn = std::function<std::optional<std::string>(
    const std::string& adapter_id,
    const std::string& tenant)>;

/**
 * @brief Optional runtime provider for adapter candidate selection.
 */
class IAdapterCandidateProvider {
public:
    virtual ~IAdapterCandidateProvider() = default;

    /**
     * @brief Select adapter candidates for the current request context.
     */
    [[nodiscard]] virtual AdapterSelectionResult
    selectCandidates(const AdapterSelectionInput& input) const = 0;
};

/**
 * @brief Optional runtime service to apply adapters before generation.
 */
class IAdapterApplyService {
public:
    virtual ~IAdapterApplyService() = default;

    /**
     * @brief Apply an adapter for the current runtime context.
     */
    [[nodiscard]] virtual bool applyAdapter(const std::string& adapter_id,
                                            const std::string& tenant,
                                            float              scale) = 0;

    /**
     * @brief Return current adapter id, or empty string if none is active.
     */
    [[nodiscard]] virtual std::string currentAdapter() const = 0;

    /**
     * @brief Whether switching is currently allowed.
     */
    [[nodiscard]] virtual bool canSwitch() const = 0;
};

/**
 * @brief Cost model input for one RAG orchestration run.
 */
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

/**
 * @brief Cost model output for one RAG orchestration run.
 */
struct RagCostEstimate {
    double total_cost = 0.0;
    double retrieval_cost = 0.0;
    double inference_cost = 0.0;
    double adapter_cost = 0.0;
    std::string model = "";
    std::string unit = "cost_units";
    json extra;
};

/**
 * @brief Optional runtime service to estimate end-to-end RAG cost.
 */
class IRagCostModelService {
public:
    virtual ~IRagCostModelService() = default;

    /**
     * @brief Estimate total RAG cost for a single run.
     * @return Cost estimate, or nullopt when no estimate can be produced.
     */
    [[nodiscard]] virtual std::optional<RagCostEstimate>
    estimate(const RagCostModelInput& input) const = 0;
};

// ============================================================================
// AIOrchestrator
// ============================================================================

/**
 * @brief Central LLM orchestration runtime.
 *
 * Loads a ModePack at startup and executes pipeline runs based on the
 * selected mode.  Supports "ask" and "rag" end-to-end; "agentic" / "ethics"
 * / "multi_agent" are scaffolded with extension points.
 *
 * Thread-safe: multiple threads may call run() concurrently.
 *
 * Usage:
 * @code
 *   auto pack = ModeSpecLoader::loadFromFile("config/ai_ml/llm/modes/default.yaml");
 *   AIOrchestrator orch(pack);
 *   orch.setLLMPlugin(my_plugin);
 *
 *   OrchestratorContext ctx;
 *   ctx.query   = "How do I configure sharding?";
 *   ctx.mode_id = "rag";
 *   auto result = orch.run(ctx);
 * @endcode
 */
class AIOrchestrator {
public:
    explicit AIOrchestrator(const ModePack& pack);
    ~AIOrchestrator();

    // No copy
    AIOrchestrator(const AIOrchestrator&) = delete;
    AIOrchestrator& operator=(const AIOrchestrator&) = delete;

    // ── Configuration ────────────────────────────────────────────────────────

    /** @brief Set (or replace) the LLM plugin used for inference. */
    void setLLMPlugin(std::shared_ptr<ILLMPlugin> plugin);

    /** @brief Set (or clear) the optional adapter candidate provider. */
    void setAdapterCandidateProvider(std::shared_ptr<IAdapterCandidateProvider> provider);

    /** @brief Set (or clear) the optional adapter apply service. */
    void setAdapterApplyService(std::shared_ptr<IAdapterApplyService> service);

    /** @brief Configure adapter switch policy guardrails. */
    void setAdapterSwitchPolicy(const AdapterSwitchPolicy& policy);

    /**
     * @brief Set (or clear) adapter path resolver used by the default apply bridge.
     *
     * @param resolver Resolver callback. Pass empty function to clear resolver.
     *        If resolver is set and returns empty/nullopt, adapter apply fails
     *        with metadata error code "resolver_empty_path".
     */
    void setAdapterPathResolver(AdapterPathResolverFn resolver);

    /** @brief Set (or clear) RAG cost model service. */
    void setRagCostModelService(std::shared_ptr<IRagCostModelService> service);

    /** @brief Expose the internal tool registry for external registrations. */
    ToolRegistry& toolRegistry();

    // ── Execution ────────────────────────────────────────────────────────────

    /**
     * @brief Execute a pipeline run for the given context.
     *
     * Selects the mode, validates permissions, executes retrieval (if needed),
     * assembles the prompt, calls the LLM, records observability data, and
     * returns the result.
     *
     * @param ctx     Request context.
     * @return OrchestratorResult with text and metadata.
     */
    OrchestratorResult run(const OrchestratorContext& ctx) const;

    // ── Introspection ─────────────────────────────────────────────────────────

    /** @brief Return the loaded ModePack. */
    const ModePack& modePack() const;

    /** @brief Find a mode spec by id; nullptr if not found. */
    const ModeSpec* findMode(const std::string& id) const;

    /** @brief Return the default ModeSpec (ModePack::default_mode). */
    const ModeSpec* defaultMode() const;

    /** @brief Return last-run statistics as JSON. */
    json stats() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;

    // ── Internal pipeline steps ───────────────────────────────────────────────

    OrchestratorResult runAsk(const OrchestratorContext& ctx,
                               const ModeSpec&            mode) const;

    OrchestratorResult runRag(const OrchestratorContext& ctx,
                               const ModeSpec&            mode) const;

    OrchestratorResult runAgentic(const OrchestratorContext& ctx,
                                   const ModeSpec&            mode) const;

    OrchestratorResult runEthics(const OrchestratorContext& ctx,
                                  const ModeSpec&            mode) const;

    OrchestratorResult runMultiAgent(const OrchestratorContext& ctx,
                                      const ModeSpec&            mode) const;

    /// Build InferenceRequest from mode spec and context
    InferenceRequest buildRequest(const OrchestratorContext& ctx,
                                   const ModeSpec&            mode) const;

    /// Assemble prompt with optional RAG context
    std::string assemblePrompt(const std::string&                         query,
                                const std::vector<RAGContext::Document>&   docs,
                                const ModeSpec&                            /*mode*/) const;

    /// Emit run metadata to logging / metrics
    void emitObservability(const RunMetadata& meta,
                            const ModeSpec&    mode) const;
};

// ============================================================================
// McpToolBridge – connect MCP server tools into the ToolRegistry
// ============================================================================

/**
 * @brief Utility that bridges McpServer-registered tools into an AIOrchestrator
 *        ToolRegistry.
 *
 * When the MCP server and the AI Orchestrator run together, tools registered in
 * the MCP server can be forwarded to the orchestrator's ToolRegistry so that
 * mode pipelines (e.g. "rag", "agentic") can invoke them without duplicating
 * registration logic.
 *
 * Usage:
 * @code
 * #include "llm/ai_orchestrator.h"
 * #include "server/mcp_server.h"
 *
 * AIOrchestrator orch(pack);
 * McpServer mcp(io);
 *
 * // Wire orchestrator into MCP (MCP exposes modes as tools)
 * mcp.attachOrchestrator(std::make_shared<AIOrchestrator>(pack));
 *
 * // Wire MCP tools into orchestrator (orchestrator can call MCP tools)
 * McpToolBridge::bridgeTools(mcp, orch.toolRegistry());
 * @endcode
 *
 * Only available when THEMIS_ENABLE_MCP is defined.
 */
#ifdef THEMIS_ENABLE_MCP
// Forward-declare McpServer to avoid a circular include between
// llm/ai_orchestrator.h and server/mcp_server.h.
namespace themis::server { class McpServer; }

class McpToolBridge {
public:
    /**
     * @brief Import all tools currently registered in @p mcp into @p registry.
     *
     * For each tool found in the MCP server, a ToolSpec is created (using the
     * tool's description and schema) and a handler is registered that forwards
     * calls to @p mcp.handleRequest() with a JSON-RPC "tools/call" envelope.
     *
     * Existing entries in @p registry with the same name are overwritten.
     *
     * @param mcp      Reference to the McpServer whose tools to import.
     * @param registry ToolRegistry to populate.
     * @param prefix   Optional name prefix added to every imported tool name,
     *                 e.g. "mcp_" → "mcp_docs_search".  Default: no prefix.
     */
    static void bridgeTools(themis::server::McpServer& mcp,
                            ToolRegistry&               registry,
                            const std::string&          prefix = "");

    /**
     * @brief Import a single named MCP tool into the registry.
     *
     * @param mcp        Reference to the McpServer.
     * @param tool_name  Name of the tool inside the MCP server.
     * @param registry   ToolRegistry to populate.
     * @param alias      Name to use in the registry; defaults to @p tool_name.
     */
    static void bridgeTool(themis::server::McpServer& mcp,
                            const std::string&         tool_name,
                            ToolRegistry&               registry,
                            const std::string&          alias = "");
};
#endif // THEMIS_ENABLE_MCP

} // namespace themis::llm

