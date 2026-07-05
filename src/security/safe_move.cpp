/**
 * @file src/security/safe_move.cpp
 * @brief Move semantics safety utilities implementation
 *
 * Provides move validation and tracking implementations for ThemisDB security utilities.
 *
 * @author ThemisDB Team
 * @date 2026-07-05
 * @license Apache 2.0
 */

#include "include/security/safe_move.h"

#include <iostream>
#include <sstream>

namespace themis::security {

// Explicit template instantiations for common types
template class MoveValidator<std::string>;
template class MoveValidator<std::vector<uint8_t>>;
template class MoveValidator<std::vector<int>>;
template class MoveValidator<std::vector<double>>;
template class MoveValidator<std::vector<std::string>>;
template class MoveValidator<std::unique_ptr<std::vector<uint8_t>>>;
template class MoveValidator<std::shared_ptr<std::vector<uint8_t>>>;

template class MoveGuard<std::vector<uint8_t>>;
template class MoveGuard<std::vector<int>>;
template class MoveGuard<std::string>;

template class SafeMove<std::string>;
template class SafeMove<std::vector<uint8_t>>;
template class SafeMove<std::vector<int>>;

} // namespace themis::security
