/**
 * @file huge_pages.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// ThemisDB Huge Pages Support
// Provides support for transparent huge pages (2MB/1GB) for improved TLB performance
//
// Based on: "Optimizing Database Performance using Huge Pages" (FAST'14)
//
// Huge pages reduce TLB (Translation Lookaside Buffer) misses significantly,
// especially for large memory allocations typical in database workloads.

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <cstring>

// Platform-specific includes
#ifdef __linux__
#include <sys/mman.h>
#include <unistd.h>
#endif

#ifdef _WIN32
#include <windows.h>
#endif

namespace themis {
namespace memory {

/**
 * @brief Huge page sizes supported by the system
 */
enum class HugePageSize {
    Default = 0,      // Use system default (usually 4KB on x86-64)
    Size_2MB = 2097152,   // 2MB huge pages (common on x86-64)
    Size_1GB = 1073741824 // 1GB huge pages (requires CPU support)
};

/**
 * @brief Check if huge pages are available on the system
 * 
 * @return true if huge pages can be used
 */
inline bool huge_pages_available() {
    #ifdef THEMIS_USE_HUGE_PAGES
    #ifdef __linux__
    // Check if /proc/meminfo shows huge pages
    FILE* fp = fopen("/proc/meminfo", "r");
    if (!fp) {
      return false;
    }
    
    char line[256];
    bool has_huge_pages = false;
    while (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, "HugePages_Total:", 16) == 0) {
            has_huge_pages = true;
            break;
        }
    }
    fclose(fp);
    return has_huge_pages;
    #elif defined(_WIN32)
    // Windows supports large pages via VirtualAlloc with MEM_LARGE_PAGES
    return true;
    #else
    return false;
    #endif
    #else
    return false;
    #endif
}

/**
 * @brief Get the huge page size in bytes
 * 
 * @return Size of huge pages, or 0 if not available
 */
inline size_t get_huge_page_size() {
    #ifdef THEMIS_USE_HUGE_PAGES
    #ifdef __linux__
    // Most Linux systems use 2MB huge pages
    return static_cast<size_t>(HugePageSize::Size_2MB);
    #elif defined(_WIN32)
    // Windows typically uses 2MB large pages
    return static_cast<size_t>(HugePageSize::Size_2MB);
    #else
    return 0;
    #endif
    #else
    return 0;
    #endif
}

/**
 * @brief Allocate memory using huge pages
 * 
 * Attempts to allocate memory using huge pages for improved TLB performance.
 * Falls back to regular allocation if huge pages are not available.
 * 
 * @param size Number of bytes to allocate (should be multiple of huge page size)
 * @return Pointer to allocated memory, or nullptr on failure
 */
inline void* allocate_huge_pages([[maybe_unused]] size_t size) {
    #ifdef THEMIS_USE_HUGE_PAGES
    
    #ifdef __linux__
    // Try to allocate with MAP_HUGETLB
    void* ptr = mmap(nullptr, size,
                    PROT_READ | PROT_WRITE,
                    MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB,
                    -1, 0);
    
    if (ptr != MAP_FAILED) {
        return ptr;
    }
    
    // Fallback: try transparent huge pages (THP)
    ptr = mmap(nullptr, size,
              PROT_READ | PROT_WRITE,
              MAP_PRIVATE | MAP_ANONYMOUS,
              -1, 0);
    
    if (ptr != MAP_FAILED) {
        // Advise kernel to use huge pages
        madvise(ptr, size, MADV_HUGEPAGE);
        return ptr;
    }
    
    return nullptr;
    
    #elif defined(_WIN32)
    // Windows large pages allocation
    SIZE_T minSize = GetLargePageMinimum();
    if (minSize > 0 && size >= minSize) {
        void* ptr = VirtualAlloc(nullptr, size,
                                MEM_RESERVE | MEM_COMMIT | MEM_LARGE_PAGES,
                                PAGE_READWRITE);
        if (ptr) {
            return ptr;
        }
    }
    
    // Fallback to regular allocation
    return VirtualAlloc(nullptr, size,
                       MEM_RESERVE | MEM_COMMIT,
                       PAGE_READWRITE);
    
    #else
    // Fallback for unsupported platforms
    return nullptr;
    #endif
    
    #else
    // Huge pages not enabled at compile time
    return nullptr;
    #endif
}

/**
 * @brief Free memory allocated with allocate_huge_pages()
 * 
 * @param ptr Pointer to memory to free
 * @param size Size of the allocation
 */
inline void deallocate_huge_pages(void* ptr, [[maybe_unused]] size_t size) {
    if (!ptr) {
      return;
    }
    
    #ifdef THEMIS_USE_HUGE_PAGES
    
    #ifdef __linux__
    munmap(ptr, size);
    
    #elif defined(_WIN32)
    VirtualFree(ptr, 0, MEM_RELEASE);
    
    #endif
    
    #endif
}

/**
 * @brief Check if huge pages are enabled at compile time
 */
inline bool is_huge_pages_enabled() {
    #ifdef THEMIS_USE_HUGE_PAGES
    return true;
    #else
    return false;
    #endif
}

/**
 * @brief Get status string for huge pages
 */
inline std::string huge_pages_status() {
    if (!is_huge_pages_enabled()) {
        return "disabled (compile-time)";
    }
    
    if (!huge_pages_available()) {
        return "enabled but unavailable (system)";
    }
    
    size_t page_size = get_huge_page_size();
    if (page_size == static_cast<size_t>(HugePageSize::Size_2MB)) {
        return "enabled (2MB pages)";
    } else if (page_size == static_cast<size_t>(HugePageSize::Size_1GB)) {
        return "enabled (1GB pages)";
    } else {
        return "enabled (unknown size)";
    }
}

/**
 * @brief Configure system for huge pages (Linux only)
 * 
 * This function provides guidance on configuring the system to support huge pages.
 * Must be run with appropriate privileges.
 * 
 * @param num_pages Number of 2MB huge pages to reserve
 * @return Instructions string
 */
inline std::string get_huge_pages_setup_instructions([[maybe_unused]] int num_pages = 1024) {
    #ifdef __linux__
    return 
        "To enable huge pages on Linux:\n"
        "1. Reserve huge pages:\n"
        "   echo " + std::to_string(num_pages) + " | sudo tee /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages\n"
        "\n"
        "2. Or enable transparent huge pages (THP):\n"
        "   echo always | sudo tee /sys/kernel/mm/transparent_hugepage/enabled\n"
        "\n"
        "3. Verify configuration:\n"
        "   cat /proc/meminfo | grep Huge\n";
    #elif defined(_WIN32)
    return
        "To enable large pages on Windows:\n"
        "1. Run 'secpol.msc'\n"
        "2. Navigate to: Local Policies > User Rights Assignment\n"
        "3. Add your user to 'Lock pages in memory'\n"
        "4. Restart the application\n";
    #else
    return "Huge pages setup not available on this platform\n";
    #endif
}

} // namespace memory
} // namespace themis
