/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            cron_parser.cpp                                    ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-02-21 07:18:15                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     606                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "utils/cron_parser.h"
#include "utils/logger.h"
#include <sstream>
#include <algorithm>
#include <ctime>
#include <iomanip>

namespace themis {

// ===== CronExpression Implementation =====

CronExpression::CronExpression(const std::string& expression,
                               std::set<int> minutes,
                               std::set<int> hours,
                               std::set<int> days,
                               std::set<int> months,
                               std::set<int> weekdays)
    : expression_(expression),
      minutes_(std::move(minutes)),
      hours_(std::move(hours)),
      days_(std::move(days)),
      months_(std::move(months)),
      weekdays_(std::move(weekdays)) {}

CronExpression::CronExpression(const std::string& expression,
                               std::set<int> minutes,
                               std::set<int> hours,
                               std::set<int> days,
                               std::set<int> months,
                               std::set<int> weekdays,
                               std::set<int> years)
    : expression_(expression),
      minutes_(std::move(minutes)),
      hours_(std::move(hours)),
      days_(std::move(days)),
      months_(std::move(months)),
      weekdays_(std::move(weekdays)),
      years_(std::move(years)) {}

std::optional<CronExpression> CronExpression::parse(const std::string& expression) {
    // Handle special @-expressions
    if (!expression.empty() && expression[0] == '@') {
        std::string special = expression;
        // Trim whitespace
        while (!special.empty() && (special.back() == ' ' || special.back() == '\t')) {
            special.pop_back();
        }

        if (special == "@yearly" || special == "@annually") {
            return parse("0 0 1 1 *");
        } else if (special == "@monthly") {
            return parse("0 0 1 * *");
        } else if (special == "@weekly") {
            return parse("0 0 * * 0");
        } else if (special == "@daily" || special == "@midnight") {
            return parse("0 0 * * *");
        } else if (special == "@hourly") {
            return parse("0 * * * *");
        } else if (special == "@reboot") {
            // @reboot runs once at startup – treat as a valid expression that never
            // matches during normal scheduling (handled separately by the scheduler).
            // Represent it internally as an impossible schedule (minute 60, which
            // never fires) so the object is still valid.
            auto all_months = parseWildcard(1, 12);
            auto all_days   = parseWildcard(1, 31);
            auto all_wdays  = parseWildcard(0, 6);
            return CronExpression(expression, {60}, {0}, *all_days, *all_months, *all_wdays);
        } else {
            THEMIS_ERROR("Unknown special cron expression: {}", expression);
            return std::nullopt;
        }
    }

    // Split expression into fields
    std::istringstream iss(expression);
    std::vector<std::string> fields;
    std::string field;
    
    while (iss >> field) {
        fields.push_back(field);
    }
    
    if (fields.size() != 5 && fields.size() != 6) {
        THEMIS_ERROR("Invalid cron expression: expected 5 or 6 fields, got {}", fields.size());
        return std::nullopt;
    }

    // Parse each field
    auto minutes = parseField(fields[0], 0, 59);
    if (!minutes) {
        THEMIS_ERROR("Invalid minute field: {}", fields[0]);
        return std::nullopt;
    }

    auto hours = parseField(fields[1], 0, 23);
    if (!hours) {
        THEMIS_ERROR("Invalid hour field: {}", fields[1]);
        return std::nullopt;
    }

    auto days = parseField(fields[2], 1, 31);
    if (!days) {
        THEMIS_ERROR("Invalid day field: {}", fields[2]);
        return std::nullopt;
    }

    auto months = parseField(fields[3], 1, 12);
    if (!months) {
        THEMIS_ERROR("Invalid month field: {}", fields[3]);
        return std::nullopt;
    }

    auto weekdays = parseField(fields[4], 0, 6);
    if (!weekdays) {
        THEMIS_ERROR("Invalid weekday field: {}", fields[4]);
        return std::nullopt;
    }

    // Optional 6th field: year (1970-2199)
    if (fields.size() == 6) {
        auto years = parseField(fields[5], 1970, 2199);
        if (!years) {
            THEMIS_ERROR("Invalid year field: {} (must be in range 1970-2199)", fields[5]);
            return std::nullopt;
        }
        return CronExpression(expression, *minutes, *hours, *days, *months, *weekdays, *years);
    }

    return CronExpression(expression, *minutes, *hours, *days, *months, *weekdays);
}

CronValidationResult CronExpression::validate(const std::string& expression) {
    CronValidationResult result;
    
    // Handle special @-expressions
    if (!expression.empty() && expression[0] == '@') {
        std::string special = expression;
        while (!special.empty() && (special.back() == ' ' || special.back() == '\t')) {
            special.pop_back();
        }
        static const std::set<std::string> VALID_SPECIALS = {
            "@yearly", "@annually", "@monthly", "@weekly",
            "@daily", "@midnight", "@hourly", "@reboot"
        };
        if (VALID_SPECIALS.count(special)) {
            result.is_valid = true;
            return result;
        }
        result.error_message = "Unknown special expression '" + special +
            "'. Valid specials: @yearly, @annually, @monthly, @weekly, @daily, @midnight, @hourly, @reboot";
        return result;
    }

    // Try to parse - if it succeeds, it's valid
    auto parsed = parse(expression);
    if (parsed) {
        result.is_valid = true;
        return result;
    }
    
    // Provide more detailed error message
    std::istringstream iss(expression);
    std::vector<std::string> fields;
    std::string field;
    
    while (iss >> field) {
        fields.push_back(field);
    }
    
    if (fields.size() != 5 && fields.size() != 6) {
        result.error_message = "Cron expression must have 5 fields (minute hour day month weekday)"
                               " or 6 fields (minute hour day month weekday year), got " +
                               std::to_string(fields.size());
        return result;
    }

    // Check each field
    if (!parseField(fields[0], 0, 59)) {
        result.error_message = "Invalid minute field '" + fields[0] + "' (must be 0-59)";
        return result;
    }

    if (!parseField(fields[1], 0, 23)) {
        result.error_message = "Invalid hour field '" + fields[1] + "' (must be 0-23)";
        return result;
    }

    if (!parseField(fields[2], 1, 31)) {
        result.error_message = "Invalid day field '" + fields[2] + "' (must be 1-31)";
        return result;
    }

    if (!parseField(fields[3], 1, 12)) {
        result.error_message = "Invalid month field '" + fields[3] + "' (must be 1-12)";
        return result;
    }

    if (!parseField(fields[4], 0, 6)) {
        result.error_message = "Invalid weekday field '" + fields[4] + "' (must be 0-6)";
        return result;
    }

    if (fields.size() == 6 && !parseField(fields[5], 1970, 2199)) {
        result.error_message = "Invalid year field '" + fields[5] + "' (must be 1970-2199)";
        return result;
    }

    result.is_valid = true;
    return result;
}

std::optional<std::chrono::system_clock::time_point> CronExpression::getNextExecution(
    const std::chrono::system_clock::time_point& from) const {
    
    // @reboot is handled externally – never fires via normal scheduling
    if (expression_ == "@reboot") {
        return std::nullopt;
    }

    // Start from the next minute
    auto current = advanceToNextMinute(from);
    
    // Limit search to avoid infinite loops
    // Search up to 4 years ahead (4 years * 365 days * 24 hours * 60 minutes)
    const int MAX_ITERATIONS = 4 * 365 * 24 * 60;
    
    for (int i = 0; i < MAX_ITERATIONS; ++i) {
        if (matches(current)) {
            return current;
        }
        current = advanceToNextMinute(current);
    }
    
    THEMIS_WARN("Could not find next execution time for cron expression: {}", expression_);
    return std::nullopt;
}

std::optional<std::chrono::system_clock::time_point> CronExpression::getNextExecution(
    const std::chrono::system_clock::time_point& from,
    std::chrono::seconds tz_offset_seconds) const {

    // Strategy: shift 'from' into the target timezone by adding the offset.
    // Then use gmtime (UTC interpretation) on the shifted time, which gives us
    // the wall-clock time in the target timezone.  The cron matching is done in
    // that shifted space.  Finally, subtract the offset from the result to get
    // back to UTC.
    //
    // This approach avoids touching the process-local TZ setting and is
    // thread-safe.

    if (expression_ == "@reboot") {
        return std::nullopt;
    }

    // Helper lambda: check whether a shifted time_point matches the cron expression
    // using gmtime (i.e. interpreting time_point as if it were UTC wall-clock time
    // in the target timezone).
    auto matchesInTz = [this](const std::chrono::system_clock::time_point& shifted) -> bool {
        auto tt = std::chrono::system_clock::to_time_t(shifted);
        std::tm tm = {};
#ifdef _WIN32
        gmtime_s(&tm, &tt);
#else
        gmtime_r(&tt, &tm);
#endif
        // Check year constraint
        if (!years_.empty()) {
            int year = tm.tm_year + 1900;
            if (years_.find(year) == years_.end()) return false;
        }
        if (minutes_.find(tm.tm_min) == minutes_.end())      return false;
        if (hours_.find(tm.tm_hour) == hours_.end())         return false;

        bool day_matches     = days_.find(tm.tm_mday) != days_.end();
        bool weekday_matches = weekdays_.find(tm.tm_wday) != weekdays_.end();
        bool day_is_wildcard     = days_.size() == 31;
        bool weekday_is_wildcard = weekdays_.size() == 7;

        if (day_is_wildcard && weekday_is_wildcard) {
            // ok
        } else if (!day_is_wildcard && weekday_is_wildcard) {
            if (!day_matches) return false;
        } else if (day_is_wildcard && !weekday_is_wildcard) {
            if (!weekday_matches) return false;
        } else {
            if (!day_matches && !weekday_matches) return false;
        }

        if (months_.find(tm.tm_mon + 1) == months_.end()) return false;
        return true;
    };

    // Start one minute ahead (in the shifted/TZ-adjusted space)
    auto current_shifted = advanceToNextMinute(from + tz_offset_seconds);

    const int MAX_ITERATIONS = 4 * 365 * 24 * 60;
    for (int i = 0; i < MAX_ITERATIONS; ++i) {
        if (matchesInTz(current_shifted)) {
            // Convert back to UTC
            return current_shifted - tz_offset_seconds;
        }
        current_shifted = advanceToNextMinute(current_shifted);
    }

    THEMIS_WARN("Could not find next execution time (tz-aware) for cron expression: {}",
                expression_);
    return std::nullopt;
}

bool CronExpression::matches(const std::chrono::system_clock::time_point& time) const {

    auto time_t = std::chrono::system_clock::to_time_t(time);
    std::tm tm;

#ifdef _WIN32
    localtime_s(&tm, &time_t);
#else
    localtime_r(&time_t, &tm);
#endif

    // Check year constraint (6-field form only; empty set = any year)
    if (!years_.empty()) {
        int year = tm.tm_year + 1900;
        if (years_.find(year) == years_.end()) {
            return false;
        }
    }

    // Check each component
    if (minutes_.find(tm.tm_min) == minutes_.end()) {
        return false;
    }

    if (hours_.find(tm.tm_hour) == hours_.end()) {
        return false;
    }

    // For day of month and day of week, use OR logic (match either one)
    bool day_matches = days_.find(tm.tm_mday) != days_.end();
    bool weekday_matches = weekdays_.find(tm.tm_wday) != weekdays_.end();

    // If both are wildcards (match all), then both match
    bool day_is_wildcard = days_.size() == 31; // All days
    bool weekday_is_wildcard = weekdays_.size() == 7; // All weekdays

    if (day_is_wildcard && weekday_is_wildcard) {
        // Both are wildcards, always match
    } else if (!day_is_wildcard && weekday_is_wildcard) {
        // Only day is specified, must match day
        if (!day_matches) return false;
    } else if (day_is_wildcard && !weekday_is_wildcard) {
        // Only weekday is specified, must match weekday
        if (!weekday_matches) return false;
    } else {
        // Both are specified, use OR logic (match either)
        if (!day_matches && !weekday_matches) return false;
    }

    if (months_.find(tm.tm_mon + 1) == months_.end()) { // tm_mon is 0-11
        return false;
    }

    return true;
}

std::string CronExpression::describe() const {
    std::ostringstream oss;
    
    // Simple description based on patterns
    if (minutes_.size() == 1 && *minutes_.begin() == 0 &&
        hours_.size() == 1 && *hours_.begin() == 0 &&
        days_.size() == 1 && *days_.begin() == 1 &&
        months_.size() == 12) {
        oss << "Monthly at midnight on the 1st";
        return oss.str();
    }
    
    if (minutes_.size() == 1 && *minutes_.begin() == 0 &&
        hours_.size() == 1 && *hours_.begin() == 0 &&
        days_.size() == 31 &&
        months_.size() == 12) {
        oss << "Daily at midnight";
        return oss.str();
    }
    
    if (minutes_.size() == 1 && *minutes_.begin() == 0 &&
        hours_.size() == 1 &&
        days_.size() == 31 &&
        months_.size() == 12) {
        oss << "Daily at " << *hours_.begin() << ":00";
        return oss.str();
    }
    
    if (minutes_.size() == 60 && hours_.size() == 1 &&
        days_.size() == 31 && months_.size() == 12) {
        oss << "Every minute during hour " << *hours_.begin();
        return oss.str();
    }
    
    if (minutes_.size() == 4 && hours_.size() == 24 &&
        days_.size() == 31 && months_.size() == 12) {
        oss << "Every 15 minutes";
        return oss.str();
    }
    
    if (minutes_.size() == 12 && hours_.size() == 24 &&
        days_.size() == 31 && months_.size() == 12) {
        oss << "Every 5 minutes";
        return oss.str();
    }
    
    // Default: show the expression
    oss << expression_;
    return oss.str();
}

// ===== Field Parsing =====

std::optional<std::set<int>> CronExpression::parseField(
    const std::string& field, int min_value, int max_value) {
    
    if (field.empty()) {
        return std::nullopt;
    }
    
    // Check for step syntax first (contains '/')
    if (field.find('/') != std::string::npos) {
        return parseStep(field, min_value, max_value);
    }
    
    // Check for list syntax (contains ',')
    if (field.find(',') != std::string::npos) {
        return parseList(field, min_value, max_value);
    }
    
    // Check for range syntax (contains '-')
    if (field.find('-') != std::string::npos) {
        return parseRange(field, min_value, max_value);
    }
    
    // Check for wildcard
    if (field == "*") {
        return parseWildcard(min_value, max_value);
    }
    
    // Parse as single number
    try {
        int value = std::stoi(field);
        if (value < min_value || value > max_value) {
            return std::nullopt;
        }
        return std::set<int>{value};
    } catch (...) {
        return std::nullopt;
    }
}

std::optional<std::set<int>> CronExpression::parseWildcard(int min_value, int max_value) {
    std::set<int> result;
    for (int i = min_value; i <= max_value; ++i) {
        result.insert(i);
    }
    return result;
}

std::optional<std::set<int>> CronExpression::parseRange(
    const std::string& range, int min_value, int max_value) {
    
    size_t dash_pos = range.find('-');
    if (dash_pos == std::string::npos || dash_pos == 0 || dash_pos == range.length() - 1) {
        return std::nullopt;
    }
    
    try {
        int start = std::stoi(range.substr(0, dash_pos));
        int end = std::stoi(range.substr(dash_pos + 1));
        
        if (start < min_value || start > max_value ||
            end < min_value || end > max_value ||
            start > end) {
            return std::nullopt;
        }
        
        std::set<int> result;
        for (int i = start; i <= end; ++i) {
            result.insert(i);
        }
        return result;
    } catch (...) {
        return std::nullopt;
    }
}

std::optional<std::set<int>> CronExpression::parseList(
    const std::string& list, int min_value, int max_value) {
    
    std::set<int> result;
    std::istringstream iss(list);
    std::string item;
    
    while (std::getline(iss, item, ',')) {
        try {
            int value = std::stoi(item);
            if (value < min_value || value > max_value) {
                return std::nullopt;
            }
            result.insert(value);
        } catch (...) {
            return std::nullopt;
        }
    }
    
    return result.empty() ? std::nullopt : std::optional<std::set<int>>(result);
}

std::optional<std::set<int>> CronExpression::parseStep(
    const std::string& step, int min_value, int max_value) {
    
    size_t slash_pos = step.find('/');
    if (slash_pos == std::string::npos || slash_pos == step.length() - 1) {
        return std::nullopt;
    }
    
    std::string range_part = step.substr(0, slash_pos);
    std::string step_part = step.substr(slash_pos + 1);
    
    try {
        int step_value = std::stoi(step_part);
        if (step_value <= 0) {
            return std::nullopt;
        }
        
        // Parse the range part
        std::set<int> range_values;
        
        if (range_part == "*") {
            // */n means every n starting from min_value
            for (int i = min_value; i <= max_value; i += step_value) {
                range_values.insert(i);
            }
        } else if (range_part.find('-') != std::string::npos) {
            // a-b/n means every n from a to b
            auto range = parseRange(range_part, min_value, max_value);
            if (!range) {
                return std::nullopt;
            }
            
            int start = *range->begin();
            int end = *range->rbegin();
            
            for (int i = start; i <= end; i += step_value) {
                range_values.insert(i);
            }
        } else {
            // Single number with step (e.g., 5/10) - not standard, treat as error
            return std::nullopt;
        }
        
        return range_values.empty() ? std::nullopt : std::optional<std::set<int>>(range_values);
    } catch (...) {
        return std::nullopt;
    }
}

// ===== Time Advancement =====

std::chrono::system_clock::time_point CronExpression::advanceToNextMinute(
    const std::chrono::system_clock::time_point& time) {
    return time + std::chrono::minutes(1);
}

std::chrono::system_clock::time_point CronExpression::advanceToNextHour(
    const std::chrono::system_clock::time_point& time) {
    return time + std::chrono::hours(1);
}

std::chrono::system_clock::time_point CronExpression::advanceToNextDay(
    const std::chrono::system_clock::time_point& time) {
    return time + std::chrono::hours(24);
}

std::chrono::system_clock::time_point CronExpression::advanceToNextMonth(
    const std::chrono::system_clock::time_point& time) {
    // Advance exactly one calendar month using std::tm so that months with
    // 28, 29, 30, or 31 days are handled correctly (including leap years).
    auto time_t_val = std::chrono::system_clock::to_time_t(time);
    std::tm tm = {};
#ifdef _WIN32
    localtime_s(&tm, &time_t_val);
#else
    localtime_r(&time_t_val, &tm);
#endif

    // Roll the month forward by 1 and carry into the year if needed
    ++tm.tm_mon;  // tm_mon is 0-based
    if (tm.tm_mon >= 12) {
        tm.tm_mon = 0;
        ++tm.tm_year;
    }
    // mktime normalises the struct (handles dst transitions too)
    tm.tm_isdst = -1;
    time_t next = std::mktime(&tm);
    return std::chrono::system_clock::from_time_t(next);
}

} // namespace themis
