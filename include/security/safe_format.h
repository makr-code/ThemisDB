#ifndef THEMIS_SECURITY_SAFE_FORMAT_H
#define THEMIS_SECURITY_SAFE_FORMAT_H

#include <cstdio>
#include <cstdarg>
#include <string>
#include <memory>
#include <spdlog/fmt/fmt.h>

namespace themis::security {

/**
 * @brief Safe printf-style formatting wrapper to prevent format string vulnerabilities.
 *
 * This module provides safe alternatives to standard printf-family functions that protect
 * against format string attacks (CWE-134). Format string vulnerabilities occur when
 * untrusted user input is passed directly as a format string to printf(), sprintf(), etc.
 *
 * **Key Safety Features:**
 * - Enforces compile-time format string validation
 * - Provides runtime bounds checking for buffers
 * - Uses type-safe fmt library internally for value formatting
 * - Prevents %x, %n, and other dangerous format specifiers from user input
 *
 * **Usage Pattern:**
 * ```cpp
 * // UNSAFE - Don't do this:
 * std::string user_input = get_user_data();
 * printf(user_input.c_str());  // Format string vulnerability!
 *
 * // SAFE - Use these wrappers:
 * std::string user_input = get_user_data();
 * SafeFormat::print_string(user_input);  // Safe
 * SafeFormat::sprintf_safe("User: %s", user_input.c_str());  // Safe
 * ```
 *
 * **CWE References:**
 * - CWE-134: Use of Externally-Controlled Format String
 * - OWASP: Format String Attack Prevention
 */
class SafeFormat {
public:
    /**
     * @brief Safely print a string with no format specifiers interpretation.
     * @param text The string to print (user input is safe here).
     * @return Number of characters printed, or -1 on error.
     */
    static int print_string(const std::string& text);

    /**
     * @brief Safely print to stdout with a fixed format string and variadic arguments.
     * @param format The format string (must be compile-time constant).
     * @param ... Arguments matching the format string specification.
     * @return Number of characters printed, or -1 on error.
     * 
     * **Template specialization ensures format string is known at compile time.**
     */
    template<typename... Args>
    static int printf_safe(fmt::format_string<Args...> format, Args&&... args) {
        // This is a compile-time format string; passed to fmt library for type safety
        try {
            std::string result = fmt::format(format, std::forward<Args>(args)...);
            return std::fputs(result.c_str(), stdout);
        } catch (const std::exception& e) {
            std::fputs("ERROR: Format string error\n", stderr);
            return -1;
        }
    }

    /**
     * @brief Safely print to a buffer with bounds checking.
     * @param buffer Output buffer.
     * @param buffer_size Size of the output buffer.
     * @param format The format string (must be compile-time constant).
     * @param ... Arguments matching the format string specification.
     * @return Number of characters written (excluding null terminator), or -1 on error.
     * 
     * **Prevents buffer overflow by enforcing buffer_size limits.**
     */
    template<typename... Args>
    static int snprintf_safe(char* buffer, size_t buffer_size, 
                            fmt::format_string<Args...> format, Args&&... args) {
        if (!buffer || buffer_size == 0) {
            return -1;
        }
        
        try {
            std::string result = fmt::format(format, std::forward<Args>(args)...);
            if (result.length() >= buffer_size) {
                // Truncate to fit buffer
                result = result.substr(0, buffer_size - 1);
            }
            std::strncpy(buffer, result.c_str(), buffer_size - 1);
            buffer[buffer_size - 1] = '\0';
            return result.length();
        } catch (const std::exception& e) {
            std::snprintf(buffer, buffer_size, "ERROR: Format error");
            return -1;
        }
    }

    /**
     * @brief Safely format to a string.
     * @param format The format string (must be compile-time constant).
     * @param ... Arguments matching the format string specification.
     * @return Formatted string result.
     */
    template<typename... Args>
    static std::string format_safe(fmt::format_string<Args...> format, Args&&... args) {
        try {
            return fmt::format(format, std::forward<Args>(args)...);
        } catch (const std::exception& e) {
            return std::string("ERROR: ") + e.what();
        }
    }

    /**
     * @brief Runtime-format variant: accepts non-constexpr format strings.
     * Use this when the format string is not a compile-time literal or when
     * intentionally testing malformed/missing argument scenarios.
     */
    template<typename... Args>
    static std::string format_safe_runtime(const std::string& format, Args&&... args) {
        try {
            return fmt::format(fmt::runtime(format), std::forward<Args>(args)...);
        } catch (const std::exception& e) {
            return std::string("ERROR: ") + e.what();
        }
    }

    /**
     * @brief Safely print to a file stream with a fixed format string.
     * @param stream Output file stream.
     * @param format The format string (must be compile-time constant).
     * @param ... Arguments matching the format string specification.
     * @return Number of characters written, or -1 on error.
     */
    template<typename... Args>
    static int fprintf_safe(FILE* stream, fmt::format_string<Args...> format, Args&&... args) {
        if (!stream) {
            return -1;
        }
        
        try {
            std::string result = fmt::format(format, std::forward<Args>(args)...);
            return std::fputs(result.c_str(), stream);
        } catch (const std::exception& e) {
            std::fputs("ERROR: Format string error\n", stderr);
            return -1;
        }
    }

    /**
     * @brief Validate that a string is safe to display (no control characters, etc).
     * @param text The text to validate.
     * @return Sanitized version of the text with dangerous chars escaped.
     */
    static std::string escape_for_display(const std::string& text);

    /**
     * @brief Safely log a user-controlled message with proper escaping.
     * @param message The user message to log.
     * @param context Optional context label for the log.
     */
    static void log_user_message(const std::string& message, 
                                 const std::string& context = "user-input");
};

}  // namespace themis::security

#endif  // THEMIS_SECURITY_SAFE_FORMAT_H
