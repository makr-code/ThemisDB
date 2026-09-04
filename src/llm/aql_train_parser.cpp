/**
 * @file aql_train_parser.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=18, H=20, M=8, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "llm/aql_train_parser.h"
#include "utils/string_utils.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <regex>

namespace themis {
namespace llm {

// ============================================================================
// Internal helpers
// ============================================================================

namespace {

/// Lower-case a copy of @p s.
std::string toLower(const std::string& s) {
    std::string out = {};
    out.reserve(s.size());
    std::transform(s.begin(), s.end(), std::back_inserter(out),
                   [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
    return out;
}

/// Trim leading and trailing whitespace.
// Using themis::utils::trim() from string_utils.h (Phase 1 consolidation)

/// Strip surrounding single or double quotes from a token.
std::string stripQuotes(const std::string& s) {
    if (s.size() >= 2 &&
        ((s.front() == '\'' && s.back() == '\'') ||
         (s.front() == '"'  && s.back() == '"'))) {
        return s.substr(1, s.size() - 2);
    }
    return s;
}

/**
 * @brief Case-insensitive string comparison.
 */
bool iequal(const std::string& a, const std::string& b) {
    if (a.size() != b.size()) {
      return false;
    }
    return std::equal(a.begin(), a.end(), b.begin(),
                      [](unsigned char x, unsigned char y){
                          return std::tolower(x) == std::tolower(y);
                      });
}

int parseIntegerValue(const std::string& value, const char* field_name) {
    try {
        std::size_t parsed_chars = 0;
        const long long parsed = std::stoll(value, &parsed_chars);
        if (parsed_chars != value.size()) {
            throw std::invalid_argument("trailing characters");
        }
        if (parsed < std::numeric_limits<int>::min() ||
            parsed > std::numeric_limits<int>::max()) {
            throw std::out_of_range("out of int range");
        }
        return static_cast<int>(parsed);
    } catch (const std::invalid_argument&) {
        throw std::invalid_argument(
            std::string("AQLTrainParser: invalid integer value for ") +
            field_name + ": '" + value + "'");
    } catch (const std::out_of_range&) {
        throw std::invalid_argument(
            std::string("AQLTrainParser: invalid integer value for ") +
            field_name + ": '" + value + "'");
    }
}

double parseDoubleValue(const std::string& value, const char* field_name) {
    try {
        std::size_t parsed_chars = 0;
        const double parsed = std::stod(value, &parsed_chars);
        if (parsed_chars != value.size() || !std::isfinite(parsed)) {
            throw std::invalid_argument("invalid floating value");
        }
        return parsed;
    } catch (const std::invalid_argument&) {
        throw std::invalid_argument(
            std::string("AQLTrainParser: invalid numeric value for ") +
            field_name + ": '" + value + "'");
    } catch (const std::out_of_range&) {
        throw std::invalid_argument(
            std::string("AQLTrainParser: invalid numeric value for ") +
            field_name + ": '" + value + "'");
    }
}

/**
 * @brief Find the position of a keyword (case-insensitive) in @p s, or npos.
 * Searches for whole-word occurrences only (bounded by non-alpha boundaries).
 */
std::string::size_type findKeyword(const std::string& s, const std::string& keyword) {
    const std::string lower_s  = toLower(s);
    const std::string lower_kw = toLower(keyword);
    std::string::size_type pos = 0;
    while ((pos = lower_s.find(lower_kw, pos)) != std::string::npos) {
        // Check left boundary
        bool left_ok  = (pos == 0) || !std::isalnum(static_cast<unsigned char>(s[static_cast<int>(pos - 1)]));
        // Check right boundary
        bool right_ok = ((pos + lower_kw.size()) >= s.size()) ||
                        !std::isalnum(static_cast<unsigned char>(s[pos + lower_kw.size()]));
        if (left_ok && right_ok) {
          return pos;
        }
        pos += lower_kw.size();
    }
    return std::string::npos;
}

} // anonymous namespace

// ============================================================================
// TrainStatementConfig – JSON serialisation
// ============================================================================

nlohmann::json TrainStatementConfig::toJSON() const {
    nlohmann::json j;
    j["base_model_name"]    = base_model_name;
    j["sign_adapter"]       = sign_adapter;
    j["adapter_version"]    = adapter_version;
    j["compress_manifest"]  = compress_manifest;
    j["embed_safetensors"]  = embed_safetensors;
    j["validation_split"]   = validation_split;
    j["shuffle"]            = shuffle;
    j["random_seed"]        = random_seed;
    j["custom_metadata"]    = custom_metadata;
    // Quantization & size
    j["quantization_type"]  = static_cast<int>(quantization_type);
    j["size_mode"]          = static_cast<int>(size_mode);
    // Base TrainingConfig fields
    j["dataset_name"]       = dataset_name;
    j["epochs"]             = epochs;
    j["learning_rate"]      = learning_rate;
    j["lora_rank"]          = lora_rank;
    j["lora_alpha"]         = lora_alpha;
    j["lora_dropout"]       = lora_dropout;
    j["batch_size"]         = batch_size;
    j["max_seq_length"]     = max_seq_length;
    j["optimizer"]          = optimizer;
    return j;
}

TrainStatementConfig TrainStatementConfig::fromJSON(const nlohmann::json& j) {
    try {
        TrainStatementConfig cfg = {};
        if (j.contains("base_model_name")) {
          cfg.base_model_name   = j["base_model_name"].get<std::string>();
        }
        if (j.contains("sign_adapter")) {
          cfg.sign_adapter       = j["sign_adapter"].get<bool>();
        }
        if (j.contains("adapter_version")) {
          cfg.adapter_version    = j["adapter_version"].get<std::string>();
        }
        if (j.contains("compress_manifest")) {
          cfg.compress_manifest  = j["compress_manifest"].get<bool>();
        }
        if (j.contains("embed_safetensors")) {
          cfg.embed_safetensors  = j["embed_safetensors"].get<bool>();
        }
        if (j.contains("validation_split")) {
          cfg.validation_split   = j["validation_split"].get<double>();
        }
        if (j.contains("shuffle")) {
          cfg.shuffle            = j["shuffle"].get<bool>();
        }
        if (j.contains("random_seed")) {
          cfg.random_seed        = j["random_seed"].get<int>();
        }
        if (j.contains("custom_metadata")) {
          cfg.custom_metadata    = j["custom_metadata"].get<std::map<std::string,std::string>>();
        }
        if (j.contains("quantization_type")) {
          cfg.quantization_type  = static_cast<GGUFSTConfig::QuantizationType>(j["quantization_type"].get<int>());
        }
        if (j.contains("size_mode")) {
          cfg.size_mode          = static_cast<GGUFSTConfig::SizeMode>(j["size_mode"].get<int>());
        }
        // Base
        if (j.contains("dataset_name")) {
          cfg.dataset_name       = j["dataset_name"].get<std::string>();
        }
        if (j.contains("epochs")) {
          cfg.epochs             = j["epochs"].get<int>();
        }
        if (j.contains("learning_rate")) {
          cfg.learning_rate      = j["learning_rate"].get<double>();
        }
        if (j.contains("lora_rank")) {
          cfg.lora_rank          = j["lora_rank"].get<int>();
        }
        if (j.contains("lora_alpha")) {
          cfg.lora_alpha         = j["lora_alpha"].get<double>();
        }
        if (j.contains("lora_dropout")) {
          cfg.lora_dropout       = j["lora_dropout"].get<double>();
        }
        if (j.contains("batch_size")) {
          cfg.batch_size         = j["batch_size"].get<int>();
        }
        if (j.contains("max_seq_length")) {
          cfg.max_seq_length     = j["max_seq_length"].get<int>();
        }
        if (j.contains("optimizer")) {
          cfg.optimizer          = j["optimizer"].get<std::string>();
        }
        return cfg;
    } catch (const nlohmann::json::exception& ex) {
        throw std::invalid_argument(
            std::string("AQLTrainParser: type error in TrainStatementConfig JSON: ") + ex.what());
    }
}

// ============================================================================
// GraphContextConfig – JSON serialisation
// ============================================================================

nlohmann::json GraphContextConfig::toJSON() const {
    return {
        {"relationships", relationships},
        {"max_depth",     max_depth},
        {"direction",     direction},
        {"max_nodes",     max_nodes}
    };
}

GraphContextConfig GraphContextConfig::fromJSON(const nlohmann::json& j) {
    try {
        GraphContextConfig cfg = {};
        if (j.contains("relationships")) {
          cfg.relationships = j["relationships"].get<std::vector<std::string>>();
        }
        if (j.contains("max_depth")) {
          cfg.max_depth     = j["max_depth"].get<int>();
        }
        if (j.contains("direction")) {
          cfg.direction     = j["direction"].get<std::string>();
        }
        if (j.contains("max_nodes")) {
          cfg.max_nodes     = j["max_nodes"].get<int>();
        }
        return cfg;
    } catch (const nlohmann::json::exception& ex) {
        throw std::invalid_argument(
            std::string("AQLTrainParser: type error in GraphContextConfig JSON: ") + ex.what());
    }
}

// ============================================================================
// VectorSimilarityConfig – JSON serialisation
// ============================================================================

nlohmann::json VectorSimilarityConfig::toJSON() const {
    return {
        {"field",     field},
        {"threshold", threshold},
        {"top_k",     top_k},
        {"metric",    metric}
    };
}

VectorSimilarityConfig VectorSimilarityConfig::fromJSON(const nlohmann::json& j) {
    try {
        VectorSimilarityConfig cfg = {};
        if (j.contains("field")) {
          cfg.field     = j["field"].get<std::string>();
        }
        if (j.contains("threshold")) {
          cfg.threshold = j["threshold"].get<double>();
        }
        if (j.contains("top_k")) {
          cfg.top_k     = j["top_k"].get<int>();
        }
        if (j.contains("metric")) {
          cfg.metric    = j["metric"].get<std::string>();
        }
        return cfg;
    } catch (const nlohmann::json::exception& ex) {
        throw std::invalid_argument(
            std::string("AQLTrainParser: type error in VectorSimilarityConfig JSON: ") + ex.what());
    }
}

// ============================================================================
// RelationalJoinConfig – JSON serialisation
// ============================================================================

nlohmann::json RelationalJoinConfig::toJSON() const {
    return {
        {"collection",    collection},
        {"local_field",   local_field},
        {"foreign_field", foreign_field},
        {"join_type",     join_type}
    };
}

RelationalJoinConfig RelationalJoinConfig::fromJSON(const nlohmann::json& j) {
    try {
        RelationalJoinConfig cfg = {};
        if (j.contains("collection")) {
          cfg.collection    = j["collection"].get<std::string>();
        }
        if (j.contains("local_field")) {
          cfg.local_field   = j["local_field"].get<std::string>();
        }
        if (j.contains("foreign_field")) {
          cfg.foreign_field = j["foreign_field"].get<std::string>();
        }
        if (j.contains("join_type")) {
          cfg.join_type     = j["join_type"].get<std::string>();
        }
        return cfg;
    } catch (const nlohmann::json::exception& ex) {
        throw std::invalid_argument(
            std::string("AQLTrainParser: type error in RelationalJoinConfig JSON: ") + ex.what());
    }
}

// ============================================================================
// MultiModelEnrichment – JSON serialisation
// ============================================================================

nlohmann::json MultiModelEnrichment::toJSON() const {
    nlohmann::json j;
    if (graph_context) {
      j["graph_context"]    = graph_context->toJSON();
    }
    if (vector_similarity) {
      j["vector_similarity"] = vector_similarity->toJSON();
    }
    nlohmann::json joins = nlohmann::json::array();
    for (const auto& jc : relational_joins) {
      joins.push_back(jc.toJSON());
    }
    j["relational_joins"] = joins;
    return j;
}

MultiModelEnrichment MultiModelEnrichment::fromJSON(const nlohmann::json& j) {
    MultiModelEnrichment e = {};
    if (j.contains("graph_context")) {
      e.graph_context     = GraphContextConfig::fromJSON(j["graph_context"]);
    }
    if (j.contains("vector_similarity")) {
      e.vector_similarity = VectorSimilarityConfig::fromJSON(j["vector_similarity"]);
    }
    if (j.contains("relational_joins") && j["relational_joins"].is_array()) {
        for (const auto& jc : j["relational_joins"]) {
            e.relational_joins.push_back(RelationalJoinConfig::fromJSON(jc));
        }
    }
    return e;
}

// ============================================================================
// AQLDistributedTrainingConfig – JSON serialisation
// ============================================================================

nlohmann::json AQLDistributedTrainingConfig::toJSON() const {
    return {
        {"enabled",             enabled},
        {"sync_strategy",       sync_strategy},
        {"coordinator_shard",   coordinator_shard},
        {"participant_shards",  participant_shards},
        {"sync_frequency",      sync_frequency}
    };
}

AQLDistributedTrainingConfig AQLDistributedTrainingConfig::fromJSON(const nlohmann::json& j) {
    try {
        AQLDistributedTrainingConfig cfg = {};
        if (j.contains("enabled")) {
          cfg.enabled            = j["enabled"].get<bool>();
        }
        if (j.contains("sync_strategy")) {
          cfg.sync_strategy      = j["sync_strategy"].get<std::string>();
        }
        if (j.contains("coordinator_shard")) {
          cfg.coordinator_shard  = j["coordinator_shard"].get<std::string>();
        }
        if (j.contains("participant_shards")) {
          cfg.participant_shards = j["participant_shards"].get<std::vector<std::string>>();
        }
        if (j.contains("sync_frequency")) {
          cfg.sync_frequency     = j["sync_frequency"].get<int>();
        }
        return cfg;
    } catch (const nlohmann::json::exception& ex) {
        throw std::invalid_argument(
            std::string("AQLTrainParser: type error in AQLDistributedTrainingConfig JSON: ") + ex.what());
    }
}

// ============================================================================
// TrainAdapterStmt / DeployAdapterStmt / VerifyAdapterStmt / ListAdaptersStmt
// ============================================================================

nlohmann::json TrainAdapterStmt::toJSON() const {
    return {
        {"adapter_id",         adapter_id},
        {"source_collection",  source_collection},
        {"enrichment",         enrichment.toJSON()},
        {"config",             config.toJSON()},
        {"distributed",        distributed.toJSON()},
        {"output_path",        output_path}
    };
}

TrainAdapterStmt TrainAdapterStmt::fromJSON(const nlohmann::json& j) {
    try {
        TrainAdapterStmt s = {};
        if (j.contains("adapter_id")) {
          s.adapter_id        = j["adapter_id"].get<std::string>();
        }
        if (j.contains("source_collection")) {
          s.source_collection = j["source_collection"].get<std::string>();
        }
        if (j.contains("enrichment")) {
          s.enrichment        = MultiModelEnrichment::fromJSON(j["enrichment"]);
        }
        if (j.contains("config")) {
          s.config            = TrainStatementConfig::fromJSON(j["config"]);
        }
        if (j.contains("distributed")) {
          s.distributed       = AQLDistributedTrainingConfig::fromJSON(j["distributed"]);
        }
        if (j.contains("output_path")) {
          s.output_path       = j["output_path"].get<std::string>();
        }
        return s;
    } catch (const nlohmann::json::exception& ex) {
        throw std::invalid_argument(
            std::string("AQLTrainParser: type error in TrainAdapterStmt JSON: ") + ex.what());
    }
}

nlohmann::json DeployAdapterStmt::toJSON() const {
    return {
        {"adapter_id",            adapter_id},
        {"target_shards",         target_shards},
        {"strategy",              strategy},
        {"validate_compatibility", validate_compatibility},
        {"verify_signature",      verify_signature}
    };
}

DeployAdapterStmt DeployAdapterStmt::fromJSON(const nlohmann::json& j) {
    try {
        DeployAdapterStmt s = {};
        if (j.contains("adapter_id")) {
          s.adapter_id             = j["adapter_id"].get<std::string>();
        }
        if (j.contains("target_shards")) {
          s.target_shards          = j["target_shards"].get<std::vector<std::string>>();
        }
        if (j.contains("strategy")) {
          s.strategy               = j["strategy"].get<std::string>();
        }
        if (j.contains("validate_compatibility")) {
          s.validate_compatibility = j["validate_compatibility"].get<bool>();
        }
        if (j.contains("verify_signature")) {
          s.verify_signature       = j["verify_signature"].get<bool>();
        }
        return s;
    } catch (const nlohmann::json::exception& ex) {
        throw std::invalid_argument(
            std::string("AQLTrainParser: type error in DeployAdapterStmt JSON: ") + ex.what());
    }
}

nlohmann::json VerifyAdapterStmt::toJSON() const {
    return {
        {"adapter_id",              adapter_id},
        {"check_signature",         check_signature},
        {"check_manifest",          check_manifest},
        {"check_safetensors_match", check_safetensors_match}
    };
}

VerifyAdapterStmt VerifyAdapterStmt::fromJSON(const nlohmann::json& j) {
    try {
        VerifyAdapterStmt s = {};
        if (j.contains("adapter_id")) {
          s.adapter_id              = j["adapter_id"].get<std::string>();
        }
        if (j.contains("check_signature")) {
          s.check_signature         = j["check_signature"].get<bool>();
        }
        if (j.contains("check_manifest")) {
          s.check_manifest          = j["check_manifest"].get<bool>();
        }
        if (j.contains("check_safetensors_match")) {
          s.check_safetensors_match = j["check_safetensors_match"].get<bool>();
        }
        return s;
    } catch (const nlohmann::json::exception& ex) {
        throw std::invalid_argument(
            std::string("AQLTrainParser: type error in VerifyAdapterStmt JSON: ") + ex.what());
    }
}

nlohmann::json ListAdaptersStmt::toJSON() const {
    nlohmann::json j;
    if (base_model) {
      j["base_model"] = *base_model;
    }
    if (domain) {
      j["domain"]     = *domain;
    }
    j["order_by"]   = order_by;
    j["descending"] = descending;
    j["limit"]      = limit;
    return j;
}

ListAdaptersStmt ListAdaptersStmt::fromJSON(const nlohmann::json& j) {
    try {
        ListAdaptersStmt s = {};
        if (j.contains("base_model")) {
          s.base_model = j["base_model"].get<std::string>();
        }
        if (j.contains("domain")) {
          s.domain     = j["domain"].get<std::string>();
        }
        if (j.contains("order_by")) {
          s.order_by   = j["order_by"].get<std::string>();
        }
        if (j.contains("descending")) {
          s.descending = j["descending"].get<bool>();
        }
        if (j.contains("limit")) {
          s.limit      = j["limit"].get<int>();
        }
        return s;
    } catch (const nlohmann::json::exception& ex) {
        throw std::invalid_argument(
            std::string("AQLTrainParser: type error in ListAdaptersStmt JSON: ") + ex.what());
    }
}

// ============================================================================
// AQLTrainParser – private helpers
// ============================================================================

std::vector<std::string> AQLTrainParser::tokenize(const std::string& input) {
    std::vector<std::string> tokens;
    std::istringstream iss(input);
    std::string token = {};
    while (iss >> token) {
        tokens.push_back(token);
    }
    return tokens;
}

std::string AQLTrainParser::extractClause(
    const std::string& input,
    const std::string& keyword
) {
    auto pos = findKeyword(input, keyword);
    if (pos == std::string::npos) return {};

    // Skip the keyword itself
    const std::string rest = input.substr(pos + keyword.size());

    // Return everything until the next recognised top-level keyword
    static const std::vector<std::string> kTopKeywords = {
        "FROM", "WHERE", "USING", "WITH", "DISTRIBUTED", "OUTPUT",
        "DEPLOY", "VERIFY", "LIST", "ORDER", "LIMIT", "TO", "CHECK"
    };

    std::string::size_type end_pos = rest.size();
    for (const auto& kw : kTopKeywords) {
        auto kp = findKeyword(rest, kw);
        if (kp != std::string::npos && kp < end_pos) {
            end_pos = kp;
        }
    }
    return themis::utils::trim(rest.substr(0, end_pos));
}

std::map<std::string, std::string> AQLTrainParser::parseKeyValuePairs(
    const std::string& input
) {
    std::map<std::string, std::string> result;
    size_t pos = 0;
    const size_t n = input.size();

    auto skipWhitespace = [&]([[maybe_unused]] size_t& i) {
        while (i < n && std::isspace(static_cast<unsigned char>(input[i]))) {
            ++i;
        }
    };

    while (pos < n) {
        skipWhitespace(pos);
        if (pos < n && (input[pos] == ',' || input[pos] == '{' || input[pos] == '}')) {
            ++pos;
            continue;
        }
        if (pos >= n) {
            break;
        }

        const size_t keyStart = pos;
        while (pos < n) {
            const unsigned char ch = static_cast<unsigned char>(input[pos]);
            if (std::isalnum(ch) || input[pos] == '_') {
                ++pos;
            } else {
                break;
            }
        }
        if (pos == keyStart) {
            ++pos;
            continue;
        }

        std::string key = toLower(input.substr(keyStart, pos - keyStart));
        skipWhitespace(pos);
        if (pos >= n || (input[pos] != '=' && input[pos] != ':')) {
            continue;
        }
        ++pos;
        skipWhitespace(pos);

        std::string value = {};
        if (pos < n && (input[pos] == '\'' || input[pos] == '"')) {
            const char quote = input[pos++];
            const size_t valueStart = pos;
            while (pos < n && input[pos] != quote) {
                ++pos;
            }
            value = input.substr(valueStart, pos - valueStart);
            if (pos < n && input[pos] == quote) {
                ++pos;
            }
        } else {
            const size_t valueStart = pos;
            while (pos < n && input[pos] != ',' && input[pos] != '}') {
                ++pos;
            }
            value = themis::utils::trim(input.substr(valueStart, pos - valueStart));
        }

        result[key] = value;
    }
    return result;
}

void AQLTrainParser::validateAdapterName(const std::string& name) {
    if (name.empty()) {
        throw std::invalid_argument("AQLTrainParser: adapter name must not be empty");
    }
    // Allow alphanumerics, hyphens, underscores, dots
    for (char c : name) {
        if (!std::isalnum(static_cast<unsigned char>(c)) && c != '-' && c != '_' && c != '.') {
            throw std::invalid_argument(
                "AQLTrainParser: invalid character in adapter name: '" + std::string(1, c) + "'");
        }
    }
    if (static_cast<int>(name.size()) > 128) {
        throw std::invalid_argument("AQLTrainParser: adapter name exceeds 128 characters");
    }
}

void AQLTrainParser::validateBaseModel(const std::string& model) {
    if (model.empty()) {
        throw std::invalid_argument("AQLTrainParser: base_model must not be empty");
    }
}

void AQLTrainParser::validateConfig(const TrainStatementConfig& config) {
    if (config.epochs < 1 || config.epochs > 100) {
        throw std::invalid_argument("AQLTrainParser: epochs must be in [1, 100]");
    }
    if (config.learning_rate <= 0.0 || config.learning_rate > 1.0) {
        throw std::invalid_argument("AQLTrainParser: learning_rate must be in (0, 1]");
    }
    if (config.lora_rank < 1 || config.lora_rank > 256) {
        throw std::invalid_argument("AQLTrainParser: lora_rank must be in [1, 256]");
    }
    if (config.lora_alpha <= 0.0) {
        throw std::invalid_argument("AQLTrainParser: lora_alpha must be > 0");
    }
    if (config.lora_dropout < 0.0 || config.lora_dropout >= 1.0) {
        throw std::invalid_argument("AQLTrainParser: lora_dropout must be in [0, 1)");
    }
    if (config.batch_size < 1) {
        throw std::invalid_argument("AQLTrainParser: batch_size must be >= 1");
    }
    if (config.max_seq_length < 1) {
        throw std::invalid_argument("AQLTrainParser: max_seq_length must be >= 1");
    }
    if (config.validation_split <= 0.0 || config.validation_split > 1.0) {
        throw std::invalid_argument("AQLTrainParser: validation_split must be in (0, 1]");
    }
}

TrainStatementConfig AQLTrainParser::parseTrainingConfig(const std::string& with_clause) {
    TrainStatementConfig cfg;

    // Strip outer braces { ... } if present
    std::string content = themis::utils::trim(with_clause);
    if (!content.empty() && content.front() == '{') {
        // Try JSON parse first
        try {
            auto j = nlohmann::json::parse(content);
            return TrainStatementConfig::fromJSON(j);
        } catch (const nlohmann::json::parse_error&) {
            // JSON syntax error — fall through to key-value parsing
            if (content.front() == '{' && content.back() == '}') {
                content = themis::utils::trim(content.substr(1, content.size() - 2));
            }
        }
    }

    // Key-value pair parsing
    auto kv = parseKeyValuePairs(content);

    if (kv.count("base_model")) {
      cfg.base_model_name  = kv["base_model"];
    }
    if (kv.count("epochs")) {
      cfg.epochs           = parseIntegerValue(kv["epochs"], "epochs");
    }
    if (kv.count("learning_rate")) {
      cfg.learning_rate    = parseDoubleValue(kv["learning_rate"], "learning_rate");
    }
    if (kv.count("rank")) {
      cfg.lora_rank        = parseIntegerValue(kv["rank"], "rank");
    }
    if (kv.count("lora_rank")) {
      cfg.lora_rank        = parseIntegerValue(kv["lora_rank"], "lora_rank");
    }
    if (kv.count("alpha")) {
      cfg.lora_alpha       = parseDoubleValue(kv["alpha"], "alpha");
    }
    if (kv.count("lora_alpha")) {
      cfg.lora_alpha       = parseDoubleValue(kv["lora_alpha"], "lora_alpha");
    }
    if (kv.count("batch_size")) {
      cfg.batch_size       = parseIntegerValue(kv["batch_size"], "batch_size");
    }
    if (kv.count("optimizer")) {
      cfg.optimizer        = kv["optimizer"];
    }
    if (kv.count("sign_adapter")) {
      cfg.sign_adapter     = iequal(kv["sign_adapter"], "true");
    }
    if (kv.count("version")) {
      cfg.adapter_version  = kv["version"];
    }
    if (kv.count("dropout")) {
      cfg.lora_dropout     = parseDoubleValue(kv["dropout"], "dropout");
    }
    if (kv.count("lora_dropout")) {
      cfg.lora_dropout     = parseDoubleValue(kv["lora_dropout"], "lora_dropout");
    }
    if (kv.count("validation_split")) {
      cfg.validation_split = parseDoubleValue(kv["validation_split"], "validation_split");
    }
    if (kv.count("max_seq_length")) {
      cfg.max_seq_length   = parseIntegerValue(kv["max_seq_length"], "max_seq_length");
    }
    if (kv.count("seq_length")) {
      cfg.max_seq_length   = parseIntegerValue(kv["seq_length"], "seq_length");
    }

    return cfg;
}

GraphContextConfig AQLTrainParser::parseGraphContext(const std::string& args) {
    GraphContextConfig cfg;
    auto kv = parseKeyValuePairs(args);
    if (kv.count("max_depth")) {
      cfg.max_depth  = parseIntegerValue(kv["max_depth"], "max_depth");
    }
    if (kv.count("direction")) {
      cfg.direction  = kv["direction"];
    }
    if (kv.count("max_nodes")) {
      cfg.max_nodes  = parseIntegerValue(kv["max_nodes"], "max_nodes");
    }
    // Parse relationships: relationships = ['REL1', 'REL2']
    static const std::regex rel_re(R"(\[([^\]]*)\])");
    std::smatch m = {};
    if (std::regex_search(args, m, rel_re)) {
        std::istringstream iss(m[1].str());
        std::string rel = {};
        while (std::getline(iss, rel, ',')) {
            cfg.relationships.push_back(stripQuotes(themis::utils::trim(rel)));
        }
    }
    if (cfg.max_depth < 1) {
        throw std::invalid_argument("AQLTrainParser: max_depth must be >= 1");
    }
    if (cfg.max_nodes < 1) {
        throw std::invalid_argument("AQLTrainParser: max_nodes must be >= 1");
    }
    return cfg;
}

VectorSimilarityConfig AQLTrainParser::parseVectorSimilarity(const std::string& args) {
    VectorSimilarityConfig cfg;
    auto kv = parseKeyValuePairs(args);
    if (kv.count("field")) {
      cfg.field     = kv["field"];
    }
    if (kv.count("threshold")) {
      cfg.threshold = parseDoubleValue(kv["threshold"], "threshold");
    }
    if (kv.count("top_k")) {
      cfg.top_k     = parseIntegerValue(kv["top_k"], "top_k");
    }
    if (kv.count("metric")) {
      cfg.metric    = kv["metric"];
    }
    if (cfg.top_k < 1) {
        throw std::invalid_argument("AQLTrainParser: top_k must be >= 1");
    }
    if (cfg.threshold < 0.0 || cfg.threshold > 1.0) {
        throw std::invalid_argument("AQLTrainParser: threshold must be in [0, 1]");
    }
    return cfg;
}

RelationalJoinConfig AQLTrainParser::parseRelationalJoin(const std::string& args) {
    RelationalJoinConfig cfg;
    auto kv = parseKeyValuePairs(args);
    if (kv.count("collection")) {
      cfg.collection    = kv["collection"];
    }
    if (kv.count("local_field")) {
      cfg.local_field   = kv["local_field"];
    }
    if (kv.count("foreign_field")) {
      cfg.foreign_field = kv["foreign_field"];
    }
    if (kv.count("join_type")) {
      cfg.join_type     = kv["join_type"];
    }
    return cfg;
}

MultiModelEnrichment AQLTrainParser::parseEnrichment(const std::string& using_clauses) {
    MultiModelEnrichment e;
    const std::string lower = toLower(using_clauses);

    // USING GRAPH_CONTEXT(...)
    {
        static const std::regex gc_re(R"(GRAPH_CONTEXT\s*\(([^)]*)\))", std::regex_constants::icase);
        std::smatch m = {};
        std::string tmp = using_clauses;
        if (std::regex_search(tmp, m, gc_re)) {
            e.graph_context = parseGraphContext(m[1].str());
        }
    }

    // USING VECTOR_SIMILARITY(...)
    {
        static const std::regex vs_re(R"(VECTOR_SIMILARITY\s*\(([^)]*)\))", std::regex_constants::icase);
        std::smatch m = {};
        std::string tmp = using_clauses;
        if (std::regex_search(tmp, m, vs_re)) {
            e.vector_similarity = parseVectorSimilarity(m[1].str());
        }
    }

    // USING RELATIONAL_JOIN(...) – may appear multiple times
    {
        static const std::regex rj_re(R"(RELATIONAL_JOIN\s*\(([^)]*)\))", std::regex_constants::icase);
        auto it  = std::sregex_iterator(using_clauses.begin(), using_clauses.end(), rj_re);
        auto end = std::sregex_iterator{};
        for (; it != end; ++it) {
            e.relational_joins.push_back(parseRelationalJoin((*it)[1].str()));
        }
    }

    return e;
}

AQLDistributedTrainingConfig AQLTrainParser::parseDistributed(const std::string& aql) {
    AQLDistributedTrainingConfig cfg = {};
    if (findKeyword(aql, "DISTRIBUTED") == std::string::npos) {
      return cfg;
    }
    cfg.enabled = true;

    // COORDINATOR '<shard>'
    {
        static const std::regex coord_re(R"(COORDINATOR\s+'([^']+)')", std::regex_constants::icase);
        std::smatch m = {};
        std::string tmp = aql;
        if (std::regex_search(tmp, m, coord_re)) {
            cfg.coordinator_shard = m[1].str();
        }
    }

    // SHARDS '<s1>', '<s2>', ...
    {
        static const std::regex shards_re(R"(SHARDS\s+((?:'[^']+'(?:\s*,\s*)?)+))", std::regex_constants::icase);
        std::smatch m = {};
        std::string tmp = aql;
        if (std::regex_search(tmp, m, shards_re)) {
            static const std::regex shard_re(R"('([^']+)')");
            std::string shard_list = m[1].str();
            auto it  = std::sregex_iterator(shard_list.begin(), shard_list.end(), shard_re);
            auto end = std::sregex_iterator{};
            for (; it != end; ++it) {
                cfg.participant_shards.push_back((*it)[1].str());
            }
        }
    }

    return cfg;
}

// ============================================================================
// AQLTrainParser – public API
// ============================================================================

AQLTrainParser::StatementType AQLTrainParser::detectStatementType(
    const std::string& aql
) const {
    const std::string lower = toLower(themis::utils::trim(aql));
    if (lower.find("train adapter") != std::string::npos) {
      return StatementType::TRAIN_ADAPTER;
    }
    if (lower.find("deploy adapter") != std::string::npos) {
      return StatementType::DEPLOY_ADAPTER;
    }
    if (lower.find("verify adapter") != std::string::npos) {
      return StatementType::VERIFY_ADAPTER;
    }
    if (lower.find("list adapters")  != std::string::npos) {
      return StatementType::LIST_ADAPTERS;
    }
    return StatementType::UNKNOWN;
}

std::shared_ptr<TrainAdapterStmt> AQLTrainParser::parseTrainAdapter(
    const std::string& aql
) {
    auto stmt = std::make_shared<TrainAdapterStmt>();

    // ── adapter_id ──────────────────────────────────────────────────────────
    // Syntax: TRAIN ADAPTER <id> FROM ...
    {
        static const std::regex id_re(
            R"(TRAIN\s+ADAPTER\s+['"]?(\S+?)['"]?\s+FROM)",
            std::regex_constants::icase);
        std::smatch m = {};
        if (!std::regex_search(aql, m, id_re)) {
            throw std::invalid_argument(
                "AQLTrainParser: expected 'TRAIN ADAPTER <id> FROM' in: " + aql);
        }
        stmt->adapter_id = stripQuotes(m[1].str());
    }
    validateAdapterName(stmt->adapter_id);

    // ── source_collection ────────────────────────────────────────────────────
    {
        std::string from_clause = extractClause(aql, "FROM");
        // FROM is followed by a collection name (first token)
        if (from_clause.empty()) {
            throw std::invalid_argument(
                "AQLTrainParser: missing FROM clause in: " + aql);
        }
        auto tokens = tokenize(from_clause);
        if (!tokens.empty()) {
          stmt->source_collection = tokens[0];
        }
    }

    // ── enrichment (USING clauses) ─────────────────────────────────────────
    {
        auto pos_using = findKeyword(aql, "USING");
        auto pos_with  = findKeyword(aql, "WITH");
        if (pos_using != std::string::npos) {
            std::string using_part = aql.substr(pos_using);
            if (pos_with != std::string::npos && pos_with > pos_using) {
                using_part = aql.substr(pos_using, pos_with - pos_using);
            }
            stmt->enrichment = parseEnrichment(using_part);
        }
    }

    // ── training config (WITH clause) ────────────────────────────────────────
    {
        std::string with_clause = extractClause(aql, "WITH");
        if (!with_clause.empty()) {
            stmt->config = parseTrainingConfig(with_clause);
        }
    }

    // ── distributed ──────────────────────────────────────────────────────────
    stmt->distributed = parseDistributed(aql);

    // ── output_path ──────────────────────────────────────────────────────────
    {
        std::string output_clause = extractClause(aql, "OUTPUT");
        if (!output_clause.empty()) {
            auto output_tokens = tokenize(output_clause);
            if (!output_tokens.empty()) {
                stmt->output_path = stripQuotes(output_tokens[0]);
            }
        }
    }

    validateConfig(stmt->config);
    validateBaseModel(stmt->config.base_model_name);
    return stmt;
}

std::shared_ptr<DeployAdapterStmt> AQLTrainParser::parseDeployAdapter(
    const std::string& aql
) {
    auto stmt = std::make_shared<DeployAdapterStmt>();

    // DEPLOY ADAPTER <id> TO SHARD '<shard>' ...
    {
        static const std::regex id_re(
            R"(DEPLOY\s+ADAPTER\s+['"]?(\S+?)['"]?\s+TO)",
            std::regex_constants::icase);
        std::smatch m = {};
        if (!std::regex_search(aql, m, id_re)) {
            throw std::invalid_argument(
                "AQLTrainParser: expected 'DEPLOY ADAPTER <id> TO' in: " + aql);
        }
        stmt->adapter_id = stripQuotes(m[1].str());
    }
    validateAdapterName(stmt->adapter_id);

    // Collect shard identifiers
    {
        static const std::regex shard_re(R"('([^']+)')", std::regex_constants::icase);
        std::string to_section = aql;
        auto pos_to = findKeyword(to_section, "TO");
        if (pos_to != std::string::npos) {
            to_section = to_section.substr(pos_to);
            auto pos_with = findKeyword(to_section, "WITH");
            if (pos_with != std::string::npos) {
              to_section = to_section.substr(0, pos_with);
            }
        }
        auto it  = std::sregex_iterator(to_section.begin(), to_section.end(), shard_re);
        auto end = std::sregex_iterator{};
        for (; it != end; ++it) {
            stmt->target_shards.push_back((*it)[1].str());
        }
    }

    // Optional WITH clause
    {
        auto kv = parseKeyValuePairs(extractClause(aql, "WITH"));
        if (kv.count("strategy")) {
          stmt->strategy              = kv["strategy"];
        }
        if (kv.count("validate_compatibility")) {
          stmt->validate_compatibility = iequal(kv["validate_compatibility"], "true");
        }
        if (kv.count("verify_signature")) {
          stmt->verify_signature       = iequal(kv["verify_signature"], "true");
        }
    }

    if (stmt->target_shards.empty()) {
        throw std::invalid_argument(
            "AQLTrainParser: DEPLOY ADAPTER must specify at least one target shard");
    }

    return stmt;
}

std::shared_ptr<VerifyAdapterStmt> AQLTrainParser::parseVerifyAdapter(
    const std::string& aql
) {
    auto stmt = std::make_shared<VerifyAdapterStmt>();

    {
        static const std::regex id_re(
            R"(VERIFY\s+ADAPTER\s+['"]?(\S+?)['"]?(?:\s|$))",
            std::regex_constants::icase);
        std::smatch m = {};
        if (!std::regex_search(aql, m, id_re)) {
            throw std::invalid_argument(
                "AQLTrainParser: expected 'VERIFY ADAPTER <id>' in: " + aql);
        }
        stmt->adapter_id = stripQuotes(m[1].str());
    }
    validateAdapterName(stmt->adapter_id);

    // Parse CHECK flags
    const std::string lower = toLower(aql);
    if (lower.find("signature")         != std::string::npos) {
      stmt->check_signature         = true;
    }
    if (lower.find("manifest")          != std::string::npos) {
      stmt->check_manifest          = true;
    }
    if (lower.find("safetensors_match") != std::string::npos) {
      stmt->check_safetensors_match = true;
    }

    return stmt;
}

std::shared_ptr<ListAdaptersStmt> AQLTrainParser::parseListAdapters(
    const std::string& aql
) {
    auto stmt = std::make_shared<ListAdaptersStmt>();

    // WHERE clause – base_model / domain filters
    {
        std::string where_clause = extractClause(aql, "WHERE");
        auto kv = parseKeyValuePairs(where_clause);
        if (kv.count("base_model")) {
          stmt->base_model = kv["base_model"];
        }
        if (kv.count("domain")) {
          stmt->domain     = kv["domain"];
        }
    }

    // ORDER BY field [ASC|DESC]
    {
        auto pos_order = findKeyword(aql, "ORDER BY");
        if (pos_order != std::string::npos) {
            static const std::size_t kOrderByLen = std::string_view{"ORDER BY"}.size();
            std::string rest = themis::utils::trim(aql.substr(pos_order + kOrderByLen));
            auto tokens = tokenize(rest);
            if (!tokens.empty()) {
              stmt->order_by = tokens[0];
            }
            if (static_cast<int>(tokens.size()) > = 2 && iequal(tokens[1], "ASC")) {
              stmt->descending = false;
            }
        }
    }

    // LIMIT n
    {
        auto pos_limit = findKeyword(aql, "LIMIT");
        if (pos_limit != std::string::npos) {
            std::string rest = themis::utils::trim(aql.substr(pos_limit + 5));
            auto tokens = tokenize(rest);
            if (!tokens.empty()) {
                stmt->limit = parseIntegerValue(tokens[0], "limit");
                if (stmt->limit < 1) {
                    throw std::invalid_argument("AQLTrainParser: limit must be >= 1");
                }
            }
        }
    }

    return stmt;
}

// ============================================================================
// TrainingQueryBuilder
// ============================================================================

TrainingQueryBuilder& TrainingQueryBuilder::adapter(const std::string& id) {
    stmt_.adapter_id = id; return *this;
}

TrainingQueryBuilder& TrainingQueryBuilder::from(const std::string& collection) {
    stmt_.source_collection = collection; return *this;
}

TrainingQueryBuilder& TrainingQueryBuilder::where(const std::string& condition) {
    where_clause_ = condition; return *this;
}

TrainingQueryBuilder& TrainingQueryBuilder::withGraphContext(const GraphContextConfig& cfg) {
    stmt_.enrichment.graph_context = cfg; return *this;
}

TrainingQueryBuilder& TrainingQueryBuilder::withVectorSimilarity(const VectorSimilarityConfig& cfg) {
    stmt_.enrichment.vector_similarity = cfg; return *this;
}

TrainingQueryBuilder& TrainingQueryBuilder::withRelationalJoin(const RelationalJoinConfig& cfg) {
    stmt_.enrichment.relational_joins.push_back(cfg); return *this;
}

TrainingQueryBuilder& TrainingQueryBuilder::baseModel(const std::string& model) {
    stmt_.config.base_model_name = model; return *this;
}

TrainingQueryBuilder& TrainingQueryBuilder::loraRank([[maybe_unused]] int rank) {
    stmt_.config.lora_rank = rank; return *this;
}

TrainingQueryBuilder& TrainingQueryBuilder::epochs([[maybe_unused]] int n) {
    stmt_.config.epochs = n; return *this;
}

TrainingQueryBuilder& TrainingQueryBuilder::batchSize([[maybe_unused]] int size) {
    stmt_.config.batch_size = size; return *this;
}

TrainingQueryBuilder& TrainingQueryBuilder::learningRate([[maybe_unused]] double lr) {
    stmt_.config.learning_rate = lr; return *this;
}

TrainingQueryBuilder& TrainingQueryBuilder::quantization(GGUFSTConfig::QuantizationType q) {
    stmt_.config.quantization_type = q; return *this;
}

TrainingQueryBuilder& TrainingQueryBuilder::sizeMode(GGUFSTConfig::SizeMode m) {
    stmt_.config.size_mode = m; return *this;
}

TrainingQueryBuilder& TrainingQueryBuilder::signAdapter([[maybe_unused]] bool sign) {
    stmt_.config.sign_adapter = sign; return *this;
}

TrainingQueryBuilder& TrainingQueryBuilder::distributed(const AQLDistributedTrainingConfig& cfg) {
    stmt_.distributed = cfg; return *this;
}

TrainingQueryBuilder& TrainingQueryBuilder::outputPath(const std::string& path) {
    stmt_.output_path = path; return *this;
}

std::shared_ptr<TrainAdapterStmt> TrainingQueryBuilder::build() {
    return std::make_shared<TrainAdapterStmt>(stmt_);
}

std::string TrainingQueryBuilder::toAQL() const {
    std::ostringstream oss = {};
    oss << "TRAIN ADAPTER '" << stmt_.adapter_id << "'\n"
        << "  FROM " << stmt_.source_collection;

    if (!where_clause_.empty()) {
        oss << "\n  WHERE " << where_clause_;
    }

    if (stmt_.enrichment.hasEnrichment()) {
        if (stmt_.enrichment.graph_context) {
            const auto& gc = *stmt_.enrichment.graph_context;
            oss << "\n  USING GRAPH_CONTEXT(max_depth = " << gc.max_depth
                << ", direction = '" << gc.direction << "')";
        }
        if (stmt_.enrichment.vector_similarity) {
            const auto& vs = *stmt_.enrichment.vector_similarity;
            oss << "\n  USING VECTOR_SIMILARITY(field = '" << vs.field
                << "', top_k = " << vs.top_k << ")";
        }
        for (const auto& rj : stmt_.enrichment.relational_joins) {
            oss << "\n  USING RELATIONAL_JOIN(collection = '" << rj.collection << "')";
        }
    }

    if (stmt_.distributed.enabled) {
        oss << "\n  DISTRIBUTED";
        if (!stmt_.distributed.coordinator_shard.empty()) {
            oss << " COORDINATOR '" << stmt_.distributed.coordinator_shard << "'";
        }
    }

    oss << "\n  WITH {\n"
        << "    base_model:    '" << stmt_.config.base_model_name << "',\n"
        << "    rank:          "  << stmt_.config.lora_rank       << ",\n"
        << "    alpha:         "  << stmt_.config.lora_alpha      << ",\n"
        << "    epochs:        "  << stmt_.config.epochs          << ",\n"
        << "    learning_rate: "  << stmt_.config.learning_rate   << ",\n"
        << "    batch_size:    "  << stmt_.config.batch_size      << "\n"
        << "  }";

    if (!stmt_.output_path.empty()) {
        oss << "\n  OUTPUT '" << stmt_.output_path << "'";
    }

    return oss.str();
}

} // namespace llm
} // namespace themis
