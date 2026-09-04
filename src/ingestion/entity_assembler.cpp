/**
 * @file entity_assembler.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.2
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=2, M=14, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "ingestion/entity_assembler.h"
#include <algorithm>
#include <regex>
#include <sstream>
#include <iomanip>
#include <functional>
#include <unordered_set>

namespace themis {
namespace ingestion {

// ─────────────────────────────────────────────────────────────────────────────
// EntityNormalizer
// ─────────────────────────────────────────────────────────────────────────────

EntityNormalizer::EntityNormalizer(EntityNormalizerConfig cfg)
    : cfg_(std::move(cfg)) {}

// static
std::string EntityNormalizer::toIdToken(const std::string& s) {
    std::string out = {};
    out.reserve(s.size());
    for (unsigned char c : s) {
        if (std::isalnum(c) || c == '-') {
            out.push_back(static_cast<char>(std::tolower(c)));
        } else if (c == ' ' || c == '.' || c == '/' || c == '_') {
            if (!out.empty() && out.back() != '_')
                out.push_back('_');
        }
        // strip other special chars (§, ä, ö, ü, etc.)
    }
    // Remove trailing underscores
    while (!out.empty() && out.back() == '_') {
      out.pop_back();
    }
    return out;
}

// static
std::string EntityNormalizer::shortHash(const std::string& s) {
    // FNV-1a 32-bit
    std::uint32_t h = 0x811c9dc5u;
    for (unsigned char c : s) {
        h ^= static_cast<std::uint32_t>(c);
        h *= 0x01000193u;
    }
    std::ostringstream oss = {};
    oss << std::hex << std::setw(8) << std::setfill('0') << h;
    return oss.str();
}

std::unordered_map<std::string, std::string>
EntityNormalizer::parseLegalRef(const std::string& text) const {
    // Matches patterns like:  § 4 Abs. 1  |  Art. 12 Abs 3  |  §4  |  § 4a
    static const std::regex re_law(
        R"((?:§{1,2}|Art\.?)\s*(\d+[a-z]?)(?:\s*Abs\.?\s*(\d+))?(?:\s*S(?:atz)?\.?\s*(\d+))?(?:\s+([A-Z][a-zA-ZÄÖÜäöüß]+(?:\s+[A-Z][a-zA-ZÄÖÜäöüß]+)*))?)",
        std::regex::optimize);
    std::smatch m = {};
    if (!std::regex_search(text, m, re_law)) return {};

    std::unordered_map<std::string, std::string> parts;
    parts["section"] = m[1].str();
    if (m[2].matched) {
      parts["abs"]  = m[2].str();
    }
    if (m[3].matched) {
      parts["satz"] = m[3].str();
    }
    if (m[4].matched) {
        // Potential law abbreviation at end
        const std::string candidate = m[4].str();
        if (!cfg_.known_law_abbreviations.empty()) {
            for (const auto& abbr : cfg_.known_law_abbreviations) {
                if (candidate.find(abbr) != std::string::npos) {
                    parts["abbr"] = abbr;
                    break;
                }
            }
        } else {
            parts["abbr"] = candidate;
        }
    }
    return parts;
}

std::string EntityNormalizer::canonicalId(const BaseEntity& ent,
                                            const std::string& file_id,
                                            std::size_t seq) const {
    // Check for existing canonical-looking ID (already assigned)
    if (!ent.id.empty()
        && (ent.id.substr(0, 4) == "law:"
            || ent.id.substr(0, 8) == "normref:"
            || ent.id.substr(0, 8) == "bescheid"
            || ent.id.substr(0, 7) == "person:"
            || ent.id.substr(0, 4) == "org:"
            || ent.id.substr(0, 6) == "chunk:")) {
        return ent.id;
    }

    switch (ent.entity_type) {
        case EntityType::LEGAL_PROVISION: {
            // Try to parse from properties first
            const auto& norm = ent.propertyOr("norm_id",    "");
            const auto& sec  = ent.propertyOr("section_ref", ent.text);

            std::string abbr = norm.empty()
                ? toIdToken(ent.text.substr(0, std::min(ent.text.size(), std::size_t{20})))
                : toIdToken(norm);

            auto parts = parseLegalRef(sec.empty() ? ent.text : sec);
            if (parts.count("section")) {
                std::string id = "law:" + abbr + ":§" + parts["section"];
                if (parts.count("abs")) {
                  id += ":Abs" + parts["abs"];
                }
                return id;
            }
            return "law:" + abbr + ":" + shortHash(ent.text);
        }

        case EntityType::LEGAL_NORM_REFERENCE: {
            const auto& norm = ent.propertyOr("norm_id", "");
            auto parts = parseLegalRef(ent.text);
            std::string abbr = norm.empty()
                ? (parts.count("abbr") ? toIdToken(parts["abbr"])
                                       : toIdToken(ent.text.substr(0, 20)))
                : toIdToken(norm);
            if (parts.count("section"))
                return "normref:" + abbr + ":§" + parts["section"];
            return "normref:" + abbr + ":" + shortHash(ent.text);
        }

        case EntityType::LEGAL_DECISION: {
            const auto& az = ent.propertyOr("aktenzeichen", "");
            return "bescheid:" + toIdToken(az.empty() ? ent.text : az);
        }

        case EntityType::PERSON:
            return "person:" + shortHash(ent.text);

        case EntityType::ORGANIZATION:
            return "org:" + shortHash(ent.text);

        case EntityType::LOCATION:
            return "location:" + shortHash(ent.text);

        case EntityType::DATE:
            return "date:" + toIdToken(ent.text);

        case EntityType::CHUNK:
            return "chunk:" + file_id + ":" + std::to_string(seq);

        case EntityType::GEO_FEATURE: {
            const auto& gid = ent.propertyOr("geo_id", "");
            return "geo:" + (gid.empty() ? shortHash(ent.text) : toIdToken(gid));
        }

        case EntityType::TABLE_ROW: {
            const auto& row = ent.propertyOr("row_index", "");
            return "row:" + file_id + ":" + row;
        }

        default:
            return "entity:" + file_id + ":" + shortHash(ent.text);
    }
}

void EntityNormalizer::normalize(ExtractionContext& ctx) const {
    // 1. Assign / update canonical IDs
    std::size_t chunk_seq = 0;
    for (auto& ent : ctx.entities) {
        if (ent.entity_type == EntityType::CHUNK) {
            ent.id = canonicalId(ent, ctx.manifest.file_id, chunk_seq++);
        } else {
            const std::string cid = canonicalId(ent, ctx.manifest.file_id,
                                                 ctx.entities.size());
            if (ent.id.empty() || ent.id.substr(0, 4) != "law:") {
                ent.id = cid;
            }
        }
        if (ent.source_file_id.empty())
            ent.source_file_id = ctx.manifest.file_id;
    }

    // 2. Filter by confidence
    if (cfg_.min_confidence > 0.0) {
        ctx.entities.erase(
            std::remove_if(ctx.entities.begin(), ctx.entities.end(),
                [this](const BaseEntity& e) {
                    return e.provenance.confidence < cfg_.min_confidence;
                }),
            ctx.entities.end());
    }

    // 3. Deduplicate by canonical ID
    if (cfg_.dedup_strategy == "canonical_id") {
        std::unordered_map<std::string, std::size_t> id_to_idx;
        std::vector<BaseEntity> deduped = {};

        deduped.reserve(ctx.entities.size());

        for (auto& ent : ctx.entities) {
            auto it = id_to_idx.find(ent.id);
            if (it == id_to_idx.end()) {
                id_to_idx[ent.id] = deduped.size();
                deduped.push_back(std::move(ent));
            } else {
                auto& existing = deduped[it->second];
                if (ent.provenance.confidence > existing.provenance.confidence) {
                    // Merge properties from existing into new (keep missing keys)
                    for (auto& [k, v] : existing.properties) {
                        existing.properties.try_emplace(k, v);
                    }
                    existing = std::move(ent);
                } else {
                    // Merge properties from duplicate into existing
                    for (auto& [k, v] : ent.properties) {
                        existing.properties.try_emplace(k, v);
                    }
                }
            }
        }
        ctx.entities = std::move(deduped);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// RelationBuilder
// ─────────────────────────────────────────────────────────────────────────────

RelationBuilder::RelationBuilder(RelationBuilderConfig cfg)
    : cfg_(std::move(cfg)) {}

bool RelationBuilder::wantsType(const std::string& t) const {
    return cfg_.relation_types.empty()
           || std::find(cfg_.relation_types.begin(),
                        cfg_.relation_types.end(), t)
              != cfg_.relation_types.end();
}

// static
bool RelationBuilder::edgeExists(const std::vector<EntityRelation>& rels,
                                  const std::string& from,
                                  const std::string& to,
                                  RelationType rt) {
    for (const auto& r : rels) {
        if (r.from_id == from && r.to_id == to && r.relation_type == rt)
            return true;
    }
    return false;
}

void RelationBuilder::buildCitesRelations(ExtractionContext& ctx) const {
    for (const auto& ent : ctx.entities) {
        if (ent.provenance.confidence < cfg_.min_entity_confidence) {
          continue;
        }

        // CITES: entity has a cross-reference target
        const auto& ref_target = ent.propertyOr("norm_ref_target", "");
        if (!ref_target.empty() && wantsType("CITES")) {
            EntityRelation r;
            r.from_id       = ent.id;
            r.to_id         = ref_target;
            r.relation_type = RelationType::CITES;
            r.properties["evidence"] = ent.text.substr(
                0, std::min(ent.text.size(), std::size_t{100}));
            if (!edgeExists(ctx.relations, r.from_id, r.to_id, r.relation_type))
                ctx.relations.push_back(std::move(r));
        }

        // AMENDS / SUPERSEDES from hint property
        const auto& hint = ent.propertyOr("relation_hint", "");
        if (!hint.empty()) {
            RelationType rt = RelationType::UNKNOWN;
            if (hint == "amends"     && wantsType("AMENDS")) {
              rt = RelationType::AMENDS;
            }
            if (hint == "supersedes" && wantsType("SUPERSEDES")) {
              rt = RelationType::SUPERSEDES;
            }
            if (hint == "regulates"  && wantsType("REGULATES")) {
              rt = RelationType::REGULATES;
            }

            if (rt != RelationType::UNKNOWN) {
                const auto& target = ent.propertyOr("relation_target", "");
                if (!target.empty()) {
                    EntityRelation r;
                    r.from_id       = ent.id;
                    r.to_id         = target;
                    r.relation_type = rt;
                    if (!edgeExists(ctx.relations, r.from_id, r.to_id, r.relation_type))
                        ctx.relations.push_back(std::move(r));
                }
            }
        }
    }
}

void RelationBuilder::buildPartOfRelations(ExtractionContext& ctx) const {
    if (!wantsType("PART_OF")) {
      return;
    }

    // Build a quick id→index map
    std::unordered_map<std::string, std::size_t> id_map = {};

    for (std::size_t i = 0; i < ctx.entities.size(); ++i)
        id_map[ctx.entities[i].id] = i;

    for (const auto& ent : ctx.entities) {
        if (ent.provenance.confidence < cfg_.min_entity_confidence) {
          continue;
        }
        const auto& parent = ent.propertyOr("parent_section", "");
        if (parent.empty()) {
          continue;
        }
        if (!id_map.count(parent)) {
          continue;
        }

        EntityRelation r;
        r.from_id       = ent.id;
        r.to_id         = parent;
        r.relation_type = RelationType::PART_OF;
        if (!edgeExists(ctx.relations, r.from_id, r.to_id, r.relation_type))
            ctx.relations.push_back(std::move(r));
    }
}

void RelationBuilder::buildCoOccurrence(ExtractionContext& ctx) const {
    if (!wantsType("CO_OCCURS") || !cfg_.build_co_occurrence) {
      return;
    }

    // Group entity IDs by section_ref (chunk)
    std::unordered_map<std::string, std::vector<std::string>> by_section;
    for (const auto& ent : ctx.entities) {
        if (ent.provenance.confidence < cfg_.min_entity_confidence) {
          continue;
        }
        const auto& ref = ent.propertyOr("section_ref", "");
        by_section[ref].push_back(ent.id);
    }

    for (const auto& [section, ids] : by_section) {
        if (ids.size() < 2) {
          continue;
        }
        for (std::size_t i = 0; i < ids.size(); ++i) {
            for (std::size_t j = i + 1; j < ids.size(); ++j) {
                EntityRelation r;
                r.from_id       = ids[i];
                r.to_id         = ids[j];
                r.relation_type = RelationType::CO_OCCURS;
                r.properties["section_ref"] = section;
                if (!edgeExists(ctx.relations, r.from_id, r.to_id, r.relation_type))
                    ctx.relations.push_back(std::move(r));
            }
        }
    }
}

void RelationBuilder::buildIssuedByRelations(ExtractionContext& ctx) const {
    if (!wantsType("ISSUED_BY")) {
      return;
    }

    // Build authority id map
    std::unordered_map<std::string, std::string> authority_by_text = {};

    for (const auto& ent : ctx.entities) {
        if (ent.entity_type == EntityType::LEGAL_AUTHORITY)
            authority_by_text[ent.text] = ent.id;
    }

    for (const auto& ent : ctx.entities) {
        if (ent.entity_type != EntityType::LEGAL_DECISION) {
          continue;
        }
        const auto& auth = ent.propertyOr("authority", "");
        if (auth.empty()) {
          continue;
        }

        // Find the authority entity
        auto it = authority_by_text.find(auth);
        if (it == authority_by_text.end()) {
          continue;
        }

        EntityRelation r;
        r.from_id       = ent.id;
        r.to_id         = it->second;
        r.relation_type = RelationType::ISSUED_BY;
        if (!edgeExists(ctx.relations, r.from_id, r.to_id, r.relation_type))
            ctx.relations.push_back(std::move(r));
    }
}

void RelationBuilder::build(ExtractionContext& ctx) const {
    buildCitesRelations(ctx);
    buildPartOfRelations(ctx);
    buildCoOccurrence(ctx);
    buildIssuedByRelations(ctx);
}

} // namespace ingestion
} // namespace themis
