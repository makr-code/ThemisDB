/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            result_cache.h                                     ║
  Version:         0.0.43                                             ║
  Last Modified:   2026-04-15 04:09:28                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     57                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • a629043ab2  2026-02-22  Audit: document gaps found - benchmarks and stale annotat... ║
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
