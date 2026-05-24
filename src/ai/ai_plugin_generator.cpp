// THEMIS_GAP_STATS: gaps=0 unimpl=0 stub=0 mock=0 sim=0 todo=0 debt=0 scanned=2026-05-20
/**
 * @file ai_plugin_generator.cpp
 * @brief Production implementation of AIPluginGenerator with endpoint-backed generation.
 */

#include "ai/ai_plugin_generator.h"
#include "utils/error_registry.h"
#include "utils/expected.h"

#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>

#include <spdlog/spdlog.h>
#include <stdexcept>
#include <string>
#include <array>

namespace themis {
namespace plugins {
namespace ai {

namespace {
namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

Result<nlohmann::json> postJsonHttp(const std::string& endpoint, const nlohmann::json& payload) {
    constexpr std::string_view kHttpPrefix = "http://";
    if (endpoint.rfind(kHttpPrefix, 0) != 0) {
        return tl::unexpected(
            Error(errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED,
                  "AIPluginGenerator: only http:// endpoints are supported: " + endpoint));
    }

    std::string endpoint_without_scheme = endpoint.substr(kHttpPrefix.size());
    std::string host_port = endpoint_without_scheme;
    std::string target = "/";
    if (const auto slash = endpoint_without_scheme.find('/'); slash != std::string::npos) {
        host_port = endpoint_without_scheme.substr(0, slash);
        target = endpoint_without_scheme.substr(slash);
        if (target.empty()) {
            target = "/";
        }
    }

    std::string host = host_port;
    std::string port = "80";
    if (const auto colon = host_port.rfind(':'); colon != std::string::npos && colon + 1 < host_port.size()) {
        host = host_port.substr(0, colon);
        port = host_port.substr(colon + 1);
    }

    if (host.empty()) {
        return tl::unexpected(
            Error(errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED,
                  "AIPluginGenerator: invalid endpoint host: " + endpoint));
    }

    try {
        net::io_context ioc;
        tcp::resolver resolver(ioc);
        beast::tcp_stream stream(ioc);

        auto const results = resolver.resolve(host, port);
        stream.connect(results);

        http::request<http::string_body> req{http::verb::post, target, 11};
        req.set(http::field::host, host);
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
        req.set(http::field::content_type, "application/json");
        req.body() = payload.dump();
        req.prepare_payload();

        http::write(stream, req);

        beast::flat_buffer buffer;
        http::response<http::string_body> res;
        http::read(stream, buffer, res);

        beast::error_code ec;
        stream.socket().shutdown(tcp::socket::shutdown_both, ec);

        if (res.result_int() < 200 || res.result_int() >= 300) {
            return tl::unexpected(
                Error(errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED,
                      "AIPluginGenerator: endpoint returned HTTP " + std::to_string(res.result_int())));
        }

        return nlohmann::json::parse(res.body());
    } catch (const std::exception& e) {
        return tl::unexpected(
            Error(errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED,
                  "AIPluginGenerator: endpoint request failed: " + std::string(e.what())));
    }
}

PluginType parsePluginTypeOrDefault(const nlohmann::json& type_json, PluginType fallback) {
    if (type_json.is_number_integer()) {
        const auto raw = type_json.get<int>();
        if (raw >= static_cast<int>(PluginType::COMPUTE_BACKEND) &&
            raw <= static_cast<int>(PluginType::CUSTOM)) {
            return static_cast<PluginType>(raw);
        }
    }
    if (type_json.is_string()) {
        const auto type = type_json.get<std::string>();
        static const std::array<std::pair<std::string_view, PluginType>, 11> kMap{{
            {"compute_backend", PluginType::COMPUTE_BACKEND},
            {"blob_storage", PluginType::BLOB_STORAGE},
            {"importer", PluginType::IMPORTER},
            {"exporter", PluginType::EXPORTER},
            {"hsm_provider", PluginType::HSM_PROVIDER},
            {"embedding", PluginType::EMBEDDING},
            {"llm_backend", PluginType::LLM_BACKEND},
            {"audio_processing", PluginType::AUDIO_PROCESSING},
            {"image_generation", PluginType::IMAGE_GENERATION},
            {"agentic_tool", PluginType::AGENTIC_TOOL},
            {"custom", PluginType::CUSTOM},
        }};
        for (const auto& [name, plugin_type] : kMap) {
            if (type == name) {
                return plugin_type;
            }
        }
    }
    return fallback;
}

} // namespace

AIPluginGenerator::AIPluginGenerator(const Config& config)
    : config_(config)
{}

AIPluginGenerator::~AIPluginGenerator() = default;

void AIPluginGenerator::setLlmHttpPostFn(LlmHttpPostFn fn) {
    llm_http_post_fn_ = std::move(fn);
}

Result<void> AIPluginGenerator::validatePrompt(const PluginGenerationPrompt& prompt)
{
    if (prompt.description.empty()) {
        return tl::unexpected(
            Error(errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED,
                  "AIPluginGenerator: prompt description must not be empty"));
    }
    if (prompt.description.size() > 8192u) {
        return tl::unexpected(
            Error(errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED,
                  "AIPluginGenerator: prompt description exceeds 8192-character limit"));
    }
    return {};  // success
}

Result<GeneratedPlugin> AIPluginGenerator::generatePlugin(
    const PluginGenerationPrompt& prompt)
{
    // 1. Validate inputs first.
    auto vr = validatePrompt(prompt);
    if (!vr) {
        return tl::unexpected(vr.error());
    }

    spdlog::debug("[AIPluginGenerator] generatePlugin: description='{}' endpoint='{}'",
                  prompt.description.substr(0, 80), config_.llm_endpoint);

    nlohmann::json request_payload = {
        {"description", prompt.description},
        {"plugin_type", static_cast<int>(prompt.type)},
        {"required_capabilities", prompt.required_capabilities},
        {"dependencies", prompt.dependencies},
        {"llm_model", static_cast<int>(prompt.llm_model)},
        {"security_level", static_cast<int>(prompt.security_level)},
        {"generate_tests", prompt.generate_tests},
        {"generate_docs", prompt.generate_docs}
    };

    Result<nlohmann::json> response = config_.endpoint_invoker
        ? config_.endpoint_invoker(config_.llm_endpoint, request_payload)
        : postJsonHttp(config_.llm_endpoint, request_payload);
    if (!response) {
        return tl::unexpected(response.error());
    }

    const auto& root = response.value();
    const nlohmann::json& plugin_json =
        (root.contains("plugin") && root["plugin"].is_object()) ? root["plugin"] : root;

    GeneratedPlugin generated;
    generated.header_code = plugin_json.value("header_code", std::string{});
    generated.implementation_code = plugin_json.value("implementation_code", std::string{});
    generated.test_code = plugin_json.value("test_code", std::string{});
    generated.cmake_code = plugin_json.value("cmake_code", std::string{});
    generated.passed_security_checks = plugin_json.value("passed_security_checks", false);
    generated.security_report = plugin_json.value("security_report", std::string{});

    if (generated.header_code.empty() || generated.implementation_code.empty()) {
        return tl::unexpected(
            Error(errors::ErrorCode::ERR_PLUGIN_LOAD_FAILED,
                  "AIPluginGenerator: endpoint response missing generated code"));
    }

    // Fill manifest with endpoint values when present, otherwise safe defaults.
    generated.manifest.name = plugin_json.value("name", std::string{"generated_plugin"});
    generated.manifest.version = plugin_json.value("version", std::string{"1.0.0"});
    generated.manifest.description = plugin_json.value("description", prompt.description);
    generated.manifest.type = parsePluginTypeOrDefault(plugin_json.value("type", nlohmann::json{}), prompt.type);
    generated.manifest.dependencies = prompt.dependencies;
    generated.manifest.capabilities.thread_safe = true;
    generated.manifest.capabilities.supports_batching = true;
    generated.manifest.capabilities.supports_streaming = false;
    generated.manifest.capabilities.supports_transactions = false;
    generated.manifest.capabilities.gpu_accelerated = false;

    if (plugin_json.contains("build_dependencies") && plugin_json["build_dependencies"].is_array()) {
        generated.build_dependencies = plugin_json["build_dependencies"].get<std::vector<std::string>>();
    }

    return generated;
}

} // namespace ai
} // namespace plugins
} // namespace themis
