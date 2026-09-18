/**
 * @file pii_api_handler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <vector>
#include <optional>
#include <nlohmann/json.hpp>

// Forward declarations to avoid heavy includes in header
namespace rocksdb { class TransactionDB; class ColumnFamilyHandle; }

namespace themis { namespace server {

struct PiiQueryFilter {
    std::string original_uuid;
    std::string pseudonym;
    bool active_only{false};
    int page{1};
    int page_size{100};
};

struct PiiMapping {
    std::string original_uuid;
    std::string pseudonym;
    bool active{true};
    std::string created_at; // ISO8601
    std::string updated_at; // ISO8601

    /**
     * @brief To Json.
     * @return Return value.
     */
    nlohmann::json toJson() const;
    /**
     * @brief From Json.
     * @param[in] j Input parameter.
     * @return Return value.
     */
    static PiiMapping fromJson(const nlohmann::json& j);
};


class PIIApiHandler {
public:
    PIIApiHandler() = default;
    PIIApiHandler(rocksdb::TransactionDB* db, rocksdb::ColumnFamilyHandle* cf);

    // CRUD
    /**
     * @brief Add Mapping.
     * @param[in] mapping Input parameter.
     * @return True when the operation succeeds.
     */
    bool addMapping(const PiiMapping& mapping); // false if duplicate
    /**
     * @brief Get Mapping.
     * @param[in] original_uuid Input parameter.
     * @return Return value.
     */
    std::optional<PiiMapping> getMapping(const std::string& original_uuid) const;
    /**
     * @brief Delete Mapping.
     * @param[in] original_uuid Input parameter.
     * @return True when the operation succeeds.
     */
    bool deleteMapping(const std::string& original_uuid); // hard delete

    /**
     * @brief List Mappings.
     * @param[in] filter Input parameter.
     * @return Return value.
     */
    nlohmann::json listMappings(const PiiQueryFilter& filter);

    /**
     * @brief Export Csv.
     * @param[in] filter Input parameter.
     * @return Return value.
     */
    std::string exportCsv(const PiiQueryFilter& filter);

    /**
     * @brief Delete By Uuid.
     * @param[in] uuid Input parameter.
     * @return Return value.
     */
    nlohmann::json deleteByUuid(const std::string& uuid);

private:
    rocksdb::TransactionDB* db_{nullptr};
    rocksdb::ColumnFamilyHandle* cf_{nullptr};

    static constexpr const char* KEY_PREFIX = "pii:";
    /**
     * @brief Make Key.
     * @param[in] uuid Input parameter.
     * @return Return value.
     * @details Calls: std::string().
     */
    static std::string makeKey(const std::string& uuid) { return std::string(KEY_PREFIX) + uuid; }
    /**
     * @brief Now Iso8601.
     * @return Return value.
     */
    static std::string nowIso8601();
};

}} // namespace themis::server
