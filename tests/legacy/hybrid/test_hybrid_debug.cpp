#include <gtest/gtest.h>
#include "query/query_engine.h"
#include "index/vector_index.h"
#include "index/spatial_index.h"
#include "storage/rocksdb_wrapper.h"
#include "storage/base_entity.h"
#include <nlohmann/json.hpp>

using namespace themis;

TEST(HybridDebugTest, CheckDataPresence) {
    std::filesystem::remove_all("data/debug_test");
    
    RocksDBWrapper::Config cfg;
    cfg.db_path = "data/debug_test";
    RocksDBWrapper db(cfg);
    ASSERT_TRUE(db.open());
    
    // Insert test entity
    BaseEntity img1("img1");
    img1.setField("name", std::string("Berlin Tower"));
    img1.setField("embedding", std::vector<float>{0.1f, 0.2f, 0.3f});
    img1.setField("location", std::string(R"({"type":"Point","coordinates":[13.405,52.52]})"));
    auto blob = img1.serialize();
    db.put("images:img1", blob);
    
    // Check if data is there
    auto val = db.get("images:img1");
    ASSERT_TRUE(val.has_value());
    
    // Parse it back
    auto entity = BaseEntity::deserialize("img1", *val);
    auto loc = entity.extractField("location");
    ASSERT_TRUE(loc.has_value());
    std::cout << "Location field: " << *loc << std::endl;
    
    // Check JSON parsing
    try {
        auto loc_json = nlohmann::json::parse(*loc);
        std::cout << "Location JSON: " << loc_json.dump() << std::endl;
        ASSERT_TRUE(loc_json.contains("type"));
        ASSERT_EQ(loc_json["type"], "Point");
        ASSERT_TRUE(loc_json.contains("coordinates"));
        auto coords = loc_json["coordinates"].get<std::vector<double>>();
        ASSERT_EQ(coords.size(), 2);
        std::cout << "Coordinates: [" << coords[0] << ", " << coords[1] << "]" << std::endl;
    } catch (const std::exception& e) {
        FAIL() << "JSON parsing failed: " << e.what();
    }
    
    std::filesystem::remove_all("data/debug_test");
}
