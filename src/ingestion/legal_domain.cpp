/**
 * @file legal_domain.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.2
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=28, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "ingestion/legal_domain.h"

#include "utils/string_utils.h"
#include <algorithm>
#include <array>
#include <cctype>
#include <cstring>
#include <functional>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <unordered_set>

namespace themis {
namespace ingestion {

// ─────────────────────────────────────────────────────────────────────────────
// Helper utilities (file-local)
// ─────────────────────────────────────────────────────────────────────────────

namespace {

/// Trim leading/trailing whitespace from a string.
// Using themis::utils::trim() from string_utils.h (Phase 1 consolidation)

/// Simple FNV-1a 32-bit hash for stable ID generation.
uint32_t fnv1a32(const std::string& s) {
    uint32_t h = 2166136261u;
    for (auto c : s) {
        h ^= static_cast<uint8_t>(c);
        h *= 16777619u;
    }
    return h;
}

/// Convert a string to lowercase.
std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return s;
}

std::string entityTypeName(const EntityType type) {
    switch (type) {
    case EntityType::LEGAL_PROVISION:      return "LEGAL_PROVISION";
    case EntityType::LEGAL_NORM_REFERENCE: return "LEGAL_NORM_REFERENCE";
    case EntityType::LEGAL_OBLIGATION:     return "LEGAL_OBLIGATION";
    case EntityType::LEGAL_PROHIBITION:    return "LEGAL_PROHIBITION";
    case EntityType::LEGAL_PERMISSION:     return "LEGAL_PERMISSION";
    case EntityType::LEGAL_AUTHORITY:      return "LEGAL_AUTHORITY";
    case EntityType::LEGAL_AKTENZEICHEN:   return "LEGAL_AKTENZEICHEN";
    case EntityType::LEGAL_DECISION:       return "LEGAL_DECISION";
    case EntityType::LEGAL_APPLICANT:      return "LEGAL_APPLICANT";
    case EntityType::LEGAL_EFFECTIVE_DATE: return "LEGAL_EFFECTIVE_DATE";
    default:                               return "UNKNOWN";
    }
}

std::string relationTypeName(const RelationType type) {
    switch (type) {
    case RelationType::CITES:     return "CITES";
    case RelationType::AMENDS:    return "AMENDS";
    case RelationType::SUPERSEDES:return "SUPERSEDES";
    case RelationType::REGULATES: return "REGULATES";
    case RelationType::PART_OF:   return "PART_OF";
    case RelationType::CO_OCCURS: return "CO_OCCURS";
    case RelationType::GEO_CONTAINS: return "GEO_CONTAINS";
    case RelationType::ISSUED_BY: return "ISSUED_BY";
    case RelationType::APPLIES_TO:return "APPLIES_TO";
    default:                      return "UNKNOWN";
    }
}

std::string entityDisplayLabel(const BaseEntity& e) {
    const auto it = e.properties.find("label");
    if (it != e.properties.end() && !it->second.empty()) {
        return it->second;
    }
    return e.text;
}

/// German month name → 2-digit month number.
std::string parseGermanMonth(const std::string& month) {
    static const std::array<std::pair<const char*, const char*>, 12> months{{
        {"januar",    "01"}, {"februar",  "02"}, {"märz",     "03"},
        {"april",     "04"}, {"mai",      "05"}, {"juni",     "06"},
        {"juli",      "07"}, {"august",   "08"}, {"september","09"},
        {"oktober",   "10"}, {"november", "11"}, {"dezember", "12"},
    }};
    const auto lo = toLower(month);
    for (const auto& [name, num] : months) {
        if (lo == name || lo.substr(0, strlen(name)) == name) return num;
    }
    return "";
}

/// Zero-pad a 1-2 digit number string to 2 digits.
std::string pad2(const std::string& s) {
    if (s.size() == 1) return "0" + s;
    return s;
}

} // anonymous namespace

// ─────────────────────────────────────────────────────────────────────────────
// GesetzNode
// ─────────────────────────────────────────────────────────────────────────────

void GesetzNode::traverse(
    std::function<void(const GesetzNode&, int)> fn, int depth) const
{
    fn(*this, depth);
    for (const auto& child : children) {
        child.traverse(fn, depth + 1);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// GesetzParser
// ─────────────────────────────────────────────────────────────────────────────

namespace {

// Regexes for structural recognition
const std::regex RE_PARAGRAPH(
    R"((?:^|\n)\s*(§§?\s*\d+[a-zA-Z]?(?:\s+[a-zA-Z]\w+)?)\s*\n(.*?)(?=\n\s*§§?\s*\d|\n\s*(?:Teil|Abschnitt|Kapitel)\s|\z))",
    std::regex::icase);

const std::regex RE_ABSATZ(
    R"(\((\d+)\)\s*([\s\S]*?)(?=\(\d+\)|\z))",
    std::regex::icase);

const std::regex RE_TEIL(
    R"((?:^|\n)\s*((?:Erster|Zweiter|Dritter|Vierter|Fünfter|Sechster|Siebter|Achter|Neunter|Zehnter|\d+\.)\s*Teil)\s*\n([^\n]*)\n)",
    std::regex::icase);

const std::regex RE_ABSCHNITT(
    R"((?:^|\n)\s*((?:\d+\.?\s+)?(?:Abschnitt|Kapitel|Unterabschnitt))\s*\n([^\n]*)\n)",
    std::regex::icase);

const std::regex RE_TITLE(
    R"(^[^\n]*(?:gesetz|verordnung|richtlinie|satzung|ordnung)[^\n]*$)",
    std::regex::icase);

} // anonymous namespace

Result<GesetzHierarchy> GesetzParser::parse(
    const std::string& text, const std::string& norm_abbreviation) const
{
    if (text.empty()) {
        return tl::make_unexpected(
            Error(errors::ErrorCode::ERR_WORKFLOW_CONTEXT_INVALID, "empty text"));
    }

    GesetzHierarchy hier;
    hier.norm_abbreviation = norm_abbreviation;
    hier.root.type    = GesetzNodeType::GESETZ;
    hier.root.number  = norm_abbreviation;

    // Extract full title from first matching line
    std::smatch tm;
    if (std::regex_search(text, tm, RE_TITLE)) {
        hier.full_title = themis::utils::trim(tm[0].str());
        hier.root.heading = hier.full_title;
    }

    // Extract §-paragraphs with byte offsets for Teil assignment
    auto positioned_paras = extractParagraphsWithOffsets(text);

    // Build Teil / Abschnitt containers (simple grouping by order in text)
    // Find all Teil positions
    std::vector<std::pair<std::size_t, GesetzNode>> teile;
    auto teil_begin = std::sregex_iterator(text.begin(), text.end(), RE_TEIL);
    for (auto it = teil_begin; it != std::sregex_iterator(); ++it) {
        GesetzNode tn;
        tn.type    = GesetzNodeType::TEIL;
        tn.number  = themis::utils::trim((*it)[1].str());
        tn.heading = themis::utils::trim((*it)[2].str());
        teile.emplace_back(it->position(), std::move(tn));
    }

    if (teile.empty()) {
        // Flat structure: all paragraphs under root
        for (auto& [offset, para] : positioned_paras) {
            (void)offset;
            hier.root.children.push_back(std::move(para));
        }
    } else {
        // Assign each paragraph to the last Teil whose start position precedes
        // the paragraph's start position in the source text.  Paragraphs that
        // appear before the first Teil are placed directly under the root.
        for (auto& [para_pos, para] : positioned_paras) {
            // Find the last teil_pos <= para_pos (teile are in document order)
            GesetzNode* target_teil = nullptr;
            for (auto& [teil_pos, tn] : teile) {
                if (teil_pos <= para_pos) {
                    target_teil = &tn;
                } else {
                    break;
                }
            }
            if (target_teil) {
                target_teil->children.push_back(std::move(para));
            } else {
                // Paragraph precedes the first Teil — attach to root directly
                hier.root.children.push_back(std::move(para));
            }
        }
        // Add Teil nodes (now with children assigned) to root
        for (auto& [pos, tn] : teile) {
            hier.root.children.push_back(std::move(tn));
        }
    }

    return hier;
}

std::vector<std::pair<std::size_t, GesetzNode>> GesetzParser::extractParagraphsWithOffsets(
    const std::string& text) const
{
    std::vector<std::pair<std::size_t, GesetzNode>> result;

    auto parseParagraphHeader = [](const std::string& line,
                                   std::string& number,
                                   std::string& heading) {
        const std::string trimmed = themis::utils::trim(line);
        if (trimmed.empty() || trimmed.rfind("§", 0) != 0) {
            return false;
        }

        std::size_t pos = trimmed.find_first_not_of("§ ", 0);
        if (pos == std::string::npos || !std::isdigit(static_cast<unsigned char>(trimmed[pos]))) {
            return false;
        }

        std::size_t end = pos;
        while (end < trimmed.size() &&
               (std::isalnum(static_cast<unsigned char>(trimmed[end])) || trimmed[end] == '.')) {
            ++end;
        }

        number = themis::utils::trim(trimmed.substr(0, end));
        heading = themis::utils::trim(trimmed.substr(end));
        return !number.empty();
    };

    auto finalizeParagraph = [&](std::size_t start_offset,
                                 const std::string& number,
                                 const std::string& heading,
                                 std::string& body) {
        if (number.empty()) {
            return;
        }

        GesetzNode para;
        para.type = GesetzNodeType::PARAGRAPH;
        para.number = themis::utils::trim(number);
        para.heading = themis::utils::trim(heading);
        para.text = themis::utils::trim(body);

        auto abs_it = std::sregex_iterator(para.text.begin(), para.text.end(), RE_ABSATZ);
        for (auto ae = std::sregex_iterator(); abs_it != ae; ++abs_it) {
            GesetzNode abs;
            abs.type = GesetzNodeType::ABSATZ;
            abs.number = "(" + (*abs_it)[1].str() + ")";
            abs.text = themis::utils::trim((*abs_it)[2].str());
            para.children.push_back(std::move(abs));
        }

        result.emplace_back(start_offset, std::move(para));
        body.clear();
    };

    std::istringstream input(text);
    std::string line;
    std::string current_number;
    std::string current_heading;
    std::string current_body;
    std::size_t current_offset = 0;
    std::size_t line_offset = 0;

    while (std::getline(input, line)) {
        std::string parsed_number;
        std::string parsed_heading;
        if (parseParagraphHeader(line, parsed_number, parsed_heading)) {
            finalizeParagraph(current_offset, current_number, current_heading, current_body);
            current_offset = line_offset;
            current_number = std::move(parsed_number);
            current_heading = std::move(parsed_heading);
        } else if (!current_number.empty()) {
            if (!current_body.empty()) {
                current_body += "\n";
            }
            current_body += line;
        }

        line_offset += line.size() + 1;
    }

    finalizeParagraph(current_offset, current_number, current_heading, current_body);

    return result;
}

std::vector<GesetzNode> GesetzParser::extractParagraphs(
    const std::string& text) const
{
    auto positioned = extractParagraphsWithOffsets(text);
    std::vector<GesetzNode> result;
    result.reserve(positioned.size());
    for (auto& [offset, node] : positioned) {
        (void)offset;
        result.push_back(std::move(node));
    }
    return result;
}

std::vector<BaseEntity> GesetzParser::toEntities(
    const GesetzHierarchy& hierarchy) const
{
    std::vector<BaseEntity> entities;
    const std::string& norm = toLower(hierarchy.norm_abbreviation);

    hierarchy.root.traverse([&](const GesetzNode& node, int /*depth*/) {
        if (node.type == GesetzNodeType::PARAGRAPH) {
            // Canonical ID: law:<norm>:§<n>
            const std::string raw_num = node.number;
            // Strip "§" prefix and whitespace
            std::string n = raw_num;
            const auto pos = n.find_first_of("0123456789");
            if (pos != std::string::npos) n = n.substr(pos);
            const auto end = n.find_first_not_of("0123456789abcdefghijklmnopqrstuvwxyz");
            if (end != std::string::npos) n = n.substr(0, end);

            BaseEntity e;
            e.id         = "law:" + norm + ":§" + n;
            e.entity_type = EntityType::LEGAL_PROVISION;
            e.text        = node.number + (node.heading.empty() ? "" : " " + node.heading);
            e.properties["source_doc"] = hierarchy.norm_abbreviation;
            e.properties["norm"]    = hierarchy.norm_abbreviation;
            e.properties["number"]  = node.number;
            e.properties["heading"] = node.heading;
            e.properties["text"]    = node.text.substr(
                0, std::min(node.text.size(), static_cast<std::size_t>(500)));
            entities.push_back(std::move(e));

            // Absätze
            for (const auto& child : node.children) {
                if (child.type == GesetzNodeType::ABSATZ) {
                    BaseEntity ae;
                    // Extract digit from "(N)"
                    std::string absn = child.number;
                    const auto d = absn.find_first_of("0123456789");
                    if (d != std::string::npos)
                        absn = absn.substr(d, absn.find_first_not_of("0123456789", d) - d);
                    ae.id         = "law:" + norm + ":§" + n + ":Abs" + absn;
                    ae.entity_type = EntityType::LEGAL_PROVISION;
                    ae.text        = "§" + n + " Abs." + absn;
                    ae.properties["source_doc"] = hierarchy.norm_abbreviation;
                    ae.properties["paragraph"] = "§" + n;
                    ae.properties["absatz"]    = child.number;
                    ae.properties["text"]      = child.text.substr(
                        0, std::min(child.text.size(), static_cast<std::size_t>(300)));
                    entities.push_back(std::move(ae));
                }
            }
        }
    });

    return entities;
}

// ─────────────────────────────────────────────────────────────────────────────
// TemporalExtractor
// ─────────────────────────────────────────────────────────────────────────────

std::string TemporalExtractor::normaliseDate(const std::string& raw) {
    const std::string s = themis::utils::trim(raw);

    // Numeric: DD.MM.YYYY
    static const std::regex re_dmy(R"((\d{1,2})\.(\d{1,2})\.(\d{4}))");
    std::smatch m;
    if (std::regex_search(s, m, re_dmy)) {
        return m[3].str() + "-" + pad2(m[2].str()) + "-" + pad2(m[1].str());
    }

    // With German month name: D. Monatname YYYY or D. Monatname YYYY
    static const std::regex re_gm(
        R"((\d{1,2})\.\s*([A-Za-zÄäÖöÜüß]+)\s+(\d{4}))");
    if (std::regex_search(s, m, re_gm)) {
        const std::string mm = parseGermanMonth(m[2].str());
        if (!mm.empty()) {
            return m[3].str() + "-" + mm + "-" + pad2(m[1].str());
        }
    }

    // ISO: already YYYY-MM-DD
    static const std::regex re_iso(R"(\d{4}-\d{2}-\d{2})");
    if (std::regex_search(s, m, re_iso)) {
        return m[0].str();
    }

    return s; // return raw if not parseable
}

TemporalValidity TemporalExtractor::extract(const std::string& text) const {
    TemporalValidity tv;

    // Patterns for effective_from
    static const std::regex re_from(
        R"((?:in Kraft (?:getreten|tretend) am|tritt\s+am|gilt ab|gültig\s+ab|Inkrafttreten[:\s]+am?)\s+(\d{1,2}[.\s]\w+[.\s]\d{4}|\d{4}-\d{2}-\d{2}))",
        std::regex::icase);

    // Patterns for effective_to
    static const std::regex re_to(
        R"((?:außer Kraft (?:getreten|tretend) am|aufgehoben(?:\s+mit Wirkung)? (?:vom?|am)|befristet bis(?:\s+zum?)?)\s+(\d{1,2}[.\s]\w+[.\s]\d{4}|\d{4}-\d{2}-\d{2}))",
        std::regex::icase);

    std::smatch m;
    if (std::regex_search(text, m, re_from)) {
        tv.effective_from = normaliseDate(m[1].str());
        tv.source_hint    = m[0].str();
    }
    if (std::regex_search(text, m, re_to)) {
        tv.effective_to   = normaliseDate(m[1].str());
        if (tv.source_hint.empty()) tv.source_hint = m[0].str();
    }

    return tv;
}

TemporalValidity TemporalExtractor::merge(
    const TemporalValidity& text_val, const nlohmann::json& meta) const
{
    TemporalValidity merged = text_val;

    // Keys to check (in priority order)
    static const std::vector<std::string> from_keys{
        "effective_from", "Inkrafttreten", "gültig_ab", "date", "valid_from"};
    static const std::vector<std::string> to_keys{
        "effective_to", "Außerkrafttreten", "valid_to", "expires"};

    if (meta.is_object()) {
        for (const auto& k : from_keys) {
            if (meta.contains(k) && meta[k].is_string()) {
                merged.effective_from = normaliseDate(meta[k].get<std::string>());
                break;
            }
        }
        for (const auto& k : to_keys) {
            if (meta.contains(k) && meta[k].is_string()) {
                merged.effective_to = normaliseDate(meta[k].get<std::string>());
                break;
            }
        }
    }

    return merged;
}

// ─────────────────────────────────────────────────────────────────────────────
// BehoerdenMapper
// ─────────────────────────────────────────────────────────────────────────────

BehoerdenMapper::BehoerdenMapper() {
    // Built-in table: 30 common Federal laws
    builtin_ = {
        {"GG",       "Bundesrat / Bundesregierung"},
        {"BGB",      "Bundesministerium der Justiz"},
        {"HGB",      "Bundesministerium der Justiz"},
        {"StGB",     "Bundesministerium der Justiz"},
        {"StPO",     "Bundesministerium der Justiz"},
        {"ZPO",      "Bundesministerium der Justiz"},
        {"VwGO",     "Bundesministerium der Justiz"},
        {"VwVfG",    "Bundesministerium des Innern"},
        {"BImSchG",  "Umweltbundesamt / Bundesministerium für Umwelt"},
        {"WHG",      "Umweltbundesamt"},
        {"KrWG",     "Umweltbundesamt"},
        {"BNatSchG", "Bundesamt für Naturschutz"},
        {"DSGVO",    "Bundesbeauftragte(r) für den Datenschutz"},
        {"BDSG",     "Bundesbeauftragte(r) für den Datenschutz"},
        {"TKG",      "Bundesnetzagentur"},
        {"TMG",      "Bundesministerium für Wirtschaft und Klimaschutz"},
        {"SGB",      "Bundesministerium für Arbeit und Soziales"},
        {"SGB V",    "Bundesministerium für Gesundheit"},
        {"AMG",      "Bundesamt für Arzneimittel und Medizinprodukte"},
        {"IfSG",     "Robert Koch-Institut / Bundesministerium für Gesundheit"},
        {"GewO",     "Bundesministerium für Wirtschaft und Klimaschutz"},
        {"ProdSG",   "Bundesanstalt für Arbeitsschutz und Arbeitsmedizin"},
        {"BauGB",    "Bundesministerium für Wohnen, Stadtentwicklung und Bauwesen"},
        {"BauNVO",   "Bundesministerium für Wohnen, Stadtentwicklung und Bauwesen"},
        {"EnWG",     "Bundesnetzagentur"},
        {"UVPG",     "Umweltbundesamt"},
        {"ROG",      "Bundesministerium für Wohnen, Stadtentwicklung und Bauwesen"},
        {"WaffG",    "Bundeskriminalamt"},
        {"AsylG",    "Bundesamt für Migration und Flüchtlinge"},
        {"AufenthG", "Ausländerbehörden / BAMF"},
    };
}

std::optional<std::string> BehoerdenMapper::lookupAuthority(
    const std::string& norm) const
{
    // Custom overrides first
    auto it = custom_.find(norm);
    if (it != custom_.end()) return it->second;

    // Built-in
    it = builtin_.find(norm);
    if (it != builtin_.end()) return it->second;

    // Fallback function
    if (fallback_) {
        const std::string r = fallback_(norm);
        if (!r.empty()) return r;
    }

    return std::nullopt;
}

void BehoerdenMapper::addMapping(const std::string& norm,
                                  const std::string& authority) {
    custom_[norm] = authority;
}

void BehoerdenMapper::setFallback(
    std::function<std::string(const std::string&)> fn) {
    fallback_ = std::move(fn);
}

std::size_t BehoerdenMapper::mappingCount() const {
    return builtin_.size() + custom_.size();
}

// ─────────────────────────────────────────────────────────────────────────────
// BescheidExtractor
// ─────────────────────────────────────────────────────────────────────────────

BescheidEntity BescheidExtractor::extract(const std::string& text) const {
    BescheidEntity be;

    // Aktenzeichen
    static const std::regex re_az(
        R"((?:Aktenzeichen|Az\.|Geschäftszeichen|AZ)\s*:\s*([^\r\n]+))",
        std::regex::icase);
    // Antragsteller
    static const std::regex re_ant(
        R"((?:Antragsteller(?:in)?|Antragstellende(?:r)?)[:\s]+([^\n,;.]{3,80}))",
        std::regex::icase);
    // Datum
    static const std::regex re_dat(
        R"((?:Bescheid vom|Datum[:\s]+|Ausgefertigt am|Erlassen am)[:\s]+(\d{1,2}\.\s*\w+\.?\s*\d{4}|\d{4}-\d{2}-\d{2}))",
        std::regex::icase);
    // Behörde
    static const std::regex re_beh(
        R"((?:Behörde|Erlassen durch|Ausstellende Behörde|Zuständige Behörde)[:\s]+([^\n,;.]{3,100}))",
        std::regex::icase);
    // Auflagen
    static const std::regex re_aufl_header(
        R"((?:^|\n)\s*(?:Auflagen?|Nebenbestimmungen?)\s*[:\n])",
        std::regex::icase);
    static const std::regex re_aufl_item(
        R"((?:^|\n)\s*(?:\d+[.)]\s*|[A-Z]\)\s*|-\s*)([^\n]{5,200}))",
        std::regex::icase);
    static const std::regex re_neben(
        R"((?:^|\n)\s*Nebenbestimmung[:\s]+([^\n]{5,200}))",
        std::regex::icase);

    std::smatch m;

    if (std::regex_search(text, m, re_az))  be.aktenzeichen  = themis::utils::trim(m[1].str());
    if (std::regex_search(text, m, re_ant)) be.antragsteller = themis::utils::trim(m[1].str());
    if (std::regex_search(text, m, re_dat))
        be.bescheid_datum = TemporalExtractor::normaliseDate(m[1].str());
    if (std::regex_search(text, m, re_beh)) be.behoerde = themis::utils::trim(m[1].str());

    // Extract Auflagen section
    if (std::regex_search(text, m, re_aufl_header)) {
        const std::string suffix = text.substr(static_cast<std::size_t>(m.position() + m.length()));
        // Take at most 2000 chars of the Auflagen section
        const std::string section = suffix.substr(0, std::min(suffix.size(), static_cast<std::size_t>(2000)));
        auto ai = std::sregex_iterator(section.begin(), section.end(), re_aufl_item);
        for (auto ae = std::sregex_iterator(); ai != ae; ++ai) {
            const std::string item = themis::utils::trim((*ai)[1].str());
            if (!item.empty()) be.auflagen.push_back(item);
        }
    }

    // Nebenbestimmungen
    auto ni = std::sregex_iterator(text.begin(), text.end(), re_neben);
    for (auto ne = std::sregex_iterator(); ni != ne; ++ni) {
        const std::string nb = themis::utils::trim((*ni)[1].str());
        if (!nb.empty()) be.nebenbestimmungen.push_back(nb);
    }

    return be;
}

BaseEntity BescheidExtractor::toEntity(const BescheidEntity& be,
                                        const std::string& source_doc) const
{
    BaseEntity e;
    if (!be.aktenzeichen.empty()) {
        e.id = "bescheid:" + be.aktenzeichen;
    } else {
        e.id = "bescheid:" + std::to_string(fnv1a32(source_doc));
    }
    e.entity_type = EntityType::LEGAL_DECISION;
    e.text = be.aktenzeichen.empty() ? "Bescheid" : "Bescheid " + be.aktenzeichen;
    e.properties["label"] = e.text;
    e.properties["source_doc"] = source_doc;
    e.properties["aktenzeichen"]   = be.aktenzeichen;
    e.properties["antragsteller"]  = be.antragsteller;
    e.properties["bescheid_datum"] = be.bescheid_datum;
    e.properties["behoerde"]       = be.behoerde;
    if (!be.auflagen.empty()) {
        // Serialize auflagen list as a semicolon-delimited string
        std::string auflagen_str;
        for (std::size_t i = 0; i < be.auflagen.size(); ++i) {
            if (i > 0) auflagen_str += "; ";
            auflagen_str += be.auflagen[i];
        }
        e.properties["auflagen"] = auflagen_str;
    }
    return e;
}

// ─────────────────────────────────────────────────────────────────────────────
// CrossDocumentLinker
// ─────────────────────────────────────────────────────────────────────────────

namespace {

/// Normalise a canonical ID for comparison (lowercase, trim).
std::string normId(const std::string& id) {
    std::string s = toLower(themis::utils::trim(id));
    // Remove trailing colon
    while (!s.empty() && s.back() == ':') s.pop_back();
    return s;
}

} // anonymous namespace

std::vector<EntityRelation> CrossDocumentLinker::linkDocuments(
    const ExtractionContext& ctx1, const ExtractionContext& ctx2) const
{
    std::vector<EntityRelation> edges;

    const auto& entities1  = ctx1.entities;
    const auto& entities2  = ctx2.entities;
    const std::string src1 = ctx1.manifest.original_path;
    const std::string src2 = ctx2.manifest.original_path;

    // Build ID set for ctx2 entities
    std::unordered_map<std::string, const BaseEntity*> id_map;
    for (const auto& e : entities2) {
        id_map[normId(e.id)] = &e;
    }

    // Match norm_reference / law entities in ctx1 against ctx2 entities
    for (const auto& e1 : entities1) {
        if (e1.entity_type != EntityType::LEGAL_NORM_REFERENCE
            && e1.entity_type != EntityType::LEGAL_PROVISION) {
            continue;
        }

        const auto it = id_map.find(normId(e1.id));
        if (it != id_map.end()) {
            EntityRelation rel;
            rel.from_id       = e1.id;
            rel.to_id         = it->second->id;
            rel.relation_type = RelationType::CITES;
            rel.properties["weight"] = "1.0";
            rel.properties["source_doc"] = src1;
            rel.properties["target_doc"] = src2;
            edges.push_back(std::move(rel));
        }

        // Secondary: match by label suffix
        for (const auto& e2 : entities2) {
            if (normId(e2.id) == normId(e1.id)) continue;
            const auto label2 = toLower(entityDisplayLabel(e2));
            const auto id1    = normId(e1.id);
            const auto colon  = id1.rfind(':');
            const auto suffix = (colon != std::string::npos) ? id1.substr(colon + 1) : id1;
            if (!suffix.empty() && label2.find(suffix) != std::string::npos) {
                EntityRelation rel;
                rel.from_id       = e1.id;
                rel.to_id         = e2.id;
                rel.relation_type = RelationType::CITES;
                rel.properties["weight"] = "0.7";
                rel.properties["source_doc"] = src1;
                rel.properties["target_doc"] = src2;
                rel.properties["match"]      = "label";
                edges.push_back(std::move(rel));
                break;
            }
        }
    }

    return edges;
}

std::vector<EntityRelation> CrossDocumentLinker::linkDocumentBatch(
    const ExtractionContext&              source,
    const std::vector<ExtractionContext>& targets) const
{
    std::vector<EntityRelation> all;
    for (const auto& tgt : targets) {
        auto edges = linkDocuments(source, tgt);
        all.insert(all.end(),
                   std::make_move_iterator(edges.begin()),
                   std::make_move_iterator(edges.end()));
    }
    return all;
}

// ─────────────────────────────────────────────────────────────────────────────
// LegalEntityExport
// ─────────────────────────────────────────────────────────────────────────────

std::string LegalEntityExport::escapeIriComponent(const std::string& s) {
    // Percent-encode characters that are unsafe in IRI path components
    static const std::string safe =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-._~:@!$&'()*+,;=";
    std::string out;
    out.reserve(s.size() * 3);
    for (const unsigned char c : s) {
        if (safe.find(static_cast<char>(c)) != std::string::npos) {
            out += static_cast<char>(c);
        } else {
            char buf[4];
            std::snprintf(buf, sizeof(buf), "%%%02X", c);
            out += buf;
        }
    }
    return out;
}

std::string LegalEntityExport::escapeTurtleLiteral(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 16);
    for (char c : s) {
        switch (c) {
        case '"':  out += "\\\""; break;
        case '\\': out += "\\\\"; break;
        case '\n': out += "\\n";  break;
        case '\r': out += "\\r";  break;
        default:   out += c;      break;
        }
    }
    return out;
}

nlohmann::json LegalEntityExport::exportJsonLd(
    const BaseEntitySet& es, const std::string& base_iri) const
{
    nlohmann::json doc;
    doc["@context"] = {
        {"@base",        base_iri},
        {"@vocab",       "https://schema.org/"},
        {"themis",       "https://themisdb.io/legal/"},
        {"juris",        "https://juris.bundesrecht.de/vocabulary/"},
        {"rdfs",         "http://www.w3.org/2000/01/rdf-schema#"},
        {"dc",           "http://purl.org/dc/elements/1.1/"},
        {"id",           "@id"},
        {"type",         "@type"},
        {"label",        "rdfs:label"},
        {"source_doc",   "dc:source"},
        {"relation_type","themis:relationType"},
        {"weight",       "themis:weight"},
    };

    nlohmann::json graph = nlohmann::json::array();

    // Sort entities for deterministic output
    auto entities = es.nodes;
    std::sort(entities.begin(), entities.end(),
              [](const BaseEntity& a, const BaseEntity& b) { return a.id < b.id; });

    for (const auto& e : entities) {
        nlohmann::json node;
        node["@id"]      = base_iri + escapeIriComponent(e.id);
        node["@type"]    = entityTypeName(e.entity_type);
        node["rdfs:label"] = entityDisplayLabel(e);
        const auto src_it = e.properties.find("source_doc");
        if (src_it != e.properties.end()) {
            node["dc:source"] = src_it->second;
        }
        for (const auto& [k, v] : e.properties) {
            node["themis:" + k] = v;
        }
        graph.push_back(std::move(node));
    }

    auto relations = es.edges;
    std::sort(relations.begin(), relations.end(),
              [](const EntityRelation& a, const EntityRelation& b) {
                  return a.from_id < b.from_id ||
                         (a.from_id == b.from_id && a.to_id < b.to_id);
              });

    for (const auto& r : relations) {
        nlohmann::json edge;
        edge["@type"]               = "themis:Relation";
        edge["themis:from"]         = base_iri + escapeIriComponent(r.from_id);
        edge["themis:to"]           = base_iri + escapeIriComponent(r.to_id);
        edge["themis:relationType"] = relationTypeName(r.relation_type);
        const auto w_it = r.properties.find("weight");
        if (w_it != r.properties.end()) {
            edge["themis:weight"] = w_it->second;
        }
        graph.push_back(std::move(edge));
    }

    doc["@graph"] = std::move(graph);
    return doc;
}

std::string LegalEntityExport::buildTurtle(
    const BaseEntitySet& es, const std::string& base) const
{
    std::ostringstream out;
    out << "@base <" << base << "> .\n";
    out << "@prefix rdfs: <http://www.w3.org/2000/01/rdf-schema#> .\n";
    out << "@prefix dc:   <http://purl.org/dc/elements/1.1/> .\n";
    out << "@prefix themis: <https://themisdb.io/legal/> .\n";
    out << "@prefix xsd:  <http://www.w3.org/2001/XMLSchema#> .\n\n";

    auto entities = es.nodes;
    std::sort(entities.begin(), entities.end(),
              [](const BaseEntity& a, const BaseEntity& b) { return a.id < b.id; });

    for (const auto& e : entities) {
        const std::string iri = "<" + escapeIriComponent(e.id) + ">";
        out << iri << "\n";
        out << "    a themis:" << entityTypeName(e.entity_type) << " ;\n";
        out << "    rdfs:label \"" << escapeTurtleLiteral(entityDisplayLabel(e)) << "\" ;\n";
        out << "    dc:identifier \"" << escapeTurtleLiteral(e.id) << "\" ;\n";
        const auto src_it = e.properties.find("source_doc");
        if (src_it != e.properties.end()) {
            out << "    dc:source \"" << escapeTurtleLiteral(src_it->second) << "\" ;\n";
        }
        for (const auto& [k, v] : e.properties) {
            out << "    themis:" << k << " \"" << escapeTurtleLiteral(v) << "\" ;\n";
        }
        out << "    .\n\n";
    }

    auto relations = es.edges;
    std::sort(relations.begin(), relations.end(),
              [](const EntityRelation& a, const EntityRelation& b) {
                  return a.from_id < b.from_id;
              });

    for (const auto& r : relations) {
        out << "<" << escapeIriComponent(r.from_id) << ">\n";
        out << "    themis:" << toLower(relationTypeName(r.relation_type))
            << " <" << escapeIriComponent(r.to_id) << "> .\n\n";
    }

    return out.str();
}

std::string LegalEntityExport::buildNTriples(
    const BaseEntitySet& es, const std::string& base) const
{
    std::ostringstream out;
    const std::string p_label  = "<http://www.w3.org/2000/01/rdf-schema#label>";
    const std::string p_type   = "<http://www.w3.org/1999/02/22-rdf-syntax-ns#type>";
    const std::string p_source = "<http://purl.org/dc/elements/1.1/source>";

    auto entities = es.nodes;
    std::sort(entities.begin(), entities.end(),
              [](const BaseEntity& a, const BaseEntity& b) { return a.id < b.id; });

    for (const auto& e : entities) {
        const std::string s = "<" + base + escapeIriComponent(e.id) + ">";
        out << s << " " << p_type
            << " <https://themisdb.io/legal/" << entityTypeName(e.entity_type) << "> .\n";
        out << s << " " << p_label
            << " \"" << escapeTurtleLiteral(entityDisplayLabel(e)) << "\" .\n";
        const auto src_it = e.properties.find("source_doc");
        if (src_it != e.properties.end())
            out << s << " " << p_source
                << " \"" << escapeTurtleLiteral(src_it->second) << "\" .\n";
    }

    auto relations = es.edges;
    std::sort(relations.begin(), relations.end(),
              [](const EntityRelation& a, const EntityRelation& b) {
                  return a.from_id < b.from_id;
              });

    for (const auto& r : relations) {
        const std::string pred =
            "<https://themisdb.io/legal/" + toLower(relationTypeName(r.relation_type)) + ">";
        out << "<" << base << escapeIriComponent(r.from_id) << "> "
            << pred << " "
            << "<" << base << escapeIriComponent(r.to_id) << "> .\n";
    }

    return out.str();
}

std::string LegalEntityExport::exportRdf(
    const BaseEntitySet& es, RdfFormat format,
    const std::string& base_iri) const
{
    if (format == RdfFormat::TURTLE)   return buildTurtle(es, base_iri);
    if (format == RdfFormat::N_TRIPLES) return buildNTriples(es, base_iri);
    return {};
}

} // namespace ingestion
} // namespace themis
