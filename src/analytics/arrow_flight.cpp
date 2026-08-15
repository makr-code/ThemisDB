/**
 * @file arrow_flight.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=1, M=15, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/**
 * ThemisDB Arrow Flight RPC - Implementation
 *
 * Provides in-process Arrow Flight server/client with optional native
 * Arrow Flight (gRPC) transport when THEMIS_HAS_ARROW_FLIGHT is defined.
 *
 * In-process transport (always available):
 *   A process-local registry maps path descriptors to dataset producers and
 *   put handlers.  The in-process client resolves the server directly from
 *   the registry, enabling zero-overhead data transfer within a single
 *   process and making unit tests trivially fast.
 *
 * Native Arrow Flight transport (THEMIS_HAS_ARROW_FLIGHT):
 *   When the Apache Arrow Flight C++ library is detected at build time the
 *   server additionally starts a gRPC listener and the client falls back to
 *   a real network connection when no in-process server is found at the
 *   requested endpoint.  Cross-process / cross-host data transfer then uses
 *   the standard Arrow Flight gRPC protocol, enabling zero-copy interop
 *   with Pandas, DuckDB, Spark, and any other Flight-capable tool.
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include "analytics/arrow_flight.h"

#include <algorithm>
#include <atomic>
#include <mutex>
#include <spdlog/spdlog.h>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

// ---------------------------------------------------------------------------
// Optional native Arrow Flight headers
// ---------------------------------------------------------------------------
#ifdef THEMIS_HAS_ARROW_FLIGHT
#include <arrow/api.h>
#include <arrow/flight/api.h>
#include <thread>
#endif

namespace themisdb {
namespace analytics {

// ===========================================================================
// FlightDescriptor helpers
// ===========================================================================

std::string FlightDescriptor::toString() const {
    std::ostringstream oss;
    if (type == Type::PATH) {
        oss << "path://";
        for (size_t i = 0; i < path.size(); ++i) {
            if (i > 0) {
                oss << '/';
            }
            oss << path[i];
        }
    } else {
        oss << "cmd:[" << command << ']';
    }
    return oss.str();
}

// ===========================================================================
// InProcessRegistry – shared across all in-process servers/clients
// ===========================================================================

namespace {

using RecordBatch = themis::analytics::ArrowRecordBatch;

/** Convert a path vector to a flat registry key string. */
static std::string pathToKey(const std::vector<std::string> &path) {
    std::ostringstream oss;
    for (size_t i = 0; i < path.size(); ++i) {
        if (i > 0) {
            oss << '/';
        }
        oss << path[i];
    }
    return oss.str();
}

/** Convert a FlightDescriptor to a registry lookup key. */
static std::string descriptorToKey(const FlightDescriptor &desc) {
    return (desc.type == FlightDescriptor::Type::PATH) ? pathToKey(desc.path) : desc.command;
}

/** A registered dataset entry. */
struct DatasetEntry {
    std::function<RecordBatch()> producer;
    std::function<void(RecordBatch)> put_handler;
    int64_t total_rows = -1;
};

/**
 * Process-wide singleton registry.
 *
 * Maps  endpoint -> { key -> DatasetEntry }
 *
 * Multiple in-process servers can coexist on different host:port endpoints
 * without collision.
 */
class InProcessRegistry {
  public:
    static InProcessRegistry &instance() {
        static InProcessRegistry reg;
        return reg;
    }

    // ------------------------------------------------------------------
    // Server lifetime
    // ------------------------------------------------------------------

    void registerServer(const std::string &endpoint) {
        std::lock_guard<std::mutex> lk(mutex_);
        servers_.emplace(endpoint, std::unordered_map<std::string, DatasetEntry>{});
        spdlog::debug("[ArrowFlight] registered in-process server: {}", endpoint);
    }

    void unregisterServer(const std::string &endpoint) {
        std::lock_guard<std::mutex> lk(mutex_);
        servers_.erase(endpoint);
        spdlog::debug("[ArrowFlight] unregistered in-process server: {}", endpoint);
    }

    bool hasServer(const std::string &endpoint) const {
        std::lock_guard<std::mutex> lk(mutex_);
        return servers_.count(endpoint) > 0;
    }

    // ------------------------------------------------------------------
    // Dataset management
    // ------------------------------------------------------------------

    void addDataset(const std::string &endpoint, const std::string &key, DatasetEntry entry) {
        std::lock_guard<std::mutex> lk(mutex_);
        servers_[endpoint][key] = std::move(entry);
        spdlog::debug("[ArrowFlight] registered dataset '{}' on '{}'", key, endpoint);
    }

    void removeDataset(const std::string &endpoint, const std::string &key) {
        std::lock_guard<std::mutex> lk(mutex_);
        auto it = servers_.find(endpoint);
        if (it != servers_.end()) {
            it->second.erase(key);
        }
    }

    // ------------------------------------------------------------------
    // Client operations
    // ------------------------------------------------------------------

    std::vector<FlightInfo> listFlights(const std::string &endpoint) const {
        std::lock_guard<std::mutex> lk(mutex_);
        std::vector<FlightInfo> infos;
        auto sit = servers_.find(endpoint);
        if (sit == servers_.end()) {
            return infos;
        }
        for (const auto &[key, entry] : sit->second) {
            if (!entry.producer) {
                continue; // put-only handler, skip listing
            }
            FlightInfo fi;
            // Reconstruct path components from the slash-separated key
            std::vector<std::string> parts;
            std::istringstream iss(key);
            std::string part;
            while (std::getline(iss, part, '/')) {
                if (!part.empty()) {
                    parts.push_back(part);
                }
            }
            fi.descriptor    = FlightDescriptor::fromPath(std::move(parts));
            fi.total_records = entry.total_rows;
            infos.push_back(std::move(fi));
        }
        return infos;
    }

    RecordBatch doGet(const std::string &endpoint, const FlightDescriptor &descriptor) const {
        // Copy the producer function under the lock so we can invoke it
        // outside the lock.  Calling an arbitrary user callback while holding
        // the registry mutex risks a deadlock if the callback itself registers
        // or unregisters datasets.
        std::function<RecordBatch()> producer;
        {
            std::lock_guard<std::mutex> lk(mutex_);
            auto sit = servers_.find(endpoint);
            if (sit == servers_.end()) {
                throw std::runtime_error("[ArrowFlight] no server at endpoint: " + endpoint);
            }
            const std::string key = descriptorToKey(descriptor);
            auto dit              = sit->second.find(key);
            if (dit == sit->second.end() || !dit->second.producer) {
                throw std::runtime_error("[ArrowFlight] dataset not found: " + descriptor.toString());
            }
            producer = dit->second.producer; // cheap shared_ptr copy
        }
        spdlog::debug("[ArrowFlight] doGet '{}' from '{}'", descriptorToKey(descriptor), endpoint);
        return producer(); // invoked outside the lock
    }

    FlightPutResult doPut(const std::string &endpoint, const FlightDescriptor &descriptor, RecordBatch batch) {
        // Copy the handler function under the lock, then invoke outside.
        // This prevents a deadlock if the handler itself registers or
        // unregisters datasets on the registry.
        std::function<void(RecordBatch)> handler;
        {
            std::lock_guard<std::mutex> lk(mutex_);
            auto sit = servers_.find(endpoint);
            if (sit == servers_.end()) {
                return {false, "[ArrowFlight] no server at endpoint: " + endpoint, 0, 0};
            }
            const std::string key = descriptorToKey(descriptor);
            auto dit              = sit->second.find(key);
            if (dit == sit->second.end() || !dit->second.put_handler) {
                return {false, "[ArrowFlight] no put handler for: " + descriptor.toString(), 0, 0};
            }
            handler = dit->second.put_handler; // cheap shared_ptr copy
        }
        const int64_t rows = static_cast<int64_t>(batch.rowCount());
        spdlog::debug("[ArrowFlight] doPut '{}' to '{}' ({} rows)", descriptorToKey(descriptor), endpoint, rows);
        handler(std::move(batch)); // invoked outside the lock
        return {true, "OK", rows, 0};
    }

  private:
    InProcessRegistry() = default;
    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::unordered_map<std::string, DatasetEntry>> servers_;
};

/** Build a canonical endpoint string from host + port. */
static std::string buildEndpoint(const std::string &host, int port) {
    return host + ':' + std::to_string(port);
}

// ---------------------------------------------------------------------------
// Arrow conversion helpers (used by THEMIS_HAS_ARROW_FLIGHT paths)
// ---------------------------------------------------------------------------
#ifdef THEMIS_HAS_ARROW_FLIGHT

/**
 * Convert a ThemisDB ArrowRecordBatch to an Apache Arrow RecordBatch.
 *
 * INT64/TIMESTAMP columns use zero-copy Buffer::Wrap; DOUBLE columns also
 * use zero-copy wrapping.  STRING and BOOLEAN columns use Arrow builders.
 *
 * @warning The returned arrow::RecordBatch holds non-owning buffer views
 *          into @p tb's typed buffers (int64_buffer / double_buffer).
 *          The caller MUST keep @p tb alive for at least as long as the
 *          returned RecordBatch (or any array derived from it) is in use.
 *          Columns with nulls always copy data via an Arrow builder, so
 *          this lifetime constraint only applies to all-valid columns.
 */
static arrow::Result<std::shared_ptr<arrow::RecordBatch>> toArrowBatch(const RecordBatch &tb) {
    std::vector<std::shared_ptr<arrow::Field>> fields;
    std::vector<std::shared_ptr<arrow::Array>> arrays;

    for (size_t ci = 0; ci < tb.columnCount(); ++ci) {
        const auto &col = tb.getColumn(ci);
        using DT        = themis::analytics::ArrowRecordBatch::DataType;

        switch (col.schema.type) {
            case DT::INT64:
            case DT::TIMESTAMP: {
                fields.push_back(arrow::field(col.schema.name, arrow::int64(), col.schema.nullable));
                const int64_t *raw = tb.getInt64Data(ci);
                if (raw && !col.null_bitmap.empty()
                    && std::none_of(col.null_bitmap.begin(), col.null_bitmap.end(), [](bool b) { return b; })) {
                    // All-valid: zero-copy wrap
                    auto buf = arrow::Buffer::Wrap(raw, static_cast<int64_t>(tb.rowCount()));
                    arrays.push_back(std::make_shared<arrow::Int64Array>(static_cast<int64_t>(tb.rowCount()), buf));
                } else {
                    arrow::Int64Builder builder;
                    for (size_t ri = 0; ri < tb.rowCount(); ++ri) {
                        if (!col.null_bitmap.empty() && col.null_bitmap[ri]) {
                            ARROW_RETURN_NOT_OK(builder.AppendNull());
                        } else {
                            ARROW_RETURN_NOT_OK(builder.Append(col.int64_buffer[ri]));
                        }
                    }
                    std::shared_ptr<arrow::Array> arr;
                    ARROW_RETURN_NOT_OK(builder.Finish(&arr));
                    arrays.push_back(arr);
                }
                break;
            }
            case DT::DOUBLE: {
                fields.push_back(arrow::field(col.schema.name, arrow::float64(), col.schema.nullable));
                const double *raw = tb.getDoubleData(ci);
                if (raw && !col.null_bitmap.empty()
                    && std::none_of(col.null_bitmap.begin(), col.null_bitmap.end(), [](bool b) { return b; })) {
                    // All-valid: zero-copy wrap
                    auto buf = arrow::Buffer::Wrap(raw, static_cast<int64_t>(tb.rowCount()));
                    arrays.push_back(std::make_shared<arrow::DoubleArray>(static_cast<int64_t>(tb.rowCount()), buf));
                } else {
                    arrow::DoubleBuilder builder;
                    for (size_t ri = 0; ri < tb.rowCount(); ++ri) {
                        if (!col.null_bitmap.empty() && col.null_bitmap[ri]) {
                            ARROW_RETURN_NOT_OK(builder.AppendNull());
                        } else {
                            ARROW_RETURN_NOT_OK(builder.Append(col.double_buffer[ri]));
                        }
                    }
                    std::shared_ptr<arrow::Array> arr;
                    ARROW_RETURN_NOT_OK(builder.Finish(&arr));
                    arrays.push_back(arr);
                }
                break;
            }
            case DT::STRING: {
                fields.push_back(arrow::field(col.schema.name, arrow::utf8(), col.schema.nullable));
                arrow::StringBuilder builder;
                for (size_t ri = 0; ri < tb.rowCount(); ++ri) {
                    if (!col.null_bitmap.empty() && col.null_bitmap[ri]) {
                        ARROW_RETURN_NOT_OK(builder.AppendNull());
                    } else {
                        const auto &v = col.data[ri];
                        if (const auto *s = std::get_if<std::string>(&v)) {
                            ARROW_RETURN_NOT_OK(builder.Append(*s));
                        } else {
                            ARROW_RETURN_NOT_OK(builder.AppendNull());
                        }
                    }
                }
                std::shared_ptr<arrow::Array> arr;
                ARROW_RETURN_NOT_OK(builder.Finish(&arr));
                arrays.push_back(arr);
                break;
            }
            case DT::BOOLEAN: {
                fields.push_back(arrow::field(col.schema.name, arrow::boolean(), col.schema.nullable));
                arrow::BooleanBuilder builder;
                for (size_t ri = 0; ri < tb.rowCount(); ++ri) {
                    if (!col.null_bitmap.empty() && col.null_bitmap[ri]) {
                        ARROW_RETURN_NOT_OK(builder.AppendNull());
                    } else {
                        const auto &v = col.data[ri];
                        if (const auto *b = std::get_if<bool>(&v)) {
                            ARROW_RETURN_NOT_OK(builder.Append(*b));
                        } else {
                            ARROW_RETURN_NOT_OK(builder.AppendNull());
                        }
                    }
                }
                std::shared_ptr<arrow::Array> arr;
                ARROW_RETURN_NOT_OK(builder.Finish(&arr));
                arrays.push_back(arr);
                break;
            }
        }
    }

    auto schema = arrow::schema(fields);
    return arrow::RecordBatch::Make(schema, static_cast<int64_t>(tb.rowCount()), arrays);
}

/**
 * Append rows from an Apache Arrow RecordBatch into a ThemisDB RecordBatch.
 *
 * If @p tb is empty the schema is inferred from @p ab; otherwise the
 * existing schema is assumed to be compatible (no validation performed).
 */
static void appendFromArrowBatch(const arrow::RecordBatch &ab, RecordBatch &tb) {
    if (tb.columnCount() == 0) {
        // First chunk – infer schema
        for (int ci = 0; ci < ab.schema()->num_fields(); ++ci) {
            const auto &f = ab.schema()->field(ci);
            themis::analytics::ArrowRecordBatch::ColumnSchema cs;
            cs.name        = f->name();
            cs.nullable    = f->nullable();
            const auto tid = f->type()->id();
            using DT       = themis::analytics::ArrowRecordBatch::DataType;
            if (tid == arrow::Type::INT64 || tid == arrow::Type::TIMESTAMP) {
                cs.type = DT::INT64;
            } else if (tid == arrow::Type::DOUBLE || tid == arrow::Type::FLOAT) {
                cs.type = DT::DOUBLE;
            } else if (tid == arrow::Type::BOOL) {
                cs.type = DT::BOOLEAN;
            } else {
                cs.type = DT::STRING;
            }
            tb.addColumn(cs);
        }
    }

    for (int ri = 0; ri < ab.num_rows(); ++ri) {
        std::vector<std::variant<std::nullptr_t, int64_t, double, std::string, bool>> row;
        for (int ci = 0; ci < ab.num_columns(); ++ci) {
            const auto &arr = ab.column(ci);
            if (arr->IsNull(ri)) {
                row.emplace_back(nullptr);
                continue;
            }
            switch (arr->type_id()) {
                case arrow::Type::INT64:
                case arrow::Type::TIMESTAMP: {
                    auto *a = static_cast<const arrow::Int64Array *>(arr.get());
                    row.emplace_back(a->Value(ri));
                    break;
                }
                case arrow::Type::DOUBLE:
                case arrow::Type::FLOAT: {
                    auto *a = static_cast<const arrow::DoubleArray *>(arr.get());
                    row.emplace_back(a->Value(ri));
                    break;
                }
                case arrow::Type::BOOL: {
                    auto *a = static_cast<const arrow::BooleanArray *>(arr.get());
                    row.emplace_back(a->Value(ri));
                    break;
                }
                default: {
                    auto sr = arr->GetScalar(ri);
                    row.emplace_back(sr.ok() ? sr.ValueOrDie()->ToString() : std::string{});
                    break;
                }
            }
        }
        tb.appendRow(row);
    }
}

#endif // THEMIS_HAS_ARROW_FLIGHT

} // anonymous namespace

// ===========================================================================
// InProcessArrowFlightServer
// ===========================================================================

/** @brief InProcessArrowFlightServer. */
class InProcessArrowFlightServer final : public ArrowFlightServer {
  public:
    explicit InProcessArrowFlightServer(FlightServerOptions opts)
        : opts_(std::move(opts)), endpoint_(buildEndpoint(opts_.host, opts_.port)), running_(false) {}

    ~InProcessArrowFlightServer() override {
        if (running_) {
            stop();
        }
    }

    // ------------------------------------------------------------------
    void start() override {
        if (running_.exchange(true)) {
            return; // already running
        }
        if (opts_.register_inprocess) {
            InProcessRegistry::instance().registerServer(endpoint_);
        }
        spdlog::info("[ArrowFlight] server started at '{}'", endpoint_);
#ifdef THEMIS_HAS_ARROW_FLIGHT
        startNativeFlight();
#endif
    }

    void stop() override {
        if (!running_.exchange(false)) {
            return; // already stopped
        }
#ifdef THEMIS_HAS_ARROW_FLIGHT
        stopNativeFlight();
#endif
        if (opts_.register_inprocess) {
            InProcessRegistry::instance().unregisterServer(endpoint_);
        }
        spdlog::info("[ArrowFlight] server stopped at '{}'", endpoint_);
    }

    bool isRunning() const override {
        return running_.load();
    }

    // ------------------------------------------------------------------
    void registerDataset(std::vector<std::string> path, std::function<RecordBatch()> producer,
                         int64_t total_rows) override {
        DatasetEntry entry;
        entry.producer   = std::move(producer);
        entry.total_rows = total_rows;
        InProcessRegistry::instance().addDataset(endpoint_, pathToKey(path), std::move(entry));
    }

    void registerPutHandler(std::vector<std::string> path, std::function<void(RecordBatch)> handler) override {
        DatasetEntry entry;
        entry.put_handler = std::move(handler);
        InProcessRegistry::instance().addDataset(endpoint_, pathToKey(path), std::move(entry));
    }

    void unregisterDataset(const std::vector<std::string> &path) override {
        InProcessRegistry::instance().removeDataset(endpoint_, pathToKey(path));
    }

    std::vector<FlightInfo> listRegisteredDatasets() const override {
        return InProcessRegistry::instance().listFlights(endpoint_);
    }

    std::string endpointUrl() const override {
        return "grpc://" + endpoint_;
    }

  private:
    FlightServerOptions opts_;
    std::string endpoint_;
    std::atomic<bool> running_;

    static std::string pathToKey(const std::vector<std::string> &p) {
        return ::themisdb::analytics::pathToKey(p);
    }

    // ------------------------------------------------------------------
    // Native Arrow Flight gRPC server (only when THEMIS_HAS_ARROW_FLIGHT)
    // ------------------------------------------------------------------
#ifdef THEMIS_HAS_ARROW_FLIGHT

    /**
     * FlightServerBase implementation that bridges gRPC calls to the
     * in-process registry so the same producer/handler code executes
     * regardless of whether the call arrived from in-process or over the
     * network.
     */
    class ThemisFlightService : public arrow::flight::FlightServerBase {
      public:
        explicit ThemisFlightService(std::string endpoint) : endpoint_(std::move(endpoint)) {}

        arrow::Status ListFlights(const arrow::flight::ServerCallContext &, const arrow::flight::Criteria *,
                                  std::unique_ptr<arrow::flight::FlightListing> *out) override {
            auto infos = InProcessRegistry::instance().listFlights(endpoint_);
            std::vector<arrow::flight::FlightInfo> flight_infos;
            for (const auto &fi : infos) {
                arrow::flight::FlightDescriptor fd = arrow::flight::FlightDescriptor::Path(fi.descriptor.path);
                ARROW_ASSIGN_OR_RAISE(
                    auto ainfo, arrow::flight::FlightInfo::Make(*arrow::schema({}), fd, {}, fi.total_records, -1));
                flight_infos.push_back(std::move(ainfo));
            }
            *out = std::make_unique<arrow::flight::SimpleFlightListing>(std::move(flight_infos));
            return arrow::Status::OK();
        }

        arrow::Status GetFlightInfo(const arrow::flight::ServerCallContext &,
                                    const arrow::flight::FlightDescriptor &request,
                                    std::unique_ptr<arrow::flight::FlightInfo> *out) override {
            FlightDescriptor desc = toThemisDescriptor(request);
            // Build a ticket containing the descriptor key so DoGet can
            // look it up without a second GetFlightInfo round-trip.
            arrow::flight::Ticket ticket;
            ticket.ticket = descriptorToKey(desc);

            arrow::flight::Location loc;
            ARROW_ASSIGN_OR_RAISE(loc, arrow::flight::Location::ForGrpcTcp(
                                           /* endpoint host */ endpoint_.substr(0, endpoint_.find(':')),
                                           std::stoi(endpoint_.substr(endpoint_.find(':') + 1))));

            arrow::flight::FlightEndpoint endpoint(ticket, {loc}, std::optional<arrow::flight::Timestamp>{}, "");
            ARROW_ASSIGN_OR_RAISE(auto info,
                                  arrow::flight::FlightInfo::Make(*arrow::schema({}), request, {endpoint}, -1, -1));
            *out = std::make_unique<arrow::flight::FlightInfo>(std::move(info));
            return arrow::Status::OK();
        }

        arrow::Status DoGet(const arrow::flight::ServerCallContext &, const arrow::flight::Ticket &ticket,
                            std::unique_ptr<arrow::flight::FlightDataStream> *out) override {
            FlightDescriptor desc = FlightDescriptor::fromCommand(ticket.ticket);
            try {
                auto batch = InProcessRegistry::instance().doGet(endpoint_, desc);
                // Move batch into a shared_ptr to extend its lifetime beyond
                // this scope.  The FlightDataStream returned via *out reads
                // data lazily (after DoGet returns), so the zero-copy Arrow
                // buffers wrapping batch's typed arrays must remain valid
                // until the stream is fully consumed.
                auto shared_batch = std::make_shared<RecordBatch>(std::move(batch));
                ARROW_ASSIGN_OR_RAISE(auto ab, toArrowBatch(*shared_batch));
                // Wrap the Arrow RecordBatch in a Table whose TableBatchReader
                // holds a shared_ptr<RecordBatch>, keeping the batch alive.
                // We also keep shared_batch alive by capturing it in the
                // TableBatchReader via a custom RecordBatchReader wrapper.
                struct LifetimeHolder : public arrow::RecordBatchReader {
                    std::shared_ptr<RecordBatch> owner;
                    std::shared_ptr<arrow::RecordBatch> rb;
                    std::shared_ptr<arrow::Schema> schema_;
  // scope: moved to inner block (scope_mismatch remediation B1)
                    bool done = false;
                    LifetimeHolder(std::shared_ptr<RecordBatch> o, std::shared_ptr<arrow::RecordBatch> b)
                        : owner(std::move(o)), rb(std::move(b)), schema_(rb->schema()) {}
                    std::shared_ptr<arrow::Schema> schema() const override {
                        return schema_;
                    }
                    arrow::Status ReadNext(std::shared_ptr<arrow::RecordBatch> *out) override {
                        *out = done ? nullptr : rb;
                        done = true;
                        return arrow::Status::OK();
                    }
                };
                auto holder = std::make_shared<LifetimeHolder>(shared_batch, ab);
                *out        = std::make_unique<arrow::flight::RecordBatchStream>(std::move(holder));
                return arrow::Status::OK();
            } catch (const std::exception &ex) {
                return arrow::Status::KeyError(ex.what());
            }
        }

        arrow::Status DoPut(const arrow::flight::ServerCallContext &,
                            std::unique_ptr<arrow::flight::FlightMessageReader> reader,
                            std::unique_ptr<arrow::flight::FlightMetadataWriter>) override {
            FlightDescriptor desc = toThemisDescriptor(reader->descriptor());
            RecordBatch tb;
            ARROW_ASSIGN_OR_RAISE(auto batches, reader->ToRecordBatches());
            for (const auto &ab : batches) {
                appendFromArrowBatch(*ab, tb);
            }
            InProcessRegistry::instance().doPut(endpoint_, desc, std::move(tb));
            return arrow::Status::OK();
        }

      private:
        std::string endpoint_;

        static FlightDescriptor toThemisDescriptor(const arrow::flight::FlightDescriptor &fd) {
            if (fd.type == arrow::flight::FlightDescriptor::PATH) {
                return FlightDescriptor::fromPath(fd.path);
            }
            return FlightDescriptor::fromCommand(fd.cmd);
        }
    };

    std::unique_ptr<ThemisFlightService> flight_service_;
    std::thread flight_thread_;

    void startNativeFlight() {
        arrow::flight::Location loc;
        auto loc_result = arrow::flight::Location::ForGrpcTcp(opts_.host, opts_.port);
        if (!loc_result.ok()) {
            spdlog::warn("[ArrowFlight] failed to build gRPC location: {}", loc_result.status().ToString());
            return;
        }
        flight_service_ = std::make_unique<ThemisFlightService>(endpoint_);
        arrow::flight::FlightServerOptions srv_opts(loc_result.ValueOrDie());
        auto status = flight_service_->Init(srv_opts);
        if (!status.ok()) {
            spdlog::warn("[ArrowFlight] native flight init failed: {}", status.ToString());
            flight_service_.reset();
            return;
        }
        flight_thread_ = std::thread([this]() {
            auto serve_status = flight_service_->Serve();
            if (!serve_status.ok()) {
                spdlog::warn("[ArrowFlight] serve exited with status: {}", serve_status.ToString());
            }
        });
        spdlog::info("[ArrowFlight] native gRPC transport started at '{}'", endpoint_);
    }

    void stopNativeFlight() {
        if (flight_service_) {
            auto status = flight_service_->Shutdown();
            if (!status.ok()) {
                spdlog::warn("[ArrowFlight] shutdown warning: {}", status.ToString());
            }
        }
        if (flight_thread_.joinable()) {
            flight_thread_.join();
        }
        flight_service_.reset();
    }
#endif // THEMIS_HAS_ARROW_FLIGHT
};

// ===========================================================================
// InProcessArrowFlightClient
// ===========================================================================

/** @brief InProcessArrowFlightClient. */
class InProcessArrowFlightClient final : public ArrowFlightClient {
  public:
    explicit InProcessArrowFlightClient(FlightClientOptions opts)
        : opts_(std::move(opts)), endpoint_(buildEndpoint(opts_.host, opts_.port)), connected_(false) {
        if (opts_.prefer_inprocess && InProcessRegistry::instance().hasServer(endpoint_)) {
            connected_ = true;
            spdlog::debug("[ArrowFlight] in-process client connected to '{}'", endpoint_);
            return;
        }
#ifdef THEMIS_HAS_ARROW_FLIGHT
        connectNativeFlight();
        if (connected_) {
            return;
        }
#endif
        throw std::runtime_error("[ArrowFlight] no server found at endpoint: " + endpoint_);
    }

    ~InProcessArrowFlightClient() override {
        close();
    }

    // ------------------------------------------------------------------
    std::vector<FlightInfo> listFlights(const FlightCallOptions & /*call_opts*/) override {
        ensureConnected();
#ifdef THEMIS_HAS_ARROW_FLIGHT
        if (native_client_) {
            return listFlightsNative();
        }
#endif
        return InProcessRegistry::instance().listFlights(endpoint_);
    }

    RecordBatch doGet(const FlightDescriptor &descriptor, const FlightCallOptions & /*call_opts*/) override {
        ensureConnected();
#ifdef THEMIS_HAS_ARROW_FLIGHT
        if (native_client_) {
            return doGetNative(descriptor);
        }
#endif
        return InProcessRegistry::instance().doGet(endpoint_, descriptor);
    }

    FlightPutResult doPut(const RecordBatch &batch, const FlightDescriptor &descriptor,
                          const FlightCallOptions & /*call_opts*/) override {
        ensureConnected();
#ifdef THEMIS_HAS_ARROW_FLIGHT
        if (native_client_) {
            return doPutNative(batch, descriptor);
        }
#endif
        // Pass a copy so the caller retains ownership
        RecordBatch copy = batch;
        return InProcessRegistry::instance().doPut(endpoint_, descriptor, std::move(copy));
    }

    void close() override {
        connected_ = false;
#ifdef THEMIS_HAS_ARROW_FLIGHT
        native_client_.reset();
#endif
    }

    bool isConnected() const override {
        return connected_.load();
    }

  private:
    FlightClientOptions opts_;
    std::string endpoint_;
    std::atomic<bool> connected_;

    void ensureConnected() const {
        if (!connected_) {
            throw std::runtime_error("[ArrowFlight] client not connected to: " + endpoint_);
        }
    }

    // ------------------------------------------------------------------
    // Native Arrow Flight (THEMIS_HAS_ARROW_FLIGHT)
    // ------------------------------------------------------------------
#ifdef THEMIS_HAS_ARROW_FLIGHT
    std::unique_ptr<arrow::flight::FlightClient> native_client_;

    void connectNativeFlight() {
        auto loc_result = arrow::flight::Location::ForGrpcTcp(opts_.host, opts_.port);
        if (!loc_result.ok()) {
            spdlog::debug("[ArrowFlight] cannot build location for '{}': {}", endpoint_,
                          loc_result.status().ToString());
            return;
        }
        auto client_result = arrow::flight::FlightClient::Connect(loc_result.ValueOrDie());
        if (!client_result.ok()) {
            spdlog::debug("[ArrowFlight] native connect to '{}' failed: {}", endpoint_,
                          client_result.status().ToString());
            return;
        }

        auto candidate = std::move(client_result.ValueOrDie());

        // Probe connectivity immediately. Arrow Flight client creation can
        // succeed even when the endpoint is not actually serving requests.
        auto listing_result = candidate->ListFlights();
        if (!listing_result.ok()) {
            spdlog::debug("[ArrowFlight] native probe to '{}' failed: {}", endpoint_,
                          listing_result.status().ToString());
            return;
        }
        auto &listing     = *listing_result.ValueOrDie();
        auto first_result = listing.Next();
        if (!first_result.ok()) {
            spdlog::debug("[ArrowFlight] native probe stream to '{}' failed: {}", endpoint_,
                          first_result.status().ToString());
            return;
        }

        native_client_ = std::move(candidate);
        connected_     = true;
        spdlog::info("[ArrowFlight] native client connected to '{}'", endpoint_);
    }

    std::vector<FlightInfo> listFlightsNative() {
        std::vector<FlightInfo> result;
        auto listing_result = native_client_->ListFlights();
        if (!listing_result.ok()) {
            spdlog::warn("[ArrowFlight] ListFlights failed: {}", listing_result.status().ToString());
            return result;
        }
        auto &listing = *listing_result.ValueOrDie();
        while (true) {
            auto info_result = listing.Next();
            if (!info_result.ok() || !info_result.ValueOrDie()) {
                break;
            }
            const auto &fi = *info_result.ValueOrDie();
            FlightInfo out;
            const auto &fd = fi.descriptor();
            if (fd.type == arrow::flight::FlightDescriptor::PATH) {
                out.descriptor = FlightDescriptor::fromPath(fd.path);
            } else {
                out.descriptor = FlightDescriptor::fromCommand(fd.cmd);
            }
            out.total_records = fi.total_records();
            out.total_bytes   = fi.total_bytes();
            result.push_back(std::move(out));
        }
        return result;
    }

    RecordBatch doGetNative(const FlightDescriptor &descriptor) {
        arrow::flight::FlightDescriptor fd = toArrowDescriptor(descriptor);
        auto info_result                   = native_client_->GetFlightInfo(fd);
        if (!info_result.ok()) {
            throw std::runtime_error("[ArrowFlight] GetFlightInfo failed: " + info_result.status().ToString());
        }
        const auto &info = *info_result.ValueOrDie();
        if (info.endpoints().empty()) {
            throw std::runtime_error("[ArrowFlight] no endpoints in FlightInfo");
        }
        auto stream_result = native_client_->DoGet(info.endpoints()[0].ticket);
        if (!stream_result.ok()) {
            throw std::runtime_error("[ArrowFlight] DoGet failed: " + stream_result.status().ToString());
        }
        auto stream_reader = std::move(stream_result).ValueOrDie();
        RecordBatch result;
        while (true) {
            auto batch_result = stream_reader->Next();
            if (!batch_result.ok()) {
                throw std::runtime_error("[ArrowFlight] stream read failed: " + batch_result.status().ToString());
            }
            auto chunk = std::move(batch_result).ValueOrDie();
            if (!chunk.data) {
                break;
            }
            appendFromArrowBatch(*chunk.data, result);
        }
        return result;
    }

    FlightPutResult doPutNative(const RecordBatch &batch, const FlightDescriptor &descriptor) {
        arrow::flight::FlightDescriptor fd = toArrowDescriptor(descriptor);

        // Build schema for the writer
        std::vector<std::shared_ptr<arrow::Field>> fields;
        for (size_t ci = 0; ci < batch.columnCount(); ++ci) {
            const auto &col = batch.getColumn(ci);
            using DT        = themis::analytics::ArrowRecordBatch::DataType;
            std::shared_ptr<arrow::DataType> dt;
            switch (col.schema.type) {
                case DT::INT64:
                case DT::TIMESTAMP:
                    dt = arrow::int64();
                    break;
                case DT::DOUBLE:
                    dt = arrow::float64();
                    break;
                case DT::BOOLEAN:
                    dt = arrow::boolean();
                    break;
                default:
                    dt = arrow::utf8();
                    break;
            }
            fields.push_back(arrow::field(col.schema.name, dt, col.schema.nullable));
        }
        auto schema = arrow::schema(fields);

        auto writer_result = native_client_->DoPut(fd, schema);
        if (!writer_result.ok()) {
            return {false, "[ArrowFlight] DoPut init failed: " + writer_result.status().ToString(), 0, 0};
        }
        auto put_result = std::move(writer_result).ValueOrDie();
        auto &writer    = *put_result.writer;

        auto ab_result = toArrowBatch(batch);
        if (!ab_result.ok()) {
            return {false, "[ArrowFlight] batch conversion failed: " + ab_result.status().ToString(), 0, 0};
        }
        auto status = writer.WriteRecordBatch(*ab_result.ValueOrDie());
        if (!status.ok()) {
            return {false, "[ArrowFlight] write failed: " + status.ToString(), 0, 0};
        }
        status = writer.Close();
        return {status.ok(), status.ok() ? "OK" : status.ToString(), static_cast<int64_t>(batch.rowCount()), 0};
    }

    static arrow::flight::FlightDescriptor toArrowDescriptor(const FlightDescriptor &desc) {
        if (desc.type == FlightDescriptor::Type::PATH) {
            return arrow::flight::FlightDescriptor::Path(desc.path);
        }
        return arrow::flight::FlightDescriptor::Command(desc.command);
    }
#endif // THEMIS_HAS_ARROW_FLIGHT
};

// ===========================================================================
// Factory implementations
// ===========================================================================

std::unique_ptr<ArrowFlightServer> ArrowFlightServer::create(const FlightServerOptions &opts) {
    return std::make_unique<InProcessArrowFlightServer>(opts);
}

std::unique_ptr<ArrowFlightClient> ArrowFlightClient::connect(const FlightClientOptions &opts) {
    return std::make_unique<InProcessArrowFlightClient>(opts);
}

} // namespace analytics
} // namespace themisdb
