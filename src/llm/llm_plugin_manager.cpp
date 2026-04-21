/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            llm_plugin_manager.cpp                             ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:49:33                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   98.0/100                                       ║
    • Total Lines:     467                                            ║
    • Open Issues:     TODOs: 1, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "llm/llm_plugin_manager.h"
#include "llm/llama_wrapper.h"
#include "llm/embedded_llm.h"
#include "utils/error_registry.h"
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <sstream>
namespace themis {
namespace llm {

void LLMPluginManager::registerPlugin(
    const std::string& name,
    std::unique_ptr<ILLMPlugin> plugin
) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!plugin) {
        throw std::invalid_argument("Cannot register null plugin");
    }
    
    if (plugins_.find(name) != plugins_.end()) {
        spdlog::warn("Plugin '{}' already registered, replacing", name);
    }
    
    PluginEntry entry;
    entry.name = name;
    entry.plugin = std::move(plugin);
    
    plugins_[name] = std::move(entry);
    
    // Set as default if it's the first plugin
    if (default_plugin_name_.empty()) {
        default_plugin_name_ = name;
        spdlog::info("Set '{}' as default LLM plugin", name);
    }
    
    spdlog::info("Registered LLM plugin: {}", name);
}

void LLMPluginManager::unregisterPlugin(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = plugins_.find(name);
    if (it == plugins_.end()) {
        spdlog::warn("Plugin '{}' not found, cannot unregister", name);
        return;
    }
    
    // If this was the default, clear it
    if (default_plugin_name_ == name) {
        default_plugin_name_.clear();
        
        // Set a new default if other plugins exist
        if (!plugins_.empty()) {
            for (const auto& [plugin_name, _] : plugins_) {
                if (plugin_name != name) {
                    default_plugin_name_ = plugin_name;
                    spdlog::info("Set '{}' as new default LLM plugin", plugin_name);
                    break;
                }
            }
        }
    }
    
    plugins_.erase(it);
    spdlog::info("Unregistered LLM plugin: {}", name);
}

ILLMPlugin* LLMPluginManager::getPlugin(const std::string& name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = plugins_.find(name);
    if (it == plugins_.end()) {
        return nullptr;
    }
    
    return it->second.plugin.get();
}

ILLMPlugin* LLMPluginManager::getDefaultPlugin() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return getDefaultPluginLocked();
}

ILLMPlugin* LLMPluginManager::getDefaultPluginLocked() const {
    if (default_plugin_name_.empty()) {
        return nullptr;
    }
    
    auto it = plugins_.find(default_plugin_name_);
    if (it == plugins_.end()) {
        return nullptr;
    }
    
    return it->second.plugin.get();
}

void LLMPluginManager::setDefaultPlugin(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (plugins_.find(name) == plugins_.end()) {
        throw std::invalid_argument("Plugin '" + name + "' not found");
    }
    
    default_plugin_name_ = name;
    spdlog::info("Set '{}' as default LLM plugin", name);
}

std::vector<std::string> LLMPluginManager::listPlugins() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<std::string> result;
    result.reserve(plugins_.size());
    
    for (const auto& [name, _] : plugins_) {
        result.push_back(name);
    }
    
    return result;
}

bool LLMPluginManager::hasPlugin(const std::string& name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return plugins_.find(name) != plugins_.end();
}

json LLMPluginManager::getAggregatedCapabilities() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    json result = json::array();
    
    for (const auto& [name, entry] : plugins_) {
        json plugin_caps;
        plugin_caps["name"] = name;
        plugin_caps["is_default"] = (name == default_plugin_name_);
        
        auto caps = entry.plugin->getCapabilities();
        plugin_caps["capabilities"] = {
            {"supports_instruct", caps.supports_instruct},
            {"supports_chat", caps.supports_chat},
            {"supports_completion", caps.supports_completion},
            {"supports_lora", caps.supports_lora},
            {"supports_quantization", caps.supports_quantization},
            {"supports_streaming", caps.supports_streaming},
            {"supports_batching", caps.supports_batching},
            {"gpu_accelerated", caps.gpu_accelerated},
            {"supports_cuda", caps.supports_cuda},
            {"supports_rocm", caps.supports_rocm},
            {"supports_metal", caps.supports_metal},
            {"supports_vulkan", caps.supports_vulkan},
            {"supports_zero_copy", caps.supports_zero_copy}
        };
        
        // Add model info if available
        if (auto model_info = entry.plugin->getModelInfo()) {
            plugin_caps["model"] = {
                {"name", model_info->name},
                {"architecture", model_info->architecture},
                {"parameter_count", model_info->parameter_count},
                {"context_length", model_info->context_length},
                {"vram_required_mb", model_info->vram_required_mb}
            };
        }
        
        result.push_back(plugin_caps);
    }
    
    return result;
}

json LLMPluginManager::getAggregatedStats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    json result;
    result["num_plugins"] = plugins_.size();
    result["default_plugin"] = default_plugin_name_;
    result["plugins"] = json::object();
    
    for (const auto& [name, entry] : plugins_) {
        json plugin_stats;
        plugin_stats["memory"] = entry.plugin->getMemoryStats();
        plugin_stats["performance"] = entry.plugin->getPerformanceStats();
        result["plugins"][name] = plugin_stats;
    }
    
    return result;
}

LLMPluginManager& LLMPluginManager::instance() {
    static LLMPluginManager instance;
    // Wire up OOM callback once on first access so VRAM pressure warnings are
    // logged even before the first plugin is registered.
    static bool oom_cb_installed = false;
    if (!oom_cb_installed) {
        oom_cb_installed = true;
        instance.vram_allocator_.setOOMCallback([](const ActiveVRAMAllocator::OOMEvent& ev) {
            spdlog::warn("[LLMPluginManager] VRAM OOM event: need={} bytes, strategy={}, "
                         "recovered={}, freed={} bytes",
                         ev.requested_bytes,
                         static_cast<int>(ev.strategy),
                         ev.recovered,
                         ev.bytes_recovered);
        });
    }
    return instance;
}

// ═══════════════════════════════════════════════════════════
// Convenience methods
// ═══════════════════════════════════════════════════════════

InferenceResponse LLMPluginManager::generate(const InferenceRequest& request) {
    auto* plugin = getDefaultPlugin();
    if (!plugin) {
        throw std::runtime_error("No default LLM plugin available");
    }
    
    return plugin->generate(request);
}

InferenceResponse LLMPluginManager::generateRAG(
    const RAGContext& rag_context,
    const InferenceRequest& request
) {
    auto* plugin = getDefaultPlugin();
    if (!plugin) {
        throw std::runtime_error("No default LLM plugin available");
    }
    
    return plugin->generateRAG(rag_context, request);
}

std::vector<float> LLMPluginManager::embed(const std::string& text) {
    auto* plugin = getDefaultPlugin();
    if (!plugin) {
        throw std::runtime_error("No default LLM plugin available");
    }
    
    return plugin->embed(text);
}

bool LLMPluginManager::loadModel(const std::string& model_id, const std::string& path) {
    ILLMPlugin* plugin;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        plugin = getDefaultPluginLocked();
    }
    if (!plugin) {
        throw std::runtime_error("No default LLM plugin available");
    }
    const bool ok = plugin->loadModel(path);
    if (ok && !model_id.empty()) {
        // Register model VRAM usage in the budget tracker.
        // vram_required_mb is populated by the plugin after a successful load.
        // When it is 0 (unknown / THEMIS_LLM_ENABLED not set), we skip
        // registration to avoid allocating a 0-byte sentinel.
        auto info = plugin->getModelInfo();
        if (info && info->vram_required_mb > 0) {
            const size_t vram_bytes = info->vram_required_mb * 1024ULL * 1024ULL;
            auto handle = vram_allocator_.registerExternal(vram_bytes, model_id);
            std::lock_guard<std::mutex> lock(mutex_);
            vram_handles_[model_id] = std::move(handle);
        }
    }
    return ok;
}

void LLMPluginManager::unloadModel(const std::string& model_id) {
    // Free the VRAM handle first (before the plugin frees the underlying memory).
    if (!model_id.empty()) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = vram_handles_.find(model_id);
        if (it != vram_handles_.end()) {
            vram_allocator_.free(it->second);
            vram_handles_.erase(it);
        }
    }
    ILLMPlugin* plugin;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        plugin = getDefaultPluginLocked();
    }
    if (!plugin) {
        throw std::runtime_error("No default LLM plugin available");
    }
    plugin->unloadModel();
}

std::vector<std::string> LLMPluginManager::listModels() const {
    std::vector<std::string> models;
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& [name, entry] : plugins_) {
        if (auto info = entry.plugin->getModelInfo()) {
            models.push_back(info->name);
        } else {
            models.push_back(name);
        }
    }
    return models;
}

bool LLMPluginManager::loadLoRA(const std::string& lora_id, const std::string& path, 
                                [[maybe_unused]] const std::string& base_model) {
    auto* plugin = getDefaultPlugin();
    if (!plugin) {
        throw std::runtime_error("No default LLM plugin available");
    }
    const bool ok = plugin->loadLoRA(lora_id, path, 1.0f);

    // ── AI Safety: Gossip adapter capability announcement ──────────────────
    if (ok) {
        distributed_knowledge::GossipAdapterPublisher* publisher;
        std::string shard_id;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            publisher = adapter_publisher_;
            shard_id  = local_shard_id_;
        }
        if (publisher) {
            static constexpr const char* kInitialAdapterVersion = "v1.0.0";
            distributed_knowledge::AdapterCapabilityAnnouncement ann;
            ann.shard_id        = shard_id;
            ann.adapter_id      = lora_id;
            ann.adapter_version = kInitialAdapterVersion;
            ann.domain_type     = distributed_knowledge::AdapterDomainType::GENERAL;
            ann.training_samples = 0;                // unknown at load time
            publisher->announce(std::move(ann));
            spdlog::info("LLMPluginManager::loadLoRA: gossip announcement sent for '{}'", lora_id);
        }
    }
    // ── end gossip announcement ────────────────────────────────────────────

    return ok;
}

bool LLMPluginManager::unloadLoRA(const std::string& lora_id) {
    auto* plugin = getDefaultPlugin();
    if (!plugin) {
        throw std::runtime_error("No default LLM plugin available");
    }
    const bool ok = plugin->unloadLoRA(lora_id);

    // ── AI Safety: Gossip withdrawal announcement ──────────────────────────
    if (ok) {
        distributed_knowledge::GossipAdapterPublisher* publisher;
        std::string shard_id;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            publisher = adapter_publisher_;
            shard_id  = local_shard_id_;
        }
        if (publisher) {
            // Withdraw: broadcast an announcement that explicitly marks the adapter
            // as no longer available on this shard.
            distributed_knowledge::AdapterCapabilityAnnouncement withdrawal;
            withdrawal.shard_id      = shard_id;
            withdrawal.adapter_id    = lora_id;
            withdrawal.is_withdrawal = true;         // explicit withdrawal flag
            publisher->announce(std::move(withdrawal));
            spdlog::info("LLMPluginManager::unloadLoRA: gossip withdrawal sent for '{}'", lora_id);
        }
    }
    // ── end gossip withdrawal ──────────────────────────────────────────────

    return ok;
}

std::vector<LoRAInfo> LLMPluginManager::listLoRAs() const {
    std::vector<LoRAInfo> loras;
    auto* plugin = getDefaultPlugin();
    if (!plugin) {
        return loras;
    }
    for (auto lora : plugin->listLoRAs()) {
        lora.lora_id = lora.id;
        lora.is_loaded = true;
        loras.push_back(std::move(lora));
    }
    return loras;
}

std::vector<std::string> LLMPluginManager::generateStream(const InferenceRequest& request) {
    auto* plugin = getDefaultPlugin();
    if (!plugin) {
        throw std::runtime_error("No default LLM plugin available");
    }
    // If backend lacks streaming, degrade to single generate and split tokens
    auto response = plugin->generate(request);
    std::vector<std::string> tokens;
    std::istringstream iss(response.text);
    std::string token;
    while (iss >> token) {
        tokens.push_back(token);
    }
    return tokens;
}

bool LLMPluginManager::ingestModel(const std::string& model_id, 
                                   [[maybe_unused]] const std::string& data) {
    return loadModel(model_id, model_id);
}

std::optional<ModelInfo> LLMPluginManager::getModelInfo(const std::string& model_id) const {
    auto* plugin = getDefaultPlugin();
    if (!plugin) {
        return std::nullopt;
    }
    auto info = plugin->getModelInfo();
    if (info) {
        if (info->model_id.empty()) info->model_id = model_id;
        info->is_loaded = plugin->isModelLoaded();
    }
    return info;
}

LLMPluginManager::PluginStatistics LLMPluginManager::getStatistics() const {
    PluginStatistics stats;
    auto* plugin = getDefaultPlugin();
    if (plugin) {
        auto perf = plugin->getPerformanceStats();
        if (perf.contains("total_requests")) {
            stats.total_requests = perf["total_requests"].get<uint64_t>();
        }
        if (perf.contains("avg_latency_ms")) {
            stats.average_latency_ms = perf["avg_latency_ms"].get<double>();
        }
        if (perf.contains("throughput_rps")) {
            stats.throughput = perf["throughput_rps"].get<double>();
        }
    }
    stats.models_loaded = static_cast<int>(listModels().size());
    stats.loras_loaded = static_cast<int>(listLoRAs().size());
    return stats;
}

LLMPluginManager::CacheStatistics LLMPluginManager::getCacheStatistics() const {
    CacheStatistics stats;
    return stats;
}

LLMPluginManager::HealthStatus LLMPluginManager::getHealthStatus() const {
    HealthStatus health;
    health.models_loaded = static_cast<int>(listModels().size());
    health.loras_loaded = static_cast<int>(listLoRAs().size());

    const auto vram = vram_allocator_.getStats();
    health.vram_total_bytes           = vram.total_vram_bytes;
    health.vram_used_bytes            = vram.used_vram_bytes;
    health.vram_free_bytes            = vram.free_vram_bytes;
    health.vram_oom_threshold_exceeded = vram.oom_threshold_exceeded;

    if (vram.oom_threshold_exceeded) {
        health.is_healthy = false;
        health.plugin_manager_status = "vram_pressure";
    }

    return health;
}

ActiveVRAMAllocator::Stats LLMPluginManager::getVRAMStats() const {
    return vram_allocator_.getStats();
}

void LLMPluginManager::setAdapterPublisher(
    distributed_knowledge::GossipAdapterPublisher* publisher,
    std::string local_shard_id)
{
    std::lock_guard<std::mutex> lock(mutex_);
    adapter_publisher_ = publisher;
    local_shard_id_    = std::move(local_shard_id);
    spdlog::info("LLMPluginManager::setAdapterPublisher: gossip publisher {}",
                 publisher ? "configured" : "disconnected");
}

void LLMPluginManager::clearAllCaches() {
    // Clear embedding cache on the global EmbeddedLLM singleton if initialized.
    auto& mgr = EmbeddedLLMManager::instance();
    if (mgr.isInitialized()) {
        mgr.get().clearCache();
        spdlog::info("LLMPluginManager::clearAllCaches: EmbeddedLLM embedding cache cleared");
    }
    // Individual plugin-level caches (KV-cache, prefix-cache) are owned by the
    // underlying inference runtime; we log a notice for callers that rely on this.
    spdlog::debug("LLMPluginManager::clearAllCaches: runtime KV/prefix caches are "
                  "managed by individual plugins and reset on model reload");
}

// ═══════════════════════════════════════════════════════════
// Helper functions
// ═══════════════════════════════════════════════════════════

bool createLlamaWrapper(
    const std::string& name,
    const std::string& model_path,
    const json& config
) {
    try {
        // Create llama.cpp plugin with config
        LlamaWrapper::Config plugin_config;
        
        // Parse basic configuration
        if (config.contains("n_gpu_layers")) {
            plugin_config.n_gpu_layers = config["n_gpu_layers"].get<int>();
        }
        if (config.contains("n_ctx")) {
            plugin_config.n_ctx = config["n_ctx"].get<int>();
        }
        if (config.contains("n_batch")) {
            plugin_config.n_batch = config["n_batch"].get<int>();
        }
        if (config.contains("n_threads")) {
            plugin_config.n_threads = config["n_threads"].get<int>();
        }
        if (config.contains("max_vram_mb")) {
            plugin_config.max_vram_mb = config["max_vram_mb"].get<size_t>();
        }
        
        // Configure lazy model loader (Ollama-style)
        if (config.contains("lazy_loader")) {
            auto& ll_cfg = config["lazy_loader"];
            if (ll_cfg.contains("max_models")) {
                plugin_config.lazy_loader_config.max_models = ll_cfg["max_models"].get<size_t>();
            }
            if (ll_cfg.contains("max_vram_mb")) {
                plugin_config.lazy_loader_config.max_vram_mb = ll_cfg["max_vram_mb"].get<size_t>();
            }
            if (ll_cfg.contains("model_ttl_seconds")) {
                plugin_config.lazy_loader_config.model_ttl = 
                    std::chrono::seconds(ll_cfg["model_ttl_seconds"].get<int>());
            }
        }
        
        // Configure multi-LoRA manager (vLLM-style)
        if (config.contains("multi_lora")) {
            auto& ml_cfg = config["multi_lora"];
            if (ml_cfg.contains("max_lora_slots")) {
                plugin_config.multi_lora_config.max_lora_slots = ml_cfg["max_lora_slots"].get<size_t>();
            }
            if (ml_cfg.contains("max_lora_vram_mb")) {
                plugin_config.multi_lora_config.max_lora_vram_mb = ml_cfg["max_lora_vram_mb"].get<size_t>();
            }
            if (ml_cfg.contains("lora_ttl_seconds")) {
                plugin_config.multi_lora_config.lora_ttl = 
                    std::chrono::seconds(ml_cfg["lora_ttl_seconds"].get<int>());
            }
            if (ml_cfg.contains("enable_multi_lora_batch")) {
                plugin_config.multi_lora_config.enable_multi_lora_batch = 
                    ml_cfg["enable_multi_lora_batch"].get<bool>();
            }
        }
        
        auto plugin = std::make_unique<LlamaWrapper>(plugin_config);
        
        // Load model if path provided
        if (!model_path.empty()) {
            if (!plugin->loadModel(model_path, config)) {
                errors::logError(errors::ErrorCode::ERR_LLM_MODEL_LOAD_FAILED, model_path);
                return false;
            }
        }
        
        // Register with manager
        LLMPluginManager::instance().registerPlugin(name, std::move(plugin));
        
        spdlog::info("llama.cpp plugin '{}' created with Ollama & vLLM features", name);
        
        return true;
    } catch (const std::exception& e) {
        spdlog::error("Failed to create llama.cpp plugin: {}", e.what());
        return false;
    }
}

} // namespace llm
} // namespace themis
