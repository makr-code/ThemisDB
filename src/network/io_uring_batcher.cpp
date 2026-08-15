/**
 * @file io_uring_batcher.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.10
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "network/io_uring_batcher.h"
#include "utils/logger.h"

#include <cerrno>
#include <cstring>
#include <stdexcept>

// ---- Platform guards -------------------------------------------------------
//
// The io_uring path requires:
//   1. Linux (POSIX mmap / syscall interface)
//   2. THEMIS_ENABLE_IO_URING compile flag
//   3. Kernel headers that expose io_uring structures
//
// Everything else compiles to a thin no-op shim.
// ---------------------------------------------------------------------------

#ifdef __linux__
#  include <sys/mman.h>
#  include <sys/socket.h>
#  include <sys/uio.h>
#  include <unistd.h>
#endif

#ifdef THEMIS_ENABLE_IO_URING
#  ifdef __linux__
#    include <sys/syscall.h>
#    include <signal.h>
#    include <linux/io_uring.h>

// Thin wrappers around io_uring syscalls (not in glibc < 2.36).
static int io_uring_setup(unsigned entries, struct io_uring_params* p) {
    return static_cast<int>(::syscall(__NR_io_uring_setup, entries, p));
}
static int io_uring_enter(int fd, unsigned to_submit, unsigned min_complete,
                           unsigned flags, sigset_t* sig) {
    return static_cast<int>(::syscall(__NR_io_uring_enter, fd,
                                      to_submit, min_complete, flags, sig,
                                      _NSIG / 8));
}
#  endif // __linux__
#endif // THEMIS_ENABLE_IO_URING

namespace themis {
namespace network {

// ============================================================================
// Construction / destruction
// ============================================================================

IoUringBatchedSender::IoUringBatchedSender(unsigned queue_depth)
    : queue_depth_(queue_depth)
{
#if defined(THEMIS_ENABLE_IO_URING) && defined(__linux__)
    if (!initRing(queue_depth)) {
        THEMIS_WARN("IoUringBatchedSender: ring init failed (kernel too old?); "
                    "falling back to writev(2)");
        teardownRing();
    }
#endif
}

IoUringBatchedSender::~IoUringBatchedSender() {
    teardownRing();
}

// ============================================================================
// enqueue
// ============================================================================

bool IoUringBatchedSender::enqueue(WireProtocolBatcher& batcher) {
    if (!batcher.pending()) return true;

#if defined(THEMIS_ENABLE_IO_URING) && defined(__linux__)
    if (ring_fd_ >= 0) {
        // Build an iovec array from the batcher's pending entries.
        // WireProtocolBatcher exposes its internal iov list indirectly through
        // flush(); we snapshot the iovecs by asking the batcher to hand them
        // over to us instead of calling writev directly.
        //
        // Since WireProtocolBatcher stores its iovecs as BatchIOVec (void*,
        // size_t) — compatible with struct iovec — we copy them into a
        // heap-allocated vector so they stay alive until submitAndWait().
        //
        // Implementation note: we cannot reach into WireProtocolBatcher's
        // private iov_ array from here, so we flush it synchronously when the
        // batcher does not yet support the async handoff path.  Future work:
        // add a `takePendingIovs()` method to WireProtocolBatcher.
        //
        // For now: fall through to the synchronous path below.
        // This ensures correctness while allowing the ring setup and stats
        // infrastructure to be tested in isolation.
    }
#endif

    // Fallback: synchronous writev(2) via the existing batcher flush.
    ssize_t n = batcher.flush();
    if (n < 0) {
        ++stats_.errors;
        ++total_stats_.errors;
        return false;
    }
    stats_.bytes_sent += static_cast<size_t>(n);
    total_stats_.bytes_sent += static_cast<size_t>(n);
    ++stats_.sqes_submitted;
    ++total_stats_.sqes_submitted;
    return true;
}

// ============================================================================
// submitAndWait
// ============================================================================

size_t IoUringBatchedSender::submitAndWait() {
#if defined(THEMIS_ENABLE_IO_URING) && defined(__linux__)
    if (ring_fd_ >= 0 && !pending_.empty()) {
        // -----------------------------------------------------------------
        // Step 1: Populate SQEs
        // -----------------------------------------------------------------
        size_t submitted = 0;
        for (size_t i = 0; i < pending_.size(); ++i) {
            auto& entry = pending_[i];
            if (enqueueSqe(entry.fd, entry.iovs.data(), entry.iovs.size(),
                           static_cast<uint64_t>(i))) {
                ++submitted;
            }
        }

        // -----------------------------------------------------------------
        // Step 2: Submit + wait for all completions in one syscall
        // -----------------------------------------------------------------
        if (submitted > 0) {
            int ret = io_uring_enter(ring_fd_,
                                     static_cast<unsigned>(submitted),
                                     static_cast<unsigned>(submitted),
                                     IORING_ENTER_GETEVENTS, nullptr);
            if (ret < 0) {
                THEMIS_ERROR("io_uring_enter failed: {}", std::strerror(errno));
                stats_.errors += submitted;
                total_stats_.errors += submitted;
            } else {
                // ---------------------------------------------------------
                // Step 3: Reap CQEs
                // ---------------------------------------------------------
                uint32_t head  = *cq_.head;
                uint32_t tail  = *cq_.tail;
                uint32_t mask  = *cq_.ring_mask;

                while (head != tail) {
                    // Each CQE is 16 bytes: user_data(8), res(4), flags(4).
                    // Use offsetof to safely compute the res field offset without
                    // depending on a hard-coded magic number.
                    const uint8_t* cqe = cq_.cqes +
                        (head & mask) * sizeof(struct io_uring_cqe);
                    int32_t res;
                    static constexpr size_t CQE_RES_OFFSET =
                        offsetof(struct io_uring_cqe, res);
                    std::memcpy(&res, cqe + CQE_RES_OFFSET, sizeof(res));

                    ++stats_.cqes_reaped;
                    ++total_stats_.cqes_reaped;
                    if (res < 0) {
                        ++stats_.errors;
                        ++total_stats_.errors;
                        THEMIS_WARN("io_uring CQE error: {}", std::strerror(-res));
                    } else {
                        stats_.bytes_sent += static_cast<size_t>(res);
                        total_stats_.bytes_sent += static_cast<size_t>(res);
                    }
                    ++head;
                }
                // Advance the CQ head to free consumed entries.
                *cq_.head = head;

                stats_.sqes_submitted += submitted;
                total_stats_.sqes_submitted += submitted;
            }
        }

        pending_.clear();
        ++stats_.rounds;
        ++total_stats_.rounds;
        return stats_.bytes_sent;
    }
#endif

    // Fallback: all sends were already executed synchronously in enqueue().
    ++stats_.rounds;
    ++total_stats_.rounds;
    return stats_.bytes_sent;
}

// ============================================================================
// enqueueSqe  (io_uring path only)
// ============================================================================

bool IoUringBatchedSender::enqueueSqe([[maybe_unused]] int fd, [[maybe_unused]] const ::iovec* iovs,
                                       [[maybe_unused]] size_t iov_cnt, [[maybe_unused]] uint64_t user_data) {
#if defined(THEMIS_ENABLE_IO_URING) && defined(__linux__)
    if (ring_fd_ < 0 || !sqe_base_) return false;

    uint32_t tail = *sq_.tail;
    uint32_t next = tail + 1;
    if (next - *sq_.head > *sq_.ring_entries) {
        // SQ ring full.
        return false;
    }

    // R13: Add bounds checks before all memcpy operations into SQE buffer.
    // SQE is a fixed 64-byte structure, so we must ensure all writes stay within bounds.
    const size_t SQE_SIZE = sizeof(struct io_uring_sqe);
    uint8_t* sqe = sqe_base_ + (tail & *sq_.ring_mask) * SQE_SIZE;

    // Verify SQE pointer is within allocated SQE buffer bounds
    if (sqe < sqe_base_ || sqe + SQE_SIZE > sqe_base_ + sqes_mmap_sz_) {
        // SQE pointer out of bounds: prevent buffer overflow
        return false;
    }

    std::memset(sqe, 0, SQE_SIZE);

    // SQE layout (64 bytes):
    //  0  opcode    (uint8)
    //  1  flags     (uint8)
    //  2  ioprio    (uint16)
    //  4  fd        (int32)
    //  8  off/addr2 (uint64)
    // 16  addr      (uint64)  — pointer to iovec array
    // 24  len       (uint32)  — number of iovec entries
    // 28  op_flags  (uint32)
    // 32  user_data (uint64)
    // 40  pad[3]    (uint64 * 3)

    // Bounds check all memcpy operations
    sqe[0] = IORING_OP_WRITEV;               // opcode (offset 0, size 1)
    
    // Write fd at offset 4, size 4
    if (4 + sizeof(int32_t) > SQE_SIZE) {
        return false;
    }
    int32_t fd_i = fd;
    std::memcpy(sqe + 4, &fd_i, sizeof(fd_i));
    
    // Write addr at offset 16, size sizeof(uintptr_t)
    if (16 + sizeof(uintptr_t) > SQE_SIZE) {
        return false;
    }
    auto addr = reinterpret_cast<uintptr_t>(iovs);
    std::memcpy(sqe + 16, &addr, sizeof(addr));
    
    // Write len at offset 24, size 4
    if (24 + sizeof(uint32_t) > SQE_SIZE) {
        return false;
    }
    uint32_t len = static_cast<uint32_t>(iov_cnt);
    std::memcpy(sqe + 24, &len, sizeof(len));
    
    // Write user_data at offset 32, size 8
    if (32 + sizeof(uint64_t) > SQE_SIZE) {
        return false;
    }
    std::memcpy(sqe + 32, &user_data, sizeof(user_data));

    sq_.array[tail & *sq_.ring_mask] = tail & *sq_.ring_mask;
    *sq_.tail = next;
    return true;
#else
    return false;
#endif
}

// ============================================================================
// initRing / teardownRing
// ============================================================================

bool IoUringBatchedSender::initRing([[maybe_unused]] unsigned queue_depth) {
#if defined(THEMIS_ENABLE_IO_URING) && defined(__linux__)
    struct io_uring_params params;
    std::memset(&params, 0, sizeof(params));

    ring_fd_ = io_uring_setup(queue_depth, &params);
    if (ring_fd_ < 0) {
        THEMIS_WARN("io_uring_setup failed: {}", std::strerror(errno));
        ring_fd_ = -1;
        return false;
    }

    // Map submission ring.
    sq_ring_sz_ = params.sq_off.array +
                  params.sq_entries * sizeof(uint32_t);
    sq_ring_ = ::mmap(nullptr, sq_ring_sz_,
                      PROT_READ | PROT_WRITE,
                      MAP_SHARED | MAP_POPULATE,
                      ring_fd_, IORING_OFF_SQ_RING);
    if (sq_ring_ == MAP_FAILED) {
        THEMIS_WARN("mmap SQ ring failed: {}", std::strerror(errno));
        sq_ring_ = nullptr;
        ::close(ring_fd_); ring_fd_ = -1;
        return false;
    }

    // Map SQE array.
    sqes_mmap_sz_ = params.sq_entries * sizeof(struct io_uring_sqe);
    sqes_mmap_ = ::mmap(nullptr, sqes_mmap_sz_,
                        PROT_READ | PROT_WRITE,
                        MAP_SHARED | MAP_POPULATE,
                        ring_fd_, IORING_OFF_SQES);
    if (sqes_mmap_ == MAP_FAILED) {
        THEMIS_WARN("mmap SQEs failed: {}", std::strerror(errno));
        sqes_mmap_ = nullptr;
        ::munmap(sq_ring_, sq_ring_sz_); sq_ring_ = nullptr;
        ::close(ring_fd_); ring_fd_ = -1;
        return false;
    }

    // Map completion ring.
    cq_ring_sz_ = params.cq_off.cqes +
                  params.cq_entries * sizeof(struct io_uring_cqe);
    cq_ring_ = ::mmap(nullptr, cq_ring_sz_,
                      PROT_READ | PROT_WRITE,
                      MAP_SHARED | MAP_POPULATE,
                      ring_fd_, IORING_OFF_CQ_RING);
    if (cq_ring_ == MAP_FAILED) {
        THEMIS_WARN("mmap CQ ring failed: {}", std::strerror(errno));
        cq_ring_ = nullptr;
        ::munmap(sqes_mmap_, sqes_mmap_sz_); sqes_mmap_ = nullptr;
        ::munmap(sq_ring_,   sq_ring_sz_);   sq_ring_   = nullptr;
        ::close(ring_fd_); ring_fd_ = -1;
        return false;
    }

    // Wire up pointers into the mapped memory.
    auto* sqb = static_cast<uint8_t*>(sq_ring_);
    sq_.head         = reinterpret_cast<uint32_t*>(sqb + params.sq_off.head);
    sq_.tail         = reinterpret_cast<uint32_t*>(sqb + params.sq_off.tail);
    sq_.ring_mask    = reinterpret_cast<uint32_t*>(sqb + params.sq_off.ring_mask);
    sq_.ring_entries = reinterpret_cast<uint32_t*>(sqb + params.sq_off.ring_entries);
    sq_.array        = reinterpret_cast<uint32_t*>(sqb + params.sq_off.array);
    sqe_base_        = static_cast<uint8_t*>(sqes_mmap_);

    auto* cqb = static_cast<uint8_t*>(cq_ring_);
    cq_.head         = reinterpret_cast<uint32_t*>(cqb + params.cq_off.head);
    cq_.tail         = reinterpret_cast<uint32_t*>(cqb + params.cq_off.tail);
    cq_.ring_mask    = reinterpret_cast<uint32_t*>(cqb + params.cq_off.ring_mask);
    cq_.ring_entries = reinterpret_cast<uint32_t*>(cqb + params.cq_off.ring_entries);
    cq_.cqes         = cqb + params.cq_off.cqes;

    THEMIS_INFO("IoUringBatchedSender: ring initialised (fd={}, depth={})",
                ring_fd_, params.sq_entries);
    return true;
#else
    return false;
#endif
}

void IoUringBatchedSender::teardownRing() noexcept {
#if defined(THEMIS_ENABLE_IO_URING) && defined(__linux__)
    if (cq_ring_   && cq_ring_   != MAP_FAILED)
        ::munmap(cq_ring_,   cq_ring_sz_);
    if (sqes_mmap_ && sqes_mmap_ != MAP_FAILED)
        ::munmap(sqes_mmap_, sqes_mmap_sz_);
    if (sq_ring_   && sq_ring_   != MAP_FAILED)
        ::munmap(sq_ring_,   sq_ring_sz_);
    if (ring_fd_ >= 0)
        ::close(ring_fd_);
#endif
    ring_fd_      = -1;
    sq_ring_      = nullptr;
    cq_ring_      = nullptr;
    sqes_mmap_    = nullptr;
    sq_ring_sz_   = 0;
    cq_ring_sz_   = 0;
    sqes_mmap_sz_ = 0;
    sqe_base_     = nullptr;
    sq_ = {};
    cq_ = {};
}

} // namespace network
} // namespace themis

