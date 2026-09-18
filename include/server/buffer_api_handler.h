/**
 * @file buffer_api_handler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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

class BufferAPIHandler {
public:
    BufferAPIHandler(
        std::shared_ptr<TSStore> tsstore = nullptr,
        std::shared_ptr<VectorIndexManager> vector_index = nullptr,
        std::shared_ptr<PropertyGraphManager> graph_manager = nullptr
    );
    
    ~BufferAPIHandler();
    
    /**
     * @brief Start.
     */
    void start();
    
    /**
     * @brief Stop.
     */
    void stop();
    
    /**
     * @brief Handle TSPut Buffered.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleTSPutBuffered(
        const http::request<http::string_body>& req);
    
    /**
     * @brief Handle Vector Add Buffered.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleVectorAddBuffered(
        const http::request<http::string_body>& req);
    
    /**
     * @brief Handle Graph Add Buffered.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleGraphAddBuffered(
        const http::request<http::string_body>& req);
    
    /**
     * @brief Handle Buffer Stats.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> handleBufferStats(
        const http::request<http::string_body>& req);
    
    /**
     * @brief Handle Buffer Flush.
     * @param[in] req Input parameter.
     * @return Return value.
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
    /**
     * @brief Make Response.
     * @param[in] status Input parameter.
     * @param[in] body Input parameter.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> makeResponse(
        http::status status,
        const json& body,
        const http::request<http::string_body>& req);
    
    /**
     * @brief Make Error Response.
     * @param[in] status Input parameter.
     * @param[in] message Input parameter.
     * @param[in] req Input parameter.
     * @return Return value.
     */
    http::response<http::string_body> makeErrorResponse(
        http::status status,
        const std::string& message,
        const http::request<http::string_body>& req);
};

} // namespace server
} // namespace themis
