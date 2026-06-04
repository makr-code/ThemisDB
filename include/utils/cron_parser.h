/**
 * @file cron_parser.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include <string>
#include <vector>
#include <set>
#include <chrono>
#include <optional>

namespace themis {

/**
 * @brief Result of cron expression validation
 */
struct CronValidationResult {
    bool is_valid = false;
    std::string error_message;
    
    operator bool() const { return is_valid; }
};

/**
 * @brief Parsed representation of a cron expression
 */
class CronExpression {
public:
    /**
     * @brief Parse a cron expression string
     * @param expression Cron expression (5 fields: minute hour day month weekday,
     *                   or 6 fields: minute hour day month weekday year)
     * @return Parsed cron expression or error
     */
    static std::optional<CronExpression> parse(const std::string& expression);

    /**
     * @brief Validate a cron expression without parsing
     * @param expression Cron expression to validate (5 or 6 fields)
     * @return Validation result with error details
     */
    static CronValidationResult validate(const std::string& expression);

    /**
     * @brief Calculate the next execution time from a given time point (local time).
     * @param from Starting time point
     * @return Next execution time, or nullopt if no valid next time exists
     */
    std::optional<std::chrono::system_clock::time_point> getNextExecution(
        const std::chrono::system_clock::time_point& from) const;

    /**
     * @brief Calculate the next execution time interpreted in a fixed-offset timezone.
     *
     * The cron expression fields are evaluated in the timezone defined by
     * @p tz_offset_seconds (positive = east of UTC, e.g. +3600 for UTC+1).
     * The returned time point is always in UTC.
     *
     * @param from               Starting time point (UTC)
     * @param tz_offset_seconds  UTC offset of the target timezone in seconds
     * @return Next execution time (UTC), or nullopt if no valid next time exists
     */
    std::optional<std::chrono::system_clock::time_point> getNextExecution(
        const std::chrono::system_clock::time_point& from,
        std::chrono::seconds tz_offset_seconds) const;

    /**
     * @brief Check if a given time matches the cron expression (local time).
     * @param time Time point to check
     * @return True if time matches the expression
     */
    bool matches(const std::chrono::system_clock::time_point& time) const;

    /**
     * @brief Returns true if this expression was parsed with a year constraint (6-field form).
     */
    bool hasYearConstraint() const { return !years_.empty(); }
    
    /**
     * @brief Get human-readable description of the cron expression
     * @return Description string (e.g., "Every 15 minutes")
     */
    std::string describe() const;
    
    /**
     * @brief Get the original cron expression string
     * @return Original expression
     */
    const std::string& getExpression() const { return expression_; }

private:
    // 5-field constructor (no year constraint)
    CronExpression(const std::string& expression,
                   std::set<int> minutes,
                   std::set<int> hours,
                   std::set<int> days,
                   std::set<int> months,
                   std::set<int> weekdays);

    // 6-field constructor (with year constraint)
    CronExpression(const std::string& expression,
                   std::set<int> minutes,
                   std::set<int> hours,
                   std::set<int> days,
                   std::set<int> months,
                   std::set<int> weekdays,
                   std::set<int> years);

    std::string expression_;
    std::set<int> minutes_;   // 0-59
    std::set<int> hours_;     // 0-23
    std::set<int> days_;      // 1-31
    std::set<int> months_;    // 1-12
    std::set<int> weekdays_;  // 0-6 (Sunday=0)
    std::set<int> years_;     // empty = any year; non-empty = specific years (1970-2199)
    
    // Helper methods for parsing
    static std::optional<std::set<int>> parseField(
        const std::string& field, int min_value, int max_value);
    static std::optional<std::set<int>> parseWildcard(int min_value, int max_value);
    static std::optional<std::set<int>> parseRange(
        const std::string& range, int min_value, int max_value);
    static std::optional<std::set<int>> parseList(
        const std::string& list, int min_value, int max_value);
    static std::optional<std::set<int>> parseStep(
        const std::string& step, int min_value, int max_value);
    
    // Helper for next execution calculation
    static std::chrono::system_clock::time_point advanceToNextMinute(
        const std::chrono::system_clock::time_point& time);
    static std::chrono::system_clock::time_point advanceToNextHour(
        const std::chrono::system_clock::time_point& time);
    static std::chrono::system_clock::time_point advanceToNextDay(
        const std::chrono::system_clock::time_point& time);
    static std::chrono::system_clock::time_point advanceToNextMonth(
        const std::chrono::system_clock::time_point& time);
};

} // namespace themis

