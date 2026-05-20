/*
 * ThemisDB | File: arrow_flight.h | Version: 0.0.15
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=1; TODO=0, Stub=0, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#pragma once

/**
 * @file arrow_flight.h
 * @brief Arrow Flight RPC support for remote analytics.
 *
 * Provides a client/server abstraction for transporting Apache Arrow
 * RecordBatches between ThemisDB nodes and external analytics tools.
 *
 * Two transport modes are available:
 *
 *  1. **In-process** (always available, no external dependencies):
 *     Client and server share a process-local registry keyed by endpoint
 *     name.  Useful for unit tests, single-node deployments, and as a
 *     fallback when the Arrow Flight library is absent.
 *
 *  2. **Native Arrow Flight** (requires compile-time flag
 *     `THEMIS_HAS_ARROW_FLIGHT` and the `arrow_flight` vcpkg component):
 *     Uses the real gRPC-based Arrow Flight protocol, enabling zero-copy
 *     data exchange with Pandas, DuckDB, Spark, and other Flight-capable
 *     clients.
 *
 * Typical server-side usage:
 * @code
 *   #include "analytics/arrow_flight.h"
 *
 *   auto server = themisdb::analytics::ArrowFlightServer::create();
 *   server->registerDataset("sales", []() { return buildSalesBatch(); });
 *   server->start({"0.0.0.0", 8815});
 *   // ... run queries ...
 *   server->stop();
 * @endcode
 *
 * Typical client-side usage:
 * @code
 *   auto client = themisdb::analytics::ArrowFlightClient::connect({"localhost", 8815});
 *   auto info = client->listFlights();
 *   auto batch = client->doGet(info[0].descriptor);
 *   client->doPut(batch, {"my_dataset"});
 * @endcode
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
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
