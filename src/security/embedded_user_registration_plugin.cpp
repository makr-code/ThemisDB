/**
 * @file embedded_user_registration_plugin.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=3, M=6, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "security/user_registration_plugin.h"
#include "utils/logger.h"
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/crypto.h>
#include <openssl/opensslv.h>
#include <openssl/params.h>
#if OPENSSL_VERSION_NUMBER >= 0x30200000L
#  include <openssl/kdf.h>
#endif
#include <sstream>
#include <iomanip>
#include <stdexcept>
#include <vector>
#include <unordered_map>
#include <mutex>

namespace themis {
namespace security {

/**
 * @brief Embedded User Registration Plugin
 * 
 * Local user management plugin for embedded deployments where external
 * identity providers are not available. This plugin provides a simple
 * in-memory or file-based user database.
 * 
 * **Use this plugin only when:**
 * - External identity providers (Apache, WebDAV, AD) are not available
 * - Running in embedded/standalone mode
 * - Testing or development environments
 * 
 * **For production, prefer:**
 * - WebDAV plugin with Active Directory
 * - Apache authentication integration
 * - Arrow plugin with enterprise data warehouse
 * 
 * This plugin is NOT recommended for production enterprise deployments.
 */
class EmbeddedUserRegistrationPlugin : public IUserRegistrationPlugin {
public:
    struct Config {
        std::string storage_path;  // Optional: file path for persistent storage
        bool in_memory_only = true;  // Default: in-memory only (no persistence)
        int min_password_length = 12;
        bool require_uppercase = true;
        bool require_lowercase = true;
        bool require_digit = true;
        bool require_special = true;
    };
    
    EmbeddedUserRegistrationPlugin()
        : EmbeddedUserRegistrationPlugin(Config{})
    {
    }

    explicit EmbeddedUserRegistrationPlugin(const Config& config)
        : config_(config)
    {
        THEMIS_INFO("EmbeddedUserRegistrationPlugin initialized (EMBEDDED MODE - use external auth for production)");
        if (config_.in_memory_only) {
            THEMIS_WARN("Embedded plugin running in memory-only mode - users will be lost on restart!");
        }
    }
    
    std::string getName() const override {
        return "embedded";
    }
    
    bool isAvailable() const override {
        // Always available as fallback
        return true;
    }
    
    Result<UserRegistrationData> registerUser(
        const std::string& user_id,
        const std::string& password,
        const std::unordered_map<std::string, std::string>& attributes
    ) override {
        std::lock_guard<std::mutex> lock(mutex_);
        
        THEMIS_INFO("Embedded plugin: Registering user '{}' (WARNING: Use external auth for production)", user_id);
        
        // Check if user already exists
        if (users_.find(user_id) != users_.end()) {
            return themis::Err<UserRegistrationData>(errors::ErrorCode::ERR_UTIL_INVALID_ARGUMENT, "User already exists");
        }
        
        // Validate password
        auto validation_result = validatePassword(password);
        if (!validation_result) {
            return tl::unexpected(validation_result.error());
        }
        
        // Hash password
        auto password_hash = hashPassword(password);
        
        // Store user data
        UserData user_data;
        user_data.user_id = user_id;
        user_data.password_hash = password_hash;
        user_data.attributes = attributes;
        user_data.password_history.push_back(password_hash);
        
        // Default role for embedded users
        user_data.roles.push_back("readonly");
        
        users_[user_id] = user_data;
        
        // Prepare registration data
        UserRegistrationData reg_data;
        reg_data.user_id = user_id;
        reg_data.password_hash = password_hash;
        reg_data.source = "embedded";
        reg_data.source_uri = config_.in_memory_only ? "memory://embedded" : config_.storage_path;
        reg_data.roles = user_data.roles;
        reg_data.attributes = attributes;
        
        THEMIS_WARN("User '{}' registered via EMBEDDED plugin - consider using WebDAV/Apache for production", user_id);
        
        return themis::Ok(std::move(reg_data));
    }
    
    Result<UserRegistrationData> authenticateUser(
        const std::string& user_id,
        const std::string& password
    ) override {
        std::lock_guard<std::mutex> lock(mutex_);
        
        THEMIS_DEBUG("Embedded plugin: Authenticating user '{}'", user_id);
        
        auto it = users_.find(user_id);
        if (it == users_.end()) {
            return themis::Err<UserRegistrationData>(errors::ErrorCode::ERR_API_UNAUTHORIZED, "User not found");
        }
        
        // Verify password
        if (!verifyPassword(password, it->second.password_hash)) {
            return themis::Err<UserRegistrationData>(errors::ErrorCode::ERR_API_UNAUTHORIZED, "Invalid password");
        }
        
        // Return user data
        UserRegistrationData reg_data;
        reg_data.user_id = user_id;
        reg_data.password_hash = it->second.password_hash;
        reg_data.source = "embedded";
        reg_data.source_uri = config_.in_memory_only ? "memory://embedded" : config_.storage_path;
        reg_data.roles = it->second.roles;
        reg_data.attributes = it->second.attributes;
        
        return themis::Ok(std::move(reg_data));
    }
    
    Result<std::vector<UserRegistrationData>> syncUsers() override {
        std::lock_guard<std::mutex> lock(mutex_);
        
        std::vector<UserRegistrationData> result = {};

        for (const auto& [user_id, user_data] : users_) {
            UserRegistrationData reg_data;
            reg_data.user_id = user_id;
            reg_data.password_hash = user_data.password_hash;
            reg_data.source = "embedded";
            reg_data.source_uri = config_.in_memory_only ? "memory://embedded" : config_.storage_path;
            reg_data.roles = user_data.roles;
            reg_data.attributes = user_data.attributes;
            result.push_back(reg_data);
        }
        
        return themis::Ok(std::move(result));
    }
    
    Result<UserRegistrationData> updateUser(const std::string& user_id) override {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = users_.find(user_id);
        if (it == users_.end()) {
            return themis::Err<UserRegistrationData>(errors::ErrorCode::ERR_API_UNAUTHORIZED, "User not found");
        }
        
        UserRegistrationData reg_data;
        reg_data.user_id = user_id;
        reg_data.password_hash = it->second.password_hash;
        reg_data.source = "embedded";
        reg_data.source_uri = config_.in_memory_only ? "memory://embedded" : config_.storage_path;
        reg_data.roles = it->second.roles;
        reg_data.attributes = it->second.attributes;
        
        return themis::Ok(std::move(reg_data));
    }
    
    // Additional methods for embedded plugin
    
    /**
     * @brief Change user password
     */
    Result<void> changePassword(
        const std::string& user_id,
        const std::string& old_password,
        const std::string& new_password
    ) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = users_.find(user_id);
        if (it == users_.end()) {
            return themis::ErrVoid(errors::ErrorCode::ERR_PLUGIN_NOT_FOUND, "User not found");
        }
        
        // Verify old password
        if (!verifyPassword(old_password, it->second.password_hash)) {
            return themis::ErrVoid(errors::ErrorCode::ERR_API_UNAUTHORIZED, "Invalid old password");
        }
        
        // Validate new password
        auto validation_result = validatePassword(new_password);
        if (!validation_result.has_value()) {
            return validation_result;
        }
        
        // Check password history
        auto new_hash = hashPassword(new_password);
        for (const auto& old_hash : it->second.password_history) {
            if (verifyPassword(new_password, old_hash)) {
                return themis::ErrVoid(errors::ErrorCode::ERR_UTIL_INVALID_ARGUMENT, "Password was used recently. Please choose a different password.");
            }
        }
        
        // Update password
        it->second.password_hash = new_hash;
        it->second.password_history.push_back(new_hash);
        
        // Keep only last 5 passwords
        if (it->second.password_history.size() > 5) {
            it->second.password_history.erase(it->second.password_history.begin());
        }
        
        THEMIS_INFO("Password changed for embedded user '{}'", user_id); // NOPII: user_id is a non-PII identifier, password is not logged
        return themis::OkVoid();
    }
    
    /**
     * @brief Check if user exists
     */
    bool userExists(const std::string& user_id) const {
        std::lock_guard<std::mutex> lock(mutex_);
        return users_.find(user_id) != users_.end();
    }

private:
    struct UserData {
        std::string user_id;
        std::string password_hash;
        std::vector<std::string> roles;
        std::unordered_map<std::string, std::string> attributes;
        std::vector<std::string> password_history;
    };
    
    Config config_;
    mutable std::mutex mutex_;
    std::unordered_map<std::string, UserData> users_;
    
    Result<void> validatePassword(const std::string& password) const {
        if (password.length() < static_cast<size_t>(config_.min_password_length)) {
            return themis::ErrVoid(
                errors::ErrorCode::ERR_UTIL_INVALID_ARGUMENT,
                "Password must be at least " + 
                std::to_string(config_.min_password_length) + 
                " characters long"
            );
        }
        
        if (config_.require_uppercase && 
            !std::any_of(password.begin(), password.end(), ::isupper)) {
            return themis::ErrVoid(errors::ErrorCode::ERR_UTIL_INVALID_ARGUMENT, "Password must contain at least one uppercase letter");
        }
        
        if (config_.require_lowercase && 
            !std::any_of(password.begin(), password.end(), ::islower)) {
            return themis::ErrVoid(errors::ErrorCode::ERR_UTIL_INVALID_ARGUMENT, "Password must contain at least one lowercase letter");
        }
        
        if (config_.require_digit && 
            !std::any_of(password.begin(), password.end(), ::isdigit)) {
            return themis::ErrVoid(errors::ErrorCode::ERR_UTIL_INVALID_ARGUMENT, "Password must contain at least one digit");
        }
        
        if (config_.require_special) {
            bool has_special = std::any_of(password.begin(), password.end(), [](char c) {
                return !::isalnum(c);
            });
            if (!has_special) {
                return themis::ErrVoid(errors::ErrorCode::ERR_UTIL_INVALID_ARGUMENT, "Password must contain at least one special character");
            }
        }
        
        return themis::OkVoid();
    }
    
    // -----------------------------------------------------------------------
    // Base64 helpers (no line wrapping, no padding trimming needed here)
    // -----------------------------------------------------------------------
    static std::string base64Encode(const unsigned char* data, int len) {
        // EVP_EncodeBlock writes exactly 4*ceil(len/3) chars plus a null terminator.
        int out_len = 4 * ((len + 2) / 3);
        std::string out(static_cast<size_t>(out_len) + 1, '\0');
        int actual = EVP_EncodeBlock(reinterpret_cast<unsigned char*>(&out[0]),
                                     data, len);
        out.resize(static_cast<size_t>(actual));
        return out;
    }

    static std::vector<unsigned char> base64Decode(const std::string& encoded) {
        // EVP_DecodeBlock output is at most 3*len/4 bytes (may include padding bytes)
        int max_out = static_cast<int>(encoded.size()) / 4 * 3 + 4;
        std::vector<unsigned char> out(static_cast<size_t>(max_out));
        int out_len = EVP_DecodeBlock(out.data(),
                                      reinterpret_cast<const unsigned char*>(encoded.data()),
                                      static_cast<int>(encoded.size()));
        if (out_len < 0) return {};
        // Trim padding bytes (= signs at end of base64 input add null bytes)
        size_t padding = 0;
        for (auto it = encoded.rbegin(); it != encoded.rend() && *it == '='; ++it)
            ++padding;
        out.resize(static_cast<size_t>(out_len) - padding);
        return out;
    }

    // Split a string by a delimiter character.
    static std::vector<std::string> splitBy(const std::string& s, char delim) {
        std::vector<std::string> parts;
        size_t start = 0;
        for (;;) {
            size_t end = s.find(delim, start);
            parts.push_back(s.substr(start, end == std::string::npos ? end : end - start));
            if (end == std::string::npos) {
              break;
            }
            start = end + 1;
        }
        return parts;
    }

    // -----------------------------------------------------------------------
    // Argon2id via OpenSSL 3.2+ EVP_KDF (OWASP minimum recommended params)
    // Falls back to PBKDF2-SHA256 on older OpenSSL builds.
    //
    // Format:  $argon2id$v=19$m=19456,t=2,p=1$<base64salt>$<base64hash>
    // -----------------------------------------------------------------------
    std::string hashPassword(const std::string& password) const {
        constexpr int SALT_LEN = 16;
        constexpr int DK_LEN   = 32;

        unsigned char salt[SALT_LEN];
        if (RAND_bytes(salt, SALT_LEN) != 1) {
            throw std::runtime_error("RAND_bytes failed for password salt");
        }

#if OPENSSL_VERSION_NUMBER >= 0x30200000L
        // --- Argon2id path (OpenSSL 3.2+) ---
        // OWASP recommended minimum: m=19456 KiB (≈19 MiB), t=2 iterations, p=1 lane
        uint32_t t_cost = 2, m_cost = 19456, lanes = 1, threads = 1, version = 19;

        EVP_KDF* kdf = EVP_KDF_fetch(nullptr, "ARGON2ID", nullptr);
        if (!kdf) {
            throw std::runtime_error("Argon2id KDF not available in this OpenSSL build");
        }
        EVP_KDF_CTX* ctx = EVP_KDF_CTX_new(kdf);
        EVP_KDF_free(kdf);
        if (!ctx) {
            throw std::runtime_error("EVP_KDF_CTX_new failed");
        }

        OSSL_PARAM params[] = {
            OSSL_PARAM_construct_octet_string("pass",
                const_cast<char*>(password.data()),
                password.size()),
            OSSL_PARAM_construct_octet_string("salt", salt, SALT_LEN),
            OSSL_PARAM_construct_uint32("t",       &t_cost),
            OSSL_PARAM_construct_uint32("m",       &m_cost),
            OSSL_PARAM_construct_uint32("lanes",   &lanes),
            OSSL_PARAM_construct_uint32("threads", &threads),
            OSSL_PARAM_construct_uint32("version", &version),
            OSSL_PARAM_construct_end()
        };

        unsigned char dk[DK_LEN];
        int rc = EVP_KDF_derive(ctx, dk, DK_LEN, params);
        EVP_KDF_CTX_free(ctx);
        if (rc != 1) {
            throw std::runtime_error("Argon2id EVP_KDF_derive failed");
        }

        // PHC-compatible format
        return "$argon2id$v=19$m=19456,t=2,p=1$"
             + base64Encode(salt, SALT_LEN) + "$"
             + base64Encode(dk, DK_LEN);
#else
        // --- PBKDF2-SHA256 fallback for OpenSSL < 3.2 ---
        constexpr int ITER = 100000;
        unsigned char dk[DK_LEN];
        if (PKCS5_PBKDF2_HMAC(password.c_str(),
                               static_cast<int>(password.size()),
                               salt, SALT_LEN,
                               ITER,
                               EVP_sha256(),
                               DK_LEN, dk) != 1) {
            throw std::runtime_error("PBKDF2 failed for password hashing");
        }
        auto toHex = [](const unsigned char* data, int len) {
            std::ostringstream ss = {};
            for (int i = 0; i < len; ++i)
                ss << std::hex << std::setw(2) << std::setfill('0')
                   << static_cast<int>(data[i]);
            return ss.str();
        };
        return "pbkdf2$" + toHex(salt, SALT_LEN) + "$" + toHex(dk, DK_LEN);
#endif
    }
    
    bool verifyPassword(const std::string& password, const std::string& stored_hash) const {
        // --- Argon2id hashes: $argon2id$v=19$m=...,t=...,p=...$<salt>$<hash> ---
        if (stored_hash.rfind("$argon2id$", 0) == 0) {
#if OPENSSL_VERSION_NUMBER >= 0x30200000L
            // Parse: $argon2id$v=<v>$m=<m>,t=<t>,p=<p>$<b64salt>$<b64hash>
            // We always produced m=19456,t=2,p=1 but parse them for forward compat.
            auto parse_argon2id = [&]() -> bool {
                // Split: $argon2id$v=19$m=19456,t=2,p=1$<b64salt>$<b64hash>
                // splitBy('$') produces: ["","argon2id","v=19","m=...,t=...,p=...","<salt>","<hash>"]
                auto parts = splitBy(stored_hash, '$');
                // Expect exactly 6 parts (parts[0] is empty due to leading '$')
                if (parts.size() != 6) {
                  return false;
                }

                std::string params_str = parts[3]; // "m=19456,t=2,p=1"
                std::string salt_b64   = parts[4];
                std::string hash_b64   = parts[5];

                uint32_t m = 19456, t = 2, p = 1;
                if (std::sscanf(params_str.c_str(), "m=%u,t=%u,p=%u", &m, &t, &p) != 3)
                    return false;

                auto salt_bytes = base64Decode(salt_b64);
                auto stored_dk  = base64Decode(hash_b64);
                if (salt_bytes.empty() || stored_dk.empty()) {
                  return false;
                }

                uint32_t version = 19;
                // threads is set to match lanes so Argon2 runs single-threaded
                // during verification (standard single-threaded Argon2id behavior).
                uint32_t threads_val = p;

                EVP_KDF* kdf = EVP_KDF_fetch(nullptr, "ARGON2ID", nullptr);
                if (!kdf) {
                  return false;
                }
                EVP_KDF_CTX* ctx = EVP_KDF_CTX_new(kdf);
                EVP_KDF_free(kdf);
                if (!ctx) {
                  return false;
                }

                OSSL_PARAM ossl_params[] = {
                    OSSL_PARAM_construct_octet_string("pass",
                        const_cast<char*>(password.data()),
                        password.size()),
                    OSSL_PARAM_construct_octet_string("salt",
                        salt_bytes.data(),
                        salt_bytes.size()),
                    OSSL_PARAM_construct_uint32("t",       &t),
                    OSSL_PARAM_construct_uint32("m",       &m),
                    OSSL_PARAM_construct_uint32("lanes",   &p),
                    OSSL_PARAM_construct_uint32("threads", &threads_val),
                    OSSL_PARAM_construct_uint32("version", &version),
                    OSSL_PARAM_construct_end()
                };

                std::vector<unsigned char> computed_dk(stored_dk.size());
                int rc = EVP_KDF_derive(ctx, computed_dk.data(),
                                        computed_dk.size(), ossl_params);
                EVP_KDF_CTX_free(ctx);
                if (rc != 1) {
                  return false;
                }

                return CRYPTO_memcmp(computed_dk.data(), stored_dk.data(),
                                     stored_dk.size()) == 0;
            };
            return parse_argon2id();
#else
            // Argon2id hashes cannot be verified without OpenSSL 3.2+
            return false;
#endif
        }

        // --- PBKDF2-SHA256 hashes: "pbkdf2$<hex-salt>$<hex-dk>" ---
        if (stored_hash.rfind("pbkdf2$", 0) == 0) {
            constexpr int SALT_HEX_LEN = 32; // 16 bytes * 2
            constexpr int DK_LEN       = 32;
            constexpr int ITER         = 100000;

            // Expected format: "pbkdf2$" (7) + salt_hex (32) + "$" (1) + dk_hex (64) = 104 chars
            if (stored_hash.size() != 7u + SALT_HEX_LEN + 1u + 64u) {
                return false;
            }

            std::string salt_hex = stored_hash.substr(7, SALT_HEX_LEN);
            std::string dk_hex   = stored_hash.substr(7 + SALT_HEX_LEN + 1);

            auto fromHex = [](const std::string& hex, std::vector<unsigned char>& out) {
                out.resize(hex.size() / 2);
                for (size_t i = 0; i < out.size(); ++i) {
                    out[i] = static_cast<unsigned char>(
                        std::stoul(hex.substr(i * 2, 2), nullptr, 16));
                }
            };

            std::vector<unsigned char> salt, stored_dk;
            fromHex(salt_hex, salt);
            fromHex(dk_hex, stored_dk);

            unsigned char computed_dk[DK_LEN];
            if (PKCS5_PBKDF2_HMAC(password.c_str(),
                                   static_cast<int>(password.size()),
                                   salt.data(), static_cast<int>(salt.size()),
                                   ITER,
                                   EVP_sha256(),
                                   DK_LEN, computed_dk) != 1) {
                return false;
            }

            return CRYPTO_memcmp(computed_dk, stored_dk.data(), DK_LEN) == 0;
        }

        // --- Legacy SHA-256 path: plain 64-char hex ---
        unsigned char hash[EVP_MAX_MD_SIZE];
        unsigned int  hash_len = 0;
        EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
        EVP_DigestInit_ex(mdctx, EVP_sha256(), nullptr);
        EVP_DigestUpdate(mdctx, password.c_str(), password.length());
        EVP_DigestFinal_ex(mdctx, hash, &hash_len);
        EVP_MD_CTX_free(mdctx);

        std::ostringstream ss = {};
        for (unsigned int i = 0; i < hash_len; ++i)
            ss << std::hex << std::setw(2) << std::setfill('0')
               << static_cast<int>(hash[i]);

        return ss.str() == stored_hash;
    }
};

// Factory function (declared in user_registration_plugin.h)
std::shared_ptr<IUserRegistrationPlugin> createEmbeddedUserRegistrationPlugin() {
    return std::make_shared<EmbeddedUserRegistrationPlugin>();
}

} // namespace security
} // namespace themis

