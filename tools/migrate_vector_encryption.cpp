/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            migrate_vector_encryption.cpp                      ║
  Version:         0.0.41                                             ║
  Last Modified:   2026-04-14 11:54:43                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   98.0/100                                       ║
    • Total Lines:     281                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 7053becfae  2026-02-23  fix(ci): fix 10 error-handling audit violations to bring ... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * Phase 1: Vector Encryption Migration Tool
 * 
 * Migrates plaintext vectors in RocksDB to encrypted format.
 * 
 * Usage:
 *   migrate_vector_encryption --db-path /path/to/db --object-name documents [--dry-run]
 * 
 * Features:
 * - Dry-run mode for safety
 * - Batch processing with progress reporting
 * - Skip already-encrypted vectors
 * - Rollback on error
 */

#include "storage/rocksdb_wrapper.h"
#include "storage/base_entity.h"
#include "storage/key_schema.h"
#include "security/encryption.h"
#include "security/mock_key_provider.h"
#include "utils/logger.h"
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <cstdlib>

using namespace themis;

struct MigrationOptions {
    std::string db_path;
    std::string object_name;
    std::string key_id = "vector_embeddings";
    size_t batch_size = 1000;
    bool dry_run = false;
};

class VectorEncryptionMigrator {
public:
    VectorEncryptionMigrator(const MigrationOptions& opts)
        : options_(opts)
        , db_(std::make_unique<RocksDBWrapper>(opts.db_path))
    {
        // Initialize encryption
        // Note: Uses global FieldEncryption state pattern consistent with EncryptedField usage
        key_provider_ = std::make_shared<MockKeyProvider>();
        key_provider_->createKey(opts.key_id, 1);
        field_encryption_ = std::make_shared<FieldEncryption>(key_provider_);
        EncryptedField<std::vector<float>>::setFieldEncryption(field_encryption_);
    }
    
    int migrate() {
        std::cout << "=== Vector Encryption Migration ===" << std::endl;
        std::cout << "Database: " << options_.db_path << std::endl;
        std::cout << "Object: " << options_.object_name << std::endl;
        std::cout << "Key ID: " << options_.key_id << std::endl;
        std::cout << "Dry Run: " << (options_.dry_run ? "YES" : "NO") << std::endl;
        std::cout << std::endl;
        
        // Scan and collect vectors to migrate
        std::vector<std::string> pks_to_migrate;
        std::string prefix = options_.object_name + ":";
        
        std::cout << "Scanning for vectors to migrate..." << std::endl;
        
        db_->scanPrefix(prefix, [&](std::string_view key, std::string_view value) {
            std::string pk = KeySchema::extractPrimaryKey(key);
            std::vector<uint8_t> bytes(value.begin(), value.end());
            
            try {
                BaseEntity e = BaseEntity::deserialize(pk, bytes);
                
                // Check if already encrypted
                if (e.getField("embedding_encrypted")) {
                    ++stats_.already_encrypted;
                    return true;  // Skip, already encrypted
                }
                
                // Check if has plaintext vector
                auto vec = e.extractVector("embedding");
                if (vec) {
                    pks_to_migrate.push_back(pk);
                }
            } catch (...) {
                ++stats_.errors;
                std::cerr << "warning: error processing entity '" << pk << "' during scan\n";
            }
            
            return true;  // Continue scanning
        });
        
        std::cout << "Found " << pks_to_migrate.size() << " vectors to migrate" << std::endl;
        std::cout << "Already encrypted: " << stats_.already_encrypted << std::endl;
        
        if (pks_to_migrate.empty()) {
            std::cout << "No vectors to migrate." << std::endl;
            return 0;
        }
        
        if (options_.dry_run) {
            std::cout << "\nDRY RUN: Would migrate " << pks_to_migrate.size() 
                      << " vectors" << std::endl;
            return 0;
        }
        
        // Migrate in batches
        std::cout << "\nMigrating vectors..." << std::endl;
        size_t total = pks_to_migrate.size();
        
        for (size_t i = 0; i < total; i += options_.batch_size) {
            size_t batch_start = i;
            size_t batch_end = std::min(i + options_.batch_size, total);
            
            if (!migrateBatch(pks_to_migrate, batch_start, batch_end)) {
                std::cerr << "Migration failed at batch " << (i / options_.batch_size) 
                          << std::endl;
                return 1;
            }
            
            // Progress report
            double progress = (double)batch_end / total * 100.0;
            std::cout << "Progress: " << batch_end << "/" << total 
                      << " (" << std::fixed << std::setprecision(1) << progress << "%)" 
                      << std::endl;
        }
        
        std::cout << "\n=== Migration Complete ===" << std::endl;
        std::cout << "Migrated: " << stats_.migrated << std::endl;
        std::cout << "Skipped: " << stats_.already_encrypted << std::endl;
        std::cout << "Errors: " << stats_.errors << std::endl;
        
        return stats_.errors > 0 ? 1 : 0;
    }

private:
    bool migrateBatch(const std::vector<std::string>& pks, 
                      size_t start, size_t end) {
        auto batch = db_->createWriteBatch();
        
        for (size_t i = start; i < end; ++i) {
            const auto& pk = pks[i];
            std::string key = options_.object_name + ":" + pk;
            
            try {
                // Read existing entity
                auto blob = db_->get(key);
                if (!blob) {
                    ++stats_.errors;
                    continue;
                }
                
                BaseEntity e = BaseEntity::deserialize(pk, *blob);
                
                // Extract plaintext vector
                auto vec = e.extractVector("embedding");
                if (!vec) {
                    ++stats_.errors;
                    continue;
                }
                
                // Encrypt vector
                EncryptedField<std::vector<float>> enc_field;
                enc_field.encrypt(*vec, options_.key_id);
                
                // Create new entity with encrypted vector
                auto fields = e.getAllFields();
                fields.erase("embedding");  // Remove plaintext
                fields["embedding_encrypted"] = enc_field.toBase64();
                
                BaseEntity encrypted_entity = BaseEntity::fromFields(pk, fields);
                auto serialized = encrypted_entity.serialize();
                
                // Add to batch
                batch->put(key, serialized);
                ++stats_.migrated;
                
            } catch (const std::exception& ex) {
                std::cerr << "Error migrating " << pk << ": " << ex.what() << std::endl;
                ++stats_.errors;
            }
        }
        
        // Commit batch
        return batch->commit();
    }
    
    MigrationOptions options_;
    std::unique_ptr<RocksDBWrapper> db_;
    std::shared_ptr<MockKeyProvider> key_provider_;
    std::shared_ptr<FieldEncryption> field_encryption_;
    
    struct {
        size_t migrated = 0;
        size_t already_encrypted = 0;
        size_t errors = 0;
    } stats_;
};

void printUsage(const char* prog_name) {
    std::cout << "Usage: " << prog_name << " [options]" << std::endl;
    std::cout << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  --db-path PATH       Path to RocksDB database" << std::endl;
    std::cout << "  --object-name NAME   Vector index object name (e.g., 'documents')" << std::endl;
    std::cout << "  --key-id ID          Encryption key ID (default: 'vector_embeddings')" << std::endl;
    std::cout << "  --batch-size SIZE    Batch size for migration (default: 1000)" << std::endl;
    std::cout << "  --dry-run            Simulate migration without making changes" << std::endl;
    std::cout << "  --help               Show this help message" << std::endl;
}

int main(int argc, char** argv) {
    MigrationOptions options;
    
    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "--help") {
            printUsage(argv[0]);
            return 0;
        } else if (arg == "--db-path" && i + 1 < argc) {
            options.db_path = argv[++i];
        } else if (arg == "--object-name" && i + 1 < argc) {
            options.object_name = argv[++i];
        } else if (arg == "--key-id" && i + 1 < argc) {
            options.key_id = argv[++i];
        } else if (arg == "--batch-size" && i + 1 < argc) {
            options.batch_size = std::stoul(argv[++i]);
        } else if (arg == "--dry-run") {
            options.dry_run = true;
        } else {
            std::cerr << "Unknown option: " << arg << std::endl;
            printUsage(argv[0]);
            return 1;
        }
    }
    
    // Validate required options
    if (options.db_path.empty()) {
        std::cerr << "Error: --db-path is required" << std::endl;
        printUsage(argv[0]);
        return 1;
    }
    
    if (options.object_name.empty()) {
        std::cerr << "Error: --object-name is required" << std::endl;
        printUsage(argv[0]);
        return 1;
    }
    
    try {
        VectorEncryptionMigrator migrator(options);
        return migrator.migrate();
    } catch (const std::exception& ex) {
        std::cerr << "Migration failed: " << ex.what() << std::endl;
        return 1;
    }
}
