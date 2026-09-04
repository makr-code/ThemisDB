/**
 * @file base_model_adapter.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=3, H=3, M=9, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "llm/lora_framework/base_model_adapter.h"
#include <spdlog/spdlog.h>
#include <fmt/format.h>
#include <algorithm>
#include <regex>
#include <cmath>

namespace themis {
namespace llm {
namespace lora {

// ===== BaseModelAdapter Implementation =====

BaseModelAdapter::BaseModelAdapter() 
    : model_loaded_(false) {
}

BaseModelAdapter::~BaseModelAdapter() {
    unload();
}

bool BaseModelAdapter::loadModel(const std::string& model_path) {
    spdlog::info("Loading base model from: {}", model_path);
    
    // Initialize GGUF loader
    gguf_loader_ = std::make_unique<GGUFLoader>();
    
    // Parse GGUF file
    if (!gguf_loader_->parseFile(model_path)) {
        spdlog::error("Failed to parse GGUF file: {}", model_path);
        return false;
    }
    
    model_path_ = model_path;
    
    // Extract model name from path
    size_t last_slash = model_path.find_last_of("/\\");
    model_name_ = (last_slash != std::string::npos) 
        ? model_path.substr(last_slash + 1) 
        : model_path;
    
    // Parse architecture from GGUF metadata
    if (!parseArchitecture()) {
        spdlog::error("Failed to parse model architecture");
        return false;
    }
    
    // Identify layers suitable for LoRA adaptation
    if (!identifyAdaptableLayers()) {
        spdlog::error("Failed to identify adaptable layers");
        return false;
    }
    
    model_loaded_ = true;
    
    spdlog::info("Base model loaded successfully:");
    spdlog::info("  Architecture: {}", architecture_.architecture);
    spdlog::info("  Layers: {}", architecture_.num_layers);
    spdlog::info("  Hidden size: {}", architecture_.hidden_size);
    spdlog::info("  Adaptable layers: {}",static_cast<int>(adaptable_layers_.size()));
    
    return true;
}

bool BaseModelAdapter::parseArchitecture() {
    const auto& metadata = gguf_loader_->getMetadata();
    
    // Extract architecture type
    architecture_.architecture = metadata.architecture;
    
    // Parse common architecture parameters from GGUF metadata
    const auto& config = metadata.config;
    
    // Number of layers
    if (config.find("num_layers") != config.end()) {
        architecture_.num_layers = std::stoi(config.at("num_layers"));
    } else if (config.find("n_layer") != config.end()) {
        architecture_.num_layers = std::stoi(config.at("n_layer"));
    } else {
        // Try to infer from tensor names
        int max_layer = 0;
        std::regex layer_pattern(R"(layers?\.(\d+)\.)");
        for (const auto& tensor : metadata.tensors) {
            std::smatch match = {};
            if (std::regex_search(tensor.name, match, layer_pattern)) {
                int layer_idx = std::stoi(match[1].str());
                max_layer = std::max(max_layer, layer_idx);
            }
        }
        architecture_.num_layers = max_layer + 1;
    }
    
    // Hidden size
    if (config.find("hidden_size") != config.end()) {
        architecture_.hidden_size = std::stoi(config.at("hidden_size"));
    } else if (config.find("n_embd") != config.end()) {
        architecture_.hidden_size = std::stoi(config.at("n_embd"));
    } else {
        // Default for common models
        architecture_.hidden_size = 4096;
    }
    
    // Attention heads
    if (config.find("num_attention_heads") != config.end()) {
        architecture_.num_attention_heads = std::stoi(config.at("num_attention_heads"));
    } else if (config.find("n_head") != config.end()) {
        architecture_.num_attention_heads = std::stoi(config.at("n_head"));
    } else {
        architecture_.num_attention_heads = 32;
    }
    
    // Intermediate (MLP) size
    if (config.find("intermediate_size") != config.end()) {
        architecture_.intermediate_size = std::stoi(config.at("intermediate_size"));
    } else if (config.find("n_inner") != config.end()) {
        architecture_.intermediate_size = std::stoi(config.at("n_inner"));
    } else {
        // Default: 4x hidden size (common for transformers)
        architecture_.intermediate_size = architecture_.hidden_size * 4;
    }
    
    // Vocabulary size
    if (config.find("vocab_size") != config.end()) {
        architecture_.vocab_size = std::stoi(config.at("vocab_size"));
    } else if (config.find("n_vocab") != config.end()) {
        architecture_.vocab_size = std::stoi(config.at("n_vocab"));
    } else {
        // Try to infer from embedding tensor shape
        architecture_.vocab_size = 32000;  // Default for Llama models
    }
    
    // RoPE parameters
    if (config.find("rope_freq_base") != config.end()) {
        architecture_.rope_freq_base = std::stof(config.at("rope_freq_base"));
    } else {
        architecture_.rope_freq_base = 10000.0f;
    }
    
    if (config.find("rope_scaling_type") != config.end()) {
        architecture_.rope_scaling_type = config.at("rope_scaling_type");
    } else {
        architecture_.rope_scaling_type = "none";
    }
    
    return true;
}

bool BaseModelAdapter::identifyAdaptableLayers() {
    const auto& metadata = gguf_loader_->getMetadata();
    
    adaptable_layers_.clear();
    layer_map_.clear();
    
    // Pattern for layer names: supports Llama, Mistral, GPT-NeoX, etc.
    // Llama/Mistral: layers.N.attention.{wq,wk,wv,wo}, layers.N.feed_forward.{w1,w2,w3}
    // GPT-NeoX: gpt_neox.layers.N.attention.{query_key_value,dense}, gpt_neox.layers.N.mlp.{dense_h_to_4h,dense_4h_to_h}
    
    std::vector<std::regex> attention_patterns = {
        std::regex(R"(layers?\.(\d+)\.attention\.(wq|wk|wv|wo))"),
        std::regex(R"(layers?\.(\d+)\.self_attn\.(q_proj|k_proj|v_proj|o_proj))"),
        std::regex(R"(gpt_neox\.layers\.(\d+)\.attention\.(query_key_value|dense))"),
        std::regex(R"(transformer\.h\.(\d+)\.attn\.(c_attn|c_proj))"),
    };
    
    std::vector<std::regex> mlp_patterns = {
        std::regex(R"(layers?\.(\d+)\.feed_forward\.(w1|w2|w3))"),
        std::regex(R"(layers?\.(\d+)\.mlp\.(gate_proj|up_proj|down_proj))"),
        std::regex(R"(gpt_neox\.layers\.(\d+)\.mlp\.(dense_h_to_4h|dense_4h_to_h))"),
        std::regex(R"(transformer\.h\.(\d+)\.mlp\.(c_fc|c_proj))"),
    };
    
    for (const auto& tensor : metadata.tensors) {
        BaseLayerInfo layer_info;
        
        // Try to match attention layers
        bool matched = false;
        for (const auto& pattern : attention_patterns) {
            std::smatch match = {};
            if (std::regex_search(tensor.name, match, pattern)) {
                if (parseLayerInfo(tensor, layer_info)) {
                    layer_info.layer_idx = std::stoi(match[1].str());
                    layer_info.layer_type = "attention." + match[2].str();
                    matched = true;
                    break;
                }
            }
        }
        
        // Try to match MLP layers if not already matched
        if (!matched) {
            for (const auto& pattern : mlp_patterns) {
                std::smatch match = {};
                if (std::regex_search(tensor.name, match, pattern)) {
                    if (parseLayerInfo(tensor, layer_info)) {
                        layer_info.layer_idx = std::stoi(match[1].str());
                        layer_info.layer_type = "feed_forward." + match[2].str();
                        matched = true;
                        break;
                    }
                }
            }
        }
        
        if (matched) {
            adaptable_layers_.push_back(layer_info);
            layer_map_[layer_info.name] = layer_info;
        }
    }
    
    // Sort by layer index and type for consistent ordering
    std::sort(adaptable_layers_.begin(), adaptable_layers_.end(),
        [](const BaseLayerInfo& a, const BaseLayerInfo& b) {
            if (a.layer_idx != b.layer_idx) {
                return a.layer_idx < b.layer_idx;
            }
            return a.layer_type < b.layer_type;
        });
    
    return !adaptable_layers_.empty();
}

bool BaseModelAdapter::parseLayerInfo(const TensorMetadata& tensor, BaseLayerInfo& layer_info) {
    layer_info.name = tensor.name;
    layer_info.shape.clear();
    
    for (auto dim : tensor.shape) {
        layer_info.shape.push_back(static_cast<size_t>(dim));
    }
    
    // For 2D weight matrices: shape is typically [out_features, in_features] in GGUF
    if (static_cast<int>(layer_info.shape.size()) == 2) {
        layer_info.out_features = layer_info.shape[0];
        layer_info.in_features = layer_info.shape[1];
        return true;
    }
    
    // Skip non-2D tensors (embeddings, norms, etc.)
    return false;
}

std::vector<BaseLayerInfo> BaseModelAdapter::getAdaptableLayers() const {
    return adaptable_layers_;
}

std::vector<BaseLayerInfo> BaseModelAdapter::getLayersByTargetModules(
    const std::vector<std::string>& target_modules) const {
    
    std::vector<BaseLayerInfo> matching_layers;
    
    for (const auto& layer : adaptable_layers_) {
        for (const auto& target : target_modules) {
            if (matchesTargetModule(layer.layer_type, target)) {
                matching_layers.push_back(layer);
                break;
            }
        }
    }
    
    return matching_layers;
}

bool BaseModelAdapter::matchesTargetModule(const std::string& layer_name, 
                                          const std::string& target_pattern) const {
    // Simple pattern matching - can be extended with regex if needed
    // Supports patterns like "attention.wq", "attention.*", "feed_forward.w1"
    
    if (target_pattern.find('*') != std::string::npos) {
        // Wildcard matching
        std::string prefix = target_pattern.substr(0, target_pattern.find('*'));
        return layer_name.find(prefix) == 0;
    } else {
        // Exact match
        return layer_name == target_pattern;
    }
}

std::optional<Tensor> BaseModelAdapter::getLayerWeights(const std::string& layer_name) const {
    if (!model_loaded_) {
        spdlog::warn("Model not loaded");
        return std::nullopt;
    }
    
    // Get tensor data from GGUF loader
    try {
        auto tensor_data = gguf_loader_->getTensorData(layer_name);
        
        if (tensor_data.empty()) {
            return std::nullopt;
        }
        
        // Find layer info to get shape
        auto it = layer_map_.find(layer_name);
        if (it == layer_map_.end()) {
            return std::nullopt;
        }
        
        const auto& layer_info = it->second;
        
        // Create tensor with appropriate shape
        Tensor tensor(layer_info.shape);
        
        // Copy data (assuming float32 for now - can extend for quantized types)
        size_t expected_size = tensor.size() * sizeof(float);
        if (static_cast<int>(tensor_data.size()) > = expected_size) {
            const float* src = reinterpret_cast<const float*>(tensor_data.data());
            std::copy(src, src + tensor.size(), tensor.data().begin());
        } else {
            spdlog::warn("Tensor data size mismatch for layer: {}", layer_name);
            return std::nullopt;
        }
        
        return tensor;
        
    } catch (const std::exception& e) {
        spdlog::error("Failed to get layer weights for {}: {}", layer_name, e.what());
        return std::nullopt;
    }
}

size_t BaseModelAdapter::getTotalParameters() const {
    if (!model_loaded_) {
        return 0;
    }
    
    const auto& metadata = gguf_loader_->getMetadata();
    size_t total = 0;
    
    for (const auto& tensor : metadata.tensors) {
        size_t tensor_size = 1;
        for (auto dim : tensor.shape) {
            tensor_size *= static_cast<size_t>(dim);
        }
        total += tensor_size;
    }
    
    return total;
}

void BaseModelAdapter::unload() {
    if (gguf_loader_) {
        gguf_loader_.reset();
    }
    
    adaptable_layers_.clear();
    layer_map_.clear();
    embedding_cache_.clear();
    embedding_matrix_ = nullptr;
    embedding_tensor_name_.clear();
    model_loaded_ = false;
    
    spdlog::info("Base model unloaded");
}

std::string BaseModelAdapter::findEmbeddingTensorName() const {
    if (!gguf_loader_) {
        return "";
    }
    
    const auto& metadata = gguf_loader_->getMetadata();
    
    // NOTE: We're looking for the EMBEDDING LAYER weights (first layer),
    // NOT the output embeddings from llama_get_embeddings().
    // 
    // For LoRA training, we need raw token embeddings as inputs to layers,
    // not contextualized sequence embeddings from a full forward pass.
    //
    // Try different embedding tensor names based on architecture
    std::vector<std::string> possible_names = {
        "token_embd.weight",           // Common GGUF name
        "tok_embeddings.weight",       // Llama
        "model.embed_tokens.weight",   // HuggingFace format
        "embed_tokens.weight",         // Alternative
        "wte.weight",                  // GPT-style
        "transformer.wte.weight",      // GPT-2
        "gpt_neox.embed_in.weight",    // GPT-NeoX
    };
    
    for (const auto& name : possible_names) {
        for (const auto& tensor : metadata.tensors) {
            if (tensor.name == name || tensor.name.find(name) != std::string::npos) {
                return tensor.name;
            }
        }
    }
    
    // If not found, try to find any tensor with "embed" in the name
    for (const auto& tensor : metadata.tensors) {
        if (tensor.name.find("embed") != std::string::npos && 
            tensor.name.find("tok") != std::string::npos) {
            spdlog::info("Found potential embedding tensor: {}", tensor.name);
            return tensor.name;
        }
    }
    
    spdlog::warn("Could not find embedding tensor in model");
    return "";
}

std::vector<float> BaseModelAdapter::extractEmbeddingFromGGUF([[maybe_unused]] int token_id) const {
    if (!gguf_loader_ || !model_loaded_) {
        spdlog::warn("Model not loaded");
        return {};
    }
    
    // Validate token_id is within vocabulary bounds
    if (token_id < 0 || token_id >= architecture_.vocab_size) {
        spdlog::error("Token ID {} out of bounds (vocab_size={})", token_id, architecture_.vocab_size);
        return {};
    }
    
    // Find embedding tensor name if not cached
    if (embedding_tensor_name_.empty()) {
        embedding_tensor_name_ = findEmbeddingTensorName();
        if (embedding_tensor_name_.empty()) {
            return {};
        }
    }
    
    // Get or cache embedding matrix pointer
    if (!embedding_matrix_) {
        void* tensor_ptr = gguf_loader_->mmapTensor(embedding_tensor_name_);
        if (tensor_ptr) {
            embedding_matrix_ = static_cast<const float*>(tensor_ptr);
        } else {
            // Fallback: try to load tensor data
            auto tensor_data = gguf_loader_->getTensorData(embedding_tensor_name_);
            if (!tensor_data.empty()) {
                embedding_matrix_ = reinterpret_cast<const float*>(tensor_data.data());
            } else {
                spdlog::error("Failed to load embedding tensor: {}", embedding_tensor_name_);
                return {};
            }
        }
    }
    
    // Extract single token embedding
    size_t hidden_dim = architecture_.hidden_size;
    std::vector<float> embedding(hidden_dim);
    
    const float* token_embed = embedding_matrix_ + (token_id * hidden_dim);
    std::copy(token_embed, token_embed + hidden_dim, embedding.begin());
    
    return embedding;
}

std::vector<float> BaseModelAdapter::getTokenEmbedding([[maybe_unused]] int token_id) const {
    if (!model_loaded_) {
        spdlog::warn("Model not loaded");
        return {};
    }
    
    // Check cache first
    auto it = embedding_cache_.find(token_id);
    if (it != embedding_cache_.end()) {
        ++cache_hits_;
        return it->second;
    }
    
    ++cache_misses_;
    
    // Extract from GGUF
    auto embedding = extractEmbeddingFromGGUF(token_id);
    
    if (embedding.empty()) {
        return {};
    }
    
    // Add to cache if not full
    if (static_cast<int>(embedding_cache_.size()) < MAX_CACHE_SIZE) {
        embedding_cache_[token_id] = embedding;
    }
    
    return embedding;
}

std::vector<float> BaseModelAdapter::getTokenEmbeddings(const std::vector<int>& token_ids) const {
    std::vector<float> embeddings;
    size_t hidden_dim = architecture_.hidden_size;
    embeddings.reserve(token_ids.size() * hidden_dim);
    
    for (int token_id : token_ids) {
        auto token_embedding = getTokenEmbedding(token_id);
        
        if (token_embedding.empty()) {
            // If extraction failed, fill with zeros
            spdlog::warn("Failed to get embedding for token {}, using zeros", token_id);
            embeddings.insert(embeddings.end(), hidden_dim, 0.0f);
        } else {
            embeddings.insert(embeddings.end(), token_embedding.begin(), token_embedding.end());
        }
    }
    
    return embeddings;
}

const float* BaseModelAdapter::getEmbeddingMatrix() const {
    if (!model_loaded_) {
        return nullptr;
    }
    
    // Find embedding tensor name if not cached
    if (embedding_tensor_name_.empty()) {
        embedding_tensor_name_ = findEmbeddingTensorName();
        if (embedding_tensor_name_.empty()) {
            return nullptr;
        }
    }
    
    // Get or cache embedding matrix pointer
    if (!embedding_matrix_) {
        void* tensor_ptr = gguf_loader_->mmapTensor(embedding_tensor_name_);
        if (tensor_ptr) {
            embedding_matrix_ = static_cast<const float*>(tensor_ptr);
        }
    }
    
    return embedding_matrix_;
}

void BaseModelAdapter::logCacheStats() const {
    float hit_rate = 0.0f;
    size_t total = cache_hits_ + cache_misses_;
    if (total > 0) {
        hit_rate = static_cast<float>(cache_hits_) / total * 100.0f;
    }
    
    spdlog::info("Embedding cache stats:");
    spdlog::info("  Cache size: {}/{}",static_cast<int>(embedding_cache_.size()), MAX_CACHE_SIZE);
    spdlog::info("  Cache hits: {}", cache_hits_);
    spdlog::info("  Cache misses: {}", cache_misses_);
    spdlog::info("  Hit rate: {:.1f}%", hit_rate);
}

std::string BaseModelAdapter::standardizeLayerName(const std::string& model_layer_name) const {
    // Convert model-specific layer names to standardized names
    // This helps with cross-model compatibility
    
    std::string standardized = model_layer_name;
    
    // Llama-style to standard
    std::regex llama_attn(R"(layers\.(\d+)\.attention\.(wq|wk|wv|wo))");
    standardized = std::regex_replace(standardized, llama_attn, "layers.$1.attention.$2");
    
    std::regex llama_mlp(R"(layers\.(\d+)\.feed_forward\.(w1|w2|w3))");
    standardized = std::regex_replace(standardized, llama_mlp, "layers.$1.mlp.$2");
    
    return standardized;
}

// ===== LoRAEnhancedModel Implementation =====

LoRAEnhancedModel::LoRAEnhancedModel(const Config& config)
    : config_(config), initialized_(false) {
}

LoRAEnhancedModel::~LoRAEnhancedModel() {
}

bool LoRAEnhancedModel::initialize() {
    spdlog::info("Initializing LoRA-enhanced model");
    
    // Load base model
    base_model_ = std::make_unique<BaseModelAdapter>();
    if (!base_model_->loadModel(config_.base_model_path)) {
        spdlog::error("Failed to load base model");
        return false;
    }
    
    // Get layers to adapt based on target modules
    active_layers_ = base_model_->getLayersByTargetModules(config_.target_modules);
    
    if (active_layers_.empty()) {
        std::string modules_str = {};
        for (size_t i = 0; i <static_cast<int>(config_.target_modules.size()); ++i) {
            if (i > 0) {
              modules_str += ", ";
            }
            modules_str += config_.target_modules[i];
        }
        spdlog::error("No layers matched target modules: {}", modules_str);
        return false;
    }
    
    spdlog::info("Found {} layers to adapt",static_cast<int>(active_layers_.size()));
    
    // Create LoRA adapters for each target layer
    if (!createLoRAAdapters()) {
        spdlog::error("Failed to create LoRA adapters");
        return false;
    }
    
    initialized_ = true;
    
    size_t lora_params = getLoRAParameterCount();
    size_t base_params = getBaseModelParameterCount();
    float reduction = 100.0f * (1.0f - static_cast<float>(lora_params) / base_params);
    
    spdlog::info("LoRA-enhanced model initialized:");
    spdlog::info("  Base model parameters: {}", base_params);
    spdlog::info("  LoRA trainable parameters: {}", lora_params);
    spdlog::info("  Parameter reduction: {:.2f}%", reduction);
    
    return true;
}

bool LoRAEnhancedModel::createLoRAAdapters() {
    lora_layers_.clear();
    
    for (const auto& layer_info : active_layers_) {
        // Create LoRA layer with same dimensions as base layer
        auto lora_layer = std::make_unique<LoRALayer>(
            layer_info.in_features,
            layer_info.out_features,
            config_.lora_config.rank,
            config_.lora_config.alpha / config_.lora_config.rank  // scaling
        );
        
        std::string lora_key = layer_info.name;
        lora_layers_[lora_key] = std::move(lora_layer);
        
        spdlog::debug("Created LoRA adapter for: {} ({}x{}, rank={})",
                     layer_info.name,
                     layer_info.in_features,
                     layer_info.out_features,
                     config_.lora_config.rank);
    }
    
    return true;
}

Tensor LoRAEnhancedModel::forward(const Tensor& input, int /*layer_idx*/) {
    if (!initialized_) {
        throw std::runtime_error("Model not initialized");
    }
    
    // For now, simplified forward pass
    // In production, this would:
    // 1. Compute base model output (frozen)
    // 2. Compute LoRA output
    // 3. Combine: output = base_output + lora_output * scaling
    
    // Find the first LoRA layer for this layer_idx (simplified)
    for (const auto& [name, lora_layer] : lora_layers_) {
        // In production: match by layer_idx
        // For now: just use the LoRA layer
        return lora_layer->forward(input);
    }
    
    // Fallback: return input (identity)
    return input.clone();
}

Tensor LoRAEnhancedModel::backward(const Tensor& grad_output, int /*layer_idx*/) {
    if (!initialized_) {
        throw std::runtime_error("Model not initialized");
    }
    
    // Backward pass only through LoRA layers (base is frozen)
    for (const auto& [name, lora_layer] : lora_layers_) {
        return lora_layer->backward(grad_output);
    }
    
    return grad_output.clone();
}

std::vector<Tensor*> LoRAEnhancedModel::getTrainableParameters() {
    std::vector<Tensor*> params;
    
    for (auto& [name, lora_layer] : lora_layers_) {
        auto layer_params = lora_layer->parameters();
        params.insert(params.end(), layer_params.begin(), layer_params.end());
    }
    
    return params;
}

size_t LoRAEnhancedModel::getLoRAParameterCount() const {
    size_t total = 0;
    
    for (const auto& [name, lora_layer] : lora_layers_) {
        total += lora_layer->parameter_count();
    }
    
    return total;
}

size_t LoRAEnhancedModel::getBaseModelParameterCount() const {
    return base_model_ ? base_model_->getTotalParameters() : 0;
}

std::unordered_map<std::string, std::pair<Tensor, Tensor>> 
LoRAEnhancedModel::exportLoRAWeights() const {
    std::unordered_map<std::string, std::pair<Tensor, Tensor>> weights;
    
    for (const auto& [name, lora_layer] : lora_layers_) {
        weights[name] = lora_layer->get_weights();
    }
    
    return weights;
}

bool LoRAEnhancedModel::importLoRAWeights(
    const std::unordered_map<std::string, std::pair<Tensor, Tensor>>& weights) {
    
    for (const auto& [name, weight_pair] : weights) {
        auto it = lora_layers_.find(name);
        if (it != lora_layers_.end()) {
            it->second->set_weights(weight_pair.first, weight_pair.second);
        } else {
            spdlog::warn("LoRA layer not found for import: {}", name);
        }
    }
    
    return true;
}

} // namespace lora
} // namespace llm
} // namespace themis

