// Unit tests for Cicada OCC — data install_writes() and transaction protocol

#include "performance/cicada.h"
#include <gtest/gtest.h>
#include <string>
#include <thread>
#include <vector>

using namespace themis::performance;

// -----------------------------------------------------------------------
// CicadaRecord — data storage
// -----------------------------------------------------------------------

TEST(CicadaRecordTest, DefaultDataIsEmpty) {
    CicadaRecord rec;
    EXPECT_EQ(rec.get_data(), "");
}

TEST(CicadaRecordTest, InitialDataConstructor) {
    CicadaRecord rec("hello");
    EXPECT_EQ(rec.get_data(), "hello");
}

TEST(CicadaRecordTest, SetDataStoresValue) {
    CicadaRecord rec;
    rec.set_data("world");
    EXPECT_EQ(rec.get_data(), "world");
}

TEST(CicadaRecordTest, SetDataOverwritesPreviousValue) {
    CicadaRecord rec("initial");
    rec.set_data("updated");
    EXPECT_EQ(rec.get_data(), "updated");
}

// -----------------------------------------------------------------------
// CicadaRecord — version / lock
// -----------------------------------------------------------------------

TEST(CicadaRecordTest, InitialVersionIsZero) {
    CicadaRecord rec;
    EXPECT_EQ(rec.get_version(), 0u);
}

TEST(CicadaRecordTest, LockUnlockIncrementsVersion) {
    CicadaRecord rec;
    EXPECT_TRUE(rec.try_lock());
    EXPECT_TRUE(rec.is_locked());
    rec.unlock_and_increment_version();
    EXPECT_FALSE(rec.is_locked());
    EXPECT_EQ(rec.get_version(), 1u);
}

TEST(CicadaRecordTest, DoubleLocksAreRejected) {
    CicadaRecord rec;
    EXPECT_TRUE(rec.try_lock());
    EXPECT_FALSE(rec.try_lock()); // already locked
    rec.unlock_and_increment_version();
}

// -----------------------------------------------------------------------
// CicadaTransaction — record_write now accepts data
// -----------------------------------------------------------------------

TEST(CicadaTransactionTest, CommitWritesDataToRecord) {
    CicadaRecord rec("before");
    CicadaTransaction tx;

    // Record a write with new data
    uint64_t v0 = rec.get_version();
    tx.record_read(&rec, v0);
    tx.record_write(&rec, "after");

    EXPECT_TRUE(tx.commit());
    EXPECT_EQ(rec.get_data(), "after");
}

TEST(CicadaTransactionTest, CommitIncrementsVersion) {
    CicadaRecord rec("v0");
    CicadaTransaction tx;

    uint64_t v0 = rec.get_version();
    tx.record_read(&rec, v0);
    tx.record_write(&rec, "v1");

    EXPECT_TRUE(tx.commit());
    EXPECT_GT(rec.get_version(), v0);
}

TEST(CicadaTransactionTest, AbortDoesNotWriteData) {
    CicadaRecord rec("original");
    CicadaTransaction tx;

    uint64_t v0 = rec.get_version();
    tx.record_read(&rec, v0);
    tx.record_write(&rec, "new_value");

    tx.abort();
    EXPECT_TRUE(tx.is_aborted());
    // Data must NOT have changed after an abort
    EXPECT_EQ(rec.get_data(), "original");
}

TEST(CicadaTransactionTest, CommitFailsAfterVersionChange) {
    CicadaRecord rec("initial");
    CicadaTransaction tx;

    // Record version before another writer modifies it
    uint64_t stale_version = rec.get_version();
    tx.record_read(&rec, stale_version);
    tx.record_write(&rec, "from_tx");

    // Simulate a concurrent writer bumping the version
    CicadaTransaction concurrent_tx;
    concurrent_tx.record_write(&rec, "from_concurrent");
    concurrent_tx.commit();

    // Now our transaction's read validation should fail
    EXPECT_FALSE(tx.commit());
    EXPECT_TRUE(tx.is_aborted());
    // Data should reflect the committed concurrent write, not our aborted one
    EXPECT_EQ(rec.get_data(), "from_concurrent");
}

// -----------------------------------------------------------------------
// ContentionManager
// -----------------------------------------------------------------------

TEST(ContentionManagerTest, InitialAbortRateIsZero) {
    ContentionManager cm;
    EXPECT_DOUBLE_EQ(cm.get_abort_rate(), 0.0);
    EXPECT_FALSE(cm.should_backoff());
}

TEST(ContentionManagerTest, HighAbortRateTriggersBackoff) {
    ContentionManager cm;
    for (int i = 0; i < 10; ++i) cm.record_abort();
    for (int i = 0; i < 2; ++i) cm.record_commit();
    EXPECT_GT(cm.get_abort_rate(), 0.5);
    EXPECT_TRUE(cm.should_backoff());
}

TEST(ContentionManagerTest, ResetClearsStats) {
    ContentionManager cm;
    cm.record_abort();
    cm.reset_stats();
    EXPECT_DOUBLE_EQ(cm.get_abort_rate(), 0.0);
}
