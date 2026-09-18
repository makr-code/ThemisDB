#pragma once

#include "importers/importer_interface.h"
#include "plugins/plugin_interface.h"
#include <atomic>
#include <string>
#include <vector>
#include <map>

// Forward-declare hiredis context for the THEMIS_ENABLE_REDIS production path.
// The full definition is only visible in redis_importer.cpp which includes
// <hiredis/hiredis.h> under the same build guard.
#ifdef THEMIS_ENABLE_REDIS
struct redisContext;
#endif

namespace themis {
namespace importers {

class RedisImporter : public IImporter {
public:
    RedisImporter();
    ~RedisImporter() override;

    // -------------------------------------------------------------------------
    // IImporter interface
    // -------------------------------------------------------------------------

    const char* getName() const override { return "Redis Importer"; }

    std::vector<std::string> getSupportedTypes() const override;

    bool initialize(const std::string& config) override;

    bool validateSource(const std::string& source_path,
                        std::vector<std::string>& errors) override;

    ImportStats importData(
        const std::string& source_path,
        const ImportOptions& options,
        ProgressCallback progress_callback = nullptr) override;

    std::shared_ptr<ImportHandle> importDataAsync(
        const std::string& source_path,
        const ImportOptions& options) override;

    void cancel() override;

    json getSourceSchema(const std::string& source_path) override;

    // -------------------------------------------------------------------------
    // Testing support
    // -------------------------------------------------------------------------

    using MockCommandFn = std::function<std::string(
        const std::vector<std::string>& cmd)>;
    /**
     * @brief Set Mock Command For Testing.
     * @param[in] fn Input parameter.
     */
    void setMockCommandForTesting(MockCommandFn fn);

private:
    enum class RedisValueType { String, Hash, List, Set, ZSet, Unknown };

    struct Config {
        std::string host{"127.0.0.1"};
        int port{6379};
        // password is never stored after auth; zeroed on connection.
        int db{0};
        std::string key_pattern{"*"};
        int batch_size{100};
        int pipeline_size{50};
        uint32_t timeout_ms{5000};
        bool tls{false};
    };

    /**
     * @brief Parse Redis Type.
     * @param[in] type_str Input parameter.
     * @return Return value.
     */
    static RedisValueType parseRedisType(const std::string& type_str);

    /**
     * @brief Sanitise Endpoint.
     * @param[in] host Input parameter.
     * @param[in] port Input parameter.
     * @return Return value.
     */
    static std::string sanitiseEndpoint(const std::string& host, int port);

    json fetchKeyDocument(const std::string& key,
                          RedisValueType vtype,
                          std::string& error_out,
                          void* conn = nullptr);

    Config config_;
    std::atomic<bool> cancelled_{false};
    MockCommandFn mock_command_fn_;

#ifdef THEMIS_ENABLE_REDIS
    /**
     * @brief Execute Hiredis Command.
     * @param[in,out] conn Input/output parameter.
     * @param[in] cmd Input parameter.
     * @return Return value.
     */
    static std::string executeHiredisCommand(void* conn,
                                              const std::vector<std::string>& cmd);

    /**
     * @brief Open Redis Connection.
     * @return Pointer to the result.
     */
    redisContext* openRedisConnection() const;
#endif
};

} // namespace importers
} // namespace themis
