/**
 * @file math_functions.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "query/functions/function_registry.h"
#include <cstdint>
#include <cmath>
#include <random>
#include <numeric>
#include <limits>

// Define M_PI if not available
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace themis {
namespace query {
namespace functions {

namespace {
inline int clampRoundPrecision(int64_t rawPrecision) {
    constexpr int64_t kMinPrecision = -308;
    constexpr int64_t kMaxPrecision = 308;
    return static_cast<int>(std::clamp(rawPrecision, kMinPrecision, kMaxPrecision));
}
} // namespace

// ============================================================================
// Math Functions
// ============================================================================

/**
 * @brief ABS(num) - Absolute value
 */
class AbsFunction : public IFunction {
public:
    ~AbsFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "ABS",
            .category = "Math",
            .description = "Returns the absolute value of a number",
            .arguments = {{"num", ArgType::NUMBER, true, nullptr, "Number"}},
            .return_type = ArgType::NUMBER,
            .is_deterministic = true,
            .examples = {R"(ABS(-5) // 5)"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        return std::abs(toNumber(args[0]));
    }
};

/**
 * @brief CEIL(num) - Ceiling
 */
class CeilFunction : public IFunction {
public:
    ~CeilFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "CEIL",
            .category = "Math",
            .description = "Returns the smallest integer greater than or equal to a number",
            .arguments = {{"num", ArgType::NUMBER, true, nullptr, "Number"}},
            .return_type = ArgType::INTEGER,
            .is_deterministic = true,
            .examples = {R"(CEIL(4.3) // 5)"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        return static_cast<int64_t>(std::ceil(toNumber(args[0])));
    }
};

/**
 * @brief FLOOR(num) - Floor
 */
class FloorFunction : public IFunction {
public:
    ~FloorFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "FLOOR",
            .category = "Math",
            .description = "Returns the largest integer less than or equal to a number",
            .arguments = {{"num", ArgType::NUMBER, true, nullptr, "Number"}},
            .return_type = ArgType::INTEGER,
            .is_deterministic = true,
            .examples = {R"(FLOOR(4.7) // 4)"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        return static_cast<int64_t>(std::floor(toNumber(args[0])));
    }
};

/**
 * @brief ROUND(num [, precision]) - Round to nearest
 */
class RoundFunction : public IFunction {
public:
    ~RoundFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "ROUND",
            .category = "Math",
            .description = "Rounds a number to a specified precision",
            .arguments = {
                {"num", ArgType::NUMBER, true, nullptr, "Number to round"},
                {"precision", ArgType::INTEGER, false, 0, "Decimal places (default: 0)"}
            },
            .return_type = ArgType::NUMBER,
            .is_deterministic = true,
            .examples = {
                R"(ROUND(4.567) // 5)",
                R"(ROUND(4.567, 2) // 4.57)"
            }
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        double num = toNumber(args[0]);
        int precision = args.size() > 1 ? clampRoundPrecision(args[1].get<int64_t>()) : 0;
        
        if (precision == 0) {
            return std::round(num);
        }
        
        double factor = std::pow(10.0, precision);
        return std::round(num * factor) / factor;
    }
};

/**
 * @brief SQRT(num) - Square root
 */
class SqrtFunction : public IFunction {
public:
    ~SqrtFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "SQRT",
            .category = "Math",
            .description = "Returns the square root of a number",
            .arguments = {{"num", ArgType::NUMBER, true, nullptr, "Non-negative number"}},
            .return_type = ArgType::NUMBER,
            .is_deterministic = true,
            .examples = {R"(SQRT(16) // 4)"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        double num = toNumber(args[0]);
        if (num < 0) throw std::runtime_error("SQRT: negative number");
        return std::sqrt(num);
    }
};

/**
 * @brief POW(base, exponent) - Power
 */
class PowFunction : public IFunction {
public:
    ~PowFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "POW",
            .category = "Math",
            .description = "Returns base raised to the power of exponent",
            .arguments = {
                {"base", ArgType::NUMBER, true, nullptr, "Base number"},
                {"exponent", ArgType::NUMBER, true, nullptr, "Exponent"}
            },
            .return_type = ArgType::NUMBER,
            .is_deterministic = true,
            .examples = {R"(POW(2, 3) // 8)"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        return std::pow(toNumber(args[0]), toNumber(args[1]));
    }
};

/**
 * @brief LOG(num [, base]) - Logarithm
 */
class LogFunction : public IFunction {
public:
    ~LogFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "LOG",
            .category = "Math",
            .description = "Returns the logarithm of a number (natural log by default)",
            .arguments = {
                {"num", ArgType::NUMBER, true, nullptr, "Positive number"},
                {"base", ArgType::NUMBER, false, nullptr, "Base (default: e)"}
            },
            .return_type = ArgType::NUMBER,
            .is_deterministic = true,
            .examples = {
                R"(LOG(10) // 2.302...)",
                R"(LOG(100, 10) // 2)"
            }
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        double num = toNumber(args[0]);
        if (num <= 0) throw std::runtime_error("LOG: non-positive number");
        
        if (args.size() > 1) {
            double base = toNumber(args[1]);
            if (base <= 0 || base == 1) throw std::runtime_error("LOG: invalid base");
            return std::log(num) / std::log(base);
        }
        return std::log(num);
    }
};

/**
 * @brief LOG10(num) - Base-10 logarithm
 */
class Log10Function : public IFunction {
public:
    ~Log10Function() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "LOG10",
            .category = "Math",
            .description = "Returns the base-10 logarithm of a number",
            .arguments = {{"num", ArgType::NUMBER, true, nullptr, "Positive number"}},
            .return_type = ArgType::NUMBER,
            .is_deterministic = true,
            .examples = {R"(LOG10(100) // 2)"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        double num = toNumber(args[0]);
        if (num <= 0) throw std::runtime_error("LOG10: non-positive number");
        return std::log10(num);
    }
};

/**
 * @brief EXP(num) - e raised to power
 */
class ExpFunction : public IFunction {
public:
    ~ExpFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "EXP",
            .category = "Math",
            .description = "Returns e raised to the power of a number",
            .arguments = {{"num", ArgType::NUMBER, true, nullptr, "Exponent"}},
            .return_type = ArgType::NUMBER,
            .is_deterministic = true,
            .examples = {R"(EXP(1) // 2.718...)"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        return std::exp(toNumber(args[0]));
    }
};

/**
 * @brief SIN/COS/TAN - Trigonometric functions
 */
class SinFunction : public IFunction {
public:
    ~SinFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "SIN",
            .category = "Math",
            .description = "Returns the sine of an angle (in radians)",
            .arguments = {{"angle", ArgType::NUMBER, true, nullptr, "Angle in radians"}},
            .return_type = ArgType::NUMBER,
            .is_deterministic = true,
            .examples = {R"(SIN(0) // 0)"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        return std::sin(toNumber(args[0]));
    }
};

class CosFunction : public IFunction {
public:
    ~CosFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "COS",
            .category = "Math",
            .description = "Returns the cosine of an angle (in radians)",
            .arguments = {{"angle", ArgType::NUMBER, true, nullptr, "Angle in radians"}},
            .return_type = ArgType::NUMBER,
            .is_deterministic = true,
            .examples = {R"(COS(0) // 1)"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        return std::cos(toNumber(args[0]));
    }
};

class TanFunction : public IFunction {
public:
    ~TanFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "TAN",
            .category = "Math",
            .description = "Returns the tangent of an angle (in radians)",
            .arguments = {{"angle", ArgType::NUMBER, true, nullptr, "Angle in radians"}},
            .return_type = ArgType::NUMBER,
            .is_deterministic = true,
            .examples = {R"(TAN(0) // 0)"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        return std::tan(toNumber(args[0]));
    }
};

/**
 * @brief ASIN/ACOS/ATAN - Inverse trigonometric functions
 */
class AsinFunction : public IFunction {
public:
    ~AsinFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "ASIN",
            .category = "Math",
            .description = "Returns the arcsine of a number (result in radians)",
            .arguments = {{"num", ArgType::NUMBER, true, nullptr, "Number between -1 and 1"}},
            .return_type = ArgType::NUMBER,
            .is_deterministic = true,
            .examples = {R"(ASIN(0) // 0)"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        double num = toNumber(args[0]);
        if (num < -1 || num > 1) throw std::runtime_error("ASIN: value out of range [-1, 1]");
        return std::asin(num);
    }
};

class AcosFunction : public IFunction {
public:
    ~AcosFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "ACOS",
            .category = "Math",
            .description = "Returns the arccosine of a number (result in radians)",
            .arguments = {{"num", ArgType::NUMBER, true, nullptr, "Number between -1 and 1"}},
            .return_type = ArgType::NUMBER,
            .is_deterministic = true,
            .examples = {R"(ACOS(1) // 0)"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        double num = toNumber(args[0]);
        if (num < -1 || num > 1) throw std::runtime_error("ACOS: value out of range [-1, 1]");
        return std::acos(num);
    }
};

class AtanFunction : public IFunction {
public:
    ~AtanFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "ATAN",
            .category = "Math",
            .description = "Returns the arctangent of a number (result in radians)",
            .arguments = {{"num", ArgType::NUMBER, true, nullptr, "Number"}},
            .return_type = ArgType::NUMBER,
            .is_deterministic = true,
            .examples = {R"(ATAN(0) // 0)"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        return std::atan(toNumber(args[0]));
    }
};

class Atan2Function : public IFunction {
public:
    ~Atan2Function() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "ATAN2",
            .category = "Math",
            .description = "Returns the arctangent of y/x (result in radians)",
            .arguments = {
                {"y", ArgType::NUMBER, true, nullptr, "Y coordinate"},
                {"x", ArgType::NUMBER, true, nullptr, "X coordinate"}
            },
            .return_type = ArgType::NUMBER,
            .is_deterministic = true,
            .examples = {R"(ATAN2(1, 1) // 0.785...)"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        return std::atan2(toNumber(args[0]), toNumber(args[1]));
    }
};

/**
 * @brief DEGREES/RADIANS - Angle conversion
 */
class DegreesFunction : public IFunction {
public:
    ~DegreesFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "DEGREES",
            .category = "Math",
            .description = "Converts radians to degrees",
            .arguments = {{"radians", ArgType::NUMBER, true, nullptr, "Angle in radians"}},
            .return_type = ArgType::NUMBER,
            .is_deterministic = true,
            .examples = {R"(DEGREES(3.14159) // 180)"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        return toNumber(args[0]) * 180.0 / M_PI;
    }
};

class RadiansFunction : public IFunction {
public:
    ~RadiansFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "RADIANS",
            .category = "Math",
            .description = "Converts degrees to radians",
            .arguments = {{"degrees", ArgType::NUMBER, true, nullptr, "Angle in degrees"}},
            .return_type = ArgType::NUMBER,
            .is_deterministic = true,
            .examples = {R"(RADIANS(180) // 3.14159...)"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        return toNumber(args[0]) * M_PI / 180.0;
    }
};

/**
 * @brief PI() - Pi constant
 */
class PiFunction : public IFunction {
public:
    ~PiFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "PI",
            .category = "Math",
            .description = "Returns the value of Pi",
            .arguments = {},
            .return_type = ArgType::NUMBER,
            .is_deterministic = true,
            .examples = {R"(PI() // 3.14159...)"}
        };
    }
    
    void validateArgs(const std::vector<nlohmann::json>&) const override {}
    
    nlohmann::json execute(const std::vector<nlohmann::json>&,
                           const FunctionContext&) const override {
        return M_PI;
    }
};

/**
 * @brief RANDOM() - Random number
 */
class RandomFunction : public IFunction {
public:
    ~RandomFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "RANDOM",
            .category = "Math",
            .description = "Returns a random number between 0 and 1",
            .arguments = {},
            .return_type = ArgType::NUMBER,
            .is_deterministic = false,
            .examples = {R"(RANDOM() // 0.7234...)"}
        };
    }
    
    void validateArgs(const std::vector<nlohmann::json>&) const override {}
    
    nlohmann::json execute(const std::vector<nlohmann::json>&,
                           const FunctionContext&) const override {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_real_distribution<> dis(0.0, 1.0);
        return dis(gen);
    }
};

/**
 * @brief RAND_INT(min, max) - Random integer
 */
class RandIntFunction : public IFunction {
public:
    ~RandIntFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "RAND_INT",
            .category = "Math",
            .description = "Returns a random integer between min and max (inclusive)",
            .arguments = {
                {"min", ArgType::INTEGER, true, nullptr, "Minimum value"},
                {"max", ArgType::INTEGER, true, nullptr, "Maximum value"}
            },
            .return_type = ArgType::INTEGER,
            .is_deterministic = false,
            .examples = {R"(RAND_INT(1, 100) // 42)"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        int64_t min = args[0].get<int64_t>();
        int64_t max = args[1].get<int64_t>();
        
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_int_distribution<int64_t> dis(min, max);
        return dis(gen);
    }
};

/**
 * @brief MIN/MAX for variadic arguments
 */
class MinFunction : public IFunction {
public:
    ~MinFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "MIN",
            .category = "Math",
            .description = "Returns the minimum value from arguments or array",
            .arguments = {{"values", ArgType::ANY, true, nullptr, "Numbers or array"}},
            .return_type = ArgType::NUMBER,
            .is_deterministic = true,
            .examples = {
                R"(MIN(1, 2, 3) // 1)",
                R"(MIN([5, 2, 8]) // 2)"
            }
        };
    }
    
    void validateArgs(const std::vector<nlohmann::json>&) const override {}
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        if (args.empty()) throw std::runtime_error("MIN requires at least 1 argument");
        
        std::vector<double> values;
        for (const auto& arg : args) {
            if (arg.is_array()) {
                for (const auto& elem : arg) {
                    values.push_back(toNumber(elem));
                }
            } else {
                values.push_back(toNumber(arg));
            }
        }
        
        if (values.empty()) throw std::runtime_error("MIN: no values provided");
        return *std::min_element(values.begin(), values.end());
    }
};

class MaxFunction : public IFunction {
public:
    ~MaxFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "MAX",
            .category = "Math",
            .description = "Returns the maximum value from arguments or array",
            .arguments = {{"values", ArgType::ANY, true, nullptr, "Numbers or array"}},
            .return_type = ArgType::NUMBER,
            .is_deterministic = true,
            .examples = {
                R"(MAX(1, 2, 3) // 3)",
                R"(MAX([5, 2, 8]) // 8)"
            }
        };
    }
    
    void validateArgs(const std::vector<nlohmann::json>&) const override {}
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        if (args.empty()) throw std::runtime_error("MAX requires at least 1 argument");
        
        std::vector<double> values;
        for (const auto& arg : args) {
            if (arg.is_array()) {
                for (const auto& elem : arg) {
                    values.push_back(toNumber(elem));
                }
            } else {
                values.push_back(toNumber(arg));
            }
        }
        
        if (values.empty()) throw std::runtime_error("MAX: no values provided");
        return *std::max_element(values.begin(), values.end());
    }
};

/**
 * @brief SUM(array) - Sum of array elements
 */
class SumFunction : public IFunction {
public:
    ~SumFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "SUM",
            .category = "Math",
            .description = "Returns the sum of all values in an array",
            .arguments = {{"array", ArgType::ARRAY, true, nullptr, "Array of numbers"}},
            .return_type = ArgType::NUMBER,
            .is_deterministic = true,
            .is_aggregate = true,
            .examples = {R"(SUM([1, 2, 3, 4]) // 10)"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        double sum = 0;
        for (const auto& elem : args[0]) {
            sum += toNumber(elem);
        }
        return sum;
    }
};

/**
 * @brief AVG(array) - Average of array elements
 */
class AvgFunction : public IFunction {
public:
    ~AvgFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "AVG",
            .category = "Math",
            .description = "Returns the average of all values in an array",
            .arguments = {{"array", ArgType::ARRAY, true, nullptr, "Array of numbers"}},
            .return_type = ArgType::NUMBER,
            .is_deterministic = true,
            .is_aggregate = true,
            .examples = {R"(AVG([1, 2, 3, 4]) // 2.5)"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        const auto& arr = args[0];
        if (arr.empty()) return 0.0;
        
        double sum = 0;
        for (const auto& elem : arr) {
            sum += toNumber(elem);
        }
        return sum / arr.size();
    }
};

// ============================================================================
// Register Math Functions
// ============================================================================

inline void registerMathFunctions(FunctionRegistry& reg) {
    reg.registerFunction(std::make_unique<AbsFunction>());
    reg.registerFunction(std::make_unique<CeilFunction>());
    reg.registerFunction(std::make_unique<FloorFunction>());
    reg.registerFunction(std::make_unique<RoundFunction>());
    reg.registerFunction(std::make_unique<SqrtFunction>());
    reg.registerFunction(std::make_unique<PowFunction>());
    reg.registerFunction(std::make_unique<LogFunction>());
    reg.registerFunction(std::make_unique<Log10Function>());
    reg.registerFunction(std::make_unique<ExpFunction>());
    reg.registerFunction(std::make_unique<SinFunction>());
    reg.registerFunction(std::make_unique<CosFunction>());
    reg.registerFunction(std::make_unique<TanFunction>());
    reg.registerFunction(std::make_unique<AsinFunction>());
    reg.registerFunction(std::make_unique<AcosFunction>());
    reg.registerFunction(std::make_unique<AtanFunction>());
    reg.registerFunction(std::make_unique<Atan2Function>());
    reg.registerFunction(std::make_unique<DegreesFunction>());
    reg.registerFunction(std::make_unique<RadiansFunction>());
    reg.registerFunction(std::make_unique<PiFunction>());
    reg.registerFunction(std::make_unique<RandomFunction>());
    reg.registerFunction(std::make_unique<RandIntFunction>());
    reg.registerFunction(std::make_unique<MinFunction>());
    reg.registerFunction(std::make_unique<MaxFunction>());
    reg.registerFunction(std::make_unique<SumFunction>());
    reg.registerFunction(std::make_unique<AvgFunction>());
}

} // namespace functions
} // namespace query
} // namespace themis
