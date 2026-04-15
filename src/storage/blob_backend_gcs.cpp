/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            blob_backend_gcs.cpp                               ║
  Version:         0.0.10                                             ║
  Last Modified:   2026-04-15 05:44:38                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     270                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 7c2cc11ffb  2026-04-14  refactor: replace (void)var; suppressions with C++17 [[ma... ║
    • ad6e8f172c  2026-04-14  refactor: replace (void)var; suppressions with C++17 [[ma... ║
    • edcfeb9848  2026-03-11  feat: add scripts for auditing and reconciling GitHub iss... ║
    • e561df7fd1  2026-03-09  fix(storage): address second code review - read validatio... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "storage/blob_backend_gcs.h"
#include "utils/logger.h"
#include "utils/error_registry.h"

#ifdef THEMIS_ENABLE_GCS
#include <google/cloud/storage/client.h>
namespace gcs = ::google::cloud::storage;
#endif

#include <openssl/sha.h>
#include <iomanip>
#include <sstream>
#include <mutex>
#include <chrono>

namespace themis {
namespace storage {

// ─────────────────────────────────────────────────────────────────────────────
// Pimpl (keeps google-cloud-cpp types out of the public header)
// ─────────────────────────────────────────────────────────────────────────────
struct GCSBlobBackend::Impl {
    std::string bucket;
    std::string prefix;
    bool        available{false};
    mutable std::mutex mutex;

#ifdef THEMIS_ENABLE_GCS
    std::unique_ptr<gcs::Client> client;
#endif

    Impl(const std::string& bucket_, const std::string& prefix_)
        : bucket(bucket_), prefix(prefix_)
    {
#ifdef THEMIS_ENABLE_GCS
        // Application Default Credentials – fail-closed if absent
        auto credentials = gcs::oauth2::GoogleDefaultCredentials();
        if (!credentials) {
            THEMIS_ERROR("GCSBlobBackend: no valid ADC credentials found ({}). "
                         "Set GOOGLE_APPLICATION_CREDENTIALS before enabling GCS.",
                         credentials.status().message());
            return;
        }

        auto options = gcs::ClientOptions(*credentials);
        client = std::make_unique<gcs::Client>(std::move(options));
        available = true;
        THEMIS_INFO("GCSBlobBackend initialised: bucket={}, prefix={}", bucket, prefix);
#else
        THEMIS_WARN("GCSBlobBackend: compiled without THEMIS_ENABLE_GCS – "
                    "this backend is permanently unavailable.");
#endif
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// Construction
// ─────────────────────────────────────────────────────────────────────────────
GCSBlobBackend::GCSBlobBackend(const std::string& bucket, const std::string& prefix)
    : impl_(std::make_unique<Impl>(bucket, prefix)) {}

GCSBlobBackend::~GCSBlobBackend() = default;

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────
/*static*/ std::string GCSBlobBackend::computeSHA256(const std::vector<uint8_t>& data) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(data.data(), data.size(), hash);

    std::ostringstream ss;
    ss << std::hex << std::setfill('0');
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        ss << std::setw(2) << static_cast<int>(hash[i]);
    }
    return ss.str();
}

std::string GCSBlobBackend::objectName(const std::string& blob_id) const {
    if (impl_->prefix.empty()) {
        return blob_id + ".blob";
    }
    // Ensure exactly one '/' between prefix and blob_id
    std::string pfx = impl_->prefix;
    if (pfx.back() == '/') pfx.pop_back();
    return pfx + "/" + blob_id + ".blob";
}

// ─────────────────────────────────────────────────────────────────────────────
// IBlobStorageBackend interface
// ─────────────────────────────────────────────────────────────────────────────
Result<BlobRef> GCSBlobBackend::put(const std::string& blob_id,
                                    const std::vector<uint8_t>& data) {
    std::lock_guard<std::mutex> lock(impl_->mutex);

    if (!impl_->available) {
        return Err<BlobRef>(errors::ErrorCode::ERR_UTIL_FILE_OPERATION_FAILED,
                            "GCS backend is not available (check ADC credentials)");
    }

#ifdef THEMIS_ENABLE_GCS
    const std::string obj = objectName(blob_id);
    auto writer = impl_->client->WriteObject(impl_->bucket, obj);
    writer.write(reinterpret_cast<const char*>(data.data()),
                 static_cast<std::streamsize>(data.size()));
    writer.Close();

    auto metadata = writer.metadata();
    if (!metadata) {
        THEMIS_ERROR("GCS WriteObject failed for {}: {}", obj, metadata.status().message());
        return Err<BlobRef>(errors::ErrorCode::ERR_UTIL_FILE_OPERATION_FAILED,
                            "GCS upload failed: " + metadata.status().message());
    }

    BlobRef ref;
    ref.id         = blob_id;
    ref.type       = BlobStorageType::GCS;
    ref.uri        = "gs://" + impl_->bucket + "/" + obj;
    ref.size_bytes = static_cast<int64_t>(data.size());
    ref.hash_sha256 = computeSHA256(data);
    ref.created_at = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    THEMIS_DEBUG("GCS blob stored: id={}, size={} bytes", blob_id, data.size());
    return Ok(ref);
#else
    return Err<BlobRef>(errors::ErrorCode::ERR_UTIL_FILE_OPERATION_FAILED,
                        "GCS support not compiled in");
#endif
}

Result<std::vector<uint8_t>> GCSBlobBackend::get(const BlobRef& ref) {
    std::lock_guard<std::mutex> lock(impl_->mutex);

    if (!impl_->available) {
        return Err<std::vector<uint8_t>>(
            errors::ErrorCode::ERR_UTIL_FILE_OPERATION_FAILED,
            "GCS backend is not available (check ADC credentials)");
    }

#ifdef THEMIS_ENABLE_GCS
    const std::string obj = objectName(ref.id);
    auto reader = impl_->client->ReadObject(impl_->bucket, obj);

    if (!reader) {
        const auto& status = reader.status();
        if (status.code() == google::cloud::StatusCode::kNotFound) {
            THEMIS_WARN("GCS blob not found: {}", ref.id);
            return Err<std::vector<uint8_t>>(
                errors::ErrorCode::ERR_STORAGE_FILE_NOT_FOUND,
                "Blob not found in GCS: " + ref.id);
        }
        THEMIS_ERROR("GCS ReadObject failed for {}: {}", obj, status.message());
        return Err<std::vector<uint8_t>>(
            errors::ErrorCode::ERR_UTIL_FILE_OPERATION_FAILED,
            "GCS download failed: " + status.message());
    }

    std::vector<uint8_t> data;
    if (ref.size_bytes > 0) {
        data.reserve(static_cast<std::size_t>(ref.size_bytes));
    }

    char buf[65536];
    while (reader.read(buf, sizeof(buf))) {
        const auto n = reader.gcount();
        data.insert(data.end(), buf, buf + n);
    }
    // Capture any final partial read
    const auto tail = reader.gcount();
    if (tail > 0) {
        data.insert(data.end(), buf, buf + tail);
    }

    // Integrity check
    std::string actual_hash = computeSHA256(data);
    if (!ref.hash_sha256.empty() && actual_hash != ref.hash_sha256) {
        THEMIS_ERROR("GCS blob hash mismatch: id={}, expected={}, actual={}",
                     ref.id, ref.hash_sha256, actual_hash);
        return Err<std::vector<uint8_t>>(
            errors::ErrorCode::ERR_STORAGE_CORRUPTION,
            "Hash mismatch for blob: " + ref.id);
    }

    THEMIS_DEBUG("GCS blob retrieved: id={}, size={} bytes", ref.id, data.size());
    return Ok(std::move(data));
#else
    return Err<std::vector<uint8_t>>(errors::ErrorCode::ERR_UTIL_FILE_OPERATION_FAILED,
                                     "GCS support not compiled in");
#endif
}

Result<void> GCSBlobBackend::remove(const BlobRef& ref) {
    std::lock_guard<std::mutex> lock(impl_->mutex);

    if (!impl_->available) {
        return Err<void>(errors::ErrorCode::ERR_UTIL_FILE_OPERATION_FAILED,
                         "GCS backend is not available (check ADC credentials)");
    }

#ifdef THEMIS_ENABLE_GCS
    const std::string obj = objectName(ref.id);
    auto status = impl_->client->DeleteObject(impl_->bucket, obj);

    if (!status.ok()) {
        THEMIS_ERROR("GCS DeleteObject failed for {}: {}", obj, status.message());
        return Err<void>(errors::ErrorCode::ERR_UTIL_FILE_OPERATION_FAILED,
                         "GCS delete failed: " + status.message());
    }

    THEMIS_DEBUG("GCS blob deleted: id={}", ref.id);
    return OkVoid();
#else
    return Err<void>(errors::ErrorCode::ERR_UTIL_FILE_OPERATION_FAILED,
                     "GCS support not compiled in");
#endif
}

bool GCSBlobBackend::exists(const BlobRef& ref) {
    std::lock_guard<std::mutex> lock(impl_->mutex);

    if (!impl_->available) {
        return false;
    }

#ifdef THEMIS_ENABLE_GCS
    const std::string obj = objectName(ref.id);
    auto metadata = impl_->client->GetObjectMetadata(impl_->bucket, obj);
    return metadata.ok();
#else
    return false;
#endif
}

std::string GCSBlobBackend::name() const {
    return "gcs";
}

bool GCSBlobBackend::isAvailable() const {
    return impl_->available;
}

} // namespace storage
} // namespace themis
