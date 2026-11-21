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

#include "../test_crypto_material_utils.h"

#include <grpcpp/grpcpp.h>
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

constexpr char kReceiverMode[] = "--shard-transfer-receiver";
constexpr char kRaftReceiverMode[] = "--raft-peer-receiver";
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
        const std::vector<std::string>& members) {
        const auto port = std::to_string(loopbackPort());
        const auto parent = std::to_string(::getpid());
        const auto path = root.string();
        std::string member_list;
        for (const auto& member : members) {
            if (!member_list.empty()) {
                member_list.push_back(',');
            }
            member_list += member;
        }

        // Exec instead of forking a live gRPC runtime on restart.
        char* const argv[] = {
            const_cast<char*>("/proc/self/exe"),
            const_cast<char*>(kRaftReceiverMode),
            const_cast<char*>(path.c_str()),
            const_cast<char*>(port.c_str()),
            const_cast<char*>(node_id.c_str()),
            const_cast<char*>(member_list.c_str()),
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

[[nodiscard]] int runRaftReceiver(
    const fs::path& root, const std::string& port, const std::string& node_id,
    const std::vector<std::string>& members, pid_t parent) {
    // Do not leave a peer behind if the test runner kills the parent.
    if (::prctl(PR_SET_PDEATHSIG, SIGKILL) != 0 || ::getppid() != parent) {
        return EXIT_FAILURE;
    }

    themisdb::sharding::RaftConsensus::Config raft_config;
    raft_config.raft_config.node_id = node_id;
    raft_config.raft_config.cluster_members = members;
    raft_config.raft_config.persistence_path = (root / (node_id + ".journal")).string();
    auto raft = std::make_shared<themisdb::sharding::RaftConsensus>(raft_config);

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
    for (const auto& member : members) {
        const auto identity = member + ".test";
        auto material =
            themis::tests::generateSelfSignedPemMaterial(member, identity, {identity});
        writePem(root / (member + "-cert.pem"), material.certificate_pem);
        writePem(root / (member + "-key.pem"), material.private_key_pem);
        trusted_clients += material.certificate_pem;
        certificates.emplace(member, std::move(material));
    }
    writePem(root / "trusted-clients.pem", trusted_clients);
    return certificates;
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
    const std::map<std::string, std::string>& addresses) {
    themisdb::sharding::RaftShardManager::Config config;
    config.raft_config.raft_config.node_id = "node1";
    config.raft_config.raft_config.persistence_path = (root / "node1.journal").string();
    config.raft_config.raft_config.election_timeout_min_ms = 100;
    config.raft_config.raft_config.election_timeout_max_ms = 150;
    config.raft_config.raft_config.heartbeat_interval_ms = 50;
    config.replication_factor = members.size();
    config.raft_peer_config_resolver =
        [root, addresses](const std::string& peer_node_id)
            -> std::optional<ShardRPCClient::Config> {
            const auto address = addresses.find(peer_node_id);
            if (address == addresses.end()) {
                return std::nullopt;
            }
            return raftClientConfig(root, address->second, "node1", peer_node_id);
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
        themis::tests::generateSelfSignedPemMaterial("raft-server", "localhost", {"localhost"});
    const auto client_tls =
        themis::tests::generateSelfSignedPemMaterial("raft-client", "node1.test", {"node1.test"});
    const auto unbound_tls =
        themis::tests::generateSelfSignedPemMaterial("raft-unbound", "intruder.test", {"intruder.test"});
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
    themisdb::sharding::RaftShardManager::Config manager_config;
    manager_config.raft_config.raft_config.node_id = "node1";
    manager_config.raft_config.raft_config.persistence_path =
        (root.path / "leader.journal").string();
    manager_config.raft_config.raft_config.election_timeout_min_ms = 100;
    manager_config.raft_config.raft_config.election_timeout_max_ms = 150;
    manager_config.raft_config.raft_config.heartbeat_interval_ms = 50;
    manager_config.replication_factor = 2;
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
        themis::tests::generateSelfSignedPemMaterial("volatile-server", "localhost", {"localhost"});
    const auto client_tls =
        themis::tests::generateSelfSignedPemMaterial("volatile-client", "node1.test", {"node1.test"});
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
    if (argc == 7 && std::string_view(argv[1]) == kRaftReceiverMode) {
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
                                   static_cast<pid_t>(std::stol(argv[6])));
        } catch (const std::exception& e) {
            std::cerr << "Raft peer receiver: " << e.what() << '\n';
            return EXIT_FAILURE;
        }
    }
#endif
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
