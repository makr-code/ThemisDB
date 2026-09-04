// Tests for the type-safe generic PluginRegistry (factory registration,
// creation, listing, type-mismatch rejection, and clearRegistry).

#include <gtest/gtest.h>
#include "plugins/plugin_registry.h"
#include "plugins/plugin_interface.h"

#include <memory>
#include <string>
#include <vector>
#include <thread>
#include <atomic>

using namespace themis::plugins;

// ============================================================================
// Minimal stub interfaces and implementations for testing
// ============================================================================

struct ITestInterface {
    virtual ~ITestInterface() = default;
    virtual std::string id() const = 0;
};

struct ITestInterfaceB {
    virtual ~ITestInterfaceB() = default;
    virtual int value() const = 0;
};

struct ConcreteA : public ITestInterface {
    std::string id() const override { return "ConcreteA"; }
};

struct ConcreteA2 : public ITestInterface {
    std::string id() const override { return "ConcreteA2"; }
};

struct ConcreteB : public ITestInterfaceB {
    int value() const override { return 42; }
};

// ============================================================================
// Fixture: reset the global registry before/after each test
// ============================================================================

class GenericPluginRegistryTest : public ::testing::Test {
protected:
    void SetUp() override    { PluginRegistry::clearRegistry(); }
    void TearDown() override { PluginRegistry::clearRegistry(); }
};

// ============================================================================
// Registration and creation
// ============================================================================

TEST_F(GenericPluginRegistryTest, RegisterAndCreatePlugin) {
    PluginRegistry::registerFactory<ITestInterface>(
        "a_plugin", []() { return std::make_unique<ConcreteA>(); });

    auto inst = PluginRegistry::create<ITestInterface>("a_plugin");
    ASSERT_NE(nullptr, inst);
    EXPECT_EQ("ConcreteA", inst->id());
}

TEST_F(GenericPluginRegistryTest, CreateUnregisteredPluginThrows) {
    EXPECT_THROW(
        PluginRegistry::create<ITestInterface>("missing"),
        std::runtime_error);
}

TEST_F(GenericPluginRegistryTest, LastRegistrationWinsOnNameCollision) {
    PluginRegistry::registerFactory<ITestInterface>(
        "plug", []() { return std::make_unique<ConcreteA>(); });
    PluginRegistry::registerFactory<ITestInterface>(
        "plug", []() { return std::make_unique<ConcreteA2>(); });

    auto inst = PluginRegistry::create<ITestInterface>("plug");
    ASSERT_NE(nullptr, inst);
    EXPECT_EQ("ConcreteA2", inst->id());
}

// ============================================================================
// Type isolation: registries per interface type are independent
// ============================================================================

TEST_F(GenericPluginRegistryTest, SeparateRegistriesPerInterfaceType) {
    PluginRegistry::registerFactory<ITestInterface>(
        "plug", []() { return std::make_unique<ConcreteA>(); });
    PluginRegistry::registerFactory<ITestInterfaceB>(
        "plug", []() { return std::make_unique<ConcreteB>(); });

    auto a = PluginRegistry::create<ITestInterface>("plug");
    auto b = PluginRegistry::create<ITestInterfaceB>("plug");

    ASSERT_NE(nullptr, a);
    ASSERT_NE(nullptr, b);
    EXPECT_EQ("ConcreteA", a->id());
    EXPECT_EQ(42, b->value());
}

// ============================================================================
// hasPlugin
// ============================================================================

TEST_F(GenericPluginRegistryTest, HasPluginReturnsTrueAfterRegistration) {
    PluginRegistry::registerFactory<ITestInterface>(
        "a_plugin", []() { return std::make_unique<ConcreteA>(); });
    EXPECT_TRUE(PluginRegistry::hasPlugin<ITestInterface>("a_plugin"));
}

TEST_F(GenericPluginRegistryTest, HasPluginReturnsFalseForUnregistered) {
    EXPECT_FALSE(PluginRegistry::hasPlugin<ITestInterface>("nonexistent"));
}

// ============================================================================
// listPlugins
// ============================================================================

TEST_F(GenericPluginRegistryTest, ListPluginsReturnsRegisteredNames) {
    PluginRegistry::registerFactory<ITestInterface>(
        "plugin_x", []() { return std::make_unique<ConcreteA>(); });
    PluginRegistry::registerFactory<ITestInterface>(
        "plugin_y", []() { return std::make_unique<ConcreteA2>(); });

    auto names = PluginRegistry::listPlugins<ITestInterface>();
    ASSERT_EQ(2u, names.size());

    bool foundX = false, foundY = false;
    for (const auto& n : names) {
        if (n == "plugin_x") {
          foundX = true;
        }
        if (n == "plugin_y") {
          foundY = true;
        }
    }
    EXPECT_TRUE(foundX);
    EXPECT_TRUE(foundY);
}

TEST_F(GenericPluginRegistryTest, ListPluginsReturnsEmptyWhenNoneRegistered) {
    EXPECT_TRUE(PluginRegistry::listPlugins<ITestInterface>().empty());
}

// ============================================================================
// clearRegistry
// ============================================================================

TEST_F(GenericPluginRegistryTest, ClearRegistryRemovesAllEntries) {
    PluginRegistry::registerFactory<ITestInterface>(
        "a_plugin", []() { return std::make_unique<ConcreteA>(); });
    PluginRegistry::clearRegistry();
    EXPECT_FALSE(PluginRegistry::hasPlugin<ITestInterface>("a_plugin"));
    EXPECT_TRUE(PluginRegistry::listPlugins<ITestInterface>().empty());
}

// ============================================================================
// unregisterFactory
// ============================================================================

TEST_F(GenericPluginRegistryTest, UnregisterFactoryReturnsTrueWhenFound) {
    PluginRegistry::registerFactory<ITestInterface>(
        "u_plugin", []() { return std::make_unique<ConcreteA>(); });
    EXPECT_TRUE(PluginRegistry::unregisterFactory<ITestInterface>("u_plugin"));
    EXPECT_FALSE(PluginRegistry::hasPlugin<ITestInterface>("u_plugin"));
}

TEST_F(GenericPluginRegistryTest, UnregisterFactoryReturnsFalseWhenNotFound) {
    EXPECT_FALSE(PluginRegistry::unregisterFactory<ITestInterface>("ghost"));
}

TEST_F(GenericPluginRegistryTest, UnregisterFactoryDoesNotAffectOtherPlugins) {
    PluginRegistry::registerFactory<ITestInterface>(
        "keep", []() { return std::make_unique<ConcreteA>(); });
    PluginRegistry::registerFactory<ITestInterface>(
        "remove", []() { return std::make_unique<ConcreteA2>(); });
    EXPECT_TRUE(PluginRegistry::unregisterFactory<ITestInterface>("remove"));
    EXPECT_TRUE(PluginRegistry::hasPlugin<ITestInterface>("keep"));
    EXPECT_FALSE(PluginRegistry::hasPlugin<ITestInterface>("remove"));
}

TEST_F(GenericPluginRegistryTest, UnregisterFactoryDoesNotAffectOtherInterfaces) {
    PluginRegistry::registerFactory<ITestInterface>(
        "shared_name", []() { return std::make_unique<ConcreteA>(); });
    PluginRegistry::registerFactory<ITestInterfaceB>(
        "shared_name", []() { return std::make_unique<ConcreteB>(); });
    EXPECT_TRUE(PluginRegistry::unregisterFactory<ITestInterface>("shared_name"));
    EXPECT_FALSE(PluginRegistry::hasPlugin<ITestInterface>("shared_name"));
    EXPECT_TRUE(PluginRegistry::hasPlugin<ITestInterfaceB>("shared_name"));
}

// ============================================================================
// Concurrent reads: shared_mutex allows multiple simultaneous readers
// ============================================================================

TEST_F(GenericPluginRegistryTest, ConcurrentReadsDoNotDeadlock) {
    PluginRegistry::registerFactory<ITestInterface>(
        "r_plugin", []() { return std::make_unique<ConcreteA>(); });

    constexpr int kReaders = 8;
    std::atomic<int> ready{0};
    std::atomic<int> success{0};

    auto reader = [&]() {
        ready.fetch_add(1, std::memory_order_relaxed);
        // Spin until all readers are ready to maximize concurrency
        while (ready.load(std::memory_order_acquire) < kReaders) {}

        bool found = PluginRegistry::hasPlugin<ITestInterface>("r_plugin");
        auto names = PluginRegistry::listPlugins<ITestInterface>();
        if (found && !names.empty()) {
            success.fetch_add(1, std::memory_order_relaxed);
        }
    };

    std::vector<std::thread> threads;
    threads.reserve(kReaders);
    for (int i = 0; i < kReaders; ++i) {
        threads.emplace_back(reader);
    }
    for (auto& t : threads) {
        t.join();
    }

    EXPECT_EQ(kReaders, success.load());
}
