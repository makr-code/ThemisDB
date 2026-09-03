#include <gtest/gtest.h>

#include <cstdlib>
#include <memory>
#include <string>
#include <vector>

#ifdef THEMIS_ENABLE_GGML_BRIDGE
#include "storage/ggml_tensor_bridge.h"
#include "storage/tensor_network_storage_engine.h"
#endif

namespace themis::storage {

#ifdef THEMIS_ENABLE_GGML_BRIDGE

namespace {

class ProductionModeGuard final : public ::testing::Test {
protected:
    void SetUp() override {
        const char* mode_env = std::getenv("THEMIS_PRODUCTION_MODE");
        const char* env_env = std::getenv("THEMIS_ENVIRONMENT");
        original_mode_env_ = mode_env ? std::string(mode_env) : "";
        original_env_env_ = env_env ? std::string(env_env) : "";

#ifdef _WIN32
        _putenv("THEMIS_PRODUCTION_MODE=");
        _putenv("THEMIS_ENVIRONMENT=");
#else
        unsetenv("THEMIS_PRODUCTION_MODE");
        unsetenv("THEMIS_ENVIRONMENT");
#endif
    }

    void TearDown() override {
#ifdef _WIN32
        if (!original_mode_env_.empty()) {
            _putenv_s("THEMIS_PRODUCTION_MODE", original_mode_env_.c_str());
        } else {
            _putenv("THEMIS_PRODUCTION_MODE=");
        }
        if (!original_env_env_.empty()) {
            _putenv_s("THEMIS_ENVIRONMENT", original_env_env_.c_str());
        } else {
            _putenv("THEMIS_ENVIRONMENT=");
        }
#else
        if (!original_mode_env_.empty()) {
            setenv("THEMIS_PRODUCTION_MODE", original_mode_env_.c_str(), 1);
        } else {
            unsetenv("THEMIS_PRODUCTION_MODE");
        }
        if (!original_env_env_.empty()) {
            setenv("THEMIS_ENVIRONMENT", original_env_env_.c_str(), 1);
        } else {
            unsetenv("THEMIS_ENVIRONMENT");
        }
#endif
    }

    static void enableProductionMode() {
#ifdef _WIN32
        _putenv_s("THEMIS_PRODUCTION_MODE", "1");
#else
        setenv("THEMIS_PRODUCTION_MODE", "1", 1);
#endif
    }

private:
    std::string original_mode_env_;
    std::string original_env_env_;
};

std::shared_ptr<TensorNetworkStorageEngine> makeTensorStorage(const TensorFieldKey& key) {
    auto backend = std::make_shared<InMemoryTensorBackend>();
    TensorStorageConfig cfg;
    cfg.quant_type = QuantizationType::NONE;
    cfg.tt_config.eps = 0.01;
    auto engine = std::make_shared<TensorNetworkStorageEngine>(backend, cfg);
    const std::vector<float> values{1.0f, 2.0f, 3.0f, 4.0f};
    const std::vector<std::size_t> shape{values.size()};
    EXPECT_TRUE(engine->put(key, values, shape));
    return engine;
}

} // namespace

TEST_F(ProductionModeGuard, MapFailsClosedWhenAllocatorPathUnavailable) {
    enableProductionMode();
    GgmlTensorBridge::clearGgmlAllocFn();

    TensorFieldKey key{"tenant", "collection", "field"};
    auto storage = makeTensorStorage(key);
    GgmlTensorBridge bridge(storage);

    auto handle = bridge.map(nullptr, key, 0);
    EXPECT_FALSE(handle.valid());
    EXPECT_EQ(handle.ggmlTensor(), nullptr);
}

#if !defined(THEMIS_HAS_IO_URING)
TEST_F(ProductionModeGuard, PrefetchFailsClosedWithoutProductionBackend) {
    enableProductionMode();
    GgmlTensorBridge::clearPrefetchFn();

    TensorFieldKey key{"tenant", "collection", "field"};
    auto storage = makeTensorStorage(key);
    GgmlTensorBridge bridge(storage);

    EXPECT_THROW(bridge.prefetch(key, 0), std::runtime_error);
}
#endif

#if !defined(THEMIS_HAS_GGML)
TEST_F(ProductionModeGuard, TypeRegistrationFailsClosedWithoutProductionBackend) {
    enableProductionMode();
    GgmlTensorBridge::clearTypeRegistrationFn();
    EXPECT_THROW((void)registerGgmlTypeTT(), std::runtime_error);
}
#endif

#endif // THEMIS_ENABLE_GGML_BRIDGE

} // namespace themis::storage
