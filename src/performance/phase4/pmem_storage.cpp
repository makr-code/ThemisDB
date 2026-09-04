/**
 * @file pmem_storage.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=6, H=2, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "performance/phase4/pmem_storage.h"

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <system_error>
#include <vector>

// Platform-specific includes for file mapping
#ifdef _WIN32
    #include <windows.h>
    #include <fileapi.h>
#else
    #include <fcntl.h>
    #include <sys/mman.h>
    #include <sys/stat.h>
    #include <unistd.h>
    // MAP_SYNC requires Linux 4.15+ and a DAX-capable filesystem
    #ifndef MAP_SYNC
        #define MAP_SYNC 0x80000
    #endif
    #ifndef MAP_SHARED_VALIDATE
        #define MAP_SHARED_VALIDATE 0x03
    #endif
#endif

namespace themis {
namespace performance {
namespace phase4 {

// ---------------------------------------------------------------------------
// Device detection
// ---------------------------------------------------------------------------

std::vector<PMemDeviceInfo> detect_pmem_devices() {
    std::vector<PMemDeviceInfo> devices;

#ifdef __linux__
    // Enumerate /proc/mounts for DAX-capable file systems
    FILE* mounts = fopen("/proc/mounts", "r");
    if (!mounts) {
        return devices;
    }

    char dev[256], mnt[256], fstype[64], opts[512];
    int  dump, pass;
    while (fscanf(mounts, "%254s %254s %62s %510s %d %d",
                  dev, mnt, fstype, opts, &dump, &pass) == 6) {
        // DAX is supported on ext4 and xfs with the "dax" mount option
        if (strstr(opts, "dax") != nullptr) {
            PMemDeviceInfo info;
            info.path    = mnt;
            info.is_dax  = true;
            // Try to stat the mount point to get approximate size
            struct stat st{};
            if (stat(mnt, &st) == 0) {
                // Rough estimate: use block size * blocks (may be inaccurate for DAX)
                info.size_bytes = static_cast<size_t>(st.st_blocks) * 512;
            }
            devices.push_back(std::move(info));
        }
    }
    fclose(mounts);
#endif
    // On non-Linux platforms PMem detection is not supported;
    // callers may fall back to file-based emulation.
    return devices;
}

// ---------------------------------------------------------------------------
// PMemPool implementation
// ---------------------------------------------------------------------------

// Internal helper: round @p v up to the nearest multiple of @p align.
static constexpr size_t align_up(size_t v, size_t align) noexcept {
    return (v + align - 1) & ~(align - 1);
}

PMemPool::PMemPool(const Config& config) : alignment_(config.alignment) {
    map_region(config);
    if (!base_) {
        throw std::runtime_error("PMemPool: failed to map region at " + config.path);
    }
    header_ = reinterpret_cast<PMemPoolHeader*>(base_);

    if (config.recover_on_open && header_->magic == kMagic) {
        recover();
    } else {
        init_header(mapped_size_, config.alignment);
    }
    bump_.store(header_->bump_offset, std::memory_order_relaxed);
}

PMemPool::~PMemPool() {
    unmap_region();
#ifndef _WIN32
    if (fd_ >= 0) {
        ::close(fd_);
        fd_ = -1;
    }
#endif
}

PMemPool::PMemPool(PMemPool&& o) noexcept
    : base_(o.base_)
    , mapped_size_(o.mapped_size_)
    , is_dax_(o.is_dax_)
    , alignment_(o.alignment_)
    , header_(o.header_)
    , bump_(o.bump_.load())
    , flush_count_(o.flush_count_.load())
    , fd_(o.fd_) {
    o.base_        = nullptr;
    o.mapped_size_ = 0;
    o.header_      = nullptr;
    o.fd_          = -1;
}

PMemPool& PMemPool::operator=(PMemPool&& o) noexcept {
    if (this != &o) {
        unmap_region();
#ifndef _WIN32
        if (fd_ >= 0) {
          ::close(fd_);
        }
#endif
        base_        = o.base_;
        mapped_size_ = o.mapped_size_;
        is_dax_      = o.is_dax_;
        alignment_   = o.alignment_;
        header_      = o.header_;
        bump_.store(o.bump_.load());
        flush_count_.store(o.flush_count_.load());
        fd_          = o.fd_;
        o.base_      = nullptr;
        o.mapped_size_ = 0;
        o.header_    = nullptr;
        o.fd_        = -1;
    }
    return *this;
}

void PMemPool::map_region(const Config& config) {
    const size_t size = align_up(config.pool_size, kDAXAlignment);

#ifdef _WIN32
    // Windows: use CreateFileMapping / MapViewOfFile
    HANDLE hFile = CreateFileA(
        config.path.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        0, nullptr,
        config.create_if_missing ? OPEN_ALWAYS : OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL, nullptr);

    if (hFile == INVALID_HANDLE_VALUE) {
        return;
    }

    // Extend file to desired size if creating
    LARGE_INTEGER li;
    li.QuadPart = static_cast<LONGLONG>(size);
    SetFilePointerEx(hFile, li, nullptr, FILE_BEGIN);
    SetEndOfFile(hFile);

    HANDLE hMap = CreateFileMappingA(
        hFile, nullptr, PAGE_READWRITE,
        static_cast<DWORD>(size >> 32),
        static_cast<DWORD>(size & 0xFFFFFFFF),
        nullptr);
    CloseHandle(hFile);

    if (!hMap) {
        return;
    }

    void* addr = MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, size);
    CloseHandle(hMap);

    base_        = addr;
    mapped_size_ = addr ? size : 0;
    is_dax_      = false; // Windows does not expose DAX via ordinary mapping
#else
    int flags = O_RDWR;
    if (config.create_if_missing) {
        flags |= O_CREAT;
    }
    fd_ = ::open(config.path.c_str(), flags, 0644);
    if (fd_ < 0) {
        return;
    }

    // Extend file to desired size
    if (::ftruncate(fd_, static_cast<off_t>(size)) != 0) {
        ::close(fd_);
        fd_ = -1;
        return;
    }

    // Attempt DAX mmap (MAP_SHARED_VALIDATE | MAP_SYNC)
    void* addr = nullptr;
    if (config.use_dax) {
        addr = ::mmap(nullptr, size,
                      PROT_READ | PROT_WRITE,
                      MAP_SHARED_VALIDATE | MAP_SYNC,
                      fd_, 0);
        if (addr != MAP_FAILED) {
            is_dax_ = true;
        } else {
            addr = MAP_FAILED; // retry below
        }
    }

    if (addr == MAP_FAILED || addr == nullptr) {
        // Fallback: regular shared mmap
        addr = ::mmap(nullptr, size,
                      PROT_READ | PROT_WRITE,
                      MAP_SHARED,
                      fd_, 0);
        is_dax_ = false;
    }

    if (addr == MAP_FAILED) {
        addr = nullptr;
    }

    base_        = addr;
    mapped_size_ = addr ? size : 0;
#endif
}

void PMemPool::unmap_region() noexcept {
    if (!base_ || !mapped_size_) {
        return;
    }
#ifdef _WIN32
    UnmapViewOfFile(base_);
#else
    ::munmap(base_, mapped_size_);
#endif
    base_        = nullptr;
    mapped_size_ = 0;
    header_      = nullptr;
}

void PMemPool::init_header(size_t pool_size, size_t alignment) noexcept {
    // Compute layout offsets
    const size_t header_end   = sizeof(PMemPoolHeader);
    // One bit per alignment-granule block; bitmap size in bytes
    const size_t num_blocks   = pool_size / alignment;
    const size_t bitmap_bytes = align_up(num_blocks / 8 + 1, alignment);
    const size_t data_offset  = align_up(header_end + bitmap_bytes, alignment);

    header_->magic         = kMagic;
    header_->version       = 1;
    header_->pool_size     = pool_size;
    header_->bitmap_offset = header_end;
    header_->data_offset   = data_offset;
    header_->bump_offset   = data_offset; // start of free arena
    header_->alloc_count   = 0;
    header_->free_count    = 0;

    // Zero the bitmap
    auto* bitmap = static_cast<uint8_t*>(base_) + header_end;
    std::memset(bitmap, 0, bitmap_bytes);

    pmem_persist(base_, sizeof(PMemPoolHeader) + bitmap_bytes);
}

bool PMemPool::recover() noexcept {
    if (header_->magic != kMagic) {
        return false;
    }
    // Restore bump pointer from persisted state
    bump_.store(header_->bump_offset, std::memory_order_relaxed);
    return true;
}

void* PMemPool::allocate(size_t size) noexcept {
    if (!base_ || size == 0) {
        return nullptr;
    }

    const size_t padded = align_up(size, alignment_);
    const size_t limit  = mapped_size_;

    // CAS-based bump allocation
    size_t old_bump = bump_.load(std::memory_order_relaxed);
    size_t new_bump = 0;
    do {
        new_bump = old_bump + padded;
        if (new_bump > limit) {
            return nullptr; // pool exhausted
        }
    } while (!bump_.compare_exchange_weak(
                 old_bump, new_bump,
                 std::memory_order_acq_rel,
                 std::memory_order_relaxed));

    // Persist updated bump pointer in header
    header_->bump_offset = new_bump;
    header_->alloc_count++;
    pmem_persist(header_, sizeof(PMemPoolHeader));

    return static_cast<char*>(base_) + old_bump;
}

void PMemPool::free(void* /*ptr*/, size_t /*size*/) noexcept {
    // Append-only allocator: record the free for stats but do not reuse.
    if (header_) {
        header_->free_count++;
        pmem_persist(&header_->free_count, sizeof(header_->free_count));
    }
}

void PMemPool::persist(const void* ptr, size_t size) noexcept {
    pmem_persist(ptr, size);
    flush_count_.fetch_add(1, std::memory_order_relaxed);
}

PMemPool::Stats PMemPool::get_stats() const noexcept {
    Stats s;
    s.is_dax      = is_dax_;
    s.is_healthy  = (base_ != nullptr && header_ != nullptr &&
                     header_->magic == kMagic);
    if (!s.is_healthy) {
        return s;
    }
    s.total_bytes  = header_->pool_size;
    s.used_bytes   = bump_.load(std::memory_order_relaxed) - header_->data_offset;
    s.free_bytes   = (s.used_bytes <= s.total_bytes) ? (s.total_bytes - s.used_bytes) : 0;
    s.alloc_count  = header_->alloc_count;
    s.free_count   = header_->free_count;
    s.flush_count  = flush_count_.load(std::memory_order_relaxed);
    return s;
}

// ---------------------------------------------------------------------------
// PMemStorageLayout implementation
// ---------------------------------------------------------------------------

PMemStorageLayout::PMemStorageLayout(const Config& config)
    : pool_(PMemPool::Config{
          .path              = config.path,
          .pool_size         = config.pool_size,
          .alignment         = config.write_granule,
          .create_if_missing = config.create_if_missing,
          .use_dax           = config.use_dax,
          .recover_on_open   = true,
      })
    , write_granule_(config.write_granule) {}

void* PMemStorageLayout::write(const std::string& /*key*/,
                               const void* data,
                               size_t       len) {
    if (!data || len == 0) {
        return nullptr;
    }

    // Pad to write_granule_ to avoid partial cache-line updates
    const size_t padded = align_up(len, write_granule_);
    void* dst = pool_.allocate(padded);
    if (!dst) {
        return nullptr;
    }

    // Copy payload; zero-pad the tail
    std::memcpy(dst, data, len);
    if (padded > len) {
        std::memset(static_cast<char*>(dst) + len, 0, padded - len);
    }

    // Persist to PMem
    pool_.persist(dst, padded);

    writes_.fetch_add(1, std::memory_order_relaxed);
    bytes_written_.fetch_add(len, std::memory_order_relaxed);
    bytes_persisted_.fetch_add(padded, std::memory_order_relaxed);

    return dst;
}

void PMemStorageLayout::flush_all() noexcept {
    if (pool_.base()) {
        const auto stats = pool_.get_stats();
        pmem_persist(pool_.base(), stats.used_bytes);
    }
}

PMemStorageLayout::WriteStats PMemStorageLayout::get_write_stats() const noexcept {
    WriteStats ws;
    ws.writes          = writes_.load(std::memory_order_relaxed);
    ws.bytes_written   = bytes_written_.load(std::memory_order_relaxed);
    ws.bytes_persisted = bytes_persisted_.load(std::memory_order_relaxed);
    if (ws.bytes_written > 0) {
        ws.write_amplification_x1000 =
            (ws.bytes_persisted * 1000) / ws.bytes_written;
    }
    return ws;
}

PMemPool::Stats PMemStorageLayout::get_pool_stats() const noexcept {
    return pool_.get_stats();
}

bool PMemStorageLayout::is_dax() const noexcept {
    return pool_.is_dax();
}

} // namespace phase4
} // namespace performance
} // namespace themis
