/**
 * @file supporters_util.cpp
 * @brief Implementation of supporter acknowledgment utilities
 * @version 1.0
 * @note Maturity: 🟢 PRODUCTION-READY
 */

#include "utils/supporters_util.h"
#include "generated/supporters_list.h"

namespace themis::utils {

std::string get_supporters_list() {
    return themis::decode_supporters();
}

std::string get_supporter_message() {
    auto supporters = get_supporters_list();
    return "Thank you to: " + supporters;
}

} // namespace themis::utils
