/**
 * @file keys_api_handler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

class KeysApiHandler {
public:
    /**
     * @brief Keys Api Handler.
     * @param[in] key_provider Input parameter.
     * @return Return value.
     */
    explicit KeysApiHandler(std::shared_ptr<KeyProvider> key_provider);
    
    KeysApiHandler() = default;

    /**
     * @brief List Keys.
     * @return Return value.
     */
    nlohmann::json listKeys();

    /**
     * @brief Rotate Key.
     * @param[in] key_id Identifier of the key.
     * @param[in] body Input parameter.
     * @return Return value.
     */
    nlohmann::json rotateKey(const std::string& key_id, const nlohmann::json& body);

private:
    std::shared_ptr<KeyProvider> key_provider_;
};

}} // namespace themis::server
