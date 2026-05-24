/*
 * ThemisDB | File: index_interface_stubs.cpp | Version: 0.0.13
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=1, L=0
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
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
