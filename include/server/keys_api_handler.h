/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            keys_api_handler.h                                 ║
  Version:         0.0.16                                             ║
  Last Modified:   2026-02-21 17:20:08                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     72                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <memory>
#include <string>
#include <nlohmann/json.hpp>
#include "security/encryption.h"
#include "security/key_provider.h"

namespace themis { namespace server {

/**
 * @brief Keys Admin API Handler
 * 
 * Provides REST endpoints for key management:
 * - GET /api/keys - List all encryption keys
 * - POST /api/keys/{id}/rotate - Rotate specific key
 */
class KeysApiHandler {
public:
    /**
     * @brief Initialize with KeyProvider for actual key operations
     * @param key_provider Shared pointer to KeyProvider implementation
     */
    explicit KeysApiHandler(std::shared_ptr<KeyProvider> key_provider);
    
    KeysApiHandler() = default;

    /**
     * @brief List all managed encryption keys
     * @return JSON response: { "items": [...], "total": N }
     */
    nlohmann::json listKeys();

    /**
     * @brief Rotate a specific encryption key
     * @param key_id ID of key to rotate
     * @param body Optional parameters (e.g., rotation reason)
     * @return JSON response: { "success": true, "key_id": "...", "new_version": N }
     */
    nlohmann::json rotateKey(const std::string& key_id, const nlohmann::json& body);

private:
    std::shared_ptr<KeyProvider> key_provider_;
};

}} // namespace themis::server
