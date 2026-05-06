/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            tensor_functions.cpp                               ║
  Version:         1.0.0                                              ║
  Last Modified:   2026-05-05                                         ║
  Author:          copilot                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file tensor_functions.cpp
 * @brief AQL built-in TENSOR_* function implementations.
 *
 * STUB/SIMULATION NOTE:
 * Purpose: The AQL functions accept tensor data as JSON objects with "data" and
 *          "shape" fields. In production, field references will be resolved from
 *          TensorNetworkStorageEngine via the query context.
 * Activation: Always active (no build flag required).
 * Production Delta: Full storage engine field resolution planned for Phase 2 (Q4 2026).
 * Removal Plan: Integrate TensorNetworkStorageEngine context injection Q4 2026.
 */

#include "query/functions/tensor_functions.h"
#include "query/functions/function_registry.h"
#include "query/tensor_contraction_engine.h"
#include "storage/tensor_train_decomposer.h"

#include <cmath>
#include <stdexcept>
#include <string>
#include <vector>

namespace themis {
namespace query {
namespace functions {

using json = nlohmann::json;
using storage::TensorTrainDecomposer;
using storage::TensorTrainConfig;
using storage::TTTrain;

// ============================================================================
// Helper: build a TTTrain from JSON {data:[...], shape:[...], eps:float}
// ============================================================================

static std::vector<float> jsonToFloats(const json& arr) {
    std::vector<float> out;
    out.reserve(arr.size());
    for (const auto& v : arr) out.push_back(v.get<float>());
    return out;
}

static TTTrain buildTrain(const json& arg) {
    auto data_arr  = arg.at("data");
    auto shape_arr = arg.at("shape");

    std::vector<float> data = jsonToFloats(data_arr);
    std::vector<std::size_t> shape;
    for (const auto& s : shape_arr) shape.push_back(s.get<std::size_t>());

    double eps = arg.value("eps", 0.01);

    TensorTrainConfig cfg;
    cfg.eps      = eps;
    cfg.max_rank = 64;

    TensorTrainDecomposer dec;
    auto [train, stats] = dec.decompose(data, shape, cfg);
    return std::move(train);
}

// ============================================================================
// TENSOR_SIMILARITY
// ============================================================================

class TensorSimilarityFunction : public IFunction {
public:
    FunctionSignature signature() const override {
        return FunctionSignature{
            .name     = "TENSOR_SIMILARITY",
            .category = "tensor",
            .description = "Cosine similarity of two tensors in TT-compressed domain. "
                           "Arguments: two objects {data:[...], shape:[...], eps:float}. "
                           "Returns float ∈ [-1, 1]. "
                           "Ref: Holtz et al. (2012) SIAM J. Sci. Comput.",
            .arguments = {
                ArgSpec{"a", ArgType::OBJECT, true,  nullptr, "First tensor {data, shape}"},
                ArgSpec{"b", ArgType::OBJECT, true,  nullptr, "Second tensor {data, shape}"}
            },
            .return_type     = ArgType::NUMBER,
            .is_deterministic = true,
            .is_aggregate    = false,
            .cost = FunctionCost{
                .complexity       = CostComplexity::LINEARITHMIC,
                .base_cost        = 5.0,
                .per_element_cost = 0.001,
                .can_use_index    = false,
                .is_parallelizable = false
            }
        };
    }

    json execute(const std::vector<json>& args,
                 const FunctionContext& /*ctx*/) const override {
        if (args.size() < 2)
            throw std::invalid_argument("TENSOR_SIMILARITY: requires 2 arguments");
        TTTrain a = buildTrain(args[0]);
        TTTrain b = buildTrain(args[1]);
        return json(TensorContractionEngine::cosineSimilarity(a, b));
    }
};

// ============================================================================
// TENSOR_NORM
// ============================================================================

class TensorNormFunction : public IFunction {
public:
    FunctionSignature signature() const override {
        return FunctionSignature{
            .name     = "TENSOR_NORM",
            .category = "tensor",
            .description = "Frobenius norm of a tensor in TT-compressed domain. "
                           "Argument: {data:[...], shape:[...]}. Returns float ≥ 0.",
            .arguments = {
                ArgSpec{"a", ArgType::OBJECT, true, nullptr, "Tensor {data, shape}"}
            },
            .return_type     = ArgType::NUMBER,
            .is_deterministic = true,
            .is_aggregate    = false,
            .cost = FunctionCost{
                .complexity = CostComplexity::LINEARITHMIC,
                .base_cost  = 3.0
            }
        };
    }

    json execute(const std::vector<json>& args,
                 const FunctionContext& /*ctx*/) const override {
        if (args.empty())
            throw std::invalid_argument("TENSOR_NORM: requires 1 argument");
        TTTrain a = buildTrain(args[0]);
        return json(TensorContractionEngine::frobeniusNorm(a));
    }
};

// ============================================================================
// TENSOR_SLICE
// ============================================================================

class TensorSliceFunction : public IFunction {
public:
    FunctionSignature signature() const override {
        return FunctionSignature{
            .name     = "TENSOR_SLICE",
            .category = "tensor",
            .description = "Extract subtensor by fixing mode dim to index idx. "
                           "Arguments: (tensor, dim:int, idx:int). "
                           "Returns {data, shape, compression_ratio, max_rank}.",
            .arguments = {
                ArgSpec{"a",   ArgType::OBJECT,  true, nullptr, "Tensor"},
                ArgSpec{"dim", ArgType::INTEGER, true, nullptr, "Mode dimension (0-indexed)"},
                ArgSpec{"idx", ArgType::INTEGER, true, nullptr, "Index to fix"}
            },
            .return_type     = ArgType::OBJECT,
            .is_deterministic = true,
            .cost = FunctionCost{.complexity = CostComplexity::LINEAR, .base_cost = 2.0}
        };
    }

    json execute(const std::vector<json>& args,
                 const FunctionContext& /*ctx*/) const override {
        if (args.size() < 3)
            throw std::invalid_argument("TENSOR_SLICE: requires 3 arguments (tensor, dim, idx)");
        TTTrain a   = buildTrain(args[0]);
        auto dim    = static_cast<std::size_t>(args[1].get<int>());
        auto idx    = static_cast<std::size_t>(args[2].get<int>());
        TTTrain sl  = TensorContractionEngine::slice(a, dim, idx);
        auto recon  = sl.reconstruct();
        return json{
            {"data",             recon},
            {"shape",            sl.mode_sizes},
            {"compression_ratio",sl.compressionRatio()},
            {"max_rank",         sl.maxRank()}
        };
    }
};

// ============================================================================
// TENSOR_COMPRESS
// ============================================================================

class TensorCompressFunction : public IFunction {
public:
    FunctionSignature signature() const override {
        return FunctionSignature{
            .name     = "TENSOR_COMPRESS",
            .category = "tensor",
            .description = "On-the-fly TT-recompression with tolerance eps and optional max_rank. "
                           "Arguments: (tensor, eps:float=0.01, max_rank:int=0). "
                           "Returns {data, shape, compression_ratio, max_rank, achieved_eps}.",
            .arguments = {
                ArgSpec{"a",        ArgType::OBJECT,  true,  nullptr, "Tensor"},
                ArgSpec{"eps",      ArgType::NUMBER,  false, json(0.01), "Error tolerance"},
                ArgSpec{"max_rank", ArgType::INTEGER, false, json(0), "Max TT-rank (0=unlimited)"}
            },
            .return_type     = ArgType::OBJECT,
            .is_deterministic = true,
            .cost = FunctionCost{.complexity = CostComplexity::LINEARITHMIC, .base_cost = 10.0}
        };
    }

    json execute(const std::vector<json>& args,
                 const FunctionContext& /*ctx*/) const override {
        if (args.empty())
            throw std::invalid_argument("TENSOR_COMPRESS: requires at least 1 argument");
        TTTrain a   = buildTrain(args[0]);
        double eps  = (args.size() > 1) ? args[1].get<double>() : 0.01;
        auto mr     = (args.size() > 2) ? static_cast<std::size_t>(args[2].get<int>()) : 0u;
        TTTrain comp = TensorContractionEngine::recompress(a, eps, mr);
        auto recon  = comp.reconstruct();
        return json{
            {"data",             recon},
            {"shape",            comp.mode_sizes},
            {"compression_ratio",comp.compressionRatio()},
            {"max_rank",         comp.maxRank()},
            {"achieved_eps",     comp.achieved_eps}
        };
    }
};

// ============================================================================
// TENSOR_INFO
// ============================================================================

class TensorInfoFunction : public IFunction {
public:
    FunctionSignature signature() const override {
        return FunctionSignature{
            .name     = "TENSOR_INFO",
            .category = "tensor",
            .description = "Metadata about a TT-compressed tensor. "
                           "Argument: {data:[...], shape:[...]}. "
                           "Returns {order, shape, max_rank, total_params, "
                           "compression_ratio, achieved_eps, original_norm}.",
            .arguments = {
                ArgSpec{"a", ArgType::OBJECT, true, nullptr, "Tensor"}
            },
            .return_type     = ArgType::OBJECT,
            .is_deterministic = true,
            .cost = FunctionCost{.complexity = CostComplexity::LINEAR, .base_cost = 2.0}
        };
    }

    json execute(const std::vector<json>& args,
                 const FunctionContext& /*ctx*/) const override {
        if (args.empty())
            throw std::invalid_argument("TENSOR_INFO: requires 1 argument");
        TTTrain a = buildTrain(args[0]);
        return json{
            {"order",             a.order()},
            {"shape",             a.mode_sizes},
            {"max_rank",          a.maxRank()},
            {"total_params",      a.totalParams()},
            {"compression_ratio", a.compressionRatio()},
            {"achieved_eps",      a.achieved_eps},
            {"original_norm",     a.original_norm}
        };
    }
};

// ============================================================================
// TENSOR_CONTRACT
// ============================================================================

class TensorContractFunction : public IFunction {
public:
    FunctionSignature signature() const override {
        return FunctionSignature{
            .name     = "TENSOR_CONTRACT",
            .category = "tensor",
            .description =
                "Multi-mode tensor contraction: CONTRACT(a, b, modes_a, modes_b). "
                "Contracts modes listed in modes_a of a with modes in modes_b of b. "
                "Result is a TTTrain of order (a.order - |modes_a| + b.order - |modes_b|). "
                "Full contraction (all modes) returns a scalar wrapped in a 1-element train. "
                "Ref: Oseledets (2011), paper §AQL operators.",
            .arguments = {
                ArgSpec{"a",       ArgType::OBJECT, true,  nullptr, "Tensor {data, shape}"},
                ArgSpec{"b",       ArgType::OBJECT, true,  nullptr, "Tensor {data, shape}"},
                ArgSpec{"modes_a", ArgType::ARRAY,  true,  nullptr, "Modes of a to contract"},
                ArgSpec{"modes_b", ArgType::ARRAY,  true,  nullptr, "Modes of b to contract"}
            },
            .return_type      = ArgType::OBJECT,
            .is_deterministic = true,
            .cost = FunctionCost{
                .complexity        = CostComplexity::QUADRATIC,
                .base_cost         = 20.0,
                .per_element_cost  = 0.01,
                .can_use_index     = false,
                .is_parallelizable = false
            }
        };
    }

    json execute(const std::vector<json>& args,
                 const FunctionContext& /*ctx*/) const override {
        if (args.size() < 4)
            throw std::invalid_argument(
                "TENSOR_CONTRACT: requires 4 arguments (a, b, modes_a, modes_b)");

        TTTrain a = buildTrain(args[0]);
        TTTrain b = buildTrain(args[1]);

        std::vector<std::size_t> modes_a, modes_b;
        for (const auto& v : args[2]) modes_a.push_back(v.get<std::size_t>());
        for (const auto& v : args[3]) modes_b.push_back(v.get<std::size_t>());

        TTTrain result = TensorContractionEngine::contractModes(a, b, modes_a, modes_b);
        auto    dense  = result.reconstruct();

        // If order-1 with a single element, surface scalar for convenience.
        const bool is_scalar =
            result.order() == 1 && result.mode_sizes[0] == 1;

        return json{
            {"data",             dense},
            {"shape",            result.mode_sizes},
            {"is_scalar",        is_scalar},
            {"scalar",           is_scalar ? dense[0] : 0.0f},
            {"compression_ratio",result.compressionRatio()},
            {"max_rank",         result.maxRank()},
            {"achieved_eps",     result.achieved_eps}
        };
    }
};

// ============================================================================
// TENSOR_PROJECT
// ============================================================================

class TensorProjectFunction : public IFunction {
public:
    FunctionSignature signature() const override {
        return FunctionSignature{
            .name     = "TENSOR_PROJECT",
            .category = "tensor",
            .description =
                "Marginalize a tensor over one mode: PROJECT(t, mode). "
                "Sums over all indices along mode, returning a train of order (d-1). "
                "Operates entirely in the compressed domain (O(d*n*r^2)). "
                "Ref: tensor marginalization, paper §AQL operators.",
            .arguments = {
                ArgSpec{"t",    ArgType::OBJECT,  true, nullptr, "Tensor {data, shape}"},
                ArgSpec{"mode", ArgType::INTEGER, true, nullptr, "Mode to marginalize (0-indexed)"}
            },
            .return_type      = ArgType::OBJECT,
            .is_deterministic = true,
            .cost = FunctionCost{
                .complexity = CostComplexity::LINEAR,
                .base_cost  = 4.0
            }
        };
    }

    json execute(const std::vector<json>& args,
                 const FunctionContext& /*ctx*/) const override {
        if (args.size() < 2)
            throw std::invalid_argument(
                "TENSOR_PROJECT: requires 2 arguments (t, mode)");

        TTTrain t    = buildTrain(args[0]);
        auto    mode = static_cast<std::size_t>(args[1].get<int>());

        TTTrain result = TensorContractionEngine::project(t, mode);
        auto    recon  = result.reconstruct();
        return json{
            {"data",             recon},
            {"shape",            result.mode_sizes},
            {"compression_ratio",result.compressionRatio()},
            {"max_rank",         result.maxRank()},
            {"achieved_eps",     result.achieved_eps}
        };
    }
};

// ============================================================================
// TENSOR_DECOMPOSE
// ============================================================================

class TensorDecomposeFunction : public IFunction {
public:
    FunctionSignature signature() const override {
        return FunctionSignature{
            .name     = "TENSOR_DECOMPOSE",
            .category = "tensor",
            .description =
                "On-the-fly TT decomposition: DECOMPOSE(data, shape, max_rank, eps). "
                "Arguments: (data:Array<Float>, shape:Array<Int>, "
                "max_rank:Int=0, eps:Float=0.01). "
                "Returns {data, shape, compression_ratio, max_rank, achieved_eps, "
                "original_norm, total_params}. "
                "Ref: Oseledets TT-SVD (2011), paper §AQL DECOMPOSE operator.",
            .arguments = {
                ArgSpec{"data",     ArgType::ARRAY,   true,  nullptr,    "Flat tensor data"},
                ArgSpec{"shape",    ArgType::ARRAY,   true,  nullptr,    "Mode sizes"},
                ArgSpec{"max_rank", ArgType::INTEGER, false, json(0),    "Max TT-rank (0=auto)"},
                ArgSpec{"eps",      ArgType::NUMBER,  false, json(0.01), "Error tolerance"}
            },
            .return_type      = ArgType::OBJECT,
            .is_deterministic = true,
            .cost = FunctionCost{
                .complexity       = CostComplexity::LINEARITHMIC,
                .base_cost        = 15.0,
                .per_element_cost = 0.005,
                .is_parallelizable = false
            }
        };
    }

    json execute(const std::vector<json>& args,
                 const FunctionContext& /*ctx*/) const override {
        if (args.size() < 2)
            throw std::invalid_argument(
                "TENSOR_DECOMPOSE: requires at least 2 arguments (data, shape)");

        std::vector<float>       data;
        std::vector<std::size_t> shape;
        for (const auto& v : args[0]) data.push_back(v.get<float>());
        for (const auto& s : args[1]) shape.push_back(s.get<std::size_t>());

        auto max_rank = (args.size() > 2)
            ? static_cast<std::size_t>(args[2].get<int>()) : 0u;
        double eps = (args.size() > 3) ? args[3].get<double>() : 0.01;

        TensorTrainConfig cfg;
        cfg.eps      = eps;
        cfg.max_rank = max_rank;

        TensorTrainDecomposer dec;
        auto [train, stats] = dec.decompose(data, shape, cfg);
        auto recon = train.reconstruct();

        return json{
            {"data",             recon},
            {"shape",            train.mode_sizes},
            {"compression_ratio",stats.compression_ratio},
            {"max_rank",         stats.max_rank},
            {"achieved_eps",     stats.achieved_eps},
            {"original_norm",    train.original_norm},
            {"total_params",     stats.total_params}
        };
    }
};

// ============================================================================
// Registration
// ============================================================================

void registerTensorFunctions(FunctionRegistry& registry) {
    registry.registerFunction(std::make_unique<TensorSimilarityFunction>());
    registry.registerFunction(std::make_unique<TensorNormFunction>());
    registry.registerFunction(std::make_unique<TensorSliceFunction>());
    registry.registerFunction(std::make_unique<TensorCompressFunction>());
    registry.registerFunction(std::make_unique<TensorInfoFunction>());
    registry.registerFunction(std::make_unique<TensorContractFunction>());
    registry.registerFunction(std::make_unique<TensorProjectFunction>());
    registry.registerFunction(std::make_unique<TensorDecomposeFunction>());
}

} // namespace functions
} // namespace query
} // namespace themis

