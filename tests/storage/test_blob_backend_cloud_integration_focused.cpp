// SPDX-License-Identifier: Apache-2.0

#include <gtest/gtest.h>

#include "storage/blob_backend_azure.h"
#include "storage/blob_backend_gcs.h"
#include "storage/blob_backend_s3.h"

#include <cstdlib>
#include <optional>
#include <string>
#include <vector>

using namespace themis::storage;

namespace {

bool envEnabled(const char* name) {
    const char* value = std::getenv(name);
    return value != nullptr && std::string(value) == "1";
}

std::optional<std::string> envString(const char* name) {
    const char* value = std::getenv(name);
    if (value == nullptr || std::string(value).empty()) {
        return std::nullopt;
    }
    return std::string(value);
}

BlobRef makeRef(BlobStorageType type, const std::string& id, const std::string& uri) {
    BlobRef ref;
    ref.id = id;
    ref.type = type;
    ref.uri = uri;
    ref.size_bytes = 0;
    return ref;
}

void requireIntegrationEnv() {
    if (!envEnabled("BLOB_INTEGRATION_TESTS")) {
        GTEST_SKIP() << "Set BLOB_INTEGRATION_TESTS=1 to run blob emulator integration tests";
    }
}

} // namespace

#if defined(THEMIS_HAS_AWS_SDK) && THEMIS_HAS_AWS_SDK
TEST(S3BlobBackend, PresignedUrlRejectsInvalidExpiry) {
    S3BlobBackend backend("bucket", "us-east-1");
    auto ref = makeRef(BlobStorageType::S3, "blob-a", "s3://bucket/blob-a.blob");

    auto result = backend.presignedUrl(ref, 0);
    EXPECT_FALSE(result.has_value());
    EXPECT_NE(result.error().message().find("expiry"), std::string::npos);
}

TEST(S3BlobBackend, MinioIntegrationRoundTripWhenConfigured) {
    requireIntegrationEnv();

    auto bucket = envString("THEMIS_BLOB_S3_BUCKET");
    auto region = envString("THEMIS_BLOB_S3_REGION");
    auto endpoint = envString("THEMIS_BLOB_S3_ENDPOINT");
    if (!bucket || !region || !endpoint) {
        GTEST_SKIP() << "Set THEMIS_BLOB_S3_BUCKET, THEMIS_BLOB_S3_REGION and THEMIS_BLOB_S3_ENDPOINT";
    }

    const bool force_path_style = envEnabled("THEMIS_BLOB_S3_FORCE_PATH_STYLE");
    S3BlobBackend backend(*bucket, *region, "blob-tests", SseConfig{}, RetryPolicy{}, *endpoint,
                          force_path_style);
    if (!backend.isAvailable()) {
        GTEST_SKIP() << "S3/MinIO backend unavailable with current credentials or endpoint";
    }

    const std::string blob_id = "minio-integration-blob";
    const std::vector<uint8_t> payload = {1, 3, 3, 7};
    auto put_result = backend.put(blob_id, payload);
    ASSERT_TRUE(put_result.has_value());

    auto get_result = backend.get(put_result.value());
    ASSERT_TRUE(get_result.has_value());
    EXPECT_EQ(get_result.value(), payload);

    auto delete_result = backend.remove(put_result.value());
    EXPECT_TRUE(delete_result.has_value());
}
#endif

#if defined(THEMIS_HAS_AZURE_STORAGE) && THEMIS_HAS_AZURE_STORAGE
TEST(AzureBlobBackend, PresignedUrlRejectsInvalidExpiry) {
    AzureBlobBackend backend("", "container");
    auto ref = makeRef(BlobStorageType::AZURE_BLOB, "blob-a", "azure://container/blob-a.blob");

    auto result = backend.presignedUrl(ref, 604801);
    EXPECT_FALSE(result.has_value());
    EXPECT_NE(result.error().message().find("expiry"), std::string::npos);
}

TEST(AzureBlobBackend, AzuriteIntegrationRoundTripWhenConfigured) {
    requireIntegrationEnv();

    auto connection_string = envString("THEMIS_BLOB_AZURE_CONNECTION_STRING");
    auto container = envString("THEMIS_BLOB_AZURE_CONTAINER");
    if (!connection_string || !container) {
        GTEST_SKIP() << "Set THEMIS_BLOB_AZURE_CONNECTION_STRING and THEMIS_BLOB_AZURE_CONTAINER";
    }

    AzureBlobBackend backend(*connection_string, *container, "blob-tests");
    if (!backend.isAvailable()) {
        GTEST_SKIP() << "Azure/Azurite backend unavailable with current connection string";
    }

    const std::string blob_id = "azurite-integration-blob";
    const std::vector<uint8_t> payload = {2, 4, 6, 8};
    auto put_result = backend.put(blob_id, payload);
    ASSERT_TRUE(put_result.has_value());

    auto get_result = backend.get(put_result.value());
    ASSERT_TRUE(get_result.has_value());
    EXPECT_EQ(get_result.value(), payload);

    auto delete_result = backend.remove(put_result.value());
    EXPECT_TRUE(delete_result.has_value());
}
#endif

TEST(GCSBlobBackend, FakeGcsIntegrationRoundTripWhenConfigured) {
    requireIntegrationEnv();

    auto bucket = envString("THEMIS_BLOB_GCS_BUCKET");
    if (!bucket) {
        GTEST_SKIP() << "Set THEMIS_BLOB_GCS_BUCKET (and emulator/auth environment) to run fake-gcs-server test";
    }

    GCSBlobBackend backend(*bucket, "blob-tests");
    if (!backend.isAvailable()) {
        GTEST_SKIP() << "GCS backend unavailable; configure fake-gcs-server endpoint and credentials/env first";
    }

    const std::string blob_id = "fake-gcs-integration-blob";
    const std::vector<uint8_t> payload = {8, 6, 7, 5, 3, 0, 9};
    auto put_result = backend.put(blob_id, payload);
    ASSERT_TRUE(put_result.has_value());

    auto get_result = backend.get(put_result.value());
    ASSERT_TRUE(get_result.has_value());
    EXPECT_EQ(get_result.value(), payload);

    auto delete_result = backend.remove(put_result.value());
    EXPECT_TRUE(delete_result.has_value());
}
