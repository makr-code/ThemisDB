/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            cache_provider.h                                   ║
  Version:         0.0.22                                             ║
  Last Modified:   2026-02-21 19:28:49                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     51                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 31e8b8df0  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 0d722b04c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <string>
#include <string_view>
#include <cstdint>

namespace themis { namespace cache {

struct CacheValue {
    std::string payload;   // serialized JSON (entity or result page)
    uint64_t version{0};   // monotone version (e.g., WAL index)
    uint64_t ts_ms{0};     // insert timestamp (ms)
};

class CacheProvider {
public:
    virtual ~CacheProvider() = default;
    virtual bool Get(std::string_view key, CacheValue& out) = 0;
    virtual void Put(std::string_view key, const CacheValue& v, uint64_t ttl_ms) = 0;
    virtual void Invalidate(std::string_view key) = 0;
};

inline std::string makeEntityKey(const std::string& urn) { return urn; }

}} // namespace themis::cache
