/**
 * @file round_trip_editor.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/*
 * ThemisDB — Document Module
 *
 * File:    round_trip_editor.h
 * Module:  include/document/
 * Purpose: Store-backed persistence for DELEGATE-52 round-trip document
 *          interactions (seed + intermediate versions).
 *
 * Version: 1.0.0
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "document/document_store.h"

#include <cstddef>
#include <optional>
#include <string>

namespace themis::document {

inline constexpr const char* kDefaultRoundTripCollection = "delegate_round_trip";

/**
 * @brief Persisted snapshot for one round-trip interaction.
 */
struct RoundTripSnapshot {
    std::string relay_id;           ///< Logical relay identifier
    std::size_t interaction_index;  ///< 0 = seed, >0 = edit interactions
    std::string instruction;        ///< Edit instruction used for this interaction
    std::string document;           ///< Serialized document content
};

/**
 * @brief Persistence interface for round-trip relay snapshots.
 *
 * Used by `themis::rag::delegate_eval::RoundTripSimulator` to store
 * intermediate document versions through a document-store backend.
 */
class IRoundTripEditor {
public:
    virtual ~IRoundTripEditor() = default;

    /**
     * @brief Initialize persistence for a relay and store the seed snapshot.
     *
     * @param relay_id Unique relay identifier.
     * @param seed_document Original document before any edit.
     * @return Success or storage-layer error from `IDocumentStore`.
     */
    [[nodiscard]] virtual Result<void> beginRelay(const std::string& relay_id,
                                                  const std::string& seed_document) = 0;

    /**
     * @brief Persist one interaction snapshot.
     *
     * @param relay_id Relay identifier previously passed to beginRelay().
     * @param interaction_index 1-based interaction index.
     * @param instruction Edit instruction that produced this snapshot.
     * @param document Snapshot document content.
     * @return Success or storage-layer error from `IDocumentStore`.
     */
    [[nodiscard]] virtual Result<void> saveInteraction(
        const std::string& relay_id,
        std::size_t interaction_index,
        const std::string& instruction,
        const std::string& document) = 0;

    /**
     * @brief Load one persisted interaction snapshot.
     *
     * @param relay_id Relay identifier.
     * @param interaction_index Snapshot index (0 = seed).
     * @return Optional snapshot when found, std::nullopt when missing.
     */
    [[nodiscard]] virtual Result<std::optional<RoundTripSnapshot>> loadInteraction(
        const std::string& relay_id,
        std::size_t interaction_index) const = 0;

    /**
     * @brief Count persisted snapshots for a relay (including seed snapshot).
     */
    [[nodiscard]] virtual Result<std::size_t> countSnapshots(
        const std::string& relay_id) const = 0;
};

/**
 * @brief `IRoundTripEditor` implementation backed by `IDocumentStore`.
 *
 * Snapshots are stored in one collection (`delegate_round_trip` by default),
 * encoded as JSON document bodies:
 * `{ relay_id, interaction_index, instruction, document }`.
 *
 * @note Snapshot IDs are encoded as `relay_id:NNNNNNNNNN` (10-digit zero-padded
 *       interaction index). The index width is fixed for lexical ordering.
 */
class StoreBackedRoundTripEditor final : public IRoundTripEditor {
public:
    /**
     * @brief Construct with backing store and optional collection name.
     *
     * @param store Non-owning store reference. Must outlive this editor.
     * @param collection Collection used for snapshot documents.
     */
    explicit StoreBackedRoundTripEditor(IDocumentStore& store,
                                        CollectionId collection = kDefaultRoundTripCollection);

    [[nodiscard]] Result<void> beginRelay(const std::string& relay_id,
                                          const std::string& seed_document) override;

    [[nodiscard]] Result<void> saveInteraction(
        const std::string& relay_id,
        std::size_t interaction_index,
        const std::string& instruction,
        const std::string& document) override;

    [[nodiscard]] Result<std::optional<RoundTripSnapshot>> loadInteraction(
        const std::string& relay_id,
        std::size_t interaction_index) const override;

    [[nodiscard]] Result<std::size_t> countSnapshots(
        const std::string& relay_id) const override;

private:
    [[nodiscard]] std::string makeSnapshotId(const std::string& relay_id,
                                             std::size_t interaction_index) const;

    IDocumentStore& store_;
    CollectionId collection_;
};

} // namespace themis::document
