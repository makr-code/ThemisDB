/**
 * @file hardware_telemetry.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.5
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=5, H=5, M=0, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

/*
 * ThemisDB | File: hardware_telemetry.cpp | Version: 0.0.5 | Last Modified: 2026-05-31 12:17:24
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 581
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=5, H=7, M=0, L=0
 * PR History (last 5): #4647 feat(updates): anonymous ha... (2026-04-15)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#include "updates/hardware_telemetry.h"
#include "updates/build_verifier.h"
#include "utils/logger.h"

#define LOG_ERROR(...) SPDLOG_ERROR(__VA_ARGS__)
#define LOG_INFO(...)  SPDLOG_INFO(__VA_ARGS__)
#define LOG_WARN(...)  SPDLOG_WARN(__VA_ARGS__)
#define LOG_DEBUG(...) SPDLOG_DEBUG(__VA_ARGS__)

#ifdef THEMIS_ENABLE_CURL
#  include <curl/curl.h>
#endif

#include <nlohmann/json.hpp>

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <random>
#include <sstream>
#include <thread>

#if defined(_WIN32)
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  include <windows.h>
#  include <sysinfoapi.h>
#elif defined(__APPLE__)
#  include <sys/sysctl.h>
#  include <sys/types.h>
#  include <mach/mach.h>
#  include <sys/utsname.h>
#else
   // Linux / POSIX
#  include <sys/utsname.h>
#  include <sys/sysinfo.h>
#  include <unistd.h>
#endif

namespace themis {
namespace updates {

using json = nlohmann::json;

// ---------------------------------------------------------------------------
// Bucketing helpers
// ---------------------------------------------------------------------------

namespace {

/// Round `v` down to the nearest power of 2.  Returns 0 when v == 0.
static uint32_t floorPow2(uint32_t v) noexcept {
    if (v == 0) { return 0; }
    uint32_t p = 1;
    while (p * 2 <= v) { p *= 2; }
    return p;
}

/// Round `v` down to a multiple of `bucket`.  Returns 0 when v == 0.
template<typename T>
static T floorBucket(T v, T bucket) noexcept {
    if (v == 0 || bucket == 0) { return 0; }
    return (v / bucket) * bucket;
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// HardwareSnapshot::toJson
// ---------------------------------------------------------------------------

std::string HardwareSnapshot::toJson() const {
    json j;
    j["instance_id"]    = instance_id;
    j["themis_version"] = themis_version;
    j["timestamp_utc"]  = timestamp_utc;
    if (!cpu_model.empty())  { j["cpu_model"]    = cpu_model; }
    if (cpu_cores > 0)       { j["cpu_cores"]    = cpu_cores; }
    if (total_ram_mb > 0)    { j["total_ram_mb"] = total_ram_mb; }
    if (!os_family.empty())  { j["os_family"]    = os_family; }
    if (!cpu_arch.empty())   { j["cpu_arch"]     = cpu_arch; }

    // Build provenance
    j["build_channel"]  = build_channel;
    j["build_id"]       = build_id;
    j["build_verified"] = build_verified;

    if (performance.has_value()) {
        const auto& p = *performance;
        json pj;
        if (p.avg_query_latency_us > 0)
            { pj["avg_query_latency_us"]      = p.avg_query_latency_us; }
        if (p.p99_query_latency_us > 0)
            { pj["p99_query_latency_us"]      = p.p99_query_latency_us; }
        if (p.queries_per_second_bucket > 0)
            { pj["queries_per_second_bucket"] = p.queries_per_second_bucket; }
        if (p.cache_hit_rate_pct != 255)
            { pj["cache_hit_rate_pct"]        = p.cache_hit_rate_pct; }
        if (p.process_rss_mb_bucket > 0)
            { pj["process_rss_mb_bucket"]     = p.process_rss_mb_bucket; }
        if (p.uptime_seconds > 0)
            { pj["uptime_seconds"]            = p.uptime_seconds; }
        if (p.active_connections_bucket > 0)
            { pj["active_connections_bucket"] = p.active_connections_bucket; }
        if (p.db_size_mb_bucket > 0)
            { pj["db_size_mb_bucket"]         = p.db_size_mb_bucket; }
        if (!pj.empty()) { j["performance"] = std::move(pj); }
    }

    return j.dump();
}

// ---------------------------------------------------------------------------
// SystemHardwareInfoProvider
// ---------------------------------------------------------------------------

namespace {

#if defined(__linux__)
/// Read the first occurrence of `key: value` from /proc/cpuinfo.
static std::string readProcCpuinfoField(const std::string& key) {
    std::ifstream f("/proc/cpuinfo");
    if (!f.is_open()) { return {}; }
    std::string line;
    const std::string prefix = key + "\t:";
    const std::string prefix2 = key + " :";
    while (std::getline(f, line)) {
        if (line.rfind(key, 0) == 0) {
            auto colon = line.find(':');
            if (colon != std::string::npos) {
                std::string val = line.substr(colon + 1);
                // Trim leading whitespace.
                auto first = val.find_first_not_of(" \t");
                if (first != std::string::npos) { val = val.substr(first); }
                return val;
            }
        }
    }
    (void)prefix; (void)prefix2;
    return {};
}

/// Count the number of logical processors from /proc/cpuinfo.
static unsigned int countLinuxCpuCores() {
    std::ifstream f("/proc/cpuinfo");
    if (!f.is_open()) { return 0; }
    unsigned int count = 0;
    std::string line;
    while (std::getline(f, line)) {
        if (line.rfind("processor", 0) == 0) { ++count; }
    }
    return count;
}

/// Read total RAM from /proc/meminfo (kB → MB).
static uint64_t linuxTotalRamMb() {
    std::ifstream f("/proc/meminfo");
    if (!f.is_open()) { return 0; }
    std::string line;
    while (std::getline(f, line)) {
        if (line.rfind("MemTotal:", 0) == 0) {
            std::istringstream iss(line);
            std::string label;
            uint64_t kb = 0;
            iss >> label >> kb;
            return kb / 1024;
        }
    }
    return 0;
}
#endif // __linux__

/// Round total_ram_mb down to the nearest 1 024 MiB bucket (privacy measure).
static uint64_t bucketRamMb(uint64_t raw_mb) {
    if (raw_mb == 0) { return 0; }
    return (raw_mb / 1024) * 1024;
}

} // anonymous namespace

std::string SystemHardwareInfoProvider::cpuModel() const {
#if defined(__linux__)
    return readProcCpuinfoField("model name");
#elif defined(_WIN32)
    HKEY hKey = nullptr;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE,
                      "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
                      0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        char buf[256] = {};
        DWORD size = sizeof(buf);
        DWORD type = REG_SZ;
        RegQueryValueExA(hKey, "ProcessorNameString", nullptr, &type,
                         reinterpret_cast<LPBYTE>(buf), &size);
        RegCloseKey(hKey);
        return std::string(buf);
    }
    return {};
#elif defined(__APPLE__)
    char buf[256] = {};
    std::size_t len = sizeof(buf);
    if (sysctlbyname("machdep.cpu.brand_string", buf, &len, nullptr, 0) == 0) {
        return std::string(buf);
    }
    return {};
#else
    return {};
#endif
}

unsigned int SystemHardwareInfoProvider::cpuCores() const {
#if defined(__linux__)
    unsigned int cores = countLinuxCpuCores();
    if (cores == 0) {
        auto n = static_cast<unsigned int>(sysconf(_SC_NPROCESSORS_ONLN));
        return (n > 0) ? n : 0;
    }
    return cores;
#elif defined(_WIN32)
    SYSTEM_INFO si{};
    GetSystemInfo(&si);
    return static_cast<unsigned int>(si.dwNumberOfProcessors);
#elif defined(__APPLE__)
    int32_t n = 0;
    std::size_t len = sizeof(n);
    if (sysctlbyname("hw.logicalcpu", &n, &len, nullptr, 0) == 0 && n > 0) {
        return static_cast<unsigned int>(n);
    }
    return 0;
#else
    auto n = static_cast<unsigned int>(std::thread::hardware_concurrency());
    return n;
#endif
}

uint64_t SystemHardwareInfoProvider::totalRamMb() const {
#if defined(__linux__)
    return bucketRamMb(linuxTotalRamMb());
#elif defined(_WIN32)
    MEMORYSTATUSEX ms{};
    ms.dwLength = sizeof(ms);
    if (GlobalMemoryStatusEx(&ms)) {
        uint64_t mb = ms.ullTotalPhys / (1024ULL * 1024ULL);
        return bucketRamMb(mb);
    }
    return 0;
#elif defined(__APPLE__)
    int64_t mem = 0;
    std::size_t len = sizeof(mem);
    if (sysctlbyname("hw.memsize", &mem, &len, nullptr, 0) == 0 && mem > 0) {
        uint64_t mb = static_cast<uint64_t>(mem) / (1024ULL * 1024ULL);
        return bucketRamMb(mb);
    }
    return 0;
#else
    return 0;
#endif
}

std::string SystemHardwareInfoProvider::osFamily() const {
#if defined(_WIN32)
    return "Windows";
#elif defined(__APPLE__)
    return "macOS";
#elif defined(__linux__)
    return "Linux";
#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
    return "BSD";
#else
    return "Unknown";
#endif
}

std::string SystemHardwareInfoProvider::cpuArch() const {
#if defined(__linux__) || defined(__APPLE__) || defined(__FreeBSD__)
    struct utsname u{};
    if (::uname(&u) == 0) {
        return std::string(u.machine);
    }
#elif defined(_WIN32)
    SYSTEM_INFO si{};
    GetNativeSystemInfo(&si);
    switch (si.wProcessorArchitecture) {
        case PROCESSOR_ARCHITECTURE_AMD64:  return "x86_64";
        case PROCESSOR_ARCHITECTURE_ARM:    return "arm";
        case PROCESSOR_ARCHITECTURE_ARM64:  return "aarch64";
        case PROCESSOR_ARCHITECTURE_INTEL:  return "x86";
        default:                            break;
    }
#endif
#if defined(__x86_64__) || defined(_M_X64)
    return "x86_64";
#elif defined(__aarch64__) || defined(_M_ARM64)
    return "aarch64";
#elif defined(__arm__) || defined(_M_ARM)
    return "arm";
#elif defined(__i386__) || defined(_M_IX86)
    return "x86";
#elif defined(__riscv)
    return "riscv";
#else
    return "unknown";
#endif
}

// ---------------------------------------------------------------------------
// Default libcurl HTTP sender
// ---------------------------------------------------------------------------

namespace {

#ifdef THEMIS_ENABLE_CURL
static std::size_t curlNullSink(char* /*buf*/, std::size_t /*size*/,
                                 std::size_t nmemb, void* /*userp*/) {
    return nmemb;
}

static bool curlPost(const std::string& url, const std::string& body,
                     const std::string& content_type, int timeout_seconds) {
    CURL* curl = curl_easy_init();
    if (!curl) { return false; }

    struct curl_slist* headers = nullptr;
    const std::string ct_header = "Content-Type: " + content_type;
    headers = curl_slist_append(headers, ct_header.c_str());
    headers = curl_slist_append(headers, "Accept: application/json");

    curl_easy_setopt(curl, CURLOPT_URL,            url.c_str());
    curl_easy_setopt(curl, CURLOPT_POST,            1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS,      body.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE,   static_cast<long>(body.size()));
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER,      headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,   curlNullSink);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT,         static_cast<long>(timeout_seconds));
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT,  5L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION,  1L);
    curl_easy_setopt(curl, CURLOPT_FORBID_REUSE,    0L);
    curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE,   1L);

    CURLcode res = curl_easy_perform(curl);

    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        LOG_WARN("Telemetry HTTP POST failed: {}", curl_easy_strerror(res));
        return false;
    }
    return (http_code >= 200 && http_code < 300);
}
#endif // THEMIS_ENABLE_CURL

static bool defaultHttpSend(const std::string& url, const std::string& body,
                             const std::string& content_type, int timeout_seconds) {
#ifdef THEMIS_ENABLE_CURL
    return curlPost(url, body, content_type, timeout_seconds);
#else
    (void)url; (void)body; (void)content_type; (void)timeout_seconds;
    LOG_WARN("Telemetry: CURL not available – telemetry send skipped");
    return false;
#endif
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// HardwareTelemetryReporter
// ---------------------------------------------------------------------------

std::string HardwareTelemetryReporter::generateUuid() {
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint64_t> dist;

    uint64_t hi = dist(gen);
    uint64_t lo = dist(gen);

    // Set version 4 and variant bits.
    hi = (hi & 0xFFFFFFFFFFFF0FFFULL) | 0x0000000000004000ULL;
    lo = (lo & 0x3FFFFFFFFFFFFFFFULL) | 0x8000000000000000ULL;

    char buf[37];
    std::snprintf(buf, sizeof(buf),
                  "%08x-%04x-%04x-%04x-%012llx",
                  static_cast<uint32_t>(hi >> 32),
                  static_cast<uint16_t>((hi >> 16) & 0xFFFF),
                  static_cast<uint16_t>(hi & 0xFFFF),
                  static_cast<uint16_t>(lo >> 48),
                  static_cast<unsigned long long>(lo & 0x0000FFFFFFFFFFFFULL));
    return std::string(buf);
}

HardwareTelemetryReporter::HardwareTelemetryReporter(
        TelemetryConfig config,
        std::shared_ptr<IHardwareInfoProvider> hw_provider,
        TelemetryHttpSendFunc http_sender)
    : config_(std::move(config))
    , hw_provider_(hw_provider ? std::move(hw_provider)
                               : std::make_shared<SystemHardwareInfoProvider>())
    , http_sender_(http_sender ? std::move(http_sender) : defaultHttpSend)
    , instance_id_(generateUuid()) {
    // Enforce minimum send interval of 24 h regardless of what was configured.
    constexpr int kMinIntervalSeconds = 86400;
    if (config_.send_interval_seconds < kMinIntervalSeconds) {
        LOG_WARN("Telemetry: send_interval_seconds {} is below the 24 h minimum; "
                 "clamped to {}",
                 config_.send_interval_seconds, kMinIntervalSeconds);
        config_.send_interval_seconds = kMinIntervalSeconds;
    }
}

HardwareTelemetryReporter::~HardwareTelemetryReporter() {
    stopBackgroundReporting();
}

void HardwareTelemetryReporter::setPerformanceProvider(
        std::shared_ptr<IPerformanceMetricsProvider> provider) {
    std::lock_guard<std::mutex> lock(perf_provider_mutex_);
    perf_provider_ = std::move(provider);
}

HardwareSnapshot HardwareTelemetryReporter::collect() const {
    HardwareSnapshot snap;
    snap.instance_id    = instance_id_;
    snap.themis_version = THEMIS_VERSION_STRING;
    snap.timestamp_utc  =
        static_cast<int64_t>(
            std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now().time_since_epoch()).count());

    if (config_.include_cpu_model) { snap.cpu_model    = hw_provider_->cpuModel(); }
    if (config_.include_cpu_cores) { snap.cpu_cores    = hw_provider_->cpuCores(); }
    if (config_.include_ram_mb)    { snap.total_ram_mb = hw_provider_->totalRamMb(); }
    if (config_.include_os)        { snap.os_family    = hw_provider_->osFamily(); }
    if (config_.include_arch)      { snap.cpu_arch     = hw_provider_->cpuArch(); }

    // Build provenance – always included, no opt-out (coarse data only).
    {
        const auto bv     = verifyBuildSignature();
        snap.build_channel  = bv.channel;
        snap.build_id       = bv.build_id;
        snap.build_verified = bv.verified;
    }

    if (config_.include_performance) {
        std::shared_ptr<IPerformanceMetricsProvider> provider;
        {
            std::lock_guard<std::mutex> lock(perf_provider_mutex_);
            provider = perf_provider_;
        }
        
        if (provider) {
            PerformanceSnapshot raw = provider->collect();

            // Apply bucketing to protect against workload fingerprinting.
            PerformanceSnapshot bucketed;
            bucketed.avg_query_latency_us      = raw.avg_query_latency_us;
            bucketed.p99_query_latency_us      = raw.p99_query_latency_us;
            bucketed.queries_per_second_bucket =
                floorPow2(raw.queries_per_second_bucket);
            bucketed.cache_hit_rate_pct        = raw.cache_hit_rate_pct;
            bucketed.process_rss_mb_bucket     =
                static_cast<uint32_t>(floorBucket<uint32_t>(
                    raw.process_rss_mb_bucket, 64u));
            bucketed.uptime_seconds            = raw.uptime_seconds;
            bucketed.active_connections_bucket =
                floorPow2(raw.active_connections_bucket);
            bucketed.db_size_mb_bucket         =
                static_cast<uint32_t>(floorBucket<uint32_t>(
                    raw.db_size_mb_bucket, 512u));

            snap.performance = bucketed;
        }
    }

    return snap;
}

bool HardwareTelemetryReporter::send(const HardwareSnapshot& snapshot) const {
    if (config_.endpoint_url.empty()) {
        LOG_WARN("Telemetry: endpoint_url is empty – skipping send");
        return false;
    }

    const std::string body = snapshot.toJson();
    LOG_DEBUG("Telemetry: sending snapshot to {}", config_.endpoint_url);

    for (int attempt = 0; attempt <= config_.max_retries; ++attempt) {
        if (attempt > 0) {
            LOG_DEBUG("Telemetry: retry attempt {}/{}", attempt, config_.max_retries);
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
        if (http_sender_(config_.endpoint_url, body,
                         "application/json", config_.http_timeout_seconds)) {
            LOG_INFO("Telemetry: snapshot sent successfully (instance_id={})",
                     snapshot.instance_id);
            return true;
        }
    }

    LOG_WARN("Telemetry: all {} send attempt(s) failed", config_.max_retries + 1);
    return false;
}

bool HardwareTelemetryReporter::report() {
    if (!config_.enabled) { return false; }
    return send(collect());
}

void HardwareTelemetryReporter::startBackgroundReporting() {
    if (!config_.enabled) {
        LOG_DEBUG("Telemetry: disabled – background reporting not started");
        return;
    }
    if (running_.load(std::memory_order_acquire)) {
        LOG_DEBUG("Telemetry: background reporting already running");
        return;
    }

    stop_requested_.store(false, std::memory_order_release);
    running_.store(true, std::memory_order_release);
    bg_thread_ = std::thread(&HardwareTelemetryReporter::runLoop, this);
    LOG_INFO("Telemetry: background reporting started (interval={}s, endpoint={})",
             config_.send_interval_seconds, config_.endpoint_url);
}

void HardwareTelemetryReporter::stopBackgroundReporting() {
    if (!running_.load(std::memory_order_acquire)) { return; }
    stop_requested_.store(true, std::memory_order_release);
    
    // The background thread exits promptly once stop_requested_ is set.
    if (bg_thread_.joinable()) {
        bg_thread_.join();
    }
    running_.store(false, std::memory_order_release);
    LOG_INFO("Telemetry: background reporting stopped");
}

bool HardwareTelemetryReporter::isRunning() const noexcept {
    return running_.load(std::memory_order_acquire);
}

TelemetryConfig HardwareTelemetryReporter::config() const noexcept {
    return config_;
}

const std::string& HardwareTelemetryReporter::instanceId() const noexcept {
    return instance_id_;
}

void HardwareTelemetryReporter::runLoop() {
    // Send an initial report immediately on start.
    if (!stop_requested_.load(std::memory_order_acquire)) {
        report();
    }

    const auto interval = std::chrono::seconds(config_.send_interval_seconds);
    auto next_tick = std::chrono::steady_clock::now() + interval;

    while (!stop_requested_.load(std::memory_order_acquire)) {
        // Sleep in small chunks to remain responsive to stop requests.
        const auto now = std::chrono::steady_clock::now();
        if (now >= next_tick) {
            report();
            next_tick = std::chrono::steady_clock::now() + interval;
        } else {
            // Sleep at most 500 ms at a time.
            auto remaining = next_tick - now;
            const auto max_chunk = std::chrono::duration_cast<std::chrono::steady_clock::duration>(
                std::chrono::milliseconds(500));
            auto sleep_time = std::min(remaining, max_chunk);
            std::this_thread::sleep_for(sleep_time);
        }
    }
}

} // namespace updates
} // namespace themis
