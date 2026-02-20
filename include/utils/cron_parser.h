/**
 * @file cron_parser.h
 * @brief Cron expression parser and evaluator for task scheduling
 * 
 * Implements standard 5-field cron syntax:
 * - Minute (0-59)
 * - Hour (0-23)
 * - Day of month (1-31)
 * - Month (1-12)
 * - Day of week (0-6, Sunday=0)
 * 
 * Supported syntax:
 * - Wildcards: * (any value)
 * - Ranges: 0-5 (values from 0 to 5)
 * - Lists: 1,3,5 (specific values)
 * - Steps: *\/15 (every 15 units), 0-30/5 (every 5 from 0 to 30)
 * 
 * Special expressions:
 * - @yearly / @annually  – once a year  ("0 0 1 1 *")
 * - @monthly             – once a month ("0 0 1 * *")
 * - @weekly              – once a week  ("0 0 * * 0")
 * - @daily / @midnight   – once a day   ("0 0 * * *")
 * - @hourly              – once an hour ("0 * * * *")
 * - @reboot              – at startup   (never fires via getNextExecution)
 * 
 * Examples:
 * - "0 9-17 * * 1-5" = Weekdays 9-17h every hour
 * - "*\/15 * * * *" = Every 15 minutes
 * - "0 0 1 * *" = First day of month at midnight
 * - "30 2 * * 0" = Every Sunday at 2:30 AM
 * - "@daily" = Every day at midnight
 * - "@hourly" = Every hour on the hour
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
     * @param expression Cron expression (5 fields: minute hour day month weekday)
     * @return Parsed cron expression or error
     */
    static std::optional<CronExpression> parse(const std::string& expression);
    
    /**
     * @brief Validate a cron expression without parsing
     * @param expression Cron expression to validate
     * @return Validation result with error details
     */
    static CronValidationResult validate(const std::string& expression);
    
    /**
     * @brief Calculate the next execution time from a given time point
     * @param from Starting time point
     * @return Next execution time, or nullopt if no valid next time exists
     */
    std::optional<std::chrono::system_clock::time_point> getNextExecution(
        const std::chrono::system_clock::time_point& from) const;
    
    /**
     * @brief Check if a given time matches the cron expression
     * @param time Time point to check
     * @return True if time matches the expression
     */
    bool matches(const std::chrono::system_clock::time_point& time) const;
    
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
    CronExpression(const std::string& expression,
                   std::set<int> minutes,
                   std::set<int> hours,
                   std::set<int> days,
                   std::set<int> months,
                   std::set<int> weekdays);
    
    std::string expression_;
    std::set<int> minutes_;   // 0-59
    std::set<int> hours_;     // 0-23
    std::set<int> days_;      // 1-31
    std::set<int> months_;    // 1-12
    std::set<int> weekdays_;  // 0-6 (Sunday=0)
    
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
