/**
 * @file pool_allocator.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=30, M=0, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// ThemisDB Memory Pool Allocator Implementation
// Implements high-performance memory pooling strategies

#include "utils/memory/pool_allocator.h"
#include "utils/error_registry.h"
#include <cstring>
#include <cmath>
#include <algorithm>
#include <unordered_map>
#include <map>
#include <mutex>
#include <stdexcept>
#include <limits>

namespace themis {
namespace memory {

// Constants
constexpr int PEAK_UPDATE_MAX_RETRIES = 10;  // Max retries for atomic peak memory update
constexpr size_t STACK_ALLOC_RESERVE_RATIO = 256;  // Reserve 1/256th of capacity for tracking

// Helper function to check if a number is a power of 2
static inline bool isPowerOfTwo([[maybe_unused]] size_t n) {
    return n > 0 && (n & (n - 1)) == 0;
}

// Helper function to get next power of 2
static inline size_t nextPowerOfTwo([[maybe_unused]] size_t n) {
    if (n == 0) {
      return 1;
    }
    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    n |= n >> 32;
    return n + 1;
}

// Helper function to align size to alignment boundary
static inline size_t alignSize(size_t size, size_t alignment) {
    return (size + alignment - 1) & ~(alignment - 1);
}

// ============================================================================
// Buddy Allocator Implementation
// ============================================================================

struct BuddyAllocator::Block {
    size_t size = 0;
    bool is_free;
    uintptr_t next;  // Address of next free block (0 if none)
};


struct BuddyAllocator::Impl {
    std::unique_ptr<uint8_t[]> memory;
    size_t total_size;
    size_t min_block_size;
    size_t max_order;
    
    // Free lists for each order - use indices into blocks map
    std::vector<uintptr_t> free_list_heads;  // heads[i] is address of first free block at order i
    
    // Block metadata (address -> block info) - use std::map for stable pointers
    std::map<uintptr_t, Block> blocks;
    
    std::mutex mutex = {};
    
    Impl(size_t total, size_t min_block)
        : total_size(total), min_block_size(min_block) {
        
        // Ensure sizes are powers of 2
        if (!isPowerOfTwo(total_size)) {
            total_size = nextPowerOfTwo(total_size);
        }
        if (!isPowerOfTwo(min_block_size)) {
            min_block_size = nextPowerOfTwo(min_block_size);
        }
        
        // Calculate max order
        max_order = 0;
        size_t size = min_block_size;
        while (size < total_size) {
            size *= 2;
            max_order++;
        }
        
        // Allocate memory
        memory = std::make_unique<uint8_t[]>(total_size);
        std::memset(memory.get(), 0, total_size);
        
        // Initialize free list heads
        free_list_heads.resize(max_order + 1, 0);
        
        // Add initial block to free list
        Block initial_block;
        initial_block.size = total_size;
        initial_block.is_free = true;
        initial_block.next = 0;
        
        uintptr_t addr = reinterpret_cast<uintptr_t>(memory.get());
        blocks[addr] = initial_block;
        free_list_heads[max_order] = addr;
    }
    
    ~Impl() noexcept = default;
    
    size_t getOrder([[maybe_unused]] size_t size) {
        size_t order = 0;
        size_t block_size = min_block_size;
        while (block_size < size && order < max_order) {
            block_size *= 2;
            order++;
        }
        return order;
    }
    
    void* allocateBlock([[maybe_unused]] size_t order) {
        // Find a free block at this order or higher
        for (size_t i = order; i <= max_order; ++i) {
            if (free_list_heads[i] == 0) continue;  // No free block at this order
            
            uintptr_t block_addr = free_list_heads[i];
            auto it = blocks.find(block_addr);
            if (it == blocks.end()) {
              return nullptr;
            }
            
            Block& block = it->second;
            free_list_heads[i] = block.next;  // Update head to next block
            
            // Split if necessary
            size_t current_order = i;
            while (current_order > order) {
                current_order--;
                size_t buddy_size = min_block_size << current_order;
                uintptr_t buddy_addr = block_addr + buddy_size;
                
                // Create buddy block
                Block buddy_block;
                buddy_block.size = buddy_size;
                buddy_block.is_free = true;
                buddy_block.next = free_list_heads[current_order];
                blocks[buddy_addr] = buddy_block;
                free_list_heads[current_order] = buddy_addr;
                
                block.size = buddy_size;
            }
            
            block.is_free = false;
            return reinterpret_cast<void*>(block_addr);
        }
        
        return nullptr;
    }
    
    void deallocateBlock(void* ptr) {
        uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);
        auto it = blocks.find(addr);
        if (it == blocks.end()) {
            return;
        }
        
        Block& block = it->second;
        block.is_free = true;
        size_t order = getOrder(block.size);
        uintptr_t current_addr = addr;
        
        // Try to coalesce with buddy blocks
        while (order < max_order) {
            size_t block_size = min_block_size << order;
            // Calculate buddy address (flip the bit at this level)
            uintptr_t buddy_addr = current_addr ^ block_size;
            
            // Check if buddy exists and is free
            auto buddy_it = blocks.find(buddy_addr);
            if (buddy_it == blocks.end() || !buddy_it->second.is_free || 
                buddy_it->second.size != block_size) {
                break;  // Cannot coalesce
            }
            
            // Remove buddy from its free list
            uintptr_t* list_ptr = &free_list_heads[order];
            while (*list_ptr != 0) {
                if (*list_ptr == buddy_addr) {
                    auto& buddy_block = blocks[*list_ptr];
                    *list_ptr = buddy_block.next;
                    break;
                }
                auto& current_block = blocks[*list_ptr];
                list_ptr = &current_block.next;
            }
            
            // Merge: keep the lower address block, remove the buddy
            uintptr_t merged_addr = std::min(current_addr, buddy_addr);
            blocks.erase(std::max(current_addr, buddy_addr));
            
            auto& merged_block = blocks[merged_addr];
            merged_block.size = block_size * 2;
            merged_block.is_free = true;
            merged_block.next = 0;
            
            current_addr = merged_addr;
            order++;
        }
        
        // Add final merged block to free list
        auto& final_block = blocks[current_addr];
        final_block.next = free_list_heads[order];
        free_list_heads[order] = current_addr;
    }
};

BuddyAllocator::BuddyAllocator(size_t total_size, size_t min_block_size)
    : impl_(std::make_unique<Impl>(total_size, min_block_size)) {
}

BuddyAllocator::~BuddyAllocator() noexcept = default;

Result<void*> BuddyAllocator::allocate(size_t size, AllocationHint hint) {
    if (size == 0) {
        return Err<void*>(errors::ErrorCode::ERR_MEMORY_INVALID_SIZE, 
                         "Allocation size must be greater than 0");
    }
    
    std::lock_guard<std::mutex> lock(impl_->mutex);
    
    // Align to cache line if requested
    if (hint == AllocationHint::CACHE_LINE_ALIGNED) {
        size = alignSize(size, CACHE_LINE_SIZE);
    }
    
    // Round up to minimum block size
    if (size < impl_->min_block_size) {
        size = impl_->min_block_size;
    }
    
    size_t order = impl_->getOrder(size);
    void* ptr = impl_->allocateBlock(order);
    
    if (ptr == nullptr) {
        stats_.allocation_failures.fetch_add(1);
        return Err<void*>(errors::ErrorCode::ERR_MEMORY_POOL_EXHAUSTED,
                         "Buddy allocator out of memory");
    }
    
    stats_.total_allocations.fetch_add(1);
    stats_.bytes_allocated.fetch_add(size);
    
    // Update peak with retry limit to avoid infinite loop
    uint64_t current_usage = stats_.getCurrentUsage();
    uint64_t peak = stats_.peak_memory_usage.load();
    int retry_count = 0;
    while (current_usage > peak && retry_count < PEAK_UPDATE_MAX_RETRIES && 
           !stats_.peak_memory_usage.compare_exchange_weak(peak, current_usage)) {
        retry_count++;
    }
    
    return ptr;
}

Result<void> BuddyAllocator::deallocate(void* ptr) {
    if (ptr == nullptr) {
        return OkVoid();
    }
    
    std::lock_guard<std::mutex> lock(impl_->mutex);
    
    uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);
    auto it = impl_->blocks.find(addr);
    if (it == impl_->blocks.end()) {
        return ErrVoid(errors::ErrorCode::ERR_MEMORY_DOUBLE_FREE,
                      "Invalid pointer or double free");
    }
    
    size_t size = it->second.size;
    impl_->deallocateBlock(ptr);
    
    stats_.total_deallocations.fetch_add(1);
    stats_.bytes_freed.fetch_add(size);
    
    return OkVoid();
}

Result<void> BuddyAllocator::reset() {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    
    // Clear all blocks and free lists
    impl_->blocks.clear();
    std::fill(impl_->free_list_heads.begin(), impl_->free_list_heads.end(), 0);
    
    // Re-add initial block
    Block initial_block;
    initial_block.size = impl_->total_size;
    initial_block.is_free = true;
    initial_block.next = 0;
    
    uintptr_t addr = reinterpret_cast<uintptr_t>(impl_->memory.get());
    impl_->blocks[addr] = initial_block;
    impl_->free_list_heads[impl_->max_order] = addr;
    
    stats_.reset();
    
    return OkVoid();
}

double BuddyAllocator::getFragmentation() const {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    
    size_t free_blocks = 0;
    size_t total_free_space = 0;
    
    for (size_t i = 0; i < impl_->free_list_heads.size(); ++i) {
        uintptr_t block_addr = impl_->free_list_heads[i];
        while (block_addr != 0) {
            auto it = impl_->blocks.find(block_addr);
            if (it == impl_->blocks.end()) {
              break;
            }
            free_blocks++;
            total_free_space += it->second.size;
            block_addr = it->second.next;
        }
    }
    
    if (total_free_space == 0) {
        return 0.0;
    }
    
    // Fragmentation = 1 - (largest_block / total_free_space)
    size_t largest_block = 0;
    for (int i = static_cast<int>(impl_->max_order); i >= 0; --i) {
        if (impl_->free_list_heads[i] != 0) {
            largest_block = impl_->min_block_size << i;
            break;
        }
    }
    
    return 1.0 - (static_cast<double>(largest_block) / total_free_space);
}

// ============================================================================
// Slab Allocator Implementation
// ============================================================================

struct SlabAllocator::Slab {
    std::unique_ptr<uint8_t[]> memory;
    size_t object_size = {};
    size_t object_count = {};
    std::vector<bool> free_map;
    size_t free_count = {};
    std::unique_ptr<Slab> next;
    
    Slab(size_t obj_size, size_t obj_count)
        : object_size(obj_size), object_count(obj_count),
          free_count(obj_count) {
        // Check for integer overflow: object_size * object_count
        if (obj_count > 0 && obj_size > SIZE_MAX / obj_count) {
            throw std::overflow_error("Slab allocation size overflow: object_size * object_count exceeds SIZE_MAX");
        }
        memory = std::make_unique<uint8_t[]>(object_size * object_count);
        std::memset(memory.get(), 0, object_size * object_count);
        free_map.resize(object_count, true);
    }
    
    ~Slab() noexcept = default;
    
    void* allocate() {
        if (free_count == 0) {
            return nullptr;
        }
        
        for (size_t i = 0; i < object_count; ++i) {
            if (free_map[i]) {
                free_map[i] = false;
                free_count--;
                return memory.get() + (i * object_size);
            }
        }
        
        return nullptr;
    }
    
    bool deallocate(void* ptr) {
        uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);
        uintptr_t base = reinterpret_cast<uintptr_t>(memory.get());
        
        if (addr < base || addr >= base + (object_size * object_count)) {
            return false;
        }
        
        size_t index = (addr - base) / object_size;
        if (free_map[index]) {
            return false;  // Double free
        }
        
        free_map[index] = true;
        free_count++;
        return true;
    }
    
    bool contains(void* ptr) const {
        uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);
        uintptr_t base = reinterpret_cast<uintptr_t>(memory.get());
        return addr >= base && addr < base + (object_size * object_count);
    }
};

struct SlabAllocator::Impl {
    size_t object_size = 0;
    size_t objects_per_slab = {};
    size_t max_slabs = {};
    
    std::unique_ptr<Slab> head_slab;
    size_t slab_count = {};
    
    std::mutex mutex = {};
    
    Impl(size_t obj_size, size_t objs_per_slab, size_t max)
        : object_size(obj_size), objects_per_slab(objs_per_slab),
          max_slabs(max), slab_count(0) {
        
        // Ensure object size is at least pointer size and aligned
        if (object_size < sizeof(void*)) {
            object_size = sizeof(void*);
        }
        object_size = alignSize(object_size, sizeof(void*));
    }
    
    ~Impl() noexcept = default;
    
    void* allocate() {
        // Try existing slabs first
        Slab* slab = head_slab.get();
        while (slab != nullptr) {
            void* ptr = slab->allocate();
            if (ptr != nullptr) {
                return ptr;
            }
            slab = slab->next.get();
        }
        
        // Need new slab
        if (max_slabs > 0 && slab_count >= max_slabs) {
            return nullptr;  // Hit slab limit
        }
        
        auto new_slab = std::make_unique<Slab>(object_size, objects_per_slab);
        void* allocation = new_slab->allocate();
        new_slab->next = std::move(head_slab);
        head_slab = std::move(new_slab);
        slab_count++;

        return allocation;
    }
    
    bool deallocate(void* ptr) {
        Slab* slab = head_slab.get();
        while (slab != nullptr) {
            if (slab->contains(ptr)) {
                return slab->deallocate(ptr);
            }
            slab = slab->next.get();
        }
        return false;
    }
};

SlabAllocator::SlabAllocator(size_t object_size, size_t objects_per_slab, 
                            size_t max_slabs)
    : impl_(std::make_unique<Impl>(object_size, objects_per_slab, max_slabs)) {
}

SlabAllocator::~SlabAllocator() noexcept = default;

Result<void*> SlabAllocator::allocate(size_t size, [[maybe_unused]] AllocationHint hint) {
    if (size == 0) {
        return Err<void*>(errors::ErrorCode::ERR_MEMORY_INVALID_SIZE,
                         "Allocation size must be greater than 0");
    }
    
    if (size > impl_->object_size) {
        return Err<void*>(errors::ErrorCode::ERR_MEMORY_INVALID_SIZE,
                         "Requested size exceeds slab object size");
    }
    
    std::lock_guard<std::mutex> lock(impl_->mutex);
    
    void* ptr = impl_->allocate();
    if (ptr == nullptr) {
        stats_.allocation_failures.fetch_add(1);
        return Err<void*>(errors::ErrorCode::ERR_MEMORY_POOL_EXHAUSTED,
                         "Slab allocator out of memory");
    }
    
    stats_.total_allocations.fetch_add(1);
    stats_.bytes_allocated.fetch_add(impl_->object_size);
    
    // Update peak with retry limit
    uint64_t current_usage = stats_.getCurrentUsage();
    uint64_t peak = stats_.peak_memory_usage.load();
    int retry_count = 0;
    while (current_usage > peak && retry_count < PEAK_UPDATE_MAX_RETRIES && 
           !stats_.peak_memory_usage.compare_exchange_weak(peak, current_usage)) {
        retry_count++;
    }
    
    return ptr;
}

Result<void> SlabAllocator::deallocate(void* ptr) {
    if (ptr == nullptr) {
        return OkVoid();
    }
    
    std::lock_guard<std::mutex> lock(impl_->mutex);
    
    if (!impl_->deallocate(ptr)) {
        return ErrVoid(errors::ErrorCode::ERR_MEMORY_DOUBLE_FREE,
                      "Invalid pointer or double free");
    }
    
    stats_.total_deallocations.fetch_add(1);
    stats_.bytes_freed.fetch_add(impl_->object_size);
    
    return OkVoid();
}

Result<void> SlabAllocator::reset() {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    
    // Delete all slabs
    impl_->head_slab.reset();
    impl_->slab_count = 0;
    stats_.reset();
    
    return OkVoid();
}

size_t SlabAllocator::getSlabCount() const {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    return impl_->slab_count;
}

double SlabAllocator::getUtilization() const {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    
    if (impl_->slab_count == 0) {
        return 0.0;
    }
    
    size_t total_objects = impl_->slab_count * impl_->objects_per_slab;
    size_t used_objects = 0;
    
    Slab* slab = impl_->head_slab.get();
    while (slab != nullptr) {
        used_objects += (slab->object_count - slab->free_count);
        slab = slab->next.get();
    }
    
    return static_cast<double>(used_objects) / total_objects;
}

// ============================================================================
// Stack Allocator Implementation
// ============================================================================

struct StackAllocator::Impl {
    uint8_t* memory;
    size_t capacity = {};
    size_t offset = {};
    
    std::mutex mutex;
    
    // Track allocations for validation - store pairs of (address, size)
    std::vector<std::pair<uintptr_t, size_t>> allocation_stack;
    
    Impl([[maybe_unused]] size_t cap) : capacity(cap), offset(0) {
        memory = new uint8_t[capacity];
        std::memset(memory, 0, capacity);
        // Reserve space for allocation tracking to reduce reallocations
        // Reserve approximately 1/256th of capacity, with min 16 and max 1024
        size_t reserve_size = std::max(size_t(16), 
                                      std::min(size_t(1024), 
                                              capacity / STACK_ALLOC_RESERVE_RATIO));
        allocation_stack.reserve(reserve_size);
    }
    
    ~Impl() noexcept = default;
};

StackAllocator::StackAllocator([[maybe_unused]] size_t capacity)
    : impl_(std::make_unique<Impl>(capacity)) {
}

StackAllocator::~StackAllocator() = default;

Result<void*> StackAllocator::allocate(size_t size, [[maybe_unused]] AllocationHint hint) {
    if (size == 0) {
        return Err<void*>(errors::ErrorCode::ERR_MEMORY_INVALID_SIZE,
                         "Allocation size must be greater than 0");
    }
    
    std::lock_guard<std::mutex> lock(impl_->mutex);
    
    // Align size to pointer boundary
    size_t aligned_size = alignSize(size, sizeof(void*));
    
    if (impl_->offset + aligned_size > impl_->capacity) {
        stats_.allocation_failures.fetch_add(1);
        return Err<void*>(errors::ErrorCode::ERR_MEMORY_POOL_EXHAUSTED,
                         "Stack allocator out of memory");
    }
    
    void* ptr = impl_->memory + impl_->offset;
    impl_->offset += aligned_size;
    
    // Track allocation with size
    impl_->allocation_stack.push_back({reinterpret_cast<uintptr_t>(ptr), aligned_size});
    
    stats_.total_allocations.fetch_add(1);
    stats_.bytes_allocated.fetch_add(aligned_size);
    
    // Update peak with retry limit
    uint64_t current_usage = stats_.getCurrentUsage();
    uint64_t peak = stats_.peak_memory_usage.load();
    int retry_count = 0;
    while (current_usage > peak && retry_count < PEAK_UPDATE_MAX_RETRIES && 
           !stats_.peak_memory_usage.compare_exchange_weak(peak, current_usage)) {
        retry_count++;
    }
    
    return ptr;
}

Result<void> StackAllocator::deallocate(void* ptr) {
    if (ptr == nullptr) {
        return OkVoid();
    }
    
    std::lock_guard<std::mutex> lock(impl_->mutex);
    
    // Stack allocator requires LIFO deallocation
    if (impl_->allocation_stack.empty()) {
        return ErrVoid(errors::ErrorCode::ERR_MEMORY_DOUBLE_FREE,
                      "Stack allocator: no allocations to free");
    }
    
    uintptr_t last_addr = impl_->allocation_stack.back().first;
    size_t last_size = impl_->allocation_stack.back().second;
    uintptr_t ptr_addr = reinterpret_cast<uintptr_t>(ptr);
    
    if (last_addr != ptr_addr) {
        return ErrVoid(errors::ErrorCode::ERR_MEMORY_DOUBLE_FREE,
                      "Stack allocator: non-LIFO deallocation attempted");
    }
    
    impl_->allocation_stack.pop_back();
    
    // Calculate size freed from tracked allocation
    size_t bytes_freed = last_size;
    impl_->offset -= bytes_freed;
    
    stats_.total_deallocations.fetch_add(1);
    stats_.bytes_freed.fetch_add(bytes_freed);
    
    return OkVoid();
}

Result<void> StackAllocator::reset() {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    
    impl_->offset = 0;
    impl_->allocation_stack.clear();
    stats_.reset();
    
    return OkVoid();
}

size_t StackAllocator::getCurrentOffset() const {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    return impl_->offset;
}

size_t StackAllocator::getAvailableSpace() const {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    return impl_->capacity - impl_->offset;
}

size_t StackAllocator::savePosition() const {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    return impl_->offset;
}

Result<void> StackAllocator::restorePosition([[maybe_unused]] size_t position) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    
    if (position > impl_->offset) {
        return ErrVoid(errors::ErrorCode::ERR_MEMORY_INVALID_SIZE,
                      "Cannot restore to position beyond current offset");
    }
    
    size_t bytes_freed = impl_->offset - position;
    impl_->offset = position;
    
    // Clear allocation stack to match position
    while (!impl_->allocation_stack.empty()) {
        uintptr_t addr = impl_->allocation_stack.back().first;
        size_t alloc_offset = addr - reinterpret_cast<uintptr_t>(impl_->memory);
        if (alloc_offset < position) {
            break;
        }
        impl_->allocation_stack.pop_back();
    }
    
    stats_.bytes_freed.fetch_add(bytes_freed);
    
    return OkVoid();
}

// ============================================================================
// Pool Allocator Implementation
// ============================================================================

struct PoolAllocator::Impl {
    Config config;
    
    std::unique_ptr<BuddyAllocator> buddy;
    std::unordered_map<size_t, std::unique_ptr<SlabAllocator>> slabs;
    std::unique_ptr<StackAllocator> stack;
    
    // Track which allocator owns each pointer
    std::unordered_map<uintptr_t, IAllocator*> ownership;
    std::mutex ownership_mutex = {};
    
    Impl(const Config& cfg) : config(cfg) {
        // Initialize buddy allocator
        buddy = std::make_unique<BuddyAllocator>(
            config.buddy_pool_size, config.buddy_min_block);
        
        // Initialize slab allocators for common sizes
        for (size_t size : config.slab_sizes) {
            slabs[size] = std::make_unique<SlabAllocator>(
                size, config.slab_objects_per_slab, config.slab_max_slabs);
        }
        
        // Initialize stack allocator
        stack = std::make_unique<StackAllocator>(config.stack_capacity);
    }
    
    IAllocator* selectAllocator(size_t size, AllocationHint hint) {
        // Use stack for short-lived allocations
        if (hint == AllocationHint::SHORT_LIVED) {
            return stack.get();
        }
        
        // Use slab for fixed sizes if available
        auto it = slabs.find(size);
        if (it != slabs.end()) {
            return it->second.get();
        }
        
        // Find closest slab size
        for (const auto& [slab_size, slab] : slabs) {
            if (slab_size >= size && slab_size <= size * 2) {
                return slab.get();
            }
        }
        
        // Use buddy for everything else
        return buddy.get();
    }
};

PoolAllocator::PoolAllocator()
    : PoolAllocator(Config{}) {
}

PoolAllocator::PoolAllocator(const Config& config)
    : impl_(std::make_unique<Impl>(config)) {
}

PoolAllocator::~PoolAllocator() = default;

Result<void*> PoolAllocator::allocate(size_t size, AllocationHint hint) {
    IAllocator* allocator = impl_->selectAllocator(size, hint);
    auto result = allocator->allocate(size, hint);
    
    if (result) {
        void* ptr = *result;
        std::lock_guard<std::mutex> lock(impl_->ownership_mutex);
        impl_->ownership[reinterpret_cast<uintptr_t>(ptr)] = allocator;
    }
    
    return result;
}

Result<void> PoolAllocator::deallocate(void* ptr) {
    if (ptr == nullptr) {
        return OkVoid();
    }
    
    IAllocator* allocator = nullptr;
    {
        std::lock_guard<std::mutex> lock(impl_->ownership_mutex);
        auto it = impl_->ownership.find(reinterpret_cast<uintptr_t>(ptr));
        if (it == impl_->ownership.end()) {
            return ErrVoid(errors::ErrorCode::ERR_MEMORY_DOUBLE_FREE,
                          "Pointer not allocated by this pool");
        }
        allocator = it->second;
        impl_->ownership.erase(it);
    }
    
    return allocator->deallocate(ptr);
}

AllocationStats PoolAllocator::getCombinedStats() const {
    AllocationStats combined;
    
    const auto& buddy_stats = impl_->buddy->getStats();
    combined.total_allocations.store(buddy_stats.total_allocations.load());
    combined.total_deallocations.store(buddy_stats.total_deallocations.load());
    combined.allocation_failures.store(buddy_stats.allocation_failures.load());
    combined.bytes_allocated.store(buddy_stats.bytes_allocated.load());
    combined.bytes_freed.store(buddy_stats.bytes_freed.load());
    combined.peak_memory_usage.store(buddy_stats.peak_memory_usage.load());
    
    for (const auto& [size, slab] : impl_->slabs) {
        const auto& stats = slab->getStats();
        combined.total_allocations.fetch_add(stats.total_allocations.load());
        combined.total_deallocations.fetch_add(stats.total_deallocations.load());
        combined.allocation_failures.fetch_add(stats.allocation_failures.load());
        combined.bytes_allocated.fetch_add(stats.bytes_allocated.load());
        combined.bytes_freed.fetch_add(stats.bytes_freed.load());
    }
    
    const auto& stack_stats = impl_->stack->getStats();
    combined.total_allocations.fetch_add(stack_stats.total_allocations.load());
    combined.total_deallocations.fetch_add(stack_stats.total_deallocations.load());
    combined.allocation_failures.fetch_add(stack_stats.allocation_failures.load());
    combined.bytes_allocated.fetch_add(stack_stats.bytes_allocated.load());
    combined.bytes_freed.fetch_add(stack_stats.bytes_freed.load());
    
    return combined;
}

const AllocationStats& PoolAllocator::getBuddyStats() const {
    return impl_->buddy->getStats();
}

const AllocationStats& PoolAllocator::getSlabStats([[maybe_unused]] size_t size) const {
    auto it = impl_->slabs.find(size);
    if (it != impl_->slabs.end()) {
        return it->second->getStats();
    }
    
    static AllocationStats empty;
    return empty;
}

const AllocationStats& PoolAllocator::getStackStats() const {
    return impl_->stack->getStats();
}

Result<void> PoolAllocator::reset() {
    auto buddy_result = impl_->buddy->reset();
    if (!buddy_result) {
        return buddy_result;
    }
    
    for (auto& [size, slab] : impl_->slabs) {
        auto result = slab->reset();
        if (!result) {
            return result;
        }
    }
    
    auto stack_result = impl_->stack->reset();
    if (!stack_result) {
        return stack_result;
    }
    
    std::lock_guard<std::mutex> lock(impl_->ownership_mutex);
    impl_->ownership.clear();
    
    return OkVoid();
}

} // namespace memory
} // namespace themis

