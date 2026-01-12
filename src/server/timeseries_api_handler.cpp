#include "server/timeseries_api_handler.h"
#include "storage/rocksdb_wrapper.h"
#include "timeseries/tsstore.h"
#include "timeseries/continuous_agg.h"
#include "server/auth_middleware.h"
#include "utils/logger.h"
#include "utils/tracing.h"

namespace themis {
namespace server {

TimeSeriesApiHandler::TimeSeriesApiHandler(
    std::shared_ptr<RocksDBWrapper> storage,
    std::shared_ptr<TSStore> ts_store,
    std::shared_ptr<ContinuousAggregateManager> agg_manager,
    std::shared_ptr<AuthMiddleware> auth
)
    : storage_(std::move(storage))
    , ts_store_(std::move(ts_store))
    , agg_manager_(std::move(agg_manager))
    , auth_(std::move(auth))
{
}

http::response<http::string_body> TimeSeriesApiHandler::handlePut(
    const http::request<http::string_body>& req
) {
    // TODO: Implementation to be moved from http_server.cpp handleTimeSeriesPut()
    return makeErrorResponse(http::status::not_implemented, "Not yet implemented", req);
}

http::response<http::string_body> TimeSeriesApiHandler::handleQuery(
    const http::request<http::string_body>& req
) {
    // TODO: Implementation to be moved from http_server.cpp handleTimeSeriesQuery()
    return makeErrorResponse(http::status::not_implemented, "Not yet implemented", req);
}

http::response<http::string_body> TimeSeriesApiHandler::handleAggregate(
    const http::request<http::string_body>& req
) {
    // TODO: Implementation to be moved from http_server.cpp handleTimeSeriesAggregate()
    return makeErrorResponse(http::status::not_implemented, "Not yet implemented", req);
}

http::response<http::string_body> TimeSeriesApiHandler::handleConfigGet(
    const http::request<http::string_body>& req
) {
    // TODO: Implementation to be moved from http_server.cpp handleTimeSeriesConfigGet()
    return makeErrorResponse(http::status::not_implemented, "Not yet implemented", req);
}

http::response<http::string_body> TimeSeriesApiHandler::handleConfigPut(
    const http::request<http::string_body>& req
) {
    // TODO: Implementation to be moved from http_server.cpp handleTimeSeriesConfigPut()
    return makeErrorResponse(http::status::not_implemented, "Not yet implemented", req);
}

http::response<http::string_body> TimeSeriesApiHandler::handleAggregatesGet(
    const http::request<http::string_body>& req
) {
    // TODO: Implementation to be moved from http_server.cpp handleTimeSeriesAggregatesGet()
    return makeErrorResponse(http::status::not_implemented, "Not yet implemented", req);
}

http::response<http::string_body> TimeSeriesApiHandler::handleRetentionGet(
    const http::request<http::string_body>& req
) {
    // TODO: Implementation to be moved from http_server.cpp handleTimeSeriesRetentionGet()
    return makeErrorResponse(http::status::not_implemented, "Not yet implemented", req);
}

http::response<http::string_body> TimeSeriesApiHandler::makeErrorResponse(
    http::status status, const std::string& message, const http::request<http::string_body>& req
) {
    // TODO: Helper implementation
    http::response<http::string_body> res{status, req.version()};
    res.set(http::field::content_type, "application/json");
    nlohmann::json body = {{"error", message}};
    res.body() = body.dump();
    res.prepare_payload();
    return res;
}

http::response<http::string_body> TimeSeriesApiHandler::makeResponse(
    http::status status, const std::string& body, const http::request<http::string_body>& req
) {
    // TODO: Helper implementation
    http::response<http::string_body> res{status, req.version()};
    res.set(http::field::content_type, "application/json");
    res.body() = body;
    res.prepare_payload();
    return res;
}

} // namespace server
} // namespace themis
