/**
 * @file supporters_util.cpp
 * @brief Implementation of supporter acknowledgment utilities
 * @version 1.0
 * @note Maturity: 🟢 PRODUCTION-READY
 */

#include "utils/supporters_util.h"
#include "generated/supporters_list.h"
#include <sstream>

namespace themis::utils {

std::string get_supporters_list() {
    return themis::decode_supporters();
}

std::string get_supporter_message() {
    std::ostringstream oss;
    auto supporters = get_supporters_list();
    
    oss << "\n╔════════════════════════════════════════════════════════════════╗\n"
        << "║                 🙏 Thank You to Our Supporters 🙏              ║\n"
        << "╚════════════════════════════════════════════════════════════════╝\n"
        << "We are deeply grateful for the support of:\n"
        << supporters << "\n"
        << "═══════════════════════════════════════════════════════════════════\n";
    
    return oss.str();
}

} // namespace themis::utils
