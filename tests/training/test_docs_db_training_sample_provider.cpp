#include "training/docs_db_training_sample_provider.h"
#include "utils/rocksdb_open_compat.h"

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include <rocksdb/db.h>
#include <rocksdb/options.h>

#include <chrono>
#include <filesystem>
#include <memory>
#include <string>
#include <system_error>

namespace {

class TemporaryDocsDatabase {
public:
    TemporaryDocsDatabase() {
        const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
        path_ = std::filesystem::temp_directory_path() /
                ("themis_docs_training_provider_" + std::to_string(nonce));
        std::filesystem::create_directories(path_);
    }

    ~TemporaryDocsDatabase() {
        std::error_code error;
        std::filesystem::remove_all(path_, error);
    }

    bool put(const std::string& key, const std::string& value) const {
        rocksdb::Options options;
        options.create_if_missing = true;
        rocksdb::DB* raw_db = nullptr;
        const auto status = themis::storage::detail::openDbCompat(options, path_.string(), &raw_db);
        if (!status.ok() || raw_db == nullptr) {
            return false;
        }
        std::unique_ptr<rocksdb::DB> db(raw_db);
        return db->Put(rocksdb::WriteOptions{}, key, value).ok();
    }

    [[nodiscard]] std::string path() const { return path_.string(); }

private:
    std::filesystem::path path_;
};

const std::string kLongDocumentationText =
    "ThemisDB stores documentation records in RocksDB. The documentation index contains "
    "complete chunks with stable document identifiers. A bounded training provider reads "
    "these chunks without changing the source database. TinyLlama can learn the expected "
    "continuation while the frozen base model remains unchanged.";

TEST(DocsDbTrainingSampleProviderTest, PrefersFullChunksAndHonorsSampleBound) {
    TemporaryDocsDatabase database;
    ASSERT_TRUE(database.put("doc_0", nlohmann::json{
        {"file_path", "docs/storage.md"},
        {"themis_metadata", {{"vector", {{"text_content", "preview only"}}}}}
    }.dump()));
    ASSERT_TRUE(database.put("chunk_0", nlohmann::json{
        {"doc_id", "doc-storage"}, {"text", kLongDocumentationText}
    }.dump()));
    ASSERT_TRUE(database.put("chunk_1", nlohmann::json{
        {"doc_id", "doc-query"}, {"text", kLongDocumentationText}
    }.dump()));

    themis::training::DocsDbTrainingSampleProvider provider({database.path()});
    provider.initialize(/*max_samples=*/1, /*batch_size=*/1);
    ASSERT_TRUE(provider.hasNext());
    const auto batch = provider.nextBatch();

    ASSERT_EQ(batch.size(), 1u);
    EXPECT_EQ(batch.front().sample_id, "chunk_0");
    EXPECT_EQ(batch.front().source_uri, "doc-storage");
    EXPECT_EQ(provider.sourceName(), "docs.db");
    EXPECT_FALSE(provider.hasNext());
}

TEST(DocsDbTrainingSampleProviderTest, FallsBackToLegacyDocumentPreview) {
    TemporaryDocsDatabase database;
    ASSERT_TRUE(database.put("doc_0", nlohmann::json{
        {"file_path", "docs/legacy.md"},
        {"themis_metadata", {{"vector", {{"text_content", kLongDocumentationText}}}}}
    }.dump()));

    themis::training::DocsDbTrainingSampleProvider provider({database.path()});
    provider.initialize(/*max_samples=*/4, /*batch_size=*/2);
    const auto batch = provider.nextBatch();

    ASSERT_EQ(batch.size(), 1u);
    EXPECT_EQ(batch.front().source_uri, "docs/legacy.md");
    EXPECT_FALSE(batch.front().input.empty());
    EXPECT_FALSE(batch.front().output.empty());
}

TEST(DocsDbTrainingSampleProviderTest, RejectsMissingDatabaseAndMalformedRecords) {
    const auto missing = std::filesystem::temp_directory_path() / "themis_missing_docs_db";
    themis::training::DocsDbTrainingSampleProvider missing_provider({missing.string()});
    EXPECT_THROW(missing_provider.initialize(1, 1), std::invalid_argument);

    TemporaryDocsDatabase database;
    ASSERT_TRUE(database.put("chunk_0", "not-json"));
    themis::training::DocsDbTrainingSampleProvider malformed_provider({database.path()});
    malformed_provider.initialize(1, 1);
    EXPECT_THROW(malformed_provider.nextBatch(), std::runtime_error);
}

} // namespace