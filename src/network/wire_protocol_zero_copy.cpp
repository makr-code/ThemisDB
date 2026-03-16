/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            wire_protocol_zero_copy.cpp                        ║
  Version:         0.0.1                                              ║
  Last Modified:   2026-03-16 04:16:43                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     186                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 543f66e65  2026-03-14  feat(network): implement wire protocol performance optimi... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// ThemisDB Wire Protocol – Zero-Copy Serialization (implementation)

#include "network/wire_protocol_zero_copy.h"

#include <cassert>
#include <cerrno>
#include <stdexcept>
#include <system_error>
#include <utility>

namespace themis {
namespace network {

// =============================================================================
// Byte-order helpers (avoids pulling in <arpa/inet.h> globally)
// =============================================================================

namespace {

inline uint32_t to_be32(uint32_t v) noexcept {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    return __builtin_bswap32(v);
#else
    return v;
#endif
}

inline uint16_t to_be16(uint16_t v) noexcept {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    return __builtin_bswap16(v);
#else
    return v;
#endif
}

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
}

// =============================================================================
// MemoryMappedPayload
// =============================================================================

MemoryMappedPayload::MemoryMappedPayload(const std::string& path) {
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

    // Advise sequential access to allow read-ahead.
    ::madvise(addr_, size_, MADV_SEQUENTIAL);
}

MemoryMappedPayload::MemoryMappedPayload(size_t size) {
    if (size == 0) {
        throw std::invalid_argument(
            "MemoryMappedPayload: size must be > 0 for anonymous mapping");
    }

    size_ = size;
    addr_ = ::mmap(nullptr, size_, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (addr_ == MAP_FAILED) {
        throw std::system_error(errno, std::system_category(),
                                "MemoryMappedPayload: anonymous mmap failed");
    }
    // fd_ stays -1 (anonymous mapping has no backing file).
}

MemoryMappedPayload::~MemoryMappedPayload() {
    if (addr_ != MAP_FAILED && addr_ != nullptr) {
        ::munmap(addr_, size_);
        addr_ = MAP_FAILED;
    }
    if (fd_ >= 0) {
        ::close(fd_);
        fd_ = -1;
    }
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
        if (addr_ != MAP_FAILED && addr_ != nullptr) ::munmap(addr_, size_);
        if (fd_ >= 0) ::close(fd_);

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
