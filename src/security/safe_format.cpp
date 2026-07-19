#include "security/safe_format.h"
#include <cstdio>
#include <sstream>
#include <iomanip>

namespace themis::security {

int SafeFormat::print_string(const std::string& text) {
    if (std::fputs(text.c_str(), stdout) == EOF) {
        return -1;
    }
    return static_cast<int>(text.size());
}

std::string SafeFormat::escape_for_display(const std::string& text) {
    std::ostringstream out;
    out << std::hex << std::setfill('0');
    for (unsigned char c : text) {
        switch (c) {
            case '\n': out << "\\n"; break;
            case '\r': out << "\\r"; break;
            case '\t': out << "\\t"; break;
            case '"': out << "\\\""; break;
            case '\\': out << "\\\\"; break;
            default:
                if (c < 0x20 || c > 0x7e) {
                    out << "\\x" << std::setw(2) << static_cast<int>(c);
                } else {
                    out << c;
                }
        }
    }
    return out.str();
}

void SafeFormat::log_user_message(const std::string& message, const std::string& context) {
    std::string escaped = escape_for_display(message);
    std::fprintf(stderr, "[%s] %s\n", context.c_str(), escaped.c_str());
}

} // namespace themis::security
