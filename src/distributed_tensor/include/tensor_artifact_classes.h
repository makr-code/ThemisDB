/**
 * @file tensor_artifact_classes.h
 * @brief ArtifactClassifier — validation and string-conversion helpers for
 *        ArtifactClass and TruthSemantic.
 *
 * Provides static methods used by ArtifactManifest serialization and
 * validation logic.  The key invariant enforced here is:
 *
 *   - SOURCE_OF_TRUTH artifacts MUST carry GROUND_TRUTH semantics.
 *   - DERIVED and EPHEMERAL artifacts MUST carry ADVISORY_ONLY semantics.
 *
 * This prevents accidental promotion of tensor artifacts to ground-truth
 * status, which would violate the advisory-only policy documented in
 * artifact_manifest.h.
 *
 * @see artifact_manifest.h — full lifecycle and advisory-only policy
 * @see ArtifactManifest::validate() — uses ArtifactClassifier::isValidCombination
 */

#pragma once

#include "artifact_manifest.h"

#include <optional>
#include <string>

namespace themis {
namespace distributed_tensor {

// ---------------------------------------------------------------------------
// ArtifactClassifier
// ---------------------------------------------------------------------------

struct ArtifactClassifier {

    /**
     * @brief Is Valid Combination.
     * @param[in] klass Input parameter.
     * @param[in] semantic Input parameter.
     * @return True when the operation succeeds.
     * @note Exception safety: noexcept.
     */
    static bool isValidCombination(ArtifactClass klass,
                                   TruthSemantic  semantic) noexcept;

    /**
     * @brief Class To String.
     * @param[in] klass Input parameter.
     * @return Return value.
     */
    static std::string classToString(ArtifactClass klass);

    /**
     * @brief String To Class.
     * @param[in] class_str Input parameter.
     * @return Return value.
     */
    static std::optional<ArtifactClass> stringToClass(const std::string& class_str);

    /**
     * @brief Semantic To String.
     * @param[in] semantic Input parameter.
     * @return Return value.
     */
    static std::string semanticToString(TruthSemantic semantic);

    /**
     * @brief String To Semantic.
     * @param[in] semantic_str Input parameter.
     * @return Return value.
     */
    static std::optional<TruthSemantic> stringToSemantic(const std::string& semantic_str);
};

} // namespace distributed_tensor
} // namespace themis
