/*
 * ThemisDB | File: cache_provider.h | Version: 0.0.47 | Last Modified: 2026-05-20 17:13:04
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 33
 * Open Issues: TODOs=1, Stubs=1, Gaps=3, Unimpl=0, Mock=1, Sim=0, Debt=0
 * Gap Correlation: internal=3 | external_v3=n/a | delta=n/a | status=no_external_data
 * External Severity (v3): C=n/a, H=n/a, M=n/a
 * PR: none
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
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
    [[nodiscard]] virtual bool Get(std::string_view key, CacheValue& out) = 0;
    virtual void Put(std::string_view key, const CacheValue& v, uint64_t ttl_ms) = 0;
    virtual void Invalidate(std::string_view key) = 0;
};

inline std::string makeEntityKey(const std::string& urn) { return urn; }

}} // namespace themis::cache
