/**
 * @file cte_cache.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Phase 4.3: CTE Memory Management and Spill-to-Disk
#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <optional>
#include <fstream>
#include <filesystem>

namespace themis {
namespace query {

class CTECache {
public:
    struct Config {
        size_t max_memory_bytes = 100 * 1024 * 1024;  // 100 MB default
        std::string spill_directory = "./themis_cte_spill";
        bool enable_compression = true;
        bool auto_cleanup = true;  // Delete spill files on destruction
    };
    
    struct CacheEntry {
        std::string name;
        size_t result_count = 0;
        size_t estimated_size_bytes = 0;
        bool is_spilled = false;
        std::string spill_file_path;
        std::vector<nlohmann::json> in_memory_data;  // Only populated if not spilled
    };
    
    CTECache();
    /**
     * @brief CTECache.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit CTECache(Config config);
    
    ~CTECache();
    
    // Delete copy constructor and assignment (cache is not copyable)
    CTECache(const CTECache&) = delete;
    CTECache& operator=(const CTECache&) = delete;
    
    // Allow move
    CTECache(CTECache&&) noexcept = default;
    CTECache& operator=(CTECache&&) noexcept = default;
    
    /**
     * @brief Store.
     * @param[in] name Input parameter.
     * @param[in] results Input parameter.
     * @return True when the operation succeeds.
     */
    bool store(const std::string& name, std::vector<nlohmann::json> results);
    
    /**
     * @brief Get.
     * @param[in] name Input parameter.
     * @return Return value.
     */
    std::optional<std::vector<nlohmann::json>> get(const std::string& name);
    
    /**
     * @brief Contains.
     * @param[in] name Input parameter.
     * @return True when the operation succeeds.
     */
    bool contains(const std::string& name) const;
    
    /**
     * @brief Remove.
     * @param[in] name Input parameter.
     */
    void remove(const std::string& name);
    
    /**
     * @brief Clear.
     */
    void clear();
    
    size_t getCurrentMemoryUsage() const { return current_memory_usage_; }
    
    size_t size() const { return entries_.size(); }
    
    struct Stats {
        size_t total_ctes = 0;
        size_t in_memory_ctes = 0;
        size_t spilled_ctes = 0;
        size_t memory_usage_bytes = 0;
        size_t total_results = 0;
        size_t spill_operations = 0;
        size_t disk_reads = 0;
    };
    /**
     * @brief Get Stats.
     * @return Return value.
     */
    Stats getStats() const;
    
private:
    /**
     * @brief Estimate Size.
     * @param[in] data Input parameter.
     * @return Return value.
     */
    size_t estimateSize(const std::vector<nlohmann::json>& data) const;
    
    /**
     * @brief Spill To Disk.
     * @param[in] name Input parameter.
     * @param[in] data Input parameter.
     * @return True when the operation succeeds.
     */
    bool spillToDisk(const std::string& name, const std::vector<nlohmann::json>& data);
    
    /**
     * @brief Load From Disk.
     * @param[in] name Input parameter.
     * @return Return value.
     */
    std::optional<std::vector<nlohmann::json>> loadFromDisk(const std::string& name);
    
    /**
     * @brief Make Room.
     * @param[in] required_bytes Input parameter.
     * @return True when the operation succeeds.
     */
    bool makeRoom(size_t required_bytes);
    
    /**
     * @brief Get Spill File Path.
     * @param[in] name Input parameter.
     * @return Return value.
     */
    std::string getSpillFilePath(const std::string& name) const;
    
    /**
     * @brief Ensure Spill Directory.
     */
    void ensureSpillDirectory();
    
    Config config_;
    std::unordered_map<std::string, CacheEntry> entries_;
    size_t current_memory_usage_ = 0;
    
    // Statistics
    mutable size_t stat_spill_operations_ = 0;
    mutable size_t stat_disk_reads_ = 0;
};

} // namespace query
} // namespace themis
