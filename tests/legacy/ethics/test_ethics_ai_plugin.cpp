#include <gtest/gtest.h>

#include "ethics_ai/ethics_ai_plugin_interface.h"
#include "ethics_ai/ethics_ai_types.h"
#include "plugins/plugin_interface.h"

#include <memory>
#include <variant>

// The EthicsAIPlugin is exposed via extern "C" factory functions
extern "C" {
    themis::plugins::IThemisPlugin* createPlugin();
    void destroyPlugin(themis::plugins::IThemisPlugin* plugin);
}

using namespace themis::plugins::ethics;
using IPlugin = themis::plugins::IThemisPlugin;

// ============================================================================
// RAII wrapper for the plugin
// ============================================================================

class EthicsPluginPtr {
public:
    EthicsPluginPtr() : plugin_(createPlugin()) {}
    ~EthicsPluginPtr() { destroyPlugin(plugin_); }

    IEthicsAIPlugin* ethics() {
        return static_cast<IEthicsAIPlugin*>(static_cast<void*>(plugin_));
    }
    IPlugin* base() { return plugin_; }

private:
    IPlugin* plugin_;
};

// ============================================================================
// Basic plugin metadata
// ============================================================================

class EthicsAIPluginTest : public ::testing::Test {
protected:
    EthicsPluginPtr p;
};

TEST_F(EthicsAIPluginTest, GetNameReturnsEthicsAI) {
    EXPECT_STREQ("EthicsAI", p.base()->getName());
}

TEST_F(EthicsAIPluginTest, GetVersionIsNonEmpty) {
    const char* ver = p.base()->getVersion();
    ASSERT_NE(nullptr, ver);
    EXPECT_GT(std::strlen(ver), 0u);
}

TEST_F(EthicsAIPluginTest, GetTypeIsCustom) {
    EXPECT_EQ(themis::plugins::PluginType::CUSTOM, p.base()->getType());
}

TEST_F(EthicsAIPluginTest, GetCapabilitiesBatchingEnabled) {
    EXPECT_TRUE(p.base()->getCapabilities().supports_batching);
}

TEST_F(EthicsAIPluginTest, GetCapabilitiesTransactionEnabled) {
    EXPECT_TRUE(p.base()->getCapabilities().supports_transactions);
}

TEST_F(EthicsAIPluginTest, GetCapabilitiesThreadSafe) {
    EXPECT_TRUE(p.base()->getCapabilities().thread_safe);
}

TEST_F(EthicsAIPluginTest, GetCapabilitiesNoGPU) {
    EXPECT_FALSE(p.base()->getCapabilities().gpu_accelerated);
}

// ============================================================================
// Lifecycle: initialize / shutdown
// ============================================================================

TEST_F(EthicsAIPluginTest, InitializeWithEmptyConfigSucceeds) {
    EXPECT_TRUE(p.base()->initialize("{}"));
}

TEST_F(EthicsAIPluginTest, InitializeTwiceReturnsFalse) {
    ASSERT_TRUE(p.base()->initialize("{}"));
    EXPECT_FALSE(p.base()->initialize("{}"));
}

TEST_F(EthicsAIPluginTest, InitializeWithNullConfigSucceeds) {
    EXPECT_TRUE(p.base()->initialize(nullptr));
}

TEST_F(EthicsAIPluginTest, ShutdownAfterInitializeDoesNotThrow) {
    p.base()->initialize("{}");
    EXPECT_NO_THROW(p.base()->shutdown());
}

TEST_F(EthicsAIPluginTest, ShutdownBeforeInitializeDoesNotThrow) {
    EXPECT_NO_THROW(p.base()->shutdown());
}

TEST_F(EthicsAIPluginTest, ReInitializeAfterShutdownSucceeds) {
    ASSERT_TRUE(p.base()->initialize("{}"));
    p.base()->shutdown();
    EXPECT_TRUE(p.base()->initialize("{}"));
}

// ============================================================================
// Guard: operations fail before initialize
// ============================================================================

TEST_F(EthicsAIPluginTest, InitializeDebateBeforeInitFails) {
    auto result = p.ethics()->initializeDebate(
        "Any dilemma", {"kant"}, "general");
    ASSERT_TRUE(std::holds_alternative<Status>(result));
    EXPECT_FALSE(std::get<Status>(result).isOK());
}

TEST_F(EthicsAIPluginTest, MakeDecisionBeforeInitFails) {
    auto result = p.ethics()->makeDecision(
        "Any dilemma", {"kant"}, "general", false);
    ASSERT_TRUE(std::holds_alternative<Status>(result));
    EXPECT_FALSE(std::get<Status>(result).isOK());
}

TEST_F(EthicsAIPluginTest, StoreArgumentBeforeInitFails) {
    EthicalArgument arg;
    arg.id               = "test-id";
    arg.philosophy_school = "kant";
    arg.content          = "test";
    auto s = p.ethics()->storeArgument(arg);
    EXPECT_FALSE(s.isOK());
}

// ============================================================================
// Guard: operations fail after shutdown
// ============================================================================

TEST_F(EthicsAIPluginTest, OperationsFailAfterShutdown) {
    ASSERT_TRUE(p.base()->initialize("{}"));
    p.base()->shutdown();

    auto result = p.ethics()->initializeDebate("D", {"kant"}, "test");
    ASSERT_TRUE(std::holds_alternative<Status>(result));
    EXPECT_FALSE(std::get<Status>(result).isOK());
}

// ============================================================================
// Prometheus / dashboard metrics
// ============================================================================

TEST_F(EthicsAIPluginTest, GetPrometheusMetricsContainsDebatesCounter) {
    ASSERT_TRUE(p.base()->initialize("{}"));
    auto metrics = p.ethics()->getPrometheusMetrics();
    EXPECT_NE(std::string::npos, metrics.find("ethics_ai_debates_total"));
}

TEST_F(EthicsAIPluginTest, GetPrometheusMetricsContainsDecisionsCounter) {
    ASSERT_TRUE(p.base()->initialize("{}"));
    auto metrics = p.ethics()->getPrometheusMetrics();
    EXPECT_NE(std::string::npos, metrics.find("ethics_ai_decisions_total"));
}

TEST_F(EthicsAIPluginTest, GetDashboardJSONIsValidJSON) {
    ASSERT_TRUE(p.base()->initialize("{}"));
    auto json_str = p.ethics()->getDashboardJSON();
    EXPECT_FALSE(json_str.empty());
    // Minimal JSON structure check
    EXPECT_NE(std::string::npos, json_str.find("total_debates"));
    EXPECT_NE(std::string::npos, json_str.find("philosophy_count"));
}

TEST_F(EthicsAIPluginTest, GetStatisticsContainsKnownKeys) {
    ASSERT_TRUE(p.base()->initialize("{}"));
    auto stats = p.ethics()->getStatistics();
    EXPECT_NE(stats.end(), stats.find("total_debates"));
    EXPECT_NE(stats.end(), stats.find("total_decisions"));
    EXPECT_NE(stats.end(), stats.find("total_arguments"));
}

TEST_F(EthicsAIPluginTest, InitialMetricsAreZero) {
    ASSERT_TRUE(p.base()->initialize("{}"));
    auto stats = p.ethics()->getStatistics();
    EXPECT_DOUBLE_EQ(0.0, stats.at("total_debates"));
    EXPECT_DOUBLE_EQ(0.0, stats.at("total_decisions"));
}

// ============================================================================
// Configuration: setConfig / getConfig
// ============================================================================

TEST_F(EthicsAIPluginTest, SetAndGetConfig) {
    ASSERT_TRUE(p.base()->initialize("{}"));
    ASSERT_TRUE(p.ethics()->setConfig("my_key", "my_value").isOK());
    auto val = p.ethics()->getConfig("my_key");
    ASSERT_TRUE(val.has_value());
    EXPECT_EQ("my_value", *val);
}

TEST_F(EthicsAIPluginTest, GetMissingConfigReturnsNullopt) {
    ASSERT_TRUE(p.base()->initialize("{}"));
    auto val = p.ethics()->getConfig("nonexistent_key");
    EXPECT_FALSE(val.has_value());
}

TEST_F(EthicsAIPluginTest, SetConfigOverwritesPreviousValue) {
    ASSERT_TRUE(p.base()->initialize("{}"));
    p.ethics()->setConfig("k", "v1");
    p.ethics()->setConfig("k", "v2");
    auto val = p.ethics()->getConfig("k");
    ASSERT_TRUE(val.has_value());
    EXPECT_EQ("v2", *val);
}

// ============================================================================
// Philosophy school listing (no profiles loaded)
// ============================================================================

TEST_F(EthicsAIPluginTest, ListPhilosophySchoolsInitiallyEmpty) {
    ASSERT_TRUE(p.base()->initialize("{}"));
    auto schools = p.ethics()->listPhilosophySchools();
    EXPECT_TRUE(schools.empty());
}

TEST_F(EthicsAIPluginTest, GetPhilosophyProfileForUnknownSchoolReturnsError) {
    ASSERT_TRUE(p.base()->initialize("{}"));
    auto result = p.ethics()->getPhilosophyProfile("nonexistent");
    ASSERT_TRUE(std::holds_alternative<Status>(result));
    EXPECT_FALSE(std::get<Status>(result).isOK());
}

// ============================================================================
// getInstance
// ============================================================================

TEST_F(EthicsAIPluginTest, GetInstanceReturnsSelf) {
    ASSERT_TRUE(p.base()->initialize("{}"));
    void* inst = p.base()->getInstance();
    EXPECT_NE(nullptr, inst);
}
