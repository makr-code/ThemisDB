#include "llm/lora_registry.h"
#include <rocksdb/db.h>
#include <rocksdb/transaction_db.h>
#include <chrono>
#include <algorithm>

namespace themis {
namespace llm {

// LoRAAdapter methods
nlohmann::json LoRARegistry::LoRAAdapter::toJson() const {
    return nlohmann::json{
        {"adapter_id", adapter_id},
        {"name", name},
        {"base_model", base_model},
        {"adapter_path", adapter_path},
        {"domain", domain},
        {"capabilities", capabilities},
        {"rank", rank},
        {"alpha", alpha},
        {"size_bytes", size_bytes},
        {"metadata", metadata}
    };
}

LoRARegistry::LoRAAdapter LoRARegistry::LoRAAdapter::fromJson(const nlohmann::json& j) {
    LoRAAdapter adapter;
    adapter.adapter_id = j.value("adapter_id", "");
    adapter.name = j.value("name", "");
    adapter.base_model = j.value("base_model", "");
    adapter.adapter_path = j.value("adapter_path", "");
    adapter.domain = j.value("domain", "");
    adapter.capabilities = j.value("capabilities", std::vector<std::string>{});
    adapter.rank = j.value("rank", 8);
    adapter.alpha = j.value("alpha", 16.0f);
    adapter.size_bytes = j.value("size_bytes", 0UL);
    adapter.metadata = j.value("metadata", nlohmann::json::object());
    return adapter;
}

// LoadStats methods
nlohmann::json LoRARegistry::LoadStats::toJson() const {
    return nlohmann::json{
        {"total_adapters", total_adapters},
        {"loaded_adapters", loaded_adapters},
        {"total_memory_bytes", total_memory_bytes},
        {"loaded_adapter_ids", loaded_adapter_ids}
    };
}

// Constructor
LoRARegistry::LoRARegistry(
    rocksdb::TransactionDB* db,
    rocksdb::ColumnFamilyHandle* cf
) : db_(db), cf_(cf) {
    rebuildCache();
}

// Register adapter
bool LoRARegistry::registerAdapter(const LoRAAdapter& adapter) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Store in cache
    adapter_cache_[adapter.adapter_id] = adapter;
    
    // Persist to DB
    return persistAdapter(adapter);
}

// Get adapter
std::optional<LoRARegistry::LoRAAdapter> LoRARegistry::getAdapter(
    const std::string& adapter_id
) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check cache first
    auto it = adapter_cache_.find(adapter_id);
    if (it != adapter_cache_.end()) {
        return it->second;
    }
    
    // Load from DB
    return loadAdapterMetadata(adapter_id);
}

// List adapters
std::vector<LoRARegistry::LoRAAdapter> LoRARegistry::listAdapters(
    const std::string& domain
) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<LoRAAdapter> result;
    
    for (const auto& [adapter_id, adapter] : adapter_cache_) {
        if (domain.empty() || adapter.domain == domain) {
            result.push_back(adapter);
        }
    }
    
    return result;
}

// Load adapter
bool LoRARegistry::loadAdapter(const std::string& adapter_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check if already loaded
    if (loaded_adapters_.count(adapter_id)) {
        return true;
    }
    
    // Get adapter metadata
    auto adapter_opt = getAdapter(adapter_id);
    if (!adapter_opt) {
        return false;
    }
    
    // STUB: In v1.5.0, this will actually load LoRA weights into memory
    // For now, just mark as loaded
    loaded_adapters_.insert(adapter_id);
    last_access_time_[adapter_id] = getCurrentTimestampMs();
    
    return true;
}

// Unload adapter
bool LoRARegistry::unloadAdapter(const std::string& adapter_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check if loaded
    if (!loaded_adapters_.count(adapter_id)) {
        return false;
    }
    
    // STUB: In v1.5.0, this will free LoRA weights from memory
    loaded_adapters_.erase(adapter_id);
    last_access_time_.erase(adapter_id);
    
    return true;
}

// Get loaded adapters
std::vector<std::string> LoRARegistry::getLoadedAdapters() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<std::string> result;
    for (const auto& adapter_id : loaded_adapters_) {
        result.push_back(adapter_id);
    }
    return result;
}

// Check if loaded
bool LoRARegistry::isLoaded(const std::string& adapter_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return loaded_adapters_.count(adapter_id) > 0;
}

// Get stats
LoRARegistry::LoadStats LoRARegistry::getStats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    LoadStats stats;
    stats.total_adapters = adapter_cache_.size();
    stats.loaded_adapters = loaded_adapters_.size();
    stats.total_memory_bytes = 0;
    
    for (const auto& adapter_id : loaded_adapters_) {
        auto it = adapter_cache_.find(adapter_id);
        if (it != adapter_cache_.end()) {
            stats.total_memory_bytes += it->second.size_bytes;
            stats.loaded_adapter_ids.push_back(adapter_id);
        }
    }
    
    return stats;
}

// Preload adapters
size_t LoRARegistry::preloadAdapters(const std::vector<std::string>& adapter_ids) {
    size_t loaded_count = 0;
    
    for (const auto& adapter_id : adapter_ids) {
        if (loadAdapter(adapter_id)) {
            loaded_count++;
        }
    }
    
    return loaded_count;
}

// Garbage collect unused adapters
size_t LoRARegistry::gcUnusedAdapters(int ttl_seconds) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    int64_t current_time = getCurrentTimestampMs();
    int64_t ttl_ms = ttl_seconds * 1000;
    
    std::vector<std::string> to_unload;
    
    for (const auto& adapter_id : loaded_adapters_) {
        auto it = last_access_time_.find(adapter_id);
        if (it != last_access_time_.end()) {
            if (current_time - it->second > ttl_ms) {
                to_unload.push_back(adapter_id);
            }
        }
    }
    
    for (const auto& adapter_id : to_unload) {
        loaded_adapters_.erase(adapter_id);
        last_access_time_.erase(adapter_id);
    }
    
    return to_unload.size();
}

// Update adapter
bool LoRARegistry::updateAdapter(
    const std::string& adapter_id,
    const LoRAAdapter& adapter
) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Update cache
    adapter_cache_[adapter_id] = adapter;
    
    // Persist to DB
    return persistAdapter(adapter);
}

// Delete adapter
bool LoRARegistry::deleteAdapter(const std::string& adapter_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Unload if loaded
    loaded_adapters_.erase(adapter_id);
    last_access_time_.erase(adapter_id);
    
    // Remove from cache
    adapter_cache_.erase(adapter_id);
    
    // Delete from DB
    std::string key = makeKey(adapter_id);
    rocksdb::WriteOptions write_options;
    auto status = db_->Delete(write_options, cf_, key);
    
    return status.ok();
}

// Find adapters by capability
std::vector<LoRARegistry::LoRAAdapter> LoRARegistry::findAdaptersByCapability(
    const std::string& capability
) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<LoRAAdapter> matching_adapters;
    
    for (const auto& [adapter_id, adapter] : adapter_cache_) {
        for (const auto& cap : adapter.capabilities) {
            if (cap == capability) {
                matching_adapters.push_back(adapter);
                break;
            }
        }
    }
    
    return matching_adapters;
}

// Private helper methods
std::string LoRARegistry::makeKey(const std::string& adapter_id) const {
    return "lora_adapter:" + adapter_id;
}

bool LoRARegistry::persistAdapter(const LoRAAdapter& adapter) {
    std::string key = makeKey(adapter.adapter_id);
    std::string value = adapter.toJson().dump();
    
    rocksdb::WriteOptions write_options;
    auto status = db_->Put(write_options, cf_, key, value);
    
    return status.ok();
}

std::optional<LoRARegistry::LoRAAdapter> LoRARegistry::loadAdapterMetadata(
    const std::string& adapter_id
) const {
    std::string key = makeKey(adapter_id);
    std::string value;
    
    rocksdb::ReadOptions read_options;
    auto status = db_->Get(read_options, cf_, key, &value);
    
    if (status.ok()) {
        try {
            auto json = nlohmann::json::parse(value);
            return LoRAAdapter::fromJson(json);
        } catch (...) {
            return std::nullopt;
        }
    }
    
    return std::nullopt;
}

void LoRARegistry::rebuildCache() {
    adapter_cache_.clear();
    
    // Iterate through all lora_adapter:* keys
    rocksdb::ReadOptions read_options;
    std::unique_ptr<rocksdb::Iterator> it(db_->NewIterator(read_options, cf_));
    
    std::string prefix = "lora_adapter:";
    for (it->Seek(prefix); it->Valid() && it->key().starts_with(prefix); it->Next()) {
        try {
            auto json = nlohmann::json::parse(it->value().ToString());
            auto adapter = LoRAAdapter::fromJson(json);
            adapter_cache_[adapter.adapter_id] = adapter;
        } catch (...) {
            // Skip invalid entries
        }
    }
    
    // Check iterator status
    if (!it->status().ok()) {
        // Log error but don't throw - allow partial cache rebuild
    }
}

int64_t LoRARegistry::getCurrentTimestampMs() const {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}

} // namespace llm
} // namespace themis
