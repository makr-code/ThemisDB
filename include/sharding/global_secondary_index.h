#pragma once

#include <chrono>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

namespace themis::sharding {

class GlobalSecondaryIndexManager {
public:
    struct IndexEntry {
        std::string index_name;
        std::string field_name;
        std::string value;
        std::string shard_id;
        std::string primary_key;
        std::chrono::system_clock::time_point updated_at;
    };

    struct Config {
        bool asynchronous_updates = true;
        bool eventual_consistency = true;
        std::chrono::milliseconds staleness_budget{5000};
    };

    GlobalSecondaryIndexManager();
    explicit GlobalSecondaryIndexManager(const Config& config);

    void createIndex(const std::string& index_name, const std::string& field_name);
    bool hasIndex(const std::string& index_name) const;

    void upsert(const std::string& index_name, const std::string& field_name,
                const std::string& shard_id, const std::string& primary_key,
                const std::string& value);
    void erase(const std::string& index_name, const std::string& shard_id, const std::string& primary_key);
    void eraseShard(const std::string& index_name, const std::string& shard_id);

    std::vector<IndexEntry> queryEquals(const std::string& index_name, const std::string& value) const;
    std::vector<IndexEntry> queryRange(const std::string& index_name,
                                      const std::string& lower_bound,
                                      const std::string& upper_bound) const;

    size_t size() const;

private:
    struct IndexDefinition {
        std::string name;
        std::string field_name;
    };

    Config config_;
    std::map<std::string, IndexDefinition> indexes_;
    std::map<std::string, std::map<std::string, std::vector<IndexEntry>>> entries_;
};

}  // namespace themis::sharding
