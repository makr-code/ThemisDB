/**
 * @file blockchain_integrity.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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
        std::string buildMerkleTree(
            const std::vector<json>& records,
            const std::string& leaf_hash_algorithm = "SHA256"
        );

        bool verifyRecordInTree(
            const json& record,
            const std::string& merkle_root,
            const std::vector<std::string>& sibling_hashes = {}
        );

    private:
        /**
         * @brief Sha256 Hex.
         * @param[in] input Input parameter.
         * @return Return value.
         */
        static std::string sha256Hex(const std::string& input);
        /**
         * @brief Combine Hashes.
         * @param[in] left Input parameter.
         * @param[in] right Input parameter.
         * @return Return value.
         */
        static std::string combineHashes(const std::string& left,
                                         const std::string& right);
    };

    // ------------------------------------------------------------------
    // Blockchain anchor (optional external anchoring)
    // ------------------------------------------------------------------
    class BlockchainAnchor {
    public:
        /**
         * @brief Anchor To Blockchain.
         * @param[in] merkle_root Input parameter.
         * @return Return value.
         */
        IntegrityProof anchorToBlockchain(
            const std::string& merkle_root
        );

        /**
         * @brief Verify Blockchain Anchor.
         * @param[in] proof Input parameter.
         * @return True when the operation succeeds.
         */
        bool verifyBlockchainAnchor(const IntegrityProof& proof);
    };
};

} // namespace importers
} // namespace themis
