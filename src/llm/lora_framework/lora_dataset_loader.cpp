/**
 * @file lora_dataset_loader.cpp
 * @brief Implementation of LoRADatasetLoader.
 */

#include "llm/lora_framework/lora_dataset_loader.h"

#include <algorithm>
#include <exception>
#include <utility>

namespace themis {
namespace llm {
namespace lora {

using json = nlohmann::json;

namespace {

bool isIdentStart(char c) {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_';
}

bool isIdentChar(char c) {
    return isIdentStart(c) || (c >= '0' && c <= '9');
}

std::size_t clampLimit(std::size_t limit) {
    return std::clamp<std::size_t>(limit, 1, LoRADatasetLoader::kMaxSamples);
}

}  // namespace

LoRADatasetLoader::LoRADatasetLoader(AqlExecutor executor) : executor_(std::move(executor)) {}

bool LoRADatasetLoader::isValidCollectionName(std::string_view name) {
    if (name.empty() || name.size() > 128 || !isIdentStart(name.front())) {
        return false;
    }
    return std::all_of(name.begin(), name.end(), isIdentChar);
}

std::optional<std::string> LoRADatasetLoader::buildDatasetQuery(std::string_view collection,
                                                                std::size_t limit) {
    if (!isValidCollectionName(collection)) {
        return std::nullopt;
    }
    std::string aql = "FOR d IN ";
    aql.append(collection);
    aql += " LIMIT ";
    aql += std::to_string(clampLimit(limit));
    aql += " RETURN d";
    return aql;
}

DatasetLoadResult LoRADatasetLoader::fromRows(const json& rows,
                                              const std::string& dataset_name,
                                              std::size_t limit) {
    DatasetLoadResult out;
    out.data.dataset_name = dataset_name;

    const json* arr = &rows;
    if (rows.is_object()) {
        const auto it = rows.find("results");
        if (it == rows.end()) {
            out.error = "dataset result has no 'results' array";
            return out;
        }
        arr = &*it;
    }
    if (!arr->is_array()) {
        out.error = "dataset result is not an array";
        return out;
    }

    const std::size_t max_samples = clampLimit(limit);
    for (const auto& row : *arr) {
        if (out.data.samples.size() >= max_samples) {
            break;
        }
        if (!row.is_object()) {
            ++out.skipped_rows;
            continue;
        }
        const auto in = row.find("input");
        const auto op = row.find("output");
        if (in == row.end() || op == row.end() || !in->is_string() || !op->is_string() ||
            in->get_ref<const std::string&>().empty() || op->get_ref<const std::string&>().empty()) {
            ++out.skipped_rows;
            continue;
        }
        TrainingDataSample sample;
        sample.input = in->get<std::string>();
        sample.output = op->get<std::string>();
        sample.metadata = json::object();
        const auto ctx = row.find("context");
        if (ctx != row.end() && !ctx->is_null()) {
            json context = json::array();
            if (ctx->is_string()) {
                context.push_back(*ctx);
            } else if (ctx->is_array()) {
                for (const auto& c : *ctx) {
                    if (c.is_string()) {
                        context.push_back(c);
                    }
                }
            }
            if (!context.empty()) {
                sample.metadata["context"] = std::move(context);
            }
        }
        out.data.samples.push_back(std::move(sample));
    }

    if (out.data.samples.size() < kMinSamples) {
        out.error = "dataset '" + dataset_name + "' has " + std::to_string(out.data.samples.size()) +
                    " valid samples (input/output strings); at least " + std::to_string(kMinSamples) +
                    " are required";
        return out;
    }
    out.success = true;
    return out;
}

DatasetLoadResult LoRADatasetLoader::load(const std::string& dataset_id, std::size_t limit) const {
    DatasetLoadResult out;
    const auto aql = buildDatasetQuery(dataset_id, limit);
    if (!aql) {
        out.error = "invalid dataset_id: must match ^[A-Za-z_][A-Za-z0-9_]{0,127}$";
        return out;
    }
    if (!executor_) {
        out.error = "dataset loading unavailable: no query executor configured";
        return out;
    }
    AqlRowsResult rows;
    try {
        rows = executor_(*aql);
    } catch (const std::exception& e) {
        out.error = std::string("dataset query failed: ") + e.what();
        return out;
    }
    if (!rows.ok) {
        out.error = "dataset query failed: " + rows.error;
        return out;
    }
    return fromRows(rows.rows, dataset_id, limit);
}

}  // namespace lora
}  // namespace llm
}  // namespace themis
