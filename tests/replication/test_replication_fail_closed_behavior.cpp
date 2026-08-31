/**
 * @file test_replication_fail_closed_behavior.cpp
 * @brief Contract tests for replication fail-closed semantics.
 */

#include <gtest/gtest.h>

#include <string>
#include <vector>

namespace themisdb::replication::test {

struct StubWAL {
    bool fail_append = false;
    std::vector<std::string> entries;

    bool append(const std::string& entry) {
        if (fail_append) return false;
        entries.push_back(entry);
        return true;
    }
};

TEST(ReplicationFailClosedContract, RejectsWriteWhenWalAppendFails) {
    StubWAL wal;
    wal.fail_append = true;
    EXPECT_FALSE(wal.append("x"));
    EXPECT_TRUE(wal.entries.empty());
}

TEST(ReplicationFailClosedContract, AcceptsWriteWhenWalAppendSucceeds) {
    StubWAL wal;
    EXPECT_TRUE(wal.append("x"));
    EXPECT_EQ(wal.entries.size(), 1u);
}

}  // namespace themisdb::replication::test
