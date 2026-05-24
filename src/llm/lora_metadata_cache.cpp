/*
 * ThemisDB | File: lora_metadata_cache.cpp | Version: 0.0.47 | Last Modified: 2026-04-15 18:58:58
 * Author: ThemisDB Version Bot | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 82
 * Open Issues: TODOs=1, Stubs=1, Gaps=3, Unimpl=0, Mock=1, Sim=0, Debt=0
 * Gap Correlation: internal=3 | external_v3=23 | delta=20 | status=divergent
 * External Severity (v3): C=0, H=22, M=1
 * PR: #105 Add plugin-based LLM integration v1.3.0 with llama.cpp, GPU acceler... (2026-03-11T17:03:10Z)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
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
