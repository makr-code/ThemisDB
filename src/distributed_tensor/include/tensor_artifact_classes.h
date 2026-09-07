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

/**
 * @brief Static helpers for ArtifactClass and TruthSemantic validation and
 *        string serialization.
 */
struct ArtifactClassifier {

    /**
     * @brief Return true when the (class, semantic) combination is allowed.
     *
     * Allowed combinations:
     *   - SOURCE_OF_TRUTH  + GROUND_TRUTH   → valid (Graph Truth Layer only)
     *   - DERIVED          + ADVISORY_ONLY  → valid (tensor mid-layer artifacts)
     *   - EPHEMERAL        + ADVISORY_ONLY  → valid (temporary cache entries)
     *
     * All other combinations are invalid.
     *
     * @param klass     Artifact class to check.
     * @param semantic  Truth semantic to check.
     * @return          true if the combination satisfies advisory-only policy.
     */
    static bool isValidCombination(ArtifactClass klass,
                                   TruthSemantic  semantic) noexcept;

    /**
     * @brief Convert an ArtifactClass to its canonical uppercase string.
     *
     * @param klass  Artifact class to convert.
     * @return       One of "SOURCE_OF_TRUTH", "DERIVED", "EPHEMERAL", or "UNKNOWN".
     */
    static std::string classToString(ArtifactClass klass);

    /**
     * @brief Parse an ArtifactClass from a canonical string.
     *
     * @param class_str  String from classToString().
     * @return           Parsed class, or nullopt if unrecognized.
     */
    static std::optional<ArtifactClass> stringToClass(const std::string& class_str);

    /**
     * @brief Convert a TruthSemantic to its canonical uppercase string.
     *
     * @param semantic  Truth semantic to convert.
     * @return          One of "ADVISORY_ONLY", "GROUND_TRUTH", or "UNKNOWN".
     */
    static std::string semanticToString(TruthSemantic semantic);

    /**
     * @brief Parse a TruthSemantic from a canonical string.
     *
     * @param semantic_str  String from semanticToString().
     * @return              Parsed semantic, or nullopt if unrecognized.
     */
    static std::optional<TruthSemantic> stringToSemantic(const std::string& semantic_str);
};

} // namespace distributed_tensor
} // namespace themis
