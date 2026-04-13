/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            storage_engine.cpp                                 ║
  Version:         0.0.39                                             ║
  Last Modified:   2026-04-13 20:36:48                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   91.0/100                                       ║
    • Total Lines:     578                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • dd319b9918  2026-04-13  Add CI/CD workflows and scripts for release management ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "storage/storage_engine.h"
#include "storage/rocksdb_wrapper.h"
#include "utils/expected.h"
#include "utils/tracing.h"
#include <fmt/format.h>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <cstdlib>
#include <chrono>

namespace themis {

// Default implementations for createDefault() factory
// WARNING: These are NOT production-safe implementations!
// They are provided for testing, development, and backward compatibility only.
// Production systems MUST provide real implementations via dependency injection.
namespace {

// Flag to track if production mode is enabled
// Set via environment variable THEMIS_PRODUCTION_MODE=1 or THEMIS_ENVIRONMENT=production
// Note: For testing, use the test-only reset function below
bool is_production_mode() {
    const char* mode_env = std::getenv("THEMIS_PRODUCTION_MODE");
    const char* env_env = std::getenv("THEMIS_ENVIRONMENT");
    
    // Check THEMIS_PRODUCTION_MODE first (accepts "1", "true", "production")
    if (mode_env) {
        std::string mode_str(mode_env);
        if (mode_str == "1" || mode_str == "true" || mode_str == "production") {
            return true;
        }
    }
    
    // Check THEMIS_ENVIRONMENT (accepts "production")
    if (env_env && std::string(env_env) == "production") {
        return true;
    }
    
    return false;
}

class DefaultExpressionEvaluator : public IExpressionEvaluator {
public:
    DefaultExpressionEvaluator() {
        if (is_production_mode()) {
            spdlog::warn("StorageEngine: Using default (no-op) expression evaluator in PRODUCTION mode. "
                        "This is NOT recommended. Provide a real implementation via dependency injection.");
        }
    }

    bool evaluate(const std::string& expression, const void* context) override {
        (void)context;
        // Default implementation: always return true (no filtering)
        // Real implementation would parse and evaluate the expression
        if (is_production_mode() && !expression.empty()) {
            spdlog::error("StorageEngine: Expression evaluation attempted with default evaluator in PRODUCTION mode: '{}'", expression);
        }
        return true;
    }
    
    std::string get_expression_type() const override {
        return "default";
    }
};

class DefaultFieldEncryption : public IFieldEncryption {
public:
    DefaultFieldEncryption() {
        if (is_production_mode()) {
            throw std::runtime_error(
                "StorageEngine: Cannot use default (no-op) field encryption in PRODUCTION mode. "
                "DATA WOULD NOT BE ENCRYPTED! Provide a real FieldEncryption implementation via dependency injection.");
        }
    }

    std::vector<uint8_t> encrypt_field(
        const std::string& field_name,
        const std::vector<uint8_t>& plaintext) override {
        (void)field_name;
        // Default implementation: no-op encryption (returns plaintext)
        // Real implementation would use AES-GCM or similar
        return plaintext;
    }
    
    std::vector<uint8_t> decrypt_field(
        const std::string& field_name,
        const std::vector<uint8_t>& ciphertext) override {
        (void)field_name;
        // Default implementation: no-op decryption
        return ciphertext;
    }
    
    bool should_encrypt(const std::string& field_name) const override {
        (void)field_name;
        // Default: don't encrypt any fields
        return false;
    }
};

class DefaultKeyProvider : public IKeyProvider {
public:
    DefaultKeyProvider() {
        if (is_production_mode()) {
            throw std::runtime_error(
                "StorageEngine: Cannot use default (insecure) key provider in PRODUCTION mode. "
                "KEYS WOULD NOT BE SECURE! Provide a real KeyProvider implementation via dependency injection.");
        }
    }

    std::vector<uint8_t> get_key(const std::string& key_id) override {
        (void)key_id;
        // Default implementation: return a dummy key
        // Real implementation would fetch from Vault, HSM, etc.
        return std::vector<uint8_t>(32, 0x42); // 32-byte dummy key
    }
    
    std::vector<uint8_t> rotate_key(const std::string& key_id) override {
        // Default implementation: return the same dummy key
        return get_key(key_id);
    }
};

class DefaultIndexManager : public IIndexManager {
public:
    DefaultIndexManager() {
        if (is_production_mode()) {
            spdlog::warn("StorageEngine: Using default (no-op) index manager in PRODUCTION mode. "
                        "Indexes will not be functional. Provide a real implementation via dependency injection.");
        }
    }

    Result<ISecondaryIndex*> createSecondaryIndex(
        std::string_view name,
        std::string_view field_name,
        const std::string& config = "") override {
        (void)field_name;
        (void)config;
        // Default implementation: no-op, returns nullptr
        if (is_production_mode()) {
            spdlog::warn("StorageEngine: Index creation attempted with default (no-op) index manager in PRODUCTION mode: '{}'", name);
        }
        return Ok<ISecondaryIndex*>(nullptr);
    }
    
    Result<IVectorIndex*> createVectorIndex(
        std::string_view name,
        uint32_t dimension,
        const std::string& config = "") override {
        (void)dimension;
        (void)config;
        // Default implementation: no-op, returns nullptr
        if (is_production_mode()) {
            spdlog::warn("StorageEngine: Vector index creation attempted with default (no-op) index manager in PRODUCTION mode: '{}'", name);
        }
        return Ok<IVectorIndex*>(nullptr);
    }
    
    Result<IGraphIndex*> createGraphIndex(
        std::string_view name,
        const std::string& config = "") override {
        (void)config;
        // Default implementation: no-op, returns nullptr
        if (is_production_mode()) {
            spdlog::warn("StorageEngine: Graph index creation attempted with default (no-op) index manager in PRODUCTION mode: '{}'", name);
        }
        return Ok<IGraphIndex*>(nullptr);
    }
    
    Result<ISecondaryIndex*> getSecondaryIndex(std::string_view name) const override {
        // Default implementation: always not found
        return Err<ISecondaryIndex*>(errors::ErrorCode::ERR_INDEX_NOT_FOUND, 
                                       fmt::format("Index '{}' not found (default manager)", name));
    }
    
    Result<IVectorIndex*> getVectorIndex(std::string_view name) const override {
        // Default implementation: always not found
        return Err<IVectorIndex*>(errors::ErrorCode::ERR_INDEX_NOT_FOUND,
                                    fmt::format("Index '{}' not found (default manager)", name));
    }
    
    Result<IGraphIndex*> getGraphIndex(std::string_view name) const override {
        // Default implementation: always not found
        return Err<IGraphIndex*>(errors::ErrorCode::ERR_INDEX_NOT_FOUND,
                                   fmt::format("Index '{}' not found (default manager)", name));
    }
    
    Result<void> dropIndex(std::string_view name) override {
        // Default implementation: always succeeds (no-op)
        (void)name;
        return OkVoid();
    }
    
    std::vector<std::string> listIndexes() const override {
        // Default implementation: empty list
        return {};
    }
    
    Result<IndexType> getIndexType(std::string_view name) const override {
        // Default implementation: always not found
        return Err<IndexType>(errors::ErrorCode::ERR_INDEX_NOT_FOUND,
                               std::string(name));
    }
};

} // anonymous namespace

// Factory methods for default implementations
IExpressionEvaluatorPtr StorageEngine::createDefaultEvaluator() {
    return std::make_shared<DefaultExpressionEvaluator>();
}

IFieldEncryptionPtr StorageEngine::createDefaultEncryption() {
    return std::make_shared<DefaultFieldEncryption>();
}

IKeyProviderPtr StorageEngine::createDefaultKeyProvider() {
    return std::make_shared<DefaultKeyProvider>();
}

IIndexManagerPtr StorageEngine::createDefaultIndexManager() {
    return std::make_shared<DefaultIndexManager>();
}

StorageEngine::StorageEngine(
    IExpressionEvaluatorPtr evaluator,
    IFieldEncryptionPtr encryption,
    IKeyProviderPtr key_provider,
    IIndexManagerPtr index_manager
) : evaluator_(evaluator),
    encryption_(encryption),
    key_provider_(key_provider),
    index_manager_(index_manager) {
    
    // Validate required dependencies
    if (!evaluator_) {
        throw std::invalid_argument("StorageEngine: evaluator cannot be null");
    }
    if (!encryption_) {
        throw std::invalid_argument("StorageEngine: encryption cannot be null");
    }
    if (!key_provider_) {
        throw std::invalid_argument("StorageEngine: key_provider cannot be null");
    }
    
    // index_manager is optional, so we don't validate it
}

std::shared_ptr<StorageEngine> StorageEngine::createDefault() {
    // Create default implementations using factory methods
    return std::make_shared<StorageEngine>(
        createDefaultEvaluator(),
        createDefaultEncryption(),
        createDefaultKeyProvider(),
        createDefaultIndexManager()
    );
}

Result<void> StorageEngine::open(const std::string& db_path) {
    if (is_open_) {
        return ErrVoid(errors::ErrorCode::ERR_STORAGE_TRANSACTION_FAILED, 
                       "Storage engine already open");
    }
    
    db_path_ = db_path;

    // Open the underlying RocksDB instance.
    RocksDBWrapper::Config cfg;
    cfg.db_path    = db_path;
    cfg.enable_wal = true;
    rocksdb_ = std::make_shared<RocksDBWrapper>(cfg);
    if (!rocksdb_->open()) {
        rocksdb_.reset();
        return ErrVoid(errors::ErrorCode::ERR_STORAGE_TRANSACTION_FAILED,
                       "Failed to open RocksDB at: " + db_path);
    }

    is_open_ = true;
    return OkVoid();
}

void StorageEngine::close() {
    if (!is_open_) {
        return; // Already closed
    }
    
    if (rocksdb_) {
        rocksdb_->close();
        rocksdb_.reset();
    }

    is_open_ = false;
}

// ── Helper: update a min/max atomic (relaxed, best-effort) ──────────────────
namespace {
void atomicUpdateMin(std::atomic<uint64_t>& m, uint64_t v) {
    uint64_t cur = m.load(std::memory_order_relaxed);
    while (v < cur && !m.compare_exchange_weak(cur, v, std::memory_order_relaxed))
        ;
}
void atomicUpdateMax(std::atomic<uint64_t>& m, uint64_t v) {
    uint64_t cur = m.load(std::memory_order_relaxed);
    while (v > cur && !m.compare_exchange_weak(cur, v, std::memory_order_relaxed))
        ;
}
} // anonymous namespace

Result<void> StorageEngine::put(const std::string& key, const std::string& value) {
    TracedSpan span("StorageEngine.put");
    span.setAttribute("storage.key_size", static_cast<int64_t>(key.size()));
    span.setAttribute("storage.value_size", static_cast<int64_t>(value.size()));
    
    if (!is_open_) {
        span.setStatus(false, "Storage not open");
        io_put_errors_.fetch_add(1, std::memory_order_relaxed);
        return ErrVoid(errors::ErrorCode::ERR_STORAGE_TRANSACTION_FAILED,
                       "Storage engine not open");
    }

    auto t0 = std::chrono::steady_clock::now();
    bool ok = rocksdb_->put(key, value);
    uint64_t us = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now() - t0).count());

    if (!ok) {
        io_put_errors_.fetch_add(1, std::memory_order_relaxed);
        span.setStatus(false, "RocksDB put failed");
        return ErrVoid(errors::ErrorCode::ERR_STORAGE_DISK_FULL,
                       "Failed to write key: " + key);
    }
    io_put_ops_.fetch_add(1, std::memory_order_relaxed);
    io_put_latency_.fetch_add(us, std::memory_order_relaxed);
    atomicUpdateMin(io_put_min_, us);
    atomicUpdateMax(io_put_max_, us);

    span.setStatus(true);
    return OkVoid();
}

Result<std::string> StorageEngine::get(const std::string& key) {
    TracedSpan span("StorageEngine.get");
    span.setAttribute("storage.key_size", static_cast<int64_t>(key.size()));
    
    if (!is_open_) {
        span.setStatus(false, "Storage not open");
        io_get_errors_.fetch_add(1, std::memory_order_relaxed);
        return Err<std::string>(errors::ErrorCode::ERR_STORAGE_TRANSACTION_FAILED,
                                "Storage engine not open");
    }

    auto t0 = std::chrono::steady_clock::now();
    std::string out;
    bool found = rocksdb_->get(key, out);
    uint64_t us = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now() - t0).count());

    if (found) {
        io_get_ops_.fetch_add(1, std::memory_order_relaxed);
        io_get_latency_.fetch_add(us, std::memory_order_relaxed);
        atomicUpdateMin(io_get_min_, us);
        atomicUpdateMax(io_get_max_, us);
        span.setStatus(true);
        return Ok(std::move(out));
    }

    io_get_errors_.fetch_add(1, std::memory_order_relaxed);
    span.setStatus(false, "Key not found");
    return Err<std::string>(errors::ErrorCode::ERR_STORAGE_FILE_NOT_FOUND,
                            fmt::format("Key '{}' not found", key));
}

Result<void> StorageEngine::del(const std::string& key) {
    TracedSpan span("StorageEngine.del");
    span.setAttribute("storage.key_size", static_cast<int64_t>(key.size()));
    
    if (!is_open_) {
        span.setStatus(false, "Storage not open");
        io_del_errors_.fetch_add(1, std::memory_order_relaxed);
        return ErrVoid(errors::ErrorCode::ERR_STORAGE_TRANSACTION_FAILED,
                       "Storage engine not open");
    }

    auto t0 = std::chrono::steady_clock::now();
    bool ok = rocksdb_->del(key);
    uint64_t us = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now() - t0).count());

    if (!ok) {
        io_del_errors_.fetch_add(1, std::memory_order_relaxed);
        span.setStatus(false, "RocksDB del failed");
        return ErrVoid(errors::ErrorCode::ERR_STORAGE_DISK_FULL,
                       "Failed to delete key: " + key);
    }
    io_del_ops_.fetch_add(1, std::memory_order_relaxed);
    io_del_latency_.fetch_add(us, std::memory_order_relaxed);
    atomicUpdateMin(io_del_min_, us);
    atomicUpdateMax(io_del_max_, us);

    span.setStatus(true);
    return OkVoid();
}

StorageEngine::IOMetrics StorageEngine::ioMetrics() const {
    IOMetrics m;
    m.put_ops            = io_put_ops_.load(std::memory_order_relaxed);
    m.put_errors         = io_put_errors_.load(std::memory_order_relaxed);
    m.put_latency_us     = io_put_latency_.load(std::memory_order_relaxed);
    m.put_latency_min_us = io_put_min_.load(std::memory_order_relaxed);
    m.put_latency_max_us = io_put_max_.load(std::memory_order_relaxed);

    m.get_ops            = io_get_ops_.load(std::memory_order_relaxed);
    m.get_errors         = io_get_errors_.load(std::memory_order_relaxed);
    m.get_latency_us     = io_get_latency_.load(std::memory_order_relaxed);
    m.get_latency_min_us = io_get_min_.load(std::memory_order_relaxed);
    m.get_latency_max_us = io_get_max_.load(std::memory_order_relaxed);

    m.del_ops            = io_del_ops_.load(std::memory_order_relaxed);
    m.del_errors         = io_del_errors_.load(std::memory_order_relaxed);
    m.del_latency_us     = io_del_latency_.load(std::memory_order_relaxed);
    m.del_latency_min_us = io_del_min_.load(std::memory_order_relaxed);
    m.del_latency_max_us = io_del_max_.load(std::memory_order_relaxed);
    return m;
}

void StorageEngine::resetIOMetrics() {
    io_put_ops_.store(0, std::memory_order_relaxed);
    io_put_errors_.store(0, std::memory_order_relaxed);
    io_put_latency_.store(0, std::memory_order_relaxed);
    io_put_min_.store(UINT64_MAX, std::memory_order_relaxed);
    io_put_max_.store(0, std::memory_order_relaxed);

    io_get_ops_.store(0, std::memory_order_relaxed);
    io_get_errors_.store(0, std::memory_order_relaxed);
    io_get_latency_.store(0, std::memory_order_relaxed);
    io_get_min_.store(UINT64_MAX, std::memory_order_relaxed);
    io_get_max_.store(0, std::memory_order_relaxed);

    io_del_ops_.store(0, std::memory_order_relaxed);
    io_del_errors_.store(0, std::memory_order_relaxed);
    io_del_latency_.store(0, std::memory_order_relaxed);
    io_del_min_.store(UINT64_MAX, std::memory_order_relaxed);
    io_del_max_.store(0, std::memory_order_relaxed);
}

Result<void> StorageEngine::scanRange(
    std::string_view start_key,
    std::string_view end_key,
    std::function<bool(std::string_view, std::string_view)> callback)
{
    if (!is_open_) {
        return ErrVoid(errors::ErrorCode::ERR_STORAGE_TRANSACTION_FAILED,
                       "Storage engine not open");
    }

    sc_calls_.fetch_add(1, std::memory_order_relaxed);
    bool stopped_early = false;
    rocksdb_->scanRange(start_key, end_key,
        [&](std::string_view k, std::string_view v) -> bool {
            sc_examined_.fetch_add(1, std::memory_order_relaxed);
            bool cont = callback(k, v);
            sc_returned_.fetch_add(1, std::memory_order_relaxed);
            if (!cont) stopped_early = true;
            return cont;
        });
    if (stopped_early) sc_early_stops_.fetch_add(1, std::memory_order_relaxed);
    return OkVoid();
}

Result<void> StorageEngine::scanPrefix(
    std::string_view prefix,
    std::function<bool(std::string_view, std::string_view)> callback)
{
    if (!is_open_) {
        return ErrVoid(errors::ErrorCode::ERR_STORAGE_TRANSACTION_FAILED,
                       "Storage engine not open");
    }

    sc_calls_.fetch_add(1, std::memory_order_relaxed);
    bool stopped_early = false;
    rocksdb_->scanPrefix(prefix,
        [&](std::string_view k, std::string_view v) -> bool {
            sc_examined_.fetch_add(1, std::memory_order_relaxed);
            bool cont = callback(k, v);
            sc_returned_.fetch_add(1, std::memory_order_relaxed);
            if (!cont) stopped_early = true;
            return cont;
        });
    if (stopped_early) sc_early_stops_.fetch_add(1, std::memory_order_relaxed);
    return OkVoid();
}

Result<void> StorageEngine::scanPredicate(
    std::string_view start_key,
    std::string_view end_key,
    std::function<bool(std::string_view, std::string_view)> predicate,
    std::function<bool(std::string_view, std::string_view)> callback)
{
    if (!is_open_) {
        return ErrVoid(errors::ErrorCode::ERR_STORAGE_TRANSACTION_FAILED,
                       "Storage engine not open");
    }

    sc_calls_.fetch_add(1, std::memory_order_relaxed);
    bool stopped_early = false;
    rocksdb_->scanRange(start_key, end_key,
        [&](std::string_view k, std::string_view v) -> bool {
            sc_examined_.fetch_add(1, std::memory_order_relaxed);
            if (!predicate(k, v)) return true; // skip, but continue
            sc_returned_.fetch_add(1, std::memory_order_relaxed);
            bool cont = callback(k, v);
            if (!cont) stopped_early = true;
            return cont;
        });
    if (stopped_early) sc_early_stops_.fetch_add(1, std::memory_order_relaxed);
    return OkVoid();
}

StorageEngine::ScanCounters StorageEngine::scanCounters() const {
    ScanCounters c;
    c.scan_calls    = sc_calls_.load(std::memory_order_relaxed);
    c.keys_examined = sc_examined_.load(std::memory_order_relaxed);
    c.keys_returned = sc_returned_.load(std::memory_order_relaxed);
    c.early_stops   = sc_early_stops_.load(std::memory_order_relaxed);
    return c;
}

void StorageEngine::resetScanCounters() {
    sc_calls_.store(0, std::memory_order_relaxed);
    sc_examined_.store(0, std::memory_order_relaxed);
    sc_returned_.store(0, std::memory_order_relaxed);
    sc_early_stops_.store(0, std::memory_order_relaxed);
}

bool StorageEngine::apply_filter(const std::string& filter_expr, const void* context) {
    // Use injected evaluator instead of concrete QueryEngine
    return evaluator_->evaluate(filter_expr, context);
}

std::vector<uint8_t> StorageEngine::encrypt_field(
    const std::string& field_name,
    const std::vector<uint8_t>& plaintext) {
    // Use injected encryption instead of concrete FieldEncryption
    return encryption_->encrypt_field(field_name, plaintext);
}

std::vector<uint8_t> StorageEngine::decrypt_field(
    const std::string& field_name,
    const std::vector<uint8_t>& ciphertext) {
    // Use injected encryption instead of concrete FieldEncryption
    return encryption_->decrypt_field(field_name, ciphertext);
}

} // namespace themis
