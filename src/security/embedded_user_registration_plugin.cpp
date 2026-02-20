#include "security/user_registration_plugin.h"
#include "utils/logger.h"
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/crypto.h>
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
    
    explicit EmbeddedUserRegistrationPlugin(const Config& config = Config{})
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
        
        std::vector<UserRegistrationData> result;
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
        
        THEMIS_INFO("Password changed for embedded user '{}'", user_id);
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
    
    std::string hashPassword(const std::string& password) const {
        // PBKDF2-SHA256 with a random 16-byte salt and 100,000 iterations.
        // Format: "pbkdf2$<hex-salt>$<hex-dk>"
        // This is far more resistant to brute-force than plain SHA-256.
        constexpr int SALT_LEN = 16;
        constexpr int DK_LEN   = 32;   // 256-bit derived key
        constexpr int ITER     = 100000;

        unsigned char salt[SALT_LEN];
        if (RAND_bytes(salt, SALT_LEN) != 1) {
            throw std::runtime_error("RAND_bytes failed for password salt");
        }

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
            std::ostringstream ss;
            for (int i = 0; i < len; ++i)
                ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(data[i]);
            return ss.str();
        };

        return "pbkdf2$" + toHex(salt, SALT_LEN) + "$" + toHex(dk, DK_LEN);
    }
    
    bool verifyPassword(const std::string& password, const std::string& stored_hash) const {
        // Support both legacy SHA-256 hashes (plain 64-char hex) and new PBKDF2 hashes.
        if (stored_hash.rfind("pbkdf2$", 0) == 0) {
            // Parse "pbkdf2$<hex-salt>$<hex-dk>"
            constexpr int SALT_HEX_LEN = 32; // 16 bytes * 2
            constexpr int DK_LEN       = 32;
            constexpr int ITER         = 100000;

            // Expected format: "pbkdf2$" (7) + salt_hex (32) + "$" (1) + dk_hex (64) = 104 chars total
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

            // Constant-time comparison to prevent timing attacks
            return CRYPTO_memcmp(computed_dk, stored_dk.data(), DK_LEN) == 0;
        }

        // Legacy SHA-256 path: plain 64-char hex (for passwords stored before upgrade)
        unsigned char hash[EVP_MAX_MD_SIZE];
        unsigned int  hash_len = 0;
        EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
        EVP_DigestInit_ex(mdctx, EVP_sha256(), nullptr);
        EVP_DigestUpdate(mdctx, password.c_str(), password.length());
        EVP_DigestFinal_ex(mdctx, hash, &hash_len);
        EVP_MD_CTX_free(mdctx);

        std::ostringstream ss;
        for (unsigned int i = 0; i < hash_len; ++i)
            ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);

        return ss.str() == stored_hash;
    }
};

// Factory function (declared in user_registration_plugin.h)
std::shared_ptr<IUserRegistrationPlugin> createEmbeddedUserRegistrationPlugin() {
    return std::make_shared<EmbeddedUserRegistrationPlugin>();
}

} // namespace security
} // namespace themis
