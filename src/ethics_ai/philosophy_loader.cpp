/**
 * @file philosophy_loader.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 81/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=13, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "philosophy_loader.h"

#include <filesystem>
#include <fstream>
#include <functional>
#include <sstream>

#ifdef HAVE_YAML_CPP
#include <yaml-cpp/yaml.h>
#endif

namespace themis {
namespace plugins {
namespace ethics {

std::variant<size_t, Status> PhilosophyLoader::loadFromDirectory(const std::string &directory) {
    namespace fs = std::filesystem;

    if (!fs::exists(directory)) {
        return Status::Error("Directory does not exist: " + directory);
    }

    if (!fs::is_directory(directory)) {
        return Status::Error("Path is not a directory: " + directory);
    }

    size_t count = 0;
    // COMPLEXITY FIX: Store iterator in variable to avoid temporary reference invalidation
    auto dir_iter = fs::directory_iterator(directory);
    for (const auto &entry : dir_iter) {
        if (entry.is_regular_file()) {
            auto path = entry.path();
            auto ext  = path.extension().string();

            if (ext == ".yaml" || ext == ".yml") {
                auto status = loadFromFile(path.string());
                if (status.isOK()) {
                    count++;
                }
            }
        }
    }

    return count;
}

Status PhilosophyLoader::loadFromFile(const std::string &filepath) {
#ifdef HAVE_YAML_CPP
    try {
        YAML::Node config = YAML::LoadFile(filepath);

        PhilosophyProfile profile;
        profile.school_id = config["school_id"].as<std::string>("");
        profile.name      = config["name"].as<std::string>("");

        if (profile.school_id.empty()) {
            // Use filename without extension as school_id
            namespace fs      = std::filesystem;
            profile.school_id = fs::path(filepath).stem().string();
        }

        // ----------------------------------------------------------------
        // Helpers to extract a single string from a potentially complex node
        // ----------------------------------------------------------------
        auto extractText = [](const YAML::Node &node) -> std::string {
            if (!node) {
                return "";
            }
            if (node.IsScalar()) {
                return node.as<std::string>("");
            }
            if (node.IsMap()) {
                // Rich thesis object: prefer "description", then "name", then "text"
                for (const char *key : {"description", "name", "text"}) {
                    if (node[key] && node[key].IsScalar()) {
                        return node[key].as<std::string>("");
                    }
                }
                // Fallback: join all scalar leaf values
                std::ostringstream acc = {};
                bool first = true;
                for (const auto &kv : node) {
                    if (kv.second.IsScalar()) {
                        if (!first) {
                            acc << "; ";
                        }
                        acc << kv.second.as<std::string>("");
                        first = false;
                    }
                }
                return acc.str();
            }
            if (node.IsSequence() && node.size() > 0 && node[0].IsScalar()) {
                return node[0].as<std::string>("");
            }
            return "";
        };

        auto extractPointText = [](const YAML::Node &node) -> std::string {
            if (!node) {
                return "";
            }
            if (node.IsScalar()) {
                return node.as<std::string>("");
            }
            if (node.IsMap()) {
                // Strength/weakness entries use "point" as the headline
                for (const char *key : {"point", "description", "name"}) {
                    if (node[key] && node[key].IsScalar()) {
                        return node[key].as<std::string>("");
                    }
                }
            }
            return "";
        };

        // Join a potentially nested sequence/scalar YAML value to a string
        std::function<std::string(const YAML::Node &)> joinNode;
        joinNode = [&joinNode](const YAML::Node &node) -> std::string {
            if (!node) {
                return "";
            }
            if (node.IsScalar()) {
                return node.as<std::string>("");
            }
            if (node.IsSequence()) {
                std::ostringstream acc = {};
                bool first = true;
                for (const auto &item : node) {
                    std::string s = joinNode(item);
                    if (!s.empty()) {
                        if (!first) {
                            acc << "; ";
                        }
                        acc << s;
                        first = false;
                    }
                }
                return acc.str();
            }
            if (node.IsMap()) {
                // Try common key names first, then join all scalar leaves
                for (const char *key : {"description", "name", "text", "procedure"}) {
                    if (node[key] && node[key].IsScalar()) {
                        return node[key].as<std::string>("");
                    }
                }
                std::ostringstream acc = {};
                bool first = true;
                for (const auto &kv : node) {
                    std::string s = joinNode(kv.second);
                    if (!s.empty()) {
                        if (!first) {
                            acc << "; ";
                        }
                        acc << s;
                        first = false;
                    }
                }
                return acc.str();
            }
            return "";
        };

        // Load main theses (may be plain strings or complex objects)
        if (config["main_theses"]) {
            for (const auto &thesis : config["main_theses"]) {
                std::string text = extractText(thesis);
                if (!text.empty()) {
                    profile.main_theses.push_back(text);
                }
                // Also parse as typed thesis when the entry is a YAML map
                // with a 'thesis_id' key so that token_budget /
                // activation_rounds / round_role_weights are preserved.
                if (thesis.IsMap() && thesis["thesis_id"]) {
                    PhilosophyThesis pt;
                    pt.thesis_id = thesis["thesis_id"].as<std::string>("");
                    if (thesis["name"]) {
                        pt.name = thesis["name"].as<std::string>("");
                    }
                    if (thesis["description"]) {
                        pt.description = thesis["description"].as<std::string>("");
                    }
                    if (thesis["token_budget"] && !thesis["token_budget"].IsNull()) {
                        pt.token_budget = thesis["token_budget"].as<int>(-1);
                    }
                    if (thesis["activation_rounds"] && thesis["activation_rounds"].IsSequence()) {
                        for (const auto &r : thesis["activation_rounds"]) {
                            pt.activation_rounds.push_back(r.as<int>());
                        }
                    }
                    if (thesis["round_role_weights"] && thesis["round_role_weights"].IsMap()) {
                        for (const auto &kv : thesis["round_role_weights"]) {
                            pt.round_role_weights[kv.first.as<std::string>()] = kv.second.as<float>(0.f);
                        }
                    }
                    if (!pt.thesis_id.empty()) {
                        profile.typed_theses.push_back(std::move(pt));
                    }
                }
            }
        }

        // Load secondary theses (same handling as main_theses)
        if (config["secondary_theses"]) {
            for (const auto &thesis : config["secondary_theses"]) {
                std::string text = extractText(thesis);
                if (!text.empty()) {
                    profile.secondary_theses.push_back(text);
                }
                if (thesis.IsMap() && thesis["thesis_id"]) {
                    PhilosophyThesis pt;
                    pt.thesis_id = thesis["thesis_id"].as<std::string>("");
                    if (thesis["name"]) {
                        pt.name = thesis["name"].as<std::string>("");
                    }
                    if (thesis["description"]) {
                        pt.description = thesis["description"].as<std::string>("");
                    }
                    if (thesis["token_budget"] && !thesis["token_budget"].IsNull()) {
                        pt.token_budget = thesis["token_budget"].as<int>(-1);
                    }
                    if (thesis["activation_rounds"] && thesis["activation_rounds"].IsSequence()) {
                        for (const auto &r : thesis["activation_rounds"]) {
                            pt.activation_rounds.push_back(r.as<int>());
                        }
                    }
                    if (thesis["round_role_weights"] && thesis["round_role_weights"].IsMap()) {
                        for (const auto &kv : thesis["round_role_weights"]) {
                            pt.round_role_weights[kv.first.as<std::string>()] = kv.second.as<float>(0.f);
                        }
                    }
                    if (!pt.thesis_id.empty()) {
                        profile.typed_theses.push_back(std::move(pt));
                    }
                }
            }
        }

        // Load decision framework (may be a flat map or nested structure)
        if (config["decision_framework"]) {
            const auto &df = config["decision_framework"];
            if (df.IsMap()) {
                for (const auto &kv : df) {
                    std::string key = kv.first.as<std::string>("");
                    std::string val = joinNode(kv.second);
                    if (!key.empty()) {
                        profile.decision_framework[key] = val;
                    }
                }
            }
        }

        // Load strengths (may be plain strings or {point, elaboration} objects)
        if (config["strengths"]) {
            for (const auto &item : config["strengths"]) {
                std::string text = extractPointText(item);
                if (!text.empty()) {
                    profile.strengths.push_back(text);
                }
            }
        }

        // Load weaknesses (same handling as strengths)
        if (config["weaknesses"]) {
            for (const auto &item : config["weaknesses"]) {
                std::string text = extractPointText(item);
                if (!text.empty()) {
                    profile.weaknesses.push_back(text);
                }
            }
        }

        // Load internal debate
        if (config["internal_debate"]) {
            for (const auto &kv : config["internal_debate"]) {
                profile.internal_debate[kv.first.as<std::string>()] = joinNode(kv.second);
            }
        }

        // Load philosophical positioning
        if (config["philosophical_positioning"]) {
            for (const auto &kv : config["philosophical_positioning"]) {
                profile.philosophical_positioning[kv.first.as<std::string>()] = joinNode(kv.second);
            }
        }

        profiles_[profile.school_id] = profile;
        return Status::OK();

    } catch (const YAML::Exception &e) {
        return Status::Error("YAML parsing error: " + std::string(e.what()));
    } catch (const std::exception &e) {
        return Status::Error("Error loading file: " + std::string(e.what()));
    }
#else
    return Status::Error("YAML support not enabled (yaml-cpp not found)");
#endif
}

std::variant<PhilosophyProfile, Status> PhilosophyLoader::getProfile(const std::string &school_id) const {
    auto it = profiles_.find(school_id);
    if (it == profiles_.end()) {
        return Status::Error("Philosophy profile not found: " + school_id);
    }

    return it->second;
}

bool PhilosophyLoader::hasProfile(const std::string &school_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return profiles_.find(school_id) != profiles_.end();
}

std::vector<std::string> PhilosophyLoader::getSchoolIds() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> ids = {};

    ids.reserve(profiles_.size());

    for (const auto &kv : profiles_) {
        ids.push_back(kv.first);
    }

    return ids;
}

void PhilosophyLoader::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    profiles_.clear();
}

void PhilosophyLoader::addProfile(const PhilosophyProfile &profile) {
    std::lock_guard<std::mutex> lock(mutex_);
    profiles_[profile.school_id] = profile;
}

std::map<std::string, PhilosophyProfile> PhilosophyLoader::getAllProfiles() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return profiles_;
}

std::variant<size_t, Status> PhilosophyLoader::reloadProfiles(const std::string &directory) {
    // Build a fresh map using a temporary loader (avoids holding lock during I/O).
    PhilosophyLoader tmp;
    auto result = tmp.loadFromDirectory(directory);
    if (std::holds_alternative<Status>(result)) {
        return result; // propagate error
    }

    // Atomic swap under the lock.
    std::lock_guard<std::mutex> lock(mutex_);
    profiles_ = tmp.profiles_;
    return profiles_.size();
}

} // namespace ethics
} // namespace plugins
} // namespace themis
