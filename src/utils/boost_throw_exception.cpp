/**
 * @file boost_throw_exception.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Boost throw_exception implementation for header-only Boost libraries
// Required when BOOST_NO_EXCEPTIONS is not defined

#include <boost/throw_exception.hpp>
#include <exception>

namespace boost {

/**
 * @brief Throw exception.
 * @param[in] e Input parameter.
 * @throws e if an error occurs.
 * @details Implements throw_exception without additional internal calls.
 */
void throw_exception(std::exception const& e) {
    throw e;
}

/**
 * @brief Throw exception.
 * @param[in] e Input parameter.
 * @param[in] param Input parameter.
 * @throws e if an error occurs.
 * @details Implements throw_exception without additional internal calls.
 */
void throw_exception(std::exception const& e, boost::source_location const&) {
    throw e;
}

} // namespace boost
