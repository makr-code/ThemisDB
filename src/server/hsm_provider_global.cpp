/*
 * ThemisDB | File: hsm_provider_global.cpp | Version: 0.0.15 | Last Modified: 2026-05-20 17:13:04
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 13
 * Open Issues: TODOs=1, Stubs=1, Gaps=3, Unimpl=0, Mock=1, Sim=0, Debt=0
 * Gap Correlation: internal=3 | external_v3=1 | delta=2 | status=near
 * External Severity (v3): C=0, H=0, M=1
 * PR: none
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#include "security/hsm_provider.h"

#include <memory>

std::shared_ptr<themis::security::HSMProvider> g_hsm_provider;
