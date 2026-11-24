#include <gtest/gtest.h>

#include "server/server_listen_settings.h"

#include <map>
#include <string>

using json = nlohmann::json;
using themis::server::EnvLookupFn;
using themis::server::ServerListenCliOverrides;
using themis::server::resolveServerConfigPath;
using themis::server::resolveServerListenSettings;

namespace {

EnvLookupFn envFrom(std::map<std::string, std::string> vars) {
    return [vars = std::move(vars)](std::string_view name) -> std::optional<std::string> {
        const auto it = vars.find(std::string(name));
        if (it == vars.end()) {
            return std::nullopt;
        }
        return it->second;
    };
}

const json kConfig = json::parse(R"({
    "server":  {"host": "10.0.0.1", "port": 8080, "worker_threads": 4},
    "storage": {"rocksdb_path": "/var/lib/themis/data"}
})");

TEST(ServerListenSettings, DefaultsWhenNothingSet) {
    const auto r = resolveServerListenSettings({}, envFrom({}), std::nullopt);
    ASSERT_TRUE(r.ok) << r.error;
    EXPECT_EQ(r.settings.host, "0.0.0.0");
    EXPECT_EQ(r.settings.port, 8765);
    EXPECT_EQ(r.settings.data_dir, "./data/themis_server");
    EXPECT_EQ(r.settings.worker_threads, 0u);
    EXPECT_EQ(r.settings.port_source, "default");
}

TEST(ServerListenSettings, ConfigAppliesWhenNoCliOrEnv) {
    const auto r = resolveServerListenSettings({}, envFrom({}), kConfig);
    ASSERT_TRUE(r.ok) << r.error;
    EXPECT_EQ(r.settings.host, "10.0.0.1");
    EXPECT_EQ(r.settings.port, 8080);
    EXPECT_EQ(r.settings.data_dir, "/var/lib/themis/data");
    EXPECT_EQ(r.settings.worker_threads, 4u);
    EXPECT_EQ(r.settings.data_dir_source, "config.storage.rocksdb_path");
}

TEST(ServerListenSettings, EnvOverridesConfig) {
    const auto r = resolveServerListenSettings(
        {}, envFrom({{"THEMIS_PORT", "9000"}, {"THEMIS_DATA_DIR", "/data"},
                     {"THEMIS_HOST", "127.0.0.1"}, {"THEMIS_WORKER_THREADS", "2"}}),
        kConfig);
    ASSERT_TRUE(r.ok) << r.error;
    EXPECT_EQ(r.settings.port, 9000);
    EXPECT_EQ(r.settings.port_source, "env.THEMIS_PORT");
    EXPECT_EQ(r.settings.data_dir, "/data");
    EXPECT_EQ(r.settings.host, "127.0.0.1");
    EXPECT_EQ(r.settings.worker_threads, 2u);
}

TEST(ServerListenSettings, CliOverridesEnvAndConfig) {
    ServerListenCliOverrides cli;
    cli.port = 7000;
    cli.data_dir = "/cli/data";
    const auto r = resolveServerListenSettings(
        cli, envFrom({{"THEMIS_PORT", "9000"}, {"THEMIS_DATA_DIR", "/data"}}), kConfig);
    ASSERT_TRUE(r.ok) << r.error;
    EXPECT_EQ(r.settings.port, 7000);
    EXPECT_EQ(r.settings.port_source, "cli");
    EXPECT_EQ(r.settings.data_dir, "/cli/data");
    EXPECT_EQ(r.settings.host, "10.0.0.1");
}

TEST(ServerListenSettings, EmptyEnvValueIsIgnored) {
    const auto r = resolveServerListenSettings({}, envFrom({{"THEMIS_PORT", ""}}), kConfig);
    ASSERT_TRUE(r.ok) << r.error;
    EXPECT_EQ(r.settings.port, 8080);
}

TEST(ServerListenSettings, RejectsInvalidEnvPort) {
    for (const char* bad : {"0", "65536", "80a", "-1"}) {
        const auto r = resolveServerListenSettings({}, envFrom({{"THEMIS_PORT", bad}}), std::nullopt);
        EXPECT_FALSE(r.ok) << bad;
        EXPECT_NE(r.error.find("THEMIS_PORT"), std::string::npos);
    }
}

TEST(ServerListenSettings, RejectsInvalidConfigValues) {
    EXPECT_FALSE(resolveServerListenSettings({}, envFrom({}), json::parse(R"({"server":{"port":70000}})")).ok);
    EXPECT_FALSE(resolveServerListenSettings({}, envFrom({}), json::parse(R"({"server":{"port":"8080"}})")).ok);
    EXPECT_FALSE(resolveServerListenSettings({}, envFrom({}), json::parse(R"({"storage":{"rocksdb_path":""}})")).ok);
    EXPECT_FALSE(resolveServerListenSettings({}, envFrom({}), json::parse(R"({"server":{"worker_threads":-1}})")).ok);
}

TEST(ServerListenSettings, ConfigPathPrefersCliOverEnv) {
    const auto env = envFrom({{"THEMIS_CONFIG", "/etc/themis/config.yaml"}});
    EXPECT_EQ(resolveServerConfigPath(std::string("/tmp/cli.yaml"), env), "/tmp/cli.yaml");
    EXPECT_EQ(resolveServerConfigPath(std::nullopt, env), "/etc/themis/config.yaml");
    EXPECT_FALSE(resolveServerConfigPath(std::nullopt, envFrom({{"THEMIS_CONFIG", ""}})).has_value());
}

TEST(GrpcListenSettings, DefaultsTargetLoopbackOn50051) {
    const auto r = themis::server::resolveGrpcListenSettings(envFrom({}));
    ASSERT_TRUE(r.ok) << r.error;
    EXPECT_EQ(r.settings.listen_address, "0.0.0.0:50051");
    EXPECT_EQ(r.settings.web_backend_address, "127.0.0.1:50051");
}

TEST(GrpcListenSettings, WebBackendFollowsGrpcPortAndHost) {
    auto r = themis::server::resolveGrpcListenSettings(envFrom({{"THEMIS_WAL_GRPC_PORT", "6000"}}));
    ASSERT_TRUE(r.ok) << r.error;
    EXPECT_EQ(r.settings.web_backend_address, "127.0.0.1:6000");

    r = themis::server::resolveGrpcListenSettings(
        envFrom({{"THEMIS_WAL_GRPC_HOST", "10.1.2.3"}, {"THEMIS_WAL_GRPC_PORT", "6000"}}));
    ASSERT_TRUE(r.ok) << r.error;
    EXPECT_EQ(r.settings.listen_address, "10.1.2.3:6000");
    EXPECT_EQ(r.settings.web_backend_address, "10.1.2.3:6000");

    r = themis::server::resolveGrpcListenSettings(envFrom({{"THEMIS_GRPC_WEB_BACKEND", "grpc:7000"}}));
    ASSERT_TRUE(r.ok) << r.error;
    EXPECT_EQ(r.settings.web_backend_address, "grpc:7000");
}

TEST(GrpcListenSettings, RejectsInvalidPort) {
    for (const char* bad : {"0", "70000", "abc"}) {
        const auto r = themis::server::resolveGrpcListenSettings(envFrom({{"THEMIS_WAL_GRPC_PORT", bad}}));
        EXPECT_FALSE(r.ok) << bad;
        EXPECT_NE(r.error.find("THEMIS_WAL_GRPC_PORT"), std::string::npos);
    }
}

} // namespace
