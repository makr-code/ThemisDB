/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            cron_parser.h                                      ║
  Version:         0.0.38                                             ║
  Last Modified:   2026-04-13 04:22:09                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     212                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • cc68749fe8  2026-02-22  chore(scheduler): audit cleanup – update banners Stubs:0 ... ║
    • c298befeda  2026-02-22  feat(scheduler): implement full cron expression parsing (... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file cron_parser.h
 * @brief Cron expression parser and evaluator for task scheduling
 *
 * Implements standard 5-field cron syntax (minute hour day month weekday)
 * and an optional 6-field form with an additional year field
 * (minute hour day month weekday year).
 *
 * Supported syntax:
 * - Wildcards: * (any value)
 * - Ranges: 0-5 (values from 0 to 5)
 * - Lists: 1,3,5 (specific values); list items may themselves be ranges or
 *   steps, e.g. "1,3-5,*\/10"
 * - Steps: *\/15 (every 15 units), 0-30/5 (every 5 from 0 to 30),
 *   5/15 (starting at 5, every 15 up to the field maximum)
 *
 * Name aliases (case-insensitive):
 * - Month  field: JAN FEB MAR APR MAY JUN JUL AUG SEP OCT NOV DEC
 *   (and full names JANUARY … DECEMBER)
 * - Weekday field: SUN MON TUE WED THU FRI SAT
 *   (and full names SUNDAY … SATURDAY)
 * Name aliases may be used in ranges and lists, e.g. "JAN-MAR", "MON,WED,FRI".
 *
 * Special expressions:
 * - @yearly / @annually  – once a year  ("0 0 1 1 *")
 * - @monthly             – once a month ("0 0 1 * *")
 * - @weekly              – once a week  ("0 0 * * 0")
 * - @daily / @midnight   – once a day   ("0 0 * * *")
 * - @hourly              – once an hour ("0 * * * *")
 * - @reboot              – at startup   (never fires via getNextExecution)
 *
 * Timezone support:
 * Use the getNextExecution(from, tz_offset_seconds) overload to schedule
 * tasks relative to a fixed-offset timezone (e.g. UTC+1 = +3600 seconds).
 *
 * Examples:
 * - "0 9-17 * * 1-5"      = Weekdays 9-17h every hour
 * - "0 9-17 * * MON-FRI"  = Same as above using name aliases
 * - "*\/15 * * * *"        = Every 15 minutes
 * - "5/15 * * * *"         = Minutes 5, 20, 35, 50
 * - "1,3-5,7 * * * *"     = Minutes 1, 3, 4, 5, 7
 * - "0 0 1 * *"           = First day of month at midnight
 * - "0 0 1 JAN-MAR *"     = First day of Jan, Feb, Mar at midnight
 * - "30 2 * * 0"          = Every Sunday at 2:30 AM
 * - "@daily"              = Every day at midnight
 * - "@hourly"             = Every hour on the hour
 * - "0 9 * * 1 2025"      = Every Monday at 9:00 in 2025 only (6-field)
 */

#ifndef THEMIS_CRON_PARSER_H
#define THEMIS_CRON_PARSER_H

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

#endif // THEMIS_CRON_PARSER_H
