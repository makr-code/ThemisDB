#include <gtest/gtest.h>

#include "storage/tensor_network_storage_engine.h"
#include "tensor/persistent_tensor_fingerprint_graph.h"

#include <algorithm>
#include <unordered_map>

namespace themis { namespace tensor { namespace test { 
namespace {

class MapBackend final : public storage::ITensorStorageBackend {
public:
    bool put(const std::string& key, const std::vector<uint8_t>& value) override {
        store_[key] = value;
        return true;
    }

    std::optional<std::vector<uint8_t>> get(const std::string& key) const override {
        auto it = store_.find(key);
        if (it == store_.end()) {
            return std::nullopt;
        }
        return it->second;
    }

    bool del(const std::string& key) override {
        return store_.erase(key) > 0;
    }

    std::vector<std::string> listKeys(const std::string& prefix) const override {
        std::vector<std::string> keys;
        for (const auto& kv : store_) {
            if (kv.first.rfind(prefix, 0) == 0) {
                keys.push_back(kv.first);
            }
        }
        std::sort(keys.begin(), keys.end());
        return keys;
    }

private:
    mutable std::unordered_map<std::string, std::vector<uint8_t>> store_;
};

class JournalDeleteFailBackend final : public storage::ITensorStorageBackend {
public:
    void armJournalDeleteFailure() {
        fail_next_journal_delete_ = true;
    }

    bool put(const std::string& key, const std::vector<uint8_t>& value) override {
        store_[key] = value;
        return true;
    }

    std::optional<std::vector<uint8_t>> get(const std::string& key) const override {
        auto it = store_.find(key);
        if (it == store_.end()) {
            return std::nullopt;
        }
        return it->second;
    }

    bool del(const std::string& key) override {
        if (fail_next_journal_delete_ && key.find(":txn:") != std::string::npos) {
            fail_next_journal_delete_ = false;
            return false;
        }
        return store_.erase(key) > 0;
    }

    std::vector<std::string> listKeys(const std::string& prefix) const override {
        std::vector<std::string> keys;
        for (const auto& kv : store_) {
            if (kv.first.rfind(prefix, 0) == 0) {
                keys.push_back(kv.first);
            }
        }
        std::sort(keys.begin(), keys.end());
        return keys;
    }

private:
    mutable std::unordered_map<std::string, std::vector<uint8_t>> store_;
    bool fail_next_journal_delete_ = false;
};

storage::TTTrain makeTrain(float base) {
    storage::TTTrain train;
    storage::TTCore core;
    core.r_left = 1;
    core.n = 2;
    core.r_right = 2;
    core.data = {base, base + 1.0f, base + 2.0f, base + 3.0f};
    train.cores.push_back(core);
    train.mode_sizes = {2};
    return train;
}

}  // namespace

TEST(PersistentTensorFingerprintGraphTest, SaveLoadRemoveAndTenantIsolation) {
    auto backend = std::make_shared<MapBackend>();

    auto graph_a = std::make_shared<TensorFingerprintGraph>();
    PersistentTensorFingerprintGraph persistent_a(graph_a, backend, "tenant_a", "domain_a");
    ASSERT_TRUE(persistent_a.addAdapter("adapter_a", makeTrain(1.0f), "model_a"));
    ASSERT_EQ(graph_a->size(), 1u);

    auto graph_b = std::make_shared<TensorFingerprintGraph>();
    PersistentTensorFingerprintGraph persistent_b(graph_b, backend, "tenant_b", "domain_a");
    ASSERT_TRUE(persistent_b.addAdapter("adapter_b", makeTrain(2.0f), "model_b"));
    ASSERT_EQ(graph_b->size(), 1u);

    auto rehydrated_a = std::make_shared<TensorFingerprintGraph>();
    PersistentTensorFingerprintGraph restart_a(rehydrated_a, backend, "tenant_a", "domain_a");
    ASSERT_TRUE(restart_a.rehydrate());
    EXPECT_EQ(rehydrated_a->size(), 1u);
    EXPECT_TRUE(rehydrated_a->entry("adapter_a").has_value());
    EXPECT_FALSE(rehydrated_a->entry("adapter_b").has_value());

    ASSERT_TRUE(restart_a.removeAdapter("adapter_a"));
    EXPECT_EQ(rehydrated_a->size(), 0u);

    auto rehydrated_again = std::make_shared<TensorFingerprintGraph>();
    PersistentTensorFingerprintGraph restart_again(rehydrated_again, backend, "tenant_a", "domain_a");
    ASSERT_TRUE(restart_again.rehydrate());
    EXPECT_EQ(rehydrated_again->size(), 0u);
}

TEST(PersistentTensorFingerprintGraphTest, RehydratesAcrossRestartSimulation) {
    auto backend = std::make_shared<MapBackend>();

    {
        auto graph = std::make_shared<TensorFingerprintGraph>();
        PersistentTensorFingerprintGraph persistent(graph, backend, "tenant", "domain");
        ASSERT_TRUE(persistent.addAdapter("adapter_1", makeTrain(3.0f), "model_1"));
        ASSERT_TRUE(persistent.addAdapter("adapter_2", makeTrain(4.0f), "model_2"));
        ASSERT_EQ(graph->size(), 2u);
    }

    auto graph_after_restart = std::make_shared<TensorFingerprintGraph>();
    PersistentTensorFingerprintGraph restored(graph_after_restart, backend, "tenant", "domain");
    ASSERT_TRUE(restored.rehydrate());

    EXPECT_EQ(graph_after_restart->size(), 2u);
    EXPECT_TRUE(graph_after_restart->entry("adapter_1").has_value());
    EXPECT_TRUE(graph_after_restart->entry("adapter_2").has_value());
}

TEST(PersistentTensorFingerprintGraphTest, RecoversDeleteJournalReplayWhenTargetAlreadyMissing) {
    auto backend = std::make_shared<JournalDeleteFailBackend>();
    auto graph = std::make_shared<TensorFingerprintGraph>();
    PersistentTensorFingerprintGraph persistent(graph, backend, "tenant", "domain");
    ASSERT_TRUE(persistent.addAdapter("adapter", makeTrain(1.0f), "model"));

    backend->armJournalDeleteFailure();
    EXPECT_FALSE(persistent.removeAdapter("adapter"));

    auto graph_after_restart = std::make_shared<TensorFingerprintGraph>();
    PersistentTensorFingerprintGraph restored(graph_after_restart, backend, "tenant", "domain");
    ASSERT_TRUE(restored.rehydrate());
    EXPECT_FALSE(graph_after_restart->entry("adapter").has_value());
}
} } } // namespace themis::tensor::test
