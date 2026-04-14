/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            lora_metadata_cache.cpp                            ║
  Version:         0.0.40                                             ║
  Last Modified:   2026-04-14 07:02:40                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     96                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "llm/lora_metadata_cache.h"

namespace themis {
namespace llm {

void LoRAMetadataCache::put(const std::string& lora_id, const LoRAMetadata& metadata) {
    cache_.insert(lora_id, metadata);
}

std::optional<LoRAMetadata> LoRAMetadataCache::get(const std::string& lora_id) const {
    return cache_.get(lora_id);
}

void LoRAMetadataCache::touch(const std::string& lora_id) {
    auto metadata = cache_.get(lora_id);
    if (metadata) {
        metadata->last_accessed = std::chrono::system_clock::now();
        metadata->access_count++;
        cache_.insert(lora_id, *metadata);
    }
}

void LoRAMetadataCache::markLoaded(const std::string& lora_id, int slot_id) {
    auto metadata = cache_.get(lora_id);
    if (metadata) {
        metadata->is_loaded = true;
        metadata->slot_id = slot_id;
        metadata->loaded_timestamp = std::chrono::system_clock::now();
        cache_.insert(lora_id, *metadata);
    }
}

void LoRAMetadataCache::markUnloaded(const std::string& lora_id) {
    auto metadata = cache_.get(lora_id);
    if (metadata) {
        metadata->is_loaded = false;
        metadata->slot_id = -1;
        cache_.insert(lora_id, *metadata);
    }
}

bool LoRAMetadataCache::contains(const std::string& lora_id) const {
    return cache_.contains(lora_id);
}

bool LoRAMetadataCache::remove(const std::string& lora_id) {
    return cache_.erase(lora_id);
}

size_t LoRAMetadataCache::size() const {
    return cache_.size();
}

void LoRAMetadataCache::clear() {
    cache_.clear();
}

LoRAMetadataCache::Stats LoRAMetadataCache::getStats() const {
    Stats stats{};
    stats.total_entries = cache_.size();
    
    cache_.for_each([&stats](const std::string&, const LoRAMetadata& meta) {
        stats.total_size_bytes += meta.size_bytes;
        stats.total_accesses += meta.access_count;
        if (meta.is_loaded) {
            stats.loaded_entries++;
        }
    });
    
    return stats;
}

} // namespace llm
} // namespace themis
