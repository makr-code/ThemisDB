/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            example_pki_lora.cpp                               ║
  Version:         0.0.39                                             ║
  Last Modified:   2026-04-13 20:19:33                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     110                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • a629043ab2  2026-02-22  Audit: document gaps found - benchmarks and stale annotat... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// Example: Using PKI-based encryption for LoRA adapters
// This example demonstrates how to configure and use certificate-based encryption

#include "llm/lora_framework/lora_storage_service.h"
#include "storage/rocksdb_wrapper.h"
#include <iostream>

using namespace themis::llm::lora;

int main() {
    try {
        // Initialize RocksDB for storage
        auto db = std::make_shared<themis::RocksDBWrapper>("./lora_db");
        
        // Configure LoRA storage with PKI encryption
        LoRAStorageService::Config config;
        config.backend = LoRAStorageService::Backend::ThemisDB;
        config.db = db;
        
        // Enable PKI-based encryption
        config.enable_encryption = true;
        config.use_pki_for_encryption = true;
        config.pki_cert_path = "/etc/themis/certs/lora-encryption.crt";
        config.pki_private_key_path = "/etc/themis/keys/lora-encryption.key";
        config.pki_verify_certificate = true;  // Validate certificate expiration
        
        // Initialize storage service (will use PKIKeyProvider internally)
        LoRAStorageService storage(config);
        
        std::cout << "✓ LoRA storage service initialized with PKI encryption\n";
        
        // Create sample adapter
        AdapterWeights weights;
        weights.data = {0x01, 0x02, 0x03, 0x04, 0x05};
        weights.size_bytes = weights.data.size();
        weights.format = "safetensors";
        
        AdapterMetadata metadata;
        metadata.adapter_id = "my_adapter";
        metadata.base_model = "llama-2-7b";
        metadata.rank = 8;
        metadata.alpha = 16.0;
        
        // Save adapter (will be encrypted with PKI-derived key)
        bool saved = storage.saveAdapter("my_adapter", weights, metadata);
        if (saved) {
            std::cout << "✓ Adapter saved with encryption\n";
        }
        
        // Load adapter (will be decrypted automatically)
        auto loaded = storage.loadAdapter("my_adapter");
        if (loaded.has_value()) {
            std::cout << "✓ Adapter loaded and decrypted successfully\n";
            std::cout << "  Size: " << loaded->size_bytes << " bytes\n";
            std::cout << "  Format: " << loaded->format << "\n";
            
            // Verify data integrity
            if (loaded->data == weights.data) {
                std::cout << "✓ Data integrity verified\n";
            }
        }
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        std::cerr << "\nTroubleshooting:\n";
        std::cerr << "1. Ensure certificate files exist and are readable\n";
        std::cerr << "2. Check certificate has not expired: openssl x509 -in cert.crt -noout -dates\n";
        std::cerr << "3. Verify file permissions: chmod 600 key.key && chmod 644 cert.crt\n";
        std::cerr << "4. Generate test certificate: ./scripts/generate_lora_cert.sh\n";
        return 1;
    }
}

// Compile with:
// g++ -std=c++17 example_pki_lora.cpp -o example_pki_lora \
//     -I../include -L../build -lthemis_core -lrocksdb -lcrypto -lssl

// Run with:
// 1. Generate test certificate:
//    ./scripts/generate_lora_cert.sh --dir /tmp/test_certs
//
// 2. Update paths in code to use generated certificate
//
// 3. Run example:
//    ./example_pki_lora
