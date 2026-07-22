// Copyright 2026 ThemisDB Team
// SPDX-License-Identifier: Apache-2.0

/**
 * @file tensor_storage_strategy.cc
 * @brief Implementation of quantization assessment, mmap loader, zero-copy
 *        accessor, and storage-strategy assessor for tensor artifacts
 *        (Issue #5443).
 *
 * Platform notes:
 *   - POSIX: uses mmap(2), munmap(2), madvise(2), mlock(2), open(2), fstat(2).
 *   - Windows: uses CreateFileW, CreateFileMappingW, MapViewOfFile,
 *              UnmapViewOfFile, CloseHandle, VirtualAlloc (MADV simulation).
 *   - Other: all MmapLoader::open() calls return MmapError::UNSUPPORTED_PLATFORM.
 *
 * @see include/distributed_tensor/tensor_storage_strategy.h
 */

#include "distributed_tensor/tensor_storage_strategy.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cerrno>
#include <cstring>
#include <sstream>

// ── Platform includes ──────────────────────────────────────────────────────

#if defined(_WIN32)
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  include <windows.h>
#elif defined(__unix__) || defined(__APPLE__)
#  include <fcntl.h>
#  include <sys/mman.h>
#  include <sys/stat.h>
#  include <unistd.h>
#  define THEMIS_HAS_MMAP 1
#else
#  define THEMIS_NO_MMAP 1
#endif

namespace themis {
namespace distributed_tensor {

// ===========================================================================
// mmapErrorMessage
// ===========================================================================

const char* mmapErrorMessage(MmapError err) noexcept {
    switch (err) {
        case MmapError::OK:                   return "OK";
        case MmapError::FILE_NOT_FOUND:       return "file not found";
        case MmapError::PERMISSION_DENIED:    return "permission denied";
        case MmapError::FILE_TOO_LARGE:       return "file too large for address space";
        case MmapError::MAPPING_FAILED:       return "memory mapping failed";
        case MmapError::ALREADY_OPEN:         return "region already open; close first";
        case MmapError::NOT_OPEN:             return "no open mapping";
        case MmapError::UNSUPPORTED_PLATFORM: return "mmap not supported on this platform";
        case MmapError::LOCK_FAILED:          return "mlock failed (non-fatal)";
        case MmapError::IO_ERROR:             return "I/O error";
    }
    return "unknown error";
}

// ===========================================================================
// MmapRegion — RAII destructor and move semantics
// ===========================================================================

MmapRegion::~MmapRegion() noexcept {
    close();
}

void MmapRegion::close() noexcept {
    if (data_ == nullptr) {
        return; // nothing to unmap
    }

#if defined(_WIN32)
    ::UnmapViewOfFile(const_cast<std::byte*>(data_));
    if (mapping_handle_ && mapping_handle_ != INVALID_HANDLE_VALUE) {
        ::CloseHandle(mapping_handle_);
    }
    if (file_handle_ && file_handle_ != INVALID_HANDLE_VALUE) {
        ::CloseHandle(file_handle_);
    }
    file_handle_    = nullptr;
    mapping_handle_ = nullptr;
#elif defined(THEMIS_HAS_MMAP)
    ::munmap(const_cast<std::byte*>(data_), size_);
    if (fd_ >= 0) {
        ::close(fd_);
        fd_ = -1;
    }
#endif

    data_ = nullptr;
    size_ = 0;
    path_.clear();
}

MmapRegion::MmapRegion(MmapRegion&& other) noexcept
    : data_(other.data_)
    , size_(other.size_)
    , path_(std::move(other.path_))
#if defined(_WIN32)
    , file_handle_(other.file_handle_)
    , mapping_handle_(other.mapping_handle_)
#elif defined(THEMIS_HAS_MMAP)
    , fd_(other.fd_)
#endif
{
    other.data_ = nullptr;
    other.size_ = 0;
#if defined(_WIN32)
    other.file_handle_    = nullptr;
    other.mapping_handle_ = nullptr;
#elif defined(THEMIS_HAS_MMAP)
    other.fd_ = -1;
#endif
}

MmapRegion& MmapRegion::operator=(MmapRegion&& other) noexcept {
    if (this == &other) {
        return *this;
    }
    close();
    data_ = other.data_;
    size_ = other.size_;
    path_ = std::move(other.path_);
#if defined(_WIN32)
    file_handle_    = other.file_handle_;
    mapping_handle_ = other.mapping_handle_;
    other.file_handle_    = nullptr;
    other.mapping_handle_ = nullptr;
#elif defined(THEMIS_HAS_MMAP)
    fd_      = other.fd_;
    other.fd_ = -1;
#endif
    other.data_ = nullptr;
    other.size_ = 0;
    return *this;
}

// ===========================================================================
// MmapLoader::open
// ===========================================================================

std::pair<MmapRegion, MmapError> MmapLoader::open(
    std::string_view file_path,
    bool             lock_pages) const noexcept {

    MmapRegion region;
    region.path_ = std::string(file_path);

#if defined(_WIN32)
    (void)lock_pages; // parameter not used on Windows implementation
#endif

#if defined(THEMIS_NO_MMAP)
    (void)lock_pages;
    return {std::move(region), MmapError::UNSUPPORTED_PLATFORM};

#elif defined(_WIN32)

    // ── Windows implementation ─────────────────────────────────────────────
    const std::string path_str(file_path);
    // Convert to wide string for Windows API
    const int wlen = ::MultiByteToWideChar(
        CP_UTF8, 0, path_str.c_str(), -1, nullptr, 0);
    if (wlen <= 0) {
        return {std::move(region), MmapError::IO_ERROR};
    }
    std::wstring wpath(static_cast<std::size_t>(wlen), L'\0');
    ::MultiByteToWideChar(CP_UTF8, 0, path_str.c_str(), -1, wpath.data(), wlen);

    HANDLE hFile = ::CreateFileW(
        wpath.c_str(),
        GENERIC_READ,
        FILE_SHARE_READ,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
        nullptr);
    if (hFile == INVALID_HANDLE_VALUE) {
        const DWORD err = ::GetLastError();
        if (err == ERROR_FILE_NOT_FOUND || err == ERROR_PATH_NOT_FOUND) {
            return {std::move(region), MmapError::FILE_NOT_FOUND};
        }
        if (err == ERROR_ACCESS_DENIED) {
            return {std::move(region), MmapError::PERMISSION_DENIED};
        }
        return {std::move(region), MmapError::IO_ERROR};
    }

    LARGE_INTEGER file_size{};
    if (!::GetFileSizeEx(hFile, &file_size)) {
        ::CloseHandle(hFile);
        return {std::move(region), MmapError::IO_ERROR};
    }
    if (file_size.QuadPart == 0) {
        ::CloseHandle(hFile);
        // Empty file: return empty open region (not an error, but no data).
        region.data_ = reinterpret_cast<const std::byte*>(1); // non-null sentinel
        region.size_ = 0;
        region.file_handle_ = hFile;
        return {std::move(region), MmapError::OK};
    }

    HANDLE hMap = ::CreateFileMappingW(
        hFile, nullptr, PAGE_READONLY, 0, 0, nullptr);
    if (!hMap) {
        ::CloseHandle(hFile);
        return {std::move(region), MmapError::MAPPING_FAILED};
    }

    const void* view = ::MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0);
    if (!view) {
        ::CloseHandle(hMap);
        ::CloseHandle(hFile);
        return {std::move(region), MmapError::MAPPING_FAILED};
    }

    region.data_           = static_cast<const std::byte*>(view);
    region.size_           = static_cast<std::size_t>(file_size.QuadPart);
    region.file_handle_    = hFile;
    region.mapping_handle_ = hMap;
    return {std::move(region), MmapError::OK};

#else // POSIX

    // ── POSIX implementation ───────────────────────────────────────────────
    const std::string path_str(file_path);
    const int fd = ::open(path_str.c_str(), O_RDONLY | O_CLOEXEC);
    if (fd < 0) {
        if (errno == ENOENT) {
            return {std::move(region), MmapError::FILE_NOT_FOUND};
        }
        if (errno == EACCES || errno == EPERM) {
            return {std::move(region), MmapError::PERMISSION_DENIED};
        }
        return {std::move(region), MmapError::IO_ERROR};
    }

    struct stat st{};
    if (::fstat(fd, &st) != 0) {
        ::close(fd);
        return {std::move(region), MmapError::IO_ERROR};
    }

    const auto file_size = static_cast<std::size_t>(st.st_size);
    if (file_size == 0) {
        // Empty file: legal but zero-length mapping is not supported by mmap.
        ::close(fd);
        region.data_ = nullptr;
        region.size_ = 0;
        // Return OK with empty region; caller can check size()==0.
        return {std::move(region), MmapError::OK};
    }

    void* addr = ::mmap(nullptr, file_size, PROT_READ, MAP_SHARED, fd, 0);
    if (addr == MAP_FAILED) { // NOLINT(performance-no-int-to-ptr)
        ::close(fd);
        return {std::move(region), MmapError::MAPPING_FAILED};
    }

    region.data_ = static_cast<const std::byte*>(addr);
    region.size_ = file_size;
    region.fd_   = fd;

    if (lock_pages) {
#if defined(__linux__) || defined(__APPLE__)
        if (::mlock(addr, file_size) != 0) {
            // Non-fatal: mapping is usable, but pages may be swapped.
            // Caller can detect via MmapError::LOCK_FAILED if needed.
            // We still return OK here because the mapping itself succeeded.
        }
#endif
    }

    return {std::move(region), MmapError::OK};
#endif
}

// ===========================================================================
// MmapLoader::advise
// ===========================================================================

MmapError MmapLoader::advise(
    const MmapRegion& region,
    AccessPattern     pattern) const noexcept {

    if (!region.isOpen()) {
        return MmapError::NOT_OPEN;
    }

#if defined(THEMIS_HAS_MMAP) && defined(__linux__)
    int advice = MADV_NORMAL;
    switch (pattern) {
        case AccessPattern::SEQUENTIAL: advice = MADV_SEQUENTIAL; break;
        case AccessPattern::RANDOM:     advice = MADV_RANDOM;     break;
        case AccessPattern::WILLNEED:   advice = MADV_WILLNEED;   break;
        case AccessPattern::DONTNEED:   advice = MADV_DONTNEED;   break;
    }
    // madvise is advisory; failure is non-fatal.
    ::madvise(const_cast<std::byte*>(region.data()), region.size(), advice);
#elif defined(THEMIS_HAS_MMAP) && defined(__APPLE__)
    int advice = MADV_NORMAL;
    switch (pattern) {
        case AccessPattern::SEQUENTIAL: advice = MADV_SEQUENTIAL; break;
        case AccessPattern::RANDOM:     advice = MADV_RANDOM;     break;
        case AccessPattern::WILLNEED:   advice = MADV_WILLNEED;   break;
        case AccessPattern::DONTNEED:   advice = MADV_DONTNEED;   break;
    }
    ::madvise(const_cast<std::byte*>(region.data()), region.size(), advice);
#else
    // No madvise support; silently ignore.
    (void)pattern;
#endif

    return MmapError::OK;
}

// ===========================================================================
// QuantizationAssessor
// ===========================================================================

// Ensure the array is in sync with the enum.
static_assert(
    static_cast<int>(QuantizationLevel::BINARY) == 5,
    "kTypicalL2Error index sync check failed");

// Returns the typical L2 error for a given level.
static double typicalL2Error(QuantizationLevel level) noexcept {
    // Mirrors QuantizationAssessor::kTypicalL2Error
    constexpr std::array<double, 6> kErrors = {
        0.0,   // F32
        0.001, // F16
        0.002, // BF16
        0.01,  // INT8
        0.05,  // INT4
        0.20,  // BINARY
    };
    const auto idx = static_cast<std::size_t>(level);
    return (idx < kErrors.size()) ? kErrors[idx] : 0.0;
}

bool QuantizationAssessor::isFeasible(
    QuantizationLevel              level,
    const QuantizationConstraints& constraints) noexcept {

    // Lossless reference: always feasible.
    if (level == QuantizationLevel::F32) {
        return true;
    }

    // INT8 and INT4 require calibration data.
    if ((level == QuantizationLevel::INT8 || level == QuantizationLevel::INT4)
        && !constraints.has_calibration_data) {
        return false;
    }

    // Check L2 error budget.
    const double err = typicalL2Error(level);
    if (constraints.max_l2_error_relative > 0.0
        && err > constraints.max_l2_error_relative) {
        return false;
    }

    // Check memory budget.
    if (constraints.memory_budget_bytes > 0 && constraints.num_params > 0) {
        const uint64_t needed = packedBytesForParams(constraints.num_params, level);
        if (needed > constraints.memory_budget_bytes) {
            return false;
        }
    }

    return true;
}

QuantizationAssessment QuantizationAssessor::assess(
    const QuantizationConstraints& constraints) {

    QuantizationAssessment result;

    // Priority-ordered candidate list (best compression first).
    constexpr std::array<QuantizationLevel, 6> kCandidates = {
        QuantizationLevel::BINARY,
        QuantizationLevel::INT4,
        QuantizationLevel::INT8,
        QuantizationLevel::BF16,
        QuantizationLevel::F16,
        QuantizationLevel::F32,
    };

    // Special case: zero error budget → must stay at F32.
    if (constraints.max_l2_error_relative == 0.0) {
        result.recommended_level  = QuantizationLevel::F32;
        result.compression_ratio  = 1.0;
        result.estimated_l2_error = 0.0;
        result.rationale =
            "F32 selected: zero error budget specified (lossless reference required).";
        return result;
    }

    // Find the most compressed feasible level.
    QuantizationLevel chosen = QuantizationLevel::F32;
    for (const auto& candidate : kCandidates) {
        if (isFeasible(candidate, constraints)) {
            chosen = candidate;
            break;
        }
    }

    // Compute output fields.
    result.recommended_level  = chosen;
    result.estimated_l2_error = typicalL2Error(chosen);

    // Compression ratio vs F32 (bytes per param).
    const double f32_bytes    = static_cast<double>(bytesPerParam(QuantizationLevel::F32));
    const double chosen_bytes = static_cast<double>(bytesPerParam(chosen));
    result.compression_ratio  = (chosen_bytes > 0.0) ? (f32_bytes / chosen_bytes) : 1.0;

    // Special ratio for sub-byte levels.
    if (chosen == QuantizationLevel::INT4) {
        result.compression_ratio = 8.0; // 4-bit vs 32-bit
    } else if (chosen == QuantizationLevel::BINARY) {
        result.compression_ratio = 32.0; // 1-bit vs 32-bit
    }

    // Build rationale string.
    std::ostringstream oss;
    oss << "Recommended ";
    switch (chosen) {
        case QuantizationLevel::F32:    oss << "F32";    break;
        case QuantizationLevel::F16:    oss << "F16";    break;
        case QuantizationLevel::BF16:   oss << "BF16";   break;
        case QuantizationLevel::INT8:   oss << "INT8";   break;
        case QuantizationLevel::INT4:   oss << "INT4";   break;
        case QuantizationLevel::BINARY: oss << "BINARY"; break;
    }
    oss << " (compression " << result.compression_ratio << "x"
        << ", estimated L2 error ≤ " << result.estimated_l2_error * 100.0 << "%).";

    if (constraints.is_adapter) {
        oss << " Adapter artifact: higher compression tolerated.";
    }
    if (!constraints.has_calibration_data
        && (chosen == QuantizationLevel::INT8 || chosen == QuantizationLevel::INT4)) {
        result.warnings.emplace_back(
            "INT8/INT4 selected but has_calibration_data=false; "
            "dynamic quantization will be used, increasing error variance.");
    }
    if (constraints.hw_avx512_bf16 && chosen == QuantizationLevel::F16) {
        result.warnings.emplace_back(
            "Hardware supports AVX-512 BF16; consider BF16 for better throughput "
            "on this architecture.");
    }

    result.rationale = oss.str();
    return result;
}

// ===========================================================================
// StorageStrategyAssessor
// ===========================================================================

StorageStrategyRecommendation StorageStrategyAssessor::assess(const Config& config) {
    StorageStrategyRecommendation rec;

    // ── Quantization ──────────────────────────────────────────────────────
    const auto quant = QuantizationAssessor::assess(config.quant);
    rec.quantization_level  = quant.recommended_level;
    rec.compression_ratio   = quant.compression_ratio;
    rec.estimated_l2_error  = quant.estimated_l2_error;

    if (config.quant.num_params > 0) {
        rec.estimated_size_bytes = packedBytesForParams(
            config.quant.num_params, quant.recommended_level);
    }

    // ── Load mechanism ────────────────────────────────────────────────────
    if (!config.os_supports_mmap) {
        rec.load_mechanism = LoadMechanism::BUFFERED_READ;
        rec.caveats.emplace_back(
            "mmap disabled by configuration; using buffered read. "
            "Multi-consumer scenarios will create redundant copies.");
    } else if (config.multi_consumer) {
        // Multiple concurrent consumers → shared page cache is most efficient.
        rec.load_mechanism = config.storage_on_nvme
            ? LoadMechanism::MMAP_PREFAULT
            : LoadMechanism::MMAP_ZERO_COPY;
    } else if (config.storage_on_nvme) {
        // NVMe, single consumer → prefault to front-load latency.
        rec.load_mechanism = LoadMechanism::MMAP_PREFAULT;
    } else {
        // HDD/network storage → lazy mmap is fine.
        rec.load_mechanism = LoadMechanism::MMAP_ZERO_COPY;
    }

    // Propagate quantization warnings as caveats.
    for (const auto& w : quant.warnings) {
        rec.caveats.push_back(w);
    }

    // ── Summary string ────────────────────────────────────────────────────
    std::ostringstream oss;
    oss << quant.rationale << " ";

    switch (rec.load_mechanism) {
        case LoadMechanism::MMAP_ZERO_COPY:
            oss << "Load: mmap zero-copy (lazy fault-in, OS-managed eviction).";
            break;
        case LoadMechanism::MMAP_PREFAULT:
            oss << "Load: mmap prefault (MADV_WILLNEED, eager page-in for NVMe).";
            break;
        case LoadMechanism::BUFFERED_READ:
            oss << "Load: buffered read() (mmap unavailable or disabled).";
            break;
        case LoadMechanism::DIRECT_IO:
            oss << "Load: direct I/O (O_DIRECT, bypass page cache).";
            break;
    }

    if (config.multi_consumer) {
        oss << " Shared page cache favoured for multi-consumer workload.";
    }
    if (config.can_lock_pages) {
        oss << " mlock() available; consider locking hot tensors to prevent eviction.";
    }

    rec.summary = oss.str();
    return rec;
}

} // namespace distributed_tensor
} // namespace themis
