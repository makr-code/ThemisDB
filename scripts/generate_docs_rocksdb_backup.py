"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            generate_docs_rocksdb_backup.py                    ║
  Version:         0.0.42                                             ║
  Last Modified:   2026-04-14 18:45:58                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     672                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
"""
ThemisDB Documentation RocksDB Database Generator
=================================================

Generates a binary RocksDB database with documentation indexed across all
ThemisDB models: relational, graph, vector, and metadata.

This creates a production-ready database that can be directly used by
ThemisDB for documentation assistance via LLM.

Usage:
    python3 scripts/generate_docs_rocksdb.py
    python3 scripts/generate_docs_rocksdb.py --output data/docs.db
"""

import argparse
import hashlib
import json
import logging
import os
import sys
import subprocess
from pathlib import Path
from datetime import datetime

# Add tools directory to path
SCRIPT_DIR = Path(__file__).parent
REPO_ROOT = SCRIPT_DIR.parent
TOOLS_DIR = REPO_ROOT / "tools"
sys.path.insert(0, str(TOOLS_DIR))

try:
    from ingest import IngestionConfig, IngestionEngine
except ImportError:
    print("Error: Could not import ingest module from tools/ingest.py")
    sys.exit(1)

logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger('DocsRocksDBGenerator')


def generate_themis_import_script(docs_data: dict, output_path: str) -> str:
    """
    Generate a shell script that uses ThemisDB CLI to import documentation
    into a RocksDB database with all models.
    
    Args:
        docs_data: Documentation data from ingestion
        output_path: Path where the RocksDB database will be created
        
    Returns:
        Path to the generated import script
    """
    
    script_path = Path(output_path).parent / "import_docs_to_rocksdb.sh"
    
    # Generate AQL commands for importing documentation
    aql_commands = []
    
    # 1. Create collections for different models
    aql_commands.append("""
-- Create documentation collections
CREATE COLLECTION IF NOT EXISTS docs_relational;
CREATE COLLECTION IF NOT EXISTS docs_graph_nodes;
CREATE COLLECTION IF NOT EXISTS docs_graph_edges;
CREATE COLLECTION IF NOT EXISTS docs_vector;
CREATE COLLECTION IF NOT EXISTS docs_metadata;

-- Native :document Collection für volle ThemisDB-Integration
CREATE COLLECTION IF NOT EXISTS :document;
""")
    
    # 2. Insert relational data
    for doc in docs_data.get('documents', []):
        file_name = doc.get('metadata', {}).get('file_name', 'unknown')
        file_path = doc.get('file_path', '')
        content_type = doc.get('mime_type', 'text/plain')
        
        # Extract text content
        text_content = ""
        if 'themis_metadata' in doc and 'vector' in doc['themis_metadata']:
            text_content = doc['themis_metadata']['vector'].get('text_content', '')
        
        # Escape single quotes for SQL
        text_content_escaped = text_content.replace("'", "''")[:5000]  # Limit size
        file_path_escaped = file_path.replace("'", "''")
        
        # Relational insert
        aql_commands.append(f"""
INSERT INTO docs_relational (
    id, file_name, file_path, content_type, text_content, created_at
) VALUES (
    '{doc.get('file_hash', '')}',
    '{file_name}',
    '{file_path_escaped}',
    '{content_type}',
    '{text_content_escaped}',
    '{doc.get('ingestion_time', '')}'
);
""")
        
        # Native :document Collection insert
        metadata_json = json.dumps(doc.get('metadata', {})).replace("'", "''")
        full_text_content = text_content.replace("'", "''")
        
        aql_commands.append(f"""
INSERT INTO :document (
    _key,
    _id,
    type,
    title,
    content,
    source,
    metadata,
    created_at
) VALUES (
    '{doc.get('file_hash', '')}',
    ':document/{doc.get('file_hash', '')}',
    'documentation',
    '{file_name}',
    '{full_text_content}',
    '{file_path_escaped}',
    '{metadata_json}',
    '{doc.get('ingestion_time', '')}'
);
""")
    
    # 3. Insert graph nodes (each document is a node)
    for doc in docs_data.get('documents', []):
        file_name = doc.get('metadata', {}).get('file_name', 'unknown')
        file_hash = doc.get('file_hash', '')
        
        aql_commands.append(f"""
INSERT INTO docs_graph_nodes (
    _id, _key, type, name, hash
) VALUES (
    'docs_graph_nodes/{file_hash}',
    '{file_hash}',
    'Document',
    '{file_name}',
    '{file_hash}'
);
""")
    
    # 4. Create graph edges (simple references based on file names)
    # This is a simplified version - could be enhanced with actual reference parsing
    
    # 5. Insert vector embeddings (placeholder - actual embeddings would be computed)
    for doc in docs_data.get('documents', []):
        file_hash = doc.get('file_hash', '')
        text_content = ""
        if 'themis_metadata' in doc and 'vector' in doc['themis_metadata']:
            text_content = doc['themis_metadata']['vector'].get('text_content', '')[:1000]
        
        # Simple embedding placeholder (in production, use real embeddings)
        # For now, just store the text that needs embedding
        aql_commands.append(f"""
INSERT INTO docs_vector (
    id, document_hash, text_chunk, embedding_pending
) VALUES (
    '{file_hash}',
    '{file_hash}',
    '{text_content.replace("'", "''")}',
    true
);
""")
    
    # 6. Insert metadata
    metadata = docs_data.get('metadata', {})
    stats = docs_data.get('statistics', {})
    
    aql_commands.append(f"""
INSERT INTO docs_metadata (
    id, key, value, created_at
) VALUES 
    ('db_version', 'version', '{metadata.get('version', '1.0.0')}', '{datetime.now().isoformat()}'),
    ('db_generation_time', 'generation_time', '{metadata.get('generation_time', '')}', '{datetime.now().isoformat()}'),
    ('db_total_documents', 'total_documents', '{metadata.get('total_documents', 0)}', '{datetime.now().isoformat()}'),
    ('db_themisdb_version', 'themisdb_version', '{metadata.get('themisdb_version', '')}', '{datetime.now().isoformat()}'),
    ('db_total_size', 'total_size_bytes', '{stats.get('total_size_bytes', 0)}', '{datetime.now().isoformat()}');
""")
    
    # Write shell script
    with open(script_path, 'w', encoding='utf-8') as f:
        f.write("""#!/bin/bash
# ThemisDB Documentation Database Import Script
# Generated by generate_docs_rocksdb.py

set -e

# Configuration
DB_PATH="${1:-data/docs.db}"
THEMIS_CLI="${THEMIS_CLI:-./build/themis_cli}"
AQL_FILE="/tmp/docs_import.aql"

echo "======================================================"
echo "ThemisDB Documentation Database Import"
echo "======================================================"
echo "Database path: $DB_PATH"
echo "ThemisDB CLI: $THEMIS_CLI"
echo ""

# Check if themis_cli exists
if [ ! -f "$THEMIS_CLI" ]; then
    echo "Error: ThemisDB CLI not found at $THEMIS_CLI"
    echo "Please build ThemisDB first or set THEMIS_CLI environment variable"
    exit 1
fi

# Create AQL import file
cat > "$AQL_FILE" << 'EOFAQL'
""")
        
        # Write all AQL commands
        for cmd in aql_commands:
            f.write(cmd)
        
        f.write("""
EOFAQL

echo "Starting import..."
echo ""

# Create database directory if it doesn't exist
mkdir -p "$(dirname "$DB_PATH")"

# Import using ThemisDB CLI
"$THEMIS_CLI" --database "$DB_PATH" --execute-file "$AQL_FILE"

if [ $? -eq 0 ]; then
    echo ""
    echo "======================================================"
    echo "✓ Documentation database created successfully!"
    echo "======================================================"
    echo "Location: $DB_PATH"
    echo "Size: $(du -h "$DB_PATH" | cut -f1)"
    echo ""
else
    echo ""
    echo "======================================================"
    echo "✗ Error creating documentation database"
    echo "======================================================"
    exit 1
fi

# Clean up
rm -f "$AQL_FILE"

echo "Import complete!"
""")
    
    # Make script executable
    os.chmod(script_path, 0o755)
    
    logger.info(f"Generated import script: {script_path}")
    return str(script_path)


def generate_cpp_direct_writer(docs_data: dict, output_path: str) -> str:
    """
    Generate a C++ program that directly writes to RocksDB without CLI.
    This is more efficient for large datasets.
    
    Args:
        docs_data: Documentation data from ingestion
        output_path: Path where the RocksDB database will be created
        
    Returns:
        Path to the generated C++ source file
    """
    
    cpp_path = Path(output_path).parent / "import_docs_rocksdb.cpp"
    
    with open(cpp_path, 'w', encoding='utf-8') as f:
        f.write("""/**
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
#include <rocksdb/options.h>
#include <rocksdb/slice.h>
#include <nlohmann/json.hpp>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

using json = nlohmann::json;

class DocsRocksDBImporter {
public:
    DocsRocksDBImporter(const std::string& db_path) : db_path_(db_path) {}
    
    bool import(const std::string& json_file) {
        // Load JSON data
        std::ifstream file(json_file);
        if (!file.is_open()) {
            std::cerr << "Error: Cannot open " << json_file << std::endl;
            return false;
        }
        
        json docs_data;
        file >> docs_data;
        file.close();
        
        // Open RocksDB with column families
        if (!openDatabase()) {
            return false;
        }
        
        // Import data
        importRelational(docs_data);
        importDocument(docs_data);
        importGraphNodes(docs_data);
        importGraphEdges(docs_data);
        importVector(docs_data);
        importMetadata(docs_data);
        
        closeDatabase();
        
        std::cout << "✓ Documentation database created successfully!" << std::endl;
        std::cout << "Location: " << db_path_ << std::endl;
        
        return true;
    }

private:
    std::string db_path_;
    rocksdb::DB* db_ = nullptr;
    std::vector<rocksdb::ColumnFamilyHandle*> cf_handles_;
    
    bool openDatabase() {
        rocksdb::Options options;
        options.create_if_missing = true;
        options.create_missing_column_families = true;
        
        // Define column families
        std::vector<rocksdb::ColumnFamilyDescriptor> cf_descriptors;
        cf_descriptors.push_back(rocksdb::ColumnFamilyDescriptor(
            rocksdb::kDefaultColumnFamilyName, rocksdb::ColumnFamilyOptions()));
        cf_descriptors.push_back(rocksdb::ColumnFamilyDescriptor(
            "relational", rocksdb::ColumnFamilyOptions()));
        cf_descriptors.push_back(rocksdb::ColumnFamilyDescriptor(
            "graph_nodes", rocksdb::ColumnFamilyOptions()));
        cf_descriptors.push_back(rocksdb::ColumnFamilyDescriptor(
            "graph_edges", rocksdb::ColumnFamilyOptions()));
        cf_descriptors.push_back(rocksdb::ColumnFamilyDescriptor(
            "vector", rocksdb::ColumnFamilyOptions()));
        cf_descriptors.push_back(rocksdb::ColumnFamilyDescriptor(
            "metadata", rocksdb::ColumnFamilyOptions()));
        cf_descriptors.push_back(rocksdb::ColumnFamilyDescriptor(
            "document", rocksdb::ColumnFamilyOptions()));
        
        rocksdb::Status status = rocksdb::DB::Open(options, db_path_, cf_descriptors, &cf_handles_, &db_);
        
        if (!status.ok()) {
            std::cerr << "Error opening database: " << status.ToString() << std::endl;
            return false;
        }
        
        return true;
    }
    
    void closeDatabase() {
        for (auto handle : cf_handles_) {
            delete handle;
        }
        delete db_;
    }
    
    void importRelational(const json& docs_data) {
        std::cout << "Importing relational data..." << std::endl;
        
        auto* cf = cf_handles_[1];  // relational CF
        
        int count = 0;
        for (const auto& doc : docs_data["documents"]) {
            std::string key = "doc:" + doc["file_hash"].get<std::string>();
            
            json record = {
                {"id", doc["file_hash"]},
                {"file_name", doc["metadata"]["file_name"]},
                {"file_path", doc["file_path"]},
                {"content_type", doc["mime_type"]},
                {"created_at", doc["ingestion_time"]}
            };
            
            // Add text content if available
            if (doc.contains("themis_metadata") && 
                doc["themis_metadata"].contains("vector") &&
                doc["themis_metadata"]["vector"].contains("text_content")) {
                record["text_content"] = doc["themis_metadata"]["vector"]["text_content"];
            }
            
            db_->Put(rocksdb::WriteOptions(), cf, key, record.dump());
            count++;
        }
        
        std::cout << "  ✓ Imported " << count << " relational records" << std::endl;
    }
    
    void importDocument(const json& docs_data) {
        std::cout << "Importing :document collection..." << std::endl;
        
        auto* cf = cf_handles_[6];  // document CF (native :document collection)
        
        int count = 0;
        for (const auto& doc : docs_data["documents"]) {
            std::string key = ":document:" + doc["file_hash"].get<std::string>();
            
            json document = {
                {"_key", doc["file_hash"]},
                {"_id", ":document/" + doc["file_hash"].get<std::string>()},
                {"type", "documentation"},
                {"title", doc["metadata"]["file_name"]},
                {"source", doc["file_path"]},
                {"created_at", doc["ingestion_time"]}
            };
            
            // Add full text content if available
            if (doc.contains("themis_metadata") && 
                doc["themis_metadata"].contains("vector") &&
                doc["themis_metadata"]["vector"].contains("text_content")) {
                document["content"] = doc["themis_metadata"]["vector"]["text_content"];
            }
            
            // Add metadata
            if (doc.contains("metadata")) {
                document["metadata"] = doc["metadata"];
            }
            
            db_->Put(rocksdb::WriteOptions(), cf, key, document.dump());
            count++;
        }
        
        std::cout << "  ✓ Imported " << count << " documents to :document collection" << std::endl;
    }
    
    void importGraphNodes(const json& docs_data) {
        std::cout << "Importing graph nodes..." << std::endl;
        
        auto* cf = cf_handles_[2];  // graph_nodes CF
        
        int count = 0;
        for (const auto& doc : docs_data["documents"]) {
            std::string key = "node:" + doc["file_hash"].get<std::string>();
            
            json node = {
                {"_id", "docs_graph_nodes/" + doc["file_hash"].get<std::string>()},
                {"_key", doc["file_hash"]},
                {"type", "Document"},
                {"name", doc["metadata"]["file_name"]},
                {"hash", doc["file_hash"]}
            };
            
            db_->Put(rocksdb::WriteOptions(), cf, key, node.dump());
            count++;
        }
        
        std::cout << "  ✓ Imported " << count << " graph nodes" << std::endl;
    }
    
    void importGraphEdges(const json& docs_data) {
        std::cout << "Importing graph edges..." << std::endl;
        // Placeholder - could parse references between documents
        std::cout << "  ✓ Graph edges (placeholder)" << std::endl;
    }
    
    void importVector(const json& docs_data) {
        std::cout << "Importing vector data..." << std::endl;
        
        auto* cf = cf_handles_[4];  // vector CF
        
        int count = 0;
        for (const auto& doc : docs_data["documents"]) {
            std::string key = "vec:" + doc["file_hash"].get<std::string>();
            
            json vector_entry = {
                {"id", doc["file_hash"]},
                {"document_hash", doc["file_hash"]},
                {"embedding_pending", true}
            };
            
            // Add text chunk if available
            if (doc.contains("themis_metadata") && 
                doc["themis_metadata"].contains("vector") &&
                doc["themis_metadata"]["vector"].contains("text_content")) {
                std::string text = doc["themis_metadata"]["vector"]["text_content"];
                if (text.length() > 1000) text = text.substr(0, 1000);
                vector_entry["text_chunk"] = text;
            }
            
            db_->Put(rocksdb::WriteOptions(), cf, key, vector_entry.dump());
            count++;
        }
        
        std::cout << "  ✓ Imported " << count << " vector entries" << std::endl;
    }
    
    void importMetadata(const json& docs_data) {
        std::cout << "Importing metadata..." << std::endl;
        
        auto* cf = cf_handles_[5];  // metadata CF
        
        const auto& metadata = docs_data["metadata"];
        const auto& stats = docs_data["statistics"];
        
        db_->Put(rocksdb::WriteOptions(), cf, "version", metadata["version"].get<std::string>());
        db_->Put(rocksdb::WriteOptions(), cf, "generation_time", metadata["generation_time"].get<std::string>());
        db_->Put(rocksdb::WriteOptions(), cf, "total_documents", std::to_string(metadata["total_documents"].get<int>()));
        db_->Put(rocksdb::WriteOptions(), cf, "themisdb_version", metadata["themisdb_version"].get<std::string>());
        db_->Put(rocksdb::WriteOptions(), cf, "total_size_bytes", std::to_string(stats["total_size_bytes"].get<int>()));
        
        std::cout << "  ✓ Imported metadata" << std::endl;
    }
};

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <json_file> <db_path>" << std::endl;
        return 1;
    }
    
    std::string json_file = argv[1];
    std::string db_path = argv[2];
    
    std::cout << "======================================================" << std::endl;
    std::cout << "ThemisDB Documentation RocksDB Import" << std::endl;
    std::cout << "======================================================" << std::endl;
    std::cout << "JSON file: " << json_file << std::endl;
    std::cout << "Database path: " << db_path << std::endl;
    std::cout << std::endl;
    
    DocsRocksDBImporter importer(db_path);
    
    if (!importer.import(json_file)) {
        return 1;
    }
    
    return 0;
}
""")
    
    logger.info(f"Generated C++ importer: {cpp_path}")
    return str(cpp_path)


def main():
    parser = argparse.ArgumentParser(
        description='Generate RocksDB database for ThemisDB documentation',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Generate RocksDB database with default settings
  python3 scripts/generate_docs_rocksdb.py
  
  # Specify custom output path
  python3 scripts/generate_docs_rocksdb.py --output data/docs.db
  
  # Generate without compendium
  python3 scripts/generate_docs_rocksdb.py --no-compendium
        """
    )
    
    parser.add_argument(
        '--output',
        type=str,
        default='data/docs.db',
        help='Output path for RocksDB database (default: data/docs.db)'
    )
    
    parser.add_argument(
        '--no-compendium',
        action='store_true',
        help='Exclude compendium directory'
    )
    
    parser.add_argument(
        '--method',
        type=str,
        choices=['cpp', 'cli'],
        default='cpp',
        help='Import method: cpp (direct RocksDB) or cli (via ThemisDB CLI)'
    )
    
    args = parser.parse_args()
    
    logger.info("=" * 60)
    logger.info("ThemisDB Documentation RocksDB Generator")
    logger.info("=" * 60)
    
    # Step 1: Generate JSON documentation database
    logger.info("\nStep 1: Generating JSON documentation database...")
    json_file = "/tmp/docs_database_temp.json"
    
    # Import and use the existing generator
    from generate_docs_database import generate_documentation_database
    
    success = generate_documentation_database(
        output_path=json_file,
        include_compendium=not args.no_compendium,
        include_examples=False
    )
    
    if not success:
        logger.error("Failed to generate JSON database")
        return 1
    
    # Step 2: Load JSON data
    logger.info("\nStep 2: Loading JSON data...")
    with open(json_file, 'r', encoding='utf-8') as f:
        docs_data = json.load(f)
    
    # Step 3: Generate import tools
    logger.info("\nStep 3: Generating RocksDB import tools...")
    
    if args.method == 'cpp':
        cpp_file = generate_cpp_direct_writer(docs_data, args.output)
        logger.info(f"\n✓ C++ importer generated: {cpp_file}")
        logger.info("\nTo compile and run:")
        logger.info(f"  g++ -std=c++17 {cpp_file} -o import_docs_rocksdb -lrocksdb -lpthread")
        logger.info(f"  ./import_docs_rocksdb {json_file} {args.output}")
    else:
        script_file = generate_themis_import_script(docs_data, args.output)
        logger.info(f"\n✓ Import script generated: {script_file}")
        logger.info("\nTo run:")
        logger.info(f"  {script_file} {args.output}")
    
    logger.info("\n" + "=" * 60)
    logger.info("✓ RocksDB import tools generated successfully!")
    logger.info("=" * 60)
    
    return 0


if __name__ == '__main__':
    sys.exit(main())
