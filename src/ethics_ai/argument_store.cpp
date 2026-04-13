/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            argument_store.cpp                                 ║
  Version:         0.0.5                                              ║
  Last Modified:   2026-04-13 20:30:52                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   96.0/100                                       ║
    • Total Lines:     394                                            ║
    • Open Issues:     TODOs: 2, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 11ddb98b9f  2026-04-09  Add comprehensive documentation and security measures for... ║
    • 32f246a038  2026-04-08  feat(ethics_ai): enhance plugin configuration and overrid... ║
    • 9ab72c5089  2026-03-12  refactor: flatten plugin hierarchy to src/<name>/ and inc... ║
    • acdb250dbf  2026-03-12  feat: migrate plugins to src/include with CMake switches ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "argument_store.h"
#include "ethics_base_entity_adapter.h"
#include "storage/rocksdb_wrapper.h"
#include "query/query_engine.h"
#include <algorithm>
#include <set>

namespace themis {
namespace plugins {
namespace ethics {

Status ArgumentStore::initialize(
    std::shared_ptr<RocksDBWrapper> storage,
    std::shared_ptr<query::QueryEngine> query_engine) {
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (initialized_) {
        return Status::Error("ArgumentStore already initialized");
    }
    
    if (!storage) {
        // Standalone mode - use in-memory storage for testing
        standalone_mode_ = true;
    } else {
        storage_ = storage;
        query_engine_ = query_engine;
        standalone_mode_ = false;
    }
    
    initialized_ = true;
    return Status::OK();
}

Status ArgumentStore::storeArgument(const EthicalArgument& argument, bool store_vector) {
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
    auto blob = entity.serialize();
    
    // Use ThemisDB storage directly
    storage_->put(key, blob);
    
    // TODO: When vector support is integrated:
    // if (store_vector && !argument.content.empty()) {
    //     // Generate embedding and store in vector index
    //     // This will use ThemisDB's vector index manager
    // }
    
    return Status::OK();
}

std::variant<EthicalArgument, Status> ArgumentStore::getArgument(
    const std::string& argument_id) {
    
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
    auto blob = storage_->get(key);
    
    if (!blob) {
        return Status::Error("Argument not found: " + argument_id);
    }
    
    // Deserialize BaseEntity
    BaseEntity entity = BaseEntity::deserialize(argument_id, *blob);
    
    // Convert back to EthicalArgument
    return EthicsBaseEntityAdapter::fromBaseEntity(entity);
}

std::variant<std::vector<EthicalArgument>, Status> ArgumentStore::getArgumentsByPhilosophy(
    const std::string& philosophy_school,
    const std::vector<ArgumentType>& argument_types,
    size_t limit) {
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!initialized_) {
        return Status::Error("ArgumentStore not initialized");
    }
    
    if (standalone_mode_) {
        // Standalone mode - scan in-memory storage
        std::vector<EthicalArgument> results;
        
        for (const auto& kv : arguments_) {
            const auto& arg = kv.second;
            
            if (arg.philosophy_school != philosophy_school) {
                continue;
            }
            
            if (!argument_types.empty()) {
                bool type_match = false;
                for (const auto& type : argument_types) {
                    if (arg.argument_type == type) {
                        type_match = true;
                        break;
                    }
                }
                if (!type_match) continue;
            }
            
            results.push_back(arg);
            
            if (results.size() >= limit) {
                break;
            }
        }
        
        return results;
    }
    
    // TODO: Use AQL query when query_engine_ is available:
    // - FOR arg IN ethics_arguments FILTER arg.philosophy_school == @school ...
    // For now, scan the key prefix
    
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
            if (!type_str) return true;
            
            ArgumentType type = stringToArgumentType(*type_str);
            bool type_match = false;
            for (const auto& filter_type : argument_types) {
                if (type == filter_type) {
                    type_match = true;
                    break;
                }
            }
            if (!type_match) return true;
        }
        
        // Convert and add to results
        results.push_back(EthicsBaseEntityAdapter::fromBaseEntity(entity));
        
        return true; // Continue
    });
    
    return results;
}

Status ArgumentStore::storeDecision(const EthicalDecision& decision) {
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
    auto blob = entity.serialize();
    storage_->put(key, blob);
    
    return Status::OK();
}

std::variant<EthicalDecision, Status> ArgumentStore::getDecision(
    const std::string& decision_id) {
    
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
    auto blob = storage_->get(key);
    
    if (!blob) {
        return Status::Error("Decision not found: " + decision_id);
    }
    
    // Deserialize BaseEntity
    BaseEntity entity = BaseEntity::deserialize(decision_id, *blob);
    
    // Convert back to EthicalDecision
    return EthicsBaseEntityAdapter::fromBaseEntity(entity, true);
}

Status ArgumentStore::storePhilosophyProfile(const PhilosophyProfile& profile) {
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
    auto blob = entity.serialize();
    storage_->put(key, blob);
    
    return Status::OK();
}

std::variant<PhilosophyProfile, Status> ArgumentStore::getPhilosophyProfile(
    const std::string& school) {
    
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
    auto blob = storage_->get(key);
    
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

Status ArgumentStore::storeChain(const ArgumentChain& chain) {
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

std::variant<ArgumentChain, Status> ArgumentStore::getChain(const std::string& chain_id) {
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

} // namespace ethics
} // namespace plugins
} // namespace themis
