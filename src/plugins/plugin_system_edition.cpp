/*
 * DEPRECATED - Merged into plugin_manager.cpp / plugin_manager.h
 * ==============================================================
 * The edition/license gating and utility functions previously defined here
 * have been consolidated into PluginManager:
 *
 *   PluginManager::isEditionSupported()           -- compile-time edition gate
 *   PluginManager::isLicensed()                   -- runtime license gate
 *   PluginManager::communityUnavailableMessage()  -- user-facing error text
 *   PluginManager::marketplaceInfo()              -- marketplace availability
 *   PluginManager::installationInstructions()     -- install guide
 *
 * The edition gate is now automatically applied inside loadPlugin().
 * This file is intentionally excluded from cmake/CMakeLists.txt.
 */
