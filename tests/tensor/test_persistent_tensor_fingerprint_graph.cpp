#include <gtest/gtest.h>

#include "storage/tensor_network_storage_engine.h"
#include "tensor/persistent_tensor_fingerprint_graph.h"

#include <algorithm>
#include <limits>
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
        std::vector<std::string> keys = {};

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
        std::vector<std::string> keys = {};

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
    core.r_right = 1;
    core.data = {base, base + 1.0f};
    train.cores.push_back(core);
    train.mode_sizes = {2};
    return train;
}

std::vector<uint8_t> deleteJournal(const std::string& target) {
    std::vector<uint8_t> bytes{1, 0, 0, 0, 2};
    const auto size = static_cast<uint32_t>(target.size());
    for (unsigned shift = 0; shift < 32; shift += 8) {
        bytes.push_back(static_cast<uint8_t>((size >> shift) & 0xff));
    }
    bytes.insert(bytes.end(), target.begin(), target.end());
    bytes.insert(bytes.end(), {0, 0, 0, 0});
    return bytes;
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

TEST(PersistentTensorFingerprintGraphTest, RecoversPendingMutationBeforeNextWrite) {
    auto backend = std::make_shared<JournalDeleteFailBackend>();
    auto graph = std::make_shared<TensorFingerprintGraph>();
    PersistentTensorFingerprintGraph persistent(graph, backend, "tenant", "domain");
    ASSERT_TRUE(persistent.addAdapter("adapter", makeTrain(1.0f), "model"));

    backend->armJournalDeleteFailure();
    EXPECT_TRUE(persistent.removeAdapter("adapter"));
    EXPECT_FALSE(graph->entry("adapter").has_value());
    EXPECT_TRUE(backend->get("__tfgp__:tenant:domain:txn:active"));
    ASSERT_TRUE(persistent.addAdapter("another", makeTrain(2.0f), "model"));
    EXPECT_FALSE(graph->entry("adapter").has_value());
    EXPECT_TRUE(graph->entry("another").has_value());

    auto graph_after_restart = std::make_shared<TensorFingerprintGraph>();
    PersistentTensorFingerprintGraph restored(graph_after_restart, backend, "tenant", "domain");
    ASSERT_TRUE(restored.rehydrate());
    EXPECT_FALSE(graph_after_restart->entry("adapter").has_value());
    EXPECT_TRUE(graph_after_restart->entry("another").has_value());
}

TEST(PersistentTensorFingerprintGraphTest, RejectsAmbiguousJournalsBeforeFurtherWrites) {
    auto backend = std::make_shared<MapBackend>();
    auto graph = std::make_shared<TensorFingerprintGraph>();
    PersistentTensorFingerprintGraph persistent(graph, backend, "tenant", "domain");
    ASSERT_TRUE(persistent.addAdapter("adapter", makeTrain(1.0f), "model"));

    const std::string target = "__tfgp__:tenant:domain:entry:adapter";
    ASSERT_TRUE(backend->put("__tfgp__:tenant:domain:txn:legacy-a",
                             deleteJournal(target)));
    ASSERT_TRUE(backend->put("__tfgp__:tenant:domain:txn:legacy-b",
                             deleteJournal(target)));

    EXPECT_FALSE(persistent.addAdapter("another", makeTrain(2.0f), "model"));
    EXPECT_TRUE(graph->entry("adapter"));
    EXPECT_FALSE(graph->entry("another"));
    EXPECT_TRUE(backend->get(target));
    EXPECT_FALSE(backend->get("__tfgp__:tenant:domain:entry:another"));
}

TEST(PersistentTensorFingerprintGraphTest, RehydratePreservesOtherTenantsAndRejectsCorruptEntries) {
    auto backend = std::make_shared<MapBackend>();
    auto graph = std::make_shared<TensorFingerprintGraph>();
    PersistentTensorFingerprintGraph a(graph, backend, "tenant-a", "domain");
    PersistentTensorFingerprintGraph b(graph, backend, "tenant-b", "domain");
    ASSERT_TRUE(a.addAdapter("a", makeTrain(1.0f), "model-a"));
    ASSERT_TRUE(b.addAdapter("b", makeTrain(2.0f), "model-b"));
    ASSERT_TRUE(a.rehydrate());
    EXPECT_TRUE(graph->entry("a"));
    EXPECT_TRUE(graph->entry("b"));

    ASSERT_TRUE(backend->put("__tfgp__:tenant-a:domain:entry:bad", {1, 0}));
    EXPECT_FALSE(a.rehydrate());
    EXPECT_TRUE(graph->entry("a"));
    EXPECT_TRUE(graph->entry("b"));
}

TEST(PersistentTensorFingerprintGraphTest, RehydrateRejectsMalformedTrainWithoutReplacingGraph) {
    auto backend = std::make_shared<MapBackend>();
    auto graph = std::make_shared<TensorFingerprintGraph>();
    PersistentTensorFingerprintGraph persistent(graph, backend, "tenant", "domain");
    const auto train = makeTrain(1.0f);
    ASSERT_TRUE(persistent.addAdapter("adapter", train, "model"));

    const std::string key = "__tfgp__:tenant:domain:entry:adapter";
    auto entry = backend->get(key);
    ASSERT_TRUE(entry);
    const auto train_size = train.serialize().size();
    ASSERT_GE(entry->size(), train_size);
    std::fill_n(entry->begin() + (entry->size() - train_size), 8, 0xff);
    ASSERT_TRUE(backend->put(key, *entry));

    EXPECT_FALSE(persistent.rehydrate());
    EXPECT_TRUE(graph->entry("adapter"));
}

TEST(PersistentTensorFingerprintGraphTest, RejectsInconsistentTrainShapes) {
    TensorFingerprintGraph graph;
    auto train = makeTrain(1.0f);
    ASSERT_TRUE(graph.addAdapter("existing", train, "domain", "model", "tenant"));

    auto bad_modes = train;
    bad_modes.mode_sizes[0] = 3;
    EXPECT_FALSE(graph.addAdapter("existing", bad_modes, "domain", "model", "tenant"));

    auto bad_ranks = train;
    bad_ranks.mode_sizes.push_back(2);
    auto second = train.cores.front();
    second.r_left = 1;
    bad_ranks.cores.push_back(second);
    EXPECT_FALSE(graph.addAdapter("existing", bad_ranks, "domain", "model", "tenant"));

    auto overflow = train;
    overflow.cores.front().n = std::numeric_limits<std::size_t>::max();
    overflow.mode_sizes.front() = overflow.cores.front().n;
    EXPECT_FALSE(graph.addAdapter("existing", overflow, "domain", "model", "tenant"));

    auto nan = train;
    nan.cores.front().data.front() = std::numeric_limits<float>::quiet_NaN();
    EXPECT_FALSE(graph.addAdapter("existing", nan, "domain", "model", "tenant"));
    ASSERT_TRUE(graph.entry("existing"));
    EXPECT_EQ(graph.entry("existing")->exact_train.cores.front().data,
              train.cores.front().data);
}

TEST(PersistentTensorFingerprintGraphTest, RehydrateRejectsDecodedShapeMismatch) {
    auto backend = std::make_shared<MapBackend>();
    auto graph = std::make_shared<TensorFingerprintGraph>();
    PersistentTensorFingerprintGraph persistent(graph, backend, "tenant", "domain");
    const auto train = makeTrain(1.0f);
    ASSERT_TRUE(persistent.addAdapter("adapter", train, "model"));

    const std::string key = "__tfgp__:tenant:domain:entry:adapter";
    auto entry = backend->get(key);
    ASSERT_TRUE(entry);
    const auto train_size = train.serialize().size();
    ASSERT_GE(entry->size(), train_size);
    (*entry)[entry->size() - train_size + 8] = 3;
    ASSERT_TRUE(backend->put(key, *entry));

    EXPECT_FALSE(persistent.rehydrate());
    ASSERT_TRUE(graph->entry("adapter"));
    EXPECT_EQ(graph->entry("adapter")->exact_train.mode_sizes, train.mode_sizes);
}

TEST(PersistentTensorFingerprintGraphTest, RejectsJournalDeletingAnotherNamespace) {
    auto backend = std::make_shared<MapBackend>();
    auto graph = std::make_shared<TensorFingerprintGraph>();
    PersistentTensorFingerprintGraph foreign(graph, backend, "tenant", "domain:entry:other");
    ASSERT_TRUE(foreign.addAdapter("adapter", makeTrain(1.0f), "model"));
    const std::string foreign_key =
        "__tfgp__:tenant:domain:entry:other:entry:adapter";
    ASSERT_TRUE(backend->put("__tfgp__:tenant:domain:txn:forged",
                             deleteJournal(foreign_key)));

    PersistentTensorFingerprintGraph owner(graph, backend, "tenant", "domain");
    EXPECT_FALSE(owner.rehydrate());
    EXPECT_TRUE(backend->get(foreign_key));
    EXPECT_TRUE(graph->entry("adapter"));
}

TEST(PersistentTensorFingerprintGraphTest, RejectsCrossTenantGraphKeyCollision) {
    auto backend = std::make_shared<MapBackend>();
    auto graph = std::make_shared<TensorFingerprintGraph>();
    PersistentTensorFingerprintGraph a(graph, backend, "tenant-a", "domain");
    PersistentTensorFingerprintGraph b(graph, backend, "tenant-b", "domain");
    ASSERT_TRUE(a.addAdapter("shared", makeTrain(1.0f), "model-a"));
    EXPECT_FALSE(b.addAdapter("shared", makeTrain(2.0f), "model-b"));
    EXPECT_FALSE(b.removeAdapter("shared"));
    const auto entry = graph->entry("shared");
    ASSERT_TRUE(entry);
    EXPECT_EQ(entry->tenant_id, "tenant-a");
    EXPECT_TRUE(backend->get("__tfgp__:tenant-a:domain:entry:shared"));
}
} } } // namespace themis::tensor::test
