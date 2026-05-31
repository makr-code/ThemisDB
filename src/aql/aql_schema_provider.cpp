/*
 * ThemisDB | File: aql_schema_provider.cpp | Version: 0.0.15 | Last Modified: 2026-05-21 16:50:40
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 55
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * PR History (last 5): none
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#include "aql/aql_schema_provider.h"

#include <sstream>

namespace themis {
namespace aql {

std::string formatSchemaContext(const std::vector<CollectionMetadata> &schema) {
    if (schema.empty()) {
        return {};
    }

    std::ostringstream oss;
    oss << "Available collections:\n";

    for (const auto &col : schema) {
        oss << "- " << col.name;
        if (!col.type.empty()) {
            oss << " (" << col.type << ")";
        }
        if (col.estimated_count > 0) {
            oss << " [~" << col.estimated_count << " documents]";
        }
        if (!col.fields.empty()) {
            oss << "\n  Fields:";
            for (const auto &f : col.fields) {
                oss << "\n    - " << f.name << " [" << f.type << "]";
                if (f.indexed) {
                    oss << " (indexed)";
                }
                if (!f.nullable) {
                    oss << " (required)";
                }
            }
        }
        oss << "\n";
    }

    return oss.str();
}

} // namespace aql
} // namespace themis
