/**
 * @file wire_protocol_zero_copy.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

/*
 * ThemisDB | File: wire_protocol_zero_copy.h | Version: 0.0.13 | Last Modified: 2026-05-20 17:13:04
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 249
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * PR History (last 5): #4214 feat(network): Wire Protoco... (2026-03-15)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

// ThemisDB Wire Protocol – Zero-Copy Serialization
//
// This module provides two zero-copy primitives for the wire protocol:
//
//   1. ZeroCopyFrameBuilder  – assemble a protocol frame from a fixed-size
//      header buffer and a caller-owned payload pointer, then write it to a
//      socket in a single writev(2) call.  No intermediate heap allocation is
//      required; the payload bytes are never copied.
//
//   2. MemoryMappedPayload   – memory-map a file (or an anonymous region) for
//      large payloads.  The kernel transfers data directly from the page cache
//      to the NIC buffer without a userspace copy, achieving true zero-copy I/O
//      for payloads backed by files.
//
// Both classes are header-safe: they depend only on POSIX APIs and standard
// C++ headers, so they can be included in any translation unit without extra
// link-time dependencies.
//
// Performance targets (v1.7.0):
//   - Round-trip latency: <1 ms (p99) for payloads ≤ 64 KiB
//   - Memory overhead:    <10 MB per 1 000 connections

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#include <system_error>
#include <vector>

#ifdef _WIN32
#include <BaseTsd.h>
using ssize_t = SSIZE_T;
#ifndef THEMIS_IOVEC_DEFINED
#define THEMIS_IOVEC_DEFINED
struct iovec {
    void* iov_base;
    size_t iov_len;
};
#endif
#ifndef MAP_FAILED
#define MAP_FAILED nullptr
#endif
using off_t = long long;
#else
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <fcntl.h>
#endif

namespace themis {
namespace network {

// =============================================================================
// ZeroCopyFrameBuilder
// =============================================================================

/**
 * @brief Assemble a wire-protocol frame and send it via writev(2).
 *
 * A wire-protocol frame consists of a fixed-size header followed by an
 * optional variable-length payload.  The conventional approach copies both
 * parts into a contiguous heap buffer before writing.  This class avoids that
 * copy by placing the header into a stack buffer and pointing the second iovec
 * element directly at the caller-owned payload.
 *
 * Header layout (12 bytes, network byte order):
 * @code
 *   [magic: 4B][flags: 2B][opcode: 2B][payload_size: 4B]
 * @endcode
 *
 * Usage:
 * @code
 *   ZeroCopyFrameBuilder builder(opcode, flags, payload.data(), payload.size());
 *   ssize_t written = builder.writeTo(socket_fd);
 *   if (written < 0) { // handle error }
 * @endcode
 */
class ZeroCopyFrameBuilder {
public:
    /// Wire-protocol magic bytes (little-endian "THMS").
    static constexpr uint32_t WIRE_MAGIC = 0x534D4854u;

    /// Fixed header size in bytes.
    static constexpr size_t HEADER_SIZE = 12;

    /**
     * @brief Construct a frame builder.
     *
     * @param opcode        Wire-protocol operation code (16-bit).
     * @param flags         Frame flags (16-bit; e.g., COMPRESSED, ENCRYPTED).
     * @param payload       Pointer to caller-owned payload bytes (may be null
     *                      when @p payload_size == 0).
     * @param payload_size  Number of payload bytes to include.
     */
    ZeroCopyFrameBuilder(uint16_t opcode,
                         uint16_t flags,
                         const void* payload,
                         size_t payload_size) noexcept;

    /**
     * @brief Write the frame to a file descriptor using writev(2).
     *
     * The header and payload are written atomically as a scatter-gather
     * vector; no intermediate copy is performed.
     *
     * @param fd  Open, writable file descriptor (TCP socket).
     * @return    Total bytes written on success (>= 0), or -1 on error
     *            (errno is set by the underlying writev call).
     */
    ssize_t writeTo(int fd) const noexcept;

    /**
     * @brief Large-payload path: write header via write(2), then payload via
     *        sendfile(2) / splice(2) (Linux) or sendfile(2) (macOS/FreeBSD).
     *
     * For payloads ≥ @p sendfile_threshold bytes and where @p payload_fd is
     * a valid open file descriptor, this method:
     *   1. Writes the 12-byte header with write(2).
     *   2. Transfers @c payload_size_ bytes from @p payload_fd starting at
     *      @p payload_offset using sendfile(2) (Linux / BSD) or splice(2)
     *      (Linux pipe path) — zero userspace copy.
     *
     * Falls back to @c writeTo() if:
     *   - @p payload_fd < 0 (caller signals: not file-backed).
     *   - @c payload_size_ < @p sendfile_threshold.
     *   - sendfile(2) / splice(2) fails with EINVAL / ENOSYS (e.g., the fd
     *     is a socket, not a regular file — kernel constraint).
     *
     * @param socket_fd          Destination socket file descriptor.
     * @param payload_fd         Source file descriptor for sendfile.
     *                           Pass -1 to force fallback to writev.
     * @param payload_offset     Starting offset in the source file (bytes).
     * @param sendfile_threshold Minimum payload size to use sendfile (bytes).
     *                           Default: 65536 (64 KiB).
     * @return Total bytes written (header + payload) on success, or -1 on
     *         error (errno set by the failing syscall).
     */
    ssize_t writeToWithSendfile(int      socket_fd,
                                int      payload_fd,
                                off_t    payload_offset    = 0,
                                size_t   sendfile_threshold = 65536) const noexcept;

    /**
     * @brief Total frame size in bytes (header + payload).
     */
    size_t frameSize() const noexcept { return HEADER_SIZE + payload_size_; }

    /**
     * @brief Access the serialized header bytes.
     */
    const std::array<uint8_t, HEADER_SIZE>& header() const noexcept {
        return header_;
    }

private:
    std::array<uint8_t, HEADER_SIZE> header_;
    const void*                       payload_;
    size_t                            payload_size_;
};

// =============================================================================
// MemoryMappedPayload
// =============================================================================

/**
 * @brief RAII wrapper for a read-only memory-mapped payload.
 *
 * For large payloads (> a few hundred KiB) that reside in files, memory
 * mapping avoids a read(2) call that would copy the data into userspace.
 * When the mapped region is later passed to writev/sendfile the kernel can
 * transfer bytes directly from the page cache, eliminating the copy
 * entirely on supporting platforms.
 *
 * Example:
 * @code
 *   MemoryMappedPayload mmp("/var/data/large_blob.bin");
 *   ZeroCopyFrameBuilder builder(opcode, flags, mmp.data(), mmp.size());
 *   builder.writeTo(socket_fd);
 * @endcode
 *
 * @note The mapping is released automatically on destruction.
 */
class MemoryMappedPayload {
public:
    /**
     * @brief Map the contents of @p path into the process address space.
     *
     * @throws std::system_error  on open(2) or mmap(2) failure.
     * @throws std::runtime_error if the file is empty or too large
     *         (> MemoryMappedPayload::MAX_MAP_SIZE bytes).
     */
    explicit MemoryMappedPayload(const std::string& path);

    /**
     * @brief Map an anonymous region of @p size bytes (zero-initialised).
     *
     * Useful for testing and for building large payloads in-memory without
     * additional heap allocations.
     *
     * @throws std::system_error  on mmap(2) failure.
     * @throws std::invalid_argument if @p size == 0.
     */
    explicit MemoryMappedPayload(size_t size);

    ~MemoryMappedPayload();

    // Non-copyable; moveable.
    MemoryMappedPayload(const MemoryMappedPayload&)            = delete;
    MemoryMappedPayload& operator=(const MemoryMappedPayload&) = delete;
    MemoryMappedPayload(MemoryMappedPayload&&) noexcept;
    MemoryMappedPayload& operator=(MemoryMappedPayload&&) noexcept;

    /// Pointer to the first mapped byte.
    const uint8_t* data() const noexcept {
        return static_cast<const uint8_t*>(addr_);
    }

    /// Mutable pointer (only valid for anonymous mappings).
    uint8_t* mutableData() noexcept {
        return static_cast<uint8_t*>(addr_);
    }

    /// Number of mapped bytes.
    size_t size() const noexcept { return size_; }

    /// Return true if the mapping is valid (not moved-from).
    bool valid() const noexcept { return addr_ != MAP_FAILED && addr_ != nullptr; }

    /// Maximum file size that may be mapped (256 MiB).
    static constexpr size_t MAX_MAP_SIZE = 256ULL * 1024 * 1024;

private:
    void*  addr_ = MAP_FAILED;
    size_t size_ = 0;
    int    fd_   = -1; ///< Open file descriptor (−1 for anonymous mappings).
};

// =============================================================================
// ZeroCopyStats
// =============================================================================

/**
 * @brief Aggregate statistics for zero-copy I/O operations.
 *
 * Attach one instance to a connection or session and call the appropriate
 * record* method on each write/map operation.
 */
struct ZeroCopyStats {
    uint64_t frames_written     = 0; ///< Total frames sent via writeTo()
    uint64_t bytes_written      = 0; ///< Total bytes sent via writeTo()
    uint64_t mmap_opens         = 0; ///< Total MemoryMappedPayload constructions
    uint64_t mmap_bytes_mapped  = 0; ///< Total bytes memory-mapped
    uint64_t write_errors       = 0; ///< Failed writev() calls

    void recordWrite(size_t bytes) noexcept {
        ++frames_written;
        bytes_written += bytes;
    }
    void recordWriteError() noexcept { ++write_errors; }
    void recordMmap(size_t bytes) noexcept {
        ++mmap_opens;
        mmap_bytes_mapped += bytes;
    }
};

} // namespace network
} // namespace themis
