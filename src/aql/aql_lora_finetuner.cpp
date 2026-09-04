/**
 * @file aql_lora_finetuner.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=4, M=16, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "aql/aql_lora_finetuner.h"

#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <optional>
#include <sstream>
#include <stdexcept>

#include "llm/adapter_registry.h"
#include "llm/lora_framework/lora_training_service.h"

#ifdef THEMIS_ENABLE_LOGGING
#include <spdlog/spdlog.h>
#define AQL_LOG_INFO(...) spdlog::info(__VA_ARGS__)
#define AQL_LOG_WARN(...) spdlog::warn(__VA_ARGS__)
#define AQL_LOG_ERROR(...) spdlog::error(__VA_ARGS__)
#else
#define AQL_LOG_INFO(...)                                                                                              \
    do {                                                                                                               \
    } while (0)
#define AQL_LOG_WARN(...)                                                                                              \
    do {                                                                                                               \
    } while (0)
#define AQL_LOG_ERROR(...)                                                                                             \
    do {                                                                                                               \
    } while (0)
#endif

namespace themis {
namespace aql {

// ============================================================================
// Helpers
// ============================================================================

namespace {

/// Return an ISO 8601 UTC timestamp string.
std::string isoTimestamp() {
    auto now      = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm_utc{};
#ifdef _WIN32
    gmtime_s(&tm_utc, &t);
#else
    gmtime_r(&t, &tm_utc);
#endif
    std::ostringstream oss = {};
    oss << std::put_time(&tm_utc, "%Y-%m-%dT%H:%M:%SZ");
    return oss.str();
}

/// Build a TrainingDataSample from a NL/AQL pair with category metadata.
TrainingDataSample makeSample(const std::string &input, const std::string &output, AQLSampleCategory cat) {
    TrainingDataSample s;
    s.input                = input;
    s.output               = output;
    s.metadata["category"] = static_cast<int>(cat);
    return s;
}

} // anonymous namespace

// ============================================================================
// AQLDatasetBuilder – built-in sample tables
// ============================================================================

// ----------------------------------------------------------------------------
// Relational samples
// ----------------------------------------------------------------------------

void AQLDatasetBuilder::addRelationalSamples() {
    using C = AQLSampleCategory;

    samples_.push_back(makeSample("Find all users", "FOR u IN users RETURN u", C::NL_TO_AQL));

    samples_.push_back(
        makeSample("Find all users older than 30", "FOR u IN users FILTER u.age > 30 RETURN u", C::NL_TO_AQL));

    samples_.push_back(makeSample("Find the top 10 users sorted by name",
                                  "FOR u IN users SORT u.name ASC LIMIT 10 RETURN u", C::NL_TO_AQL));

    samples_.push_back(makeSample("Count the number of orders per customer",
                                  "FOR o IN orders\n"
                                  "  COLLECT customer_id = o.customer_id WITH COUNT INTO cnt\n"
                                  "  RETURN { customer_id, count: cnt }",
                                  C::NL_TO_AQL));

    samples_.push_back(makeSample("Return the total sales amount per city",
                                  "FOR o IN orders\n"
                                  "  COLLECT city = o.city\n"
                                  "  AGGREGATE total = SUM(o.amount)\n"
                                  "  SORT total DESC\n"
                                  "  RETURN { city, total }",
                                  C::NL_TO_AQL));

    samples_.push_back(makeSample("Join users with their orders",
                                  "FOR u IN users\n"
                                  "  FOR o IN orders\n"
                                  "    FILTER o.user_id == u._key\n"
                                  "    RETURN { user: u.name, order: o._key, total: o.amount }",
                                  C::NL_TO_AQL));

    samples_.push_back(makeSample("Upsert a document in the products collection",
                                  "UPSERT { sku: @sku }\n"
                                  "  INSERT { sku: @sku, name: @name, price: @price }\n"
                                  "  UPDATE { price: @price }\n"
                                  "  IN products",
                                  C::NL_TO_AQL));

    samples_.push_back(makeSample("Delete all inactive accounts",
                                  "FOR a IN accounts\n"
                                  "  FILTER a.active == false\n"
                                  "  REMOVE a IN accounts",
                                  C::NL_TO_AQL));
}

// ----------------------------------------------------------------------------
// Graph traversal samples
// ----------------------------------------------------------------------------

void AQLDatasetBuilder::addGraphSamples() {
    using C = AQLSampleCategory;

    samples_.push_back(makeSample("Find all friends of a user up to depth 2",
                                  "FOR v IN 1..2 OUTBOUND 'users/alice' friends\n"
                                  "  RETURN v.name",
                                  C::NL_TO_AQL));

    samples_.push_back(makeSample("Find any connected nodes of a given document in both directions",
                                  "FOR v, e IN 1..3 ANY @start_vertex connections\n"
                                  "  RETURN { vertex: v, edge: e }",
                                  C::NL_TO_AQL));

    samples_.push_back(makeSample("Find the shortest path between two users",
                                  "FOR v, e IN OUTBOUND\n"
                                  "  SHORTEST_PATH 'users/alice' TO 'users/bob'\n"
                                  "  GRAPH 'social_graph'\n"
                                  "  RETURN { vertex: v.name, edge: e }",
                                  C::NL_TO_AQL));

    samples_.push_back(makeSample("Traverse inbound edges of a product node in the recommendation graph",
                                  "FOR v, e IN 1..2 INBOUND 'products/42' recommendations\n"
                                  "  FILTER e.score > 0.8\n"
                                  "  SORT e.score DESC\n"
                                  "  RETURN { product: v.name, score: e.score }",
                                  C::NL_TO_AQL));
}

// ----------------------------------------------------------------------------
// Vector similarity samples
// ----------------------------------------------------------------------------

void AQLDatasetBuilder::addVectorSamples() {
    using C = AQLSampleCategory;

    samples_.push_back(makeSample("Find the 10 most similar documents to a query vector",
                                  "FOR doc IN documents\n"
                                  "  FILTER SIMILARITY(doc.embedding, @query_vector, 10)\n"
                                  "  RETURN doc",
                                  C::NL_TO_AQL));

    samples_.push_back(makeSample("Find products similar to a given embedding with cosine similarity above 0.85",
                                  "FOR p IN products\n"
                                  "  LET score = COSINE_SIMILARITY(p.embedding, @query_vec)\n"
                                  "  FILTER score >= 0.85\n"
                                  "  SORT score DESC\n"
                                  "  LIMIT 20\n"
                                  "  RETURN { product: p.name, score }",
                                  C::NL_TO_AQL));

    samples_.push_back(makeSample("Hybrid search: filter by category and vector similarity",
                                  "FOR doc IN articles\n"
                                  "  FILTER doc.category == @category\n"
                                  "  FILTER SIMILARITY(doc.embedding, @query_embedding, 5)\n"
                                  "  RETURN { title: doc.title, snippet: doc.summary }",
                                  C::NL_TO_AQL));
}

// ----------------------------------------------------------------------------
// Geo-spatial samples
// ----------------------------------------------------------------------------

void AQLDatasetBuilder::addGeoSamples() {
    using C = AQLSampleCategory;

    samples_.push_back(makeSample("Find all stores within 5 km of a location",
                                  "FOR s IN stores\n"
                                  "  FILTER ST_DISTANCE(s.location, ST_POINT(@lon, @lat)) < 5000\n"
                                  "  RETURN { name: s.name, distance: ST_DISTANCE(s.location, ST_POINT(@lon, @lat)) }",
                                  C::NL_TO_AQL));

    samples_.push_back(makeSample("Check whether a point is inside a polygon region",
                                  "LET region = ST_POLYGON(@coordinates)\n"
                                  "LET point  = ST_POINT(@lon, @lat)\n"
                                  "RETURN ST_WITHIN(point, region)",
                                  C::NL_TO_AQL));

    samples_.push_back(makeSample("Find all delivery zones that overlap with a bounding box",
                                  "FOR zone IN delivery_zones\n"
                                  "  FILTER ST_INTERSECTS(zone.polygon, ST_ENVELOPE(@bbox))\n"
                                  "  RETURN zone.name",
                                  C::NL_TO_AQL));
}

// ----------------------------------------------------------------------------
// Timeseries samples
// ----------------------------------------------------------------------------

void AQLDatasetBuilder::addTimeseriesSamples() {
    using C = AQLSampleCategory;

    samples_.push_back(makeSample("Get the average CPU usage per hour for the last 24 hours",
                                  "FOR m IN metrics\n"
                                  "  FILTER m.type == 'cpu' AND m.timestamp >= DATE_SUBTRACT(NOW(), 1, 'day')\n"
                                  "  COLLECT hour = DATE_TRUNC(m.timestamp, 'hour')\n"
                                  "  AGGREGATE avg_cpu = AVG(m.value)\n"
                                  "  SORT hour ASC\n"
                                  "  RETURN { hour, avg_cpu }",
                                  C::NL_TO_AQL));

    samples_.push_back(makeSample("Calculate a 5-minute rolling average of temperature readings",
                                  "FOR r IN sensor_readings\n"
                                  "  FILTER r.sensor_id == @sensor_id\n"
                                  "  WINDOW { preceding: 4, following: 0 } AGGREGATE avg_temp = AVG(r.temperature)\n"
                                  "  RETURN { ts: r.timestamp, avg_temp }",
                                  C::NL_TO_AQL));

    samples_.push_back(makeSample("Find timestamps where latency spiked above the 95th percentile",
                                  "LET p95 = (\n"
                                  "  FOR r IN latency_log\n"
                                  "    SORT r.latency_ms ASC\n"
                                  "    COLLECT AGGREGATE cnt = COUNT(r.latency_ms)\n"
                                  "    RETURN ELEMENT_AT(SORTED_UNIQUE(latency_log[*].latency_ms), FLOOR(cnt * 0.95))\n"
                                  ")[0]\n"
                                  "FOR r IN latency_log\n"
                                  "  FILTER r.latency_ms > p95\n"
                                  "  RETURN { ts: r.timestamp, latency_ms: r.latency_ms }",
                                  C::NL_TO_AQL));
}

// ----------------------------------------------------------------------------
// LLM extension samples
// ----------------------------------------------------------------------------

void AQLDatasetBuilder::addLLMExtensionSamples() {
    using C = AQLSampleCategory;

    // INFER
    samples_.push_back(makeSample("Run inference with the default model to summarise a document",
                                  "LLM INFER CONCAT('Summarise the following: ', @text)\n"
                                  "  OPTIONS { max_tokens: 256, temperature: 0.3 }",
                                  C::NL_TO_AQL));

    // RAG
    samples_.push_back(makeSample("Answer a question using the documentation collection",
                                  "LLM RAG @question\n"
                                  "  FROM COLLECTION documentation\n"
                                  "  TOP 5\n"
                                  "  USING MODEL 'llama-3-8b'\n"
                                  "  OPTIONS { temperature: 0.2 }",
                                  C::NL_TO_AQL));

    // EMBED
    samples_.push_back(makeSample("Generate an embedding for a text field and store it",
                                  "FOR doc IN articles\n"
                                  "  LET emb = LLM EMBED(doc.body) USING MODEL 'all-minilm-l6-v2'\n"
                                  "  UPDATE doc WITH { embedding: emb } IN articles",
                                  C::NL_TO_AQL));

    // MODEL load
    samples_.push_back(makeSample("Load a GGUF model into the LLM runtime",
                                  "LLM MODEL LOAD 'llama-3-8b'\n"
                                  "  FROM '/models/llama-3-8b-instruct.gguf'\n"
                                  "  OPTIONS { gpu_layers: 32, context_size: 8192 }",
                                  C::NL_TO_AQL));

    // MODEL unload
    samples_.push_back(makeSample("Unload an LLM model to free memory", "LLM MODEL UNLOAD 'llama-3-8b'", C::NL_TO_AQL));

    // MODEL list
    samples_.push_back(makeSample("List all loaded LLM models", "LLM MODEL LIST", C::NL_TO_AQL));
}

// ----------------------------------------------------------------------------
// LoRA command samples
// ----------------------------------------------------------------------------

void AQLDatasetBuilder::addLoraCmdSamples() {
    using C = AQLSampleCategory;

    samples_.push_back(makeSample("Load a LoRA adapter for domain-specific inference",
                                  "LLM LORA LOAD 'legal-adapter'\n"
                                  "  FROM '/adapters/legal-v1.gguf'\n"
                                  "  FOR MODEL 'llama-3-8b'\n"
                                  "  OPTIONS { scale: 0.8 }",
                                  C::AQL_LORA_CMD));

    samples_.push_back(makeSample("List all currently loaded LoRA adapters", "LLM LORA LIST", C::AQL_LORA_CMD));

    samples_.push_back(makeSample("Unload a LoRA adapter", "LLM LORA UNLOAD 'legal-adapter'", C::AQL_LORA_CMD));

    samples_.push_back(makeSample("Run inference using a LoRA adapter for technical documentation",
                                  "LLM INFER @prompt\n"
                                  "  USING MODEL 'llama-3-8b'\n"
                                  "  USING LORA  'tech-docs-adapter'\n"
                                  "  OPTIONS { max_tokens: 512, temperature: 0.5 }",
                                  C::AQL_LORA_CMD));

    samples_.push_back(makeSample("Train a LoRA adapter from a query collection",
                                  "TRAIN ADAPTER 'aql-adapter'\n"
                                  "  FROM aql_training_pairs\n"
                                  "  WITH { base_model: 'mistral-7b', rank: 8, alpha: 16, epochs: 3 }",
                                  C::AQL_LORA_CMD));
}

// ----------------------------------------------------------------------------
// DDL samples
// ----------------------------------------------------------------------------

void AQLDatasetBuilder::addDDLSamples() {
    using C = AQLSampleCategory;

    samples_.push_back(makeSample("Create a new collection called events", "CREATE COLLECTION events", C::NL_TO_AQL));

    samples_.push_back(makeSample("Drop the legacy_data collection", "DROP COLLECTION legacy_data", C::NL_TO_AQL));

    samples_.push_back(
        makeSample("Create a vector index on the embedding field of the documents collection",
                   "CREATE INDEX idx_embedding ON documents\n"
                   "  TYPE VECTOR\n"
                   "  FIELDS ['embedding']\n"
                   "  OPTIONS { dimensions: 384, metric: 'cosine', hnsw: { m: 16, ef_construction: 200 } }",
                   C::NL_TO_AQL));

    samples_.push_back(makeSample("Create a full-text search index on the body field",
                                  "CREATE INDEX idx_body_fts ON articles\n"
                                  "  TYPE FULLTEXT\n"
                                  "  FIELDS ['body']",
                                  C::NL_TO_AQL));
}

// ============================================================================
// AQLDatasetBuilder – public API
// ============================================================================

AQLDatasetBuilder &AQLDatasetBuilder::addBuiltinSamples() {
    addRelationalSamples();
    addGraphSamples();
    addVectorSamples();
    addGeoSamples();
    addTimeseriesSamples();
    addLLMExtensionSamples();
    addLoraCmdSamples();
    addDDLSamples();
    return *this;
}

AQLDatasetBuilder &AQLDatasetBuilder::addBuiltinSamplesForCategory(AQLSampleCategory cat) {
    switch (cat) {
        case AQLSampleCategory::NL_TO_AQL:
            addRelationalSamples();
            addGraphSamples();
            addVectorSamples();
            addGeoSamples();
            addTimeseriesSamples();
            addDDLSamples();
            break;
        case AQLSampleCategory::AQL_LORA_CMD:
            addLoraCmdSamples();
            break;
        default:
            addLLMExtensionSamples();
            break;
    }
    return *this;
}

AQLDatasetBuilder &AQLDatasetBuilder::addCustomSample(const std::string &nl_input, const std::string &aql_output,
                                                      AQLSampleCategory category) {
    samples_.push_back(makeSample(nl_input, aql_output, category));
    return *this;
}

AQLDatasetBuilder &AQLDatasetBuilder::loadFromJson(const std::string &json_path) {
    std::ifstream f(json_path);
    if (!f.is_open()) {
        throw std::runtime_error("AQLDatasetBuilder: cannot open dataset file: " + json_path);
    }
    json data;
    try {
        f >> data;
    } catch (const json::exception &e) {
        throw std::runtime_error(std::string("AQLDatasetBuilder: JSON parse error in ") + json_path + ": " + e.what());
    }
    return loadFromJsonObject(data);
}

AQLDatasetBuilder &AQLDatasetBuilder::loadFromJsonObject(const json &data) {
    if (!data.is_array()) {
        throw std::runtime_error("AQLDatasetBuilder: JSON dataset must be an array of objects");
    }
    for (const auto &item : data) {
        if (!item.is_object()) {
            continue;
        }
        TrainingDataSample s = {};
        if (item.contains("input")) {
            s.input = item["input"].get<std::string>();
        }
        if (item.contains("output")) {
            s.output = item["output"].get<std::string>();
        }
        if (item.contains("metadata")) {
            s.metadata = item["metadata"];
        }
        if (!s.input.empty() && !s.output.empty()) {
            samples_.push_back(std::move(s));
        }
    }
    return *this;
}

TrainingData AQLDatasetBuilder::build(const std::string &dataset_name) const {
    TrainingData dataset;
    dataset.dataset_name             = dataset_name;
    dataset.samples                  = samples_;
    dataset.metadata["created_at"]   = isoTimestamp();
    dataset.metadata["num_samples"]  = samples_.size();
    dataset.metadata["dataset_type"] = "aql_finetuning";
    return dataset;
}

std::size_t AQLDatasetBuilder::size() const {
    return static_cast<int>(samples_.size());
}

AQLDatasetBuilder &AQLDatasetBuilder::clear() {
    samples_.clear();
    return *this;
}

json AQLDatasetBuilder::toJson() const {
    json arr = json::array();
    for (const auto &s : samples_) {
        arr.push_back({{"input", s.input}, {"output", s.output}, {"metadata", s.metadata}});
    }
    return arr;
}

// ============================================================================
// AQLLoRAFinetuner::Config defaults
// ============================================================================

AQLLoRAFinetuner::Config::Config() {
    // AQL-optimised LoRA hyperparameters – use named constants so values are
    // documented and consistent with Config::fromOptions() defaults.
    hyperparameters.rank           = kDefaultRank;
    hyperparameters.alpha          = kDefaultAlpha;
    hyperparameters.dropout        = kDefaultDropout;
    hyperparameters.learning_rate  = kDefaultLearningRate;
    hyperparameters.batch_size     = kDefaultBatchSize;
    hyperparameters.num_epochs     = kDefaultEpochs;
    hyperparameters.max_seq_length = kDefaultMaxSeqLength;
    hyperparameters.optimizer      = "adamw";
    hyperparameters.lr_scheduler   = "cosine";
    hyperparameters.warmup_steps   = kDefaultWarmupSteps;
    hyperparameters.target_modules = {"q_proj", "v_proj", "k_proj", "o_proj"};
}

AQLLoRAFinetuner::Config
AQLLoRAFinetuner::Config::fromOptions(const std::unordered_map<std::string, std::string> &options) {
    Config cfg{}; // start from defaults

    auto it = options.find("rank");
    if (it != options.end()) {
        int val = std::stoi(it->second);
        if (val < 1 || val > 256) {
            throw std::invalid_argument("LoRA 'rank' must be in [1, 256], got " + it->second);
        }
        cfg.hyperparameters.rank = val;
    }

    it = options.find("alpha");
    if (it != options.end()) {
        float val = std::stof(it->second);
        if (val <= 0.0f) {
            throw std::invalid_argument("LoRA 'alpha' must be > 0, got " + it->second);
        }
        cfg.hyperparameters.alpha = val;
    }

    it = options.find("dropout");
    if (it != options.end()) {
        float val = std::stof(it->second);
        if (val < 0.0f || val >= 1.0f) {
            throw std::invalid_argument("LoRA 'dropout' must be in [0.0, 1.0), got " + it->second);
        }
        cfg.hyperparameters.dropout = val;
    }

    it = options.find("learning_rate");
    if (it != options.end()) {
        float val = std::stof(it->second);
        if (val <= 0.0f) {
            throw std::invalid_argument("LoRA 'learning_rate' must be > 0, got " + it->second);
        }
        cfg.hyperparameters.learning_rate = val;
    }

    it = options.find("batch_size");
    if (it != options.end()) {
        int val = std::stoi(it->second);
        if (val <= 0) {
            throw std::invalid_argument("LoRA 'batch_size' must be > 0, got " + it->second);
        }
        cfg.hyperparameters.batch_size = val;
    }

    it = options.find("epochs");
    if (it != options.end()) {
        int val = std::stoi(it->second);
        if (val <= 0) {
            throw std::invalid_argument("LoRA 'epochs' must be > 0, got " + it->second);
        }
        cfg.hyperparameters.num_epochs = val;
    }

    it = options.find("max_seq_length");
    if (it != options.end()) {
        int val = std::stoi(it->second);
        if (val <= 0) {
            throw std::invalid_argument("LoRA 'max_seq_length' must be > 0, got " + it->second);
        }
        cfg.hyperparameters.max_seq_length = val;
    }

    return cfg;
}

// ============================================================================
// AQLLoRAFinetuner::Impl
// ============================================================================

struct AQLLoRAFinetuner::Impl {
    Config config;
    std::shared_ptr<::themis::llm::lora::LoRATrainingService> training_service;
    std::shared_ptr<::themis::llm::AdapterRegistry> registry;
    AQLDatasetBuilder dataset_builder;
    bool trained = false;
    std::string trained_adapter_id = {};
    mutable std::mutex mutex;

    explicit Impl(const Config &cfg, std::shared_ptr<::themis::llm::lora::LoRATrainingService> svc)
        : config(cfg), training_service(std::move(svc)) {
        if (cfg.include_builtin_samples) {
            dataset_builder.addBuiltinSamples();
        }
        if (!cfg.extra_dataset_path.empty()) {
            dataset_builder.loadFromJson(cfg.extra_dataset_path);
        }
    }
};

// ============================================================================
// AQLLoRAFinetuner – lifecycle
// ============================================================================

AQLLoRAFinetuner::AQLLoRAFinetuner(const Config &config)
    : AQLLoRAFinetuner(config, std::make_shared<::themis::llm::lora::LoRATrainingService>()) {}

AQLLoRAFinetuner::AQLLoRAFinetuner(const Config &config,
                                   std::shared_ptr<::themis::llm::lora::LoRATrainingService> training_service)
    : impl_(std::make_unique<Impl>(config, std::move(training_service))) {}

AQLLoRAFinetuner::~AQLLoRAFinetuner() = default;

AQLLoRAFinetuner::AQLLoRAFinetuner(AQLLoRAFinetuner &&) noexcept            = default;
AQLLoRAFinetuner &AQLLoRAFinetuner::operator=(AQLLoRAFinetuner &&) noexcept = default;

// ============================================================================
// AQLLoRAFinetuner – training
// ============================================================================

TrainingResult AQLLoRAFinetuner::train() {
    std::lock_guard<std::mutex> lock(impl_->mutex);

    // Build the training dataset
    auto dataset = impl_->dataset_builder.build("themisdb_aql");

    if (static_cast<int>(dataset.size()) < impl_->config.min_training_samples) {
        throw std::runtime_error("AQLLoRAFinetuner: insufficient training samples (" + std::to_string(dataset.size())
                                 + " < " + std::to_string(impl_->config.min_training_samples) + " required)");
    }

    AQL_LOG_INFO("AQLLoRAFinetuner: training adapter '{}' on {} samples (base model: {})", impl_->config.adapter_id,
                 dataset.size(), impl_->config.base_model);

    // Wire epoch_callback through LoRATrainingService::registerCallback if provided
    if ([[maybe_unused]] impl_->config.epoch_callback) {
        auto cb = impl_->config.epoch_callback;
        impl_->training_service->registerCallback([[maybe_unused]] [cb](const ::themis::llm::lora::TrainingMetrics &metrics) {
            // Deliver a callback at the end of each epoch (status == "training"
            // and current_step == total_steps within that epoch, or per-epoch
            // transition).  We forward every metrics update; callers can
            // filter by metrics.current_epoch themselves.
            cb(metrics.current_epoch, static_cast<double>(metrics.current_loss));
        });
    }

    // Delegate to the LoRA training service
    auto result = impl_->training_service->trainOnTheFly(
        impl_->config.adapter_id, dataset, std::optional<LoRAHyperparameters>{impl_->config.hyperparameters});

    if (result.success) {
        impl_->trained            = true;
        impl_->trained_adapter_id = result.adapter_id.empty() ? impl_->config.adapter_id : result.adapter_id;

        AQL_LOG_INFO("AQLLoRAFinetuner: training complete – adapter_id='{}' final_loss={:.4f}",
                     impl_->trained_adapter_id, result.final_loss);

        // Register in the optional adapter registry
        if (impl_->registry) {
            ::themis::llm::AdapterMetadata meta;
            meta.adapter_id      = impl_->trained_adapter_id;
            meta.base_model_name = impl_->config.base_model;
            meta.domain          = "aql";
            meta.task_type       = "nl_to_aql";
            meta.status          = ::themis::llm::AdapterMetadata::Status::TRAINED;
            meta.created_at      = isoTimestamp();
            meta.updated_at      = meta.created_at;

            meta.training_config.dataset_name   = "themisdb_aql";
            meta.training_config.num_samples    = dataset.size();
            meta.training_config.epochs         = impl_->config.hyperparameters.num_epochs;
            meta.training_config.learning_rate  = static_cast<double>(impl_->config.hyperparameters.learning_rate);
            meta.training_config.lora_rank      = impl_->config.hyperparameters.rank;
            meta.training_config.lora_alpha     = static_cast<double>(impl_->config.hyperparameters.alpha);
            meta.training_config.lora_dropout   = static_cast<double>(impl_->config.hyperparameters.dropout);
            meta.training_config.target_modules = impl_->config.hyperparameters.target_modules;

            meta.quality_metrics.final_loss       = result.final_loss;
            meta.quality_metrics.training_samples = dataset.size();

            impl_->registry->registerAdapter(meta);
            AQL_LOG_INFO("AQLLoRAFinetuner: registered adapter '{}' in registry", impl_->trained_adapter_id);
        }
    } else {
        AQL_LOG_ERROR("AQLLoRAFinetuner: training failed – adapter_id='{}' error='{}'", impl_->config.adapter_id,
                      result.error_message);
    }

    return result;
}

void AQLLoRAFinetuner::addCustomSample(const std::string &nl_input, const std::string &aql_output,
                                       AQLSampleCategory category) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    impl_->dataset_builder.addCustomSample(nl_input, aql_output, category);
}

void AQLLoRAFinetuner::loadExtraDataset(const std::string &json_path) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    impl_->dataset_builder.loadFromJson(json_path);
}

// ============================================================================
// AQLLoRAFinetuner – adapter resolution
// ============================================================================

std::string AQLLoRAFinetuner::getAdapterID() const {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    return impl_->trained ? impl_->trained_adapter_id : impl_->config.adapter_id;
}

bool AQLLoRAFinetuner::isTrained() const {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    return impl_->trained;
}

// ============================================================================
// AQLLoRAFinetuner – dataset introspection
// ============================================================================

TrainingData AQLLoRAFinetuner::buildDataset() const {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    return impl_->dataset_builder.build("themisdb_aql");
}

json AQLLoRAFinetuner::exportDatasetJson() const {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    return impl_->dataset_builder.toJson();
}

// ============================================================================
// AQLLoRAFinetuner – registry integration
// ============================================================================

void AQLLoRAFinetuner::setAdapterRegistry(std::shared_ptr<::themis::llm::AdapterRegistry> registry) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    impl_->registry = std::move(registry);
}

} // namespace aql
} // namespace themis
