/**
 * @file server_listen_settings.h
 * @brief Resolution of themis_server listener, data-directory and config-file
 *        settings from CLI, environment, config file and built-in defaults.
 *
 * @note Header-only. Used by `src/main_server.cpp` and by container
 *       deployments (Docker/Kubernetes), which configure the server mainly via
 *       environment variables and a mounted config file.
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <optional>
#include <string>
#include <string_view>

#include <nlohmann/json.hpp>

namespace themis::server {

/// @brief Built-in defaults used when no CLI/env/config value is present.
struct ServerListenDefaults {
    std::string host = "0.0.0.0";
    std::uint16_t port = 8765;
    std::string data_dir = "./data/themis_server";
    std::size_t worker_threads = 0; ///< 0 = auto (hardware concurrency)
};

/// @brief Values explicitly supplied on the command line (unset = not given).
struct ServerListenCliOverrides {
    std::optional<std::string> host;
    std::optional<std::uint16_t> port;
    std::optional<std::string> data_dir;
    std::optional<std::size_t> worker_threads;
};

/// @brief Effective listener settings plus the source of each value.
struct ServerListenSettings {
    std::string host;
    std::uint16_t port = 0;
    std::string data_dir;
    std::size_t worker_threads = 0;
    std::string host_source;
    std::string port_source;
    std::string data_dir_source;
    std::string worker_threads_source;
};

/// @brief Result of resolveServerListenSettings().
struct ServerListenResolution {
    bool ok = false;
    ServerListenSettings settings;
    std::string error; ///< Non-empty when ok == false.
};

/**
 * @brief Environment lookup callback.
 * @note Returns std::nullopt for unset variables. Empty values are treated as
 *       unset by the resolver so `-e THEMIS_PORT=` does not override config.
 */
using EnvLookupFn = std::function<std::optional<std::string>(std::string_view)>;

/// @brief Default EnvLookupFn backed by std::getenv.
[[nodiscard]] inline std::optional<std::string> processEnvLookup(std::string_view name) {
    const char* value = std::getenv(std::string(name).c_str());
    if (value == nullptr) {
        return std::nullopt;
    }
    return std::string(value);
}

namespace detail {

[[nodiscard]] inline std::optional<std::string> nonEmptyEnv(const EnvLookupFn& env, std::string_view name) {
    if (!env) {
        return std::nullopt;
    }
    auto value = env(name);
    if (!value || value->empty()) {
        return std::nullopt;
    }
    return value;
}

[[nodiscard]] inline std::optional<std::uint64_t> parseUnsigned(const std::string& text) {
    if (text.empty()) {
        return std::nullopt;
    }
    std::uint64_t value = 0;
    for (const char c : text) {
        if (c < '0' || c > '9') {
            return std::nullopt;
        }
        value = value * 10 + static_cast<std::uint64_t>(c - '0');
        if (value > 0xFFFFFFFFull) {
            return std::nullopt;
        }
    }
    return value;
}

[[nodiscard]] inline const nlohmann::json* jsonAt(const std::optional<nlohmann::json>& config,
                                                  std::string_view section,
                                                  std::string_view key) {
    if (!config || !config->is_object()) {
        return nullptr;
    }
    const auto sec = config->find(std::string(section));
    if (sec == config->end() || !sec->is_object()) {
        return nullptr;
    }
    const auto it = sec->find(std::string(key));
    if (it == sec->end() || it->is_null()) {
        return nullptr;
    }
    return &(*it);
}

} // namespace detail

/**
 * @brief Resolve host, port, data directory and worker threads for themis_server.
 *
 * Precedence per value: CLI > environment > config file > built-in default.
 * This lets container images ship a default config file while operators still
 * override individual values with `-e THEMIS_PORT=...` or `--port`.
 *
 * | Value          | CLI            | Environment             | Config key              |
 * |----------------|----------------|-------------------------|-------------------------|
 * | host           | `--host`       | `THEMIS_HOST`           | `server.host`           |
 * | port           | `--port`       | `THEMIS_PORT`           | `server.port`           |
 * | data directory | `--data-dir`   | `THEMIS_DATA_DIR`       | `storage.rocksdb_path`  |
 * | worker threads | `--threads`    | `THEMIS_WORKER_THREADS` | `server.worker_threads` |
 *
 * @param cli Values explicitly provided on the command line.
 * @param env Environment lookup; may be empty to ignore the environment.
 *        Empty variable values are treated as unset.
 * @param config Parsed config file (JSON or YAML converted to JSON), if any.
 * @param defaults Built-in defaults used when no other source supplies a value.
 * @return Resolution with effective settings and per-value source labels
 *         (`cli`, `env.<NAME>`, `config.<path>`, `default`). `ok` is false and
 *         `error` names the offending source when a port is outside 1..65535,
 *         a numeric value is malformed, or a config value has the wrong type.
 * @note Pure function; does not throw for malformed input and is thread-safe as
 *       long as @p env is thread-safe.
 */
[[nodiscard]] inline ServerListenResolution resolveServerListenSettings(
    const ServerListenCliOverrides& cli,
    const EnvLookupFn& env,
    const std::optional<nlohmann::json>& config,
    const ServerListenDefaults& defaults = {}
) {
    ServerListenResolution result;
    auto& s = result.settings;
    auto fail = [&](std::string message) {
        result.ok = false;
        result.error = std::move(message);
        return result;
    };

    // host
    if (cli.host && !cli.host->empty()) {
        s.host = *cli.host;
        s.host_source = "cli";
    } else if (auto v = detail::nonEmptyEnv(env, "THEMIS_HOST")) {
        s.host = *v;
        s.host_source = "env.THEMIS_HOST";
    } else if (const auto* j = detail::jsonAt(config, "server", "host")) {
        if (!j->is_string() || j->get<std::string>().empty()) {
            return fail("config.server.host must be a non-empty string");
        }
        s.host = j->get<std::string>();
        s.host_source = "config.server.host";
    } else {
        s.host = defaults.host;
        s.host_source = "default";
    }

    // port
    auto valid_port = [](std::uint64_t p) { return p >= 1 && p <= 65535; };
    if (cli.port) {
        s.port = *cli.port;
        s.port_source = "cli";
    } else if (auto v = detail::nonEmptyEnv(env, "THEMIS_PORT")) {
        const auto parsed = detail::parseUnsigned(*v);
        if (!parsed || !valid_port(*parsed)) {
            return fail("env.THEMIS_PORT must be an integer in 1..65535, got '" + *v + "'");
        }
        s.port = static_cast<std::uint16_t>(*parsed);
        s.port_source = "env.THEMIS_PORT";
    } else if (const auto* j = detail::jsonAt(config, "server", "port")) {
        if (!j->is_number_integer() || j->get<std::int64_t>() < 1 || j->get<std::int64_t>() > 65535) {
            return fail("config.server.port must be an integer in 1..65535, got " + j->dump());
        }
        s.port = static_cast<std::uint16_t>(j->get<std::int64_t>());
        s.port_source = "config.server.port";
    } else {
        s.port = defaults.port;
        s.port_source = "default";
    }

    // data directory
    if (cli.data_dir && !cli.data_dir->empty()) {
        s.data_dir = *cli.data_dir;
        s.data_dir_source = "cli";
    } else if (auto v = detail::nonEmptyEnv(env, "THEMIS_DATA_DIR")) {
        s.data_dir = *v;
        s.data_dir_source = "env.THEMIS_DATA_DIR";
    } else if (const auto* j = detail::jsonAt(config, "storage", "rocksdb_path")) {
        if (!j->is_string() || j->get<std::string>().empty()) {
            return fail("config.storage.rocksdb_path must be a non-empty string");
        }
        s.data_dir = j->get<std::string>();
        s.data_dir_source = "config.storage.rocksdb_path";
    } else {
        s.data_dir = defaults.data_dir;
        s.data_dir_source = "default";
    }

    // worker threads
    if (cli.worker_threads) {
        s.worker_threads = *cli.worker_threads;
        s.worker_threads_source = "cli";
    } else if (auto v = detail::nonEmptyEnv(env, "THEMIS_WORKER_THREADS")) {
        const auto parsed = detail::parseUnsigned(*v);
        if (!parsed) {
            return fail("env.THEMIS_WORKER_THREADS must be a non-negative integer, got '" + *v + "'");
        }
        s.worker_threads = static_cast<std::size_t>(*parsed);
        s.worker_threads_source = "env.THEMIS_WORKER_THREADS";
    } else if (const auto* j = detail::jsonAt(config, "server", "worker_threads")) {
        if (!j->is_number_integer() || j->get<std::int64_t>() < 0) {
            return fail("config.server.worker_threads must be a non-negative integer, got " + j->dump());
        }
        s.worker_threads = static_cast<std::size_t>(j->get<std::int64_t>());
        s.worker_threads_source = "config.server.worker_threads";
    } else {
        s.worker_threads = defaults.worker_threads;
        s.worker_threads_source = "default";
    }

    result.ok = true;
    return result;
}

/**
 * @brief Resolve the explicit config file path.
 *
 * Precedence: `--config` CLI value > `THEMIS_CONFIG` environment variable.
 * When neither is set, the caller falls back to its default search paths.
 *
 * @param cli_config Value of `--config`, if given.
 * @param env Environment lookup; may be empty. Empty values are treated as unset.
 * @return Explicit config path, or std::nullopt when none was requested.
 *         An explicit path that cannot be read must be treated as a startup error
 *         by the caller (fail fast instead of silently using defaults).
 */
[[nodiscard]] inline std::optional<std::string> resolveServerConfigPath(
    const std::optional<std::string>& cli_config,
    const EnvLookupFn& env
) {
    if (cli_config && !cli_config->empty()) {
        return cli_config;
    }
    return detail::nonEmptyEnv(env, "THEMIS_CONFIG");
}

/// @brief Default port of the in-process gRPC server (WAL apply + ThemisDB API).
inline constexpr std::uint16_t kDefaultGrpcPort = 50051;

/// @brief Effective gRPC listener and the loopback address used by in-process clients.
struct GrpcListenSettings {
    std::string host = "0.0.0.0";
    std::uint16_t port = kDefaultGrpcPort;
    std::string listen_address;         ///< host:port passed to grpc::ServerBuilder.
    std::string web_backend_address;    ///< Target of the gRPC-Web proxy on the HTTP port.
};

/// @brief Result of resolveGrpcListenSettings().
struct GrpcListenResolution {
    bool ok = false;
    GrpcListenSettings settings;
    std::string error;
};

/**
 * @brief Resolve the gRPC listener and the gRPC-Web proxy backend from the environment.
 *
 * | Value             | Environment              | Default            |
 * |-------------------|--------------------------|--------------------|
 * | bind host         | `THEMIS_WAL_GRPC_HOST`   | `0.0.0.0`          |
 * | port              | `THEMIS_WAL_GRPC_PORT`   | `50051`            |
 * | gRPC-Web backend  | `THEMIS_GRPC_WEB_BACKEND`| `127.0.0.1:<port>` |
 *
 * The gRPC-Web proxy (`POST /grpc-web/*` on the HTTP port) forwards to the
 * in-process gRPC server, so its default backend follows `THEMIS_WAL_GRPC_PORT`.
 * When the gRPC host is a concrete address (not a wildcard), the proxy targets
 * that address instead of loopback.
 *
 * @param env Environment lookup; may be empty. Empty values are treated as unset.
 * @return `ok == false` with an error naming the variable when the port is not
 *         an integer in 1..65535. Pure function; never throws for bad input.
 */
[[nodiscard]] inline GrpcListenResolution resolveGrpcListenSettings(const EnvLookupFn& env) {
    GrpcListenResolution result;
    auto& s = result.settings;
    if (auto h = detail::nonEmptyEnv(env, "THEMIS_WAL_GRPC_HOST")) {
        s.host = *h;
    }
    if (auto p = detail::nonEmptyEnv(env, "THEMIS_WAL_GRPC_PORT")) {
        const auto parsed = detail::parseUnsigned(*p);
        if (!parsed || *parsed < 1 || *parsed > 65535) {
            result.error = "env.THEMIS_WAL_GRPC_PORT must be an integer in 1..65535, got '" + *p + "'";
            return result;
        }
        s.port = static_cast<std::uint16_t>(*parsed);
    }
    s.listen_address = s.host + ":" + std::to_string(s.port);
    if (auto backend = detail::nonEmptyEnv(env, "THEMIS_GRPC_WEB_BACKEND")) {
        s.web_backend_address = *backend;
    } else {
        const bool wildcard = s.host == "0.0.0.0" || s.host == "::" || s.host == "[::]";
        s.web_backend_address = (wildcard ? std::string("127.0.0.1") : s.host) + ":" + std::to_string(s.port);
    }
    result.ok = true;
    return result;
}

} // namespace themis::server
