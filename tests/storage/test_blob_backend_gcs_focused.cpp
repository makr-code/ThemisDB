// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_blob_backend_gcs_focused.cpp
 * @brief GCS blob backend contract-hardening focused tests (GCS-01..08).
 *
 * All tests run without an actual GCS bucket.  When the backend is compiled
 * without THEMIS_ENABLE_GCS (the normal CI configuration), the constructor
 * marks itself unavailable and every API method must return a structured error.
 * When THEMIS_ENABLE_GCS is defined but no ADC credentials are present, the
 * constructor also marks itself unavailable via the fail-closed path.
 *
 * ## Test Cases
 *
 * ### GCS-01 — Unavailable backend put() returns error
 *   Verified: put() on unavailable backend returns ERR_UTIL_FILE_OPERATION_FAILED.
 *
 * ### GCS-02 — Unavailable backend get() returns error
 *   Verified: get() on unavailable backend returns an error.
 *
 * ### GCS-03 — Unavailable backend remove() returns error
 *   Verified: remove() on unavailable backend returns an error.
 *
 * ### GCS-04 — Unavailable backend exists() returns false
 *   Verified: exists() on unavailable backend returns false.
 *
 * ### GCS-05 — name() returns "gcs"
 *   Verified: backend type identifier is always "gcs".
 *
 * ### GCS-06 — isAvailable() returns false without ADC credentials
 *   Verified: backend is unavailable when GCS is disabled / no credentials.
 *
 * ### GCS-07 — presignedUrl() returns error when backend is unavailable
 *   Verified: presignedUrl() on unavailable backend returns an error.
 *
 * ### GCS-08 — presignedUrl() rejects invalid expiry values
 *   Verified: expiry <= 0 and expiry > 604800 both return an error.
 *
 * @see include/storage/blob_backend_gcs.h
 * @see src/storage/blob_backend_gcs.cpp
 * @see plugins/blob_storage/gcs/plugin.json
 */

#include <gtest/gtest.h>

#include "storage/blob_backend_gcs.h"
#include "storage/blob_storage_backend.h"

using namespace themis::storage;

namespace {

/// Build an unavailable GCS backend (no ADC credentials in CI, or GCS disabled).
GCSBlobBackend makeUnavailableBackend() {
    return GCSBlobBackend("test-bucket", "test/prefix");
}

/// Build a BlobRef suitable for read/delete/presign tests.
BlobRef makeBlobRef(const std::string& id = "test-blob-id") {
    BlobRef ref;
    ref.id         = id;
    ref.type       = BlobStorageType::GCS;
    ref.uri        = "gs://test-bucket/test/prefix/" + id + ".blob";
    ref.size_bytes = 42;
    ref.hash_sha256 = "";
    return ref;
}

} // namespace

// ─────────────────────────────────────────────────────────────────────────────
// GCS-01 — put() on unavailable backend returns error
// ─────────────────────────────────────────────────────────────────────────────
TEST(GCSBlobBackend, GCS01_PutReturnsErrorWhenUnavailable) {
    auto backend = makeUnavailableBackend();
    ASSERT_FALSE(backend.isAvailable());

    std::vector<uint8_t> data = {0x01, 0x02, 0x03};
    auto result = backend.put("blob-id", data);
    EXPECT_FALSE(result.has_value());
    EXPECT_FALSE(result.error().message().empty());
}

// ─────────────────────────────────────────────────────────────────────────────
// GCS-02 — get() on unavailable backend returns error
// ─────────────────────────────────────────────────────────────────────────────
TEST(GCSBlobBackend, GCS02_GetReturnsErrorWhenUnavailable) {
    auto backend = makeUnavailableBackend();
    ASSERT_FALSE(backend.isAvailable());

    auto result = backend.get(makeBlobRef());
    EXPECT_FALSE(result.has_value());
    EXPECT_FALSE(result.error().message().empty());
}

// ─────────────────────────────────────────────────────────────────────────────
// GCS-03 — remove() on unavailable backend returns error
// ─────────────────────────────────────────────────────────────────────────────
TEST(GCSBlobBackend, GCS03_RemoveReturnsErrorWhenUnavailable) {
    auto backend = makeUnavailableBackend();
    ASSERT_FALSE(backend.isAvailable());

    auto result = backend.remove(makeBlobRef());
    EXPECT_FALSE(result.has_value());
    EXPECT_FALSE(result.error().message().empty());
}

// ─────────────────────────────────────────────────────────────────────────────
// GCS-04 — exists() on unavailable backend returns false
// ─────────────────────────────────────────────────────────────────────────────
TEST(GCSBlobBackend, GCS04_ExistsReturnsFalseWhenUnavailable) {
    auto backend = makeUnavailableBackend();
    ASSERT_FALSE(backend.isAvailable());

    EXPECT_FALSE(backend.exists(makeBlobRef()));
}

// ─────────────────────────────────────────────────────────────────────────────
// GCS-05 — name() returns "gcs"
// ─────────────────────────────────────────────────────────────────────────────
TEST(GCSBlobBackend, GCS05_NameIsGcs) {
    auto backend = makeUnavailableBackend();
    EXPECT_EQ(backend.name(), "gcs");
}

// ─────────────────────────────────────────────────────────────────────────────
// GCS-06 — isAvailable() is false without ADC credentials / GCS disabled
// ─────────────────────────────────────────────────────────────────────────────
TEST(GCSBlobBackend, GCS06_IsAvailableFalseWithoutCredentials) {
    GCSBlobBackend backend("my-bucket");
    EXPECT_FALSE(backend.isAvailable());
}

// ─────────────────────────────────────────────────────────────────────────────
// GCS-07 — presignedUrl() returns error when backend is unavailable
// ─────────────────────────────────────────────────────────────────────────────
TEST(GCSBlobBackend, GCS07_PresignedUrlErrorWhenUnavailable) {
    auto backend = makeUnavailableBackend();
    ASSERT_FALSE(backend.isAvailable());

    auto result = backend.presignedUrl(makeBlobRef(), 3600);
    EXPECT_FALSE(result.has_value());
    EXPECT_FALSE(result.error().message().empty());
}

// ─────────────────────────────────────────────────────────────────────────────
// GCS-08 — presignedUrl() rejects invalid expiry values
// ─────────────────────────────────────────────────────────────────────────────
TEST(GCSBlobBackend, GCS08_PresignedUrlRejectsInvalidExpiry) {
    auto backend = makeUnavailableBackend();

    // expiry == 0 → expiry validation fires before availability check
    auto r1 = backend.presignedUrl(makeBlobRef(), 0);
    EXPECT_FALSE(r1.has_value());
    EXPECT_NE(r1.error().message().find("expiry"), std::string::npos);

    // expiry < 0
    auto r2 = backend.presignedUrl(makeBlobRef(), -1);
    EXPECT_FALSE(r2.has_value());
    EXPECT_NE(r2.error().message().find("expiry"), std::string::npos);

    // expiry > 604800 (one second over the maximum)
    auto r3 = backend.presignedUrl(makeBlobRef(), 604801);
    EXPECT_FALSE(r3.has_value());
    EXPECT_NE(r3.error().message().find("expiry"), std::string::npos);

    // valid lower boundary — fails with unavailable, not expiry
    auto r4 = backend.presignedUrl(makeBlobRef(), 1);
    EXPECT_FALSE(r4.has_value());
    EXPECT_EQ(r4.error().message().find("expiry"), std::string::npos);

    // valid upper boundary — fails with unavailable, not expiry
    auto r5 = backend.presignedUrl(makeBlobRef(), 604800);
    EXPECT_FALSE(r5.has_value());
    EXPECT_EQ(r5.error().message().find("expiry"), std::string::npos);
}
