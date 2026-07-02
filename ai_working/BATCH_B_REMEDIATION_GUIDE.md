# Batch B Remediation Guide: Format String + ReDoS Patterns

**Status:** Implementation Guide  
**Date:** 2026-07-02  
**Applies to:** 202 gaps (93 format string + 109 ReDoS)

---

## Part 1: Format String Remediation (CWE-134)

### Pattern Overview

**Vulnerability:** User-controlled data used directly as format string to printf-style functions.

**Risk:** Information disclosure, memory corruption, code execution.

---

### Remediation Pattern B1.1: Basic Printf Replacement

**Before (Vulnerable):**
```cpp
std::string user_msg = get_user_input();
printf(user_msg.c_str());  // Format string vulnerability!
```

**After (Remediated):**
```cpp
#include "security/safe_format.h"
using namespace themis::security;

std::string user_msg = get_user_input();
SafeFormat::print_string(user_msg);  // Safe: no format specifiers interpreted
```

**Modules with this pattern:**
- `src/query/query_profiler.cpp` (~15 occurrences)
- `src/security/audit_logger.cpp` (~12 occurrences)
- `src/analytics/event_processor.cpp` (~10 occurrences)

---

### Remediation Pattern B1.2: Sprintf with Buffer Bounds

**Before (Vulnerable):**
```cpp
char buffer[256];
std::string event_name = get_user_event();
sprintf(buffer, event_name.c_str(), additional_args);  // No bounds check!
```

**After (Remediated):**
```cpp
#include "security/safe_format.h"
using namespace themis::security;

char buffer[256];
std::string event_name = get_user_event();
SafeFormat::snprintf_safe(buffer, sizeof(buffer), "Event: {}", event_name);
```

**Key Changes:**
- Use `snprintf_safe` instead of `sprintf`
- Provide buffer size parameter
- Use fmt-style `{}` placeholders for type safety

**Modules with this pattern:**
- `src/api/rest_handler.cpp` (~8 occurrences)
- `src/server/http_server.cpp` (~7 occurrences)
- `src/utils/logger.cpp` (~6 occurrences)

---

### Remediation Pattern B1.3: Logging with User Input

**Before (Vulnerable):**
```cpp
std::string user_query = parse_query_string();
spdlog::info(user_query);  // If user_query contains format specifiers
// or
logger.log_event(error_msg);  // If implementation uses printf internally
```

**After (Remediated):**
```cpp
#include "security/safe_format.h"
using namespace themis::security;

std::string user_query = parse_query_string();
spdlog::info("Query: {}", user_query);  // Safe: user input as argument
// or
std::string safe_msg = SafeFormat::escape_for_display(error_msg);
logger.log_event(safe_msg);
```

**Key Changes:**
- Always provide explicit format string (never user-controlled)
- Pass user input as arguments, not format string
- Use escape functions for display purposes

**Modules with this pattern:**
- `src/query/query_executor.cpp` (~8 occurrences)
- `src/security/input_validator.cpp` (~6 occurrences)

---

### Remediation Pattern B1.4: Multiple Arguments with Format Control

**Before (Vulnerable):**
```cpp
printf(format_from_user, arg1, arg2);  // Dangerous!
```

**After (Remediated):**
```cpp
#include "security/safe_format.h"
using namespace themis::security;

// Option A: If format is hardcoded
SafeFormat::printf_safe("Format: {}, {}", arg1, arg2);

// Option B: If format needs to be flexible, use controlled templates
std::string safe_format = SafeFormat::get_safe_template("query_result");
SafeFormat::printf_safe(safe_format.c_str(), arg1, arg2);
```

**Modules with this pattern:**
- `src/analytics/filter_engine.cpp` (~9 occurrences)

---

## Part 2: ReDoS Remediation (CWE-1333)

### Pattern Overview

**Vulnerability:** Regular expressions with catastrophic backtracking when applied to untrusted input.

**Risk:** Denial of service, resource exhaustion, application hang.

---

### Remediation Pattern B2.1: Input Validation Before Regex

**Before (Vulnerable):**
```cpp
std::string user_pattern = get_user_regex();
std::regex re(user_pattern);  // User pattern, dangerous!
if (std::regex_search(text, re)) {
    // Process match
}
```

**After (Remediated):**
```cpp
#include "security/safe_regex.h"
using namespace themis::security;

std::string user_pattern = get_user_regex();

// Step 1: Validate input length
if (!SafeRegex::validate_input(user_pattern)) {
    throw std::runtime_error("Input too long for regex");
}

// Step 2: Check if pattern is safe
if (!SafeRegex::is_pattern_safe(user_pattern)) {
    throw std::runtime_error("Pattern contains dangerous constructs");
}

// Step 3: Match with timeout protection
SafeRegex safe_regex(5);  // 5 second timeout
if (safe_regex.search(user_pattern, text)) {
    // Process match
}
```

**Modules with this pattern:**
- `src/query/query_validator.cpp` (~18 occurrences)
- `src/security/input_validator.cpp` (~15 occurrences)

---

### Remediation Pattern B2.2: Hardcoded Pattern with Timeout

**Before (Potentially Vulnerable):**
```cpp
std::regex email_pattern("^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\\.[a-zA-Z]{2,}$");
if (std::regex_search(user_email, email_pattern)) {
    // Validate email
}
```

**After (Remediated):**
```cpp
#include "security/safe_regex.h"
using namespace themis::security;

static SafeRegex safe_regex(5);  // 5 second timeout
static const std::string EMAIL_PATTERN = 
    "^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\\.[a-zA-Z]{2,}$";

try {
    if (safe_regex.search(EMAIL_PATTERN, user_email, 
                         std::chrono::seconds(5))) {
        // Validate email
    }
} catch (const std::exception& e) {
    SPDLOG_ERROR("Email validation failed: {}", e.what());
}
```

**Modules with this pattern:**
- `src/analytics/filter_engine.cpp` (~12 occurrences)

---

### Remediation Pattern B2.3: Nested Quantifier Removal

**Before (Vulnerable):**
```cpp
// User provides pattern like "(a+)+" or "(a|ab)*"
std::regex pattern(user_pattern);  // ReDoS risk!
std::regex_search(text, pattern);
```

**After (Remediated):**
```cpp
#include "security/safe_regex.h"
using namespace themis::security;

std::string user_pattern = get_user_pattern();

// Sanitize pattern: remove nested quantifiers
std::string sanitized = SafeRegex::simplify_pattern(user_pattern);

// Or reject patterns with dangerous constructs
if (!SafeRegex::is_pattern_safe(user_pattern)) {
    throw std::runtime_error("Pattern contains nested quantifiers");
}

SafeRegex safe_regex;
safe_regex.search(user_pattern, text);
```

**Common Dangerous Patterns:**
```
(a+)+           → (a+)
(a*)*           → a*
(a|a)*          → a*
(a|ab)+         → (a|ab)+ [can be slow on certain input]
(a|a|a)*        → a*
[a-z]+[a-z]+*   → [a-z]+
```

**Modules with this pattern:**
- `src/api/request_parser.cpp` (~10 occurrences)
- `src/search/search_engine.cpp` (~9 occurrences)

---

### Remediation Pattern B2.4: Regex Caching for Performance

**Before (Inefficient):**
```cpp
for (const auto& item : items) {
    std::regex pattern("^item_[0-9]+$");  // Recompiled each iteration!
    if (std::regex_search(item, pattern)) {
        process(item);
    }
}
```

**After (Remediated):**
```cpp
#include "security/safe_regex.h"
using namespace themis::security;

static SafeRegex safe_regex;  // Shared instance with cache

for (const auto& item : items) {
    if (safe_regex.search("^item_[0-9]+$", item)) {
        process(item);
    }
}
```

**Benefit:** Compiled patterns are cached internally, reducing CPU overhead.

**Modules with this pattern:**
- `src/importers/csv_parser.cpp` (~8 occurrences)

---

### Remediation Pattern B2.5: Complex Pattern Simplification

**Before (Potentially Slow):**
```cpp
// User-supplied complex pattern
std::string pattern = user_pattern;  // Could be "(.+)+@(.+)+" 
std::regex re(pattern);
std::regex_search(email, re);
```

**After (Remediated with Simplification):**
```cpp
#include "security/safe_regex.h"
using namespace themis::security;

std::string user_pattern = get_user_pattern();

// Option 1: Use pre-approved patterns instead
std::string approved_pattern = SafeRegex::get_approved_pattern("email");
SafeRegex safe_regex;
safe_regex.search(approved_pattern, user_email);

// Option 2: Simplify complex patterns
std::string simplified = SafeRegex::simplify_pattern(user_pattern);
SafeRegex safe_regex;
safe_regex.search(simplified, user_email);
```

---

## Part 3: Implementation Checklist

### For Each Identified Gap

#### Format String (B1):
- [ ] Locate the vulnerable printf/sprintf/logging call
- [ ] Identify the user-controlled input
- [ ] Replace with appropriate SafeFormat function:
  - `printf(user_input)` → `SafeFormat::print_string(user_input)`
  - `sprintf(buf, fmt, user_var)` → `SafeFormat::snprintf_safe(buf, size, fmt, user_var)`
  - `syslog(user_input)` → `spdlog::info("{}", user_input)` or `SafeFormat::log_user_message(user_input)`
- [ ] Add `#include "security/safe_format.h"`
- [ ] Test the change (compile + run existing tests)
- [ ] Verify format specifiers are NOT interpreted from user input

#### ReDoS (B2):
- [ ] Locate the vulnerable regex call
- [ ] Identify the input pattern source (user-controlled vs. hardcoded)
- [ ] Apply appropriate SafeRegex remediation:
  - If user-controlled pattern: Add validation
  - If hardcoded pattern: Add timeout
  - If complex pattern: Simplify or reject
- [ ] Add `#include "security/safe_regex.h"`
- [ ] Wrap with timeout protection
- [ ] Test with pathological inputs (e.g., "aaaa...aaaa" for (a+)+ patterns)
- [ ] Verify timeout mechanism works

---

## Part 4: Testing Strategy

### For Format String Changes

```cpp
// Test case to add to test file:
TEST(FormatStringRemediationTest, UserInput_NoFormatSpecifiers) {
    std::string vulnerable_input = "Read memory: %x %x %x";
    std::string output = capture_output([&]() {
        SafeFormat::print_string(vulnerable_input);
    });
    
    // Should print literally, not interpret format specifiers
    EXPECT_THAT(output, testing::HasSubstr("%x"));
}
```

### For ReDoS Changes

```cpp
// Test case to add to test file:
TEST(ReDoSRemediationTest, NestedQuantifier_Blocked) {
    std::string dangerous_pattern = "(a+)+";
    std::string pathological_input = std::string(100, 'a');
    
    EXPECT_FALSE(SafeRegex::is_pattern_safe(dangerous_pattern));
    
    // Or verify timeout works:
    SafeRegex regex(1);  // 1 second timeout
    try {
        regex.search(dangerous_pattern, pathological_input);
        FAIL() << "Expected timeout or exception";
    } catch (const std::runtime_error& e) {
        EXPECT_THAT(e.what(), testing::HasSubstr("timeout|unsafe"));
    }
}
```

---

## Part 5: Module-by-Module Implementation Order

### Priority 1 (High Risk, High Impact)
1. `src/query/query_planner.cpp` - 15 format string gaps
2. `src/security/audit_logger.cpp` - 12 format string gaps
3. `src/query/query_validator.cpp` - 18 ReDoS gaps
4. `src/security/input_validator.cpp` - 15 format string + 15 ReDoS gaps

### Priority 2 (Medium Risk)
5. `src/analytics/event_processor.cpp` - 10 format string gaps
6. `src/analytics/filter_engine.cpp` - 9 ReDoS + 9 format string gaps
7. `src/api/rest_handler.cpp` - 8 format string gaps
8. `src/api/request_parser.cpp` - 10 ReDoS gaps

### Priority 3 (Lower Risk)
9. `src/server/http_server.cpp` - 7 format string gaps
10. `src/search/search_engine.cpp` - 9 ReDoS gaps
... (remaining modules)

---

## Part 6: Validation & Sign-Off

### Pre-Merge Checklist
- [ ] All identified format string gaps have SafeFormat integration
- [ ] All identified ReDoS gaps have SafeRegex + timeout integration
- [ ] New integration tests pass (test_safe_format.cpp, test_safe_regex.cpp)
- [ ] Existing tests pass (no regressions)
- [ ] No compiler warnings introduced
- [ ] Code review: Format/ReDoS patterns verified
- [ ] Security audit: No new vulnerabilities introduced

### Performance Validation
- [ ] SafeFormat overhead < 5% (negligible for logging)
- [ ] SafeRegex timeout protection working (5-second default)
- [ ] Regex cache hit rate > 80% in typical workloads

---

## Part 7: Documentation Updates

After implementing Batch B, update:
1. `CHANGELOG.md` - Add entry for Batch B remediation
2. `ROADMAP.md` - Mark Batch B complete, 100 gaps remediated
3. `docs/SECURITY.md` - Document SafeFormat and SafeRegex requirements
4. `docs/CODE_PATTERNS.md` - Add do's and don'ts for format strings and regex

---

## Reference: SafeFormat API

```cpp
namespace themis::security {
    class SafeFormat {
    public:
        // Print without interpreting format specifiers
        static int print_string(const std::string& text);
        
        // Safe printf with controlled format (compile-time constant required)
        template<typename... Args>
        static int printf_safe(const char* format, Args&&... args);
        
        // Safe sprintf with buffer bounds checking
        template<typename... Args>
        static int snprintf_safe(char* buffer, size_t size, 
                                const char* format, Args&&... args);
        
        // Escape control/special characters for display
        static std::string escape_for_display(const std::string& input);
        
        // Log user message safely
        static void log_user_message(const std::string& msg, 
                                    const std::string& context);
        
        // Type-safe formatting
        static std::string format_safe(const std::string& fmt, ...);
    };
}
```

## Reference: SafeRegex API

```cpp
namespace themis::security {
    class SafeRegex {
    public:
        explicit SafeRegex(size_t timeout_seconds = 5);
        
        // Check if pattern is safe (no nested quantifiers, etc.)
        static bool is_pattern_safe(const std::string& pattern);
        
        // Validate input length
        static bool validate_input(const std::string& text, 
                                  size_t max_length = 10000);
        
        // Match with timeout protection
        bool match(const std::string& pattern, const std::string& text,
                  std::chrono::milliseconds timeout = 
                      std::chrono::milliseconds(0));
        
        // Search with timeout
        bool search(const std::string& pattern, const std::string& text,
                   std::chrono::milliseconds timeout = 
                       std::chrono::milliseconds(0));
        
        // Replace with pattern safety checks
        std::string replace(const std::string& pattern, 
                           const std::string& text,
                           const std::string& replacement);
        
        // Split on pattern
        std::vector<std::string> split(const std::string& pattern,
                                      const std::string& text);
        
        // Simplify dangerous patterns
        static std::string simplify_pattern(const std::string& pattern);
        
        // Cache management
        void clear_cache();
        std::string cache_stats() const;
    };
}
```

---

*Batch B Remediation Guide - ThemisDB Security Initiative*  
*Created: 2026-07-02*  
*Target: v1.5.0 (2026-08-31)*
