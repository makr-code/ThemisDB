/**
 * @file ethics_api_handler.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=7; TODO=1, Stub=5, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=6, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "server/ethics_api_handler.h"
#include "storage/rocksdb_wrapper.h"
#include "storage/base_entity.h"
#include "query/query_engine.h"
#include "query/aql_parser.h"
#include "query/aql_translator.h"
#include "server/auth_middleware.h"
#include "utils/logger.h"
#include "utils/tracing.h"
#include <sstream>
#include <regex>
#include <functional>

namespace themis {
namespace server {

EthicsApiHandler::EthicsApiHandler(
    std::shared_ptr<RocksDBWrapper> storage,
    std::shared_ptr<QueryEngine> query_engine,
    std::shared_ptr<themis::AuthMiddleware> auth
)
    : storage_(std::move(storage))
    , query_engine_(std::move(query_engine))
    , auth_(std::move(auth))
{
}

http::response<http::string_body> EthicsApiHandler::handle(
    const http::request<http::string_body>& req,
    const std::string& target
) {
    std::string path = target;
    const auto qpos = path.find('?');
    if (qpos != std::string::npos) {
        path = path.substr(0, qpos);
    }

    // Normalize optional /api prefix so routing only needs /ethics/... paths.
    if (path.rfind("/api/ethics/", 0) == 0) {
        path = path.substr(4);  // "/api/ethics/..." -> "/ethics/..."
    }

    const auto method = req.method();
    if (method == http::verb::post && path == "/ethics/debate/init") {
        return handleDebateInit(req);
    }
    if (method == http::verb::post && path == "/ethics/decision/make") {
        return handleMakeDecision(req);
    }
    if (method == http::verb::post && path == "/ethics/evaluation") {
        return handleEvaluation(req);
    }
    if (method == http::verb::get && path == "/ethics/arguments") {
        return handleGetArguments(req);
    }
    if (method == http::verb::post && path == "/ethics/arguments/search") {
        return handleSearchArguments(req);
    }
    if (method == http::verb::get && path == "/ethics/philosophies") {
        return handleListPhilosophies(req);
    }
    if (method == http::verb::get && path.rfind("/ethics/philosophies/", 0) == 0) {
        return handleGetPhilosophy(req, path.substr(std::string("/ethics/philosophies/").size()));
    }
    if (method == http::verb::post && path == "/ethics/rag/context") {
        return handleBuildContext(req);
    }
    if (method == http::verb::get && path == "/ethics/metrics") {
        return handleGetMetrics(req);
    }

    return makeErrorResponse(http::status::not_found, "Ethics endpoint not found", req);
}

http::response<http::string_body> EthicsApiHandler::handleDebateInit(
    const http::request<http::string_body>& req
) {
    auto span = Tracer::startSpan("handleEthicsDebateInit");
    
    try {
        nlohmann::json body = nlohmann::json::parse(req.body());
        
        // Validate required fields
        if (!body.contains("dilemma_description") || !body.contains("philosophy_schools")) {
            span.setStatus(false, "invalid_request");
            return makeErrorResponse(http::status::bad_request, 
                "Missing required fields: dilemma_description, philosophy_schools", req);
        }
        
        // Extract parameters
        std::string dilemma = body["dilemma_description"];
        auto philosophies = body["philosophy_schools"].get<std::vector<std::string>>();
        std::string category = body.value("category", "general");
        
        // Execute AQL function via QueryEngine
        std::string aql = "RETURN ETHICS_INITIALIZE_DEBATE(@dilemma, @philosophies, @category)";
        nlohmann::json bind_vars = {
            {"dilemma", dilemma},
            {"philosophies", philosophies},
            {"category", category}
        };
        
        auto result = executeAQL(aql, bind_vars);
        
        span.setStatus(true);
        return makeResponse(http::status::created, result.dump(), req);
        
    } catch (const nlohmann::json::exception& e) {
        span.setStatus(false, "json_error");
        return makeErrorResponse(http::status::bad_request, 
            "JSON error: " + std::string(e.what()), req);
    } catch (const std::exception& e) {
        span.setStatus(false, "error");
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> EthicsApiHandler::handleMakeDecision(
    const http::request<http::string_body>& req
) {
    auto span = Tracer::startSpan("handleEthicsMakeDecision");
    
    try {
        nlohmann::json body = nlohmann::json::parse(req.body());
        
        // Validate required fields
        if (!body.contains("dilemma_description") || !body.contains("philosophy_schools")) {
            span.setStatus(false, "invalid_request");
            return makeErrorResponse(http::status::bad_request, 
                "Missing required fields: dilemma_description, philosophy_schools", req);
        }
        
        // Extract parameters
        std::string dilemma = body["dilemma_description"];
        auto philosophies = body["philosophy_schools"].get<std::vector<std::string>>();
        std::string category = body.value("category", "general");
        bool use_rag = body.value("use_rag", true);
        
        // Execute AQL function via QueryEngine
        std::string aql = "RETURN ETHICS_MAKE_DECISION(@dilemma, @philosophies, @category, @use_rag)";
        nlohmann::json bind_vars = {
            {"dilemma", dilemma},
            {"philosophies", philosophies},
            {"category", category},
            {"use_rag", use_rag}
        };
        
        auto result = executeAQL(aql, bind_vars);
        
        span.setStatus(true);
        return makeResponse(http::status::ok, result.dump(), req);
        
    } catch (const nlohmann::json::exception& e) {
        span.setStatus(false, "json_error");
        return makeErrorResponse(http::status::bad_request, 
            "JSON error: " + std::string(e.what()), req);
    } catch (const std::exception& e) {
        span.setStatus(false, "error");
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> EthicsApiHandler::handleEvaluation(
    const http::request<http::string_body>& req
) {
    auto span = Tracer::startSpan("handleEthicsEvaluation");
    
    try {
        nlohmann::json body = nlohmann::json::parse(req.body());
        
        // Validate required fields
        if (!body.contains("decision")) {
            span.setStatus(false, "invalid_request");
            return makeErrorResponse(http::status::bad_request, 
                "Missing required field: decision", req);
        }
        
        // Extract parameters
        nlohmann::json decision = body["decision"];
        nlohmann::json arguments = body.value("arguments", nlohmann::json::array());
        
        // Execute AQL function via QueryEngine
        std::string aql = "RETURN ETHICS_EVALUATE(@decision, @arguments)";
        nlohmann::json bind_vars = {
            {"decision", decision},
            {"arguments", arguments}
        };
        
        auto result = executeAQL(aql, bind_vars);
        
        span.setStatus(true);
        return makeResponse(http::status::ok, result.dump(), req);
        
    } catch (const nlohmann::json::exception& e) {
        span.setStatus(false, "json_error");
        return makeErrorResponse(http::status::bad_request, 
            "JSON error: " + std::string(e.what()), req);
    } catch (const std::exception& e) {
        span.setStatus(false, "error");
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> EthicsApiHandler::handleGetArguments(
    const http::request<http::string_body>& req
) {
    auto span = Tracer::startSpan("handleEthicsGetArguments");
    
    try {
        // Extract query parameters
        std::string target = std::string(req.target());
        std::string philosophy = extractQueryParam(target, "philosophy");
        std::string types_str = extractQueryParam(target, "type");
        std::string limit_str = extractQueryParam(target, "limit");
        
        if (philosophy.empty()) {
            span.setStatus(false, "invalid_request");
            return makeErrorResponse(http::status::bad_request, 
                "Missing required query parameter: philosophy", req);
        }
        
        // Parse types (comma-separated)
        std::vector<std::string> types = {};

        if (!types_str.empty()) {
            std::stringstream ss(types_str);
            std::string type = {};
            while (std::getline(ss, type, ',')) {
                types.push_back(type);
            }
        }
        
        int limit = limit_str.empty() ? 20 : std::stoi(limit_str);
        
        // Execute AQL function via QueryEngine
        std::string aql = "RETURN ETHICS_GET_ARGUMENTS(@philosophy, @types, @limit)";
        nlohmann::json bind_vars = {
            {"philosophy", philosophy},
            {"types", types},
            {"limit", limit}
        };
        
        auto result = executeAQL(aql, bind_vars);
        
        span.setStatus(true);
        return makeResponse(http::status::ok, result.dump(), req);
        
    } catch (const nlohmann::json::exception& e) {
        span.setStatus(false, "json_error");
        return makeErrorResponse(http::status::bad_request, 
            "JSON error: " + std::string(e.what()), req);
    } catch (const std::exception& e) {
        span.setStatus(false, "error");
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> EthicsApiHandler::handleSearchArguments(
    const http::request<http::string_body>& req
) {
    auto span = Tracer::startSpan("handleEthicsSearchArguments");
    
    try {
        nlohmann::json body = nlohmann::json::parse(req.body());
        
        // Validate required fields
        if (!body.contains("query_text")) {
            span.setStatus(false, "invalid_request");
            return makeErrorResponse(http::status::bad_request, 
                "Missing required field: query_text", req);
        }
        
        // Extract parameters
        std::string query_text = body["query_text"];
        double threshold = body.value("threshold", 0.65);
        int limit = body.value("limit", 10);
        
        // Execute AQL function via QueryEngine
        // Note: ETHICS_FIND_SIMILAR_DILEMMAS searches for similar ethical dilemmas
        std::string aql = "RETURN ETHICS_FIND_SIMILAR_DILEMMAS(@query, @threshold, @limit)";
        nlohmann::json bind_vars = {
            {"query", query_text},
            {"threshold", threshold},
            {"limit", limit}
        };
        
        auto result = executeAQL(aql, bind_vars);
        
        span.setStatus(true);
        return makeResponse(http::status::ok, result.dump(), req);
        
    } catch (const nlohmann::json::exception& e) {
        span.setStatus(false, "json_error");
        return makeErrorResponse(http::status::bad_request, 
            "JSON error: " + std::string(e.what()), req);
    } catch (const std::exception& e) {
        span.setStatus(false, "error");
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> EthicsApiHandler::handleListPhilosophies(
    const http::request<http::string_body>& req
) {
    auto span = Tracer::startSpan("handleEthicsListPhilosophies");
    
    try {
        // Execute AQL function via QueryEngine
        std::string aql = "RETURN ETHICS_LIST_SCHOOLS()";
        
        auto result = executeAQL(aql);
        
        span.setStatus(true);
        return makeResponse(http::status::ok, result.dump(), req);
        
    } catch (const std::exception& e) {
        span.setStatus(false, "error");
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> EthicsApiHandler::handleGetPhilosophy(
    const http::request<http::string_body>& req,
    const std::string& school
) {
    auto span = Tracer::startSpan("handleEthicsGetPhilosophy");
    
    try {
        // Execute AQL function via QueryEngine
        std::string aql = "RETURN ETHICS_LOAD_PROFILE(@school)";
        nlohmann::json bind_vars = {
            {"school", school}
        };
        
        auto result = executeAQL(aql, bind_vars);
        
        span.setStatus(true);
        return makeResponse(http::status::ok, result.dump(), req);
        
    } catch (const std::exception& e) {
        span.setStatus(false, "error");
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> EthicsApiHandler::handleBuildContext(
    const http::request<http::string_body>& req
) {
    auto span = Tracer::startSpan("handleEthicsBuildContext");
    
    try {
        nlohmann::json body = nlohmann::json::parse(req.body());
        
        // Validate required fields
        if (!body.contains("dilemma_description") || !body.contains("philosophy_schools")) {
            span.setStatus(false, "invalid_request");
            return makeErrorResponse(http::status::bad_request, 
                "Missing required fields: dilemma_description, philosophy_schools", req);
        }
        
        // Extract parameters
        std::string dilemma = body["dilemma_description"];
        auto philosophies = body["philosophy_schools"].get<std::vector<std::string>>();
        std::string category = body.value("category", "general");
        
        // Execute AQL function via QueryEngine
        std::string aql = "RETURN ETHICS_BUILD_CONTEXT(@dilemma, @philosophies, @category)";
        nlohmann::json bind_vars = {
            {"dilemma", dilemma},
            {"philosophies", philosophies},
            {"category", category}
        };
        
        auto result = executeAQL(aql, bind_vars);
        
        span.setStatus(true);
        return makeResponse(http::status::ok, result.dump(), req);
        
    } catch (const nlohmann::json::exception& e) {
        span.setStatus(false, "json_error");
        return makeErrorResponse(http::status::bad_request, 
            "JSON error: " + std::string(e.what()), req);
    } catch (const std::exception& e) {
        span.setStatus(false, "error");
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

http::response<http::string_body> EthicsApiHandler::handleGetMetrics(
    const http::request<http::string_body>& req
) {
    auto span = Tracer::startSpan("handleEthicsGetMetrics");
    
    try {
        // Extract query parameter for format
        std::string target = std::string(req.target());
        std::string format = extractQueryParam(target, "format");
        if (format.empty()) {
          format = "json";
        }
        
        // Execute AQL function via QueryEngine
        std::string aql = "RETURN ETHICS_METRICS()";
        
        auto result = executeAQL(aql);
        
        // If Prometheus format requested, convert JSON metrics to text format
        if (format == "prometheus") {
            // result is a JSON object; each key becomes a Prometheus metric.
            // Nested objects are flattened with underscore separators.
            // Non-numeric leaves are skipped.
            std::string prom = {};
            std::function<void(const nlohmann::json&, const std::string&)> flatten =
                [&](const nlohmann::json& node, const std::string& prefix) {
                    if (node.is_object()) {
                        for (auto it = node.begin(); it != node.end(); ++it) {
                            std::string key = prefix.empty()
                                ? it.key()
                                : prefix + "_" + it.key();
                            flatten(it.value(), key);
                        }
                    } else if (node.is_number()) {
                        prom += "# TYPE " + prefix + " gauge\n";
                        prom += prefix + " " + node.dump() + "\n";
                    }
                    // arrays and strings are skipped
                };

            // result may be wrapped in an array (AQL RETURN produces an array)
            if (result.is_array() && !result.empty()) {
                flatten(result[0], "ethics");
            } else {
                flatten(result, "ethics");
            }

            http::response<http::string_body> prom_res{http::status::ok, 11};
            prom_res.set(http::field::content_type, "text/plain; version=0.0.4");
            prom_res.body() = prom;
            prom_res.prepare_payload();
            span.setStatus(true);
            return prom_res;
        }

        span.setStatus(true);
        return makeResponse(http::status::ok, result.dump(), req);
        
    } catch (const std::exception& e) {
        span.setStatus(false, "error");
        return makeErrorResponse(http::status::internal_server_error, e.what(), req);
    }
}

// Helper methods

http::response<http::string_body> EthicsApiHandler::makeErrorResponse(
    http::status status, 
    const std::string& message, 
    const http::request<http::string_body>& req
) {
    nlohmann::json error_body = {
        {"error", true},
        {"message", message},
        {"status_code", static_cast<int>(status)}
    };
    
    http::response<http::string_body> res{status, req.version()};
    res.set(http::field::content_type, "application/json");
    res.set(http::field::server, "ThemisDB");
    res.keep_alive(req.keep_alive());
    res.body() = error_body.dump();
    res.prepare_payload();
    
    return res;
}

http::response<http::string_body> EthicsApiHandler::makeResponse(
    http::status status, 
    const std::string& body, 
    const http::request<http::string_body>& req
) {
    http::response<http::string_body> res{status, req.version()};
    res.set(http::field::content_type, "application/json");
    res.set(http::field::server, "ThemisDB");
    res.keep_alive(req.keep_alive());
    res.body() = body;
    res.prepare_payload();
    
    return res;
}

nlohmann::json EthicsApiHandler::executeAQL(
    const std::string& aql_query,
    const nlohmann::json& bind_vars
) {
    if (!query_engine_) {
        throw std::runtime_error("QueryEngine not available");
    }
    auto& query_engine = *query_engine_;

    // Substitute bind parameters (@name → value literal) for simple string/number vars.
    // Each placeholder is replaced exactly once, left-to-right, to prevent re-substitution.
    std::string resolved_query = aql_query;
    for (auto it = bind_vars.begin(); it != bind_vars.end(); ++it) {
        const std::string placeholder = "@" + it.key();
        std::string replacement = {};
        if (it.value().is_string()) {
            // Escape embedded single quotes to prevent AQL injection
            std::string raw = it.value().get<std::string>();
            std::string escaped = {};
            escaped.reserve(raw.size());
            for (char c : raw) {
                if (c == '\'') { escaped += "''"; } else { escaped += c; }
            }
            replacement = "'" + escaped + "'";
        } else {
            replacement = it.value().dump();
        }
        // Replace all occurrences, advancing past each replacement to avoid re-substitution
        size_t pos = 0;
        while ((pos = resolved_query.find(placeholder, pos)) != std::string::npos) {
            resolved_query.replace(pos, placeholder.size(), replacement);
            pos += replacement.size(); // skip over newly inserted replacement text
        }
    }

    // Parse the AQL query
    query::AQLParser parser;
    auto parse_result = parser.parse(resolved_query);
    if (!parse_result.has_value()) {
        throw std::runtime_error("AQL parse error: " + parse_result.error().message());
    }

    // Translate the AST to a QueryEngine query
    auto translation = AQLTranslator::translate(*parse_result);
    if (!translation.success) {
        throw std::runtime_error("AQL translation error: " + translation.error_message);
    }

    // Execute and return results as JSON array
    nlohmann::json rows = nlohmann::json::array();

    if (translation.disjunctive.has_value()) {
        auto result = query_engine.executeOrEntities(translation.disjunctive.value());
        if (result.has_value()) {
            for (const auto& entity : result.value()) {
                rows.push_back(nlohmann::json::parse(entity.toJson()));
            }
        }
    } else {
        auto result = query_engine.executeAndEntities(translation.conjunctive_query);
        if (result.has_value()) {
            for (const auto& entity : result.value()) {
                rows.push_back(nlohmann::json::parse(entity.toJson()));
            }
        }
    }

    return nlohmann::json{{"results", rows}, {"count", rows.size()}};
}

std::string EthicsApiHandler::extractQueryParam(
    const std::string& target,
    const std::string& param
) const {
    // Find query string
    size_t query_start = target.find('?');
    if (query_start == std::string::npos) {
        return "";
    }
    
    std::string query_string = target.substr(query_start + 1);
    
    // Parse parameters
    std::string param_prefix = param + "=";
    size_t param_pos = query_string.find(param_prefix);
    if (param_pos == std::string::npos) {
        return "";
    }
    
    size_t value_start = param_pos + param_prefix.length();
    size_t value_end = query_string.find('&', value_start);
    
    if (value_end == std::string::npos) {
        return query_string.substr(value_start);
    } else {
        return query_string.substr(value_start, value_end - value_start);
    }
}

} // namespace server
} // namespace themis
