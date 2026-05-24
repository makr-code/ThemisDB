/*
 * ThemisDB | File: blob_backend_filesystem.cpp | Version: 0.0.47 | Last Modified: 2026-05-20 17:13:04
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 157
 * Open Issues: TODOs=1, Stubs=1, Gaps=3, Unimpl=0, Mock=1, Sim=0, Debt=0
 * Gap Correlation: internal=3 | external_v3=36 | delta=33 | status=divergent
 * External Severity (v3): C=1, H=26, M=9
 * PR: #746 [Phase 4] Storage Layer: Migrate error handling to Result<T> pattern (2026-03-11T18:06:57Z)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#include "storage/blob_backend_filesystem.h"
#include <stdexcept>
#include "utils/error_registry.h"
#include "utils/expected.h"
#include "utils/logger.h"
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <openssl/sha.h>

namespace themis {
namespace storage {

namespace fs = std::filesystem;

std::string FilesystemBlobBackend::computeSHA256(const std::vector<uint8_t>& data) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(data.data(), data.size(), hash);

    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0')
           << static_cast<int>(hash[i]);
    }
    return ss.str();
}

std::string FilesystemBlobBackend::getPath(const std::string& blob_id) const {
    if (blob_id.length() < 4) {
        throw std::runtime_error("Invalid blob_id: too short");
    }

    std::string prefix = blob_id.substr(0, 2);
    std::string subdir = blob_id.substr(2, 2);

    fs::path dir_path = fs::path(base_path_) / prefix / subdir;
    return (dir_path / (blob_id + ".blob")).string();
}

FilesystemBlobBackend::FilesystemBlobBackend(const std::string& base_path)
    : base_path_(base_path) {
    try {
        fs::create_directories(base_path_);
        THEMIS_INFO("FilesystemBlobBackend initialized: path={}", base_path_);
    } catch (const std::exception& e) {
        THEMIS_ERROR("Failed to create blob storage directory: {}", e.what());
        throw;
    }
}

Result<BlobRef> FilesystemBlobBackend::put(const std::string& blob_id, const std::vector<uint8_t>& data) {
    std::string file_path = getPath(blob_id);

    try {
        fs::create_directories(fs::path(file_path).parent_path());

        std::ofstream ofs(file_path, std::ios::binary);
        if (!ofs) {
            throw std::runtime_error("Failed to open file for writing: " + file_path);
        }

        ofs.write(reinterpret_cast<const char*>(data.data()), data.size());
        ofs.close();

        if (!ofs) {
            throw std::runtime_error("Failed to write blob to file: " + file_path);
        }

        BlobRef ref;
        ref.id = blob_id;
        ref.type = BlobStorageType::FILESYSTEM;
        ref.uri = file_path;
        ref.size_bytes = static_cast<int64_t>(data.size());
        ref.hash_sha256 = computeSHA256(data);
        ref.created_at = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();

        THEMIS_DEBUG("FilesystemBlobBackend: Stored blob {} ({} bytes) at {}",
            blob_id, data.size(), file_path);

        return Ok(std::move(ref));

    } catch (const std::exception& e) {
        THEMIS_ERROR("FilesystemBlobBackend::put failed for {}: {}", blob_id, e.what());
        return Err<BlobRef>(errors::ErrorCode::ERR_STORAGE_DISK_FULL, e.what());
    }
}

Result<std::vector<uint8_t>> FilesystemBlobBackend::get(const BlobRef& ref) {
    try {
        std::ifstream ifs(ref.uri, std::ios::binary);
        if (!ifs) {
            THEMIS_WARN("FilesystemBlobBackend: Blob not found: {}", ref.uri);
            return Err<std::vector<uint8_t>>(errors::ErrorCode::ERR_STORAGE_FILE_NOT_FOUND,
                                             "Blob not found: " + ref.uri);
        }

        std::vector<uint8_t> data(
            (std::istreambuf_iterator<char>(ifs)),
            std::istreambuf_iterator<char>()
        );

        THEMIS_DEBUG("FilesystemBlobBackend: Retrieved blob {} ({} bytes)",
            ref.id, data.size());

        return Ok(std::move(data));

    } catch (const std::exception& e) {
        THEMIS_ERROR("FilesystemBlobBackend::get failed for {}: {}", ref.id, e.what());
        return Err<std::vector<uint8_t>>(errors::ErrorCode::ERR_STORAGE_CORRUPTION, e.what());
    }
}

Result<void> FilesystemBlobBackend::remove(const BlobRef& ref) {
    try {
        if (fs::remove(ref.uri)) {
            THEMIS_DEBUG("FilesystemBlobBackend: Removed blob {}", ref.id);
            return OkVoid();
        }
        return Err<void>(
            errors::ErrorCode::ERR_STORAGE_FILE_NOT_FOUND,
            "Blob not found: " + ref.uri
        );
    } catch (const std::exception& e) {
        THEMIS_ERROR("FilesystemBlobBackend::remove failed for {}: {}", ref.id, e.what());
        return Err<void>(errors::ErrorCode::ERR_UTIL_FILE_OPERATION_FAILED, e.what());
    }
}

bool FilesystemBlobBackend::exists(const BlobRef& ref) {
    return fs::exists(ref.uri);
}

std::string FilesystemBlobBackend::name() const {
    return "filesystem";
}

bool FilesystemBlobBackend::isAvailable() const {
    try {
        return fs::exists(base_path_) && fs::is_directory(base_path_);
    } catch (...) {
        return false;
    }
}

} // namespace storage
} // namespace themis

