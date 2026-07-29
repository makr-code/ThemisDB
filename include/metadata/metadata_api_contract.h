// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors
#pragma once

/**
 * @file metadata_api_contract.h
 * @brief Frozen API contract for the ThemisDB metadata module.
 * @version 1.0.0
 *
 * @section purpose Purpose
 * The metadata module manages schema catalogues, AQL bridging, and
 * metadata export/import operations.
 *
 * @section contracts API Contracts
 *
 * ### CatalogExporter
 * - `exportSchema()` serialises the complete schema graph; partial export is
 *   not allowed (all-or-nothing).
 * - Missing collection → META_COLLECTION_NOT_FOUND.
 *
 * ### AqlSchemaBridge
 * - `resolve()` maps AQL field references to physical column descriptors.
 * - Unresolvable reference → META_FIELD_NOT_FOUND (never silently returns null).
 *
 * @section error_taxonomy Error Taxonomy
 * | Code                       | Meaning                                       |
 * |----------------------------|-----------------------------------------------|
 * | META_COLLECTION_NOT_FOUND  | Requested collection not in catalogue         |
 * | META_FIELD_NOT_FOUND       | AQL field reference cannot be resolved        |
 * | META_SCHEMA_MISMATCH       | Schema version incompatible with data         |
 * | META_EXPORT_FAILED         | Serialisation to export format failed         |
 *
 * @section threading Threading Guarantees
 * - `CatalogExporter` is thread-safe for concurrent reads.
 * - Schema mutations require exclusive lock.
 *
 * @section contract_freeze Contract Freeze
 * Frozen for ThemisDB v2.x.
 */

#include <cstdint>
#include <string>

namespace themis::metadata {

enum class MetaError : int32_t {
    kCollectionNotFound = 7900,
    kFieldNotFound      = 7901,
    kSchemaMismatch     = 7902,
    kExportFailed       = 7903,
};

} // namespace themis::metadata
