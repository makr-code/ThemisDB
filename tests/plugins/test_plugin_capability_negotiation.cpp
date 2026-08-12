// Test: Plugin Capability Negotiation (Version Ranges)
// Tests for PluginCapabilityNegotiator, PluginVersionRange, and related types.
// Also tests PluginManager::negotiateCapabilities().

#include <gtest/gtest.h>
#include "plugins/plugin_interface.h"
#include "plugins/plugin_manager.h"
#include <string>
#include <vector>

using namespace themis::plugins;

// ============================================================================
// Test helpers
// ============================================================================

// Minimal concrete IThemisPlugin for testing
class MockPlugin : public IThemisPlugin {
public:
    std::string name_{"test_plugin"};
    std::string version_{"1.0.0"};
    PluginCapabilities caps_{};

    const char* getName() const override { return name_.c_str(); }
    const char* getVersion() const override { return version_.c_str(); }
    PluginType getType() const override { return PluginType::CUSTOM; }
    PluginCapabilities getCapabilities() const override { return caps_; }
    bool initialize(const char*) override { return true; }
    void shutdown() override {}
    void* getInstance() override { return nullptr; }
};

// ============================================================================
// PluginVersionRange tests
// ============================================================================

TEST(PluginVersionRange, IsUnconstrainedWhenBothEmpty) {
    PluginVersionRange range;
    EXPECT_TRUE(range.isUnconstrained());
}

TEST(PluginVersionRange, IsConstrainedWhenMinSet) {
    PluginVersionRange range{"1.0.0", ""};
    EXPECT_FALSE(range.isUnconstrained());
}

TEST(PluginVersionRange, IsConstrainedWhenMaxSet) {
    PluginVersionRange range{"", "2.0.0"};
    EXPECT_FALSE(range.isUnconstrained());
}

// ============================================================================
// PluginCapabilityNegotiator::isVersionInRange tests
// ============================================================================

TEST(PluginCapabilityNegotiatorVersionRange, UnconstrainedAcceptsAnyVersion) {
    PluginVersionRange range;
    EXPECT_TRUE(PluginCapabilityNegotiator::isVersionInRange("0.0.1", range));
    EXPECT_TRUE(PluginCapabilityNegotiator::isVersionInRange("99.99.99", range));
    EXPECT_TRUE(PluginCapabilityNegotiator::isVersionInRange("", range));
}

TEST(PluginCapabilityNegotiatorVersionRange, EmptyVersionRejectsConstrainedRange) {
    PluginVersionRange range{"1.0.0", ""};
    EXPECT_FALSE(PluginCapabilityNegotiator::isVersionInRange("", range));
}

TEST(PluginCapabilityNegotiatorVersionRange, ExactVersionMatchesRange) {
    PluginVersionRange range{"1.2.3", "1.2.3"};
    EXPECT_TRUE(PluginCapabilityNegotiator::isVersionInRange("1.2.3", range));
}

TEST(PluginCapabilityNegotiatorVersionRange, MinBoundInclusive) {
    PluginVersionRange range{"1.0.0", ""};
    EXPECT_TRUE(PluginCapabilityNegotiator::isVersionInRange("1.0.0", range));
    EXPECT_TRUE(PluginCapabilityNegotiator::isVersionInRange("2.0.0", range));
    EXPECT_FALSE(PluginCapabilityNegotiator::isVersionInRange("0.9.9", range));
}

TEST(PluginCapabilityNegotiatorVersionRange, MaxBoundInclusive) {
    PluginVersionRange range{"", "2.0.0"};
    EXPECT_TRUE(PluginCapabilityNegotiator::isVersionInRange("2.0.0", range));
    EXPECT_TRUE(PluginCapabilityNegotiator::isVersionInRange("1.5.0", range));
    EXPECT_FALSE(PluginCapabilityNegotiator::isVersionInRange("2.0.1", range));
}

TEST(PluginCapabilityNegotiatorVersionRange, FullRangeAcceptsVersionsWithin) {
    PluginVersionRange range{"1.0.0", "2.0.0"};
    EXPECT_TRUE(PluginCapabilityNegotiator::isVersionInRange("1.0.0", range));
    EXPECT_TRUE(PluginCapabilityNegotiator::isVersionInRange("1.5.0", range));
    EXPECT_TRUE(PluginCapabilityNegotiator::isVersionInRange("2.0.0", range));
    EXPECT_FALSE(PluginCapabilityNegotiator::isVersionInRange("0.9.9", range));
    EXPECT_FALSE(PluginCapabilityNegotiator::isVersionInRange("2.0.1", range));
}

TEST(PluginCapabilityNegotiatorVersionRange, MinorVersionComparison) {
    PluginVersionRange range{"1.2.0", "1.3.0"};
    EXPECT_TRUE(PluginCapabilityNegotiator::isVersionInRange("1.2.5", range));
    EXPECT_FALSE(PluginCapabilityNegotiator::isVersionInRange("1.1.9", range));
    EXPECT_FALSE(PluginCapabilityNegotiator::isVersionInRange("1.3.1", range));
}

TEST(PluginCapabilityNegotiatorVersionRange, PatchVersionComparison) {
    PluginVersionRange range{"1.0.1", "1.0.3"};
    EXPECT_TRUE(PluginCapabilityNegotiator::isVersionInRange("1.0.2", range));
    EXPECT_FALSE(PluginCapabilityNegotiator::isVersionInRange("1.0.0", range));
    EXPECT_FALSE(PluginCapabilityNegotiator::isVersionInRange("1.0.4", range));
}

// ============================================================================
// PluginCapabilityNegotiator::checkCapability tests
// ============================================================================

TEST(PluginCapabilityNegotiatorCheckCapability, StreamingFlagRespected) {
    PluginCapabilities caps;
    caps.supports_streaming = true;
    EXPECT_TRUE(PluginCapabilityNegotiator::checkCapability("streaming", caps));
    caps.supports_streaming = false;
    EXPECT_FALSE(PluginCapabilityNegotiator::checkCapability("streaming", caps));
}

TEST(PluginCapabilityNegotiatorCheckCapability, BatchingFlagRespected) {
    PluginCapabilities caps;
    caps.supports_batching = true;
    EXPECT_TRUE(PluginCapabilityNegotiator::checkCapability("batching", caps));
    caps.supports_batching = false;
    EXPECT_FALSE(PluginCapabilityNegotiator::checkCapability("batching", caps));
}

TEST(PluginCapabilityNegotiatorCheckCapability, TransactionsFlagRespected) {
    PluginCapabilities caps;
    caps.supports_transactions = true;
    EXPECT_TRUE(PluginCapabilityNegotiator::checkCapability("transactions", caps));
}

TEST(PluginCapabilityNegotiatorCheckCapability, ThreadSafeFlagRespected) {
    PluginCapabilities caps;
    caps.thread_safe = true;
    EXPECT_TRUE(PluginCapabilityNegotiator::checkCapability("thread_safe", caps));
}

TEST(PluginCapabilityNegotiatorCheckCapability, GpuAcceleratedFlagRespected) {
    PluginCapabilities caps;
    caps.gpu_accelerated = true;
    EXPECT_TRUE(PluginCapabilityNegotiator::checkCapability("gpu_accelerated", caps));
}

TEST(PluginCapabilityNegotiatorCheckCapability, UnknownCapabilityReturnsFalse) {
    PluginCapabilities caps;
    caps.supports_streaming = true;
    EXPECT_FALSE(PluginCapabilityNegotiator::checkCapability("nonexistent", caps));
    EXPECT_FALSE(PluginCapabilityNegotiator::checkCapability("", caps));
}

// ============================================================================
// PluginCapabilityNegotiator::negotiate tests
// ============================================================================

TEST(PluginCapabilityNegotiatorNegotiate, EmptyRequirementsSucceeds) {
    MockPlugin plugin;
    auto result = PluginCapabilityNegotiator::negotiate(plugin, {});
    EXPECT_TRUE(result.success);
    EXPECT_TRUE(result.satisfied.empty());
    EXPECT_TRUE(result.unsatisfied.empty());
    EXPECT_TRUE(result.error_message.empty());
}

TEST(PluginCapabilityNegotiatorNegotiate, SatisfiedCapabilityWithNoVersionRange) {
    MockPlugin plugin;
    plugin.caps_.supports_streaming = true;

    std::vector<PluginCapabilityRequirement> reqs{
        {"streaming", {}}
    };
    auto result = PluginCapabilityNegotiator::negotiate(plugin, reqs);

    EXPECT_TRUE(result.success);
    ASSERT_EQ(result.satisfied.size(), 1u);
    EXPECT_EQ(result.satisfied[0], "streaming");
    EXPECT_TRUE(result.unsatisfied.empty());
}

TEST(PluginCapabilityNegotiatorNegotiate, UnsupportedCapabilityFails) {
    MockPlugin plugin;
    plugin.caps_.supports_streaming = false;

    std::vector<PluginCapabilityRequirement> reqs{
        {"streaming", {}}
    };
    auto result = PluginCapabilityNegotiator::negotiate(plugin, reqs);

    EXPECT_FALSE(result.success);
    ASSERT_EQ(result.unsatisfied.size(), 1u);
    EXPECT_EQ(result.unsatisfied[0], "streaming");
    EXPECT_FALSE(result.error_message.empty());
}

TEST(PluginCapabilityNegotiatorNegotiate, VersionInRangeSucceeds) {
    MockPlugin plugin;
    plugin.version_ = "1.5.0";
    plugin.caps_.supports_batching = true;

    std::vector<PluginCapabilityRequirement> reqs{
        {"batching", {"1.0.0", "2.0.0"}}
    };
    auto result = PluginCapabilityNegotiator::negotiate(plugin, reqs);

    EXPECT_TRUE(result.success);
    ASSERT_EQ(result.satisfied.size(), 1u);
    EXPECT_EQ(result.satisfied[0], "batching");
}

TEST(PluginCapabilityNegotiatorNegotiate, VersionBelowMinFails) {
    MockPlugin plugin;
    plugin.version_ = "0.9.0";
    plugin.caps_.supports_batching = true;

    std::vector<PluginCapabilityRequirement> reqs{
        {"batching", {"1.0.0", ""}}
    };
    auto result = PluginCapabilityNegotiator::negotiate(plugin, reqs);

    EXPECT_FALSE(result.success);
    ASSERT_EQ(result.unsatisfied.size(), 1u);
    EXPECT_NE(result.error_message.find("out of range"), std::string::npos);
}

TEST(PluginCapabilityNegotiatorNegotiate, VersionAboveMaxFails) {
    MockPlugin plugin;
    plugin.version_ = "3.0.0";
    plugin.caps_.supports_batching = true;

    std::vector<PluginCapabilityRequirement> reqs{
        {"batching", {"", "2.0.0"}}
    };
    auto result = PluginCapabilityNegotiator::negotiate(plugin, reqs);

    EXPECT_FALSE(result.success);
    ASSERT_EQ(result.unsatisfied.size(), 1u);
}

TEST(PluginCapabilityNegotiatorNegotiate, MultipleRequirementsAllSatisfied) {
    MockPlugin plugin;
    plugin.version_ = "1.2.0";
    plugin.caps_.supports_streaming = true;
    plugin.caps_.supports_batching = true;
    plugin.caps_.thread_safe = true;

    std::vector<PluginCapabilityRequirement> reqs{
        {"streaming",   {"1.0.0", "2.0.0"}},
        {"batching",    {}},
        {"thread_safe", {"1.0.0", ""}},
    };
    auto result = PluginCapabilityNegotiator::negotiate(plugin, reqs);

    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.satisfied.size(), 3u);
    EXPECT_TRUE(result.unsatisfied.empty());
}

TEST(PluginCapabilityNegotiatorNegotiate, MultipleRequirementsPartialFailure) {
    MockPlugin plugin;
    plugin.version_ = "1.2.0";
    plugin.caps_.supports_streaming = true;
    plugin.caps_.supports_batching = false;

    std::vector<PluginCapabilityRequirement> reqs{
        {"streaming", {}},
        {"batching",  {}},
    };
    auto result = PluginCapabilityNegotiator::negotiate(plugin, reqs);

    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.satisfied.size(), 1u);
    EXPECT_EQ(result.unsatisfied.size(), 1u);
    EXPECT_EQ(result.satisfied[0], "streaming");
    EXPECT_EQ(result.unsatisfied[0], "batching");
}

TEST(PluginCapabilityNegotiatorNegotiate, ErrorMessageListsAllFailures) {
    MockPlugin plugin;
    plugin.version_ = "0.5.0";
    plugin.caps_.supports_streaming = false;
    plugin.caps_.supports_batching = true;

    std::vector<PluginCapabilityRequirement> reqs{
        {"streaming", {}},
        {"batching",  {"1.0.0", ""}},
    };
    auto result = PluginCapabilityNegotiator::negotiate(plugin, reqs);

    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.unsatisfied.size(), 2u);
    // Error message should mention both failures
    EXPECT_NE(result.error_message.find("streaming"), std::string::npos);
    EXPECT_NE(result.error_message.find("batching"), std::string::npos);
}

TEST(PluginCapabilityNegotiatorNegotiate, NullVersionTreatedAsEmpty) {
    // A plugin that returns nullptr from getVersion() should behave like ""
    class NullVersionPlugin : public IThemisPlugin {
    public:
        const char* getName() const override { return "null_ver"; }
        const char* getVersion() const override { return nullptr; }
        PluginType getType() const override { return PluginType::CUSTOM; }
        PluginCapabilities getCapabilities() const override {
            PluginCapabilities c;
            c.supports_streaming = true;
            return c;
        }
        bool initialize(const char*) override { return true; }
        void shutdown() override {}
        void* getInstance() override { return nullptr; }
    };

    NullVersionPlugin plugin;

    // Unconstrained range: should succeed
    auto r1 = PluginCapabilityNegotiator::negotiate(plugin, {{"streaming", {}}});
    EXPECT_TRUE(r1.success);

    // Constrained range: null/empty version fails
    auto r2 = PluginCapabilityNegotiator::negotiate(plugin,
        {{"streaming", {"1.0.0", ""}}});
    EXPECT_FALSE(r2.success);
}

// ============================================================================
// PluginManager::negotiateCapabilities tests
// ============================================================================

TEST(PluginManagerNegotiateCapabilities, UnknownPluginReturnsFalse) {
    PluginManager mgr;
    auto result = mgr.negotiateCapabilities("nonexistent_plugin", {});
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.error_message.empty());
    EXPECT_NE(result.error_message.find("nonexistent_plugin"), std::string::npos);
}

TEST(PluginManagerNegotiateCapabilities, UnknownPluginWithRequirementsReturnsFalse) {
    PluginManager mgr;
    std::vector<PluginCapabilityRequirement> reqs{{"streaming", {}}};
    auto result = mgr.negotiateCapabilities("missing", reqs);
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.error_message.empty());
    EXPECT_NE(result.error_message.find("missing"), std::string::npos);
}
