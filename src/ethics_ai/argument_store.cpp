/**
 * @file argument_store.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=5, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "argument_store.h"

#include <algorithm>
#include <set>
#include <spdlog/spdlog.h>

#include "ethics_base_entity_adapter.h"
#include "query/query_engine.h"
#include "storage/rocksdb_wrapper.h"

namespace themis {
namespace plugins {
namespace ethics {

// Bring the canonical query types into scope.
using themis::query::ConjunctiveQuery;
using themis::query::PredicateEq;

void ArgumentStore::setVectorStoreFunction(VectorStoreFn fn) {
    std::lock_guard<std::mutex> lock(mutex_);
    vector_store_fn_ = std::move(fn);
}

Status ArgumentStore::initialize(std::shared_ptr<RocksDBWrapper> storage, std::shared_ptr<query::QueryEngine> query_engine) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (initialized_) {
        return Status::Error("ArgumentStore already initialized");
    }

    if (!storage) {
        // Standalone mode - use in-memory storage for testing
        standalone_mode_ = true;
    } else {
        storage_         = storage;
        query_engine_    = query_engine;
        standalone_mode_ = false;
    }

    initialized_ = true;
    return Status::OK();
}

Status ArgumentStore::storeArgument(const EthicalArgument &argument, [[maybe_unused]] bool store_vector) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!initialized_) {
        return Status::Error("ArgumentStore not initialized");
    }

    if (argument.id.empty()) {
        return Status::Error("Argument ID cannot be empty");
    }

    if (standalone_mode_) {
        // Standalone mode - use in-memory storage
        arguments_[argument.id] = argument;
        return Status::OK();
    }

    // Convert to BaseEntity
    BaseEntity entity = EthicsBaseEntityAdapter::toBaseEntity(argument);

    // Store in RocksDB with proper key format
    std::string key = EthicsBaseEntityAdapter::makeArgumentKey(argument.id);
    auto blob       = entity.serialize();

    // Use ThemisDB storage directly
    storage_->put(key, blob);

    // Trigger vector embedding storage if a writer is injected and requested.
    if (store_vector && !argument.content.empty() && vector_store_fn_) {
        try {
            (*vector_store_fn_)(argument.id, argument.content);
        } catch (const std::exception &ex) {
            spdlog::warn("ArgumentStore::storeArgument — vector store failed for id='{}': {}", argument.id, ex.what());
        }
    }

    return Status::OK();
}

std::variant<EthicalArgument, Status> ArgumentStore::getArgument(const std::string &argument_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!initialized_) {
        return Status::Error("ArgumentStore not initialized");
    }

    if (standalone_mode_) {
        // Standalone mode - use in-memory storage
        auto it = arguments_.find(argument_id);
        if (it == arguments_.end()) {
            return Status::Error("Argument not found: " + argument_id);
        }
        return it->second;
    }

    // Load from RocksDB as BaseEntity
    std::string key = EthicsBaseEntityAdapter::makeArgumentKey(argument_id);
    auto blob       = storage_->get(key);

    if (!blob) {
        return Status::Error("Argument not found: " + argument_id);
    }

    // Deserialize BaseEntity
    BaseEntity entity = BaseEntity::deserialize(argument_id, *blob);

    // Convert back to EthicalArgument
    return EthicsBaseEntityAdapter::fromBaseEntity(entity);
}

std::variant<std::vector<EthicalArgument>, Status>
ArgumentStore::getArgumentsByPhilosophy(const std::string &philosophy_school,
                                        const std::vector<ArgumentType> &argument_types, size_t limit) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!initialized_) {
        return Status::Error("ArgumentStore not initialized");
    }

    if (standalone_mode_) {
        // Standalone mode - scan in-memory storage
        std::vector<EthicalArgument> results;

        for (const auto &kv : arguments_) {
            const auto &arg = kv.second;

            if (arg.philosophy_school != philosophy_school) {
                continue;
            }

            if (!argument_types.empty()) {
                bool type_match = false;
                for (const auto &type : argument_types) {
                    if (arg.argument_type == type) {
                        type_match = true;
                        break;
                    }
                }
                if (!type_match) {
                    continue;
                }
            }

            results.push_back(arg);

            if (results.size() >= limit) {
                break;
            }
        }

        return results;
    }

    // If the query engine is available, use ConjunctiveQuery for index-backed
    // retrieval instead of a full prefix scan (avoids deserializing every entity).
    if (query_engine_) {
        ConjunctiveQuery q;
        q.table = "ethics_arguments";
        q.predicates.push_back({"philosophy_school", philosophy_school});

        auto result = query_engine_->executeAndEntities(q);
        if (result.has_value()) {
            std::vector<EthicalArgument> out;
            for (const auto &entity : result.value()) {
                if (!argument_types.empty()) {
                    auto type_str = entity.getFieldAsString("argument_type");
                    if (!type_str) {
                        continue;
                    }
                    ArgumentType t = stringToArgumentType(*type_str);
                    bool match     = std::any_of(argument_types.begin(), argument_types.end(),
                                                 [t](ArgumentType ft) { return t == ft; });
                    if (!match) {
                        continue;
                    }
                }
                out.push_back(EthicsBaseEntityAdapter::fromBaseEntity(entity));
                if (out.size() >= limit) {
                    break;
                }
            }
            return out;
        }
        // Fall through to prefix scan on query engine error.
        spdlog::warn("ArgumentStore::getArgumentsByPhilosophy — query engine error ({}); "
                     "falling back to prefix scan",
                     result.error().message());
    }

    std::vector<EthicalArgument> results;
    std::string prefix = "entity:ethics_arguments:";

    // Scan RocksDB with prefix
    storage_->scanPrefix(prefix, [&](std::string_view key, std::string_view value) -> bool {
        if (results.size() >= limit) {
            return false; // Stop iteration
        }

        // Extract PK from key
        std::string pk = std::string(key).substr(prefix.length());

        // Deserialize BaseEntity
        std::vector<uint8_t> blob(value.begin(), value.end());
        BaseEntity entity = BaseEntity::deserialize(pk, blob);

        // Check philosophy school filter
        auto school = entity.getFieldAsString("philosophy_school");
        if (!school || *school != philosophy_school) {
            return true; // Continue
        }

        // Check argument type filter
        if (!argument_types.empty()) {
            auto type_str = entity.getFieldAsString("argument_type");
            if (!type_str) {
                return true;
            }

            ArgumentType type = stringToArgumentType(*type_str);
            bool type_match   = std::any_of(argument_types.begin(), argument_types.end(),
                                            [type](ArgumentType ft) { return type == ft; });
            if (!type_match) {
                return true;
            }
        }

        // Convert and add to results
        results.push_back(EthicsBaseEntityAdapter::fromBaseEntity(entity));

        return true; // Continue
    });

    return results;
}

Status ArgumentStore::storeDecision(const EthicalDecision &decision) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!initialized_) {
        return Status::Error("ArgumentStore not initialized");
    }

    if (decision.decision_id.empty()) {
        return Status::Error("Decision ID cannot be empty");
    }

    if (standalone_mode_) {
        decisions_[decision.decision_id] = decision;
        return Status::OK();
    }

    // Convert to BaseEntity
    BaseEntity entity = EthicsBaseEntityAdapter::toBaseEntity(decision);

    // Store in RocksDB
    std::string key = EthicsBaseEntityAdapter::makeDecisionKey(decision.decision_id);
    auto blob       = entity.serialize();
    storage_->put(key, blob);

    return Status::OK();
}

std::variant<EthicalDecision, Status> ArgumentStore::getDecision(const std::string &decision_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!initialized_) {
        return Status::Error("ArgumentStore not initialized");
    }

    if (standalone_mode_) {
        auto it = decisions_.find(decision_id);
        if (it == decisions_.end()) {
            return Status::Error("Decision not found: " + decision_id);
        }
        return it->second;
    }

    // Load from RocksDB
    std::string key = EthicsBaseEntityAdapter::makeDecisionKey(decision_id);
    auto blob       = storage_->get(key);

    if (!blob) {
        return Status::Error("Decision not found: " + decision_id);
    }

    // Deserialize BaseEntity
    BaseEntity entity = BaseEntity::deserialize(decision_id, *blob);

    // Convert back to EthicalDecision
    return EthicsBaseEntityAdapter::fromBaseEntity(entity, true);
}

Status ArgumentStore::storePhilosophyProfile(const PhilosophyProfile &profile) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!initialized_) {
        return Status::Error("ArgumentStore not initialized");
    }

    if (profile.school_id.empty()) {
        return Status::Error("Profile school cannot be empty");
    }

    if (standalone_mode_) {
        profiles_[profile.school_id] = profile;
        return Status::OK();
    }

    // Convert to BaseEntity
    BaseEntity entity = EthicsBaseEntityAdapter::toBaseEntity(profile);

    // Store in RocksDB
    std::string key = EthicsBaseEntityAdapter::makeProfileKey(profile.school_id);
    auto blob       = entity.serialize();
    storage_->put(key, blob);

    return Status::OK();
}

std::variant<PhilosophyProfile, Status> ArgumentStore::getPhilosophyProfile(const std::string &school) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!initialized_) {
        return Status::Error("ArgumentStore not initialized");
    }

    if (standalone_mode_) {
        auto it = profiles_.find(school);
        if (it == profiles_.end()) {
            return Status::Error("Profile not found: " + school);
        }
        return it->second;
    }

    // Load from RocksDB
    std::string key = EthicsBaseEntityAdapter::makeProfileKey(school);
    auto blob       = storage_->get(key);

    if (!blob) {
        return Status::Error("Profile not found: " + school);
    }

    // Deserialize BaseEntity
    BaseEntity entity = BaseEntity::deserialize(school, *blob);

    // Convert back to PhilosophyProfile
    return EthicsBaseEntityAdapter::fromBaseEntityToProfile(entity);
}

void ArgumentStore::shutdown() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!initialized_) {
        return;
    }

    // Clear references (storage is managed externally)
    storage_.reset();
    query_engine_.reset();

    // Clear in-memory data
    arguments_.clear();
    decisions_.clear();
    profiles_.clear();
    chains_.clear();

    initialized_ = false;
}

Status ArgumentStore::storeChain(const ArgumentChain &chain) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!initialized_) {
        return Status::Error("ArgumentStore not initialized");
    }

    if (chain.id.empty()) {
        return Status::Error("Chain ID cannot be empty");
    }

    chains_[chain.id] = chain;
    return Status::OK();
}

std::variant<ArgumentChain, Status> ArgumentStore::getChain(const std::string &chain_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!initialized_) {
        return Status::Error("ArgumentStore not initialized");
    }

    auto it = chains_.find(chain_id);
    if (it == chains_.end()) {
        return Status::Error("Chain not found: " + chain_id);
    }

    return it->second;
}

// ============================================================================
// v0.2.0 — Debate Transcript Storage
// ============================================================================

Status ArgumentStore::storeDebateRound(const DebateRound &round) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto &rounds = debate_rounds_[round.debate_id];
    // Replace if a round with the same number already exists.
    for (auto &existing : rounds) {
        if (existing.round_number == round.round_number) {
            existing = round;
            return Status::OK();
        }
    }
    rounds.push_back(round);
    // Keep rounds sorted by round_number.
    std::sort(rounds.begin(), rounds.end(),
              [](const DebateRound &a, const DebateRound &b) { return a.round_number < b.round_number; });
    return Status::OK();
}

std::variant<std::vector<DebateRound>, Status> ArgumentStore::getDebateTranscript(const std::string &debate_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = debate_rounds_.find(debate_id);
    if (it == debate_rounds_.end()) {
        return std::vector<DebateRound>{}; // no rounds yet — not an error
    }
    return it->second;
}

} // namespace ethics
} // namespace plugins
} // namespace themis
