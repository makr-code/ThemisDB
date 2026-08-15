/**
 * @file wire_protocol_zero_copy.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=3, M=3, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// ThemisDB Wire Protocol – Zero-Copy Serialization (implementation)

#include "network/wire_protocol_zero_copy.h"

#include <cassert>
#include <cerrno>
#include <fstream>
#include <stdexcept>
#include <system_error>
#include <utility>

#ifdef _WIN32
#include <malloc.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#  ifdef __linux__
#    include <sys/sendfile.h>
#  endif
#endif

#include <vector>

namespace themis {
namespace network {

// =============================================================================
// Byte-order helpers (avoids pulling in <arpa/inet.h> globally)
// =============================================================================

namespace {

inline uint32_t to_be32(uint32_t v) noexcept {
#ifdef _WIN32
    return _byteswap_ulong(v);
#elif __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    return __builtin_bswap32(v);
#else
    return v;
#endif
}

inline uint16_t to_be16(uint16_t v) noexcept {
#ifdef _WIN32
    return _byteswap_ushort(v);
#elif __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    return __builtin_bswap16(v);
#else
    return v;
#endif
}

// R09, R11: Helper to check socket readiness with timeout (non-blocking I/O + poll)
// Returns: true if socket is ready for writing, false on timeout.
// On timeout, sets errno to ETIMEDOUT.
#ifndef _WIN32
inline bool waitForSocketWritable(int fd, int timeout_ms) noexcept {
    struct pollfd pfd;
    pfd.fd = fd;
    pfd.events = POLLOUT;
    pfd.revents = 0;
    
    int result = ::poll(&pfd, 1, timeout_ms);
    if (result <= 0) {
        if (result == 0) {
            errno = ETIMEDOUT;
        }
        return false;
    }
    return (pfd.revents & POLLOUT) != 0;
}
#endif

} // anonymous namespace

// =============================================================================
// ZeroCopyFrameBuilder
// =============================================================================

ZeroCopyFrameBuilder::ZeroCopyFrameBuilder(uint16_t   opcode,
                                           uint16_t   flags,
                                           const void* payload,
                                           size_t      payload_size) noexcept
    : payload_(payload), payload_size_(payload_size)
{
    // Serialise header into the stack buffer (big-endian / network order).
    const uint32_t magic_be   = to_be32(WIRE_MAGIC);
    const uint16_t flags_be   = to_be16(flags);
    const uint16_t opcode_be  = to_be16(opcode);
    const uint32_t psize_be   = to_be32(static_cast<uint32_t>(payload_size));

    static_assert(HEADER_SIZE == 12, "Header size must be 12 bytes");

    uint8_t* p = header_.data();
    std::memcpy(p,     &magic_be,  4); p += 4;
    std::memcpy(p,     &flags_be,  2); p += 2;
    std::memcpy(p,     &opcode_be, 2); p += 2;
    std::memcpy(p,     &psize_be,  4);
}

ssize_t ZeroCopyFrameBuilder::writeTo(int fd) const noexcept {
#ifdef _WIN32
    WSABUF bufs[2];
    bufs[0].buf = reinterpret_cast<char*>(const_cast<uint8_t*>(header_.data()));
    bufs[0].len = static_cast<ULONG>(HEADER_SIZE);
    bufs[1].buf = reinterpret_cast<char*>(const_cast<void*>(payload_));
    bufs[1].len = static_cast<ULONG>(payload_size_);

    DWORD sent = 0;
    const DWORD buf_count = payload_size_ == 0 ? 1u : 2u;
    const int rc = ::WSASend(static_cast<SOCKET>(fd), bufs, buf_count, &sent, 0, nullptr, nullptr);
    return rc == 0 ? static_cast<ssize_t>(sent) : -1;
#else
    // R09: Add timeout enforcement to blocking write operations.
    // Use poll() with 5000ms timeout to prevent indefinite blocking.
    const int timeout_ms = 5000;
    if (!waitForSocketWritable(fd, timeout_ms)) {
        return -1;  // errno set to ETIMEDOUT by waitForSocketWritable
    }

    if (payload_size_ == 0) {
        // Header-only frame: single write.
        return ::write(fd, header_.data(), HEADER_SIZE);
    }

    // Scatter-gather write: header + payload in one syscall.
    struct iovec iov[2];
    iov[0].iov_base = const_cast<uint8_t*>(header_.data());
    iov[0].iov_len  = HEADER_SIZE;
    iov[1].iov_base = const_cast<void*>(payload_);
    iov[1].iov_len  = payload_size_;

    return ::writev(fd, iov, 2);
#endif
}

ssize_t ZeroCopyFrameBuilder::writeToWithSendfile(int    socket_fd,
                                                   int    payload_fd,
                                                   off_t  payload_offset,
                                                   size_t sendfile_threshold) const {
#ifdef _WIN32
    // Windows: no sendfile equivalent for socket+file — fall back to writev path.
    (void)payload_fd; (void)payload_offset; (void)sendfile_threshold;
    return writeTo(socket_fd);
#else
    // Use sendfile only for large, file-backed payloads.
    const bool use_sendfile = (payload_fd >= 0)
                               && (payload_size_ >= sendfile_threshold);

    if (!use_sendfile) {
        return writeTo(socket_fd);
    }

    // Step 1: write the 12-byte header.
    ssize_t hdr_written = 0;
    while (hdr_written < static_cast<ssize_t>(HEADER_SIZE)) {
        const ssize_t n = ::write(socket_fd,
                                  header_.data() + hdr_written,
                                  HEADER_SIZE - static_cast<size_t>(hdr_written));
        if (n < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        hdr_written += n;
    }

    // Step 2: zero-copy payload transfer via sendfile(2) (Linux / macOS / FreeBSD).
#  if defined(__linux__)
    // Linux sendfile: sendfile(out_fd, in_fd, offset, count)
    // Falls back to writev if the source fd is not a regular file (EINVAL/ENOSYS).
    // R11: Add timeout enforcement to sendfile loop (5000ms total deadline).
    off_t  off           = payload_offset;
    size_t remaining     = payload_size_;
    ssize_t sf_written   = 0;
    const int timeout_ms = 5000;

    while (remaining > 0) {
        // Check socket readiness with timeout before sendfile call
        if (!waitForSocketWritable(socket_fd, timeout_ms)) {
            return sf_written > 0 ? hdr_written + sf_written : -1;  // errno set to ETIMEDOUT
        }

        const ssize_t n = ::sendfile(socket_fd, payload_fd, &off, remaining);
        if (n < 0) {
            if (errno == EINTR) continue;
            if (errno == EINVAL || errno == ENOSYS || errno == ENOTSUP) {
                // sendfile not supported for this fd type (e.g., socket source).
                // Fall back to copy-based writev for the remaining bytes.
                // Re-read remaining bytes via pread and write.
                std::vector<uint8_t> tmp(remaining);
                const ssize_t rd = ::pread(payload_fd, tmp.data(), remaining,
                                           payload_offset + static_cast<off_t>(sf_written));
                if (rd <= 0) return sf_written > 0 ? hdr_written + sf_written : -1;
                
                // Check socket readiness before fallback write
                if (!waitForSocketWritable(socket_fd, timeout_ms)) {
                    return sf_written > 0 ? hdr_written + sf_written : -1;
                }
                
                const ssize_t wn = ::write(socket_fd, tmp.data(), static_cast<size_t>(rd));
                if (wn < 0) return sf_written > 0 ? hdr_written + sf_written : -1;
                sf_written += wn;
                break;
            }
            // Other errors are fatal.
            return sf_written > 0 ? hdr_written + sf_written : -1;
        }
        sf_written += n;
        remaining  -= static_cast<size_t>(n);
    }

    return hdr_written + sf_written;

#  elif defined(__APPLE__) || defined(__FreeBSD__)
    // macOS / FreeBSD sendfile: sendfile(in_fd, out_fd, offset, len, hdtr, written, flags)
    off_t len        = static_cast<off_t>(payload_size_);
    off_t off        = payload_offset;
    off_t sf_written = 0;
    int   rc         = 0;
#    if defined(__APPLE__)
    rc = ::sendfile(payload_fd, socket_fd, off, &len, nullptr, 0);
    sf_written = len;
#    else // FreeBSD
    rc = ::sendfile(payload_fd, socket_fd, off, payload_size_, nullptr, &sf_written, 0);
#    endif
    if (rc < 0 && errno != EAGAIN && errno != EINTR) {
        return sf_written > 0 ? hdr_written + sf_written : -1;
    }
    return hdr_written + sf_written;

#  else
    // Platform does not support sendfile: fall back to pread + write.
    (void)payload_offset;
    return writeTo(socket_fd);
#  endif
#endif // !_WIN32
}

// =============================================================================
// MemoryMappedPayload
// =============================================================================

MemoryMappedPayload::MemoryMappedPayload(const std::string& path) {
#ifdef _WIN32
    std::ifstream in(path, std::ios::binary | std::ios::ate);
    if (!in) {
        throw std::runtime_error("MemoryMappedPayload: open failed: " + path);
    }
    const std::streamsize file_size = in.tellg();
    if (file_size <= 0) {
        throw std::runtime_error("MemoryMappedPayload: file is empty: " + path);
    }

    size_ = static_cast<size_t>(file_size);
    if (size_ > MAX_MAP_SIZE) {
        throw std::runtime_error("MemoryMappedPayload: file too large (> 256 MiB): " + path);
    }

    void* mem = std::malloc(size_);
    if (mem == nullptr) {
        throw std::runtime_error("MemoryMappedPayload: allocation failed");
    }
    addr_ = mem;

    in.seekg(0, std::ios::beg);
    if (!in.read(static_cast<char*>(addr_), static_cast<std::streamsize>(size_))) {
        std::free(addr_);
        addr_ = MAP_FAILED;
        size_ = 0;
        throw std::runtime_error("MemoryMappedPayload: read failed: " + path);
    }
#else
    fd_ = ::open(path.c_str(), O_RDONLY | O_CLOEXEC);
    if (fd_ < 0) {
        throw std::system_error(errno, std::system_category(),
                                "MemoryMappedPayload: open failed: " + path);
    }

    const off_t file_size = ::lseek(fd_, 0, SEEK_END);
    if (file_size <= 0) {
        ::close(fd_);
        fd_ = -1;
        throw std::runtime_error(
            "MemoryMappedPayload: file is empty or lseek failed: " + path);
    }

    size_ = static_cast<size_t>(file_size);
    if (size_ > MAX_MAP_SIZE) {
        ::close(fd_);
        fd_ = -1;
        throw std::runtime_error(
            "MemoryMappedPayload: file too large (> 256 MiB): " + path);
    }

    addr_ = ::mmap(nullptr, size_, PROT_READ, MAP_SHARED, fd_, 0);
    if (addr_ == MAP_FAILED) {
        const int err = errno;
        ::close(fd_);
        fd_ = -1;
        throw std::system_error(err, std::system_category(),
                                "MemoryMappedPayload: mmap failed: " + path);
    }

    // R19: File Descriptor Cleanup Pattern
    // Ensures fd_ is properly closed even if exceptions occur:
    // 1. On lseek failure (line 296): ::close(fd_) before throw
    // 2. On size validation failure (line 304): ::close(fd_) before throw
    // 3. On mmap failure (line 313): ::close(fd_) before throw
    // 4. On successful mmap (here): fd_ retained; will be closed in destructor
    //    or transferred via move semantics (see ~MemoryMappedPayload, operator=)
    // 
    // Exception Safety: Strong guarantee via RAII (see destructor at line ~359)
    // If exception thrown after this point, destructor will close fd_.

    // Advise sequential access to allow read-ahead.
    ::madvise(addr_, size_, MADV_SEQUENTIAL);
#endif
}

MemoryMappedPayload::MemoryMappedPayload(size_t size) {
    if (size == 0) {
        throw std::invalid_argument(
            "MemoryMappedPayload: size must be > 0 for anonymous mapping");
    }

#ifdef _WIN32
    addr_ = std::calloc(1, size);
    if (addr_ == nullptr) {
        throw std::runtime_error("MemoryMappedPayload: allocation failed");
    }
    size_ = size;
#else
    size_ = size;
    addr_ = ::mmap(nullptr, size_, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (addr_ == MAP_FAILED) {
        throw std::system_error(errno, std::system_category(),
                                "MemoryMappedPayload: anonymous mmap failed");
    }
    // fd_ stays -1 (anonymous mapping has no backing file).
#endif
}

MemoryMappedPayload::~MemoryMappedPayload() {
#ifdef _WIN32
    if (addr_ != nullptr && addr_ != MAP_FAILED) {
        std::free(addr_);
        addr_ = MAP_FAILED;
    }
#else
    if (addr_ != MAP_FAILED && addr_ != nullptr) {
        ::munmap(addr_, size_);
        addr_ = MAP_FAILED;
    }
    if (fd_ >= 0) {
        ::close(fd_);
        fd_ = -1;
    }
#endif
}

MemoryMappedPayload::MemoryMappedPayload(MemoryMappedPayload&& other) noexcept
    : addr_(other.addr_), size_(other.size_), fd_(other.fd_)
{
    other.addr_ = MAP_FAILED;
    other.size_ = 0;
    other.fd_   = -1;
}

MemoryMappedPayload& MemoryMappedPayload::operator=(
    MemoryMappedPayload&& other) noexcept
{
    if (this != &other) {
        // Release current mapping.
#ifdef _WIN32
        if (addr_ != MAP_FAILED && addr_ != nullptr) std::free(addr_);
#else
        if (addr_ != MAP_FAILED && addr_ != nullptr) ::munmap(addr_, size_);
        if (fd_ >= 0) ::close(fd_);
#endif

        addr_       = other.addr_;
        size_       = other.size_;
        fd_         = other.fd_;
        other.addr_ = MAP_FAILED;
        other.size_ = 0;
        other.fd_   = -1;
    }
    return *this;
}

} // namespace network
} // namespace themis
