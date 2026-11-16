// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#include "replication/replication_manager.h"

#include <gtest/gtest.h>

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <string>
#include <system_error>

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

class ReplicationWalCrashWorker {
public:
    ReplicationWalCrashWorker() = default;
    ReplicationWalCrashWorker(const ReplicationWalCrashWorker&) = delete;
    ReplicationWalCrashWorker& operator=(const ReplicationWalCrashWorker&) = delete;

    ~ReplicationWalCrashWorker() {
        terminate();
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

        char* const arguments[] = {
            const_cast<char*>(THEMIS_REPLICATION_WAL_CRASH_WORKER),
            const_cast<char*>(directory.c_str()),
            nullptr,
        };
        if (spawn_error == 0) {
            spawn_error = ::posix_spawn(
                &pid_, THEMIS_REPLICATION_WAL_CRASH_WORKER, &actions, nullptr,
                arguments, environ);
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
        for (;;) {
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
            const int replacement =
                ::fcntl(descriptor, F_DUPFD, STDERR_FILENO + 1);
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

    void terminate() {
        if (pid_ <= 0) {
            return;
        }
        ::kill(pid_, SIGKILL);
        int status = 0;
        while (::waitpid(pid_, &status, 0) < 0 && errno == EINTR) {
        }
        pid_ = -1;
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

class TemporaryDirectory {
public:
    explicit TemporaryDirectory(const std::string& name) {
        path_ = (fs::temp_directory_path() /
                 ("themis_replication_crash_" + name + "_" +
                  std::to_string(
                      std::chrono::steady_clock::now().time_since_epoch().count())))
                    .string();
        fs::create_directories(path_);
    }

    ~TemporaryDirectory() {
        std::error_code error;
        fs::remove_all(path_, error);
    }

    const std::string& path() const { return path_; }

private:
    std::string path_;
};

void expectRecoveredState(const std::string& directory,
                          uint64_t expected_sequence) {
    themisdb::replication::ReplicationConfig config;
    config.wal_directory = directory;
    config.wal_sync_on_commit = true;
    config.wal_segment_size_bytes = 1;

    themisdb::replication::WALManager recovered(config);
    EXPECT_EQ(recovered.getRecoveryStatus(),
              themisdb::replication::WALRecoveryStatus::COMPLETE);
    EXPECT_EQ(recovered.getCurrentSequence(), expected_sequence);
    EXPECT_EQ(recovered.getCurrentTerm(), 1u);

    const auto entries = recovered.readFrom(1);
    ASSERT_EQ(entries.size(), expected_sequence);
    for (uint64_t index = 1; index <= expected_sequence; ++index) {
        EXPECT_EQ(entries[index - 1].sequence_number, index);
        EXPECT_EQ(entries[index - 1].document_id,
                  "entry-" + std::to_string(index));
        EXPECT_EQ(entries[index - 1].data,
                  "durable-payload-" + std::to_string(index));
        EXPECT_EQ(entries[index - 1].term, 1u);
    }
}

TEST(ReplicationWalProcessCrash, RestartPreservesTermAndEveryAcknowledgedEntry) {
    for (const uint64_t acknowledged_count : {0u, 1u, 2u}) {
        SCOPED_TRACE("acknowledged_count=" + std::to_string(acknowledged_count));
        TemporaryDirectory directory(std::to_string(acknowledged_count));
        ReplicationWalCrashWorker worker;
        std::string error;
        ASSERT_TRUE(worker.start(directory.path(), error)) << error;

        std::string message;
        ASSERT_TRUE(worker.readLine(message));
        ASSERT_EQ(message, "READY");
        ASSERT_TRUE(worker.readLine(message));
        ASSERT_EQ(message, "TERM 1");

        if (acknowledged_count > 0) {
            ASSERT_TRUE(worker.continueWorker());
        }
        for (uint64_t sequence = 1; sequence <= acknowledged_count; ++sequence) {
            ASSERT_TRUE(worker.readLine(message));
            ASSERT_EQ(message, "ACK " + std::to_string(sequence));
            if (sequence < acknowledged_count) {
                ASSERT_TRUE(worker.continueWorker());
            }
        }

        ASSERT_TRUE(worker.killAndWait());
        expectRecoveredState(directory.path(), acknowledged_count);
    }
}

}  // namespace

#else

TEST(ReplicationWalProcessCrash, RequiresPosixProcessControls) {
    GTEST_SKIP() << "The replication WAL subprocess test requires POSIX.";
}

#endif
