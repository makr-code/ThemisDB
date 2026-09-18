/**
 * @file version_manager.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <vector>
#include <optional>
#include <cstdint>
#include <unordered_map>

namespace themis {
namespace content {

class VersionManager {
public:
    struct Version {
        int version_number = 0;
        int64_t timestamp;
        std::string author;
        std::string comment;
        std::string content_hash;  // SHA-256 of content
        size_t size_bytes;
        std::string content;       // Full content snapshot
        std::string delta;         // Delta (diff) from previous version; empty for v1
    };

    int createVersion(
        const std::string& content_id,
        const std::string& content_hash,
        size_t size_bytes,
        const std::string& author = "",
        const std::string& comment = ""
    );

    int createVersionWithContent(
        const std::string& content_id,
        const std::string& content,
        const std::string& author = "",
        const std::string& comment = ""
    );

    /**
     * @brief Get Content.
     * @param[in] content_id Identifier of the content.
     * @param[in] version_number Input parameter.
     * @return Return value.
     */
    std::optional<std::string> getContent(
        const std::string& content_id,
        int version_number
    ) const;

    /**
     * @brief Get Version History.
     * @param[in] content_id Identifier of the content.
     * @return Return value.
     */
    std::vector<Version> getVersionHistory(const std::string& content_id) const;

    /**
     * @brief Get Version.
     * @param[in] content_id Identifier of the content.
     * @param[in] version_number Input parameter.
     * @return Return value.
     */
    std::optional<Version> getVersion(const std::string& content_id, int version_number) const;

    /**
     * @brief Get Latest Version.
     * @param[in] content_id Identifier of the content.
     * @return Return value.
     */
    int getLatestVersion(const std::string& content_id) const;

    /**
     * @brief Has Versions.
     * @param[in] content_id Identifier of the content.
     * @return True when the operation succeeds.
     */
    bool hasVersions(const std::string& content_id) const;

    /**
     * @brief Delete Version.
     * @param[in] content_id Identifier of the content.
     * @param[in] version_number Input parameter.
     * @return True when the operation succeeds.
     */
    bool deleteVersion(const std::string& content_id, int version_number);


    /**
     * @brief Compute Hash.
     * @param[in] data Input parameter.
     * @return Return value.
     */
    static std::string computeHash(const std::string& data);

    /**
     * @brief Compute Delta.
     * @param[in] old_content Input parameter.
     * @param[in] new_content Input parameter.
     * @return Return value.
     */
    static std::string computeDelta(const std::string& old_content,
                                    const std::string& new_content);

private:
    // In-memory storage (simplified - in production would use RocksDB)
    std::unordered_map<std::string, std::vector<Version>> versions_;
};

} // namespace content
} // namespace themis
