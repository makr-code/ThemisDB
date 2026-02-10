#include <gtest/gtest.h>
#include "utils/cron_parser.h"
#include <chrono>
#include <ctime>

using namespace themis;

class CronParserTest : public ::testing::Test {
protected:
    std::chrono::system_clock::time_point makeTime(int year, int month, int day, 
                                                     int hour, int minute, int second = 0) {
        std::tm tm = {};
        tm.tm_year = year - 1900;
        tm.tm_mon = month - 1;
        tm.tm_mday = day;
        tm.tm_hour = hour;
        tm.tm_min = minute;
        tm.tm_sec = second;
        
        std::time_t time = std::mktime(&tm);
        return std::chrono::system_clock::from_time_t(time);
    }
};

// ===== Validation Tests =====

TEST_F(CronParserTest, ValidateValidExpression) {
    auto result = CronExpression::validate("0 9-17 * * 1-5");
    EXPECT_TRUE(result.is_valid);
    EXPECT_TRUE(result.error_message.empty());
}

TEST_F(CronParserTest, ValidateInvalidFieldCount) {
    auto result = CronExpression::validate("0 9");
    EXPECT_FALSE(result.is_valid);
    EXPECT_FALSE(result.error_message.empty());
}

TEST_F(CronParserTest, ValidateInvalidMinute) {
    auto result = CronExpression::validate("60 * * * *");
    EXPECT_FALSE(result.is_valid);
    EXPECT_FALSE(result.error_message.empty());
}

TEST_F(CronParserTest, ValidateInvalidHour) {
    auto result = CronExpression::validate("0 24 * * *");
    EXPECT_FALSE(result.is_valid);
    EXPECT_FALSE(result.error_message.empty());
}

TEST_F(CronParserTest, ValidateInvalidDay) {
    auto result = CronExpression::validate("0 0 32 * *");
    EXPECT_FALSE(result.is_valid);
    EXPECT_FALSE(result.error_message.empty());
}

TEST_F(CronParserTest, ValidateInvalidMonth) {
    auto result = CronExpression::validate("0 0 1 13 *");
    EXPECT_FALSE(result.is_valid);
    EXPECT_FALSE(result.error_message.empty());
}

TEST_F(CronParserTest, ValidateInvalidWeekday) {
    auto result = CronExpression::validate("0 0 * * 7");
    EXPECT_FALSE(result.is_valid);
    EXPECT_FALSE(result.error_message.empty());
}

// ===== Parsing Tests =====

TEST_F(CronParserTest, ParseSimpleWildcard) {
    auto cron = CronExpression::parse("* * * * *");
    ASSERT_TRUE(cron.has_value());
    EXPECT_EQ(cron->getExpression(), "* * * * *");
}

TEST_F(CronParserTest, ParseRanges) {
    auto cron = CronExpression::parse("0 9-17 * * 1-5");
    ASSERT_TRUE(cron.has_value());
    EXPECT_EQ(cron->getExpression(), "0 9-17 * * 1-5");
}

TEST_F(CronParserTest, ParseLists) {
    auto cron = CronExpression::parse("0,15,30,45 * * * *");
    ASSERT_TRUE(cron.has_value());
    EXPECT_EQ(cron->getExpression(), "0,15,30,45 * * * *");
}

TEST_F(CronParserTest, ParseSteps) {
    auto cron = CronExpression::parse("*/15 * * * *");
    ASSERT_TRUE(cron.has_value());
    EXPECT_EQ(cron->getExpression(), "*/15 * * * *");
}

TEST_F(CronParserTest, ParseStepsWithRange) {
    auto cron = CronExpression::parse("0-30/5 * * * *");
    ASSERT_TRUE(cron.has_value());
    EXPECT_EQ(cron->getExpression(), "0-30/5 * * * *");
}

TEST_F(CronParserTest, ParseComplex) {
    auto cron = CronExpression::parse("0,30 9-17 * * 1-5");
    ASSERT_TRUE(cron.has_value());
    EXPECT_EQ(cron->getExpression(), "0,30 9-17 * * 1-5");
}

// ===== Matching Tests =====

TEST_F(CronParserTest, MatchesEveryMinute) {
    auto cron = CronExpression::parse("* * * * *");
    ASSERT_TRUE(cron.has_value());
    
    auto time = makeTime(2024, 1, 15, 10, 30);
    EXPECT_TRUE(cron->matches(time));
}

TEST_F(CronParserTest, MatchesSpecificMinute) {
    auto cron = CronExpression::parse("30 * * * *");
    ASSERT_TRUE(cron.has_value());
    
    auto matching = makeTime(2024, 1, 15, 10, 30);
    auto not_matching = makeTime(2024, 1, 15, 10, 31);
    
    EXPECT_TRUE(cron->matches(matching));
    EXPECT_FALSE(cron->matches(not_matching));
}

TEST_F(CronParserTest, MatchesSpecificHour) {
    auto cron = CronExpression::parse("0 9 * * *");
    ASSERT_TRUE(cron.has_value());
    
    auto matching = makeTime(2024, 1, 15, 9, 0);
    auto not_matching = makeTime(2024, 1, 15, 10, 0);
    
    EXPECT_TRUE(cron->matches(matching));
    EXPECT_FALSE(cron->matches(not_matching));
}

TEST_F(CronParserTest, MatchesWeekdays) {
    auto cron = CronExpression::parse("0 9 * * 1-5");
    ASSERT_TRUE(cron.has_value());
    
    // Monday (Jan 15, 2024)
    auto monday = makeTime(2024, 1, 15, 9, 0);
    EXPECT_TRUE(cron->matches(monday));
    
    // Sunday (Jan 14, 2024)
    auto sunday = makeTime(2024, 1, 14, 9, 0);
    EXPECT_FALSE(cron->matches(sunday));
}

TEST_F(CronParserTest, MatchesHourRange) {
    auto cron = CronExpression::parse("0 9-17 * * *");
    ASSERT_TRUE(cron.has_value());
    
    auto morning = makeTime(2024, 1, 15, 9, 0);
    auto afternoon = makeTime(2024, 1, 15, 15, 0);
    auto evening = makeTime(2024, 1, 15, 18, 0);
    
    EXPECT_TRUE(cron->matches(morning));
    EXPECT_TRUE(cron->matches(afternoon));
    EXPECT_FALSE(cron->matches(evening));
}

TEST_F(CronParserTest, MatchesMinuteList) {
    auto cron = CronExpression::parse("0,15,30,45 * * * *");
    ASSERT_TRUE(cron.has_value());
    
    auto minute_0 = makeTime(2024, 1, 15, 10, 0);
    auto minute_15 = makeTime(2024, 1, 15, 10, 15);
    auto minute_30 = makeTime(2024, 1, 15, 10, 30);
    auto minute_45 = makeTime(2024, 1, 15, 10, 45);
    auto minute_10 = makeTime(2024, 1, 15, 10, 10);
    
    EXPECT_TRUE(cron->matches(minute_0));
    EXPECT_TRUE(cron->matches(minute_15));
    EXPECT_TRUE(cron->matches(minute_30));
    EXPECT_TRUE(cron->matches(minute_45));
    EXPECT_FALSE(cron->matches(minute_10));
}

TEST_F(CronParserTest, MatchesEvery15Minutes) {
    auto cron = CronExpression::parse("*/15 * * * *");
    ASSERT_TRUE(cron.has_value());
    
    auto minute_0 = makeTime(2024, 1, 15, 10, 0);
    auto minute_15 = makeTime(2024, 1, 15, 10, 15);
    auto minute_30 = makeTime(2024, 1, 15, 10, 30);
    auto minute_45 = makeTime(2024, 1, 15, 10, 45);
    auto minute_10 = makeTime(2024, 1, 15, 10, 10);
    
    EXPECT_TRUE(cron->matches(minute_0));
    EXPECT_TRUE(cron->matches(minute_15));
    EXPECT_TRUE(cron->matches(minute_30));
    EXPECT_TRUE(cron->matches(minute_45));
    EXPECT_FALSE(cron->matches(minute_10));
}

// ===== Next Execution Tests =====

TEST_F(CronParserTest, GetNextExecutionSimple) {
    auto cron = CronExpression::parse("30 10 * * *");
    ASSERT_TRUE(cron.has_value());
    
    // Current time: 10:00
    auto now = makeTime(2024, 1, 15, 10, 0);
    
    // Next execution should be at 10:30 same day
    auto next = cron->getNextExecution(now);
    ASSERT_TRUE(next.has_value());
    
    auto time_t = std::chrono::system_clock::to_time_t(*next);
    std::tm tm;
#ifdef _WIN32
    localtime_s(&tm, &time_t);
#else
    localtime_r(&time_t, &tm);
#endif
    
    EXPECT_EQ(tm.tm_hour, 10);
    EXPECT_EQ(tm.tm_min, 30);
}

TEST_F(CronParserTest, GetNextExecutionNextDay) {
    auto cron = CronExpression::parse("0 9 * * *");
    ASSERT_TRUE(cron.has_value());
    
    // Current time: 10:00 (after 9:00)
    auto now = makeTime(2024, 1, 15, 10, 0);
    
    // Next execution should be at 9:00 next day
    auto next = cron->getNextExecution(now);
    ASSERT_TRUE(next.has_value());
    
    auto time_t = std::chrono::system_clock::to_time_t(*next);
    std::tm tm;
#ifdef _WIN32
    localtime_s(&tm, &time_t);
#else
    localtime_r(&time_t, &tm);
#endif
    
    EXPECT_EQ(tm.tm_mday, 16);  // Next day
    EXPECT_EQ(tm.tm_hour, 9);
    EXPECT_EQ(tm.tm_min, 0);
}

TEST_F(CronParserTest, GetNextExecutionEvery15Minutes) {
    auto cron = CronExpression::parse("*/15 * * * *");
    ASSERT_TRUE(cron.has_value());
    
    // Current time: 10:07
    auto now = makeTime(2024, 1, 15, 10, 7);
    
    // Next execution should be at 10:15
    auto next = cron->getNextExecution(now);
    ASSERT_TRUE(next.has_value());
    
    auto time_t = std::chrono::system_clock::to_time_t(*next);
    std::tm tm;
#ifdef _WIN32
    localtime_s(&tm, &time_t);
#else
    localtime_r(&time_t, &tm);
#endif
    
    EXPECT_EQ(tm.tm_hour, 10);
    EXPECT_EQ(tm.tm_min, 15);
}

// ===== Description Tests =====

TEST_F(CronParserTest, DescribeDaily) {
    auto cron = CronExpression::parse("0 0 * * *");
    ASSERT_TRUE(cron.has_value());
    
    auto desc = cron->describe();
    // Should contain some description or the expression itself
    EXPECT_FALSE(desc.empty());
}

TEST_F(CronParserTest, DescribeEvery15Minutes) {
    auto cron = CronExpression::parse("*/15 * * * *");
    ASSERT_TRUE(cron.has_value());
    
    auto desc = cron->describe();
    EXPECT_FALSE(desc.empty());
}

TEST_F(CronParserTest, DescribeWeekdays) {
    auto cron = CronExpression::parse("0 9-17 * * 1-5");
    ASSERT_TRUE(cron.has_value());
    
    auto desc = cron->describe();
    EXPECT_FALSE(desc.empty());
}
