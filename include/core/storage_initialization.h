/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            storage_initialization.h                           ║
  Version:         0.0.39                                             ║
  Last Modified:   2026-04-13 20:21:35                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     149                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • a629043ab2  2026-02-22  Audit: document gaps found - benchmarks and stale annotat... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
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
    StorageEngineBuilder() = default;
    
    /**
     * @brief Set the expression evaluator
     * 
     * @param eval Expression evaluator implementation
     * @return Reference to this builder for chaining
     */
    StorageEngineBuilder& withEvaluator(IExpressionEvaluatorPtr eval) {
        evaluator_ = eval;
        return *this;
    }
    
    /**
     * @brief Set the field encryption provider
     * 
     * @param enc Field encryption implementation
     * @return Reference to this builder for chaining
     */
    StorageEngineBuilder& withEncryption(IFieldEncryptionPtr enc) {
        encryption_ = enc;
        return *this;
    }
    
    /**
     * @brief Set the key provider
     * 
     * @param provider Key provider implementation
     * @return Reference to this builder for chaining
     */
    StorageEngineBuilder& withKeyProvider(IKeyProviderPtr provider) {
        key_provider_ = provider;
        return *this;
    }
    
    /**
     * @brief Set the index manager
     * 
     * @param index Index manager implementation
     * @return Reference to this builder for chaining
     */
    StorageEngineBuilder& withIndexManager(IIndexManagerPtr index) {
        index_manager_ = index;
        return *this;
    }
    
    /**
     * @brief Build the StorageEngine instance
     * 
     * @return Shared pointer to configured StorageEngine
     * @throws std::runtime_error if required dependencies are missing
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
     * Returns a builder pre-configured with default implementations
     * of all dependencies. You can override individual components
     * before calling build().
     * 
     * @return Builder with default implementations
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
