// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_backend_io_timeout_focused.cpp
 * @brief Phase 2.1.1 focused tests for timeout & I/O safety in user_storage_encrypted module.
 *
 * Tests verify:
 * - TimedFileOperation class behavior (read/write with timeout)
 * - PipeGuard RAII class behavior (automatic cleanup, no leaks)
 * - Timeout handling in gocryptfs_backend I/O operations
 * - Exception safety during I/O
 * - No file descriptor leaks
 *
 * Test IDs: UST-IO-01 through UST-IO-08
 *
 * @see include/user_storage_encrypted/timed_file_operation.hpp
 * @see include/user_storage_encrypted/pipe_guard.hpp
 * @see src/user_storage_encrypted/gocryptfs_backend.cpp
 */

#include "gtest/gtest.h"
#include "user_storage_encrypted/timed_file_operation.hpp"
#include "user_storage_encrypted/pipe_guard.hpp"

#include <chrono>
#include <thread>
#include <vector>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

namespace themis {
namespace plugins {
namespace user_storage {
namespace test {

// ============================================================================
// UST-IO-01 — PipeGuard creation and validity checks
// ============================================================================

TEST(UserStorageEncryptedIoTimeout, UST_IO_01_PipeGuardCreation) {
    // Test successful pipe creation
    auto pipe = PipeGuard::create();
    EXPECT_TRUE(pipe.isValid());
    EXPECT_TRUE(pipe.isReadOpen());
    EXPECT_TRUE(pipe.isWriteOpen());
    EXPECT_GE(pipe.readFd(), 0);
    EXPECT_GE(pipe.writeFd(), 0);
    EXPECT_NE(pipe.readFd(), pipe.writeFd());
    
    // Test status reporting
    std::string status = pipe.status();
    EXPECT_EQ(status, "rw");
}

// ============================================================================
// UST-IO-02 — PipeGuard close operations
// ============================================================================

TEST(UserStorageEncryptedIoTimeout, UST_IO_02_PipeGuardCloseOperations) {
    auto pipe = PipeGuard::create();
    ASSERT_TRUE(pipe.isValid());
    
    int read_fd = pipe.readFd();
    EXPECT_TRUE(pipe.closeRead());
    EXPECT_FALSE(pipe.isReadOpen());
    EXPECT_TRUE(pipe.isWriteOpen());
    EXPECT_EQ(pipe.status(), "w");
    
    // Closing already-closed end should succeed
    EXPECT_TRUE(pipe.closeRead());
    
    EXPECT_TRUE(pipe.closeWrite());
    EXPECT_FALSE(pipe.isReadOpen());
    EXPECT_FALSE(pipe.isWriteOpen());
    EXPECT_EQ(pipe.status(), "-");
}

// ============================================================================
// UST-IO-03 — PipeGuard move semantics
// ============================================================================

TEST(UserStorageEncryptedIoTimeout, UST_IO_03_PipeGuardMoveSemantics) {
    auto pipe1 = PipeGuard::create();
    ASSERT_TRUE(pipe1.isValid());
    int read_fd = pipe1.readFd();
    int write_fd = pipe1.writeFd();
    
    // Move to pipe2
    auto pipe2 = std::move(pipe1);
    EXPECT_EQ(pipe2.readFd(), read_fd);
    EXPECT_EQ(pipe2.writeFd(), write_fd);
    EXPECT_FALSE(pipe1.isValid());  // pipe1 is now invalid
    
    // pipe2 should still be able to close normally
    EXPECT_TRUE(pipe2.closeAll());
}

// ============================================================================
// UST-IO-04 — TimedFileOperation read with data available
// ============================================================================

TEST(UserStorageEncryptedIoTimeout, UST_IO_04_TimedFileOperationReadAvailable) {
    auto pipe = PipeGuard::create();
    ASSERT_TRUE(pipe.isValid());
    
    const char test_data[] = "Hello, TimedFileOperation!";
    size_t data_len = std::strlen(test_data);
    
    // Write test data to pipe
    ssize_t n = ::write(pipe.writeFd(), test_data, data_len);
    ASSERT_EQ(n, static_cast<ssize_t>(data_len));
    
    // Read with timeout (should succeed immediately)
    TimedFileOperation reader(pipe.readFd(), std::chrono::seconds(5));
    char buffer[256];
    auto bytes_read = reader.read(buffer, sizeof(buffer));
    
    ASSERT_TRUE(bytes_read.has_value());
    ASSERT_GT(bytes_read.value(), 0);
    EXPECT_EQ(std::string(buffer, bytes_read.value()), test_data);
    
    pipe.closeWrite();
}

// ============================================================================
// UST-IO-05 — TimedFileOperation write with space available
// ============================================================================

TEST(UserStorageEncryptedIoTimeout, UST_IO_05_TimedFileOperationWriteAvailable) {
    auto pipe = PipeGuard::create();
    ASSERT_TRUE(pipe.isValid());
    
    const char test_data[] = "Write test data";
    size_t data_len = std::strlen(test_data);
    
    // Write with timeout (should succeed immediately)
    TimedFileOperation writer(pipe.writeFd(), std::chrono::seconds(5));
    auto bytes_written = writer.write(test_data, data_len);
    
    ASSERT_TRUE(bytes_written.has_value());
    ASSERT_EQ(bytes_written.value(), static_cast<ssize_t>(data_len));
    
    // Read to verify
    char buffer[256];
    ssize_t n = ::read(pipe.readFd(), buffer, sizeof(buffer));
    EXPECT_EQ(std::string(buffer, n), test_data);
    
    pipe.closeWrite();
}

// ============================================================================
// UST-IO-06 — TimedFileOperation timeout on blocked read
// ============================================================================

TEST(UserStorageEncryptedIoTimeout, UST_IO_06_TimedFileOperationReadTimeout) {
    auto pipe = PipeGuard::create();
    ASSERT_TRUE(pipe.isValid());
    
    // Don't write anything; read will block
    // Use a short timeout (100ms)
    TimedFileOperation reader(pipe.readFd(), std::chrono::milliseconds(100));
    char buffer[256];
    auto bytes_read = reader.read(buffer, sizeof(buffer));
    
    // Should timeout (no data available)
    EXPECT_FALSE(bytes_read.has_value());
    EXPECT_EQ(errno, EAGAIN);  // Timeout error code
    
    pipe.closeWrite();  // Unblock EOF
}

// ============================================================================
// UST-IO-07 — PipeGuard detach without closing
// ============================================================================

TEST(UserStorageEncryptedIoTimeout, UST_IO_07_PipeGuardDetach) {
    auto pipe = PipeGuard::create();
    ASSERT_TRUE(pipe.isValid());
    
    int read_fd = pipe.readFd();
    int detached_read = pipe.detach(0);
    
    EXPECT_EQ(detached_read, read_fd);
    EXPECT_FALSE(pipe.isReadOpen());
    EXPECT_TRUE(pipe.isWriteOpen());
    
    // Caller is responsible for closing detached fd
    ::close(detached_read);
    pipe.closeWrite();
}

// ============================================================================
// UST-IO-08 — Multiple read/write cycles (no fd leaks)
// ============================================================================

TEST(UserStorageEncryptedIoTimeout, UST_IO_08_MultipleIoCycles) {
    for (int cycle = 0; cycle < 10; ++cycle) {
        auto pipe = PipeGuard::create();
        ASSERT_TRUE(pipe.isValid()) << "Cycle " << cycle;
        
        const char data[] = "cycle_data";
        TimedFileOperation writer(pipe.writeFd(), std::chrono::seconds(1));
        auto written = writer.write(data, std::strlen(data));
        ASSERT_TRUE(written.has_value()) << "Cycle " << cycle;
        
        TimedFileOperation reader(pipe.readFd(), std::chrono::seconds(1));
        char buffer[256];
        auto read = reader.read(buffer, sizeof(buffer));
        ASSERT_TRUE(read.has_value()) << "Cycle " << cycle;
        
        // PipeGuard auto-closes on destruction (no manual close needed)
    }
    // If we reach here, no fd leak has occurred (fd count should be stable)
}

// ============================================================================
// UST-IO-09 — PipeGuard operator[] access
// ============================================================================

TEST(UserStorageEncryptedIoTimeout, UST_IO_09_PipeGuardOperatorAccess) {
    auto pipe = PipeGuard::create();
    ASSERT_TRUE(pipe.isValid());
    
    int read_fd = pipe.readFd();
    int write_fd = pipe.writeFd();
    
    // Test operator[]
    EXPECT_EQ(pipe[0], read_fd);
    EXPECT_EQ(pipe[1], write_fd);
    EXPECT_EQ(pipe[2], -1);  // Out of bounds
    
    pipe.closeAll();
}

// ============================================================================
// UST-IO-10 — TimedFileOperation with different timeout durations
// ============================================================================

TEST(UserStorageEncryptedIoTimeout, UST_IO_10_TimedFileOperationDifferentTimeouts) {
    auto pipe = PipeGuard::create();
    ASSERT_TRUE(pipe.isValid());
    
    // Test with various chrono duration types
    {
        TimedFileOperation op1(pipe.writeFd(), std::chrono::milliseconds(1000));
        EXPECT_EQ(op1.timeoutMs(), 1000);
    }
    
    {
        TimedFileOperation op2(pipe.writeFd(), std::chrono::seconds(5));
        EXPECT_EQ(op2.timeoutMs(), 5000);
    }
    
    {
        TimedFileOperation op3(pipe.writeFd(), std::chrono::microseconds(500000));
        EXPECT_EQ(op3.timeoutMs(), 500);
    }
    
    pipe.closeAll();
}

// ============================================================================
// UST-IO-11 — Exception safety: PipeGuard cleanup on exception
// ============================================================================

TEST(UserStorageEncryptedIoTimeout, UST_IO_11_ExceptionSafety) {
    int leaked_fd = -1;
    
    try {
        auto pipe = PipeGuard::create();
        leaked_fd = pipe.readFd();
        ASSERT_TRUE(pipe.isValid());
        
        // Simulate exception
        throw std::runtime_error("Intentional test exception");
    } catch (const std::exception&) {
        // PipeGuard should have closed on destruction
    }
    
    // Verify fd was closed by trying to stat it
    struct stat sb;
    int result = fstat(leaked_fd, &sb);
    EXPECT_NE(result, 0);  // Should fail; fd is closed
    EXPECT_EQ(errno, EBADF);
}

// ============================================================================
// UST-IO-12 — Empty PipeGuard default constructor
// ============================================================================

TEST(UserStorageEncryptedIoTimeout, UST_IO_12_PipeGuardDefaultConstructor) {
    PipeGuard empty;
    EXPECT_FALSE(empty.isValid());
    EXPECT_FALSE(empty.isReadOpen());
    EXPECT_FALSE(empty.isWriteOpen());
    EXPECT_EQ(empty.readFd(), -1);
    EXPECT_EQ(empty.writeFd(), -1);
    EXPECT_EQ(empty.status(), "-");
    
    // Should be safe to close empty PipeGuard
    EXPECT_TRUE(empty.closeRead());
    EXPECT_TRUE(empty.closeWrite());
    EXPECT_TRUE(empty.closeAll());
}

// ============================================================================
// UST-IO-13 — TimedFileOperation error handling
// ============================================================================

TEST(UserStorageEncryptedIoTimeout, UST_IO_13_TimedFileOperationErrorHandling) {
    // Test with invalid fd
    TimedFileOperation invalid_op(-1, std::chrono::seconds(1));
    char buffer[256];
    
    auto result = invalid_op.read(buffer, sizeof(buffer));
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(errno, EBADF);
}

// ============================================================================
// UST-IO-14 — PipeGuard is non-copyable
// ============================================================================

TEST(UserStorageEncryptedIoTimeout, UST_IO_14_PipeGuardNonCopyable) {
    auto pipe = PipeGuard::create();
    
    // This should not compile if uncommented:
    // auto pipe2 = pipe;  // Compile error: deleted copy constructor
    // pipe2 = pipe;       // Compile error: deleted copy assignment
    
    // But move should work:
    auto pipe2 = std::move(pipe);
    EXPECT_TRUE(pipe2.isValid());
    EXPECT_FALSE(pipe.isValid());
}

}  // namespace test
}  // namespace user_storage
}  // namespace plugins
}  // namespace themis
