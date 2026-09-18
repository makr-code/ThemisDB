/**
 * @file query_engine_builder.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/// @file query_engine_builder.h
/// @brief Builder pattern for QueryEngine construction with dependency injection
///
/// This builder enables flexible construction of QueryEngine instances with
/// various dependency configurations. It supports:
/// - Fluent API for setting dependencies
/// - Standard factory method for default configuration
/// - Validation of required dependencies before build
///
/// Failure behavior:
/// - build() throws std::runtime_error when required dependencies are missing.
/// - storage is optional at build time and may be bound later via QueryEngine
///   APIs; index manager is mandatory.
/// 
/// Example usage:
/// ```cpp
/// auto engine = QueryEngineBuilder::standard()
///     .withStorage(my_storage)
///     .withIndexManager(my_index_mgr)
///     .build();
/// ```

#pragma once

#include "query/query_engine.h"
#include "themis/base/interfaces/storage_interface.h"
#include "themis/base/interfaces/index_interface.h"
#include <memory>
#include <stdexcept>
#include <string>

namespace themis {

class QueryEngineBuilder {
public:
    QueryEngineBuilder() = default;
    
    /**
     * @brief With Storage.
     * @param[in] storage Input parameter.
     * @return Return value.
     * @details Implements withStorage without additional internal calls.
     */
    QueryEngineBuilder& withStorage(IStorageEnginePtr storage) {
        storage_ = storage;
        return *this;
    }
    
    /**
     * @brief With Index Manager.
     * @param[in] index_manager Input parameter.
     * @return Return value.
     * @details Implements withIndexManager without additional internal calls.
     */
    QueryEngineBuilder& withIndexManager(IIndexManagerPtr index_manager) {
        index_manager_ = index_manager;
        return *this;
    }
    
    /**
     * @brief Build.
     * @return Return value.
     * @throws std::runtime_error if an error occurs.
     * @details Implements build without additional internal calls.
     */
    std::shared_ptr<query::QueryEngine> build() {
        if (!index_manager_) {
            throw std::runtime_error("QueryEngineBuilder: IndexManager is required");
        }
        
        // Note: storage_ can be nullptr for late binding via setStorage()
        return std::make_shared<query::QueryEngine>(storage_, index_manager_);
    }
    
    /**
     * @brief Standard.
     * @return Return value.
     * @details Implements standard without additional internal calls.
     */
    static QueryEngineBuilder standard() {
        QueryEngineBuilder builder = {};
        return builder;
    }
    
private:
    IStorageEnginePtr storage_;
    IIndexManagerPtr index_manager_;
};

} // namespace themis
