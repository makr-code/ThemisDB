/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            result_cache.h                                     ║
  Version:         0.0.16                                             ║
  Last Modified:   2026-02-21 17:20:03                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     60                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
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

    virtual std::optional<Entry> Get(const Key& k) const = 0;
    virtual void Put(const Key& k, const Entry& e) = 0;
    virtual void InvalidatePlan(const std::string& plan_hash) = 0;
};

}} // namespace themis::cache
