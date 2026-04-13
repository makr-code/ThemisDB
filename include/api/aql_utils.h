/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            aql_utils.h                                        ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-04-13 20:20:04                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     72                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • e2d928fc19  2026-04-07  fix(api): address code-review feedback – underscore UB fi... ║
    • 02a975f292  2026-04-07  fix(api): AQL identifier injection, BatchWrite partial-fa... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once
/*
 * aql_utils.h – lightweight AQL escaping and identifier validation helpers.
 *
 * These utilities are intentionally header-only so that they can be used both
 * from production code and from unit tests without requiring link-time coupling
 * to the gRPC service library.
 */

#include <cctype>
#include <string>

namespace themis::api {

/// Escape a string for safe embedding inside an AQL single-quoted literal.
/// Replaces `\` and `'` to prevent AQL string-literal injection.
inline std::string aqlEscapeLiteral(const std::string& raw) {
    std::string out;
    out.reserve(raw.size() + 4);
    for (char c : raw) {
        if (c == '\\') { out += "\\\\"; }
        else if (c == '\'') { out += "\\'"; }
        else { out += c; }
    }
    return out;
}

/// Validate a name that will be used as an AQL collection identifier
/// (i.e. directly after `FOR doc IN <name>` or `INSERT ... INTO <name>`).
///
/// AQL identifiers must:
///   - not be empty
///   - start with a letter (`a-z`, `A-Z`) or underscore (`_`)
///   - contain only letters, digits (`0-9`), or underscores
///
/// Returns `true` if the name is safe to embed as a bare AQL identifier.
inline bool isValidAqlIdentifier(const std::string& name) {
    if (name.empty()) return false;
    // Check underscore before isalpha to avoid UB on negative char values.
    if (name[0] != '_' && !std::isalpha(static_cast<unsigned char>(name[0])))
        return false;
    for (char c : name) {
        if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_')
            return false;
    }
    return true;
}

} // namespace themis::api
