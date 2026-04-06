/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_auto_failover_manager.cpp                     ║
  Version:         0.0.1                                              ║
  Last Modified:   2026-04-06 04:25:19                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     69                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 5bee4e8e41  2026-04-03  Implement Disaster Recovery Manager and associated tests ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include <chrono>
#include <memory>
#include <thread>

#include <gtest/gtest.h>

#include "failover/auto_failover_manager.h"

using namespace std::chrono_literals;

namespace themis::failover::test {

TEST(AutoFailoverManagerFocusedTest, StartStopLifecycle) {
    AutoFailoverConfig cfg;
    cfg.health_check_interval = 10ms;

    AutoFailoverManager mgr(
        cfg,
        nullptr,
        nullptr,
        nullptr,
        nullptr
    );

    EXPECT_FALSE(mgr.isRunning());
    EXPECT_TRUE(mgr.start());
    EXPECT_TRUE(mgr.isRunning());

    std::this_thread::sleep_for(30ms);

    EXPECT_TRUE(mgr.stop());
    EXPECT_FALSE(mgr.isRunning());
}

TEST(AutoFailoverManagerFocusedTest, RejectsFailoverWhenStopped) {
    AutoFailoverConfig cfg;
    AutoFailoverManager mgr(
        cfg,
        nullptr,
        nullptr,
        nullptr,
        nullptr
    );

    EXPECT_FALSE(mgr.triggerManualFailover("node-a"));
}

TEST(AutoFailoverManagerFocusedTest, AcceptsManualFailoverWhenRunning) {
    AutoFailoverConfig cfg;
    cfg.health_check_interval = 10ms;

    AutoFailoverManager mgr(
        cfg,
        nullptr,
        nullptr,
        nullptr,
        nullptr
    );

    ASSERT_TRUE(mgr.start());
    EXPECT_TRUE(mgr.triggerManualFailover("node-a"));

    std::this_thread::sleep_for(50ms);

    EXPECT_FALSE(mgr.isFailoverInProgress());
    ASSERT_TRUE(mgr.stop());
}

}  // namespace themis::failover::test
