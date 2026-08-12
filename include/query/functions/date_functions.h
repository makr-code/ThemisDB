/**
 * @file date_functions.h
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
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <set>

namespace themis {
namespace query {
namespace functions {

// Portable timegm implementation for Windows compatibility
#ifdef _WIN32
inline time_t portable_timegm(struct tm* tm) {
    return _mkgmtime(tm);
}
#else
inline time_t portable_timegm(struct tm* tm) {
    return ::timegm(tm);
}
#endif

// ============================================================================
// Date/Time Functions
// ============================================================================

/**
 * @brief DATE_NOW() - Current timestamp
 */
class DateNowFunction : public IFunction {
public:
    ~DateNowFunction() override = default;
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

// ============================================================================
// SQL-Compatible Helper Functions
// ============================================================================

/**
 * @brief NOW() - SQL-compatible alias for DATE_NOW()
 * 
 * Standard SQL function returning current timestamp.
 */
class NowFunction : public IFunction {
public:
    ~NowFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "NOW",
            .category = "Date",
            .description = "SQL-compatible: Returns current Unix timestamp in milliseconds",
            .arguments = {},
            .return_type = ArgType::INTEGER,
            .is_deterministic = false,
            .examples = {
                R"(NOW() // 1700000000000)",
                R"(NOW() - DAYS(7) // One week ago)"
            }
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
 * @brief CURRENT_TIMESTAMP() - SQL-compatible current timestamp
 */
class CurrentTimestampFunction : public IFunction {
public:
    ~CurrentTimestampFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "CURRENT_TIMESTAMP",
            .category = "Date",
            .description = "SQL-compatible: Returns current Unix timestamp in milliseconds",
            .arguments = {},
            .return_type = ArgType::INTEGER,
            .is_deterministic = false,
            .examples = {R"(CURRENT_TIMESTAMP() // 1700000000000)"}
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
 * @brief CURRENT_DATE() - SQL-compatible current date (without time)
 * 
 * Returns the current date as timestamp at 00:00:00 UTC.
 */
class CurrentDateFunction : public IFunction {
public:
    ~CurrentDateFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "CURRENT_DATE",
            .category = "Date",
            .description = "SQL-compatible: Returns current date as timestamp at 00:00:00 UTC",
            .arguments = {},
            .return_type = ArgType::INTEGER,
            .is_deterministic = false,
            .examples = {
                R"(CURRENT_DATE() // Start of today)",
                R"(CURRENT_DATE() + DAYS(1) // Start of tomorrow)"
            }
        };
    }
    
    void validateArgs(const std::vector<nlohmann::json>&) const override {}
    
    nlohmann::json execute(const std::vector<nlohmann::json>&,
                           const FunctionContext&) const override {
        auto now = std::chrono::system_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()
        ).count();
        
        // Truncate to day
        int64_t dayMs = 24 * 60 * 60 * 1000;
        return (ms / dayMs) * dayMs;
    }
};

/**
 * @brief CURRENT_TIME() - SQL-compatible current time
 * 
 * Returns the current time as milliseconds since midnight UTC.
 */
class CurrentTimeFunction : public IFunction {
public:
    ~CurrentTimeFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "CURRENT_TIME",
            .category = "Date",
            .description = "SQL-compatible: Returns current time as milliseconds since midnight UTC",
            .arguments = {},
            .return_type = ArgType::INTEGER,
            .is_deterministic = false,
            .examples = {
                R"(CURRENT_TIME() // e.g., 52200000 for 14:30:00)",
                R"(CURRENT_TIME() / 3600000 // Current hour)"
            }
        };
    }
    
    void validateArgs(const std::vector<nlohmann::json>&) const override {}
    
    nlohmann::json execute(const std::vector<nlohmann::json>&,
                           const FunctionContext&) const override {
        auto now = std::chrono::system_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()
        ).count();
        
        // Time since midnight
        int64_t dayMs = 24 * 60 * 60 * 1000;
        return ms % dayMs;
    }
};

/**
 * @brief TODAY() - Returns start of today
 * 
 * Convenient alias for CURRENT_DATE().
 */
class TodayFunction : public IFunction {
public:
    ~TodayFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "TODAY",
            .category = "Date",
            .description = "Returns start of today (00:00:00 UTC) as timestamp",
            .arguments = {},
            .return_type = ArgType::INTEGER,
            .is_deterministic = false,
            .examples = {
                R"(TODAY() // Start of today)",
                R"(event.date >= TODAY() // Events today or later)"
            }
        };
    }
    
    void validateArgs(const std::vector<nlohmann::json>&) const override {}
    
    nlohmann::json execute(const std::vector<nlohmann::json>&,
                           const FunctionContext&) const override {
        auto now = std::chrono::system_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()
        ).count();
        
        int64_t dayMs = 24 * 60 * 60 * 1000;
        return (ms / dayMs) * dayMs;
    }
};

/**
 * @brief YESTERDAY() - Returns start of yesterday
 */
class YesterdayFunction : public IFunction {
public:
    ~YesterdayFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "YESTERDAY",
            .category = "Date",
            .description = "Returns start of yesterday (00:00:00 UTC) as timestamp",
            .arguments = {},
            .return_type = ArgType::INTEGER,
            .is_deterministic = false,
            .examples = {
                R"(YESTERDAY() // Start of yesterday)",
                R"(event.date >= YESTERDAY() AND event.date < TODAY())"
            }
        };
    }
    
    void validateArgs(const std::vector<nlohmann::json>&) const override {}
    
    nlohmann::json execute(const std::vector<nlohmann::json>&,
                           const FunctionContext&) const override {
        auto now = std::chrono::system_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()
        ).count();
        
        int64_t dayMs = 24 * 60 * 60 * 1000;
        return ((ms / dayMs) - 1) * dayMs;
    }
};

/**
 * @brief TOMORROW() - Returns start of tomorrow
 */
class TomorrowFunction : public IFunction {
public:
    ~TomorrowFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "TOMORROW",
            .category = "Date",
            .description = "Returns start of tomorrow (00:00:00 UTC) as timestamp",
            .arguments = {},
            .return_type = ArgType::INTEGER,
            .is_deterministic = false,
            .examples = {
                R"(TOMORROW() // Start of tomorrow)",
                R"(deadline < TOMORROW() // Due today)"
            }
        };
    }
    
    void validateArgs(const std::vector<nlohmann::json>&) const override {}
    
    nlohmann::json execute(const std::vector<nlohmann::json>&,
                           const FunctionContext&) const override {
        auto now = std::chrono::system_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()
        ).count();
        
        int64_t dayMs = 24 * 60 * 60 * 1000;
        return ((ms / dayMs) + 1) * dayMs;
    }
};

/**
 * @brief GETDATE() - SQL Server compatible current timestamp
 */
class GetDateFunction : public IFunction {
public:
    ~GetDateFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "GETDATE",
            .category = "Date",
            .description = "SQL Server compatible: Returns current Unix timestamp in milliseconds",
            .arguments = {},
            .return_type = ArgType::INTEGER,
            .is_deterministic = false,
            .examples = {R"(GETDATE() // 1700000000000)"}
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
 * @brief SYSDATE() - Oracle compatible current timestamp
 */
class SysdateFunction : public IFunction {
public:
    ~SysdateFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "SYSDATE",
            .category = "Date",
            .description = "Oracle compatible: Returns current Unix timestamp in milliseconds",
            .arguments = {},
            .return_type = ArgType::INTEGER,
            .is_deterministic = false,
            .examples = {R"(SYSDATE() // 1700000000000)"}
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
 * @brief UNIX_TIMESTAMP() - MySQL compatible, returns seconds
 */
class UnixTimestampFunction : public IFunction {
public:
    ~UnixTimestampFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "UNIX_TIMESTAMP",
            .category = "Date",
            .description = "MySQL compatible: Returns current Unix timestamp in seconds",
            .arguments = {},
            .return_type = ArgType::INTEGER,
            .is_deterministic = false,
            .examples = {R"(UNIX_TIMESTAMP() // 1700000000)"}
        };
    }
    
    void validateArgs(const std::vector<nlohmann::json>&) const override {}
    
    nlohmann::json execute(const std::vector<nlohmann::json>&,
                           const FunctionContext&) const override {
        auto now = std::chrono::system_clock::now();
        auto sec = std::chrono::duration_cast<std::chrono::seconds>(
            now.time_since_epoch()
        ).count();
        return sec;
    }
};

/**
 * @brief FROM_UNIXTIME(seconds) - MySQL compatible, convert seconds to timestamp
 */
class FromUnixTimeFunction : public IFunction {
public:
    ~FromUnixTimeFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "FROM_UNIXTIME",
            .category = "Date",
            .description = "MySQL compatible: Converts Unix seconds to millisecond timestamp",
            .arguments = {
                {"seconds", ArgType::INTEGER, true, nullptr, "Unix timestamp in seconds"}
            },
            .return_type = ArgType::INTEGER,
            .is_deterministic = true,
            .examples = {R"(FROM_UNIXTIME(1700000000) // 1700000000000)"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        int64_t sec = args[0].get<int64_t>();
        return sec * 1000;
    }
};

/**
 * @brief EPOCH_MS(timestamp) - Convert timestamp to epoch milliseconds (identity for our format)
 */
class EpochMsFunction : public IFunction {
public:
    ~EpochMsFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "EPOCH_MS",
            .category = "Date",
            .description = "Returns timestamp as epoch milliseconds (identity function)",
            .arguments = {
                {"timestamp", ArgType::INTEGER, true, nullptr, "Unix timestamp in ms"}
            },
            .return_type = ArgType::INTEGER,
            .is_deterministic = true,
            .examples = {R"(EPOCH_MS(DATE_NOW()) // Same as input)"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        return args[0].get<int64_t>();
    }
};

/**
 * @brief EPOCH_SECONDS(timestamp) - Convert timestamp to epoch seconds
 */
class EpochSecondsFunction : public IFunction {
public:
    ~EpochSecondsFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "EPOCH_SECONDS",
            .category = "Date",
            .description = "Converts millisecond timestamp to epoch seconds",
            .arguments = {
                {"timestamp", ArgType::INTEGER, true, nullptr, "Unix timestamp in ms"}
            },
            .return_type = ArgType::INTEGER,
            .is_deterministic = true,
            .examples = {R"(EPOCH_SECONDS(1700000000000) // 1700000000)"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        return args[0].get<int64_t>() / 1000;
    }
};

/**
 * @brief MAKE_DATE(year, month, day) - Create a date from components
 */
class MakeDateFunction : public IFunction {
public:
    ~MakeDateFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "MAKE_DATE",
            .category = "Date",
            .description = "Creates a timestamp from year, month, day components",
            .arguments = {
                {"year", ArgType::INTEGER, true, nullptr, "Year"},
                {"month", ArgType::INTEGER, true, nullptr, "Month (1-12)"},
                {"day", ArgType::INTEGER, true, nullptr, "Day (1-31)"}
            },
            .return_type = ArgType::INTEGER,
            .is_deterministic = true,
            .examples = {
                R"(MAKE_DATE(2024, 12, 25) // Christmas 2024)",
                R"(MAKE_DATE(DATE_YEAR(NOW()), 1, 1) // Start of this year)"
            }
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        int year = args[0].get<int>();
        int month = args[1].get<int>();
        int day = args[2].get<int>();
        
        std::tm tm = {};
        tm.tm_year = year - 1900;
        tm.tm_mon = month - 1;
        tm.tm_mday = day;
        tm.tm_hour = 0;
        tm.tm_min = 0;
        tm.tm_sec = 0;
        
        return static_cast<int64_t>(portable_timegm(&tm)) * 1000;
    }
};

/**
 * @brief MAKE_DATETIME(year, month, day, hour, minute, second) - Create a datetime from components
 */
class MakeDateTimeFunction : public IFunction {
public:
    ~MakeDateTimeFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "MAKE_DATETIME",
            .category = "Date",
            .description = "Creates a timestamp from date and time components",
            .arguments = {
                {"year", ArgType::INTEGER, true, nullptr, "Year"},
                {"month", ArgType::INTEGER, true, nullptr, "Month (1-12)"},
                {"day", ArgType::INTEGER, true, nullptr, "Day (1-31)"},
                {"hour", ArgType::INTEGER, false, nullptr, "Hour (0-23), default 0"},
                {"minute", ArgType::INTEGER, false, nullptr, "Minute (0-59), default 0"},
                {"second", ArgType::INTEGER, false, nullptr, "Second (0-59), default 0"}
            },
            .return_type = ArgType::INTEGER,
            .is_deterministic = true,
            .examples = {
                R"(MAKE_DATETIME(2024, 12, 31, 23, 59, 59) // New Year's Eve countdown)",
                R"(MAKE_DATETIME(2024, 1, 1) // Midnight on New Year)"
            }
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        int year = args[0].get<int>();
        int month = args[1].get<int>();
        int day = args[2].get<int>();
        int hour = args.size() > 3 ? args[3].get<int>() : 0;
        int minute = args.size() > 4 ? args[4].get<int>() : 0;
        int second = args.size() > 5 ? args[5].get<int>() : 0;
        
        std::tm tm = {};
        tm.tm_year = year - 1900;
        tm.tm_mon = month - 1;
        tm.tm_mday = day;
        tm.tm_hour = hour;
        tm.tm_min = minute;
        tm.tm_sec = second;
        
        return static_cast<int64_t>(portable_timegm(&tm)) * 1000;
    }
};

/**
 * @brief MAKE_TIME(hour, minute, second) - Create time as milliseconds since midnight
 */
class MakeTimeFunction : public IFunction {
public:
    ~MakeTimeFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "MAKE_TIME",
            .category = "Date",
            .description = "Creates a time value as milliseconds since midnight",
            .arguments = {
                {"hour", ArgType::INTEGER, true, nullptr, "Hour (0-23)"},
                {"minute", ArgType::INTEGER, true, nullptr, "Minute (0-59)"},
                {"second", ArgType::INTEGER, false, nullptr, "Second (0-59), default 0"}
            },
            .return_type = ArgType::INTEGER,
            .is_deterministic = true,
            .examples = {
                R"(MAKE_TIME(14, 30) // 52200000 (2:30 PM))",
                R"(MAKE_TIME(9, 0, 0) // 32400000 (9:00 AM))"
            }
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        int hour = args[0].get<int>();
        int minute = args[1].get<int>();
        int second = args.size() > 2 ? args[2].get<int>() : 0;
        
        return static_cast<int64_t>(hour * 3600 + minute * 60 + second) * 1000;
    }
};

/**
 * @brief DATE_COMPARE(date1, date2) - Compare two dates
 * 
 * Returns -1 if date1 < date2, 0 if equal, 1 if date1 > date2
 */
class DateCompareFunction : public IFunction {
public:
    ~DateCompareFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "DATE_COMPARE",
            .category = "Date",
            .description = "Compares two timestamps: returns -1, 0, or 1",
            .arguments = {
                {"date1", ArgType::INTEGER, true, nullptr, "First timestamp in ms"},
                {"date2", ArgType::INTEGER, true, nullptr, "Second timestamp in ms"}
            },
            .return_type = ArgType::INTEGER,
            .is_deterministic = true,
            .examples = {
                R"(DATE_COMPARE(NOW(), YESTERDAY()) // 1 (now is after yesterday))",
                R"(DATE_COMPARE(event1.date, event2.date))"
            }
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        int64_t d1 = args[0].get<int64_t>();
        int64_t d2 = args[1].get<int64_t>();
        
        if (d1 < d2) return -1;
        if (d1 > d2) return 1;
        return 0;
    }
};

/**
 * @brief DATE_BETWEEN(date, startDate, endDate) - Check if date is in range
 */
class DateBetweenFunction : public IFunction {
public:
    ~DateBetweenFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "DATE_BETWEEN",
            .category = "Date",
            .description = "Returns true if date is between start and end (inclusive)",
            .arguments = {
                {"date", ArgType::INTEGER, true, nullptr, "Date to check"},
                {"startDate", ArgType::INTEGER, true, nullptr, "Range start"},
                {"endDate", ArgType::INTEGER, true, nullptr, "Range end"}
            },
            .return_type = ArgType::BOOLEAN,
            .is_deterministic = true,
            .examples = {
                R"(DATE_BETWEEN(event.date, MAKE_DATE(2024,1,1), MAKE_DATE(2024,12,31)))",
                R"(DATE_BETWEEN(NOW(), startTime, endTime))"
            }
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        int64_t date = args[0].get<int64_t>();
        int64_t start = args[1].get<int64_t>();
        int64_t end = args[2].get<int64_t>();
        
        return date >= start && date <= end;
    }
};

/**
 * @brief DATE_TIMESTAMP(dateStr) - Parse date string to timestamp
 */
class DateTimestampFunction : public IFunction {
public:
    ~DateTimestampFunction() override = default;
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
        
        auto tp = std::chrono::system_clock::from_time_t(portable_timegm(&tm));
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
    ~DateIso8601Function() override = default;
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
    ~DateYearFunction() override = default;
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

/** @brief Date month query function. */
class DateMonthFunction : public IFunction {
public:
    ~DateMonthFunction() override = default;
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

/** @brief Date day query function. */
class DateDayFunction : public IFunction {
public:
    ~DateDayFunction() override = default;
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

/** @brief Date hour query function. */
class DateHourFunction : public IFunction {
public:
    ~DateHourFunction() override = default;
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

/** @brief Date minute query function. */
class DateMinuteFunction : public IFunction {
public:
    ~DateMinuteFunction() override = default;
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

/** @brief Date second query function. */
class DateSecondFunction : public IFunction {
public:
    ~DateSecondFunction() override = default;
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

/** @brief Date millisecond query function. */
class DateMillisecondFunction : public IFunction {
public:
    ~DateMillisecondFunction() override = default;
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
    ~DateDayOfWeekFunction() override = default;
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
    ~DateDayOfYearFunction() override = default;
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
    ~DateAddFunction() override = default;
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
    ~DateSubtractFunction() override = default;
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
    ~DateDiffFunction() override = default;
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
    ~DateTruncFunction() override = default;
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
        
        return static_cast<int64_t>(portable_timegm(tm)) * 1000;
    }
};

/**
 * @brief DATE_FORMAT(timestamp, format) - Format date with pattern
 */
class DateFormatFunction : public IFunction {
public:
    ~DateFormatFunction() override = default;
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
// Interval and Relative Time Functions
// ============================================================================

/**
 * @brief INTERVAL(amount, unit) - Create an interval value in milliseconds
 * 
 * Creates a time interval that can be used for date arithmetic.
 * Supports: years, months, weeks, days, hours, minutes, seconds, milliseconds
 */
class IntervalFunction : public IFunction {
public:
    ~IntervalFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "INTERVAL",
            .category = "Date",
            .description = "Creates a time interval in milliseconds for date arithmetic",
            .arguments = {
                {"amount", ArgType::NUMBER, true, nullptr, "Numeric amount (can be negative)"},
                {"unit", ArgType::STRING, true, nullptr, "Unit: years, months, weeks, days, hours, minutes, seconds, ms"}
            },
            .return_type = ArgType::INTEGER,
            .is_deterministic = true,
            .examples = {
                R"(INTERVAL(7, "days") // 604800000 ms)",
                R"(INTERVAL(2, "weeks") // 1209600000 ms)",
                R"(DATE_NOW() + INTERVAL(1, "month"))"
            }
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        double amount = args[0].get<double>();
        std::string unit = args[1].get<std::string>();
        
        // Normalize unit to lowercase and remove trailing 's'
        std::string normalizedUnit = unit;
        std::transform(normalizedUnit.begin(), normalizedUnit.end(), normalizedUnit.begin(), ::tolower);
        if (!normalizedUnit.empty() && normalizedUnit.back() == 's') {
            normalizedUnit.pop_back();
        }
        
        int64_t ms = 0;
        if (normalizedUnit == "millisecond" || normalizedUnit == "ms") {
            ms = static_cast<int64_t>(amount);
        } else if (normalizedUnit == "second") {
            ms = static_cast<int64_t>(amount * 1000);
        } else if (normalizedUnit == "minute") {
            ms = static_cast<int64_t>(amount * 60 * 1000);
        } else if (normalizedUnit == "hour") {
            ms = static_cast<int64_t>(amount * 60 * 60 * 1000);
        } else if (normalizedUnit == "day") {
            ms = static_cast<int64_t>(amount * 24 * 60 * 60 * 1000);
        } else if (normalizedUnit == "week") {
            ms = static_cast<int64_t>(amount * 7 * 24 * 60 * 60 * 1000);
        } else if (normalizedUnit == "month") {
            // Average month: 30.4375 days
            ms = static_cast<int64_t>(amount * 30.4375 * 24 * 60 * 60 * 1000);
        } else if (normalizedUnit == "year") {
            // Average year: 365.25 days
            ms = static_cast<int64_t>(amount * 365.25 * 24 * 60 * 60 * 1000);
        } else {
            throw std::runtime_error("INTERVAL: unknown unit '" + unit + "'");
        }
        
        return ms;
    }
};

/**
 * @brief YEARS(amount) - Create an interval in years
 */
class YearsFunction : public IFunction {
public:
    ~YearsFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "YEARS",
            .category = "Date",
            .description = "Creates an interval of N years in milliseconds",
            .arguments = {
                {"amount", ArgType::NUMBER, true, nullptr, "Number of years (can be fractional or negative)"}
            },
            .return_type = ArgType::INTEGER,
            .is_deterministic = true,
            .examples = {
                R"(YEARS(1) // 31557600000 ms (365.25 days))",
                R"(DATE_NOW() + YEARS(2))",
                R"(YEARS(-1) // One year ago)"
            }
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        double years = args[0].get<double>();
        // 365.25 days per year (accounting for leap years)
        return static_cast<int64_t>(years * 365.25 * 24 * 60 * 60 * 1000);
    }
};

/**
 * @brief MONTHS(amount) - Create an interval in months
 */
class MonthsFunction : public IFunction {
public:
    ~MonthsFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "MONTHS",
            .category = "Date",
            .description = "Creates an interval of N months in milliseconds",
            .arguments = {
                {"amount", ArgType::NUMBER, true, nullptr, "Number of months (can be fractional or negative)"}
            },
            .return_type = ArgType::INTEGER,
            .is_deterministic = true,
            .examples = {
                R"(MONTHS(3) // 7889400000 ms (91.3125 days))",
                R"(DATE_NOW() - MONTHS(6))",
                R"(MONTHS(0.5) // Half a month)"
            }
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        double months = args[0].get<double>();
        // 30.4375 days per month on average
        return static_cast<int64_t>(months * 30.4375 * 24 * 60 * 60 * 1000);
    }
};

/**
 * @brief WEEKS(amount) - Create an interval in weeks
 */
class WeeksFunction : public IFunction {
public:
    ~WeeksFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "WEEKS",
            .category = "Date",
            .description = "Creates an interval of N weeks in milliseconds",
            .arguments = {
                {"amount", ArgType::NUMBER, true, nullptr, "Number of weeks (can be fractional or negative)"}
            },
            .return_type = ArgType::INTEGER,
            .is_deterministic = true,
            .examples = {
                R"(WEEKS(2) // 1209600000 ms)",
                R"(DATE_NOW() + WEEKS(4))",
                R"(WEEKS(-1) // One week ago)"
            }
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        double weeks = args[0].get<double>();
        return static_cast<int64_t>(weeks * 7 * 24 * 60 * 60 * 1000);
    }
};

/**
 * @brief DAYS(amount) - Create an interval in days
 */
class DaysFunction : public IFunction {
public:
    ~DaysFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "DAYS",
            .category = "Date",
            .description = "Creates an interval of N days in milliseconds",
            .arguments = {
                {"amount", ArgType::NUMBER, true, nullptr, "Number of days (can be fractional or negative)"}
            },
            .return_type = ArgType::INTEGER,
            .is_deterministic = true,
            .examples = {
                R"(DAYS(7) // 604800000 ms)",
                R"(DATE_NOW() + DAYS(30))",
                R"(DAYS(-14) // Two weeks ago)"
            }
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        double days = args[0].get<double>();
        return static_cast<int64_t>(days * 24 * 60 * 60 * 1000);
    }
};

/**
 * @brief HOURS(amount) - Create an interval in hours
 */
class HoursFunction : public IFunction {
public:
    ~HoursFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "HOURS",
            .category = "Date",
            .description = "Creates an interval of N hours in milliseconds",
            .arguments = {
                {"amount", ArgType::NUMBER, true, nullptr, "Number of hours (can be fractional or negative)"}
            },
            .return_type = ArgType::INTEGER,
            .is_deterministic = true,
            .examples = {
                R"(HOURS(24) // 86400000 ms (1 day))",
                R"(DATE_NOW() - HOURS(12))",
                R"(HOURS(1.5) // 90 minutes)"
            }
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        double hours = args[0].get<double>();
        return static_cast<int64_t>(hours * 60 * 60 * 1000);
    }
};

/**
 * @brief MINUTES(amount) - Create an interval in minutes
 */
class MinutesFunction : public IFunction {
public:
    ~MinutesFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "MINUTES",
            .category = "Date",
            .description = "Creates an interval of N minutes in milliseconds",
            .arguments = {
                {"amount", ArgType::NUMBER, true, nullptr, "Number of minutes (can be fractional or negative)"}
            },
            .return_type = ArgType::INTEGER,
            .is_deterministic = true,
            .examples = {
                R"(MINUTES(60) // 3600000 ms (1 hour))",
                R"(DATE_NOW() + MINUTES(30))",
                R"(MINUTES(-15) // 15 minutes ago)"
            }
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        double minutes = args[0].get<double>();
        return static_cast<int64_t>(minutes * 60 * 1000);
    }
};

/**
 * @brief SECONDS(amount) - Create an interval in seconds
 */
class SecondsFunction : public IFunction {
public:
    ~SecondsFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "SECONDS",
            .category = "Date",
            .description = "Creates an interval of N seconds in milliseconds",
            .arguments = {
                {"amount", ArgType::NUMBER, true, nullptr, "Number of seconds (can be fractional or negative)"}
            },
            .return_type = ArgType::INTEGER,
            .is_deterministic = true,
            .examples = {
                R"(SECONDS(60) // 60000 ms (1 minute))",
                R"(DATE_NOW() + SECONDS(30))",
                R"(SECONDS(0.5) // 500 ms)"
            }
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        double seconds = args[0].get<double>();
        return static_cast<int64_t>(seconds * 1000);
    }
};

/**
 * @brief WORKDAYS(startDate, endDate, holidays?) - Count business days between dates
 * 
 * Calculates the number of business days (Monday-Friday) between two dates,
 * optionally excluding holidays.
 */
class WorkdaysFunction : public IFunction {
public:
    ~WorkdaysFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "WORKDAYS",
            .category = "Date",
            .description = "Counts business days (Mon-Fri) between two dates, optionally excluding holidays",
            .arguments = {
                {"startDate", ArgType::INTEGER, true, nullptr, "Start timestamp in ms"},
                {"endDate", ArgType::INTEGER, true, nullptr, "End timestamp in ms"},
                {"holidays", ArgType::ARRAY, false, nullptr, "Optional array of holiday timestamps"}
            },
            .return_type = ArgType::INTEGER,
            .is_deterministic = true,
            .examples = {
                R"(WORKDAYS(DATE_TIMESTAMP("2024-01-01"), DATE_TIMESTAMP("2024-01-31")))",
                R"(WORKDAYS(startDate, endDate, [DATE_TIMESTAMP("2024-12-25")]))"
            }
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        int64_t startMs = args[0].get<int64_t>();
        int64_t endMs = args[1].get<int64_t>();
        
        // Ensure start <= end
        if (startMs > endMs) {
            std::swap(startMs, endMs);
        }
        
        // Build holiday set
        std::set<int64_t> holidays;
        if (args.size() > 2 && args[2].is_array()) {
            for (const auto& h : args[2]) {
                // Normalize to day start
                int64_t dayMs = (h.get<int64_t>() / (24 * 60 * 60 * 1000)) * (24 * 60 * 60 * 1000);
                holidays.insert(dayMs);
            }
        }
        
        int64_t count = 0;
        int64_t dayMs = 24 * 60 * 60 * 1000;
        
        for (int64_t ts = startMs; ts <= endMs; ts += dayMs) {
            std::time_t seconds = static_cast<std::time_t>(ts / 1000);
            std::tm* tm = std::gmtime(&seconds);
            
            int dayOfWeek = tm->tm_wday;
            // 0 = Sunday, 6 = Saturday
            bool isWeekend = (dayOfWeek == 0 || dayOfWeek == 6);
            
            // Normalize ts to day start for holiday check
            int64_t dayStart = (ts / dayMs) * dayMs;
            bool isHoliday = holidays.count(dayStart) > 0;
            
            if (!isWeekend && !isHoliday) {
                count++;
            }
        }
        
        return count;
    }
};

/**
 * @brief WORKDAYS_ADD(startDate, workdays, holidays?) - Add workdays to a date
 * 
 * Adds a number of business days to a date, skipping weekends and holidays.
 */
class WorkdaysAddFunction : public IFunction {
public:
    ~WorkdaysAddFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "WORKDAYS_ADD",
            .category = "Date",
            .description = "Adds N business days to a date, skipping weekends and holidays",
            .arguments = {
                {"startDate", ArgType::INTEGER, true, nullptr, "Start timestamp in ms"},
                {"workdays", ArgType::INTEGER, true, nullptr, "Number of workdays to add (can be negative)"},
                {"holidays", ArgType::ARRAY, false, nullptr, "Optional array of holiday timestamps"}
            },
            .return_type = ArgType::INTEGER,
            .is_deterministic = true,
            .examples = {
                R"(WORKDAYS_ADD(DATE_NOW(), 10) // 10 business days from now)",
                R"(WORKDAYS_ADD(startDate, -5) // 5 business days ago)"
            }
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        int64_t startMs = args[0].get<int64_t>();
        int64_t workdays = args[1].get<int64_t>();
        
        // Build holiday set
        std::set<int64_t> holidays;
        if (args.size() > 2 && args[2].is_array()) {
            for (const auto& h : args[2]) {
                int64_t dayMs = (h.get<int64_t>() / (24 * 60 * 60 * 1000)) * (24 * 60 * 60 * 1000);
                holidays.insert(dayMs);
            }
        }
        
        int64_t dayMs = 24 * 60 * 60 * 1000;
        int64_t direction = workdays >= 0 ? 1 : -1;
        workdays = std::abs(workdays);
        
        int64_t currentTs = startMs;
        int64_t added = 0;
        
        while (added < workdays) {
            currentTs += direction * dayMs;
            
            std::time_t seconds = static_cast<std::time_t>(currentTs / 1000);
            std::tm* tm = std::gmtime(&seconds);
            
            int dayOfWeek = tm->tm_wday;
            bool isWeekend = (dayOfWeek == 0 || dayOfWeek == 6);
            
            int64_t dayStart = (currentTs / dayMs) * dayMs;
            bool isHoliday = holidays.count(dayStart) > 0;
            
            if (!isWeekend && !isHoliday) {
                added++;
            }
        }
        
        return currentTs;
    }
};

/**
 * @brief IS_WEEKEND(timestamp) - Check if date falls on weekend
 */
class IsWeekendFunction : public IFunction {
public:
    ~IsWeekendFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "IS_WEEKEND",
            .category = "Date",
            .description = "Returns true if the date falls on Saturday or Sunday",
            .arguments = {
                {"timestamp", ArgType::INTEGER, true, nullptr, "Unix timestamp in ms"}
            },
            .return_type = ArgType::BOOLEAN,
            .is_deterministic = true,
            .examples = {R"(IS_WEEKEND(DATE_TIMESTAMP("2024-01-06")) // true (Saturday))"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        std::time_t seconds = static_cast<std::time_t>(args[0].get<int64_t>() / 1000);
        std::tm* tm = std::gmtime(&seconds);
        return tm->tm_wday == 0 || tm->tm_wday == 6;
    }
};

/**
 * @brief IS_WORKDAY(timestamp, holidays?) - Check if date is a business day
 */
class IsWorkdayFunction : public IFunction {
public:
    ~IsWorkdayFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "IS_WORKDAY",
            .category = "Date",
            .description = "Returns true if the date is a business day (Mon-Fri, not a holiday)",
            .arguments = {
                {"timestamp", ArgType::INTEGER, true, nullptr, "Unix timestamp in ms"},
                {"holidays", ArgType::ARRAY, false, nullptr, "Optional array of holiday timestamps"}
            },
            .return_type = ArgType::BOOLEAN,
            .is_deterministic = true,
            .examples = {
                R"(IS_WORKDAY(DATE_TIMESTAMP("2024-01-08")) // true (Monday))",
                R"(IS_WORKDAY(date, holidayArray))"
            }
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        int64_t ts = args[0].get<int64_t>();
        std::time_t seconds = static_cast<std::time_t>(ts / 1000);
        std::tm* tm = std::gmtime(&seconds);
        
        // Check weekend
        if (tm->tm_wday == 0 || tm->tm_wday == 6) {
            return false;
        }
        
        // Check holidays
        if (args.size() > 1 && args[1].is_array()) {
            int64_t dayMs = 24 * 60 * 60 * 1000;
            int64_t dayStart = (ts / dayMs) * dayMs;
            
            for (const auto& h : args[1]) {
                int64_t holidayDay = (h.get<int64_t>() / dayMs) * dayMs;
                if (dayStart == holidayDay) {
                    return false;
                }
            }
        }
        
        return true;
    }
};

/**
 * @brief DATE_QUARTER(timestamp) - Get quarter of year (1-4)
 */
class DateQuarterFunction : public IFunction {
public:
    ~DateQuarterFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "DATE_QUARTER",
            .category = "Date",
            .description = "Returns the quarter of the year (1-4)",
            .arguments = {
                {"timestamp", ArgType::INTEGER, true, nullptr, "Unix timestamp in ms"}
            },
            .return_type = ArgType::INTEGER,
            .is_deterministic = true,
            .examples = {R"(DATE_QUARTER(DATE_TIMESTAMP("2024-08-15")) // 3)"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        std::time_t seconds = static_cast<std::time_t>(args[0].get<int64_t>() / 1000);
        std::tm* tm = std::gmtime(&seconds);
        return static_cast<int64_t>((tm->tm_mon / 3) + 1);
    }
};

/**
 * @brief DATE_WEEK(timestamp) - Get ISO week number (1-53)
 */
class DateWeekFunction : public IFunction {
public:
    ~DateWeekFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "DATE_WEEK",
            .category = "Date",
            .description = "Returns the ISO week number (1-53)",
            .arguments = {
                {"timestamp", ArgType::INTEGER, true, nullptr, "Unix timestamp in ms"}
            },
            .return_type = ArgType::INTEGER,
            .is_deterministic = true,
            .examples = {R"(DATE_WEEK(DATE_TIMESTAMP("2024-01-15")) // 3)"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        std::time_t seconds = static_cast<std::time_t>(args[0].get<int64_t>() / 1000);
        std::tm* tm = std::gmtime(&seconds);
        
        // Calculate ISO week number
        // First, get the day of year
        int dayOfYear = tm->tm_yday + 1;
        
        // Get the weekday (Mon=1, Sun=7)
        int weekday = tm->tm_wday == 0 ? 7 : tm->tm_wday;
        
        // Calculate week number
        int week = (dayOfYear - weekday + 10) / 7;
        
        // Handle edge cases for weeks 0 and 53
        if (week < 1) {
            week = 52; // Could be 52 or 53, simplified
        } else if (week > 52) {
            // Check if it's really week 53 or week 1 of next year
            int daysInYear = ((tm->tm_year + 1900) % 4 == 0) ? 366 : 365;
            if (dayOfYear > daysInYear - 3) {
                week = 1;
            }
        }
        
        return static_cast<int64_t>(week);
    }
};

/**
 * @brief DATE_LEAPYEAR(year) - Check if year is a leap year
 */
class DateLeapYearFunction : public IFunction {
public:
    ~DateLeapYearFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "DATE_LEAPYEAR",
            .category = "Date",
            .description = "Returns true if the year is a leap year",
            .arguments = {
                {"year", ArgType::INTEGER, true, nullptr, "Year to check (e.g., 2024)"}
            },
            .return_type = ArgType::BOOLEAN,
            .is_deterministic = true,
            .examples = {
                R"(DATE_LEAPYEAR(2024) // true)",
                R"(DATE_LEAPYEAR(2023) // false)"
            }
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        int64_t year = args[0].get<int64_t>();
        bool isLeap = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
        return isLeap;
    }
};

/**
 * @brief DATE_DAYS_IN_MONTH(year, month) - Get number of days in a month
 */
class DateDaysInMonthFunction : public IFunction {
public:
    ~DateDaysInMonthFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "DATE_DAYS_IN_MONTH",
            .category = "Date",
            .description = "Returns the number of days in the specified month",
            .arguments = {
                {"year", ArgType::INTEGER, true, nullptr, "Year"},
                {"month", ArgType::INTEGER, true, nullptr, "Month (1-12)"}
            },
            .return_type = ArgType::INTEGER,
            .is_deterministic = true,
            .examples = {
                R"(DATE_DAYS_IN_MONTH(2024, 2) // 29 (leap year))",
                R"(DATE_DAYS_IN_MONTH(2023, 2) // 28)"
            }
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        int64_t year = args[0].get<int64_t>();
        int64_t month = args[1].get<int64_t>();
        
        if (month < 1 || month > 12) {
            throw std::runtime_error("DATE_DAYS_IN_MONTH: month must be 1-12");
        }
        
        static const int daysPerMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
        int days = daysPerMonth[month - 1];
        
        // February in leap year
        if (month == 2) {
            bool isLeap = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
            if (isLeap) days = 29;
        }
        
        return days;
    }
};

/**
 * @brief DATE_START_OF_WEEK(timestamp, startDay?) - Get start of week
 */
class DateStartOfWeekFunction : public IFunction {
public:
    ~DateStartOfWeekFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "DATE_START_OF_WEEK",
            .category = "Date",
            .description = "Returns the timestamp of the start of the week",
            .arguments = {
                {"timestamp", ArgType::INTEGER, true, nullptr, "Unix timestamp in ms"},
                {"startDay", ArgType::INTEGER, false, nullptr, "First day of week (0=Sun, 1=Mon). Default: 1 (Monday)"}
            },
            .return_type = ArgType::INTEGER,
            .is_deterministic = true,
            .examples = {
                R"(DATE_START_OF_WEEK(DATE_NOW()) // Monday of current week)",
                R"(DATE_START_OF_WEEK(DATE_NOW(), 0) // Sunday of current week)"
            }
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        int64_t ts = args[0].get<int64_t>();
        int startDay = args.size() > 1 ? args[1].get<int>() : 1;
        
        std::time_t seconds = static_cast<std::time_t>(ts / 1000);
        std::tm* tm = std::gmtime(&seconds);
        
        // Calculate days to subtract
        int currentDay = tm->tm_wday;
        int diff = currentDay - startDay;
        if (diff < 0) diff += 7;
        
        // Set to start of day
        tm->tm_hour = 0;
        tm->tm_min = 0;
        tm->tm_sec = 0;
        
        int64_t startOfDay = static_cast<int64_t>(portable_timegm(tm)) * 1000;
        return startOfDay - (diff * 24 * 60 * 60 * 1000);
    }
};

/**
 * @brief DATE_END_OF_MONTH(timestamp) - Get end of month
 */
class DateEndOfMonthFunction : public IFunction {
public:
    ~DateEndOfMonthFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "DATE_END_OF_MONTH",
            .category = "Date",
            .description = "Returns the timestamp of the last day of the month (23:59:59.999)",
            .arguments = {
                {"timestamp", ArgType::INTEGER, true, nullptr, "Unix timestamp in ms"}
            },
            .return_type = ArgType::INTEGER,
            .is_deterministic = true,
            .examples = {R"(DATE_END_OF_MONTH(DATE_NOW()))"}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        int64_t ts = args[0].get<int64_t>();
        
        std::time_t seconds = static_cast<std::time_t>(ts / 1000);
        std::tm* tm = std::gmtime(&seconds);
        
        int year = tm->tm_year + 1900;
        int month = tm->tm_mon + 1;
        
        // Get days in current month
        static const int daysPerMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
        int days = daysPerMonth[month - 1];
        if (month == 2) {
            bool isLeap = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
            if (isLeap) days = 29;
        }
        
        tm->tm_mday = days;
        tm->tm_hour = 23;
        tm->tm_min = 59;
        tm->tm_sec = 59;
        
        return static_cast<int64_t>(portable_timegm(tm)) * 1000 + 999;
    }
};

/**
 * @brief AGE(birthdate, referenceDate?) - Calculate age in years
 */
class AgeFunction : public IFunction {
public:
    ~AgeFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "AGE",
            .category = "Date",
            .description = "Calculates the age in complete years between two dates",
            .arguments = {
                {"birthdate", ArgType::INTEGER, true, nullptr, "Birth date timestamp in ms"},
                {"referenceDate", ArgType::INTEGER, false, nullptr, "Reference date (default: now)"}
            },
            .return_type = ArgType::INTEGER,
            .is_deterministic = false,
            .examples = {
                R"(AGE(DATE_TIMESTAMP("1990-05-15")) // Age today)",
                R"(AGE(birthdate, DATE_TIMESTAMP("2020-01-01")) // Age on specific date)"
            }
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        int64_t birthMs = args[0].get<int64_t>();
        int64_t refMs = args.size() > 1 ? args[1].get<int64_t>() : 
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()
            ).count();
        
        std::time_t birthSec = static_cast<std::time_t>(birthMs / 1000);
        std::time_t refSec = static_cast<std::time_t>(refMs / 1000);
        
        std::tm* birthTm = std::gmtime(&birthSec);
        int birthYear = birthTm->tm_year;
        int birthMonth = birthTm->tm_mon;
        int birthDay = birthTm->tm_mday;
        
        std::tm* refTm = std::gmtime(&refSec);
        int refYear = refTm->tm_year;
        int refMonth = refTm->tm_mon;
        int refDay = refTm->tm_mday;
        
        int age = refYear - birthYear;
        
        // Adjust if birthday hasn't occurred yet this year
        if (refMonth < birthMonth || (refMonth == birthMonth && refDay < birthDay)) {
            age--;
        }
        
        return static_cast<int64_t>(age);
    }
};

// ============================================================================
// Register Date Functions
// ============================================================================

inline void registerDateFunctions(FunctionRegistry& reg) {
    // Core date/time functions
    reg.registerFunction(std::make_unique<DateNowFunction>());
    reg.registerFunction(std::make_unique<DateTimestampFunction>());
    reg.registerFunction(std::make_unique<DateIso8601Function>());
    
    // SQL-compatible aliases
    reg.registerFunction(std::make_unique<NowFunction>());
    reg.registerFunction(std::make_unique<CurrentTimestampFunction>());
    reg.registerFunction(std::make_unique<CurrentDateFunction>());
    reg.registerFunction(std::make_unique<CurrentTimeFunction>());
    reg.registerFunction(std::make_unique<TodayFunction>());
    reg.registerFunction(std::make_unique<YesterdayFunction>());
    reg.registerFunction(std::make_unique<TomorrowFunction>());
    reg.registerFunction(std::make_unique<GetDateFunction>());
    reg.registerFunction(std::make_unique<SysdateFunction>());
    reg.registerFunction(std::make_unique<UnixTimestampFunction>());
    reg.registerFunction(std::make_unique<FromUnixTimeFunction>());
    reg.registerFunction(std::make_unique<EpochMsFunction>());
    reg.registerFunction(std::make_unique<EpochSecondsFunction>());
    
    // Date/time construction
    reg.registerFunction(std::make_unique<MakeDateFunction>());
    reg.registerFunction(std::make_unique<MakeDateTimeFunction>());
    reg.registerFunction(std::make_unique<MakeTimeFunction>());
    
    // Component extraction
    reg.registerFunction(std::make_unique<DateYearFunction>());
    reg.registerFunction(std::make_unique<DateMonthFunction>());
    reg.registerFunction(std::make_unique<DateDayFunction>());
    reg.registerFunction(std::make_unique<DateHourFunction>());
    reg.registerFunction(std::make_unique<DateMinuteFunction>());
    reg.registerFunction(std::make_unique<DateSecondFunction>());
    reg.registerFunction(std::make_unique<DateMillisecondFunction>());
    reg.registerFunction(std::make_unique<DateDayOfWeekFunction>());
    reg.registerFunction(std::make_unique<DateDayOfYearFunction>());
    reg.registerFunction(std::make_unique<DateQuarterFunction>());
    reg.registerFunction(std::make_unique<DateWeekFunction>());
    
    // Date arithmetic
    reg.registerFunction(std::make_unique<DateAddFunction>());
    reg.registerFunction(std::make_unique<DateSubtractFunction>());
    reg.registerFunction(std::make_unique<DateDiffFunction>());
    reg.registerFunction(std::make_unique<DateTruncFunction>());
    reg.registerFunction(std::make_unique<DateFormatFunction>());
    
    // Interval functions
    reg.registerFunction(std::make_unique<IntervalFunction>());
    reg.registerFunction(std::make_unique<YearsFunction>());
    reg.registerFunction(std::make_unique<MonthsFunction>());
    reg.registerFunction(std::make_unique<WeeksFunction>());
    reg.registerFunction(std::make_unique<DaysFunction>());
    reg.registerFunction(std::make_unique<HoursFunction>());
    reg.registerFunction(std::make_unique<MinutesFunction>());
    reg.registerFunction(std::make_unique<SecondsFunction>());
    
    // Business day functions
    reg.registerFunction(std::make_unique<WorkdaysFunction>());
    reg.registerFunction(std::make_unique<WorkdaysAddFunction>());
    reg.registerFunction(std::make_unique<IsWeekendFunction>());
    reg.registerFunction(std::make_unique<IsWorkdayFunction>());
    
    // Utility functions
    reg.registerFunction(std::make_unique<DateLeapYearFunction>());
    reg.registerFunction(std::make_unique<DateDaysInMonthFunction>());
    reg.registerFunction(std::make_unique<DateStartOfWeekFunction>());
    reg.registerFunction(std::make_unique<DateEndOfMonthFunction>());
    reg.registerFunction(std::make_unique<AgeFunction>());
    reg.registerFunction(std::make_unique<DateCompareFunction>());
    reg.registerFunction(std::make_unique<DateBetweenFunction>());
}

} // namespace functions
} // namespace query
} // namespace themis

