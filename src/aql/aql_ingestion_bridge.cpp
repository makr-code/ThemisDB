/**
 * @file aql_ingestion_bridge.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.1.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=0, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "aql/aql_ingestion_bridge.h"

#include <sstream>
#include <stdexcept>

namespace themis {
namespace aql {

// ─────────────────────────────────────────────────────────────────────────────
// Construction / destruction
// ─────────────────────────────────────────────────────────────────────────────

AQLIngestionBridge::AQLIngestionBridge(
    std::shared_ptr<toolbox::IngestionToolbox> toolbox,
    std::shared_ptr<ingestion::IGraphWriter>   graph_writer,
    std::string                                text_field_key)
    : toolbox_(std::move(toolbox))
    , graph_writer_(std::move(graph_writer))
    , text_field_key_(std::move(text_field_key))
{
    if (!toolbox_) {
        throw std::invalid_argument(
            "AQLIngestionBridge: toolbox must not be null");
    }
}

AQLIngestionBridge::~AQLIngestionBridge() = default;

AQLIngestionBridge::AQLIngestionBridge(AQLIngestionBridge&&) noexcept = default;
AQLIngestionBridge& AQLIngestionBridge::operator=(AQLIngestionBridge&&) noexcept = default;

// ─────────────────────────────────────────────────────────────────────────────
// Core operations
// ─────────────────────────────────────────────────────────────────────────────

std::string AQLIngestionBridge::enrichInsertPayload(nlohmann::json& payload) {
    if (!payload.is_object()) {
        return {};
    }

    auto it = payload.find(text_field_key_);
    if (it == payload.end() || !it->is_string()) {
        return {};
    }

    const std::string text = it->get<std::string>();
    if (text.empty()) {
        return {};
    }

    auto entities = toolbox_->extractEntities(text);
    if (entities.empty()) {
        return {};
    }

    // Append enriched entities to the document payload
    nlohmann::json entity_array = nlohmann::json::array();
    for (const auto& e : entities) {
        entity_array.push_back({
            {"id",   e.id},
            {"type", entityTypeName(e.entity_type)},
            {"text", e.text}
        });
    }
    payload["_entities"] = std::move(entity_array);

    // Write to graph store when a sink is available
    if (graph_writer_) {
        // Retrieve the full entity set from the last execution is not directly
        // available here, so we write only the nodes. Relations require the
        // full BaseEntitySet produced by the workflow, which is accessible via
        // the WorkflowEngine directly when needed.
        static_cast<void>(graph_writer_->writeEntities(entities));
    }

    return buildEntityContext(entities);
}

std::vector<ingestion::BaseEntity>
AQLIngestionBridge::extractEntitiesForContext(const std::string& text) {
    return toolbox_->extractEntities(text);
}

// ─────────────────────────────────────────────────────────────────────────────
// Static helpers
// ─────────────────────────────────────────────────────────────────────────────

std::string AQLIngestionBridge::buildEntityContext(
    const std::vector<ingestion::BaseEntity>& entities)
{
    if (entities.empty()) {
        return {};
    }

    std::ostringstream oss;
    oss << "Extracted entities:";
    bool first = true;
    for (const auto& e : entities) {
        if (!first) {
            oss << " |";
        }
        oss << " " << entityTypeName(e.entity_type);
        if (!e.id.empty()) {
            oss << " " << e.id;
        }
        first = false;
    }
    return oss.str();
}

std::string AQLIngestionBridge::entityTypeName(ingestion::EntityType et) {
    using ET = ingestion::EntityType;
    switch (et) {
        case ET::UNKNOWN:              return "UNKNOWN";
        case ET::CHUNK:                return "CHUNK";
        case ET::PERSON:               return "PERSON";
        case ET::ORGANIZATION:         return "ORGANIZATION";
        case ET::LOCATION:             return "LOCATION";
        case ET::DATE:                 return "DATE";
        case ET::URL:                  return "URL";
        case ET::TABLE_ROW:            return "TABLE_ROW";
        case ET::GEO_FEATURE:          return "GEO_FEATURE";
        case ET::IMAGE_REGION:         return "IMAGE_REGION";
        case ET::LEGAL_PROVISION:      return "LEGAL_PROVISION";
        case ET::LEGAL_NORM_REFERENCE: return "LEGAL_NORM_REFERENCE";
        case ET::LEGAL_OBLIGATION:     return "LEGAL_OBLIGATION";
        case ET::LEGAL_PROHIBITION:    return "LEGAL_PROHIBITION";
        case ET::LEGAL_PERMISSION:     return "LEGAL_PERMISSION";
        case ET::LEGAL_AUTHORITY:      return "LEGAL_AUTHORITY";
        case ET::LEGAL_AKTENZEICHEN:   return "LEGAL_AKTENZEICHEN";
        case ET::LEGAL_DECISION:       return "LEGAL_DECISION";
        case ET::LEGAL_APPLICANT:      return "LEGAL_APPLICANT";
        case ET::LEGAL_EFFECTIVE_DATE: return "LEGAL_EFFECTIVE_DATE";
        default:                       return "UNKNOWN";
    }
}

// ── Accessors ─────────────────────────────────────────────────────────────────

std::shared_ptr<toolbox::IngestionToolbox> AQLIngestionBridge::toolbox() const {
    return toolbox_;
}

std::shared_ptr<ingestion::IGraphWriter> AQLIngestionBridge::graphWriter() const {
    return graph_writer_;
}

} // namespace aql
} // namespace themis
