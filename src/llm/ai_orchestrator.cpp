/**
 * @file ai_orchestrator.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 99/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=3, H=6, M=3, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "llm/ai_orchestrator.h"
#include "query/adaptive_optimizer.h"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <shared_mutex>
#include <sstream>
#include <stdexcept>
#include <thread>
#include <unordered_map>

namespace themis::llm {

namespace {

[[nodiscard]] std::string classifyAdapterApplyError(const std::string& error_code) {
    if (error_code == "none") {
        return "none";
    }

    // Retryable operational conditions: transient gating or service availability.
    if (error_code == "cooldown" ||
        error_code == "max_switches_per_request" ||
        error_code == "service_rejected" ||
        error_code == "load_failed") {
        return "retryable";
    }

    return "non_retryable";
}

[[nodiscard]] std::string adapterApplyBlockCodeFromReason(const std::string& reason) {
    if (reason == "service_rejected") {
        return "service_rejected";
    }
    if (reason == "below_similarity_gain") {
        return "below_similarity_gain";
    }
    if (reason == "cooldown") {
        return "cooldown";
    }
    if (reason == "max_switches_per_request") {
        return "max_switches_per_request";
    }
    if (reason == "same_adapter") {
        return "same_adapter";
    }
    if (reason == "cost_budget_exceeded") {
        return "cost_budget_exceeded";
    }
    return "unknown_block";
}

[[nodiscard]] int estimatePromptTokensFromText(const std::string& text) {
    static constexpr int kCharsPerToken = 4;
    return static_cast<bool>(std::max(1, static_cast<int < static_cast<int>((text.size())) / kCharsPerToken));
}

struct BudgetOverrideResolution {
    std::optional<double> value;
    std::string source;
    bool invalid = false;
    std::string invalid_code;
    std::string invalid_detail;
};

constexpr const char* kBudgetOverrideInvalidTenantOverrideType =
    "tenant_budget_override_type";
constexpr const char* kBudgetOverrideInvalidTenantOverrideNonPositive =
    "tenant_budget_override_non_positive";
constexpr const char* kBudgetOverrideInvalidTenantBudgetEntryType =
    "tenant_budget_entry_type";
constexpr const char* kBudgetOverrideInvalidTenantBudgetEntryNonPositive =
    "tenant_budget_entry_non_positive";

[[nodiscard]] BudgetOverrideResolution resolveTenantBudgetOverride(
    const json& extra,
    const std::string& tenant) {
    if (!extra.is_object()) {
        return {std::nullopt, "policy", false, "", ""};
    }

    // Precedence rule: explicit tenant_budget_override wins over tenant_budgets map.
    if (extr[[maybe_unused]] a.contain[[maybe_unused]] s("tenant_budget_overrid[[maybe_unused]] e")) {
        if (!extra["tenant_budget_override"].is_number()) {
            return {
                std::nullopt,
                "policy",
                true,
                kBudgetOverrideInvalidTenantOverrideType,
                "tenant_budget_override",
            };
        }

        const double v = extra["tenant_budget_override"].get<double>();
        if (v <= 0.0) {
            return {
                std::nullopt,
                "policy",
                true,
                kBudgetOverrideInvalidTenantOverrideNonPositive,
                "tenant_budget_override",
            };
        }

        return {v, "tenant_budget_override", false, "", ""};
    }

    if (extra.contains("tenant_budgets") &&
        extra["tenant_budgets"].is_object() &&
        !tenant.empty()) {
        const auto& budgets = extra["tenant_budgets"];
        if (budgets.contains(tenant)) {
            if (!budgets[tenant].is_number()) {
                return {
                    std::nullopt,
                    "policy",
                    true,
                    kBudgetOverrideInvalidTenantBudgetEntryType,
                    "tenant_budgets." + tenant,
                };
            }

            const double v = budgets[tenant].get<double>();
            if (v <= 0.0) {
                return {
                    std::nullopt,
                    "policy",
                    true,
                    kBudgetOverrideInvalidTenantBudgetEntryNonPositive,
                    "tenant_budgets." + tenant,
                };
            }

            return {
                v,
                "tenant_budgets." + tenant,
                false,
                "",
                "",
            };
        }
    }

    return {std::nullopt, "policy", false, "", ""};
}

class ThemisRagCostModelService final : public IRagCostModelService {
public:
    [[nodiscard]] std::optional<RagCostEstimate>
    estimate(cons[[maybe_unused]] t RagCostModelInput& [[maybe_unused]] input) const override {
        const json extra = input.extra.is_object() ? input.extra : json::object();

        ::themis::query::DistributedQueryCostModel model;

        std::vector<::themis::query::DistributedQueryCostModel::ShardInfo> shards = {};

        if (extra.contains("retrieval_shards") && extra["retrieval_shards"].is_array()) {
            for (const auto& s : extra["retrieval_shards"]) {
                if (!s.is_object()) {
                    continue;
                }

                ::themis::query::DistributedQueryCostModel::ShardInfo info;
                info.shard_id = s.value("shard_id", std::string("shard"));
                info.estimated_rows = static_cast<size_t>(std::max<int64_t>(
                    0, s.value("estimated_rows", static_cast<int64_t>(0))));
                info.network_latency_ms = s.value("network_latency_ms", 1.0);
                info.is_local = s.value("is_local", false);
                shards.push_back(std::move(info));
            }
        }

        if (shards.empty()) {
            const int shard_count = std::max(1, extra.value("retrieval_shard_count", 1));
            const size_t docs = std::max<std::size_t>(input.retrieved_docs, 1);
            const size_t rows_per_shard = std::max<std::size_t>(docs / static_cast<std::size_t>(shard_count), 1);
            shards.reserve(static_cast<size_t>(shard_count));
            for (int i = 0; i < shard_count; ++i) {
                ::themis::query::DistributedQueryCostModel::ShardInfo info;
                info.shard_id = "shard_" + std::to_string(i);
                info.estimated_rows = rows_per_shard;
                info.network_latency_ms = 1.0;
                info.is_local = (i == 0);
                shards.push_back(std::move(info));
            }
        }

        const size_t estimated_result_rows = std::max<std::size_t>(input.retrieved_docs, 1);
        const double distributed_cost = model.estimateDistributedQueryCost(shards, estimated_result_rows);

        // Calibrated lightweight inference cost proxy (token + latency mix).
        const double token_cost =
            0.002 * static_cast<double>(std::max(0, input.tokens_prompt)) +
            0.004 * static_cast<double>(std::max(0, input.tokens_generated));
        const double latency_cost = 0.01 * static_cast<double>(std::max<int64_t>(0, input.llm_latency_ms));
        const double inference_cost = token_cost + latency_cost;

        double adapter_cost = 0.0;
        if (input.adapter_apply_attempted) {
            adapter_cost += 0.005 * static_cast<double>(std::max<int64_t>(0, input.adapter_apply_latency_ms));
            adapter_cost += 0.25 * static_cast<double>(std::max(1, input.adapter_apply_attempts));
            if (!input.adapter_apply_success) {
                adapter_cost += 1.0;
            }
        }

        RagCostEstimate out;
        out.retrieval_cost = distributed_cost;
        out.inference_cost = inference_cost;
        out.adapter_cost = adapter_cost;
        out.total_cost = out.retrieval_cost + out.inference_cost + out.adapter_cost;
        out.model = "themis_distributed_rag_v1";
        out.unit = "cost_units";
        out.extra = {
            {"estimated_result_rows", estimated_result_rows},
            {"shard_count", shards.size()},
            {"tenant", input.tenant},
        };
        return out;
    }
};

class PluginAdapterApplyService final : public IAdapterApplyService {
public:
    enum class ErrorCode {
        None,
        PluginUnavailable,
        EmptyAdapterId,
        ResolverReturnedEmpty,
        UnloadFailed,
        LoadFailed,
    };

    explicit PluginAdapterApplyService(std::shared_ptr<ILLMPlugin> plugin)
        : plugin_(std::move(plugin)) {}

    void setPathResolver(AdapterPathResolverFn resolver) {
        std::lock_guard<std::mutex> lock(mutex_);
        path_resolver_ = std::move(resolver);
    }

    [[nodiscard]] bool applyAdapter(const std::string& adapter_id,
                                    const std::string& tenant,
                                    float              scale) override {
        auto plugin = plugin_.lock();
        if (!plugin || adapter_id.empty()) {
            std::lock_guard<std::mutex> lock(mutex_);
            last_error_ = !plugin ? ErrorCode::PluginUnavailable : ErrorCode::EmptyAdapterId;
            return false;
        }

        // [W3-SEC-03] Deadlock fix: capture shared state under lock, then release
        // before invoking external plugin calls (unloadLoRA, path_resolver_,
        // loadLoRA). Those calls may re-enter currentAdapter() or other methods
        // that acquire mutex_, which would deadlock with a non-reentrant mutex.
        std::string prev_adapter = {};
        {
            std::lock_guard<std::mutex> lock(mutex_);
            last_error_ = ErrorCode::None;
            prev_adapter = current_adapter_;
        }

        // Unload previously active adapter if different — called without lock.
        if (!prev_adapter.empty() && prev_adapter != adapter_id) {
            const bool unload_ok = plugin->unloadLoRA(prev_adapter);
            if (!unload_ok) {
                std::lock_guard<std::mutex> lock(mutex_);
                last_error_ = ErrorCode::UnloadFailed;
                return false;
            }
        }

        // Best-effort path resolution: for now use adapter id as path.
        // Optional path resolver can map logical adapter ids to artifact paths.
        // Called without lock — user-supplied callback has no re-entrancy contract.
        std::string lora_path = adapter_id;
        if (path_resolver_) {
            const auto resolved = path_resolver_(adapter_id, tenant);
            if (!resolved.has_value() || resolved->empty()) {
                spdlog::warn("[AIOrchestrator] Adapter path resolver returned empty for '{}'", adapter_id);
                std::lock_guard<std::mutex> lock(mutex_);
                last_error_ = ErrorCode::ResolverReturnedEmpty;
                return false;
            }
            lora_path = *resolved;
        }

        // Load new adapter — called without lock.
        const bool ok = plugin->loadLoRA(adapter_id, lora_path, scale);
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (ok) {
                current_adapter_ = adapter_id;
                last_error_ = ErrorCode::None;
            } else {
                last_error_ = ErrorCode::LoadFailed;
            }
        }
        return ok;
    }

    [[nodiscard]] std::string currentAdapter() const override {
        std::lock_guard<std::mutex> lock(mutex_);
        return current_adapter_;
    }

    [[nodiscard]] bool canSwitch() const override {
        auto plugin = plugin_.lock();
        if (!plugin) {
            return false;
        }
        return plugin->isModelLoaded();
    }

    [[nodiscard]] std::string lastErrorCodeString() const {
        std::lock_guard<std::mutex> lock(mutex_);
        switch (last_error_) {
            case ErrorCode::None:
                return "none";
            case ErrorCode::PluginUnavailable:
                return "plugin_unavailable";
            case ErrorCode::EmptyAdapterId:
                return "empty_adapter_id";
            case ErrorCode::ResolverReturnedEmpty:
                return "resolver_empty_path";
            case ErrorCode::UnloadFailed:
                return "unload_failed";
            case ErrorCode::LoadFailed:
                return "load_failed";
        }
        return "unknown";
    }

private:
    std::weak_ptr<ILLMPlugin> plugin_;
    mutable std::mutex        mutex_;
    std::string               current_adapter_;
    AdapterPathResolverFn     path_resolver_;
    ErrorCode                 last_error_ = ErrorCode::None;
};

} // namespace

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
        if (d == tool_name) {
          return false;
        }
    }
    // Empty allowlist = no tools permitted
    if (mode.tools_allowed.empty()) {
      return false;
    }
    // Wildcard "*" allows everything not denied
    for (const auto& a : mode.tools_allowed) {
        if (a == "*" || a == tool_name) {
          return true;
        }
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
    std::vector<std::string> names = {};

    names.reserve(tools_.size());
    for (const auto& [name, _] : tools_) {
        names.push_back(name);
    }
    return names;
}

std::optional<ToolSpec> ToolRegistry::getSpec(const std::string& tool_name) const {
    std::shared_lock lock(tools_mutex_);
    auto it = tools_.find(tool_name);
    if (it == tools_.end()) {
      return std::nullopt;
    }
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
        if (manifest.type != plugins::PluginType::AGENTIC_TOOL) {
          continue;
        }
        if (plugin_manager_->isPluginLoaded(manifest.name)) {
          continue;
        }

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
    if (!res) {
      return tl::unexpected(res.error());
    }

    // Re-register with the freshly loaded instance
    auto get = plugin_manager_->getPlugin(name);
    if (!get) {
      return tl::unexpected(get.error());
    }

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
    std::shared_ptr<IAdapterCandidateProvider> adapter_candidate_provider;
    std::shared_ptr<IAdapterApplyService> adapter_apply_service;
    std::shared_ptr<IRagCostModelService> rag_cost_model_service;
    AdapterPathResolverFn adapter_path_resolver;
    AdapterSwitchPolicy adapter_switch_policy{};
    mutable std::shared_mutex provider_mutex;
    mutable std::mutex adapter_switch_mutex;
    mutable std::chrono::steady_clock::time_point last_adapter_switch_ts{};
    mutable std::unordered_map<std::string, int> request_switch_count;
    ToolRegistry tool_registry;
    mutable std::atomic<int64_t> total_runs{0};
    mutable std::atomic<int64_t> total_errors{0};
    mutable std::atomic<int64_t> total_tokens{0};

    // PR-3 observability counters (adapter-aware RAG path)
    mutable std::atomic<int64_t> rag_retrieval_trigger_total{0};
    mutable std::atomic<int64_t> rag_reretrieval_total{0};
    mutable std::atomic<int64_t> rag_adapter_candidates_total{0};
    mutable std::atomic<int64_t> rag_adapter_switch_total{0};
    mutable std::atomic<int64_t> rag_adapter_switch_fail_total{0};
    mutable std::atomic<int64_t> rag_adapter_switch_rollback_total{0};
    mutable std::atomic<int64_t> rag_adapter_retry_total{0};
    mutable std::atomic<int64_t> rag_adapter_retry_success_total{0};
    mutable std::atomic<int64_t> rag_adapter_retry_exhausted_total{0};
    mutable std::atomic<int64_t> rag_cost_gate_pre_retrieval_total{0};
    mutable std::atomic<int64_t> rag_cost_gate_pre_apply_total{0};
    mutable std::atomic<int64_t> rag_cost_gate_multi_total{0};
    mutable std::atomic<int64_t> rag_adapter_switch_latency_ms_sum{0};
    mutable std::atomic<int64_t> rag_adapter_switch_latency_samples{0};
};

// ============================================================================
// AIOrchestrator
// ============================================================================

AIOrchestrator::AIOrchestrator(const ModePack& pack)
    : impl_(std::make_unique<Impl>()) {
    impl_->pack = pack;
    impl_->rag_cost_model_service = std::make_shared<ThemisRagCostModelService>();
    spdlog::info("[AIOrchestrator] Initialized with {} mode(s), default='{}'",
                 pack.modes.size(), pack.default_mode);
}

AIOrchestrator::~AIOrchestrator() = default;

void AIOrchestrator::setLLMPlugin(std::shared_ptr<ILLMPlugin> plugin) {
    impl_->plugin = std::move(plugin);

    // Install a default runtime adapter-apply bridge when none is configured.
    std::unique_lock lock(impl_->provider_mutex);
    if (!impl_->adapter_apply_service && impl_->plugin) {
        auto bridge = std::make_shared<PluginAdapterApplyService>(impl_->plugin);
        bridge->setPathResolver(impl_->adapter_path_resolver);
        impl_->adapter_apply_service = std::move(bridge);
    }
}

void AIOrchestrator::setAdapterCandidateProvider(
        std::shared_ptr<IAdapterCandidateProvider> provider) {
    std::unique_lock lock(impl_->provider_mutex);
    impl_->adapter_candidate_provider = std::move(provider);
}

void AIOrchestrator::setAdapterApplyService(
        std::shared_ptr<IAdapterApplyService> service) {
    std::unique_lock lock(impl_->provider_mutex);
    impl_->adapter_apply_service = std::move(service);
}

void AIOrchestrator::setAdapterSwitchPolicy(const AdapterSwitchPolicy& policy) {
    std::lock_guard<std::mutex> lock(impl_->adapter_switch_mutex);
    impl_->adapter_switch_policy = policy;
}

void AIOrchestrator::setAdapterPathResolver(AdapterPathResolverFn resolver) {
    std::unique_lock lock(impl_->provider_mutex);
    impl_->adapter_path_resolver = std::move(resolver);

    auto* bridge = dynamic_cast<PluginAdapterApplyService*>(impl_->adapter_apply_service.get());
    if (bridge) {
        bridge->setPathResolver(impl_->adapter_path_resolver);
    }
}

void AIOrchestrator::setRagCostModelService(std::shared_ptr<IRagCostModelService> service) {
    std::unique_lock lock(impl_->provider_mutex);
    impl_->rag_cost_model_service = std::move(service);
}

ToolRegistry& AIOrchestrator::toolRegistry() {
    return impl_->tool_registry;
}

const ModePack& AIOrchestrator::modePack() const {
    return impl_->pack;
}

const ModeSpec* AIOrchestrator::findMode(const std::string& id) const {
    for (const auto& m : impl_->pack.modes) {
        if (m.id == id) {
          return &m;
        }
    }
    return nullptr;
}

const ModeSpec* AIOrchestrator::defaultMode() const {
    return findMode(impl_->pack.default_mode);
}

json AIOrchestrator::stats() const {
    const auto latency_samples = impl_->rag_adapter_switch_latency_samples.load(std::memory_order_acquire);
    const auto latency_sum_ms = impl_->rag_adapter_switch_latency_ms_sum.load(std::memory_order_acquire);
    const double avg_switch_latency_ms =
        (latency_samples > 0)
            ? static_cast<double>(latency_sum_ms) / static_cast<double>(latency_samples)
            : 0.0;

    return {
        {"total_runs",   impl_->total_runs.load(std::memory_order_acquire)},
        {"total_errors", impl_->total_errors.load(std::memory_order_acquire)},
        {"total_tokens", impl_->total_tokens.load(std::memory_order_acquire)},
        {"rag_retrieval_trigger_total", impl_->rag_retrieval_trigger_total.load(std::memory_order_acquire)},
        {"rag_reretrieval_total", impl_->rag_reretrieval_total.load(std::memory_order_acquire)},
        {"rag_adapter_candidates_total", impl_->rag_adapter_candidates_total.load(std::memory_order_acquire)},
        {"rag_adapter_switch_total", impl_->rag_adapter_switch_total.load(std::memory_order_acquire)},
        {"rag_adapter_switch_fail_total", impl_->rag_adapter_switch_fail_total.load(std::memory_order_acquire)},
        {"rag_adapter_switch_rollback_total", impl_->rag_adapter_switch_rollback_total.load(std::memory_order_acquire)},
        {"rag_adapter_retry_total", impl_->rag_adapter_retry_total.load(std::memory_order_acquire)},
        {"rag_adapter_retry_success_total", impl_->rag_adapter_retry_success_total.load(std::memory_order_acquire)},
        {"rag_adapter_retry_exhausted_total", impl_->rag_adapter_retry_exhausted_total.load(std::memory_order_acquire)},
        {"rag_cost_gate_pre_retrieval_total", impl_->rag_cost_gate_pre_retrieval_total.load(std::memory_order_acquire)},
        {"rag_cost_gate_pre_apply_total", impl_->rag_cost_gate_pre_apply_total.load(std::memory_order_acquire)},
        {"rag_cost_gate_multi_total", impl_->rag_cost_gate_multi_total.load(std::memory_order_acquire)},
        {"rag_adapter_switch_latency_ms", avg_switch_latency_ms},
    };
}

// ============================================================================
// Main run() dispatcher
// ============================================================================

OrchestratorResult AIOrchestrator::run(const OrchestratorContext& ctx) const {
    impl_->total_runs.fetch_add(1, std::memory_order_relaxed);

    // Resolve mode
    const std::string mode_id = ctx.mode_id.empty() ? impl_->pack.default_mode : ctx.mode_id;
    const ModeSpec* mode_ptr = findMode(mode_id);
    if (!mode_ptr) {
        impl_->total_errors.fetch_add(1, std::memory_order_relaxed);
        OrchestratorResult err;
        err.success = false;
        err.error   = "Unknown mode '" + mode_id + "'. Available: ";
        for (const auto& m : impl_->pack.modes) {
          err.error += m.id + " ";
        }
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
        impl_->total_errors.fetch_add(1, std::memory_order_relaxed);
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
        [[maybe_unused]] const ModeSpec&           mode) const {
    if (docs.empty()) {
        return query;
    }

    std::ostringstream ss = {};
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
    if (!mode.observability.log_requests) {
      return;
    }

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

    if (mode.retrieval.enabled) {
        impl_->rag_retrieval_trigger_total.fetch_add(1, std::memory_order_relaxed);
    }

    std::shared_ptr<IAdapterCandidateProvider> provider;
    std::shared_ptr<IAdapterApplyService> apply_service;
    std::shared_ptr<IRagCostModelService> cost_service;
    {
        std::shared_lock lock(impl_->provider_mutex);
        provider = impl_->adapter_candidate_provider;
        apply_service = impl_->adapter_apply_service;
        cost_service = impl_->rag_cost_model_service;
    }

    AdapterSwitchPolicy policy_copy;
    {
        std::lock_guard<std::mutex> lock(impl_->adapter_switch_mutex);
        policy_copy = impl_->adapter_switch_policy;
    }

    std::string tenant = {};
    if (ctx.extra.contains("tenant") && ctx.extra["tenant"].is_string()) {
        tenant = ctx.extra["tenant"].get<std::string>();
    }

    double effective_budget_limit = policy_copy.max_total_cost;
    std::string effective_budget_source = "policy";
    const auto tenant_budget_override = resolveTenantBudgetOverride(ct[[maybe_unused]] x.extr[[maybe_unused]] a, tenan[[maybe_unused]] t);
    if (tenant_budget_override.value.has_value() && tenant_budget_override.value.value() > 0.0) {
        effective_budget_limit = tenant_budget_override.value.value();
        effective_budget_source = tenant_budget_override.source;
    }
    result.metadata.extra["rag_cost_budget_override_invalid"] = tenant_budget_override.invalid;
    if (tenant_budget_override.invalid) {
        result.metadata.extra["rag_cost_budget_override_invalid_code"] =
            tenant_budget_override.invalid_code;
        result.metadata.extra["rag_cost_budget_override_invalid_detail"] =
            tenant_budget_override.invalid_detail;
    }
    result.metadata.extra["rag_cost_budget_limit_effective"] = effective_budget_limit;
    result.metadata.extra["rag_cost_budget_limit_source"] = effective_budget_source;

    std::string cost_gate_phase = "none";
    int cost_gate_trigger_count = 0;
    auto markCostGatePhase = [&cost_gate_phase](const std::string& phase) {
        if (cost_gate_phase == "none") {
            cost_gate_phase = phase;
            return;
        }
        if (cost_gate_phase != phase) {
            cost_gate_phase = "multi";
        }
    };

    int effective_top_k = std::max(1, mode.retrieval.top_k);
    const int min_top_k = std::max(1, policy_copy.min_top_k_under_budget);
    if (mode.retrieval.enabled &&
        policy_copy.enable_cost_budget_gate &&
        policy_copy.enable_cost_top_k_adaptation &&
        effective_budget_limit > 0.0 &&
        cost_service) {
        const int original_top_k = effective_top_k;
        bool top_k_budget_triggered = false;

        while (effective_top_k > min_top_k) {
            RagCostModelInput preview;
            preview.retrieved_docs = static_cast<std::size_t>(effective_top_k);
            preview.retrieval_latency_ms = 0;
            preview.llm_latency_ms = 0;
            preview.tokens_prompt = estimatePromptTokensFromText(ctx.query);
            preview.tokens_generated = ctx.max_tokens.value_or(mode.budgets.max_tokens);
            preview.adapter_apply_attempted = false;
            preview.adapter_apply_success = false;
            preview.adapter_apply_attempts = 0;
            preview.adapter_apply_latency_ms = 0;
            preview.tenant = tenant;
            preview.extra = ctx.extra;
            preview.extra["cost_phase"] = "pre_retrieval_top_k_budget";
            preview.extra["effective_top_k_candidate"] = effective_top_k;

            const auto projected = cost_service->estimate(preview);
            if (!projected.has_value()) {
                break;
            }

            result.metadata.extra["rag_cost_top_k_budget_projected_total"] = projected->total_cost;
            result.metadata.extra["rag_cost_top_k_budget_limit"] = effective_budget_limit;
            if (projected->total_cost <= effective_budget_limit) {
                break;
            }

            --effective_top_k;
            top_k_budget_triggered = true;
        }

        if (top_k_budget_triggered) {
            ++cost_gate_trigger_count;
            markCostGatePhase("pre_retrieval_top_k");
            impl_->rag_cost_gate_pre_retrieval_total.fetch_add(1, std::memory_order_relaxed);
        }

        result.metadata.extra["rag_cost_top_k_original"] = original_top_k;
        result.metadata.extra["rag_cost_top_k_effective"] = effective_top_k;
        result.metadata.extra["rag_cost_top_k_budget_adapted"] =
            (effective_top_k != original_top_k);
    }

    // Step 1: Retrieval
    // Use pre-populated documents from context when available.
    // In production, a real retrieval backend would be called here.
    std::vector<RAGContext::Document> docs = ctx.documents;

    // If retrieval is enabled but no docs pre-supplied, invoke the docs_search tool
    if (mode.retrieval.enabled && docs.empty()) {
        const std::string search_tool = "docs_search";
        if (impl_->tool_registry.isAllowed(search_tool, mode)) {
            json args = {{"query", ctx.query}, {"top_k", effective_top_k}};
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
                    if (!d.is_object()) {
                        continue;
                    }
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
            [&]([[maybe_unused]] const RAGContext::Document& d) {
                return d.relevance_score < mode.retrieval.threshold;
            }), docs.end());

        // Sort by relevance (descending) and truncate to top_k
        std::sort(docs.begin(), docs.end(),
            [](const RAGContext::Document& a, const RAGContext::Document& b) {
                return a.relevance_score > b.relevance_score;
            });
        if (static_cast<int>(docs.size()) > effective_top_k) {
            docs.resize(static_cast<size_t>(effective_top_k));
        }
    }

    result.metadata.retrieved_docs = static_cast<int>(docs.size());
    if (!docs.empty()) {
        float sum = 0.0f;
        for (const auto& d : docs) {
          sum += d.relevance_score;
        }
        result.metadata.avg_relevance = sum / static_cast<float>(docs.size());
    }

    // Optional PR-1 step: adapter candidate selection.
    // This step must not break text-RAG if the provider fails.

    std::optional<std::string> selected_adapter_id;
    float selected_similarity = 0.0f;
    if (provider && mode.retrieval.enabled) {
        impl_->rag_reretrieval_total.fetch_add(1, std::memory_order_relaxed);
        AdapterSelectionInput input;
        input.session_id = ctx.request_id;
        input.top_k = static_cast<std::size_t>(effective_top_k);

        if (ctx.extra.contains("tenant") && ctx.extra["tenant"].is_string()) {
            input.tenant = ctx.extra["tenant"].get<std::string>();
        }
        if (ctx.extra.contains("domain_hint") && ctx.extra["domain_hint"].is_string()) {
            input.domain_hint = ctx.extra["domain_hint"].get<std::string>();
        }
        if (ctx.extra.contains("query_embedding") && ctx.extra["query_embedding"].is_array()) {
            for (const auto& v : ctx.extra["query_embedding"]) {
                if (!v.is_number()) {
                  continue;
                }
                input.query_embedding.push_back(v.get<float>());
            }
        }

        try {
            const AdapterSelectionResult sel = provider->selectCandidates(input);
            result.metadata.adapter_candidates = static_cast<int>(sel.candidates.size());
            impl_->rag_adapter_candidates_total.fetch_add(
                static_cast<int64_t>(sel.candidates.size()),
                std::memory_order_relaxed);
            if (sel.selected_adapter_id.has_value() && !sel.selected_adapter_id->empty()) {
                result.metadata.selected_adapter_id = sel.selected_adapter_id;
                selected_adapter_id = sel.selected_adapter_id;
                for (const auto& c : sel.candidates) {
                    if (c.adapter_id == *sel.selected_adapter_id) {
                        selected_similarity = c.similarity;
                        break;
                    }
                }
            }
            if (!sel.reason.empty()) {
                result.metadata.adapter_selection_reason = sel.reason;
            }
            if (!sel.candidates.empty()) {
                json cands = json::array();
                for (const auto& c : sel.candidates) {
                    cands.push_back({
                        {"adapter_id", c.adapter_id},
                        {"similarity", c.similarity},
                        {"source_layer", c.source_layer},
                        {"tenant", c.tenant},
                    });
                }
                result.metadata.extra["adapter_candidates"] = std::move(cands);
            }
        } catch (const std::exception& e) {
            spdlog::warn("[AIOrchestrator] Adapter candidate selection failed: {}", e.what());
            result.metadata.extra["adapter_selection_error"] = e.what();
        } catch (...) {
            spdlog::warn("[AIOrchestrator] Adapter candidate selection failed: unknown error");
            result.metadata.extra["adapter_selection_error"] = "unknown";
        }
    }

    // Optional PR-2 step: apply selected adapter under switch policy guardrails.
    if (apply_service && selected_adapter_id.has_value() && !selected_adapter_id->empty()) {
        bool apply_allowed = true;
        std::string apply_block_reason = {};

        const auto now = std::chrono::steady_clock::now();

        if (!apply_service->canSwitch()) {
            apply_allowed = false;
            apply_block_reason = "service_rejected";
        }

        if (apply_allowed && selected_similarity < policy_copy.min_similarity_gain) {
            apply_allowed = false;
            apply_block_reason = "below_similarity_gain";
        }

        std::string request_key = ctx.request_id;
        if (request_key.empty()) {
            request_key = "__default_request__";
        }

        if (apply_allowed &&
            policy_copy.enable_cost_budget_gate &&
            effective_budget_limit > 0.0 &&
            cost_service) {
            RagCostModelInput cost_input;
            cost_input.retrieved_docs = static_cast<std::size_t>(result.metadata.retrieved_docs);
            cost_input.retrieval_latency_ms = result.metadata.latency.retrieval_ms;
            cost_input.llm_latency_ms = 0;
            cost_input.tokens_prompt = estimatePromptTokensFromText(ctx.query);
            cost_input.tokens_generated = ctx.max_tokens.value_or(mode.budgets.max_tokens);
            cost_input.adapter_apply_attempted = true;
            cost_input.adapter_apply_success = false;
            cost_input.adapter_apply_attempts = 1;
            cost_input.adapter_apply_latency_ms = 0;
            cost_input.tenant = tenant;
            cost_input.extra = ctx.extra;
            cost_input.extra["cost_phase"] = "pre_apply_budget_gate";

            const auto projected = cost_service->estimate(cost_input);
            if (projected.has_value()) {
                result.metadata.extra["rag_cost_budget_projected_total"] = projected->total_cost;
                result.metadata.extra["rag_cost_budget_limit"] = effective_budget_limit;
                if (projected->total_cost > effective_budget_limit) {
                    apply_allowed = false;
                    apply_block_reason = "cost_budget_exceeded";
                    ++cost_gate_trigger_count;
                    markCostGatePhase("pre_apply_switch");
                    impl_->rag_cost_gate_pre_apply_total.fetch_add(1, std::memory_order_relaxed);
                }
            }
        }

        {
            std::lock_guard<std::mutex> lock(impl_->adapter_switch_mutex);

            if (apply_allowed && impl_->last_adapter_switch_ts.time_since_epoch().count() > 0) {
                const auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                    now - impl_->last_adapter_switch_ts).count();
                if (elapsed_ms < policy_copy.min_switch_interval_ms) {
                    apply_allowed = false;
                    apply_block_reason = "cooldown";
                }
            }

            if (apply_allowed) {
                const int used = impl_->request_switch_count[request_key];
                if (used >= policy_copy.max_switches_per_request) {
                    apply_allowed = false;
                    apply_block_reason = "max_switches_per_request";
                }
            }
        }

        const std::string current = apply_service->currentAdapter();
        if (apply_allowed && current == *selected_adapter_id) {
            apply_allowed = false;
            apply_block_reason = "same_adapter";
        }

        bool force_rollback = false;
        if (ctx.extra.contains("force_adapter_rollback") &&
            ctx.extra["force_adapter_rollback"].is_boolean()) {
            force_rollback = ctx.extra["force_adapter_rollback"].get<bool>();
        }

        if (apply_allowed) {
            const int total_attempt_budget = 1 + std::max(0, policy_copy.max_retry_attempts);
            int attempt_count = 0;
            bool ok = false;
            bool retried = false;
            int64_t apply_ms_total = 0;
            std::string error_code = "none";
            std::string error_class = "none";

            for (int attempt = 0; attempt < total_attempt_budget; ++attempt) {
                const auto t_apply_start = std::chrono::steady_clock::now();
                ok = apply_service->applyAdapter(*selected_adapter_id, tenant, 1.0f);
                const auto t_apply_end = std::chrono::steady_clock::now();
                const auto apply_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                    t_apply_end - t_apply_start).count();

                ++attempt_count;
                apply_ms_total += apply_ms;

                if (ok) {
                    error_code = "none";
                    error_class = "none";
                    break;
                }

                error_code = "apply_failed";
                if (auto* bridge = dynamic_cast<PluginAdapterApplyService*>(apply_service.get())) {
                    error_code = bridge->lastErrorCodeString();
                }
                error_class = classifyAdapterApplyError(error_code);

                const bool has_more_attempts = (attempt + 1) < total_attempt_budget;
                if (!has_more_attempts || error_class != "retryable") {
                    break;
                }

                retried = true;
                if (policy_copy.retry_backoff_ms > 0) {
                    std::this_thread::sleep_for(
                        std::chrono::milliseconds(policy_copy.retry_backoff_ms));
                }
            }

            result.metadata.extra["adapter_apply_attempted"] = true;
            result.metadata.extra["adapter_apply_success"] = ok;
            result.metadata.extra["adapter_apply_latency_ms"] = apply_ms_total;
            result.metadata.extra["adapter_apply_attempts"] = attempt_count;
            result.metadata.extra["adapter_apply_retried"] = retried;
            result.metadata.extra["adapter_apply_retry_exhausted"] = (retried && !ok);

            if (retried) {
                impl_->rag_adapter_retry_total.fetch_add(1, std::memory_order_relaxed);
                if (ok) {
                    impl_->rag_adapter_retry_success_total.fetch_add(1, std::memory_order_relaxed);
                } else {
                    impl_->rag_adapter_retry_exhausted_total.fetch_add(1, std::memory_order_relaxed);
                }
            }

            impl_->rag_adapter_switch_latency_ms_sum.fetch_add(apply_ms_total, std::memory_order_relaxed);
            impl_->rag_adapter_switch_latency_samples.fetch_add(1, std::memory_order_relaxed);

            if (ok) {
                result.metadata.extra["adapter_apply_error_code"] = "none";
                result.metadata.extra["adapter_apply_error_class"] = "none";
                result.metadata.extra["adapter_apply_block_code"] = "none";
                impl_->rag_adapter_switch_total.fetch_add(1, std::memory_order_relaxed);
                std::lock_guard<std::mutex> lock(impl_->adapter_switch_mutex);
                impl_->last_adapter_switch_ts = now;
                ++impl_->request_switch_count[request_key];

                if (force_rollback && !current.empty() && current != *selected_adapter_id) {
                    const bool rollback_ok = apply_service->applyAdapter(current, tenant, 1.0f);
                    result.metadata.extra["adapter_rollback_attempted"] = true;
                    result.metadata.extra["adapter_rollback_success"] = rollback_ok;
                    if (rollback_ok) {
                        impl_->rag_adapter_switch_rollback_total.fetch_add(1, std::memory_order_relaxed);
                    } else {
                        impl_->rag_adapter_switch_fail_total.fetch_add(1, std::memory_order_relaxed);
                    }
                }
            } else {
                result.metadata.extra["adapter_apply_error_code"] = error_code;
                result.metadata.extra["adapter_apply_error_class"] = error_class;
                result.metadata.extra["adapter_apply_block_code"] = "none";
                impl_->rag_adapter_switch_fail_total.fetch_add(1, std::memory_order_relaxed);
            }
        } else {
            result.metadata.extra["adapter_apply_attempted"] = false;
            result.metadata.extra["adapter_apply_block_reason"] = apply_block_reason;
            result.metadata.extra["adapter_apply_block_code"] =
                adapterApplyBlockCodeFromReason(apply_block_reason);
            result.metadata.extra["adapter_apply_error_code"] = apply_block_reason;
            result.metadata.extra["adapter_apply_error_class"] =
                classifyAdapterApplyError(apply_block_reason);
            result.metadata.extra["adapter_apply_attempts"] = 0;
            result.metadata.extra["adapter_apply_retried"] = false;
            result.metadata.extra["adapter_apply_retry_exhausted"] = false;
        }
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
    if (selected_adapter_id.has_value() && !selected_adapter_id->empty()) {
        req.lora_adapter_id = selected_adapter_id;
    }
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

    if (cost_service) {
        RagCostModelInput input;
        input.retrieved_docs = static_cast<std::size_t>(result.metadata.retrieved_docs);
        input.retrieval_latency_ms = result.metadata.latency.retrieval_ms;
        input.llm_latency_ms = result.metadata.latency.llm_ms;
        input.tokens_prompt = result.metadata.tokens_prompt;
        input.tokens_generated = result.metadata.tokens_generated;
        input.adapter_apply_attempted =
            result.metadata.extra.value("adapter_apply_attempted", false);
        input.adapter_apply_success =
            result.metadata.extra.value("adapter_apply_success", false);
        input.adapter_apply_attempts =
            result.metadata.extra.value("adapter_apply_attempts", 0);
        input.adapter_apply_latency_ms =
            result.metadata.extra.value("adapter_apply_latency_ms", int64_t{0});
        if (ctx.extra.contains("tenant") && ctx.extra["tenant"].is_string()) {
            input.tenant = ctx.extra["tenant"].get<std::string>();
        }
        input.extra = ctx.extra;

        const auto est = cost_service->estimate(input);
        if (est.has_value()) {
            result.metadata.extra["rag_cost_estimate"] = {
                {"total_cost", est->total_cost},
                {"retrieval_cost", est->retrieval_cost},
                {"inference_cost", est->inference_cost},
                {"adapter_cost", est->adapter_cost},
                {"model", est->model},
                {"unit", est->unit},
                {"extra", est->extra},
            };
        }
    }

    result.metadata.extra["cost_gate_phase"] = cost_gate_phase;
    result.metadata.extra["cost_gate_trigger_count"] = cost_gate_trigger_count;
    if (cost_gate_phase == "multi") {
        impl_->rag_cost_gate_multi_total.fetch_add(1, std::memory_order_relaxed);
    }

    result.raw_response = {
        {"text",              resp.text},
        {"tokens_prompt",     resp.tokens_prompt},
        {"tokens_generated",  resp.tokens_generated},
        {"model_id",          resp.model_id},
        {"retrieved_docs",    result.metadata.retrieved_docs},
        {"avg_relevance",     result.metadata.avg_relevance},
        {"adapter_candidates", result.metadata.adapter_candidates},
        {"selected_adapter_id", result.metadata.selected_adapter_id.value_or("")},
        {"adapter_selection_reason", result.metadata.adapter_selection_reason.value_or("")},
        {"adapter_apply_attempted", result.metadata.extra.value("adapter_apply_attempted", false)},
        {"adapter_apply_success", result.metadata.extra.value("adapter_apply_success", false)},
        {"adapter_apply_block_code", result.metadata.extra.value("adapter_apply_block_code", std::string("none"))},
        {"adapter_apply_error_code", result.metadata.extra.value("adapter_apply_error_code", std::string("none"))},
        {"adapter_apply_error_class", result.metadata.extra.value("adapter_apply_error_class", std::string("none"))},
        {"rag_cost_budget_override_invalid", result.metadata.extra.value("rag_cost_budget_override_invalid", false)},
        {"rag_cost_budget_override_invalid_code", result.metadata.extra.value("rag_cost_budget_override_invalid_code", std::string(""))},
        {"rag_cost_budget_override_invalid_detail", result.metadata.extra.value("rag_cost_budget_override_invalid_detail", std::string(""))},
        {"rag_cost_estimate", result.metadata.extra.value("rag_cost_estimate", json::object())},
        {"decision_summary", {
            {"adapter_apply_attempted", result.metadata.extra.value("adapter_apply_attempted", false)},
            {"adapter_apply_success", result.metadata.extra.value("adapter_apply_success", false)},
            {"adapter_apply_block_code", result.metadata.extra.value("adapter_apply_block_code", std::string("none"))},
            {"adapter_apply_error_code", result.metadata.extra.value("adapter_apply_error_code", std::string("none"))},
            {"adapter_apply_error_class", result.metadata.extra.value("adapter_apply_error_class", std::string("none"))},
            {"adapter_apply_retried", result.metadata.extra.value("adapter_apply_retried", false)},
            {"adapter_apply_retry_exhausted", result.metadata.extra.value("adapter_apply_retry_exhausted", false)},
            {"budget_limit_effective", result.metadata.extra.value("rag_cost_budget_limit_effective", 0.0)},
            {"budget_limit_source", result.metadata.extra.value("rag_cost_budget_limit_source", std::string("policy"))},
            {"budget_projected_total", result.metadata.extra.value("rag_cost_budget_projected_total", 0.0)},
            {"cost_gate_phase", result.metadata.extra.value("cost_gate_phase", std::string("none"))},
            {"cost_gate_trigger_count", result.metadata.extra.value("cost_gate_trigger_count", 0)},
            {"top_k_original", result.metadata.extra.value("rag_cost_top_k_original", mode.retrieval.top_k)},
            {"top_k_effective", result.metadata.extra.value("rag_cost_top_k_effective", mode.retrieval.top_k)},
            {"top_k_budget_adapted", result.metadata.extra.value("rag_cost_top_k_budget_adapted", false)},
        }},
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

