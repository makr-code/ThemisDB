/**
 * @file arrow_user_registration_plugin.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=7, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "security/arrow_user_registration_plugin.h"
#include "security/user_registration_plugin.h"
#include "utils/logger.h"
#include <openssl/evp.h>
#include <memory>
#include <sstream>
#include <iomanip>

namespace themis {
namespace security {

namespace {

// ── RAII Wrappers for OpenSSL objects ─────────────────────────────────────────
struct ArrowUser_EVP_MD_CTX_Deleter {
    void operator()(EVP_MD_CTX* p) const { if (p) EVP_MD_CTX_free(p); }
};

using ArrowUser_EVP_MD_CTX_ptr = std::unique_ptr<EVP_MD_CTX, ArrowUser_EVP_MD_CTX_Deleter>;

} // anonymous namespace

ArrowUserRegistrationPlugin::ArrowUserRegistrationPlugin(const Config& config)
    : config_(config)
{
    THEMIS_INFO("ArrowUserRegistrationPlugin initialized with source: {}",
                config_.arrow_source_uri);
}

std::string ArrowUserRegistrationPlugin::getName() const {
    return "arrow";
}

bool ArrowUserRegistrationPlugin::isAvailable() const {
#ifdef THEMIS_ENABLE_ARROW
    return !config_.arrow_source_uri.empty();
#else
    return false;
#endif
}

Result<UserRegistrationData> ArrowUserRegistrationPlugin::registerUser(
    const std::string& user_id,
    [[maybe_unused]] const std::string& password,
    [[maybe_unused]] const std::unordered_map<std::string, std::string>& attributes)
{
    THEMIS_INFO("Arrow plugin: Registering user '{}'", user_id);

#ifdef THEMIS_ENABLE_ARROW
    UserRegistrationData data;
    data.user_id       = user_id;
    data.password_hash = hashPassword(password);
    data.source        = "arrow";
    data.source_uri    = config_.arrow_source_uri;

    for (const auto& [key, value] : attributes) {
        data.attributes[key] = value;
    }

    if (data.roles.empty()) {
        data.roles.push_back("readonly");
    }

    {
        std::lock_guard<std::mutex> lock(store_mutex_);
        user_store_[user_id] = data;
    }

    return themis::Ok(std::move(data));
#else
    return themis::Err<UserRegistrationData>(
        errors::ErrorCode::ERR_PLUGIN_INCOMPATIBLE,
        "Apache Arrow support not enabled in build"
    );
#endif
}

Result<UserRegistrationData> ArrowUserRegistrationPlugin::authenticateUser(
    const std::string& user_id,
    [[maybe_unused]] const std::string& password)
{
    THEMIS_INFO("Arrow plugin: Authenticating user '{}'", user_id);

#ifdef THEMIS_ENABLE_ARROW
    return authenticateFromArrow(user_id, password);
#else
    return themis::Err<UserRegistrationData>(
        errors::ErrorCode::ERR_PLUGIN_INCOMPATIBLE,
        "Apache Arrow support not enabled in build"
    );
#endif
}

Result<std::vector<UserRegistrationData>> ArrowUserRegistrationPlugin::syncUsers() {
    THEMIS_INFO("Arrow plugin: Syncing users from source '{}'", config_.arrow_source_uri);

#ifdef THEMIS_ENABLE_ARROW
    std::vector<UserRegistrationData> users;

    {
        std::lock_guard<std::mutex> lock(store_mutex_);
        users.reserve(user_store_.size());
        for (const auto& [id, data] : user_store_) {
            users.push_back(data);
        }
    }

    THEMIS_INFO("Arrow plugin: Synced {} users", users.size());
    return themis::Ok(std::move(users));
#else
    return themis::Err<std::vector<UserRegistrationData>>(
        errors::ErrorCode::ERR_PLUGIN_INCOMPATIBLE,
        "Apache Arrow support not enabled in build"
    );
#endif
}

Result<UserRegistrationData> ArrowUserRegistrationPlugin::updateUser(
    const std::string& user_id)
{
    THEMIS_INFO("Arrow plugin: Updating user '{}'", user_id);

#ifdef THEMIS_ENABLE_ARROW
    std::lock_guard<std::mutex> lock(store_mutex_);
    auto it = user_store_.find(user_id);
    if (it == user_store_.end()) {
        return themis::Err<UserRegistrationData>(
            errors::ErrorCode::ERR_PLUGIN_NOT_FOUND,
            "User '" + user_id + "' not found in Arrow-backed store"
        );
    }
    return themis::Ok(it->second);
#else
    return themis::Err<UserRegistrationData>(
        themis::errors::ErrorCode::ERR_PLUGIN_INCOMPATIBLE,
        "Apache Arrow support not enabled in build"
    );
#endif
}

#ifdef THEMIS_ENABLE_ARROW

Result<size_t> ArrowUserRegistrationPlugin::bulkSyncFromArrow(
    const arrow::RecordBatch& batch)
{
    auto schema = batch.schema();

    int user_id_idx  = schema->GetFieldIndex(config_.user_id_column);
    int password_idx = schema->GetFieldIndex(config_.password_column);
    int roles_idx    = schema->GetFieldIndex(config_.roles_column);
    int email_idx    = schema->GetFieldIndex(config_.email_column);

    if (user_id_idx < 0) {
        return themis::Err<size_t>(
            errors::ErrorCode::ERR_UTIL_INVALID_ARGUMENT,
            "RecordBatch missing required column '" + config_.user_id_column + "'"
        );
    }
    if (password_idx < 0) {
        return themis::Err<size_t>(
            errors::ErrorCode::ERR_UTIL_INVALID_ARGUMENT,
            "RecordBatch missing required column '" + config_.password_column + "'"
        );
    }

    auto user_id_col  = std::static_pointer_cast<arrow::StringArray>(batch.column(user_id_idx));
    auto password_col = std::static_pointer_cast<arrow::StringArray>(batch.column(password_idx));

    std::shared_ptr<arrow::StringArray> roles_col;
    std::shared_ptr<arrow::StringArray> email_col;
    if (roles_idx >= 0) {
        roles_col = std::static_pointer_cast<arrow::StringArray>(batch.column(roles_idx));
    }
    if (email_idx >= 0) {
        email_col = std::static_pointer_cast<arrow::StringArray>(batch.column(email_idx));
    }

    std::lock_guard<std::mutex> lock(store_mutex_);
    const int64_t num_rows = batch.num_rows();
    for (int64_t i = 0; i < num_rows; ++i) {
        UserRegistrationData data;
        data.user_id       = user_id_col->IsNull(i)  ? "" : user_id_col->GetString(i);
        data.password_hash = password_col->IsNull(i) ? "" : password_col->GetString(i);
        data.source        = "arrow";
        data.source_uri    = config_.arrow_source_uri;

        if (roles_col && !roles_col->IsNull(i)) {
            std::string roles_str = roles_col->GetString(i);
            std::string role;
            for (char c : roles_str) {
                if (c == ',') {
                    if (!role.empty()) {
                        data.roles.push_back(role);
                        role.clear();
                    }
                } else {
                    role += c;
                }
            }
            if (!role.empty()) {
                data.roles.push_back(role);
            }
        }

        if (email_col && !email_col->IsNull(i)) {
            data.attributes["email"] = email_col->GetString(i);
        }

        if (!data.user_id.empty()) {
            user_store_[data.user_id] = std::move(data);
        }
    }
    return themis::Ok(static_cast<size_t>(num_rows));
}

Result<UserRegistrationData> ArrowUserRegistrationPlugin::authenticateFromArrow(
    const std::string& user_id,
    const std::string& credentials)
{
    std::lock_guard<std::mutex> lock(store_mutex_);
    auto it = user_store_.find(user_id);
    if (it == user_store_.end()) {
        return themis::Err<UserRegistrationData>(
            errors::ErrorCode::ERR_PLUGIN_NOT_FOUND,
            "User '" + user_id + "' not found in Arrow-backed store"
        );
    }

    const std::string& stored_hash = it->second.password_hash;
    if (stored_hash != credentials && stored_hash != hashPassword(credentials)) {
        return themis::Err<UserRegistrationData>(
            errors::ErrorCode::ERR_PLUGIN_INCOMPATIBLE,
            "Authentication failed for user '" + user_id + "'"
        );
    }
    return themis::Ok(it->second);
}

#endif // THEMIS_ENABLE_ARROW

std::string ArrowUserRegistrationPlugin::hashPassword(const std::string& password) const {
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int  hash_len = 0;

    EVP_MD_CTX_ptr mdctx(EVP_MD_CTX_new());
    EVP_DigestInit_ex(mdctx.get(), EVP_sha256(), nullptr);
    EVP_DigestUpdate(mdctx.get(), password.c_str(), password.length());
    EVP_DigestFinal_ex(mdctx.get(), hash, &hash_len);

    std::stringstream ss;
    for (unsigned int i = 0; i < hash_len; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }
    return ss.str();
}

} // namespace security
} // namespace themis

