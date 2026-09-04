/**
 * @file holiday_provider.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <mutex>
#include <fstream>
#include <sstream>
#include <regex>
#include <stdexcept>
#include <chrono>
#include <ctime>
#include <algorithm>
#include <filesystem>
#include <functional>

namespace themis {
namespace query {
namespace functions {

/**
 * @brief HolidayProvider - Secure centralized holiday calendar management
 * 
 * The HolidayProvider manages holiday calendars centrally and securely.
 * 
 * **Security Model:**
 * - AQL queries can ONLY access calendars by name (no file paths)
 * - External files are loaded at server startup by admin
 * - Built-in calendars for common regions (DE, AT, CH, US, etc.)
 * - No user input ever touches the filesystem
 * 
 * **Architecture:**
 * ```
 * ┌─────────────────────────────────────────────────────────────┐
 * │                     HolidayProvider                         │
 * │  ┌─────────────────────────────────────────────────────┐   │
 * │  │              Built-in Calendars                      │   │
 * │  │  DE_2024, DE_2025, AT_2024, CH_2024, US_2024, ...   │   │
 * │  └─────────────────────────────────────────────────────┘   │
 * │  ┌─────────────────────────────────────────────────────┐   │
 * │  │         Admin-Loaded Calendars (startup)             │   │
 * │  │  company_holidays, custom_calendar, ...              │   │
 * │  └─────────────────────────────────────────────────────┘   │
 * │                           │                                 │
 * │                     ┌─────┴─────┐                          │
 * │                     │ AQL API   │                          │
 * │                     │ (by name) │                          │
 * │                     └───────────┘                          │
 * └─────────────────────────────────────────────────────────────┘
 * ```
 * 
 * **Usage in AQL (safe - only names allowed):**
 *   LET holidays = HOLIDAYS("DE_2024")
 *   LET workdays = WORKDAYS(start, end, holidays)
 *   LET holidays = HOLIDAYS("DE_2024", "company_holidays")  // Merge calendars
 * 
 * **Admin API (server startup only):**
 *   HolidayProvider::instance().loadCalendarFromFile("company", "/path/to/file.json");
 *   HolidayProvider::instance().registerCalendar("custom", {dates...});
 */
class HolidayProvider {
public:
    /**
     * @brief Holiday calendar with metadata
     */
    struct Calendar {
        std::string name;             // Calendar name
        std::string region;           // Region code (ISO 3166-1)
        std::string description;      // Human-readable description
        int year = 0;                 // Year if applicable (0 = recurring)
        std::set<int64_t> holidays;   // Holiday timestamps (day start, ms)
        bool isBuiltin = false;       // True for built-in calendars
    };
    
    /**
     * @brief Get singleton instance
     */
    static HolidayProvider& instance() {
        static HolidayProvider provider;
        return provider;
    }
    
    /**
     * @brief Initialize with built-in calendars
     * 
     * Called automatically on first access.
     */
    void initialize() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (initialized_) {
          return;
        }
        
        registerBuiltinCalendars();
        initialized_ = true;
    }
    
    /**
     * @brief Get holidays by calendar name (AQL-safe)
     * 
     * This is the ONLY method that should be called from AQL queries.
     * It only accepts registered calendar names, not file paths.
     * 
     * @param name Calendar name (e.g., "DE_2024", "US_FEDERAL_2024")
     * @return Set of holiday timestamps
     * @throws std::runtime_error if calendar not found
     */
    std::set<int64_t> getHolidays(const std::string& name) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (!initialized_) {
            registerBuiltinCalendars();
            initialized_ = true;
        }
        
        // Validate name format (alphanumeric, underscore, hyphen only)
        validateName(name);
        
        // Look up calendar
        std::string upperName = toUpperCase(name);
        auto it = calendars_.find(upperName);
        if (it == calendars_.end()) {
            // Try without year suffix for recurring calendars
            it = calendars_.find(upperName);
            if (it == calendars_.end()) {
                throw std::runtime_error("HOLIDAYS: Calendar '" + name + "' not found. "
                    "Use LIST_CALENDARS() to see available calendars.");
            }
        }
        
        return it->second.holidays;
    }
    
    /**
     * @brief Merge multiple calendars (AQL-safe)
     * 
     * @param names List of calendar names
     * @return Combined set of holidays
     */
    std::set<int64_t> getMergedHolidays(const std::vector<std::string>& names) {
        std::set<int64_t> result;
        
        for (const auto& name : names) {
            auto holidays = getHolidays(name);
            result.insert(holidays.begin(), holidays.end());
        }
        
        return result;
    }
    
    /**
     * @brief List all available calendar names (AQL-safe)
     */
    std::vector<std::string> listCalendars() {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (!initialized_) {
            registerBuiltinCalendars();
            initialized_ = true;
        }
        
        std::vector<std::string> names = {};

        names.reserve(calendars_.size());
        
        for (const auto& [name, cal] : calendars_) {
            names.push_back(name);
        }
        
        std::sort(names.begin(), names.end());
        return names;
    }
    
    /**
     * @brief Get calendar metadata (AQL-safe)
     */
    std::optional<Calendar> getCalendarInfo(const std::string& name) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        std::string upperName = toUpperCase(name);
        auto it = calendars_.find(upperName);
        if (it == calendars_.end()) {
            return std::nullopt;
        }
        
        return it->second;
    }
    
    // =========================================================================
    // Admin API - Server startup only, NOT accessible from AQL
    // =========================================================================
    
    /**
     * @brief Register a calendar programmatically (Admin API)
     * 
     * Used at server startup to register custom calendars.
     * NOT accessible from AQL queries.
     * 
     * @param name Calendar name (uppercase recommended)
     * @param region Region code
     * @param year Year or 0 for recurring
     * @param holidays Set of date timestamps
     * @param description Human-readable description
     */
    void registerCalendar(const std::string& name, 
                          const std::string& region,
                          int year,
                          const std::set<int64_t>& holidays,
                          const std::string& description = "") {
        std::lock_guard<std::mutex> lock(mutex_);
        
        std::string upperName = toUpperCase(name);
        
        Calendar cal;
        cal.name = upperName;
        cal.region = region;
        cal.year = year;
        cal.holidays = holidays;
        cal.description = description;
        cal.isBuiltin = false;
        
        calendars_[upperName] = cal;
    }
    
    /**
     * @brief Load calendar from JSON file (Admin API)
     * 
     * Used at server startup to load calendars from files.
     * NOT accessible from AQL queries.
     * 
     * @param name Calendar name to register under
     * @param filePath Path to JSON file
     * @throws std::runtime_error if file invalid
     */
    void loadCalendarFromFile(const std::string& name, const std::string& filePath) {
        // Read file
        std::ifstream file(filePath);
        if (!file.is_open()) {
            throw std::runtime_error("Cannot open calendar file: " + filePath);
        }
        
        std::stringstream buffer = {};
        buffer << file.rdbuf();
        std::string content = buffer.str();
        file.close();
        
        // Parse JSON
        nlohmann::json doc;
        try {
            doc = nlohmann::json::parse(content);
        } catch (const nlohmann::json::exception& e) {
            throw std::runtime_error("Invalid JSON in calendar file: " + std::string(e.what()));
        }
        
        // Extract metadata
        std::string region = doc.value("region", "");
        int year = doc.value("year", 0);
        std::string description = doc.value("description", "");
        
        // Parse holidays
        std::set<int64_t> holidays;
        
        nlohmann::json holidaysArray;
        if (doc.contains("holidays") && doc["holidays"].is_array()) {
            holidaysArray = doc["holidays"];
        } else if (doc.is_array()) {
            holidaysArray = doc;
        } else {
            throw std::runtime_error("Calendar file must contain 'holidays' array");
        }
        
        for (const auto& item : holidaysArray) {
            if (item.is_string()) {
                int64_t ts = parseDateToTimestamp(item.get<std::string>());
                holidays.insert(ts);
            } else if (item.is_number()) {
                int64_t ts = item.get<int64_t>();
                ts = (ts / (24 * 60 * 60 * 1000)) * (24 * 60 * 60 * 1000);
                holidays.insert(ts);
            } else if (item.is_object() && item.contains("date")) {
                if (item["date"].is_string()) {
                    int64_t ts = parseDateToTimestamp(item["date"].get<std::string>());
                    holidays.insert(ts);
                }
            }
        }
        
        // Register
        registerCalendar(name, region, year, holidays, description);
    }
    
    /**
     * @brief Clear all custom calendars (Admin API)
     * 
     * Removes all non-builtin calendars.
     */
    void clearCustomCalendars() {
        std::lock_guard<std::mutex> lock(mutex_);
        
        for (auto it = calendars_.begin(); it != calendars_.end(); ) {
            if (!it->second.isBuiltin) {
                it = calendars_.erase(it);
            } else {
                ++it;
            }
        }
    }
    
    // =========================================================================
    // Utility functions
    // =========================================================================
    
    /**
     * @brief Convert holidays set to JSON array
     */
    static nlohmann::json toJsonArray(const std::set<int64_t>& holidays) {
        nlohmann::json arr = nlohmann::json::array();
        for (int64_t h : holidays) {
            arr.push_back(h);
        }
        return arr;
    }
    
    /**
     * @brief Parse date string to timestamp
     * 
     * Supports: YYYY-MM-DD, YYYY/MM/DD, DD.MM.YYYY
     */
    static int64_t parseDateToTimestamp(const std::string& dateStr) {
        std::tm tm = {};
        
        // Try YYYY-MM-DD
        if (std::regex_match(dateStr, std::regex(R"(\d{4}-\d{2}-\d{2})"))) {
            std::istringstream ss(dateStr);
            ss >> std::get_time(&tm, "%Y-%m-%d");
            if (!ss.fail()) {
                return tmToMs(tm);
            }
        }
        
        // Try YYYY/MM/DD
        if (std::regex_match(dateStr, std::regex(R"(\d{4}/\d{2}/\d{2})"))) {
            std::istringstream ss(dateStr);
            ss >> std::get_time(&tm, "%Y/%m/%d");
            if (!ss.fail()) {
                return tmToMs(tm);
            }
        }
        
        // Try DD.MM.YYYY (German format)
        if (std::regex_match(dateStr, std::regex(R"(\d{2}\.\d{2}\.\d{4})"))) {
            std::istringstream ss(dateStr);
            ss >> std::get_time(&tm, "%d.%m.%Y");
            if (!ss.fail()) {
                return tmToMs(tm);
            }
        }
        
        // Try timestamp (milliseconds)
        try {
            return std::stoll(dateStr);
        } catch (...) {}
        
        throw std::runtime_error("Invalid date format: " + dateStr);
    }
    
    /**
     * @brief Create timestamp from date components
     */
    static int64_t makeDate(int year, int month, int day) {
        std::tm tm = {};
        tm.tm_year = year - 1900;
        tm.tm_mon = month - 1;
        tm.tm_mday = day;
        tm.tm_hour = 0;
        tm.tm_min = 0;
        tm.tm_sec = 0;
        
        return tmToMs(tm);
    }

private:
    HolidayProvider() = default;
    ~HolidayProvider() = default;
    HolidayProvider(const HolidayProvider&) = delete;
    HolidayProvider& operator=(const HolidayProvider&) = delete;
    
    /**
     * @brief Register all built-in calendars
     */
    void registerBuiltinCalendars() {
        // Germany 2024
        registerBuiltinCalendar("DE_2024", "DE", 2024, "Germany Federal Holidays 2024", {
            makeDate(2024, 1, 1),   // Neujahr
            makeDate(2024, 3, 29),  // Karfreitag
            makeDate(2024, 4, 1),   // Ostermontag
            makeDate(2024, 5, 1),   // Tag der Arbeit
            makeDate(2024, 5, 9),   // Christi Himmelfahrt
            makeDate(2024, 5, 20),  // Pfingstmontag
            makeDate(2024, 10, 3),  // Tag der Deutschen Einheit
            makeDate(2024, 12, 25), // 1. Weihnachtstag
            makeDate(2024, 12, 26), // 2. Weihnachtstag
        });
        
        // Germany 2025
        registerBuiltinCalendar("DE_2025", "DE", 2025, "Germany Federal Holidays 2025", {
            makeDate(2025, 1, 1),   // Neujahr
            makeDate(2025, 4, 18),  // Karfreitag
            makeDate(2025, 4, 21),  // Ostermontag
            makeDate(2025, 5, 1),   // Tag der Arbeit
            makeDate(2025, 5, 29),  // Christi Himmelfahrt
            makeDate(2025, 6, 9),   // Pfingstmontag
            makeDate(2025, 10, 3),  // Tag der Deutschen Einheit
            makeDate(2025, 12, 25), // 1. Weihnachtstag
            makeDate(2025, 12, 26), // 2. Weihnachtstag
        });
        
        // Austria 2024
        registerBuiltinCalendar("AT_2024", "AT", 2024, "Austria Holidays 2024", {
            makeDate(2024, 1, 1),   // Neujahr
            makeDate(2024, 1, 6),   // Heilige Drei Könige
            makeDate(2024, 4, 1),   // Ostermontag
            makeDate(2024, 5, 1),   // Staatsfeiertag
            makeDate(2024, 5, 9),   // Christi Himmelfahrt
            makeDate(2024, 5, 20),  // Pfingstmontag
            makeDate(2024, 5, 30),  // Fronleichnam
            makeDate(2024, 8, 15),  // Mariä Himmelfahrt
            makeDate(2024, 10, 26), // Nationalfeiertag
            makeDate(2024, 11, 1),  // Allerheiligen
            makeDate(2024, 12, 8),  // Mariä Empfängnis
            makeDate(2024, 12, 25), // Christtag
            makeDate(2024, 12, 26), // Stefanitag
        });
        
        // Switzerland 2024
        registerBuiltinCalendar("CH_2024", "CH", 2024, "Switzerland Federal Holidays 2024", {
            makeDate(2024, 1, 1),   // Neujahr
            makeDate(2024, 3, 29),  // Karfreitag
            makeDate(2024, 4, 1),   // Ostermontag
            makeDate(2024, 5, 9),   // Auffahrt
            makeDate(2024, 5, 20),  // Pfingstmontag
            makeDate(2024, 8, 1),   // Bundesfeier
            makeDate(2024, 12, 25), // Weihnachten
            makeDate(2024, 12, 26), // Stephanstag
        });
        
        // US Federal 2024
        registerBuiltinCalendar("US_FEDERAL_2024", "US", 2024, "US Federal Holidays 2024", {
            makeDate(2024, 1, 1),   // New Year's Day
            makeDate(2024, 1, 15),  // Martin Luther King Jr. Day
            makeDate(2024, 2, 19),  // Presidents' Day
            makeDate(2024, 5, 27),  // Memorial Day
            makeDate(2024, 6, 19),  // Juneteenth
            makeDate(2024, 7, 4),   // Independence Day
            makeDate(2024, 9, 2),   // Labor Day
            makeDate(2024, 10, 14), // Columbus Day
            makeDate(2024, 11, 11), // Veterans Day
            makeDate(2024, 11, 28), // Thanksgiving Day
            makeDate(2024, 12, 25), // Christmas Day
        });
        
        // US Federal 2025
        registerBuiltinCalendar("US_FEDERAL_2025", "US", 2025, "US Federal Holidays 2025", {
            makeDate(2025, 1, 1),   // New Year's Day
            makeDate(2025, 1, 20),  // Martin Luther King Jr. Day
            makeDate(2025, 2, 17),  // Presidents' Day
            makeDate(2025, 5, 26),  // Memorial Day
            makeDate(2025, 6, 19),  // Juneteenth
            makeDate(2025, 7, 4),   // Independence Day
            makeDate(2025, 9, 1),   // Labor Day
            makeDate(2025, 10, 13), // Columbus Day
            makeDate(2025, 11, 11), // Veterans Day
            makeDate(2025, 11, 27), // Thanksgiving Day
            makeDate(2025, 12, 25), // Christmas Day
        });
        
        // UK 2024
        registerBuiltinCalendar("UK_2024", "GB", 2024, "UK Bank Holidays 2024", {
            makeDate(2024, 1, 1),   // New Year's Day
            makeDate(2024, 3, 29),  // Good Friday
            makeDate(2024, 4, 1),   // Easter Monday
            makeDate(2024, 5, 6),   // Early May Bank Holiday
            makeDate(2024, 5, 27),  // Spring Bank Holiday
            makeDate(2024, 8, 26),  // Summer Bank Holiday
            makeDate(2024, 12, 25), // Christmas Day
            makeDate(2024, 12, 26), // Boxing Day
        });
        
        // France 2024
        registerBuiltinCalendar("FR_2024", "FR", 2024, "France Public Holidays 2024", {
            makeDate(2024, 1, 1),   // Jour de l'an
            makeDate(2024, 4, 1),   // Lundi de Pâques
            makeDate(2024, 5, 1),   // Fête du Travail
            makeDate(2024, 5, 8),   // Victoire 1945
            makeDate(2024, 5, 9),   // Ascension
            makeDate(2024, 5, 20),  // Lundi de Pentecôte
            makeDate(2024, 7, 14),  // Fête nationale
            makeDate(2024, 8, 15),  // Assomption
            makeDate(2024, 11, 1),  // Toussaint
            makeDate(2024, 11, 11), // Armistice
            makeDate(2024, 12, 25), // Noël
        });
        
        // Empty calendar for testing
        registerBuiltinCalendar("NONE", "", 0, "No holidays (empty calendar)", {});
        
        // Weekend-only (no public holidays)
        registerBuiltinCalendar("WEEKENDS_ONLY", "", 0, "Only weekends, no public holidays", {});
    }
    
    void registerBuiltinCalendar(const std::string& name,
                                  const std::string& region,
                                  int year,
                                  const std::string& description,
                                  const std::set<int64_t>& holidays) {
        Calendar cal;
        cal.name = name;
        cal.region = region;
        cal.year = year;
        cal.description = description;
        cal.holidays = holidays;
        cal.isBuiltin = true;
        
        calendars_[name] = cal;
    }
    
    /**
     * @brief Validate calendar name (injection protection)
     */
    void validateName(const std::string& name) {
        if (name.empty()) {
            throw std::runtime_error("HOLIDAYS: Calendar name cannot be empty");
        }
        
        if (name.length() > 100) {
            throw std::runtime_error("HOLIDAYS: Calendar name too long");
        }
        
        // Only allow alphanumeric, underscore, hyphen
        static const std::regex validName(R"(^[a-zA-Z0-9_\-]+$)");
        if (!std::regex_match(name, validName)) {
            throw std::runtime_error("HOLIDAYS: Invalid calendar name '" + name + 
                "'. Only alphanumeric, underscore, and hyphen allowed.");
        }
    }
    
    static std::string toUpperCase(const std::string& str) {
        std::string result = str;
        std::transform(result.begin(), result.end(), result.begin(), ::toupper);
        return result;
    }
    
    static int64_t tmToMs(const std::tm& tm) {
        std::tm copy = tm;
        copy.tm_hour = 0;
        copy.tm_min = 0;
        copy.tm_sec = 0;
        
#ifdef _WIN32
        time_t t = _mkgmtime(&copy);
#else
        time_t t = timegm(&copy);
#endif
        
        return static_cast<int64_t>(t) * 1000;
    }
    
    std::mutex mutex_;
    bool initialized_ = false;
    std::map<std::string, Calendar> calendars_;
};

} // namespace functions
} // namespace query
} // namespace themis
