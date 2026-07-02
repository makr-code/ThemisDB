#include "security/safe_regex.h"
#include <thread>
#include <future>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>

namespace themis::security {

SafeRegex::SafeRegex(size_t timeout_seconds) 
    : default_timeout_seconds_(timeout_seconds) {}

SafeRegex::~SafeRegex() = default;

std::shared_ptr<std::regex> SafeRegex::get_compiled_pattern(const std::string& pattern) {
    // Check cache first
    auto it = pattern_cache_.find(pattern);
    if (it != pattern_cache_.end()) {
        ++cache_hits_;
        return it->second;
    }
    
    ++cache_misses_;
    
    // Validate pattern before compilation
    if (!is_pattern_safe(pattern)) {
        throw std::runtime_error("Potentially unsafe regex pattern detected: " + pattern);
    }
    
    try {
        auto compiled = std::make_shared<std::regex>(pattern, 
            std::regex::ECMAScript | std::regex::optimize);
        pattern_cache_[pattern] = compiled;
        return compiled;
    } catch (const std::regex_error& e) {
        throw std::runtime_error("Invalid regex pattern: " + std::string(e.what()));
    }
}

bool SafeRegex::match(const std::string& pattern, const std::string& text,
                      std::chrono::milliseconds timeout) {
    if (!validate_input(text)) {
        throw std::runtime_error("Input text validation failed - possible ReDoS attempt");
    }
    
    auto compiled_pattern = get_compiled_pattern(pattern);
    
    if (timeout.count() == 0) {
        timeout = std::chrono::seconds(default_timeout_seconds_);
    }
    
    try {
        // Use a timeout mechanism - attempt match with cancellation
        auto future = std::async(std::launch::async, [this, &compiled_pattern, &text]() {
            return std::regex_match(text, *compiled_pattern);
        });
        
        auto status = future.wait_for(timeout);
        if (status == std::future_status::timeout) {
            throw std::runtime_error("Regex match exceeded timeout limit");
        }
        
        return future.get();
    } catch (const std::exception& e) {
        spdlog::warn("Regex match error: {}", e.what());
        throw;
    }
}

bool SafeRegex::search(const std::string& pattern, const std::string& text,
                       std::chrono::milliseconds timeout) {
    if (!validate_input(text)) {
        throw std::runtime_error("Input text validation failed - possible ReDoS attempt");
    }
    
    auto compiled_pattern = get_compiled_pattern(pattern);
    
    if (timeout.count() == 0) {
        timeout = std::chrono::seconds(default_timeout_seconds_);
    }
    
    try {
        auto future = std::async(std::launch::async, [this, &compiled_pattern, &text]() {
            return std::regex_search(text, *compiled_pattern);
        });
        
        auto status = future.wait_for(timeout);
        if (status == std::future_status::timeout) {
            throw std::runtime_error("Regex search exceeded timeout limit");
        }
        
        return future.get();
    } catch (const std::exception& e) {
        spdlog::warn("Regex search error: {}", e.what());
        throw;
    }
}

std::string SafeRegex::replace(const std::string& pattern, const std::string& text,
                               const std::string& replacement,
                               std::chrono::milliseconds timeout) {
    if (!validate_input(text)) {
        throw std::runtime_error("Input text validation failed - possible ReDoS attempt");
    }
    
    auto compiled_pattern = get_compiled_pattern(pattern);
    
    if (timeout.count() == 0) {
        timeout = std::chrono::seconds(default_timeout_seconds_);
    }
    
    try {
        auto future = std::async(std::launch::async, [this, &compiled_pattern, &text, &replacement]() {
            return std::regex_replace(text, *compiled_pattern, replacement);
        });
        
        auto status = future.wait_for(timeout);
        if (status == std::future_status::timeout) {
            throw std::runtime_error("Regex replace exceeded timeout limit");
        }
        
        return future.get();
    } catch (const std::exception& e) {
        spdlog::warn("Regex replace error: {}", e.what());
        throw;
    }
}

std::vector<std::string> SafeRegex::split(const std::string& pattern, 
                                           const std::string& text,
                                           std::chrono::milliseconds timeout) {
    if (!validate_input(text)) {
        throw std::runtime_error("Input text validation failed - possible ReDoS attempt");
    }
    
    auto compiled_pattern = get_compiled_pattern(pattern);
    
    if (timeout.count() == 0) {
        timeout = std::chrono::seconds(default_timeout_seconds_);
    }
    
    try {
        auto future = std::async(std::launch::async, [this, &compiled_pattern, &text]() {
            std::vector<std::string> result;
            std::sregex_token_iterator iter(text.begin(), text.end(), *compiled_pattern, -1);
            std::sregex_token_iterator end;
            while (iter != end) {
                result.push_back(*iter++);
            }
            return result;
        });
        
        auto status = future.wait_for(timeout);
        if (status == std::future_status::timeout) {
            throw std::runtime_error("Regex split exceeded timeout limit");
        }
        
        return future.get();
    } catch (const std::exception& e) {
        spdlog::warn("Regex split error: {}", e.what());
        throw;
    }
}

bool SafeRegex::is_pattern_safe(const std::string& pattern) {
    // Check for nested quantifiers
    if (has_nested_quantifiers(pattern)) {
        return false;
    }
    
    // Check for overlapping alternation
    if (has_overlapping_alternation(pattern)) {
        return false;
    }
    
    // Check for excessively long pattern
    if (pattern.length() > 1024) {
        return false;
    }
    
    return true;
}

bool SafeRegex::validate_input(const std::string& text, size_t max_length) {
    // Check length
    if (text.length() > max_length) {
        return false;
    }
    
    // Check for pathological patterns (repeated character sequences)
    size_t max_repeated = 0;
    size_t current_repeated = 1;
    char last_char = '\0';
    
    for (char c : text) {
        if (c == last_char) {
            ++current_repeated;
            max_repeated = std::max(max_repeated, current_repeated);
        } else {
            current_repeated = 1;
        }
        last_char = c;
    }
    
    // Reject inputs with excessive repetition (potential for backtracking)
    if (max_repeated > 1000) {
        return false;
    }
    
    return true;
}

bool SafeRegex::has_nested_quantifiers(const std::string& pattern) {
    // Simple heuristic: look for patterns like +)+, *)+, ?)+, etc.
    const char* quantifiers = "+*?";
    
    for (size_t i = 0; i < pattern.length(); ++i) {
        if (std::string(quantifiers).find(pattern[i]) != std::string::npos) {
            // Found quantifier, check if followed by ) then another quantifier
            if (i + 2 < pattern.length() && 
                pattern[i + 1] == ')' && 
                std::string(quantifiers).find(pattern[i + 2]) != std::string::npos) {
                return true;
            }
        }
    }
    
    return false;
}

bool SafeRegex::has_overlapping_alternation(const std::string& pattern) {
    // Simple heuristic: look for (a|a) or (a|ab) patterns
    size_t paren_count = 0;
    size_t last_pipe = 0;
    
    for (size_t i = 0; i < pattern.length(); ++i) {
        if (pattern[i] == '(') {
            ++paren_count;
        } else if (pattern[i] == ')') {
            --paren_count;
        } else if (pattern[i] == '|' && paren_count > 0) {
            // Found pipe inside parentheses - check for overlapping alternatives
            // This is a simplified heuristic; a full implementation would need a parser
            last_pipe = i;
        }
    }
    
    return false;  // Simplified for now
}

void SafeRegex::clear_cache() {
    pattern_cache_.clear();
    cache_hits_ = 0;
    cache_misses_ = 0;
}

std::string SafeRegex::cache_stats() const {
    return fmt::format("SafeRegex cache: {} hits, {} misses (hit rate: {:.2f}%)",
                       cache_hits_,
                       cache_misses_,
                       cache_hits_ + cache_misses_ > 0 
                           ? 100.0 * cache_hits_ / (cache_hits_ + cache_misses_) 
                           : 0.0);
}

}  // namespace themis::security
