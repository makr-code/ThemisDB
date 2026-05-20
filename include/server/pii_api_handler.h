/*
 * ThemisDB | File: pii_api_handler.h | Version: 0.0.47
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
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

    nlohmann::json toJson() const;
    static PiiMapping fromJson(const nlohmann::json& j);
};

class PIIApiHandler {
public:
    PIIApiHandler() = default;
    PIIApiHandler(rocksdb::TransactionDB* db, rocksdb::ColumnFamilyHandle* cf);

    // CRUD
    bool addMapping(const PiiMapping& mapping); // false if duplicate
    std::optional<PiiMapping> getMapping(const std::string& original_uuid) const;
    bool deleteMapping(const std::string& original_uuid); // hard delete

    // Listing helpers
    // Returns a JSON object: { "items": [ ... ], "total": N, "page": p, "page_size": s }
    nlohmann::json listMappings(const PiiQueryFilter& filter);

    // Returns CSV string with header
    std::string exportCsv(const PiiQueryFilter& filter);

    // Backward-compatible demo method retained (delegates to deleteMapping)
    nlohmann::json deleteByUuid(const std::string& uuid);

private:
    rocksdb::TransactionDB* db_{nullptr};
    rocksdb::ColumnFamilyHandle* cf_{nullptr};

    static constexpr const char* KEY_PREFIX = "pii:";
    static std::string makeKey(const std::string& uuid) { return std::string(KEY_PREFIX) + uuid; }
    static std::string nowIso8601();
};

}} // namespace themis::server
