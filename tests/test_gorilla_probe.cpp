/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_gorilla_probe.cpp                             ║
  Version:         0.0.10                                             ║
  Last Modified:   2026-02-21 13:57:24                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     55                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include <gtest/gtest.h>
#include "timeseries/gorilla.h"
#include <cstdio>
#include <vector>
#include <cmath>

using namespace themis;

TEST(GorillaProbe, FindFirstMismatch) {
    std::vector<std::pair<int64_t,double>> series;
    int64_t t0 = 1700000000000LL;
    for (int i = 0; i < 1000; ++i) {
        series.emplace_back(t0 + i * 1000, i == 0 ? 0.0 : std::sin(i * 0.01));
    }
    
    GorillaEncoder enc;
    for (auto &p : series) enc.add(p.first, p.second);
    auto bytes = enc.finish();

    GorillaDecoder dec(bytes);
    for (size_t i = 0; i < series.size(); ++i) {
        auto nxt = dec.next();
        ASSERT_TRUE(nxt.has_value()) << "decode ended early at i=" << i;
        EXPECT_EQ(nxt->first, series[i].first) << "timestamp mismatch at i=" << i;
        EXPECT_NEAR(nxt->second, series[i].second, 1e-12) << "value mismatch at i=" << i;
    }
    auto extra = dec.next();
    EXPECT_FALSE(extra.has_value()) << "decoder produced extra values";
}
