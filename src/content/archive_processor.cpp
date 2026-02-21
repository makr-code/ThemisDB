/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            archive_processor.cpp                              ║
  Version:         0.0.9                                              ║
  Last Modified:   2026-02-21 13:48:46                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   81.0/100                                       ║
    • Total Lines:     667                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file archive_processor.cpp
 * @brief Archive Content Processor Implementation
 * 
 * Handles compressed archives (.zip, .tar, .tar.gz, etc.) with configurable
 * extraction strategies and security protections.
 * 
 * @author ThemisDB Team
 * @date January 2026
 */

#include "content/archive_processor.h"
#include "utils/logger.h"
#include <filesystem>
#include <fstream>
#include <cstring>
#include <algorithm>
#include <random>
#include <sstream>
#include <iomanip>

#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#else
#include <unistd.h>
#endif

// ZIP handling with libzip
#include <zip.h>

namespace fs = std::filesystem;

namespace themis {
namespace content {

// ============================================================================
// Magic Bytes for Archive Format Detection
// ============================================================================

constexpr uint32_t ZIP_MAGIC = 0x04034b50;  // PK\x03\x04
constexpr uint16_t GZIP_MAGIC = 0x8b1f;     // \x1f\x8b
constexpr char TAR_MAGIC[] = "ustar";       // POSIX tar signature at offset 257

// 7-Zip signature
constexpr unsigned char SEVEN_ZIP_MAGIC[] = {0x37, 0x7A, 0xBC, 0xAF, 0x27, 0x1C};

// ============================================================================
// Helper Functions
// ============================================================================

namespace {

/**
 * @brief Generate random temporary directory name
 */
std::string generateRandomString(size_t length) {
    static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, sizeof(alphanum) - 2);
    
    std::string result;
    result.reserve(length);
    for (size_t i = 0; i < length; ++i) {
        result += alphanum[dis(gen)];
    }
    return result;
}

/**
 * @brief Write blob to temporary file
 */
bool writeBlobToFile(const std::string& path, const std::string& blob) {
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

ArchiveProcessor::ArchiveProcessor(ArchiveProcessorConfig config)
    : config_(std::move(config))
{
}

bool ArchiveProcessor::canHandle(const std::string& mime_type) const {
    // Common archive MIME types
    static const std::vector<std::string> archive_mimes = {
        "application/zip",
        "application/x-zip-compressed",
        "application/x-tar",
        "application/x-gtar",
        "application/x-gzip",
        "application/gzip",
        "application/x-bzip2",
        "application/x-xz",
        "application/x-7z-compressed",
        "application/x-compressed",
        "application/x-compress"
    };
    
    return std::find(archive_mimes.begin(), archive_mimes.end(), mime_type) != archive_mimes.end();
}

ArchiveFormat ArchiveProcessor::detectFormat(const std::string& blob, const std::string& filename) {
    // Check magic bytes first
    if (blob.size() >= 4) {
        uint32_t magic32;
        std::memcpy(&magic32, blob.data(), 4);
        if (magic32 == ZIP_MAGIC) {
            return ArchiveFormat::ZIP;
        }
    }
    
    if (blob.size() >= 2) {
        uint16_t magic16;
        std::memcpy(&magic16, blob.data(), 2);
        if (magic16 == GZIP_MAGIC) {
            return ArchiveFormat::TAR_GZ;
        }
    }
    
    if (blob.size() >= 6) {
        if (std::memcmp(blob.data(), SEVEN_ZIP_MAGIC, 6) == 0) {
            return ArchiveFormat::SEVEN_ZIP;
        }
    }
    
    // Check for TAR signature at offset 257
    if (blob.size() >= 262) {
        if (std::memcmp(blob.data() + 257, TAR_MAGIC, 5) == 0) {
            return ArchiveFormat::TAR;
        }
    }
    
    // Fallback to filename extension
    std::string lower_filename = filename;
    std::transform(lower_filename.begin(), lower_filename.end(), lower_filename.begin(), ::tolower);
    
    if (lower_filename.ends_with(".zip")) return ArchiveFormat::ZIP;
    if (lower_filename.ends_with(".tar")) return ArchiveFormat::TAR;
    if (lower_filename.ends_with(".tar.gz") || lower_filename.ends_with(".tgz")) return ArchiveFormat::TAR_GZ;
    if (lower_filename.ends_with(".tar.bz2") || lower_filename.ends_with(".tbz2")) return ArchiveFormat::TAR_BZ2;
    if (lower_filename.ends_with(".tar.xz") || lower_filename.ends_with(".txz")) return ArchiveFormat::TAR_XZ;
    if (lower_filename.ends_with(".7z")) return ArchiveFormat::SEVEN_ZIP;
    
    return ArchiveFormat::UNKNOWN;
}

std::optional<ArchiveMetadata> ArchiveProcessor::extractMetadata(
    const std::string& blob,
    ArchiveFormat format
) {
    ArchiveMetadata metadata;
    metadata.format = format;
    metadata.is_encrypted = false;
    metadata.total_uncompressed_size = 0;
    metadata.total_compressed_size = blob.size();
    metadata.member_count = 0;
    metadata.directory_count = 0;
    metadata.file_count = 0;
    
    if (format == ArchiveFormat::ZIP) {
        // Use libzip to extract metadata
        // Create temporary file for zip_open
        auto temp_dir = fs::temp_directory_path();
        auto temp_file = temp_dir / ("themis_tmp_" + generateRandomString(16) + ".zip");
        
        if (!writeBlobToFile(temp_file.string(), blob)) {
            fs::remove(temp_file);
            return std::nullopt;
        }
        
        int err = 0;
        zip_t* za = zip_open(temp_file.string().c_str(), ZIP_RDONLY, &err);
        if (!za) {
            fs::remove(temp_file);
            return std::nullopt;
        }
        
        zip_int64_t num_entries = zip_get_num_entries(za, 0);
        metadata.member_count = static_cast<size_t>(num_entries);
        
        for (zip_int64_t i = 0; i < num_entries; ++i) {
            zip_stat_t stat;
            if (zip_stat_index(za, i, 0, &stat) == 0) {
                ArchiveMember member;
                member.path = stat.name ? stat.name : "";
                member.uncompressed_size = stat.size;
                member.compressed_size = stat.comp_size;
                member.is_directory = member.path.ends_with("/");
                member.is_encrypted = (stat.encryption_method != ZIP_EM_NONE);
                
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
        const char* comment = zip_get_archive_comment(za, nullptr, 0);
        if (comment) {
            metadata.comment = comment;
        }
        
        zip_close(za);
        fs::remove(temp_file);
        
        return metadata;
    }
    
    // For TAR and other formats, we would need additional libraries or manual parsing
    // For now, return basic metadata
    return metadata;
}

bool ArchiveProcessor::isEncrypted(const std::string& blob, ArchiveFormat format) {
    auto metadata = extractMetadata(blob, format);
    return metadata.has_value() && metadata->is_encrypted;
}

std::string ArchiveProcessor::sanitizePath(const std::string& path) {
    std::string result;
    result.reserve(path.size());
    
    std::vector<std::string> components;
    std::istringstream iss(path);
    std::string component;
    
    while (std::getline(iss, component, '/')) {
        if (component.empty() || component == ".") {
            continue;  // Skip empty and current directory
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
        if (i > 0) result += "/";
        result += components[i];
    }
    
    return result;
}

std::string ArchiveProcessor::generateTempDirectory() const {
    auto temp_base = fs::temp_directory_path();
    std::string dir_name = "themis_archive_" + generateRandomString(16);
    auto temp_dir = temp_base / dir_name;
    
    fs::create_directories(temp_dir);
    return temp_dir.string();
}

bool ArchiveProcessor::checkCompressionRatio(uint64_t compressed, uint64_t uncompressed) const {
    if (compressed == 0) return true;
    uint64_t ratio = uncompressed / compressed;
    return ratio <= config_.max_compression_ratio;
}

bool ArchiveProcessor::validateArchive(const ArchiveMetadata& metadata, std::string& error_message) const {
    // Check total size limit
    if (metadata.total_uncompressed_size > config_.max_total_size) {
        error_message = "Archive exceeds maximum total size: " + 
                       std::to_string(metadata.total_uncompressed_size) + " > " +
                       std::to_string(config_.max_total_size);
        return false;
    }
    
    // Check file count limit
    if (metadata.member_count > config_.max_file_count) {
        error_message = "Archive exceeds maximum file count: " + 
                       std::to_string(metadata.member_count) + " > " +
                       std::to_string(config_.max_file_count);
        return false;
    }
    
    // Check compression ratio (zip bomb protection)
    if (!checkCompressionRatio(metadata.total_compressed_size, metadata.total_uncompressed_size)) {
        error_message = "Archive has suspicious compression ratio (possible zip bomb)";
        return false;
    }
    
    // Check individual file sizes and paths
    for (const auto& member : metadata.members) {
        if (member.is_directory) continue;
        
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

ArchiveExtractionResult ArchiveProcessor::extractZip(const std::string& blob, const std::string& password) {
    ArchiveExtractionResult result;
    result.success = false;
    result.temp_directory = generateTempDirectory();
    
    // Write blob to temporary file
    auto temp_zip = fs::path(result.temp_directory) / "archive.zip";
    if (!writeBlobToFile(temp_zip.string(), blob)) {
        result.error_message = "Failed to write archive to temporary file";
        return result;
    }
    
    int err = 0;
    zip_t* za = zip_open(temp_zip.string().c_str(), ZIP_RDONLY, &err);
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
    
    for (zip_int64_t i = 0; i < num_entries; ++i) {
        zip_stat_t stat;
        if (zip_stat_index(za, i, 0, &stat) != 0) {
            continue;
        }
        
        std::string member_path = stat.name ? stat.name : "";
        if (member_path.empty()) continue;
        
        // Sanitize path
        std::string safe_path = sanitizePath(member_path);
        if (safe_path.empty()) continue;
        
        auto extract_path = fs::path(result.temp_directory) / safe_path;
        
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
        zip_file_t* zf = zip_fopen_index(za, i, 0);
        if (!zf) {
            continue;  // Skip files that can't be opened (might be encrypted without password)
        }
        
        std::ofstream out_file(extract_path, std::ios::binary);
        if (!out_file) {
            zip_fclose(zf);
            continue;
        }
        
        // Read and write in chunks
        char buffer[8192];
        zip_int64_t bytes_read;
        while ((bytes_read = zip_fread(zf, buffer, sizeof(buffer))) > 0) {
            out_file.write(buffer, bytes_read);
        }
        
        zip_fclose(zf);
        out_file.close();
        
        if (bytes_read == 0) {  // Successfully extracted
            result.extracted_files.push_back(extract_path.string());
        }
    }
    
    zip_close(za);
    fs::remove(temp_zip);
    
    result.success = true;
    return result;
}

ArchiveExtractionResult ArchiveProcessor::extractTar(const std::string& blob, ArchiveFormat format) {
    ArchiveExtractionResult result;
    result.success = false;
    result.error_message = "TAR extraction not yet implemented (requires libarchive)";
    return result;
}

ArchiveExtractionResult ArchiveProcessor::extractToTemp(
    const std::string& blob,
    ArchiveFormat format,
    const std::string& password
) {
    if (format == ArchiveFormat::ZIP) {
        return extractZip(blob, password);
    } else if (format == ArchiveFormat::TAR || 
               format == ArchiveFormat::TAR_GZ || 
               format == ArchiveFormat::TAR_BZ2 || 
               format == ArchiveFormat::TAR_XZ) {
        return extractTar(blob, format);
    } else {
        ArchiveExtractionResult result;
        result.success = false;
        result.error_message = "Unsupported archive format";
        return result;
    }
}

void ArchiveProcessor::cleanupTempDirectory(const std::string& temp_dir) {
    try {
        if (fs::exists(temp_dir)) {
            fs::remove_all(temp_dir);
        }
    } catch (const std::exception& e) {
        THEMIS_ERROR("Failed to cleanup temp directory {}: {}", temp_dir, e.what());
    }
}

ArchiveProcessorResult ArchiveProcessor::process(
    const std::string& blob,
    const std::string& mime_type,
    const std::string& filename
) {
    ArchiveProcessorResult result;
    result.success = false;
    
    // Detect format
    ArchiveFormat format = detectFormat(blob, filename);
    if (format == ArchiveFormat::UNKNOWN) {
        result.error_message = "Unknown archive format";
        return result;
    }
    
    // Extract metadata
    auto metadata_opt = extractMetadata(blob, format);
    if (!metadata_opt.has_value()) {
        result.error_message = "Failed to extract archive metadata";
        return result;
    }
    
    const auto& metadata = metadata_opt.value();
    
    // Check if archive is encrypted
    if (metadata.is_encrypted) {
        if (config_.encrypted_policy == EncryptedArchivePolicy::REJECT) {
            result.error_message = "Encrypted archives are not accepted";
            return result;
        } else if (config_.encrypted_policy == EncryptedArchivePolicy::METADATA_ONLY) {
            // Just store metadata without extraction
            result.success = true;
            result.metadata = json{
                {"format", static_cast<int>(format)},
                {"encrypted", true},
                {"member_count", metadata.member_count},
                {"total_size", metadata.total_uncompressed_size},
                {"extraction_strategy", "METADATA_ONLY"}
            };
            return result;
        } else if (config_.encrypted_policy == EncryptedArchivePolicy::REQUIRE_PASSWORD) {
            if (config_.password.empty()) {
                result.error_message = "Password required for encrypted archive";
                return result;
            }
        }
    }
    
    // Validate archive
    std::string validation_error;
    if (!validateArchive(metadata, validation_error)) {
        result.error_message = validation_error;
        return result;
    }
    
    // Handle based on strategy
    if (config_.strategy == ArchiveStrategy::REJECT) {
        result.error_message = "Archive uploads are not accepted";
        return result;
    } else if (config_.strategy == ArchiveStrategy::METADATA_ONLY) {
        result.success = true;
        result.metadata = json{
            {"format", static_cast<int>(format)},
            {"encrypted", metadata.is_encrypted},
            {"member_count", metadata.member_count},
            {"file_count", metadata.file_count},
            {"directory_count", metadata.directory_count},
            {"total_uncompressed_size", metadata.total_uncompressed_size},
            {"extraction_strategy", "METADATA_ONLY"}
        };
        
        // Add member list
        json members = json::array();
        for (const auto& member : metadata.members) {
            members.push_back({
                {"path", member.path},
                {"size", member.uncompressed_size},
                {"is_directory", member.is_directory}
            });
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
    
    result.success = true;
    result.metadata = json{
        {"format", static_cast<int>(format)},
        {"encrypted", metadata.is_encrypted},
        {"member_count", metadata.member_count},
        {"file_count", metadata.file_count},
        {"directory_count", metadata.directory_count},
        {"total_uncompressed_size", metadata.total_uncompressed_size},
        {"extraction_strategy", "EXTRACT_AND_INGEST"},
        {"extracted_file_count", extraction.extracted_files.size()},
        {"temp_directory", extraction.temp_directory}
    };
    
    // Add extracted file list
    json extracted_files = json::array();
    for (const auto& file_path : extraction.extracted_files) {
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

ExtractionResult ArchiveProcessor::extract(
    const std::string& blob,
    const ContentType& content_type
) {
    ExtractionResult result;
    result.ok = false;
    
    // Use the archive-specific process() method
    auto archive_result = process(blob, content_type.mime_type, "archive");
    
    result.ok = archive_result.success;
    result.error_message = archive_result.error_message;
    result.metadata = archive_result.metadata;
    result.text = ""; // Archives don't have direct text content
    
    return result;
}

std::vector<json> ArchiveProcessor::chunk(
    const ExtractionResult& extraction_result,
    int chunk_size,
    int overlap
) {
    // Archives don't need chunking - they're metadata containers
    // Return a single chunk with the metadata
    std::vector<json> chunks;
    
    if (!extraction_result.metadata.empty()) {
        json chunk = {
            {"type", "archive_metadata"},
            {"metadata", extraction_result.metadata},
            {"seq_num", 0}
        };
        chunks.push_back(chunk);
    }
    
    return chunks;
}

std::vector<float> ArchiveProcessor::generateEmbedding(const std::string& chunk_data) {
    // Archives don't generate embeddings
    // Embeddings are generated for the extracted files instead
    return {};
}

} // namespace content
} // namespace themis
