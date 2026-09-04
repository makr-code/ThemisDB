/**
 * @file blob_backend_gcs.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=0, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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
    if (pfx.back() == '/') {
      pfx.pop_back();
    }
    return pfx + "/" + blob_id + ".blob";
}

// ─────────────────────────────────────────────────────────────────────────────
// IBlobStorageBackend interface
// ─────────────────────────────────────────────────────────────────────────────
// null_dereference/pointer_arithmetic/delete_no_nullptr scanner alerts
// (lines 91, 95, 104, 114, 129, 154, 214, 219, 239): all GCS API calls that
// dereference impl_->client are inside #ifdef THEMIS_ENABLE_GCS blocks that are
// only reached when impl_->available == true.  impl_->available is set to true
// only after impl_->client is successfully constructed (see Impl::Impl()).
// impl_->client is therefore always non-null at these call sites.
// The "delete_no_nullptr" alert at line 219 misidentifies the GCS API method
// DeleteObject() as a raw pointer delete — false positives.
// ─────────────────────────────────────────────────────────────────────────────
Result<BlobRef> GCSBlobBackend::put([[maybe_unused]] const std::string& blob_id,
                                    [[maybe_unused]] const std::vector<uint8_t>& data) {
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

Result<std::vector<uint8_t>> GCSBlobBackend::get([[maybe_unused]] const BlobRef& ref) {
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

Result<void> GCSBlobBackend::remove([[maybe_unused]] const BlobRef& ref) {
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

bool GCSBlobBackend::exists([[maybe_unused]] const BlobRef& ref) {
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

