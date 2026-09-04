/**
 * @file graphql_api_handler.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.18
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=4, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "server/graphql_api_handler.h"
#include "api/graphql_aql_resolver.h"
#include "utils/tracing.h"
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
    if (!val || val->isNull()) {
      return json(nullptr);
    }
    if (val->isBool()) {
      return json(val->asBool());
    }
    if (val->isInt()) {
      return json(val->asInt());
    }
    if (val->isFloat()) {
      return json(val->asFloat());
    }
    if (val->isString()) {
      return json(val->asString());
    }
    if (val->isEnum()) {
      return json(val->asString());
    }
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
    auto span = Tracer::startSpan("POST /graphql");
    try {
        // ── Parse request body ─────────────────────────────────────────────
        json body_json = json::object();
        if (!req.body().empty()) {
            body_json = json::parse(req.body());
        }

        if (!body_json.contains("query") || !body_json["query"].is_string()) {
            span.setStatus(false, "Missing query field");
            return makeErrorResponse(
                http::status::bad_request,
                "GraphQL request must contain a 'query' field",
                req);
        }
        const std::string gql_query = body_json["query"].get<std::string>();

        std::string op_name;
        if (body_json.contains("operationName") &&
            body_json["operationName"].is_string()) {
            op_name = body_json["operationName"].get<std::string>();
        }

        // ── Parse GraphQL document ─────────────────────────────────────────
        auto parse_result = graphql::Parser::parse(gql_query);
        if (!parse_result.success) {
            json errors_array = json::array();
            for (const auto& pe : parse_result.errors)
                errors_array.push_back({{"message", pe.toString()}});
            json err_body = {{"errors", errors_array}, {"data", nullptr}};
            span.setStatus(false, "Parse error");
            return makeResponse(http::status::bad_request, err_body.dump(), req);
        }

        // ── Complexity check (cost model enforcement) ──────────────────────
        //
        // The GraphQL complexity score is computed before execution.  A high
        // score tightens the AQL resource limits injected into each resolver;
        // a score above kGraphQLMaxComplexity is rejected immediately (HTTP 400).
        const uint32_t complexity =
            graphql::GraphQLComplexityEstimator::estimate(
                std::make_shared<graphql::Document>(parse_result.document));
        span.setAttribute("graphql.complexity", static_cast<int64_t>(complexity));

        if (complexity > graphql::kGraphQLMaxComplexity) {
            const std::string msg = graphql::makeComplexityErrorMessage(
                complexity, graphql::kGraphQLMaxComplexity);
            span.setStatus(false, msg);
            json err_body = {
                {"errors", json::array({{{"message", msg},
                                         {"extensions", {{"code", "COMPLEXITY_EXCEEDED"}}}}})},
                {"data", nullptr}
            };
            return makeResponse(http::status::bad_request, err_body.dump(), req);
        }

        // ── Build execution context with variables ─────────────────────────
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

        // ── Inject AQL + versioning resolvers ─────────────────────────────
        //
        // GraphQLAqlResolverFactory wires:
        //   aql(query)          → executeAqlWithLimits (read queries)
        //   aqlMutation(query)  → executeAqlWithLimits (DML statements)
        //   apiVersion          → static version string
        //   schemaVersion       → static schema version string
        //
        // The limits passed to executeAqlWithLimits are derived from the
        // complexity score above so that expensive GraphQL documents
        // automatically receive tighter AQL budgets.
        graphql::GraphQLAqlResolverFactory::injectResolvers(
            ctx, parse_result.document, engine_);

        // ── Execute ────────────────────────────────────────────────────────
        graphql::Executor executor;
        auto exec_result = executor.execute(parse_result.document, ctx, op_name);

        // ── Serialize response ─────────────────────────────────────────────
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

        span.setAttribute("graphql.errors",
                          static_cast<int64_t>(exec_result.errors.size()));
        span.setStatus(!exec_result.hasErrors());
        return makeResponse(http::status::ok, result_json.dump(), req);

    } catch (const std::runtime_error& e) {
        // Catches complexity errors thrown by GraphQLComplexityEstimator
        // and AQL execution errors surfaced through resolvers.
        json err_body = {
            {"errors", json::array({{{"message", std::string(e.what())},
                                     {"extensions", {{"code", "EXECUTION_ERROR"}}}}})},
            {"data", nullptr}
        };
        span.setStatus(false, e.what());
        return makeResponse(http::status::bad_request, err_body.dump(), req);
    } catch (const json::exception& e) {
        span.setStatus(false, e.what());
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
    auto span = Tracer::startSpan("GET /graphql/schema");
    auto schema  = graphql::ThemisSchemaBuilder::build();
    std::string sdl = schema.toSDL();
    span.setAttribute("graphql.schema.size_bytes", static_cast<int64_t>(sdl.size()));
    span.setStatus(true);

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
