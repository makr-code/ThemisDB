#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <limits>
#include <cstring>
#include <random>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#ifdef _WIN32
#include <d3d11.h>
#include <dxgi.h>
#include <intrin.h>
#include <windows.h>
#include <winioctl.h>
#endif

namespace {

struct CpuSample {
    double elapsed_s = 0;
    double ops_per_s;
    std::uint64_t ops;
};

struct CpuFeatures {
    bool sse42 = 0;
    bool avx;
    bool avx2;
    bool avx512f;
    bool fma;
    bool aes;
    bool bmi1;
    bool bmi2;
    bool popcnt;
};

struct StreamSample {
    double copy_gb_s = 0;
    double scale_gb_s;
    double add_gb_s;
    double triad_gb_s;
};

CpuSample sampleIntegerCpu(double seconds) {
    using clock = std::chrono::steady_clock;
    const auto start = clock::now();
    std::uint64_t ops = 0;
    std::uint64_t x = 1;

    while (std::chrono::duration<double>(clock::now() - start).count() < seconds) {
        x = (x * 1664525ull + 1013904223ull) & 0x7FFFFFFFull;
        ++ops;
    }

    const double elapsed = std::chrono::duration<double>(clock::now() - start).count();
    (void)x;
    return {elapsed, ops / elapsed, ops};
}

CpuSample sampleFloatCpu(double seconds) {
    using clock = std::chrono::steady_clock;
    const auto start = clock::now();
    std::uint64_t ops = 0;
    double x = 1.000001;

    while (std::chrono::duration<double>(clock::now() - start).count() < seconds) {
        x = std::sqrt(x * 1.000000119 + 0.0000003);
        ++ops;
    }

    const double elapsed = std::chrono::duration<double>(clock::now() - start).count();
    (void)x;
    return {elapsed, ops / elapsed, ops};
}

double sampleMemoryCopyMBs(std::size_t sizeMB, int iterations) {
    const std::size_t bytes = sizeMB * 1024ull * 1024ull;
    std::vector<std::uint8_t> src(bytes, 0);
    std::vector<std::uint8_t> dst(bytes, 0);

    for (std::size_t i = 0; i < bytes; i += 4096) {
        src[i] = static_cast<std::uint8_t>(i % 251);
    }

    const auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < iterations; ++i) {
        std::copy(src.begin(), src.end(), dst.begin());
    }
    const auto end = std::chrono::steady_clock::now();

    const double elapsed = std::chrono::duration<double>(end - start).count();
    const double totalMB = (static_cast<double>(bytes) * iterations) / (1024.0 * 1024.0);
    return totalMB / elapsed;
}

struct DiskSample {
    double write_mb_s = 0;
    double read_mb_s;
    double random_read_iops;
    double random_write_iops;
};

struct DiskInventory {
    std::string volume_root;
    std::string drive_type;
    double total_gb;
    double free_gb;
};

struct GpuInventory {
    std::string name;
    std::uint32_t vendor_id;
    std::uint32_t device_id;
    double dedicated_vram_gb;
};

struct CapabilityTier {
    std::string simd;
    std::string memory;
    std::string storage;
    std::string gpu;
};

struct TransferSample {
    bool available = 0;
    double host_to_vram_gb_s;
    double vram_to_host_gb_s;
    double cpu_to_gpu_dispatch_us;
    std::string backend;
    std::string reason;
    std::size_t payload_bytes;
    int iterations;
};

double bytesToGB(double bytes) {
    return bytes / (1024.0 * 1024.0 * 1024.0);
}

std::string jsonEscape(const std::string& input) {
    std::string out;
    out.reserve(input.size());
    for (char c : input) {
        if (c == '\\') {
            out += "\\\\";
        } else if (c == '"') {
            out += "\\\"";
        } else {
            out += c;
        }
    }
    return out;
}

#ifdef _WIN32
std::string utf8FromWide(const wchar_t* ws) {
    if (ws == nullptr) {
        return "unknown";
    }

    const int size = WideCharToMultiByte(CP_UTF8, 0, ws, -1, nullptr, 0, nullptr, nullptr);
    if (size <= 1) {
        return "unknown";
    }

    std::string out(static_cast<std::size_t>(size - 1), '\0');
    WideCharToMultiByte(CP_UTF8, 0, ws, -1, out.data(), size, nullptr, nullptr);
    return out;
}
#endif

CpuFeatures detectCpuFeatures() {
    CpuFeatures f{false, false, false, false, false, false, false, false, false};

#ifdef _WIN32
    int regs[4] = {0, 0, 0, 0};
    __cpuid(regs, 0);
    const int maxLeaf = regs[0];

    if (maxLeaf >= 1) {
        __cpuidex(regs, 1, 0);
        const int ecx = regs[2];

        const bool hwAvx = (ecx & (1 << 28)) != 0;
        const bool osxsave = (ecx & (1 << 27)) != 0;
        bool osAvxEnabled = false;
        if (hwAvx && osxsave) {
            const std::uint64_t xcr0 = _xgetbv(0);
            osAvxEnabled = (xcr0 & 0x6) == 0x6;
        }

        f.sse42 = (ecx & (1 << 20)) != 0;
        f.avx = hwAvx && osAvxEnabled;
        f.fma = ((ecx & (1 << 12)) != 0) && osAvxEnabled;
        f.aes = (ecx & (1 << 25)) != 0;
        f.popcnt = (ecx & (1 << 23)) != 0;
    }

    if (maxLeaf >= 7) {
        __cpuidex(regs, 7, 0);
        const int ebx = regs[1];
        const bool hwAvx2 = (ebx & (1 << 5)) != 0;
        const bool hwAvx512f = (ebx & (1 << 16)) != 0;
        const bool hwBmi1 = (ebx & (1 << 3)) != 0;
        const bool hwBmi2 = (ebx & (1 << 8)) != 0;

        bool osAvx512Enabled = false;
        if (f.avx && hwAvx512f) {
            const std::uint64_t xcr0 = _xgetbv(0);
            osAvx512Enabled = (xcr0 & 0xE6) == 0xE6;
        }

        f.avx2 = hwAvx2 && f.avx;
        f.avx512f = hwAvx512f && osAvx512Enabled;
        f.bmi1 = hwBmi1;
        f.bmi2 = hwBmi2;
    }
#endif

    return f;
}

std::string deriveSimdTier(const CpuFeatures& f) {
    if (f.avx512f) {
        return "avx512";
    }
    if (f.avx2 && f.fma) {
        return "avx2_fma";
    }
    if (f.sse42) {
        return "sse42";
    }
    return "scalar";
}

DiskSample sampleDiskSequential(const std::filesystem::path& filePath, std::size_t fileSizeMB, std::size_t blockMB) {
    const std::size_t fileBytes = fileSizeMB * 1024ull * 1024ull;
    const std::size_t blockBytes = blockMB * 1024ull * 1024ull;

    std::vector<std::uint8_t> block(blockBytes);
    std::mt19937 rng(42);
    std::uniform_int_distribution<int> dist(0, 255);
    for (auto& b : block) {
        b = static_cast<std::uint8_t>(dist(rng));
    }

    {
        std::ofstream out(filePath, std::ios::binary | std::ios::trunc);
        if (!out.good()) {
            ADD_FAILURE() << "Failed to open baseline disk file for write: " << filePath.string();
            return {0.0, 0.0, 0.0, 0.0};
        }
        auto start = std::chrono::steady_clock::now();
        std::size_t written = 0;
        while (written < fileBytes) {
            const std::size_t chunk = std::min(blockBytes, fileBytes - written);
            out.write(reinterpret_cast<const char*>(block.data()), static_cast<std::streamsize>(chunk));
            written += chunk;
        }
        out.flush();
        out.close();
        auto end = std::chrono::steady_clock::now();
        const double elapsed = std::chrono::duration<double>(end - start).count();

        std::ifstream in(filePath, std::ios::binary);
        if (!in.good()) {
            ADD_FAILURE() << "Failed to open baseline disk file for read: " << filePath.string();
            return {0.0, 0.0, 0.0, 0.0};
        }
        start = std::chrono::steady_clock::now();
        std::size_t read = 0;
        while (read < fileBytes && in.good()) {
            const std::size_t chunk = std::min(blockBytes, fileBytes - read);
            in.read(reinterpret_cast<char*>(block.data()), static_cast<std::streamsize>(chunk));
            const auto got = static_cast<std::size_t>(in.gcount());
            if (got == 0) {
                break;
            }
            read += got;
        }
        end = std::chrono::steady_clock::now();

        const double writeMBs = static_cast<double>(fileSizeMB) / elapsed;
        const double readElapsed = std::chrono::duration<double>(end - start).count();
        const double readMBs = static_cast<double>(fileSizeMB) / readElapsed;
        return {writeMBs, readMBs, 0.0, 0.0};
    }
}

double sampleRandomIops(const std::filesystem::path& filePath, std::size_t fileSizeMB, bool writeMode) {
    const std::size_t blockSize = 4096;
    const std::size_t fileBytes = fileSizeMB * 1024ull * 1024ull;
    const std::size_t blockCount = fileBytes / blockSize;
    if (blockCount == 0) {
        return 0.0;
    }

    std::fstream fs(filePath, std::ios::binary | std::ios::in | std::ios::out);
    if (!fs.good()) {
        ADD_FAILURE() << "Failed to open disk file for random IOPS: " << filePath.string();
        return 0.0;
    }

    std::vector<std::uint8_t> block(blockSize, 0xA5);
    std::mt19937_64 rng(1337);
    std::uniform_int_distribution<std::size_t> dist(0, blockCount - 1);
    constexpr int kOps = 3000;

    const auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < kOps; ++i) {
        const std::size_t offset = dist(rng) * blockSize;
        if (writeMode) {
            fs.seekp(static_cast<std::streamoff>(offset), std::ios::beg);
            fs.write(reinterpret_cast<const char*>(block.data()), static_cast<std::streamsize>(block.size()));
        } else {
            fs.seekg(static_cast<std::streamoff>(offset), std::ios::beg);
            fs.read(reinterpret_cast<char*>(block.data()), static_cast<std::streamsize>(block.size()));
        }
    }
    fs.flush();
    const auto end = std::chrono::steady_clock::now();
    const double elapsed = std::chrono::duration<double>(end - start).count();

    if (elapsed <= 0.0) {
        return 0.0;
    }
    return static_cast<double>(kOps) / elapsed;
}

StreamSample sampleStreamLikeBandwidth(std::size_t n, int iterations) {
    std::vector<double> a(n, 1.0);
    std::vector<double> b(n, 2.0);
    std::vector<double> c(n, 0.0);

    const auto kernel = [&](auto&& fn, double bytesMoved) -> double {
        double best = 0.0;
        for (int i = 0; i < iterations; ++i) {
            const auto start = std::chrono::steady_clock::now();
            fn();
            const auto end = std::chrono::steady_clock::now();
            const double elapsed = std::chrono::duration<double>(end - start).count();
            if (elapsed > 0.0) {
                const double gbS = (bytesMoved / elapsed) / (1024.0 * 1024.0 * 1024.0);
                best = std::max(best, gbS);
            }
        }
        return best;
    };

    const double bytesCopy = static_cast<double>(2ull * n * sizeof(double));
    const double bytesScale = static_cast<double>(2ull * n * sizeof(double));
    const double bytesAdd = static_cast<double>(3ull * n * sizeof(double));
    const double bytesTriad = static_cast<double>(3ull * n * sizeof(double));

    const double copy = kernel([&]() {
        for (std::size_t i = 0; i < n; ++i) {
            c[i] = a[i];
        }
    }, bytesCopy);

    const double scale = kernel([&]() {
        for (std::size_t i = 0; i < n; ++i) {
            b[i] = 3.0 * c[i];
        }
    }, bytesScale);

    const double add = kernel([&]() {
        for (std::size_t i = 0; i < n; ++i) {
            c[i] = a[i] + b[i];
        }
    }, bytesAdd);

    const double triad = kernel([&]() {
        for (std::size_t i = 0; i < n; ++i) {
            a[i] = b[i] + 3.0 * c[i];
        }
    }, bytesTriad);

    return {copy, scale, add, triad};
}

DiskInventory detectDiskInventory(const std::filesystem::path& path) {
#ifdef _WIN32
    const std::filesystem::path rootPath = path.root_path().empty() ? std::filesystem::path("C:\\") : path.root_path();
    const std::wstring root = rootPath.wstring();

    ULARGE_INTEGER freeBytesAvailable{};
    ULARGE_INTEGER totalBytes{};
    ULARGE_INTEGER totalFreeBytes{};

    double totalGB = 0.0;
    double freeGB = 0.0;
    if (GetDiskFreeSpaceExW(root.c_str(), &freeBytesAvailable, &totalBytes, &totalFreeBytes)) {
        totalGB = static_cast<double>(totalBytes.QuadPart) / (1024.0 * 1024.0 * 1024.0);
        freeGB = static_cast<double>(totalFreeBytes.QuadPart) / (1024.0 * 1024.0 * 1024.0);
    }

    std::string driveType = "UNKNOWN";
    const UINT type = GetDriveTypeW(root.c_str());
    if (type == DRIVE_FIXED) {
        driveType = "FIXED";
    } else if (type == DRIVE_REMOVABLE) {
        driveType = "REMOVABLE";
    } else if (type == DRIVE_REMOTE) {
        driveType = "REMOTE";
    } else if (type == DRIVE_CDROM) {
        driveType = "CDROM";
    } else if (type == DRIVE_RAMDISK) {
        driveType = "RAMDISK";
    }

    std::string mediaType = "UNKNOWN";
    if (root.size() >= 2 && root[1] == L':') {
        std::wstring device = L"\\\\.\\";
        device += root.substr(0, 2);
        HANDLE h = CreateFileW(device.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING,
                               FILE_ATTRIBUTE_NORMAL, nullptr);
        if (h != INVALID_HANDLE_VALUE) {
            STORAGE_PROPERTY_QUERY query{};
            query.PropertyId = StorageDeviceSeekPenaltyProperty;
            query.QueryType = PropertyStandardQuery;
            DEVICE_SEEK_PENALTY_DESCRIPTOR seekPenalty{};
            DWORD returned = 0;
            if (DeviceIoControl(h, IOCTL_STORAGE_QUERY_PROPERTY, &query, sizeof(query), &seekPenalty,
                                sizeof(seekPenalty), &returned, nullptr)) {
                mediaType = seekPenalty.IncursSeekPenalty ? "HDD" : "SSD_OR_NVME";
            }
            CloseHandle(h);
        }
    }

    if (mediaType != "UNKNOWN") {
        driveType += "_" + mediaType;
    }

    return {utf8FromWide(root.c_str()), driveType, totalGB, freeGB};
#else
    (void)path;
    return {"unknown", "UNKNOWN", 0.0, 0.0};
#endif
}

GpuInventory detectPrimaryGpu() {
#ifdef _WIN32
    IDXGIFactory1* factory = nullptr;
    const HRESULT createHr = CreateDXGIFactory1(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(&factory));
    if (FAILED(createHr) || factory == nullptr) {
        return {"unknown", 0u, 0u, 0.0};
    }

    IDXGIAdapter1* adapter = nullptr;
    DXGI_ADAPTER_DESC1 bestDesc{};
    bool found = false;
    SIZE_T bestVram = 0;

    for (UINT i = 0; factory->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND; ++i) {
        DXGI_ADAPTER_DESC1 desc{};
        if (FAILED(adapter->GetDesc1(&desc))) {
            adapter->Release();
            continue;
        }

        const bool software = (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) != 0;
        if (!software && desc.DedicatedVideoMemory >= bestVram) {
            bestDesc = desc;
            bestVram = desc.DedicatedVideoMemory;
            found = true;
        }
        adapter->Release();
    }

    if (!found) {
        factory->Release();
        return {"unknown", 0, 0, 0.0};
    }

    const std::string name = utf8FromWide(bestDesc.Description);
    const double vramGB = bytesToGB(static_cast<double>(bestDesc.DedicatedVideoMemory));
    factory->Release();
    return {name, bestDesc.VendorId, bestDesc.DeviceId, vramGB};
#else
    return {"unknown", 0, 0, 0.0};
#endif
}

CapabilityTier deriveCapabilityTier(const CpuFeatures& cpuFeatures,
                                    const StreamSample& stream,
                                    const DiskSample& disk,
                                    const GpuInventory& gpu) {
    CapabilityTier tier{};
    tier.simd = deriveSimdTier(cpuFeatures);

    const double mem = stream.triad_gb_s;
    if (mem >= 35.0) {
        tier.memory = "high";
    } else if (mem >= 18.0) {
        tier.memory = "medium";
    } else {
        tier.memory = "low";
    }

    if (disk.read_mb_s >= 1200.0 && disk.random_read_iops >= 50000.0) {
        tier.storage = "nvme_class";
    } else if (disk.read_mb_s >= 400.0 && disk.random_read_iops >= 10000.0) {
        tier.storage = "ssd_class";
    } else {
        tier.storage = "hdd_class_or_limited";
    }

    if (gpu.dedicated_vram_gb >= 16.0) {
        tier.gpu = "high";
    } else if (gpu.dedicated_vram_gb >= 8.0) {
        tier.gpu = "medium";
    } else if (gpu.dedicated_vram_gb > 0.0) {
        tier.gpu = "entry";
    } else {
        tier.gpu = "none_or_unknown";
    }

    return tier;
}

std::string detectTotalMemoryGB() {
#ifdef _WIN32
    MEMORYSTATUSEX mem;
    mem.dwLength = sizeof(mem);
    if (GlobalMemoryStatusEx(&mem)) {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2) << (static_cast<double>(mem.ullTotalPhys) / (1024.0 * 1024.0 * 1024.0));
        return oss.str();
    }
#endif
    return "unknown";
}

std::string isoNowUtc() {
    const auto now = std::chrono::system_clock::now();
    const std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tmv{};
#ifdef _WIN32
    gmtime_s(&tmv, &t);
#else
    gmtime_r(&t, &tmv);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tmv, "%Y-%m-%dT%H:%M:%SZ");
    return oss.str();
}

std::string detectBuildType() {
#ifdef NDEBUG
    return "Release";
#else
    return "Debug";
#endif
}

std::string detectCompilerId() {
#ifdef _MSC_VER
    return "MSVC";
#elif defined(__clang__)
    return "Clang";
#elif defined(__GNUC__)
    return "GCC";
#else
    return "unknown";
#endif
}

std::string detectCompilerVersion() {
#ifdef _MSC_FULL_VER
    return std::to_string(_MSC_FULL_VER);
#elif defined(__clang_major__) && defined(__clang_minor__) && defined(__clang_patchlevel__)
    return std::to_string(__clang_major__) + "." + std::to_string(__clang_minor__) + "." + std::to_string(__clang_patchlevel__);
#elif defined(__GNUC__) && defined(__GNUC_MINOR__) && defined(__GNUC_PATCHLEVEL__)
    return std::to_string(__GNUC__) + "." + std::to_string(__GNUC_MINOR__) + "." + std::to_string(__GNUC_PATCHLEVEL__);
#else
    return "unknown";
#endif
}

std::string detectGitCommit() {
#ifdef THEMIS_GIT_COMMIT
    return THEMIS_GIT_COMMIT;
#else
    return "unknown";
#endif
}

std::string detectOsName() {
#ifdef _WIN32
    return "windows";
#else
    return "unknown";
#endif
}

TransferSample sampleGpuTransfers(std::size_t payloadMB, int iterations) {
#ifdef _WIN32
    const std::size_t payloadBytes = payloadMB * 1024ull * 1024ull;
    if (payloadBytes == 0 || iterations <= 0 || payloadBytes > static_cast<std::size_t>(std::numeric_limits<UINT>::max())) {
        return {false, 0.0, 0.0, 0.0, "d3d11", "invalid_parameters", payloadBytes, iterations};
    }

    ID3D11Device* device = nullptr;
    ID3D11DeviceContext* context = nullptr;
    D3D_FEATURE_LEVEL level{};
    const HRESULT createHr = D3D11CreateDevice(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        0,
        nullptr,
        0,
        D3D11_SDK_VERSION,
        &device,
        &level,
        &context);
    if (FAILED(createHr) || device == nullptr || context == nullptr) {
        if (context != nullptr) {
          context->Release();
        }
        if (device != nullptr) {
          device->Release();
        }
        return {false, 0.0, 0.0, 0.0, "d3d11", "device_creation_failed", payloadBytes, iterations};
    }

    D3D11_BUFFER_DESC gpuDesc{};
    gpuDesc.ByteWidth = static_cast<UINT>(payloadBytes);
    gpuDesc.Usage = D3D11_USAGE_DEFAULT;
    gpuDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_BUFFER_DESC uploadDesc{};
    uploadDesc.ByteWidth = static_cast<UINT>(payloadBytes);
    uploadDesc.Usage = D3D11_USAGE_STAGING;
    uploadDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    D3D11_BUFFER_DESC readbackDesc{};
    readbackDesc.ByteWidth = static_cast<UINT>(payloadBytes);
    readbackDesc.Usage = D3D11_USAGE_STAGING;
    readbackDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

    ID3D11Buffer* gpuBuffer = nullptr;
    ID3D11Buffer* uploadBuffer = nullptr;
    ID3D11Buffer* readbackBuffer = nullptr;
    ID3D11Query* query = nullptr;

    const HRESULT gpuHr = device->CreateBuffer(&gpuDesc, nullptr, &gpuBuffer);
    const HRESULT uploadHr = device->CreateBuffer(&uploadDesc, nullptr, &uploadBuffer);
    const HRESULT readbackHr = device->CreateBuffer(&readbackDesc, nullptr, &readbackBuffer);

    D3D11_QUERY_DESC qd{};
    qd.Query = D3D11_QUERY_EVENT;
    const HRESULT queryHr = device->CreateQuery(&qd, &query);

    if (FAILED(gpuHr) || FAILED(uploadHr) || FAILED(readbackHr) || FAILED(queryHr) ||
        gpuBuffer == nullptr || uploadBuffer == nullptr || readbackBuffer == nullptr || query == nullptr) {
        if (query != nullptr) {
          query->Release();
        }
        if (readbackBuffer != nullptr) {
          readbackBuffer->Release();
        }
        if (uploadBuffer != nullptr) {
          uploadBuffer->Release();
        }
        if (gpuBuffer != nullptr) {
          gpuBuffer->Release();
        }
        context->Release();
        device->Release();
        return {false, 0.0, 0.0, 0.0, "d3d11", "buffer_or_query_creation_failed", payloadBytes, iterations};
    }

    auto waitForGpu = [&]() {
        context->End(query);
        while (context->GetData(query, nullptr, 0, 0) == S_FALSE) {
            Sleep(0);
        }
    };

    std::vector<std::uint8_t> host(payloadBytes, 0x3A);
    std::vector<std::uint8_t> sink(payloadBytes, 0);

    D3D11_MAPPED_SUBRESOURCE mapped{};
    if (SUCCEEDED(context->Map(uploadBuffer, 0, D3D11_MAP_WRITE, 0, &mapped))) {
        std::memcpy(mapped.pData, host.data(), payloadBytes);
        context->Unmap(uploadBuffer, 0);
        context->CopyResource(gpuBuffer, uploadBuffer);
        waitForGpu();
    }

    const auto h2dStart = std::chrono::steady_clock::now();
    for (int i = 0; i < iterations; ++i) {
        if (FAILED(context->Map(uploadBuffer, 0, D3D11_MAP_WRITE, 0, &mapped))) {
            continue;
        }
        std::memcpy(mapped.pData, host.data(), payloadBytes);
        context->Unmap(uploadBuffer, 0);
        context->CopyResource(gpuBuffer, uploadBuffer);
        waitForGpu();
    }
    const auto h2dEnd = std::chrono::steady_clock::now();

    const auto d2hStart = std::chrono::steady_clock::now();
    for (int i = 0; i < iterations; ++i) {
        context->CopyResource(readbackBuffer, gpuBuffer);
        waitForGpu();
        if (FAILED(context->Map(readbackBuffer, 0, D3D11_MAP_READ, 0, &mapped))) {
            continue;
        }
        std::memcpy(sink.data(), mapped.pData, payloadBytes);
        context->Unmap(readbackBuffer, 0);
    }
    const auto d2hEnd = std::chrono::steady_clock::now();

    const auto dispatchStart = std::chrono::steady_clock::now();
    for (int i = 0; i < iterations; ++i) {
        waitForGpu();
    }
    const auto dispatchEnd = std::chrono::steady_clock::now();

    const double h2dElapsed = std::chrono::duration<double>(h2dEnd - h2dStart).count();
    const double d2hElapsed = std::chrono::duration<double>(d2hEnd - d2hStart).count();
    const double dispatchElapsed = std::chrono::duration<double>(dispatchEnd - dispatchStart).count();

    const double totalBytes = static_cast<double>(payloadBytes) * iterations;
    const double h2dGbS = h2dElapsed > 0.0 ? (totalBytes / h2dElapsed) / (1024.0 * 1024.0 * 1024.0) : 0.0;
    const double d2hGbS = d2hElapsed > 0.0 ? (totalBytes / d2hElapsed) / (1024.0 * 1024.0 * 1024.0) : 0.0;
    const double dispatchUs = dispatchElapsed > 0.0 ? (dispatchElapsed * 1e6) / static_cast<double>(iterations) : 0.0;

    volatile std::uint8_t guard = sink.empty() ? 0 : sink[0];
    (void)guard;

    query->Release();
    readbackBuffer->Release();
    uploadBuffer->Release();
    gpuBuffer->Release();
    context->Release();
    device->Release();

    if (h2dGbS <= 0.0 || d2hGbS <= 0.0 || dispatchUs <= 0.0) {
        return {false, h2dGbS, d2hGbS, dispatchUs, "d3d11", "measurement_failed", payloadBytes, iterations};
    }
    return {true, h2dGbS, d2hGbS, dispatchUs, "d3d11", "", payloadBytes, iterations};
#else
    (void)payloadMB;
    (void)iterations;
    return {false, 0.0, 0.0, 0.0, "none", "unsupported_platform", 0u, 0};
#endif
}

}  // namespace

TEST(HardwareBaseline, CaptureAndPersist) {
    namespace fs = std::filesystem;

    const auto root = fs::current_path();
    const auto outDir = root / "logs" / "hardware_baseline";
    fs::create_directories(outDir);

    const auto ts = std::to_string(
        std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch())
            .count());
    const auto diskFile = outDir / ("baseline_disk_" + ts + ".bin");
    const auto outFile = outDir / ("hardware_baseline_gtest_" + ts + ".json");

    const CpuFeatures cpuFeatures = detectCpuFeatures();
    const CpuSample cpuInt = sampleIntegerCpu(0.25);
    const CpuSample cpuFloat = sampleFloatCpu(0.25);
    const double memMBs = sampleMemoryCopyMBs(64, 8);
    const StreamSample stream = sampleStreamLikeBandwidth(2'000'000, 4);
    DiskSample disk = sampleDiskSequential(diskFile, 128, 4);
    disk.random_read_iops = sampleRandomIops(diskFile, 128, false);
    disk.random_write_iops = sampleRandomIops(diskFile, 128, true);
    const DiskInventory diskInv = detectDiskInventory(root);
    const GpuInventory gpuInv = detectPrimaryGpu();
    const CapabilityTier tier = deriveCapabilityTier(cpuFeatures, stream, disk, gpuInv);
    const std::string runId = "hardware-baseline-" + ts;
    constexpr int kCpuWarmupIterations = 0;
    constexpr int kCpuSampleCount = 2;
    constexpr int kMemCopyWarmupIterations = 1;
    constexpr int kMemCopyIterations = 8;
    constexpr int kStreamWarmupIterations = 1;
    constexpr int kStreamIterations = 4;
    constexpr int kDiskSeqFileSizeMB = 128;
    constexpr int kDiskSeqBlockMB = 4;
    constexpr int kDiskRandomBlockBytes = 4096;
    constexpr int kDiskRandomOps = 3000;
    constexpr int kTransferPayloadMB = 64;
    constexpr int kTransferIterations = 20;
    const TransferSample transfer = sampleGpuTransfers(kTransferPayloadMB, kTransferIterations);

    std::ostringstream json;
    json << "{\n";
    json << "  \"schema_version\": \"1.1\",\n";
    json << "  \"run_id\": \"" << runId << "\",\n";
    json << "  \"generated_at_utc\": \"" << isoNowUtc() << "\",\n";
    json << "  \"context\": {\n";
    json << "    \"host_fingerprint\": {\n";
    json << "      \"cpu_simd_tier\": \"" << tier.simd << "\",\n";
    json << "      \"logical_cpus\": " << std::thread::hardware_concurrency() << ",\n";
    json << "      \"ram_gb\": \"" << detectTotalMemoryGB() << "\",\n";
    json << "      \"gpu_vendor_device\": \"" << gpuInv.vendor_id << ":" << gpuInv.device_id << "\",\n";
    json << "      \"storage_root\": \"" << jsonEscape(diskInv.volume_root) << "\"\n";
    json << "    },\n";
    json << "    \"build_context\": {\n";
    json << "      \"git_commit\": \"" << jsonEscape(detectGitCommit()) << "\",\n";
    json << "      \"build_type\": \"" << detectBuildType() << "\",\n";
    json << "      \"compiler_id\": \"" << detectCompilerId() << "\",\n";
    json << "      \"compiler_version\": \"" << detectCompilerVersion() << "\",\n";
    json << "      \"simd_flags\": \"" << tier.simd << "\"\n";
    json << "    },\n";
    json << "    \"os_context\": {\n";
    json << "      \"os_name\": \"" << detectOsName() << "\",\n";
    json << "      \"power_profile\": \"unknown\"\n";
    json << "    },\n";
    json << "    \"runtime_context\": {\n";
    json << "      \"api_backend\": \"" << transfer.backend << "\",\n";
    json << "      \"numa_policy\": \"default\",\n";
    json << "      \"thermal_state\": \"unknown\",\n";
    json << "      \"driver_version_gpu\": \"unknown\"\n";
    json << "    }\n";
    json << "  },\n";
    json << "  \"measurement_config\": {\n";
    json << "    \"cpu\": {\n";
    json << "      \"method\": \"fixed-time micro throughput\",\n";
    json << "      \"unit\": \"ops/s\",\n";
    json << "      \"warmup_iterations\": " << kCpuWarmupIterations << ",\n";
    json << "      \"sample_count\": " << kCpuSampleCount << "\n";
    json << "    },\n";
    json << "    \"memory_copy\": {\n";
    json << "      \"method\": \"host memcpy-like std::copy\",\n";
    json << "      \"unit\": \"MB/s\",\n";
    json << "      \"payload_mb\": 64,\n";
    json << "      \"warmup_iterations\": " << kMemCopyWarmupIterations << ",\n";
    json << "      \"measure_iterations\": " << kMemCopyIterations << "\n";
    json << "    },\n";
    json << "    \"stream\": {\n";
    json << "      \"method\": \"STREAM-like copy/scale/add/triad\",\n";
    json << "      \"unit\": \"GB/s\",\n";
    json << "      \"elements\": 2000000,\n";
    json << "      \"warmup_iterations\": " << kStreamWarmupIterations << ",\n";
    json << "      \"measure_iterations\": " << kStreamIterations << "\n";
    json << "    },\n";
    json << "    \"disk\": {\n";
    json << "      \"method\": \"sequential throughput + 4K random IOPS\",\n";
    json << "      \"seq_unit\": \"MB/s\",\n";
    json << "      \"random_unit\": \"IOPS\",\n";
    json << "      \"file_size_mb\": " << kDiskSeqFileSizeMB << ",\n";
    json << "      \"seq_block_mb\": " << kDiskSeqBlockMB << ",\n";
    json << "      \"random_block_bytes\": " << kDiskRandomBlockBytes << ",\n";
    json << "      \"random_ops\": " << kDiskRandomOps << "\n";
    json << "    },\n";
    json << "    \"transfer\": {\n";
    json << "      \"payload_mb\": " << kTransferPayloadMB << ",\n";
    json << "      \"measure_iterations\": " << kTransferIterations << ",\n";
    json << "      \"host_to_vram_gb_s\": {\n";
    json << "        \"available\": " << (transfer.available ? "true" : "false") << ",\n";
    json << "        \"reason\": \"" << jsonEscape(transfer.available ? "" : transfer.reason) << "\"\n";
    json << "      },\n";
    json << "      \"vram_to_host_gb_s\": {\n";
    json << "        \"available\": " << (transfer.available ? "true" : "false") << ",\n";
    json << "        \"reason\": \"" << jsonEscape(transfer.available ? "" : transfer.reason) << "\"\n";
    json << "      },\n";
    json << "      \"cpu_to_gpu_dispatch_us\": {\n";
    json << "        \"available\": " << (transfer.available ? "true" : "false") << ",\n";
    json << "        \"reason\": \"" << jsonEscape(transfer.available ? "" : transfer.reason) << "\"\n";
    json << "      }\n";
    json << "    }\n";
    json << "  },\n";
    json << "  \"hardware\": {\n";
    json << "    \"logical_cpus\": " << std::thread::hardware_concurrency() << ",\n";
    json << "    \"pointer_bits\": " << (8 * sizeof(void*)) << ",\n";
    json << "    \"cpu_features\": {\n";
    json << "      \"sse42\": " << (cpuFeatures.sse42 ? "true" : "false") << ",\n";
    json << "      \"avx\": " << (cpuFeatures.avx ? "true" : "false") << ",\n";
    json << "      \"avx2\": " << (cpuFeatures.avx2 ? "true" : "false") << ",\n";
    json << "      \"avx512f\": " << (cpuFeatures.avx512f ? "true" : "false") << ",\n";
    json << "      \"fma\": " << (cpuFeatures.fma ? "true" : "false") << ",\n";
    json << "      \"aes\": " << (cpuFeatures.aes ? "true" : "false") << ",\n";
    json << "      \"bmi1\": " << (cpuFeatures.bmi1 ? "true" : "false") << ",\n";
    json << "      \"bmi2\": " << (cpuFeatures.bmi2 ? "true" : "false") << ",\n";
    json << "      \"popcnt\": " << (cpuFeatures.popcnt ? "true" : "false") << "\n";
    json << "    },\n";
    json << "    \"ram_gb\": \"" << detectTotalMemoryGB() << "\",\n";
    json << "    \"gpu_name\": \"" << jsonEscape(gpuInv.name) << "\",\n";
    json << "    \"gpu_vendor_id\": " << gpuInv.vendor_id << ",\n";
    json << "    \"gpu_device_id\": " << gpuInv.device_id << ",\n";
    json << "    \"gpu_vram_gb\": " << std::fixed << std::setprecision(2) << gpuInv.dedicated_vram_gb << ",\n";
    json << "    \"hdd_volume_root\": \"" << jsonEscape(diskInv.volume_root) << "\",\n";
    json << "    \"hdd_drive_type\": \"" << diskInv.drive_type << "\",\n";
    json << "    \"hdd_total_gb\": " << std::fixed << std::setprecision(2) << diskInv.total_gb << ",\n";
    json << "    \"hdd_free_gb\": " << std::fixed << std::setprecision(2) << diskInv.free_gb << "\n";
    json << "  },\n";
    json << "  \"results\": {\n";
    json << "    \"cpu_integer_ops_per_s\": " << std::fixed << std::setprecision(2) << cpuInt.ops_per_s << ",\n";
    json << "    \"cpu_float_ops_per_s\": " << std::fixed << std::setprecision(2) << cpuFloat.ops_per_s << ",\n";
    json << "    \"memory_copy_mb_s\": " << std::fixed << std::setprecision(2) << memMBs << ",\n";
    json << "    \"stream_copy_gb_s\": " << std::fixed << std::setprecision(2) << stream.copy_gb_s << ",\n";
    json << "    \"stream_scale_gb_s\": " << std::fixed << std::setprecision(2) << stream.scale_gb_s << ",\n";
    json << "    \"stream_add_gb_s\": " << std::fixed << std::setprecision(2) << stream.add_gb_s << ",\n";
    json << "    \"stream_triad_gb_s\": " << std::fixed << std::setprecision(2) << stream.triad_gb_s << ",\n";
    json << "    \"disk_write_mb_s\": " << std::fixed << std::setprecision(2) << disk.write_mb_s << ",\n";
    json << "    \"disk_read_mb_s\": " << std::fixed << std::setprecision(2) << disk.read_mb_s << ",\n";
    json << "    \"disk_random_read_iops\": " << std::fixed << std::setprecision(2) << disk.random_read_iops << ",\n";
    json << "    \"disk_random_write_iops\": " << std::fixed << std::setprecision(2) << disk.random_write_iops << ",\n";
    json << "    \"host_to_vram_gb_s\": ";
    if (transfer.available) {
        json << std::fixed << std::setprecision(2) << transfer.host_to_vram_gb_s;
    } else {
        json << "null";
    }
    json << ",\n";
    json << "    \"vram_to_host_gb_s\": ";
    if (transfer.available) {
        json << std::fixed << std::setprecision(2) << transfer.vram_to_host_gb_s;
    } else {
        json << "null";
    }
    json << ",\n";
    json << "    \"cpu_to_gpu_dispatch_us\": ";
    if (transfer.available) {
        json << std::fixed << std::setprecision(2) << transfer.cpu_to_gpu_dispatch_us;
    } else {
        json << "null";
    }
    json << "\n";
    json << "  },\n";
    json << "  \"themis_capability_tier\": {\n";
    json << "    \"simd\": \"" << tier.simd << "\",\n";
    json << "    \"memory\": \"" << tier.memory << "\",\n";
    json << "    \"storage\": \"" << tier.storage << "\",\n";
    json << "    \"gpu\": \"" << tier.gpu << "\"\n";
    json << "  },\n";
    json << "  \"benchmark_methodology\": {\n";
    json << "    \"memory\": \"STREAM-like copy/scale/add/triad\",\n";
    json << "    \"storage\": \"sequential throughput + 4K random IOPS\",\n";
    json << "    \"cpu\": \"CPUID feature detection + integer/float micro throughput\"\n";
    json << "  }\n";
    json << "}\n";

    {
        std::ofstream out(outFile, std::ios::binary | std::ios::trunc);
        ASSERT_TRUE(out.good()) << "Failed to open hardware baseline output file: " << outFile.string();
        out << json.str();
    }

    fs::remove(diskFile);

    ASSERT_TRUE(fs::exists(outFile)) << "Hardware baseline JSON not written";
    ASSERT_GT(cpuInt.ops_per_s, 0.0);
    ASSERT_GT(cpuFloat.ops_per_s, 0.0);
    ASSERT_GT(memMBs, 0.0);
    ASSERT_GT(stream.triad_gb_s, 0.0);
    ASSERT_GT(disk.write_mb_s, 0.0);
    ASSERT_GT(disk.read_mb_s, 0.0);
    ASSERT_GT(disk.random_read_iops, 0.0);
    ASSERT_GT(disk.random_write_iops, 0.0);
    if (transfer.available) {
        ASSERT_GT(transfer.host_to_vram_gb_s, 0.0);
        ASSERT_GT(transfer.vram_to_host_gb_s, 0.0);
        ASSERT_GT(transfer.cpu_to_gpu_dispatch_us, 0.0);
    }
}
