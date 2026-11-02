#include "transaction/distributed_transaction_manager.h"

#include <gtest/gtest.h>

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
#endif
    g_executable_path = std::filesystem::absolute(argv[0]).string();
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
