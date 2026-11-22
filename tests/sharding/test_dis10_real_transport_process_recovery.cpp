// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#include <gtest/gtest.h>

#if defined(__linux__)

#include "sharding/shard_rpc_client_adapter.h"
#include "sharding/shard_rpc_server.h"
#include "sharding/two_phase_commit_coordinator.h"
#include "sharding/two_phase_commit_participant.h"
#include "sharding/wal_manager.h"

#include "../test_crypto_material_utils.h"

#include <sys/prctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <fcntl.h>
#include <poll.h>
#include <spawn.h>

#include <atomic>
#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <utility>
#include <vector>
#include <algorithm>

extern char** environ;

namespace {

namespace fs = std::filesystem;
using namespace std::chrono_literals;
using themis::sharding::CoordinatorTxnResult;
using themis::sharding::CoordinatorTxnState;
using themis::sharding::EpochToken;
using themis::sharding::ShardRPCClient;
using themis::sharding::ShardRPCClientAdapter;
using themis::sharding::ShardRPCServer;
using themis::sharding::TwoPhaseCommitCoordinator;
using themis::sharding::TwoPhaseCommitParticipant;
using themis::sharding::WALEntry;
using themis::sharding::WALEntryType;
using themis::sharding::WALManager;
using themis::sharding::WALManagerConfig;

constexpr std::string_view kParticipantMode = "--dis10-participant";
constexpr std::string_view kCoordinatorMode = "--dis10-coordinator";
constexpr std::string_view kCoordinatorId = "coord-1";
constexpr std::string_view kCoordinatorIdentity = "coord.test";
constexpr std::string_view kParticipantACertName = "participant-a";
constexpr std::string_view kParticipantBCertName = "participant-b";
constexpr std::string_view kParticipantDCertName = "participant-d";
constexpr std::string_view kShardA = "shard-a";
constexpr std::string_view kShardB = "shard-b";
constexpr std::string_view kShardC = "shard-c";
constexpr std::string_view kShardD = "shard-d";

[[nodiscard]] int loopbackPort() {
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
        throw std::system_error(error, std::generic_category(), "allocate loopback port");
    }
    return ntohs(addr.sin_port);
}

void writePem(const fs::path& path, const std::string& pem) {
    std::ofstream out(path);
    out.exceptions(std::ios::badbit | std::ios::failbit);
    out << pem;
}

void clearCloseOnExec(const int fd) {
    const int flags = ::fcntl(fd, F_GETFD);
    if (flags < 0 || ::fcntl(fd, F_SETFD, flags & ~FD_CLOEXEC) < 0) {
        throw std::system_error(errno, std::generic_category(), "fcntl(FD_CLOEXEC)");
    }
}

[[nodiscard]] bool waitForByte(const int fd, const char expected, const int timeout_ms) {
    pollfd descriptor{fd, POLLIN, 0};
    const int polled = ::poll(&descriptor, 1, timeout_ms);
    if (polled <= 0 || (descriptor.revents & POLLIN) == 0) {
        return false;
    }
    char marker = '\0';
    return ::read(fd, &marker, sizeof(marker)) == sizeof(marker) && marker == expected;
}

class FixtureDirectory {
public:
    explicit FixtureDirectory(std::string prefix) {
        for (int attempt = 0; attempt < 100; ++attempt) {
            path_ = fs::current_path() /
                (std::move(prefix) + "-" + std::to_string(::getpid()) + "-" + std::to_string(attempt));
            std::error_code error;
            if (fs::create_directories(path_, error)) {
                return;
            }
            if (!error || error != std::errc::file_exists) {
                throw std::runtime_error("failed to create DIS-10 fixture directory");
            }
            prefix = "dis10-real-transport";
        }
        throw std::runtime_error("could not allocate DIS-10 fixture directory");
    }

    ~FixtureDirectory() {
        std::error_code error;
        fs::remove_all(path_, error);
    }

    [[nodiscard]] const fs::path& path() const { return path_; }

private:
    fs::path path_;
};

class SpawnedProcess {
public:
    SpawnedProcess() = default;
    explicit SpawnedProcess(pid_t pid) : pid_(pid) {}
    SpawnedProcess(const SpawnedProcess&) = delete;
    SpawnedProcess& operator=(const SpawnedProcess&) = delete;
    SpawnedProcess(SpawnedProcess&& other) noexcept : pid_(std::exchange(other.pid_, -1)) {}
    SpawnedProcess& operator=(SpawnedProcess&& other) noexcept {
        if (this != &other) {
            terminate();
            pid_ = std::exchange(other.pid_, -1);
        }
        return *this;
    }

    ~SpawnedProcess() {
        terminate();
    }

    [[nodiscard]] pid_t pid() const { return pid_; }

    [[nodiscard]] bool waitForExit(const int timeout_ms, int* status_out = nullptr) {
        if (pid_ <= 0) {
            if (status_out) {
                *status_out = 0;
            }
            return true;
        }
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);
        int status = 0;
        while (std::chrono::steady_clock::now() < deadline) {
            const pid_t rc = ::waitpid(pid_, &status, WNOHANG);
            if (rc == pid_) {
                pid_ = -1;
                if (status_out) {
                    *status_out = status;
                }
                return true;
            }
            if (rc < 0 && errno != EINTR) {
                break;
            }
            std::this_thread::sleep_for(20ms);
        }
        return false;
    }

    [[nodiscard]] bool crash() {
        if (pid_ <= 0) {
            return true;
        }
        const int kill_result = ::kill(pid_, SIGKILL);
        int status = 0;
        pid_t rc;
        do {
            rc = ::waitpid(pid_, &status, 0);
        } while (rc < 0 && errno == EINTR);
        const bool crashed = kill_result == 0 && rc > 0 && WIFSIGNALED(status) && WTERMSIG(status) == SIGKILL;
        pid_ = -1;
        return crashed;
    }

    void terminate() {
        if (pid_ <= 0) {
            return;
        }
        if (::kill(pid_, SIGKILL) == 0) {
            int status = 0;
            while (::waitpid(pid_, &status, 0) < 0 && errno == EINTR) {
            }
        }
        pid_ = -1;
    }

private:
    pid_t pid_ = -1;
};

[[nodiscard]] SpawnedProcess spawnSelf(const std::vector<std::string>& args) {
    std::vector<char*> argv;
    argv.reserve(args.size() + 1);
    for (const auto& value : args) {
        argv.push_back(const_cast<char*>(value.c_str()));
    }
    argv.push_back(nullptr);

    pid_t pid = -1;
    const int error = ::posix_spawn(&pid, argv[0], nullptr, nullptr, argv.data(), environ);
    if (error != 0) {
        throw std::system_error(error, std::generic_category(), "posix_spawn");
    }
    return SpawnedProcess(pid);
}

[[nodiscard]] std::string endpointForPort(const int port) {
    return std::string{"localhost:"} + std::to_string(port);
}

[[nodiscard]] std::vector<WALEntry> readWalSafe(const fs::path& wal_directory) {
    try {
        WALManagerConfig config;
        config.wal_directory = wal_directory.string();
        WALManager wal(config);
        return wal.readRange(wal.getOldestLSN());
    } catch (...) {
        return {};
    }
}

[[nodiscard]] std::size_t countWalEntries(const fs::path& wal_directory,
                                          const std::string& transaction_id,
                                          const WALEntryType type,
                                          const std::optional<std::string>& phase = std::nullopt) {
    const auto entries = readWalSafe(wal_directory);
    std::size_t count = 0;
    for (const auto& entry : entries) {
        if (entry.transaction_id != transaction_id || entry.type != type) {
            continue;
        }
        if (phase && entry.data.value("phase", std::string{}) != *phase) {
            continue;
        }
        ++count;
    }
    return count;
}

[[nodiscard]] bool waitForWalPredicate(const fs::path& wal_directory,
                                       const std::function<bool(const std::vector<WALEntry>&)>& predicate,
                                       const std::chrono::milliseconds timeout = 8s) {
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < deadline) {
        if (predicate(readWalSafe(wal_directory))) {
            return true;
        }
        std::this_thread::sleep_for(50ms);
    }
    return false;
}

[[nodiscard]] bool waitForCoordinatorDecision(const fs::path& wal_directory,
                                              const std::string& transaction_id) {
    return waitForWalPredicate(
        wal_directory,
        [&](const std::vector<WALEntry>& entries) {
            return std::any_of(entries.begin(), entries.end(), [&](const WALEntry& entry) {
                return entry.transaction_id == transaction_id &&
                       entry.type == WALEntryType::COMMIT_TX &&
                       entry.data.value("phase", std::string{}) == "decision";
            });
        });
}

[[nodiscard]] bool waitForParticipantCommit(const fs::path& wal_directory,
                                            const std::string& transaction_id) {
    return waitForWalPredicate(
        wal_directory,
        [&](const std::vector<WALEntry>& entries) {
            return std::any_of(entries.begin(), entries.end(), [&](const WALEntry& entry) {
                return entry.transaction_id == transaction_id &&
                       entry.type == WALEntryType::COMMIT_TX;
            });
        });
}

[[nodiscard]] bool waitForFile(const fs::path& path,
                               const std::chrono::milliseconds timeout = 8s) {
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < deadline) {
        if (fs::exists(path)) {
            return true;
        }
        std::this_thread::sleep_for(20ms);
    }
    return fs::exists(path);
}

ShardRPCClient::Config makeRpcConfig(const fs::path& root,
                                     const std::string& endpoint,
                                     const std::string& shard_id,
                                     const int timeout_ms = 800) {
    ShardRPCClient::Config config;
    config.endpoint = endpoint;
    config.shard_id = shard_id;
    config.timeout_ms = timeout_ms;
    config.max_retries = 1;
    config.retry_delay_ms = 10;
    config.enable_circuit_breaker = false;
    config.tls_cert_path = (root / "coordinator-cert.pem").string();
    config.tls_key_path = (root / "coordinator-key.pem").string();
    config.tls_ca_cert_path = (root / "participant-cert.pem").string();
    return config;
}

// Test-only transparent proxy: inject TCP blackholes without terminating TLS.
class TcpPartitionProxy {
public:
    explicit TcpPartitionProxy(const int target_port) {
        listener_ = ::socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
        if (listener_ < 0) {
            throw std::system_error(errno, std::generic_category(), "partition proxy socket");
        }

        int reuse_address = 1;
        (void)::setsockopt(listener_, SOL_SOCKET, SO_REUSEADDR, &reuse_address, sizeof(reuse_address));
        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        address.sin_port = 0;
        if (::bind(listener_, reinterpret_cast<const sockaddr*>(&address), sizeof(address)) != 0 ||
            ::listen(listener_, 8) != 0) {
            const int error = errno;
            ::close(listener_);
            listener_ = -1;
            throw std::system_error(error, std::generic_category(), "partition proxy bind/listen");
        }

        socklen_t address_size = sizeof(address);
        if (::getsockname(listener_, reinterpret_cast<sockaddr*>(&address), &address_size) != 0) {
            const int error = errno;
            ::close(listener_);
            listener_ = -1;
            throw std::system_error(error, std::generic_category(), "partition proxy getsockname");
        }
        port_ = ntohs(address.sin_port);
        target_port_ = target_port;
        accept_thread_ = std::thread([this] { acceptConnections(); });
    }

    TcpPartitionProxy(const TcpPartitionProxy&) = delete;
    TcpPartitionProxy& operator=(const TcpPartitionProxy&) = delete;

    ~TcpPartitionProxy() {
        stopping_.store(true, std::memory_order_release);
        if (listener_ >= 0) {
            (void)::shutdown(listener_, SHUT_RDWR);
        }
        if (accept_thread_.joinable()) {
            accept_thread_.join();
        }
        for (const int socket : sockets_) {
            (void)::shutdown(socket, SHUT_RDWR);
        }
        for (auto& thread : connection_threads_) {
            if (thread.joinable()) {
                thread.join();
            }
        }
        for (const int socket : sockets_) {
            ::close(socket);
        }
        if (listener_ >= 0) {
            ::close(listener_);
        }
    }

    [[nodiscard]] std::string endpoint() const { return endpointForPort(port_); }

    void partition() { fault_mode_.store(FaultMode::BothDirections, std::memory_order_release); }
    void dropParticipantResponses() {
        fault_mode_.store(FaultMode::ParticipantResponses, std::memory_order_release);
    }
    void dropCoordinatorRequests() {
        fault_mode_.store(FaultMode::CoordinatorRequests, std::memory_order_release);
    }
    void delayCoordinatorRequests(const std::chrono::milliseconds delay) {
        coordinator_request_latency_ms_.store(static_cast<int>(delay.count()), std::memory_order_release);
        fault_mode_.store(FaultMode::CoordinatorRequestLatency, std::memory_order_release);
    }
    void heal() { fault_mode_.store(FaultMode::Healthy, std::memory_order_release); }
    [[nodiscard]] std::uint64_t droppedParticipantResponseBytes() const {
        return dropped_participant_response_bytes_.load(std::memory_order_acquire);
    }
    [[nodiscard]] std::uint64_t droppedCoordinatorRequestBytes() const {
        return dropped_coordinator_request_bytes_.load(std::memory_order_acquire);
    }
    [[nodiscard]] std::uint64_t delayedCoordinatorRequestBytes() const {
        return delayed_coordinator_request_bytes_.load(std::memory_order_acquire);
    }

private:
    enum class FaultMode : uint8_t {
        Healthy,
        BothDirections,
        ParticipantResponses,
        CoordinatorRequests,
        CoordinatorRequestLatency
    };

    void acceptConnections() {
        while (!stopping_.load(std::memory_order_acquire)) {
            pollfd descriptor{listener_, POLLIN, 0};
            const int ready = ::poll(&descriptor, 1, 50);
            if (ready <= 0 || (descriptor.revents & POLLIN) == 0) {
                continue;
            }

            const int client = ::accept4(listener_, nullptr, nullptr, SOCK_CLOEXEC);
            if (client < 0) {
                if (errno == EINTR || stopping_.load(std::memory_order_acquire)) {
                    continue;
                }
                continue;
            }

            const int upstream = ::socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
            if (upstream < 0) {
                ::close(client);
                continue;
            }
            sockaddr_in target{};
            target.sin_family = AF_INET;
            target.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
            target.sin_port = htons(static_cast<uint16_t>(target_port_));
            if (::connect(upstream, reinterpret_cast<const sockaddr*>(&target), sizeof(target)) != 0) {
                ::close(client);
                ::close(upstream);
                continue;
            }

            sockets_.push_back(client);
            sockets_.push_back(upstream);
            connection_threads_.emplace_back([this, client, upstream] {
                relayConnection(client, upstream);
            });
        }
    }

    [[nodiscard]] bool relayBytes(const int source, const int destination,
                                  const bool drop_bytes,
                                  const bool participant_response,
                                  const bool delay_request) {
        char buffer[16 * 1024];
        const ssize_t received = ::recv(source, buffer, sizeof(buffer), 0);
        if (received <= 0) {
            return false;
        }

        if (drop_bytes) {
            // Drop raw TLS bytes without parsing records or gRPC frame boundaries.
            if (participant_response) {
                dropped_participant_response_bytes_.fetch_add(
                    static_cast<std::uint64_t>(received), std::memory_order_release);
            } else {
                dropped_coordinator_request_bytes_.fetch_add(
                    static_cast<std::uint64_t>(received), std::memory_order_release);
            }
            return true;
        }

        if (delay_request && !coordinator_request_delay_applied_.exchange(true, std::memory_order_acq_rel)) {
            delayed_coordinator_request_bytes_.fetch_add(
                static_cast<std::uint64_t>(received), std::memory_order_release);
            std::this_thread::sleep_for(std::chrono::milliseconds(
                coordinator_request_latency_ms_.load(std::memory_order_acquire)));
        }

        ssize_t sent = 0;
        while (sent < received) {
            const ssize_t count = ::send(destination, buffer + sent,
                                         static_cast<std::size_t>(received - sent), MSG_NOSIGNAL);
            if (count < 0 && errno == EINTR) {
                continue;
            }
            if (count <= 0) {
                return false;
            }
            sent += count;
        }
        return true;
    }

    void relayConnection(const int client, const int upstream) {
        constexpr int kClientSocketIndex = 0;
        constexpr int kParticipantSocketIndex = 1;
        pollfd descriptors[2]{{client, 0, 0}, {upstream, 0, 0}};
        while (!stopping_.load(std::memory_order_acquire)) {
            const auto fault_mode = fault_mode_.load(std::memory_order_acquire);
            const short read_events = fault_mode == FaultMode::BothDirections ? 0 : POLLIN;
            descriptors[0].events = read_events;
            descriptors[1].events = read_events;
            descriptors[0].revents = 0;
            descriptors[1].revents = 0;

            const int ready = ::poll(descriptors, 2, 50);
            if (ready < 0 && errno == EINTR) {
                continue;
            }
            if (ready < 0) {
                return;
            }
            for (int index = 0; index < 2; ++index) {
                if ((descriptors[index].revents & (POLLERR | POLLHUP | POLLNVAL)) != 0) {
                    return;
                }
                if ((descriptors[index].revents & POLLIN) != 0) {
                    const int source = descriptors[index].fd;
                    const int destination_index =
                        index == kClientSocketIndex ? kParticipantSocketIndex : kClientSocketIndex;
                    const int destination = descriptors[destination_index].fd;
                    const bool drop_response =
                        fault_mode == FaultMode::ParticipantResponses &&
                        index == kParticipantSocketIndex;
                    const bool drop_request =
                        fault_mode == FaultMode::CoordinatorRequests &&
                        index == kClientSocketIndex;
                    const bool delay_request =
                        fault_mode == FaultMode::CoordinatorRequestLatency &&
                        index == kClientSocketIndex;
                    if (!relayBytes(source, destination, drop_response || drop_request,
                                    drop_response, delay_request)) {
                        return;
                    }
                }
            }
        }
    }

    int listener_ = -1;
    int port_ = 0;
    int target_port_ = 0;
    std::atomic<FaultMode> fault_mode_{FaultMode::Healthy};
    std::atomic<std::uint64_t> dropped_participant_response_bytes_{0};
    std::atomic<std::uint64_t> dropped_coordinator_request_bytes_{0};
    std::atomic<std::uint64_t> delayed_coordinator_request_bytes_{0};
    std::atomic<int> coordinator_request_latency_ms_{0};
    std::atomic<bool> coordinator_request_delay_applied_{false};
    std::atomic<bool> stopping_{false};
    std::thread accept_thread_;
    std::vector<std::thread> connection_threads_;
    std::vector<int> sockets_;
};

class ParticipantProcess {
public:
    ParticipantProcess(const fs::path& root,
                       const std::string& shard_id,
                       const std::string& behavior,
                       const std::optional<int> fixed_port = std::nullopt)
        : root_(root), shard_id_(shard_id), behavior_(behavior),
          port_(fixed_port.value_or(loopbackPort())) {
        int pipe_fds[2];
        if (::pipe(pipe_fds) != 0) {
            throw std::system_error(errno, std::generic_category(), "pipe");
        }
        clearCloseOnExec(pipe_fds[1]);

        const std::vector<std::string> args = {
            "/proc/self/exe",
            std::string{kParticipantMode},
            root_.string(),
            shard_id_,
            std::to_string(port_),
            behavior_,
            std::to_string(::getpid()),
            std::to_string(pipe_fds[1])
        };
        process_ = spawnSelf(args);
        ::close(pipe_fds[1]);

        const bool ready = waitForByte(pipe_fds[0], 'S', 15000);
        ::close(pipe_fds[0]);
        if (!ready) {
            process_.terminate();
            throw std::runtime_error("participant process failed to report readiness");
        }
    }

    ParticipantProcess(const ParticipantProcess&) = delete;
    ParticipantProcess& operator=(const ParticipantProcess&) = delete;

    [[nodiscard]] int port() const { return port_; }
    [[nodiscard]] std::string endpoint() const { return endpointForPort(port_); }

    [[nodiscard]] bool crash() { return process_.crash(); }
    [[nodiscard]] bool waitForExit(const int timeout_ms, int* status_out = nullptr) {
        return process_.waitForExit(timeout_ms, status_out);
    }

private:
    fs::path root_;
    std::string shard_id_;
    std::string behavior_;
    int port_ = 0;
    SpawnedProcess process_;
};

class CoordinatorRunner {
public:
    CoordinatorRunner(const fs::path& root,
                      const std::string& transaction_id,
                      const std::string& mode,
                      const std::string& endpoint_a,
                      const std::string& endpoint_b,
                      const std::string& endpoint_c = {},
                      const std::string& endpoint_d = {})
        : transaction_id_(transaction_id),
          coordinator_wal_(root / "coordinator-wal") {
        const std::vector<std::string> args = {
            "/proc/self/exe",
            std::string{kCoordinatorMode},
            root.string(),
            transaction_id,
            mode,
            endpoint_a,
            endpoint_b,
            endpoint_c,
            endpoint_d,
            std::to_string(::getpid())
        };
        process_ = spawnSelf(args);
    }

    [[nodiscard]] bool waitForExit(const int timeout_ms, int* status = nullptr) {
        return process_.waitForExit(timeout_ms, status);
    }

    [[nodiscard]] bool crash() { return process_.crash(); }

    [[nodiscard]] const fs::path& coordinatorWal() const { return coordinator_wal_; }

private:
    std::string transaction_id_;
    fs::path coordinator_wal_;
    SpawnedProcess process_;
};

void writeCryptoMaterial(const fs::path& root) {
    const auto coordinator_tls =
        themis::tests::generateSelfSignedPemMaterial("701", "coord", {kCoordinatorIdentity});
    const auto participant_tls =
        themis::tests::generateSelfSignedPemMaterial("702", "localhost", {"localhost"});
    const auto participant_a_tls =
        themis::tests::generateSelfSignedPemMaterial("703", std::string(kParticipantACertName), {"localhost"});
    const auto participant_b_tls =
        themis::tests::generateSelfSignedPemMaterial("704", std::string(kParticipantBCertName), {"localhost"});
    const auto participant_c_tls =
        themis::tests::generateSelfSignedPemMaterial("705", "participant-c", {"localhost"});
    const auto participant_d_tls =
        themis::tests::generateSelfSignedPemMaterial("706", std::string(kParticipantDCertName), {"localhost"});

    writePem(root / "coordinator-cert.pem", coordinator_tls.certificate_pem);
    writePem(root / "coordinator-key.pem", coordinator_tls.private_key_pem);

    // Both participant servers trust the same coordinator certificate.
    writePem(root / "trusted-clients.pem", coordinator_tls.certificate_pem);

    // Coordinator trusts the participant certificates for server verification.
    writePem(root / "participant-cert.pem",
             participant_tls.certificate_pem + participant_a_tls.certificate_pem +
                 participant_b_tls.certificate_pem + participant_c_tls.certificate_pem +
                 participant_d_tls.certificate_pem);

    // Shared cert/key simplify the endpoint config for both participant processes.
    writePem(root / "participant-a-cert.pem", participant_a_tls.certificate_pem);
    writePem(root / "participant-a-key.pem", participant_a_tls.private_key_pem);
    writePem(root / "participant-b-cert.pem", participant_b_tls.certificate_pem);
    writePem(root / "participant-b-key.pem", participant_b_tls.private_key_pem);
    writePem(root / "participant-c-cert.pem", participant_c_tls.certificate_pem);
    writePem(root / "participant-c-key.pem", participant_c_tls.private_key_pem);
    writePem(root / "participant-d-cert.pem", participant_d_tls.certificate_pem);
    writePem(root / "participant-d-key.pem", participant_d_tls.private_key_pem);
}

int runParticipantProcess(const fs::path& root,
                          const std::string& shard_id,
                          const std::string& port,
                          const std::string& behavior,
                          const pid_t parent,
                          const int marker_fd) {
    if (::prctl(PR_SET_PDEATHSIG, SIGKILL) != 0 || ::getppid() != parent) {
        return 101;
    }

    TwoPhaseCommitParticipant::Config participant_config;
    participant_config.wal_directory = (root / (shard_id + "-wal")).string();
    participant_config.prepare_timeout = 5000ms;

    themis::sharding::EpochFencingConfig fencing;
    fencing.shard_id = shard_id;
    fencing.node_id = "participant-node-" + shard_id;
    fencing.auto_stonith = false;
    participant_config.fencing_manager =
        std::make_shared<themis::sharding::EpochFencingManager>(fencing);

    bool first_commit = true;
    TwoPhaseCommitParticipant participant(
        shard_id,
        participant_config,
        [](const std::string&, const nlohmann::json&) { return true; },
        [&](const std::string&, const nlohmann::json&, int64_t) {
            if (behavior == "signal_then_hang_on_commit" && first_commit) {
                first_commit = false;
                writePem(root / "participant-a-commit-entered", "S");
                for (;;) {
                    ::pause();
                }
            }
            if (behavior == "hang_on_commit" && first_commit) {
                first_commit = false;
                for (;;) {
                    ::pause();
                }
            }
            if (behavior == "crash_on_commit" && first_commit) {
                first_commit = false;
                ::kill(::getpid(), SIGKILL);
            }
            return true;
        },
        [](const std::string&) {});

    ShardRPCServer::Config server_config;
    server_config.listen_address = "127.0.0.1:" + port;
    const std::string participant_name =
        shard_id == kShardA ? "participant-a" :
        shard_id == kShardB ? "participant-b" :
        shard_id == kShardC ? "participant-c" : "participant-d";
    server_config.tls_cert_path = (root / (participant_name + "-cert.pem")).string();
    server_config.tls_key_path = (root / (participant_name + "-key.pem")).string();
    server_config.tls_ca_cert_path = (root / "trusted-clients.pem").string();
    server_config.coordinator_identity_bindings = {
        {"x509_subject_alternative_name", std::string{kCoordinatorIdentity}, std::string{kCoordinatorId}}
    };

    ShardRPCServer server(server_config);
    server.setRequestHandler(&participant);
    if (!server.start()) {
        return 102;
    }

    const char marker = 'S';
    if (::write(marker_fd, &marker, sizeof(marker)) != sizeof(marker)) {
        server.stop();
        return 103;
    }
    ::close(marker_fd);

    server.wait();
    return 0;
}

int runCoordinatorProcess(const fs::path& root,
                          const std::string& transaction_id,
                          const std::string& mode,
                          const std::string& endpoint_a,
                          const std::string& endpoint_b,
                          const std::string& endpoint_c,
                          const std::string& endpoint_d,
                          const pid_t parent) {
    if (::prctl(PR_SET_PDEATHSIG, SIGKILL) != 0 || ::getppid() != parent) {
        return 201;
    }

    TwoPhaseCommitCoordinator::Config coordinator_config;
    coordinator_config.wal_directory = (root / "coordinator-wal").string();
    coordinator_config.prepare_timeout = 1000ms;
    coordinator_config.fencing_token_provider = [](const std::string& shard_id) -> std::optional<EpochToken> {
        EpochToken token;
        token.epoch = 1;
        token.issuer = std::string{kCoordinatorId};
        token.shard_id = shard_id;
        token.issued_at = std::chrono::system_clock::now();
        return token;
    };

    TwoPhaseCommitCoordinator coordinator(std::string{kCoordinatorId}, coordinator_config);

    std::vector<std::unique_ptr<ShardRPCClientAdapter>> adapters;
    adapters.emplace_back(std::make_unique<ShardRPCClientAdapter>(
        makeRpcConfig(root, endpoint_a, std::string{kShardA}, 1200)));
    adapters.emplace_back(std::make_unique<ShardRPCClientAdapter>(
        makeRpcConfig(root, endpoint_b, std::string{kShardB}, 1200)));
    coordinator.registerParticipant(std::string{kShardA}, adapters[0].get());
    coordinator.registerParticipant(std::string{kShardB}, adapters[1].get());
    if (!endpoint_c.empty()) {
        adapters.emplace_back(std::make_unique<ShardRPCClientAdapter>(
            makeRpcConfig(root, endpoint_c, std::string{kShardC}, 1200)));
        coordinator.registerParticipant(std::string{kShardC}, adapters[2].get());
    }
    if (!endpoint_d.empty()) {
        adapters.emplace_back(std::make_unique<ShardRPCClientAdapter>(
            makeRpcConfig(root, endpoint_d, std::string{kShardD}, 1200)));
        coordinator.registerParticipant(std::string{kShardD}, adapters[3].get());
    }

    if (mode == "recover") {
        (void)coordinator.recoverInDoubtTransactions();
        const auto state = coordinator.getTransactionState(transaction_id);
        const auto decision = coordinator.getTransactionDecision(transaction_id);
        return state && *state == CoordinatorTxnState::COMPLETED && decision && *decision ? 0 : 202;
    }

    const nlohmann::json ops_a = nlohmann::json::array({{{"type", "put"}, {"key", "alpha"}, {"value", "A"}}});
    const nlohmann::json ops_b = nlohmann::json::array({{{"type", "put"}, {"key", "beta"}, {"value", "B"}}});
    std::map<std::string, nlohmann::json> operations{
        {std::string{kShardA}, ops_a},
        {std::string{kShardB}, ops_b}
    };
    if (!endpoint_c.empty()) {
        operations.emplace(std::string{kShardC},
                           nlohmann::json::array({{{"type", "put"}, {"key", "gamma"}, {"value", "C"}}}));
    }
    if (!endpoint_d.empty()) {
        operations.emplace(std::string{kShardD},
                           nlohmann::json::array({{{"type", "put"}, {"key", "delta"}, {"value", "D"}}}));
    }
    const auto outcome = coordinator.commit(transaction_id, operations);

    if (outcome.result == CoordinatorTxnResult::COMMITTED) {
        return 0;
    }

    const auto decision = coordinator.getTransactionDecision(transaction_id);
    const auto state = coordinator.getTransactionState(transaction_id);
    if (outcome.result == CoordinatorTxnResult::FAILURE && decision && *decision &&
        state && *state == CoordinatorTxnState::COMMIT_DECIDED) {
        return 2;
    }

    return 203;
}

void expectCommittedHistoryConsistent(const fs::path& coordinator_wal,
                                      const std::string& transaction_id,
                                      const std::vector<std::string>& participants) {
    EXPECT_EQ(countWalEntries(coordinator_wal, transaction_id, WALEntryType::COMMIT_TX, "decision"), 1u);
    EXPECT_EQ(countWalEntries(coordinator_wal, transaction_id, WALEntryType::COMMIT_TX, "complete"), 1u);
    for (const auto& shard : participants) {
        const auto participant_wal = coordinator_wal.parent_path() / (shard + "-wal");
        EXPECT_EQ(countWalEntries(participant_wal, transaction_id, WALEntryType::COMMIT_TX), 1u)
            << "participant " << shard << " must have one durable COMMIT";
        EXPECT_EQ(countWalEntries(participant_wal, transaction_id, WALEntryType::ABORT_TX), 0u)
            << "participant " << shard << " must not record ABORT after a durable COMMIT decision";
    }
}

TEST(DIS10RealTransportProcessRecovery, CommitsAcrossRealGrpcMtlsParticipants) {
    const FixtureDirectory fixture("dis10-real-transport-success");
    writeCryptoMaterial(fixture.path());

    ParticipantProcess participant_a(fixture.path(), std::string{kShardA}, "normal");
    ParticipantProcess participant_b(fixture.path(), std::string{kShardB}, "normal");

    const std::string transaction_id = "dis10-success";
    CoordinatorRunner coordinator(
        fixture.path(), transaction_id, "commit", participant_a.endpoint(), participant_b.endpoint());

    int status = 0;
    ASSERT_TRUE(coordinator.waitForExit(20000, &status));
    ASSERT_TRUE(WIFEXITED(status));
    ASSERT_EQ(WEXITSTATUS(status), 0);

    const auto coordinator_wal = fixture.path() / "coordinator-wal";
    EXPECT_EQ(countWalEntries(coordinator_wal, transaction_id, WALEntryType::COMMIT_TX, "decision"), 1u);
    EXPECT_EQ(countWalEntries(coordinator_wal, transaction_id, WALEntryType::COMMIT_TX, "complete"), 1u);

    for (const auto& shard : {std::string{kShardA}, std::string{kShardB}}) {
        const auto wal = fixture.path() / (shard + "-wal");
        EXPECT_GE(countWalEntries(wal, transaction_id, WALEntryType::PREPARE_TX), 1u);
        EXPECT_GE(countWalEntries(wal, transaction_id, WALEntryType::COMMIT_TX), 1u);
        EXPECT_EQ(countWalEntries(wal, transaction_id, WALEntryType::ABORT_TX), 0u);
    }
}

TEST(DIS10RealTransportProcessRecovery, RecoversCommitAfterCoordinatorSigkill) {
    const FixtureDirectory fixture("dis10-real-transport-coordinator-crash");
    writeCryptoMaterial(fixture.path());

    ParticipantProcess participant_a(fixture.path(), std::string{kShardA}, "hang_on_commit");
    ParticipantProcess participant_b(fixture.path(), std::string{kShardB}, "normal");

    const std::string transaction_id = "dis10-coordinator-crash";
    CoordinatorRunner coordinator(
        fixture.path(), transaction_id, "commit", participant_a.endpoint(), participant_b.endpoint());

    const auto coordinator_wal = fixture.path() / "coordinator-wal";
    ASSERT_TRUE(waitForCoordinatorDecision(coordinator_wal, transaction_id));
    ASSERT_TRUE(coordinator.crash());

    ASSERT_TRUE(participant_a.crash());
    ParticipantProcess restarted_a(
        fixture.path(), std::string{kShardA}, "normal", participant_a.port());

    CoordinatorRunner recovery(
        fixture.path(), transaction_id, "recover", restarted_a.endpoint(), participant_b.endpoint());
    int recovery_status = 0;
    ASSERT_TRUE(recovery.waitForExit(20000, &recovery_status));
    ASSERT_TRUE(WIFEXITED(recovery_status));
    ASSERT_EQ(WEXITSTATUS(recovery_status), 0);

    EXPECT_EQ(countWalEntries(coordinator_wal, transaction_id, WALEntryType::COMMIT_TX, "decision"), 1u);
    EXPECT_EQ(countWalEntries(coordinator_wal, transaction_id, WALEntryType::COMMIT_TX, "complete"), 1u);

    for (const auto& shard : {std::string{kShardA}, std::string{kShardB}}) {
        const auto wal = fixture.path() / (shard + "-wal");
        EXPECT_TRUE(waitForParticipantCommit(wal, transaction_id));
    }
}

TEST(DIS10RealTransportProcessRecovery, HealsParticipantUnavailabilityAfterRestart) {
    const FixtureDirectory fixture("dis10-real-transport-participant-restart");
    writeCryptoMaterial(fixture.path());

    ParticipantProcess participant_a(fixture.path(), std::string{kShardA}, "normal");
    ParticipantProcess participant_b(fixture.path(), std::string{kShardB}, "crash_on_commit");

    const std::string transaction_id = "dis10-participant-restart";
    CoordinatorRunner coordinator(
        fixture.path(), transaction_id, "commit", participant_a.endpoint(), participant_b.endpoint());

    int status = 0;
    ASSERT_TRUE(coordinator.waitForExit(20000, &status));
    ASSERT_TRUE(WIFEXITED(status));
    ASSERT_EQ(WEXITSTATUS(status), 2);

    const auto coordinator_wal = fixture.path() / "coordinator-wal";
    EXPECT_EQ(countWalEntries(coordinator_wal, transaction_id, WALEntryType::COMMIT_TX, "decision"), 1u);
    EXPECT_EQ(countWalEntries(coordinator_wal, transaction_id, WALEntryType::COMMIT_TX, "complete"), 0u);

    int participant_b_status = 0;
    EXPECT_TRUE(participant_b.waitForExit(5000, &participant_b_status));

    ParticipantProcess restarted_b(
        fixture.path(), std::string{kShardB}, "normal", participant_b.port());
    CoordinatorRunner recovery(
        fixture.path(), transaction_id, "recover", participant_a.endpoint(), restarted_b.endpoint());

    int recovery_status = 0;
    ASSERT_TRUE(recovery.waitForExit(20000, &recovery_status));
    ASSERT_TRUE(WIFEXITED(recovery_status));
    ASSERT_EQ(WEXITSTATUS(recovery_status), 0);

    EXPECT_EQ(countWalEntries(coordinator_wal, transaction_id, WALEntryType::COMMIT_TX, "complete"), 1u);
    EXPECT_TRUE(waitForParticipantCommit(fixture.path() / (std::string{kShardA} + "-wal"), transaction_id));
    EXPECT_TRUE(waitForParticipantCommit(fixture.path() / (std::string{kShardB} + "-wal"), transaction_id));
}

TEST(DIS10RealTransportProcessRecovery, RecoversCommitAfterTcpPartitionHeals) {
    const FixtureDirectory fixture("dis10-real-transport-partition");
    writeCryptoMaterial(fixture.path());

    ParticipantProcess participant_a(
        fixture.path(), std::string{kShardA}, "signal_then_hang_on_commit");
    ParticipantProcess participant_b(fixture.path(), std::string{kShardB}, "normal");
    TcpPartitionProxy proxy(participant_b.port());

    const std::string transaction_id = "dis10-partition-healing";
    CoordinatorRunner coordinator(
        fixture.path(), transaction_id, "commit", participant_a.endpoint(), proxy.endpoint());

    const auto coordinator_wal = fixture.path() / "coordinator-wal";
    ASSERT_TRUE(waitForCoordinatorDecision(coordinator_wal, transaction_id));
    ASSERT_TRUE(waitForFile(fixture.path() / "participant-a-commit-entered"));

    proxy.partition();
    int status = 0;
    ASSERT_TRUE(coordinator.waitForExit(20000, &status));
    ASSERT_TRUE(WIFEXITED(status));
    ASSERT_EQ(WEXITSTATUS(status), 2);

    EXPECT_EQ(countWalEntries(coordinator_wal, transaction_id, WALEntryType::COMMIT_TX, "decision"), 1u);
    EXPECT_EQ(countWalEntries(coordinator_wal, transaction_id, WALEntryType::COMMIT_TX, "complete"), 0u);
    const auto participant_b_wal = fixture.path() / (std::string{kShardB} + "-wal");
    EXPECT_GE(countWalEntries(participant_b_wal, transaction_id, WALEntryType::PREPARE_TX), 1u);
    EXPECT_EQ(countWalEntries(participant_b_wal, transaction_id, WALEntryType::COMMIT_TX), 0u);

    ASSERT_TRUE(participant_a.crash());
    ParticipantProcess restarted_a(
        fixture.path(), std::string{kShardA}, "normal", participant_a.port());
    proxy.heal();

    CoordinatorRunner recovery(
        fixture.path(), transaction_id, "recover", restarted_a.endpoint(), proxy.endpoint());
    int recovery_status = 0;
    ASSERT_TRUE(recovery.waitForExit(20000, &recovery_status));
    ASSERT_TRUE(WIFEXITED(recovery_status));
    ASSERT_EQ(WEXITSTATUS(recovery_status), 0);

    EXPECT_EQ(countWalEntries(coordinator_wal, transaction_id, WALEntryType::COMMIT_TX, "complete"), 1u);
    EXPECT_TRUE(waitForParticipantCommit(fixture.path() / (std::string{kShardA} + "-wal"), transaction_id));
    EXPECT_TRUE(waitForParticipantCommit(participant_b_wal, transaction_id));
}

TEST(DIS10RealTransportProcessRecovery, RecoversCommitAfterAsymmetricTcpPartitionHeals) {
    const FixtureDirectory fixture("dis10-real-transport-asymmetric-partition");
    writeCryptoMaterial(fixture.path());

    ParticipantProcess participant_a(
        fixture.path(), std::string{kShardA}, "signal_then_hang_on_commit");
    ParticipantProcess participant_b(fixture.path(), std::string{kShardB}, "normal");
    TcpPartitionProxy proxy(participant_b.port());

    const std::string transaction_id = "dis10-asymmetric-partition";
    CoordinatorRunner coordinator(
        fixture.path(), transaction_id, "commit", participant_a.endpoint(), proxy.endpoint());

    const auto coordinator_wal = fixture.path() / "coordinator-wal";
    ASSERT_TRUE(waitForCoordinatorDecision(coordinator_wal, transaction_id));
    ASSERT_TRUE(waitForFile(fixture.path() / "participant-a-commit-entered"));

    proxy.dropParticipantResponses();
    int status = 0;
    ASSERT_TRUE(coordinator.waitForExit(20000, &status));
    ASSERT_TRUE(WIFEXITED(status));
    ASSERT_EQ(WEXITSTATUS(status), 2);

    EXPECT_GT(proxy.droppedParticipantResponseBytes(), 0u);
    EXPECT_EQ(countWalEntries(coordinator_wal, transaction_id, WALEntryType::COMMIT_TX, "decision"), 1u);
    EXPECT_EQ(countWalEntries(coordinator_wal, transaction_id, WALEntryType::COMMIT_TX, "complete"), 0u);
    const auto participant_b_wal = fixture.path() / (std::string{kShardB} + "-wal");
    EXPECT_GE(countWalEntries(participant_b_wal, transaction_id, WALEntryType::PREPARE_TX), 1u);
    EXPECT_EQ(countWalEntries(participant_b_wal, transaction_id, WALEntryType::COMMIT_TX), 1u);

    ASSERT_TRUE(participant_a.crash());
    ParticipantProcess restarted_a(
        fixture.path(), std::string{kShardA}, "normal", participant_a.port());
    proxy.heal();

    CoordinatorRunner recovery(
        fixture.path(), transaction_id, "recover", restarted_a.endpoint(), proxy.endpoint());
    int recovery_status = 0;
    ASSERT_TRUE(recovery.waitForExit(20000, &recovery_status));
    ASSERT_TRUE(WIFEXITED(recovery_status));
    ASSERT_EQ(WEXITSTATUS(recovery_status), 0);

    EXPECT_EQ(countWalEntries(coordinator_wal, transaction_id, WALEntryType::COMMIT_TX, "complete"), 1u);
    EXPECT_TRUE(waitForParticipantCommit(fixture.path() / (std::string{kShardA} + "-wal"), transaction_id));
    EXPECT_EQ(countWalEntries(participant_b_wal, transaction_id, WALEntryType::COMMIT_TX), 1u);
}

TEST(DIS10RealTransportProcessRecovery, RecoversCommitAfterReverseAsymmetricTcpPartitionHeals) {
    const FixtureDirectory fixture("dis10-real-transport-reverse-asymmetric-partition");
    writeCryptoMaterial(fixture.path());

    ParticipantProcess participant_a(
        fixture.path(), std::string{kShardA}, "signal_then_hang_on_commit");
    ParticipantProcess participant_b(fixture.path(), std::string{kShardB}, "normal");
    TcpPartitionProxy proxy(participant_b.port());

    const std::string transaction_id = "dis10-reverse-asymmetric-partition";
    CoordinatorRunner coordinator(
        fixture.path(), transaction_id, "commit", participant_a.endpoint(), proxy.endpoint());

    const auto coordinator_wal = fixture.path() / "coordinator-wal";
    ASSERT_TRUE(waitForCoordinatorDecision(coordinator_wal, transaction_id));
    ASSERT_TRUE(waitForFile(fixture.path() / "participant-a-commit-entered"));

    proxy.dropCoordinatorRequests();
    int status = 0;
    ASSERT_TRUE(coordinator.waitForExit(20000, &status));
    ASSERT_TRUE(WIFEXITED(status));
    ASSERT_EQ(WEXITSTATUS(status), 2);

    EXPECT_GT(proxy.droppedCoordinatorRequestBytes(), 0u);
    EXPECT_EQ(countWalEntries(coordinator_wal, transaction_id, WALEntryType::COMMIT_TX, "decision"), 1u);
    EXPECT_EQ(countWalEntries(coordinator_wal, transaction_id, WALEntryType::COMMIT_TX, "complete"), 0u);
    const auto participant_b_wal = fixture.path() / (std::string{kShardB} + "-wal");
    EXPECT_GE(countWalEntries(participant_b_wal, transaction_id, WALEntryType::PREPARE_TX), 1u);
    EXPECT_EQ(countWalEntries(participant_b_wal, transaction_id, WALEntryType::COMMIT_TX), 0u);

    ASSERT_TRUE(participant_a.crash());
    ParticipantProcess restarted_a(
        fixture.path(), std::string{kShardA}, "normal", participant_a.port());
    proxy.heal();

    CoordinatorRunner recovery(
        fixture.path(), transaction_id, "recover", restarted_a.endpoint(), proxy.endpoint());
    int recovery_status = 0;
    ASSERT_TRUE(recovery.waitForExit(20000, &recovery_status));
    ASSERT_TRUE(WIFEXITED(recovery_status));
    ASSERT_EQ(WEXITSTATUS(recovery_status), 0);

    EXPECT_EQ(countWalEntries(coordinator_wal, transaction_id, WALEntryType::COMMIT_TX, "complete"), 1u);
    EXPECT_TRUE(waitForParticipantCommit(fixture.path() / (std::string{kShardA} + "-wal"), transaction_id));
    EXPECT_EQ(countWalEntries(participant_b_wal, transaction_id, WALEntryType::COMMIT_TX), 1u);
}

TEST(DIS10RealTransportProcessRecovery, RecoversCommitAfterCoordinatorRequestLatency) {
    const FixtureDirectory fixture("dis10-real-transport-latency");
    writeCryptoMaterial(fixture.path());

    ParticipantProcess participant_a(
        fixture.path(), std::string{kShardA}, "signal_then_hang_on_commit");
    ParticipantProcess participant_b(fixture.path(), std::string{kShardB}, "normal");
    TcpPartitionProxy proxy(participant_b.port());

    const std::string transaction_id = "dis10-request-latency";
    CoordinatorRunner coordinator(
        fixture.path(), transaction_id, "commit", participant_a.endpoint(), proxy.endpoint());
    const auto coordinator_wal = fixture.path() / "coordinator-wal";
    ASSERT_TRUE(waitForCoordinatorDecision(coordinator_wal, transaction_id));
    ASSERT_TRUE(waitForFile(fixture.path() / "participant-a-commit-entered"));

    proxy.delayCoordinatorRequests(1800ms);
    int status = 0;
    ASSERT_TRUE(coordinator.waitForExit(20000, &status));
    ASSERT_TRUE(WIFEXITED(status));
    ASSERT_EQ(WEXITSTATUS(status), 2);

    EXPECT_GT(proxy.delayedCoordinatorRequestBytes(), 0u);
    EXPECT_EQ(countWalEntries(coordinator_wal, transaction_id, WALEntryType::COMMIT_TX, "decision"), 1u);
    EXPECT_EQ(countWalEntries(coordinator_wal, transaction_id, WALEntryType::COMMIT_TX, "complete"), 0u);

    ASSERT_TRUE(participant_a.crash());
    ParticipantProcess restarted_a(
        fixture.path(), std::string{kShardA}, "normal", participant_a.port());
    proxy.heal();

    CoordinatorRunner recovery(
        fixture.path(), transaction_id, "recover", restarted_a.endpoint(), proxy.endpoint());
    int recovery_status = 0;
    ASSERT_TRUE(recovery.waitForExit(20000, &recovery_status));
    ASSERT_TRUE(WIFEXITED(recovery_status));
    ASSERT_EQ(WEXITSTATUS(recovery_status), 0);

    EXPECT_EQ(countWalEntries(coordinator_wal, transaction_id, WALEntryType::COMMIT_TX, "complete"), 1u);
    EXPECT_EQ(countWalEntries(fixture.path() / (std::string{kShardA} + "-wal"),
                              transaction_id, WALEntryType::COMMIT_TX), 1u);
    EXPECT_EQ(countWalEntries(fixture.path() / (std::string{kShardB} + "-wal"),
                              transaction_id, WALEntryType::COMMIT_TX), 1u);
}

TEST(DIS10RealTransportProcessRecovery, RecoversThreeParticipantCommitAfterParticipantCrash) {
    const FixtureDirectory fixture("dis10-real-transport-three-participants");
    writeCryptoMaterial(fixture.path());

    ParticipantProcess participant_a(fixture.path(), std::string{kShardA}, "normal");
    ParticipantProcess participant_b(fixture.path(), std::string{kShardB}, "normal");
    ParticipantProcess participant_c(fixture.path(), std::string{kShardC}, "crash_on_commit");

    const std::string transaction_id = "dis10-three-participant-crash";
    CoordinatorRunner coordinator(
        fixture.path(), transaction_id, "commit",
        participant_a.endpoint(), participant_b.endpoint(), participant_c.endpoint());
    int status = 0;
    ASSERT_TRUE(coordinator.waitForExit(20000, &status));
    ASSERT_TRUE(WIFEXITED(status));
    ASSERT_EQ(WEXITSTATUS(status), 2);

    const auto coordinator_wal = fixture.path() / "coordinator-wal";
    EXPECT_EQ(countWalEntries(coordinator_wal, transaction_id, WALEntryType::COMMIT_TX, "decision"), 1u);
    EXPECT_EQ(countWalEntries(coordinator_wal, transaction_id, WALEntryType::COMMIT_TX, "complete"), 0u);

    int participant_c_status = 0;
    ASSERT_TRUE(participant_c.waitForExit(5000, &participant_c_status));
    ASSERT_TRUE(WIFSIGNALED(participant_c_status));
    ASSERT_EQ(WTERMSIG(participant_c_status), SIGKILL);
    ParticipantProcess restarted_c(
        fixture.path(), std::string{kShardC}, "normal", participant_c.port());

    CoordinatorRunner recovery(
        fixture.path(), transaction_id, "recover",
        participant_a.endpoint(), participant_b.endpoint(), restarted_c.endpoint());
    int recovery_status = 0;
    ASSERT_TRUE(recovery.waitForExit(20000, &recovery_status));
    ASSERT_TRUE(WIFEXITED(recovery_status));
    ASSERT_EQ(WEXITSTATUS(recovery_status), 0);

    expectCommittedHistoryConsistent(
        coordinator_wal, transaction_id,
        {std::string{kShardA}, std::string{kShardB}, std::string{kShardC}});
}

TEST(DIS10RealTransportProcessRecovery, RecoversFourParticipantCommitAfterParticipantCrash) {
    const FixtureDirectory fixture("dis10-real-transport-four-participants");
    writeCryptoMaterial(fixture.path());

    ParticipantProcess participant_a(fixture.path(), std::string{kShardA}, "normal");
    ParticipantProcess participant_b(fixture.path(), std::string{kShardB}, "normal");
    ParticipantProcess participant_c(fixture.path(), std::string{kShardC}, "normal");
    ParticipantProcess participant_d(fixture.path(), std::string{kShardD}, "crash_on_commit");

    const std::string transaction_id = "dis10-four-participant-crash";
    CoordinatorRunner coordinator(
        fixture.path(), transaction_id, "commit",
        participant_a.endpoint(), participant_b.endpoint(),
        participant_c.endpoint(), participant_d.endpoint());
    int status = 0;
    ASSERT_TRUE(coordinator.waitForExit(20000, &status));
    ASSERT_TRUE(WIFEXITED(status));
    ASSERT_EQ(WEXITSTATUS(status), 2);

    const auto coordinator_wal = fixture.path() / "coordinator-wal";
    EXPECT_EQ(countWalEntries(coordinator_wal, transaction_id, WALEntryType::COMMIT_TX, "decision"), 1u);
    EXPECT_EQ(countWalEntries(coordinator_wal, transaction_id, WALEntryType::COMMIT_TX, "complete"), 0u);

    int participant_d_status = 0;
    ASSERT_TRUE(participant_d.waitForExit(5000, &participant_d_status));
    ASSERT_TRUE(WIFSIGNALED(participant_d_status));
    ASSERT_EQ(WTERMSIG(participant_d_status), SIGKILL);
    ParticipantProcess restarted_d(
        fixture.path(), std::string{kShardD}, "normal", participant_d.port());

    CoordinatorRunner recovery(
        fixture.path(), transaction_id, "recover",
        participant_a.endpoint(), participant_b.endpoint(),
        participant_c.endpoint(), restarted_d.endpoint());
    int recovery_status = 0;
    ASSERT_TRUE(recovery.waitForExit(20000, &recovery_status));
    ASSERT_TRUE(WIFEXITED(recovery_status));
    ASSERT_EQ(WEXITSTATUS(recovery_status), 0);

    expectCommittedHistoryConsistent(
        coordinator_wal, transaction_id,
        {std::string{kShardA}, std::string{kShardB},
         std::string{kShardC}, std::string{kShardD}});
}

} // namespace

#else

TEST(DIS10RealTransportProcessRecovery, LinuxOnly) {
    GTEST_SKIP() << "DIS-10 real-transport process recovery test requires Linux";
}

#endif

int main(int argc, char** argv) {
#if defined(__linux__)
    if (argc == 8 && std::string_view(argv[1]) == kParticipantMode) {
        try {
            return runParticipantProcess(
                argv[2], argv[3], argv[4], argv[5], static_cast<pid_t>(std::stol(argv[6])), std::stoi(argv[7]));
        } catch (const std::exception& error) {
            std::cerr << "participant process: " << error.what() << '\n';
            return 104;
        }
    }

    if (argc == 10 && std::string_view(argv[1]) == kCoordinatorMode) {
        try {
            return runCoordinatorProcess(
                argv[2], argv[3], argv[4], argv[5], argv[6], argv[7], argv[8],
                static_cast<pid_t>(std::stol(argv[9])));
        } catch (const std::exception& error) {
            std::cerr << "coordinator process: " << error.what() << '\n';
            return 204;
        }
    }
#endif

    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
