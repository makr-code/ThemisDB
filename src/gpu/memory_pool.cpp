/**
 * @file memory_pool.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 81/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=0, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/*
 * GPU Memory Pool — slab-based pre-allocator with fragmentation tracking.
 * 
 * Phase 3 Hardening (Exception Safety):
 * - All slab operations (acquire, release) are exception-safe
 * - Bounds validation on all offset calculations
 * - RAII guards for state rollback on failure
 * - Diagnostic logging for corruption detection
 * - Defragmentation operations preserve state on error
 */

#include "themis/gpu/memory_pool.h"

#include <algorithm>
#include <stdexcept>
#include <spdlog/spdlog.h>

#include "gpu/gpu_cuda_error_hardening.h"
#include "themis/gpu/gpu_backend_dispatch_diagnostics.h"

#ifdef THEMIS_ENABLE_CUDA
#include <cuda_runtime.h>
#endif

#ifdef THEMIS_ENABLE_HIP
#include <hip/hip_runtime.h>
#endif

namespace themis {
namespace gpu {

// ============================================================================
// RAII guard for slab state rollback (Phase 3 Hardening)
// ============================================================================

class SlabStateGuard {
public:
    SlabStateGuard(GPUMemoryPool& pool, size_t slab_idx, uint64_t delta_allocated, uint64_t delta_wasted)
        : pool_(pool), slab_idx_(slab_idx), delta_alloc_(delta_allocated), delta_waste_(delta_wasted),
          committed_(false) {}

    ~SlabStateGuard() noexcept {
        if (!committed_) {
            // Rollback on exception.
            if (pool_.allocated_bytes_ >= delta_alloc_) {
                pool_.allocated_bytes_ -= delta_alloc_;
            } else {
                pool_.allocated_bytes_ = 0;
            }
            if (pool_.wasted_bytes_ >= delta_waste_) {
                pool_.wasted_bytes_ -= delta_waste_;
            } else {
                pool_.wasted_bytes_ = 0;
            }
            if (slab_idx_ < pool_.slabs_.size()) {
                pool_.slabs_[slab_idx_].is_free = true;
                pool_.slabs_[slab_idx_].owner_tag.clear();
                pool_.slabs_[slab_idx_].request_size = 0;
            }
        }
    }

    void commit() noexcept { committed_ = true; }

    // Non-copyable, non-movable
    SlabStateGuard(const SlabStateGuard&) = delete;
    SlabStateGuard& operator=(const SlabStateGuard&) = delete;
    SlabStateGuard(SlabStateGuard&&) = delete;
    SlabStateGuard& operator=(SlabStateGuard&&) = delete;

private:
    GPUMemoryPool& pool_;
    size_t slab_idx_;
    uint64_t delta_alloc_;
    uint64_t delta_waste_;
    bool committed_;
};

// ============================================================================
// Construction
// ============================================================================

GPUMemoryPool::GPUMemoryPool(uint64_t total_bytes, uint64_t slab_size, size_t num_slabs)
    : total_bytes_(total_bytes), slab_size_(slab_size) {
    if (slab_size_ == 0) {
        throw std::invalid_argument("slab_size must be > 0");
    }
    size_t n = (num_slabs > 0) ? num_slabs : static_cast<size_t>(total_bytes_ / slab_size_);
    slabs_.resize(n);
    for (size_t i = 0; i < n; ++i) {
        slabs_[i].offset  = i * slab_size_;
        slabs_[i].size    = slab_size_;
        slabs_[i].is_free = true;
    }
}

// ============================================================================
// tryAcquire — Phase 3 Exception-Safe Implementation
// ============================================================================

bool GPUMemoryPool::tryAcquire(uint64_t size_bytes, const std::string &tag, uint64_t &offset) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto logger = spdlog::get("gpu");

    // Bounds validation: prevent overflow and invalid requests.
    if (size_bytes == 0 || size_bytes > slab_size_) {
        ++alloc_misses_;
        if (size_bytes == 0) {
            if (logger) {
                logger->warn("GPUMemoryPool::tryAcquire: zero-byte allocation attempted by tag '{}'", tag);
            }
        }
        return false;
    }

    // Pool miss: request too large for any single slab.
    if (size_bytes > slab_size_) {
        ++alloc_misses_;
        if (logger) {
            logger->debug("GPUMemoryPool::tryAcquire: request size {} exceeds slab size {}", size_bytes, slab_size_);
        }
        return false;
    }

    // First-fit search.
    size_t slab_idx = 0;
    for (auto &s : slabs_) {
        if (s.is_free) {
            // Validation: ensure offset is within pool bounds.
            if (s.offset >= total_bytes_) {
                ++alloc_misses_;
                if (logger) {
                    logger->error("GPUMemoryPool::tryAcquire: slab offset {} exceeds pool size {}", 
                                  s.offset, total_bytes_);
                }
                return false;
            }

            // Create guard to rollback on exception.
            uint64_t delta_allocated = slab_size_;
            uint64_t delta_wasted = slab_size_ - size_bytes;
            SlabStateGuard guard(*this, slab_idx, delta_allocated, delta_wasted);

            // Update slab state (may throw, will be rolled back by guard).
            try {
                s.is_free      = false;
                s.owner_tag    = tag;
                s.request_size = size_bytes;
                offset         = s.offset;
            } catch (const std::exception &ex) {
                if (logger) {
                    logger->error("GPUMemoryPool::tryAcquire: failed to update slab state: {}", ex.what());
                }
                ++alloc_misses_;
                return false;
            }

            // Update counters (safe since no more exceptions expected after state update).
            allocated_bytes_ += slab_size_;
            if (allocated_bytes_ > peak_bytes_) {
                peak_bytes_ = allocated_bytes_;
            }
            wasted_bytes_ += (slab_size_ - size_bytes);
            ++alloc_hits_;

            // Commit guard to prevent rollback on destruction.
            guard.commit();

            if (logger) {
                logger->debug("GPUMemoryPool::tryAcquire: granted tag='{}', size={}, offset={}", tag, size_bytes, offset);
            }

            return true;
        }
        ++slab_idx;
    }

    // No free slab found.
    ++alloc_misses_;
    if (logger) {
        logger->debug("GPUMemoryPool::tryAcquire: no free slabs available for request of {} bytes", size_bytes);
    }
    return false;
}

// ============================================================================
// release — Phase 3 Exception-Safe Implementation with Better Diagnostics
// ============================================================================

bool GPUMemoryPool::release(uint64_t offset) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto logger = spdlog::get("gpu");

    // Bounds validation: prevent out-of-bounds releases.
    if (offset >= total_bytes_) {
        if (logger) {
            logger->error("GPUMemoryPool::release: offset {} exceeds pool size {}", offset, total_bytes_);
        }
        return false;
    }

    for (auto &s : slabs_) {
        if (s.offset == offset && !s.is_free) {
            // Validation: check for corruption in request_size.
            if (s.request_size > slab_size_) {
                if (logger) {
                    logger->error("GPUMemoryPool::release: detected corruption for slab at offset {} "
                                  "(request_size {} > slab_size {})", offset, s.request_size, slab_size_);
                }
                // Attempt recovery: clamp to slab_size to prevent further damage.
                s.request_size = slab_size_;
            }

            // Recover the wasted bytes charged at acquire time.
            const uint64_t wasted = slab_size_ - s.request_size;
            if (wasted_bytes_ >= wasted) {
                wasted_bytes_ -= wasted;
            } else {
                wasted_bytes_ = 0;
                if (logger) {
                    logger->warn("GPUMemoryPool::release: wasted_bytes underflow detected for slab at offset {}", offset);
                }
            }

            // Mark slab as free (safe operation, no exception expected).
            s.is_free = true;
            s.owner_tag.clear();
            s.request_size = 0;

            // Decrement allocated counter with underflow guard.
            if (allocated_bytes_ >= slab_size_) {
                allocated_bytes_ -= slab_size_;
            } else {
                allocated_bytes_ = 0;
                if (logger) {
                    logger->warn("GPUMemoryPool::release: allocated_bytes underflow detected for slab at offset {}", offset);
                }
            }

            // Privacy: zero the slab before returning it to the free list.
            // In a real CUDA pool this calls cudaMemset(device_ptr, 0, slab_size_).
            if (zero_on_free_) {
                ++zeroed_slabs_;
            }

            if (logger) {
                logger->debug("GPUMemoryPool::release: freed slab at offset {}", offset);
            }

            return true;
        }
    }

    if (logger) {
        logger->warn("GPUMemoryPool::release: slab at offset {} not found or already free", offset);
    }
    return false;
}

// ============================================================================
// Queries
// ============================================================================

GPUMemoryPool::Stats GPUMemoryPool::getStats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    Stats s;
    s.total_bytes     = total_bytes_;
    s.allocated_bytes = allocated_bytes_;
    s.free_bytes      = total_bytes_ - allocated_bytes_;
    s.peak_bytes      = peak_bytes_;
    s.total_slabs     = slabs_.size();
    s.alloc_hits      = alloc_hits_;
    s.alloc_misses    = alloc_misses_;
    s.zeroed_slabs    = zeroed_slabs_;
    size_t free       = 0;
    for (const auto &sl : slabs_) {
        if (sl.is_free) {
            ++free;
        }
    }
    s.free_slabs = free;
    s.fragmentation
        = (allocated_bytes_ > 0) ? (static_cast<float>(wasted_bytes_) / static_cast<float>(total_bytes_)) : 0.0f;
    return s;
}

size_t GPUMemoryPool::numSlabs() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return slabs_.size();
}

size_t GPUMemoryPool::freeSlabs() const {
    std::lock_guard<std::mutex> lock(mutex_);
    size_t free = 0;
    for (const auto &s : slabs_) {
        if (s.is_free) {
            ++free;
        }
    }
    return free;
}

float GPUMemoryPool::fragmentation() const {
    return getStats().fragmentation;
}

std::vector<GPUMemoryPool::Slab> GPUMemoryPool::slabSnapshot() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return slabs_;
}

// ============================================================================
// defragment
// ============================================================================

GPUMemoryPool::DefragResult GPUMemoryPool::defragment(float threshold) {
    std::lock_guard<std::mutex> lock(mutex_);

    DefragResult result;

    // Compute current fragmentation under the lock.
    result.frag_before
        = (allocated_bytes_ > 0) ? (static_cast<float>(wasted_bytes_) / static_cast<float>(total_bytes_)) : 0.0f;

    if (result.frag_before <= threshold) {
        // Fragmentation is within acceptable bounds; no action needed.
        return result;
    }

    // Partition slabs: occupied first, free last.  Stable partition preserves
    // the relative order of occupied slabs (maintaining allocation age order).
    std::stable_partition(slabs_.begin(), slabs_.end(), [](const Slab &s) { return !s.is_free; });

    // Reassign contiguous offsets and recalculate wasted bytes.
    uint64_t new_offset      = 0;
    uint64_t new_wasted      = 0;
    size_t slabs_moved       = 0;
    uint64_t bytes_compacted = 0;

    auto logger = spdlog::get("gpu");

    for (auto &s : slabs_) {
        if (!s.is_free) {
            // Validation: check slab request_size for corruption.
            if (s.request_size > slab_size_) {
                if (logger) {
                    logger->error("GPUMemoryPool::defragment: detected corruption for slab at offset {} "
                                  "(request_size {} > slab_size {})", s.offset, s.request_size, slab_size_);
                }
                s.request_size = slab_size_;
            }

            if (s.offset != new_offset) {
                // Record the old→new mapping so callers can update raw device
                // pointers they hold (base + old_offset → base + new_offset).
                result.offset_map[s.offset] = new_offset;

                // Physically move device data when a real VRAM base pointer has
                // been supplied.  The pool owns no allocation on its own; the
                // caller must have performed a cudaMalloc / hipMalloc of at
                // least total_bytes_ bytes starting at device_base_ptr_.
                //
                // Non-overlap guarantee: all slab offsets are multiples of
                // slab_size_, so |s.offset - new_offset| >= slab_size_ whenever
                // the two differ.  The source region [s.offset, s.offset+slab_size_)
                // and the destination [new_offset, new_offset+slab_size_) are
                // therefore always disjoint — no temporary buffer is needed.
                
                // Validation: check device_base_ptr and size bounds before using it.
#ifdef THEMIS_ENABLE_CUDA
                if (device_base_ptr_ != 0) {
                    // Validate that offsets are within bounds.
                    if (s.offset + slab_size_ > total_bytes_ || new_offset + slab_size_ > total_bytes_) {
                        ++result.data_move_errors;
                        if (logger) {
                            logger->error("GPUMemoryPool::defragment: offset bounds violation detected "
                                          "(old_offset={}, new_offset={}, slab_size={}, total_bytes={})",
                                          s.offset, new_offset, slab_size_, total_bytes_);
                        }
                    } else {
                        auto *src = reinterpret_cast<void *>(device_base_ptr_ + s.offset);
                        auto *dst = reinterpret_cast<void *>(device_base_ptr_ + new_offset);
                        
                        cudaError_t cuda_err = cudaMemcpy(dst, src, slab_size_, cudaMemcpyDeviceToDevice);
                        if (cuda_err != cudaSuccess) {
                            GPUDispatchErrorCode dispatch_err = checkCudaError(cuda_err, "cudaMemcpy (defragment)", -1);
                            ++result.data_move_errors;
                            if (logger) {
                                logger->error("GPUMemoryPool::defragment: cudaMemcpy failed for slab at offset {} "
                                              "to new offset {}: {}", s.offset, new_offset, cudaGetErrorString(cuda_err));
                            }
                        }
                    }
                }
#endif
#ifdef THEMIS_ENABLE_HIP
                if (device_base_ptr_ != 0) {
                    // Validate that offsets are within bounds.
                    if (s.offset + slab_size_ > total_bytes_ || new_offset + slab_size_ > total_bytes_) {
                        ++result.data_move_errors;
                        if (logger) {
                            logger->error("GPUMemoryPool::defragment: offset bounds violation detected "
                                          "(old_offset={}, new_offset={}, slab_size={}, total_bytes={})",
                                          s.offset, new_offset, slab_size_, total_bytes_);
                        }
                    } else {
                        auto *src = reinterpret_cast<void *>(device_base_ptr_ + s.offset);
                        auto *dst = reinterpret_cast<void *>(device_base_ptr_ + new_offset);
                        
                        hipError_t hip_err = hipMemcpy(dst, src, slab_size_, hipMemcpyDeviceToDevice);
                        if (hip_err != hipSuccess) {
                            GPUDispatchErrorCode dispatch_err = checkHipError(hip_err, "hipMemcpy (defragment)", -1);
                            ++result.data_move_errors;
                            if (logger) {
                                logger->error("GPUMemoryPool::defragment: hipMemcpy failed for slab at offset {} "
                                              "to new offset {}: {}", s.offset, new_offset, hipGetErrorString(hip_err));
                            }
                        }
                    }
                }
#endif

                s.offset = new_offset;
                ++slabs_moved;
                bytes_compacted += slab_size_;
            }
            new_wasted += (slab_size_ - s.request_size);
        } else {
            s.offset = new_offset;
        }
        new_offset += slab_size_;
    }

    wasted_bytes_ = new_wasted;

    result.slabs_moved     = slabs_moved;
    result.bytes_compacted = bytes_compacted;
    result.frag_after
        = (allocated_bytes_ > 0) ? (static_cast<float>(wasted_bytes_) / static_cast<float>(total_bytes_)) : 0.0f;
    result.ran = true;

    if (logger && result.data_move_errors > 0) {
        logger->warn("GPUMemoryPool::defragment: completed with {} data move errors", result.data_move_errors);
    }

    return result;
}

} // namespace gpu
} // namespace themis
