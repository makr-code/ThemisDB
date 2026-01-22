// ThemisDB Memory Pool Allocator Implementation
// Implements high-performance memory pooling strategies

#include "utils/memory/pool_allocator.h"
#include "utils/error_registry.h"
#include <cstring>
#include <cmath>
#include <algorithm>
#include <unordered_map>
#include <mutex>

namespace themis {
namespace memory {

// Helper function to check if a number is a power of 2
static inline bool isPowerOfTwo(size_t n) {
    return n > 0 && (n & (n - 1)) == 0;
}

// Helper function to get next power of 2
static inline size_t nextPowerOfTwo(size_t n) {
    if (n == 0) return 1;
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
    size_t size;
    bool is_free;
    Block* next;  // For free list
};

struct BuddyAllocator::Impl {
    uint8_t* memory;
    size_t total_size;
    size_t min_block_size;
    size_t max_order;
    
    // Free lists for each order
    std::vector<Block*> free_lists;
    
    // Block metadata (address -> block info)
    std::unordered_map<uintptr_t, Block> blocks;
    
    std::mutex mutex;
    
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
        memory = new uint8_t[total_size];
        std::memset(memory, 0, total_size);
        
        // Initialize free lists
        free_lists.resize(max_order + 1, nullptr);
        
        // Add initial block to free list
        Block initial_block;
        initial_block.size = total_size;
        initial_block.is_free = true;
        initial_block.next = nullptr;
        
        uintptr_t addr = reinterpret_cast<uintptr_t>(memory);
        blocks[addr] = initial_block;
        free_lists[max_order] = &blocks[addr];
    }
    
    ~Impl() {
        delete[] memory;
    }
    
    size_t getOrder(size_t size) {
        size_t order = 0;
        size_t block_size = min_block_size;
        while (block_size < size && order < max_order) {
            block_size *= 2;
            order++;
        }
        return order;
    }
    
    void* allocateBlock(size_t order) {
        // Find a free block at this order or higher
        for (size_t i = order; i <= max_order; ++i) {
            if (free_lists[i] != nullptr) {
                // Found a free block
                Block* block = free_lists[i];
                free_lists[i] = block->next;
                
                // Split if necessary
                while (i > order) {
                    i--;
                    size_t buddy_size = min_block_size << i;
                    
                    uintptr_t block_addr = reinterpret_cast<uintptr_t>(memory) +
                        (reinterpret_cast<uint8_t*>(block) - memory);
                    uintptr_t buddy_addr = block_addr + buddy_size;
                    
                    // Create buddy block
                    Block buddy_block;
                    buddy_block.size = buddy_size;
                    buddy_block.is_free = true;
                    buddy_block.next = free_lists[i];
                    blocks[buddy_addr] = buddy_block;
                    free_lists[i] = &blocks[buddy_addr];
                    
                    block->size = buddy_size;
                }
                
                block->is_free = false;
                return reinterpret_cast<void*>(
                    memory + (reinterpret_cast<uint8_t*>(block) - 
                              reinterpret_cast<uint8_t*>(&blocks.begin()->second)));
            }
        }
        
        return nullptr;
    }
    
    void deallocateBlock(void* ptr) {
        uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);
        auto it = blocks.find(addr);
        if (it == blocks.end()) {
            return;
        }
        
        Block* block = &it->second;
        block->is_free = true;
        
        // Try to coalesce with buddy
        size_t order = getOrder(block->size);
        
        // Add back to free list
        block->next = free_lists[order];
        free_lists[order] = block;
    }
};

BuddyAllocator::BuddyAllocator(size_t total_size, size_t min_block_size)
    : impl_(std::make_unique<Impl>(total_size, min_block_size)) {
}

BuddyAllocator::~BuddyAllocator() = default;

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
    
    uint64_t current_usage = stats_.getCurrentUsage();
    uint64_t peak = stats_.peak_memory_usage.load();
    while (current_usage > peak && 
           !stats_.peak_memory_usage.compare_exchange_weak(peak, current_usage)) {
        // Retry if another thread updated peak
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
    std::fill(impl_->free_lists.begin(), impl_->free_lists.end(), nullptr);
    
    // Re-add initial block
    Block initial_block;
    initial_block.size = impl_->total_size;
    initial_block.is_free = true;
    initial_block.next = nullptr;
    
    uintptr_t addr = reinterpret_cast<uintptr_t>(impl_->memory);
    impl_->blocks[addr] = initial_block;
    impl_->free_lists[impl_->max_order] = &impl_->blocks[addr];
    
    stats_.reset();
    
    return OkVoid();
}

double BuddyAllocator::getFragmentation() const {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    
    size_t free_blocks = 0;
    size_t total_free_space = 0;
    
    for (const auto& list_ptr : impl_->free_lists) {
        Block* block = list_ptr;
        while (block != nullptr) {
            free_blocks++;
            total_free_space += block->size;
            block = block->next;
        }
    }
    
    if (total_free_space == 0) {
        return 0.0;
    }
    
    // Fragmentation = 1 - (largest_block / total_free_space)
    size_t largest_block = 0;
    for (size_t i = impl_->max_order; i >= 0 && i <= impl_->max_order; --i) {
        if (impl_->free_lists[i] != nullptr) {
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
    uint8_t* memory;
    size_t object_size;
    size_t object_count;
    std::vector<bool> free_map;
    size_t free_count;
    Slab* next;
    
    Slab(size_t obj_size, size_t obj_count)
        : object_size(obj_size), object_count(obj_count), 
          free_count(obj_count), next(nullptr) {
        memory = new uint8_t[object_size * object_count];
        std::memset(memory, 0, object_size * object_count);
        free_map.resize(object_count, true);
    }
    
    ~Slab() {
        delete[] memory;
    }
    
    void* allocate() {
        if (free_count == 0) {
            return nullptr;
        }
        
        for (size_t i = 0; i < object_count; ++i) {
            if (free_map[i]) {
                free_map[i] = false;
                free_count--;
                return memory + (i * object_size);
            }
        }
        
        return nullptr;
    }
    
    bool deallocate(void* ptr) {
        uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);
        uintptr_t base = reinterpret_cast<uintptr_t>(memory);
        
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
        uintptr_t base = reinterpret_cast<uintptr_t>(memory);
        return addr >= base && addr < base + (object_size * object_count);
    }
};

struct SlabAllocator::Impl {
    size_t object_size;
    size_t objects_per_slab;
    size_t max_slabs;
    
    Slab* head_slab;
    size_t slab_count;
    
    std::mutex mutex;
    
    Impl(size_t obj_size, size_t objs_per_slab, size_t max)
        : object_size(obj_size), objects_per_slab(objs_per_slab),
          max_slabs(max), head_slab(nullptr), slab_count(0) {
        
        // Ensure object size is at least pointer size and aligned
        if (object_size < sizeof(void*)) {
            object_size = sizeof(void*);
        }
        object_size = alignSize(object_size, sizeof(void*));
    }
    
    ~Impl() {
        Slab* slab = head_slab;
        while (slab != nullptr) {
            Slab* next = slab->next;
            delete slab;
            slab = next;
        }
    }
    
    void* allocate() {
        // Try existing slabs first
        Slab* slab = head_slab;
        while (slab != nullptr) {
            void* ptr = slab->allocate();
            if (ptr != nullptr) {
                return ptr;
            }
            slab = slab->next;
        }
        
        // Need new slab
        if (max_slabs > 0 && slab_count >= max_slabs) {
            return nullptr;  // Hit slab limit
        }
        
        Slab* new_slab = new Slab(object_size, objects_per_slab);
        new_slab->next = head_slab;
        head_slab = new_slab;
        slab_count++;
        
        return new_slab->allocate();
    }
    
    bool deallocate(void* ptr) {
        Slab* slab = head_slab;
        while (slab != nullptr) {
            if (slab->contains(ptr)) {
                return slab->deallocate(ptr);
            }
            slab = slab->next;
        }
        return false;
    }
};

SlabAllocator::SlabAllocator(size_t object_size, size_t objects_per_slab, 
                            size_t max_slabs)
    : impl_(std::make_unique<Impl>(object_size, objects_per_slab, max_slabs)) {
}

SlabAllocator::~SlabAllocator() = default;

Result<void*> SlabAllocator::allocate(size_t size, AllocationHint hint) {
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
    
    uint64_t current_usage = stats_.getCurrentUsage();
    uint64_t peak = stats_.peak_memory_usage.load();
    while (current_usage > peak && 
           !stats_.peak_memory_usage.compare_exchange_weak(peak, current_usage)) {
        // Retry
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
    Slab* slab = impl_->head_slab;
    while (slab != nullptr) {
        Slab* next = slab->next;
        delete slab;
        slab = next;
    }
    
    impl_->head_slab = nullptr;
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
    
    Slab* slab = impl_->head_slab;
    while (slab != nullptr) {
        used_objects += (slab->object_count - slab->free_count);
        slab = slab->next;
    }
    
    return static_cast<double>(used_objects) / total_objects;
}

// ============================================================================
// Stack Allocator Implementation
// ============================================================================

struct StackAllocator::Impl {
    uint8_t* memory;
    size_t capacity;
    size_t offset;
    
    std::mutex mutex;
    
    // Track allocations for validation
    std::vector<uintptr_t> allocation_stack;
    
    Impl(size_t cap) : capacity(cap), offset(0) {
        memory = new uint8_t[capacity];
        std::memset(memory, 0, capacity);
    }
    
    ~Impl() {
        delete[] memory;
    }
};

StackAllocator::StackAllocator(size_t capacity)
    : impl_(std::make_unique<Impl>(capacity)) {
}

StackAllocator::~StackAllocator() = default;

Result<void*> StackAllocator::allocate(size_t size, AllocationHint hint) {
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
    
    // Track allocation
    impl_->allocation_stack.push_back(reinterpret_cast<uintptr_t>(ptr));
    
    stats_.total_allocations.fetch_add(1);
    stats_.bytes_allocated.fetch_add(aligned_size);
    
    uint64_t current_usage = stats_.getCurrentUsage();
    uint64_t peak = stats_.peak_memory_usage.load();
    while (current_usage > peak && 
           !stats_.peak_memory_usage.compare_exchange_weak(peak, current_usage)) {
        // Retry
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
    
    uintptr_t last_alloc = impl_->allocation_stack.back();
    uintptr_t ptr_addr = reinterpret_cast<uintptr_t>(ptr);
    
    if (last_alloc != ptr_addr) {
        return ErrVoid(errors::ErrorCode::ERR_MEMORY_DOUBLE_FREE,
                      "Stack allocator: non-LIFO deallocation attempted");
    }
    
    impl_->allocation_stack.pop_back();
    
    // Calculate size freed (distance to next allocation or end)
    size_t bytes_freed;
    if (impl_->allocation_stack.empty()) {
        bytes_freed = impl_->offset;
        impl_->offset = 0;
    } else {
        uintptr_t prev_alloc = impl_->allocation_stack.back();
        bytes_freed = ptr_addr - prev_alloc;
        impl_->offset = prev_alloc - reinterpret_cast<uintptr_t>(impl_->memory);
    }
    
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

Result<void> StackAllocator::restorePosition(size_t position) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    
    if (position > impl_->offset) {
        return ErrVoid(errors::ErrorCode::ERR_MEMORY_INVALID_SIZE,
                      "Cannot restore to position beyond current offset");
    }
    
    size_t bytes_freed = impl_->offset - position;
    impl_->offset = position;
    
    // Clear allocation stack to match position
    while (!impl_->allocation_stack.empty()) {
        uintptr_t addr = impl_->allocation_stack.back();
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
    std::mutex ownership_mutex;
    
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

const AllocationStats& PoolAllocator::getSlabStats(size_t size) const {
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
