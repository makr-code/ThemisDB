// SPDX-License-Identifier: Apache-2.0

#include <gtest/gtest.h>

#include "storage/federated_blob_router.h"

#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

using namespace themis::storage;

namespace {

class InMemoryBlobBackend final : public IBlobStorageBackend {
public:
    explicit InMemoryBlobBackend(std::string backend_name)
        : name_(std::move(backend_name)) {}

    Result<BlobRef> put(const std::string& blob_id,
                        const std::vector<uint8_t>& data) override {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!available_) {
            return Err<BlobRef>(errors::ErrorCode::ERR_UTIL_FILE_OPERATION_FAILED,
                                name_ + " unavailable");
        }
        if (fail_put_) {
            return Err<BlobRef>(errors::ErrorCode::ERR_UTIL_FILE_OPERATION_FAILED,
                                name_ + " put failure");
        }

        data_[blob_id] = data;

        BlobRef ref;
        ref.id = blob_id;
        ref.type = BlobStorageType::CUSTOM;
        ref.uri = name_ + "://" + blob_id;
        ref.size_bytes = static_cast<int64_t>(data.size());
        ref.hash_sha256 = "test";
        return Ok(ref);
    }

    Result<std::vector<uint8_t>> get(const BlobRef& ref) override {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!available_ || fail_get_) {
            return Err<std::vector<uint8_t>>(errors::ErrorCode::ERR_UTIL_FILE_OPERATION_FAILED,
                                             name_ + " get failure");
        }

        auto it = data_.find(ref.id);
        if (it == data_.end()) {
            return Err<std::vector<uint8_t>>(errors::ErrorCode::ERR_STORAGE_FILE_NOT_FOUND,
                                             name_ + " missing blob");
        }
        return Ok(it->second);
    }

    Result<void> remove(const BlobRef& ref) override {
        std::lock_guard<std::mutex> lock(mutex_);
        data_.erase(ref.id);
        return OkVoid();
    }

    bool exists(const BlobRef& ref) override {
        std::lock_guard<std::mutex> lock(mutex_);
        return data_.contains(ref.id);
    }

    std::string name() const override { return name_; }
    bool isAvailable() const override { return available_; }

    void setFailPut(bool value) { fail_put_ = value; }
    void setFailGet(bool value) { fail_get_ = value; }
    void setAvailable(bool value) { available_ = value; }
    size_t storedCount() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return data_.size();
    }

private:
    std::string name_;
    bool available_{true};
    bool fail_put_{false};
    bool fail_get_{false};
    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::vector<uint8_t>> data_;
};

TEST(FederatedBlobRouter, RegisterBackendRejectsEmptyRegion) {
    FederatedBlobRouter router;
    auto backend = std::make_shared<InMemoryBlobBackend>("primary");

    auto result = router.registerBackend("", backend);
    EXPECT_FALSE(result.has_value());
}

TEST(FederatedBlobRouter, PutReplicatesToPrimaryAndReplica) {
    FederatedBlobRouter router;
    auto primary = std::make_shared<InMemoryBlobBackend>("primary");
    auto replica = std::make_shared<InMemoryBlobBackend>("replica");

    ASSERT_TRUE(router.registerBackend("eu-central-1", primary).has_value());
    ASSERT_TRUE(router.registerBackend("us-east-1", replica).has_value());

    FederatedBlobWritePlan plan;
    plan.primary_region = "eu-central-1";
    plan.replica_targets.push_back({"us-east-1", true});

    const std::vector<uint8_t> payload = {1, 2, 3, 4};
    auto result = router.put(plan, "blob-a", payload);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().primary_region, "eu-central-1");
    EXPECT_TRUE(result.value().region_refs.contains("eu-central-1"));
    EXPECT_TRUE(result.value().region_refs.contains("us-east-1"));
    EXPECT_EQ(primary->storedCount(), 1u);
    EXPECT_EQ(replica->storedCount(), 1u);
}

TEST(FederatedBlobRouter, GetFallsBackAcrossRegions) {
    FederatedBlobRouter router;
    auto primary = std::make_shared<InMemoryBlobBackend>("primary");
    auto replica = std::make_shared<InMemoryBlobBackend>("replica");

    ASSERT_TRUE(router.registerBackend("eu-central-1", primary).has_value());
    ASSERT_TRUE(router.registerBackend("us-east-1", replica).has_value());

    FederatedBlobWritePlan plan;
    plan.primary_region = "eu-central-1";
    plan.replica_targets.push_back({"us-east-1", true});

    const std::vector<uint8_t> payload = {9, 8, 7};
    auto route = router.put(plan, "blob-b", payload);
    ASSERT_TRUE(route.has_value());

    primary->setFailGet(true);
    auto result = router.get(route.value(), "eu-central-1");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), payload);
}

TEST(FederatedBlobRouter, RequiredReplicaFailureRollsBackPrimary) {
    FederatedBlobRouter router;
    auto primary = std::make_shared<InMemoryBlobBackend>("primary");
    auto replica = std::make_shared<InMemoryBlobBackend>("replica");
    replica->setFailPut(true);

    ASSERT_TRUE(router.registerBackend("eu-central-1", primary).has_value());
    ASSERT_TRUE(router.registerBackend("us-east-1", replica).has_value());

    FederatedBlobWritePlan plan;
    plan.primary_region = "eu-central-1";
    plan.replica_targets.push_back({"us-east-1", true});

    auto result = router.put(plan, "blob-c", {5, 4, 3});
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(primary->storedCount(), 0u);
    EXPECT_EQ(replica->storedCount(), 0u);
}

TEST(FederatedBlobRouter, OptionalReplicaFailureDoesNotAbortPrimary) {
    FederatedBlobRouter router;
    auto primary = std::make_shared<InMemoryBlobBackend>("primary");
    auto replica = std::make_shared<InMemoryBlobBackend>("replica");
    replica->setFailPut(true);

    ASSERT_TRUE(router.registerBackend("eu-central-1", primary).has_value());
    ASSERT_TRUE(router.registerBackend("us-east-1", replica).has_value());

    FederatedBlobWritePlan plan;
    plan.primary_region = "eu-central-1";
    plan.replica_targets.push_back({"us-east-1", false});

    auto result = router.put(plan, "blob-d", {1, 1, 2, 3, 5});
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().optional_failures.size(), 1u);
    EXPECT_EQ(result.value().optional_failures.front(), "us-east-1");
    EXPECT_EQ(primary->storedCount(), 1u);
}

} // namespace
