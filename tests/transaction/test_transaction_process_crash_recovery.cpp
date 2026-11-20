#include "transaction/distributed_transaction_manager.h"
#include "sharding/epoch_fencing.h"
#include "sharding/two_phase_commit_participant.h"
#include "sharding/two_phase_commit_coordinator.h"
#include "transaction/lock_manager.h"

#include <gtest/gtest.h>
#include <rocksdb/db.h>
#include <rocksdb/write_batch.h>

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <memory>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#if !defined(_WIN32)
#include <csignal>
#include <fcntl.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

using namespace themis::transaction;

namespace {

std::string g_executable_path;

void appendParticipantRecord(
    const std::string& wal_directory,
    const std::string& transaction_id,
    themis::sharding::WALEntryType type
) {
    themis::sharding::WALManagerConfig config;
    config.wal_directory = wal_directory;
    themis::sharding::WALManager wal(config);

    themis::sharding::WALEntry entry;
    entry.type = type;
    entry.transaction_id = transaction_id;
    entry.data = nlohmann::json::object();
    wal.append(entry);
    wal.flush();
}

std::vector<themis::sharding::WALEntry> readWal(const std::string& wal_directory) {
    themis::sharding::WALManagerConfig config;
    config.wal_directory = wal_directory;
    themis::sharding::WALManager wal(config);
    return wal.readRange(wal.getOldestLSN());
}

std::string makePreparePayload(
    const std::shared_ptr<themis::sharding::EpochFencingManager>& fencing_manager
) {
    const auto token = fencing_manager->makeToken();
    const auto issued_at_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        token.issued_at.time_since_epoch()).count();
    nlohmann::json payload;
    payload["operations"] = nlohmann::json::array({
        {{"type", "put"}, {"key", "participant-key"}, {"value", "durable"}},
    });
    payload["fencing_token"] = {
        {"epoch", token.epoch},
        {"issuer", token.issuer},
        {"shard_id", token.shard_id},
        {"issued_at_ms", issued_at_ms},
    };
    return payload.dump();
}

std::string makeTempDirectory(const std::string& scenario) {
    const auto path = std::filesystem::current_path() /
        ("themis_process_crash_" + scenario + "_" +
         std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(path);
    return path.string();
}

#if !defined(_WIN32)
void notifyAndWait(int descriptor, char marker) {
    if (::write(descriptor, &marker, sizeof(marker)) != sizeof(marker)) {
        return;
    }
    for (;;) {
        ::pause();
    }
}

int runCoordinatorChild(
    const std::string& stage,
    const std::string& coordinator_wal,
    const std::string& participant_wal,
    int control_fd
) {
    DistributedTxnManagerConfig config;
    config.wal_directory = coordinator_wal;
    config.prepare_timeout = std::chrono::minutes(2);
    config.commit_timeout = std::chrono::minutes(2);
    config.remote_phase1_dispatch =
        [&stage, participant_wal, control_fd](
            const std::string& transaction_id,
            const std::string&,
            const std::string&,
            const std::set<std::string>&) {
            appendParticipantRecord(
                participant_wal, transaction_id,
                themis::sharding::WALEntryType::PREPARE_TX);
            if (stage == "prepare_before_ack") {
                notifyAndWait(control_fd, 'V');
            }
            return true;
        };
    config.remote_phase2_dispatch =
        [&stage, participant_wal, control_fd](
            const std::string& transaction_id,
            const std::string&,
            const std::string&,
            bool do_commit) {
            if (stage == "after_decision") {
                notifyAndWait(control_fd, 'D');
            }
            if (stage == "during_commit" || stage == "after_ack" ||
                stage == "normal") {
                appendParticipantRecord(
                    participant_wal, transaction_id,
                    do_commit ? themis::sharding::WALEntryType::COMMIT_TX
                              : themis::sharding::WALEntryType::ABORT_TX);
            }
            if (stage == "during_commit") {
                notifyAndWait(control_fd, 'C');
            }
            return true;
        };

    DistributedTransactionManager coordinator("process-crash-coordinator", config);
    Participant participant;
    participant.node_id = "participant-1";
    participant.endpoint = "process://participant-1";
    participant.affected_keys = {"record-1"};
    const auto transaction_id = coordinator.beginDistributed({participant});

    if (stage == "before_prepare") {
        notifyAndWait(control_fd, 'B');
    }

    const auto prepared = coordinator.prepareDistributed(transaction_id);
    if (!prepared.ok) {
        return 2;
    }
    if (stage == "before_prepare") {
        return 3;
    }
    if (stage == "after_prepare") {
        notifyAndWait(control_fd, 'P');
    }

    const auto committed = coordinator.commitDistributed(transaction_id);
    if (!committed.ok) {
        return 4;
    }
    if (stage == "after_ack") {
        notifyAndWait(control_fd, 'A');
    }
    return 0;
}

int runParticipantChild(
    const std::string& stage,
    const std::string& participant_wal,
    const std::string& transaction_id,
    int control_fd
) {
    themis::sharding::TwoPhaseCommitParticipant::Config config;
    config.wal_directory = participant_wal;
    themis::sharding::EpochFencingConfig fencing_config;
    fencing_config.shard_id = "participant-process";
    fencing_config.node_id = "coordinator-process";
    config.fencing_manager =
        std::make_shared<themis::sharding::EpochFencingManager>(fencing_config);
    themis::sharding::TwoPhaseCommitParticipant participant(
        "participant-process", config,
        [](const std::string&, const nlohmann::json&) { return true; },
        [&stage, control_fd](const std::string&, const nlohmann::json&, int64_t) {
            if (stage == "during_commit") {
                notifyAndWait(control_fd, 'A');
            }
            return true;
        });
    const auto payload = makePreparePayload(config.fencing_manager);

    if (stage == "prepare") {
        if (!participant.onPrepare(transaction_id, "coordinator-process", payload)) {
            return 10;
        }
        notifyAndWait(control_fd, 'P');
        return 0;
    }

    if (stage == "before_prepare") {
        notifyAndWait(control_fd, 'B');
        return 0;
    }

    if (stage == "after_prepare") {
        if (!participant.onPrepare(transaction_id, "coordinator-process", payload)) {
            return 13;
        }
        notifyAndWait(control_fd, 'P');
        return 0;
    }

    if (stage == "during_commit" || stage == "after_ack") {
        if (!participant.onPrepare(transaction_id, "coordinator-process", payload)) {
            return 14;
        }
        if (!participant.onCommit(transaction_id)) {
            return 15;
        }
        if (stage == "after_ack") {
            notifyAndWait(control_fd, 'C');
        }
        return 0;
    }

    if (stage == "recover_commit") {
        if (participant.recoverFromWAL() != 1 ||
            participant.getTransactionState(transaction_id) !=
                themis::sharding::ParticipantTxnState::PREPARED ||
            !participant.onCommit(transaction_id) ||
            participant.getTransactionState(transaction_id) !=
                themis::sharding::ParticipantTxnState::COMMITTED) {
            return 11;
        }
        notifyAndWait(control_fd, 'C');
        return 0;
    }

    if (stage == "verify_committed") {
        participant.recoverFromWAL();
        if (participant.getTransactionState(transaction_id) !=
            themis::sharding::ParticipantTxnState::COMMITTED) {
            return 16;
        }
        notifyAndWait(control_fd, 'V');
        return 0;
    }

    return 12;
}

int runReplicaChild(
    const std::string& stage,
    const std::string& replica_wal,
    const std::string& transaction_id,
    int control_fd
) {
    if (stage == "verify_commit" || stage == "verify_prepare" ||
        stage == "verify_empty") {
        const auto entries = readWal(replica_wal);
        const auto expected_type = stage == "verify_commit"
            ? themis::sharding::WALEntryType::COMMIT_TX
            : themis::sharding::WALEntryType::PREPARE_TX;
        const auto has_expected_entry = std::any_of(
            entries.begin(), entries.end(),
            [&transaction_id, expected_type](const auto& entry) {
                return entry.transaction_id == transaction_id &&
                       entry.type == expected_type;
            });
        const bool valid = stage == "verify_empty"
            ? std::none_of(entries.begin(), entries.end(),
                [&transaction_id](const auto& entry) {
                    return entry.transaction_id == transaction_id;
                })
            : has_expected_entry;
        if (!valid) {
            return 17;
        }
        notifyAndWait(control_fd, 'V');
        return 0;
    }

    if (stage == "before_prepare") {
        notifyAndWait(control_fd, 'B');
        return 0;
    }

    const auto type = stage == "after_prepare"
        ? themis::sharding::WALEntryType::PREPARE_TX
        : themis::sharding::WALEntryType::COMMIT_TX;
    appendParticipantRecord(replica_wal, transaction_id, type);
    const char marker = stage == "after_prepare" ? 'P'
        : stage == "after_decision" ? 'D'
        : stage == "during_commit" ? 'C' : 'A';
    notifyAndWait(control_fd, marker);
    return 0;
}

class ChildProcessGuard {
public:
    ChildProcessGuard(pid_t pid, int read_fd) : pid_(pid), read_fd_(read_fd) {}
    ChildProcessGuard(const ChildProcessGuard&) = delete;
    ChildProcessGuard& operator=(const ChildProcessGuard&) = delete;
    ChildProcessGuard(ChildProcessGuard&& other) noexcept
        : pid_(std::exchange(other.pid_, -1)),
          read_fd_(std::exchange(other.read_fd_, -1)) {}

    ~ChildProcessGuard() {
        terminateAndReap();
        if (read_fd_ >= 0) {
            ::close(read_fd_);
        }
    }

    bool waitForMarker(char& marker) const {
        pollfd descriptor{read_fd_, POLLIN, 0};
        const int result = ::poll(&descriptor, 1, 15000);
        return result > 0 && (descriptor.revents & POLLIN) != 0 &&
               ::read(read_fd_, &marker, sizeof(marker)) == sizeof(marker);
    }

    void terminateAndReap() {
        if (pid_ <= 0) {
            return;
        }
        ::kill(pid_, SIGKILL);
        int status = 0;
        while (::waitpid(pid_, &status, 0) < 0 && errno == EINTR) {
        }
        pid_ = -1;
    }

private:
    pid_t pid_ = -1;
    int read_fd_ = -1;
};

ChildProcessGuard startCoordinatorChild(
    const std::string& stage,
    const std::string& coordinator_wal,
    const std::string& participant_wal
) {
    int pipe_fds[2];
    if (::pipe(pipe_fds) != 0) {
        throw std::runtime_error("could not create process control pipe");
    }
    const int flags = ::fcntl(pipe_fds[1], F_GETFD);
    if (flags < 0 || ::fcntl(pipe_fds[1], F_SETFD, flags & ~FD_CLOEXEC) < 0) {
        ::close(pipe_fds[0]);
        ::close(pipe_fds[1]);
        throw std::runtime_error("could not preserve process control pipe");
    }

    const pid_t pid = ::fork();
    if (pid < 0) {
        ::close(pipe_fds[0]);
        ::close(pipe_fds[1]);
        throw std::runtime_error("could not fork crash-test child");
    }
    if (pid == 0) {
        ::close(pipe_fds[0]);
        const auto fd = std::to_string(pipe_fds[1]);
        char* const arguments[] = {
            const_cast<char*>(g_executable_path.c_str()),
            const_cast<char*>("--dtm-crash-child"),
            const_cast<char*>(stage.c_str()),
            const_cast<char*>(coordinator_wal.c_str()),
            const_cast<char*>(participant_wal.c_str()),
            const_cast<char*>(fd.c_str()),
            nullptr
        };
        ::execv(g_executable_path.c_str(), arguments);
        ::_exit(127);
    }

    ::close(pipe_fds[1]);
    return ChildProcessGuard(pid, pipe_fds[0]);
}

ChildProcessGuard startParticipantChild(
    const std::string& stage,
    const std::string& participant_wal,
    const std::string& transaction_id = "participant-process-recovery"
) {
    int pipe_fds[2];
    if (::pipe(pipe_fds) != 0) {
        throw std::runtime_error("could not create participant control pipe");
    }
    const int flags = ::fcntl(pipe_fds[1], F_GETFD);
    if (flags < 0 || ::fcntl(pipe_fds[1], F_SETFD, flags & ~FD_CLOEXEC) < 0) {
        ::close(pipe_fds[0]);
        ::close(pipe_fds[1]);
        throw std::runtime_error("could not preserve participant control pipe");
    }

    const pid_t pid = ::fork();
    if (pid < 0) {
        ::close(pipe_fds[0]);
        ::close(pipe_fds[1]);
        throw std::runtime_error("could not fork participant child");
    }
    if (pid == 0) {
        ::close(pipe_fds[0]);
        const auto fd = std::to_string(pipe_fds[1]);
        char* const arguments[] = {
            const_cast<char*>(g_executable_path.c_str()),
            const_cast<char*>("--participant-crash-child"),
            const_cast<char*>(stage.c_str()),
            const_cast<char*>(participant_wal.c_str()),
            const_cast<char*>(transaction_id.c_str()),
            const_cast<char*>(fd.c_str()),
            nullptr
        };
        ::execv(g_executable_path.c_str(), arguments);
        ::_exit(127);
    }

    ::close(pipe_fds[1]);
    return ChildProcessGuard(pid, pipe_fds[0]);
}

ChildProcessGuard startReplicaChild(
    const std::string& stage,
    const std::string& replica_wal,
    const std::string& transaction_id
) {
    int pipe_fds[2];
    if (::pipe(pipe_fds) != 0) {
        throw std::runtime_error("could not create replica control pipe");
    }
    const int flags = ::fcntl(pipe_fds[1], F_GETFD);
    if (flags < 0 || ::fcntl(pipe_fds[1], F_SETFD, flags & ~FD_CLOEXEC) < 0) {
        ::close(pipe_fds[0]);
        ::close(pipe_fds[1]);
        throw std::runtime_error("could not preserve replica control pipe");
    }

    const pid_t pid = ::fork();
    if (pid < 0) {
        ::close(pipe_fds[0]);
        ::close(pipe_fds[1]);
        throw std::runtime_error("could not fork replica child");
    }
    if (pid == 0) {
        ::close(pipe_fds[0]);
        const auto fd = std::to_string(pipe_fds[1]);
        char* const arguments[] = {
            const_cast<char*>(g_executable_path.c_str()),
            const_cast<char*>("--replica-crash-child"),
            const_cast<char*>(stage.c_str()),
            const_cast<char*>(replica_wal.c_str()),
            const_cast<char*>(transaction_id.c_str()),
            const_cast<char*>(fd.c_str()),
            nullptr
        };
        ::execv(g_executable_path.c_str(), arguments);
        ::_exit(127);
    }

    ::close(pipe_fds[1]);
    return ChildProcessGuard(pid, pipe_fds[0]);
}

// DIS-06 test-only transport: these pipes cross exec'd process boundaries but
// do not exercise ShardRPC gRPC/mTLS, network partitions, Raft, or replication.
// Only the RequestHandler boundary is proxied; production coordinator,
// participant, WAL, LockManager and RocksDB execute without transport substitutes
// in production code. The older DTM/replica cases above remain WAL-only evidence.
class ProcessPipe {
public:
    ProcessPipe() {
        if (::pipe(fds_) != 0) {
            throw std::runtime_error("could not create canonical 2PC pipe");
        }
        for (const int fd : fds_) {
            if (::fcntl(fd, F_SETFD, FD_CLOEXEC) < 0) {
                closeRead();
                closeWrite();
                throw std::runtime_error("could not protect canonical 2PC pipe");
            }
        }
    }
    ~ProcessPipe() { closeRead(); closeWrite(); }
    ProcessPipe(const ProcessPipe&) = delete;
    ProcessPipe& operator=(const ProcessPipe&) = delete;
    int reader() const { return fds_[0]; }
    int writer() const { return fds_[1]; }
    void closeRead() { closeDescriptor(fds_[0]); }
    void closeWrite() { closeDescriptor(fds_[1]); }

private:
    static void closeDescriptor(int& fd) {
        if (fd >= 0) {
            ::close(fd);
            fd = -1;
        }
    }
    int fds_[2]{-1, -1};
};

void writePacket(int fd, const nlohmann::json& packet) {
    const auto bytes = packet.dump() + "\n";
    size_t written = 0;
    while (written < bytes.size()) {
        const auto count = ::write(fd, bytes.data() + written, bytes.size() - written);
        if (count < 0 && errno == EINTR) {
            continue;
        }
        if (count <= 0) {
            throw std::runtime_error("canonical 2PC pipe write failed");
        }
        written += static_cast<size_t>(count);
    }
}

nlohmann::json readPacket(int fd) {
    std::string bytes;
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
    while (bytes.size() < 1024 * 1024) {
        const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
            deadline - std::chrono::steady_clock::now()).count();
        if (remaining <= 0) {
            throw std::runtime_error("canonical 2PC pipe response timed out");
        }
        pollfd descriptor{fd, POLLIN, 0};
        const auto ready = ::poll(&descriptor, 1, static_cast<int>(remaining));
        if (ready < 0 && errno == EINTR) {
            continue;
        }
        char byte = '\0';
        if (ready <= 0 || ::read(fd, &byte, 1) != 1) {
            throw std::runtime_error("canonical 2PC pipe closed or timed out");
        }
        if (byte == '\n') {
            return nlohmann::json::parse(bytes);
        }
        bytes.push_back(byte);
    }
    throw std::runtime_error("canonical 2PC pipe packet exceeds limit");
}

ChildProcessGuard startCanonicalChild(
    std::vector<std::string> arguments, const std::vector<int>& inherited_fds
) {
    ProcessPipe control;
    arguments.insert(arguments.begin(), g_executable_path);
    arguments.push_back(std::to_string(control.writer()));
    std::vector<char*> argv;
    for (auto& argument : arguments) {
        argv.push_back(argument.data());
    }
    argv.push_back(nullptr);
    const auto pid = ::fork();
    if (pid < 0) {
        throw std::runtime_error("could not fork canonical 2PC child");
    }
    if (pid == 0) {
        for (const int fd : inherited_fds) {
            if (::fcntl(fd, F_SETFD, 0) < 0) {
                ::_exit(126);
            }
        }
        if (::fcntl(control.writer(), F_SETFD, 0) < 0) {
            ::_exit(126);
        }
        ::execv(g_executable_path.c_str(), argv.data());
        ::_exit(127);
    }
    // Transfer the read end to the existing process-reaping guard.
    const int read_fd = ::dup(control.reader());
    if (read_fd < 0) {
        ChildProcessGuard cleanup(pid, -1);
        throw std::runtime_error("could not retain canonical child control pipe");
    }
    ::fcntl(read_fd, F_SETFD, FD_CLOEXEC);
    return ChildProcessGuard(pid, read_fd);
}

class PipeRequestHandler final : public themis::sharding::ShardRPCServer::RequestHandler {
public:
    PipeRequestHandler(int request_fd, int response_fd,
                       std::string stage = {}, int control_fd = -1)
        : request_fd_(request_fd), response_fd_(response_fd),
          stage_(std::move(stage)), control_fd_(control_fd) {}

    nlohmann::json request(const nlohmann::json& message) {
        writePacket(request_fd_, message);
        return readPacket(response_fd_);
    }
    bool onPrepare(const std::string& id, const std::string& coordinator,
                   const std::string& payload) override {
        if (stage_ == "before_prepare") {
            notifyAndWait(control_fd_, 'B');
        }
        return request({{"method", "prepare"}, {"id", id},
                        {"coordinator", coordinator}, {"payload", payload}}).at("ok");
    }
    bool onCommit(const std::string& id) override { return phaseTwo("commit", id); }
    bool onAbort(const std::string& id) override { return phaseTwo("abort", id); }
    themis::sharding::ShardRPCServer::HealthInfo onHealthCheck() override { return {}; }

private:
    bool phaseTwo(const std::string& method, const std::string& id) {
        if (stage_ == "crash_decision") {
            notifyAndWait(control_fd_, 'D');
        }
        const bool acknowledged = request({{"method", method}, {"id", id}}).at("ok");
        // The participant really receives and durably applies the decision;
        // this fault drops its reply, never fabricates a successful ACK.
        return acknowledged && stage_ != "drop_ack";
    }
    int request_fd_;
    int response_fd_;
    std::string stage_;
    int control_fd_;
};

class ParticipantProcess {
public:
    ParticipantProcess(const std::string& root, const std::string& shard)
        : process_(startCanonicalChild(
              {"--canonical-participant", root, shard,
               std::to_string(requests_.reader()), std::to_string(replies_.writer())},
              {requests_.reader(), replies_.writer()})),
          handler_(requests_.writer(), replies_.reader()) {
        requests_.closeRead();
        replies_.closeWrite();
        char marker = '\0';
        if (!process_.waitForMarker(marker) || marker != 'S') {
            throw std::runtime_error("canonical participant did not become ready");
        }
    }
    PipeRequestHandler& handler() { return handler_; }
    nlohmann::json descriptor(const std::string& shard) const {
        return {{"shard", shard}, {"request", requests_.writer()},
                {"reply", replies_.reader()}};
    }
    void crash() { process_.terminateAndReap(); }

private:
    ProcessPipe requests_;
    ProcessPipe replies_;
    ChildProcessGuard process_;
    PipeRequestHandler handler_;
};

int runCanonicalParticipant(const std::string& root, const std::string& shard,
                            int request_fd, int reply_fd, int control_fd) {
    using namespace themis::sharding;
    rocksdb::Options options;
    options.create_if_missing = true;
    rocksdb::DB* raw_db = nullptr;
    const auto opened = rocksdb::DB::Open(options, root + "/" + shard + "-db", &raw_db);
    std::unique_ptr<rocksdb::DB> db(raw_db);
    if (!opened.ok()) {
        return 30;
    }
    themis::LockManager locks;
    constexpr themis::LockManager::TransactionId owner = 1;
    const std::string key = "process-key";
    TwoPhaseCommitParticipant::Config config;
    config.wal_directory = root + "/" + shard + "-wal";
    // Zero deterministically expires PREPARE without sleep-based timing.
    config.prepare_timeout = std::chrono::milliseconds(0);
    EpochFencingConfig fencing;
    fencing.shard_id = shard;
    fencing.node_id = "canonical-coordinator";
    fencing.auto_stonith = false;
    config.fencing_manager = std::make_shared<EpochFencingManager>(fencing);
    size_t apply_calls = 0;
    TwoPhaseCommitParticipant participant(
        shard, config,
        [&](const std::string&, const nlohmann::json& ops) {
            if (ops.empty() || ops.at(0).value("reject", false)) {
                return false;
            }
            return locks.acquireLock(owner, key, themis::LockType::EXCLUSIVE,
                                     std::chrono::milliseconds(0)).status ==
                   themis::LockStatus::GRANTED;
        },
        [&](const std::string& id, const nlohmann::json& ops, int64_t) {
            // Test wiring to a real durable store, not the shard server's full
            // TransactionManager/index/MVCC integration.
            ++apply_calls;
            std::string existing;
            const auto prior = db->Get(rocksdb::ReadOptions{}, "applied/" + id, &existing);
            if (prior.ok()) {
                return true;
            }
            if (!prior.IsNotFound()) {
                return false;
            }
            rocksdb::WriteBatch batch;
            batch.Put(key, ops.at(0).at("value").get<std::string>());
            batch.Put("applied/" + id, "1");
            rocksdb::WriteOptions write_options;
            write_options.sync = true;
            return db->Write(write_options, &batch).ok();
        },
        [&](const std::string&) { locks.releaseAllLocks(owner); });
    const char ready = 'S';
    if (::write(control_fd, &ready, 1) != 1) {
        return 31;
    }
    for (;;) {
        const auto message = readPacket(request_fd);
        const auto method = message.at("method").get<std::string>();
        const auto id = message.at("id").get<std::string>();
        nlohmann::json response;
        if (method == "prepare") {
            response["ok"] = participant.onPrepare(
                id, message.at("coordinator"), message.at("payload"));
        } else if (method == "commit") {
            response["ok"] = participant.onCommit(id);
        } else if (method == "abort") {
            response["ok"] = participant.onAbort(id);
        } else if (method == "inspect") {
            response["timeouts_aborted"] = participant.abortTimedOutTransactions();
            response["ready"] = participant.onHealthCheck().is_healthy;
            const auto state = participant.getTransactionState(id);
            response["state"] = state ? static_cast<int>(*state) : -1;
            response["locked"] = locks.holdsLock(owner, key, themis::LockType::EXCLUSIVE);
            const auto probe = locks.acquireLock(
                2, key, themis::LockType::EXCLUSIVE, std::chrono::milliseconds(0));
            response["conflict_blocked"] = probe.status != themis::LockStatus::GRANTED;
            locks.releaseAllLocks(2);
            std::string value;
            const auto found = db->Get(rocksdb::ReadOptions{}, key, &value);
            response["value"] = found.ok() ? value : "";
            response["applied"] = db->Get(
                rocksdb::ReadOptions{}, "applied/" + id, &value).ok();
            response["apply_calls"] = apply_calls;
        } else {
            return 32;
        }
        writePacket(reply_fd, response);
    }
}

int runCanonicalCoordinator(const std::string& stage, const std::string& root,
                            bool do_commit, const nlohmann::json& endpoints,
                            int control_fd) {
    using namespace themis::sharding;
    TwoPhaseCommitCoordinator::Config config;
    config.wal_directory = root + "/canonical-coordinator-wal";
    config.fencing_token_provider = [](const std::string& shard) -> std::optional<EpochToken> {
        EpochToken token;
        token.epoch = 1;
        token.issuer = "canonical-coordinator";
        token.shard_id = shard;
        token.issued_at = std::chrono::system_clock::now();
        return token;
    };
    TwoPhaseCommitCoordinator coordinator("canonical-coordinator", config);
    std::vector<std::unique_ptr<PipeRequestHandler>> proxies;
    for (const auto& endpoint : endpoints) {
        if (stage == "missing_participant" && endpoint.at("shard") == "b") {
            continue;
        }
        proxies.push_back(std::make_unique<PipeRequestHandler>(
            endpoint.at("request"), endpoint.at("reply"), stage, control_fd));
        coordinator.registerParticipant(endpoint.at("shard"), proxies.back().get());
    }
    const std::string id = do_commit ? "canonical-commit" : "canonical-abort";
    if (stage == "crash_decision" || stage == "before_prepare" ||
        stage == "after_prepare") {
        const auto ops = nlohmann::json::array({
            {{"type", "put"}, {"key", "process-key"}, {"value", "durable-value"}}
        });
        auto second_ops = ops;
        second_ops[0]["reject"] = !do_commit && stage == "crash_decision";
        if (stage == "after_prepare") {
            if (!coordinator.prepare(id, {{"a", ops}, {"b", second_ops}})) {
                return 43;
            }
            notifyAndWait(control_fd, 'P');
        }
        (void)coordinator.commit(id, {{"a", ops}, {"b", second_ops}});
        return 40; // The selected crash barrier must stop the process.
    }
    if (coordinator.isReady()) {
        return 44; // Startup WAL must block readiness before any recovery RPC.
    }
    const auto report = GlobalTwoPhaseCommitRecoveryManager::recoverAll({&coordinator});
    const bool expect_ack = stage == "recover_ack";
    if (report.resolved != (expect_ack ? 1u : 0u) ||
        report.in_doubt_after != (expect_ack ? 0u : 1u) ||
        report.ready_for_service != expect_ack ||
        coordinator.isReady() != expect_ack ||
        coordinator.getRecoverableTransactions().empty() != expect_ack) {
        return 41;
    }
    notifyAndWait(control_fd, expect_ack ? 'R' : 'U');
    return 0;
}

ChildProcessGuard startCanonicalCoordinator(
    const std::string& stage, const std::string& root, bool do_commit,
    const ParticipantProcess& first, const ParticipantProcess& second
) {
    const auto endpoints = nlohmann::json::array({
        first.descriptor("a"), second.descriptor("b")});
    std::vector<int> fds;
    for (const auto& endpoint : endpoints) {
        fds.push_back(endpoint.at("request"));
        fds.push_back(endpoint.at("reply"));
    }
    return startCanonicalChild(
        {"--canonical-coordinator", stage, root, do_commit ? "commit" : "abort",
         endpoints.dump()}, fds);
}

class CrashDirectory {
public:
    explicit CrashDirectory(const std::string& scenario) : path(makeTempDirectory(scenario)) {}
    ~CrashDirectory() {
        std::error_code error;
        std::filesystem::remove_all(path, error);
    }
    const std::string path;
};
#endif

} // namespace

TEST(TransactionProcessCrashRecovery, CanonicalDecisionsSurviveProcessCrashesAndLostAcknowledgements) {
#if defined(_WIN32)
    GTEST_SKIP() << "Canonical participant/coordinator IPC crash coverage requires POSIX";
#else
    using namespace themis::sharding;
    for (const bool do_commit : {true, false}) {
        SCOPED_TRACE(do_commit ? "COMMIT" : "ABORT");
        CrashDirectory directory(do_commit ? "canonical_commit" : "canonical_abort");
        const auto& root = directory.path;
        const std::string id = do_commit ? "canonical-commit" : "canonical-abort";
        const auto coordinator_wal = root + "/canonical-coordinator-wal";
        auto first = std::make_unique<ParticipantProcess>(root, "a");
        auto second = std::make_unique<ParticipantProcess>(root, "b");
        auto coordinator = startCanonicalCoordinator(
            "crash_decision", root, do_commit, *first, *second);
        char marker = '\0';
        ASSERT_TRUE(coordinator.waitForMarker(marker));
        ASSERT_EQ(marker, 'D');
        coordinator.terminateAndReap();

        const auto assert_decision = [&](bool completed) {
            const auto entries = readWal(coordinator_wal);
            size_t commits = 0, aborts = 0, completions = 0;
            for (const auto& entry : entries) {
                if (entry.transaction_id != id) {
                    continue;
                }
                const auto phase = entry.data.value("phase", std::string{});
                commits += entry.type == WALEntryType::COMMIT_TX && phase == "decision";
                aborts += entry.type == WALEntryType::ABORT_TX && phase == "decision";
                completions += phase == "complete";
            }
            EXPECT_EQ(commits, do_commit ? 1u : 0u);
            EXPECT_EQ(aborts, do_commit ? 0u : 1u);
            EXPECT_EQ(completions, completed ? 1u : 0u);
        };
        assert_decision(false);
        const auto assert_prepared = [&] {
            const auto inspection = first->handler().request(
                {{"method", "inspect"}, {"id", id}});
            EXPECT_EQ(inspection.at("state"), static_cast<int>(ParticipantTxnState::PREPARED));
            EXPECT_TRUE(inspection.at("locked").get<bool>());
            EXPECT_TRUE(inspection.at("conflict_blocked").get<bool>());
            EXPECT_FALSE(inspection.at("ready").get<bool>());
            EXPECT_EQ(inspection.at("timeouts_aborted"), 0);
            EXPECT_FALSE(inspection.at("applied").get<bool>());
        };
        assert_prepared();
        // A genuine SIGKILL/re-exec restores PREPARE and its real lock callback.
        first->crash();
        second->crash();
        const auto prepared_entries = readWal(root + "/a-wal");
        EXPECT_EQ(std::count_if(prepared_entries.begin(), prepared_entries.end(),
            [&](const auto& entry) {
                return entry.transaction_id == id && entry.type == WALEntryType::PREPARE_TX;
            }), 1);
        EXPECT_TRUE(std::none_of(prepared_entries.begin(), prepared_entries.end(),
            [&](const auto& entry) {
                return entry.transaction_id == id &&
                    (entry.type == WALEntryType::COMMIT_TX ||
                     entry.type == WALEntryType::ABORT_TX);
            }));
        first = std::make_unique<ParticipantProcess>(root, "a");
        second = std::make_unique<ParticipantProcess>(root, "b");
        assert_prepared();
        assert_decision(false); // Expiration did not replace the durable decision.

        auto unavailable = startCanonicalCoordinator(
            "missing_participant", root, do_commit, *first, *second);
        ASSERT_TRUE(unavailable.waitForMarker(marker));
        ASSERT_EQ(marker, 'U');
        unavailable.terminateAndReap();
        assert_decision(false);
        if (do_commit) {
            const auto still_prepared = second->handler().request(
                {{"method", "inspect"}, {"id", id}});
            EXPECT_EQ(still_prepared.at("state"),
                      static_cast<int>(ParticipantTxnState::PREPARED));
            EXPECT_TRUE(still_prepared.at("locked").get<bool>());
            EXPECT_TRUE(still_prepared.at("conflict_blocked").get<bool>());
            EXPECT_FALSE(still_prepared.at("ready").get<bool>());
            EXPECT_EQ(still_prepared.at("timeouts_aborted"), 0);
        }
        auto lost_ack = startCanonicalCoordinator(
            "drop_ack", root, do_commit, *first, *second);
        ASSERT_TRUE(lost_ack.waitForMarker(marker));
        ASSERT_EQ(marker, 'U');
        lost_ack.terminateAndReap();
        assert_decision(false); // Real participant apply alone is not completion.

        const auto terminal = do_commit ? ParticipantTxnState::COMMITTED
                                        : ParticipantTxnState::ABORTED;
        for (auto* participant : {first.get(), second.get()}) {
            auto state = participant->handler().request({{"method", "inspect"}, {"id", id}});
            EXPECT_EQ(state.at("state"), static_cast<int>(terminal));
            EXPECT_FALSE(state.at("locked").get<bool>());
            EXPECT_FALSE(state.at("conflict_blocked").get<bool>());
            EXPECT_TRUE(state.at("ready").get<bool>());
            EXPECT_EQ(state.at("applied").get<bool>(), do_commit);
            EXPECT_EQ(state.at("value"), do_commit ? "durable-value" : "");
            EXPECT_EQ(state.at("apply_calls"), do_commit ? 1u : 0u);
            EXPECT_TRUE(do_commit ? participant->handler().onCommit(id)
                                  : participant->handler().onAbort(id));
            // An opposite terminal message must neither rewrite nor undo it.
            EXPECT_FALSE(do_commit ? participant->handler().onAbort(id)
                                   : participant->handler().onCommit(id));
            state = participant->handler().request({{"method", "inspect"}, {"id", id}});
            EXPECT_EQ(state.at("state"), static_cast<int>(terminal));
            EXPECT_EQ(state.at("apply_calls"), do_commit ? 1u : 0u);
        }

        // Restart again after apply but before a coordinator completion record.
        first->crash();
        second->crash();
        first = std::make_unique<ParticipantProcess>(root, "a");
        second = std::make_unique<ParticipantProcess>(root, "b");
        auto recovered = startCanonicalCoordinator(
            "recover_ack", root, do_commit, *first, *second);
        ASSERT_TRUE(recovered.waitForMarker(marker));
        ASSERT_EQ(marker, 'R');
        recovered.terminateAndReap();
        assert_decision(true);
        for (auto* participant : {first.get(), second.get()}) {
            const auto state = participant->handler().request(
                {{"method", "inspect"}, {"id", id}});
            EXPECT_EQ(state.at("state"), static_cast<int>(terminal));
            EXPECT_EQ(state.at("applied").get<bool>(), do_commit);
            EXPECT_EQ(state.at("apply_calls"), 0);
            EXPECT_FALSE(state.at("locked").get<bool>());
        }
    }
#endif
}

TEST(TransactionProcessCrashRecovery, CanonicalPredecisionCrashesRecoverAbortAfterParticipantRestart) {
#if defined(_WIN32)
    GTEST_SKIP() << "Canonical participant/coordinator IPC crash coverage requires POSIX";
#else
    using namespace themis::sharding;
    for (const std::string stage : {"before_prepare", "after_prepare"}) {
        SCOPED_TRACE(stage);
        CrashDirectory directory("canonical_" + stage);
        const auto& root = directory.path;
        const std::string id = "canonical-abort";
        auto first = std::make_unique<ParticipantProcess>(root, "a");
        auto second = std::make_unique<ParticipantProcess>(root, "b");
        auto coordinator = startCanonicalCoordinator(stage, root, false, *first, *second);
        char marker = '\0';
        ASSERT_TRUE(coordinator.waitForMarker(marker));
        ASSERT_EQ(marker, stage == "before_prepare" ? 'B' : 'P');
        coordinator.terminateAndReap();
        const bool prepared = stage == "after_prepare";
        const auto assert_in_doubt = [&] {
            for (auto* participant : {first.get(), second.get()}) {
                const auto state = participant->handler().request(
                    {{"method", "inspect"}, {"id", id}});
                EXPECT_EQ(state.at("state"),
                          prepared ? static_cast<int>(ParticipantTxnState::PREPARED) : -1);
                EXPECT_EQ(state.at("locked").get<bool>(), prepared);
                EXPECT_EQ(state.at("conflict_blocked").get<bool>(), prepared);
                EXPECT_EQ(state.at("ready").get<bool>(), !prepared);
                EXPECT_EQ(state.at("timeouts_aborted"), 0);
                EXPECT_FALSE(state.at("applied").get<bool>());
            }
        };
        assert_in_doubt();
        first->crash();
        second->crash();
        first = std::make_unique<ParticipantProcess>(root, "a");
        second = std::make_unique<ParticipantProcess>(root, "b");
        assert_in_doubt();
        const auto before_recovery = readWal(root + "/canonical-coordinator-wal");
        EXPECT_TRUE(std::none_of(before_recovery.begin(), before_recovery.end(),
            [&](const auto& entry) {
                return entry.transaction_id == id &&
                    (entry.type == WALEntryType::COMMIT_TX ||
                     entry.type == WALEntryType::ABORT_TX);
            }));

        auto recovered = startCanonicalCoordinator(
            "recover_ack", root, false, *first, *second);
        ASSERT_TRUE(recovered.waitForMarker(marker));
        ASSERT_EQ(marker, 'R');
        recovered.terminateAndReap();
        for (auto* participant : {first.get(), second.get()}) {
            const auto state = participant->handler().request(
                {{"method", "inspect"}, {"id", id}});
            EXPECT_EQ(state.at("state"), static_cast<int>(ParticipantTxnState::ABORTED));
            EXPECT_FALSE(state.at("locked").get<bool>());
            EXPECT_FALSE(state.at("conflict_blocked").get<bool>());
            EXPECT_TRUE(state.at("ready").get<bool>());
            EXPECT_FALSE(state.at("applied").get<bool>());
            EXPECT_EQ(state.at("apply_calls"), 0);
        }
        const auto recovered_entries = readWal(root + "/canonical-coordinator-wal");
        size_t abort_decisions = 0, completions = 0;
        for (const auto& entry : recovered_entries) {
            if (entry.transaction_id == id) {
                EXPECT_NE(entry.type, WALEntryType::COMMIT_TX);
                abort_decisions += entry.type == WALEntryType::ABORT_TX &&
                    entry.data.value("phase", std::string{}) == "decision";
                completions += entry.data.value("phase", std::string{}) == "complete";
            }
        }
        EXPECT_EQ(abort_decisions, 1u);
        EXPECT_EQ(completions, 1u);
    }
#endif
}

TEST(TransactionProcessCrashRecovery, RecoversDecisionBoundariesAcrossCoordinatorRestart) {
#if defined(_WIN32)
    GTEST_SKIP() << "The subprocess crash harness currently requires POSIX process controls";
#else
    struct Scenario {
        const char* stage;
        char marker;
        bool expected_commit;
        bool expected_recovery;
    };
    const std::vector<Scenario> scenarios = {
        {"before_prepare", 'B', false, true},
        {"prepare_before_ack", 'V', false, true},
        {"after_prepare", 'P', false, true},
        {"after_decision", 'D', true, true},
        {"during_commit", 'C', true, true},
        {"after_ack", 'A', true, false}
    };

    for (const auto& scenario : scenarios) {
        SCOPED_TRACE(scenario.stage);
        const auto root = makeTempDirectory(scenario.stage);
        const auto coordinator_wal = (std::filesystem::path(root) / "coordinator").string();
        const auto participant_wal = (std::filesystem::path(root) / "participant").string();
        std::filesystem::create_directories(coordinator_wal);
        std::filesystem::create_directories(participant_wal);

        auto child = startCoordinatorChild(
            scenario.stage, coordinator_wal, participant_wal);
        char marker = '\0';
        ASSERT_TRUE(child.waitForMarker(marker))
            << "coordinator child did not reach crash point " << scenario.stage;
        ASSERT_EQ(marker, scenario.marker);
        child.terminateAndReap();

        const auto coordinator_entries = readWal(coordinator_wal);
        ASSERT_FALSE(coordinator_entries.empty());
        const auto transaction_id = coordinator_entries.front().transaction_id;
        ASSERT_FALSE(transaction_id.empty());

        std::atomic<bool> replayed_expected_decision{false};
        DistributedTxnManagerConfig recovery_config;
        recovery_config.wal_directory = coordinator_wal;
        recovery_config.remote_phase2_dispatch =
            [&participant_wal, &replayed_expected_decision, &scenario](
                const std::string& txn_id,
                const std::string&,
                const std::string& endpoint,
                bool do_commit) {
                const bool expected = do_commit == scenario.expected_commit &&
                                      endpoint == "process://participant-1";
                appendParticipantRecord(
                    participant_wal, txn_id,
                    do_commit ? themis::sharding::WALEntryType::COMMIT_TX
                              : themis::sharding::WALEntryType::ABORT_TX);
                replayed_expected_decision = expected;
                return expected;
            };
        recovery_config.recovery_endpoint_resolver =
            [](const std::string& node_id) -> std::optional<std::string> {
                if (node_id == "participant-1") {
                    return "process://participant-1";
                }
                return std::nullopt;
            };

        DistributedTransactionManager restarted(
            "process-crash-coordinator", recovery_config);
        const auto report =
            GlobalTwoPhaseCommitRecoveryManager::recoverAll({&restarted});
        EXPECT_EQ(report.resolved, scenario.expected_recovery ? 1u : 0u);
        EXPECT_EQ(report.in_doubt_after, 0u);
        EXPECT_TRUE(report.ready_for_service);
        EXPECT_EQ(replayed_expected_decision.load(), scenario.expected_recovery);
        EXPECT_TRUE(restarted.getRecoverableTransactions().empty());

        const auto final_participant_entries = readWal(participant_wal);
        std::optional<themis::sharding::WALEntryType> final_decision;
        for (const auto& entry : final_participant_entries) {
            if (entry.transaction_id == transaction_id &&
                (entry.type == themis::sharding::WALEntryType::COMMIT_TX ||
                 entry.type == themis::sharding::WALEntryType::ABORT_TX)) {
                final_decision = entry.type;
            }
        }
        ASSERT_TRUE(final_decision.has_value());
        EXPECT_EQ(*final_decision,
                  scenario.expected_commit
                      ? themis::sharding::WALEntryType::COMMIT_TX
                      : themis::sharding::WALEntryType::ABORT_TX);
        std::filesystem::remove_all(root);
    }
#endif
}

TEST(TransactionProcessCrashRecovery, ParticipantProcessRestartRecoversPreparedTransaction) {
#if defined(_WIN32)
    GTEST_SKIP() << "The subprocess crash harness currently requires POSIX process controls";
#else
    const auto root = makeTempDirectory("participant_restart");
    const auto participant_wal = (std::filesystem::path(root) / "participant").string();
    std::filesystem::create_directories(participant_wal);

    auto crashed_participant = startParticipantChild("prepare", participant_wal);
    char marker = '\0';
    ASSERT_TRUE(crashed_participant.waitForMarker(marker));
    ASSERT_EQ(marker, 'P');
    crashed_participant.terminateAndReap();

    auto restarted_participant =
        startParticipantChild("recover_commit", participant_wal);
    ASSERT_TRUE(restarted_participant.waitForMarker(marker));
    EXPECT_EQ(marker, 'C');
    restarted_participant.terminateAndReap();

    const auto entries = readWal(participant_wal);
    bool saw_prepare = false;
    bool saw_commit = false;
    for (const auto& entry : entries) {
        if (entry.transaction_id != "participant-process-recovery") {
            continue;
        }
        saw_prepare = saw_prepare ||
            entry.type == themis::sharding::WALEntryType::PREPARE_TX;
        saw_commit = saw_commit ||
            entry.type == themis::sharding::WALEntryType::COMMIT_TX;
    }
    EXPECT_TRUE(saw_prepare);
    EXPECT_TRUE(saw_commit);
    std::filesystem::remove_all(root);
#endif
}

TEST(TransactionProcessCrashRecovery, ParticipantCrashMatrixRecoversDurableDecision) {
#if defined(_WIN32)
    GTEST_SKIP() << "The subprocess crash harness currently requires POSIX process controls";
#else
    struct Scenario {
        const char* stage;
        char marker;
        bool has_prepare;
    };
    const std::vector<Scenario> scenarios = {
        {"before_prepare", 'B', false},
        {"after_prepare", 'P', true},
        {"during_commit", 'A', true},
        {"after_ack", 'C', true}
    };

    for (const auto& scenario : scenarios) {
        SCOPED_TRACE(scenario.stage);
        const auto root = makeTempDirectory(scenario.stage);
        const auto participant_wal = (std::filesystem::path(root) / "participant").string();
        std::filesystem::create_directories(participant_wal);
        const auto transaction_id = std::string("participant-matrix-") + scenario.stage;

        auto crashed_participant =
            startParticipantChild(scenario.stage, participant_wal, transaction_id);
        char marker = '\0';
        ASSERT_TRUE(crashed_participant.waitForMarker(marker));
        ASSERT_EQ(marker, scenario.marker);
        crashed_participant.terminateAndReap();

        const auto entries_before_recovery = readWal(participant_wal);
        if (!scenario.has_prepare) {
            EXPECT_TRUE(entries_before_recovery.empty());
            std::filesystem::remove_all(root);
            continue;
        }

        const bool decision_already_durable = std::string(scenario.stage) == "after_ack";
        auto recovered_participant = startParticipantChild(
            decision_already_durable ? "verify_committed" : "recover_commit",
            participant_wal, transaction_id);
        ASSERT_TRUE(recovered_participant.waitForMarker(marker));
        EXPECT_EQ(marker, decision_already_durable ? 'V' : 'C');
        recovered_participant.terminateAndReap();

        const auto entries_after_recovery = readWal(participant_wal);
        EXPECT_TRUE(std::any_of(
            entries_after_recovery.begin(), entries_after_recovery.end(),
            [&transaction_id](const auto& entry) {
                return entry.transaction_id == transaction_id &&
                       entry.type == themis::sharding::WALEntryType::COMMIT_TX;
            }));
        std::filesystem::remove_all(root);
    }
#endif
}

TEST(TransactionProcessCrashRecovery, CombinedCoordinatorParticipantAndReplicaCrashesRecoverDecision) {
#if defined(_WIN32)
    GTEST_SKIP() << "The subprocess crash harness currently requires POSIX process controls";
#else
    const auto root = makeTempDirectory("combined_crash_matrix");
    const auto coordinator_wal = (std::filesystem::path(root) / "coordinator").string();
    const auto participant_wal = (std::filesystem::path(root) / "participant").string();
    const auto replica_wal = (std::filesystem::path(root) / "replica").string();
    std::filesystem::create_directories(coordinator_wal);
    std::filesystem::create_directories(participant_wal);
    std::filesystem::create_directories(replica_wal);

    auto crashed_coordinator =
        startCoordinatorChild("after_decision", coordinator_wal, participant_wal);
    char marker = '\0';
    ASSERT_TRUE(crashed_coordinator.waitForMarker(marker));
    ASSERT_EQ(marker, 'D');
    crashed_coordinator.terminateAndReap();

    const auto coordinator_entries = readWal(coordinator_wal);
    ASSERT_FALSE(coordinator_entries.empty());
    const auto transaction_id = coordinator_entries.front().transaction_id;
    ASSERT_FALSE(transaction_id.empty());

    auto crashed_participant =
        startParticipantChild("after_prepare", participant_wal, transaction_id);
    ASSERT_TRUE(crashed_participant.waitForMarker(marker));
    ASSERT_EQ(marker, 'P');
    crashed_participant.terminateAndReap();

    DistributedTxnManagerConfig recovery_config;
    recovery_config.wal_directory = coordinator_wal;
    recovery_config.recovery_endpoint_resolver =
        [](const std::string& node_id) -> std::optional<std::string> {
            return node_id == "participant-1"
                ? std::optional<std::string>("process://participant-1")
                : std::nullopt;
        };
    recovery_config.remote_phase2_dispatch =
        [&participant_wal](const std::string& txn_id,
                           const std::string&,
                           const std::string& endpoint,
                           bool do_commit) {
            if (endpoint != "process://participant-1" || !do_commit) {
                return false;
            }
            appendParticipantRecord(
                participant_wal, txn_id,
                themis::sharding::WALEntryType::COMMIT_TX);
            return true;
        };

    DistributedTransactionManager restarted_coordinator(
        "combined-crash-coordinator", recovery_config);
    const auto recovery_report =
        GlobalTwoPhaseCommitRecoveryManager::recoverAll({&restarted_coordinator});
    ASSERT_EQ(recovery_report.resolved, 1u);
    EXPECT_EQ(recovery_report.in_doubt_after, 0u);
    EXPECT_TRUE(recovery_report.ready_for_service);

    auto crashed_replica =
        startReplicaChild("during_commit", replica_wal, transaction_id);
    ASSERT_TRUE(crashed_replica.waitForMarker(marker));
    ASSERT_EQ(marker, 'C');
    crashed_replica.terminateAndReap();

    auto restarted_participant =
        startParticipantChild("verify_committed", participant_wal, transaction_id);
    ASSERT_TRUE(restarted_participant.waitForMarker(marker));
    EXPECT_EQ(marker, 'V');
    restarted_participant.terminateAndReap();

    auto restarted_replica =
        startReplicaChild("verify_commit", replica_wal, transaction_id);
    ASSERT_TRUE(restarted_replica.waitForMarker(marker));
    EXPECT_EQ(marker, 'V');
    restarted_replica.terminateAndReap();
    std::filesystem::remove_all(root);
#endif
}

TEST(TransactionProcessCrashRecovery, ReplicaProcessCrashMatrixRecoversCommittedDecision) {
#if defined(_WIN32)
    GTEST_SKIP() << "The subprocess crash harness currently requires POSIX process controls";
#else
    struct Scenario {
        const char* stage;
        char marker;
        bool expected_prepare;
        bool expected_commit;
    };
    const std::vector<Scenario> scenarios = {
        {"before_prepare", 'B', false, false},
        {"after_prepare", 'P', true, false},
        {"after_decision", 'D', false, true},
        {"during_commit", 'C', false, true},
        {"after_ack", 'A', false, true}
    };

    for (const auto& scenario : scenarios) {
        SCOPED_TRACE(scenario.stage);
        const auto root = makeTempDirectory(scenario.stage);
        const auto replica_wal = (std::filesystem::path(root) / "replica").string();
        std::filesystem::create_directories(replica_wal);
        const auto transaction_id = std::string("replica-matrix-") + scenario.stage;

        auto crashed_replica =
            startReplicaChild(scenario.stage, replica_wal, transaction_id);
        char marker = '\0';
        ASSERT_TRUE(crashed_replica.waitForMarker(marker));
        ASSERT_EQ(marker, scenario.marker);
        crashed_replica.terminateAndReap();

        const auto entries = readWal(replica_wal);
        const auto has_type = [&entries, &transaction_id](
            themis::sharding::WALEntryType type) {
            return std::any_of(
                entries.begin(), entries.end(),
                [&transaction_id, type](const auto& entry) {
                    return entry.transaction_id == transaction_id && entry.type == type;
                });
        };
        EXPECT_EQ(has_type(themis::sharding::WALEntryType::PREPARE_TX),
                  scenario.expected_prepare);
        EXPECT_EQ(has_type(themis::sharding::WALEntryType::COMMIT_TX),
                  scenario.expected_commit);

        const auto recovery_stage = scenario.expected_commit ? "verify_commit"
            : scenario.expected_prepare ? "verify_prepare" : "verify_empty";
        auto recovered_replica =
            startReplicaChild(recovery_stage, replica_wal, transaction_id);
        ASSERT_TRUE(recovered_replica.waitForMarker(marker));
        EXPECT_EQ(marker, 'V');
        recovered_replica.terminateAndReap();
        std::filesystem::remove_all(root);
    }
#endif
}

int main(int argc, char** argv) {
#if !defined(_WIN32)
    ::signal(SIGPIPE, SIG_IGN);
    if (argc == 7 && std::string(argv[1]) == "--canonical-participant") {
        try {
            return runCanonicalParticipant(
                argv[2], argv[3], std::stoi(argv[4]), std::stoi(argv[5]),
                std::stoi(argv[6]));
        } catch (const std::exception& error) {
            std::cerr << "canonical participant: " << error.what() << '\n';
            return 33;
        }
    }
    if (argc == 7 && std::string(argv[1]) == "--canonical-coordinator") {
        try {
            return runCanonicalCoordinator(
                argv[2], argv[3], std::string(argv[4]) == "commit",
                nlohmann::json::parse(argv[5]), std::stoi(argv[6]));
        } catch (const std::exception& error) {
            std::cerr << "canonical coordinator: " << error.what() << '\n';
            return 42;
        }
    }
    if (argc == 6 && std::string(argv[1]) == "--dtm-crash-child") {
        try {
            return runCoordinatorChild(
                argv[2], argv[3], argv[4], std::stoi(argv[5]));
        } catch (...) {
            return 5;
        }
    }
    if (argc == 6 && std::string(argv[1]) == "--participant-crash-child") {
        try {
            return runParticipantChild(
                argv[2], argv[3], argv[4], std::stoi(argv[5]));
        } catch (...) {
            return 6;
        }
    }
    if (argc == 6 && std::string(argv[1]) == "--replica-crash-child") {
        try {
            return runReplicaChild(
                argv[2], argv[3], argv[4], std::stoi(argv[5]));
        } catch (...) {
            return 7;
        }
    }
#endif
    g_executable_path = std::filesystem::absolute(argv[0]).string();
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
