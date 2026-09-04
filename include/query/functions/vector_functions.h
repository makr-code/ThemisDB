/**
 * @file vector_functions.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "function_registry.h"
#include <cstddef>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <random>
#include <limits>



namespace themis {
namespace query {
namespace functions {

/**
 * @brief Vector/Embedding Functions for AQL
 * 
 * Provides functions for vector operations commonly used in:
 * - Machine Learning embeddings (BERT, OpenAI, etc.)
 * - Similarity search
 * - Recommendation systems
 * - Semantic search
 * 
 * ## Supported Operations
 * - Similarity: COSINE_SIMILARITY, EUCLIDEAN_DISTANCE, DOT_PRODUCT, MANHATTAN_DISTANCE
 * - Normalization: L2_NORMALIZE, MIN_MAX_NORMALIZE
 * - Aggregation: VECTOR_SUM, VECTOR_AVG, VECTOR_MEAN
 * - Operations: VECTOR_ADD, VECTOR_SUB, VECTOR_SCALE, VECTOR_DOT
 * - Search: SIMILARITY (top-k search), VECTOR_NEAREST
 * 
 * ## Vector Format
 * Vectors are represented as JSON arrays of numbers: [0.1, 0.2, 0.3, ...]
 * 
 * ## ArangoDB Compatibility
 * Compatible with ArangoDB's vector search functions
 */

// ============================================================================
// Helper Functions
// ============================================================================

namespace vector_helpers {

// Validate that a JSON value is a numeric vector
inline void validateVector(const nlohmann::json& vec, const std::string& funcName) {
    if (!vec.is_array()) {
        throw std::runtime_error(funcName + ": Expected vector (array of numbers)");
    }
    for (const auto& elem : vec) {
        if (!elem.is_number()) {
            throw std::runtime_error(funcName + ": Vector must contain only numbers");
        }
    }
}

// Validate two vectors have same dimension
inline void validateSameDimension(const nlohmann::json& v1, const nlohmann::json& v2, 
                                   const std::string& funcName) {
    if (v1.size() != v2.size()) {
        throw std::runtime_error(funcName + ": Vectors must have same dimension");
    }
}

// Convert JSON array to std::vector<double>
inline std::vector<double> toVector(const nlohmann::json& vec) {
    std::vector<double> result = {};

    result.reserve(vec.size());
    for (const auto& elem : vec) {
        result.push_back(elem.get<double>());
    }
    return result;
}

// Convert std::vector<double> to JSON array
inline nlohmann::json fromVector(const std::vector<double>& vec) {
    nlohmann::json result = nlohmann::json::array();
    for (double v : vec) {
        result.push_back(v);
    }
    return result;
}

// L2 norm (Euclidean length)
inline double l2Norm(const std::vector<double>& vec) {
    double sum = 0.0;
    for (double v : vec) {
        sum += v * v;
    }
    return std::sqrt(sum);
}

// Dot product
inline double dotProduct(const std::vector<double>& v1, const std::vector<double>& v2) {
    double sum = 0.0;
    for (size_t i = 0; i < v1.size(); ++i) {
        sum += v1[i] * v2[i];
    }
    return sum;
}

} // namespace vector_helpers

// ============================================================================
// Similarity Functions
// ============================================================================

/**
 * @brief COSINE_SIMILARITY(vec1, vec2) - Cosine similarity between vectors
 * Returns: Value between -1 and 1 (1 = identical direction)
 */
class CosineSimilarityFunction : public IFunction {
public:
    ~CosineSimilarityFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "COSINE_SIMILARITY",
            "Vector",
            "Calculate cosine similarity between two vectors (1 = identical, -1 = opposite)",
            {
                {"vec1", ArgType::VECTOR, true, nullptr, "First vector"},
                {"vec2", ArgType::VECTOR, true, nullptr, "Second vector"}
            },
            ArgType::NUMBER,
            true,
            false,
            {"COSINE_SIMILARITY([1,0,0], [1,0,0]) = 1.0", "COSINE_SIMILARITY([1,0], [0,1]) = 0.0"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           [[maybe_unused]] const FunctionContext& ctx) const override {
        vector_helpers::validateVector(args[0], "COSINE_SIMILARITY");
        vector_helpers::validateVector(args[1], "COSINE_SIMILARITY");
        vector_helpers::validateSameDimension(args[0], args[1], "COSINE_SIMILARITY");
        
        auto v1 = vector_helpers::toVector(args[0]);
        auto v2 = vector_helpers::toVector(args[1]);
        
        double dot = vector_helpers::dotProduct(v1, v2);
        double norm1 = vector_helpers::l2Norm(v1);
        double norm2 = vector_helpers::l2Norm(v2);
        
        if (norm1 == 0.0 || norm2 == 0.0) {
            return 0.0; // Handle zero vectors
        }
        
        return dot / (norm1 * norm2);
    }
};

/**
 * @brief EUCLIDEAN_DISTANCE(vec1, vec2) - Euclidean (L2) distance
 */
class EuclideanDistanceFunction : public IFunction {
public:
    ~EuclideanDistanceFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "EUCLIDEAN_DISTANCE",
            "Vector",
            "Calculate Euclidean (L2) distance between two vectors",
            {
                {"vec1", ArgType::VECTOR, true, nullptr, "First vector"},
                {"vec2", ArgType::VECTOR, true, nullptr, "Second vector"}
            },
            ArgType::NUMBER,
            true,
            false,
            {"EUCLIDEAN_DISTANCE([0,0], [3,4]) = 5.0"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           [[maybe_unused]] const FunctionContext& ctx) const override {
        vector_helpers::validateVector(args[0], "EUCLIDEAN_DISTANCE");
        vector_helpers::validateVector(args[1], "EUCLIDEAN_DISTANCE");
        vector_helpers::validateSameDimension(args[0], args[1], "EUCLIDEAN_DISTANCE");
        
        auto v1 = vector_helpers::toVector(args[0]);
        auto v2 = vector_helpers::toVector(args[1]);
        
        double sum = 0.0;
        for (size_t i = 0; i < v1.size(); ++i) {
            double diff = v1[i] - v2[i];
            sum += diff * diff;
        }
        
        return std::sqrt(sum);
    }
};

/**
 * @brief DOT_PRODUCT(vec1, vec2) - Dot product of two vectors
 */
class DotProductFunction : public IFunction {
public:
    ~DotProductFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "DOT_PRODUCT",
            "Vector",
            "Calculate dot product (inner product) of two vectors",
            {
                {"vec1", ArgType::VECTOR, true, nullptr, "First vector"},
                {"vec2", ArgType::VECTOR, true, nullptr, "Second vector"}
            },
            ArgType::NUMBER,
            true,
            false,
            {"DOT_PRODUCT([1,2,3], [4,5,6]) = 32"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           [[maybe_unused]] const FunctionContext& ctx) const override {
        vector_helpers::validateVector(args[0], "DOT_PRODUCT");
        vector_helpers::validateVector(args[1], "DOT_PRODUCT");
        vector_helpers::validateSameDimension(args[0], args[1], "DOT_PRODUCT");
        
        auto v1 = vector_helpers::toVector(args[0]);
        auto v2 = vector_helpers::toVector(args[1]);
        
        return vector_helpers::dotProduct(v1, v2);
    }
};

/**
 * @brief MANHATTAN_DISTANCE(vec1, vec2) - Manhattan (L1) distance
 */
class ManhattanDistanceFunction : public IFunction {
public:
    ~ManhattanDistanceFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "MANHATTAN_DISTANCE",
            "Vector",
            "Calculate Manhattan (L1/taxicab) distance between two vectors",
            {
                {"vec1", ArgType::VECTOR, true, nullptr, "First vector"},
                {"vec2", ArgType::VECTOR, true, nullptr, "Second vector"}
            },
            ArgType::NUMBER,
            true,
            false,
            {"MANHATTAN_DISTANCE([0,0], [3,4]) = 7"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           [[maybe_unused]] const FunctionContext& ctx) const override {
        vector_helpers::validateVector(args[0], "MANHATTAN_DISTANCE");
        vector_helpers::validateVector(args[1], "MANHATTAN_DISTANCE");
        vector_helpers::validateSameDimension(args[0], args[1], "MANHATTAN_DISTANCE");
        
        auto v1 = vector_helpers::toVector(args[0]);
        auto v2 = vector_helpers::toVector(args[1]);
        
        double sum = 0.0;
        for (size_t i = 0; i < v1.size(); ++i) {
            sum += std::abs(v1[i] - v2[i]);
        }
        
        return sum;
    }
};

/**
 * @brief CHEBYSHEV_DISTANCE(vec1, vec2) - Chebyshev (L∞) distance
 */
class ChebyshevDistanceFunction : public IFunction {
public:
    ~ChebyshevDistanceFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "CHEBYSHEV_DISTANCE",
            "Vector",
            "Calculate Chebyshev (L-infinity/max) distance between two vectors",
            {
                {"vec1", ArgType::VECTOR, true, nullptr, "First vector"},
                {"vec2", ArgType::VECTOR, true, nullptr, "Second vector"}
            },
            ArgType::NUMBER,
            true,
            false,
            {"CHEBYSHEV_DISTANCE([0,0], [3,4]) = 4"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           [[maybe_unused]] const FunctionContext& ctx) const override {
        vector_helpers::validateVector(args[0], "CHEBYSHEV_DISTANCE");
        vector_helpers::validateVector(args[1], "CHEBYSHEV_DISTANCE");
        vector_helpers::validateSameDimension(args[0], args[1], "CHEBYSHEV_DISTANCE");
        
        auto v1 = vector_helpers::toVector(args[0]);
        auto v2 = vector_helpers::toVector(args[1]);
        
        double maxDiff = 0.0;
        for (size_t i = 0; i < v1.size(); ++i) {
            maxDiff = std::max(maxDiff, std::abs(v1[i] - v2[i]));
        }
        
        return maxDiff;
    }
};

/**
 * @brief SIMILARITY(vec1, vec2, k) - Top-k similarity search helper
 * Note: In practice, this is used with an index. This is a placeholder.
 */
class SimilarityFunction : public IFunction {
public:
    ~SimilarityFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "SIMILARITY",
            "Vector",
            "Calculate similarity score (higher = more similar). Used in vector search queries.",
            {
                {"vec1", ArgType::VECTOR, true, nullptr, "Query vector"},
                {"vec2", ArgType::VECTOR, true, nullptr, "Target vector"},
                {"k", ArgType::INTEGER, false, nlohmann::json(10), "Number of results (for index search)"}
            },
            ArgType::NUMBER,
            true,
            false,
            {"SIMILARITY(query_embedding, doc._embedding, 5)"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           [[maybe_unused]] const FunctionContext& ctx) const override {
        // Default implementation uses cosine similarity
        vector_helpers::validateVector(args[0], "SIMILARITY");
        vector_helpers::validateVector(args[1], "SIMILARITY");
        vector_helpers::validateSameDimension(args[0], args[1], "SIMILARITY");
        
        auto v1 = vector_helpers::toVector(args[0]);
        auto v2 = vector_helpers::toVector(args[1]);
        
        double dot = vector_helpers::dotProduct(v1, v2);
        double norm1 = vector_helpers::l2Norm(v1);
        double norm2 = vector_helpers::l2Norm(v2);
        
        if (norm1 == 0.0 || norm2 == 0.0) {
            return 0.0;
        }
        
        // Return cosine similarity as score (0 to 1 range for normalized vectors)
        return (dot / (norm1 * norm2) + 1.0) / 2.0; // Map [-1,1] to [0,1]
    }
};

// ============================================================================
// Normalization Functions
// ============================================================================

/**
 * @brief L2_NORMALIZE(vec) - Normalize vector to unit length
 */
class L2NormalizeFunction : public IFunction {
public:
    ~L2NormalizeFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "L2_NORMALIZE",
            "Vector",
            "Normalize vector to unit length (L2 norm = 1)",
            {
                {"vec", ArgType::VECTOR, true, nullptr, "Vector to normalize"}
            },
            ArgType::VECTOR,
            true,
            false,
            {"L2_NORMALIZE([3,4]) = [0.6, 0.8]"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           [[maybe_unused]] const FunctionContext& ctx) const override {
        vector_helpers::validateVector(args[0], "L2_NORMALIZE");
        
        auto vec = vector_helpers::toVector(args[0]);
        double norm = vector_helpers::l2Norm(vec);
        
        if (norm == 0.0) {
            return args[0]; // Return original if zero vector
        }
        
        for (double& v : vec) {
            v /= norm;
        }
        
        return vector_helpers::fromVector(vec);
    }
};

/**
 * @brief MIN_MAX_NORMALIZE(vec, min, max) - Scale vector to [0,1] range
 */
class MinMaxNormalizeFunction : public IFunction {
public:
    ~MinMaxNormalizeFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "MIN_MAX_NORMALIZE",
            "Vector",
            "Scale vector values to [0,1] range based on min/max",
            {
                {"vec", ArgType::VECTOR, true, nullptr, "Vector to normalize"},
                {"min", ArgType::NUMBER, false, nullptr, "Minimum value (auto-detected if not provided)"},
                {"max", ArgType::NUMBER, false, nullptr, "Maximum value (auto-detected if not provided)"}
            },
            ArgType::VECTOR,
            true,
            false,
            {"MIN_MAX_NORMALIZE([2,4,6], 0, 10) = [0.2, 0.4, 0.6]"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           [[maybe_unused]] const FunctionContext& ctx) const override {
        vector_helpers::validateVector(args[0], "MIN_MAX_NORMALIZE");
        
        auto vec = vector_helpers::toVector(args[0]);
        
        double minVal, maxVal;
        if (args.size() >= 3 && !args[1].is_null() && !args[2].is_null()) {
            minVal = args[1].get<double>();
            maxVal = args[2].get<double>();
        } else {
            minVal = *std::min_element(vec.begin(), vec.end());
            maxVal = *std::max_element(vec.begin(), vec.end());
        }
        
        double range = maxVal - minVal;
        if (range == 0.0) {
            return args[0]; // All values are the same
        }
        
        for (double& v : vec) {
            v = (v - minVal) / range;
        }
        
        return vector_helpers::fromVector(vec);
    }
};

// ============================================================================
// Vector Arithmetic Functions
// ============================================================================

/**
 * @brief VECTOR_ADD(vec1, vec2) - Element-wise addition
 */
class VectorAddFunction : public IFunction {
public:
    ~VectorAddFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "VECTOR_ADD",
            "Vector",
            "Add two vectors element-wise",
            {
                {"vec1", ArgType::VECTOR, true, nullptr, "First vector"},
                {"vec2", ArgType::VECTOR, true, nullptr, "Second vector"}
            },
            ArgType::VECTOR,
            true,
            false,
            {"VECTOR_ADD([1,2,3], [4,5,6]) = [5,7,9]"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           [[maybe_unused]] const FunctionContext& ctx) const override {
        vector_helpers::validateVector(args[0], "VECTOR_ADD");
        vector_helpers::validateVector(args[1], "VECTOR_ADD");
        vector_helpers::validateSameDimension(args[0], args[1], "VECTOR_ADD");
        
        auto v1 = vector_helpers::toVector(args[0]);
        auto v2 = vector_helpers::toVector(args[1]);
        
        for (size_t i = 0; i < v1.size(); ++i) {
            v1[i] += v2[i];
        }
        
        return vector_helpers::fromVector(v1);
    }
};

/**
 * @brief VECTOR_SUB(vec1, vec2) - Element-wise subtraction
 */
class VectorSubFunction : public IFunction {
public:
    ~VectorSubFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "VECTOR_SUB",
            "Vector",
            "Subtract second vector from first element-wise",
            {
                {"vec1", ArgType::VECTOR, true, nullptr, "First vector"},
                {"vec2", ArgType::VECTOR, true, nullptr, "Second vector (subtracted)"}
            },
            ArgType::VECTOR,
            true,
            false,
            {"VECTOR_SUB([5,7,9], [1,2,3]) = [4,5,6]"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           [[maybe_unused]] const FunctionContext& ctx) const override {
        vector_helpers::validateVector(args[0], "VECTOR_SUB");
        vector_helpers::validateVector(args[1], "VECTOR_SUB");
        vector_helpers::validateSameDimension(args[0], args[1], "VECTOR_SUB");
        
        auto v1 = vector_helpers::toVector(args[0]);
        auto v2 = vector_helpers::toVector(args[1]);
        
        for (size_t i = 0; i < v1.size(); ++i) {
            v1[i] -= v2[i];
        }
        
        return vector_helpers::fromVector(v1);
    }
};

/**
 * @brief VECTOR_MUL(vec1, vec2) - Element-wise multiplication (Hadamard product)
 */
class VectorMulFunction : public IFunction {
public:
    ~VectorMulFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "VECTOR_MUL",
            "Vector",
            "Multiply two vectors element-wise (Hadamard product)",
            {
                {"vec1", ArgType::VECTOR, true, nullptr, "First vector"},
                {"vec2", ArgType::VECTOR, true, nullptr, "Second vector"}
            },
            ArgType::VECTOR,
            true,
            false,
            {"VECTOR_MUL([1,2,3], [4,5,6]) = [4,10,18]"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           [[maybe_unused]] const FunctionContext& ctx) const override {
        vector_helpers::validateVector(args[0], "VECTOR_MUL");
        vector_helpers::validateVector(args[1], "VECTOR_MUL");
        vector_helpers::validateSameDimension(args[0], args[1], "VECTOR_MUL");
        
        auto v1 = vector_helpers::toVector(args[0]);
        auto v2 = vector_helpers::toVector(args[1]);
        
        for (size_t i = 0; i < v1.size(); ++i) {
            v1[i] *= v2[i];
        }
        
        return vector_helpers::fromVector(v1);
    }
};

/**
 * @brief VECTOR_SCALE(vec, scalar) - Scale vector by scalar
 */
class VectorScaleFunction : public IFunction {
public:
    ~VectorScaleFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "VECTOR_SCALE",
            "Vector",
            "Multiply all vector elements by a scalar",
            {
                {"vec", ArgType::VECTOR, true, nullptr, "Vector to scale"},
                {"scalar", ArgType::NUMBER, true, nullptr, "Scalar multiplier"}
            },
            ArgType::VECTOR,
            true,
            false,
            {"VECTOR_SCALE([1,2,3], 2) = [2,4,6]"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           [[maybe_unused]] const FunctionContext& ctx) const override {
        vector_helpers::validateVector(args[0], "VECTOR_SCALE");
        
        auto vec = vector_helpers::toVector(args[0]);
        double scalar = args[1].get<double>();
        
        for (double& v : vec) {
            v *= scalar;
        }
        
        return vector_helpers::fromVector(vec);
    }
};

// ============================================================================
// Aggregation Functions
// ============================================================================

/**
 * @brief VECTOR_SUM(vec) - Sum of all vector elements
 */
class VectorSumFunction : public IFunction {
public:
    ~VectorSumFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "VECTOR_SUM",
            "Vector",
            "Calculate sum of all vector elements",
            {
                {"vec", ArgType::VECTOR, true, nullptr, "Input vector"}
            },
            ArgType::NUMBER,
            true,
            false,
            {"VECTOR_SUM([1,2,3,4]) = 10"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           [[maybe_unused]] const FunctionContext& ctx) const override {
        vector_helpers::validateVector(args[0], "VECTOR_SUM");
        
        auto vec = vector_helpers::toVector(args[0]);
        return std::accumulate(vec.begin(), vec.end(), 0.0);
    }
};

/**
 * @brief VECTOR_AVG(vec) or VECTOR_MEAN(vec) - Average of vector elements
 */
class VectorAvgFunction : public IFunction {
public:
    ~VectorAvgFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "VECTOR_AVG",
            "Vector",
            "Calculate average (mean) of all vector elements",
            {
                {"vec", ArgType::VECTOR, true, nullptr, "Input vector"}
            },
            ArgType::NUMBER,
            true,
            false,
            {"VECTOR_AVG([1,2,3,4]) = 2.5"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           [[maybe_unused]] const FunctionContext& ctx) const override {
        vector_helpers::validateVector(args[0], "VECTOR_AVG");
        
        auto vec = vector_helpers::toVector(args[0]);
        if (vec.empty()) {
            return 0.0;
        }
        
        double sum = std::accumulate(vec.begin(), vec.end(), 0.0);
        return sum / vec.size();
    }
};

/**
 * @brief VECTOR_NORM(vec, p) - Lp norm of vector
 */
class VectorNormFunction : public IFunction {
public:
    ~VectorNormFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "VECTOR_NORM",
            "Vector",
            "Calculate Lp norm of vector (default L2/Euclidean)",
            {
                {"vec", ArgType::VECTOR, true, nullptr, "Input vector"},
                {"p", ArgType::NUMBER, false, nlohmann::json(2), "Norm order (1=Manhattan, 2=Euclidean, inf=Chebyshev)"}
            },
            ArgType::NUMBER,
            true,
            false,
            {"VECTOR_NORM([3,4]) = 5", "VECTOR_NORM([3,4], 1) = 7"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           [[maybe_unused]] const FunctionContext& ctx) const override {
        vector_helpers::validateVector(args[0], "VECTOR_NORM");
        
        auto vec = vector_helpers::toVector(args[0]);
        double p = args.size() > 1 && !args[1].is_null() ? args[1].get<double>() : 2.0;
        
        if (std::isinf(p)) {
            // L-infinity norm (max absolute value)
            double maxVal = 0.0;
            for (double v : vec) {
                maxVal = std::max(maxVal, std::abs(v));
            }
            return maxVal;
        }
        
        double sum = 0.0;
        for (double v : vec) {
            sum += std::pow(std::abs(v), p);
        }
        return std::pow(sum, 1.0 / p);
    }
};

/**
 * @brief VECTOR_DIM(vec) - Dimensionality of vector
 */
class VectorDimFunction : public IFunction {
public:
    ~VectorDimFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "VECTOR_DIM",
            "Vector",
            "Get dimensionality (number of elements) of vector",
            {
                {"vec", ArgType::VECTOR, true, nullptr, "Input vector"}
            },
            ArgType::INTEGER,
            true,
            false,
            {"VECTOR_DIM([1,2,3]) = 3"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           [[maybe_unused]] const FunctionContext& ctx) const override {
        vector_helpers::validateVector(args[0], "VECTOR_DIM");
        return static_cast<int64_t>(args[0].size());
    }
};

/**
 * @brief VECTOR_MIN(vec) - Minimum element in vector
 */
class VectorMinFunction : public IFunction {
public:
    ~VectorMinFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "VECTOR_MIN",
            "Vector",
            "Get minimum element in vector",
            {
                {"vec", ArgType::VECTOR, true, nullptr, "Input vector"}
            },
            ArgType::NUMBER,
            true,
            false,
            {"VECTOR_MIN([3,1,4,1,5]) = 1"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           [[maybe_unused]] const FunctionContext& ctx) const override {
        vector_helpers::validateVector(args[0], "VECTOR_MIN");
        
        auto vec = vector_helpers::toVector(args[0]);
        if (vec.empty()) {
            return nullptr;
        }
        
        return *std::min_element(vec.begin(), vec.end());
    }
};

/**
 * @brief VECTOR_MAX(vec) - Maximum element in vector
 */
class VectorMaxFunction : public IFunction {
public:
    ~VectorMaxFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "VECTOR_MAX",
            "Vector",
            "Get maximum element in vector",
            {
                {"vec", ArgType::VECTOR, true, nullptr, "Input vector"}
            },
            ArgType::NUMBER,
            true,
            false,
            {"VECTOR_MAX([3,1,4,1,5]) = 5"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           [[maybe_unused]] const FunctionContext& ctx) const override {
        vector_helpers::validateVector(args[0], "VECTOR_MAX");
        
        auto vec = vector_helpers::toVector(args[0]);
        if (vec.empty()) {
            return nullptr;
        }
        
        return *std::max_element(vec.begin(), vec.end());
    }
};

// ============================================================================
// Utility Functions
// ============================================================================

/**
 * @brief VECTOR_ZEROS(n) - Create zero vector
 */
class VectorZerosFunction : public IFunction {
public:
    ~VectorZerosFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "VECTOR_ZEROS",
            "Vector",
            "Create a zero vector of specified dimension",
            {
                {"dimension", ArgType::INTEGER, true, nullptr, "Number of dimensions"}
            },
            ArgType::VECTOR,
            true,
            false,
            {"VECTOR_ZEROS(3) = [0,0,0]"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           [[maybe_unused]] const FunctionContext& ctx) const override {
        int n = args[0].get<int>();
        if (n < 0) {
            throw std::runtime_error("VECTOR_ZEROS: Dimension must be non-negative");
        }
        
        std::vector<double> vec(n, 0.0);
        return vector_helpers::fromVector(vec);
    }
};

/**
 * @brief VECTOR_ONES(n) - Create vector of ones
 */
class VectorOnesFunction : public IFunction {
public:
    ~VectorOnesFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "VECTOR_ONES",
            "Vector",
            "Create a vector of ones with specified dimension",
            {
                {"dimension", ArgType::INTEGER, true, nullptr, "Number of dimensions"}
            },
            ArgType::VECTOR,
            true,
            false,
            {"VECTOR_ONES(3) = [1,1,1]"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           [[maybe_unused]] const FunctionContext& ctx) const override {
        int n = args[0].get<int>();
        if (n < 0) {
            throw std::runtime_error("VECTOR_ONES: Dimension must be non-negative");
        }
        
        std::vector<double> vec(n, 1.0);
        return vector_helpers::fromVector(vec);
    }
};

/**
 * @brief VECTOR_RANDOM(n, min, max) - Create random vector
 */
class VectorRandomFunction : public IFunction {
public:
    ~VectorRandomFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "VECTOR_RANDOM",
            "Vector",
            "Create a random vector with values in [min, max] range",
            {
                {"dimension", ArgType::INTEGER, true, nullptr, "Number of dimensions"},
                {"min", ArgType::NUMBER, false, nlohmann::json(0.0), "Minimum value"},
                {"max", ArgType::NUMBER, false, nlohmann::json(1.0), "Maximum value"}
            },
            ArgType::VECTOR,
            false, // Not deterministic
            false,
            {"VECTOR_RANDOM(3)", "VECTOR_RANDOM(5, -1, 1)"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           [[maybe_unused]] const FunctionContext& ctx) const override {
        int n = args[0].get<int>();
        if (n < 0) {
            throw std::runtime_error("VECTOR_RANDOM: Dimension must be non-negative");
        }
        
        double minVal = args.size() > 1 && !args[1].is_null() ? args[1].get<double>() : 0.0;
        double maxVal = args.size() > 2 && !args[2].is_null() ? args[2].get<double>() : 1.0;
        
        std::random_device rd = {};
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> dis(minVal, maxVal);
        
        std::vector<double> vec(n);
        for (int i = 0; i < n; ++i) {
            vec[i] = dis(gen);
        }
        
        return vector_helpers::fromVector(vec);
    }
};

/**
 * @brief VECTOR_SLICE(vec, start, end) - Extract sub-vector
 */
class VectorSliceFunction : public IFunction {
public:
    ~VectorSliceFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "VECTOR_SLICE",
            "Vector",
            "Extract a sub-vector from start to end index",
            {
                {"vec", ArgType::VECTOR, true, nullptr, "Input vector"},
                {"start", ArgType::INTEGER, true, nullptr, "Start index (inclusive)"},
                {"end", ArgType::INTEGER, false, nullptr, "End index (exclusive, default: vector length)"}
            },
            ArgType::VECTOR,
            true,
            false,
            {"VECTOR_SLICE([1,2,3,4,5], 1, 4) = [2,3,4]"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           [[maybe_unused]] const FunctionContext& ctx) const override {
        vector_helpers::validateVector(args[0], "VECTOR_SLICE");
        
        auto vec = vector_helpers::toVector(args[0]);
        const int64_t vec_size_i64 = static_cast<int64_t>(
            std::min(vec.size(), static_cast<size_t>(std::numeric_limits<int64_t>::max())));
        int64_t start = args[1].get<int64_t>();
        int64_t end = args.size() > 2 && !args[2].is_null() ? args[2].get<int64_t>() : vec_size_i64;

        if (start < 0) {
          start = 0;
        }
        if (end < 0) {
          end = 0;
        }
        if (start > vec_size_i64) {
          start = vec_size_i64;
        }
        if (end > vec_size_i64) {
          end = vec_size_i64;
        }
        if (start >= end) {
          return nlohmann::json::array();
        }
        
        std::vector<double> result(
            vec.begin() + static_cast<std::ptrdiff_t>(start),
            vec.begin() + static_cast<std::ptrdiff_t>(end));
        return vector_helpers::fromVector(result);
    }
};

/**
 * @brief VECTOR_CONCAT(vec1, vec2, ...) - Concatenate vectors
 */
class VectorConcatFunction : public IFunction {
public:
    ~VectorConcatFunction() override = default;
    FunctionSignature signature() const override {
        return {
            "VECTOR_CONCAT",
            "Vector",
            "Concatenate multiple vectors into one",
            {
                {"vectors", ArgType::ANY, true, nullptr, "Vectors to concatenate (variadic)"}
            },
            ArgType::VECTOR,
            true,
            false,
            {"VECTOR_CONCAT([1,2], [3,4]) = [1,2,3,4]"}
        };
    }
    
    void validateArgs(const std::vector<nlohmann::json>& args) const override {
        if (args.empty()) {
            throw std::runtime_error("VECTOR_CONCAT requires at least one argument");
        }
        for (const auto& arg : args) {
            vector_helpers::validateVector(arg, "VECTOR_CONCAT");
        }
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           [[maybe_unused]] const FunctionContext& ctx) const override {
        std::vector<double> result;
        
        for (const auto& arg : args) {
            auto vec = vector_helpers::toVector(arg);
            result.insert(result.end(), vec.begin(), vec.end());
        }
        
        return vector_helpers::fromVector(result);
    }
};

// ============================================================================
// Registration Function
// ============================================================================

/**
 * @brief Register all Vector functions with the registry
 */
inline void registerVectorFunctions(FunctionRegistry& registry) {
    // Similarity
    registry.registerFunction(std::make_unique<CosineSimilarityFunction>());
    registry.registerFunction(std::make_unique<EuclideanDistanceFunction>());
    registry.registerFunction(std::make_unique<DotProductFunction>());
    registry.registerFunction(std::make_unique<ManhattanDistanceFunction>());
    registry.registerFunction(std::make_unique<ChebyshevDistanceFunction>());
    registry.registerFunction(std::make_unique<SimilarityFunction>());
    
    // Normalization
    registry.registerFunction(std::make_unique<L2NormalizeFunction>());
    registry.registerFunction(std::make_unique<MinMaxNormalizeFunction>());
    
    // Arithmetic
    registry.registerFunction(std::make_unique<VectorAddFunction>());
    registry.registerFunction(std::make_unique<VectorSubFunction>());
    registry.registerFunction(std::make_unique<VectorMulFunction>());
    registry.registerFunction(std::make_unique<VectorScaleFunction>());
    
    // Aggregation
    registry.registerFunction(std::make_unique<VectorSumFunction>());
    registry.registerFunction(std::make_unique<VectorAvgFunction>());
    registry.registerFunction(std::make_unique<VectorNormFunction>());
    registry.registerFunction(std::make_unique<VectorDimFunction>());
    registry.registerFunction(std::make_unique<VectorMinFunction>());
    registry.registerFunction(std::make_unique<VectorMaxFunction>());
    
    // Utility
    registry.registerFunction(std::make_unique<VectorZerosFunction>());
    registry.registerFunction(std::make_unique<VectorOnesFunction>());
    registry.registerFunction(std::make_unique<VectorRandomFunction>());
    registry.registerFunction(std::make_unique<VectorSliceFunction>());
    registry.registerFunction(std::make_unique<VectorConcatFunction>());
}

} // namespace functions
} // namespace query
} // namespace themis
