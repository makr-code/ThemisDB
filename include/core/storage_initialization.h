/**
 * @file storage_initialization.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

class StorageEngineBuilder {
public:
    StorageEngineBuilder() = default;
    
    /**
     * @brief With Evaluator.
     * @param[in] eval Input parameter.
     * @return Return value.
     * @details Implements withEvaluator without additional internal calls.
     */
    StorageEngineBuilder& withEvaluator(IExpressionEvaluatorPtr eval) {
        evaluator_ = eval;
        return *this;
    }
    
    /**
     * @brief With Encryption.
     * @param[in] enc Input parameter.
     * @return Return value.
     * @details Implements withEncryption without additional internal calls.
     */
    StorageEngineBuilder& withEncryption(IFieldEncryptionPtr enc) {
        encryption_ = enc;
        return *this;
    }
    
    /**
     * @brief With Key Provider.
     * @param[in] provider Input parameter.
     * @return Return value.
     * @details Implements withKeyProvider without additional internal calls.
     */
    StorageEngineBuilder& withKeyProvider(IKeyProviderPtr provider) {
        key_provider_ = provider;
        return *this;
    }
    
    /**
     * @brief With Index Manager.
     * @param[in] index Input parameter.
     * @return Return value.
     * @details Implements withIndexManager without additional internal calls.
     */
    StorageEngineBuilder& withIndexManager(IIndexManagerPtr index) {
        index_manager_ = index;
        return *this;
    }
    
    /**
     * @brief Build.
     * @return Return value.
     * @throws std::runtime_error if an error occurs.
     * @details Implements build without additional internal calls.
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
     * @brief Standard.
     * @return Return value.
     * @details Calls: withEvaluator(), StorageEngine::createDefaultEvaluator(), withEncryption(), StorageEngine::createDefaultEncryption(), withKeyProvider(), StorageEngine::createDefaultKeyProvider(), withIndexManager(), StorageEngine::createDefaultIndexManager().
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
