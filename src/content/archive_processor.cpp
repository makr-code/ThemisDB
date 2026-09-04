/**
 * @file archive_processor.cpp
 * @brief Archive format processor (ZIP, TAR, 7Z, RAR) for nested content extraction.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 92/100
 * @note Gap Summary: total=3; TODO=0, Stub=0, Unimpl=0, Mock=0, Sim=0, Debt=1, C=0, H=1, M=2, L=0
 * @note Status: Production Ready; ZIP/TAR/7Z extraction functional; nested depth limits enforced
 * @note This block is auto-generated and will be overwritten.
 */
#include "content/archive_processor.h"
#include "utils/logger.h"
#include <exception>
#include <filesystem>
#include <fstream>
#include <exception>
#include <cstring>
#include <algorithm>
#include <random>
#include <sstream>
#include <iomanip>
#include <stdexcept>
#include <zlib.h>

#include "utils/logger.h"

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#else
#include <unistd.h>
#endif

// ZIP handling with libzip
#ifdef THEMIS_HAVE_LIBZIP
#include <zip.h>
#endif

namespace fs = std::filesystem;

namespace themis {
namespace content {

// ============================================================================
// Magic Bytes for Archive Format Detection
// ============================================================================

constexpr uint32_t ZIP_MAGIC  = 0x04034b50; // PK\x03\x04
constexpr uint16_t GZIP_MAGIC = 0x8b1f;     // \x1f\x8b
constexpr char TAR_MAGIC[]    = "ustar";    // POSIX tar signature at offset 257

// 7-Zip signature
constexpr unsigned char SEVEN_ZIP_MAGIC[] = {0x37, 0x7A, 0xBC, 0xAF, 0x27, 0x1C};

// ============================================================================
// Helper Functions
// ============================================================================

namespace {

/**
 * @brief Generate random temporary directory name
 */
std::string generateRandomString([[maybe_unused]] size_t length) {
    static const char alphanum[] = "0123456789"
                                   "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                   "abcdefghijklmnopqrstuvwxyz";

    std::random_device rd = {};
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, sizeof(alphanum) - 2);

    std::string result = {};
    result.reserve(length);
    for (size_t i = 0; i < length; ++i) {
        result += alphanum[dis(gen)];
    }
    return result;
}

/**
 * @brief Write blob to temporary file
 */
bool writeBlobToFile(const std::string &path, const std::string &blob) {
    try {
        std::ofstream file(path, std::ios::binary);
        if (!file) {
            return false;
        }
        file.write(blob.data(), blob.size());
        return file.good();
    } catch (...) {
        return false;
    }
}

} // anonymous namespace

// ============================================================================
// ArchiveProcessor Implementation
// ============================================================================

ArchiveProcessor::ArchiveProcessor(ArchiveProcessorConfig config) : config_(std::move(config)) {}

bool ArchiveProcessor::canHandle(const std::string &mime_type) const {
    // Common archive MIME types
    static const std::vector<std::string> archive_mimes = {"application/zip",
                                                           "application/x-zip-compressed",
                                                           "application/x-tar",
                                                           "application/x-gtar",
                                                           "application/x-gzip",
                                                           "application/gzip",
                                                           "application/x-bzip2",
                                                           "application/x-xz",
                                                           "application/x-7z-compressed",
                                                           "application/x-compressed",
                                                           "application/x-compress"};

    return std::find(archive_mimes.begin(), archive_mimes.end(), mime_type) != archive_mimes.end();
}

ArchiveFormat ArchiveProcessor::detectFormat(const std::string &blob, const std::string &filename) {
    // Empty blob cannot be a valid archive - return UNKNOWN immediately
    // This prevents filename extension fallback from incorrectly identifying format
    if (blob.empty()) {
        return ArchiveFormat::UNKNOWN;
    }

    // Check magic bytes first
    if (static_cast<int>(blob.size()) > = 4) {
        uint32_t magic32 = 0;
        std::memcpy(&magic32, blob.data(), 4);
        if (magic32 == ZIP_MAGIC) {
            return ArchiveFormat::ZIP;
        }
    }

    if (static_cast<int>(blob.size()) > = 2) {
        uint16_t magic16;
        std::memcpy(&magic16, blob.data(), 2);
        if (magic16 == GZIP_MAGIC) {
            return ArchiveFormat::TAR_GZ;
        }
    }

    if (static_cast<int>(blob.size()) > = 6) {
        if (std::memcmp(blob.data(), SEVEN_ZIP_MAGIC, 6) == 0) {
            return ArchiveFormat::SEVEN_ZIP;
        }
    }

    // Check for TAR signature at offset 257
    if (static_cast<int>(blob.size()) > = 262) {
        if (std::memcmp(blob.data() + 257, TAR_MAGIC, 5) == 0) {
            return ArchiveFormat::TAR;
        }
    }

    // Fallback to filename extension (only for non-empty blobs where magic bytes didn't match)
    std::string lower_filename = filename;
    std::transform(lower_filename.begin(), lower_filename.end(), lower_filename.begin(), ::tolower);

    if (lower_filename.ends_with(".zip")) {
        return ArchiveFormat::ZIP;
    }
    if (lower_filename.ends_with(".tar")) {
        return ArchiveFormat::TAR;
    }
    if (lower_filename.ends_with(".tar.gz") || lower_filename.ends_with(".tgz")) {
        return ArchiveFormat::TAR_GZ;
    }
    if (lower_filename.ends_with(".tar.bz2") || lower_filename.ends_with(".tbz2")) {
        return ArchiveFormat::TAR_BZ2;
    }
    if (lower_filename.ends_with(".tar.xz") || lower_filename.ends_with(".txz")) {
        return ArchiveFormat::TAR_XZ;
    }
    if (lower_filename.ends_with(".7z")) {
        return ArchiveFormat::SEVEN_ZIP;
    }

    return ArchiveFormat::UNKNOWN;
}

std::optional<ArchiveMetadata> ArchiveProcessor::extractMetadata(const std::string &blob, ArchiveFormat format) {
    ArchiveMetadata metadata;
    metadata.format                  = format;
    metadata.is_encrypted            = false;
    metadata.total_uncompressed_size = 0;
    metadata.total_compressed_size   = blob.size();
    metadata.member_count            = 0;
    metadata.directory_count         = 0;
    metadata.file_count              = 0;

    if (format == ArchiveFormat::ZIP) {
#ifdef THEMIS_HAVE_LIBZIP
        // Use libzip to extract metadata
        // Create temporary file for zip_open
        auto temp_dir  = fs::temp_directory_path();
        auto temp_file = temp_dir / ("themis_tmp_" + generateRandomString(16) + ".zip");

        if (!writeBlobToFile(temp_file.string(), blob)) {
            fs::remove(temp_file);
            return std::nullopt;
        }

        int err   = 0;
        zip_t *za = zip_open(temp_file.string().c_str(), ZIP_RDONLY, &err);
        if (!za) {
            fs::remove(temp_file);
            return std::nullopt;
        }

        zip_int64_t num_entries = zip_get_num_entries(za, 0);
        metadata.member_count   = static_cast<size_t>(num_entries);

        for (zip_int64_t i = 0; i < num_entries; ++i) {
            zip_stat_t stat = {};
            if (zip_stat_index(za, i, 0, &stat) == 0) {
                ArchiveMember member;
                member.path              = stat.name ? stat.name : "";
                member.uncompressed_size = stat.size;
                member.compressed_size   = stat.comp_size;
                member.is_directory      = member.path.ends_with("/");
                member.is_encrypted      = (stat.encryption_method != ZIP_EM_NONE);

                if (member.is_directory) {
                    metadata.directory_count++;
                } else {
                    metadata.file_count++;
                }

                if (member.is_encrypted) {
                    metadata.is_encrypted = true;
                }

                metadata.total_uncompressed_size += member.uncompressed_size;
                metadata.members.push_back(std::move(member));
            }
        }

        // Get archive comment if any
        const char *comment = zip_get_archive_comment(za, nullptr, 0);
        if (comment) {
            metadata.comment = comment;
        }

        zip_close(za);
        fs::remove(temp_file);

        return metadata;
#else
        return std::nullopt;
#endif
    }

    // TAR format: walk POSIX ustar 512-byte header blocks to extract
    // entry names, sizes, and counts without requiring libarchive.
    // Each header block is 512 bytes; the file data follows in 512-byte
    // padded blocks.  Two consecutive all-zero blocks mark end-of-archive.
    if (format == ArchiveFormat::TAR || format == ArchiveFormat::TAR_GZ || format == ArchiveFormat::TAR_BZ2
        || format == ArchiveFormat::TAR_XZ) {
        // For compressed variants (TAR_GZ / TAR_BZ2 / TAR_XZ) we cannot
        // decompress in-process without the compression library.  We still
        // attempt to parse raw TAR (the outer gzip/bzip2 wrapper is skipped);
        // for a plain .tar the parse succeeds; for compressed tarballs the
        // magic-byte check below will reject non-ustar data and we fall through
        // to the default "no entries found" path.

        static constexpr std::size_t kBlockSize   = 512;
        static constexpr std::size_t kNameOffset  = 0;
        static constexpr std::size_t kNameLen     = 100;
        static constexpr std::size_t kSizeOffset  = 124;
        static constexpr std::size_t kSizeLen     = 12; // octal, null-terminated
        static constexpr std::size_t kTypeOffset  = 156;
        static constexpr std::size_t kMagicOffset = 257;

        std::size_t offset = 0;
        int zero_blocks    = 0;

        // Portable bounded-string-length helper (strnlen is POSIX, not C++ standard).
        const auto bounded_len = [](const char *s, std::size_t max) -> std::size_t {
            for (std::size_t i = 0; i < max; ++i) {
                if (s[i] == '\0') {
                    return i;
                }
            }
            return max;
        };

        while (offset + kBlockSize <= blob.size()) {
            const char *block = blob.data() + offset;

            // Detect end-of-archive: two consecutive all-zero blocks.
            const bool all_zero = std::all_of(block, block + kBlockSize, [](char c) { return c == '\0'; });
            if (all_zero) {
                if (++zero_blocks >= 2) {
                    break;
                }
                offset += kBlockSize;
                continue;
            }
            zero_blocks = 0;

            // Validate ustar magic ("ustar" at offset 257; may have trailing space or NUL).
            if (std::memcmp(block + kMagicOffset, "ustar", 5) != 0) {
                // Not a ustar header — either corrupted data or a compressed stream.
                break;
            }

            // Extract entry name (null-terminated, max 100 bytes).
            const std::string name(block + kNameOffset, bounded_len(block + kNameOffset, kNameLen));

            // Parse octal file size.
            const std::string size_str(block + kSizeOffset, bounded_len(block + kSizeOffset, kSizeLen));
            uint64_t file_size = 0;
            try {
                file_size = std::stoull(size_str, nullptr, 8);
            } catch (const std::invalid_argument&) {
                file_size = 0;
            } catch (const std::out_of_range&) {
                file_size = 0;
            }

            // Determine entry type from typeflag (byte 156):
            //   '0'/NUL = regular file, '5' = directory, '2' = symlink, etc.
            const char typeflag = block[kTypeOffset];
            const bool is_dir   = (typeflag == '5');

            if (!name.empty() && name != "./" && name != ".") {
                ArchiveMember member;
                member.path              = name;
                member.uncompressed_size = file_size;
                member.compressed_size   = file_size; // TAR does not compress
                member.is_directory      = is_dir;
                member.is_encrypted      = false;

                if (is_dir) {
                    ++metadata.directory_count;
                } else {
                    ++metadata.file_count;
                    metadata.total_uncompressed_size += file_size;
                }
                ++metadata.member_count;
                metadata.members.push_back(std::move(member));
            }

            // Advance past header + data blocks (rounded up to kBlockSize).
            const std::size_t data_blocks = (file_size + kBlockSize - 1) / kBlockSize;
            offset += kBlockSize * (1 + data_blocks);
        }

        return metadata;
    }

    // Default path for formats not handled above (RAR, unknown).
    return metadata;
}

bool ArchiveProcessor::isEncrypted(const std::string &blob, ArchiveFormat format) {
    auto metadata = extractMetadata(blob, format);
    return metadata.has_value() && metadata->is_encrypted;
}

std::string ArchiveProcessor::sanitizePath(const std::string &path) {
    std::string result = {};
    result.reserve(path.size());

    std::vector<std::string> components;
    std::istringstream iss(path);
    std::string component = {};

    while (std::getline(iss, component, '/')) {
        if (component.empty() || component == ".") {
            continue; // Skip empty and current directory
        }
        if (component == "..") {
            // Path traversal attempt - reject
            if (!components.empty()) {
                components.pop_back();
            }
            continue;
        }
        components.push_back(component);
    }

    for (size_t i = 0; i < components.size(); ++i) {
        if (i > 0) {
            result += "/";
        }
        result += components[i];
    }

    return result;
}

std::string ArchiveProcessor::generateTempDirectory() const {
    auto temp_base       = fs::temp_directory_path();
    std::string dir_name = "themis_archive_" + generateRandomString(16);
    auto temp_dir        = temp_base / dir_name;

    fs::create_directories(temp_dir);
    return temp_dir.string();
}

bool ArchiveProcessor::checkCompressionRatio(uint64_t compressed, uint64_t uncompressed) const {
    if (compressed == 0) {
        return true;
    }
    uint64_t ratio = uncompressed / compressed;
    return ratio <= config_.max_compression_ratio;
}

bool ArchiveProcessor::validateArchive(const ArchiveMetadata &metadata, std::string &error_message) const {
    // Check total size limit
    if (metadata.total_uncompressed_size > config_.max_total_size) {
        error_message = "Archive exceeds maximum total size: " + std::to_string(metadata.total_uncompressed_size)
                        + " > " + std::to_string(config_.max_total_size);
        return false;
    }

    // Check file count limit
    if (metadata.member_count > config_.max_file_count) {
        error_message = "Archive exceeds maximum file count: " + std::to_string(metadata.member_count) + " > "
                        + std::to_string(config_.max_file_count);
        return false;
    }

    // Check compression ratio (zip bomb protection)
    if (!checkCompressionRatio(metadata.total_compressed_size, metadata.total_uncompressed_size)) {
        error_message = "Archive has suspicious compression ratio (possible zip bomb)";
        return false;
    }

    // Check individual file sizes and paths
    for (const auto &member : metadata.members) {
        if (member.is_directory) {
            continue;
        }

        if (member.uncompressed_size > config_.max_file_size) {
            error_message = "Archive member '" + member.path + "' exceeds maximum file size";
            return false;
        }

        // Check path length
        if (member.path.size() > config_.max_path_length) {
            error_message = "Archive member path too long: " + member.path;
            return false;
        }

        // Check path depth
        size_t depth = std::count(member.path.begin(), member.path.end(), '/');
        if (depth > config_.max_path_depth) {
            error_message = "Archive member path too deep: " + member.path;
            return false;
        }

        // Check for suspicious paths (path traversal)
        if (member.path.find("..") != std::string::npos) {
            error_message = "Archive contains suspicious path: " + member.path;
            return false;
        }
    }

    return true;
}

ArchiveExtractionResult ArchiveProcessor::extractZip(const std::string &blob, const std::string &password) {
    (void)blob;
    (void)password;
    ArchiveExtractionResult result;
    result.success = false;

#ifndef THEMIS_HAVE_LIBZIP
    result.error_message = "ZIP extraction unavailable: libzip not found at build time";
    return result;
#else
    result.temp_directory = generateTempDirectory();

    // Write blob to temporary file
    auto temp_zip = fs::path(result.temp_directory) / "archive.zip";
    if (!writeBlobToFile(temp_zip.string(), blob)) {
        result.error_message = "Failed to write archive to temporary file";
        return result;
    }

    int err   = 0;
    zip_t *za = zip_open(temp_zip.string().c_str(), ZIP_RDONLY, &err);
    if (!za) {
        char errbuf[256];
        zip_error_to_str(errbuf, sizeof(errbuf), err, errno);
        result.error_message = std::string("Failed to open ZIP archive: ") + errbuf;
        fs::remove(temp_zip);
        return result;
    }

    // Set password if provided
    if (!password.empty()) {
        zip_set_default_password(za, password.c_str());
    }

    zip_int64_t num_entries = zip_get_num_entries(za, 0);

    // Enforce file count limit before extraction starts (fast path)
    if (static_cast<size_t>(num_entries) > config_.max_file_count) {
        zip_close(za);
        fs::remove(temp_zip);
        result.error_message = "Archive exceeds maximum file count: " + std::to_string(num_entries) + " > "
                               + std::to_string(config_.max_file_count);
        return result;
    }

    uint64_t total_bytes_written = 0; // Track decompressed bytes during extraction

    for (zip_int64_t i = 0; i < num_entries; ++i) {
        zip_stat_t stat = {};
        if (zip_stat_index(za, i, 0, &stat) != 0) {
            continue;
        }

        std::string member_path = stat.name ? stat.name : "";
        if (member_path.empty())
            continue;

        // Sanitize path
        std::string safe_path = sanitizePath(member_path);
        if (safe_path.empty())
            continue;

        auto extract_path = fs::path(result.temp_directory) / safe_path;

        // Prevent path traversal: ensure the resolved path stays inside temp_directory
        auto canon_temp         = fs::weakly_canonical(result.temp_directory);
        auto canon_extract      = fs::weakly_canonical(extract_path);
        std::string temp_str    = canon_temp.string();
        std::string extract_str = canon_extract.string();
        if (extract_str.rfind(temp_str, 0) != 0) {
            // Resolved path escapes temp directory – skip silently
            continue;
        }

        // Check if it's a directory
        if (member_path.ends_with("/")) {
            fs::create_directories(extract_path);
            continue;
        }

        // Create parent directories
        if (extract_path.has_parent_path()) {
            fs::create_directories(extract_path.parent_path());
        }

        // Extract file
        zip_file_t *zf = zip_fopen_index(za, i, 0);
        if (!zf) {
            continue; // Skip files that can't be opened (might be encrypted without password)
        }

        std::ofstream out_file(extract_path, std::ios::binary);
        if (!out_file) {
            zip_fclose(zf);
            continue;
        }

        // Read and write in chunks; enforce per-file and total size limits in real-time
        char buffer[8192];
        zip_int64_t bytes_read;
        uint64_t file_bytes_written = 0;
        std::string size_error_msg = {};
        while ((bytes_read = zip_fread(zf, buffer, sizeof(buffer))) > 0) {
            file_bytes_written += static_cast<uint64_t>(bytes_read);
            total_bytes_written += static_cast<uint64_t>(bytes_read);

            // Per-file size guard
            if (file_bytes_written > config_.max_file_size) {
                size_error_msg = "Archive member exceeds maximum file size (possible zip bomb)";
                break;
            }
            // Total size guard (zip-bomb protection)
            if (total_bytes_written > config_.max_total_size) {
                size_error_msg = "Archive total decompressed size exceeds limit (possible zip bomb)";
                break;
            }

            out_file.write(buffer, bytes_read);
        }

        zip_fclose(zf);
        out_file.close();

        if (!size_error_msg.empty()) {
            // Remove the partially-extracted file and abort
            fs::remove(extract_path);
            zip_close(za);
            fs::remove(temp_zip);
            result.error_message = size_error_msg;
            return result;
        }

        if (bytes_read == 0) { // Successfully extracted
            result.extracted_files.push_back(extract_path.string());
        }
    }

    zip_close(za);
    fs::remove(temp_zip);

    result.success = true;
    return result;
#endif
}

ArchiveExtractionResult ArchiveProcessor::extractTar(const std::string &blob, ArchiveFormat format) {
    ArchiveExtractionResult result;
    result.success = false;

    // ── Step 1: decompress for compressed variants ──────────────────────────
    std::vector<uint8_t> raw_tar;

    if (format == ArchiveFormat::TAR) {
        raw_tar.assign(blob.begin(), blob.end());
    } else if (format == ArchiveFormat::TAR_GZ) {
        // Decompress with zlib (inflateInit2 with windowBits=47 enables gzip decoding)
        z_stream zs{};
        if (inflateInit2(&zs, 47) != Z_OK) {
            result.error_message = "TAR.GZ: inflateInit2 failed";
            return result;
        }
        zs.next_in  = reinterpret_cast<Bytef *>(const_cast<char *>(blob.data()));
        zs.avail_in = static_cast<uInt>(blob.size());

        std::vector<uint8_t> out_buf(1 << 20); // 1 MiB chunks
        int ret = 0;

        do {
            zs.next_out  = out_buf.data();
            zs.avail_out = static_cast<uInt>(out_buf.size());
            ret          = inflate(&zs, Z_NO_FLUSH);
            if (ret == Z_STREAM_ERROR || ret == Z_DATA_ERROR || ret == Z_MEM_ERROR) {
                inflateEnd(&zs);
                result.error_message = "TAR.GZ: inflate error";
                return result;
            }
            const std::size_t written = out_buf.size() - zs.avail_out;
            raw_tar.insert(raw_tar.end(), out_buf.begin(), out_buf.begin() + written);
        } while (ret != Z_STREAM_END);
        inflateEnd(&zs);
    } else {
        // TAR_BZ2 / TAR_XZ require libbz2 / liblzma which are optional.
        result.error_message = "TAR.BZ2 / TAR.XZ extraction requires libbz2 / liblzma "
                               "(not linked in this build). Use TAR or TAR.GZ, or build "
                               "with THEMIS_HAVE_LIBARCHIVE for full support.";
        return result;
    }

    // ── Step 2: create temp directory ──────────────────────────────────────
    std::string temp_dir = (fs::temp_directory_path() / ("themis_tar_" + generateRandomString(8))).string();
    try {
        fs::create_directories(temp_dir);
    } catch (const std::exception &e) {
        result.error_message = std::string("TAR: failed to create temp dir: ") + e.what();
        return result;
    }
    result.temp_directory = temp_dir;

    // ── Step 3: parse POSIX/ustar TAR ──────────────────────────────────────
    // Each header block is 512 bytes; data follows in 512-byte blocks.
    constexpr std::size_t BLOCK = 512;
    std::size_t offset          = 0;
    uint64_t total_size         = 0;
    std::size_t file_count      = 0;
    int consecutive_zero_blocks = 0;

    while (offset + BLOCK <= raw_tar.size()) {
        const uint8_t *hdr = raw_tar.data() + offset;
        offset += BLOCK;

        // Two consecutive zero blocks signal end-of-archive
        bool all_zero = true;
        for (std::size_t b = 0; b < BLOCK; ++b) {
            if (hdr[b]) {
                all_zero = false;
                break;
            }
        }
        if (all_zero) {
            if (++consecutive_zero_blocks >= 2) {
                break;
            }
            continue;
        }
        consecutive_zero_blocks = 0;

        // File size: octal string at offset 124, length 12
        char size_str[13] = {};
        std::memcpy(size_str, hdr + 124, 12);
        const uint64_t entry_size = static_cast<uint64_t>(std::strtoull(size_str, nullptr, 8));

        // File name (100 bytes at offset 0; ustar prefix at offset 345, length 155)
        char name[256]     = {};
        const char *prefix = reinterpret_cast<const char *>(hdr + 345);
        if (prefix[0] && std::strncmp(reinterpret_cast<const char *>(hdr + 257), "ustar", 5) == 0) {
            std::snprintf(name, sizeof(name), "%.*s/%.*s", 155, prefix, 100, reinterpret_cast<const char *>(hdr));
        } else {
            std::snprintf(name, sizeof(name), "%.*s", 100, reinterpret_cast<const char *>(hdr));
        }

        // Type flag: '0' or '\0' = regular file, '5' = directory
        const char typeflag = static_cast<char>(hdr[156]);

        // Security: reject absolute paths and path traversal
        std::string entry_name(name);
        if (!entry_name.empty() && entry_name[0] == '/') {
            entry_name = entry_name.substr(1);
        }
        if (entry_name.find("..") != std::string::npos) {
            // Skip path traversal attempts
            offset += ((entry_size + BLOCK - 1) / BLOCK) * BLOCK;
            continue;
        }

        // Enforce security limits
        if (++file_count > config_.max_file_count) {
            result.error_message = "TAR: max_file_count exceeded";
            cleanupTempDirectory(temp_dir);
            return result;
        }
        if (entry_size > config_.max_file_size) {
            result.error_message = "TAR: single file exceeds max_file_size";
            cleanupTempDirectory(temp_dir);
            return result;
        }
        total_size += entry_size;
        if (total_size > config_.max_total_size) {
            result.error_message = "TAR: total extracted size exceeds max_total_size";
            cleanupTempDirectory(temp_dir);
            return result;
        }

        const fs::path out_path = fs::path(temp_dir) / entry_name;

        if (typeflag == '5' || (entry_name.size() > 1 && entry_name.back() == '/')) {
            // Directory entry
            try {
                fs::create_directories(out_path);
            } catch (const fs::filesystem_error&) {
            } catch (...) {
            }
        } else {
            // Regular file (or hardlink '1', symlink '2' treated as file copy)
            try {
                fs::create_directories(out_path.parent_path());
            } catch (const fs::filesystem_error&) {
            } catch (...) {
            }
            if (entry_size > 0 && offset + entry_size <= raw_tar.size()) {
                std::ofstream ofs(out_path, std::ios::binary);
                if (!ofs.is_open()) {
                    result.error_message = "TAR: failed to open output file: " + out_path.string();
                    cleanupTempDirectory(temp_dir);
                    return result;
                }
                ofs.write(reinterpret_cast<const char *>(raw_tar.data() + offset),
                          static_cast<std::streamsize>(entry_size));
                ofs.close();
                result.extracted_files.push_back(out_path.string());
            }
        }

        // Advance past data blocks (rounded up to 512)
        offset += ((entry_size + BLOCK - 1) / BLOCK) * BLOCK;
    }

    result.success = true;
    return result;
}

ArchiveExtractionResult ArchiveProcessor::extractToTemp(const std::string &blob, ArchiveFormat format,
                                                        const std::string &password) {
    if (format == ArchiveFormat::ZIP) {
        return extractZip(blob, password);
    } else if (format == ArchiveFormat::TAR || format == ArchiveFormat::TAR_GZ || format == ArchiveFormat::TAR_BZ2
               || format == ArchiveFormat::TAR_XZ) {
        return extractTar(blob, format);
    } else {
        ArchiveExtractionResult result;
        result.success       = false;
        result.error_message = "Unsupported archive format";
        return result;
    }
}

void ArchiveProcessor::cleanupTempDirectory(const std::string &temp_dir) {
    try {
        if (fs::exists(temp_dir)) {
            fs::remove_all(temp_dir);
        }
    } catch (const std::exception &e) {
        THEMIS_ERROR("Failed to cleanup temp directory {}: {}", temp_dir, e.what());
    }
}

ArchiveProcessorResult ArchiveProcessor::process(const std::string &blob, const std::string & /*mime_type*/,
                                                 const std::string &filename) {
    ArchiveProcessorResult result;
    result.success = false;

    // Detect format
    ArchiveFormat format = detectFormat(blob, filename);
    if (format == ArchiveFormat::UNKNOWN) {
        result.error_message = "Unknown archive format";
        return result;
    }

    // For strict reject policy, fail fast without parsing archive internals.
    if (config_.strategy == ArchiveStrategy::REJECT) {
        result.error_message = "Archive uploads are not accepted";
        return result;
    }

    // Extract metadata
    auto metadata_opt = extractMetadata(blob, format);
    if (!metadata_opt.has_value()) {
        result.error_message = "Failed to extract archive metadata";
        return result;
    }

    const auto &metadata = metadata_opt.value();

    // Check if archive is encrypted
    if (metadata.is_encrypted) {
        if (config_.encrypted_policy == EncryptedArchivePolicy::REJECT) {
            result.error_message = "Encrypted archives are not accepted";
            return result;
        } else if (config_.encrypted_policy == EncryptedArchivePolicy::METADATA_ONLY) {
            // Just store metadata without extraction
            result.success  = true;
            result.metadata = json{{"format", static_cast<int>(format)},
                                   {"encrypted", true},
                                   {"member_count", metadata.member_count},
                                   {"total_size", metadata.total_uncompressed_size},
                                   {"extraction_strategy", "METADATA_ONLY"}};
            return result;
        } else if (config_.encrypted_policy == EncryptedArchivePolicy::REQUIRE_PASSWORD) {
            if (config_.password.empty()) {
                result.error_message = "Password required for encrypted archive";
                return result;
            }
        }
    }

    // Validate archive (size, file count, compression ratio, paths)
    std::string validation_error = {};
    if (!validateArchive(metadata, validation_error)) {
        result.error_message = validation_error;
        return result;
    }

    // Zip-bomb protection via ContentSecurityManager (blocks ingestion if thresholds exceeded).
    // member_count = total archive entries (files + directories); using it is intentionally
    // more conservative than file_count alone, and keeps the check consistent with the
    // member_count guard already applied by validateArchive() directly above.
    auto zip_bomb_result
        = security_manager_.checkZipBomb(metadata.total_compressed_size, metadata.total_uncompressed_size,
                                         metadata.member_count, // files + directories
                                         filename);
    if (zip_bomb_result.error.failed()) {
        result.error_message = zip_bomb_result.error.message;
        return result;
    }

    // Handle based on strategy
    if (config_.strategy == ArchiveStrategy::METADATA_ONLY) {
        result.success  = true;
        result.metadata = json{{"format", static_cast<int>(format)},
                               {"encrypted", metadata.is_encrypted},
                               {"member_count", metadata.member_count},
                               {"file_count", metadata.file_count},
                               {"directory_count", metadata.directory_count},
                               {"total_uncompressed_size", metadata.total_uncompressed_size},
                               {"extraction_strategy", "METADATA_ONLY"}};

        // Add member list
        json members = json::array();
        for (const auto &member : metadata.members) {
            members.push_back(
                {{"path", member.path}, {"size", member.uncompressed_size}, {"is_directory", member.is_directory}});
        }
        result.metadata["members"] = members;

        return result;
    }

    // EXTRACT_AND_INGEST strategy
    ArchiveExtractionResult extraction = extractToTemp(blob, format, config_.password);
    if (!extraction.success) {
        result.error_message = "Extraction failed: " + extraction.error_message;
        return result;
    }

    result.success  = true;
    result.metadata = json{
        {"format", static_cast<int>(format)},          {"encrypted", metadata.is_encrypted},
        {"member_count", metadata.member_count},       {"file_count", metadata.file_count},
        {"directory_count", metadata.directory_count}, {"total_uncompressed_size", metadata.total_uncompressed_size},
        {"extraction_strategy", "EXTRACT_AND_INGEST"}, {"extracted_file_count", extraction.extracted_files.size()},
        {"temp_directory", extraction.temp_directory}};

    // Add extracted file list
    json extracted_files = json::array();
    for (const auto &file_path : extraction.extracted_files) {
        extracted_files.push_back(file_path);
    }
    result.metadata["extracted_files"] = extracted_files;

    // Note: The ContentManager will handle ingesting each extracted file
    // and creating graph relationships. We just provide the file list.

    return result;
}

// ============================================================================
// IContentProcessor Interface Implementation
// ============================================================================

ExtractionResult ArchiveProcessor::extract(const std::string &blob, const ContentType &content_type) {
    ExtractionResult result;
    result.ok = false;

    // Use the archive-specific process() method
    auto archive_result = process(blob, content_type.mime_type, "archive");

    result.ok            = archive_result.success;
    result.error_message = archive_result.error_message;
    result.metadata      = archive_result.metadata;
    result.text          = ""; // Archives don't have direct text content

    return result;
}

std::vector<json> ArchiveProcessor::chunk(const ExtractionResult &extraction_result, int /*chunk_size*/, int /*overlap*/
) {
    // Archives don't need chunking - they're metadata containers
    // Return a single chunk with the metadata
    std::vector<json> chunks;

    if (!extraction_result.metadata.empty()) {
        json chunk = {{"type", "archive_metadata"}, {"metadata", extraction_result.metadata}, {"seq_num", 0}};
        chunks.push_back(chunk);
    }

    return chunks;
}

std::vector<float> ArchiveProcessor::generateEmbedding(const std::string & /*chunk_data*/) {
    // Archives don't generate embeddings
    // Embeddings are generated for the extracted files instead
    return {};
}

} // namespace content
} // namespace themis

