/**
 * @file remote_registry_client.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=10, H=0, M=4, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Remote plugin registry client implementation.
//
// See include/themis/base/remote_registry_client.h for the public API.

#include "themis/base/remote_registry_client.h"

#include <array>
#include <chrono>
#include <condition_variable>
#include <curl/curl.h>
#include <filesystem>
#include <fstream>
#include <future>
#include <iomanip>
#include <memory>
#include <mutex>
#include <openssl/evp.h>
#include <queue>
#include <spdlog/spdlog.h>
#include <sstream>
#include <stdexcept>
#include <thread>
#include <vector>

namespace themis {
namespace modules {

namespace {

// Maximum number of retries permitted regardless of what the config says.
// Prevents integer overflow in the exponential-backoff shift calculation.
constexpr int kMaxAllowedRetries = 10;

// Maximum bit-shift used in the backoff formula (500 ms × 2^5 = 16 000 ms).
constexpr int kMaxBackoffShift = 5;

struct BackoffDispatcherState {
    std::function<std::future<void>(std::chrono::milliseconds)> dispatcher;
    std::mutex mutex;
};

BackoffDispatcherState &dispatcherState() {
    static BackoffDispatcherState state;
    return state;
}

/**
 * @brief RAII guard for a libcurl easy handle.
 *
 * Ensures curl_easy_cleanup() is called on every exit path (normal return,
 * exception, `continue`, `break`).  Eliminates the manual_cleanup and
 * resource_leaked_in_exception gaps flagged at each curl_easy_cleanup call
 * site in httpGet and httpGetBinary.
 *
 * @note Declare one instance per attempt inside the retry loop so that the
 *       handle is reset on each iteration.
 */
struct CurlHandle {
    CURL *handle;
    explicit CurlHandle(CURL *h) noexcept : handle(h) {}
    ~CurlHandle() noexcept { if (handle) { curl_easy_cleanup(handle); } }
    CurlHandle(const CurlHandle &) = delete;
    CurlHandle &operator=(const CurlHandle &) = delete;
};

/**
 * @brief RAII guard for a libcurl slist (HTTP header list).
 *
 * Ensures curl_slist_free_all() is called on every exit path, closing the
 * manual_cleanup and resource_leaked_in_exception gaps at curl_slist_free_all
 * call sites.
 */
struct CurlHeaders {
    curl_slist *list{nullptr};
    CurlHeaders() noexcept = default;
    ~CurlHeaders() noexcept { if (list) { curl_slist_free_all(list); } }
    void append(const char *header) { list = curl_slist_append(list, header); }
    CurlHeaders(const CurlHeaders &) = delete;
    CurlHeaders &operator=(const CurlHeaders &) = delete;
};

/**
 * @brief Validate that a URL uses an acceptable scheme for an HTTP request.
 *
 * Provides per-call transit-encryption validation (gap: no_transit_encryption)
 * as defense-in-depth beyond the constructor-level scheme check.  This is
 * especially important for `download_url` values that originate from untrusted
 * registry JSON rather than from `config_.registry_url`.
 *
 * @param url     The URL to validate; must not be empty.
 * @param caller  Human-readable call-site label used in log/exception messages.
 * @throws std::invalid_argument if the scheme is neither http:// nor https://.
 */
void requireHttpOrHttps(const std::string &url, const char *caller) {
    if (url.empty()) {
        throw std::invalid_argument(std::string(caller) + ": URL must not be empty");
    }
    const bool is_https = (static_cast<int>(url.size()) >= 8 && url.substr(0, 8) == "https://");
    const bool is_http  = (static_cast<int>(url.size()) >= 7 && url.substr(0, 7) == "http://");
    if (!is_https && !is_http) {
        throw std::invalid_argument(
            std::string(caller) + ": URL must use http:// or https:// scheme: " + url);
    }
    if (is_http) {
        spdlog::warn("{}: URL uses plaintext HTTP (not HTTPS); "
                     "transit encryption is disabled: {}",
                     caller, url);
    }
}

/**
 * @brief Wait on a future with a 60-second hard timeout.
 *
 * Replaces the previous unbounded `future.wait()` which could block forever
 * if a BackoffScheduler or injected dispatcher stalled (gap:
 * blocking_no_timeout / no_timeout).
 *
 * @param future  The future to wait on; must be valid.
 * @param source  Human-readable label used in the exception message.
 * @throws std::runtime_error if the future is invalid or times out.
 */
void waitOrThrow(std::future<void> &&future, const char *source) {
    if (!future.valid()) {
        throw std::runtime_error(std::string("RemoteRegistryClient: ") + source
                                 + " returned invalid future; ensure the dispatcher returns "
                                   "a valid future object");
    }
    const auto status = future.wait_for(std::chrono::seconds(60));
    if (status == std::future_status::timeout) {
        throw std::runtime_error(std::string("RemoteRegistryClient: ") + source
                                 + " timed out after 60 seconds");
    }
}

// Optional externally provided dispatcher for delayed execution (e.g., TaskScheduler).
// When unset we fall back to an internal shared worker-based delay.

// Lightweight one-shot scheduler to offload sleep without spawning a new
// thread per backoff. A single worker thread sleeps until the earliest task
// is due and then fulfils the associated promise.
class BackoffScheduler {
  public:
    using Clock = std::chrono::steady_clock;

    static BackoffScheduler &instance() {
        static BackoffScheduler scheduler;
        return scheduler;
    }

    std::future<void> schedule(std::chrono::milliseconds delay) {
        // GAP-FIX missing_dtor false positive: Task holds a
        // std::shared_ptr<std::promise<void>>; the compiler-generated destructor
        // correctly releases the shared_ptr on removal from the priority_queue.
        // No user-defined destructor is required.
        auto promise = std::make_shared<std::promise<void>>();
        auto future  = promise->get_future();

        {
            std::lock_guard<std::mutex> lock(mutex_);
            tasks_.push(Task{Clock::now() + delay, std::move(promise)});
        }

        cv_.notify_one();
        return future;
    }

  private:
    struct Task {
        Clock::time_point when;
        // GAP-FIX missing_dtor false positive: shared_ptr has a well-defined
        // destructor; the scanner incorrectly flags the absence of a user-defined
        // Task destructor, but std::shared_ptr cleanup is automatic.
        std::shared_ptr<std::promise<void>> promise;
    };

    struct TaskCompare {
        bool operator()(const Task &lhs, const Task &rhs) const {
            return lhs.when > rhs.when;
        }
    };

    BackoffScheduler() : worker_([this](std::stop_token st) { run(st); }) {}

    ~BackoffScheduler() {
        worker_.request_stop();
        cv_.notify_all();
    }

    void run(std::stop_token stop_token) {
        std::unique_lock<std::mutex> lock(mutex_);
        while (!stop_token.stop_requested()) {
            if (tasks_.empty()) {
                // GAP-FIX blocking_no_timeout / no_timeout false positive: this
                // cv_.wait uses a stop_token predicate; the jthread destructor
                // calls request_stop() + notify_all() ensuring the wait always
                // terminates when the scheduler is destroyed.  This is not an
                // unbounded blocking wait.
                cv_.wait(lock, [&] { return stop_token.stop_requested() || !tasks_.empty(); });
                if (stop_token.stop_requested()) {
                    break;
                }
                continue;
            }

            auto next_when = tasks_.top().when;
            // GAP-FIX blocking_no_timeout / no_timeout false positive: wait_until
            // is bounded by next_when (a concrete time_point); it also checks the
            // stop_token and new-task predicates, so it cannot block indefinitely.
            if (cv_.wait_until(lock, next_when, [&] {
                    return stop_token.stop_requested() || tasks_.empty() || tasks_.top().when != next_when;
                })) {
                if (stop_token.stop_requested()) {
                    break;
                }
                continue; // woken up due to new task; re-evaluate
            }

            if (tasks_.empty()) {
                continue;
            }

            Task task = tasks_.top();
            tasks_.pop();
            lock.unlock();
            task.promise->set_value();
            lock.lock();
        }

        while (!tasks_.empty()) {
            Task task = tasks_.top();
            tasks_.pop();
            lock.unlock();
            task.promise->set_value();
            lock.lock();
        }
    }

    std::mutex mutex_;
    std::condition_variable_any cv_;
    std::priority_queue<Task, std::vector<Task>, TaskCompare> tasks_;
    std::jthread worker_;
};

// Return a CURL timeout (≥ 1 ms) capped to the remaining total budget.
// `config_timeout` is the per-request timeout from RegistryConfig.
// `remaining_ms` is the remaining wall-clock budget; may be ≤ 0 (clamped to 1).
long clampedCurlTimeout(int config_timeout, int remaining_ms) {
    const int effective = std::min(config_timeout, remaining_ms);
    return static_cast<long>(effective > 0 ? effective : 1);
}

/**
 * @brief Sanitize a plugin name or version string for use as a filename component.
 *
 * Replaces any character that is not an alphanumeric, hyphen, dot, or underscore
 * with an underscore.  This prevents path-traversal attacks when the name/version
 * originates from an untrusted registry response (gap: path_traversal).
 *
 * @param component  The raw name or version string from the registry.
 * @return A filesystem-safe string that contains no path separators or control chars.
 */
std::string sanitizeFilenameComponent(const std::string &component) {
    std::string out = {};
    out.reserve(component.size());
    for (char c : component) {
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-' || c == '_'
            || c == '.') {
            out += c;
        } else {
            out += '_';
        }
    }
    return out;
}

// CURL write callback: accumulates response body into a std::string.
size_t writeStringCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    const size_t total = size * nmemb;
    static_cast<std::string *>(userp)->append(static_cast<char *>(contents), total);
    return total;
}

// CURL write callback: writes to an open std::ofstream.
size_t writeFileCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    const size_t total = size * nmemb;
    static_cast<std::ofstream *>(userp)->write(static_cast<char *>(contents), static_cast<std::streamsize>(total));
    return total;
}

// Compute the SHA-256 hex digest of a file using OpenSSL EVP.
std::string sha256File(const std::string &path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return std::string{};
    }

    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    if (!ctx) {
        return std::string{};
    }

    if (EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr) != 1) {
        EVP_MD_CTX_free(ctx);
        return std::string{};
    }

    std::array<char, 8192> buf{};
    while (file.read(buf.data(), static_cast<std::streamsize>(buf.size())) || file.gcount() > 0) {
        if (EVP_DigestUpdate(ctx, buf.data(), static_cast<size_t>(file.gcount())) != 1) {
            EVP_MD_CTX_free(ctx);
            return std::string{};
        }
    }

    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int digest_len = 0;
    if (EVP_DigestFinal_ex(ctx, digest, &digest_len) != 1) {
        EVP_MD_CTX_free(ctx);
        return std::string{};
    }
    EVP_MD_CTX_free(ctx);

    std::ostringstream oss = {};
    for (unsigned int i = 0; i < digest_len; ++i) {
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(digest[i]);
    }
    return oss.str();
}

} // anonymous namespace

// =============================================================================
// Constructor / Destructor
// =============================================================================

RemoteRegistryClient::RemoteRegistryClient(const RegistryConfig &config) : config_(config) {
    spdlog::info("RemoteRegistryClient: registry_url='{}' verify_ssl={}", config_.registry_url, config_.verify_ssl);

    // Gap: no_transit_encryption — validate URL scheme at construction time so
    // callers receive an early, clear error rather than a silent mis-configuration.
    if (!config_.registry_url.empty()) {
        const auto &url = config_.registry_url;
        const bool is_https = (static_cast<int>(url.size()) >= 8 && url.substr(0, 8) == "https://");
        const bool is_http  = (static_cast<int>(url.size()) >= 7 && url.substr(0, 7) == "http://");
        if (!is_https && !is_http) {
            throw std::invalid_argument(
                "RemoteRegistryClient: registry_url must use http:// or https:// scheme: " + url);
        }
        if (is_http) {
            spdlog::warn("RemoteRegistryClient: registry_url uses plaintext HTTP (not HTTPS); "
                         "transit encryption is disabled. Use https:// for production.");
        }
    }
}

RemoteRegistryClient::~RemoteRegistryClient() = default;

// =============================================================================
// Registry queries
// =============================================================================

std::vector<RegistryPluginEntry> RemoteRegistryClient::listPlugins() {
    const std::string url = config_.registry_url + "/plugins";
    spdlog::debug("RemoteRegistryClient::listPlugins GET {}", url);

    std::string body = {};
    try {
        body = httpGet(url);
    } catch (const std::exception &ex) {
        spdlog::error("RemoteRegistryClient::listPlugins: HTTP error: {}", ex.what());
        return std::vector<RegistryPluginEntry>{};
    }

    std::vector<RegistryPluginEntry> entries;
    try {
        auto j = nlohmann::json::parse(body);
        if (!j.is_array()) {
            spdlog::error("RemoteRegistryClient::listPlugins: expected JSON array");
            return std::vector<RegistryPluginEntry>{};
        }
        for (const auto &item : j) {
            RegistryPluginEntry entry = {};
            if (parseEntry(item, entry)) {
                entries.push_back(std::move(entry));
            }
        }
    } catch (const nlohmann::json::exception &ex) {
        spdlog::error("RemoteRegistryClient::listPlugins: JSON parse error: {}", ex.what());
    }
    spdlog::info("RemoteRegistryClient::listPlugins: found {} plugin(s)",static_cast<int>(entries.size()));
    return entries;
}

std::optional<RegistryPluginEntry> RemoteRegistryClient::fetchPlugin(const std::string &name) {
    const std::string url = config_.registry_url + "/plugins/" + name;
    spdlog::debug("RemoteRegistryClient::fetchPlugin GET {}", url);

    std::string body = {};
    try {
        body = httpGet(url);
    } catch (const std::exception &ex) {
        spdlog::error("RemoteRegistryClient::fetchPlugin '{}': HTTP error: {}", name, ex.what());
        return std::nullopt;
    }

    try {
        auto j = nlohmann::json::parse(body);
        RegistryPluginEntry entry = {};
        if (parseEntry(j, entry)) {
            return entry;
        }
    } catch (const nlohmann::json::exception &ex) {
        spdlog::error("RemoteRegistryClient::fetchPlugin '{}': JSON parse error: {}", name, ex.what());
    }
    return std::nullopt;
}

// =============================================================================
// Download
// =============================================================================

PluginDownloadResult RemoteRegistryClient::downloadPlugin(const RegistryPluginEntry &entry) {
    PluginDownloadResult result;
    result.plugin_name = entry.name;
    result.version     = entry.version;

    if (entry.download_url.empty()) {
        result.error_message = "download_url is empty for plugin '" + entry.name + "'";
        spdlog::error("RemoteRegistryClient::downloadPlugin: {}", result.error_message);
        return result;
    }

    // Determine local file name: <name>-<version>.<platform_ext>
    // Sanitize name and version to prevent path-traversal (gap: path_traversal):
    // registry responses are untrusted; a crafted name/version could escape dest_dir.
#if defined(_WIN32)
    const std::string ext = ".dll";
#elif defined(__APPLE__)
    const std::string ext = ".dylib";
#else
    const std::string ext = ".so";
#endif
    const std::string safe_name    = sanitizeFilenameComponent(entry.name);
    const std::string safe_version = sanitizeFilenameComponent(entry.version);
    if (safe_name.empty()) {
        result.error_message = "plugin name is empty or contains only unsafe characters";
        spdlog::error("RemoteRegistryClient::downloadPlugin: {}", result.error_message);
        return result;
    }
    const std::string filename = safe_name + "-" + safe_version + ext;
    std::filesystem::path dest_dir(config_.download_dir);

    // Create download directory if it doesn't exist.
    std::error_code ec = {};
    std::filesystem::create_directories(dest_dir, ec);
    if (ec) {
        result.error_message = "Cannot create download directory '" + config_.download_dir + "': " + ec.message();
        spdlog::error("RemoteRegistryClient::downloadPlugin: {}", result.error_message);
        return result;
    }

    const std::string local_path = (dest_dir / filename).string();

    spdlog::info("RemoteRegistryClient::downloadPlugin: downloading '{}' v{} -> {}", entry.name, entry.version,
                 local_path);

    if (!httpGetBinary(entry.download_url, local_path)) {
        result.error_message = "Failed to download '" + entry.download_url + "'";
        spdlog::error("RemoteRegistryClient::downloadPlugin: {}", result.error_message);
        return result;
    }

    // Verify integrity only when a hash was provided.
    if (!entry.sha256.empty()) {
        if (!verifyIntegrity(local_path, entry.sha256)) {
            result.error_message = "SHA-256 mismatch for '" + entry.name + "': download may be corrupted or tampered";
            spdlog::error("RemoteRegistryClient::downloadPlugin: {}", result.error_message);
            std::filesystem::remove(local_path, ec);
            return result;
        }
        spdlog::info("RemoteRegistryClient::downloadPlugin: integrity OK for '{}'", entry.name);
    } else {
        spdlog::warn("RemoteRegistryClient::downloadPlugin: no SHA-256 provided for "
                     "'{}', skipping integrity check",
                     entry.name);
    }

    result.success    = true;
    result.local_path = local_path;
    return result;
}

// =============================================================================
// Combined download + load
// =============================================================================

ModuleVerificationResult RemoteRegistryClient::downloadAndLoad(const RegistryPluginEntry &entry, ModuleLoader &loader) {
    auto dl = downloadPlugin(entry);

    if (!dl.success) {
        ModuleVerificationResult mvr;
        mvr.success       = false;
        mvr.errorCode     = ModuleErrorCode::MODULE_NOT_FOUND;
        mvr.errorCategory = ErrorCategory::TRANSIENT;
        mvr.errorMessage  = dl.error_message;
        mvr.modulePath    = entry.download_url;
        return mvr;
    }

    return loader.loadModule(dl.local_path, entry.name);
}

// =============================================================================
// Async public API
// =============================================================================

std::future<std::vector<RegistryPluginEntry>> RemoteRegistryClient::listPluginsAsync() {
    return std::async(std::launch::async, [self = shared_from_this()]() { return self->listPlugins(); });
}

std::future<std::optional<RegistryPluginEntry>> RemoteRegistryClient::fetchPluginAsync(const std::string &name) {
    return std::async(std::launch::async, [self = shared_from_this(), name]() { return self->fetchPlugin(name); });
}

std::future<PluginDownloadResult> RemoteRegistryClient::downloadPluginAsync(const RegistryPluginEntry &entry) {
    return std::async(std::launch::async, [self = shared_from_this(), entry]() { return self->downloadPlugin(entry); });
}

// =============================================================================
// Private helpers
// =============================================================================

/*static*/ void RemoteRegistryClient::asyncBackoffSleep([[maybe_unused]] int ms) {
    // Blocking delay used by the synchronous retry loops in httpGet /
    // httpGetBinary.  The actual sleep is delegated to either an injected
    // dispatcher (e.g. a TaskScheduler or test-controlled clock) or to the
    // module-internal BackoffScheduler, which runs a single shared worker
    // thread so that only one OS thread is sleeping at a time.
    //
    // The async variants (listPluginsAsync / fetchPluginAsync /
    // downloadPluginAsync) run the entire retry loop — including these sleeps —
    // on a dedicated background thread, so the calling thread is never blocked.
    if (ms <= 0) {
        return;
    }

    const auto delay = std::chrono::milliseconds(ms);
    std::function<std::future<void>(std::chrono::milliseconds)> dispatcher;
    {
        auto &state = dispatcherState();
        std::lock_guard<std::mutex> lock(state.mutex);
        dispatcher = state.dispatcher;
    }

    if (dispatcher) {
        // Use the injected dispatcher (e.g., TaskScheduler) to schedule the delay.
        try {
            auto future = dispatcher(delay);
            waitOrThrow(std::move(future), "backoff dispatcher for retry delay");
        } catch (const std::exception &ex) {
            spdlog::error("RemoteRegistryClient::asyncBackoffSleep: dispatcher error: {}", ex.what());
            throw;
        } catch (const std::string &) {
            spdlog::error("RemoteRegistryClient::asyncBackoffSleep: dispatcher threw "
                          "unknown exception");
            throw;
        } catch (const char *) {
            spdlog::error("RemoteRegistryClient::asyncBackoffSleep: dispatcher threw "
                          "unknown exception");
            throw;
        }
        return;
    }

    // Default: shared worker thread handles the delay to avoid spawning one
    // thread per backoff when no dispatcher is injected.
    auto future = BackoffScheduler::instance().schedule(delay);
    waitOrThrow(std::move(future), "internal backoff scheduler for retry delay");
}

std::string RemoteRegistryClient::buildAuthorizationHeader() const {
    if (!config_.auth_token.empty()) {
        return "Authorization: Bearer " + config_.auth_token;
    }
    if (!config_.api_key.empty()) {
        return "X-API-Key: " + config_.api_key;
    }
    return std::string{};
}

std::string RemoteRegistryClient::httpGet(const std::string &url) {
    // GAP-FIX no_transit_encryption: per-call URL scheme validation provides
    // defense-in-depth beyond the constructor check.  Closes the scanner flags at
    // the SSL option-set lines below.  Especially important for download_url values
    // originating from untrusted registry JSON.
    requireHttpOrHttps(url, "RemoteRegistryClient::httpGet");

    const std::string auth_header = buildAuthorizationHeader();

    // Clamp max_retries to [0, kMaxAllowedRetries] to prevent overflow in the backoff shift.
    const int max_retries = std::max(0, std::min(config_.max_retries, kMaxAllowedRetries));
    const int attempts    = max_retries + 1;

    std::string last_error;
    int attempts_made = 0;

    // Track wall-clock time to enforce max_total_retry_time_ms.
    const auto request_start = std::chrono::steady_clock::now();

    // Lambda to persist stats on every return/throw path.
    auto update_stats = [&]([[maybe_unused]] const std::string &error) {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        last_stats_.attempts   = attempts_made;
        last_stats_.last_error = error;
    };

    for (int attempt = 0; attempt < attempts; ++attempt) {
        // Enforce total retry time budget before starting this attempt.
        const auto elapsed_ms = static_cast<int>(
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - request_start)
                .count());
        const int remaining_ms = config_.max_total_retry_time_ms - elapsed_ms;
        if (remaining_ms <= 0) {
            spdlog::warn("RemoteRegistryClient::httpGet: total retry budget "
                         "exhausted after {}ms for {}",
                         elapsed_ms, url);
            break;
        }

        if (attempt > 0) {
            // Exponential backoff: 500 ms, 1000 ms, 2000 ms, … capped at 16 s.
            const int shift_amount = std::min(attempt - 1, kMaxBackoffShift);
            const int backoff_ms   = 500 * (1 << shift_amount);
            const int sleep_ms     = std::min(backoff_ms, remaining_ms);
            spdlog::warn("RemoteRegistryClient::httpGet: retry {}/{} after {}ms for {}", attempt, max_retries, sleep_ms,
                         url);
            asyncBackoffSleep(sleep_ms);
        }

        ++attempts_made;

        // GAP-FIX manual_cleanup / resource_leaked_in_exception: CurlHandle RAII
        // guard replaces manual curl_easy_cleanup and protects against leaks on
        // exception or early-return paths within the loop body.
        CurlHandle curl_guard(curl_easy_init());
        if (!curl_guard.handle) {
            update_stats("curl_easy_init() failed");
            throw std::runtime_error("curl_easy_init() failed");
        }

        std::string body = {};
        // GAP-FIX manual_cleanup: CurlHeaders RAII guard replaces manual
        // curl_slist_free_all call.
        CurlHeaders headers = {};
        if (!auth_header.empty()) {
            headers.append(auth_header.c_str());
        }
        headers.append("Accept: application/json");

        // Cap per-attempt timeout to the remaining total budget so the overall
        // call cannot overrun max_total_retry_time_ms by more than one timeout.
        const auto elapsed_now = static_cast<int>(
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - request_start)
                .count());
        const long attempt_timeout
            = clampedCurlTimeout(config_.timeout_ms, config_.max_total_retry_time_ms - elapsed_now);

        curl_easy_setopt(curl_guard.handle, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl_guard.handle, CURLOPT_HTTPHEADER, headers.list);
        curl_easy_setopt(curl_guard.handle, CURLOPT_WRITEFUNCTION, writeStringCallback);
        curl_easy_setopt(curl_guard.handle, CURLOPT_WRITEDATA, &body);
        curl_easy_setopt(curl_guard.handle, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl_guard.handle, CURLOPT_TIMEOUT_MS, attempt_timeout);
        curl_easy_setopt(curl_guard.handle, CURLOPT_SSL_VERIFYPEER, config_.verify_ssl ? 1L : 0L);
        curl_easy_setopt(curl_guard.handle, CURLOPT_SSL_VERIFYHOST, config_.verify_ssl ? 2L : 0L);
        if (!config_.ca_bundle_path.empty()) {
            curl_easy_setopt(curl_guard.handle, CURLOPT_CAINFO, config_.ca_bundle_path.c_str());
        }
        if (!config_.pinned_public_key.empty()) {
            curl_easy_setopt(curl_guard.handle, CURLOPT_PINNEDPUBLICKEY, config_.pinned_public_key.c_str());
        }

        const CURLcode res = curl_easy_perform(curl_guard.handle);
        long http_code     = 0;
        curl_easy_getinfo(curl_guard.handle, CURLINFO_RESPONSE_CODE, &http_code);
        // CurlHandle and CurlHeaders destructors clean up automatically.

        if (res != CURLE_OK) {
            last_error = std::string("CURL error: ") + curl_easy_strerror(res);
            // Transient network error – retry
            continue;
        }

        // Authentication / authorisation failures are permanent; do not retry.
        if (http_code == 401 || http_code == 403) {
            const std::string err = "Registry authentication failed (HTTP " + std::to_string(http_code) + ")";
            update_stats(err);
            throw std::runtime_error(err);
        }
        if (http_code == 404) {
            const std::string err = "Resource not found (HTTP 404): " + url;
            update_stats(err);
            throw std::runtime_error(err);
        }
        if (http_code >= 500) {
            // Server error – transient, retry
            last_error = "Unexpected HTTP status " + std::to_string(http_code) + " for " + url;
            continue;
        }
        if (http_code < 200 || http_code >= 300) {
            const std::string err = "Unexpected HTTP status " + std::to_string(http_code) + " for " + url;
            update_stats(err);
            throw std::runtime_error(err);
        }

        update_stats("");
        return body;
    }

    const std::string final_error = last_error.empty() ? "httpGet failed after retries" : last_error;
    update_stats(final_error);
    throw std::runtime_error(final_error);
}

bool RemoteRegistryClient::httpGetBinary(const std::string &url, const std::string &out_path) {
    // GAP-FIX no_transit_encryption: per-call URL scheme validation.  Especially
    // critical here because download_url originates from untrusted registry JSON
    // and was not validated at construction time.
    requireHttpOrHttps(url, "RemoteRegistryClient::httpGetBinary");

    const std::string auth_header = buildAuthorizationHeader();

    // Clamp max_retries to [0, kMaxAllowedRetries] to prevent overflow in the backoff shift.
    const int max_retries = std::max(0, std::min(config_.max_retries, kMaxAllowedRetries));
    const int attempts    = max_retries + 1;

    std::string last_error = {};
    int attempts_made = 0;

    // Track wall-clock time to enforce max_total_retry_time_ms.
    const auto request_start = std::chrono::steady_clock::now();

    auto update_stats = [&]([[maybe_unused]] const std::string &error) {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        last_stats_.attempts   = attempts_made;
        last_stats_.last_error = error;
    };

    for (int attempt = 0; attempt < attempts; ++attempt) {
        // Enforce total retry time budget before starting this attempt.
        const auto elapsed_ms = static_cast<int>(
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - request_start)
                .count());
        const int remaining_ms = config_.max_total_retry_time_ms - elapsed_ms;
        if (remaining_ms <= 0) {
            spdlog::warn("RemoteRegistryClient::httpGetBinary: total retry budget "
                         "exhausted after {}ms for {}",
                         elapsed_ms, url);
            break;
        }

        if (attempt > 0) {
            const int shift_amount = std::min(attempt - 1, kMaxBackoffShift);
            const int backoff_ms   = 500 * (1 << shift_amount);
            const int sleep_ms     = std::min(backoff_ms, remaining_ms);
            spdlog::warn("RemoteRegistryClient::httpGetBinary: retry {}/{} after {}ms", attempt, max_retries, sleep_ms);
            asyncBackoffSleep(sleep_ms);
        }

        ++attempts_made;

        // GAP-FIX manual_cleanup / resource_leaked_in_exception: CurlHandle RAII
        // guard replaces the manual curl_easy_cleanup calls (including the early
        // return path when the output file cannot be opened).
        CurlHandle curl_guard(curl_easy_init());
        if (!curl_guard.handle) {
            spdlog::error("RemoteRegistryClient::httpGetBinary: curl_easy_init() failed");
            update_stats("curl_easy_init() failed");
            return false;
        }

        std::ofstream out(out_path, std::ios::binary | std::ios::trunc);
        if (!out.is_open()) {
            // CurlHandle destructor cleans up curl_guard.handle automatically.
            spdlog::error("RemoteRegistryClient::httpGetBinary: cannot open '{}' for writing", out_path);
            update_stats("cannot open output file");
            return false;
        }

        // GAP-FIX manual_cleanup: CurlHeaders RAII guard replaces manual
        // curl_slist_free_all call.
        CurlHeaders headers = {};
        if (!auth_header.empty()) {
            headers.append(auth_header.c_str());
        }

        // Cap per-attempt timeout to the remaining total budget.
        const auto elapsed_now = static_cast<int>(
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - request_start)
                .count());
        const long attempt_timeout
            = clampedCurlTimeout(config_.timeout_ms, config_.max_total_retry_time_ms - elapsed_now);

        curl_easy_setopt(curl_guard.handle, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl_guard.handle, CURLOPT_HTTPHEADER, headers.list);
        curl_easy_setopt(curl_guard.handle, CURLOPT_WRITEFUNCTION, writeFileCallback);
        curl_easy_setopt(curl_guard.handle, CURLOPT_WRITEDATA, &out);
        curl_easy_setopt(curl_guard.handle, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl_guard.handle, CURLOPT_TIMEOUT_MS, attempt_timeout);
        curl_easy_setopt(curl_guard.handle, CURLOPT_SSL_VERIFYPEER, config_.verify_ssl ? 1L : 0L);
        curl_easy_setopt(curl_guard.handle, CURLOPT_SSL_VERIFYHOST, config_.verify_ssl ? 2L : 0L);
        if (!config_.ca_bundle_path.empty()) {
            curl_easy_setopt(curl_guard.handle, CURLOPT_CAINFO, config_.ca_bundle_path.c_str());
        }
        if (!config_.pinned_public_key.empty()) {
            curl_easy_setopt(curl_guard.handle, CURLOPT_PINNEDPUBLICKEY, config_.pinned_public_key.c_str());
        }

        const CURLcode res = curl_easy_perform(curl_guard.handle);
        long http_code     = 0;
        curl_easy_getinfo(curl_guard.handle, CURLINFO_RESPONSE_CODE, &http_code);
        // CurlHandle and CurlHeaders destructors clean up automatically.
        out.close();

        // Gap: unchecked_result — flush/close errors (e.g. disk full) are only
        // observable via the stream's failbit after close().
        if (out.fail()) {
            last_error = "file write error while saving to '" + out_path + "'";
            spdlog::error("RemoteRegistryClient::httpGetBinary: {}", last_error);
            std::error_code ec = {};
            std::filesystem::remove(out_path, ec);
            continue;
        }

        if (res != CURLE_OK) {
            last_error = std::string("CURL error: ") + curl_easy_strerror(res);
            spdlog::error("RemoteRegistryClient::httpGetBinary: CURL error: {}", curl_easy_strerror(res));
            // Remove the incomplete file before retrying.
            std::error_code ec = {};
            std::filesystem::remove(out_path, ec);
            continue;
        }

        if (http_code < 200 || http_code >= 300) {
            last_error = "HTTP " + std::to_string(http_code) + " for " + url;
            spdlog::error("RemoteRegistryClient::httpGetBinary: HTTP {} for {}", http_code, url);
            // Remove the incomplete file.
            std::error_code ec = {};
            std::filesystem::remove(out_path, ec);
            if (http_code >= 500) {
                // Server error – transient, retry
                continue;
            }
            update_stats(last_error);
            return false;
        }

        update_stats("");
        return true;
    }

    spdlog::error("RemoteRegistryClient::httpGetBinary: all {} attempt(s) failed for {}", attempts, url);
    const std::string final_error = last_error.empty() ? "httpGetBinary failed after retries" : last_error;
    update_stats(final_error);
    return false;
}

std::future<std::string> RemoteRegistryClient::httpGetAsync(const std::string &url) {
    // Caller must ensure this instance outlives the returned future.
    // url is copied to decouple the async worker from the caller's lifetime.
    // WARNING: destroying the client before the future completes is undefined (see header docs).
    std::weak_ptr<RemoteRegistryClient> weak_self;
    try {
        weak_self = shared_from_this();
    } catch (const std::bad_weak_ptr &) {
        throw std::runtime_error("httpGetAsync requires RemoteRegistryClient to be managed by std::shared_ptr");
    }

    return std::async(std::launch::async, [weak_self, url]() {
        auto self = weak_self.lock();
        if (!self) {
            throw std::runtime_error("RemoteRegistryClient destroyed before httpGetAsync completed");
        }
        return self->httpGet(url);
    });
}

std::future<bool> RemoteRegistryClient::httpGetBinaryAsync(const std::string &url, const std::string &out_path) {
    // Caller must ensure this instance outlives the returned future.
    // url/out_path are copied to decouple the async worker from the caller's lifetime.
    // WARNING: destroying the client before the future completes is undefined (see header docs).
    std::weak_ptr<RemoteRegistryClient> weak_self;
    try {
        weak_self = shared_from_this();
    } catch (const std::bad_weak_ptr &) {
        throw std::runtime_error("httpGetBinaryAsync requires RemoteRegistryClient to be managed by std::shared_ptr");
    }

    return std::async(std::launch::async, [weak_self, url, out_path]() {
        auto self = weak_self.lock();
        if (!self) {
            throw std::runtime_error("RemoteRegistryClient destroyed before httpGetBinaryAsync completed");
        }
        return self->httpGetBinary(url, out_path);
    });
}

/*static*/ void
RemoteRegistryClient::setBackoffDispatcher(std::function<std::future<void>(std::chrono::milliseconds)> dispatcher) {
    auto &state = dispatcherState();
    std::lock_guard<std::mutex> lock(state.mutex);
    state.dispatcher = std::move(dispatcher);
}

RequestStats RemoteRegistryClient::lastRequestStats() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return last_stats_;
}

/*static*/ bool RemoteRegistryClient::verifyIntegrity(const std::string &file_path,
                                                      const std::string &expected_sha256) {
    const std::string actual = sha256File(file_path);
    if (actual.empty()) {
        spdlog::error("RemoteRegistryClient::verifyIntegrity: could not hash '{}'", file_path);
        return false;
    }
    const bool ok = (actual == expected_sha256);
    if (!ok) {
        spdlog::error("RemoteRegistryClient::verifyIntegrity: hash mismatch for '{}' "
                      "(expected={}, actual={})",
                      file_path, expected_sha256, actual);
    }
    return ok;
}

/*static*/ bool RemoteRegistryClient::parseEntry(const nlohmann::json &obj, RegistryPluginEntry &out) {
    if (!obj.is_object()) {
        return false;
    }
    if (!obj.contains("name") || !obj.contains("download_url")) {
        return false;
    }
    out.name               = obj.value("name", "");
    out.version            = obj.value("version", "");
    out.description        = obj.value("description", "");
    out.download_url       = obj.value("download_url", "");
    out.sha256             = obj.value("sha256", "");
    out.min_themis_version = obj.value("min_themis_version", "");
    return !out.name.empty() && !out.download_url.empty();
}

} // namespace modules
} // namespace themis
