/**
 * @file arrow_user_registration_plugin.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.12
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "security/user_registration_plugin.h"

#ifdef THEMIS_ENABLE_ARROW
#include <arrow/api.h>
#endif

namespace themis {
namespace security {

class ArrowUserRegistrationPlugin : public IUserRegistrationPlugin {
public:
    struct Config {
        std::string arrow_source_uri;         ///< e.g., "file:///path/to/users.parquet"
        std::string arrow_flight_endpoint;    ///< Optional Arrow Flight endpoint
        std::string user_id_column       = "user_id";
        std::string username_column      = "username";
        std::string password_column      = "password_hash";
        std::string roles_column         = "roles";
        std::string email_column         = "email";
        bool        auto_sync            = false; ///< Automatically sync users on startup
    };

    /**
     * @brief Arrow User Registration Plugin.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit ArrowUserRegistrationPlugin(const Config& config);

    std::string getName() const override;
    bool        isAvailable() const override;

    Result<UserRegistrationData> registerUser(
        const std::string& user_id,
        const std::string& password,
        const std::unordered_map<std::string, std::string>& attributes = {}
    ) override;

    Result<UserRegistrationData> authenticateUser(
        const std::string& user_id,
        const std::string& password
    ) override;

    Result<std::vector<UserRegistrationData>> syncUsers() override;

    Result<UserRegistrationData> updateUser(const std::string& user_id) override;

#ifdef THEMIS_ENABLE_ARROW
    /**
     * @brief Bulk Sync From Arrow.
     * @param[in] batch Input parameter.
     * @return Return value.
     */
    Result<size_t> bulkSyncFromArrow(const arrow::RecordBatch& batch);

    /**
     * @brief Authenticate From Arrow.
     * @param[in] user_id Identifier of the user.
     * @param[in] credentials Input parameter.
     * @return Return value.
     */
    Result<UserRegistrationData> authenticateFromArrow(
        const std::string& user_id,
        const std::string& credentials
    );
#endif // THEMIS_ENABLE_ARROW

private:
    Config                                              config_;
    std::unordered_map<std::string, UserRegistrationData> user_store_;
    mutable std::mutex                                  store_mutex_;

    /**
     * @brief Hash Password.
     * @param[in] password Input parameter.
     * @return Return value.
     */
    std::string hashPassword(const std::string& password) const;
};

} // namespace security
} // namespace themis
