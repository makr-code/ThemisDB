/**
 * @file wire_protocol_performance.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=1, M=4, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

// ThemisDB Wire Protocol v1 Performance Components
// WireProtocolMetrics, PayloadBufferPool, CompressionAdvisor

#include "network/wire_protocol_performance.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <numeric>
#include <stdexcept>

namespace themis {
namespace network {

// =============================================================================
// WireProtocolMetrics
// =============================================================================

WireProtocolMetrics::WireProtocolMetrics(const Config &cfg) : cfg_(cfg) {
    latency_samples_.resize(cfg_.max_samples, 0.0);
}

void WireProtocolMetrics::recordLatency(std::chrono::steady_clock::time_point started) {
    auto now  = std::chrono::steady_clock::now();
    double ms = std::chrono::duration<double, std::milli>(now - started).count();
    recordLatencyMs(ms);
}

void WireProtocolMetrics::recordLatencyMs([[maybe_unused]] double ms) {
    requests_total_.fetch_add(1, std::memory_order_relaxed);
    std::lock_guard<std::mutex> lock(latency_mutex_);
    latency_samples_[write_pos_] = ms;
    write_pos_                   = (write_pos_ + 1) % cfg_.max_samples;
    if (write_pos_ == 0)
        buffer_full_ = true;
}

void WireProtocolMetrics::recordBytes(uint64_t received, uint64_t sent) {
    bytes_received_total_.fetch_add(received, std::memory_order_relaxed);
    bytes_sent_total_.fetch_add(sent, std::memory_order_relaxed);
}

void WireProtocolMetrics::recordCompression(uint64_t original_bytes, uint64_t compressed_bytes) {
    compressed_payloads_.fetch_add(1, std::memory_order_relaxed);
    original_bytes_total_.fetch_add(original_bytes, std::memory_order_relaxed);
    compressed_bytes_total_.fetch_add(compressed_bytes, std::memory_order_relaxed);
}

void WireProtocolMetrics::recordError(const char *kind) {
    errors_total_.fetch_add(1, std::memory_order_relaxed);
    if (!kind)
        return;
    std::string k(kind);
    if (k == "connection")
        connection_errors_.fetch_add(1, std::memory_order_relaxed);
    else if (k == "timeout")
        timeout_errors_.fetch_add(1, std::memory_order_relaxed);
    else if (k == "parse")
        parse_errors_.fetch_add(1, std::memory_order_relaxed);
    else if (k == "auth")
        auth_errors_.fetch_add(1, std::memory_order_relaxed);
}

WireProtocolMetrics::Snapshot WireProtocolMetrics::snapshot() const {
    Snapshot snap;
    snap.captured_at = std::chrono::steady_clock::now();

    // ── Throughput ─────────────────────────────────────────────────────
    snap.throughput.bytes_received_total = bytes_received_total_.load(std::memory_order_relaxed);
    snap.throughput.bytes_sent_total     = bytes_sent_total_.load(std::memory_order_relaxed);
    snap.throughput.requests_total       = requests_total_.load(std::memory_order_relaxed);
    snap.throughput.compressed_payloads  = compressed_payloads_.load(std::memory_order_relaxed);
    {
        uint64_t orig                     = original_bytes_total_.load(std::memory_order_relaxed);
        uint64_t comp                     = compressed_bytes_total_.load(std::memory_order_relaxed);
        snap.throughput.compression_ratio = (orig > 0) ? static_cast<double>(comp) / static_cast<double>(orig) : 1.0;
    }

    // ── Errors ─────────────────────────────────────────────────────────
    snap.errors.connection_errors = connection_errors_.load(std::memory_order_relaxed);
    snap.errors.timeout_errors    = timeout_errors_.load(std::memory_order_relaxed);
    snap.errors.parse_errors      = parse_errors_.load(std::memory_order_relaxed);
    snap.errors.auth_errors       = auth_errors_.load(std::memory_order_relaxed);
    {
        uint64_t total  = snap.throughput.requests_total;
        uint64_t errors = errors_total_.load(std::memory_order_relaxed);
        snap.errors.error_rate
            = (total + errors > 0) ? static_cast<double>(errors) / static_cast<double>(total + errors) : 0.0;
    }

    // ── Latency percentiles ────────────────────────────────────────────
    {
        std::lock_guard<std::mutex> lock(latency_mutex_);
        size_t count = buffer_full_ ? cfg_.max_samples : write_pos_;
        if (count == 0) {
            snap.latency = LatencyStats{};
        } else {
            std::vector<double> sorted_samples(latency_samples_.begin(),
                                               std::next(latency_samples_.begin(), static_cast<ptrdiff_t>(count)));
            std::sort(sorted_samples.begin(), sorted_samples.end());

            snap.latency.sample_count = count;
            snap.latency.p50_ms       = percentile(sorted_samples, 50.0);
            snap.latency.p75_ms       = percentile(sorted_samples, 75.0);
            snap.latency.p95_ms       = percentile(sorted_samples, 95.0);
            snap.latency.p99_ms       = percentile(sorted_samples, 99.0);
            snap.latency.p999_ms      = percentile(sorted_samples, 99.9);
            snap.latency.max_ms       = sorted_samples.back();
            snap.latency.mean_ms
                = std::accumulate(sorted_samples.begin(), sorted_samples.end(), 0.0) / static_cast<double>(count);

            // Build histogram with exponential buckets: 1,2,4,8,16,32,64,128,256,512,1024 ms
            std::array<uint64_t, 11> buckets_ms = {1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024};
            std::map<uint64_t, uint64_t> hist;
            for (uint64_t b : buckets_ms)
                hist[b] = 0;

            for (double s : sorted_samples) {
                uint64_t ms_bucket = static_cast<uint64_t>(std::ceil(s));
                // Round up to the next bucket
                for (uint64_t b : buckets_ms) {
                    if (ms_bucket <= b) {
                        hist[b]++;
                        break;
                    }
                }
            }
            snap.latency_histogram = hist;
        }
    }

    return snap;
}

void WireProtocolMetrics::reset() {
    {
        std::lock_guard<std::mutex> lock(latency_mutex_);
        std::fill(latency_samples_.begin(), latency_samples_.end(), 0.0);
        write_pos_   = 0;
        buffer_full_ = false;
    }
    bytes_received_total_.store(0, std::memory_order_relaxed);
    bytes_sent_total_.store(0, std::memory_order_relaxed);
    requests_total_.store(0, std::memory_order_relaxed);
    compressed_payloads_.store(0, std::memory_order_relaxed);
    original_bytes_total_.store(0, std::memory_order_relaxed);
    compressed_bytes_total_.store(0, std::memory_order_relaxed);
    errors_total_.store(0, std::memory_order_relaxed);
    connection_errors_.store(0, std::memory_order_relaxed);
    timeout_errors_.store(0, std::memory_order_relaxed);
    parse_errors_.store(0, std::memory_order_relaxed);
    auth_errors_.store(0, std::memory_order_relaxed);
}

/*static*/ double WireProtocolMetrics::percentile(const std::vector<double> &sorted, double p) noexcept {
    if (sorted.empty())
        return 0.0;
    if (sorted.size() == 1)
        return sorted[0];

    double rank = (p / 100.0) * static_cast<double>(sorted.size() - 1);
    size_t lo   = static_cast<size_t>(rank);
    size_t hi   = lo + 1;
    if (hi >= sorted.size())
        return sorted.back();

    double frac = rank - static_cast<double>(lo);
    return sorted[lo] + frac * (sorted[hi] - sorted[lo]);
}

// =============================================================================
// PayloadBufferPool::Handle
// =============================================================================

PayloadBufferPool::Handle::Handle(std::unique_ptr<Buffer> buf, PayloadBufferPool *pool) noexcept
    : buf_(std::move(buf)), pool_(pool) {}

PayloadBufferPool::Handle::Handle(Handle &&o) noexcept : buf_(std::move(o.buf_)), pool_(o.pool_) {
    o.pool_ = nullptr;
}

PayloadBufferPool::Handle &PayloadBufferPool::Handle::operator=(Handle &&o) noexcept {
    if (this != &o) {
        release();
        buf_    = std::move(o.buf_);
        pool_   = o.pool_;
        o.pool_ = nullptr;
    }
    return *this;
}

void PayloadBufferPool::Handle::release() noexcept {
    if (buf_ && pool_) {
        pool_->returnBuffer(std::move(buf_));
        pool_ = nullptr;
    }
    buf_.reset();
}

PayloadBufferPool::Handle::~Handle() {
    release();
}

// =============================================================================
// PayloadBufferPool
// =============================================================================

PayloadBufferPool::PayloadBufferPool(size_t slab_size, size_t pool_depth)
    : slab_size_(slab_size), pool_depth_(pool_depth) {
    idle_slabs_.reserve(pool_depth_);
}

PayloadBufferPool::~PayloadBufferPool() = default;

PayloadBufferPool::Handle PayloadBufferPool::acquire() {
    std::unique_ptr<Buffer> buf;

    {
        // R16: Add timeout enforcement with try_lock_for to prevent indefinite
        // blocking on high contention. Uses 100µs timeout for fast path.
        std::unique_lock<std::timed_mutex> lock(pool_mutex_, std::defer_lock);
        if (!lock.try_lock_for(std::chrono::microseconds(100))) {
            // Timeout on lock acquisition: fall through to heap allocation
            // This is acceptable for low-contention fast path
            miss_count_.fetch_add(1, std::memory_order_relaxed);
            buf = std::make_unique<Buffer>();
            buf->reserve(slab_size_);
            buf->clear();
            return Handle(std::move(buf), this);
        }

        if (!idle_slabs_.empty()) {
            buf = std::move(idle_slabs_.back());
            idle_slabs_.pop_back();
            hit_count_.fetch_add(1, std::memory_order_relaxed);
        }
    }

    if (!buf) {
        miss_count_.fetch_add(1, std::memory_order_relaxed);
        buf = std::make_unique<Buffer>();
        buf->reserve(slab_size_);
    }

    buf->clear(); // reset length, keep capacity
    return Handle(std::move(buf), this);
}

void PayloadBufferPool::returnBuffer(std::unique_ptr<Buffer> buf) noexcept {
    if (!buf)
        return;
    buf->clear();
    buf->reserve(slab_size_); // re-warm capacity

    std::lock_guard<std::timed_mutex> lock(pool_mutex_);
    if (idle_slabs_.size() < pool_depth_) {
        idle_slabs_.push_back(std::move(buf));
    }
    // If pool is full, just drop (let unique_ptr destructor free it)
}

size_t PayloadBufferPool::poolDepth() const noexcept {
    std::lock_guard<std::timed_mutex> lock(pool_mutex_);
    return idle_slabs_.size();
}

size_t PayloadBufferPool::slabSize() const noexcept {
    return slab_size_;
}
uint64_t PayloadBufferPool::hitCount() const noexcept {
    return hit_count_.load(std::memory_order_relaxed);
}
uint64_t PayloadBufferPool::missCount() const noexcept {
    return miss_count_.load(std::memory_order_relaxed);
}
double PayloadBufferPool::hitRate() const noexcept {
    uint64_t h = hitCount(), m = missCount();
    return (h + m > 0) ? static_cast<double>(h) / static_cast<double>(h + m) : 0.0;
}

// =============================================================================
// CompressionAdvisor
// =============================================================================

CompressionAdvisor::CompressionAdvisor(const Config &cfg) : cfg_(cfg) {}

CompressionAdvisor::Decision CompressionAdvisor::advise([[maybe_unused]] size_t payload_size) const noexcept {
    if (payload_size < cfg_.min_compressible_bytes)
        return Decision::SKIP;
    if (payload_size >= cfg_.lz4_fast_threshold) {
        return cfg_.prefer_speed ? Decision::LZ4_FAST_X : Decision::LZ4_HC;
    }
    return cfg_.prefer_speed ? Decision::LZ4_FAST_X : Decision::LZ4_FAST;
}

int CompressionAdvisor::lz4Acceleration(Decision d) const noexcept {
    switch (d) {
        case Decision::LZ4_FAST:
            return cfg_.lz4_fast_acceleration;
        case Decision::LZ4_FAST_X:
            return cfg_.lz4_fast_x_acceleration;
        default:
            return 0; // HC and SKIP don't use the acceleration field
    }
}

} // namespace network
} // namespace themis
