#include <iostream>
#include <filesystem>
#include "storage/rocksdb_wrapper.h"

using namespace themis;

int main() {
    std::cout << "=== RocksDB Put Debug Test ===" << std::endl;
    
    // Create temp path
    auto temp_dir = std::filesystem::temp_directory_path() / "test_rocksdb_debug";
    std::filesystem::remove_all(temp_dir);
    std::filesystem::create_directories(temp_dir);
    
    std::cout << "Database path: " << temp_dir.string() << std::endl;
    
    // Open database
    RocksDBWrapper::Config cfg;
    cfg.db_path = temp_dir.string();
    cfg.enable_blobdb = false;
    
    RocksDBWrapper db(cfg);
    std::cout << "Calling db.open()..." << std::endl;
    bool open_result = db.open();
    std::cout << "db.open() returned: " << (open_result ? "true" : "false") << std::endl;
    
    if (!open_result) {
        std::cerr << "FAILED to open database!" << std::endl;
        return 1;
    }
    
    // Try to put
    std::cout << "\nTrying db.put('test_key', vector<uint8_t>{1,2,3})..." << std::endl;
    std::vector<uint8_t> value{1, 2, 3};
    bool put_result = db.put("test_key", value);
    std::cout << "db.put() returned: " << (put_result ? "true" : "false") << std::endl;
    
    if (!put_result) {
        std::cerr << "FAILED to put!" << std::endl;
        return 1;
    }
    
    // Try to get it back
    std::cout << "\nTrying db.get('test_key')..." << std::endl;
    std::optional<std::vector<uint8_t>> got = db.get("test_key");
    if (got) {
        std::cout << "SUCCESS: Retrieved value with size: " << got->size() << std::endl;
    } else {
        std::cerr << "FAILED to get value!" << std::endl;
        return 1;
    }
    
    std::cout << "\n=== ALL TESTS PASSED ===" << std::endl;
    
    // Cleanup
    std::filesystem::remove_all(temp_dir);
    return 0;
}
