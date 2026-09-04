/**
 * @file timestamp_utils.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=0, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "utils/timestamp_utils.h"

#include <array>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace themis {
namespace utils {

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

namespace {

// Parse a fixed-width integer at position pos in s, advancing pos by width.
// width is caller-controlled (max 4 for a 4-digit year) so overflow is not
// possible for valid timestamps; we still guard against non-digit characters.
int parseField(const std::string& s, size_t& pos, size_t width) {
    if (pos + width > s.size()) {
        throw std::invalid_argument("TimestampUtils::parse: unexpected end of string");
    }
    if (width > 9) {
        // Defensive: prevent accumulating more than 9 digits into a signed int.
        throw std::invalid_argument("TimestampUtils::parse: field width exceeds safe limit");
    }
    int val = 0;
    for (size_t i = 0; i < width; ++i) {
        char c = s[pos + i];
        if (c < '0' || c > '9') {
            throw std::invalid_argument(std::string("TimestampUtils::parse: expected digit, got '") + c + "'");
        }
        val = val * 10 + (c - '0');
    }
    pos += width;
    return val;
}

void expectChar(const std::string& s, size_t& pos, char expected) {
    if (pos >= s.size() || s[pos] != expected) {
        throw std::invalid_argument(
            std::string("TimestampUtils::parse: expected '") + expected + "' at position " + std::to_string(pos));
    }
    ++pos;
}

// Portable timegm: convert broken-down UTC time to time_t.
time_t utc_to_time_t(std::tm& tm_utc) {
#if defined(_WIN32)
    return _mkgmtime(&tm_utc);
#else
    return timegm(&tm_utc);
#endif
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// format
// ---------------------------------------------------------------------------

std::string TimestampUtils::format(std::chrono::system_clock::time_point tp, bool include_ms) {
    auto ms_since_epoch = std::chrono::duration_cast<std::chrono::milliseconds>(tp.time_since_epoch());
    time_t sec = static_cast<time_t>(ms_since_epoch.count() / 1000);
    int ms_part = static_cast<int>(ms_since_epoch.count() % 1000);
    if (ms_part < 0) { --sec; ms_part += 1000; }

    std::tm tm_utc{};
#if defined(_WIN32)
    gmtime_s(&tm_utc, &sec);
#else
    gmtime_r(&sec, &tm_utc);
#endif

    char buf[64];
    std::snprintf(buf, sizeof(buf),
                  "%04d-%02d-%02dT%02d:%02d:%02d",
                  tm_utc.tm_year + 1900,
                  tm_utc.tm_mon  + 1,
                  tm_utc.tm_mday,
                  tm_utc.tm_hour,
                  tm_utc.tm_min,
                  tm_utc.tm_sec);

    std::string result(buf);
    if (include_ms) {
        char ms_buf[16];
        const int written = std::snprintf(ms_buf, sizeof(ms_buf), ".%03d", ms_part);
        if (written < 0 || written >= static_cast<int>(sizeof(ms_buf))) {
            throw std::overflow_error("Timestamp millisecond formatting overflow");
        }
        result += ms_buf;
    }
    result += 'Z';
    return result;
}

// ---------------------------------------------------------------------------
// parse
// ---------------------------------------------------------------------------

std::chrono::system_clock::time_point TimestampUtils::parse(const std::string& s) {
    // Expected: YYYY-MM-DDTHH:MM:SS[.mmm][Z|(+|-)HH:MM]
    size_t pos = 0;

    int year   = parseField(s, pos, 4);
    expectChar(s, pos, '-');
    int month  = parseField(s, pos, 2);
    expectChar(s, pos, '-');
    int day    = parseField(s, pos, 2);
    expectChar(s, pos, 'T');
    int hour   = parseField(s, pos, 2);
    expectChar(s, pos, ':');
    int minute = parseField(s, pos, 2);
    expectChar(s, pos, ':');
    int second = parseField(s, pos, 2);

    // Optional fractional seconds
    int ms = 0;
    if (pos < s.size() && s[pos] == '.') {
        ++pos;
        // Collect up to 3 digits; shorter fractions are left-padded with zeros.
        int digits = 0;
        while (pos < s.size() && s[pos] >= '0' && s[pos] <= '9' && digits < 3) {
            ms = ms * 10 + (s[pos] - '0');
            ++pos;
            ++digits;
        }
        // Skip remaining sub-millisecond digits
        while (pos < s.size() && s[pos] >= '0' && s[pos] <= '9') ++pos;
        for (; digits < 3; ++digits) ms *= 10;
    }

    // Timezone
    int offset_sec = 0;
    if (pos < s.size()) {
        char tz = s[pos];
        if (tz == 'Z') {
            ++pos;
        } else if (tz == '+' || tz == '-') {
            int sign = (tz == '+') ? 1 : -1;
            ++pos;
            int tz_h = parseField(s, pos, 2);
            expectChar(s, pos, ':');
            int tz_m = parseField(s, pos, 2);
            offset_sec = sign * (tz_h * 3600 + tz_m * 60);
        } else {
            throw std::invalid_argument(std::string("TimestampUtils::parse: unexpected character '") + tz + "' in timezone");
        }
    }

    if (pos != s.size()) {
        throw std::invalid_argument("TimestampUtils::parse: trailing characters in timestamp");
    }

    // Validate ranges
    if (month < 1 || month > 12 || day < 1 || day > 31 ||
        hour < 0 || hour > 23 || minute < 0 || minute > 59 ||
        second < 0 || second > 60 || ms < 0 || ms > 999) {
        throw std::invalid_argument("TimestampUtils::parse: field out of range");
    }

    std::tm tm_utc{};
    tm_utc.tm_year = year - 1900;
    tm_utc.tm_mon  = month - 1;
    tm_utc.tm_mday = day;
    tm_utc.tm_hour = hour;
    tm_utc.tm_min  = minute;
    tm_utc.tm_sec  = second;

    time_t t = utc_to_time_t(tm_utc);
    if (t == static_cast<time_t>(-1)) {
        throw std::invalid_argument("TimestampUtils::parse: invalid date/time values");
    }
    t -= offset_sec; // convert local time to UTC

    auto epoch_ms = static_cast<int64_t>(t) * 1000 + ms;
    return std::chrono::system_clock::time_point{std::chrono::milliseconds{epoch_ms}};
}

// ---------------------------------------------------------------------------
// now
// ---------------------------------------------------------------------------

std::string TimestampUtils::now([[maybe_unused]] bool include_ms) {
    return format(std::chrono::system_clock::now(), include_ms);
}

// ---------------------------------------------------------------------------
// formatDuration
// ---------------------------------------------------------------------------

std::string TimestampUtils::formatDuration(std::chrono::nanoseconds ns) {
    using namespace std::chrono;

    bool negative = ns.count() < 0;
    if (negative) ns = -ns;

    auto total_ms  = duration_cast<milliseconds>(ns);
    auto total_sec = duration_cast<seconds>(total_ms);
    auto h         = duration_cast<hours>(total_sec);
    auto m         = duration_cast<minutes>(total_sec - h);
    auto s         = total_sec - h - m;
    auto ms_part   = total_ms - duration_cast<milliseconds>(total_sec);

    std::ostringstream oss;
    if (negative) oss << '-';

    bool any = false;
    if (h.count() > 0) { oss << h.count() << 'h'; any = true; }
    if (m.count() > 0 || any) {
        if (any) oss << ' ';
        oss << m.count() << 'm';
        any = true;
    }
    if (any) oss << ' ';

    // Seconds with millisecond fraction
    oss << s.count();
    if (ms_part.count() > 0) {
        char ms_buf[16];
        std::snprintf(ms_buf, sizeof(ms_buf), ".%03lld", static_cast<long long>(ms_part.count()));
        oss << ms_buf;
    }
    oss << 's';
    return oss.str();
}

// ---------------------------------------------------------------------------
// toUnixMs / fromUnixMs
// ---------------------------------------------------------------------------

int64_t TimestampUtils::toUnixMs(std::chrono::system_clock::time_point tp) {
    return std::chrono::duration_cast<std::chrono::milliseconds>(tp.time_since_epoch()).count();
}

std::chrono::system_clock::time_point TimestampUtils::fromUnixMs(int64_t ms) {
    return std::chrono::system_clock::time_point{std::chrono::milliseconds{ms}};
}

} // namespace utils
} // namespace themis

