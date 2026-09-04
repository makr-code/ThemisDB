#include <gtest/gtest.h>
#include "utils/audit_logger.h"

#include <filesystem>
#include <fstream>
#include <cstdint>
#include <string>

using namespace themis::utils;
namespace fs = std::filesystem;

// ---------------------------------------------------------------------------
// Test fixture
// ---------------------------------------------------------------------------

class HashChainAuditTest : public ::testing::Test {
protected:
    static constexpr const char* kLog  = "/tmp/test_hc_audit.jsonl";
    static constexpr const char* kHead = "/tmp/test_hc_head.bin";

    HashChainAuditWriterConfig cfg;

    void SetUp() override {
        cfg.log_path        = kLog;
        cfg.chain_head_path = kHead;
        fs::remove(kLog);
        fs::remove(kHead);
    }

    void TearDown() override {
        fs::remove(kLog);
        fs::remove(kHead);
    }
};

// ============================================================================
// HashChainAuditWriter — basic write
// ============================================================================

TEST_F(HashChainAuditTest, ConstructionDoesNotThrow) {
    EXPECT_NO_THROW(HashChainAuditWriter local_writer(this->cfg));
}

TEST_F(HashChainAuditTest, WriteCreatesLogFile) {
    HashChainAuditWriter writer(cfg);
    writer.write({{"event", "LOGIN"}, {"user", "alice"}});
    EXPECT_TRUE(fs::exists(kLog));
}

TEST_F(HashChainAuditTest, SequenceNumberIncrements) {
    HashChainAuditWriter writer(cfg);
    EXPECT_EQ(writer.sequenceNumber(), 0u);
    writer.write({{"msg", "first"}});
    EXPECT_EQ(writer.sequenceNumber(), 1u);
    writer.write({{"msg", "second"}});
    EXPECT_EQ(writer.sequenceNumber(), 2u);
}

TEST_F(HashChainAuditTest, HeadHashChangesAfterEachWrite) {
    HashChainAuditWriter writer(cfg);
    std::string h0 = writer.headHash();
    writer.write({{"a", 1}});
    std::string h1 = writer.headHash();
    writer.write({{"b", 2}});
    std::string h2 = writer.headHash();

    EXPECT_NE(h0, h1);
    EXPECT_NE(h1, h2);
}

TEST_F(HashChainAuditTest, ChainSeedChangesGenesisHash) {
    HashChainAuditWriterConfig cfg2 = cfg;
    const auto unique_suffix = std::to_string(reinterpret_cast<std::uintptr_t>(this));
    cfg2.log_path        = "/tmp/test_hc_audit2_" + unique_suffix + ".jsonl";
    cfg2.chain_head_path = "/tmp/test_hc_head2_" + unique_suffix + ".bin";

    {
        HashChainAuditWriter w1(cfg,  "seed-A");
        HashChainAuditWriter w2(cfg2, "seed-B");

        EXPECT_NE(w1.headHash(), w2.headHash());
    }
}

// ============================================================================
// HashChainAuditWriter — persisted head survives restart
// ============================================================================

TEST_F(HashChainAuditTest, HeadPersistedAcrossInstances) {
    std::string first_hash;
    {
        HashChainAuditWriter w(cfg);
        w.write({{"e", "A"}});
        w.write({{"e", "B"}});
        first_hash = w.headHash();
    }
    // Reconstruct — should reload from head file.
    HashChainAuditWriter w2(cfg);
    EXPECT_EQ(w2.headHash(), first_hash);
    EXPECT_EQ(w2.sequenceNumber(), 2u);
}

// ============================================================================
// AuditLogVerifier — intact chain
// ============================================================================

TEST_F(HashChainAuditTest, VerifyCleanChain) {
    HashChainAuditWriter writer(cfg);
    for (int i = 0; i < 5; ++i) {
        writer.write({{"seq", i}, {"event", "AUDIT"}});
    }

    AuditLogVerifier verifier;
    auto result = verifier.verify_chain(kLog);

    EXPECT_TRUE(result.ok);
    EXPECT_EQ(result.entries_ok, 5u);
    EXPECT_EQ(result.entries_total, 5u);
}

TEST_F(HashChainAuditTest, VerifyEmptyLogIsOk) {
    AuditLogVerifier verifier;
    auto result = verifier.verify_chain(kLog); // file does not exist
    EXPECT_TRUE(result.ok);
    EXPECT_EQ(result.entries_total, 0u);
}

// ============================================================================
// AuditLogVerifier — tampered chain
// ============================================================================

TEST_F(HashChainAuditTest, VerifyDetectsTampering) {
    {
        HashChainAuditWriter writer(cfg);
        writer.write({{"event","A"}});
        writer.write({{"event","B"}});
        writer.write({{"event","C"}});
    }

    // Read the log, flip a single byte in the second line, write back.
    {
        std::ifstream ifs(kLog);
        std::vector<std::string> lines;
        std::string line;
        while (std::getline(ifs, line)) {
          lines.push_back(line);
        }
        ifs.close();

        ASSERT_GE(lines.size(), 2u);
        // Corrupt the second record's "event" field value.
        auto& l = lines[1];
        auto pos = l.find('"', l.find("event") + 5);
        if (pos != std::string::npos && pos + 1 < l.size()) {
            l[pos + 1] = (l[pos + 1] == 'B') ? 'X' : 'B';
        }

        std::ofstream ofs(kLog, std::ios::trunc);
        for (const auto& s : lines) {
          ofs << s << '\n';
        }
    }

    AuditLogVerifier verifier;
    auto result = verifier.verify_chain(kLog);

    EXPECT_FALSE(result.ok);
    EXPECT_GT(result.entries_total, 0u);
    EXPECT_FALSE(result.error_message.empty());
}
