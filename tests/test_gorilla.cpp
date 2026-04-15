/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_gorilla.cpp                                   ║
  Version:         0.0.45                                             ║
  Last Modified:   2026-04-15 07:18:29                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     85                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include <gtest/gtest.h>
#include "timeseries/gorilla.h"
#include <vector>
#include <cmath>

using namespace themis;

TEST(GorillaCodecTest, RoundtripBasic) {
    std::vector<std::pair<int64_t,double>> series;
    int64_t t0 = 1700000000000LL;
    for (int i = 0; i < 1000; ++i) {
        series.emplace_back(t0 + i * 1000, std::sin(i * 0.01));
    }
    GorillaEncoder enc;
    for (auto& p : series) enc.add(p.first, p.second);
    auto bytes = enc.finish();
    ASSERT_GT(bytes.size(), 0u);
    // Debug: print first few bytes to help diagnose encoding
    {
        size_t n = std::min<size_t>(bytes.size(), 32);
        fprintf(stderr, "[GORILLA TEST] encoded size=%zu first %zu bytes:\n", bytes.size(), n);
        for (size_t i = 0; i < n; ++i) {
            fprintf(stderr, "%02X ", bytes[i]);
        }
        fprintf(stderr, "\n");
    }

    GorillaDecoder dec(bytes);
    for (size_t i = 0; i < series.size(); ++i) {
        auto nxt = dec.next();
        ASSERT_TRUE(nxt.has_value());
        EXPECT_EQ(nxt->first, series[i].first);
        EXPECT_NEAR(nxt->second, series[i].second, 1e-12);
    }
    // No extra points
    EXPECT_FALSE(dec.next().has_value());
}

TEST(GorillaCodecTest, CompressionMonotonic) {
    std::vector<std::pair<int64_t,double>> series;
    int64_t t0 = 1700000000000LL;
    for (int i = 0; i < 2000; ++i) {
        series.emplace_back(t0 + i * 1000, i * 0.001);
    }
    GorillaEncoder enc;
    for (auto& p : series) enc.add(p.first, p.second);
    auto bytes = enc.finish();
    // Uncompressed: 16 bytes per point = 32KB
    // Our simplified implementation (always writing new headers) achieves ~8.7 bytes/point
    // Timestamps compress extremely well with delta-of-delta (constant delta = 0 bits after first)
    // Values use full XOR encoding with headers
    size_t uncompressed = series.size() * (sizeof(int64_t) + sizeof(double));
    EXPECT_LT(bytes.size(), uncompressed * 0.6); // expect at least 40% compression
    
    // Verify roundtrip correctness
    GorillaDecoder dec(bytes);
    for (size_t i = 0; i < series.size(); ++i) {
        auto nxt = dec.next();
        ASSERT_TRUE(nxt.has_value());
        EXPECT_EQ(nxt->first, series[i].first);
        EXPECT_NEAR(nxt->second, series[i].second, 1e-12);
    }
}
