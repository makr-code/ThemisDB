/**
 * @file arrow_flight.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "analytics/arrow_export.h"

#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace themisdb {
namespace analytics {

// ---------------------------------------------------------------------------
// FlightDescriptor
// ---------------------------------------------------------------------------

struct FlightDescriptor {
    enum class Type { PATH, COMMAND };

    Type type = Type::PATH;

    std::vector<std::string> path;

    std::string command;

    // Convenience constructors
    /**
     * @brief From Path.
     * @param[in] components Input parameter.
     * @return Return value.
     * @details Calls: std::move().
     */
    static FlightDescriptor fromPath(std::vector<std::string> components) {
        FlightDescriptor fd;
        fd.type = Type::PATH;
        fd.path = std::move(components);
        return fd;
    }

    /**
     * @brief From Command.
     * @param[in] cmd Input parameter.
     * @return Return value.
     * @details Calls: std::move().
     */
    static FlightDescriptor fromCommand(std::string cmd) {
        FlightDescriptor fd;
        fd.type = Type::COMMAND;
        fd.command = std::move(cmd);
        return fd;
    }

    /**
     * @brief To String.
     * @return Return value.
     */
    std::string toString() const;
};

// ---------------------------------------------------------------------------
// FlightInfo
// ---------------------------------------------------------------------------

struct FlightInfo {
    FlightDescriptor descriptor;
    int64_t total_records = -1;
    int64_t total_bytes   = -1;
    std::string metadata;
};

// ---------------------------------------------------------------------------
// FlightCallOptions
// ---------------------------------------------------------------------------

struct FlightCallOptions {
    std::chrono::milliseconds timeout{0};
    std::vector<std::pair<std::string, std::string>> headers;
};

// ---------------------------------------------------------------------------
// FlightServerOptions
// ---------------------------------------------------------------------------

struct FlightServerOptions {
    std::string host = "0.0.0.0";
    int port = 8815;
    bool register_inprocess = true;
};

// ---------------------------------------------------------------------------
// FlightClientOptions
// ---------------------------------------------------------------------------

struct FlightClientOptions {
    std::string host = "localhost";
    int port = 8815;
    bool prefer_inprocess = true;
    std::chrono::milliseconds connect_timeout{5000};
};

// ---------------------------------------------------------------------------
// FlightPutResult
// ---------------------------------------------------------------------------

struct FlightPutResult {
    bool success = false;
    std::string message;
    int64_t rows_accepted = 0;
    int64_t bytes_transferred = 0;
};

// ---------------------------------------------------------------------------
// ArrowFlightServer
// ---------------------------------------------------------------------------

class ArrowFlightServer {
public:
    /**
     * @brief Arrow Flight Server.
     * @return Return value.
     */
    virtual ~ArrowFlightServer() = default;

    ArrowFlightServer(ArrowFlightServer&&) noexcept = default;

    ArrowFlightServer& operator=(ArrowFlightServer&&) noexcept = default;

    ArrowFlightServer(const ArrowFlightServer&) = delete;
    ArrowFlightServer& operator=(const ArrowFlightServer&) = delete;

protected:
    ArrowFlightServer() = default;

public:
    static std::unique_ptr<ArrowFlightServer> create(
        const FlightServerOptions& opts = FlightServerOptions{});

    /**
     * @brief Start.
     */
    virtual void start() = 0;

    /**
     * @brief Stop.
     */
    virtual void stop() = 0;

    [[nodiscard]] virtual bool isRunning() const = 0;

    virtual void registerDataset(
        std::vector<std::string> path,
        std::function<themis::analytics::ArrowRecordBatch()> producer,
        int64_t total_rows = -1) = 0;

    virtual void registerPutHandler(
        std::vector<std::string> path,
        std::function<void(themis::analytics::ArrowRecordBatch)> handler) = 0;

    /**
     * @brief Unregister Dataset.
     * @param[in] path Input parameter.
     */
    virtual void unregisterDataset(const std::vector<std::string>& path) = 0;

    [[nodiscard]] virtual std::vector<FlightInfo> listRegisteredDatasets() const = 0;

    [[nodiscard]] virtual std::string endpointUrl() const = 0;
};

// ---------------------------------------------------------------------------
// ArrowFlightClient
// ---------------------------------------------------------------------------

class ArrowFlightClient {
public:
    /**
     * @brief Arrow Flight Client.
     * @return Return value.
     */
    virtual ~ArrowFlightClient() = default;

    ArrowFlightClient(ArrowFlightClient&&) noexcept = default;

    ArrowFlightClient& operator=(ArrowFlightClient&&) noexcept = default;

    ArrowFlightClient(const ArrowFlightClient&) = delete;
    ArrowFlightClient& operator=(const ArrowFlightClient&) = delete;

protected:
    ArrowFlightClient() = default;

public:
    static std::unique_ptr<ArrowFlightClient> connect(
        const FlightClientOptions& opts = FlightClientOptions{});

    [[nodiscard]] virtual std::vector<FlightInfo> listFlights(
        const FlightCallOptions& call_opts = {}) = 0;

    [[nodiscard]] virtual themis::analytics::ArrowRecordBatch doGet(
        const FlightDescriptor& descriptor,
        const FlightCallOptions& call_opts = {}) = 0;

    [[nodiscard]] virtual FlightPutResult doPut(
        const themis::analytics::ArrowRecordBatch& batch,
        const FlightDescriptor& descriptor,
        const FlightCallOptions& call_opts = {}) = 0;

    /**
     * @brief Close.
     */
    virtual void close() = 0;

    [[nodiscard]] virtual bool isConnected() const = 0;
};

} // namespace analytics
} // namespace themisdb
