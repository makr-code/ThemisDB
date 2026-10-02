#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <string>

#include "validator.h"

namespace fs = std::filesystem;

namespace themis::tools::tests {

TEST(ValidatorTests, RejectMissingDatabaseDirectory) {
    const auto missing_dir = fs::temp_directory_path() / "themis_validator_missing";
    fs::remove_all(missing_dir);

    EXPECT_FALSE(Validator::validate(missing_dir.string()));
}

TEST(ValidatorTests, RejectDatabaseWithoutCurrentManifest) {
    const auto db_path = fs::temp_directory_path() / "themis_validator_no_current";
    fs::create_directories(db_path);
    std::ofstream manifest(db_path / "MANIFEST");
    manifest << "MANIFEST-000001\n";
    manifest.close();

    EXPECT_FALSE(Validator::validate(db_path.string()));
    fs::remove_all(db_path);
}

TEST(ValidatorTests, AcceptValidRocksDbDirectory) {
    const auto db_path = fs::temp_directory_path() / "themis_validator_valid";
    fs::create_directories(db_path);

    std::ofstream current(db_path / "CURRENT");
    current << "MANIFEST-000001\n";
    current.close();

    std::ofstream manifest(db_path / "MANIFEST-000001");
    manifest << "rocksdb version\n";
    manifest.close();

    EXPECT_TRUE(Validator::validate(db_path.string()));
    fs::remove_all(db_path);
}

} // namespace themis::tools::tests
