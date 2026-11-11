#include "training/docs_db_training_sample_provider.h"

#include "utils/rocksdb_open_compat.h"

#include <nlohmann/json.hpp>
#include <rocksdb/db.h>
#include <rocksdb/iterator.h>
#include <rocksdb/options.h>

#include <algorithm>
#include <filesystem>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <utility>

namespace themis::training {
namespace {

std::string stringField(const nlohmann::json& object, std::string_view key) {
    if (!object.is_object()) {
        return {};
    }
    const auto value = object.find(std::string(key));
    return value != object.end() && value->is_string()
        ? value->get<std::string>()
        : std::string{};
}

std::string documentPreview(const nlohmann::json& record) {
    const auto metadata = record.find("themis_metadata");
    if (metadata == record.end() || !metadata->is_object()) {
        return {};
    }
    const auto vector = metadata->find("vector");
    return vector != metadata->end() && vector->is_object()
        ? stringField(*vector, "text_content")
        : std::string{};
}

std::string trimText(std::string text) {
    const auto first = text.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return {};
    }
    const auto last = text.find_last_not_of(" \t\r\n");
    return text.substr(first, last - first + 1);
}

bool hasPrefix(const rocksdb::Slice& key, std::string_view prefix) noexcept {
    return key.size() >= prefix.size() &&
           std::string_view(key.data(), prefix.size()) == prefix;
}

std::optional<TrainingSampleRecord> makeSample(const nlohmann::json& record,
                                                const std::string& key,
                                                bool is_chunk) {
    std::string text = trimText(is_chunk ? stringField(record, "text") : documentPreview(record));
    if (text.size() < 64) {
        return std::nullopt;
    }

    const size_t split = text.find_first_of(" \t\r\n", text.size() / 2);
    if (split == std::string::npos || split < 32) {
        return std::nullopt;
    }
    const size_t target_begin = text.find_first_not_of(" \t\r\n", split);
    if (target_begin == std::string::npos || text.size() - target_begin < 32) {
        return std::nullopt;
    }

    TrainingSampleRecord sample;
    sample.sample_id = key;
    sample.source_uri = is_chunk
        ? stringField(record, "doc_id")
        : stringField(record, "file_path");
    if (sample.source_uri.empty()) {
        sample.source_uri = key;
    }
    sample.instruction = "Continue the official ThemisDB documentation faithfully.";
    sample.input = "Documentation source: " + sample.source_uri + "\n" + text.substr(0, split);
    sample.output = text.substr(target_begin);
    return sample;
}

} // namespace

class DocsDbTrainingSampleProvider::Impl {
public:
    explicit Impl(Config provider_config) : config(std::move(provider_config)) {}

    Config config;
    std::unique_ptr<rocksdb::DB> db;
    std::unique_ptr<rocksdb::Iterator> iterator;
    std::string key_prefix;
    size_t max_samples = 0;
    size_t batch_size = 0;
    size_t emitted_samples = 0;
    size_t scanned_records = 0;
    bool initialized = false;
};

DocsDbTrainingSampleProvider::DocsDbTrainingSampleProvider(Config config)
    : impl_(std::make_unique<Impl>(std::move(config))) {}

DocsDbTrainingSampleProvider::~DocsDbTrainingSampleProvider() = default;
DocsDbTrainingSampleProvider::DocsDbTrainingSampleProvider(DocsDbTrainingSampleProvider&&) noexcept = default;
DocsDbTrainingSampleProvider& DocsDbTrainingSampleProvider::operator=(DocsDbTrainingSampleProvider&&) noexcept = default;

void DocsDbTrainingSampleProvider::initialize(size_t max_samples, size_t batch_size) {
    namespace fs = std::filesystem;
    if (impl_->config.database_path.empty() || !fs::is_directory(impl_->config.database_path) ||
        impl_->config.max_record_bytes == 0 || impl_->config.max_records_to_scan == 0 ||
        max_samples == 0 || batch_size == 0) {
        throw std::invalid_argument("Invalid docs.db provider path or read bounds");
    }

    impl_->iterator.reset();
    impl_->db.reset();
    impl_->max_samples = max_samples;
    impl_->batch_size = batch_size;
    impl_->emitted_samples = 0;
    impl_->scanned_records = 0;
    impl_->initialized = false;

    rocksdb::Options options;
    options.create_if_missing = false;
    rocksdb::DB* raw_db = nullptr;
    const auto status = themis::storage::detail::openDbForReadOnlyCompat(
        options, impl_->config.database_path, &raw_db);
    impl_->db.reset(raw_db);
    if (!status.ok() || !impl_->db) {
        throw std::runtime_error("Failed to open docs.db read-only: " + status.ToString());
    }

    impl_->iterator.reset(impl_->db->NewIterator(rocksdb::ReadOptions{}));
    if (!impl_->iterator) {
        throw std::runtime_error("Failed to create a docs.db read-only iterator");
    }

    impl_->iterator->Seek("chunk_");
    if (impl_->iterator->Valid() && hasPrefix(impl_->iterator->key(), "chunk_")) {
        impl_->key_prefix = "chunk_";
    } else {
        impl_->iterator->Seek("doc_");
        if (!impl_->iterator->Valid() || !hasPrefix(impl_->iterator->key(), "doc_")) {
            if (!impl_->iterator->status().ok()) {
                throw std::runtime_error("Failed while locating docs.db records: " +
                                         impl_->iterator->status().ToString());
            }
            throw std::runtime_error("docs.db has no supported chunk_ or doc_ records");
        }
        impl_->key_prefix = "doc_";
    }
    impl_->initialized = true;
}

bool DocsDbTrainingSampleProvider::hasNext() const noexcept {
    return impl_ && impl_->initialized && impl_->iterator && impl_->iterator->Valid() &&
           impl_->emitted_samples < impl_->max_samples &&
            hasPrefix(impl_->iterator->key(), impl_->key_prefix);
}

std::vector<TrainingSampleRecord> DocsDbTrainingSampleProvider::nextBatch() {
    if (!impl_ || !impl_->initialized) {
        throw std::logic_error("DocsDbTrainingSampleProvider must be initialized before reading");
    }

    std::vector<TrainingSampleRecord> batch;
    batch.reserve(std::min(impl_->batch_size, impl_->max_samples - impl_->emitted_samples));
    while (hasNext() && batch.size() < impl_->batch_size) {
        const std::string key = impl_->iterator->key().ToString();
        const auto value = impl_->iterator->value();
        if (value.size() > impl_->config.max_record_bytes) {
            throw std::runtime_error("docs.db record exceeds max_record_bytes: " + key);
        }
        if (++impl_->scanned_records > impl_->config.max_records_to_scan) {
            throw std::runtime_error("docs.db scan exceeded max_records_to_scan");
        }

        nlohmann::json record;
        try {
            record = nlohmann::json::parse(value.data(), value.data() + value.size());
        } catch (const nlohmann::json::exception& exception) {
            throw std::runtime_error("Invalid JSON in docs.db record " + key + ": " + exception.what());
        }
        if (!record.is_object()) {
            throw std::runtime_error("docs.db record is not a JSON object: " + key);
        }
        if (auto sample = makeSample(record, key, impl_->key_prefix == "chunk_")) {
            batch.push_back(std::move(*sample));
            ++impl_->emitted_samples;
        }
        impl_->iterator->Next();
    }
    if (!impl_->iterator->status().ok()) {
        throw std::runtime_error("Failed while reading docs.db: " + impl_->iterator->status().ToString());
    }
    return batch;
}

std::string_view DocsDbTrainingSampleProvider::sourceName() const noexcept {
    return "docs.db";
}

} // namespace themis::training