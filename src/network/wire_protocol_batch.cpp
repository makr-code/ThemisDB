/**
 * @file wire_protocol_batch.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// ThemisDB Wire Protocol – Batch Write Processing (implementation)

#include "network/wire_protocol_batch.h"

#include <cassert>
#include <cerrno>
#include <cstring>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

namespace themis {
namespace network {

namespace {

inline int setSockOptInt(int fd, int level, int optname, const int* value) {
#ifdef _WIN32
    return ::setsockopt(static_cast<SOCKET>(fd), level, optname,
                        reinterpret_cast<const char*>(value),
                        static_cast<int>(sizeof(*value)));
#else
    return ::setsockopt(fd, level, optname, value, sizeof(*value));
#endif
}

} // anonymous namespace

// =============================================================================
// NagleController
// =============================================================================

NagleController::NagleController(int fd) noexcept : fd_(fd) {}

bool NagleController::setMode(Mode mode) noexcept {
    if (fd_ < 0) return false;

    // Reset both flags first so we reach a clean state.
    int on  = 1;
    [[maybe_unused]] int off = 0;

    switch (mode) {
    case Mode::NODELAY: {
        // Enable TCP_NODELAY, disable TCP_CORK.
        if (setSockOptInt(fd_, IPPROTO_TCP, TCP_NODELAY, &on) != 0) return false;
#ifdef TCP_CORK
        setSockOptInt(fd_, IPPROTO_TCP, TCP_CORK, &off);
#elif defined(TCP_NOPUSH)
        setSockOptInt(fd_, IPPROTO_TCP, TCP_NOPUSH, &off);
#endif
        mode_ = Mode::NODELAY;
        return true;
    }

    case Mode::CORK: {
        // Disable TCP_NODELAY, enable TCP_CORK/TCP_NOPUSH.
        setSockOptInt(fd_, IPPROTO_TCP, TCP_NODELAY, &off);
#ifdef TCP_CORK
        if (setSockOptInt(fd_, IPPROTO_TCP, TCP_CORK, &on) != 0) return false;
#elif defined(TCP_NOPUSH)
        if (setSockOptInt(fd_, IPPROTO_TCP, TCP_NOPUSH, &on) != 0) return false;
#else
        // Platform has neither TCP_CORK nor TCP_NOPUSH; cork is a no-op.
        return false;
#endif
        mode_ = Mode::CORK;
        return true;
    }

    case Mode::DEFAULT: {
        // Clear both flags.
        setSockOptInt(fd_, IPPROTO_TCP, TCP_NODELAY, &off);
#ifdef TCP_CORK
        setSockOptInt(fd_, IPPROTO_TCP, TCP_CORK, &off);
#elif defined(TCP_NOPUSH)
        setSockOptInt(fd_, IPPROTO_TCP, TCP_NOPUSH, &off);
#endif
        mode_ = Mode::DEFAULT;
        return true;
    }
    }

    return false;
}

bool NagleController::uncork() noexcept {
    if (fd_ < 0) return false;

    // Clearing TCP_CORK/TCP_NOPUSH triggers an immediate flush of any
    // data held in the kernel's send buffer.
    [[maybe_unused]] int off = 0;
#ifdef TCP_CORK
    if (setSockOptInt(fd_, IPPROTO_TCP, TCP_CORK, &off) != 0)
        return false;
#elif defined(TCP_NOPUSH)
    if (setSockOptInt(fd_, IPPROTO_TCP, TCP_NOPUSH, &off) != 0)
        return false;
#else
    // Platform has neither TCP_CORK nor TCP_NOPUSH; cork/uncork is a no-op.
    // Return false to signal that the operation had no effect.
    return false;
#endif
    mode_ = Mode::DEFAULT;
    return true;
}

// =============================================================================
// WireProtocolBatcher
// =============================================================================

WireProtocolBatcher::WireProtocolBatcher(int fd, const Config& cfg)
    : fd_(fd), cfg_(cfg)
{
    // Zero-initialise the iov array so valgrind is happy.
    std::memset(iov_, 0, sizeof(iov_));
}

bool WireProtocolBatcher::add(const void* data, size_t size) {
    if (size == 0) return true; // Nothing to add.

    // Auto-flush if we would exceed the message or byte limits.
    const bool count_limit = (iov_count_ >= cfg_.max_messages_per_batch);
    const bool bytes_limit = (pending_bytes_ + size > cfg_.max_bytes_per_batch);

    if ((count_limit || bytes_limit) && cfg_.auto_flush_on_limit) {
        if (flush() < 0) return false;
        ++stats_.forced_flushes;
    }

    // Guard against iov list overflow (should not happen after auto-flush).
    if (iov_count_ >= MAX_IOV) return false;

    iov_[iov_count_].iov_base = const_cast<void*>(data);
    iov_[iov_count_].iov_len  = size;
    ++iov_count_;
    pending_bytes_ += size;
    ++stats_.messages_queued;

    return true;
}

ssize_t WireProtocolBatcher::flush() {
    if (iov_count_ == 0) return 0;

    ssize_t written = -1;
#ifdef _WIN32
    WSABUF bufs[MAX_IOV];
    for (size_t i = 0; i < iov_count_; ++i) {
        bufs[i].buf = reinterpret_cast<char*>(iov_[i].iov_base);
        bufs[i].len = static_cast<ULONG>(iov_[i].iov_len);
    }
    DWORD sent = 0;
    if (::WSASend(static_cast<SOCKET>(fd_), bufs, static_cast<DWORD>(iov_count_),
                  &sent, 0, nullptr, nullptr) == 0) {
        written = static_cast<ssize_t>(sent);
    }
#else
    std::vector<iovec> posix_iov;
    posix_iov.reserve(iov_count_);
    for (size_t i = 0; i < iov_count_; ++i) {
        iovec item;
        item.iov_base = iov_[i].iov_base;
        item.iov_len = iov_[i].iov_len;
        posix_iov.push_back(item);
    }
    written = ::writev(fd_, posix_iov.data(), static_cast<int>(posix_iov.size()));
#endif

    if (written >= 0) {
        ++stats_.batches_flushed;
        stats_.bytes_flushed += static_cast<uint64_t>(written);
    } else {
        ++stats_.flush_errors;
    }

    // Always clear the iov list so the batcher is ready for the next batch.
    iov_count_    = 0;
    pending_bytes_ = 0;

    return written;
}

} // namespace network
} // namespace themis

