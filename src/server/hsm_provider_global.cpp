/*
 * ThemisDB | File: hsm_provider_global.cpp | Version: 0.0.15 | Last Modified: 2026-05-20 17:27:23
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 14
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * PR History (last 5): none
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#include "security/hsm_provider.h"

#include <memory>

std::shared_ptr<themis::security::HSMProvider> g_hsm_provider;
