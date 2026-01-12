#pragma once

#include <memory>
#include <string>
#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>

namespace themis {

// Forward declarations
class RocksDBWrapper;

namespace sharding {
class WALApplier;
class WALManager;
class ReplicationCoordinator;
}

namespace server {

namespace beast = boost::beast;
namespace http = beast::http;

class AuthMiddleware;

/**
 * @brief Handler for Write-Ahead Log (WAL) Operations
 * 
 * This handler manages all WAL-related endpoints:
 * - POST /api/v1/wal/apply - Apply WAL entries for replication
 * 
 * Features:
 * - WAL entry application
 * - Replication support
 * - Conflict resolution
 * - Transaction replay
 * 
 * Extracted from http_server.cpp (~220 lines) to improve maintainability.
 */
class WALApiHandler {
public:
    /**
     * @brief Construct a new WAL API Handler
     * 
     * @param storage Storage backend
     * @param wal_applier WAL applier for entry processing
     * @param wal_manager WAL manager
     * @param replication_coordinator Replication coordinator
     * @param auth Authentication/authorization middleware
     */
    WALApiHandler(
        std::shared_ptr<RocksDBWrapper> storage,
        std::shared_ptr<sharding::WALApplier> wal_applier,
        std::shared_ptr<sharding::WALManager> wal_manager,
        std::shared_ptr<sharding::ReplicationCoordinator> replication_coordinator,
        std::shared_ptr<AuthMiddleware> auth
    );

    /**
     * @brief Handle POST /api/v1/wal/apply request
     * @param req HTTP request with WAL entries to apply
     * @return HTTP response with application status
     */
    http::response<http::string_body> handleApply(const http::request<http::string_body>& req);

private:
    std::shared_ptr<RocksDBWrapper> storage_;
    std::shared_ptr<sharding::WALApplier> wal_applier_;
    std::shared_ptr<sharding::WALManager> wal_manager_;
    std::shared_ptr<sharding::ReplicationCoordinator> replication_coordinator_;
    std::shared_ptr<AuthMiddleware> auth_;

    // Helper methods (to be implemented)
    http::response<http::string_body> makeErrorResponse(
        http::status status, const std::string& message, const http::request<http::string_body>& req);
    http::response<http::string_body> makeResponse(
        http::status status, const std::string& body, const http::request<http::string_body>& req);
};

} // namespace server
} // namespace themis
