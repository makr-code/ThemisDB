/**
 * @file io_uring_zero_copy.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=56, H=2, M=5, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "performance/phase4/io_uring_zero_copy.h"

#include <cerrno>
#include <cstring>
#include <new>
#include <stdexcept>

// ---------------------------------------------------------------------------
// Platform-specific includes
// ---------------------------------------------------------------------------

#ifdef __linux__
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <unistd.h>
#include <time.h>
#endif // __linux__

#ifdef THEMIS_ENABLE_IO_URING
#ifdef __linux__

#include <sys/syscall.h>
#include <signal.h>
#include <linux/io_uring.h>

// Thin wrappers around io_uring syscalls not exposed by glibc < 2.36
static int io_uring_setup(unsigned entries, struct io_uring_params* p) {
    return static_cast<int>(::syscall(__NR_io_uring_setup, entries, p));
}

static int io_uring_enter(int fd, unsigned to_submit, unsigned min_complete,
                          unsigned flags, sigset_t* sig) {
    return static_cast<int>(::syscall(__NR_io_uring_enter, fd,
                                      to_submit, min_complete, flags, sig, _NSIG / 8));
}

static int io_uring_register(int fd, unsigned opcode, void* arg,
                              unsigned nr_args) {
    return static_cast<int>(::syscall(__NR_io_uring_register, fd, opcode, arg, nr_args));
}

#define IORING_REGISTER_BUFFERS  0
#define IORING_UNREGISTER_BUFFERS 1
#define IORING_REGISTER_FILES    2
#define IORING_UNREGISTER_FILES  3

#endif // __linux__
#endif // THEMIS_ENABLE_IO_URING

namespace themis {
namespace performance {
namespace phase4 {

// ===========================================================================
// ZeroCopyBuffer
// ===========================================================================

ZeroCopyBuffer::ZeroCopyBuffer(size_t size) : size_(size) {
    if (size == 0) return;
#ifdef __linux__
    // Allocate page-aligned memory so the kernel can pin it without splitting
    void* p = ::mmap(nullptr, size,
                     PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE,
                     -1, 0);
    data_ = (p == MAP_FAILED) ? nullptr : p;
#else
    data_ = ::operator new(size, std::nothrow);
#endif
}

ZeroCopyBuffer::~ZeroCopyBuffer() noexcept {
    if (!data_) return;
#ifdef __linux__
    ::munmap(data_, size_);
#else
    ::operator delete(data_);
#endif
    data_ = nullptr;
}

ZeroCopyBuffer::ZeroCopyBuffer(ZeroCopyBuffer&& other) noexcept
    : data_(other.data_), size_(other.size_) {
    other.data_ = nullptr;
    other.size_ = 0;
}

ZeroCopyBuffer& ZeroCopyBuffer::operator=(ZeroCopyBuffer&& other) noexcept {
    if (this != &other) {
        this->~ZeroCopyBuffer();
        data_ = other.data_;
        size_ = other.size_;
        other.data_ = nullptr;
        other.size_ = 0;
    }
    return *this;
}

// ===========================================================================
// IoUringZeroCopyIO::RingImpl  (Linux-only internals)
// ===========================================================================

#ifdef THEMIS_ENABLE_IO_URING
#ifdef __linux__

struct IoUringZeroCopyIO::RingImpl {
    int ring_fd{-1};

    // SQ ring
    unsigned* sq_head{nullptr};
    unsigned* sq_tail{nullptr};
    unsigned* sq_ring_mask{nullptr};
    unsigned* sq_ring_entries{nullptr};
    unsigned* sq_flags{nullptr};
    unsigned* sq_dropped{nullptr};
    unsigned* sq_array{nullptr};
    struct io_uring_sqe* sqes{nullptr};

    // CQ ring
    unsigned* cq_head{nullptr};
    unsigned* cq_tail{nullptr};
    unsigned* cq_ring_mask{nullptr};
    unsigned* cq_ring_entries{nullptr};
    struct io_uring_cqe* cqes{nullptr};

    // mmap regions
    void* sq_ptr{nullptr};
    size_t sq_mmap_size{0};
    void* sqe_ptr{nullptr};
    size_t sqe_mmap_size{0};
    void* cq_ptr{nullptr};
    size_t cq_mmap_size{0};

    // Registered file descriptors
    std::vector<int> registered_fds;
};

#else  // !__linux__

struct IoUringZeroCopyIO::RingImpl {
    int ring_fd{-1};
};

#endif // __linux__

#else // !THEMIS_ENABLE_IO_URING

struct IoUringZeroCopyIO::RingImpl {
    int ring_fd{-1};
};

#endif // THEMIS_ENABLE_IO_URING

// ===========================================================================
// IoUringZeroCopyIO – constructor / destructor
// ===========================================================================

IoUringZeroCopyIO::IoUringZeroCopyIO(const IoUringConfig& config)
    : config_(config), ring_(std::make_unique<RingImpl>())
{
    // Allocate pre-registered I/O buffers first (independent of io_uring)
    buffers_.reserve(config_.num_buffers);
    for (uint32_t i = 0; i < config_.num_buffers; ++i) {
        buffers_.emplace_back(config_.buffer_size);
    }

    setup_ring(config_);
}

IoUringZeroCopyIO::~IoUringZeroCopyIO() noexcept {
    teardown_ring();
}

// ===========================================================================
// setup_ring / teardown_ring
// ===========================================================================

void IoUringZeroCopyIO::setup_ring(const IoUringConfig& config) noexcept {
#ifdef THEMIS_ENABLE_IO_URING
#ifdef __linux__
    std::lock_guard<std::mutex> lock(ring_mutex_);
    
    struct io_uring_params params{};

    if (config.sq_poll) {
        params.flags |= IORING_SETUP_SQPOLL;
        params.sq_thread_idle = config.sq_thread_idle_ms;
        if (config.sq_thread_cpu >= 0) {
            params.flags |= IORING_SETUP_SQ_AFF;
            params.sq_thread_cpu = static_cast<uint32_t>(config.sq_thread_cpu);
        }
    }

    int fd = io_uring_setup(config.ring_size, &params);
    if (fd < 0) {
        // io_uring unavailable (old kernel, seccomp, etc.) – stay in fallback mode
        return;
    }
    ring_->ring_fd = fd;

    // ---- Map submission queue ----
    size_t sq_ring_sz = params.sq_off.array +
                        params.sq_entries * sizeof(unsigned);
    void* sq_ptr = ::mmap(nullptr, sq_ring_sz,
                          PROT_READ | PROT_WRITE,
                          MAP_SHARED | MAP_POPULATE,
                          fd, IORING_OFF_SQ_RING);
    if (sq_ptr == MAP_FAILED) {
        ::close(fd);
        ring_->ring_fd = -1;
        return;
    }
    ring_->sq_ptr       = sq_ptr;
    ring_->sq_mmap_size = sq_ring_sz;

    auto* sq_base = static_cast<char*>(sq_ptr);
    ring_->sq_head         = reinterpret_cast<unsigned*>(sq_base + params.sq_off.head);
    ring_->sq_tail         = reinterpret_cast<unsigned*>(sq_base + params.sq_off.tail);
    ring_->sq_ring_mask    = reinterpret_cast<unsigned*>(sq_base + params.sq_off.ring_mask);
    ring_->sq_ring_entries = reinterpret_cast<unsigned*>(sq_base + params.sq_off.ring_entries);
    ring_->sq_flags        = reinterpret_cast<unsigned*>(sq_base + params.sq_off.flags);
    ring_->sq_dropped      = reinterpret_cast<unsigned*>(sq_base + params.sq_off.dropped);
    ring_->sq_array        = reinterpret_cast<unsigned*>(sq_base + params.sq_off.array);

    // ---- Map SQEs ----
    size_t sqe_sz = params.sq_entries * sizeof(struct io_uring_sqe);
    void* sqe_ptr = ::mmap(nullptr, sqe_sz,
                           PROT_READ | PROT_WRITE,
                           MAP_SHARED | MAP_POPULATE,
                           fd, IORING_OFF_SQES);
    if (sqe_ptr == MAP_FAILED) {
        ::munmap(sq_ptr, sq_ring_sz);
        ::close(fd);
        ring_->ring_fd  = -1;
        ring_->sq_ptr   = nullptr;
        return;
    }
    ring_->sqe_ptr      = sqe_ptr;
    ring_->sqe_mmap_size = sqe_sz;
    ring_->sqes = static_cast<struct io_uring_sqe*>(sqe_ptr);

    // ---- Map completion queue ----
    size_t cq_ring_sz = params.cq_off.cqes +
                        params.cq_entries * sizeof(struct io_uring_cqe);
    void* cq_ptr = ::mmap(nullptr, cq_ring_sz,
                          PROT_READ | PROT_WRITE,
                          MAP_SHARED | MAP_POPULATE,
                          fd, IORING_OFF_CQ_RING);
    if (cq_ptr == MAP_FAILED) {
        ::munmap(sqe_ptr, sqe_sz);
        ::munmap(sq_ptr,  sq_ring_sz);
        ::close(fd);
        ring_->ring_fd  = -1;
        ring_->sq_ptr   = nullptr;
        ring_->sqe_ptr  = nullptr;
        return;
    }
    ring_->cq_ptr       = cq_ptr;
    ring_->cq_mmap_size = cq_ring_sz;

    auto* cq_base = static_cast<char*>(cq_ptr);
    ring_->cq_head         = reinterpret_cast<unsigned*>(cq_base + params.cq_off.head);
    ring_->cq_tail         = reinterpret_cast<unsigned*>(cq_base + params.cq_off.tail);
    ring_->cq_ring_mask    = reinterpret_cast<unsigned*>(cq_base + params.cq_off.ring_mask);
    ring_->cq_ring_entries = reinterpret_cast<unsigned*>(cq_base + params.cq_off.ring_entries);
    ring_->cqes = reinterpret_cast<struct io_uring_cqe*>(cq_base + params.cq_off.cqes);

    // ---- Register buffers ----
    if (!register_buffers()) {
        // Non-fatal: we can still use non-registered sends
    }

    available_ = true;
#endif // __linux__
#endif // THEMIS_ENABLE_IO_URING
}

void IoUringZeroCopyIO::teardown_ring() noexcept {
#ifdef THEMIS_ENABLE_IO_URING
#ifdef __linux__
    std::lock_guard<std::mutex> lock(ring_mutex_);
    
    if (!ring_) return;
    if (ring_->cq_ptr)  ::munmap(ring_->cq_ptr,  ring_->cq_mmap_size);
    if (ring_->sqe_ptr) ::munmap(ring_->sqe_ptr, ring_->sqe_mmap_size);
    if (ring_->sq_ptr)  ::munmap(ring_->sq_ptr,  ring_->sq_mmap_size);
    if (ring_->ring_fd >= 0) ::close(ring_->ring_fd);
    ring_->ring_fd  = -1;
    ring_->sq_ptr   = nullptr;
    ring_->sqe_ptr  = nullptr;
    ring_->cq_ptr   = nullptr;
#endif
#endif
}

bool IoUringZeroCopyIO::register_buffers() noexcept {
#ifdef THEMIS_ENABLE_IO_URING
#ifdef __linux__
    // Note: called from setup_ring() which already holds ring_mutex_
    if (ring_->ring_fd < 0 || buffers_.empty()) return false;

    std::vector<struct iovec> iovecs(buffers_.size());
    for (size_t i = 0; i < buffers_.size(); ++i) {
        iovecs[i].iov_base = buffers_[i].data();
        iovecs[i].iov_len  = buffers_[i].size();
    }

    int ret = io_uring_register(ring_->ring_fd,
                                IORING_REGISTER_BUFFERS,
                                iovecs.data(),
                                static_cast<unsigned>(iovecs.size()));
    return ret == 0;
#else
    return false;
#endif
#else
    return false;
#endif
}

// ===========================================================================
// Public API
// ===========================================================================

bool IoUringZeroCopyIO::register_fd(int fd) noexcept {
#ifdef THEMIS_ENABLE_IO_URING
#ifdef __linux__
    std::lock_guard<std::mutex> lock(ring_mutex_);
    
    if (!available_ || !config_.fixed_files) return false;
    ring_->registered_fds.push_back(fd);
    int ret = io_uring_register(ring_->ring_fd,
                                IORING_REGISTER_FILES,
                                ring_->registered_fds.data(),
                                static_cast<unsigned>(ring_->registered_fds.size()));
    if (ret != 0) {
        ring_->registered_fds.pop_back();
        return false;
    }
    return true;
#endif
#endif
    return false;
}

int IoUringZeroCopyIO::send_zerocopy(int fd, uint32_t buf_index, size_t len) noexcept {
    if (buf_index >= buffers_.size()) return -EINVAL;

#ifdef THEMIS_ENABLE_IO_URING
#ifdef __linux__
    {
        std::lock_guard<std::mutex> lock(ring_mutex_);
        
        if (available_ && ring_->ring_fd >= 0) {
            unsigned tail   = *ring_->sq_tail;
            unsigned mask   = *ring_->sq_ring_mask;
            unsigned index  = tail & mask;

            struct io_uring_sqe* sqe = &ring_->sqes[index];
            std::memset(sqe, 0, sizeof(*sqe));
            sqe->opcode      = IORING_OP_SEND;
            sqe->fd          = fd;
            sqe->addr        = reinterpret_cast<uint64_t>(buffers_[buf_index].data());
            sqe->len         = static_cast<uint32_t>(len);
            sqe->buf_index   = static_cast<uint16_t>(buf_index);
            // Use IORING_RECVSEND_FIXED_BUF when available (kernel ≥ 5.17)
#ifdef IORING_RECVSEND_FIXED_BUF
            sqe->ioprio      = IORING_RECVSEND_FIXED_BUF;
#endif
            sqe->user_data   = static_cast<uint64_t>(buf_index);

            ring_->sq_array[index] = index;
            // Store-release so the kernel sees our SQE before we advance the tail
            __atomic_store_n(ring_->sq_tail, tail + 1, __ATOMIC_RELEASE);

            int submitted = submit_sqes();
            if (submitted >= 0) {
                sq_submitted_.fetch_add(1, std::memory_order_relaxed);
                bytes_sent_.fetch_add(len, std::memory_order_relaxed);
                return 0;
            }
            // Fall through to fallback on submission error
        }
    }
#endif // __linux__
#endif // THEMIS_ENABLE_IO_URING

    // Fallback: standard blocking send()
    ssize_t ret = ::send(fd, buffers_[buf_index].data(), len,
#ifdef MSG_NOSIGNAL
                         MSG_NOSIGNAL
#else
                         0
#endif
                         );
    if (ret < 0) return -errno;
    fallback_sends_.fetch_add(1, std::memory_order_relaxed);
    bytes_sent_.fetch_add(static_cast<uint64_t>(ret), std::memory_order_relaxed);
    return 0;
}

int IoUringZeroCopyIO::recv_zerocopy(int fd, uint32_t buf_index, size_t max_len) noexcept {
    if (buf_index >= buffers_.size()) return -EINVAL;

#ifdef THEMIS_ENABLE_IO_URING
#ifdef __linux__
    {
        std::lock_guard<std::mutex> lock(ring_mutex_);
        
        if (available_ && ring_->ring_fd >= 0) {
            unsigned tail   = *ring_->sq_tail;
            unsigned mask   = *ring_->sq_ring_mask;
            unsigned index  = tail & mask;

            struct io_uring_sqe* sqe = &ring_->sqes[index];
            std::memset(sqe, 0, sizeof(*sqe));
            sqe->opcode    = IORING_OP_RECV;
            sqe->fd        = fd;
            sqe->addr      = reinterpret_cast<uint64_t>(buffers_[buf_index].data());
            sqe->len       = static_cast<uint32_t>(max_len);
            sqe->buf_index = static_cast<uint16_t>(buf_index);
#ifdef IORING_RECVSEND_FIXED_BUF
            sqe->ioprio    = IORING_RECVSEND_FIXED_BUF;
#endif
            sqe->user_data = static_cast<uint64_t>(buf_index);

            ring_->sq_array[index] = index;
            __atomic_store_n(ring_->sq_tail, tail + 1, __ATOMIC_RELEASE);

            int submitted = submit_sqes();
            if (submitted >= 0) {
                sq_submitted_.fetch_add(1, std::memory_order_relaxed);
                return 0;
            }
        }
    }
#endif // __linux__
#endif // THEMIS_ENABLE_IO_URING

    // Fallback: standard blocking recv()
    ssize_t ret = ::recv(fd, buffers_[buf_index].data(), max_len, 0);
    if (ret < 0) return -errno;
    fallback_recvs_.fetch_add(1, std::memory_order_relaxed);
    bytes_received_.fetch_add(static_cast<uint64_t>(ret), std::memory_order_relaxed);
    return static_cast<int>(ret);
}

uint32_t IoUringZeroCopyIO::wait_completions(uint32_t min_completions) noexcept {
#ifdef THEMIS_ENABLE_IO_URING
#ifdef __linux__
    {
        std::lock_guard<std::mutex> lock(ring_mutex_);
        
        if (!available_ || ring_->ring_fd < 0) return 0;

        // Call io_uring_enter with IORING_ENTER_GETEVENTS to wait
        int ret = io_uring_enter(ring_->ring_fd, 0, min_completions,
                                 IORING_ENTER_GETEVENTS, nullptr);
        if (ret < 0) return 0;

        uint32_t count = 0;
        unsigned head = *ring_->cq_head;
        unsigned mask = *ring_->cq_ring_mask;

        __atomic_thread_fence(__ATOMIC_ACQUIRE);

        while (head != *ring_->cq_tail) {
            struct io_uring_cqe* cqe = &ring_->cqes[head & mask];
            if (cqe->res > 0) {
                bytes_received_.fetch_add(static_cast<uint64_t>(cqe->res),
                                          std::memory_order_relaxed);
            }
            ++head;
            ++count;
        }
        __atomic_store_n(ring_->cq_head, head, __ATOMIC_RELEASE);
        cq_completed_.fetch_add(count, std::memory_order_relaxed);
        return count;
    }
#endif
#endif
    return 0;
}

int IoUringZeroCopyIO::submit_sqes() noexcept {
#ifdef THEMIS_ENABLE_IO_URING
#ifdef __linux__
    // Must be called with ring_mutex_ already held by caller
    if (ring_->ring_fd < 0) return -1;
    // SQ-poll mode: kernel drains the SQ automatically – no need to call enter
    if (config_.sq_poll) {
        // Signal the SQ poll thread if it has gone idle
        if (*ring_->sq_flags & IORING_SQ_NEED_WAKEUP) {
            return io_uring_enter(ring_->ring_fd, 1, 0,
                                  IORING_ENTER_SQ_WAKEUP, nullptr);
        }
        return 1; // SQ poll thread is running
    }
    return io_uring_enter(ring_->ring_fd, 1, 0, 0, nullptr);
#endif
#endif
    return -1;
}

ZeroCopyBuffer& IoUringZeroCopyIO::get_buffer(uint32_t index) {
    return buffers_.at(index);
}

const ZeroCopyBuffer& IoUringZeroCopyIO::get_buffer(uint32_t index) const {
    return buffers_.at(index);
}

IoUringStats IoUringZeroCopyIO::get_stats() const noexcept {
    IoUringStats s;
    s.sq_entries_submitted  = sq_submitted_.load(std::memory_order_relaxed);
    s.cq_entries_completed  = cq_completed_.load(std::memory_order_relaxed);
    s.bytes_sent            = bytes_sent_.load(std::memory_order_relaxed);
    s.bytes_received        = bytes_received_.load(std::memory_order_relaxed);
    s.registered_buffers    = static_cast<uint64_t>(buffers_.size());
    s.fallback_sends        = fallback_sends_.load(std::memory_order_relaxed);
    s.fallback_recvs        = fallback_recvs_.load(std::memory_order_relaxed);
    s.io_uring_available    = available_;
    return s;
}

// static
bool IoUringZeroCopyIO::io_uring_accessible() noexcept {
#ifdef THEMIS_ENABLE_IO_URING
#ifdef __linux__
    struct io_uring_params params{};
    int fd = io_uring_setup(1, &params); // minimal ring
    if (fd >= 0) {
        ::close(fd);
        return true;
    }
#endif
#endif
    return false;
}

// ===========================================================================
// ScopedIoUringTimer
// ===========================================================================

ScopedIoUringTimer::ScopedIoUringTimer(uint64_t* output_ns) noexcept
    : output_ns_(output_ns), start_ns_(0)
{
#ifdef __linux__
    struct timespec ts{};
    ::clock_gettime(CLOCK_MONOTONIC, &ts);
    start_ns_ = static_cast<uint64_t>(ts.tv_sec) * 1'000'000'000ULL +
                static_cast<uint64_t>(ts.tv_nsec);
#endif
}

ScopedIoUringTimer::~ScopedIoUringTimer() noexcept {
    if (!output_ns_) return;
#ifdef __linux__
    struct timespec ts{};
    ::clock_gettime(CLOCK_MONOTONIC, &ts);
    uint64_t end_ns = static_cast<uint64_t>(ts.tv_sec) * 1'000'000'000ULL +
                      static_cast<uint64_t>(ts.tv_nsec);
    *output_ns_ = (end_ns >= start_ns_) ? (end_ns - start_ns_) : 0;
#else
    *output_ns_ = 0;
#endif
}

} // namespace phase4
} // namespace performance
} // namespace themis

