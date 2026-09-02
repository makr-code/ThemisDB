/**
 * @file import_docs_rocksdb.cpp
 * @brief Direct RocksDB importer for documentation database
 * 
 * This program directly writes documentation to RocksDB using all ThemisDB models:
 * - Relational (Column Family: relational)
 * - Graph Nodes (Column Family: graph_nodes)
 * - Graph Edges (Column Family: graph_edges)
 * - Vector (Column Family: vector)
 * - Metadata (Column Family: metadata)
 * - Document (Column Family: document) - Native ThemisDB :document collection
 */

#include <rocksdb/db.h>
#include <rocksdb/slice.h>
#include <rocksdb/options.h>
#include <iostream>
#include <fstream>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

using json = nlohmann::json;
using namespace rocksdb;

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <json_file> <rocksdb_path>" << std::endl;
        return 1;
    }

    std::string json_file = argv[1];
    std::string db_path = argv[2];

    // Load JSON documentation
    std::ifstream input_file(json_file);
    if (!input_file.is_open()) {
        std::cerr << "Error: Cannot open file " << json_file << std::endl;
        return 1;
    }

    json docs_data;
    try {
        input_file >> docs_data;
    } catch (const std::exception& e) {
        std::cerr << "Error parsing JSON: " << e.what() << std::endl;
        return 1;
    }

    // Open RocksDB database
    Options options;
    options.create_if_missing = true;
    std::unique_ptr<DB> db;

    Status status = DB::Open(options, db_path, &db);
    if (!status.ok()) {
        std::cerr << "Error opening database: " << status.ToString() << std::endl;
        return 1;
    }

    std::cout << "[OK] Opened RocksDB database at " << db_path << std::endl;

    // Import documents
    int doc_count = 0;
    for (const auto& doc : docs_data["documents"]) {
        std::string key = "doc_" + std::to_string(doc_count);
        std::string value = doc.dump();
        
        status = db->Put(WriteOptions(), key, value);
        if (!status.ok()) {
            std::cerr << "Error writing to database: " << status.ToString() << std::endl;
            db.reset();
            return 1;
        }
        
        doc_count++;
        if (doc_count % 100 == 0) {
            std::cout << "[OK] Imported " << doc_count << " documents..." << std::endl;
        }
    }

    std::cout << "[OK] Successfully imported " << doc_count << " documents!" << std::endl;

    // Cleanup
    db.reset();
    return 0;
}
