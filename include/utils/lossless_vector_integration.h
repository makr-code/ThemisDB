/**
 * @file lossless_vector_integration.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// EXPERIMENTAL: Lossless Vector Compression Integration for VectorIndexManager
//
// WARNING: This is a scientific experiment and may be rolled back.
// 
// This file provides optional lossless compression support alongside
// the existing SQ8 (lossy) quantization in VectorIndexManager.
//
// Configuration via DB key "config:vector_compression_lossless":
// {
//   "enabled": false,              // Enable lossless compression (default: false)
//   "mode": "auto",                // "auto", "sparse_csr", "delta_varint", "dictionary", "none"
//   "sparse_threshold": 0.95,      // Auto-select CSR if >=95% zeros
//   "fallback_to_sq8": true        // Use SQ8 if lossless not applicable
// }
//
// Priority: If lossless compression is enabled and applicable, it takes
// precedence over SQ8. Otherwise, falls back to existing SQ8 or raw storage.

#pragma once

#include "utils/lossless_vector_compression.h"
#include "storage/base_entity.h"
#include "storage/rocksdb_wrapper.h"
#include "utils/logger.h"
#include <nlohmann/json.hpp>
#include <optional>

namespace themis {
namespace experimental {

struct LosslessCompressionConfig {
    bool enabled = false;
    std::string mode = "auto";  // "auto", "sparse_csr", "delta_varint", "dictionary", "none"
    float sparse_threshold = 0.95f;
    bool fallback_to_sq8 = true;
    
    static LosslessCompressionConfig load(RocksDBWrapper& db) {
        LosslessCompressionConfig config;
        
        try {
            if (auto cfg = db.get("config:vector_compression_lossless")) {
                std::string s(cfg->begin(), cfg->end());
                nlohmann::json j = nlohmann::json::parse(s);
                
                config.enabled = j.value("enabled", false);
                config.mode = j.value("mode", std::string("auto"));
                config.sparse_threshold = j.value("sparse_threshold", 0.95f);
                config.fallback_to_sq8 = j.value("fallback_to_sq8", true);
            }
        } catch (const std::exception& e) {
            THEMIS_WARN("Failed to load lossless compression config: {}", e.what());
        }
        
        return config;
    }
};

/** @brief Vector compression helper. */
class VectorCompressionHelper {
public:
    // Try to compress vector using lossless methods
    // Returns serialized entity with compressed embedding, or nullopt if not applicable
    static std::optional<std::vector<uint8_t>> tryLosslessCompression(
        const BaseEntity& e,
        const std::vector<float>& vec,
        RocksDBWrapper& db
    ) {
        auto config = LosslessCompressionConfig::load(db);
        
        if (!config.enabled) {
            return std::nullopt; // Lossless compression disabled
        }
        
        // Determine compression method
        LosslessCompressionMethod method = {};
        
        if (config.mode == "auto") {
            method = AdaptiveCompressor::selectMethod(vec, config.sparse_threshold);
        } else if (config.mode == "sparse_csr") {
            method = LosslessCompressionMethod::SPARSE_CSR;
        } else if (config.mode == "delta_varint") {
            method = LosslessCompressionMethod::DELTA_VARINT;
        } else if (config.mode == "dictionary") {
            method = LosslessCompressionMethod::DICTIONARY;
        } else {
            method = LosslessCompressionMethod::NONE;
        }
        
        if (method == LosslessCompressionMethod::NONE) {
            return std::nullopt; // No suitable lossless method
        }
        
        // Apply compression
        try {
            auto fields = e.getAllFields();
            fields.erase("embedding");
            
            switch (method) {
                case LosslessCompressionMethod::SPARSE_CSR: {
                    auto compressed = SparseVectorCodec::compress(vec);
                    auto serialized = compressed.serialize();
                    
                    fields["embedding_csr"] = serialized;
                    fields["compression_method"] = std::string("sparse_csr");
                    
                    THEMIS_DEBUG("Lossless CSR compression: {} -> {} bytes ({}x)",
                                vec.size() * sizeof(float),
                                serialized.size(),
                                static_cast<float>(vec.size() * sizeof(float)) / serialized.size());
                    
                    BaseEntity compressed_entity = BaseEntity::fromFields(e.getPrimaryKey(), fields);
                    return compressed_entity.serialize();
                }
                
                case LosslessCompressionMethod::DELTA_VARINT: {
                    // Convert to integers with range validation
                    std::vector<int32_t> int_vec = {};

                    int_vec.reserve(vec.size());
                    for (float f : vec) {
                        // Validate range to prevent overflow
                        if (f < -2147483648.0f || f > 2147483647.0f) {
                            THEMIS_WARN("Delta+VarInt: Value {} out of int32 range, skipping lossless compression", f);
                            return std::nullopt;
                        }
                        int_vec.push_back(static_cast<int32_t>(std::round(f)));
                    }
                    
                    auto compressed = VarIntCodec::compress_delta(int_vec);
                    
                    fields["embedding_varint"] = compressed;
                    fields["compression_method"] = std::string("delta_varint");
                    
                    THEMIS_DEBUG("Lossless Delta+VarInt compression: {} -> {} bytes ({}x)",
                                vec.size() * sizeof(int32_t),
                                compressed.size(),
                                static_cast<float>(vec.size() * sizeof(int32_t)) / compressed.size());
                    
                    BaseEntity compressed_entity = BaseEntity::fromFields(e.getPrimaryKey(), fields);
                    return compressed_entity.serialize();
                }
                
                case LosslessCompressionMethod::DICTIONARY: {
                    auto compressed = DictionaryCodec<float>::compress(vec);
                    
                    // Serialize dictionary and indices
                    std::vector<uint8_t> dict_data;
                    dict_data.insert(dict_data.end(), 
                                   (uint8_t*)compressed.dictionary.data(),
                                   (uint8_t*)compressed.dictionary.data() + compressed.dictionary.size() * sizeof(float));
                    
                    std::vector<uint8_t> indices_data;
                    indices_data.insert(indices_data.end(),
                                      (uint8_t*)compressed.indices.data(),
                                      (uint8_t*)compressed.indices.data() + compressed.indices.size() * sizeof(uint32_t));
                    
                    fields["embedding_dict"] = dict_data;
                    fields["embedding_dict_indices"] = indices_data;
                    fields["compression_method"] = std::string("dictionary");
                    
                    THEMIS_DEBUG("Lossless Dictionary compression: {} unique values, {} -> {} bytes",
                                compressed.dictionary.size(),
                                vec.size() * sizeof(float),
                                dict_data.size() + indices_data.size());
                    
                    BaseEntity compressed_entity = BaseEntity::fromFields(e.getPrimaryKey(), fields);
                    return compressed_entity.serialize();
                }
                
                default:
                    return std::nullopt;
            }
        } catch (const std::exception& e) {
            THEMIS_WARN("Lossless compression failed: {}", e.what());
            return std::nullopt;
        }
    }
    
    // Decompress vector from entity
    static std::optional<std::vector<float>> decompressVector(const BaseEntity& e) {
        auto fields = e.getAllFields();
        
        // Check for compression method
        auto method_it = fields.find("compression_method");
        if (method_it == fields.end()) {
            return std::nullopt; // Not compressed with lossless method
        }
        
        std::string method = std::get<std::string>(method_it->second);
        
        try {
            if (method == "sparse_csr") {
                auto it = fields.find("embedding_csr");
                if (it == fields.end()) {
                  return std::nullopt;
                }
                
                auto& bytes = std::get<std::vector<uint8_t>>(it->second);
                auto sparse = SparseVectorCSR::deserialize(bytes);
                return SparseVectorCodec::decompress(sparse);
                
            } else if (method == "delta_varint") {
                auto it = fields.find("embedding_varint");
                if (it == fields.end()) {
                  return std::nullopt;
                }
                
                auto& bytes = std::get<std::vector<uint8_t>>(it->second);
                auto int_vec = VarIntCodec::decompress_delta(bytes);
                
                std::vector<float> result = {};

                result.reserve(int_vec.size());
                for (int32_t i : int_vec) {
                    result.push_back(static_cast<float>(i));
                }
                return result;
                
            } else if (method == "dictionary") {
                auto dict_it = fields.find("embedding_dict");
                auto indices_it = fields.find("embedding_dict_indices");
                if (dict_it == fields.end() || indices_it == fields.end()) {
                  return std::nullopt;
                }
                
                auto& dict_bytes = std::get<std::vector<uint8_t>>(dict_it->second);
                auto& indices_bytes = std::get<std::vector<uint8_t>>(indices_it->second);
                
                // Reconstruct dictionary
                size_t dict_size = dict_bytes.size() / sizeof(float);
                std::vector<float> dictionary(dict_size);
                std::memcpy(dictionary.data(), dict_bytes.data(), dict_bytes.size());
                
                // Reconstruct indices
                size_t indices_count = indices_bytes.size() / sizeof(uint32_t);
                std::vector<uint32_t> indices(indices_count);
                std::memcpy(indices.data(), indices_bytes.data(), indices_bytes.size());
                
                // Decompress
                DictionaryCompressed<float> compressed;
                compressed.dictionary = std::move(dictionary);
                compressed.indices = std::move(indices);
                compressed.original_size = indices_count;
                
                return DictionaryCodec<float>::decompress(compressed);
            }
        } catch (const std::exception& e) {
            THEMIS_WARN("Lossless decompression failed: {}", e.what());
        }
        
        return std::nullopt;
    }
};

} // namespace experimental
} // namespace themis
