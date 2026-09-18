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
    /**
     * @brief TBD: Describe GlobalSecondaryIndexManager.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit GlobalSecondaryIndexManager(const Config& config);

    /**
     * @brief TBD: Describe createIndex.
     * @param[in] index_name Input parameter.
     * @param[in] field_name Input parameter.
     */
    void createIndex(const std::string& index_name, const std::string& field_name);
    /**
     * @brief TBD: Describe hasIndex.
     * @param[in] index_name Input parameter.
     * @return True on success.
     */
    bool hasIndex(const std::string& index_name) const;

    /**
     * @brief TBD: Describe upsert.
     * @param[in] index_name Input parameter.
     * @param[in] field_name Input parameter.
     * @param[in] shard_id Input parameter.
     * @param[in] primary_key Input parameter.
     * @param[in] value Input parameter.
     */
    void upsert(const std::string& index_name, const std::string& field_name,
                const std::string& shard_id, const std::string& primary_key,
                const std::string& value);
    /**
     * @brief TBD: Describe erase.
     * @param[in] index_name Input parameter.
     * @param[in] shard_id Input parameter.
     * @param[in] primary_key Input parameter.
     */
    void erase(const std::string& index_name, const std::string& shard_id, const std::string& primary_key);
    /**
     * @brief TBD: Describe eraseShard.
     * @param[in] index_name Input parameter.
     * @param[in] shard_id Input parameter.
     */
    void eraseShard(const std::string& index_name, const std::string& shard_id);

    /**
     * @brief TBD: Describe queryEquals.
     * @param[in] index_name Input parameter.
     * @param[in] value Input parameter.
     * @return Return value.
     */
    std::vector<IndexEntry> queryEquals(const std::string& index_name, const std::string& value) const;
    /**
     * @brief TBD: Describe queryRange.
     * @param[in] index_name Input parameter.
     * @param[in] lower_bound Input parameter.
     * @param[in] upper_bound Input parameter.
     * @return Return value.
     */
    std::vector<IndexEntry> queryRange(const std::string& index_name,
                                      const std::string& lower_bound,
                                      const std::string& upper_bound) const;

    /**
     * @brief TBD: Describe size.
     * @return Return value.
     */
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
