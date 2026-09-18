/**
 * @file graph_api_handler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once
#include "server/auth_middleware.h"
#include "graph/graph_query_optimizer.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <boost/beast/http.hpp>

namespace beast = boost::beast;
namespace http = beast::http;
#include <nlohmann/json.hpp>

namespace themis {

// Forward declarations
class RocksDBWrapper;
class GraphIndexManager;

namespace server {

class GraphApiHandler {
public:
    GraphApiHandler(
        std::shared_ptr<RocksDBWrapper> storage,
        std::shared_ptr<GraphIndexManager> graph_index,
        std::shared_ptr<themis::AuthMiddleware> auth
    );

    /**
     * @brief Handle Traverse.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleTraverse(const http::request<http::string_body>& req);

    /**
     * @brief Handle Edge Create.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleEdgeCreate(const http::request<http::string_body>& req);

    /**
     * @brief Handle Edge Delete.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleEdgeDelete(const http::request<http::string_body>& req);

    /**
     * @brief Handle Metrics.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleMetrics(const http::request<http::string_body>& req);

    /**
     * @brief Handle Metrics Prometheus.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleMetricsPrometheus(const http::request<http::string_body>& req);

    /**
     * @brief Handle Incremental Query Register.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleIncrementalQueryRegister(const http::request<http::string_body>& req);

    /**
     * @brief Handle Incremental Query Unregister.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleIncrementalQueryUnregister(const http::request<http::string_body>& req);

    /**
     * @brief Handle Graph Changes.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleGraphChanges(const http::request<http::string_body>& req);

    /**
     * @brief Handle Cost Model Calibrate.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleCostModelCalibrate(const http::request<http::string_body>& req);

    /**
     * @brief Handle Cost Model Export.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleCostModelExport(const http::request<http::string_body>& req);

    /**
     * @brief Handle Cost Model Import.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleCostModelImport(const http::request<http::string_body>& req);

    /**
     * @brief Handle Query Explain.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleQueryExplain(const http::request<http::string_body>& req);

private:
    std::shared_ptr<RocksDBWrapper> storage_;
    std::shared_ptr<GraphIndexManager> graph_index_;
    std::shared_ptr<themis::AuthMiddleware> auth_;
    std::unique_ptr<themis::graph::GraphQueryOptimizer> optimizer_;

    std::unordered_map<
        themis::graph::GraphQueryOptimizer::IncrementalQueryHandle,
        themis::graph::GraphQueryOptimizer::IncrementalQueryResult
    > incremental_results_;

    static themis::graph::GraphQueryOptimizer::GraphChangeSet
    parseChangeSet(const nlohmann::json& changes_array);

    // Helper methods
    /**
     * @brief Extract Path Param.
     * @param[in] target Input parameter.
     * @param[in] prefix Input parameter.
     * @return Return value.
     */
    std::string extractPathParam(const std::string& target, const std::string& prefix);
    /**
     * @brief Make Error Response.
     * @param[in] status Input parameter.
     * @param[in] message Input parameter.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> makeErrorResponse(
        http::status status, const std::string& message, const http::request<http::string_body>& req);
    /**
     * @brief Make Response.
     * @param[in] status Input parameter.
     * @param[in] body Input parameter.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> makeResponse(
        http::status status, const std::string& body, const http::request<http::string_body>& req);
};

} // namespace server
} // namespace themis
