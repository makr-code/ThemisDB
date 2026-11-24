// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#include <gtest/gtest.h>

#if defined(__linux__)

#include "sharding/shard_rpc_client.h"
#include "sharding/shard_rpc_server.h"
#include "sharding/shard_range_snapshot.h"
#include "sharding/shard_transfer_executor.h"
#include "sharding/shard_transfer_store.h"
#include "sharding/raft_consensus.h"
#include "sharding/raft_shard_manager.h"
#include "shard_rpc.grpc.pb.h"

#if defined(THEMIS_TEST_HAS_DISTRIBUTED_TENSOR_RPC_ADAPTER)
#include "distributed_tensor/shard_rpc_fragment_fetcher.h"
#include "distributed_tensor/graph_summary_projection.h"
#include "distributed_tensor/local_projection_publisher.h"
#include "sharding/shard_artifact_fragment_store.h"
#endif

#if defined(THEMIS_TEST_HAS_DISTRIBUTED_TENSOR_GRAPH)
#include "rag/graph_truth_validator.h"
#endif

#include "../test_crypto_material_utils.h"

#include <grpcpp/grpcpp.h>
#include <nlohmann/json.hpp>
#include <openssl/evp.h>

#include <algorithm>
#include <arpa/inet.h>
#include <spawn.h>
#include <sys/prctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

extern char** environ;

namespace {

namespace fs = std::filesystem;
namespace rpc = themis::sharding::proto;
using themis::sharding::ShardRPCServer;
using themis::sharding::ShardRPCClient;
using themis::sharding::AutoRebalancer;
using themis::sharding::ShardTransferManifest;
using themis::sharding::ShardTransferStore;
using namespace std::chrono_literals;

[[nodiscard]] themisdb::sharding::RaftShardManager::Config raftManagerConfig(
    const fs::path& root, const std::vector<std::string>& members,
    const std::map<std::string, std::string>& addresses,
    const std::string& local_node_id = "node1");

constexpr char kReceiverMode[] = "--shard-transfer-receiver";
constexpr char kRaftReceiverMode[] = "--raft-peer-receiver";
constexpr char kRaftLeaderMode[] = "--raft-leader-process";
constexpr char kSource[] = "source-shard";
constexpr char kTarget[] = "target-shard";
constexpr char kSourceIdentity[] = "source.test";
constexpr char kRaftGroup[] = "payments";
constexpr std::uint64_t kEpoch = 17;
const std::string kValue(ShardTransferStore::kMaxChunkBytes - 9, 'v');
// Two canonical records, with the first record ending exactly at the chunk boundary.
const std::string kFirstPayload = std::string("\0\0\0\1\0\x0f\xff\xf7" "a", 9) + kValue;
constexpr std::string_view kSecondPayload("\0\0\0\1\0\0\0\3" "zb\0c", 12);

[[nodiscard]] std::string sha256(std::string_view payload) {
    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int length = 0;
    if (EVP_Digest(payload.data(), payload.size(), digest, &length, EVP_sha256(), nullptr) != 1) {
        throw std::runtime_error("could not digest test payload");
    }
    constexpr char hex[] = "0123456789abcdef";
    std::string result;
    for (unsigned int i = 0; i < length; ++i) {
        result.push_back(hex[digest[i] >> 4]);
        result.push_back(hex[digest[i] & 15]);
    }
    return result;
}

[[nodiscard]] std::vector<std::pair<std::string, std::string>> decodeRecords(std::string_view bytes) {
    std::vector<std::pair<std::string, std::string>> records;
    while (!bytes.empty()) {
        if (bytes.size() < 8) {
            throw std::runtime_error("truncated canonical record header");
        }
        const auto read_size = [&](std::size_t offset) {
            std::uint32_t size = 0;
            for (std::size_t i = 0; i < 4; ++i) {
                size = (size << 8) | static_cast<unsigned char>(bytes[offset + i]);
            }
            return size;
        };
        const auto key_size = read_size(0);
        const auto value_size = read_size(4);
        bytes.remove_prefix(8);
        const auto record_size = static_cast<std::uint64_t>(key_size) + value_size;
        if (record_size > bytes.size()) {
            throw std::runtime_error("truncated canonical record");
        }
        records.emplace_back(bytes.substr(0, key_size), bytes.substr(key_size, value_size));
        bytes.remove_prefix(static_cast<std::size_t>(record_size));
    }
    return records;
}

[[nodiscard]] auto snapshotFor(const std::string& id) {
    ShardTransferManifest manifest;
    manifest.id = id;
    manifest.source = kSource;
    manifest.target = kTarget;
    manifest.range_start = 100;
    manifest.range_end = 200;
    manifest.epoch = kEpoch;
    manifest.watermark = "42";
    return themis::sharding::materializeShardRangeSnapshot(std::move(manifest), [](const auto& visit) {
        visit(100, "a", kValue);
        visit(199, "z", std::string_view("b\0c", 3));
    });
}

[[nodiscard]] ShardTransferManifest assignment(const std::string& id) {
    return snapshotFor(id).manifest;
}

[[nodiscard]] rpc::ShardTransferManifest prepareRequest(const ShardTransferManifest& manifest) {
    rpc::ShardTransferManifest request;
    request.set_migration_id(manifest.id);
    request.set_source_shard(manifest.source);
    request.set_target_shard(manifest.target);
    request.set_range_start(manifest.range_start);
    request.set_range_end(manifest.range_end);
    request.set_topology_epoch(manifest.epoch);
    request.set_snapshot_watermark(manifest.watermark);
    request.set_chunks(manifest.chunks);
    request.set_bytes(manifest.bytes);
    request.set_sha256(manifest.sha256);
    return request;
}

[[nodiscard]] rpc::ShardTransferChunk chunk(
    const std::string& id, std::uint64_t index, std::string_view payload) {
    rpc::ShardTransferChunk request;
    request.set_migration_id(id);
    request.set_index(index);
    request.set_payload(payload.data(), payload.size());
    request.set_sha256(sha256(payload));
    return request;
}

class FixtureDirectory {
public:
    FixtureDirectory() {
        for (int attempt = 0; attempt < 100; ++attempt) {
            path = ".shard-transfer-grpc-" + std::to_string(::getpid()) + "-" +
                   std::to_string(attempt);
            if (::mkdir(path.c_str(), 0700) == 0) {
                return;
            }
            if (errno != EEXIST) {
                throw std::system_error(errno, std::generic_category(), "mkdir test directory");
            }
        }
        throw std::runtime_error("could not allocate shard transfer test directory");
    }
    FixtureDirectory(const FixtureDirectory&) = delete;
    FixtureDirectory& operator=(const FixtureDirectory&) = delete;
    ~FixtureDirectory() {
        std::error_code error;
        fs::remove_all(path, error);
    }

    fs::path path;
};

void writePem(const fs::path& path, const std::string& pem) {
    std::ofstream out;
    out.exceptions(std::ios::badbit | std::ios::failbit);
    out.open(path);
    out << pem;
    out.close();
}

[[nodiscard]] int loopbackPort() {
    // ShardRPCServer has no selected-port accessor. Hold an ephemeral port
    // just long enough for the OS to choose it, then hand it to the receiver.
    const int fd = ::socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if (fd < 0) {
        throw std::system_error(errno, std::generic_category(), "socket");
    }
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    socklen_t len = sizeof(addr);
    const int rc = ::bind(fd, reinterpret_cast<const sockaddr*>(&addr), len) == 0
        ? ::getsockname(fd, reinterpret_cast<sockaddr*>(&addr), &len)
        : -1;
    const int error = errno;
    ::close(fd);
    if (rc != 0) {
        throw std::system_error(error, std::generic_category(), "assign loopback port");
    }
    return ntohs(addr.sin_port);
}

class ReceiverProcess {
public:
    ReceiverProcess() = default;
    ReceiverProcess(const ReceiverProcess&) = delete;
    ReceiverProcess& operator=(const ReceiverProcess&) = delete;
    ~ReceiverProcess() {
        if (pid_ > 0) {
            (void)crash();
        }
    }

    [[nodiscard]] std::string start(const fs::path& root, bool seed, bool advance_epoch = false) {
        const auto port = std::to_string(loopbackPort());
        const auto parent = std::to_string(::getpid());
        const std::string path = root.string();
        // Exec instead of forking a live gRPC runtime on receiver restart.
        char* const argv[] = {
            const_cast<char*>("/proc/self/exe"),
            const_cast<char*>(kReceiverMode),
            const_cast<char*>(path.c_str()),
            const_cast<char*>(port.c_str()),
            const_cast<char*>(seed ? "seed" : (advance_epoch ? "advance-epoch" : "restart")),
            const_cast<char*>(parent.c_str()),
            nullptr,
        };
        const int error = ::posix_spawn(&pid_, argv[0], nullptr, nullptr, argv, environ);
        if (error != 0) {
            pid_ = -1;
            throw std::system_error(error, std::generic_category(), "spawn shard receiver");
        }
        return "localhost:" + port;
    }

    [[nodiscard]] bool crash() {
        const int killed = ::kill(pid_, SIGKILL);
        int status = 0;
        pid_t finished;
        do {
            finished = ::waitpid(pid_, &status, 0);
        } while (finished < 0 && errno == EINTR);
        pid_ = -1;
        return killed == 0 && finished > 0 && WIFSIGNALED(status) && WTERMSIG(status) == SIGKILL;
    }

private:
    pid_t pid_ = -1;
};

[[nodiscard]] int runReceiver(const fs::path& root, const std::string& port,
                              std::string_view mode, pid_t parent) {
    // Do not leave a receiver behind if CTest times out and kills the sender.
    if (::prctl(PR_SET_PDEATHSIG, SIGKILL) != 0 || ::getppid() != parent) {
        return EXIT_FAILURE;
    }
    const auto epoch = kEpoch + (mode == "advance-epoch" ? 1u : 0u);
    auto store = std::make_shared<ShardTransferStore>(
        (root / "stage").string(), kTarget, [epoch](const ShardTransferManifest& manifest) {
            return manifest.source == kSource && manifest.target == kTarget &&
                   manifest.epoch == epoch && manifest.range_start == 100 &&
                   manifest.range_end == 200;
        });
    if (mode == "seed") {
        for (const char* id : {"resume", "abort", "bad-digest", "assigned-transfer-id"}) {
            store->assign(assignment(id), kSource);
        }
    }
    ShardRPCServer::Config config;
    config.listen_address = "127.0.0.1:" + port;
    config.tls_cert_path = (root / "receiver-cert.pem").string();
    config.tls_key_path = (root / "receiver-key.pem").string();
    config.tls_ca_cert_path = (root / "trusted-clients.pem").string();
    config.transfer_identity_bindings = {
        {"x509_subject_alternative_name", kSourceIdentity, kSource},
    };
    config.transfer_store = std::move(store);
    ShardRPCServer server(config);
    if (!server.start()) {
        std::cerr << "shard transfer receiver failed to start\n";
        return EXIT_FAILURE;
    }
    server.wait();
    return EXIT_SUCCESS;
}

class RaftPeerProcess {
public:
    RaftPeerProcess() = default;
    RaftPeerProcess(const RaftPeerProcess&) = delete;
    RaftPeerProcess& operator=(const RaftPeerProcess&) = delete;
    ~RaftPeerProcess() {
        if (pid_ > 0) {
            (void)crash();
        }
    }

    [[nodiscard]] std::string start(
        const fs::path& root, const std::string& node_id,
        const std::vector<std::string>& members,
        const std::map<std::string, std::string>& peer_addresses = {},
        std::optional<std::string> forced_port = std::nullopt) {
        const auto port = forced_port.has_value()
                              ? *forced_port
                              : std::to_string(loopbackPort());
        const auto parent = std::to_string(::getpid());
        const auto path = root.string();
        std::string member_list;
        std::string address_list;
        for (const auto& member : members) {
            if (!member_list.empty()) {
                member_list.push_back(',');
            }
            member_list += member;

            const auto address = peer_addresses.find(member);
            if (address != peer_addresses.end()) {
                if (!address_list.empty()) {
                    address_list.push_back(';');
                }
                address_list += member + "=" + address->second;
            }
        }

        // Exec instead of forking a live gRPC runtime on restart.
        char* const argv[] = {
            const_cast<char*>("/proc/self/exe"),
            const_cast<char*>(kRaftReceiverMode),
            const_cast<char*>(path.c_str()),
            const_cast<char*>(port.c_str()),
            const_cast<char*>(node_id.c_str()),
            const_cast<char*>(member_list.c_str()),
            const_cast<char*>(address_list.c_str()),
            const_cast<char*>(parent.c_str()),
            nullptr,
        };
        const int error = ::posix_spawn(&pid_, argv[0], nullptr, nullptr, argv, environ);
        if (error != 0) {
            pid_ = -1;
            throw std::system_error(error, std::generic_category(), "spawn Raft peer");
        }
        return "localhost:" + port;
    }

    [[nodiscard]] bool crash() {
        if (pid_ <= 0) {
            return false;
        }
        const int killed = ::kill(pid_, SIGKILL);
        int status = 0;
        pid_t finished;
        do {
            finished = ::waitpid(pid_, &status, 0);
        } while (finished < 0 && errno == EINTR);
        pid_ = -1;
        return killed == 0 && finished > 0 && WIFSIGNALED(status) &&
               WTERMSIG(status) == SIGKILL;
    }

private:
    pid_t pid_ = -1;
};

class RaftLeaderProcess {
public:
    RaftLeaderProcess() = default;
    RaftLeaderProcess(const RaftLeaderProcess&) = delete;
    RaftLeaderProcess& operator=(const RaftLeaderProcess&) = delete;
    ~RaftLeaderProcess() {
        if (pid_ > 0) {
            (void)crash();
        }
    }

    [[nodiscard]] std::string start(
        const fs::path& root, const std::string& leader_port,
        const std::vector<std::string>& members,
        const std::map<std::string, std::string>& peer_addresses,
        bool commit_on_start = true) {
        const auto parent = std::to_string(::getpid());
        const auto path = root.string();
        const auto startup_mode = commit_on_start ? "commit" : "recover";
        std::string member_list;
        std::string address_list;
        for (const auto& member : members) {
            if (!member_list.empty()) {
                member_list.push_back(',');
            }
            member_list += member;

            const auto address = peer_addresses.find(member);
            if (address != peer_addresses.end()) {
                if (!address_list.empty()) {
                    address_list.push_back(';');
                }
                address_list += member + "=" + address->second;
            }
        }

        char* const argv[] = {
            const_cast<char*>("/proc/self/exe"),
            const_cast<char*>(kRaftLeaderMode),
            const_cast<char*>(path.c_str()),
            const_cast<char*>(leader_port.c_str()),
            const_cast<char*>(member_list.c_str()),
            const_cast<char*>(address_list.c_str()),
            const_cast<char*>(parent.c_str()),
            const_cast<char*>(startup_mode),
            nullptr,
        };
        const int error = ::posix_spawn(&pid_, argv[0], nullptr, nullptr, argv, environ);
        if (error != 0) {
            pid_ = -1;
            throw std::system_error(error, std::generic_category(), "spawn Raft leader");
        }
        return "localhost:" + leader_port;
    }

    [[nodiscard]] bool crash() {
        if (pid_ <= 0) {
            return false;
        }
        const int killed = ::kill(pid_, SIGKILL);
        int status = 0;
        pid_t finished;
        do {
            finished = ::waitpid(pid_, &status, 0);
        } while (finished < 0 && errno == EINTR);
        pid_ = -1;
        return killed == 0 && finished > 0 && WIFSIGNALED(status) &&
               WTERMSIG(status) == SIGKILL;
    }

private:
    pid_t pid_ = -1;
};

[[nodiscard]] int runRaftReceiver(
    const fs::path& root, const std::string& port, const std::string& node_id,
    const std::vector<std::string>& members, const std::string& address_list,
    pid_t parent) {
    // Do not leave a peer behind if the test runner kills the parent.
    if (::prctl(PR_SET_PDEATHSIG, SIGKILL) != 0 || ::getppid() != parent) {
        return EXIT_FAILURE;
    }

    std::map<std::string, std::string> peer_addresses;
    std::size_t start = 0;
    while (start < address_list.size()) {
        const auto separator = address_list.find(';', start);
        const auto item = address_list.substr(
            start, separator == std::string::npos
                       ? std::string::npos
                       : separator - start);
        const auto equals = item.find('=');
        if (equals == std::string::npos) {
            return EXIT_FAILURE;
        }
        peer_addresses.emplace(item.substr(0, equals), item.substr(equals + 1));
        if (separator == std::string::npos) {
            break;
        }
        start = separator + 1;
    }

    if (peer_addresses.empty()) {
        themisdb::sharding::RaftConsensus::Config raft_config;
        raft_config.raft_config.node_id = node_id;
        raft_config.raft_config.cluster_members = members;
        raft_config.raft_config.persistence_path =
            (root / (node_id + ".journal")).string();
        raft_config.raft_config.election_timeout_min_ms = 1000;
        raft_config.raft_config.election_timeout_max_ms = 1200;
        auto raft =
            std::make_shared<themisdb::sharding::RaftConsensus>(raft_config);

        ShardRPCServer::Config server_config;
        server_config.listen_address = "127.0.0.1:" + port;
        server_config.tls_cert_path = (root / (node_id + "-cert.pem")).string();
        server_config.tls_key_path = (root / (node_id + "-key.pem")).string();
        server_config.tls_ca_cert_path = (root / "trusted-clients.pem").string();
        for (const auto& member : members) {
            server_config.raft_identity_bindings.push_back(
                {"x509_subject_alternative_name", member + ".test", member});
        }
        server_config.raft_group_resolver =
            [raft](const std::string& group_id)
                -> std::shared_ptr<themisdb::sharding::RaftConsensus> {
                if (group_id == kRaftGroup) {
                    return raft;
                }
                return {};
            };
        ShardRPCServer server(server_config);
        if (!server.start()) {
            std::cerr << "Raft peer failed to start\n";
            return EXIT_FAILURE;
        }
        raft->start();
        server.wait();
        return EXIT_SUCCESS;
    }

    themisdb::sharding::RaftShardManager::Config manager_config =
        raftManagerConfig(root, members, peer_addresses, node_id);
    manager_config.raft_config.raft_config.election_timeout_min_ms = 3000;
    manager_config.raft_config.raft_config.election_timeout_max_ms = 3500;
    themisdb::sharding::RaftShardManager manager(manager_config);
    if (!manager.initializeShard(kRaftGroup, members)) {
        return EXIT_FAILURE;
    }
    const auto raft = manager.getRaftInstance(kRaftGroup);
    if (!raft) {
        return EXIT_FAILURE;
    }

    ShardRPCServer::Config server_config;
    server_config.listen_address = "127.0.0.1:" + port;
    server_config.tls_cert_path = (root / (node_id + "-cert.pem")).string();
    server_config.tls_key_path = (root / (node_id + "-key.pem")).string();
    server_config.tls_ca_cert_path = (root / "trusted-clients.pem").string();
    for (const auto& member : members) {
        server_config.raft_identity_bindings.push_back(
            {"x509_subject_alternative_name", member + ".test", member});
    }
    server_config.raft_group_resolver =
        [raft](const std::string& group_id)
            -> std::shared_ptr<themisdb::sharding::RaftConsensus> {
            if (group_id == kRaftGroup) {
                return raft;
            }
            return {};
        };

    ShardRPCServer server(server_config);
    if (!server.start()) {
        std::cerr << "Raft peer failed to start\n";
        return EXIT_FAILURE;
    }
    if (!manager.startShard(kRaftGroup)) {
        std::cerr << "Raft peer failed to start its consensus group\n";
        return EXIT_FAILURE;
    }
    const auto ready = root / (node_id + ".ready");
    const auto state_path = root / (node_id + ".state");
    while (true) {
        std::ofstream state_marker(state_path);
        if (!state_marker) {
            std::cerr << "Raft peer failed to write its state marker\n";
            return EXIT_FAILURE;
        }
        state_marker << raft->getCurrentTerm() << '\n'
                     << raft->getRaftState().getLog().getCommitIndex() << '\n';
        state_marker.close();
        if (!state_marker) {
            std::cerr << "Raft peer failed to persist its state marker\n";
            return EXIT_FAILURE;
        }

        if (raft->isLeader()) {
            std::ofstream marker(ready);
            marker << raft->getCurrentTerm() << '\n'
                   << raft->getRaftState().getLog().getCommitIndex() << '\n';
            marker.close();
        }
        std::this_thread::sleep_for(20ms);
    }
}

[[nodiscard]] int runRaftLeader(
    const fs::path& root, const std::string& port,
    const std::vector<std::string>& members,
    const std::string& address_list, pid_t parent, bool commit_on_start) {
    if (::prctl(PR_SET_PDEATHSIG, SIGKILL) != 0 || ::getppid() != parent) {
        return EXIT_FAILURE;
    }

    std::map<std::string, std::string> peer_addresses;
    std::size_t start = 0;
    while (start < address_list.size()) {
        const auto separator = address_list.find(';', start);
        const auto item = address_list.substr(
            start, separator == std::string::npos
                       ? std::string::npos
                       : separator - start);
        const auto equals = item.find('=');
        if (equals == std::string::npos) {
            return EXIT_FAILURE;
        }
        peer_addresses.emplace(item.substr(0, equals), item.substr(equals + 1));
        if (separator == std::string::npos) {
            break;
        }
        start = separator + 1;
    }

    ShardRPCServer::Config server_config;
    server_config.listen_address = "127.0.0.1:" + port;
    server_config.tls_cert_path = (root / "node1-cert.pem").string();
    server_config.tls_key_path = (root / "node1-key.pem").string();
    server_config.tls_ca_cert_path = (root / "trusted-clients.pem").string();
    for (const auto& member : members) {
        server_config.raft_identity_bindings.push_back(
            {"x509_subject_alternative_name", member + ".test", member});
    }

    themisdb::sharding::RaftShardManager::Config manager_config =
        raftManagerConfig(root, members, peer_addresses);
    themisdb::sharding::RaftShardManager manager(manager_config);
    auto raft_group_resolver =
        [&manager](const std::string& group_id)
            -> std::shared_ptr<themisdb::sharding::RaftConsensus> {
            if (group_id == kRaftGroup) {
                return manager.getRaftInstance(kRaftGroup);
            }
            return {};
        };
    server_config.raft_group_resolver = std::move(raft_group_resolver);

    ShardRPCServer server(server_config);
    if (!server.start()) {
        return EXIT_FAILURE;
    }
    if (!manager.initializeShard(kRaftGroup, members)) {
        return EXIT_FAILURE;
    }

    const auto leader = manager.getRaftInstance(kRaftGroup);
    if (!leader) {
        return EXIT_FAILURE;
    }
    if (!manager.startShard(kRaftGroup)) {
        return EXIT_FAILURE;
    }

    const auto ready = root / "node1.ready";
    std::error_code error;
    fs::remove(ready, error);
    if (commit_on_start) {
        const auto deadline = std::chrono::steady_clock::now() + 10s;
        while (!leader->isLeader() && std::chrono::steady_clock::now() < deadline) {
            std::this_thread::sleep_for(10ms);
        }
        if (!leader->isLeader() ||
            !manager.proposeWrite(kRaftGroup, "leader-process-durable-entry").get()) {
            return EXIT_FAILURE;
        }
    }
    std::ofstream marker(ready);
    marker << leader->getCurrentTerm() << '\n'
           << leader->getRaftState().getLog().getCommitIndex() << '\n';
    marker.close();
    if (!marker) {
        return EXIT_FAILURE;
    }

    server.wait();
    return EXIT_SUCCESS;
}

[[nodiscard]] std::shared_ptr<grpc::Channel> connect(
    const std::string& address, const std::string& server_cert,
    const themis::tests::PemMaterial& client) {
    grpc::SslCredentialsOptions tls;
    tls.pem_root_certs = server_cert;
    tls.pem_cert_chain = client.certificate_pem;
    tls.pem_private_key = client.private_key_pem;
    grpc::ChannelArguments args;
    args.SetInt("grpc.enable_http_proxy", 0);
    // The address and server SAN are both localhost; no TLS name override.
    return grpc::CreateCustomChannel(address, grpc::SslCredentials(tls), args);
}

[[nodiscard]] std::shared_ptr<grpc::Channel> connectRaftPeer(
    const std::string& address, const themis::tests::PemMaterial& server,
    const themis::tests::PemMaterial& client, const std::string& peer_node_id) {
    grpc::SslCredentialsOptions tls;
    tls.pem_root_certs = server.certificate_pem;
    tls.pem_cert_chain = client.certificate_pem;
    tls.pem_private_key = client.private_key_pem;
    grpc::ChannelArguments args;
    args.SetInt("grpc.enable_http_proxy", 0);
    args.SetSslTargetNameOverride(peer_node_id + ".test");
    return grpc::CreateCustomChannel(address, grpc::SslCredentials(tls), args);
}

[[nodiscard]] auto deadline() {
    return std::chrono::system_clock::now() + 5s;
}

using RaftCertificates = std::map<std::string, themis::tests::PemMaterial>;

[[nodiscard]] RaftCertificates writeRaftCertificates(
    const fs::path& root, const std::vector<std::string>& members) {
    RaftCertificates certificates;
    std::string trusted_clients;
    std::size_t serial = 1;
    for (const auto& member : members) {
        const auto identity = member + ".test";
        auto material = themis::tests::generateSelfSignedPemMaterial(
            std::to_string(serial++), identity, {identity});
        writePem(root / (member + "-cert.pem"), material.certificate_pem);
        writePem(root / (member + "-key.pem"), material.private_key_pem);
        trusted_clients += material.certificate_pem;
        certificates.emplace(member, std::move(material));
    }
    writePem(root / "trusted-clients.pem", trusted_clients);
    return certificates;
}

[[nodiscard]] bool waitForFile(const fs::path& path, std::chrono::milliseconds timeout) {
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    std::error_code error;
    while (std::chrono::steady_clock::now() < deadline) {
        if (fs::exists(path, error) && !error) {
            return true;
        }
        std::this_thread::sleep_for(10ms);
    }
    return false;
}

[[nodiscard]] std::optional<std::pair<std::uint64_t, std::uint64_t>>
readRaftReadyMarker(const fs::path& path) {
    std::ifstream marker(path);
    std::uint64_t term = 0;
    std::uint64_t commit_index = 0;
    if (!(marker >> term >> commit_index)) {
        return std::nullopt;
    }
    return std::pair<std::uint64_t, std::uint64_t>{term, commit_index};
}

[[nodiscard]] std::optional<std::pair<std::uint64_t, std::uint64_t>>
waitForRaftReadyMarker(const fs::path& path, std::chrono::milliseconds timeout) {
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < deadline) {
        if (const auto marker = readRaftReadyMarker(path)) {
            return marker;
        }
        std::this_thread::sleep_for(10ms);
    }
    return std::nullopt;
}

[[nodiscard]] bool waitForRaftCommitIndex(
    const std::vector<fs::path>& paths, std::uint64_t expected_commit_index,
    std::chrono::milliseconds timeout) {
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < deadline) {
        for (const auto& path : paths) {
            if (const auto marker = readRaftReadyMarker(path);
                marker && marker->second >= expected_commit_index) {
                return true;
            }
        }
        std::this_thread::sleep_for(10ms);
    }
    return false;
}

[[nodiscard]] ShardRPCClient::Config raftClientConfig(
    const fs::path& root, const std::string& endpoint,
    const std::string& local_node_id, const std::string& peer_node_id) {
    ShardRPCClient::Config config;
    config.endpoint = endpoint;
    config.timeout_ms = 1000;
    config.tls_cert_path = (root / (local_node_id + "-cert.pem")).string();
    config.tls_key_path = (root / (local_node_id + "-key.pem")).string();
    config.tls_ca_cert_path = (root / (peer_node_id + "-cert.pem")).string();
    config.local_node_id = local_node_id;
    config.expected_peer_node_id = peer_node_id;
    config.expected_peer_identity_property = "x509_subject_alternative_name";
    config.expected_peer_identity_value = peer_node_id + ".test";
    return config;
}

[[nodiscard]] bool waitForRaftPeer(
    const std::string& address, const RaftCertificates& certificates,
    const std::string& local_node_id, const std::string& peer_node_id) {
    auto channel = connectRaftPeer(address, certificates.at(peer_node_id),
                                   certificates.at(local_node_id), peer_node_id);
    return channel->WaitForConnected(deadline());
}

[[nodiscard]] themisdb::sharding::RaftShardManager::Config raftManagerConfig(
    const fs::path& root, const std::vector<std::string>& members,
    const std::map<std::string, std::string>& addresses,
    const std::string& local_node_id) {
    themisdb::sharding::RaftShardManager::Config config;
    config.raft_config.raft_config.node_id = local_node_id;
    config.raft_config.raft_config.persistence_path =
        (root / (local_node_id + ".journal")).string();
    config.enable_auto_start = false;
    config.raft_config.raft_config.election_timeout_min_ms = 100;
    config.raft_config.raft_config.election_timeout_max_ms = 150;
    config.raft_config.raft_config.heartbeat_interval_ms = 50;
    config.replication_factor = members.size();
    config.raft_peer_config_resolver =
        [root, addresses, local_node_id](const std::string& peer_node_id)
            -> std::optional<ShardRPCClient::Config> {
            const auto address = addresses.find(peer_node_id);
            if (address == addresses.end()) {
                return std::nullopt;
            }
            return raftClientConfig(root, address->second, local_node_id, peer_node_id);
        };
    return config;
}

[[nodiscard]] grpc::Status prepare(
    rpc::ShardService::Stub& stub, const rpc::ShardTransferManifest& request,
    rpc::ShardTransferCheckpoint& checkpoint) {
    grpc::ClientContext context;
    context.set_deadline(deadline());
    return stub.PrepareShardTransfer(&context, request, &checkpoint);
}

TEST(ShardFragmentGrpc, FetchesBoundedManifestVerifiedPayloadOverMtls) {
    const FixtureDirectory root;
    const std::vector<std::string> members{"node1", "node2", "node3"};
    writeRaftCertificates(root.path, members);
    const std::string payload("tensor\0fragment", 15);
    const auto digest = sha256(payload);

    ShardRPCServer::Config server_config;
    server_config.listen_address =
        "127.0.0.1:" + std::to_string(loopbackPort());
    server_config.tls_cert_path = (root.path / "node2-cert.pem").string();
    server_config.tls_key_path = (root.path / "node2-key.pem").string();
    server_config.tls_ca_cert_path =
        (root.path / "trusted-clients.pem").string();
    server_config.tensor_fragment_shard_id = "shard-a";
    server_config.tensor_fragment_identity_bindings.push_back(
        {"x509_subject_alternative_name", "node1.test", "principal-a"});
    server_config.tensor_fragment_reader =
        [payload, digest](const std::string& principal,
                         const std::string& shard_id,
                         const std::string& artifact_id,
                         uint64_t max_bytes)
            -> std::optional<themis::sharding::TensorFragmentPayload> {
        if (principal != "principal-a" || shard_id != "shard-a" ||
            (artifact_id != "artifact-a" &&
             artifact_id != "corrupt-artifact")) {
            return std::nullopt;
        }
        (void)max_bytes;
        auto stored_bytes = payload;
        if (artifact_id == "corrupt-artifact") {
            stored_bytes[0] = 'X';
        }
        return themis::sharding::TensorFragmentPayload{
            std::vector<uint8_t>(stored_bytes.begin(), stored_bytes.end()), digest};
    };

    ShardRPCServer server(server_config);
    ASSERT_TRUE(server.start());
    const auto endpoint = server_config.listen_address;

    auto client_config = raftClientConfig(root.path, endpoint, "node1", "node2");
    ShardRPCClient client(client_config);
    const auto fetched = client.fetchTensorFragment(
        "node2", "shard-a", "artifact-a", payload.size(), 1000, "corr-a");
    EXPECT_EQ(std::string(fetched.bytes.begin(), fetched.bytes.end()), payload);
    EXPECT_EQ(fetched.sha256, digest);

    auto undersized = ShardRPCClient(client_config);
    EXPECT_THROW(undersized.fetchTensorFragment(
                     "node2", "shard-a", "artifact-a", payload.size() - 1,
                     1000, "corr-a"),
                 std::runtime_error);

    auto unauthorized_config =
        raftClientConfig(root.path, endpoint, "node3", "node2");
    ShardRPCClient unauthorized_client(unauthorized_config);
    EXPECT_THROW(unauthorized_client.fetchTensorFragment(
                     "node2", "shard-a", "artifact-a", payload.size(), 1000,
                     "corr-a"),
                 std::runtime_error);

#if defined(THEMIS_TEST_HAS_DISTRIBUTED_TENSOR_RPC_ADAPTER)
    auto adapter_client = std::make_shared<ShardRPCClient>(client_config);
    themis::distributed_tensor::ShardRPCFragmentFetcher fetcher(
        {{"node2", adapter_client}});
    themis::distributed_tensor::FragmentLoadRequest request;
    request.target_node_id = "node2";
    request.shard_id = "shard-a";
    request.artifact_id = "artifact-a";
    request.expected_content_hash = digest;
    request.max_response_bytes = payload.size();
    request.timeout_ms = 1000;
    const auto result = fetcher.fetchFragment(request, "corr-a");
    ASSERT_TRUE(result.success) << result.error_reason;
    EXPECT_EQ(std::string(result.fragment_data.begin(), result.fragment_data.end()),
              payload);

    request.artifact_id = "corrupt-artifact";
    const auto altered_bytes = fetcher.fetchFragment(request, "corr-a");
    EXPECT_FALSE(altered_bytes.success);
    EXPECT_TRUE(altered_bytes.fragment_data.empty());

    request.artifact_id = "artifact-a";
    request.expected_content_hash = sha256("wrong");
    const auto corrupt = fetcher.fetchFragment(request, "corr-a");
    EXPECT_FALSE(corrupt.success);
    EXPECT_TRUE(corrupt.fragment_data.empty());

    themis::distributed_tensor::ArtifactManifest manifest(
        "artifact-a", themis::distributed_tensor::ArtifactClass::PRIMARY);
    manifest.set_version("v1");
    manifest.set_content_hash(digest);
    themis::distributed_tensor::ShardPlacement placement;
    placement.shard_id = "shard-a";
    placement.node_id = "node2";
    placement.shard_content_hash = digest;
    placement.shard_size_bytes = payload.size();
    manifest.add_shard_placement(std::move(placement));
    themis::distributed_tensor::DistributedTensorPlanner planner;
    themis::tensor::ShardSummary summary;
    summary.shard_id = "shard-a";
    summary.shard_relevance = 0.95f;
    auto plan = planner.buildRetrievalPlan(
        std::vector<themis::tensor::ShardSummary>{summary}, manifest,
        "query", "corr-a");
    ASSERT_EQ(plan.fragment_requests.size(), 1u);
    EXPECT_EQ(plan.fragment_requests.front().target_node_id, "node2");
    EXPECT_EQ(plan.fragment_requests.front().expected_content_hash, digest);
    plan.fragment_results.push_back(corrupt);
    auto planner_config = planner.config();
    planner_config.strict_graph_validation = false;
    planner.setConfig(planner_config);
    EXPECT_FALSE(planner.isValidFinalPlan(plan));
    plan.fragment_results.front() = result;
    EXPECT_TRUE(planner.isValidFinalPlan(plan));
#endif
}

#if defined(THEMIS_TEST_HAS_DISTRIBUTED_TENSOR_RPC_ADAPTER)
TEST(ShardFragmentGrpc, StoredProjectionFlowsThroughMtlsAndManifestBoundPlanner) {
    namespace dt = themis::distributed_tensor;
    namespace tensor = themis::tensor;
    const FixtureDirectory root;
    writeRaftCertificates(root.path, {"node1", "node2", "node3"});

    themis::RocksDBWrapper::Config db_config;
    db_config.db_path = (root.path / "fragment-db").string();
    auto db = std::make_shared<themis::RocksDBWrapper>(db_config);
    ASSERT_TRUE(db->open());
    auto store = std::make_shared<themis::sharding::ShardArtifactFragmentStore>(db);

    themis::storage::TTTrain train;
    themis::storage::TTCore core;
    core.r_left = 1;
    core.n = 2;
    core.r_right = 1;
    core.data = {1.0f, 2.0f};
    train.cores.push_back(core);
    train.mode_sizes = {2};
    tensor::TensorFingerprintGraph graph_source;
    const std::string query_key = "__adapters__:tenant-a:Alpha:Alpha";
    ASSERT_TRUE(graph_source.addAdapter(query_key, train, "Alpha", "Alpha", "tenant-a"));
    ASSERT_TRUE(graph_source.addAdapter("__adapters__:tenant-a:alpha:model-a", train,
                                        "alpha", "model-a", "tenant-a"));
    const auto source_snapshot = graph_source.snapshotSimilarForTenant(query_key, "tenant-a", 1);
    ASSERT_TRUE(source_snapshot);
    ASSERT_EQ(source_snapshot->candidates.size(), 1u);
    tensor::TensorLayerSummary summary;
    summary.scope_key = query_key;
    summary.layer_kind = tensor::TensorLayerKind::FingerprintSummary;
    summary.similar_adapters = source_snapshot->candidates;
    summary.candidate_count = summary.similar_adapters.size();
    const auto payload = dt::GraphSummaryProjectionCodec::encode(summary);
    const auto digest = sha256(std::string_view(
        reinterpret_cast<const char*>(payload.data()), payload.size()));
    auto publisher = std::make_unique<dt::LocalProjectionPublisher>(
        db, "shard-a", "node2",
        themis::sharding::ShardArtifactFragmentStore::kDefaultMaxFragmentBytes);
    auto manifest = publisher->publishFromGraph(
        graph_source, "principal-a", "tenant-a", "source-a", "Alpha", "Alpha", 1);
    EXPECT_THROW((void)publisher->publishFromGraph(
        graph_source, "principal-a", "tenant-b", "source-a", "Alpha", "Alpha", 1),
        std::runtime_error);
    EXPECT_THROW((void)publisher->publishFromGraph(
        graph_source, "principal-a", "tenant-a", "source-a", "Alpha", "missing", 1),
        std::runtime_error);
    EXPECT_THROW((void)publisher->publishFromGraph(
        graph_source, "principal-a", "tenant-a", "source-a", "Alpha", "Alpha", 0),
        std::invalid_argument);
    tensor::TensorFingerprintGraph invalid_query;
    ASSERT_TRUE(invalid_query.addAdapter(query_key, train, "other", "model-a", "tenant-a"));
    EXPECT_THROW((void)publisher->publishFromGraph(
        invalid_query, "principal-a", "tenant-a", "source-a", "Alpha", "Alpha", 1),
        std::runtime_error);
    ASSERT_TRUE(graph_source.addAdapter("__adapters__:tenant-b:foreign:model", train,
                                        "foreign", "model", "tenant-a"));
    EXPECT_THROW((void)publisher->publishFromGraph(
        graph_source, "principal-a", "tenant-a", "source-a", "Alpha", "Alpha", 2),
        std::invalid_argument);
    ASSERT_TRUE(graph_source.removeAdapter("__adapters__:tenant-b:foreign:model"));
    EXPECT_EQ(publisher->publish("principal-a", "tenant-a", "source-a", summary).artifact_id(),
              manifest.artifact_id());
    EXPECT_NE(publisher->publish("principal-a", "tenant-a", "source-b", summary).artifact_id(),
              manifest.artifact_id());
    EXPECT_THROW((void)publisher->publish("principal-a", "tenant-b", "source-a", summary),
                 std::invalid_argument);
    auto foreign_summary = summary;
    foreign_summary.similar_adapters.front().adapter_key =
        "__adapters__:tenant-b:alpha:peer";
    EXPECT_THROW((void)publisher->publish(
        "principal-a", "tenant-a", "source-a", foreign_summary), std::invalid_argument);
    auto inconsistent_summary = summary;
    inconsistent_summary.similar_adapters.front().domain = "Other";
    EXPECT_THROW((void)publisher->publish(
        "principal-a", "tenant-a", "source-a", inconsistent_summary), std::invalid_argument);
    publisher.reset();
    store.reset();
    db->close();
    ASSERT_TRUE(db->open());
    store = std::make_shared<themis::sharding::ShardArtifactFragmentStore>(db);
    publisher = std::make_unique<dt::LocalProjectionPublisher>(
        db, "shard-a", "node2",
        themis::sharding::ShardArtifactFragmentStore::kDefaultMaxFragmentBytes);
    auto restored_manifest = publisher->load("principal-a", manifest.artifact_id());
    ASSERT_TRUE(restored_manifest);
    EXPECT_EQ(restored_manifest->artifact_id(), manifest.artifact_id());
    EXPECT_FALSE(publisher->load("principal-b", manifest.artifact_id()));
    dt::LocalProjectionPublisher smaller_limit(db, "shard-a", "node2", 1);
    EXPECT_THROW((void)smaller_limit.load("principal-a", manifest.artifact_id()),
                 std::runtime_error);
    manifest = std::move(*restored_manifest);

    ShardRPCServer::Config config;
    config.listen_address = "127.0.0.1:" + std::to_string(loopbackPort());
    config.tls_cert_path = (root.path / "node2-cert.pem").string();
    config.tls_key_path = (root.path / "node2-key.pem").string();
    config.tls_ca_cert_path = (root.path / "trusted-clients.pem").string();
    config.tensor_fragment_shard_id = "shard-a";
    config.tensor_fragment_identity_bindings.push_back(
        {"x509_subject_alternative_name", "node1.test", "principal-a"});
    config.tensor_fragment_reader =
        [store](const std::string& principal, const std::string& shard,
                const std::string& artifact, uint64_t max_bytes) {
            return store->read(principal, shard, artifact, max_bytes);
        };
    ShardRPCServer server(config);
    ASSERT_TRUE(server.start());
    const auto client_config =
        raftClientConfig(root.path, config.listen_address, "node1", "node2");
    auto rpc_client = std::make_shared<ShardRPCClient>(client_config);
    auto remote_manifest = dt::LocalProjectionPublisher::loadRemote(
        *rpc_client, "principal-a", "tenant-a", "shard-a", "node2",
        manifest.artifact_id(), payload.size(), 1000, "manifest-a");
    EXPECT_EQ(remote_manifest.artifact_id(), manifest.artifact_id());
    EXPECT_THROW((void)dt::LocalProjectionPublisher::loadRemote(
        *rpc_client, "principal-a", "tenant-b", "shard-a", "node2",
        manifest.artifact_id(), payload.size(), 1000, "manifest-b"),
        std::runtime_error);
    EXPECT_THROW((void)dt::LocalProjectionPublisher::loadRemote(
        *rpc_client, "principal-b", "tenant-a", "shard-a", "node2",
        manifest.artifact_id(), payload.size(), 1000, "manifest-c"),
        std::runtime_error);
    EXPECT_THROW((void)dt::LocalProjectionPublisher::loadRemote(
        *rpc_client, "principal-a", "tenant-a", "shard-a", "node2",
        manifest.artifact_id(), 1, 1000, "manifest-d"),
        std::runtime_error);
    const std::string invalid_id = "tspj1-" + std::string(64, '0');
    ASSERT_EQ(store->put("principal-a", "shard-a", "tspjm1-" + invalid_id,
                         std::vector<uint8_t>{'{', '}'}),
              themis::sharding::ShardArtifactFragmentStore::PutResult::Stored);
    EXPECT_THROW((void)dt::LocalProjectionPublisher::loadRemote(
        *rpc_client, "principal-a", "tenant-a", "shard-a", "node2",
        invalid_id, payload.size(), 1000, "manifest-invalid"),
        std::runtime_error);
    const auto foreign_payload = dt::GraphSummaryProjectionCodec::encode(foreign_summary);
    const auto foreign_digest = sha256(std::string_view(
        reinterpret_cast<const char*>(foreign_payload.data()), foreign_payload.size()));
    std::string id_input = "themis.tensor.local-projection/v1";
    for (const auto& field : {std::string("principal-a"), std::string("tenant-a"),
                              std::string("source-foreign"), std::string("shard-a"),
                              std::string("node2"), foreign_summary.scope_key,
                              foreign_digest}) {
        id_input += std::to_string(field.size()) + ":" + field;
    }
    const auto foreign_id = "tspj1-" + sha256(id_input);
    const nlohmann::json foreign_record = {
        {"format", 1}, {"principal", "principal-a"}, {"tenant", "tenant-a"},
        {"source", "source-foreign"}, {"shard", "shard-a"}, {"node", "node2"},
        {"scope", foreign_summary.scope_key}, {"digest", foreign_digest},
        {"size", foreign_payload.size()}, {"artifact", foreign_id}};
    const auto foreign_record_text = foreign_record.dump();
    ASSERT_EQ(store->put("principal-a", "shard-a", foreign_id, foreign_payload),
              themis::sharding::ShardArtifactFragmentStore::PutResult::Stored);
    ASSERT_EQ(store->put("principal-a", "shard-a", "tspjm1-" + foreign_id,
                         std::vector<uint8_t>(foreign_record_text.begin(),
                                              foreign_record_text.end())),
              themis::sharding::ShardArtifactFragmentStore::PutResult::Stored);
    EXPECT_THROW((void)dt::LocalProjectionPublisher::loadRemote(
        *rpc_client, "principal-a", "tenant-a", "shard-a", "node2",
        foreign_id, foreign_payload.size(), 1000, "manifest-foreign"),
        std::runtime_error);
    manifest = std::move(remote_manifest);
    auto fetcher = std::make_shared<dt::ShardRPCFragmentFetcher>(
        std::unordered_map<std::string, std::shared_ptr<ShardRPCClient>>{
            {"node2", rpc_client}});

    ASSERT_EQ(manifest.shard_placements().size(), 1u);
    auto placement = manifest.shard_placements().front();
    ASSERT_EQ(placement.shard_content_hash, digest);

#if defined(THEMIS_TEST_HAS_DISTRIBUTED_TENSOR_GRAPH)
    namespace kg = themis::rag::kg;
    kg::KnowledgeGraph graph;
    kg::KGNode node;
    node.id = "alpha-node";
    node.canonical_name = "Alpha";
    node.type = kg::EntityType::CONCEPT;
    graph.addNode(std::move(node));
    auto validator = std::make_shared<themis::rag::GraphTruthValidator>();
    validator->setKnowledgeGraphRetriever(
        std::make_shared<kg::KnowledgeGraphRetriever>(graph));
#endif
    dt::DistributedTensorPlanner planner;
    planner.setFragmentFetcher(fetcher);
#if defined(THEMIS_TEST_HAS_DISTRIBUTED_TENSOR_GRAPH)
    planner.setGraphValidator(validator);
#endif
    tensor::ShardSummary routing;
    routing.shard_id = "shard-a";
    routing.shard_relevance = 0.9f;

    auto plan = planner.buildRetrievalPlan({routing}, manifest, "Alpha", "corr-a");
    ASSERT_EQ(plan.fragment_requests.size(), 1u);
    plan.fragment_results = planner.executeFragmentLoads(plan.fragment_requests, "corr-a");
    ASSERT_EQ(plan.fragment_results.size(), 1u);
    ASSERT_TRUE(plan.fragment_results[0].success) << plan.fragment_results[0].error_reason;
    EXPECT_EQ(plan.fragment_results[0].fragment_data, payload);
    const auto decoded = dt::GraphSummaryProjectionCodec::decode(
        plan.fragment_results[0].fragment_data);
    ASSERT_TRUE(decoded.ok());
    EXPECT_EQ(decoded.summary->scope_key, summary.scope_key);
#if defined(THEMIS_TEST_HAS_DISTRIBUTED_TENSOR_GRAPH)
    const auto validation = planner.validateAgainstGraphTruth(plan, "Alpha");
    ASSERT_FALSE(validation.evidences.empty());
    EXPECT_TRUE(plan.passed_graph_validation);
    EXPECT_TRUE(planner.isValidFinalPlan(plan));
#endif

    placement.shard_content_hash = sha256("wrong");
    manifest.add_shard_placement(placement);
    auto corrupt_plan =
        planner.buildRetrievalPlan({routing}, manifest, "Alpha", "corr-b");
    ASSERT_EQ(corrupt_plan.fragment_requests.size(), 1u);
    corrupt_plan.fragment_results =
        planner.executeFragmentLoads(corrupt_plan.fragment_requests, "corr-b");
    ASSERT_EQ(corrupt_plan.fragment_results.size(), 1u);
    EXPECT_FALSE(corrupt_plan.fragment_results[0].success);
    EXPECT_TRUE(corrupt_plan.fragment_results[0].fragment_data.empty());
    EXPECT_FALSE(planner.isValidFinalPlan(corrupt_plan));

    const auto unauthorized_config =
        raftClientConfig(root.path, config.listen_address, "node3", "node2");
    ShardRPCClient unauthorized(unauthorized_config);
    EXPECT_THROW((void)dt::LocalProjectionPublisher::loadRemote(
        unauthorized, "principal-a", "tenant-a", "shard-a", "node2",
        manifest.artifact_id(), payload.size(), 1000, "manifest-e"),
        std::runtime_error);
    EXPECT_THROW(unauthorized.fetchTensorFragment(
        "node2", "shard-a", manifest.artifact_id(), payload.size(), 1000, "corr-c"),
        std::runtime_error);

    constexpr char digits[] = "0123456789abcdef";
    std::string artifact_hex;
    for (const unsigned char byte : manifest.artifact_id()) {
        artifact_hex.push_back(digits[byte >> 4]);
        artifact_hex.push_back(digits[byte & 15]);
    }
    const auto manifest_key =
        "themis/tensor-projection-manifest/v1/7072696e636970616c2d61/" + artifact_hex;
    ASSERT_TRUE(db->put(manifest_key, std::string_view{"invalid"}));
    EXPECT_THROW((void)publisher->load("principal-a", manifest.artifact_id()),
                 std::runtime_error);
    EXPECT_THROW((void)publisher->publish("principal-a", "tenant-a", "source-a", summary),
                 std::runtime_error);
}
#endif

void sendChunk(rpc::ShardService::Stub& stub, const rpc::ShardTransferChunk& request,
               grpc::StatusCode expected_status, std::uint64_t expected_checkpoint = 0) {
    grpc::ClientContext context;
    context.set_deadline(deadline());
    auto stream = stub.StreamShardTransfer(&context);
    const bool wrote = stream->Write(request);
    stream->WritesDone();
    rpc::ShardTransferCheckpoint checkpoint;
    const bool acknowledged = stream->Read(&checkpoint);
    if (acknowledged) {
        rpc::ShardTransferCheckpoint extra;
        EXPECT_FALSE(stream->Read(&extra));
    }
    const auto status = stream->Finish();
    ASSERT_EQ(status.error_code(), expected_status) << status.error_message();
    if (expected_status == grpc::StatusCode::OK) {
        EXPECT_TRUE(wrote);
        ASSERT_TRUE(acknowledged);
        EXPECT_EQ(checkpoint.next_chunk(), expected_checkpoint);
    } else {
        EXPECT_FALSE(acknowledged);
    }
}

TEST(ShardTransferGrpc, ResumesAfterReceiverCrashAndRejectsInvalidTransfers) {
    const FixtureDirectory root;
    const auto receiver_tls = themis::tests::generateSelfSignedPemMaterial("1", "localhost", {"localhost"});
    const auto sender_tls = themis::tests::generateSelfSignedPemMaterial("2", kSourceIdentity, {kSourceIdentity});
    const auto unbound_tls = themis::tests::generateSelfSignedPemMaterial("3", "unbound.test", {"unbound.test"});
    writePem(root.path / "receiver-cert.pem", receiver_tls.certificate_pem);
    writePem(root.path / "receiver-key.pem", receiver_tls.private_key_pem);
    writePem(root.path / "sender-cert.pem", sender_tls.certificate_pem);
    writePem(root.path / "sender-key.pem", sender_tls.private_key_pem);
    writePem(root.path / "trusted-clients.pem",
             sender_tls.certificate_pem + unbound_tls.certificate_pem);

    ReceiverProcess receiver;
    const auto first_address = receiver.start(root.path, true);
    auto channel = connect(first_address, receiver_tls.certificate_pem, sender_tls);
    ASSERT_TRUE(channel->WaitForConnected(deadline())) << "initial mTLS handshake failed";
    auto stub = rpc::ShardService::NewStub(channel);
    const auto snapshot = snapshotFor("resume");
    const auto& manifest = snapshot.manifest;
    const auto resume = prepareRequest(manifest);
    rpc::ShardTransferCheckpoint checkpoint;
    auto status = prepare(*stub, resume, checkpoint);
    ASSERT_TRUE(status.ok()) << status.error_message();
    ASSERT_EQ(checkpoint.next_chunk(), 0u);

    const auto first = chunk(resume.migration_id(), 0, kFirstPayload);
    ASSERT_NO_FATAL_FAILURE(sendChunk(*stub, first, grpc::StatusCode::OK, 1));
    ASSERT_TRUE(receiver.crash());
    stub.reset();
    channel.reset();

    const auto second_address = receiver.start(root.path, false);
    channel = connect(second_address, receiver_tls.certificate_pem, sender_tls);
    ASSERT_TRUE(channel->WaitForConnected(deadline())) << "mTLS handshake after restart failed";
    stub = rpc::ShardService::NewStub(channel);
    checkpoint.Clear();
    status = prepare(*stub, resume, checkpoint);
    ASSERT_TRUE(status.ok()) << status.error_message();
    ASSERT_EQ(checkpoint.next_chunk(), 1u);

    // Identical replay must acknowledge the persisted checkpoint, not append bytes again.
    ASSERT_NO_FATAL_FAILURE(sendChunk(*stub, first, grpc::StatusCode::OK, 1));
    ShardRPCClient::Config client_config;
    client_config.endpoint = second_address;
    client_config.shard_id = kSource;
    client_config.tls_cert_path = (root.path / "sender-cert.pem").string();
    client_config.tls_key_path = (root.path / "sender-key.pem").string();
    client_config.tls_ca_cert_path = (root.path / "receiver-cert.pem").string();
    ShardRPCClient client(client_config);
    bool admitted = false;
    unsigned reads = 0;
    ASSERT_TRUE(client.transferShard(
        manifest,
        [&](std::uint64_t index) {
            EXPECT_TRUE(admitted);
            EXPECT_EQ(index, 1u);
            ++reads;
            return snapshot.read_chunk(index);
        },
        [&] {
            EXPECT_FALSE(admitted);
            admitted = true;
        }));
    EXPECT_TRUE(admitted);
    EXPECT_EQ(reads, 1u);
    rpc::ShardTransferId id;
    id.set_migration_id(resume.migration_id());
    grpc::ClientContext verify_context;
    verify_context.set_deadline(deadline());
    rpc::ShardTransferVerified verified;
    status = stub->VerifyShardTransfer(&verify_context, id, &verified);
    ASSERT_TRUE(status.ok()) << status.error_message();
    EXPECT_TRUE(verified.verified());

    const auto separately_assigned = snapshotFor("assigned-transfer-id");
    auto executor = themis::sharding::makeShardTransferExecutor(
        [&](const AutoRebalancer::MigrationRequest& approved) {
            EXPECT_EQ(approved.operation_id, "approved-operation-id");
            EXPECT_EQ(approved.operator_signature, "signed");
            return themis::sharding::ShardRangeExport{
                separately_assigned.manifest,
                std::make_shared<ShardRPCClient>(client_config),
                separately_assigned.read_chunk};
        });
    AutoRebalancer::MigrationRequest approved_request;
    approved_request.operation_id = "approved-operation-id";
    approved_request.operator_signature = "signed";
    approved_request.operation.source_shard_id = kSource;
    approved_request.operation.target_shard_id = kTarget;
    approved_request.operation.token_range_start = 100;
    approved_request.operation.token_range_end = 200;
    const auto executor_result = executor(approved_request);
    EXPECT_EQ(executor_result.outcome, AutoRebalancer::MigrationOutcome::TRANSFER_VERIFIED);

    auto unbound_channel = connect(second_address, receiver_tls.certificate_pem, unbound_tls);
    ASSERT_TRUE(unbound_channel->WaitForConnected(deadline()))
        << "unbound certificate must be trusted by TLS before testing principal rejection";
    auto unbound = rpc::ShardService::NewStub(unbound_channel);
    status = prepare(*unbound, resume, checkpoint);
    EXPECT_EQ(status.error_code(), grpc::StatusCode::PERMISSION_DENIED) << status.error_message();

    const auto abort = prepareRequest(assignment("abort"));
    status = prepare(*stub, abort, checkpoint);
    ASSERT_TRUE(status.ok()) << status.error_message();
    ASSERT_NO_FATAL_FAILURE(sendChunk(
        *stub, chunk(abort.migration_id(), 0, kFirstPayload), grpc::StatusCode::OK, 1));
    id.set_migration_id(abort.migration_id());
    grpc::ClientContext abort_context;
    abort_context.set_deadline(deadline());
    rpc::ShardTransferAborted aborted;
    status = stub->AbortShardTransfer(&abort_context, id, &aborted);
    ASSERT_TRUE(status.ok()) << status.error_message();
    EXPECT_TRUE(aborted.aborted());
    status = prepare(*stub, abort, checkpoint);
    EXPECT_EQ(status.error_code(), grpc::StatusCode::FAILED_PRECONDITION) << status.error_message();

    const auto bad_digest = prepareRequest(assignment("bad-digest"));
    status = prepare(*stub, bad_digest, checkpoint);
    ASSERT_TRUE(status.ok()) << status.error_message();
    auto corrupt = chunk(bad_digest.migration_id(), 0, kFirstPayload);
    corrupt.set_sha256(sha256("different payload"));
    ASSERT_NO_FATAL_FAILURE(sendChunk(*stub, corrupt, grpc::StatusCode::INVALID_ARGUMENT));
    status = prepare(*stub, bad_digest, checkpoint);
    ASSERT_TRUE(status.ok()) << status.error_message();
    EXPECT_EQ(checkpoint.next_chunk(), 0u);

    ASSERT_TRUE(receiver.crash());
    {
        ShardTransferStore reopened((root.path / "stage").string(), kTarget,
            [&](const ShardTransferManifest& candidate) { return candidate == manifest; });
        std::string persisted;
        reopened.readVerified(manifest, [&](std::string_view chunk) { persisted.append(chunk); });
        const auto records = decodeRecords(persisted);
        ASSERT_EQ(records.size(), 2u);
        EXPECT_EQ(records[0].first, "a");
        EXPECT_EQ(records[0].second, kValue);
        EXPECT_EQ(records[1].first, "z");
        EXPECT_EQ(records[1].second, std::string("b\0c", 3));
    }

    auto stale_channel = connect(receiver.start(root.path, false, true),
                                 receiver_tls.certificate_pem, sender_tls);
    ASSERT_TRUE(stale_channel->WaitForConnected(deadline()));
    auto stale = rpc::ShardService::NewStub(stale_channel);
    status = prepare(*stale, resume, checkpoint);
    EXPECT_EQ(status.error_code(), grpc::StatusCode::FAILED_PRECONDITION) << status.error_message();
    id.set_migration_id(resume.migration_id());
    grpc::ClientContext stale_verify_context;
    stale_verify_context.set_deadline(deadline());
    status = stale->VerifyShardTransfer(&stale_verify_context, id, &verified);
    EXPECT_EQ(status.error_code(), grpc::StatusCode::FAILED_PRECONDITION) << status.error_message();
}

TEST(RaftPeerGrpc, AuthenticatedVoteAndDurableAppendEntries) {
    const FixtureDirectory root;
    const auto server_tls =
        themis::tests::generateSelfSignedPemMaterial("101", "localhost", {"localhost"});
    const auto client_tls =
        themis::tests::generateSelfSignedPemMaterial("102", "node1.test", {"node1.test"});
    const auto unbound_tls =
        themis::tests::generateSelfSignedPemMaterial("103", "intruder.test", {"intruder.test"});
    writePem(root.path / "server-cert.pem", server_tls.certificate_pem);
    writePem(root.path / "server-key.pem", server_tls.private_key_pem);
    writePem(root.path / "client-cert.pem", client_tls.certificate_pem);
    writePem(root.path / "client-key.pem", client_tls.private_key_pem);
    writePem(root.path / "trusted-clients.pem",
             client_tls.certificate_pem + unbound_tls.certificate_pem);

    themisdb::sharding::RaftConsensus::Config raft_config;
    raft_config.raft_config.node_id = "node2";
    raft_config.raft_config.cluster_members = {"node1", "node2"};
    raft_config.raft_config.persistence_path = (root.path / "raft.journal").string();
    auto raft = std::make_shared<themisdb::sharding::RaftConsensus>(raft_config);

    ShardRPCServer::Config server_config;
    server_config.listen_address = "127.0.0.1:" + std::to_string(loopbackPort());
    server_config.tls_cert_path = (root.path / "server-cert.pem").string();
    server_config.tls_key_path = (root.path / "server-key.pem").string();
    server_config.tls_ca_cert_path = (root.path / "trusted-clients.pem").string();
    server_config.raft_identity_bindings = {
        {"x509_subject_alternative_name", "node1.test", "node1"},
    };
    server_config.raft_group_resolver =
        [raft](const std::string& group_id)
            -> std::shared_ptr<themisdb::sharding::RaftConsensus> {
            if (group_id == "payments") {
                return raft;
            }
            return {};
        };
    ShardRPCServer server(server_config);
    ASSERT_TRUE(server.start());

    ShardRPCClient::Config client_config;
    client_config.endpoint = server_config.listen_address;
    client_config.tls_cert_path = (root.path / "client-cert.pem").string();
    client_config.tls_key_path = (root.path / "client-key.pem").string();
    client_config.tls_ca_cert_path = (root.path / "server-cert.pem").string();
    client_config.local_node_id = "node1";
    client_config.expected_peer_node_id = "node2";
    client_config.expected_peer_identity_property = "x509_subject_alternative_name";
    client_config.expected_peer_identity_value = "localhost";
    client_config.timeout_ms = 1000;
    themisdb::sharding::RaftShardManager::Config manager_config;
    manager_config.raft_config.raft_config.node_id = "node1";
    manager_config.raft_config.raft_config.persistence_path =
        (root.path / "leader.journal").string();
    manager_config.raft_config.raft_config.election_timeout_min_ms = 100;
    manager_config.raft_config.raft_config.election_timeout_max_ms = 150;
    manager_config.raft_config.raft_config.heartbeat_interval_ms = 50;
    manager_config.replication_factor = 2;
    manager_config.enable_auto_start = false;
    manager_config.raft_peer_config_resolver =
        [client_config](const std::string& peer_node_id) mutable
            -> std::optional<ShardRPCClient::Config> {
            if (peer_node_id != "node2") {
                return std::nullopt;
            }
            return client_config;
        };
    themisdb::sharding::RaftShardManager manager(manager_config);
    ASSERT_TRUE(manager.initializeShard("payments", {"node1", "node2"}));
    ASSERT_TRUE(manager.startShard("payments"));
    const auto leader = manager.getRaftInstance("payments");
    ASSERT_NE(leader, nullptr);
    const auto election_deadline = std::chrono::steady_clock::now() + 5s;
    while (!leader->isLeader() && std::chrono::steady_clock::now() < election_deadline) {
        std::this_thread::sleep_for(10ms);
    }
    ASSERT_TRUE(leader->isLeader()) << "mTLS VoteRequest did not establish a leader";
    ASSERT_TRUE(manager.proposeWrite("payments", "durable-decision").get())
        << "mTLS AppendEntries did not reach quorum";
    EXPECT_EQ(leader->getRaftState().getLog().getCommitIndex(), 1u);
    ASSERT_TRUE(raft->getRaftState().getLog().getEntry(1).has_value());
    EXPECT_EQ(raft->getRaftState().getLog().getEntry(1)->command, "durable-decision");

    grpc::SslCredentialsOptions unbound_credentials;
    unbound_credentials.pem_root_certs = server_tls.certificate_pem;
    unbound_credentials.pem_cert_chain = unbound_tls.certificate_pem;
    unbound_credentials.pem_private_key = unbound_tls.private_key_pem;
    grpc::ChannelArguments channel_args;
    channel_args.SetInt("grpc.enable_http_proxy", 0);
    channel_args.SetSslTargetNameOverride("localhost");
    auto unbound_channel = grpc::CreateCustomChannel(
        server_config.listen_address, grpc::SslCredentials(unbound_credentials), channel_args);
    auto unbound_stub = rpc::ShardService::NewStub(unbound_channel);
    rpc::RaftVoteRequestProto forged_vote;
    forged_vote.set_raft_group_id("payments");
    forged_vote.set_candidate_id("intruder");
    forged_vote.set_term(2);
    rpc::RaftVoteResponseProto forged_response;
    grpc::ClientContext forged_context;
    forged_context.set_deadline(std::chrono::system_clock::now() + 2s);
    const auto forged_status =
        unbound_stub->RequestVote(&forged_context, forged_vote, &forged_response);
    EXPECT_EQ(forged_status.error_code(), grpc::StatusCode::PERMISSION_DENIED);
    EXPECT_EQ(raft->getCurrentTerm(), 1u);
}

TEST(RaftPeerGrpc, VolatileFollowerCannotGrantVoteOrAppendAck) {
    const FixtureDirectory root;
    const auto server_tls =
        themis::tests::generateSelfSignedPemMaterial("201", "localhost", {"localhost"});
    const auto client_tls =
        themis::tests::generateSelfSignedPemMaterial("202", "node1.test", {"node1.test"});
    writePem(root.path / "volatile-server-cert.pem", server_tls.certificate_pem);
    writePem(root.path / "volatile-server-key.pem", server_tls.private_key_pem);
    writePem(root.path / "volatile-client-cert.pem", client_tls.certificate_pem);
    writePem(root.path / "volatile-client-key.pem", client_tls.private_key_pem);
    writePem(root.path / "volatile-trusted-clients.pem", client_tls.certificate_pem);

    themisdb::sharding::RaftConsensus::Config raft_config;
    raft_config.raft_config.node_id = "node2";
    raft_config.raft_config.cluster_members = {"node1", "node2"};
    raft_config.raft_config.persistence_path.clear();
    auto volatile_raft =
        std::make_shared<themisdb::sharding::RaftConsensus>(raft_config);

    ShardRPCServer::Config server_config;
    server_config.listen_address = "127.0.0.1:" + std::to_string(loopbackPort());
    server_config.tls_cert_path = (root.path / "volatile-server-cert.pem").string();
    server_config.tls_key_path = (root.path / "volatile-server-key.pem").string();
    server_config.tls_ca_cert_path = (root.path / "volatile-trusted-clients.pem").string();
    server_config.raft_identity_bindings = {
        {"x509_subject_alternative_name", "node1.test", "node1"},
    };
    server_config.raft_group_resolver =
        [volatile_raft](const std::string& group_id)
            -> std::shared_ptr<themisdb::sharding::RaftConsensus> {
            if (group_id == "payments") {
                return volatile_raft;
            }
            return {};
        };
    ShardRPCServer server(server_config);
    ASSERT_TRUE(server.start());

    ShardRPCClient::Config client_config;
    client_config.endpoint = server_config.listen_address;
    client_config.tls_cert_path = (root.path / "volatile-client-cert.pem").string();
    client_config.tls_key_path = (root.path / "volatile-client-key.pem").string();
    client_config.tls_ca_cert_path = (root.path / "volatile-server-cert.pem").string();
    client_config.local_node_id = "node1";
    client_config.expected_peer_node_id = "node2";
    client_config.expected_peer_identity_property =
        "x509_subject_alternative_name";
    client_config.expected_peer_identity_value = "localhost";
    ShardRPCClient client(client_config);

    themisdb::sharding::AppendEntriesRequest append_request;
    append_request.term = 0;
    append_request.leader_id = "node1";
    append_request.prev_log_index = 0;
    append_request.prev_log_term = 0;
    append_request.leader_commit = 0;
    append_request.entries.push_back(
        themisdb::sharding::LogEntry(0, 1, "must-not-be-acked", 0));
    const auto append =
        client.appendRaftEntries("payments", "node2", append_request);
    EXPECT_FALSE(append.success);
    EXPECT_EQ(append.match_index, 0u);

    themisdb::sharding::VoteRequest vote_request;
    vote_request.candidate_id = "node1";
    vote_request.term = 1;
    vote_request.last_log_index = 0;
    vote_request.last_log_term = 0;
    EXPECT_THROW(
        client.requestRaftVote("payments", "node2", vote_request),
        std::runtime_error);
    EXPECT_FALSE(volatile_raft->getRaftState().getLog().getEntry(1).has_value());
}

TEST(RaftPeerGrpc, TwoProcessRestartRetainsCommittedLogAndRejectsConflicts) {
    const FixtureDirectory root;
    const std::vector<std::string> members{"node1", "node2"};
    const auto certificates = writeRaftCertificates(root.path, members);

    RaftPeerProcess follower;
    auto follower_address = follower.start(root.path, "node2", members);
    ASSERT_TRUE(waitForRaftPeer(follower_address, certificates, "node1", "node2"))
        << "initial follower mTLS handshake failed";

    const std::map<std::string, std::string> addresses{{"node2", follower_address}};
    themisdb::sharding::RaftShardManager manager(raftManagerConfig(root.path, members, addresses));
    ASSERT_TRUE(manager.initializeShard(kRaftGroup, members));
    ASSERT_TRUE(manager.startShard(kRaftGroup));
    const auto leader = manager.getRaftInstance(kRaftGroup);
    ASSERT_NE(leader, nullptr);
    const auto election_deadline = std::chrono::steady_clock::now() + 5s;
    while (!leader->isLeader() && std::chrono::steady_clock::now() < election_deadline) {
        std::this_thread::sleep_for(10ms);
    }
    ASSERT_TRUE(leader->isLeader()) << "remote follower vote did not elect the leader";
    ASSERT_TRUE(manager.proposeWrite(kRaftGroup, "durable-two-process-entry").get())
        << "remote follower did not durably acknowledge the proposal";

    const auto entry = leader->getRaftState().getLog().getEntry(1);
    ASSERT_TRUE(entry.has_value());
    themisdb::sharding::AppendEntriesRequest append_request;
    append_request.term = leader->getCurrentTerm();
    append_request.leader_id = "node1";
    append_request.entries.push_back(*entry);
    append_request.leader_commit = entry->index;
    const auto append_to_follower = [&](const std::string& address,
                                        const themisdb::sharding::AppendEntriesRequest& request) {
        ShardRPCClient client(raftClientConfig(root.path, address, "node1", "node2"));
        return client.appendRaftEntries(kRaftGroup, "node2", request);
    };
    const auto durable_ack = append_to_follower(follower_address, append_request);
    ASSERT_TRUE(durable_ack.success);
    ASSERT_EQ(durable_ack.match_index, entry->index);

    ASSERT_TRUE(follower.crash()) << "follower process did not exit after SIGKILL";
    follower_address = follower.start(root.path, "node2", members);
    ASSERT_TRUE(waitForRaftPeer(follower_address, certificates, "node1", "node2"))
        << "follower mTLS handshake failed after journal recovery";

    auto conflicting_request = append_request;
    conflicting_request.entries.front().command = "conflicting-payload";
    const auto conflict = append_to_follower(follower_address, conflicting_request);
    EXPECT_FALSE(conflict.success)
        << "restarted follower accepted a different payload at the persisted index and term";

    themisdb::sharding::AppendEntriesRequest recovered_prefix;
    recovered_prefix.term = append_request.term;
    recovered_prefix.leader_id = append_request.leader_id;
    recovered_prefix.prev_log_index = entry->index;
    recovered_prefix.prev_log_term = entry->term;
    recovered_prefix.leader_commit = entry->index;
    const auto recovered = append_to_follower(follower_address, recovered_prefix);
    EXPECT_TRUE(recovered.success)
        << "restarted follower did not recover the committed log prefix";
    EXPECT_EQ(recovered.match_index, entry->index);

    if (append_request.term > 0) {
        auto stale_request = recovered_prefix;
        stale_request.term = append_request.term - 1;
        const auto stale = append_to_follower(follower_address, stale_request);
        EXPECT_FALSE(stale.success);
        EXPECT_GE(stale.term, append_request.term);
    }

    themisdb::sharding::AppendEntriesRequest partial_prefix = recovered_prefix;
    partial_prefix.prev_log_index = entry->index + 100;
    partial_prefix.prev_log_term = entry->term;
    const auto partial = append_to_follower(follower_address, partial_prefix);
    EXPECT_FALSE(partial.success);
    EXPECT_LT(partial.match_index, partial_prefix.prev_log_index);

    ShardRPCClient::Config timeout_config =
        raftClientConfig(root.path, "127.0.0.1:" + std::to_string(loopbackPort()),
                         "node1", "node2");
    timeout_config.timeout_ms = 100;
    ShardRPCClient timeout_client(timeout_config);
    EXPECT_THROW(
        timeout_client.appendRaftEntries(kRaftGroup, "node2", recovered_prefix),
        std::runtime_error);
}

TEST(RaftPeerGrpc, ThreeProcessQuorumLossAndFollowerRestartPreserveDurablePrefix) {
    const FixtureDirectory root;
    const std::vector<std::string> members{"node1", "node2", "node3"};
    const auto certificates = writeRaftCertificates(root.path, members);

    RaftPeerProcess follower2;
    RaftPeerProcess follower3;
    auto address2 = follower2.start(root.path, "node2", members);
    auto address3 = follower3.start(root.path, "node3", members);
    ASSERT_TRUE(waitForRaftPeer(address2, certificates, "node1", "node2"))
        << "node2 mTLS handshake failed";
    ASSERT_TRUE(waitForRaftPeer(address3, certificates, "node1", "node3"))
        << "node3 mTLS handshake failed";

    const std::map<std::string, std::string> addresses{
        {"node2", address2}, {"node3", address3}};
    themisdb::sharding::RaftShardManager manager(raftManagerConfig(root.path, members, addresses));
    ASSERT_TRUE(manager.initializeShard(kRaftGroup, members));
    ASSERT_TRUE(manager.startShard(kRaftGroup));
    const auto leader = manager.getRaftInstance(kRaftGroup);
    ASSERT_NE(leader, nullptr);
    const auto election_deadline = std::chrono::steady_clock::now() + 5s;
    while (!leader->isLeader() && std::chrono::steady_clock::now() < election_deadline) {
        std::this_thread::sleep_for(10ms);
    }
    ASSERT_TRUE(leader->isLeader()) << "remote follower votes did not elect the leader";
    ASSERT_TRUE(manager.proposeWrite(kRaftGroup, "durable-three-process-entry").get())
        << "three-process Raft proposal did not reach quorum";

    const auto entry = leader->getRaftState().getLog().getEntry(1);
    ASSERT_TRUE(entry.has_value());
    themisdb::sharding::AppendEntriesRequest append_request;
    append_request.term = leader->getCurrentTerm();
    append_request.leader_id = "node1";
    append_request.entries.push_back(*entry);
    append_request.leader_commit = entry->index;
    const auto append_to_follower = [&](const std::string& peer_node_id,
                                        const std::string& address,
                                        const themisdb::sharding::AppendEntriesRequest& request) {
        ShardRPCClient client(
            raftClientConfig(root.path, address, "node1", peer_node_id));
        return client.appendRaftEntries(kRaftGroup, peer_node_id, request);
    };
    for (const auto& [node_id, address] : addresses) {
        const auto response = append_to_follower(node_id, address, append_request);
        ASSERT_TRUE(response.success) << node_id << " rejected the durable log entry";
        ASSERT_EQ(response.match_index, entry->index);
    }

    ASSERT_TRUE(follower2.crash());
    ASSERT_TRUE(follower3.crash());
    EXPECT_FALSE(manager.proposeWrite(kRaftGroup, "must-not-commit-without-quorum").get())
        << "leader reported a committed write after both remote processes were killed";
    EXPECT_EQ(leader->getRaftState().getLog().getCommitIndex(), entry->index);

    address2 = follower2.start(root.path, "node2", members);
    ASSERT_TRUE(waitForRaftPeer(address2, certificates, "node1", "node2"))
        << "node2 mTLS handshake failed after crash/restart";
    {
        ShardRPCClient candidate_client(
            raftClientConfig(root.path, address2, "node3", "node2"));
        themisdb::sharding::VoteRequest competing_vote;
        competing_vote.candidate_id = "node3";
        competing_vote.term = append_request.term;
        competing_vote.last_log_index = entry->index;
        competing_vote.last_log_term = entry->term;
        const auto vote = candidate_client.requestRaftVote(
            kRaftGroup, "node2", competing_vote);
        EXPECT_FALSE(vote.vote_granted)
            << "restarted node2 forgot its durable vote for the current term";
    }

    auto conflicting_request = append_request;
    conflicting_request.entries.front().command = "conflicting-payload";
    const auto conflict = append_to_follower("node2", address2, conflicting_request);
    EXPECT_FALSE(conflict.success)
        << "restarted node2 accepted a different payload at the persisted index and term";

    themisdb::sharding::AppendEntriesRequest recovered_prefix;
    recovered_prefix.term = append_request.term;
    recovered_prefix.leader_id = append_request.leader_id;
    recovered_prefix.prev_log_index = entry->index;
    recovered_prefix.prev_log_term = entry->term;
    recovered_prefix.leader_commit = entry->index;
    const auto recovered = append_to_follower("node2", address2, recovered_prefix);
    EXPECT_TRUE(recovered.success)
        << "restarted node2 did not recover the committed log prefix";
    EXPECT_EQ(recovered.match_index, entry->index);
}

TEST(RaftPeerGrpc, LeaderProcessCrashAndRestartPreservesJournalCommit) {
    const FixtureDirectory root;
    const std::vector<std::string> members{"node1", "node2", "node3"};
    const auto certificates = writeRaftCertificates(root.path, members);

    const auto leader_port = std::to_string(loopbackPort());
    const auto node2_port = std::to_string(loopbackPort());
    const auto node3_port = std::to_string(loopbackPort());
    const std::map<std::string, std::string> peer_addresses{
        {"node1", "localhost:" + leader_port},
        {"node2", "localhost:" + node2_port},
        {"node3", "localhost:" + node3_port}};

    RaftPeerProcess follower2;
    RaftPeerProcess follower3;
    follower2.start(root.path, "node2", members, peer_addresses, node2_port);
    follower3.start(root.path, "node3", members, peer_addresses, node3_port);
    const auto address2 = peer_addresses.at("node2");
    const auto address3 = peer_addresses.at("node3");
    ASSERT_TRUE(waitForRaftPeer(address2, certificates, "node1", "node2"));
    ASSERT_TRUE(waitForRaftPeer(address3, certificates, "node1", "node3"));

    RaftLeaderProcess leader;
    const auto ready = root.path / "node1.ready";
    std::error_code error;
    fs::remove(ready, error);
    auto leader_address = leader.start(root.path, leader_port, members, peer_addresses);
    const auto initial_leader_marker =
        waitForRaftReadyMarker(ready, 10s);
    ASSERT_TRUE(initial_leader_marker.has_value())
        << "separate leader process did not elect and commit";
    ASSERT_GE(initial_leader_marker->second, 1u);
    ASSERT_TRUE(waitForRaftPeer(leader_address, certificates, "node2", "node1"))
        << "leader mTLS server did not accept peers";

    ASSERT_TRUE(waitForRaftCommitIndex(
        {root.path / "node2.state", root.path / "node3.state"},
        initial_leader_marker->second, 5s))
        << "no follower applied the leader commit through normal heartbeats";

    ASSERT_TRUE(leader.crash()) << "leader process did not exit after SIGKILL";
    fs::remove(ready, error);
    const auto node2_ready = root.path / "node2.ready";
    const auto node3_ready = root.path / "node3.ready";
    fs::remove(node2_ready, error);
    fs::remove(node3_ready, error);

    const auto node2_election =
        waitForRaftReadyMarker(node2_ready, 10s);
    const auto node3_election =
        waitForRaftReadyMarker(node3_ready, 10s);
    ASSERT_TRUE(node2_election.has_value() || node3_election.has_value())
        << "a follower did not elect after leader SIGKILL";
    const auto elected_marker =
        node2_election.has_value() ? node2_election : node3_election;
    EXPECT_GT(elected_marker->first, initial_leader_marker->first)
        << "new follower leader did not advance beyond the crashed leader term";
    EXPECT_GE(elected_marker->second, initial_leader_marker->second)
        << "new follower leader lost the committed log prefix";

    leader_address = leader.start(root.path, leader_port, members, peer_addresses, false);
    const auto restarted_leader_marker =
        waitForRaftReadyMarker(ready, 10s);
    ASSERT_TRUE(restarted_leader_marker.has_value())
        << "restarted node did not report its recovered durable Raft state";
    EXPECT_GE(restarted_leader_marker->first, initial_leader_marker->first)
        << "restarted node forgot its durable Raft term";
    EXPECT_GE(restarted_leader_marker->second, initial_leader_marker->second)
        << "restarted node forgot its durable committed log prefix";
    ASSERT_TRUE(waitForRaftPeer(leader_address, certificates, "node2", "node1"))
        << "leader mTLS server did not recover after restart";

    std::ifstream marker(ready);
    std::uint64_t term = 0;
    std::uint64_t commit_index = 0;
    ASSERT_TRUE(static_cast<bool>(marker >> term >> commit_index));
    EXPECT_EQ(term, restarted_leader_marker->first);
    EXPECT_EQ(commit_index, restarted_leader_marker->second);
    EXPECT_GE(commit_index, 1u);
}

} // namespace

#else

TEST(ShardTransferGrpc, ResumesAfterReceiverCrashAndRejectsInvalidTransfers) {
    GTEST_SKIP() << "Receiver self-exec and parent-death cleanup require Linux";
}

#endif

int main(int argc, char** argv) {
#if defined(__linux__)
    if (argc == 6 && std::string_view(argv[1]) == kReceiverMode) {
        try {
            return runReceiver(argv[2], argv[3], argv[4],
                               static_cast<pid_t>(std::stol(argv[5])));
        } catch (const std::exception& e) {
            std::cerr << "shard transfer receiver: " << e.what() << '\n';
            return EXIT_FAILURE;
        }
    }
    if (argc == 8 && std::string_view(argv[1]) == kRaftReceiverMode) {
        try {
            std::vector<std::string> members;
            std::string member_list(argv[5]);
            std::size_t start = 0;
            while (start < member_list.size()) {
                const auto separator = member_list.find(',', start);
                members.push_back(member_list.substr(
                    start, separator == std::string::npos
                               ? std::string::npos
                               : separator - start));
                if (separator == std::string::npos) {
                    break;
                }
                start = separator + 1;
            }
            if (members.empty() ||
                std::find(members.begin(), members.end(), std::string{}) != members.end()) {
                throw std::invalid_argument("Raft peer membership must not be empty");
            }
            return runRaftReceiver(argv[2], argv[3], argv[4], members,
                                   argv[6], static_cast<pid_t>(std::stol(argv[7])));
        } catch (const std::exception& e) {
            std::cerr << "Raft peer receiver: " << e.what() << '\n';
            return EXIT_FAILURE;
        }
    }
    if (argc == 8 && std::string_view(argv[1]) == kRaftLeaderMode) {
        try {
            std::vector<std::string> members;
            std::string member_list(argv[4]);
            std::size_t start = 0;
            while (start < member_list.size()) {
                const auto separator = member_list.find(',', start);
                members.push_back(member_list.substr(
                    start, separator == std::string::npos
                               ? std::string::npos
                               : separator - start));
                if (separator == std::string::npos) {
                    break;
                }
                start = separator + 1;
            }
            if (members.empty() ||
                std::find(members.begin(), members.end(), std::string{}) != members.end()) {
                throw std::invalid_argument("Raft leader membership must not be empty");
            }
            const std::string_view startup_mode(argv[7]);
            if (startup_mode != "commit" && startup_mode != "recover") {
                throw std::invalid_argument("Raft leader startup mode must be commit or recover");
            }
            return runRaftLeader(argv[2], argv[3], members, argv[5],
                                 static_cast<pid_t>(std::stol(argv[6])),
                                 startup_mode == "commit");
        } catch (const std::exception& e) {
            std::cerr << "Raft leader process: " << e.what() << '\n';
            return EXIT_FAILURE;
        }
    }
#endif
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
