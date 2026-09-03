#include "sharding/global_secondary_index.h"

#include <algorithm>

namespace themis::sharding {

GlobalSecondaryIndexManager::GlobalSecondaryIndexManager() : config_({}) {}

GlobalSecondaryIndexManager::GlobalSecondaryIndexManager(const Config& config) : config_(config) {}

void GlobalSecondaryIndexManager::createIndex(const std::string& index_name, const std::string& field_name) {
    indexes_[index_name] = IndexDefinition{index_name, field_name};
}

bool GlobalSecondaryIndexManager::hasIndex(const std::string& index_name) const {
    return indexes_.find(index_name) != indexes_.end();
}

void GlobalSecondaryIndexManager::upsert(const std::string& index_name, const std::string& field_name,
                                        const std::string& shard_id, const std::string& primary_key,
                                        const std::string& value) {
    if (index_name.empty() || field_name.empty()) {
        return;
    }
    if (!hasIndex(index_name)) {
        createIndex(index_name, field_name);
    }
    IndexEntry entry;
    entry.index_name = index_name;
    entry.field_name = field_name;
    entry.value = value;
    entry.shard_id = shard_id;
    entry.primary_key = primary_key;
    entry.updated_at = std::chrono::system_clock::now();
    auto& index_entries = entries_[index_name][value];
    auto it = std::find_if(index_entries.begin(), index_entries.end(), [&](const IndexEntry& e) {
        return e.shard_id == shard_id && e.primary_key == primary_key;
    });
    if (it != index_entries.end()) {
        *it = entry;
    } else {
        index_entries.push_back(entry);
    }
}

void GlobalSecondaryIndexManager::erase(const std::string& index_name, const std::string& shard_id, const std::string& primary_key) {
    auto it = entries_.find(index_name);
    if (it == entries_.end()) {
        return;
    }
    for (auto value_it = it->second.begin(); value_it != it->second.end(); ++value_it) {
        auto& items = value_it->second;
        items.erase(std::remove_if(items.begin(), items.end(), [&](const IndexEntry& entry) {
            return entry.shard_id == shard_id && entry.primary_key == primary_key;
        }), items.end());
        if (items.empty()) {
            it->second.erase(value_it);
        }
    }
}

void GlobalSecondaryIndexManager::eraseShard(const std::string& index_name, const std::string& shard_id) {
    auto it = entries_.find(index_name);
    if (it == entries_.end()) {
        return;
    }
    for (auto value_it = it->second.begin(); value_it != it->second.end(); ) {
        auto& items = value_it->second;
        auto before = items.size();
        items.erase(std::remove_if(items.begin(), items.end(), [&](const IndexEntry& entry) {
            return entry.shard_id == shard_id;
        }), items.end());
        if (items.empty() || items.size() == before) {
            value_it = it->second.erase(value_it);
        } else {
            ++value_it;
        }
    }
}

std::vector<GlobalSecondaryIndexManager::IndexEntry> GlobalSecondaryIndexManager::queryEquals(
    const std::string& index_name, const std::string& value) const {
    std::vector<IndexEntry> result;
    auto it = entries_.find(index_name);
    if (it == entries_.end()) {
        return result;
    }
    auto value_it = it->second.find(value);
    if (value_it == it->second.end()) {
        return result;
    }
    result = value_it->second;
    return result;
}

std::vector<GlobalSecondaryIndexManager::IndexEntry> GlobalSecondaryIndexManager::queryRange(
    const std::string& index_name, const std::string& lower_bound, const std::string& upper_bound) const {
    std::vector<IndexEntry> result;
    auto index_it = entries_.find(index_name);
    if (index_it == entries_.end()) {
        return result;
    }
    for (const auto& [value, items] : index_it->second) {
        if (value >= lower_bound && value <= upper_bound) {
            result.insert(result.end(), items.begin(), items.end());
        }
    }
    return result;
}

size_t GlobalSecondaryIndexManager::size() const {
    size_t count = 0;
    for (const auto& [_, by_value] : entries_) {
        for (const auto& [_, items] : by_value) {
            count += items.size();
        }
    }
    return count;
}

}  // namespace themis::sharding
