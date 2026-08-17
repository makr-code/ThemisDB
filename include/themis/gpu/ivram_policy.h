/**
 * @file ivram_policy.h
 * @brief IVRAM placement policy for GPU tensor buffers.
 *
 * Defines policies for allocating and evicting tensor data to and
 * from in-video-RAM (IVRAM) on discrete GPU devices.
 */

#pragma once

#include <cstdint>
#include <string>

namespace themis {
namespace gpu {

/**
 * @brief Abstract VRAM allocation-policy contract for the unified GPU memory hierarchy.
 *
 * All GPU memory managers in ThemisDB — regardless of namespace — share this
 * interface so that tenant isolation, OOM detection, and policy gating are
 * enforced through a single, observable surface.
 *
 * Hierarchy overview:
 * @code
 *   IVRAMPolicy  (this file)
 *   └── themis::gpu::GPUMemoryManager   — canonical, edition-aware singleton
 *       ├── themis::llm::GPUMemoryManager   — delegates accounting upward
 *       └── themis::llm::lora::GPUMemoryManager — delegates accounting upward
 * @endcode
 *
 * Thread safety: implementations must be individually thread-safe.
 */
class IVRAMPolicy {
public:
    virtual ~IVRAMPolicy() = default;

    // Non-copyable, non-movable by default.
    IVRAMPolicy(const IVRAMPolicy&) = delete;
    IVRAMPolicy& operator=(const IVRAMPolicy&) = delete;

    /**
     * @brief Return true iff @p size_bytes can be allocated under current policy.
     *
     * Checks both the global edition VRAM limit and the per-tenant quota when
     * @p tenant_id is non-empty. Does not modify any state.
     *
     * @param size_bytes  Bytes that the caller intends to allocate.
     * @param tenant_id   Tenant identifier; empty = no per-tenant check.
     * @return true when the allocation would be granted.
     */
    [[nodiscard]] virtual bool canAllocate(uint64_t size_bytes,
                                           const std::string& tenant_id = "") const = 0;

    /**
     * @brief Notify the policy that @p size_bytes have been successfully allocated.
     *
     * Updates accounting state (used bytes, tenant counters, peak).
     *
     * @param size_bytes  Bytes that were allocated.
     * @param tag         Owner / reason label.
     * @param tenant_id   Tenant identifier; empty = no per-tenant tracking.
     */
    virtual void onAllocate(uint64_t size_bytes,
                            const std::string& tag,
                            const std::string& tenant_id = "") = 0;

    /**
     * @brief Notify the policy that @p size_bytes have been freed.
     *
     * Updates accounting state. Implementations must clamp to zero on
     * mis-matched sizes to prevent underflow.
     *
     * @param size_bytes  Bytes that were freed.
     * @param tenant_id   Tenant identifier; empty = no per-tenant update.
     */
    virtual void onDeallocate(uint64_t size_bytes,
                              const std::string& tenant_id = "") = 0;

    /**
     * @brief Return the number of VRAM bytes currently tracked as allocated.
     *
     * @return Currently accounted VRAM in bytes.
     */
    [[nodiscard]] virtual uint64_t usedBytes() const = 0;

    /**
     * @brief Return true when this policy allows any GPU acceleration.
     *
     * Implementations should return false when the edition limit is zero or
     * no GPU hardware is available.
     */
    [[nodiscard]] virtual bool isGPUEnabled() const noexcept = 0;

protected:
    IVRAMPolicy() = default;
    IVRAMPolicy(IVRAMPolicy&&) noexcept = default;
    IVRAMPolicy& operator=(IVRAMPolicy&&) noexcept = default;
};

} // namespace gpu
} // namespace themis
