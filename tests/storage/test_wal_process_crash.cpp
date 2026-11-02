// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#include <gtest/gtest.h>

#include "storage/wal_storage.h"

#include <chrono>
#include <filesystem>
#include <string>
#include <utility>
#include <vector>

#if defined(__unix__) || defined(__APPLE__)

#include <cerrno>
#include <csignal>
#include <cstring>
#include <fcntl.h>
#include <poll.h>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>

extern char** environ;

namespace {

namespace fs = std::filesystem;

class WalCrashWorker {
public:
    WalCrashWorker() = default;
    WalCrashWorker(const WalCrashWorker&) = delete;
    WalCrashWorker& operator=(const WalCrashWorker&) = delete;

    ~WalCrashWorker() {
        if (pid_ > 0) {
            ::kill(pid_, SIGKILL);
            int status = 0;
            while (::waitpid(pid_, &status, 0) < 0 && errno == EINTR) {
            }
        }
        closeDescriptors();
    }

    bool start(const std::string& directory, std::string& error) {
        int parent_to_child[2] = {-1, -1};
        int child_to_parent[2] = {-1, -1};
        if (::pipe(parent_to_child) != 0 || ::pipe(child_to_parent) != 0) {
            error = std::strerror(errno);
            closePipe(parent_to_child);
            closePipe(child_to_parent);
            return false;
        }
        if (!moveAboveStandardDescriptors(parent_to_child, error) ||
            !moveAboveStandardDescriptors(child_to_parent, error)) {
            closePipe(parent_to_child);
            closePipe(child_to_parent);
            return false;
        }

        posix_spawn_file_actions_t actions;
        int spawn_error = ::posix_spawn_file_actions_init(&actions);
        if (spawn_error != 0) {
            error = std::strerror(spawn_error);
            closePipe(parent_to_child);
            closePipe(child_to_parent);
            return false;
        }
        spawn_error = ::posix_spawn_file_actions_adddup2(
            &actions, parent_to_child[0], STDIN_FILENO);
        if (spawn_error == 0) {
            spawn_error = ::posix_spawn_file_actions_adddup2(
                &actions, child_to_parent[1], STDOUT_FILENO);
        }
        for (const int descriptor : {parent_to_child[0], parent_to_child[1],
                                     child_to_parent[0], child_to_parent[1]}) {
            if (spawn_error == 0) {
                spawn_error =
                    ::posix_spawn_file_actions_addclose(&actions, descriptor);
            }
        }

        const std::string rotation_threshold = "1";
        const std::string checkpoint_after = "3";
        char* const arguments[] = {
            const_cast<char*>(THEMIS_STO07_WAL_CRASH_WORKER),
            const_cast<char*>(directory.c_str()),
            const_cast<char*>(rotation_threshold.c_str()),
            const_cast<char*>(checkpoint_after.c_str()),
            nullptr,
        };
        if (spawn_error == 0) {
            spawn_error = ::posix_spawn(&pid_, THEMIS_STO07_WAL_CRASH_WORKER,
                                        &actions, nullptr, arguments, environ);
        }
        ::posix_spawn_file_actions_destroy(&actions);

        if (spawn_error != 0) {
            error = std::strerror(spawn_error);
            closePipe(parent_to_child);
            closePipe(child_to_parent);
            pid_ = -1;
            return false;
        }

        ::close(parent_to_child[0]);
        ::close(child_to_parent[1]);
        write_fd_ = parent_to_child[1];
        read_fd_ = child_to_parent[0];
        return true;
    }

    bool readLine(std::string& line) const {
        line.clear();
        const auto deadline = std::chrono::steady_clock::now() +
                              std::chrono::seconds(10);
        while (std::chrono::steady_clock::now() < deadline) {
            const auto remaining = std::chrono::duration_cast<
                std::chrono::milliseconds>(deadline -
                                           std::chrono::steady_clock::now());
            pollfd descriptor{read_fd_, POLLIN, 0};
            const int ready =
                ::poll(&descriptor, 1, static_cast<int>(remaining.count()));
            if (ready < 0 && errno == EINTR) {
                continue;
            }
            if (ready <= 0) {
                return false;
            }

            char character = '\0';
            const auto bytes_read = ::read(read_fd_, &character, 1);
            if (bytes_read <= 0) {
                return false;
            }
            if (character == '\n') {
                return true;
            }
            line.push_back(character);
        }
        return false;
    }

    bool continueWorker() const {
        constexpr char command = 'C';
        while (true) {
            const auto bytes_written = ::write(write_fd_, &command, 1);
            if (bytes_written == 1) {
                return true;
            }
            if (bytes_written < 0 && errno == EINTR) {
                continue;
            }
            return false;
        }
    }

    bool killAndWait() {
        if (pid_ <= 0 || ::kill(pid_, SIGKILL) != 0) {
            return false;
        }

        int status = 0;
        pid_t waited = -1;
        do {
            waited = ::waitpid(pid_, &status, 0);
        } while (waited < 0 && errno == EINTR);
        pid_ = -1;
        return waited > 0 && WIFSIGNALED(status) && WTERMSIG(status) == SIGKILL;
    }

private:
    static bool moveAboveStandardDescriptors(int (&descriptors)[2],
                                             std::string& error) {
        for (int& descriptor : descriptors) {
            if (descriptor > STDERR_FILENO) {
                continue;
            }
            const int replacement = ::fcntl(descriptor, F_DUPFD, STDERR_FILENO + 1);
            if (replacement < 0) {
                error = std::strerror(errno);
                return false;
            }
            ::close(descriptor);
            descriptor = replacement;
        }
        return true;
    }

    static void closePipe(int (&descriptors)[2]) {
        for (int& descriptor : descriptors) {
            if (descriptor >= 0) {
                ::close(descriptor);
                descriptor = -1;
            }
        }
    }

    void closeDescriptors() {
        if (write_fd_ >= 0) {
            ::close(write_fd_);
            write_fd_ = -1;
        }
        if (read_fd_ >= 0) {
            ::close(read_fd_);
            read_fd_ = -1;
        }
    }

    pid_t pid_ = -1;
    int write_fd_ = -1;
    int read_fd_ = -1;
};

class WalProcessCrashTest : public ::testing::Test {
protected:
    void SetUp() override {
        const auto unique_suffix =
            std::to_string(::getpid()) + "_" +
            std::to_string(std::chrono::steady_clock::now()
                               .time_since_epoch()
                               .count());
        directory_ =
            (fs::temp_directory_path() / ("themis_sto07_" + unique_suffix))
                .string();
        fs::remove_all(directory_);
    }

    void TearDown() override {
        fs::remove_all(directory_);
    }

    themis::WALStorage::Config config() const {
        themis::WALStorage::Config wal_config;
        wal_config.dir = directory_;
        wal_config.rotation_threshold_bytes = 1;
        wal_config.fsync_on_write = true;
        return wal_config;
    }

    size_t segmentCount() const {
        size_t count = 0;
        for (const auto& entry : fs::directory_iterator(directory_)) {
            if (entry.path().filename().string().starts_with("wal_") &&
                entry.path().extension() == ".log") {
                ++count;
            }
        }
        return count;
    }

    void expectRecoveredPuts(size_t acknowledged_count) {
        std::vector<std::pair<std::string, std::string>> recovered;
        themis::WALStorage::RecoveryCallback callback =
            [&recovered](const themis::WALStorage::Entry& entry) {
                if (entry.type == themis::WALStorage::EntryType::PUT) {
                    recovered.emplace_back(entry.key, entry.value);
                }
                return true;
            };
        themis::WALStorage::RecoveryResult recovery;
        auto reopened = themis::WALStorage::open(config(), callback, &recovery);
        ASSERT_TRUE(reopened);
        EXPECT_EQ(recovery.status,
                  themis::WALStorage::RecoveryStatus::Complete);
        ASSERT_EQ(recovered.size(), acknowledged_count);
        for (size_t index = 0; index < acknowledged_count; ++index) {
            const auto sequence = std::to_string(index + 1);
            EXPECT_EQ(recovered[index].first, "key_" + sequence);
            EXPECT_EQ(recovered[index].second, "value_" + sequence);
        }
    }

    std::string directory_;
};

TEST_F(WalProcessCrashTest, AcknowledgedWritesSurviveKillAfterRotation) {
    WalCrashWorker worker;
    std::string error;
    ASSERT_TRUE(worker.start(directory_, error)) << error;

    std::string message;
    ASSERT_TRUE(worker.readLine(message));
    ASSERT_EQ(message, "READY");
    for (size_t sequence = 1; sequence <= 2; ++sequence) {
        ASSERT_TRUE(worker.readLine(message));
        ASSERT_EQ(message, "ACK " + std::to_string(sequence));
        if (sequence < 2) {
            ASSERT_TRUE(worker.continueWorker());
        }
    }

    ASSERT_GE(segmentCount(), 2u);
    ASSERT_TRUE(worker.killAndWait());
    expectRecoveredPuts(2);
}

TEST_F(WalProcessCrashTest, AcknowledgedWritesSurviveKillAfterCheckpoint) {
    WalCrashWorker worker;
    std::string error;
    ASSERT_TRUE(worker.start(directory_, error)) << error;

    std::string message;
    ASSERT_TRUE(worker.readLine(message));
    ASSERT_EQ(message, "READY");
    for (size_t sequence = 1; sequence <= 3; ++sequence) {
        ASSERT_TRUE(worker.readLine(message));
        ASSERT_EQ(message, "ACK " + std::to_string(sequence));
        ASSERT_TRUE(worker.continueWorker());
    }
    ASSERT_TRUE(worker.readLine(message));
    ASSERT_EQ(message, "CHECKPOINT 3");
    ASSERT_GE(segmentCount(), 4u);

    ASSERT_TRUE(worker.killAndWait());
    expectRecoveredPuts(3);
}

}  // namespace

#else

TEST(WalProcessCrashTest, RequiresPosixProcessControls) {
    GTEST_SKIP() << "The WAL subprocess crash harness requires POSIX.";
}

#endif
