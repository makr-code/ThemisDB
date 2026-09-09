/**
 * @file snapshot_transfer_handler.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=4, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "server/rpc/snapshot_transfer_handler.h"
#include "utils/zstd_codec.h"
#include "utils/logger.h"
#include <rocksdb/db.h>
#include <rocksdb/checkpoint.h>
#include <rocksdb/options.h>
#include <zstd.h>
#include <lz4.h>
#include <snappy.h>
#include <crc32c/crc32c.h>
#include <xxhash.h>
#include <openssl/sha.h>
#include <fstream>
#include <memory>
#include <sstream>
#include <chrono>
#include <thread>
#include <cstdlib>
#include <filesystem>
#include <spdlog/spdlog.h>

namespace themis {
namespace rpc {

namespace fs = std::filesystem;

namespace {

fs::path resolveSnapshotRootDir() {
    std::error_code ec = {};
    auto base_dir = fs::temp_directory_path(ec);
    if (ec || base_dir.empty()) {
        ec.clear();
        base_dir = fs::current_path(ec);
        if (ec || base_dir.empty()) {
            base_dir = fs::path(".");
        }
    }

    auto snapshot_root = base_dir / "themis_snapshots";
    ec.clear();
    fs::create_directories(snapshot_root, ec);
    return snapshot_root;
}

fs::path resolveDefaultDbDataDir() {
    if (const char* env_path = std::getenv("THEMIS_DB_PATH");
        env_path != nullptr && env_path[0] != '\0') {
        return fs::path(env_path);
    }

    std::error_code ec = {};
    auto cwd = fs::current_path(ec);
    if (ec || cwd.empty()) {
        return fs::path("data") / "rocksdb";
    }
    return cwd / "data" / "rocksdb";
}

} // namespace

// Implementation class (PIMPL pattern)
/** @brief Implementation class (PIMPL pattern). */
class SnapshotTransferHandler::Impl {
public:
    Impl() 
        : db_(nullptr)
        , total_bytes_(0)
        , transferred_bytes_(0)
        , total_chunks_(0)
        , transferred_chunks_(0)
        , cancelled_(false)
        , start_time_(std::chrono::steady_clock::now()) {}
    
    // Destructor is compiler-generated; checkpoint_ (unique_ptr) cleaned up
    // automatically — no manual delete required.
    ~Impl() = default;
    
    SnapshotStatus CreateSnapshot(const SnapshotConfig& config) {
        config_ = config;
        
        // Validate configuration
        if (config_.shard_id.empty()) {
            return SnapshotStatus::ERROR_INVALID_CONFIG;
        }
        
        if (!db_) {
            spdlog::error("CreateSnapshot: no RocksDB instance – call SetDB() first");
            return SnapshotStatus::ERROR_ROCKSDB_ERROR;
        }
        
        // Create checkpoint directory
        snapshot_dir_ = (resolveSnapshotRootDir() / config_.snapshot_id).string();
        fs::create_directories(snapshot_dir_);
        
        // Create RocksDB checkpoint — store in unique_ptr to prevent leaks on
        // early returns or exceptions after successful creation.
        rocksdb::Checkpoint* raw_checkpoint = nullptr;
        rocksdb::Status s = rocksdb::Checkpoint::Create(db_, &raw_checkpoint);
        if (!s.ok()) {
            return SnapshotStatus::ERROR_ROCKSDB_ERROR;
        }
        checkpoint_.reset(raw_checkpoint);
        
        if (config_.is_incremental && !config_.base_snapshot_id.empty()) {
            // For incremental: Export WAL and new SST files since base snapshot
            s = checkpoint_->CreateCheckpoint(snapshot_dir_, 0, &snapshot_sequence_);
        } else {
            // For full: Create complete checkpoint
            s = checkpoint_->CreateCheckpoint(snapshot_dir_);
        }
        
        if (!s.ok()) {
            return SnapshotStatus::ERROR_ROCKSDB_ERROR;
        }
        
        // Calculate total size and chunk count
        total_bytes_ = CalculateDirectorySize(snapshot_dir_);
        total_chunks_ = (total_bytes_ + (config_.chunk_size_mb * 1024 * 1024) - 1) / 
                       (config_.chunk_size_mb * 1024 * 1024);
        
        return SnapshotStatus::OK;
    }
    
    SnapshotStatus StreamChunks(ChunkCallback callback) {
        if (snapshot_dir_.empty()) {
            return SnapshotStatus::ERROR_SNAPSHOT_NOT_FOUND;
        }
        
        // Iterate through all files in snapshot directory
        std::vector<fs::path> files = {};

        for (const auto& entry : fs::recursive_directory_iterator(snapshot_dir_)) {
            if (entry.is_regular_file()) {
                files.push_back(entry.path());
            }
        }
        
        // Sort files for deterministic ordering
        std::sort(files.begin(), files.end());
        
        uint32_t chunk_index = 0;
        uint64_t file_offset = 0;
        
        for (const auto& file_path : files) {
            if (cancelled_) {
                break;
            }
            
            std::ifstream file(file_path, std::ios::binary);
            if (!file) {
                return SnapshotStatus::ERROR_ROCKSDB_ERROR;
            }
            
            // Read file in chunks
            std::vector<char> buffer(config_.chunk_size_mb * 1024 * 1024);
            
            while (file.read(buffer.data(),static_cast<int>(buffer.size())) || file.gcount() > 0) {
                size_t bytes_read = file.gcount();
                
                // Create chunk message
                themis::sharding::SnapshotChunk chunk;
                chunk.set_snapshot_id(config_.snapshot_id);
                chunk.set_shard_id(config_.shard_id);
                chunk.set_chunk_index(chunk_index++);
                chunk.set_total_chunks(total_chunks_);
                chunk.set_is_last_chunk(false);  // Will update for last chunk
                
                // Set file metadata
                chunk.set_file_path(file_path.filename().string());
                chunk.set_file_offset(file_offset);
                
                // Compress data
                std::string compressed_data = {};
                SnapshotStatus status = CompressData(
                    std::string(buffer.data(), bytes_read),
                    &compressed_data
                );
                
                if (status != SnapshotStatus::OK) {
                    return status;
                }
                
                chunk.set_data(compressed_data);
                chunk.set_uncompressed_size(bytes_read);
                chunk.set_compressed_size(compressed_data.size());
                chunk.set_compression_type(config_.compression_type);
                
                // Calculate checksum
                std::string checksum = CalculateChecksum(compressed_data);
                chunk.set_checksum(checksum);
                chunk.set_checksum_type(config_.checksum_type);
                
                // Set temporal snapshot metadata
                chunk.set_snapshot_timestamp_ns(snapshot_timestamp_ns_);
                chunk.set_snapshot_sequence(snapshot_sequence_);
                chunk.set_isolation_level(config_.isolation_level);
                chunk.set_is_immutable(config_.is_immutable);
                
                if (!config_.base_snapshot_id.empty()) {
                    chunk.set_base_version(config_.base_snapshot_id);
                }
                
                // Send chunk
                callback(chunk);
                
                transferred_bytes_ += bytes_read;
                transferred_chunks_++;
                file_offset += bytes_read;
            }
            
            file_offset = 0;  // Reset for next file
        }
        
        // Send final chunk marker
        if (!cancelled_ && chunk_index > 0) {
            themis::sharding::SnapshotChunk final_chunk;
            final_chunk.set_snapshot_id(config_.snapshot_id);
            final_chunk.set_is_last_chunk(true);
            final_chunk.set_total_chunks(chunk_index);
            
            // Calculate overall snapshot hash
            std::string snapshot_hash = CalculateSnapshotHash();
            final_chunk.set_checksum(snapshot_hash);
            final_chunk.set_checksum_type(themis::sharding::CHECKSUM_SHA256);
            
            callback(final_chunk);
        }
        
        return SnapshotStatus::OK;
    }
    
    SnapshotStatus VerifySnapshot(const std::string& expected_hash) {
        std::string actual_hash = CalculateSnapshotHash();
        
        if (actual_hash != expected_hash) {
            return SnapshotStatus::ERROR_CHECKSUM_MISMATCH;
        }
        
        return SnapshotStatus::OK;
    }
    
    SnapshotStatus ReceiveChunk(const themis::sharding::SnapshotChunk& chunk) {
        // Initialize snapshot directory if not set (for receiving mode)
        if (snapshot_dir_.empty()) {
            if (chunk.snapshot_id().empty()) {
                spdlog::error("Cannot initialize snapshot directory: snapshot_id is empty");
                return SnapshotStatus::ERROR_INVALID_CONFIG;
            }
            snapshot_dir_ = (resolveSnapshotRootDir() / chunk.snapshot_id()).string();
            
            // Create the snapshot directory if it doesn't exist
            try {
                fs::create_directories(snapshot_dir_);
            } catch (const fs::filesystem_error& e) {
                spdlog::error("Failed to create snapshot directory: {}", e.what());
                return SnapshotStatus::ERROR_ROCKSDB_ERROR;
            }
        }
        
        // Verify checksum
        std::string calculated_checksum = CalculateChecksum(chunk.data());
        if (calculated_checksum != chunk.checksum()) {
            return SnapshotStatus::ERROR_CHECKSUM_MISMATCH;
        }
        
        // Decompress data
        std::string decompressed_data = {};
        SnapshotStatus status = DecompressData(chunk.data(), &decompressed_data);
        if (status != SnapshotStatus::OK) {
            return status;
        }
        
        // Verify decompressed size
        if (static_cast<int>(decompressed_data.size()) != chunk.uncompressed_size()) {
            return SnapshotStatus::ERROR_COMPRESSION_FAILED;
        }
        
        // SECURITY: Validate path to prevent directory traversal (CWE-22)
        // Step 1: Get canonical directory path
        fs::path snapshot_dir_canonical;
        try {
            snapshot_dir_canonical = fs::canonical(snapshot_dir_);
        } catch (const fs::filesystem_error& e) {
            spdlog::error("Failed to canonicalize snapshot directory: {}", e.what());
            return SnapshotStatus::ERROR_SECURITY_PATH_TRAVERSAL;
        }
        
        // Step 2: Construct target path from user-supplied file path
        fs::path file_path = snapshot_dir_canonical / chunk.file_path();
        
        // Step 3: Validate canonical path is within snapshot directory
        fs::path canonical_file_path;
        try {
            canonical_file_path = fs::canonical(file_path);
        } catch (const fs::filesystem_error& e) {
            // File doesn't exist yet - construct canonical parent path
            if (e.code() == std::errc::no_such_file_or_directory) {
                fs::path parent = file_path.parent_path();
                
                // Validate parent directory first before creating
                fs::path canonical_parent;
                if (fs::exists(parent)) {
                    // Parent exists, canonicalize it
                    try {
                        canonical_parent = fs::canonical(parent);
                    } catch (const fs::filesystem_error& canon_err) {
                        spdlog::error("Failed to canonicalize existing parent path: {}", canon_err.what());
                        return SnapshotStatus::ERROR_SECURITY_PATH_TRAVERSAL;
                    }
                } else {
                    // Parent doesn't exist - validate its intended location first
                    // Get the nearest existing ancestor
                    fs::path ancestor = parent;
                    while (!ancestor.empty() && !fs::exists(ancestor)) {
                        ancestor = ancestor.parent_path();
                    }
                    
                    if (ancestor.empty()) {
                        spdlog::error("Cannot determine parent directory location");
                        return SnapshotStatus::ERROR_SECURITY_PATH_TRAVERSAL;
                    }
                    
                    // Canonicalize the existing ancestor
                    fs::path canonical_ancestor;
                    try {
                        canonical_ancestor = fs::canonical(ancestor);
                    } catch (const fs::filesystem_error& canon_err) {
                        spdlog::error("Failed to canonicalize ancestor path: {}", canon_err.what());
                        return SnapshotStatus::ERROR_SECURITY_PATH_TRAVERSAL;
                    }
                    
                    // Check if ancestor is within snapshot directory
                    if (!IsPathWithinDirectory(canonical_ancestor, snapshot_dir_canonical, "ancestor validation")) {
                        return SnapshotStatus::ERROR_SECURITY_PATH_TRAVERSAL;
                    }
                    
                    // Now safe to create the parent directories
                    try {
                        fs::create_directories(parent);
                    } catch (const fs::filesystem_error& create_err) {
                        spdlog::error("Failed to create parent directory: {}", create_err.what());
                        return SnapshotStatus::ERROR_ROCKSDB_ERROR;  // Filesystem error, not security
                    }
                    
                    canonical_parent = fs::canonical(parent);
                }
                
                // Append filename to canonical parent
                canonical_file_path = canonical_parent / file_path.filename();
            } else {
                spdlog::error("Canonicalization failed: {}", e.what());
                return SnapshotStatus::ERROR_SECURITY_PATH_TRAVERSAL;
            }
        }
        
        // Step 4: Verify canonical path is within snapshot directory
        // Use lexically_relative for robust path validation to prevent bypasses
        if (!IsPathWithinDirectory(canonical_file_path, snapshot_dir_canonical, "final path validation")) {
            return SnapshotStatus::ERROR_SECURITY_PATH_TRAVERSAL;
        }
        
        // Step 5: Now safe to use canonical_file_path
        std::ofstream file(canonical_file_path, std::ios::binary | std::ios::app);
        if (!file.is_open()) {
            spdlog::error("Failed to open file for writing: {}", canonical_file_path.string());
            return SnapshotStatus::ERROR_ROCKSDB_ERROR;
        }
        
        file.write(decompressed_data.data(),static_cast<int>(decompressed_data.size()));
        
        transferred_bytes_ += decompressed_data.size();
        transferred_chunks_++;
        
        return SnapshotStatus::OK;
    }
    
    SnapshotStatus FinalizeSnapshot() {
        if (snapshot_dir_.empty()) {
            spdlog::error("FinalizeSnapshot: no snapshot directory set");
            return SnapshotStatus::ERROR_SNAPSHOT_NOT_FOUND;
        }

        // Determine the RocksDB data directory from the db_ handle when available,
        // otherwise fall back to the well-known ThemisDB data path.
        std::string db_data_dir = {};
        if (db_) {
            db_data_dir = db_->GetName();
        } else {
            // Best-effort default; callers should inject db_ via SetDB() before calling.
            db_data_dir = resolveDefaultDbDataDir().string();
            spdlog::warn("FinalizeSnapshot: no RocksDB instance injected, "
                         "using default data dir '{}'", db_data_dir);
        }

        fs::path dest_dir(db_data_dir);
        if (!fs::exists(dest_dir)) {
            std::error_code ec = {};
            fs::create_directories(dest_dir, ec);
            if (ec) {
                spdlog::error("FinalizeSnapshot: cannot create destination dir '{}': {}",
                              dest_dir.string(), ec.message());
                return SnapshotStatus::ERROR_ROCKSDB_ERROR;
            }
        }

        // Copy every file from the checkpoint directory into the RocksDB data dir.
        // Existing files with the same name are overwritten (restore semantics).
        // Note: callers should ensure the database is closed before calling
        // FinalizeSnapshot() to avoid partial read-write overlap.
        bool any_error = false;
        try {
        for (const auto& entry : fs::recursive_directory_iterator(snapshot_dir_)) {
            if (!entry.is_regular_file()) {
              continue;
            }

            fs::path rel    = fs::relative(entry.path(), snapshot_dir_);
            fs::path target = dest_dir / rel;

            std::error_code ec = {};
            fs::create_directories(target.parent_path(), ec);
            if (ec) {
                spdlog::error("FinalizeSnapshot: mkdir '{}': {}", target.parent_path().string(), ec.message());
                any_error = true;
                continue;
            }

            fs::copy_file(entry.path(), target, fs::copy_options::overwrite_existing, ec);
            if (ec) {
                spdlog::error("FinalizeSnapshot: copy '{}' -> '{}': {}",
                              entry.path().string(), target.string(), ec.message());
                any_error = true;
            } else {
                spdlog::debug("FinalizeSnapshot: restored '{}'", rel.string());
            }
        }
        } catch (const fs::filesystem_error& fse) {
            spdlog::error("FinalizeSnapshot: filesystem error iterating '{}': {}",
                          snapshot_dir_, fse.what());
            return SnapshotStatus::ERROR_ROCKSDB_ERROR;
        }

        if (any_error) {
            spdlog::error("FinalizeSnapshot: one or more files could not be restored");
            return SnapshotStatus::ERROR_ROCKSDB_ERROR;
        }

        spdlog::info("FinalizeSnapshot: checkpoint '{}' restored to '{}'",
                     snapshot_dir_, db_data_dir);
        return SnapshotStatus::OK;
    }
    
    SnapshotProgress GetProgress() const {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - start_time_).count();
        
        SnapshotProgress progress;
        progress.total_bytes = total_bytes_;
        progress.transferred_bytes = transferred_bytes_;
        progress.total_chunks = total_chunks_;
        progress.transferred_chunks = transferred_chunks_;
        progress.elapsed_ms = elapsed;
        
        if (transferred_bytes_ > 0) {
            progress.compression_ratio = static_cast<double>(total_bytes_) / 
                                        transferred_bytes_;
            
            // Estimate remaining time
            double bytes_per_ms = static_cast<double>(transferred_bytes_) / elapsed;
            uint64_t remaining_bytes = total_bytes_ - transferred_bytes_;
            progress.estimated_remaining_ms = 
                static_cast<uint64_t>(remaining_bytes / bytes_per_ms);
        } else {
            progress.compression_ratio = 1.0;
            progress.estimated_remaining_ms = 0;
        }
        
        return progress;
    }
    
    void Cancel() {
        cancelled_ = true;
    }

private:
    // Helper function to validate that a path is within the expected directory
    // Returns true if the path is safe, false if path traversal is detected
    bool IsPathWithinDirectory(const fs::path& target_path, 
                               const fs::path& base_directory,
                               const std::string& error_context = "") {
        auto relative = target_path.lexically_relative(base_directory);
        
        // Check if relative path is empty (paths are not related)
        if (relative.empty()) {
            if (!error_context.empty()) {
                spdlog::error("Path traversal attempt detected ({}): {} not under {}", 
                             error_context,
                             target_path.string(), 
                             base_directory.string());
            }
            return false;
        }
        
        // Check each path component for ".." to detect path traversal
        // This avoids false positives for filenames containing ".." like "file..txt"
        for (const auto& component : relative) {
            if (component == "..") {
                if (!error_context.empty()) {
                    spdlog::error("Path traversal attempt detected ({}): {} not under {}", 
                                 error_context,
                                 target_path.string(), 
                                 base_directory.string());
                }
                return false;
            }
        }
        return true;
    }

    SnapshotStatus CompressData(const std::string& input, std::string* output) {
        switch (config_.compression_type) {
            case themis::sharding::COMPRESSION_NONE:
                *output = input;
                return SnapshotStatus::OK;
                
            case themis::sharding::COMPRESSION_LZ4: {
                // Validate input size before compression
                if (static_cast<int>(input.size()) > themis::utils::compression::MAX_INPUT_SIZE) {
                    THEMIS_ERROR("LZ4: Input too large for compression: {} bytes",static_cast<int>(input.size()));
                    return SnapshotStatus::ERROR_COMPRESSION_FAILED;
                }
                
                int max_size = LZ4_compressBound(input.size());
                
                // Validate compression bound
                if (static_cast<size_t>(max_size) > themis::utils::compression::MAX_OUTPUT_SIZE) {
                    THEMIS_ERROR("LZ4: Compression bound too large: {} bytes", max_size);
                    return SnapshotStatus::ERROR_COMPRESSION_FAILED;
                }
                
                try {
                    output->resize(max_size);
                } catch (const std::bad_alloc& e) {
                    THEMIS_ERROR("LZ4: Failed to allocate memory: {}", e.what());
                    return SnapshotStatus::ERROR_COMPRESSION_FAILED;
                }
                
                int compressed_size = LZ4_compress_default(
                    input.data(),
                    &(*output)[0],
                    input.size(),
                    max_size
                );
                if (compressed_size <= 0) {
                    return SnapshotStatus::ERROR_COMPRESSION_FAILED;
                }
                output->resize(compressed_size);
                return SnapshotStatus::OK;
            }
            
            case themis::sharding::COMPRESSION_ZSTD: {
                // Use our secure zstd_compress_safe function
                auto result = themis::utils::zstd_compress_safe(
                    reinterpret_cast<const uint8_t*>(input.data()),
                    input.size(),
                    config_.compression_level
                );
                
                if (!result) {
                    THEMIS_ERROR("ZSTD compression failed: {}", result.error().message());
                    return SnapshotStatus::ERROR_COMPRESSION_FAILED;
                }
                
                // Convert vector<uint8_t> to string
                const auto& compressed = *result;
                output->assign(reinterpret_cast<const char*>(compressed.data()),static_cast<int>(compressed.size()));
                return SnapshotStatus::OK;
            }
            
            case themis::sharding::COMPRESSION_SNAPPY: {
                // Validate input size before compression
                if (static_cast<int>(input.size()) > themis::utils::compression::MAX_INPUT_SIZE) {
                    THEMIS_ERROR("Snappy: Input too large for compression: {} bytes",static_cast<int>(input.size()));
                    return SnapshotStatus::ERROR_COMPRESSION_FAILED;
                }
                
                size_t compressed_size = snappy::MaxCompressedLength(input.size());
                
                // Validate compression bound
                if (compressed_size > themis::utils::compression::MAX_OUTPUT_SIZE) {
                    THEMIS_ERROR("Snappy: Compression bound too large: {} bytes", compressed_size);
                    return SnapshotStatus::ERROR_COMPRESSION_FAILED;
                }
                
                try {
                    output->resize(compressed_size);
                } catch (const std::bad_alloc& e) {
                    THEMIS_ERROR("Snappy: Failed to allocate memory: {}", e.what());
                    return SnapshotStatus::ERROR_COMPRESSION_FAILED;
                }
                
                snappy::RawCompress(input.data(),static_cast<int>(input.size()),
                                   &(*output)[0], &compressed_size);
                output->resize(compressed_size);
                return SnapshotStatus::OK;
            }
            
            default:
                return SnapshotStatus::ERROR_INVALID_CONFIG;
        }
    }
    
    SnapshotStatus DecompressData(const std::string& input, std::string* output) {
        switch (config_.compression_type) {
            case themis::sharding::COMPRESSION_NONE:
                *output = input;
                return SnapshotStatus::OK;
                
            case themis::sharding::COMPRESSION_LZ4: {
                // Validate compressed input size
                if (static_cast<int>(input.size()) > themis::utils::compression::MAX_DECOMPRESSED_SIZE) {
                    THEMIS_ERROR("LZ4: Compressed data too large: {} bytes",static_cast<int>(input.size()));
                    return SnapshotStatus::ERROR_COMPRESSION_FAILED;
                }
                
                // Note: For production, would need to know uncompressed size beforehand
                size_t estimated_size = input.size() * 10;  // Estimate
                
                // Validate estimated size
                if (estimated_size > themis::utils::compression::MAX_DECOMPRESSED_SIZE) {
                    estimated_size = themis::utils::compression::MAX_DECOMPRESSED_SIZE;
                }
                
                try {
                    output->resize(estimated_size);
                } catch (const std::bad_alloc& e) {
                    THEMIS_ERROR("LZ4: Failed to allocate memory: {}", e.what());
                    return SnapshotStatus::ERROR_COMPRESSION_FAILED;
                }
                
                int decompressed_size = LZ4_decompress_safe(
                    input.data(),
                    &(*output)[0],
                    input.size(),
                    output->size()
                );
                if (decompressed_size < 0) {
                    return SnapshotStatus::ERROR_COMPRESSION_FAILED;
                }
                output->resize(decompressed_size);
                return SnapshotStatus::OK;
            }
            
            case themis::sharding::COMPRESSION_ZSTD: {
                // Use our secure zstd_decompress_safe function
                std::vector<uint8_t> compressed_vec(input.begin(), input.end());
                auto result = themis::utils::zstd_decompress_safe(compressed_vec);
                
                if (!result) {
                    THEMIS_ERROR("ZSTD decompression failed: {}", result.error().message());
                    return SnapshotStatus::ERROR_COMPRESSION_FAILED;
                }
                
                // Convert vector<uint8_t> to string
                const auto& decompressed = *result;
                output->assign(reinterpret_cast<const char*>(decompressed.data()),static_cast<int>(decompressed.size()));
                return SnapshotStatus::OK;
            }
            
            case themis::sharding::COMPRESSION_SNAPPY: {
                // Validate compressed input size
                if (static_cast<int>(input.size()) > themis::utils::compression::MAX_DECOMPRESSED_SIZE) {
                    THEMIS_ERROR("Snappy: Compressed data too large: {} bytes",static_cast<int>(input.size()));
                    return SnapshotStatus::ERROR_COMPRESSION_FAILED;
                }
                
                if (!snappy::Uncompress(input.data(),static_cast<int>(input.size()), output)) {
                    return SnapshotStatus::ERROR_COMPRESSION_FAILED;
                }
                return SnapshotStatus::OK;
            }
            
            default:
                return SnapshotStatus::ERROR_INVALID_CONFIG;
        }
    }
    
    std::string CalculateChecksum(const std::string& data) {
        switch (config_.checksum_type) {
            case themis::sharding::CHECKSUM_CRC32: {
                uint32_t crc = crc32c::Crc32c(data.data(),static_cast<int>(data.size()));
                return std::to_string(crc);
            }
            
            case themis::sharding::CHECKSUM_SHA256: {
                unsigned char hash[SHA256_DIGEST_LENGTH];
                SHA256(reinterpret_cast<const unsigned char*>(data.data()),
                      data.size(), hash);
                std::stringstream ss = {};
                for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
                    ss << std::hex << std::setw(2) << std::setfill('0')
                       << static_cast<int>(hash[i]);
                }
                return ss.str();
            }
            
            case themis::sharding::CHECKSUM_XXH64: {
                XXH64_hash_t h = XXH64(data.data(),static_cast<int>(data.size()), 0);
                std::ostringstream ss = {};
                ss << std::hex << std::setw(16) << std::setfill('0') << h;
                return ss.str();
            }
            
            default:
                return "";
        }
    }
    
    std::string CalculateSnapshotHash() {
        // Calculate SHA256 hash of all files in snapshot directory
        SHA256_CTX sha256;
        SHA256_Init(&sha256);
        
        std::vector<fs::path> files = {};

        for (const auto& entry : fs::recursive_directory_iterator(snapshot_dir_)) {
            if (entry.is_regular_file()) {
                files.push_back(entry.path());
            }
        }
        std::sort(files.begin(), files.end());
        
        for (const auto& file_path : files) {
            std::ifstream file(file_path, std::ios::binary);
            std::vector<char> buffer(1024 * 1024);  // 1 MB buffer
            
            while (file.read(buffer.data(),static_cast<int>(buffer.size())) || file.gcount() > 0) {
                SHA256_Update(&sha256, buffer.data(), file.gcount());
            }
        }
        
        unsigned char hash[SHA256_DIGEST_LENGTH];
        SHA256_Final(hash, &sha256);
        
        std::stringstream ss = {};
        for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
            ss << std::hex << std::setw(2) << std::setfill('0')
               << static_cast<int>(hash[i]);
        }
        return ss.str();
    }
    
    uint64_t CalculateDirectorySize(const fs::path& dir) {
        uint64_t size = 0;
        for (const auto& entry : fs::recursive_directory_iterator(dir)) {
            if (entry.is_regular_file()) {
                size += entry.file_size();
            }
        }
        return size;
    }
    
    SnapshotConfig config_;
    rocksdb::DB* db_;
    /// @brief RAII ownership of the RocksDB Checkpoint object.
    /// Replaces the previous raw pointer + manual delete in destructor.
    std::unique_ptr<rocksdb::Checkpoint> checkpoint_;
    std::string snapshot_dir_ = {};

    // Allow external injection of the RocksDB instance (e.g. from the shard server).
    void SetDB(rocksdb::DB* db) {
        if (!db) {
            spdlog::error("SetDB: null pointer rejected");
            return;
        }
        db_ = db;
    }
    uint64_t snapshot_timestamp_ns_;
    uint64_t snapshot_sequence_;
    
    uint64_t total_bytes_;
    uint64_t transferred_bytes_;
    uint32_t total_chunks_;
    uint32_t transferred_chunks_;
    std::atomic<bool> cancelled_;  // THREAD-SAFE: Use atomic for cancellation flag
    std::chrono::steady_clock::time_point start_time_;
};

// Public API implementation
SnapshotTransferHandler::SnapshotTransferHandler()
    : impl_(std::make_unique<Impl>()) {}

SnapshotTransferHandler::~SnapshotTransferHandler() = default;

SnapshotStatus SnapshotTransferHandler::CreateSnapshot(const SnapshotConfig& config) {
    return impl_->CreateSnapshot(config);
}

SnapshotStatus SnapshotTransferHandler::StreamChunks(ChunkCallback callback) {
    return impl_->StreamChunks(callback);
}

SnapshotStatus SnapshotTransferHandler::VerifySnapshot(const std::string& expected_hash) {
    return impl_->VerifySnapshot(expected_hash);
}

SnapshotStatus SnapshotTransferHandler::ReceiveChunk(
    const themis::sharding::SnapshotChunk& chunk) {
    return impl_->ReceiveChunk(chunk);
}

SnapshotStatus SnapshotTransferHandler::FinalizeSnapshot() {
    return impl_->FinalizeSnapshot();
}

SnapshotProgress SnapshotTransferHandler::GetProgress() const {
    return impl_->GetProgress();
}

void SnapshotTransferHandler::Cancel() {
    impl_->Cancel();
}

void SnapshotTransferHandler::SetDB(rocksdb::DB* db) {
    impl_->SetDB(db);
}

} // namespace rpc
} // namespace themis

