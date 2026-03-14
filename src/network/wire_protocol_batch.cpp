/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            wire_protocol_batch.cpp                            ║
  Version:         0.0.1                                              ║
  Last Modified:   2026-03-14 06:30:00                                ║
  Author:          copilot                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// ThemisDB Wire Protocol – Batch Write Processing (implementation)

#include "network/wire_protocol_batch.h"

#include <cassert>
#include <cerrno>
#include <cstring>

namespace themis {
namespace network {

// =============================================================================
// NagleController
// =============================================================================

NagleController::NagleController(int fd) noexcept : fd_(fd) {}

bool NagleController::setMode(Mode mode) noexcept {
    if (fd_ < 0) return false;

    // Reset both flags first so we reach a clean state.
    int on  = 1;
    int off = 0;

    switch (mode) {
    case Mode::NODELAY: {
        // Enable TCP_NODELAY, disable TCP_CORK.
        if (::setsockopt(fd_, IPPROTO_TCP, TCP_NODELAY,
                         &on, sizeof(on)) != 0) return false;
#ifdef TCP_CORK
        ::setsockopt(fd_, IPPROTO_TCP, TCP_CORK, &off, sizeof(off));
#elif defined(TCP_NOPUSH)
        ::setsockopt(fd_, IPPROTO_TCP, TCP_NOPUSH, &off, sizeof(off));
#endif
        mode_ = Mode::NODELAY;
        return true;
    }

    case Mode::CORK: {
        // Disable TCP_NODELAY, enable TCP_CORK/TCP_NOPUSH.
        ::setsockopt(fd_, IPPROTO_TCP, TCP_NODELAY, &off, sizeof(off));
#ifdef TCP_CORK
        if (::setsockopt(fd_, IPPROTO_TCP, TCP_CORK,
                         &on, sizeof(on)) != 0) return false;
#elif defined(TCP_NOPUSH)
        if (::setsockopt(fd_, IPPROTO_TCP, TCP_NOPUSH,
                         &on, sizeof(on)) != 0) return false;
#endif
        mode_ = Mode::CORK;
        return true;
    }

    case Mode::DEFAULT: {
        // Clear both flags.
        ::setsockopt(fd_, IPPROTO_TCP, TCP_NODELAY, &off, sizeof(off));
#ifdef TCP_CORK
        ::setsockopt(fd_, IPPROTO_TCP, TCP_CORK, &off, sizeof(off));
#elif defined(TCP_NOPUSH)
        ::setsockopt(fd_, IPPROTO_TCP, TCP_NOPUSH, &off, sizeof(off));
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
    int off = 0;
#ifdef TCP_CORK
    if (::setsockopt(fd_, IPPROTO_TCP, TCP_CORK, &off, sizeof(off)) != 0)
        return false;
#elif defined(TCP_NOPUSH)
    if (::setsockopt(fd_, IPPROTO_TCP, TCP_NOPUSH, &off, sizeof(off)) != 0)
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

    const ssize_t written = ::writev(fd_, iov_, static_cast<int>(iov_count_));

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
