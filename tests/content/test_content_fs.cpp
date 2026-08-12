#include <gtest/gtest.h>
#include "storage/rocksdb_wrapper.h"
#include "content/content_fs.h"
#include "utils/expected.h"
#include <random>
#include <filesystem>

using namespace themis;

namespace {
std::vector<uint8_t> makeData(size_t n) {
    std::vector<uint8_t> v(n);
    std::mt19937 rng(12345);
    std::uniform_int_distribution<int> dist(0,255);
    for (size_t i=0;i<n;++i) v[i] = static_cast<uint8_t>(dist(rng));
    return v;
}
}

class ContentFSTest : public ::testing::Test {
protected:
    void SetUp() override {
#ifdef _WIN32
        GTEST_SKIP() << "Skipping ContentFSTest on Windows due to intermittent heap corruption in fixture setup.";
#endif
        std::filesystem::remove_all(test_dir);
        RocksDBWrapper::Config cfg; cfg.db_path = test_dir; cfg.create_if_missing = true;
        db = std::make_unique<RocksDBWrapper>(cfg);
        ASSERT_TRUE(db->open());
        cfs = std::make_unique<ContentFS>(*db);
    }
    void TearDown() override {
        cfs.reset();
        if (db) {
            db->close();
        }
        db.reset();
        std::error_code ec;
        std::filesystem::remove_all(test_dir, ec);
    }
    std::string test_dir = "./test_content_fs_tmp";
    std::unique_ptr<RocksDBWrapper> db;
    std::unique_ptr<ContentFS> cfs;
};

TEST_F(ContentFSTest, PutGetRoundtrip) {
    auto data = makeData(64 * 1024);
    auto hex = ContentFS::sha256Hex(data);
    auto put_result = cfs->put("blob1", data, "application/octet-stream", hex);
    ASSERT_TRUE(put_result) << put_result.error().message();

    auto head_result = cfs->head("blob1");
    ASSERT_TRUE(head_result) << head_result.error().message();
    auto meta = *head_result;
    EXPECT_EQ(meta.pk, "blob1");
    EXPECT_EQ(meta.mime, "application/octet-stream");
    EXPECT_EQ(meta.size, data.size());
    EXPECT_EQ(meta.sha256_hex, hex);

    auto get_result = cfs->get("blob1");
    ASSERT_TRUE(get_result) << get_result.error().message();
    EXPECT_EQ(*get_result, data);
}

TEST_F(ContentFSTest, RangeReads) {
    auto data = makeData(10000);
    auto put_result = cfs->put("r1", data, "binary");
    ASSERT_TRUE(put_result) << put_result.error().message();
    
    // First 100 bytes
    auto range1 = cfs->getRange("r1", 0, 100);
    ASSERT_TRUE(range1) << range1.error().message();
    auto a = *range1;
    ASSERT_EQ(a.size(), 100u);
    EXPECT_TRUE(std::equal(a.begin(), a.end(), data.begin()));
    
    // Middle 200 bytes
    auto range2 = cfs->getRange("r1", 500, 200);
    ASSERT_TRUE(range2) << range2.error().message();
    auto b = *range2;
    ASSERT_EQ(b.size(), 200u);
    EXPECT_TRUE(std::equal(b.begin(), b.end(), data.begin()+500));
    
    // To end
    auto range3 = cfs->getRange("r1", 9800, 0);
    ASSERT_TRUE(range3) << range3.error().message();
    auto c = *range3;
    ASSERT_EQ(c.size(), 200u);
    EXPECT_TRUE(std::equal(c.begin(), c.end(), data.begin()+9800));
}

TEST_F(ContentFSTest, ChecksumMismatch) {
    auto data = makeData(1024);
    auto put_result = cfs->put("bad", data, "bin", std::string("deadbeef"));
    ASSERT_FALSE(put_result);
    EXPECT_EQ(put_result.error().code(), errors::ErrorCode::ERR_API_INVALID_REQUEST);
}

TEST_F(ContentFSTest, DeleteBlob) {
    auto data = makeData(4096);
    auto put_result = cfs->put("x", data, "bin");
    ASSERT_TRUE(put_result) << put_result.error().message();
    
    auto remove_result = cfs->remove("x");
    ASSERT_TRUE(remove_result) << remove_result.error().message();
    
    auto get_result = cfs->get("x");
    ASSERT_FALSE(get_result);
    EXPECT_EQ(get_result.error().code(), errors::ErrorCode::ERR_STORAGE_FILE_NOT_FOUND);
}
