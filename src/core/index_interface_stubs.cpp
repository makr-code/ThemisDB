/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            index_interface_stubs.cpp                          ║
  Version:         0.0.7                                              ║
  Last Modified:   2026-04-14 11:32:52                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     37                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • efc8af71bb  2026-03-11  feat: add LLM-assisted content analysis methods and impro... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/// @file index_interface_stubs.cpp
/// @brief Forces MSVC to emit ISecondaryIndex, IVectorIndex, IGraphIndex
///        constructor and destructor symbols into themis_base.dll.
///
/// When compiled with THEMIS_BASE_EXPORTS defined (i.e. as part of the
/// themis_base shared library), THEMIS_BASE_API expands to
/// __declspec(dllexport), which causes the MSVC linker to emit the
/// implicit default constructor and virtual destructor of each abstract
/// interface class.  Without this translation unit those symbols are
/// never emitted and every other module that derives from the interfaces
/// fails to link with LNK2019 "__imp_??0..." / "__imp_??1..." errors.
///
/// Nothing else should be placed in this file.

#include "themis/base/interfaces/index_interface.h"
