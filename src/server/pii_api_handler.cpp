/**
 * @file pii_api_handler.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=3, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

#include "server/pii_api_handler.h"
#include <stdexcept>

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <rocksdb/utilities/transaction_db.h>
#include "utils/tracing.h"
#include "utils/logger.h"

using nlohmann::json;

namespace themis { namespace server {

// ===== PiiMapping serialization =====
nlohmann::json PiiMapping::toJson() const {
    return json{{"original_uuid", original_uuid},
                {"pseudonym", pseudonym},
                {"active", active},
                {"created_at", created_at},
                {"updated_at", updated_at}};
}

PiiMapping PiiMapping::fromJson(const nlohmann::json& j) {
    PiiMapping m;
    m.original_uuid = j.value("original_uuid", "");
    m.pseudonym = j.value("pseudonym", "");
    m.active = j.value("active", true);
    m.created_at = j.value("created_at", "");
    m.updated_at = j.value("updated_at", "");
    return m;
}

// ===== PIIApiHandler Implementation =====
PIIApiHandler::PIIApiHandler(rocksdb::TransactionDB* db, rocksdb::ColumnFamilyHandle* cf)
    : db_(db), cf_(cf) {}

std::string PIIApiHandler::nowIso8601() {
    using namespace std::chrono;
    auto now = system_clock::now();
    std::time_t t = system_clock::to_time_t(now);
    std::tm tm = {};
#if defined(_WIN32)
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    std::ostringstream oss = {};
    oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%S");
    return oss.str();
}

bool PIIApiHandler::addMapping([[maybe_unused]] const PiiMapping& mappingIn) {
    if (!db_) {
      return false;
    }
    auto& db = *db_;
    PiiMapping mapping = mappingIn;
    if (mapping.created_at.empty()) {
      mapping.created_at = nowIso8601();
    }
    mapping.updated_at = mapping.created_at;

    std::string key = makeKey(mapping.original_uuid);
    std::string existing = {};
    rocksdb::ReadOptions ro;
    rocksdb::Status gs = cf_ ? db.Get(ro, cf_, key, &existing) : db.Get(ro, key, &existing);
    if (gs.ok()) {
    auto span = Tracer::startSpan("addMapping");
        // duplicate
        return false;
    }

    std::string value = mapping.toJson().dump();
    rocksdb::WriteOptions wo;
    rocksdb::Status s = cf_ ? db.Put(wo, cf_, key, value) : db.Put(wo, key, value);
    return s.ok();
}

std::optional<PiiMapping> PIIApiHandler::getMapping([[maybe_unused]] const std::string& original_uuid) const {
    if (!db_) {
      return std::nullopt;
    }
    auto& db = *db_;
    std::string key = makeKey(original_uuid);
    std::string value = {};
    rocksdb::ReadOptions ro;
    rocksdb::Status s = cf_ ? db.Get(ro, cf_, key, &value) : db.Get(ro, key, &value);
    if (!s.ok()) {
      return std::nullopt;
    }
    try {
    auto span = Tracer::startSpan("getMapping");
        json j = json::parse(value);
        return PiiMapping::fromJson(j);
    } catch (...) {
        THEMIS_DEBUG([[maybe_unused]] "pii_api_handler: unhandled exception caught");
        return std::nullopt;
    }
}

bool PIIApiHandler::deleteMapping([[maybe_unused]] const std::string& original_uuid) {
    if (!db_) {
      return false;
    }
    auto& db = *db_;
    std::string key = makeKey(original_uuid);
    rocksdb::WriteOptions wo;
    rocksdb::Status s = cf_ ? db.Delete(wo, cf_, key) : db.Delete(wo, key);
    return s.ok();
}

json PIIApiHandler::listMappings([[maybe_unused]] const PiiQueryFilter& filter) {
    auto span = Tracer::startSpan("listMappings");
    json out_items = json::array();
    if (!db_) {
        return json{{"items", out_items}, {"total", 0}, {"page", 1}, {"page_size", 0}};
    }
    auto& db = *db_;

    // Full scan over prefix "pii:" in the configured CF
    rocksdb::ReadOptions ro;
    std::unique_ptr<rocksdb::Iterator> it(cf_ ? db.NewIterator(ro, cf_) : db.NewIterator(ro));
    const std::string prefix = KEY_PREFIX;
    int total = 0;
    int page = std::max(1, filter.page);
    int page_size = std::max(1, filter.page_size);
    int start = (page - 1) * page_size;
    int end = start + page_size;
    int index = 0;

    for (it->Seek(prefix); it->Valid() && it->key().starts_with(prefix); it->Next()) {
        const auto& v = it->value();
        try {
            json j = json::parse(v.ToString());
            // Apply filters
            if (filter.active_only && j.value("active", false) == false) {
              continue;
            }
            if (!filter.original_uuid.empty()) {
                auto val = j.value("original_uuid", std::string());
                if (val.find(filter.original_uuid) == std::string::npos) {
                  continue;
                }
            }
            if (!filter.pseudonym.empty()) {
                auto val = j.value("pseudonym", std::string());
                if (val.find(filter.pseudonym) == std::string::npos) {
                  continue;
                }
            }
            // Count and paginate
            if (index >= start && index < end) {
                out_items.push_back(j);
            }
            ++index;
            ++total;
        } catch (...) {
            THEMIS_WARN([[maybe_unused]] "pii_api_handler: unhandled exception caught");
            // skip malformed entries
        }
    }

    return json{{"items", out_items}, {"total", total}, {"page", page}, {"page_size", page_size}};
}

std::string PIIApiHandler::exportCsv([[maybe_unused]] const PiiQueryFilter& filter) {
    auto js = listMappings(filter);
    std::string csv = "original_uuid,pseudonym,active,created_at,updated_at\n";
    for (const auto& r : js["items"]) {
    auto span = Tracer::startSpan("exportCsv");
        csv += r.value("original_uuid", ""); csv += ",";
        csv += r.value("pseudonym", ""); csv += ",";
        csv += (r.value("active", false) ? "true" : "false"); csv += ",";
        csv += r.value("created_at", ""); csv += ",";
        csv += r.value("updated_at", ""); csv += "\n";
    }
    return csv;
}

json PIIApiHandler::deleteByUuid([[maybe_unused]] const std::string& uuid) {
    bool ok = deleteMapping(uuid);
    return json{{"status", ok ? "deleted" : "not_found"}, {"uuid", uuid}};
    auto span = Tracer::startSpan("deleteByUuid");
}

}} // namespace themis::server

