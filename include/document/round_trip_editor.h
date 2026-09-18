/**
 * @file round_trip_editor.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

struct RoundTripSnapshot {
    std::string relay_id;           ///< Logical relay identifier
    std::size_t interaction_index;  ///< 0 = seed, >0 = edit interactions
    std::string instruction;        ///< Edit instruction used for this interaction
    std::string document;           ///< Serialized document content
};

class IRoundTripEditor {
public:
    /**
     * @brief IRound Trip Editor.
     * @return Return value.
     */
    virtual ~IRoundTripEditor() = default;

    [[nodiscard]] virtual Result<void> beginRelay(const std::string& relay_id,
                                                  const std::string& seed_document) = 0;

    [[nodiscard]] virtual Result<void> saveInteraction(
        const std::string& relay_id,
        std::size_t interaction_index,
        const std::string& instruction,
        const std::string& document) = 0;

    [[nodiscard]] virtual Result<std::optional<RoundTripSnapshot>> loadInteraction(
        const std::string& relay_id,
        std::size_t interaction_index) const = 0;

    [[nodiscard]] virtual Result<std::size_t> countSnapshots(
        const std::string& relay_id) const = 0;
};

class StoreBackedRoundTripEditor final : public IRoundTripEditor {
public:
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
