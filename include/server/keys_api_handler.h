/**
 * @file keys_api_handler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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
