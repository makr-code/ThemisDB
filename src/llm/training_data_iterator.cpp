/**
 * @file training_data_iterator.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 *
 * @note **Stub Implementation**: Iterator for training data batches.
 *       Provides efficient sequential or shuffled access to training samples.
 */

#include "llm/training_data_iterator.h"
#include "query/aql_parser.h"
#include "query/aql_runner.h"
#include "query/query_engine.h"

#include <algorithm>
#include <cctype>
#include <iterator>
#include <numeric>
#include <random>
#include <sstream>
#include <stdexcept>
#include <unordered_set>

#include <spdlog/spdlog.h>

namespace themis::llm {
namespace {

std::string jsonStringField(const nlohmann::json& row, const std::string& key) {
    if (!row.is_object() || !row.contains(key) || row[key].is_null()) {
        return {};
    }
    if (row[key].is_string()) {
        return row[key].get<std::string>();
    }
    return row[key].dump();
}

std::string firstStringField(const nlohmann::json& row,
                             const std::vector<std::string>& keys) {
    for (const auto& key : keys) {
        auto value = jsonStringField(row, key);
        if (!value.empty()) {
            return value;
        }
    }
    return {};
}

size_t countTokens(const std::string& text) {
    std::istringstream input(text);
    return static_cast<size_t>(std::distance(
        std::istream_iterator<std::string>(input), std::istream_iterator<std::string>()));
}

bool isValidCollectionName(const std::string& name) {
    if (name.empty() || !(std::isalpha(static_cast<unsigned char>(name.front())) || name.front() == '_')) {
        return false;
    }
    return std::all_of(name.begin() + 1, name.end(), [](unsigned char ch) {
        return std::isalnum(ch) || ch == '_';
    });
}

}  // namespace

// ---------------------------------------------------------------------------
// TrainingSample / TrainingBatch serialization
// ---------------------------------------------------------------------------

nlohmann::json TrainingDataIterator::TrainingSample::toJson() const {
    nlohmann::json result = {
        {"instruction", instruction},
        {"input", input},
        {"output", output},
        {"weight", weight},
        {"metadata", metadata}
    };
    if (graph_context) result["graph_context"] = *graph_context;
    if (vector_context) result["vector_context"] = *vector_context;
    if (relational_context) result["relational_context"] = *relational_context;
    return result;
}

TrainingDataIterator::TrainingSample TrainingDataIterator::TrainingSample::fromJson(
    const nlohmann::json& value) {
    TrainingSample sample;
    if (!value.is_object()) {
        return sample;
    }
    sample.instruction = firstStringField(value, {"instruction", "prompt", "question"});
    sample.input = firstStringField(value, {"input", "context"});
    sample.output = firstStringField(value, {"output", "answer", "completion"});
    if (value.contains("weight") && value["weight"].is_number()) {
        sample.weight = value["weight"].get<double>();
    }
    if (value.contains("metadata") && value["metadata"].is_object()) {
        for (auto it = value["metadata"].begin(); it != value["metadata"].end(); ++it) {
            sample.metadata[it.key()] = it.value().is_string()
                ? it.value().get<std::string>() : it.value().dump();
        }
    }
    if (value.contains("graph_context") && value["graph_context"].is_string()) {
        sample.graph_context = value["graph_context"].get<std::string>();
    }
    if (value.contains("vector_context") && value["vector_context"].is_string()) {
        sample.vector_context = value["vector_context"].get<std::string>();
    }
    if (value.contains("relational_context") && value["relational_context"].is_string()) {
        sample.relational_context = value["relational_context"].get<std::string>();
    }
    return sample;
}

size_t TrainingDataIterator::TrainingBatch::getTotalTokens() const {
    size_t total = 0;
    for (const auto& sample : samples) {
        total += countTokens(sample.instruction) + countTokens(sample.input) + countTokens(sample.output);
        if (sample.graph_context) total += countTokens(*sample.graph_context);
        if (sample.vector_context) total += countTokens(*sample.vector_context);
        if (sample.relational_context) total += countTokens(*sample.relational_context);
    }
    return total;
}

nlohmann::json TrainingDataIterator::TrainingBatch::toJson() const {
    nlohmann::json result = {
        {"batch_id", batch_id},
        {"total_batches", total_batches},
        {"samples", nlohmann::json::array()}
    };
    for (const auto& sample : samples) {
        result["samples"].push_back(sample.toJson());
    }
    return result;
}

// ---------------------------------------------------------------------------
// Iterator lifecycle
// ---------------------------------------------------------------------------

TrainingDataIterator::TrainingDataIterator(
    std::shared_ptr<RocksDBWrapper> db,
    std::shared_ptr<exporters::JSONLLLMExporter> exporter)
    : TrainingDataIterator(std::move(db), std::move(exporter), Config{}, nullptr) {
}

TrainingDataIterator::TrainingDataIterator(
    std::shared_ptr<RocksDBWrapper> db,
    std::shared_ptr<exporters::JSONLLLMExporter> exporter,
    query::QueryEngine* query_engine)
    : TrainingDataIterator(std::move(db), std::move(exporter), Config{}, query_engine) {
}

TrainingDataIterator::TrainingDataIterator(
    std::shared_ptr<RocksDBWrapper> db,
    std::shared_ptr<exporters::JSONLLLMExporter> exporter,
    Config config)
    : TrainingDataIterator(std::move(db), std::move(exporter), std::move(config), nullptr) {
}

TrainingDataIterator::TrainingDataIterator(
    std::shared_ptr<RocksDBWrapper> db,
    std::shared_ptr<exporters::JSONLLLMExporter> exporter,
    Config config,
    query::QueryEngine* query_engine)
    : db_(std::move(db)),
      exporter_(std::move(exporter)),
      query_engine_(query_engine),
      config_(std::move(config)) {
    if (config_.batch_size == 0) {
        throw std::invalid_argument("TrainingDataIterator batch_size must be positive");
    }
}

void TrainingDataIterator::setQueryEngine(query::QueryEngine* query_engine) noexcept {
    query_engine_ = query_engine;
    initialized_ = false;
    sample_buffer_.clear();
    sample_indices_.clear();
}

bool TrainingDataIterator::initialize(const std::string& aql_query,
                                      const AdapterMetadata& metadata) {
    (void)metadata;
    initialized_ = false;
    last_error_.clear();
    sample_buffer_.clear();
    sample_indices_.clear();
    current_position_ = 0;
    current_batch_id_ = 0;
    total_samples_ = 0;
    total_batches_ = 0;
    stats_ = IteratorStats{};
    start_time_ = std::chrono::steady_clock::now();

    if (!db_ || !exporter_ || !query_engine_) {
        last_error_ = "TrainingDataIterator requires an open DB, exporter, and QueryEngine";
        return false;
    }
    if (config_.batch_size == 0) {
        last_error_ = "TrainingDataIterator batch_size must be positive";
        return false;
    }

    if (!loadSamples(aql_query)) {
        return false;
    }

    total_samples_ = sample_buffer_.size();
    total_batches_ = (total_samples_ + config_.batch_size - 1) / config_.batch_size;
    stats_.total_samples = total_samples_;
    stats_.total_batches = total_batches_;
    shuffleSamples();
    initialized_ = true;
    return true;
}

bool TrainingDataIterator::hasNext() const {
    return initialized_ && current_position_ < sample_indices_.size();
}

std::optional<TrainingDataIterator::TrainingBatch> TrainingDataIterator::getNextBatch() {
    if (!hasNext()) {
        return std::nullopt;
    }

    TrainingBatch batch;
    batch.batch_id = current_batch_id_++;
    batch.total_batches = total_batches_;
    const size_t end = std::min(sample_indices_.size(), current_position_ + config_.batch_size);
    batch.samples.reserve(end - current_position_);
    while (current_position_ < end) {
        batch.samples.push_back(sample_buffer_[sample_indices_[current_position_++]]);
    }

    stats_.processed_samples += batch.samples.size();
    stats_.processed_batches++;
    stats_.total_tokens += batch.getTotalTokens();
    stats_.avg_samples_per_batch = stats_.processed_batches == 0
        ? 0.0
        : static_cast<double>(stats_.processed_samples) / stats_.processed_batches;
    stats_.avg_tokens_per_sample = stats_.processed_samples == 0
        ? 0.0
        : static_cast<double>(stats_.total_tokens) / stats_.processed_samples;
    stats_.iteration_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start_time_);

    if (config_.progress_callback && config_.progress_interval > 0 &&
        (stats_.processed_samples % config_.progress_interval == 0 ||
         stats_.processed_samples == stats_.total_samples)) {
        config_.progress_callback(stats_.processed_samples, stats_.total_samples);
    }
    return batch;
}

void TrainingDataIterator::reset() {
    current_position_ = 0;
    current_batch_id_ = 0;
    stats_.processed_samples = 0;
    stats_.processed_batches = 0;
    stats_.total_tokens = 0;
    stats_.avg_samples_per_batch = 0.0;
    stats_.avg_tokens_per_sample = 0.0;
    start_time_ = std::chrono::steady_clock::now();
    shuffleSamples();
}

void TrainingDataIterator::setConfig(const Config& config) {
    if (config.batch_size == 0) {
        throw std::invalid_argument("TrainingDataIterator batch_size must be positive");
    }
    config_ = config;
    initialized_ = false;
    sample_buffer_.clear();
    sample_indices_.clear();
    current_position_ = 0;
    current_batch_id_ = 0;
}

TrainingDataIterator::IteratorStats TrainingDataIterator::getStats() const {
    auto current = stats_;
    if (initialized_) {
        current.iteration_time = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start_time_);
    }
    return current;
}

bool TrainingDataIterator::loadSamples(const std::string& aql_query) {
    if (aql_query.empty()) {
        last_error_ = "TrainingDataIterator AQL query must not be empty";
        return false;
    }

    // AQLParser::parse accepts read queries; mutation statements use a separate parser.
    query::AQLParser parser;
    auto parsed_result = parser.parse(aql_query);
    if (!parsed_result) {
        last_error_ = "TrainingDataIterator rejected invalid or non-read AQL: " +
                      parsed_result.error().message();
        return false;
    }

    auto parsed_query = *parsed_result;
    if (!parsed_query || parsed_query->traversal || parsed_query->for_nodes.size() > 1 ||
        parsed_query->with_clause || parsed_query->collect) {
        last_error_ = "TrainingDataIterator supports one-collection read queries only";
        return false;
    }
    if (config_.max_samples == 0) {
        last_error_ = "TrainingDataIterator max_samples must be positive";
        return false;
    }
    if (!parsed_query->limit) {
        parsed_query->limit = std::make_shared<query::LimitNode>(
            0, static_cast<int64_t>(config_.max_samples));
    } else {
        parsed_query->limit->count = std::min(
            parsed_query->limit->count, static_cast<int64_t>(config_.max_samples));
    }

    auto translated = themis::AQLTranslator::translate(parsed_query);
    if (!translated.success || translated.join || translated.disjunctive ||
        translated.traversal || translated.vector_geo || translated.content_geo ||
        translated.spatial_join) {
        last_error_ = translated.success
            ? "TrainingDataIterator supports conjunctive collection reads only"
            : "TrainingDataIterator query translation failed: " + translated.error_message;
        return false;
    }

    auto result = query_engine_->executeAndEntitiesWithFallback(translated.conjunctive_query, true);
    if (!result) {
        last_error_ = "TrainingDataIterator AQL execution failed: " + result.error().message();
        return false;
    }

    std::unordered_set<std::string> seen;
    for (const auto& row : *result) {
        auto sample = convertToTrainingSample(row);
        if ((config_.enable_graph_context && !sample.graph_context) ||
            (config_.enable_vector_similarity && !sample.vector_context) ||
            (config_.enable_relational_joins && !sample.relational_context)) {
            last_error_ = "TrainingDataIterator requires precomputed context fields when enrichment is enabled";
            return false;
        }
        if (!passesQualityFilter(sample)) {
            if (!last_error_.empty()) {
                return false;
            }
            stats_.skipped_samples++;
            continue;
        }
        if (config_.quality_filter.skip_duplicates) {
            const std::string key = sample.instruction + '\x1f' + sample.input + '\x1f' + sample.output;
            if (!seen.insert(key).second) {
                stats_.skipped_samples++;
                continue;
            }
        }
        enrichSampleWithGraphContext(sample);
        enrichSampleWithVectorContext(sample);
        enrichSampleWithRelationalContext(sample);
        sample_buffer_.push_back(std::move(sample));
    }
    return true;
}

TrainingDataIterator::TrainingSample TrainingDataIterator::convertToTrainingSample(
    const BaseEntity& row) const {
    const auto& config = exporter_->getConfig();
    TrainingSample sample;
    const auto mapped = [&](const std::string& field, const std::vector<std::string>& fallback) {
        auto value = row.getFieldAsString(field);
        if (value && !value->empty()) {
            return *value;
        }
        for (const auto& key : fallback) {
            value = row.getFieldAsString(key);
            if (value && !value->empty()) {
                return *value;
            }
        }
        return std::string{};
    };

    switch (config.style) {
        case exporters::JSONLFormat::Style::CHAT_COMPLETION:
            sample.instruction = mapped(config.field_mapping.system_field, {"system", "instruction"});
            sample.input = mapped(config.field_mapping.user_field, {"user", "input", "prompt"});
            sample.output = mapped(config.field_mapping.assistant_field, {"assistant", "output", "answer"});
            break;
        case exporters::JSONLFormat::Style::TEXT_COMPLETION:
            sample.output = mapped(config.field_mapping.text_field, {"text", "output", "completion"});
            break;
        case exporters::JSONLFormat::Style::INSTRUCTION_TUNING:
        case exporters::JSONLFormat::Style::CUSTOM:
        default:
            sample.instruction = mapped(config.field_mapping.instruction_field,
                                        {"instruction", "prompt", "question"});
            sample.input = mapped(config.field_mapping.input_field, {"input", "context"});
            sample.output = mapped(config.field_mapping.output_field, {"output", "answer", "completion"});
            break;
    }

    if (config.weighting.enable_weights) {
        const auto weight = row.getFieldAsDouble(config.weighting.weight_field);
        if (weight) {
            sample.weight = *weight;
        }
    }
    const auto metadata_json = row.getFieldAsString("metadata");
    if (metadata_json) {
        try {
            const auto metadata = nlohmann::json::parse(*metadata_json);
            if (metadata.is_object()) {
                for (auto it = metadata.begin(); it != metadata.end(); ++it) {
                    sample.metadata[it.key()] = it.value().is_string()
                        ? it.value().get<std::string>() : it.value().dump();
                }
            }
        } catch (const nlohmann::json::exception&) {
            sample.metadata["metadata_parse_error"] = "true";
        }
    }
    sample.metadata["sample_id"] = row.getPrimaryKey();
    const auto context = [&](const char* key) -> std::optional<std::string> {
        const auto value = row.getFieldAsString(key);
        if (value && !value->empty()) {
            return *value;
        }
        const auto it = sample.metadata.find(key);
        return it == sample.metadata.end() ? std::nullopt : std::optional<std::string>(it->second);
    };
    sample.graph_context = context("graph_context");
    sample.vector_context = context("vector_context");
    sample.relational_context = context("relational_context");
    return sample;
}

void TrainingDataIterator::shuffleSamples() {
    sample_indices_.resize(sample_buffer_.size());
    std::iota(sample_indices_.begin(), sample_indices_.end(), size_t{0});
    if (config_.shuffle_data) {
        std::mt19937_64 rng(config_.random_seed);
        std::shuffle(sample_indices_.begin(), sample_indices_.end(), rng);
    }
}

void TrainingDataIterator::enrichSampleWithGraphContext(TrainingSample& sample) {
    if (config_.enable_graph_context && !sample.graph_context) {
        sample.graph_context = firstStringField(nlohmann::json(sample.metadata), {"graph_context"});
    }
}

void TrainingDataIterator::enrichSampleWithVectorContext(TrainingSample& sample) {
    if (config_.enable_vector_similarity && !sample.vector_context) {
        sample.vector_context = firstStringField(nlohmann::json(sample.metadata), {"vector_context"});
    }
}

void TrainingDataIterator::enrichSampleWithRelationalContext(TrainingSample& sample) {
    if (config_.enable_relational_joins && !sample.relational_context) {
        sample.relational_context = firstStringField(nlohmann::json(sample.metadata), {"relational_context"});
    }
}

bool TrainingDataIterator::passesQualityFilter(const TrainingSample& sample) {
    const auto& quality = exporter_->getConfig().quality;
    if (quality.skip_empty_outputs && sample.output.empty()) {
        return false;
    }
    if (!sample.output.empty() &&
        (sample.output.size() < quality.min_text_length || sample.output.size() > quality.max_text_length)) {
        return false;
    }
    if (quality.enable_toxicity_filter) {
        const auto score = sample.metadata.find("toxicity_score");
        if (score == sample.metadata.end()) {
            last_error_ = "toxicity filtering requires a precomputed metadata.toxicity_score";
            return false;
        }
        try {
            if (std::stod(score->second) > quality.max_toxicity_score) {
                return false;
            }
        } catch (const std::exception&) {
            last_error_ = "metadata.toxicity_score must be numeric";
            return false;
        }
    }
    return true;
}

nlohmann::json TrainingDataIterator::IteratorStats::toJson() const {
    return {
        {"total_samples", total_samples},
        {"processed_samples", processed_samples},
        {"skipped_samples", skipped_samples},
        {"total_batches", total_batches},
        {"processed_batches", processed_batches},
        {"avg_samples_per_batch", avg_samples_per_batch},
        {"avg_tokens_per_sample", avg_tokens_per_sample},
        {"total_tokens", total_tokens},
        {"iteration_time_ms", iteration_time.count()}
    };
}

}  // namespace themis::llm
