/**
 * STUB/SHIM: ThemisDBAdapter fallback implementation
 * ==================================================
 * Purpose: Minimal, non-production fallback used only by focused unit
 * tests when the full CHIMERA adapter implementation is not linked.
 *
 * Activation: Included into focused test targets via
 * `tests/chimera/CMakeLists.txt` through `target_sources()`.
 *
 * Production Delta: This file MUST NOT be used in production builds.
 * It intentionally returns `ErrorCode::NOT_IMPLEMENTED` for most
 * operations and provides no real persistence or engine integration.
 *
 * Removal Plan: Replace with the real `ThemisDBAdapter` implementation
 * or link the upstream CHIMERA adapter sources before merging into
 * release branches. Track removal in `src/STUB_INVENTORY.md`.
 *
 * Approval: Any change adding production logic here requires explicit
 * human approval and a documented justification in the PR body.
 *
 * Marker: "CHIMERA_FALLBACK_STUB" compile-definition is set by the
 * focused-test CMake to help tooling detect stub usage.
 */

#include "chimera/themisdb_adapter.hpp"

// Machine-readable stub tag for static scans
// STUB_TAG: CHIMERA_FALLBACK_STUB

namespace chimera {

// NOTE: Constructors and methods below intentionally minimal. Do not
// implement real production behavior here.

ThemisDBAdapter::ThemisDBAdapter(themis::QueryEngine* q, themis::VectorIndexManager* v, themis::GraphIndexManager* g)
    : query_engine_(q), vector_index_(v), graph_index_(g) {}

Result<std::string> ThemisDBAdapter::insert_document(const std::string& collection, const Document& doc) {
    return Result<std::string>::err(ErrorCode::NOT_IMPLEMENTED, "stub: insert_document");
}

Result<size_t> ThemisDBAdapter::batch_insert_documents(const std::string& collection, const std::vector<Document>& docs) {
    return Result<size_t>::err(ErrorCode::NOT_IMPLEMENTED, "stub: batch_insert_documents");
}

Result<std::vector<Document>> ThemisDBAdapter::find_documents(const std::string& collection, const std::map<std::string, Scalar>& filter, size_t limit) {
    return Result<std::vector<Document>>::err(ErrorCode::NOT_IMPLEMENTED, "stub: find_documents");
}

Result<size_t> ThemisDBAdapter::update_documents(const std::string& collection, const std::map<std::string, Scalar>& filter, const std::map<std::string, Scalar>& updates) {
    return Result<size_t>::err(ErrorCode::NOT_IMPLEMENTED, "stub: update_documents");
}

Result<std::string> ThemisDBAdapter::begin_transaction(const TransactionOptions& options) {
    return Result<std::string>::err(ErrorCode::NOT_IMPLEMENTED, "stub: begin_transaction");
}

Result<bool> ThemisDBAdapter::commit_transaction(const std::string& transaction_id) {
    return Result<bool>::err(ErrorCode::NOT_IMPLEMENTED, "stub: commit_transaction");
}

Result<bool> ThemisDBAdapter::rollback_transaction(const std::string& transaction_id) {
    return Result<bool>::err(ErrorCode::NOT_IMPLEMENTED, "stub: rollback_transaction");
}

Result<std::string> ThemisDBAdapter::create_savepoint(const std::string& transaction_id, const std::string& savepoint_name) {
    return Result<std::string>::err(ErrorCode::NOT_IMPLEMENTED, "stub: create_savepoint");
}

Result<bool> ThemisDBAdapter::rollback_to_savepoint(const std::string& transaction_id, const std::string& savepoint_name) {
    return Result<bool>::err(ErrorCode::NOT_IMPLEMENTED, "stub: rollback_to_savepoint");
}

Result<bool> ThemisDBAdapter::release_savepoint(const std::string& transaction_id, const std::string& savepoint_name) {
    return Result<bool>::err(ErrorCode::NOT_IMPLEMENTED, "stub: release_savepoint");
}

Result<TransactionStats> ThemisDBAdapter::get_transaction_stats(const std::string& transaction_id) {
    return Result<TransactionStats>::err(ErrorCode::NOT_IMPLEMENTED, "stub: get_transaction_stats");
}

Result<TransactionState> ThemisDBAdapter::get_transaction_state(const std::string& transaction_id) {
    return Result<TransactionState>::err(ErrorCode::NOT_IMPLEMENTED, "stub: get_transaction_state");
}

Result<SystemInfo> ThemisDBAdapter::get_system_info() const {
    return Result<SystemInfo>::err(ErrorCode::NOT_IMPLEMENTED, "stub: get_system_info");
}

Result<SystemMetrics> ThemisDBAdapter::get_metrics() const {
    return Result<SystemMetrics>::err(ErrorCode::NOT_IMPLEMENTED, "stub: get_metrics");
}

bool ThemisDBAdapter::has_capability(Capability cap) const {
    (void)cap;
    return false;
}

std::vector<Capability> ThemisDBAdapter::get_capabilities() const {
    return {};
}

std::future<Result<RelationalTable>> ThemisDBAdapter::execute_query_async(const std::string& query, const std::vector<Scalar>& params, const AsyncQueryOptions& opts) {
    return std::async(std::launch::deferred, [](){ return Result<RelationalTable>::err(ErrorCode::NOT_IMPLEMENTED, "stub: execute_query_async"); });
}

std::future<Result<size_t>> ThemisDBAdapter::batch_insert_async(const std::string& table_name, const std::vector<RelationalRow>& rows, std::function<void(size_t)> progress_callback, const AsyncQueryOptions& opts) {
    (void)progress_callback; (void)opts; (void)table_name; (void)rows;
    return std::async(std::launch::deferred, [](){ return Result<size_t>::err(ErrorCode::NOT_IMPLEMENTED, "stub: batch_insert_async"); });
}

std::future<Result<std::vector<std::pair<Vector,double>>>> ThemisDBAdapter::search_vectors_async(const std::string& collection, const Vector& query_vector, size_t k, const std::map<std::string, Scalar>& filters, const AsyncQueryOptions& opts) {
    (void)collection; (void)query_vector; (void)k; (void)filters; (void)opts;
    return std::async(std::launch::deferred, [](){ return Result<std::vector<std::pair<Vector,double>>>::err(ErrorCode::NOT_IMPLEMENTED, "stub: search_vectors_async"); });
}

Result<bool> ThemisDBAdapter::cancel_async(const std::string& operation_id) {
    (void)operation_id;
    return Result<bool>::err(ErrorCode::NOT_IMPLEMENTED, "stub: cancel_async");
}

Result<std::unique_ptr<IResultStream>> ThemisDBAdapter::execute_query_stream(const std::string& query, const std::vector<Scalar>& params) {
    (void)query; (void)params;
    return Result<std::unique_ptr<IResultStream>>::err(ErrorCode::NOT_IMPLEMENTED, "stub: execute_query_stream");
}

Result<bool> ThemisDBAdapter::set_stream_config(const StreamConfig& config) {
    (void)config;
    return Result<bool>::err(ErrorCode::NOT_IMPLEMENTED, "stub: set_stream_config");
}

Result<std::unique_ptr<IPreparedStatement>> ThemisDBAdapter::prepare(const std::string& query) {
    (void)query;
    return Result<std::unique_ptr<IPreparedStatement>>::err(ErrorCode::NOT_IMPLEMENTED, "stub: prepare");
}

Result<bool> ThemisDBAdapter::unprepare(const std::string& statement_id) {
    (void)statement_id;
    return Result<bool>::err(ErrorCode::NOT_IMPLEMENTED, "stub: unprepare");
}

Result<std::vector<std::string>> ThemisDBAdapter::list_prepared() {
    return Result<std::vector<std::string>>::ok({});
}

} // namespace chimera
