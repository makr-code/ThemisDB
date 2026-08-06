/**
 * @file process_model_manager.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=12, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

/*
 * ThemisDB | File: process_model_manager.cpp | Version: 0.0.13 | Last Modified: 2026-05-31 12:17:24
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 872
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=15, L=0
 * PR History (last 5): none
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

/*
 * ThemisDB - Process Modeling Module
 *
 * File:    process_model_manager.cpp
 * Module:  src/process/
 * Purpose: High-level manager for BPMN/EPK process model definitions stored
 *          as base-entity documents in the `_process_definitions` system
 *          collection.
 */

#include "process/process_model_manager.h"
#include "process/process_common.h"
#include "process/bpmn_serializer.h"
#include "process/epk_serializer.h"
#include "process/epk_aris_xml_importer.h"
#include "process/llm_process_descriptor.h"
#include "process/vcc_vpb_importer.h"
#include "index/inverted_index.h"
#include "index/vector_index.h"
#include "storage/rocksdb_wrapper.h"
#include "utils/logger.h"

#include <algorithm>
#include <chrono>
#include <sstream>
#include <stdexcept>

namespace themis {
namespace process {

using json = nlohmann::json;

// ---------------------------------------------------------------------------
// Enum helpers
// ---------------------------------------------------------------------------

std::string_view toString(ProcessNotation n) {
    switch (n) {
        case ProcessNotation::BPMN_2_0:  return "BPMN_2_0";
        case ProcessNotation::EPK:        return "EPK";
        case ProcessNotation::VCC_VPB:    return "VCC_VPB";
        case ProcessNotation::CMMN_1_1:  return "CMMN_1_1";
        case ProcessNotation::DMN_1_5:   return "DMN_1_5";
    }
    return "UNKNOWN";
}

std::string_view toString(ProcessDomain d) {
    switch (d) {
        case ProcessDomain::ADMINISTRATION:  return "ADMINISTRATION";
        case ProcessDomain::BUSINESS:        return "BUSINESS";
        case ProcessDomain::IT_SERVICE:      return "IT_SERVICE";
        case ProcessDomain::HEALTHCARE:      return "HEALTHCARE";
        case ProcessDomain::FINANCE:         return "FINANCE";
        case ProcessDomain::CUSTOMER_SERVICE:return "CUSTOMER_SERVICE";
        case ProcessDomain::CUSTOM:          return "CUSTOM";
    }
    return "CUSTOM";
}

std::string_view toString(ProcessModelState s) {
    switch (s) {
        case ProcessModelState::DRAFT:      return "DRAFT";
        case ProcessModelState::ACTIVE:     return "ACTIVE";
        case ProcessModelState::DEPRECATED: return "DEPRECATED";
        case ProcessModelState::ARCHIVED:   return "ARCHIVED";
    }
    return "DRAFT";
}

ProcessNotation notationFromString(std::string_view s) {
    if (s == "BPMN_2_0" || s == "BPMN") return ProcessNotation::BPMN_2_0;
    if (s == "EPK")                       return ProcessNotation::EPK;
    if (s == "VCC_VPB")                   return ProcessNotation::VCC_VPB;
    if (s == "CMMN_1_1" || s == "CMMN")  return ProcessNotation::CMMN_1_1;
    if (s == "DMN_1_5"  || s == "DMN")   return ProcessNotation::DMN_1_5;
    return ProcessNotation::BPMN_2_0;
}

ProcessDomain domainFromString(std::string_view s) {
    if (s == "ADMINISTRATION") return ProcessDomain::ADMINISTRATION;
    if (s == "BUSINESS")       return ProcessDomain::BUSINESS;
    if (s == "IT_SERVICE")     return ProcessDomain::IT_SERVICE;
    if (s == "HEALTHCARE")     return ProcessDomain::HEALTHCARE;
    if (s == "FINANCE")        return ProcessDomain::FINANCE;
    if (s == "CUSTOMER_SERVICE")return ProcessDomain::CUSTOMER_SERVICE;
    return ProcessDomain::CUSTOM;
}

ProcessModelState stateFromString(std::string_view s) {
    if (s == "ACTIVE")     return ProcessModelState::ACTIVE;
    if (s == "DEPRECATED") return ProcessModelState::DEPRECATED;
    if (s == "ARCHIVED")   return ProcessModelState::ARCHIVED;
    return ProcessModelState::DRAFT;
}

// ---------------------------------------------------------------------------
// ProcessModelResult
// ---------------------------------------------------------------------------

ProcessModelResult ProcessModelResult::success(std::string_view id) {
    ProcessModelResult r;
    r.ok       = true;
    r.message  = "OK";
    r.model_id = std::string(id);
    return r;
}

ProcessModelResult ProcessModelResult::failure(std::string_view msg) {
    ProcessModelResult r;
    r.ok      = false;
    r.message = std::string(msg);
    return r;
}

// ---------------------------------------------------------------------------
// ProcessModelRecord serialization
// ---------------------------------------------------------------------------

json ProcessModelRecord::toDocument() const {
    json doc;
    // Reserved ThemisDB fields
    doc["_id"]      = id;
    doc["_type"]    = "process_definition";
    doc["_version"] = revision;
    doc["_created_at"] = created_at_ms;
    doc["_updated_at"] = updated_at_ms;
    doc["_created_by"] = created_by;

    // Process-specific fields
    doc["id"]          = id;
    doc["name"]        = name;
    doc["name_en"]     = name_en;
    doc["version"]     = version;
    doc["revision"]    = revision;
    doc["notation"]    = std::string(toString(notation));
    doc["domain"]      = std::string(toString(domain));
    doc["state"]       = std::string(toString(state));
    doc["description"] = description;
    doc["description_en"] = description_en;
    doc["long_description"] = long_description;
    doc["compliance_tags"]  = compliance_tags;
    doc["owner"]       = owner;
    doc["updated_by"]  = updated_by;
    doc["raw_payload"] = raw_payload;
    doc["normalized"]  = normalized;

    if (!embedding.empty()) {
        doc["_embedding"] = embedding;
    }

    return doc;
}

ProcessModelRecord ProcessModelRecord::fromDocument(const json& doc) {
    ProcessModelRecord r;

    auto getStr = [&](const char* key, const std::string& def = "") -> std::string {
        if (doc.contains(key) && doc[key].is_string()) {
            return doc[key].get<std::string>();
        }
        return def;
    };

    r.id          = getStr("id");
    r.name        = getStr("name");
    r.name_en     = getStr("name_en");
    r.version     = getStr("version", "1.0.0");
    r.revision    = doc.value("revision", 0);
    r.notation    = notationFromString(getStr("notation", "BPMN_2_0"));
    r.domain      = domainFromString(getStr("domain", "BUSINESS"));
    r.state       = stateFromString(getStr("state", "DRAFT"));
    r.description = getStr("description");
    r.description_en = getStr("description_en");
    r.long_description = getStr("long_description");
    r.owner       = getStr("owner");
    r.created_by  = getStr("_created_by");
    r.updated_by  = getStr("updated_by");
    r.created_at_ms = doc.value("_created_at", int64_t{0});
    r.updated_at_ms = doc.value("_updated_at", int64_t{0});
    r.raw_payload = getStr("raw_payload");

    if (doc.contains("compliance_tags") && doc["compliance_tags"].is_array()) {
        for (auto& tag : doc["compliance_tags"]) {
            if (tag.is_string()) {
                r.compliance_tags.push_back(tag.get<std::string>());
            }
        }
    }

    if (doc.contains("normalized")) {
        r.normalized = doc["normalized"];
    }

    if (doc.contains("_embedding") && doc["_embedding"].is_array()) {
        for (auto& v : doc["_embedding"]) {
            if (v.is_number()) {
                r.embedding.push_back(v.get<float>());
            }
        }
    }

    return r;
}

// ---------------------------------------------------------------------------
// ProcessModelManager implementation
// ---------------------------------------------------------------------------

ProcessModelManager::ProcessModelManager(::themis::RocksDBWrapper& db) : db_(db) {}

ProcessModelManager::~ProcessModelManager() = default;

void ProcessModelManager::setEmbedder(
    std::function<std::vector<float>(std::string_view)> embedder)
{
    embedder_ = std::move(embedder);
}

void ProcessModelManager::setInvertedIndex(
    std::shared_ptr<InvertedIndex> fts)
{
    fts_index_ = std::move(fts);
    // Ensure the logical index exists.
    if (fts_index_) {
        if (!fts_index_->exists("process_definitions", "text")) {
            InvertedIndex::Config cfg;
            cfg.stemming_enabled  = false;
            cfg.language          = "none";
            cfg.stopwords_enabled = false;
            fts_index_->create("process_definitions", "text", cfg);
        }
    }
}

void ProcessModelManager::setVectorIndex(std::shared_ptr<VectorIndexManager> vi)
{
    vector_index_ = std::move(vi);
}

std::string ProcessModelManager::makeKey_(std::string_view model_id) const {
    return "proc:def:" + std::string(model_id);
}

std::string ProcessModelManager::makeVersionedKey_(
    std::string_view model_id, int revision) const
{
    return "proc:def:" + std::string(model_id) + ":rev:" + std::to_string(revision);
}

// ---- buildNormalizedGraph_ --------------------------------------------------

json ProcessModelManager::buildNormalizedGraph_(
    const std::vector<ProcessNodeInfo>& nodes,
    const std::vector<ProcessEdgeInfo>& edges,
    const ProcessModelRecord& meta)
{
    json g;
    g["process_id"] = meta.id;
    g["name"]       = meta.name;
    g["domain"]     = std::string(toString(meta.domain));
    g["notation"]   = std::string(toString(meta.notation));

    json jnodes = json::array();
    for (const auto& n : nodes) {
        json jn;
        jn["id"]          = n.node_id;
        jn["name"]        = n.name;
        jn["description"] = n.description;
        jn["subtype"]     = n.subtype;
        jn["is_async"]    = n.is_async;
        jn["max_retries"] = n.max_retries;

        if (std::holds_alternative<BPMNNodeType>(n.node_type)) {
            auto t = std::get<BPMNNodeType>(n.node_type);
            // Encode type as string
            switch (t) {
                case BPMNNodeType::START_EVENT:         jn["type"] = "START_EVENT"; break;
                case BPMNNodeType::END_EVENT:           jn["type"] = "END_EVENT"; break;
                case BPMNNodeType::INTERMEDIATE_EVENT:  jn["type"] = "INTERMEDIATE_EVENT"; break;
                case BPMNNodeType::BOUNDARY_EVENT:      jn["type"] = "BOUNDARY_EVENT"; break;
                case BPMNNodeType::TASK:                jn["type"] = "TASK"; break;
                case BPMNNodeType::SUBPROCESS:          jn["type"] = "SUBPROCESS"; break;
                case BPMNNodeType::CALL_ACTIVITY:       jn["type"] = "CALL_ACTIVITY"; break;
                case BPMNNodeType::EXCLUSIVE_GATEWAY:   jn["type"] = "EXCLUSIVE_GATEWAY"; break;
                case BPMNNodeType::PARALLEL_GATEWAY:    jn["type"] = "PARALLEL_GATEWAY"; break;
                case BPMNNodeType::INCLUSIVE_GATEWAY:   jn["type"] = "INCLUSIVE_GATEWAY"; break;
                case BPMNNodeType::EVENT_BASED_GATEWAY: jn["type"] = "EVENT_BASED_GATEWAY"; break;
                case BPMNNodeType::COMPLEX_GATEWAY:     jn["type"] = "COMPLEX_GATEWAY"; break;
                case BPMNNodeType::POOL:                jn["type"] = "POOL"; break;
                case BPMNNodeType::LANE:                jn["type"] = "LANE"; break;
                case BPMNNodeType::DATA_OBJECT:         jn["type"] = "DATA_OBJECT"; break;
                case BPMNNodeType::DATA_STORE:          jn["type"] = "DATA_STORE"; break;
                case BPMNNodeType::GROUP:               jn["type"] = "GROUP"; break;
                case BPMNNodeType::ANNOTATION:          jn["type"] = "ANNOTATION"; break;
                default:                                jn["type"] = "UNKNOWN"; break;
            }
            jn["notation"] = "BPMN";
        } else if (std::holds_alternative<EPKNodeType>(n.node_type)) {
            auto t = std::get<EPKNodeType>(n.node_type);
            switch (t) {
                case EPKNodeType::EVENT:              jn["type"] = "EVENT"; break;
                case EPKNodeType::FUNCTION:           jn["type"] = "FUNCTION"; break;
                case EPKNodeType::AND_CONNECTOR:      jn["type"] = "AND_CONNECTOR"; break;
                case EPKNodeType::OR_CONNECTOR:       jn["type"] = "OR_CONNECTOR"; break;
                case EPKNodeType::XOR_CONNECTOR:      jn["type"] = "XOR_CONNECTOR"; break;
                case EPKNodeType::ORGANIZATIONAL_UNIT:jn["type"] = "ORGANIZATIONAL_UNIT"; break;
                case EPKNodeType::INFORMATION_OBJECT: jn["type"] = "INFORMATION_OBJECT"; break;
                case EPKNodeType::APPLICATION_SYSTEM: jn["type"] = "APPLICATION_SYSTEM"; break;
                case EPKNodeType::PROCESS_PATH:       jn["type"] = "PROCESS_PATH"; break;
                default:                              jn["type"] = "UNKNOWN"; break;
            }
            jn["notation"] = "EPK";
        }

        if (n.timeout) {
            jn["timeout_ms"] = n.timeout->count();
        }
        if (!n.metadata.is_null() && !n.metadata.empty()) {
            jn["metadata"] = n.metadata;
        }
        jnodes.push_back(std::move(jn));
    }
    g["nodes"] = std::move(jnodes);

    json jedges = json::array();
    for (const auto& e : edges) {
        json je;
        je["id"]        = e.edge_id;
        je["from"]      = e.from_node;
        je["to"]        = e.to_node;
        je["condition"] = e.condition_expression.value_or("");
        switch (e.edge_type) {
            case ProcessEdgeType::SEQUENCE_FLOW:   je["type"] = "SEQUENCE_FLOW"; break;
            case ProcessEdgeType::MESSAGE_FLOW:    je["type"] = "MESSAGE_FLOW"; break;
            case ProcessEdgeType::ASSOCIATION:     je["type"] = "ASSOCIATION"; break;
            case ProcessEdgeType::DATA_ASSOCIATION:je["type"] = "DATA_ASSOCIATION"; break;
            case ProcessEdgeType::CONTROL_FLOW:    je["type"] = "CONTROL_FLOW"; break;
            case ProcessEdgeType::INFORMATION_FLOW:je["type"] = "INFORMATION_FLOW"; break;
            case ProcessEdgeType::ORGANIZATION_FLOW:je["type"]= "ORGANIZATION_FLOW"; break;
            case ProcessEdgeType::DEFAULT_FLOW:    je["type"] = "DEFAULT_FLOW"; break;
            case ProcessEdgeType::CONDITIONAL_FLOW:je["type"] = "CONDITIONAL_FLOW"; break;
            case ProcessEdgeType::EXCEPTION_FLOW:  je["type"] = "EXCEPTION_FLOW"; break;
            default:                               je["type"] = "SEQUENCE_FLOW"; break;
        }
        jedges.push_back(std::move(je));
    }
    g["edges"] = std::move(jedges);

    return g;
}

// ---- Import -----------------------------------------------------------------

ProcessModelResult ProcessModelManager::importBpmn(
    std::string_view bpmn_xml,
    const ProcessModelRecord& meta)
{
    auto result = BpmnSerializer::importXml(bpmn_xml);
    if (!result.ok) {
        return ProcessModelResult::failure("BPMN import failed: " + result.message);
    }

    ProcessModelRecord record = meta;
    if (record.id.empty())   record.id   = result.process_id;
    if (record.name.empty()) record.name = result.process_name;
    record.notation    = ProcessNotation::BPMN_2_0;
    record.raw_payload = std::string(bpmn_xml);
    record.normalized  = buildNormalizedGraph_(result.nodes, result.edges, record);

    return save(record);
}

ProcessModelResult ProcessModelManager::importEpk(
    std::string_view epk_text,
    const ProcessModelRecord& meta)
{
    auto result = EpkSerializer::importText(epk_text);
    if (!result.ok) {
        return ProcessModelResult::failure("EPK import failed: " + result.message);
    }

    ProcessModelRecord record = meta;
    if (record.id.empty())   record.id   = result.process_id;
    if (record.name.empty()) record.name = result.process_name;
    record.notation    = ProcessNotation::EPK;
    record.raw_payload = std::string(epk_text);
    record.normalized  = buildNormalizedGraph_(result.nodes, result.edges, record);

    return save(record);
}

ProcessModelResult ProcessModelManager::importVccVpb(
    std::string_view yaml_text,
    const ProcessModelRecord& meta)
{
    auto result = VccVpbImporter::importYaml(yaml_text, meta);
    if (!result.ok) {
        return ProcessModelResult::failure("VCC-VPB import failed: " + result.message);
    }

    return save(result.record);
}

ProcessModelResult ProcessModelManager::importArisXml(
    std::string_view aml_xml,
    const ProcessModelRecord& meta)
{
    auto result = EpkArisXmlImporter::importAml(aml_xml);
    if (!result.ok) {
        return ProcessModelResult::failure("ARIS-XML import failed: " + result.message);
    }

    ProcessModelRecord record = meta;
    if (record.id.empty())   record.id   = result.process_id;
    if (record.name.empty()) record.name = result.process_name;
    record.notation    = ProcessNotation::EPK;
    record.raw_payload = std::string(aml_xml);
    record.normalized  = buildNormalizedGraph_(result.nodes, result.edges, record);

    return save(record);
}

// ---- CRUD -------------------------------------------------------------------

ProcessModelResult ProcessModelManager::save(const ProcessModelRecord& record) {
    if (record.id.empty()) {
        return ProcessModelResult::failure("ProcessModelRecord.id must not be empty");
    }

    // Load existing to determine revision
    int next_revision = 0;
    auto existing = load(record.id);
    if (existing) {
        next_revision = existing->revision + 1;
        // Keep versioned snapshot of previous revision
        auto versioned_key  = makeVersionedKey_(record.id, existing->revision);
        auto versioned_doc  = existing->toDocument();
        auto versioned_json = versioned_doc.dump();
        db_.put(versioned_key, versioned_json);
    }

    ProcessModelRecord to_save = record;
    to_save.revision    = next_revision;
    to_save.updated_at_ms = nowMs();
    if (to_save.created_at_ms == 0) {
        to_save.created_at_ms = to_save.updated_at_ms;
    }

    // Auto-generate embedding when an embedder is wired and the record
    // doesn't already carry one.
    if (to_save.embedding.empty() && embedder_) {
        const std::string embed_text =
            to_save.name + " " + to_save.description + " " + to_save.long_description;
        try {
            to_save.embedding = embedder_(embed_text);
        } catch (const std::exception& e) {
            SPDLOG_WARN("[process] embedding generation failed for '{}': {}",
                        to_save.id, e.what());
        }
    }

    auto key  = makeKey_(record.id);
    auto doc  = to_save.toDocument();
    auto jstr = doc.dump();

    if (!db_.put(key, jstr)) {
        return ProcessModelResult::failure(
            "DB write failed for process model '" + record.id + "'");
    }

    // Update full-text index.
    if (fts_index_) {
        const std::string fts_text =
            to_save.name + " " + to_save.name_en + " " +
            to_save.description + " " + to_save.description_en + " " +
            to_save.long_description;
        auto st = fts_index_->index("process_definitions", "text",
                                    to_save.id, fts_text);
        if (!st.ok) {
            SPDLOG_WARN("[process] FTS index update failed for '{}': {}",
                        to_save.id, st.message);
        }
    }

    // Upsert into HNSW vector index when wired and embedding is available.
    if (vector_index_ && !to_save.embedding.empty()) {
        BaseEntity ve = BaseEntity::fromFields(to_save.id, {{"id", to_save.id}});
        // Store the embedding under the "embedding" field expected by VectorIndexManager.
        nlohmann::json emb_json(to_save.embedding);
        ve.setField("embedding", emb_json.dump());
        auto vst = vector_index_->addEntity(ve, "embedding");
        if (!vst.ok) {
            // Update path: if add fails (e.g. duplicate), try update.
            vector_index_->updateEntity(ve, "embedding");
        }
    }

    SPDLOG_INFO("[process] saved model '{}' rev={}", record.id, next_revision);
    return ProcessModelResult::success(record.id);
}

std::optional<ProcessModelRecord> ProcessModelManager::load(
    std::string_view model_id) const
{
    auto key = makeKey_(model_id);
    std::string value;
    if (!db_.get(key, value)) {
        return std::nullopt;
    }

    try {
        auto doc = json::parse(value);
        return ProcessModelRecord::fromDocument(doc);
    } catch (const std::exception& ex) {
        SPDLOG_WARN("[process] failed to parse model '{}': {}", model_id, ex.what());
        return std::nullopt;
    }
}

ProcessModelResult ProcessModelManager::remove(std::string_view model_id) {
    auto existing = load(model_id);
    if (!existing) {
        return ProcessModelResult::failure(
            "Process model '" + std::string(model_id) + "' not found");
    }

    // Remove from full-text index before archiving.
    if (fts_index_) {
        fts_index_->deindex("process_definitions", "text",
                            existing->id, /*text=*/"");
    }

    // Remove from HNSW vector index.
    if (vector_index_) {
        vector_index_->removeByPk(existing->id);
    }

    // Soft-delete: mark as ARCHIVED
    existing->state = ProcessModelState::ARCHIVED;
    return save(*existing);
}

// ---- Query ------------------------------------------------------------------

std::vector<ProcessModelRecord> ProcessModelManager::list(
    std::optional<ProcessDomain>      domain,
    std::optional<ProcessModelState>  state,
    size_t                            limit) const
{
    std::vector<ProcessModelRecord> results;

    // Iterate over all proc:def: keys
    db_.scanPrefix("proc:def:", [&](std::string_view /*key*/, std::string_view value) -> bool {
        // Skip versioned snapshots (keys contain ":rev:")
        // The prefix scan will include them; filter by key structure is handled
        // by the caller in the lambda — we check the document instead.
        try {
            auto doc = json::parse(std::string(value));
            // Skip revisions (they have an integer revision suffix in the key)
            if (!doc.contains("id")) return true; // continue
            auto r = ProcessModelRecord::fromDocument(doc);

            if (domain && r.domain != *domain) return true;
            if (state  && r.state  != *state)  return true;

            results.push_back(std::move(r));
            if (limit > 0 && results.size() >= limit) return false; // stop
        } catch (...) {
            // Skip malformed records
        }
        return true;
    });

    return results;
}

std::vector<ProcessModelRecord> ProcessModelManager::search(
    std::string_view query,
    size_t           limit) const
{
    // If an InvertedIndex is wired, use BM25 search and resolve PKs.
    if (fts_index_) {
        auto [st, hits] = fts_index_->search(
            "process_definitions", "text", query,
            limit > 0 ? limit : 1000u);

        std::vector<ProcessModelRecord> results;
        results.reserve(hits.size());
        for (const auto& hit : hits) {
            auto rec = load(hit.pk);
            if (rec) {
                results.push_back(std::move(*rec));
            }
            if (limit > 0 && results.size() >= limit) break;
        }
        return results;
    }

    // Fallback: linear keyword scan.
    std::string q_lower(query);
    std::transform(q_lower.begin(), q_lower.end(), q_lower.begin(), ::tolower);

    std::vector<ProcessModelRecord> results;

    db_.scanPrefix("proc:def:", [&](std::string_view /*key*/, std::string_view value) -> bool {
        try {
            auto doc = json::parse(std::string(value));
            if (!doc.contains("id")) return true;
            auto r = ProcessModelRecord::fromDocument(doc);

            auto match_field = [&](const std::string& field) {
                std::string f_lower = field;
                std::transform(f_lower.begin(), f_lower.end(), f_lower.begin(), ::tolower);
                return f_lower.find(q_lower) != std::string::npos;
            };

            if (match_field(r.name) || match_field(r.name_en) ||
                match_field(r.description) || match_field(r.description_en)) {
                results.push_back(std::move(r));
                if (limit > 0 && results.size() >= limit) return false;
            }
        } catch (...) {}
        return true;
    });

    return results;
}

std::vector<std::pair<ProcessModelRecord, float>> ProcessModelManager::findSimilar(
    const std::vector<float>& query_embedding,
    size_t                    k,
    float                     min_similarity) const
{
    if (query_embedding.empty()) return {};

    std::vector<std::pair<ProcessModelRecord, float>> candidates;

    // Fast path: HNSW index when wired.
    if (vector_index_) {
        const size_t search_k = (k > 0) ? k * 4 : 40; // over-fetch for min_similarity filter
        auto [vst, hits] = vector_index_->searchKnn(query_embedding, search_k);
        if (vst.ok) {
            for (const auto& hit : hits) {
                // VectorIndexManager returns distance (1 - cosine for COSINE metric).
                const float sim = 1.0f - hit.distance;
                if (sim < min_similarity) continue;
                auto rec = load(hit.pk);
                if (!rec) continue;
                candidates.emplace_back(std::move(*rec), sim);
                if (k > 0 && candidates.size() >= k) break;
            }
            return candidates; // already ordered by distance ascending → sim descending
        }
        // Fall through to linear scan if HNSW search failed.
    }

    // Fallback: linear cosine scan over all stored embeddings.
    db_.scanPrefix("proc:def:", [&](std::string_view /*key*/, std::string_view value) -> bool {
        try {
            auto doc = json::parse(std::string(value));
            if (!doc.contains("id") || !doc.contains("_embedding")) return true;
            auto r = ProcessModelRecord::fromDocument(doc);
            if (r.embedding.empty() || r.embedding.size() != query_embedding.size())
                return true;

            // Compute cosine similarity
            float dot = 0.f, qa = 0.f, ra = 0.f;
            for (size_t i = 0; i < query_embedding.size(); ++i) {
                dot += query_embedding[i] * r.embedding[i];
                qa  += query_embedding[i] * query_embedding[i];
                ra  += r.embedding[i]     * r.embedding[i];
            }
            float denom = std::sqrt(qa) * std::sqrt(ra);
            float sim   = (denom > 1e-9f) ? (dot / denom) : 0.f;

            if (sim >= min_similarity) {
                candidates.emplace_back(std::move(r), sim);
            }
        } catch (...) {}
        return true;
    });

    // Sort by descending similarity
    std::sort(candidates.begin(), candidates.end(),
        [](const auto& a, const auto& b) { return a.second > b.second; });

    if (k > 0 && candidates.size() > k) {
        candidates.resize(k);
    }

    return candidates;
}

// ---- Export -----------------------------------------------------------------

std::string ProcessModelManager::exportBpmn(std::string_view model_id) const {
    auto record = load(model_id);
    if (!record) return {};
    return BpmnSerializer::exportFromJson(record->normalized);
}

std::string ProcessModelManager::exportEpk(std::string_view model_id) const {
    auto record = load(model_id);
    if (!record) return {};

    // Reconstruct minimal ProcessNodeInfo / ProcessEdgeInfo from normalized JSON
    std::vector<ProcessNodeInfo> nodes;
    std::vector<ProcessEdgeInfo> edges;

    if (record->normalized.contains("nodes")) {
        for (const auto& jn : record->normalized["nodes"]) {
            ProcessNodeInfo n;
            n.node_id     = jn.value("id", "");
            n.name        = jn.value("name", "");
            n.description = jn.value("description", "");
            n.node_type   = EPKNodeType::FUNCTION; // default
            nodes.push_back(n);
        }
    }
    if (record->normalized.contains("edges")) {
        for (const auto& je : record->normalized["edges"]) {
            ProcessEdgeInfo e;
            e.edge_id   = je.value("id", "");
            e.from_node = je.value("from", "");
            e.to_node   = je.value("to", "");
            e.edge_type = ProcessEdgeType::CONTROL_FLOW;
            edges.push_back(e);
        }
    }

    return EpkSerializer::exportText(record->name, nodes, edges);
}

nlohmann::json ProcessModelManager::generateLlmDescriptor(
    std::string_view model_id) const
{
    auto record = load(model_id);
    if (!record) return {};
    return LlmProcessDescriptor::generate(*record);
}

// ---- Execution bridge -------------------------------------------------------

ProcessModelResult ProcessModelManager::deployToEngine(
    std::string_view     model_id,
    ProcessGraphManager& engine) const
{
    auto record = load(model_id);
    if (!record) {
        return ProcessModelResult::failure(
            "Process model '" + std::string(model_id) + "' not found");
    }

    // Register process definition
    auto status = engine.registerProcess(record->id, record->name);
    if (!status.ok) {
        return ProcessModelResult::failure(
            "Failed to register process '" + record->id + "': " + status.message);
    }

    // Add nodes
    if (record->normalized.contains("nodes")) {
        for (const auto& jn : record->normalized["nodes"]) {
            ProcessNodeInfo node;
            node.node_id     = jn.value("id", "");
            node.name        = jn.value("name", "");
            node.description = jn.value("description", "");
            node.subtype     = jn.value("subtype", "");

            // Determine node type
            std::string type_str = jn.value("type", "TASK");
            std::string notation = jn.value("notation", "BPMN");

            if (notation == "EPK") {
                if      (type_str == "EVENT")              node.node_type = EPKNodeType::EVENT;
                else if (type_str == "FUNCTION")           node.node_type = EPKNodeType::FUNCTION;
                else if (type_str == "AND_CONNECTOR")      node.node_type = EPKNodeType::AND_CONNECTOR;
                else if (type_str == "OR_CONNECTOR")       node.node_type = EPKNodeType::OR_CONNECTOR;
                else if (type_str == "XOR_CONNECTOR")      node.node_type = EPKNodeType::XOR_CONNECTOR;
                else if (type_str == "ORGANIZATIONAL_UNIT")node.node_type = EPKNodeType::ORGANIZATIONAL_UNIT;
                else if (type_str == "INFORMATION_OBJECT") node.node_type = EPKNodeType::INFORMATION_OBJECT;
                else if (type_str == "APPLICATION_SYSTEM") node.node_type = EPKNodeType::APPLICATION_SYSTEM;
                else if (type_str == "PROCESS_PATH")       node.node_type = EPKNodeType::PROCESS_PATH;
                else                                       node.node_type = EPKNodeType::FUNCTION;
            } else {
                // Default to BPMN
                if      (type_str == "START_EVENT")         node.node_type = BPMNNodeType::START_EVENT;
                else if (type_str == "END_EVENT")           node.node_type = BPMNNodeType::END_EVENT;
                else if (type_str == "INTERMEDIATE_EVENT")  node.node_type = BPMNNodeType::INTERMEDIATE_EVENT;
                else if (type_str == "BOUNDARY_EVENT")      node.node_type = BPMNNodeType::BOUNDARY_EVENT;
                else if (type_str == "SUBPROCESS")          node.node_type = BPMNNodeType::SUBPROCESS;
                else if (type_str == "CALL_ACTIVITY")       node.node_type = BPMNNodeType::CALL_ACTIVITY;
                else if (type_str == "EXCLUSIVE_GATEWAY")   node.node_type = BPMNNodeType::EXCLUSIVE_GATEWAY;
                else if (type_str == "PARALLEL_GATEWAY")    node.node_type = BPMNNodeType::PARALLEL_GATEWAY;
                else if (type_str == "INCLUSIVE_GATEWAY")   node.node_type = BPMNNodeType::INCLUSIVE_GATEWAY;
                else if (type_str == "EVENT_BASED_GATEWAY") node.node_type = BPMNNodeType::EVENT_BASED_GATEWAY;
                else if (type_str == "COMPLEX_GATEWAY")     node.node_type = BPMNNodeType::COMPLEX_GATEWAY;
                else if (type_str == "DATA_OBJECT")         node.node_type = BPMNNodeType::DATA_OBJECT;
                else if (type_str == "DATA_STORE")          node.node_type = BPMNNodeType::DATA_STORE;
                else                                        node.node_type = BPMNNodeType::TASK;
            }

            if (jn.contains("timeout_ms") && jn["timeout_ms"].is_number()) {
                node.timeout = std::chrono::milliseconds(jn["timeout_ms"].get<int64_t>());
            }

            auto ns = engine.addProcessNode(record->id, node);
            if (!ns.ok) {
                return ProcessModelResult::failure(
                    "Failed to add node '" + node.node_id + "': " + ns.message);
            }
        }
    }

    // Add edges
    if (record->normalized.contains("edges")) {
        for (const auto& je : record->normalized["edges"]) {
            ProcessEdgeInfo edge;
            edge.edge_id   = je.value("id", "");
            edge.from_node = je.value("from", "");
            edge.to_node   = je.value("to", "");

            std::string cond = je.value("condition", "");
            if (!cond.empty()) edge.condition_expression = cond;

            std::string et = je.value("type", "SEQUENCE_FLOW");
            if      (et == "MESSAGE_FLOW")    edge.edge_type = ProcessEdgeType::MESSAGE_FLOW;
            else if (et == "ASSOCIATION")     edge.edge_type = ProcessEdgeType::ASSOCIATION;
            else if (et == "DATA_ASSOCIATION")edge.edge_type = ProcessEdgeType::DATA_ASSOCIATION;
            else if (et == "CONTROL_FLOW")    edge.edge_type = ProcessEdgeType::CONTROL_FLOW;
            else if (et == "INFORMATION_FLOW")edge.edge_type = ProcessEdgeType::INFORMATION_FLOW;
            else if (et == "ORGANIZATION_FLOW")edge.edge_type= ProcessEdgeType::ORGANIZATION_FLOW;
            else if (et == "DEFAULT_FLOW")    edge.edge_type = ProcessEdgeType::DEFAULT_FLOW;
            else if (et == "CONDITIONAL_FLOW")edge.edge_type = ProcessEdgeType::CONDITIONAL_FLOW;
            else if (et == "EXCEPTION_FLOW")  edge.edge_type = ProcessEdgeType::EXCEPTION_FLOW;
            else                              edge.edge_type = ProcessEdgeType::SEQUENCE_FLOW;

            auto es = engine.addProcessEdge(record->id, edge);
            if (!es.ok) {
                return ProcessModelResult::failure(
                    "Failed to add edge '" + edge.edge_id + "': " + es.message);
            }
        }
    }

    SPDLOG_INFO("[process] deployed model '{}' to engine", model_id);
    return ProcessModelResult::success(std::string(model_id));
}

ProcessModelResult ProcessModelManager::undeployFromEngine(
    std::string_view     model_id,
    [[maybe_unused]] ProcessGraphManager& engine) const
{
    // ProcessGraphManager doesn't have an unregister API yet; use the
    // removeProcess method if available, or log a warning.
    SPDLOG_WARN("[process] undeployFromEngine: engine.removeProcess not yet available "
                "for model '{}'", model_id);
    return ProcessModelResult::success(std::string(model_id));
}

// ---------------------------------------------------------------------------
// Phase 2: Concurrency and Determinism Implementation
// ---------------------------------------------------------------------------

ProcessModelManager::TransactionGuard::~TransactionGuard() {
    if (failed_) {
        manager_.rollbackTransaction_(context_);
    }
}

bool ProcessModelManager::detectConflict_(std::string_view model_id, int expected_revision) const {
    std::shared_lock<std::shared_mutex> lock(model_state_lock_);
    
    // Attempt to read current revision from database
    std::string key = makeKey_(model_id);
    std::string doc_str = db_.get(key);
    
    if (doc_str.empty()) {
        // Model doesn't exist (or was deleted), conflict detected
        return true;
    }
    
    try {
        auto doc = json::parse(doc_str);
        int current_revision = doc.value("revision", 0);
        // Conflict if current revision differs from expected
        return current_revision != expected_revision;
    } catch (const std::exception& e) {
        SPDLOG_WARN("[process] detectConflict_: Failed to parse model '{}': {}",
                    model_id, e.what());
        return true;  // Assume conflict on parse error
    }
}

void ProcessModelManager::rollbackTransaction_(const TransactionContext& txn) {
    std::unique_lock<std::shared_mutex> lock(model_state_lock_);
    
    // Roll back all modifications recorded in the transaction
    for (const auto& key : txn.modified_keys) {
        // Re-read the previous version if available
        std::string versioned_key = makeVersionedKey_(txn.model_id, txn.revision_at_start);
        std::string prev_value = db_.get(versioned_key);
        
        if (!prev_value.empty()) {
            // Restore previous value
            db_.put(key, prev_value);
            SPDLOG_INFO("[process] Rolled back modification to key: {}", key);
        } else {
            // Delete the key if no previous version exists
            db_.delete(key);
            SPDLOG_INFO("[process] Deleted key during rollback: {}", key);
        }
    }
    
    SPDLOG_WARN("[process] Transaction {} rolled back for model '{}' due to conflict",
                txn.txn_id, txn.model_id);
}

ProcessModelManager::TransactionContext ProcessModelManager::createTransaction_(
    std::string_view model_id)
{
    std::shared_lock<std::shared_mutex> lock(model_state_lock_);
    
    uint64_t txn_id = operation_counter_++;
    std::string key = makeKey_(model_id);
    std::string doc_str = db_.get(key);
    int revision = 0;
    
    if (!doc_str.empty()) {
        try {
            auto doc = json::parse(doc_str);
            revision = doc.value("revision", 0);
        } catch (...) {
            // Use default revision
        }
    }
    
    int64_t now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    
    return TransactionContext{
        txn_id,
        std::string(model_id),
        now_ms,
        revision,
        {},  // modified_keys
        true // is_active
    };
}

} // namespace process
} // namespace themis

