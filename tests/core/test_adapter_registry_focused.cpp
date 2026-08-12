#include <gtest/gtest.h>
#include "core/concerns/adapter_registry.h"
#include "core/concerns/adapter_metadata.h"

#include <thread>
#include <vector>
#include <atomic>
#include <typeindex>

using namespace themis::core::concerns;

// ---------------------------------------------------------------------------
// Helper types
// ---------------------------------------------------------------------------

struct IFakeAlpha {
    virtual ~IFakeAlpha() = default;
    virtual int value() const = 0;
};

struct IFakeBeta {
    virtual ~IFakeBeta() = default;
    virtual std::string label() const = 0;
};

struct FakeAlphaImpl : IFakeAlpha {
    explicit FakeAlphaImpl(int v) : v_(v) {}
    int value() const override { return v_; }
    int v_;
};

struct FakeBetaImpl : IFakeBeta {
    explicit FakeBetaImpl(std::string l) : l_(std::move(l)) {}
    std::string label() const override { return l_; }
    std::string l_;
};

// ---------------------------------------------------------------------------
// AR_01 — empty registry: resolve returns null shared_ptr
// ---------------------------------------------------------------------------
TEST(AdapterRegistryTest, AR_01_EmptyRegistryResolveReturnsNull) {
    AdapterRegistry reg;
    auto result = reg.resolve<IFakeAlpha>();
    EXPECT_EQ(result, nullptr);
}

// ---------------------------------------------------------------------------
// AR_02 — registerAdapter + resolve: returns the registered adapter
// ---------------------------------------------------------------------------
TEST(AdapterRegistryTest, AR_02_RegisterAndResolve) {
    AdapterRegistry reg;
    auto adapter = std::make_shared<FakeAlphaImpl>(42);
    reg.registerAdapter<IFakeAlpha>("alpha", adapter);

    auto resolved = reg.resolve<IFakeAlpha>();
    ASSERT_NE(resolved, nullptr);
    EXPECT_EQ(resolved->value(), 42);
}

// ---------------------------------------------------------------------------
// AR_03 — resolving a different type returns null
// ---------------------------------------------------------------------------
TEST(AdapterRegistryTest, AR_03_WrongTypeResolveReturnsNull) {
    AdapterRegistry reg;
    auto adapter = std::make_shared<FakeAlphaImpl>(7);
    reg.registerAdapter<IFakeAlpha>("alpha", adapter);

    // Resolve IFakeBeta — not registered
    auto result = reg.resolve<IFakeBeta>();
    EXPECT_EQ(result, nullptr);
}

// ---------------------------------------------------------------------------
// AR_04 — apiVersion below kCurrentApiVersion throws std::invalid_argument
// ---------------------------------------------------------------------------
TEST(AdapterRegistryTest, AR_04_ApiVersionBelowCurrentThrows) {
    static_assert(kCurrentApiVersion > 0, "kCurrentApiVersion must remain positive");
    AdapterRegistry reg;
    auto adapter = std::make_shared<FakeAlphaImpl>(1);

    AdapterMetadata meta;
    meta.id         = "alpha";
    meta.apiVersion = kCurrentApiVersion - 1; // explicitly invalid

    EXPECT_THROW(
        reg.registerAdapter<IFakeAlpha>("alpha", adapter, nullptr, meta),
        std::invalid_argument
    );
}

// ---------------------------------------------------------------------------
// AR_05 — empty id throws std::invalid_argument
// ---------------------------------------------------------------------------
TEST(AdapterRegistryTest, AR_05_EmptyIdThrows) {
    AdapterRegistry reg;
    auto adapter = std::make_shared<FakeAlphaImpl>(1);

    EXPECT_THROW(
        reg.registerAdapter<IFakeAlpha>("", adapter),
        std::invalid_argument
    );
}

// ---------------------------------------------------------------------------
// AR_06 — hotSwap installs new adapter; old adapter is observable as replaced
// ---------------------------------------------------------------------------
TEST(AdapterRegistryTest, AR_06_HotSwapReplacesAdapter) {
    AdapterRegistry reg;

    auto old_adapter = std::make_shared<FakeAlphaImpl>(10);
    reg.registerAdapter<IFakeAlpha>("alpha", old_adapter);

    auto new_adapter = std::make_shared<FakeAlphaImpl>(99);
    bool ok = reg.hotSwap<IFakeAlpha>(new_adapter);
    EXPECT_TRUE(ok);

    auto resolved = reg.resolve<IFakeAlpha>();
    ASSERT_NE(resolved, nullptr);
    EXPECT_EQ(resolved->value(), 99);
}

// ---------------------------------------------------------------------------
// AR_07 — validator returning false causes registerAdapter to throw
// ---------------------------------------------------------------------------

class RejectAllValidator : public AdapterValidator {
public:
    bool validate(const AdapterMetadata& /*m*/) override { return false; }
};

TEST(AdapterRegistryTest, AR_07_ValidatorRejectionThrows) {
    AdapterRegistry reg;
    auto adapter   = std::make_shared<FakeAlphaImpl>(1);
    RejectAllValidator validator;

    EXPECT_THROW(
        reg.registerAdapter<IFakeAlpha>("alpha", adapter, &validator),
        std::invalid_argument
    );
}

// ---------------------------------------------------------------------------
// AR_08 — loadFromPlugin: non-existent path returns false
// ---------------------------------------------------------------------------
TEST(AdapterRegistryTest, AR_08_LoadFromPluginNonExistentPathReturnsFalse) {
    AdapterRegistry reg;
    // A path that is guaranteed not to exist
    bool result = reg.loadFromPlugin("/tmp/__themis_no_such_plugin_xyz__.so", "alpha");
    EXPECT_FALSE(result);
}

// ---------------------------------------------------------------------------
// AR_09 — count() and hasAdapter() correct after register
// ---------------------------------------------------------------------------
TEST(AdapterRegistryTest, AR_09_CountAndHasAdapterCorrect) {
    AdapterRegistry reg;

    EXPECT_EQ(reg.count(), 0u);
    EXPECT_FALSE(reg.hasAdapter(std::type_index(typeid(IFakeAlpha))));

    reg.registerAdapter<IFakeAlpha>("a1", std::make_shared<FakeAlphaImpl>(1));
    EXPECT_EQ(reg.count(), 1u);
    EXPECT_TRUE(reg.hasAdapter(std::type_index(typeid(IFakeAlpha))));

    reg.registerAdapter<IFakeBeta>("b1", std::make_shared<FakeBetaImpl>("x"));
    EXPECT_EQ(reg.count(), 2u);
    EXPECT_TRUE(reg.hasAdapter(std::type_index(typeid(IFakeBeta))));

    // Second register of same type overwrites, count stays at 2
    reg.registerAdapter<IFakeAlpha>("a2", std::make_shared<FakeAlphaImpl>(2));
    EXPECT_EQ(reg.count(), 2u);
}

// ---------------------------------------------------------------------------
// AR_10 — concurrent resolve (16 threads × 500 calls) does not crash
// ---------------------------------------------------------------------------
TEST(AdapterRegistryTest, AR_10_ConcurrentResolveNoCrash) {
    AdapterRegistry reg;
    reg.registerAdapter<IFakeAlpha>("alpha", std::make_shared<FakeAlphaImpl>(7));

    constexpr int kThreads = 16;
    constexpr int kCalls   = 500;

    std::vector<std::thread> threads;
    std::atomic<int> total_sum{0};
    threads.reserve(kThreads);

    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&] {
            int local = 0;
            for (int i = 0; i < kCalls; ++i) {
                auto p = reg.resolve<IFakeAlpha>();
                if (p) local += p->value();
            }
            total_sum.fetch_add(local, std::memory_order_relaxed);
        });
    }

    for (auto& th : threads) th.join();

    // 16 threads × 500 calls × value 7
    EXPECT_EQ(total_sum.load(), kThreads * kCalls * 7);
}

// ---------------------------------------------------------------------------
// AR_11 — nullptr adapter is rejected
// ---------------------------------------------------------------------------
TEST(AdapterRegistryTest, AR_11_NullptrAdapterThrows) {
    AdapterRegistry reg;
    std::shared_ptr<IFakeAlpha> adapter;

    EXPECT_THROW(
        reg.registerAdapter<IFakeAlpha>("alpha", adapter),
        std::invalid_argument
    );
}
