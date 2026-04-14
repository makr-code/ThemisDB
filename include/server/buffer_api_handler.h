/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            buffer_api_handler.h                               ║
  Version:         0.0.40                                             ║
  Last Modified:   2026-04-14 06:56:05                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     166                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <boost/beast.hpp>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include "timeseries/ts_auto_buffer.h"
#include "index/vector_auto_buffer.h"
#include "index/graph_auto_buffer.h"

namespace themis {

// Forward declarations
class TSStore;
class VectorIndexManager;
class PropertyGraphManager;

namespace server {

namespace beast = boost::beast;
namespace http = beast::http;
using json = nlohmann::json;

/**
 * @brief HTTP API Handler for AutoBuffer operations
 * 
 * Provides buffered endpoints for high-throughput ingestion:
 * - POST /ts/put/buffered - Buffered time series data points
 * - POST /vectors/add/buffered - Buffered vector index operations
 * - POST /graph/add/buffered - Buffered graph node/edge operations
 * - GET /buffer/stats - Get buffer statistics
 * - POST /buffer/flush - Manual flush of all buffers
 */
class BufferAPIHandler {
public:
    /**
     * @brief Constructor
     * @param tsstore Time series store (optional)
     * @param vector_index Vector index manager (optional)
     * @param graph_manager Property graph manager (optional)
     */
    BufferAPIHandler(
        std::shared_ptr<TSStore> tsstore = nullptr,
        std::shared_ptr<VectorIndexManager> vector_index = nullptr,
        std::shared_ptr<PropertyGraphManager> graph_manager = nullptr
    );
    
    ~BufferAPIHandler();
    
    /**
     * @brief Start all auto-buffers
     */
    void start();
    
    /**
     * @brief Stop all auto-buffers (flushes remaining data)
     */
    void stop();
    
    /**
     * @brief Handle buffered time series put
     * POST /ts/put/buffered
     * Body: {
     *   "metric": "string",
     *   "entity": "string",
     *   "timestamp": number,
     *   "value": number
     * }
     */
    http::response<http::string_body> handleTSPutBuffered(
        const http::request<http::string_body>& req);
    
    /**
     * @brief Handle buffered vector add
     * POST /vectors/add/buffered
     * Body: {
     *   "pk": "string",
     *   "embedding": [float...],
     *   "metadata": {...}
     * }
     */
    http::response<http::string_body> handleVectorAddBuffered(
        const http::request<http::string_body>& req);
    
    /**
     * @brief Handle buffered graph node/edge add
     * POST /graph/add/buffered
     * Body: {
     *   "graph_id": "string",
     *   "type": "node" | "edge",
     *   "pk": "string",
     *   "properties": {...}
     * }
     */
    http::response<http::string_body> handleGraphAddBuffered(
        const http::request<http::string_body>& req);
    
    /**
     * @brief Get buffer statistics
     * GET /buffer/stats
     * Response: {
     *   "ts_buffer": {...},
     *   "vector_buffer": {...},
     *   "graph_buffer": {...}
     * }
     */
    http::response<http::string_body> handleBufferStats(
        const http::request<http::string_body>& req);
    
    /**
     * @brief Manually flush all buffers
     * POST /buffer/flush
     * Body: {
     *   "buffer": "all" | "ts" | "vector" | "graph"
     * }
     */
    http::response<http::string_body> handleBufferFlush(
        const http::request<http::string_body>& req);

private:
    // Component references
    std::shared_ptr<TSStore> tsstore_;
    std::shared_ptr<VectorIndexManager> vector_index_;
    std::shared_ptr<PropertyGraphManager> graph_manager_;
    
    // AutoBuffer instances
    std::unique_ptr<TSAutoBuffer> ts_buffer_;
    std::unique_ptr<VectorAutoBuffer> vector_buffer_;
    std::unique_ptr<GraphAutoBuffer> graph_buffer_;
    
    // Helper methods
    http::response<http::string_body> makeResponse(
        http::status status,
        const json& body,
        const http::request<http::string_body>& req);
    
    http::response<http::string_body> makeErrorResponse(
        http::status status,
        const std::string& message,
        const http::request<http::string_body>& req);
};

} // namespace server
} // namespace themis
