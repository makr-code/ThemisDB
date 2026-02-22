/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            model_metadata_cache.cpp                           ║
  Version:         0.0.24                                             ║
  Last Modified:   2026-02-22 08:12:23                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     81                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 00c723d27  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 03329d86d  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31e8b8df0  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 0d722b04c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
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
