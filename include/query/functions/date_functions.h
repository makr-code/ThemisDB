#pragma once

#include "query/functions/function_registry.h"
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace themis {
namespace query {
namespace functions {

// ============================================================================
// Date/Time Functions
// ============================================================================

/**
 * @brief DATE_NOW() - Current timestamp
 */
class DateNowFunction : public IFunction {
public:
    FunctionSignature signature() const override {
        return {
            .name = "DATE_NOW",
            .category = "Date",
            .description = "Returns the current Unix timestamp in milliseconds",
            .arguments = {},
            .return_type = ArgType::INTEGER,
            .is_deterministic = false,
            .examples = {R"(DATE_NOW() // 1700000000000)"}
        };
    }
    
    void validateArgs(const std::vector<nlohmann::json>&) const override {}
    
    nlohmann::json execute(const std::vector<nlohmann::json>&,
                           const FunctionContext&) const override {
        auto now = std::chrono::system_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()
        ).count();
        return ms;
    }
};

/**
 * @brief DATE_TIMESTAMP(dateStr) - Parse date string to timestamp
 */
class DateTimestampFunction : public IFunction {
public:
    FunctionSignature signature() const override {
        return {
            .name = "DATE_TIMESTAMP",
            .category = "Date",
            .description = "Converts an ISO 8601 date string to a Unix timestamp",
            .arguments = {{"dateStr", ArgType::STRING, true, nullptr, "ISO 8601 date string"}},
            .return_type = ArgType::INTEGER,
            .is_deterministic = true,
            .examples = {R"(DATE_TIMESTAMP("2024-01-15T10:30:00Z"))"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        std::string dateStr = args[0].get<std::string>();
        return parseISO8601(dateStr);
    }

private:
    static int64_t parseISO8601(const std::string& str) {
        std::tm tm = {};
        std::istringstream ss(str);
        ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
        if (ss.fail()) {
            // Try date-only format
            ss.clear();
            ss.str(str);
            ss >> std::get_time(&tm, "%Y-%m-%d");
            if (ss.fail()) {
                throw std::runtime_error("DATE_TIMESTAMP: invalid date format");
            }
        }
        
        auto tp = std::chrono::system_clock::from_time_t(timegm(&tm));
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            tp.time_since_epoch()
        ).count();
    }
};

/**
 * @brief DATE_ISO8601(timestamp) - Format timestamp as ISO 8601
 */
class DateIso8601Function : public IFunction {
public:
    FunctionSignature signature() const override {
        return {
            .name = "DATE_ISO8601",
            .category = "Date",
            .description = "Formats a Unix timestamp as an ISO 8601 date string",
            .arguments = {{"timestamp", ArgType::INTEGER, true, nullptr, "Unix timestamp in ms"}},
            .return_type = ArgType::STRING,
            .is_deterministic = true,
            .examples = {R"(DATE_ISO8601(1700000000000) // "2023-11-14T22:13:20Z")"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        int64_t ts = args[0].get<int64_t>();
        return formatISO8601(ts);
    }

private:
    static std::string formatISO8601(int64_t ms) {
        std::time_t seconds = static_cast<std::time_t>(ms / 1000);
        std::tm* tm = std::gmtime(&seconds);
        
        std::ostringstream ss;
        ss << std::put_time(tm, "%Y-%m-%dT%H:%M:%SZ");
        return ss.str();
    }
};

/**
 * @brief DATE_YEAR/MONTH/DAY/HOUR/MINUTE/SECOND - Extract components
 */
class DateYearFunction : public IFunction {
public:
    FunctionSignature signature() const override {
        return {
            .name = "DATE_YEAR",
            .category = "Date",
            .description = "Extracts the year from a timestamp",
            .arguments = {{"timestamp", ArgType::INTEGER, true, nullptr, "Unix timestamp in ms"}},
            .return_type = ArgType::INTEGER,
            .is_deterministic = true,
            .examples = {R"(DATE_YEAR(1700000000000) // 2023)"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        std::time_t seconds = static_cast<std::time_t>(args[0].get<int64_t>() / 1000);
        std::tm* tm = std::gmtime(&seconds);
        return static_cast<int64_t>(tm->tm_year + 1900);
    }
};

class DateMonthFunction : public IFunction {
public:
    FunctionSignature signature() const override {
        return {
            .name = "DATE_MONTH",
            .category = "Date",
            .description = "Extracts the month (1-12) from a timestamp",
            .arguments = {{"timestamp", ArgType::INTEGER, true, nullptr, "Unix timestamp in ms"}},
            .return_type = ArgType::INTEGER,
            .is_deterministic = true,
            .examples = {R"(DATE_MONTH(1700000000000) // 11)"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        std::time_t seconds = static_cast<std::time_t>(args[0].get<int64_t>() / 1000);
        std::tm* tm = std::gmtime(&seconds);
        return static_cast<int64_t>(tm->tm_mon + 1);
    }
};

class DateDayFunction : public IFunction {
public:
    FunctionSignature signature() const override {
        return {
            .name = "DATE_DAY",
            .category = "Date",
            .description = "Extracts the day of month (1-31) from a timestamp",
            .arguments = {{"timestamp", ArgType::INTEGER, true, nullptr, "Unix timestamp in ms"}},
            .return_type = ArgType::INTEGER,
            .is_deterministic = true,
            .examples = {R"(DATE_DAY(1700000000000) // 14)"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        std::time_t seconds = static_cast<std::time_t>(args[0].get<int64_t>() / 1000);
        std::tm* tm = std::gmtime(&seconds);
        return static_cast<int64_t>(tm->tm_mday);
    }
};

class DateHourFunction : public IFunction {
public:
    FunctionSignature signature() const override {
        return {
            .name = "DATE_HOUR",
            .category = "Date",
            .description = "Extracts the hour (0-23) from a timestamp",
            .arguments = {{"timestamp", ArgType::INTEGER, true, nullptr, "Unix timestamp in ms"}},
            .return_type = ArgType::INTEGER,
            .is_deterministic = true,
            .examples = {R"(DATE_HOUR(1700000000000) // 22)"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        std::time_t seconds = static_cast<std::time_t>(args[0].get<int64_t>() / 1000);
        std::tm* tm = std::gmtime(&seconds);
        return static_cast<int64_t>(tm->tm_hour);
    }
};

class DateMinuteFunction : public IFunction {
public:
    FunctionSignature signature() const override {
        return {
            .name = "DATE_MINUTE",
            .category = "Date",
            .description = "Extracts the minute (0-59) from a timestamp",
            .arguments = {{"timestamp", ArgType::INTEGER, true, nullptr, "Unix timestamp in ms"}},
            .return_type = ArgType::INTEGER,
            .is_deterministic = true,
            .examples = {R"(DATE_MINUTE(1700000000000) // 13)"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        std::time_t seconds = static_cast<std::time_t>(args[0].get<int64_t>() / 1000);
        std::tm* tm = std::gmtime(&seconds);
        return static_cast<int64_t>(tm->tm_min);
    }
};

class DateSecondFunction : public IFunction {
public:
    FunctionSignature signature() const override {
        return {
            .name = "DATE_SECOND",
            .category = "Date",
            .description = "Extracts the second (0-59) from a timestamp",
            .arguments = {{"timestamp", ArgType::INTEGER, true, nullptr, "Unix timestamp in ms"}},
            .return_type = ArgType::INTEGER,
            .is_deterministic = true,
            .examples = {R"(DATE_SECOND(1700000000000) // 20)"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        std::time_t seconds = static_cast<std::time_t>(args[0].get<int64_t>() / 1000);
        std::tm* tm = std::gmtime(&seconds);
        return static_cast<int64_t>(tm->tm_sec);
    }
};

class DateMillisecondFunction : public IFunction {
public:
    FunctionSignature signature() const override {
        return {
            .name = "DATE_MILLISECOND",
            .category = "Date",
            .description = "Extracts the millisecond (0-999) from a timestamp",
            .arguments = {{"timestamp", ArgType::INTEGER, true, nullptr, "Unix timestamp in ms"}},
            .return_type = ArgType::INTEGER,
            .is_deterministic = true,
            .examples = {R"(DATE_MILLISECOND(1700000000123) // 123)"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        return args[0].get<int64_t>() % 1000;
    }
};

/**
 * @brief DATE_DAYOFWEEK(timestamp) - Day of week (0=Sunday, 6=Saturday)
 */
class DateDayOfWeekFunction : public IFunction {
public:
    FunctionSignature signature() const override {
        return {
            .name = "DATE_DAYOFWEEK",
            .category = "Date",
            .description = "Returns the day of week (0=Sunday, 6=Saturday)",
            .arguments = {{"timestamp", ArgType::INTEGER, true, nullptr, "Unix timestamp in ms"}},
            .return_type = ArgType::INTEGER,
            .is_deterministic = true,
            .examples = {R"(DATE_DAYOFWEEK(1700000000000) // 2 (Tuesday))"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        std::time_t seconds = static_cast<std::time_t>(args[0].get<int64_t>() / 1000);
        std::tm* tm = std::gmtime(&seconds);
        return static_cast<int64_t>(tm->tm_wday);
    }
};

/**
 * @brief DATE_DAYOFYEAR(timestamp) - Day of year (1-366)
 */
class DateDayOfYearFunction : public IFunction {
public:
    FunctionSignature signature() const override {
        return {
            .name = "DATE_DAYOFYEAR",
            .category = "Date",
            .description = "Returns the day of year (1-366)",
            .arguments = {{"timestamp", ArgType::INTEGER, true, nullptr, "Unix timestamp in ms"}},
            .return_type = ArgType::INTEGER,
            .is_deterministic = true,
            .examples = {R"(DATE_DAYOFYEAR(1700000000000) // 318)"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        std::time_t seconds = static_cast<std::time_t>(args[0].get<int64_t>() / 1000);
        std::tm* tm = std::gmtime(&seconds);
        return static_cast<int64_t>(tm->tm_yday + 1);
    }
};

/**
 * @brief DATE_ADD(timestamp, amount, unit) - Add time
 */
class DateAddFunction : public IFunction {
public:
    FunctionSignature signature() const override {
        return {
            .name = "DATE_ADD",
            .category = "Date",
            .description = "Adds a time amount to a timestamp",
            .arguments = {
                {"timestamp", ArgType::INTEGER, true, nullptr, "Unix timestamp in ms"},
                {"amount", ArgType::INTEGER, true, nullptr, "Amount to add"},
                {"unit", ArgType::STRING, true, nullptr, "Unit: year, month, day, hour, minute, second, millisecond"}
            },
            .return_type = ArgType::INTEGER,
            .is_deterministic = true,
            .examples = {R"(DATE_ADD(1700000000000, 7, "day"))"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        int64_t ts = args[0].get<int64_t>();
        int64_t amount = args[1].get<int64_t>();
        std::string unit = args[2].get<std::string>();
        
        int64_t ms = 0;
        if (unit == "millisecond" || unit == "ms") {
            ms = amount;
        } else if (unit == "second" || unit == "s") {
            ms = amount * 1000;
        } else if (unit == "minute" || unit == "m") {
            ms = amount * 60 * 1000;
        } else if (unit == "hour" || unit == "h") {
            ms = amount * 60 * 60 * 1000;
        } else if (unit == "day" || unit == "d") {
            ms = amount * 24 * 60 * 60 * 1000;
        } else if (unit == "week" || unit == "w") {
            ms = amount * 7 * 24 * 60 * 60 * 1000;
        } else if (unit == "month" || unit == "M") {
            // Approximate: 30 days
            ms = amount * 30 * 24 * 60 * 60 * 1000;
        } else if (unit == "year" || unit == "y") {
            // Approximate: 365 days
            ms = amount * 365 * 24 * 60 * 60 * 1000;
        } else {
            throw std::runtime_error("DATE_ADD: unknown unit '" + unit + "'");
        }
        
        return ts + ms;
    }
};

/**
 * @brief DATE_SUBTRACT(timestamp, amount, unit) - Subtract time
 */
class DateSubtractFunction : public IFunction {
public:
    FunctionSignature signature() const override {
        return {
            .name = "DATE_SUBTRACT",
            .category = "Date",
            .description = "Subtracts a time amount from a timestamp",
            .arguments = {
                {"timestamp", ArgType::INTEGER, true, nullptr, "Unix timestamp in ms"},
                {"amount", ArgType::INTEGER, true, nullptr, "Amount to subtract"},
                {"unit", ArgType::STRING, true, nullptr, "Unit: year, month, day, hour, minute, second, millisecond"}
            },
            .return_type = ArgType::INTEGER,
            .is_deterministic = true,
            .examples = {R"(DATE_SUBTRACT(1700000000000, 1, "month"))"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        // Reuse DATE_ADD with negative amount
        std::vector<nlohmann::json> addArgs = {
            args[0],
            -args[1].get<int64_t>(),
            args[2]
        };
        DateAddFunction add;
        return add.execute(addArgs, FunctionContext());
    }
};

/**
 * @brief DATE_DIFF(ts1, ts2, unit) - Difference between dates
 */
class DateDiffFunction : public IFunction {
public:
    FunctionSignature signature() const override {
        return {
            .name = "DATE_DIFF",
            .category = "Date",
            .description = "Returns the difference between two timestamps",
            .arguments = {
                {"timestamp1", ArgType::INTEGER, true, nullptr, "First timestamp in ms"},
                {"timestamp2", ArgType::INTEGER, true, nullptr, "Second timestamp in ms"},
                {"unit", ArgType::STRING, true, nullptr, "Unit for result: day, hour, minute, second, millisecond"}
            },
            .return_type = ArgType::NUMBER,
            .is_deterministic = true,
            .examples = {R"(DATE_DIFF(ts2, ts1, "day") // days between)"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        int64_t ts1 = args[0].get<int64_t>();
        int64_t ts2 = args[1].get<int64_t>();
        std::string unit = args[2].get<std::string>();
        
        int64_t diffMs = ts1 - ts2;
        
        if (unit == "millisecond" || unit == "ms") {
            return diffMs;
        } else if (unit == "second" || unit == "s") {
            return static_cast<double>(diffMs) / 1000.0;
        } else if (unit == "minute" || unit == "m") {
            return static_cast<double>(diffMs) / (60.0 * 1000.0);
        } else if (unit == "hour" || unit == "h") {
            return static_cast<double>(diffMs) / (60.0 * 60.0 * 1000.0);
        } else if (unit == "day" || unit == "d") {
            return static_cast<double>(diffMs) / (24.0 * 60.0 * 60.0 * 1000.0);
        } else if (unit == "week" || unit == "w") {
            return static_cast<double>(diffMs) / (7.0 * 24.0 * 60.0 * 60.0 * 1000.0);
        } else {
            throw std::runtime_error("DATE_DIFF: unknown unit '" + unit + "'");
        }
    }
};

/**
 * @brief DATE_TRUNC(timestamp, unit) - Truncate to unit boundary
 */
class DateTruncFunction : public IFunction {
public:
    FunctionSignature signature() const override {
        return {
            .name = "DATE_TRUNC",
            .category = "Date",
            .description = "Truncates a timestamp to the specified unit boundary",
            .arguments = {
                {"timestamp", ArgType::INTEGER, true, nullptr, "Unix timestamp in ms"},
                {"unit", ArgType::STRING, true, nullptr, "Unit: year, month, day, hour, minute, second"}
            },
            .return_type = ArgType::INTEGER,
            .is_deterministic = true,
            .examples = {R"(DATE_TRUNC(1700000000000, "day") // Start of day)"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        int64_t ts = args[0].get<int64_t>();
        std::string unit = args[1].get<std::string>();
        
        std::time_t seconds = static_cast<std::time_t>(ts / 1000);
        std::tm* tm = std::gmtime(&seconds);
        
        if (unit == "year" || unit == "y") {
            tm->tm_mon = 0;
            tm->tm_mday = 1;
            tm->tm_hour = 0;
            tm->tm_min = 0;
            tm->tm_sec = 0;
        } else if (unit == "month" || unit == "M") {
            tm->tm_mday = 1;
            tm->tm_hour = 0;
            tm->tm_min = 0;
            tm->tm_sec = 0;
        } else if (unit == "day" || unit == "d") {
            tm->tm_hour = 0;
            tm->tm_min = 0;
            tm->tm_sec = 0;
        } else if (unit == "hour" || unit == "h") {
            tm->tm_min = 0;
            tm->tm_sec = 0;
        } else if (unit == "minute" || unit == "m") {
            tm->tm_sec = 0;
        } else if (unit == "second" || unit == "s") {
            // Already at second precision, just remove ms
            return (ts / 1000) * 1000;
        } else {
            throw std::runtime_error("DATE_TRUNC: unknown unit '" + unit + "'");
        }
        
        return static_cast<int64_t>(timegm(tm)) * 1000;
    }
};

/**
 * @brief DATE_FORMAT(timestamp, format) - Format date with pattern
 */
class DateFormatFunction : public IFunction {
public:
    FunctionSignature signature() const override {
        return {
            .name = "DATE_FORMAT",
            .category = "Date",
            .description = "Formats a timestamp using a format pattern",
            .arguments = {
                {"timestamp", ArgType::INTEGER, true, nullptr, "Unix timestamp in ms"},
                {"format", ArgType::STRING, true, nullptr, "Format pattern (strftime)"}
            },
            .return_type = ArgType::STRING,
            .is_deterministic = true,
            .examples = {R"(DATE_FORMAT(1700000000000, "%Y-%m-%d") // "2023-11-14")"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        int64_t ts = args[0].get<int64_t>();
        std::string format = args[1].get<std::string>();
        
        std::time_t seconds = static_cast<std::time_t>(ts / 1000);
        std::tm* tm = std::gmtime(&seconds);
        
        std::ostringstream ss;
        ss << std::put_time(tm, format.c_str());
        return ss.str();
    }
};

// ============================================================================
// Register Date Functions
// ============================================================================

inline void registerDateFunctions() {
    auto& reg = FunctionRegistry::instance();
    reg.registerFunction(std::make_unique<DateNowFunction>());
    reg.registerFunction(std::make_unique<DateTimestampFunction>());
    reg.registerFunction(std::make_unique<DateIso8601Function>());
    reg.registerFunction(std::make_unique<DateYearFunction>());
    reg.registerFunction(std::make_unique<DateMonthFunction>());
    reg.registerFunction(std::make_unique<DateDayFunction>());
    reg.registerFunction(std::make_unique<DateHourFunction>());
    reg.registerFunction(std::make_unique<DateMinuteFunction>());
    reg.registerFunction(std::make_unique<DateSecondFunction>());
    reg.registerFunction(std::make_unique<DateMillisecondFunction>());
    reg.registerFunction(std::make_unique<DateDayOfWeekFunction>());
    reg.registerFunction(std::make_unique<DateDayOfYearFunction>());
    reg.registerFunction(std::make_unique<DateAddFunction>());
    reg.registerFunction(std::make_unique<DateSubtractFunction>());
    reg.registerFunction(std::make_unique<DateDiffFunction>());
    reg.registerFunction(std::make_unique<DateTruncFunction>());
    reg.registerFunction(std::make_unique<DateFormatFunction>());
}

} // namespace functions
} // namespace query
} // namespace themis
