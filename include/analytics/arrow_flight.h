/**
 * @file arrow_flight.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/**
 * @brief Identifies a dataset or query exposed by an Arrow Flight server.
 *
 * A descriptor is either a *path* (hierarchical name, e.g. {"sales",
 * "2024-Q1"}) or a *command* (an opaque byte string carrying a serialised
 * query). ThemisDB currently uses path descriptors; command descriptors are
 * reserved for future AQL-over-Flight support.
 */
struct FlightDescriptor {
    enum class Type { PATH, COMMAND };

    Type type = Type::PATH;

    /** Hierarchical path components (used when type == PATH). */
    std::vector<std::string> path;

    /** Opaque command bytes (used when type == COMMAND). */
    std::string command;

    // Convenience constructors
    static FlightDescriptor fromPath(std::vector<std::string> components) {
        FlightDescriptor fd;
        fd.type = Type::PATH;
        fd.path = std::move(components);
        return fd;
    }

    static FlightDescriptor fromCommand(std::string cmd) {
        FlightDescriptor fd;
        fd.type = Type::COMMAND;
        fd.command = std::move(cmd);
        return fd;
    }

    /** Returns a human-readable string representation. */
    std::string toString() const;
};

// ---------------------------------------------------------------------------
// FlightInfo
// ---------------------------------------------------------------------------

/**
 * @brief Metadata about a dataset available from a Flight server.
 *
 * Returned by ArrowFlightClient::listFlights() and used to obtain a
 * FlightDescriptor that can be passed to doGet().
 */
struct FlightInfo {
    FlightDescriptor descriptor;
    /** Approximate total number of rows in the dataset (-1 = unknown). */
    int64_t total_records = -1;
    /** Approximate total byte size of the dataset (-1 = unknown). */
    int64_t total_bytes   = -1;
    /** Arbitrary metadata string (JSON, protobuf, etc.). */
    std::string metadata;
};

// ---------------------------------------------------------------------------
// FlightCallOptions
// ---------------------------------------------------------------------------

/**
 * @brief Per-call configuration for Arrow Flight RPC operations.
 */
struct FlightCallOptions {
    /** Timeout for the RPC call (0 = no timeout). */
    std::chrono::milliseconds timeout{0};
    /** Key/value metadata headers sent with the call. */
    std::vector<std::pair<std::string, std::string>> headers;
};

// ---------------------------------------------------------------------------
// FlightServerOptions
// ---------------------------------------------------------------------------

/**
 * @brief Configuration for an Arrow Flight server instance.
 */
struct FlightServerOptions {
    /** Bind address (e.g. "0.0.0.0" or "127.0.0.1"). */
    std::string host = "0.0.0.0";
    /** TCP port to listen on (default: 8815, the Arrow Flight standard port). */
    int port = 8815;
    /**
     * When true the server also registers itself in the in-process registry
     * so that in-process clients can find it without a real network
     * connection (useful for single-node / test deployments).
     */
    bool register_inprocess = true;
};

// ---------------------------------------------------------------------------
// FlightClientOptions
// ---------------------------------------------------------------------------

/**
 * @brief Configuration for an Arrow Flight client connection.
 */
struct FlightClientOptions {
    /** Remote host name or IP address. */
    std::string host = "localhost";
    /** Remote TCP port. */
    int port = 8815;
    /**
     * When true the client attempts to resolve the server from the
     * in-process registry before opening a real network connection.
     * This enables zero-copy in-process communication.
     */
    bool prefer_inprocess = true;
    /** Connection timeout. */
    std::chrono::milliseconds connect_timeout{5000};
};

// ---------------------------------------------------------------------------
// FlightPutResult
// ---------------------------------------------------------------------------

/**
 * @brief Result of an ArrowFlightClient::doPut() call.
 */
struct FlightPutResult {
    bool success = false;
    std::string message;
    /** Number of rows accepted by the server. */
    int64_t rows_accepted = 0;
    /** Number of bytes transferred. */
    int64_t bytes_transferred = 0;
};

// ---------------------------------------------------------------------------
// ArrowFlightServer
// ---------------------------------------------------------------------------

/**
 * @brief Arrow Flight server that exposes ArrowRecordBatch datasets.
 *
 * Servers register named datasets (or query handlers) which clients can
 * enumerate via listFlights() and retrieve via doGet().  Clients may also
 * push data to the server via doPut().
 *
 * The concrete implementation returned by create() is an in-process server
 * when `register_inprocess == true`; when `THEMIS_HAS_ARROW_FLIGHT` is
 * defined a gRPC-based transport is used in addition.
 *
 * Thread-safety: start()/stop()/registerDataset() are thread-safe.
 */
class ArrowFlightServer {
public:
    virtual ~ArrowFlightServer() = default;

    /// @brief Move constructor for polymorphic ArrowFlightServer base.
    /// @note Move semantics: abstract base carries no data members; derived classes must delegate here.
    ArrowFlightServer(ArrowFlightServer&&) noexcept = default;

    /// @brief Move assignment operator for polymorphic ArrowFlightServer base.
    /// @note Move semantics: no-op on data-less abstract base; derived classes extend this.
    ArrowFlightServer& operator=(ArrowFlightServer&&) noexcept = default;

    ArrowFlightServer(const ArrowFlightServer&) = delete;
    ArrowFlightServer& operator=(const ArrowFlightServer&) = delete;

protected:
    ArrowFlightServer() = default;

public:
    /**
     * @brief Create a new server instance.
     * @param opts Server configuration.
     * @return Owning pointer to the server (not yet started).
     */
    static std::unique_ptr<ArrowFlightServer> create(
        const FlightServerOptions& opts = FlightServerOptions{});

    /**
     * @brief Start serving requests.
     * @throws std::runtime_error if the server cannot bind to the port.
     */
    virtual void start() = 0;

    /**
     * @brief Stop the server and release resources.
     */
    virtual void stop() = 0;

    /**
     * @brief Returns true if the server is currently running.
     */
    [[nodiscard]] virtual bool isRunning() const = 0;

    /**
     * @brief Register a dataset producer under the given path.
     *
     * The @p producer callback is invoked each time a client calls doGet()
     * for this path.  The returned batch is sent to the client.
     *
     * @param path        Hierarchical path components.
     * @param producer    Callable that produces the RecordBatch on demand.
     * @param total_rows  Hint for listFlights() (-1 = unknown).
     */
    virtual void registerDataset(
        std::vector<std::string> path,
        std::function<themis::analytics::ArrowRecordBatch()> producer,
        int64_t total_rows = -1) = 0;

    /**
     * @brief Register a put handler under the given path.
     *
     * The @p handler callback is invoked when a client calls doPut() for
     * this path.  The batch pushed by the client is passed to the handler.
     *
     * @param path    Hierarchical path components.
     * @param handler Callable that processes the incoming batch.
     */
    virtual void registerPutHandler(
        std::vector<std::string> path,
        std::function<void(themis::analytics::ArrowRecordBatch)> handler) = 0;

    /**
     * @brief Remove a previously registered dataset or put handler.
     */
    virtual void unregisterDataset(const std::vector<std::string>& path) = 0;

    /**
     * @brief List currently registered datasets.
     */
    [[nodiscard]] virtual std::vector<FlightInfo> listRegisteredDatasets() const = 0;

    /**
     * @brief Returns the server endpoint URL (e.g. "grpc://0.0.0.0:8815").
     */
    [[nodiscard]] virtual std::string endpointUrl() const = 0;
};

// ---------------------------------------------------------------------------
// ArrowFlightClient
// ---------------------------------------------------------------------------

/**
 * @brief Arrow Flight client for retrieving and pushing analytics data.
 *
 * Connects to an ArrowFlightServer and exposes the standard Flight
 * operations: listFlights, doGet, and doPut.
 *
 * Thread-safety: individual method calls are thread-safe.
 */
class ArrowFlightClient {
public:
    virtual ~ArrowFlightClient() = default;

    /// @brief Move constructor for polymorphic ArrowFlightClient base.
    /// @note Move semantics: abstract base carries no data members; derived classes must delegate here.
    ArrowFlightClient(ArrowFlightClient&&) noexcept = default;

    /// @brief Move assignment operator for polymorphic ArrowFlightClient base.
    /// @note Move semantics: no-op on data-less abstract base; derived classes extend this.
    ArrowFlightClient& operator=(ArrowFlightClient&&) noexcept = default;

    ArrowFlightClient(const ArrowFlightClient&) = delete;
    ArrowFlightClient& operator=(const ArrowFlightClient&) = delete;

protected:
    ArrowFlightClient() = default;

public:
    /**
     * @brief Connect to an Arrow Flight server.
     * @param opts Connection configuration.
     * @return Connected client instance.
     * @throws std::runtime_error if the connection cannot be established.
     */
    static std::unique_ptr<ArrowFlightClient> connect(
        const FlightClientOptions& opts = FlightClientOptions{});

    /**
     * @brief Enumerate datasets available on the remote server.
     * @param call_opts Per-call options (timeout, metadata).
     * @return Vector of FlightInfo descriptors.
     */
    [[nodiscard]] virtual std::vector<FlightInfo> listFlights(
        const FlightCallOptions& call_opts = {}) = 0;

    /**
     * @brief Retrieve a RecordBatch from the server.
     * @param descriptor Identifies the dataset to fetch.
     * @param call_opts  Per-call options.
     * @return The RecordBatch produced by the server.
     * @throws std::runtime_error if the dataset is not found or the call fails.
     */
    [[nodiscard]] virtual themis::analytics::ArrowRecordBatch doGet(
        const FlightDescriptor& descriptor,
        const FlightCallOptions& call_opts = {}) = 0;

    /**
     * @brief Push a RecordBatch to the server.
     * @param batch      Data to send.
     * @param descriptor Target path on the server.
     * @param call_opts  Per-call options.
     * @return Result of the put operation.
     */
    [[nodiscard]] virtual FlightPutResult doPut(
        const themis::analytics::ArrowRecordBatch& batch,
        const FlightDescriptor& descriptor,
        const FlightCallOptions& call_opts = {}) = 0;

    /**
     * @brief Close the connection.
     */
    virtual void close() = 0;

    /**
     * @brief Returns true if the client is connected.
     */
    [[nodiscard]] virtual bool isConnected() const = 0;
};

} // namespace analytics
} // namespace themisdb
