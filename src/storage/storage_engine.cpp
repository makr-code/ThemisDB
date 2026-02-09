#include "storage/storage_engine.h"
#include "utils/expected.h"
#include "utils/tracing.h"
#include <fmt/format.h>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <cstdlib>

namespace themis {

// Default implementations for createDefault() factory
// WARNING: These are NOT production-safe implementations!
// They are provided for testing, development, and backward compatibility only.
// Production systems MUST provide real implementations via dependency injection.
namespace {

// Flag to track if production mode is enabled
// Set via environment variable THEMIS_PRODUCTION_MODE=1
bool is_production_mode() {
    static const bool production = []() {
        const char* env = std::getenv("THEMIS_PRODUCTION_MODE");
        return env && (std::string(env) == "1" || std::string(env) == "true");
    }();
    return production;
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
            spdlog::error("StorageEngine: Using default (no-op) field encryption in PRODUCTION mode. "
                         "DATA IS NOT ENCRYPTED! Provide a real implementation via dependency injection.");
        }
    }

    std::vector<uint8_t> encrypt_field(
        const std::string& field_name,
        const std::vector<uint8_t>& plaintext) override {
        // Default implementation: no-op encryption (returns plaintext)
        // Real implementation would use AES-GCM or similar
        if (is_production_mode()) {
            spdlog::error("StorageEngine: Field encryption attempted with default (no-op) encryption in PRODUCTION mode: '{}'", field_name);
        }
        return plaintext;
    }
    
    std::vector<uint8_t> decrypt_field(
        const std::string& field_name,
        const std::vector<uint8_t>& ciphertext) override {
        // Default implementation: no-op decryption
        return ciphertext;
    }
    
    bool should_encrypt(const std::string& field_name) const override {
        // Default: don't encrypt any fields
        return false;
    }
};

class DefaultKeyProvider : public IKeyProvider {
public:
    DefaultKeyProvider() {
        if (is_production_mode()) {
            spdlog::error("StorageEngine: Using default (insecure) key provider in PRODUCTION mode. "
                         "KEYS ARE NOT SECURE! Provide a real implementation via dependency injection.");
        }
    }

    std::vector<uint8_t> get_key(const std::string& key_id) override {
        // Default implementation: return a dummy key
        // Real implementation would fetch from Vault, HSM, etc.
        if (is_production_mode()) {
            spdlog::error("StorageEngine: Key retrieval attempted with default (insecure) key provider in PRODUCTION mode: '{}'", key_id);
        }
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
        // Default implementation: no-op, returns nullptr
        if (is_production_mode()) {
            spdlog::warn("StorageEngine: Vector index creation attempted with default (no-op) index manager in PRODUCTION mode: '{}'", name);
        }
        return Ok<IVectorIndex*>(nullptr);
    }
    
    Result<IGraphIndex*> createGraphIndex(
        std::string_view name,
        const std::string& config = "") override {
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
    is_open_ = true;
    
    // Real implementation would initialize RocksDB here
    return OkVoid();
}

void StorageEngine::close() {
    if (!is_open_) {
        return; // Already closed
    }
    
    is_open_ = false;
    
    // Real implementation would close RocksDB here
}

Result<void> StorageEngine::put(const std::string& key, const std::string& value) {
    TracedSpan span("StorageEngine.put");
    span.setAttribute("storage.key_size", static_cast<int64_t>(key.size()));
    span.setAttribute("storage.value_size", static_cast<int64_t>(value.size()));
    
    if (!is_open_) {
        span.setStatus(false, "Storage not open");
        return ErrVoid(errors::ErrorCode::ERR_STORAGE_TRANSACTION_FAILED,
                       "Storage engine not open");
    }
    
    // Real implementation would write to RocksDB here
    span.setStatus(true);
    return OkVoid();
}

Result<std::string> StorageEngine::get(const std::string& key) {
    TracedSpan span("StorageEngine.get");
    span.setAttribute("storage.key_size", static_cast<int64_t>(key.size()));
    
    if (!is_open_) {
        span.setStatus(false, "Storage not open");
        return Err<std::string>(errors::ErrorCode::ERR_STORAGE_TRANSACTION_FAILED,
                                "Storage engine not open");
    }
    
    // Real implementation would read from RocksDB here
    // For now, return key not found
    span.setStatus(false, "Key not found");
    return Err<std::string>(errors::ErrorCode::ERR_STORAGE_FILE_NOT_FOUND,
                            fmt::format("Key '{}' not found", key));
}

Result<void> StorageEngine::del(const std::string& key) {
    TracedSpan span("StorageEngine.del");
    span.setAttribute("storage.key_size", static_cast<int64_t>(key.size()));
    
    if (!is_open_) {
        span.setStatus(false, "Storage not open");
        return ErrVoid(errors::ErrorCode::ERR_STORAGE_TRANSACTION_FAILED,
                       "Storage engine not open");
    }
    
    // Real implementation would delete from RocksDB here
    span.setStatus(true);
    return OkVoid();
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
