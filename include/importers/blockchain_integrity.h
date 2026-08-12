/**
 * @file blockchain_integrity.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace themis {
namespace importers {

using json = nlohmann::json;

/**
 * @brief Merkle-tree based data-integrity verification for mission-critical imports.
 *
 * Computes a SHA-256 Merkle root over a batch of imported records.  The root
 * can optionally be anchored on an external blockchain for tamper-evidence.
 *
 * Use cases: Healthcare (HIPAA), Financial (SOX), Legal (e-discovery).
 */
class BlockchainIntegrityVerifier {
public:
    struct IntegrityProof {
        std::string merkle_root;             ///< Hex-encoded SHA-256 root
        std::string blockchain_tx_hash;      ///< On-chain transaction hash (if anchored)
        std::string timestamp_rfc3339;
        std::string smart_contract_address;  ///< EVM contract address (if anchored)
    };

    // ------------------------------------------------------------------
    // Merkle tree builder
    // ------------------------------------------------------------------
    class MerkleTreeBuilder {
    public:
        /**
         * @brief Compute a Merkle root over a list of JSON records.
         *
         * Each record is serialised deterministically (sorted keys), hashed
         * with SHA-256, and combined in a binary Merkle tree.
         *
         * @param records              Input records.
         * @param leaf_hash_algorithm  Only "SHA256" is supported currently.
         * @return Hex-encoded Merkle root.
         */
        std::string buildMerkleTree(
            const std::vector<json>& records,
            const std::string& leaf_hash_algorithm = "SHA256"
        );

        /**
         * @brief Verify that a single record belongs to the tree identified
         *        by merkle_root.
         *
         * @param record       The record to verify.
         * @param merkle_root  Hex-encoded root previously computed by buildMerkleTree.
         * @param sibling_hashes  Proof path (hex-encoded, leaf → root order).
         * @return true if the proof validates.
         */
        bool verifyRecordInTree(
            const json& record,
            const std::string& merkle_root,
            const std::vector<std::string>& sibling_hashes = {}
        );

    private:
        static std::string sha256Hex(const std::string& input);
        static std::string combineHashes(const std::string& left,
                                         const std::string& right);
    };

    // ------------------------------------------------------------------
    // Blockchain anchor (optional external anchoring)
    // ------------------------------------------------------------------
    class BlockchainAnchor {
    public:
        /**
         * @brief Anchor the Merkle root on an external blockchain.
         *
         * When no real blockchain connection is configured this method
         * returns a proof with a synthetic transaction hash derived from
         * the merkle_root itself (for testing / offline use).
         *
         * @param merkle_root  Root to anchor.
         * @return IntegrityProof with populated fields.
         */
        IntegrityProof anchorToBlockchain(
            const std::string& merkle_root
        );

        /**
         * @brief Verify that a proof is consistent (root matches; offline check).
         * @return true if the proof is self-consistent.
         */
        bool verifyBlockchainAnchor(const IntegrityProof& proof);
    };
};

} // namespace importers
} // namespace themis
