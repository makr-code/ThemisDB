/**
 * @file argument_store.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=7; TODO=1, Stub=4, Unimpl=0, Mock=1, Sim=1, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/**
 * @brief Argument Storage using ThemisDB BaseEntity and AQL
 * 
 * Stores ethical arguments directly as BaseEntity instances.
 * No duplicate storage structures - uses ThemisDB's unified storage.
 * 
 * Storage Collections:
 * - ethics_arguments: Argument entities
 * - ethics_decisions: Decision entities
 * - ethics_debates: Debate session entities
 * - ethics_profiles: Philosophy profile entities
 * 
 * Keys follow ThemisDB pattern:
 * - entity:ethics_arguments:{id}
 * - entity:ethics_decisions:{id}
 * etc.
 */
class ArgumentStore {
public:
    ArgumentStore() = default;
    ~ArgumentStore() = default;

    /**
     * @brief Callable type for vector embedding storage.
     *
     * Called by storeArgument() when store_vector=true and argument.content
     * is non-empty.  The function is responsible for:
     *  1. Generating an embedding for @p content (e.g. via an IEmbeddingBackend)
     *  2. Writing the embedding + @p id into a vector index / IVectorWriter
     *
     * @param id       Argument ID (used as the vector document key)
     * @param content  Text content to embed
     */
    using VectorStoreFn = std::function<void(const std::string& id,
                                             const std::string& content)>;

    /**
     * @brief Inject a vector-embedding storage function.
     *
     * When set, storeArgument() will call @p fn for every argument whose
     * store_vector flag is true and whose content is non-empty.  Replaces the
     * former STUB/SIMULATION NOTE for the vector path in storeArgument().
     *
     * @param fn  Callable matching VectorStoreFn; must be thread-safe.
     */
    void setVectorStoreFunction(VectorStoreFn fn);
    
    /**
     * @brief Initialize the argument store with ThemisDB storage
     * @param storage RocksDB storage backend
     * @param query_engine Query engine for AQL execution (optional for standalone mode)
     * @return Status indicating success/failure
     */
    Status initialize(
        std::shared_ptr<RocksDBWrapper> storage,
        std::shared_ptr<query::QueryEngine> query_engine = nullptr
    );
    
    /**
     * @brief Store an ethical argument as BaseEntity
     * @param argument The argument to store
     * @param store_vector Whether to generate and store vector embedding
     * @return Status indicating success/failure
     */
    Status storeArgument(const EthicalArgument& argument, bool store_vector = true);
    
    /**
     * @brief Retrieve argument by ID (uses BaseEntity)
     * @param argument_id Argument identifier
     * @return Argument or error
     */
    std::variant<EthicalArgument, Status> getArgument(const std::string& argument_id);
    
    /**
     * @brief Get arguments by philosophy school (uses AQL if available, else scan)
     * @param philosophy_school School identifier
     * @param argument_types Filter by types (empty = all)
     * @param limit Maximum results
     * @return List of arguments or error
     */
    std::variant<std::vector<EthicalArgument>, Status> getArgumentsByPhilosophy(
        const std::string& philosophy_school,
        const std::vector<ArgumentType>& argument_types,
        size_t limit
    );
    
    /**
     * @brief Store a decision as BaseEntity
     * @param decision The decision to store
     * @return Status indicating success/failure
     */
    Status storeDecision(const EthicalDecision& decision);
    
    /**
     * @brief Retrieve decision by ID
     * @param decision_id Decision identifier
     * @return Decision or error
     */
    std::variant<EthicalDecision, Status> getDecision(const std::string& decision_id);
    
    /**
     * @brief Store a philosophy profile as BaseEntity
     * @param profile The profile to store
     * @return Status indicating success/failure
     */
    Status storePhilosophyProfile(const PhilosophyProfile& profile);
    
    /**
     * @brief Retrieve philosophy profile by school
     * @param school School identifier
     * @return Profile or error
     */
    std::variant<PhilosophyProfile, Status> getPhilosophyProfile(const std::string& school);

    /**
     * @brief Store an argument chain
     * @param chain The argument chain to store
     * @return Status indicating success/failure
     */
    Status storeChain(const ArgumentChain& chain);

    /**
     * @brief Retrieve an argument chain by ID
     * @param chain_id Chain identifier
     * @return Chain or error
     */
    std::variant<ArgumentChain, Status> getChain(const std::string& chain_id);

    /**
     * @brief Store a single debate round (all arguments produced in one round).
     * @param round The round to store
     * @return Status indicating success/failure
     */
    Status storeDebateRound(const DebateRound& round);

    /**
     * @brief Retrieve the full debate transcript (all rounds) for a given debate.
     *
     * Returns all `DebateRound` objects that were stored for @p debate_id,
     * ordered by `round_number` ascending.  If no rounds are stored for the
     * debate, returns an empty vector (not an error).
     *
     * @param debate_id  Debate identifier (from `DebateInitialization::debate_id`).
     * @return Ordered list of rounds, or `Status::Error` on storage failure.
     */
    std::variant<std::vector<DebateRound>, Status> getDebateTranscript(
        const std::string& debate_id
    );

    /**
     * @brief Shutdown the store
     */
    void shutdown();

    /**
     * @brief Inject a vector-index writer for semantic similarity search.
     *
     * When set, `storeArgument()` generates a deterministic embedding from the
     * argument content and persists it via the writer.  Callers can replace
     * the default hash-based embedding by providing @p embedding_fn.
     *
     * @param writer      Vector index writer (must outlive the store, or be kept
     *                    alive by the shared_ptr).
     * @param embedding_fn Optional embedding function: content → float vector.
     *                    If nullptr, a hash-based 128-dim placeholder is used
     *                    (not semantically meaningful for similarity search).
     */
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
    /// Key = debate_id; value = rounds ordered by round_number
    std::map<std::string, std::vector<DebateRound>> debate_rounds_;

    // Vector index integration (Stub #33 resolved)
    std::shared_ptr<IVectorWriter> vector_writer_;
    std::function<std::vector<float>(const std::string&)> embedding_fn_;
};

} // namespace ethics
} // namespace plugins
} // namespace themis
