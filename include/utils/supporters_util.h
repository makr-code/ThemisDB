/**
 * @file supporters_util.h
 * @brief Utilities for displaying supporter acknowledgments
 * @version 1.0
 * @note Maturity: 🟢 PRODUCTION-READY
 */

#ifndef THEMIS_UTILS_SUPPORTERS_UTIL_H
#define THEMIS_UTILS_SUPPORTERS_UTIL_H

#include <string>
#include <string_view>

namespace themis::utils {

    /**
     * @brief Get the formatted supporter acknowledgment message
     * @return Formatted message suitable for display at startup
     */
    std::string get_supporter_message();

    /**
     * @brief Get the raw comma-separated list of supporters
     * @return Comma-separated string of supporter names
     */
    std::string get_supporters_list();

} // namespace themis::utils

#endif // THEMIS_UTILS_SUPPORTERS_UTIL_H
