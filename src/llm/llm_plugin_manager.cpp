#include "llm/llm_plugin_manager.h"
#include "llm/llamacpp_plugin.h"
#include <spdlog/spdlog.h>
#include <stdexcept>

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

// ═══════════════════════════════════════════════════════════
// Helper functions
// ═══════════════════════════════════════════════════════════

bool createLlamaCppPlugin(
    const std::string& name,
    const std::string& model_path,
    const json& config
) {
    try {
        // Create llama.cpp plugin with config
        LlamaCppPlugin::Config plugin_config;
        
        // Parse configuration
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
        if (config.contains("lora_cache_slots")) {
            plugin_config.lora_cache_slots = config["lora_cache_slots"].get<int>();
        }
        
        auto plugin = std::make_unique<LlamaCppPlugin>(plugin_config);
        
        // Load model if path provided
        if (!model_path.empty()) {
            if (!plugin->loadModel(model_path, config)) {
                spdlog::error("Failed to load model: {}", model_path);
                return false;
            }
        }
        
        // Register with manager
        LLMPluginManager::instance().registerPlugin(name, std::move(plugin));
        
        return true;
    } catch (const std::exception& e) {
        spdlog::error("Failed to create llama.cpp plugin: {}", e.what());
        return false;
    }
}

} // namespace llm
} // namespace themis
