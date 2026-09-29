/**
 * @file hazard_pointers.cpp
 * @brief Implementation of lock-free memory reclamation using hazard pointers
 */

#include "utils/hazard_pointers.h"
#include <algorithm>
#include <mutex>

namespace themis {
namespace lockfree {

// Thread-local storage for hazard pointer slots
template <typename T>
thread_local std::vector<HazardPointer<T>::HazardSlot> 
    HazardPointer<T>::thread_local_hazards;

// Thread-local storage for retired pointers pending reclamation
template <typename T>
thread_local std::vector<typename HazardPointer<T>::RetiredPointer> 
    HazardPointer<T>::thread_local_retired;

// Global mutex protecting access to all thread-local hazard lists during reclamation
static std::mutex g_hazard_reclamation_mutex;

template <typename T>
std::size_t HazardPointer<T>::acquire_slot() {
    auto& hazards = thread_local_hazards;
    
    // Find or create available slot
    for (std::size_t i = 0; i < hazards.size(); ++i) {
        if (hazards[i].ptr.load(std::memory_order_relaxed) == nullptr) {
            return i;
        }
    }
    
    // No available slot, create new one
    if (hazards.size() < K_HAZARD_POINTERS_PER_THREAD) {
        hazards.emplace_back();
        return hazards.size() - 1;
    }
    
    // Exceeded per-thread limit; spin wait for a slot to free up
    // In production, this should be replaced with a more sophisticated allocation strategy
    while (true) {
        for (std::size_t i = 0; i < hazards.size(); ++i) {
            if (hazards[i].ptr.load(std::memory_order_acquire) == nullptr) {
                return i;
            }
        }
        // Yield to other threads if all slots are occupied
        std::this_thread::yield();
    }
}

template <typename T>
void HazardPointer<T>::release_slot(std::size_t slot) {
    if (slot < thread_local_hazards.size()) {
        thread_local_hazards[slot].ptr.store(nullptr, std::memory_order_release);
    }
}

template <typename T>
void HazardPointer<T>::protect(std::size_t slot, T* ptr) {
    if (slot < thread_local_hazards.size()) {
        thread_local_hazards[slot].ptr.store(ptr, std::memory_order_release);
    }
}

template <typename T>
T* HazardPointer<T>::get_protected(std::size_t slot) {
    if (slot < thread_local_hazards.size()) {
        return thread_local_hazards[slot].ptr.load(std::memory_order_acquire);
    }
    return nullptr;
}

template <typename T>
void HazardPointer<T>::clear(std::size_t slot) {
    if (slot < thread_local_hazards.size()) {
        thread_local_hazards[slot].ptr.store(nullptr, std::memory_order_release);
    }
}

template <typename T>
template <typename Deleter>
void HazardPointer<T>::retire(T* ptr, Deleter deleter) {
    auto& retired = thread_local_retired;
    retired.push_back({ptr, deleter});
    
    // If retired list exceeds threshold, attempt reclamation
    if (retired.size() >= K_RETIRE_THRESHOLD) {
        reclaim();
    }
}

template <typename T>
void HazardPointer<T>::reclaim() {
    auto& retired = thread_local_retired;
    if (retired.empty()) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(g_hazard_reclamation_mutex);
    
    // Collect all active hazard pointers from all threads
    std::vector<T*> hazard_ptrs;
    
    // Note: In a full implementation, we'd need thread-local storage visibility
    // to collect hazard pointers from all threads. This simplified version
    // only checks the current thread's hazards and assumes delayed reclamation.
    
    for (const auto& hazard : thread_local_hazards) {
        T* ptr = hazard.ptr.load(std::memory_order_acquire);
        if (ptr != nullptr) {
            hazard_ptrs.push_back(ptr);
        }
    }
    
    // Remove from retired list and delete those not protected by any hazard pointer
    auto new_end = std::remove_if(retired.begin(), retired.end(),
        [&hazard_ptrs](const RetiredPointer& retired_ptr) {
            bool is_protected = std::find(hazard_ptrs.begin(), hazard_ptrs.end(),
                                         retired_ptr.ptr) != hazard_ptrs.end();
            if (!is_protected) {
                // Safe to delete: no hazard pointer protects this node
                retired_ptr.deleter(retired_ptr.ptr);
                return true;  // Remove from retired list
            }
            return false;  // Keep in retired list (still protected)
        });
    
    retired.erase(new_end, retired.end());
}

template <typename T>
std::size_t HazardPointer<T>::pending_reclamation_count() {
    return thread_local_retired.size();
}

} // namespace lockfree
} // namespace themis

// Explicit instantiation for common pointer types
template class themis::lockfree::HazardPointer<void>;
