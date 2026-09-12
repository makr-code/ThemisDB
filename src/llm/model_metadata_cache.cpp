/**
 * @file model_metadata_cache.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "llm/model_metadata_cache.h"

namespace themis {
namespace llm {

void ModelMetadataCache::put(const std::string& model_id, const ModelMetadata& metadata) {
    cache_.insert(model_id, metadata);
}

std::optional<ModelMetadata> ModelMetadataCache::get(const std::string& model_id) const {
    return cache_.get(model_id);
}

void ModelMetadataCache::touch(const std::string& model_id) {
    auto metadata = cache_.get(model_id);
    if (metadata) {
        metadata->last_accessed = std::chrono::system_clock::now();
        metadata->access_count++;
        cache_.insert(model_id, *metadata);
    }
}

bool ModelMetadataCache::contains(const std::string& model_id) const {
    return cache_.contains(model_id);
}

bool ModelMetadataCache::remove(const std::string& model_id) {
    return cache_.erase(model_id);
}

size_t ModelMetadataCache::size() const {
    return cache_.size();
}

void ModelMetadataCache::clear() {
    cache_.clear();
}

ModelMetadataCache::Stats ModelMetadataCache::getStats() const {
    Stats stats{};
    stats.total_entries = cache_.size();
    
    cache_.for_each([&stats](const std::string&, const ModelMetadata& meta) {
        stats.total_size_bytes += meta.size_bytes;
        stats.total_accesses += meta.access_count;
        if (meta.is_pinned) {
            stats.pinned_entries++;
        }
    });
    
    return stats;
}

} // namespace llm
} // namespace themis
