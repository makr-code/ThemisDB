/**
 * @file tensor_functions.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=11, M=8, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "query/functions/tensor_functions.h"
#include "query/functions/function_registry.h"
#include "query/tensor_contraction_engine.h"
#include "storage/tensor_train_decomposer.h"

#include <cmath>
#include <cctype>
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
    std::vector<float> out = {};

    out.reserve(arr.size());
    for (const auto& v : arr) {
      out.push_back(v.get<float>());
    }
    return out;
}

[[nodiscard]] static bool isAllDigits(const std::string& token) {
    if (token.empty()) {
      return false;
    }
    return std::all_of(token.begin(), token.end(), [](unsigned char c) {
        return std::isdigit(c);
    });
}

[[nodiscard]] static const json* resolvePathSegments(const json& root,
                                                     const std::string& path) {
    const json* current = &root;
    std::size_t pos = 0;
    while (static_cast<size_t>(pos) < path.size()) {
        const auto next = path.find('.', pos);
        const auto token = (next == std::string::npos)
            ? path.substr(pos)
            : path.substr(pos, next - pos);
        if (token.empty()) {
          return nullptr;
        }

        if (current->is_object()) {
            auto it = current->find(token);
            if (it == current->end()) {
              return nullptr;
            }
            current = &(*it);
        } else if (current->is_array() && isAllDigits(token)) {
            try {
                const auto idx = static_cast<std::size_t>(std::stoull(token));
                if (idx >= current->size()) {
                  return nullptr;
                }
                current = &(*current)[idx];
            } catch (...) {
                return nullptr;
            }
        } else {
            return nullptr;
        }

        if (next == std::string::npos) {
          break;
        }
        pos = next + 1;
    }

    return current;
}

[[nodiscard]] static json resolveTensorArg(const json& arg, const FunctionContext& ctx) {
    if (arg.is_object()) {
        if (!arg.contains("data") || !arg.contains("shape")) {
            throw std::invalid_argument(
                "Tensor argument object must contain 'data' and 'shape'");
        }
        return arg;
    }

    if (!arg.is_string()) {
        throw std::invalid_argument(
            "Tensor argument must be an object {data,shape} or a string field path");
    }

    std::string normalized_path = arg.get<std::string>();
    if (normalized_path.empty()) {
        throw std::invalid_argument("Tensor field path cannot be empty");
    }
    if (normalized_path.front() == '$') {
        normalized_path.erase(normalized_path.begin());
        if (normalized_path.empty()) {
            throw std::invalid_argument("Tensor field path cannot be empty");
        }
    }

    // 1) Variable lookup: "var" or "var.sub.path"
    const auto dot = normalized_path.find('.');
    const auto var_name = (dot == std::string::npos) ? normalized_path : normalized_path.substr(0, dot);
    auto variable = ctx.getVariable(var_name);
    if (!variable.is_null()) {
        const json* resolved = nullptr;
        if (dot == std::string::npos) {
            resolved = &variable;
        } else {
            resolved = resolvePathSegments(variable, normalized_path.substr(dot + 1));
        }
        if (resolved && resolved->is_object() &&
            resolved->contains("data") && resolved->contains("shape")) {
            return *resolved;
        }
    }

    // 2) Current-document lookup (supports dot-path and JSON pointer syntax)
    const auto& doc = ctx.currentDocument();
    if (!doc.is_null()) {
        if (!normalized_path.empty() && normalized_path.front() == '/') {
            try {
                const auto* ptr = &doc.at(nlohmann::json::json_pointer(normalized_path));
                if (ptr->is_object() &&
                    ptr->contains("data") && ptr->contains("shape")) {
                    return *ptr;
                }
            } catch (const nlohmann::json::exception&) {
                // Expected for invalid/missing pointer paths; unified error below.
            }
        } else {
            if (const auto* resolved = resolvePathSegments(doc, normalized_path)) {
                if (resolved->is_object() &&
                    resolved->contains("data") && resolved->contains("shape")) {
                    return *resolved;
                }
            }
        }
    }

    throw std::invalid_argument(
        "Tensor field path '" + normalized_path +
        "' could not be resolved to an object with 'data' and 'shape'");
}

static TTTrain buildTrain(const json& arg, const FunctionContext& ctx) {
    const auto tensor_arg = resolveTensorArg(arg, ctx);
    auto data_arr  = tensor_arg.at("data");
    auto shape_arr = tensor_arg.at("shape");

    std::vector<float> data = jsonToFloats(data_arr);
    std::vector<std::size_t> shape = {};

    for (const auto& s : shape_arr) {
      shape.push_back(s.get<std::size_t>());
    }

    double eps = tensor_arg.value("eps", 0.01);

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

/** @brief TENSOR_SIMILARITY. */
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
                ArgSpec{"a", ArgType::ANY, true,  nullptr, "First tensor {data, shape} or field path"},
                ArgSpec{"b", ArgType::ANY, true,  nullptr, "Second tensor {data, shape} or field path"}
            },
            .return_type     = ArgType::NUMBER,
            .is_deterministic = true,
            .is_aggregate    = false,
            .examples = {},
            .cost = FunctionCost{CostComplexity::LINEARITHMIC, 5.0, 0.001, false, false, ""}
        };
    }

    json execute(const std::vector<json>& args,
                 const FunctionContext& ctx) const override {
        if (static_cast<int>(args.size()) < 2)
            throw std::invalid_argument("TENSOR_SIMILARITY: requires 2 arguments");
        TTTrain a = buildTrain(args[0], ctx);
        TTTrain b = buildTrain(args[1], ctx);
        return json(TensorContractionEngine::cosineSimilarity(a, b));
    }
};

// ============================================================================
// TENSOR_NORM
// ============================================================================

/** @brief TENSOR_NORM. */
class TensorNormFunction : public IFunction {
public:
    FunctionSignature signature() const override {
        return FunctionSignature{
            .name     = "TENSOR_NORM",
            .category = "tensor",
            .description = "Frobenius norm of a tensor in TT-compressed domain. "
                           "Argument: {data:[...], shape:[...]}. Returns float ≥ 0.",
            .arguments = {
                ArgSpec{"a", ArgType::ANY, true, nullptr, "Tensor {data, shape} or field path"}
            },
            .return_type     = ArgType::NUMBER,
            .is_deterministic = true,
            .is_aggregate    = false,
            .examples = {},
            .cost = FunctionCost{CostComplexity::LINEARITHMIC, 3.0, 0.0, false, false, ""}
        };
    }

    json execute(const std::vector<json>& args,
                 const FunctionContext& ctx) const override {
        if (args.empty())
            throw std::invalid_argument("TENSOR_NORM: requires 1 argument");
        TTTrain a = buildTrain(args[0], ctx);
        return json(TensorContractionEngine::frobeniusNorm(a));
    }
};

// ============================================================================
// TENSOR_SLICE
// ============================================================================

/** @brief TENSOR_SLICE. */
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
                ArgSpec{"a",   ArgType::ANY,     true, nullptr, "Tensor {data, shape} or field path"},
                ArgSpec{"dim", ArgType::INTEGER, true, nullptr, "Mode dimension (0-indexed)"},
                ArgSpec{"idx", ArgType::INTEGER, true, nullptr, "Index to fix"}
            },
            .return_type     = ArgType::OBJECT,
            .is_deterministic = true,
            .examples = {},
            .cost = FunctionCost{CostComplexity::LINEAR, 2.0, 0.0, false, false, ""}
        };
    }

    json execute(const std::vector<json>& args,
                 const FunctionContext& ctx) const override {
        if (static_cast<int>(args.size()) < 3)
            throw std::invalid_argument("TENSOR_SLICE: requires 3 arguments (tensor, dim, idx)");
        TTTrain a   = buildTrain(args[0], ctx);
        // TC-16: guard against negative user-supplied dim/idx — negative int wraps
        // to a huge size_t and would cause out-of-bounds access inside slice().
        int dimI = args[1].get<int>();
        int idxI = args[2].get<int>();
        if (dimI < 0) {
          throw std::invalid_argument("TENSOR_SLICE: dim must be >= 0");
        }
        if (idxI < 0) {
          throw std::invalid_argument("TENSOR_SLICE: idx must be >= 0");
        }
        auto dim    = static_cast<std::size_t>(dimI);
        auto idx    = static_cast<std::size_t>(idxI);
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

/** @brief TENSOR_COMPRESS. */
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
                ArgSpec{"a",        ArgType::ANY,     true,  nullptr, "Tensor {data, shape} or field path"},
                ArgSpec{"eps",      ArgType::NUMBER,  false, json(0.01), "Error tolerance"},
                ArgSpec{"max_rank", ArgType::INTEGER, false, json(0), "Max TT-rank (0=unlimited)"}
            },
            .return_type     = ArgType::OBJECT,
            .is_deterministic = true,
            .examples = {},
            .cost = FunctionCost{CostComplexity::LINEARITHMIC, 10.0, 0.0, false, false, ""}
        };
    }

    json execute(const std::vector<json>& args,
                 const FunctionContext& ctx) const override {
        if (args.empty())
            throw std::invalid_argument("TENSOR_COMPRESS: requires at least 1 argument");
        TTTrain a   = buildTrain(args[0], ctx);
        double eps  = (args.size() > 1) ? args[1].get<double>() : 0.01;
        // TC-18: guard against negative max_rank — wraps to huge size_t.
        std::size_t mr = 0u;
        if (static_cast<int>(args.size()) > 2) {
            int mrI = args[2].get<int>();
            if (mrI < 0) {
              throw std::invalid_argument("TENSOR_COMPRESS: max_rank must be >= 0");
            }
            mr = static_cast<std::size_t>(mrI);
        }
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

/** @brief TENSOR_INFO. */
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
                ArgSpec{"a", ArgType::ANY, true, nullptr, "Tensor {data, shape} or field path"}
            },
            .return_type     = ArgType::OBJECT,
            .is_deterministic = true,
            .examples = {},
            .cost = FunctionCost{CostComplexity::LINEAR, 2.0, 0.0, false, false, ""}
        };
    }

    json execute(const std::vector<json>& args,
                 const FunctionContext& ctx) const override {
        if (args.empty())
            throw std::invalid_argument("TENSOR_INFO: requires 1 argument");
        TTTrain a = buildTrain(args[0], ctx);
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

/** @brief TENSOR_CONTRACT. */
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
                ArgSpec{"a",       ArgType::ANY, true,  nullptr, "Tensor {data, shape} or field path"},
                ArgSpec{"b",       ArgType::ANY, true,  nullptr, "Tensor {data, shape} or field path"},
                ArgSpec{"modes_a", ArgType::ARRAY,  true,  nullptr, "Modes of a to contract"},
                ArgSpec{"modes_b", ArgType::ARRAY,  true,  nullptr, "Modes of b to contract"}
            },
            .return_type      = ArgType::OBJECT,
            .is_deterministic = true,
            .examples = {},
            .cost = FunctionCost{CostComplexity::QUADRATIC, 20.0, 0.01, false, false, ""}
        };
    }

    json execute(const std::vector<json>& args,
                 const FunctionContext& ctx) const override {
        if (static_cast<int>(args.size()) < 4)
            throw std::invalid_argument(
                "TENSOR_CONTRACT: requires 4 arguments (a, b, modes_a, modes_b)");

        TTTrain a = buildTrain(args[0], ctx);
        TTTrain b = buildTrain(args[1], ctx);

        std::vector<std::size_t> modes_a, modes_b;
        for (const auto& v : args[2]) {
          modes_a.push_back(v.get<std::size_t>());
        }
        for (const auto& v : args[3]) {
          modes_b.push_back(v.get<std::size_t>());
        }

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

/** @brief TENSOR_PROJECT. */
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
                ArgSpec{"t",    ArgType::ANY,     true, nullptr, "Tensor {data, shape} or field path"},
                ArgSpec{"mode", ArgType::INTEGER, true, nullptr, "Mode to marginalize (0-indexed)"}
            },
            .return_type      = ArgType::OBJECT,
            .is_deterministic = true,
            .examples = {},
            .cost = FunctionCost{CostComplexity::LINEAR, 4.0, 0.0, false, false, ""}
        };
    }

    json execute(const std::vector<json>& args,
                 const FunctionContext& ctx) const override {
        if (static_cast<int>(args.size()) < 2)
            throw std::invalid_argument(
                "TENSOR_PROJECT: requires 2 arguments (t, mode)");

        TTTrain t    = buildTrain(args[0], ctx);
        // TC-17: guard against negative mode — wraps to huge size_t.
        int modeI = args[1].get<int>();
        if (modeI < 0) {
          throw std::invalid_argument("TENSOR_PROJECT: mode must be >= 0");
        }
        auto    mode = static_cast<std::size_t>(modeI);

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

/** @brief TENSOR_DECOMPOSE. */
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
            .examples = {},
            .cost = FunctionCost{CostComplexity::LINEARITHMIC, 15.0, 0.005, false, false, ""}
        };
    }

    json execute(const std::vector<json>& args,
                 const FunctionContext& /*ctx*/) const override {
        if (static_cast<int>(args.size()) < 2)
            throw std::invalid_argument(
                "TENSOR_DECOMPOSE: requires at least 2 arguments (data, shape)");

        std::vector<float>       data;
        std::vector<std::size_t> shape = {};

        for (const auto& v : args[0]) {
          data.push_back(v.get<float>());
        }
        for (const auto& s : args[1]) {
          shape.push_back(s.get<std::size_t>());
        }

        auto max_rank = [&]() -> std::size_t {
            if (static_cast<int>(args.size()) > 2) {
                // TC-19: guard against negative max_rank — wraps to huge size_t.
                int mrI = args[2].get<int>();
                if (mrI < 0) {
                  throw std::invalid_argument("TENSOR_DECOMPOSE: max_rank must be >= 0");
                }
                return static_cast<std::size_t>(mrI);
            }
            return 0u;
        }();
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


