#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "updates/delta_update_engine.h"
#include "updates/schema_migration.h"

namespace themis {
namespace updates {
namespace test {

namespace {

bool writeTextFile(const std::filesystem::path& path, const std::string& content) {
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    if (!stream.is_open()) {
        return false;
    }
    stream << content;
    return static_cast<bool>(stream);
}

std::string readTextFile(const std::filesystem::path& path) {
    std::ifstream stream(path, std::ios::binary);
    std::string content((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
    return content;
}

class MemoryMigrationStorage final : public IMigrationStorage {
public:
    bool put(const std::string& key, const std::string& value) override {
        values_[key] = value;
        return true;
    }

    bool get(const std::string& key, std::string& value) override {
        const auto it = values_.find(key);
        if (it == values_.end()) {
            return false;
        }
        value = it->second;
        return true;
    }

    bool remove(const std::string& key) override {
        values_.erase(key);
        return true;
    }

    bool listKeys(std::vector<std::string>& out) override {
        for (const auto& [key, _] : values_) {
            out.push_back(key);
        }
        return true;
    }

    std::unordered_map<std::string, std::string> values_;
};

std::filesystem::path makeTempDir(const std::string& suffix) {
    auto dir = std::filesystem::temp_directory_path() / ("themis_updates_" + suffix);
    std::filesystem::remove_all(dir);
    std::filesystem::create_directories(dir);
    return dir;
}

} // namespace

TEST(DeltaEngineHardeningTest, PatchRoundTripAndCorruptionRejection) {
    const auto root_dir = makeTempDir("delta_roundtrip");
    const auto base_path = root_dir / "base.bin";
    const auto target_path = root_dir / "target.bin";
    const auto patch_path = root_dir / "patch.bin";
    const auto reconstructed_path = root_dir / "reconstructed.bin";
    const auto corrupted_patch_path = root_dir / "corrupted_patch.bin";

    ASSERT_TRUE(writeTextFile(base_path, "alpha beta gamma delta\n"));
    ASSERT_TRUE(writeTextFile(target_path, "alpha beta gamma epsilon\n"));

    DeltaUpdateEngine engine(root_dir.string(), root_dir.string());
    EXPECT_TRUE(engine.generatePatch(base_path.string(), target_path.string(), patch_path.string()));
    EXPECT_TRUE(engine.applyPatch(base_path.string(), patch_path.string(), reconstructed_path.string()));
    EXPECT_EQ(readTextFile(reconstructed_path), readTextFile(target_path));

    std::ifstream patch_in(patch_path, std::ios::binary);
    std::vector<uint8_t> patch_bytes((std::istreambuf_iterator<char>(patch_in)), std::istreambuf_iterator<char>());
    ASSERT_FALSE(patch_bytes.empty());
    patch_bytes[0] ^= 0xFF;
    {
        std::ofstream out(corrupted_patch_path, std::ios::binary | std::ios::trunc);
        out.write(reinterpret_cast<const char*>(patch_bytes.data()), static_cast<std::streamsize>(patch_bytes.size()));
    }

    EXPECT_FALSE(engine.applyPatch(base_path.string(), corrupted_patch_path.string(), (root_dir / "should_not_exist.bin").string()));

    std::filesystem::remove_all(root_dir);
}

TEST(SchemaMigrationHardeningTest, ApplyCustomMigrationAndRollbackNoOp) {
    MemoryMigrationStorage storage;
    SchemaMigration migration("1.5.0");

    migration.setRollbackStrategy(RollbackStrategy::AUTOMATIC);
    migration.addCustomMigration([](MigrationContext& ctx) {
        return ctx.storage != nullptr && ctx.storage->put("users:1", "alice");
    });

    auto result = migration.apply(storage);
    EXPECT_TRUE(result.success) << result.error_message;
    EXPECT_EQ(result.version, "1.5.0");

    std::string stored_value = {};
    EXPECT_TRUE(storage.get("users:1", stored_value));
    EXPECT_EQ(stored_value, "alice");

    auto rollback_result = migration.rollback();
    EXPECT_TRUE(rollback_result.success) << rollback_result.error_message;
}

TEST(SchemaMigrationHardeningTest, AddColumnMigrationApplies) {
    MemoryMigrationStorage storage;
    SchemaMigration migration("1.5.1");

    ColumnDef column;
    column.name = "status";
    column.type = "TEXT";
    column.nullable = false;
    column.default_value = "'active'";

    migration.addColumn("users", column);

    auto result = migration.apply(storage);
    EXPECT_TRUE(result.success) << result.error_message;
    EXPECT_EQ(result.version, "1.5.1");
    EXPECT_FALSE(result.phase_reached == OnlineDDLPhase::IDLE);
}

} // namespace test
} // namespace updates
} // namespace themis
