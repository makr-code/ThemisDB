/**
 * @file io_uring_batcher.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.10
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "network/wire_protocol_batch.h"

#include <cstddef>
#include <cstdint>
#include <vector>

#ifdef __linux__
#  include <sys/uio.h>
#endif

namespace themis {
namespace network {

/**
 * @brief Statistics for a single submit-and-reap round.
 */
struct IoUringBatchStats {
    size_t   sqes_submitted  = 0; ///< SQEs queued in this round
    size_t   cqes_reaped     = 0; ///< CQEs reaped (completed operations)
    size_t   bytes_sent      = 0; ///< Total bytes confirmed written
    size_t   errors          = 0; ///< Operations that returned an error
    uint64_t rounds          = 0; ///< Total submitAndWait() calls
};

/**
 * @brief Batched sender that amortises syscall overhead across connections.
 *
 * Usage:
 * @code
 *   IoUringBatchedSender sender(256); // queue_depth
 *   if (!sender.isAvailable()) {
 *       // fall back to individual WireProtocolBatcher::flush() calls
 *   }
 *   for (auto& batcher : active_batchers) {
 *       sender.enqueue(batcher);
 *   }
 *   sender.submitAndWait();           // one io_uring_enter() for all
 *   auto stats = sender.lastStats();  // inspect results
 * @endcode
 *
 * When io_uring is not available `isAvailable()` returns false and
 * `enqueue()` immediately calls `WireProtocolBatcher::flush()` so the
 * caller's loop body stays identical in both paths.
 */
class IoUringBatchedSender {
public:
    /**
     * @brief Construct and, if io_uring is available, initialise the ring.
     *
     * @param queue_depth  Number of SQ/CQ entries to allocate.  Must be a
     *                     power of two; values that are not will be rounded
     *                     up by the kernel.  128–512 is typical.
     */
    explicit IoUringBatchedSender(unsigned queue_depth = 256);

    ~IoUringBatchedSender();

    // Non-copyable, non-movable (owns kernel resources).
    IoUringBatchedSender(const IoUringBatchedSender&)            = delete;
    IoUringBatchedSender& operator=(const IoUringBatchedSender&) = delete;

    /**
     * @brief Whether io_uring was successfully initialised.
     *
     * When false, `enqueue()` falls back to synchronous `writev(2)`.
     */
    bool isAvailable() const noexcept { return ring_fd_ >= 0; }

    /**
     * @brief Queue all pending iov entries of @p batcher for the next send round.
     *
     * If io_uring is unavailable the pending data is written immediately via
     * `WireProtocolBatcher::flush()` and the return value mirrors that call.
     *
     * @param batcher  A batcher with pending data (`batcher.pending() == true`).
     * @return true on success; false if the SQ ring is full or flush failed.
     */
    bool enqueue(WireProtocolBatcher& batcher);

    /**
     * @brief Submit all queued SQEs and wait for all CQEs to complete.
     *
     * Blocks until every previously enqueued operation has been acknowledged
     * by the kernel.  This is the only syscall issued for the whole batch.
     *
     * No-op (returns immediately) when io_uring is unavailable — all sends
     * have already been executed synchronously inside `enqueue()`.
     *
     * @return Number of bytes successfully written across all operations.
     *         Individual operation errors are counted in `lastStats().errors`.
     */
    size_t submitAndWait();

    /**
     * @brief Statistics from the most recent `submitAndWait()` call.
     */
    const IoUringBatchStats& lastStats() const noexcept { return stats_; }

    /**
     * @brief Cumulative statistics since construction.
     */
    const IoUringBatchStats& totalStats() const noexcept { return total_stats_; }

private:
    int      ring_fd_      = -1;  ///< io_uring file descriptor (-1 = unavailable)
    unsigned queue_depth_  = 0;

    // Mmap'd submission/completion ring pointers (null when unavailable).
    void* sq_ring_   = nullptr;
    void* cq_ring_   = nullptr;
    void* sqes_mmap_ = nullptr;

    // Ring sizes (bytes) for munmap on destruction.
    size_t sq_ring_sz_   = 0;
    size_t cq_ring_sz_   = 0;
    size_t sqes_mmap_sz_ = 0;

    // Pending iov arrays — kept alive until submitAndWait() returns.
    struct PendingEntry {
        int                   fd = 0;
        std::vector<::iovec>  iovs;
    };
    std::vector<PendingEntry> pending_;

    IoUringBatchStats stats_;        ///< Last round
    IoUringBatchStats total_stats_;  ///< Cumulative

    // io_uring ring offsets/indices (accessed via mapped memory).
    struct SqRing {
        uint32_t* head        = nullptr;
        uint32_t* tail        = nullptr;
        uint32_t* ring_mask   = nullptr;
        uint32_t* ring_entries= nullptr;
        uint32_t* array       = nullptr;
    } sq_;

    struct CqRing {
        uint32_t* head        = nullptr;
        uint32_t* tail        = nullptr;
        uint32_t* ring_mask   = nullptr;
        uint32_t* ring_entries= nullptr;
        // cqe array is 16 bytes per entry: {user_data(8), res(4), flags(4)}
        uint8_t*  cqes        = nullptr;
    } cq_;

    // Pointer into the sqes_mmap_ region — one 64-byte SQE per slot.
    uint8_t* sqe_base_ = nullptr;

    bool initRing(unsigned queue_depth);
    void teardownRing() noexcept;

    // Enqueue a single IORING_OP_WRITEV SQE for (fd, iovs).
    // Returns false when the SQ ring is full.
    bool enqueueSqe(int fd, const ::iovec* iovs, size_t iov_cnt, uint64_t user_data);
};

} // namespace network
} // namespace themis
