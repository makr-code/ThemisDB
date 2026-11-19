// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#include <gtest/gtest.h>

#if defined(__linux__)

#include "sharding/shard_rpc_client.h"
#include "sharding/shard_rpc_server.h"
#include "sharding/shard_transfer_store.h"
#include "shard_rpc.grpc.pb.h"

#include "../test_crypto_material_utils.h"

#include <grpcpp/grpcpp.h>
#include <openssl/evp.h>

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
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

extern char** environ;

namespace {

namespace fs = std::filesystem;
namespace rpc = themis::sharding::proto;
using themis::sharding::ShardRPCServer;
using themis::sharding::ShardRPCClient;
using themis::sharding::ShardTransferManifest;
using themis::sharding::ShardTransferStore;
using namespace std::chrono_literals;

constexpr char kReceiverMode[] = "--shard-transfer-receiver";
constexpr char kSource[] = "source-shard";
constexpr char kTarget[] = "target-shard";
constexpr char kSourceIdentity[] = "source.test";
constexpr std::uint64_t kEpoch = 17;
constexpr std::string_view kFirstPayload("first\0chunk", 11);
constexpr std::string_view kSecondPayload = "second chunk";

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

[[nodiscard]] ShardTransferManifest assignment(const std::string& id) {
    ShardTransferManifest manifest;
    manifest.id = id;
    manifest.source = kSource;
    manifest.target = kTarget;
    manifest.range_start = 100;
    manifest.range_end = 200;
    manifest.epoch = kEpoch;
    manifest.watermark = "42";
    manifest.chunks = 2;
    manifest.bytes = kFirstPayload.size() + kSecondPayload.size();
    manifest.sha256 = sha256(std::string(kFirstPayload).append(kSecondPayload));
    return manifest;
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

    [[nodiscard]] std::string start(const fs::path& root, bool seed) {
        const auto port = std::to_string(loopbackPort());
        const auto parent = std::to_string(::getpid());
        const std::string path = root.string();
        // Exec instead of forking a live gRPC runtime on receiver restart.
        char* const argv[] = {
            const_cast<char*>("/proc/self/exe"),
            const_cast<char*>(kReceiverMode),
            const_cast<char*>(path.c_str()),
            const_cast<char*>(port.c_str()),
            const_cast<char*>(seed ? "seed" : "restart"),
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
                              bool seed, pid_t parent) {
    // Do not leave a receiver behind if CTest times out and kills the sender.
    if (::prctl(PR_SET_PDEATHSIG, SIGKILL) != 0 || ::getppid() != parent) {
        return EXIT_FAILURE;
    }
    auto store = std::make_shared<ShardTransferStore>(
        (root / "stage").string(), kTarget, [](const ShardTransferManifest& manifest) {
            return manifest.source == kSource && manifest.target == kTarget &&
                   manifest.epoch == kEpoch && manifest.range_start == 100 &&
                   manifest.range_end == 200;
        });
    if (seed) {
        for (const char* id : {"resume", "abort", "bad-digest"}) {
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

[[nodiscard]] auto deadline() {
    return std::chrono::system_clock::now() + 5s;
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
    const auto manifest = assignment("resume");
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
            return std::pair{std::string(kSecondPayload), sha256(kSecondPayload)};
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
            return runReceiver(argv[2], argv[3], std::string_view(argv[4]) == "seed",
                               static_cast<pid_t>(std::stol(argv[5])));
        } catch (const std::exception& e) {
            std::cerr << "shard transfer receiver: " << e.what() << '\n';
            return EXIT_FAILURE;
        }
    }
#endif
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
