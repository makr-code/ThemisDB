/**
 * @file round_trip_editor.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/*
 * ThemisDB — Document Module
 *
 * File:    round_trip_editor.cpp
 * Module:  src/document/
 * Purpose: Store-backed persistence for DELEGATE-52 round-trip snapshots.
 *
 * Version: 1.0.0
 * SPDX-License-Identifier: Apache-2.0
 */

#include "document/round_trip_editor.h"

#include <chrono>
#include <iomanip>
#include <sstream>

namespace themis::document {

namespace {

int64_t nowMs() noexcept {
    using namespace std::chrono;
    // Wall-clock timestamp is intentional for persisted audit/debug visibility.
    // Monotonic ordering is not required for snapshot IDs (they are index-based).
    return duration_cast<milliseconds>(
               system_clock::now().time_since_epoch())
        .count();
}

} // namespace

StoreBackedRoundTripEditor::StoreBackedRoundTripEditor(IDocumentStore& store,
                                                       CollectionId collection)
    : store_(store), collection_(std::move(collection)) {}

Result<void> StoreBackedRoundTripEditor::beginRelay(const std::string& relay_id,
                                                    const std::string& seed_document) {
    DocumentRecord record;
    record.id = makeSnapshotId(relay_id, 0);
    record.collection_id = collection_;
    record.body = nlohmann::json{
        {"relay_id", relay_id},
        {"interaction_index", 0},
        {"instruction", "seed"},
        {"document", seed_document},
        {"created_at_ms", nowMs()}};

    auto put_res = store_.put(record);
    if (!put_res) {
        return tl::unexpected(put_res.error());
    }
    return {};
}

Result<void> StoreBackedRoundTripEditor::saveInteraction(
    const std::string& relay_id,
    std::size_t interaction_index,
    const std::string& instruction,
    const std::string& document) {
    DocumentRecord record;
    record.id = makeSnapshotId(relay_id, interaction_index);
    record.collection_id = collection_;
    record.body = nlohmann::json{
        {"relay_id", relay_id},
        {"interaction_index", interaction_index},
        {"instruction", instruction},
        {"document", document},
        {"created_at_ms", nowMs()}};

    auto put_res = store_.put(record);
    if (!put_res) {
        return tl::unexpected(put_res.error());
    }
    return {};
}

Result<std::optional<RoundTripSnapshot>> StoreBackedRoundTripEditor::loadInteraction(
    const std::string& relay_id,
    std::size_t interaction_index) const {
    auto get_res = store_.get(collection_, makeSnapshotId(relay_id, interaction_index));
    if (!get_res) {
        return tl::unexpected(get_res.error());
    }
    if (!get_res->has_value()) {
        return std::optional<RoundTripSnapshot>{std::nullopt};
    }

    const auto& body = get_res->value().body;
    RoundTripSnapshot snap;
    snap.relay_id = body.value("relay_id", relay_id);
    snap.interaction_index = body.value("interaction_index", interaction_index);
    snap.instruction = body.value("instruction", std::string{});
    snap.document = body.value("document", std::string{});
    return std::optional<RoundTripSnapshot>{std::move(snap)};
}

Result<std::size_t> StoreBackedRoundTripEditor::countSnapshots(
    const std::string& relay_id) const {
    auto list_res = store_.list(collection_);
    if (!list_res) {
        return tl::unexpected(list_res.error());
    }

    const auto prefix = relay_id + ":";
    std::size_t count = 0;
    for (const auto& id : *list_res) {
        if (id.rfind(prefix, 0) == 0) {
            ++count;
        }
    }
    return count;
}

std::string StoreBackedRoundTripEditor::makeSnapshotId(const std::string& relay_id,
                                                       std::size_t interaction_index) const {
    std::ostringstream oss = {};
    oss << relay_id << ':' << std::setw(10) << std::setfill('0') << interaction_index;
    return oss.str();
}

} // namespace themis::document
