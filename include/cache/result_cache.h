/*
 * ThemisDB | File: result_cache.h | Version: 0.0.47 | Last Modified: 2026-05-20 17:13:04
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 42
 * Open Issues: TODOs=1, Stubs=1, Gaps=3, Unimpl=0, Mock=1, Sim=0, Debt=0
 * Gap Correlation: internal=3 | external_v3=n/a | delta=n/a | status=no_external_data
 * External Severity (v3): C=n/a, H=n/a, M=n/a
 * PR: none
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#pragma once

#include <string>
#include <vector>
#include <optional>
#include <cstdint>

namespace themis { namespace cache {

// Query-Result-Cache (AQL) – speichert seitenweise Ergebnisse unter Plan-Hash
class ResultCache {
public:
    virtual ~ResultCache() = default;

    // Key-Komponenten
    struct Key {
        std::string plan_hash;    // normalized query + params
        std::string namespace_;   // tenant isolation
        std::string shard_scope;  // all|namespace|single-shard
        uint64_t page{0};         // page index
    };

    struct Entry {
        std::string page_json;    // serialized JSON array
        uint64_t ts_ms{0};        // insert timestamp
        uint64_t ttl_ms{0};       // time to live
    };

    [[nodiscard]] virtual std::optional<Entry> Get(const Key& k) const = 0;
    virtual void Put(const Key& k, const Entry& e) = 0;
    virtual void InvalidatePlan(const std::string& plan_hash) = 0;
};

}} // namespace themis::cache
