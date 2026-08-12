#include <gtest/gtest.h>
#include "themis/gpu/alerts.h"
#include <atomic>
#include <thread>
#include <vector>

using namespace themis::gpu;

static GPUAlerts::Config DefaultConfig() {
    GPUAlerts::Config cfg;
    cfg.vram_high_threshold     = 0.80f;
    cfg.error_rate_threshold    = 0.10f;
    cfg.fallback_rate_threshold = 0.20f;
    return cfg;
}

// ---------------------------------------------------------------------------
// Initial state — no alerts fire without evaluate()
// ---------------------------------------------------------------------------

TEST(GPUAlertsTest, InitialState_NoFiringAlerts) {
    GPUAlerts alerts(DefaultConfig());
    EXPECT_EQ(alerts.firingCount(), 0u);
    EXPECT_FALSE(alerts.isFiring(GPUAlerts::ALERT_VRAM_HIGH));
}

TEST(GPUAlertsTest, Evaluate_AllMetricsNominal_NoAlerts) {
    GPUAlerts alerts(DefaultConfig());
    alerts.setVRAMUsage(0.50f);
    alerts.setErrorRate(0.01f);
    alerts.setFallbackRate(0.05f);
    alerts.setCircuitOpen(false);
    alerts.setDeviceAvailable(true);
    EXPECT_EQ(alerts.evaluate(), 0u);
}

// ---------------------------------------------------------------------------
// VRAM_HIGH
// ---------------------------------------------------------------------------

TEST(GPUAlertsTest, VRAMHigh_Fires_WhenAtThreshold) {
    GPUAlerts alerts(DefaultConfig());
    alerts.setVRAMUsage(0.80f);  // exactly at threshold
    EXPECT_GE(alerts.evaluate(), 1u);
    EXPECT_TRUE(alerts.isFiring(GPUAlerts::ALERT_VRAM_HIGH));
}

TEST(GPUAlertsTest, VRAMHigh_DoesNotFire_WhenBelowThreshold) {
    GPUAlerts alerts(DefaultConfig());
    alerts.setVRAMUsage(0.79f);
    alerts.evaluate();
    EXPECT_FALSE(alerts.isFiring(GPUAlerts::ALERT_VRAM_HIGH));
}

TEST(GPUAlertsTest, VRAMHigh_Resolves_WhenDropsBelow) {
    GPUAlerts alerts(DefaultConfig());
    alerts.setVRAMUsage(0.90f);
    alerts.evaluate();
    EXPECT_TRUE(alerts.isFiring(GPUAlerts::ALERT_VRAM_HIGH));

    alerts.setVRAMUsage(0.60f);
    alerts.evaluate();
    EXPECT_FALSE(alerts.isFiring(GPUAlerts::ALERT_VRAM_HIGH));
}

// ---------------------------------------------------------------------------
// ERROR_RATE_HIGH
// ---------------------------------------------------------------------------

TEST(GPUAlertsTest, ErrorRate_Fires_WhenAboveThreshold) {
    GPUAlerts alerts(DefaultConfig());
    alerts.setErrorRate(0.15f);
    alerts.evaluate();
    EXPECT_TRUE(alerts.isFiring(GPUAlerts::ALERT_ERROR_RATE_HIGH));
}

TEST(GPUAlertsTest, ErrorRate_DoesNotFire_WhenBelow) {
    GPUAlerts alerts(DefaultConfig());
    alerts.setErrorRate(0.05f);
    alerts.evaluate();
    EXPECT_FALSE(alerts.isFiring(GPUAlerts::ALERT_ERROR_RATE_HIGH));
}

// ---------------------------------------------------------------------------
// FALLBACK_RATE_HIGH
// ---------------------------------------------------------------------------

TEST(GPUAlertsTest, FallbackRate_Fires_WhenAboveThreshold) {
    GPUAlerts alerts(DefaultConfig());
    alerts.setFallbackRate(0.25f);
    alerts.evaluate();
    EXPECT_TRUE(alerts.isFiring(GPUAlerts::ALERT_FALLBACK_RATE));
}

// ---------------------------------------------------------------------------
// CIRCUIT_OPEN
// ---------------------------------------------------------------------------

TEST(GPUAlertsTest, CircuitOpen_Fires_WhenSet) {
    GPUAlerts alerts(DefaultConfig());
    alerts.setCircuitOpen(true);
    alerts.evaluate();
    EXPECT_TRUE(alerts.isFiring(GPUAlerts::ALERT_CIRCUIT_OPEN));
}

TEST(GPUAlertsTest, CircuitOpen_Resolves_WhenCleared) {
    GPUAlerts alerts(DefaultConfig());
    alerts.setCircuitOpen(true);
    alerts.evaluate();
    alerts.setCircuitOpen(false);
    alerts.evaluate();
    EXPECT_FALSE(alerts.isFiring(GPUAlerts::ALERT_CIRCUIT_OPEN));
}

// ---------------------------------------------------------------------------
// DEVICE_UNAVAILABLE
// ---------------------------------------------------------------------------

TEST(GPUAlertsTest, DeviceUnavailable_Fires_WhenNoDevice) {
    GPUAlerts alerts(DefaultConfig());
    alerts.setDeviceAvailable(false);
    alerts.evaluate();
    EXPECT_TRUE(alerts.isFiring(GPUAlerts::ALERT_DEVICE_UNAVAIL));
}

TEST(GPUAlertsTest, DeviceUnavailable_Resolves_WhenRestored) {
    GPUAlerts alerts(DefaultConfig());
    alerts.setDeviceAvailable(false);
    alerts.evaluate();
    alerts.setDeviceAvailable(true);
    alerts.evaluate();
    EXPECT_FALSE(alerts.isFiring(GPUAlerts::ALERT_DEVICE_UNAVAIL));
}

// ---------------------------------------------------------------------------
// Multiple alerts firing simultaneously
// ---------------------------------------------------------------------------

TEST(GPUAlertsTest, MultipleAlerts_AllFire) {
    GPUAlerts alerts(DefaultConfig());
    alerts.setVRAMUsage(0.95f);
    alerts.setErrorRate(0.30f);
    alerts.setCircuitOpen(true);
    alerts.setDeviceAvailable(false);
    const size_t n = alerts.evaluate();
    EXPECT_GE(n, 4u);
}

// ---------------------------------------------------------------------------
// Callback on transition
// ---------------------------------------------------------------------------

TEST(GPUAlertsTest, Callback_CalledOnFire) {
    GPUAlerts alerts(DefaultConfig());
    std::atomic<int> fire_count{0};
    alerts.onAlert([&](const GPUAlerts::AlertStatus& s) {
        if (s.state == GPUAlerts::AlertState::FIRING) {
            fire_count.fetch_add(1);
        }
    });
    alerts.setVRAMUsage(0.90f);
    alerts.evaluate();
    EXPECT_GE(fire_count.load(), 1);
}

TEST(GPUAlertsTest, Callback_CalledOnResolve) {
    GPUAlerts alerts(DefaultConfig());
    std::atomic<int> resolve_count{0};
    alerts.onAlert([&](const GPUAlerts::AlertStatus& s) {
        if (s.state == GPUAlerts::AlertState::INACTIVE) {
            resolve_count.fetch_add(1);
        }
    });
    alerts.setVRAMUsage(0.90f);
    alerts.evaluate();
    alerts.setVRAMUsage(0.30f);
    alerts.evaluate();
    EXPECT_GE(resolve_count.load(), 1);
}

TEST(GPUAlertsTest, Callback_NotCalledWhenStateUnchanged) {
    GPUAlerts alerts(DefaultConfig());
    std::atomic<int> call_count{0};
    alerts.onAlert([&](const GPUAlerts::AlertStatus&) {
        call_count.fetch_add(1);
    });
    alerts.setVRAMUsage(0.90f);
    alerts.evaluate();
    const int after_first = call_count.load();
    // Second evaluate with same value — no state change.
    alerts.evaluate();
    EXPECT_EQ(call_count.load(), after_first);
}

// ---------------------------------------------------------------------------
// currentStatuses
// ---------------------------------------------------------------------------

TEST(GPUAlertsTest, CurrentStatuses_ContainsAllAlerts_AfterEvaluate) {
    GPUAlerts alerts(DefaultConfig());
    alerts.evaluate();
    const auto statuses = alerts.currentStatuses();
    EXPECT_EQ(statuses.size(), 5u);
}

TEST(GPUAlertsTest, AlertStatus_HasMessage) {
    GPUAlerts alerts(DefaultConfig());
    alerts.setVRAMUsage(0.90f);
    alerts.evaluate();
    const auto statuses = alerts.currentStatuses();
    for (const auto& s : statuses) {
        if (s.name == GPUAlerts::ALERT_VRAM_HIGH) {
            EXPECT_FALSE(s.message.empty());
        }
    }
}
