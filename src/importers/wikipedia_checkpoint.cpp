/**
 * @file wikipedia_checkpoint.cpp
 * @brief Wikipedia import checkpoint implementation.
 *
 * Implements serialisation, deserialisation, and durability guarantees
 * for the Wikipedia pipeline checkpoint state.
 */

#include "importers/wikipedia_checkpoint.hpp"

#include <filesystem>
#include <fstream>

namespace themis::importers {

WikipediaCheckpointStore::WikipediaCheckpointStore(std::string path)
    : path_(std::move(path)) {}

void WikipediaCheckpointStore::setPath(std::string path) {
    path_ = std::move(path);
}

const std::string& WikipediaCheckpointStore::path() const {
    return path_;
}

bool WikipediaCheckpointStore::hasPath() const {
    return !path_.empty();
}

bool WikipediaCheckpointStore::exists() const {
    return hasPath() && std::filesystem::exists(path_);
}

WikipediaCheckpointState WikipediaCheckpointStore::load() const {
    if (!exists()) {
        return {};
    }

    std::ifstream input(path_);
    if (!input.is_open()) {
        return {};
    }

    json payload;
    input >> payload;
    return WikipediaCheckpointState::fromJson(payload);
}

bool WikipediaCheckpointStore::save(const WikipediaCheckpointState& state) const {
    if (!hasPath()) {
        return false;
    }

    const auto parent = std::filesystem::path(path_).parent_path();
    if (!parent.empty()) {
        std::filesystem::create_directories(parent);
    }

    std::ofstream output(path_);
    if (!output.is_open()) {
        return false;
    }

    output << state.toJson().dump(2) << '\n';
    return true;
}

bool WikipediaCheckpointStore::clear() const {
    if (!exists()) {
        return false;
    }
    return std::filesystem::remove(path_);
}

} // namespace themis::importers
