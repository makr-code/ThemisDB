/**
 * @file hazard_pointers.h
 * @brief Lock-free memory reclamation using hazard pointers
 * @version 0.1.0
 * @note Implements Maged M. Michael's hazard pointer technique for safe concurrent deletion
 * @note Used in lock-free search structures to prevent use-after-free errors
 */

#pragma once

#include <atomic>
#include <cstddef>
#include <memory>
#include <vector>
#include <thread>

namespace themis {
namespace lockfree {

/**
 * @class HazardPointer
 * @brief Lock-free memory management using hazard pointers
 * @tparam T Type of pointers being managed
 * 
 * @details Hazard pointers allow safe reclamation of dynamically allocated nodes
 *          in lock-free data structures without requiring garbage collection.
 *          
 *          The basic algorithm:
 *          1. Reader announces pointer to node it's accessing (acquire)
 *          2. Reader uses the node safely
 *          3. Reader releases the hazard pointer (release)
 *          4. Deleter collects nodes to be deleted
 *          5. Deleter checks if any hazard pointers protect deleted nodes
 *          6. Only unprotected nodes are actually deallocated
 */
template <typename T>
class HazardPointer {
public:
    /// Maximum number of concurrent readers per thread (typical: 1-4)
    static constexpr std::size_t K_HAZARD_POINTERS_PER_THREAD = 2;
    
    /// Threshold for batching deletions (postpone reclamation until batch reaches this size)
    static constexpr std::size_t K_RETIRE_THRESHOLD = 128;
    
    /**
     * @brief Acquire a hazard pointer slot
     * @return Slot index [0, K_HAZARD_POINTERS_PER_THREAD)
     * @details Caller must store pointer and release when done with node
     */
    static std::size_t acquire_slot();
    
    /**
     * @brief Release a hazard pointer slot
     * @param[in] slot Slot index returned by acquire_slot()
     */
    static void release_slot(std::size_t slot);
    
    /**
     * @brief Protect pointer in hazard slot
     * @param[in] slot Slot index
     * @param[in] ptr Pointer to protect (will be visible to deleters)
     * @details Must be called before accessing the node
     */
    static void protect(std::size_t slot, T* ptr);
    
    /**
     * @brief Get protected pointer from hazard slot
     * @param[in] slot Slot index
     * @return Currently protected pointer, may be nullptr
     */
    static T* get_protected(std::size_t slot);
    
    /**
     * @brief Clear hazard pointer (mark as no longer in use)
     * @param[in] slot Slot index
     * @details Call when done with node; allows reclamation if no other readers hold it
     */
    static void clear(std::size_t slot);
    
    /**
     * @brief Retire a pointer for delayed reclamation
     * @param[in] ptr Pointer to retire (should have been protected by readers)
     * @param[in] deleter Function to call on ptr when safe to reclaim
     *            (e.g., [](auto* p) { delete p; })
     * @details Does NOT immediately delete; waits until safe (no hazard pointers held)
     */
    template <typename Deleter>
    static void retire(T* ptr, Deleter deleter);
    
    /**
     * @brief Force reclamation of retired pointers (expensive, may block)
     * @details Scans all hazard pointers and reclaims any retired pointers that are safe
     *          Typically called periodically or when retire() queue exceeds threshold
     */
    static void reclaim();
    
    /**
     * @brief Get number of pointers pending reclamation
     * @return Count of retired but not yet reclaimed pointers
     */
    static std::size_t pending_reclamation_count();

private:
    struct HazardSlot {
        std::atomic<T*> ptr{nullptr};
    };
    
    struct RetiredPointer {
        T* ptr;
        std::function<void(T*)> deleter;
    };
    
    thread_local static std::vector<HazardSlot> thread_local_hazards;
    thread_local static std::vector<RetiredPointer> thread_local_retired;
};

/**
 * @class HazardPointerGuard
 * @brief RAII guard for hazard pointer acquire/release
 * @tparam T Type of managed pointer
 * 
 * @details Automatically acquires hazard slot on construction,
 *          releases on destruction (even if exception occurs)
 */
template <typename T>
class HazardPointerGuard {
public:
    /// Constructor: acquire hazard pointer slot
    HazardPointerGuard() : slot_(HazardPointer<T>::acquire_slot()) {}
    
    /// Destructor: release hazard pointer slot
    ~HazardPointerGuard() {
        HazardPointer<T>::release_slot(slot_);
    }
    
    /// Deleted copy operations
    HazardPointerGuard(const HazardPointerGuard&) = delete;
    HazardPointerGuard& operator=(const HazardPointerGuard&) = delete;
    
    /// Allow move semantics
    HazardPointerGuard(HazardPointerGuard&& other) noexcept 
        : slot_(other.slot_) {
        other.slot_ = -1;
    }
    
    HazardPointerGuard& operator=(HazardPointerGuard&& other) noexcept {
        if (this != &other) {
            if (slot_ != static_cast<std::size_t>(-1)) {
                HazardPointer<T>::release_slot(slot_);
            }
            slot_ = other.slot_;
            other.slot_ = -1;
        }
        return *this;
    }
    
    /// Protect a pointer in the guarded slot
    void protect(T* ptr) {
        HazardPointer<T>::protect(slot_, ptr);
    }
    
    /// Get the protected pointer
    T* get() const {
        return HazardPointer<T>::get_protected(slot_);
    }
    
    /// Clear the hazard pointer
    void clear() {
        HazardPointer<T>::clear(slot_);
    }
    
    /// Get slot index
    std::size_t slot() const {
        return slot_;
    }

private:
    std::size_t slot_;
};

} // namespace lockfree
} // namespace themis
