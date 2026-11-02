#include "transaction/distributed_transaction_manager.h"
#include "sharding/two_phase_commit_participant.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <chrono>
#include <filesystem>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
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
    wal.append(entry);
    if (!wal.flush()) {
        throw std::runtime_error("participant WAL flush failed");
    }
}

std::vector<themis::sharding::WALEntry> readWal(const std::string& wal_directory) {
    themis::sharding::WALManagerConfig config;
    config.wal_directory = wal_directory;
    themis::sharding::WALManager wal(config);
    return wal.readRange(wal.getOldestLSN());
}

std::string makeTempDirectory(const std::string& scenario) {
    const auto path = std::filesystem::temp_directory_path() /
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
    themis::sharding::TwoPhaseCommitParticipant participant(
        "participant-process", config,
        [](const std::string&, const nlohmann::json&) { return true; },
        [&stage, control_fd](const std::string&, const nlohmann::json&, int64_t) {
            if (stage == "during_commit") {
                notifyAndWait(control_fd, 'A');
            }
            return true;
        });

    if (stage == "prepare") {
        const auto payload =
            R"({"operations":[{"type":"put","key":"participant-key","value":"durable"}]})";
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
        const auto payload =
            R"({"operations":[{"type":"put","key":"participant-key","value":"durable"}]})";
        if (!participant.onPrepare(transaction_id, "coordinator-process", payload)) {
            return 13;
        }
        notifyAndWait(control_fd, 'P');
        return 0;
    }

    if (stage == "during_commit" || stage == "after_ack") {
        const auto payload =
            R"({"operations":[{"type":"put","key":"participant-key","value":"durable"}]})";
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
#endif

} // namespace

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
