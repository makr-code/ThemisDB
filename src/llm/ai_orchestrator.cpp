/*
 * ThemisDB | File: ai_orchestrator.cpp | Version: 0.0.15 | Last Modified: 2026-05-31 12:17:24
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 700
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=4, H=11, M=3, L=0
 * PR History (last 5): #4332 Implement AIOrchestrator to... (2026-03-19) | #2590 feat: YAML-configurable LLM... (2026-03-11)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

/**
 * @file ai_orchestrator.cpp
 * @brief LLM Orchestration runtime for ThemisDB.
 *
 * Executes LLM pipelines according to the loaded ThemisModePack spec.
 * Supports modes: ask, edit, rag, agentic, multi_agent, ethics.
 *
 * The "ask" and "rag" pipelines are fully functional end-to-end.
 * "agentic", "multi_agent", and "ethics" provide extensible scaffolding.
 */

#include "llm/ai_orchestrator.h"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <shared_mutex>
#include <sstream>
#include <stdexcept>

namespace themis::llm {

// ============================================================================
// ToolRegistry
// ============================================================================

void ToolRegistry::registerTool(const ToolSpec& spec, ToolHandler handler) {
    if (spec.name.empty()) {
        throw std::invalid_argument("ToolRegistry: tool name must not be empty");
    }
    std::unique_lock lock(tools_mutex_);
    tools_[spec.name] = {spec, std::move(handler), /*is_plugin=*/false};
    spdlog::debug("[ToolRegistry] Registered tool '{}'", spec.name);
}

bool ToolRegistry::isAllowed(const std::string& tool_name,
                              const ModeSpec&    mode) const {
    // Check deny list first (takes precedence)
    for (const auto& d : mode.tools_denied) {
        if (d == tool_name) return false;
    }
    // Empty allowlist = no tools permitted
    if (mode.tools_allowed.empty()) return false;
    // Wildcard "*" allows everything not denied
    for (const auto& a : mode.tools_allowed) {
        if (a == "*" || a == tool_name) return true;
    }
    return false;
}

json ToolRegistry::invokeTool(const std::string& tool_name,
                               const json&        args,
                               const ModeSpec&    mode) const {
    if (!isAllowed(tool_name, mode)) {
        spdlog::warn("[ToolRegistry] Tool '{}' not permitted for mode '{}'",
                     tool_name, mode.id);
        return {{"error", "tool '" + tool_name + "' is not permitted for mode '" + mode.id + "'"}};
    }
    // Take a shared (read) lock to lookup and *copy* the handler safely.
    // Copying std::function is safe while holding a shared lock because no
    // writer can modify tools_ concurrently.  The handler is then invoked
    // *outside* the lock so slow tools don't block concurrent registrations
    // or other read operations.
    ToolHandler handler_copy;
    {
        std::shared_lock lock(tools_mutex_);
        auto it = tools_.find(tool_name);
        if (it == tools_.end()) {
            spdlog::warn("[ToolRegistry] Tool '{}' is not registered", tool_name);
            return {{"error", "tool '" + tool_name + "' is not registered"}};
        }
        handler_copy = it->second.handler;
    }
    try {
        return handler_copy(args, mode);
    } catch (const std::exception& e) {
        spdlog::error("[ToolRegistry] Tool '{}' threw: {}", tool_name, e.what());
        return {{"error", std::string("tool execution failed: ") + e.what()}};
    }
}

std::vector<std::string> ToolRegistry::listTools() const {
    std::shared_lock lock(tools_mutex_);
    std::vector<std::string> names;
    names.reserve(tools_.size());
    for (const auto& [name, _] : tools_) {
        names.push_back(name);
    }
    return names;
}

std::optional<ToolSpec> ToolRegistry::getSpec(const std::string& tool_name) const {
    std::shared_lock lock(tools_mutex_);
    auto it = tools_.find(tool_name);
    if (it == tools_.end()) return std::nullopt;
    return it->second.spec;
}

// ── Constructor / Destructor ─────────────────────────────────────────────────

ToolRegistry::ToolRegistry()
    : plugin_manager_(std::make_unique<plugins::PluginManager>()) {}

ToolRegistry::~ToolRegistry() = default;

// ── Private helper ────────────────────────────────────────────────────────────

void ToolRegistry::registerPluginTool(IThemisTool* tool) {
    ToolSpec spec;
    spec.name        = tool->getName();
    spec.description = tool->getVersion();  // version as a lightweight description proxy
    spec.args_schema = tool->inputSchema();

    // Capture a non-owning raw pointer — lifetime is managed by PluginManager.
    ToolHandler handler = [tool](const json& args, const ModeSpec& /*mode*/) -> json {
        return tool->execute(args);
    };

    std::unique_lock lock(tools_mutex_);
    tools_[spec.name] = {std::move(spec), std::move(handler), /*is_plugin=*/true};
    spdlog::info("[ToolRegistry] Plugin tool '{}' registered", tool->getName());
}

// ── Dynamic loading ────────────────────────────────────────────────────────────

Result<void> ToolRegistry::loadToolPlugin(const std::string& path,
                                           const std::string& config) {
    auto result = plugin_manager_->loadPluginFromPath(path, config);
    if (!result) {
        return tl::unexpected(result.error());
    }

    auto* base = result.value();
    auto* tool = dynamic_cast<IThemisTool*>(base);
    if (!tool) {
        // Not an IThemisTool — unload and report
        (void)plugin_manager_->unloadPlugin(base->getName());
        return ErrVoid(errors::ErrorCode::ERR_TOOL_PLUGIN_NOT_A_TOOL,
                       "plugin at '" + path + "' does not implement IThemisTool");
    }

    registerPluginTool(tool);
    return OkVoid();
}

Result<size_t> ToolRegistry::loadToolsFromDirectory(const std::string& directory) {
    auto scan = plugin_manager_->scanPluginDirectory(directory);
    if (!scan) {
        return tl::unexpected(scan.error());
    }

    // Load every manifest with type == AGENTIC_TOOL
    auto manifests = plugin_manager_->listPlugins();
    size_t loaded = 0;
    for (const auto& manifest : manifests) {
        if (manifest.type != plugins::PluginType::AGENTIC_TOOL) continue;
        if (plugin_manager_->isPluginLoaded(manifest.name)) continue;

        auto res = plugin_manager_->loadPlugin(manifest.name);
        if (!res) {
            spdlog::warn("[ToolRegistry] Failed to load tool plugin '{}': {}",
                         manifest.name, res.error().message());
            continue;
        }

        auto* tool = dynamic_cast<IThemisTool*>(res.value());
        if (!tool) {
            spdlog::warn("[ToolRegistry] Plugin '{}' is not an IThemisTool — skipped",
                         manifest.name);
            (void)plugin_manager_->unloadPlugin(manifest.name);
            continue;
        }

        registerPluginTool(tool);
        ++loaded;
    }

    return loaded;
}

Result<void> ToolRegistry::reloadTool(const std::string& name) {
    // Verify it is a plugin-backed tool
    {
        std::shared_lock lock(tools_mutex_);
        auto it = tools_.find(name);
        if (it == tools_.end() || !it->second.is_plugin) {
            return ErrVoid(errors::ErrorCode::ERR_TOOL_NOT_FOUND,
                           "tool '" + name + "' is not a loaded plugin tool");
        }
    }

    auto res = plugin_manager_->reloadPlugin(name);
    if (!res) return tl::unexpected(res.error());

    // Re-register with the freshly loaded instance
    auto get = plugin_manager_->getPlugin(name);
    if (!get) return tl::unexpected(get.error());

    auto* tool = dynamic_cast<IThemisTool*>(get.value());
    if (!tool) {
        return ErrVoid(errors::ErrorCode::ERR_TOOL_PLUGIN_NOT_A_TOOL,
                       "reloaded plugin '" + name + "' no longer implements IThemisTool");
    }

    registerPluginTool(tool);
    spdlog::info("[ToolRegistry] Tool '{}' hot-reloaded", name);
    return OkVoid();
}

Result<void> ToolRegistry::unloadTool(const std::string& name) {
    {
        std::unique_lock lock(tools_mutex_);
        auto it = tools_.find(name);
        if (it != tools_.end() && it->second.is_plugin) {
            tools_.erase(it);
        }
    }

    if (plugin_manager_->isPluginLoaded(name)) {
        return plugin_manager_->unloadPlugin(name);
    }
    return OkVoid();
}

bool ToolRegistry::isPluginTool(const std::string& name) const {
    std::shared_lock lock(tools_mutex_);
    auto it = tools_.find(name);
    return it != tools_.end() && it->second.is_plugin;
}

// ============================================================================
// AIOrchestrator::Impl
// ============================================================================

struct AIOrchestrator::Impl {
    ModePack pack;
    std::shared_ptr<ILLMPlugin> plugin;
    ToolRegistry tool_registry;
    mutable std::atomic<int64_t> total_runs{0};
    mutable std::atomic<int64_t> total_errors{0};
    mutable std::atomic<int64_t> total_tokens{0};
};

// ============================================================================
// AIOrchestrator
// ============================================================================

AIOrchestrator::AIOrchestrator(const ModePack& pack)
    : impl_(std::make_unique<Impl>()) {
    impl_->pack = pack;
    spdlog::info("[AIOrchestrator] Initialized with {} mode(s), default='{}'",
                 pack.modes.size(), pack.default_mode);
}

AIOrchestrator::~AIOrchestrator() = default;

void AIOrchestrator::setLLMPlugin(std::shared_ptr<ILLMPlugin> plugin) {
    impl_->plugin = std::move(plugin);
}

ToolRegistry& AIOrchestrator::toolRegistry() {
    return impl_->tool_registry;
}

const ModePack& AIOrchestrator::modePack() const {
    return impl_->pack;
}

const ModeSpec* AIOrchestrator::findMode(const std::string& id) const {
    for (const auto& m : impl_->pack.modes) {
        if (m.id == id) return &m;
    }
    return nullptr;
}

const ModeSpec* AIOrchestrator::defaultMode() const {
    return findMode(impl_->pack.default_mode);
}

json AIOrchestrator::stats() const {
    return {
        {"total_runs",   impl_->total_runs.load()},
        {"total_errors", impl_->total_errors.load()},
        {"total_tokens", impl_->total_tokens.load()},
    };
}

// ============================================================================
// Main run() dispatcher
// ============================================================================

OrchestratorResult AIOrchestrator::run(const OrchestratorContext& ctx) const {
    ++impl_->total_runs;

    // Resolve mode
    const std::string mode_id = ctx.mode_id.empty() ? impl_->pack.default_mode : ctx.mode_id;
    const ModeSpec* mode_ptr = findMode(mode_id);
    if (!mode_ptr) {
        ++impl_->total_errors;
        OrchestratorResult err;
        err.success = false;
        err.error   = "Unknown mode '" + mode_id + "'. Available: ";
        for (const auto& m : impl_->pack.modes) err.error += m.id + " ";
        spdlog::error("[AIOrchestrator] {}", err.error);
        return err;
    }
    const ModeSpec& mode = *mode_ptr;

    spdlog::debug("[AIOrchestrator] run() mode='{}' query_len={}", mode.id, ctx.query.size());

    OrchestratorResult result;
    try {
        switch (mode.mode_id) {
            case ModeId::Ask:
                result = runAsk(ctx, mode);
                break;
            case ModeId::Edit:
                // Edit mode uses the same pipeline as Ask with a different system prompt
                result = runAsk(ctx, mode);
                break;
            case ModeId::Rag:
                result = runRag(ctx, mode);
                break;
            case ModeId::Agentic:
                result = runAgentic(ctx, mode);
                break;
            case ModeId::MultiAgent:
                result = runMultiAgent(ctx, mode);
                break;
            case ModeId::Ethics:
                result = runEthics(ctx, mode);
                break;
            default:
                // Custom mode: default to ask pipeline
                result = runAsk(ctx, mode);
                break;
        }
    } catch (const std::exception& e) {
        ++impl_->total_errors;
        result.success = false;
        result.error   = std::string("Orchestrator exception: ") + e.what();
        spdlog::error("[AIOrchestrator] {}", result.error);
        return result;
    }

    // Record token usage
    impl_->total_tokens += result.metadata.tokens_generated;

    // Observability
    emitObservability(result.metadata, mode);

    return result;
}

// ============================================================================
// Pipeline helpers
// ============================================================================

InferenceRequest AIOrchestrator::buildRequest(const OrchestratorContext& ctx,
                                               const ModeSpec&            mode) const {
    InferenceRequest req;
    req.model_id    = mode.model_id;
    req.request_id  = ctx.request_id;
    req.max_tokens  = ctx.max_tokens.value_or(mode.budgets.max_tokens);
    req.temperature = ctx.temperature.value_or(mode.budgets.temperature);
    req.top_p       = mode.budgets.top_p;
    req.top_k       = mode.budgets.top_k;

    if (ctx.system_prompt.has_value()) {
        req.system_prompt = ctx.system_prompt;
    }

    if (!mode.lora_adapter_id.empty()) {
        req.lora_adapter_id = mode.lora_adapter_id;
    }

    // Output constraints
    if (mode.output.grammar.has_value()) {
        req.grammar_type = mode.output.grammar;
    }

    return req;
}

std::string AIOrchestrator::assemblePrompt(
        const std::string&                         query,
        const std::vector<RAGContext::Document>&   docs,
        const ModeSpec&                            mode) const {
    if (docs.empty()) {
        return query;
    }

    std::ostringstream ss;
    ss << "Context:\n";
    int idx = 1;
    for (const auto& doc : docs) {
        ss << "[" << idx++ << "] (source: " << doc.source
           << ", relevance: " << doc.relevance_score << ")\n"
           << doc.content << "\n\n";
    }
    ss << "Question: " << query << "\n\n"
       << "Answer based on the context provided above:";
    return ss.str();
}

void AIOrchestrator::emitObservability(const RunMetadata& meta,
                                        const ModeSpec&    mode) const {
    if (!mode.observability.log_requests) return;

    spdlog::info("[AIOrchestrator] run completed: mode={} model={} "
                 "tokens_in={} tokens_out={} latency_total_ms={} "
                 "retrieved_docs={} tool_calls={}",
                 meta.mode_id,
                 meta.model_id,
                 meta.tokens_prompt,
                 meta.tokens_generated,
                 meta.latency.total_ms,
                 meta.retrieved_docs,
                 meta.tool_calls_made.size());
}

// ============================================================================
// Ask pipeline
// ============================================================================

OrchestratorResult AIOrchestrator::runAsk(const OrchestratorContext& ctx,
                                           const ModeSpec&            mode) const {
    OrchestratorResult result;
    result.metadata.mode_id    = mode.id;
    result.metadata.model_id   = mode.model_id;
    result.metadata.request_id = ctx.request_id;

    auto t_start = std::chrono::steady_clock::now();

    if (!impl_->plugin) {
        // No plugin: return query echoed (useful for testing)
        result.success           = true;
        result.text              = "[LLM not configured] Query received: " + ctx.query;
        result.metadata.tokens_generated = 0;

        auto t_end = std::chrono::steady_clock::now();
        result.metadata.latency.llm_ms =
            std::chrono::duration_cast<std::chrono::milliseconds>(t_end - t_start).count();
        result.metadata.latency.total_ms = result.metadata.latency.llm_ms;
        return result;
    }

    InferenceRequest req  = buildRequest(ctx, mode);
    req.prompt            = assemblePrompt(ctx.query, {}, mode);

    auto t_llm = std::chrono::steady_clock::now();
    InferenceResponse resp = impl_->plugin->generate(req);
    auto t_llm_end = std::chrono::steady_clock::now();

    result.success                    = true;
    result.text                       = resp.text;
    result.metadata.tokens_prompt     = resp.tokens_prompt;
    result.metadata.tokens_generated  = resp.tokens_generated;
    result.metadata.latency.llm_ms    =
        std::chrono::duration_cast<std::chrono::milliseconds>(t_llm_end - t_llm).count();
    result.metadata.latency.total_ms  = result.metadata.latency.llm_ms;

    result.raw_response = {
        {"text",             resp.text},
        {"tokens_prompt",    resp.tokens_prompt},
        {"tokens_generated", resp.tokens_generated},
        {"model_id",         resp.model_id},
        {"cache_hit",        resp.cache_hit},
    };

    return result;
}

// ============================================================================
// RAG pipeline
// ============================================================================

OrchestratorResult AIOrchestrator::runRag(const OrchestratorContext& ctx,
                                           const ModeSpec&            mode) const {
    OrchestratorResult result;
    result.metadata.mode_id    = mode.id;
    result.metadata.model_id   = mode.model_id;
    result.metadata.request_id = ctx.request_id;

    auto t_start = std::chrono::steady_clock::now();

    // Step 1: Retrieval
    // Use pre-populated documents from context when available.
    // In production, a real retrieval backend would be called here.
    std::vector<RAGContext::Document> docs = ctx.documents;

    // If retrieval is enabled but no docs pre-supplied, invoke the docs_search tool
    if (mode.retrieval.enabled && docs.empty()) {
        const std::string search_tool = "docs_search";
        if (impl_->tool_registry.isAllowed(search_tool, mode)) {
            json args = {{"query", ctx.query}, {"top_k", mode.retrieval.top_k}};
            auto t_ret = std::chrono::steady_clock::now();
            json tool_result = impl_->tool_registry.invokeTool(search_tool, args, mode);
            auto t_ret_end = std::chrono::steady_clock::now();

            result.metadata.tool_calls_made.push_back(search_tool);
            result.metadata.latency.retrieval_ms =
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    t_ret_end - t_ret).count();

            // Parse tool results into documents
            if (tool_result.contains("documents") && tool_result["documents"].is_array()) {
                for (const auto& d : tool_result["documents"]) {
                    RAGContext::Document doc;
                    doc.content         = d.value("content", "");
                    doc.source          = d.value("source",  "");
                    doc.relevance_score = d.value("relevance_score", 0.0f);
                    docs.push_back(std::move(doc));
                }
            }
        }
    }

    // Apply threshold and top-k filter
    if (mode.retrieval.enabled) {
        // Filter by threshold
        docs.erase(std::remove_if(docs.begin(), docs.end(),
            [&](const RAGContext::Document& d) {
                return d.relevance_score < mode.retrieval.threshold;
            }), docs.end());

        // Sort by relevance (descending) and truncate to top_k
        std::sort(docs.begin(), docs.end(),
            [](const RAGContext::Document& a, const RAGContext::Document& b) {
                return a.relevance_score > b.relevance_score;
            });
        if (static_cast<int>(docs.size()) > mode.retrieval.top_k) {
            docs.resize(static_cast<size_t>(mode.retrieval.top_k));
        }
    }

    result.metadata.retrieved_docs = static_cast<int>(docs.size());
    if (!docs.empty()) {
        float sum = 0.0f;
        for (const auto& d : docs) sum += d.relevance_score;
        result.metadata.avg_relevance = sum / static_cast<float>(docs.size());
    }

    // Step 2: LLM generation with assembled context
    if (!impl_->plugin) {
        result.success = true;
        result.text    = "[LLM not configured] RAG pipeline: retrieved " +
                         std::to_string(docs.size()) + " doc(s) for: " + ctx.query;

        auto t_end = std::chrono::steady_clock::now();
        result.metadata.latency.total_ms =
            std::chrono::duration_cast<std::chrono::milliseconds>(t_end - t_start).count();
        return result;
    }

    InferenceRequest req = buildRequest(ctx, mode);
    req.prompt           = assemblePrompt(ctx.query, docs, mode);

    // Build RAGContext for plugin
    RAGContext rag_ctx;
    rag_ctx.query            = ctx.query;
    rag_ctx.top_k            = mode.retrieval.top_k;
    rag_ctx.documents        = docs;

    auto t_llm = std::chrono::steady_clock::now();
    InferenceResponse resp   = impl_->plugin->generateRAG(rag_ctx, req);
    auto t_llm_end = std::chrono::steady_clock::now();

    result.success                   = true;
    result.text                      = resp.text;
    result.metadata.tokens_prompt    = resp.tokens_prompt;
    result.metadata.tokens_generated = resp.tokens_generated;
    result.metadata.latency.llm_ms   =
        std::chrono::duration_cast<std::chrono::milliseconds>(t_llm_end - t_llm).count();
    result.metadata.latency.total_ms =
        result.metadata.latency.retrieval_ms + result.metadata.latency.llm_ms;

    result.raw_response = {
        {"text",              resp.text},
        {"tokens_prompt",     resp.tokens_prompt},
        {"tokens_generated",  resp.tokens_generated},
        {"model_id",          resp.model_id},
        {"retrieved_docs",    result.metadata.retrieved_docs},
        {"avg_relevance",     result.metadata.avg_relevance},
    };

    return result;
}

// ============================================================================
// Agentic pipeline (extensible skeleton)
// ============================================================================

OrchestratorResult AIOrchestrator::runAgentic(const OrchestratorContext& ctx,
                                               const ModeSpec&            mode) const {
    // Agentic mode: run the ask pipeline first, then parse any tool call from
    // the response text and dispatch it via the tool registry (ReAct-style).
    OrchestratorResult result = runAsk(ctx, mode);
    result.metadata.mode_id   = mode.id; // keep correct mode label

    // Parse tool calls from result.text.
    // Expected JSON format: {"name": "<tool>", "arguments": {<args>}}
    // On malformed JSON or missing fields: log a warning and return the raw text.
    try {
        json tool_call_json = json::parse(result.text);
        if (tool_call_json.contains("name") && tool_call_json["name"].is_string()) {
            std::string tool_name = tool_call_json["name"].get<std::string>();
            json        tool_args = tool_call_json.value("arguments", json::object());

            spdlog::debug("[AIOrchestrator] agentic mode: dispatching tool call '{}'",
                          tool_name);

            auto t_tool = std::chrono::steady_clock::now();
            json tool_result = impl_->tool_registry.invokeTool(tool_name, tool_args, mode);
            auto t_tool_end  = std::chrono::steady_clock::now();

            result.metadata.tool_calls_made.push_back(tool_name);
            auto tool_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                               t_tool_end - t_tool).count();
            result.metadata.latency.tool_calls_ms += tool_ms;
            result.metadata.latency.total_ms      += tool_ms;

            // Replace response text with the serialised tool result and annotate
            // the raw_response so callers can distinguish tool-call results.
            result.text                       = tool_result.dump();
            result.raw_response["tool_name"]  = tool_name;
            result.raw_response["tool_result"] = tool_result;
        } else {
            spdlog::debug("[AIOrchestrator] agentic mode: response is valid JSON "
                          "but does not contain a tool call");
        }
    } catch (const json::parse_error&) {
        // result.text is plain text, not a JSON tool call – nothing to dispatch.
        spdlog::debug("[AIOrchestrator] agentic mode: response is not JSON, "
                      "no tool call to parse");
    } catch (const std::exception& e) {
        // Tool dispatch failed; log a warning and preserve the raw LLM response.
        spdlog::warn("[AIOrchestrator] agentic mode: tool call handling failed: {}",
                     e.what());
    }

    return result;
}

// ============================================================================
// Ethics pipeline (extensible skeleton)
// ============================================================================

OrchestratorResult AIOrchestrator::runEthics(const OrchestratorContext& ctx,
                                              const ModeSpec&            mode) const {
    // Ethics mode: runs RAG with ethics safety profile applied as system prompt.
    ModeSpec eth_mode    = mode;
    eth_mode.mode_id     = ModeId::Rag;

    // Prepend ethics context into system prompt
    std::string ethics_system =
        "You are a Constitutional AI assistant. Evaluate responses against ethical "
        "guidelines and refuse harmful requests. Profile: " +
        mode.safety.ethics_profile;
    if (!ctx.system_prompt.has_value() || ctx.system_prompt->empty()) {
        OrchestratorContext eth_ctx = ctx;
        eth_ctx.system_prompt       = ethics_system;
        OrchestratorResult result   = runRag(eth_ctx, eth_mode);
        result.metadata.mode_id     = mode.id;
        return result;
    }

    OrchestratorResult result = runRag(ctx, eth_mode);
    result.metadata.mode_id   = mode.id;
    return result;
}

// ============================================================================
// Multi-agent pipeline (interface skeleton)
// ============================================================================

OrchestratorResult AIOrchestrator::runMultiAgent(const OrchestratorContext& ctx,
                                                   const ModeSpec&            mode) const {
    // Multi-agent scaffold: message is routed to the local agent.
    // Future extension: broadcast to peer agents via message-passing interface.
    OrchestratorResult result = runAsk(ctx, mode);
    result.metadata.mode_id   = mode.id;

    spdlog::debug("[AIOrchestrator] multi_agent mode: peer-broadcast extension point "
                  "(sender_agent_id='{}')", ctx.sender_agent_id);

    return result;
}

} // namespace themis::llm

