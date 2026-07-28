/**
 * @file tensor_compat.h
 * @brief Compatibility aliases for legacy tensor type names.
 *
 * Provides lightweight wrapper types and aliases so that tests and
 * tool code using older ThemisDB tensor type names continue to compile.
 */

// Compatibility aliases and lightweight wrappers for legacy test/type names
#pragma once

#include "storage/tensor_train_decomposer.h"

namespace themis {
namespace tensor {

// Lightweight metadata struct previously exposed as `TensorTrain` in tests.
struct TensorTrain {
	std::size_t order = 0;
	std::vector<std::size_t> shape;
	std::vector<std::size_t> ranks;
};

// Wrapper around storage::TTTrain that exposes a `cores` member for legacy tests
// and provides implicit conversion to the canonical storage type.
struct TensorTrainCore {
	themis::storage::TTTrain train;
	std::vector<themis::storage::TTCore>& cores;

	TensorTrainCore() : train(), cores(train.cores) {}

	// Implicit conversion so functions expecting storage::TTTrain accept this.
	operator const themis::storage::TTTrain&() const { return train; }
	operator themis::storage::TTTrain&() { return train; }
};

// Forward declarations for classes declared elsewhere in the tensor headers.
class SimilarityBasedDetector;
class TTDecompositionStrategy;

// Legacy detector alias
using SimilarityBasedRedundancyDetector = SimilarityBasedDetector;

// Legacy generic name for the primary compression strategy (map to TT strategy)
using CompressionStrategy = TTDecompositionStrategy;

} // namespace tensor
} // namespace themis
