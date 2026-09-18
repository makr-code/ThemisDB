/**
 * @file argument_store.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "ethics_ai/ethics_ai_types.h"
#include "storage/base_entity.h"
#include <functional>
#include <memory>
#include <mutex>
#include <optional>

// Forward declarations for ThemisDB components
namespace themis {
class RocksDBWrapper;
class IVectorWriter;
namespace query {
class QueryEngine;
}
}

namespace themis {
namespace plugins {
namespace ethics {

class ArgumentStore {
public:
    ArgumentStore() = default;
    ~ArgumentStore() = default;

    using VectorStoreFn = std::function<void(const std::string& id,
                                             const std::string& content)>;

    /**
     * @brief Set Vector Store Function.
     * @param[in] fn Input parameter.
     */
    void setVectorStoreFunction(VectorStoreFn fn);
    
    Status initialize(
        std::shared_ptr<RocksDBWrapper> storage,
        std::shared_ptr<query::QueryEngine> query_engine = nullptr
    );
    
    Status storeArgument(const EthicalArgument& argument, bool store_vector = true);
    
    std::variant<EthicalArgument, Status> getArgument(const std::string& argument_id);
    
    std::variant<std::vector<EthicalArgument>, Status> getArgumentsByPhilosophy(
        const std::string& philosophy_school,
        const std::vector<ArgumentType>& argument_types,
        size_t limit
    );
    
    /**
     * @brief Store Decision.
     * @param[in] decision Input parameter.
     * @return Return value.
     */
    Status storeDecision(const EthicalDecision& decision);
    
    std::variant<EthicalDecision, Status> getDecision(const std::string& decision_id);
    
    /**
     * @brief Store Philosophy Profile.
     * @param[in] profile Input parameter.
     * @return Return value.
     */
    Status storePhilosophyProfile(const PhilosophyProfile& profile);
    
    std::variant<PhilosophyProfile, Status> getPhilosophyProfile(const std::string& school);

    /**
     * @brief Store Chain.
     * @param[in] chain Input parameter.
     * @return Return value.
     */
    Status storeChain(const ArgumentChain& chain);

    std::variant<ArgumentChain, Status> getChain(const std::string& chain_id);

    /**
     * @brief Store Debate Round.
     * @param[in] round Input parameter.
     * @return Return value.
     */
    Status storeDebateRound(const DebateRound& round);

    std::variant<std::vector<DebateRound>, Status> getDebateTranscript(
        const std::string& debate_id
    );

    /**
     * @brief Shutdown.
     */
    void shutdown();

    void setVectorWriter(
        std::shared_ptr<IVectorWriter> writer,
        std::function<std::vector<float>(const std::string&)> embedding_fn = nullptr
    );
    
private:
    std::mutex mutex_;
    bool initialized_ = false;
    
    // Direct ThemisDB storage - no wrappers
    std::shared_ptr<RocksDBWrapper> storage_;
    std::shared_ptr<query::QueryEngine> query_engine_;
    
    // Optional injected vector embedding function
    std::optional<VectorStoreFn> vector_store_fn_;

    // Fallback in-memory storage for standalone mode (testing)
    bool standalone_mode_ = false;
    std::map<std::string, EthicalArgument> arguments_;
    std::map<std::string, EthicalDecision> decisions_;
    std::map<std::string, PhilosophyProfile> profiles_;
    std::map<std::string, ArgumentChain> chains_; ///< In-memory chain cache
    std::map<std::string, std::vector<DebateRound>> debate_rounds_;

    // Vector index integration (Stub #33 resolved)
    std::shared_ptr<IVectorWriter> vector_writer_;
    std::function<std::vector<float>(const std::string&)> embedding_fn_;
};

} // namespace ethics
} // namespace plugins
} // namespace themis
