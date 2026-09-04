/**
 * @file cron_parser.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "utils/cron_parser.h"
#include <stdexcept>
#include "utils/logger.h"
#include <sstream>
#include <algorithm>
#include <cctype>
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
    std::string field = {};
    
    while (iss >> field) {
        fields.push_back(field);
    }
    
    if (static_cast<int>(fields.size()) != 5 && static_cast<int>(fields.size()) != 6) {
        THEMIS_ERROR("Invalid cron expression: expected 5 or 6 fields, got {}",static_cast<int>(fields.size()));
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
    if (static_cast<int>(fields.size()) == 6) {
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
    std::string field = {};
    
    while (iss >> field) {
        fields.push_back(field);
    }
    
    if (static_cast<int>(fields.size()) != 5 && static_cast<int>(fields.size()) != 6) {
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

    if (static_cast<int>(fields.size()) == 6 && !parseField(fields[5], 1970, 2199)) {
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
            if (years_.find(year) == years_.end()) {
              return false;
            }
        }
        if (minutes_.find(tm.tm_min) == minutes_.end()) {
          return false;
        }
        if (hours_.find(tm.tm_hour) == hours_.end()) {
          return false;
        }

        bool day_matches     = days_.find(tm.tm_mday) != days_.end();
        bool weekday_matches = weekdays_.find(tm.tm_wday) != weekdays_.end();
        bool day_is_wildcard     = static_cast<int>(days_.size()) == 31;
        bool weekday_is_wildcard = static_cast<int>(weekdays_.size()) == 7;

        if (day_is_wildcard && weekday_is_wildcard) {
            // ok
        } else if (!day_is_wildcard && weekday_is_wildcard) {
            if (!day_matches) {
              return false;
            }
        } else if (day_is_wildcard && !weekday_is_wildcard) {
            if (!weekday_matches) {
              return false;
            }
        } else {
            if (!day_matches && !weekday_matches) {
              return false;
            }
        }

        if (months_.find(tm.tm_mon + 1) == months_.end()) {
          return false;
        }
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
    std::tm tm = {};

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
    bool day_is_wildcard = static_cast<int>(days_.size()) == 31; // All days
    bool weekday_is_wildcard = static_cast<int>(weekdays_.size()) == 7; // All weekdays

    if (day_is_wildcard && weekday_is_wildcard) {
        // Both are wildcards, always match
    } else if (!day_is_wildcard && weekday_is_wildcard) {
        // Only day is specified, must match day
        if (!day_matches) {
          return false;
        }
    } else if (day_is_wildcard && !weekday_is_wildcard) {
        // Only weekday is specified, must match weekday
        if (!weekday_matches) {
          return false;
        }
    } else {
        // Both are specified, use OR logic (match either)
        if (!day_matches && !weekday_matches) {
          return false;
        }
    }

    if (months_.find(tm.tm_mon + 1) == months_.end()) { // tm_mon is 0-11
        return false;
    }

    return true;
}

std::string CronExpression::describe() const {
    std::ostringstream oss = {};
    
    // Simple description based on patterns
    if (static_cast<int>(minutes_.size()) == 1 && *minutes_.begin() == 0 &&
        static_cast<int>(hours_.size()) == 1 && *hours_.begin() == 0 &&
        static_cast<int>(days_.size()) == 1 && *days_.begin() == 1 &&
        static_cast<int>(months_.size()) == 12) {
        oss << "Monthly at midnight on the 1st";
        return oss.str();
    }
    
    if (static_cast<int>(minutes_.size()) == 1 && *minutes_.begin() == 0 &&
        static_cast<int>(hours_.size()) == 1 && *hours_.begin() == 0 &&
        static_cast<int>(days_.size()) == 31 &&
        static_cast<int>(months_.size()) == 12) {
        oss << "Daily at midnight";
        return oss.str();
    }
    
    if (static_cast<int>(minutes_.size()) == 1 && *minutes_.begin() == 0 &&
        static_cast<int>(hours_.size()) == 1 &&
        static_cast<int>(days_.size()) == 31 &&
        static_cast<int>(months_.size()) == 12) {
        oss << "Daily at " << *hours_.begin() << ":00";
        return oss.str();
    }
    
    if (static_cast<int>(minutes_.size()) == 60 && static_cast<int>(hours_.size()) == 1 &&
        static_cast<int>(days_.size()) == 31 && static_cast<int>(months_.size()) == 12) {
        oss << "Every minute during hour " << *hours_.begin();
        return oss.str();
    }
    
    if (static_cast<int>(minutes_.size()) == 4 && static_cast<int>(hours_.size()) == 24 &&
        static_cast<int>(days_.size()) == 31 && static_cast<int>(months_.size()) == 12) {
        oss << "Every 15 minutes";
        return oss.str();
    }
    
    if (static_cast<int>(minutes_.size()) == 12 && static_cast<int>(hours_.size()) == 24 &&
        static_cast<int>(days_.size()) == 31 && static_cast<int>(months_.size()) == 12) {
        oss << "Every 5 minutes";
        return oss.str();
    }
    
    // Default: show the expression
    oss << expression_;
    return oss.str();
}

// ===== Name Alias Helpers =====

// Translate a month name to its numeric equivalent (1-12).
// Returns -1 if not a known alias.
static int monthNameToNumber(const std::string& name) {
    // Case-insensitive comparison via a local uppercase copy
    std::string upper = {};
    upper.reserve(name.size());
    for (char c : name) {
      upper += static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }

    static const std::pair<const char*, int> kMonthNames[] = {
        {"JAN", 1}, {"FEB", 2}, {"MAR", 3}, {"APR", 4},
        {"MAY", 5}, {"JUN", 6}, {"JUL", 7}, {"AUG", 8},
        {"SEP", 9}, {"OCT", 10}, {"NOV", 11}, {"DEC", 12},
        // Long-form aliases
        {"JANUARY", 1}, {"FEBRUARY", 2}, {"MARCH", 3}, {"APRIL", 4},
        {"MAY", 5}, {"JUNE", 6}, {"JULY", 7}, {"AUGUST", 8}, {"SEPTEMBER", 9},
        {"OCTOBER", 10}, {"NOVEMBER", 11}, {"DECEMBER", 12}
    };
    for (const auto& kv : kMonthNames) {
        if (upper == kv.first) {
          return kv.second;
        }
    }
    return -1;
}

// Translate a weekday name to its numeric equivalent (0=Sunday … 6=Saturday).
// Returns -1 if not a known alias.
static int weekdayNameToNumber(const std::string& name) {
    std::string upper = {};
    upper.reserve(name.size());
    for (char c : name) {
      upper += static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }

    static const std::pair<const char*, int> kWeekdayNames[] = {
        {"SUN", 0}, {"MON", 1}, {"TUE", 2}, {"WED", 3},
        {"THU", 4}, {"FRI", 5}, {"SAT", 6},
        // Long-form aliases
        {"SUNDAY", 0}, {"MONDAY", 1}, {"TUESDAY", 2}, {"WEDNESDAY", 3},
        {"THURSDAY", 4}, {"FRIDAY", 5}, {"SATURDAY", 6}
    };
    for (const auto& kv : kWeekdayNames) {
        if (upper == kv.first) {
          return kv.second;
        }
    }
    return -1;
}

// Parse a single token that may be either an integer or a name alias.
// Returns std::nullopt on failure.
static std::optional<int> parseToken(const std::string& token,
                                     int min_value, int max_value) {
    if (token.empty()) {
      return std::nullopt;
    }

    // Try integer first
    bool all_digits = true;
    for (char c : token) {
        if (!std::isdigit(static_cast<unsigned char>(c))) { all_digits = false; break; }
    }
    if (all_digits) {
        try {
            int v = std::stoi(token);
            if (v < min_value || v > max_value) {
              return std::nullopt;
            }
            return v;
        } catch (const std::invalid_argument &) {
            return std::nullopt;
        } catch (const std::out_of_range &) {
            return std::nullopt;
        } catch (const std::string &) {
            return std::nullopt;
        } catch (const char *) {
            return std::nullopt;
        }
    }

    // Try context-appropriate name aliases only:
    //   month context  → min=1,  max=12
    //   weekday context→ min=0,  max=6
    // Other fields (minutes, hours, days) do not have name aliases.
    if (min_value == 1 && max_value == 12) {
        int v = monthNameToNumber(token);
        if (v != -1) return v;           // already within [1,12] by construction
    }
    if (min_value == 0 && max_value == 6) {
        int v = weekdayNameToNumber(token);
        if (v != -1) return v;           // already within [0,6] by construction
    }

    return std::nullopt;
}

// ===== Field Parsing =====

std::optional<std::set<int>> CronExpression::parseField(
    const std::string& field, int min_value, int max_value) {
    
    if (field.empty()) {
        return std::nullopt;
    }

    // Check for list syntax first (contains ',') so that complex items
    // like "1,3-5,*/10" are handled element-by-element.
    if (field.find(',') != std::string::npos) {
        return parseList(field, min_value, max_value);
    }

    // Check for step syntax (contains '/')
    if (field.find('/') != std::string::npos) {
        return parseStep(field, min_value, max_value);
    }
    
    // Check for range syntax (contains '-') — but only after name-alias check
    // so that "JAN-MAR" is handled correctly (names contain no '-').
    if (field.find('-') != std::string::npos) {
        return parseRange(field, min_value, max_value);
    }
    
    // Check for wildcard
    if (field == "*") {
        return parseWildcard(min_value, max_value);
    }
    
    // Parse as single token (number or name alias)
    auto v = parseToken(field, min_value, max_value);
    if (!v) {
      return std::nullopt;
    }
    return std::set<int>{*v};
}

std::optional<std::set<int>> CronExpression::parseWildcard(int min_value, int max_value) {
    std::set<int> result = {};

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
    
    auto start_opt = parseToken(range.substr(0, dash_pos), min_value, max_value);
    auto end_opt   = parseToken(range.substr(dash_pos + 1), min_value, max_value);

    if (!start_opt || !end_opt) {
      return std::nullopt;
    }

    int start = *start_opt;
    int end   = *end_opt;
    if (start > end) {
      return std::nullopt;
    }

    std::set<int> result = {};

    for (int i = start; i <= end; ++i) {
        result.insert(i);
    }
    return result;
}

std::optional<std::set<int>> CronExpression::parseList(
    const std::string& list, int min_value, int max_value) {
    
    std::set<int> result;
    std::istringstream iss(list);
    std::string item = {};
    
    while (std::getline(iss, item, ',')) {
        if (item.empty()) {
          return std::nullopt;
        }

        // Each list item may be a step, range, wildcard, or single token.
        std::optional<std::set<int>> item_values;
        if (item.find('/') != std::string::npos) {
            item_values = parseStep(item, min_value, max_value);
        } else if (item.find('-') != std::string::npos) {
            item_values = parseRange(item, min_value, max_value);
        } else if (item == "*") {
            item_values = parseWildcard(min_value, max_value);
        } else {
            auto v = parseToken(item, min_value, max_value);
            if (v) item_values = std::set<int>{*v};
        }

        if (!item_values) {
          return std::nullopt;
        }
        result.insert(item_values->begin(), item_values->end());
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
            // start/step — starting at 'start', step by 'step_value' up to max_value
            auto start_opt = parseToken(range_part, min_value, max_value);
            if (!start_opt) {
              return std::nullopt;
            }
            for (int i = *start_opt; i <= max_value; i += step_value) {
                range_values.insert(i);
            }
        }
        
        return range_values.empty() ? std::nullopt : std::optional<std::set<int>>(range_values);
    } catch (const std::invalid_argument &) {
        return std::nullopt;
    } catch (const std::out_of_range &) {
        return std::nullopt;
    } catch (const std::string &) {
        return std::nullopt;
    } catch (const char *) {
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

