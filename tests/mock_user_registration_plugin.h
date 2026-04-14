/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            mock_user_registration_plugin.h                    ║
  Version:         0.0.41                                             ║
  Last Modified:   2026-04-14 11:40:50                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   90.0/100                                       ║
    • Total Lines:     105                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "security/user_registration_plugin.h"
#include <openssl/evp.h>
#include <sstream>
#include <iomanip>

namespace themis {
namespace security {

/**
 * @brief Mock User Registration Plugin for Testing
 * 
 * Simple in-memory plugin for unit tests
 */
class MockUserRegistrationPlugin : public IUserRegistrationPlugin {
public:
    MockUserRegistrationPlugin() = default;
    
    std::string getName() const override {
        return "mock";
    }
    
    bool isAvailable() const override {
        return true;
    }
    
    Result<UserRegistrationData> registerUser(
        const std::string& user_id,
        const std::string& password,
        const std::unordered_map<std::string, std::string>& attributes
    ) override {
        UserRegistrationData data;
        data.user_id = user_id;
        data.password_hash = hashPassword(password);
        data.source = "mock";
        data.source_uri = "memory://test";
        data.roles.push_back("readonly");
        data.attributes = attributes;
        
        return data;
    }
    
    Result<UserRegistrationData> authenticateUser(
        const std::string& user_id,
        const std::string& password
    ) override {
        return registerUser(user_id, password, {});
    }
    
    Result<std::vector<UserRegistrationData>> syncUsers() override {
        return std::vector<UserRegistrationData>{};
    }
    
    Result<UserRegistrationData> updateUser(const std::string& user_id) override {
        UserRegistrationData data;
        data.user_id = user_id;
        data.source = "mock";
        return data;
    }

private:
    std::string hashPassword(const std::string& password) const {
        unsigned char hash[EVP_MAX_MD_SIZE];
        unsigned int hash_len = 0;
        
        EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
        EVP_DigestInit_ex(mdctx, EVP_sha256(), nullptr);
        EVP_DigestUpdate(mdctx, password.c_str(), password.length());
        EVP_DigestFinal_ex(mdctx, hash, &hash_len);
        EVP_MD_CTX_free(mdctx);
        
        std::stringstream ss;
        for (unsigned int i = 0; i < hash_len; i++) {
            ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
        }
        
        return ss.str();
    }
};

} // namespace security
} // namespace themis
