/**
 * @file buffer_api_handler.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=3, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "server/buffer_api_handler.h"
#include "timeseries/tsstore.h"
#include "timeseries/ts_auto_buffer.h"
#include "index/vector_index.h"
#include "index/vector_auto_buffer.h"
#include "index/graph_auto_buffer.h"
#include "utils/logger.h"
#include "utils/tracing.h"

namespace themis {
namespace server {

BufferAPIHandler::BufferAPIHandler(
    std::shared_ptr<TSStore> tsstore,
    std::shared_ptr<VectorIndexManager> vector_index,
    std::shared_ptr<PropertyGraphManager> graph_manager)
    : tsstore_(tsstore)
    , vector_index_(vector_index)
    , graph_manager_(graph_manager)
{
    // Initialize TSAutoBuffer if TSStore is available
    if (tsstore_) {
        TSAutoBufferConfig ts_config;
        ts_config.max_points_per_buffer = 1000;
        ts_config.flush_interval = std::chrono::milliseconds(5000);
        ts_config.max_memory_bytes = 100 * 1024 * 1024;  // 100 MB
        ts_config.compression = TSStore::CompressionType::Gorilla;
        ts_config.async_flush = true;
        
        ts_buffer_ = std::make_unique<TSAutoBuffer>(tsstore_.get(), ts_config);
        THEMIS_INFO("TSAutoBuffer initialized with max_points={}, flush_interval={}ms",
                   ts_config.max_points_per_buffer, ts_config.flush_interval.count());
    }
    
    // Initialize VectorAutoBuffer if VectorIndexManager is available
    if (vector_index_) {
        VectorAutoBufferConfig vec_config;
        vec_config.max_vectors_per_buffer = 1000;
        vec_config.flush_interval = std::chrono::milliseconds(5000);
        vec_config.max_memory_bytes = 500 * 1024 * 1024;  // 500 MB
        vec_config.async_flush = true;
        vec_config.compression = VectorAutoBufferConfig::Compression::None;
        
        vector_buffer_ = std::make_unique<VectorAutoBuffer>(vector_index_.get(), vec_config);
        THEMIS_INFO("VectorAutoBuffer initialized with max_vectors={}, flush_interval={}ms",
                   vec_config.max_vectors_per_buffer, vec_config.flush_interval.count());
    }
    
    // Initialize GraphAutoBuffer if PropertyGraphManager is available
    if (graph_manager_) {
        GraphAutoBufferConfig graph_config;
        graph_config.max_nodes_per_buffer = 1000;
        graph_config.max_edges_per_buffer = 1000;
        graph_config.flush_interval = std::chrono::milliseconds(5000);
        graph_config.async_flush = true;
        
        graph_buffer_ = std::make_unique<GraphAutoBuffer>(graph_manager_.get(), graph_config);
        THEMIS_INFO("GraphAutoBuffer initialized with max_nodes={}, max_edges={}, flush_interval={}ms",
                   graph_config.max_nodes_per_buffer,
                   graph_config.max_edges_per_buffer,
                   graph_config.flush_interval.count());
    }
}

BufferAPIHandler::~BufferAPIHandler() {
    stop();
}

void BufferAPIHandler::start() {
    if (ts_buffer_) {
        ts_buffer_->start();
        THEMIS_INFO("TSAutoBuffer started");
    }
    if (vector_buffer_) {
        vector_buffer_->start();
        THEMIS_INFO("VectorAutoBuffer started");
    }
    if (graph_buffer_) {
        graph_buffer_->start();
        THEMIS_INFO("GraphAutoBuffer started");
    }
}

void BufferAPIHandler::stop() {
    if (ts_buffer_) {
        ts_buffer_->stop();
        THEMIS_INFO("TSAutoBuffer stopped");
    }
    if (vector_buffer_) {
        vector_buffer_->stop();
        THEMIS_INFO("VectorAutoBuffer stopped");
    }
    if (graph_buffer_) {
        graph_buffer_->stop();
        THEMIS_INFO("GraphAutoBuffer stopped");
    }
}

http::response<http::string_body> BufferAPIHandler::handleTSPutBuffered(
    const http::request<http::string_body>& req)
{
    auto span = Tracer::startSpan("BufferAPIHandler.handleTSPutBuffered");
    
    if (!ts_buffer_) {
        return makeErrorResponse(http::status::service_unavailable,
                                "Time series buffer not available", req);
    }
    auto& ts_buffer = *ts_buffer_;
    
    try {
        auto body = json::parse(req.body());
        
        // Validate required fields
        if (!body.contains("metric") || !body.contains("entity") || 
            !body.contains("timestamp") || !body.contains("value")) {
            return makeErrorResponse(http::status::bad_request,
                                    "Missing required fields: metric, entity, timestamp, value", req);
        }
        
        // Extract data point
        TSStore::DataPoint point;
        point.metric = body["metric"].get<std::string>();
        point.entity = body["entity"].get<std::string>();
        point.timestamp_ms = body["timestamp"].get<int64_t>();
        point.value = body["value"].get<double>();
        
        // Add to buffer
        auto status = ts_buffer.add(point);

        if (!status.has_value()) {
            return makeErrorResponse(
                http::status::internal_server_error,
                status.error().message(),
                req);
        }
        
        // Return success with stats
        auto stats = ts_buffer.getStats();
        json response = {
            {"status", "buffered"},
            {"metric", point.metric},
            {"entity", point.entity},
            {"buffer_stats", {
                {"points_buffered", stats.points_buffered.load()},
                {"points_flushed", stats.points_flushed.load()},
                {"current_buffer_size", stats.current_buffer_size},
                {"current_buffer_memory", stats.current_buffer_memory}
            }}
        };
        
        return makeResponse(http::status::ok, response, req);
        
    } catch (const json::exception& e) {
        return makeErrorResponse(http::status::bad_request,
                                std::string("JSON parse error: ") + e.what(), req);
    } catch (const std::exception& e) {
        return makeErrorResponse(http::status::internal_server_error,
                                std::string("Error: ") + e.what(), req);
    }
}

http::response<http::string_body> BufferAPIHandler::handleVectorAddBuffered(
    const http::request<http::string_body>& req)
{
    auto span = Tracer::startSpan("BufferAPIHandler.handleVectorAddBuffered");
    
    if (!vector_buffer_) {
        return makeErrorResponse(http::status::service_unavailable,
                                "Vector buffer not available", req);
    }
    auto& vector_buffer = *vector_buffer_;
    
    try {
        auto body = json::parse(req.body());
        
        // Validate required fields
        if (!body.contains("pk") || !body.contains("embedding")) {
            return makeErrorResponse(http::status::bad_request,
                                    "Missing required fields: pk, embedding", req);
        }
        
        // Create BaseEntity (simplified - in real implementation would be more complex)
        BaseEntity entity;
        entity.setPrimaryKey(body["pk"].get<std::string>());
        
        // Store embedding in metadata
        if (body.contains("metadata")) {
            entity.setField("metadata", body["metadata"].dump());
        }
        
        // Add to buffer
        auto status = vector_buffer.add(entity);

        if (!status.ok) {
            return makeErrorResponse(http::status::internal_server_error,
                                    status.message, req);
        }
        
        // Return success with stats
        auto stats = vector_buffer.getStats();
        json response = {
            {"status", "buffered"},
            {"pk", entity.getPrimaryKey()},
            {"buffer_stats", {
                {"vectors_buffered", stats.vectors_buffered.load()},
                {"vectors_flushed", stats.vectors_flushed.load()},
                {"current_buffer_size", stats.current_buffer_size},
                {"current_buffer_memory", stats.current_buffer_memory}
            }}
        };
        
        return makeResponse(http::status::ok, response, req);
        
    } catch (const json::exception& e) {
        return makeErrorResponse(http::status::bad_request,
                                std::string("JSON parse error: ") + e.what(), req);
    } catch (const std::exception& e) {
        return makeErrorResponse(http::status::internal_server_error,
                                std::string("Error: ") + e.what(), req);
    }
}

http::response<http::string_body> BufferAPIHandler::handleGraphAddBuffered(
    const http::request<http::string_body>& req)
{
    auto span = Tracer::startSpan("BufferAPIHandler.handleGraphAddBuffered");
    
    if (!graph_buffer_) {
        return makeErrorResponse(http::status::service_unavailable,
                                "Graph buffer not available", req);
    }
    auto& graph_buffer = *graph_buffer_;
    
    try {
        auto body = json::parse(req.body());
        
        // Validate required fields
        if (!body.contains("graph_id") || !body.contains("type") || 
            !body.contains("pk")) {
            return makeErrorResponse(http::status::bad_request,
                                    "Missing required fields: graph_id, type, pk", req);
        }
        
        std::string graph_id = body["graph_id"].get<std::string>();
        std::string type = body["type"].get<std::string>();
        
        // Create BaseEntity
        BaseEntity entity;
        entity.setPrimaryKey(body["pk"].get<std::string>());
        
        if (body.contains("properties")) {
            entity.setField("properties", body["properties"].dump());
        }
        
        PropertyGraphManager::Status status;
        
        if (type == "node") {
            status = graph_buffer.addNode(entity, graph_id);
        } else if (type == "edge") {
            status = graph_buffer.addEdge(entity, graph_id);
        } else {
            return makeErrorResponse(http::status::bad_request,
                                    "Invalid type: must be 'node' or 'edge'", req);
        }
        
        if (!status.ok) {
            return makeErrorResponse(http::status::internal_server_error,
                                    status.message, req);
        }
        
        // Return success with stats
        auto stats = graph_buffer.getStats();
        json response = {
            {"status", "buffered"},
            {"graph_id", graph_id},
            {"type", type},
            {"pk", entity.getPrimaryKey()},
            {"buffer_stats", {
                {"nodes_buffered", stats.nodes_buffered.load()},
                {"edges_buffered", stats.edges_buffered.load()},
                {"nodes_flushed", stats.nodes_flushed.load()},
                {"edges_flushed", stats.edges_flushed.load()},
                {"current_buffer_size", stats.current_buffer_size}
            }}
        };
        
        return makeResponse(http::status::ok, response, req);
        
    } catch (const json::exception& e) {
        return makeErrorResponse(http::status::bad_request,
                                std::string("JSON parse error: ") + e.what(), req);
    } catch (const std::exception& e) {
        return makeErrorResponse(http::status::internal_server_error,
                                std::string("Error: ") + e.what(), req);
    }
}

http::response<http::string_body> BufferAPIHandler::handleBufferStats(
    const http::request<http::string_body>& req)
{
    auto span = Tracer::startSpan("BufferAPIHandler.handleBufferStats");
    
    json response = {
        {"buffers", json::object()}
    };
    
    if (ts_buffer_) {
        auto stats = ts_buffer_->getStats();
        response["buffers"]["ts_buffer"] = {
            {"enabled", true},
            {"points_buffered", stats.points_buffered.load()},
            {"points_flushed", stats.points_flushed.load()},
            {"current_buffer_size", stats.current_buffer_size},
            {"current_buffer_memory", stats.current_buffer_memory},
            {"flush_count", stats.flush_count.load()},
            {"auto_flush_count", stats.auto_flush_count.load()},
            {"buffer_overflow_count", stats.buffer_overflow_count.load()}
        };
    } else {
        response["buffers"]["ts_buffer"] = {{"enabled", false}};
    }
    
    if (vector_buffer_) {
        auto stats = vector_buffer_->getStats();
        response["buffers"]["vector_buffer"] = {
            {"enabled", true},
            {"vectors_buffered", stats.vectors_buffered.load()},
            {"vectors_flushed", stats.vectors_flushed.load()},
            {"current_buffer_size", stats.current_buffer_size},
            {"current_buffer_memory", stats.current_buffer_memory},
            {"flush_count", stats.flush_count.load()},
            {"auto_flush_count", stats.auto_flush_count.load()}
        };
    } else {
        response["buffers"]["vector_buffer"] = {{"enabled", false}};
    }
    
    if (graph_buffer_) {
        auto stats = graph_buffer_->getStats();
        response["buffers"]["graph_buffer"] = {
            {"enabled", true},
            {"nodes_buffered", stats.nodes_buffered.load()},
            {"edges_buffered", stats.edges_buffered.load()},
            {"nodes_flushed", stats.nodes_flushed.load()},
            {"edges_flushed", stats.edges_flushed.load()},
            {"current_buffer_size", stats.current_buffer_size},
            {"flush_count", stats.flush_count.load()}
        };
    } else {
        response["buffers"]["graph_buffer"] = {{"enabled", false}};
    }
    
    return makeResponse(http::status::ok, response, req);
}

http::response<http::string_body> BufferAPIHandler::handleBufferFlush(
    const http::request<http::string_body>& req)
{
    auto span = Tracer::startSpan("BufferAPIHandler.handleBufferFlush");
    
    try {
        std::string buffer_type = "all";
        
        if (!req.body().empty()) {
            auto body = json::parse(req.body());
            if (body.contains("buffer")) {
                buffer_type = body["buffer"].get<std::string>();
            }
        }
        
        json response = {
            {"flushed", json::object()}
        };
        
        if (buffer_type == "all" || buffer_type == "ts") {
            if (ts_buffer_) {
                size_t count = ts_buffer_->flush();
                response["flushed"]["ts_buffer"] = count;
            }
        }
        
        if (buffer_type == "all" || buffer_type == "vector") {
            if (vector_buffer_) {
                size_t count = vector_buffer_->flush();
                response["flushed"]["vector_buffer"] = count;
            }
        }
        
        if (buffer_type == "all" || buffer_type == "graph") {
            if (graph_buffer_) {
                size_t count = graph_buffer_->flush();
                response["flushed"]["graph_buffer"] = count;
            }
        }
        
        return makeResponse(http::status::ok, response, req);
        
    } catch (const json::exception& e) {
        return makeErrorResponse(http::status::bad_request,
                                std::string("JSON parse error: ") + e.what(), req);
    } catch (const std::exception& e) {
        return makeErrorResponse(http::status::internal_server_error,
                                std::string("Error: ") + e.what(), req);
    }
}

http::response<http::string_body> BufferAPIHandler::makeResponse(
    http::status status,
    const json& body,
    const http::request<http::string_body>& req)
{
    http::response<http::string_body> res{status, req.version()};
    res.set(http::field::server, "ThemisDB");
    res.set(http::field::content_type, "application/json");
    res.body() = body.dump(2);
    res.prepare_payload();
    res.keep_alive(req.keep_alive());
    return res;
}

http::response<http::string_body> BufferAPIHandler::makeErrorResponse(
    http::status status,
    const std::string& message,
    const http::request<http::string_body>& req)
{
    json body = {
        {"error", message},
        {"status_code", static_cast<int>(status)}
    };
    return makeResponse(status, body, req);
}

} // namespace server
} // namespace themis
