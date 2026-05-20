/*
 * ThemisDB | File: plugin_system_edition.cpp | Version: 0.0.1 | Last Modified: 2026-04-20 21:36:21
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 23
 * Open Issues: TODOs=1, Stubs=1, Gaps=3, Unimpl=0, Mock=1, Sim=0, Debt=0
 * Gap Correlation: internal=3 | external_v3=5 | delta=2 | status=near
 * External Severity (v3): C=0, H=5, M=0
 * PR: #4256 feat(plugins): upgrade PluginRegistry global mutex to shared_mutex ... (2026-03-15T15:57:39Z)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

/*
 * DEPRECATED - Merged into plugin_manager.cpp / plugin_manager.h
 * ==============================================================
 * The edition/license gating and utility functions previously defined here
 * have been consolidated into PluginManager:
 *
 *   PluginManager::isEditionSupported()           -- compile-time edition gate
 *   PluginManager::isLicensed()                   -- runtime license gate
 *   PluginManager::communityUnavailableMessage()  -- user-facing error text
 *   PluginManager::mark