#include "server/graphql_api_handler.h"
#include <nlohmann/json.hpp>

namespace themis {
namespace server {

using json = nlohmann::json;

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

http::response<http::string_body> GraphQLApiHandler::makeResponse(
    http::status status,
    const std::string& body,
    const http::request<http::string_body>& req)
{
    http::response<http::string_body> res{status, req.version()};
    res.set(http::field::server, "THEMIS/0.1.0");
    res.set(http::field::content_type, "application/json");
    res.keep_alive(req.keep_alive());
    res.body() = body;
    res.prepare_payload();
    return res;
}

http::response<http::string_body> GraphQLApiHandler::makeErrorResponse(
    http::status status,
    const std::string& message,
    const http::request<http::string_body>& req)
{
    json error_body = {
        {"errors", json::array({{{"message", message}}})},
        {"data",   nullptr}
    };
    return makeResponse(status, error_body.dump(), req);
}

json GraphQLApiHandler::serializeValue(
    const std::shared_ptr<graphql::Value>& val) const
{
    if (!val || val->isNull())  return json(nullptr);
    if (val->isBool())          return json(val->asBool());
    if (val->isInt())           return json(val->asInt());
    if (val->isFloat())         return json(val->asFloat());
    if (val->isString())        return json(val->asString());
    if (val->isEnum())          return json(val->asString());
    if (val->isList()) {
        json arr = json::array();
        for (const auto& item : val->asList())
            arr.push_back(serializeValue(item));
        return arr;
    }
    if (val->isObject()) {
        json obj = json::object();
        for (const auto& [k, v] : val->asObject())
            obj[k] = serializeValue(v);
        return obj;
    }
    return json(nullptr);
}

// ---------------------------------------------------------------------------
// Public handlers
// ---------------------------------------------------------------------------

/**
 * POST /graphql (or POST /api/v1/graphql)
 *
 * Request body (JSON):
 *   { "query": "...", "variables": {}, "operationName": "..." }
 *
 * Response body (JSON):
 *   { "data": {...}, "errors": [...] }
 */
http::response<http::string_body> GraphQLApiHandler::handlePost(
    const http::request<http::string_body>& req)
{
    try {
        // Parse the request body as a JSON object.
        json body_json = json::object();
        if (!req.body().empty()) {
            body_json = json::parse(req.body());
        }

        // The "query" field is mandatory.
        if (!body_json.contains("query") || !body_json["query"].is_string()) {
            return makeErrorResponse(
                http::status::bad_request,
                "GraphQL request must contain a 'query' field",
                req);
        }
        const std::string gql_query = body_json["query"].get<std::string>();

        // Optional: operationName
        std::string op_name;
        if (body_json.contains("operationName") &&
            body_json["operationName"].is_string()) {
            op_name = body_json["operationName"].get<std::string>();
        }

        // Build the execution context and populate variables.
        graphql::ExecutionContext ctx;
        if (body_json.contains("variables") &&
            body_json["variables"].is_object()) {
            for (auto& [k, v] : body_json["variables"].items()) {
                if (v.is_string())
                    ctx.variables[k] = graphql::Value::string(v.get<std::string>());
                else if (v.is_number_integer())
                    ctx.variables[k] = graphql::Value::integer(v.get<int64_t>());
                else if (v.is_number_float())
                    ctx.variables[k] = graphql::Value::floating(v.get<double>());
                else if (v.is_boolean())
                    ctx.variables[k] = graphql::Value::boolean(v.get<bool>());
                else
                    ctx.variables[k] = graphql::Value::null();
            }
        }

        // Parse the GraphQL query.
        auto parse_result = graphql::Parser::parse(gql_query);
        if (!parse_result.success) {
            json errors_array = json::array();
            for (const auto& pe : parse_result.errors)
                errors_array.push_back({{"message", pe.toString()}});
            json err_body = {{"errors", errors_array}, {"data", nullptr}};
            return makeResponse(http::status::bad_request, err_body.dump(), req);
        }

        // Execute the operation.
        graphql::Executor executor;
        auto exec_result = executor.execute(parse_result.document, ctx, op_name);

        // Build the response envelope.
        json result_json = json::object();
        result_json["data"] = exec_result.data
            ? serializeValue(exec_result.data)
            : json(nullptr);

        if (exec_result.hasErrors()) {
            json errors_array = json::array();
            for (const auto& me : exec_result.errors) {
                errors_array.push_back({
                    {"message",    me.message},
                    {"extensions", {{"code", me.code}}}
                });
            }
            result_json["errors"] = errors_array;
        }

        return makeResponse(http::status::ok, result_json.dump(), req);

    } catch (const json::exception& e) {
        return makeErrorResponse(
            http::status::bad_request,
            std::string("Invalid JSON in GraphQL request: ") + e.what(),
            req);
    }
}

/**
 * GET /graphql/schema (or GET /api/v1/graphql/schema)
 *
 * Returns the Schema Definition Language (SDL) document for introspection
 * and client-side tooling.
 */
http::response<http::string_body> GraphQLApiHandler::handleSchemaGet(
    const http::request<http::string_body>& req)
{
    auto schema  = graphql::ThemisSchemaBuilder::build();
    std::string sdl = schema.toSDL();

    http::response<http::string_body> res{http::status::ok, req.version()};
    res.set(http::field::server, "THEMIS/0.1.0");
    res.set(http::field::content_type, "text/plain; charset=utf-8");
    res.keep_alive(req.keep_alive());
    res.body() = sdl;
    res.prepare_payload();
    return res;
}

} // namespace server
} // namespace themis
