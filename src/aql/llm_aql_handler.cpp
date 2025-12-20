#include "aql/llm_aql_handler.h"
#include "llm/llm_plugin_manager.h"
#include <stdexcept>
#include <sstream>

namespace themis {
namespace aql {

class LLMAQLHandler::Impl {
public:
    Impl() = default;
    
    llm::LLMPluginManager& getPluginManager() {
        return llm::LLMPluginManager::instance();
    }
};

LLMAQLHandler::LLMAQLHandler() 
    : impl_(std::make_unique<Impl>()) {}

LLMAQLHandler::~LLMAQLHandler() = default;

std::string LLMAQLHandler::executeInfer(
    const std::string& prompt,
    const std::string& model_id,
    const std::string& lora_id,
    const std::unordered_map<std::string, std::string>& options
) {
    try {
        auto& plugin_mgr = impl_->getPluginManager();
        
        llm::InferenceRequest request;
        request.prompt = prompt;
        request.model_id = model_id.empty() ? "default" : model_id;
        request.lora_adapter_id = lora_id;
        
        // Parse options
        if (options.count("max_tokens")) {
            request.max_tokens = std::stoi(options.at("max_tokens"));
        }
        if (options.count("temperature")) {
            request.temperature = std::stof(options.at("temperature"));
        }
        if (options.count("top_p")) {
            request.top_p = std::stof(options.at("top_p"));
        }
        
        auto response = plugin_mgr.generate(request);
        return response.text;
        
    } catch (const std::exception& e) {
        throw std::runtime_error(
            std::string("LLM INFER failed: ") + e.what()
        );
    }
}

std::string LLMAQLHandler::executeRAG(
    const std::string& query,
    const std::string& collection,
    int top_k,
    const std::string& lora_id,
    const std::unordered_map<std::string, std::string>& options
) {
    try {
        auto& plugin_mgr = impl_->getPluginManager();
        
        // TODO: Integrate with vector search to retrieve documents
        // For now, create empty context
        llm::RAGContext context;
        context.query = query;
        context.collection_name = collection;
        context.top_k = top_k;
        
        llm::InferenceRequest request;
        request.prompt = query;
        request.lora_adapter_id = lora_id;
        
        // Parse options
        if (options.count("max_tokens")) {
            request.max_tokens = std::stoi(options.at("max_tokens"));
        }
        if (options.count("similarity_threshold")) {
            // Store for vector search
        }
        
        auto response = plugin_mgr.generateRAG(context, request);
        return response.text;
        
    } catch (const std::exception& e) {
        throw std::runtime_error(
            std::string("LLM RAG failed: ") + e.what()
        );
    }
}

std::vector<float> LLMAQLHandler::executeEmbed(
    const std::string& text,
    const std::string& model_id
) {
    try {
        auto& plugin_mgr = impl_->getPluginManager();
        (void)model_id;
        return plugin_mgr.embed(text);
        
    } catch (const std::exception& e) {
        throw std::runtime_error(
            std::string("LLM EMBED failed: ") + e.what()
        );
    }
}

void LLMAQLHandler::executeModelLoad(
    const std::string& model_id,
    const std::string& path
) {
    try {
        auto& plugin_mgr = impl_->getPluginManager();
        plugin_mgr.loadModel(model_id, path);
    } catch (const std::exception& e) {
        throw std::runtime_error(
            std::string("LLM MODEL LOAD failed: ") + e.what()
        );
    }
}

void LLMAQLHandler::executeModelUnload(const std::string& model_id) {
    try {
        auto& plugin_mgr = impl_->getPluginManager();
        plugin_mgr.unloadModel(model_id);
    } catch (const std::exception& e) {
        throw std::runtime_error(
            std::string("LLM MODEL UNLOAD failed: ") + e.what()
        );
    }
}

std::vector<std::string> LLMAQLHandler::executeModelList() {
    try {
        auto& plugin_mgr = impl_->getPluginManager();
        return plugin_mgr.listModels();
    } catch (const std::exception& e) {
        throw std::runtime_error(
            std::string("LLM MODEL LIST failed: ") + e.what()
        );
    }
}

void LLMAQLHandler::executeModelIngest(
    const std::string& model_id,
    const std::string& blob_urn
) {
    try {
        auto& plugin_mgr = impl_->getPluginManager();
        plugin_mgr.loadModel(model_id, blob_urn);
    } catch (const std::exception& e) {
        throw std::runtime_error(
            std::string("LLM MODEL INGEST failed: ") + e.what()
        );
    }
}

void LLMAQLHandler::executeLoRALoad(
    const std::string& lora_id,
    const std::string& path
) {
    try {
        auto& plugin_mgr = impl_->getPluginManager();
        plugin_mgr.loadLoRA(lora_id, path, "default");
    } catch (const std::exception& e) {
        throw std::runtime_error(
            std::string("LLM LORA LOAD failed: ") + e.what()
        );
    }
}

void LLMAQLHandler::executeLoRAUnload(const std::string& lora_id) {
    try {
        auto& plugin_mgr = impl_->getPluginManager();
        plugin_mgr.unloadLoRA(lora_id);
    } catch (const std::exception& e) {
        throw std::runtime_error(
            std::string("LLM LORA UNLOAD failed: ") + e.what()
        );
    }
}

std::vector<std::string> LLMAQLHandler::executeLoRAList() {
    try {
        auto& plugin_mgr = impl_->getPluginManager();
        std::vector<std::string> ids;
        for (const auto& lora : plugin_mgr.listLoRAs()) {
            ids.push_back(lora.lora_id.empty() ? lora.id : lora.lora_id);
        }
        return ids;
    } catch (const std::exception& e) {
        throw std::runtime_error(
            std::string("LLM LORA LIST failed: ") + e.what()
        );
    }
}

std::string LLMAQLHandler::executeStats() {
    try {
        auto& plugin_mgr = impl_->getPluginManager();
        auto stats = plugin_mgr.getStatistics();
        
        std::ostringstream oss;
        oss << "LLM Statistics:\n";
        oss << "  Models loaded: " << stats.models_loaded << "\n";
        oss << "  LoRAs loaded: " << stats.loras_loaded << "\n";
        oss << "  Total requests: " << stats.total_requests << "\n";
        oss << "  Average latency: " << stats.average_latency_ms << " ms\n";
        oss << "  Throughput: " << stats.throughput << " req/s\n";
        
        return oss.str();
    } catch (const std::exception& e) {
        throw std::runtime_error(
            std::string("LLM STATS failed: ") + e.what()
        );
    }
}

std::string LLMAQLHandler::executeCacheStats() {
    try {
        auto& plugin_mgr = impl_->getPluginManager();
        auto stats = plugin_mgr.getCacheStatistics();
        
        std::ostringstream oss;
        oss << "LLM Cache Statistics:\n";
        oss << "  Response cache hits: " << stats.response_cache_hits << "\n";
        oss << "  Response cache misses: " << stats.response_cache_misses << "\n";
        oss << "  Response cache entries: " << stats.response_cache_entries << "\n";
        oss << "  Response cache hit rate: " << stats.response_cache_hit_rate << "\n";
        oss << "  Prefix cache hits: " << stats.prefix_cache_hits << "\n";
        oss << "  Prefix cache misses: " << stats.prefix_cache_misses << "\n";
        oss << "  Prefix cache entries: " << stats.prefix_cache_entries << "\n";
        oss << "  Prefix cache hit rate: " << stats.prefix_cache_hit_rate << "\n";
        
        return oss.str();
    } catch (const std::exception& e) {
        throw std::runtime_error(
            std::string("LLM CACHE STATS failed: ") + e.what()
        );
    }
}

void LLMAQLHandler::executeCacheClear() {
    try {
        auto& plugin_mgr = impl_->getPluginManager();
        plugin_mgr.clearAllCaches();
    } catch (const std::exception& e) {
        throw std::runtime_error(
            std::string("LLM CACHE CLEAR failed: ") + e.what()
        );
    }
}

std::vector<std::string> LLMAQLHandler::executeBatchInfer(
    const std::vector<BatchInferRequest>& requests
) {
    try {
        auto& plugin_mgr = impl_->getPluginManager();
        std::vector<std::string> results;
        results.reserve(requests.size());
        
        // Batch optimization: group requests by model/lora
        // For simplicity, execute sequentially for now
        // TODO: Implement true batch inference
        for (const auto& req : requests) {
            llm::InferenceRequest inf_req;
            inf_req.prompt = req.prompt;
            inf_req.model_id = req.model_id.empty() ? "default" : req.model_id;
            inf_req.lora_adapter_id = req.lora_id;
            
            // Parse options
            if (req.options.count("max_tokens")) {
                inf_req.max_tokens = std::stoi(req.options.at("max_tokens"));
            }
            
            auto response = plugin_mgr.generate(inf_req);
            results.push_back(response.text);
        }
        
        return results;
    } catch (const std::exception& e) {
        throw std::runtime_error(
            std::string("Batch LLM INFER failed: ") + e.what()
        );
    }
}

} // namespace aql
} // namespace themis
