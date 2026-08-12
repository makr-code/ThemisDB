/**
 * @file storage_initialization.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "storage/storage_engine.h"
#include "themis/base/interfaces/storage_interface.h"
#include "themis/base/interfaces/query_interface.h"
#include "themis/base/interfaces/security_interface.h"
#include <memory>
#include <stdexcept>

namespace themis {

/**
 * @brief Builder pattern for constructing StorageEngine instances
 * 
 * Provides a fluent interface for configuring and building StorageEngine
 * instances with custom or default dependencies.
 * 
 * Example usage:
 * @code
 * auto engine = StorageEngineBuilder::standard()
 *     .withEvaluator(custom_evaluator)
 *     .withEncryption(custom_encryption)
 *     .build();
 * @endcode
 */
class StorageEngineBuilder {
public:
    /**
     * @brief Construct an empty builder.
     *
     * Required dependencies are supplied explicitly before build(). The
     * builder does not allocate any backend resources on its own.
     */
    StorageEngineBuilder() = default;
    
    /**
     * @brief Set the expression evaluator
     * 
     * @param eval Expression evaluator implementation. If null, build() will
     *        fail fast with a descriptive runtime error.
     * @return Reference to this builder for chaining
     */
    StorageEngineBuilder& withEvaluator(IExpressionEvaluatorPtr eval) {
        evaluator_ = eval;
        return *this;
    }
    
    /**
     * @brief Set the field encryption provider
     * 
     * @param enc Field encryption implementation. A null value is accepted at
     *        assignment time but rejected during build().
     * @return Reference to this builder for chaining
     */
    StorageEngineBuilder& withEncryption(IFieldEncryptionPtr enc) {
        encryption_ = enc;
        return *this;
    }
    
    /**
     * @brief Set the key provider
     * 
     * @param provider Key provider implementation. A null value is accepted at
     *        assignment time but rejected during build().
     * @return Reference to this builder for chaining
     */
    StorageEngineBuilder& withKeyProvider(IKeyProviderPtr provider) {
        key_provider_ = provider;
        return *this;
    }
    
    /**
     * @brief Set the index manager
     * 
     * @param index Index manager implementation. May be null to disable index
     *        integration in the constructed storage engine.
     * @return Reference to this builder for chaining
     */
    StorageEngineBuilder& withIndexManager(IIndexManagerPtr index) {
        index_manager_ = index;
        return *this;
    }
    
    /**
     * @brief Build the StorageEngine instance
     * 
        * @return Shared pointer to configured StorageEngine.
        * @throws std::runtime_error if a required dependency is missing. The
        *         builder validates the evaluator, encryption provider, and key
        *         provider before constructing the engine.
     */
    std::shared_ptr<StorageEngine> build() {
        // Validate required dependencies
        if (!evaluator_) {
            throw std::runtime_error("StorageEngineBuilder: Evaluator required");
        }
        if (!encryption_) {
            throw std::runtime_error("StorageEngineBuilder: Encryption required");
        }
        if (!key_provider_) {
            throw std::runtime_error("StorageEngineBuilder: KeyProvider required");
        }
        
        // Build and return
        return std::make_shared<StorageEngine>(
            evaluator_, encryption_, key_provider_, index_manager_
        );
    }
    
    /**
     * @brief Create a builder with standard default implementations
     * 
     * Returns a builder pre-configured with default implementations of all
     * dependencies. Callers can override individual components before calling
     * build().
     * 
     * @return Builder with default implementations.
     */
    static StorageEngineBuilder standard() {
        StorageEngineBuilder builder;
        
        // Populate with default implementations
        builder.withEvaluator(StorageEngine::createDefaultEvaluator());
        builder.withEncryption(StorageEngine::createDefaultEncryption());
        builder.withKeyProvider(StorageEngine::createDefaultKeyProvider());
        builder.withIndexManager(StorageEngine::createDefaultIndexManager());
        
        return builder;
    }
    
private:
    IExpressionEvaluatorPtr evaluator_;
    IFieldEncryptionPtr encryption_;
    IKeyProviderPtr key_provider_;
    IIndexManagerPtr index_manager_;
};

} // namespace themis
