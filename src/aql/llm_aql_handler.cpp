#include "aql/llm_aql_handler.h"
#include "llm/llm_plugin_manager.h"
#include "llm/embedded_llm.h"
#include "llm/llama_wrapper.h"
#include "index/vector_index.h"
#include <stdexcept>
#include <sstream>
#include <algorithm>

namespace themis {
namespace aql {

class LLMAQLHandler::Impl {
public:
    Impl() = default;
    
    llm::LLMPluginManager& getPluginManager() {
        return llm::LLMPluginManager::instance();
    }
    
    // Store optional vector index manager for RAG queries
    VectorIndexManager* vector_index_mgr_ = nullptr;
    
    // Default configuration constants
    static constexpr float DEFAULT_SIMILARITY_THRESHOLD = 0.7f;
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
        
        // Build inference request with model and LoRA selection
        llm::InferenceRequest request;
        request.prompt = prompt;
        
        // Set model if specified
        if (!model_id.empty()) {
            request.model_id = model_id;
        }
        
        // Set LoRA adapter if specified
        if (!lora_id.empty()) {
            request.lora_adapter_id = lora_id;
        }
        
        // Parse options for generation parameters
        if (options.count("max_tokens")) {
            request.max_tokens = std::stoi(options.at("max_tokens"));
        }
        if (options.count("temperature")) {
            request.temperature = std::stof(options.at("temperature"));
        }
        if (options.count("top_p")) {
            request.top_p = std::stof(options.at("top_p"));
        }
        if (options.count("top_k")) {
            request.top_k = std::stoi(options.at("top_k"));
        }
        if (options.count("repetition_penalty")) {
            request.repetition_penalty = std::stof(options.at("repetition_penalty"));
        }
        
        // Execute via plugin manager
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
        
        // Build RAG context with vector search integration
        llm::RAGContext context;
        context.query = query;
        context.collection_name = collection;
        context.top_k = top_k;
        
        // If vector index manager is available, perform similarity search
        if (impl_->vector_index_mgr_) {
            try {
                // Generate query embedding
                auto query_embedding = THEMIS_LLM_EMBED(query);
                
                // Search for similar documents
                float similarity_threshold = Impl::DEFAULT_SIMILARITY_THRESHOLD;
                if (options.count("similarity_threshold")) {
                    similarity_threshold = std::stof(options.at("similarity_threshold"));
                }
                
                auto [status, results] = impl_->vector_index_mgr_->searchKnn(
                    query_embedding,
                    top_k
                );
                
                if (status.ok) {
                    // Retrieve documents and build context
                    for (const auto& result : results) {
                        // Convert distance metric to similarity score
                        // For COSINE/L2 metrics: lower distance = higher similarity
                        float similarity = 1.0f - result.distance;
                        
                        // Filter by similarity threshold
                        if (similarity >= similarity_threshold) {
                            llm::RAGContext::Document doc;
                            doc.source = result.pk;
                            doc.relevance_score = similarity;
                            // Note: Content would need to be fetched from storage
                            // For now, we'll use the pk as content placeholder
                            doc.content = result.pk;
                            context.documents.push_back(doc);
                        }
                    }
                }
            } catch (const std::exception& /* e */) {
                // Log error but continue with empty context
                // In production, this would be logged properly
            }
        }
        
        // Build inference request with RAG context
        llm::InferenceRequest request;
        request.prompt = query;
        
        // Set LoRA adapter if specified
        if (!lora_id.empty()) {
            request.lora_adapter_id = lora_id;
        }
        
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
        
        // Execute RAG query
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
        
        // If model_id is specified, use plugin manager for model-specific embedding
        if (!model_id.empty()) {
            // Build request for specific model
            llm::InferenceRequest request;
            request.prompt = text;
            request.model_id = model_id;
            
            // Note: Plugin manager would need an embedWithModel method
            // For now, fall back to default embedding
        }
        
        // Use simplified EmbeddedLLM API for default embedding
        auto embedding = THEMIS_LLM_EMBED(text);
        return embedding;
        
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

std::string LLMAQLHandler::translateNLToAQL(
    const std::string& nl_query,
    const std::string& schema_context
) {
    try {
        // Build system prompt for AQL translation
        std::ostringstream system_prompt;
        system_prompt << "You are an expert in AQL (Application Query Language) for ThemisDB.\n";
        system_prompt << "ThemisDB AQL is based on ArangoDB's AQL but extended with additional features.\n\n";
        
        // Add schema context if provided
        if (!schema_context.empty()) {
            system_prompt << "Database schema:\n" << schema_context << "\n\n";
        } else {
            // Default schema description
            system_prompt << "ThemisDB is a distributed graph database with AQL support.\n";
            system_prompt << "Common collections: documents, nodes, edges, users, etc.\n";
            system_prompt << "Graph structures use edges to connect nodes.\n\n";
        }
        
        system_prompt << "Your task: Convert natural language queries to valid AQL.\n";
        system_prompt << "Requirements:\n";
        system_prompt << "- Return ONLY the AQL query, no explanations or markdown\n";
        system_prompt << "- Use proper AQL syntax (FOR, FILTER, SORT, LIMIT, RETURN)\n";
        system_prompt << "- Handle graph traversals with proper edge syntax if needed\n";
        system_prompt << "- Optimize for performance\n\n";
        
        // Build user prompt
        std::ostringstream user_prompt;
        user_prompt << "Natural language query: " << nl_query << "\n\n";
        user_prompt << "Generate the corresponding AQL query:";
        
        // Create chat messages for better context
        std::vector<llm::ChatMessage> messages;
        messages.emplace_back("system", system_prompt.str());
        messages.emplace_back("user", user_prompt.str());
        
        // Use chat interface for better results
        auto response = executeChat(messages);
        
        // Clean up response - remove markdown code blocks if present
        std::string aql_query = response;
        
        // Remove ```aql or ``` markers
        size_t start_marker = aql_query.find("```");
        if (start_marker != std::string::npos) {
            // Find the actual start of the query (after ```aql or ```)
            size_t query_start = aql_query.find('\n', start_marker);
            if (query_start != std::string::npos) {
                query_start++;
                // Find end marker
                size_t end_marker = aql_query.find("```", query_start);
                if (end_marker != std::string::npos) {
                    aql_query = aql_query.substr(query_start, end_marker - query_start);
                }
            }
        }
        
        // Trim whitespace
        auto trim = [](std::string& s) {
            s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
                return !std::isspace(ch);
            }));
            s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
                return !std::isspace(ch);
            }).base(), s.end());
        };
        trim(aql_query);
        
        return aql_query;
        
    } catch (const std::exception& e) {
        throw std::runtime_error(
            std::string("NL to AQL translation failed: ") + e.what()
        );
    }
}

std::string LLMAQLHandler::executeChat(
    const std::vector<llm::ChatMessage>& messages,
    const std::string& model_id,
    const std::unordered_map<std::string, std::string>& options
) {
    try {
        // Use EmbeddedLLM chat interface
        auto& llm = llm::EmbeddedLLMManager::instance().get();
        
        // Note: EmbeddedLLM's chat() doesn't directly support custom parameters
        // We can use generateWithParams for the formatted chat prompt instead
        
        // Determine chat format from options or use default
        llm::ChatFormat format = llm::ChatFormat::ChatML;
        if (options.count("chat_format")) {
            const auto& fmt = options.at("chat_format");
            if (fmt == "llama2") format = llm::ChatFormat::Llama2;
            else if (fmt == "alpaca") format = llm::ChatFormat::Alpaca;
            else if (fmt == "vicuna") format = llm::ChatFormat::Vicuna;
        }
        
        // Note: model_id selection would require extending EmbeddedLLM API
        // For now, use the default model
        (void)model_id;
        
        // If we have custom parameters, we might need to use a different approach
        // For now, use the standard chat method with default parameters
        auto response = llm.chat(messages, format);
        return response;
        
    } catch (const std::exception& e) {
        throw std::runtime_error(
            std::string("LLM CHAT failed: ") + e.what()
        );
    }
}

} // namespace aql
} // namespace themis
