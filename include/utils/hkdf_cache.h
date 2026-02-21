/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            hkdf_cache.h                                       ║
  Version:         0.0.7                                              ║
  Last Modified:   2026-02-21 11:48:31                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     64                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// Thread-local HKDF LRU cache (single-definition header)
#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <memory>

namespace themis {
namespace utils {

class HKDFCache {
public:
    // Return a thread-local instance (preferred by callers in code/tests)
    static HKDFCache& threadLocal();

    // Derive with cache: ikm, salt, info, output_length
    // NOTE: function name matches existing callers: derive_cached
    std::vector<uint8_t> derive_cached(const std::vector<uint8_t>& ikm,
                                       const std::vector<uint8_t>& salt,
                                       const std::string& info,
                                       size_t output_length);

    // Clear cache (useful for tests or explicit invalidation)
    void clear();

    // Configure capacity (default ~1024)
    void setCapacity(size_t cap);

private:
    HKDFCache();
    ~HKDFCache();
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace utils
} // namespace themis
