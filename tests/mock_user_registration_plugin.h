/**
 * @file mock_user_registration_plugin.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=7; TODO=1, Stub=1, Unimpl=0, Mock=5, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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
