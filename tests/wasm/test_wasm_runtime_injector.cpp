// RESTORED FROM HISTORY: 892fbc132819cf3446b54bb51b8b14ec2dd61db5


#include <gtest/gtest.h>
#include "themis/base/wasm_runtime_injector.h"
#include "plugins/wasm_host_api.h"

namespace themis {
namespace modules {
namespace {

// ─────────────────────────────────────────────────────────────────────────────
// Minimal test runtime
// ─────────────────────────────────────────────────────────────────────────────

class MockWasmRuntime : public IWasmRuntime {
public:
    explicit MockWasmRuntime(std::string name) : name_(std::move(name)) {}

    bool instantiate(const std::vector<uint8_t>&,
                      const std::vector<WasmHostFunction>&,
                      size_t) override {
        instantiated_ = true;
        return true;
    }

    bool call(const std::string& fn_name,
              const std::vector<uint8_t>& args,
              std::vector<uint8_t>& out) override {
        out = args;  // echo
        last_fn_ = fn_name;
        return true;
    }

    uint8_t* linearMemory(size_t& out_size) override {
        out_size = mem_.size();
        return mem_.data();
    }

    std::string name()    const override { return name_; }
    std::string version() const override { return "1.0-mock"; }
    bool isInstantiated() const override { return instantiated_; }

    bool        instantiated_ = false;
    std::string last_fn_;

private:
    std::string          name_;
    std::vector<uint8_t> mem_ = std::vector<uint8_t>(64, 0);
};

// ─────────────────────────────────────────────────────────────────────────────
// Fixture (clears registry between tests)
// ─────────────────────────────────────────────────────────────────────────────

struct InjectorFixture : ::testing::Test {
    void SetUp() override { WasmRuntimeInjector::clearAll(); }
    void TearDown() override { WasmRuntimeInjector::clearAll(); }
};

// ─────────────────────────────────────────────────────────────────────────────
// Tests
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(InjectorFixture, NoRegisteredRuntimes_NotAvailable) {
    EXPECT_FALSE(WasmRuntimeInjector::available());
}

TEST_F(InjectorFixture, RegisteredRuntime_IsAvailable) {
    WasmRuntimeInjector::registerRuntime({
        "test-rt", 10, "test",
        []{ return std::make_unique<MockWasmRuntime>("test-rt"); }
    });
    EXPECT_TRUE(WasmRuntimeInjector::available());
}

TEST_F(InjectorFixture, CreateByName_ReturnsCorrectRuntime) {
    WasmRuntimeInjector::registerRuntime({
        "alpha", 10, "Alpha RT",
        []{ return std::make_unique<MockWasmRuntime>("alpha"); }
    });
    auto rt = WasmRuntimeInjector::create("alpha");
    ASSERT_NE(rt, nullptr);
    EXPECT_EQ(rt->name(), "alpha");
}

TEST_F(InjectorFixture, CreateUnknownName_ReturnsNullptr) {
    auto rt = WasmRuntimeInjector::create("nonexistent");
    EXPECT_EQ(rt, nullptr);
}

TEST_F(InjectorFixture, AutoSelectPicksHighestPriority) {
    WasmRuntimeInjector::registerRuntime({
        "low-prio", 5, "Low",
        []{ return std::make_unique<MockWasmRuntime>("low-prio"); }
    });
    WasmRuntimeInjector::registerRuntime({
        "high-prio", 100, "High",
        []{ return std::make_unique<MockWasmRuntime>("high-prio"); }
    });

    auto rt = WasmRuntimeInjector::create();
    ASSERT_NE(rt, nullptr);
    EXPECT_EQ(rt->name(), "high-prio");
}

TEST_F(InjectorFixture, RegisteredNames_SortedByPriority) {
    WasmRuntimeInjector::registerRuntime({"c-rt", 1,  "C", []{ return nullptr; }});
    WasmRuntimeInjector::registerRuntime({"a-rt", 50, "A", []{ return nullptr; }});
    WasmRuntimeInjector::registerRuntime({"b-rt", 20, "B", []{ return nullptr; }});

    auto names = WasmRuntimeInjector::registeredNames();
    ASSERT_EQ(names.size(), 3u);
    EXPECT_EQ(names[0], "a-rt");   // highest priority first
    EXPECT_EQ(names[1], "b-rt");
    EXPECT_EQ(names[2], "c-rt");
}

TEST_F(InjectorFixture, DuplicateNameReplacesExisting) {
    WasmRuntimeInjector::registerRuntime({
        "rt", 10, "v1",
        []{ return std::make_unique<MockWasmRuntime>("rt-v1"); }
    });
    WasmRuntimeInjector::registerRuntime({
        "rt", 20, "v2",
        []{ return std::make_unique<MockWasmRuntime>("rt-v2"); }
    });

    auto names = WasmRuntimeInjector::registeredNames();
    EXPECT_EQ(names.size(), 1u);
    auto rt = WasmRuntimeInjector::create("rt");
    ASSERT_NE(rt, nullptr);
    EXPECT_EQ(rt->name(), "rt-v2");  // v2 replaced v1
}

TEST_F(InjectorFixture, RuntimeCallAfterInstantiate) {
    WasmRuntimeInjector::registerRuntime({
        "echo", 10, "Echo RT",
        []{ return std::make_unique<MockWasmRuntime>("echo"); }
    });
    auto rt = WasmRuntimeInjector::create("echo");
    ASSERT_NE(rt, nullptr);

    rt->instantiate({}, {}, 0);
    EXPECT_TRUE(rt->isInstantiated());

    std::vector<uint8_t> args = {1, 2, 3};
    std::vector<uint8_t> out;
    rt->call("myfn", args, out);
    EXPECT_EQ(out, args);
    EXPECT_EQ(dynamic_cast<MockWasmRuntime*>(rt.get())->last_fn_, "myfn");
}

TEST_F(InjectorFixture, ClearAllRemovesAllRuntimes) {
    WasmRuntimeInjector::registerRuntime({"x", 1, "X", []{ return nullptr; }});
    WasmRuntimeInjector::clearAll();
    EXPECT_FALSE(WasmRuntimeInjector::available());
    EXPECT_TRUE(WasmRuntimeInjector::registeredNames().empty());
}

TEST_F(InjectorFixture, WasmPluginLoaderBridgeIsUsed) {
    using themis::plugins::IThemisPlugin;
    using themis::plugins::IStatefulPlugin;
    using themis::plugins::WasmPluginRuntime;

    class DummyPlugin final : public IThemisPlugin, public IStatefulPlugin {
    public:
        const char* getName() const override { return "dummy"; }
        const char* getVersion() const override { return "1.0"; }
        themis::plugins::PluginType getType() const override { return themis::plugins::PluginType::CUSTOM; }
        themis::plugins::PluginCapabilities getCapabilities() const override { return {}; }
        bool initialize(const char*) override { return true; }
        void shutdown() override {}
        void* getInstance() override { return nullptr; }
        std::string saveState() override { return {}; }
        bool restoreState(const std::string&) override { return true; }
    };

    themis::plugins::setWasmPluginLoadFn(
        []([[maybe_unused]] const std::string& wasm_path,
           [[maybe_unused]] const std::string& expected_sha256,
           [[maybe_unused]] WasmPluginRuntime runtime,
           const std::string& module_name,
           [[maybe_unused]] std::string& error_out) {
            if (module_name == "bridge-module") {
                return std::unique_ptr<IThemisPlugin>(std::make_unique<DummyPlugin>());
            }
            return std::unique_ptr<IThemisPlugin>{};
        });

    std::string error;
    auto plugin = themis::plugins::loadWasmPlugin(
        "ignored.wasm", "", WasmPluginRuntime::NONE, "bridge-module", error);
    ASSERT_NE(plugin, nullptr);
    EXPECT_STREQ(plugin->getName(), "dummy");

    themis::plugins::setWasmPluginLoadFn({});
}

} // anonymous namespace
} // namespace modules
} // namespace themis
