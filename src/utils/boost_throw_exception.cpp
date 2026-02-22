/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            boost_throw_exception.cpp                          ║
  Version:         0.0.26                                             ║
  Last Modified:   2026-02-22 08:39:12                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     36                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// Boost throw_exception implementation for header-only Boost libraries
// Required when BOOST_NO_EXCEPTIONS is not defined

#include <boost/throw_exception.hpp>
#include <exception>

namespace boost {

void throw_exception(std::exception const& e) {
    throw e;
}

void throw_exception(std::exception const& e, boost::source_location const&) {
    throw e;
}

} // namespace boost
