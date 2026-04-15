/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            wire_protocol_zero_copy.cpp                        ║
  Version:         0.0.9                                              ║
  Last Modified:   2026-04-15 04:17:56                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     272                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 354c97d283  2026-03-16  feat: Add new erasure coding backend and related components ║
    • 543f66e654  2026-03-14  feat(network): implement wire protocol performance optimi... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
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
#include <unistd.h>
#endif

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
