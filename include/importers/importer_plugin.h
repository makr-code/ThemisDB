/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            importer_plugin.h                                  ║
  Version:         0.0.7                                              ║
  Last Modified:   2026-04-14 11:24:55                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     331                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2019b146f9  2026-03-15  feat(importers): Importer Plugin API v1.9.0 — stable C AB... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

/**
 * @file importer_plugin.h
 * @brief Stable C-linkage ABI for ThemisDB importer plugins (v1.9.0+).
 *
 * This header defines the versioned C ABI that third-party importer plugins
 * must implement.  It is written in pure C so that plugin authors can use any
 * language capable of exporting C symbols (C, C++, Rust, Go cgo, etc.).
 *
 * ## ABI contract
 *
 * Every plugin shared library must export exactly one C function:
 *
 * @code
 *   extern "C"
 *   THEMIS_IMPORTER_V1_EXPORT_ATTR
 *   const THEMIS_IMPORTER_PLUGIN_V1* themis_importer_create(void);
 * @endcode
 *
 * The function returns a pointer to a statically-allocated descriptor struct.
 * ThemisDB reads the descriptor, validates the @c abi_version field, and
 * calls the function pointers to create and drive importer instances.
 *
 * ## ABI versioning
 *
 * The THEMIS_IMPORTER_PLUGIN_V1 struct is versioned via two fields:
 *  - @c abi_version — must equal THEMIS_IMPORTER_PLUGIN_ABI_V1 (1).
 *  - @c struct_size — must equal @c sizeof(THEMIS_IMPORTER_PLUGIN_V1).
 *
 * Future ABI revisions (V2, V3, …) will add fields at the end of the struct
 * and introduce a new version constant.  The host checks @c abi_version on
 * load; a plugin compiled against V1 can always be loaded by a V2+ host.
 *
 * ## Memory / sandbox
 *
 * The host may pass a @c ThemisImporterAllocator to @c create_instance and
 * @c destroy_instance.  Plugins @b SHOULD use this allocator for all heap
 * allocations so that the host's sandbox can enforce per-job memory limits.
 * Plugins that bypass the allocator (use system @c malloc directly) will not
 * be subject to the memory limit.
 *
 * ## Quick-start
 *
 * Use the THEMIS_IMPORTER_PLUGIN_V1_EXPORT() convenience macro in one
 * translation unit to generate the @c themis_importer_create entry point.
 * See @c docs/importers/plugin_guide.md for a worked Oracle importer example.
 */

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * ABI version constants
 * ===================================================================== */

/** @brief ABI version for the @c THEMIS_IMPORTER_PLUGIN_V1 struct.
 *
 * Value is 1.  Must be stored in @c ThemisImporterPluginV1::abi_version by
 * every V1 plugin.
 */
#define THEMIS_IMPORTER_PLUGIN_ABI_V1  ((uint32_t)1u)

/** @brief Name of the factory symbol every plugin must export.
 *
 * Use this constant with @c dlsym / @c GetProcAddress to locate the factory:
 * @code
 *   auto* fn = (themis_importer_create_fn_t)dlsym(handle, THEMIS_IMPORTER_CREATE_SYMBOL);
 * @endcode
 */
#define THEMIS_IMPORTER_CREATE_SYMBOL  "themis_importer_create"

/* =========================================================================
 * Allocator hook (optional — enables sandbox memory-limit enforcement)
 * ===================================================================== */

/**
 * @brief Allocator callbacks passed to plugin instances by the host sandbox.
 *
 * When loaded via @c ImporterPluginRegistry::loadPlugin() with a non-zero
 * @c memory_limit_bytes, the host populates this struct with a counting
 * allocator.  Plugins @b SHOULD use these callbacks instead of @c malloc /
 * @c free to participate in memory-limit enforcement.
 *
 * Both function pointers may be @c NULL; in that case the plugin must fall
 * back to the system allocator.
 */
typedef struct ThemisImporterAllocator {
    /** Allocate @p bytes.  Returns @c NULL if the sandbox limit is exceeded. */
    void* (*alloc)(size_t bytes, void* user_data);
    /** Free a pointer previously returned by @c alloc.  Accepts @c NULL. */
    void  (*free)(void* ptr, void* user_data);
    /** Opaque host-owned state; do not read or write. */
    void* user_data;
} ThemisImporterAllocator;

/* =========================================================================
 * THEMIS_IMPORTER_PLUGIN_V1 — stable C ABI descriptor struct
 * ===================================================================== */

/**
 * @brief Stable C-linkage ABI descriptor for a ThemisDB importer plugin.
 *
 * A plugin shared library exports a function named @c themis_importer_create
 * (see THEMIS_IMPORTER_CREATE_SYMBOL) that returns a pointer to a statically-
 * allocated instance of this struct.
 *
 * ### Field reference
 *
 * | Field              | Notes                                                  |
 * |--------------------|--------------------------------------------------------|
 * | @c abi_version     | Must == THEMIS_IMPORTER_PLUGIN_ABI_V1 (1)              |
 * | @c struct_size     | Must == sizeof(ThemisImporterPluginV1)                 |
 * | @c name            | Unique plugin identifier (snake_case, ASCII, stable)   |
 * | @c version         | Semantic version string (e.g. "1.0.0")                 |
 * | @c create_instance | Create an opaque plugin instance; @p allocator may be  |
 * |                    | NULL — fall back to system malloc                      |
 * | @c destroy_instance| Release instance resources; must accept NULL safely    |
 * | @c initialize      | Configure instance from JSON; returns 0 on success     |
 * | @c validate_source | Pre-flight check; writes error into @p error_buf       |
 * | @c import_data     | Execute import; populates *imported_out / *failed_out  |
 * | @c get_schema      | JSON schema string owned by instance; may be NULL      |
 * | @c cancel          | Signal in-progress import to stop; non-blocking        |
 * | @c reserved        | Zero-initialised in V1; reserved for future ABI fields |
 */
typedef struct ThemisImporterPluginV1 {
    /** ABI version; must equal THEMIS_IMPORTER_PLUGIN_ABI_V1. */
    uint32_t  abi_version;
    /** Struct size; must equal @c sizeof(ThemisImporterPluginV1). */
    uint32_t  struct_size;
    /** Plugin identifier (null-terminated ASCII, snake_case recommended). */
    const char* name;
    /** Plugin semantic version string (null-terminated, e.g. "1.0.0"). */
    const char* version;

    /**
     * Create a new importer instance.
     * @param allocator  Host-provided allocator; may be NULL.
     * @return  Opaque handle, or NULL on failure.
     */
    void* (*create_instance)(const ThemisImporterAllocator* allocator);

    /**
     * Destroy an instance returned by @c create_instance.
     * @param instance   Handle to destroy; NULL-safe.
     * @param allocator  Same allocator passed to @c create_instance; may be NULL.
     */
    void  (*destroy_instance)(void* instance,
                              const ThemisImporterAllocator* allocator);

    /**
     * Configure the instance.
     * @param instance    Opaque instance handle.
     * @param config_json Null-terminated JSON configuration string.
     * @return 0 on success, non-zero on error.
     */
    int   (*initialize)(void* instance, const char* config_json);

    /**
     * Validate a source before importing.
     * @param instance       Opaque instance handle.
     * @param source_path    Null-terminated source path or connection string.
     * @param error_buf      Buffer for a human-readable error message.
     * @param error_buf_size Capacity of @p error_buf in bytes.
     * @return 0 if the source is valid, non-zero otherwise.
     */
    int   (*validate_source)(void* instance,
                             const char* source_path,
                             char* error_buf, size_t error_buf_size);

    /**
     * Execute the import synchronously.
     * @param instance       Opaque instance handle.
     * @param source_path    Null-terminated source path or connection string.
     * @param options_json   Null-terminated JSON options object; may be NULL.
     * @param imported_out   Set to number of successfully imported records; may be NULL.
     * @param failed_out     Set to number of failed records; may be NULL.
     * @return 0 on success, non-zero on error.
     */
    int   (*import_data)(void* instance,
                         const char* source_path,
                         const char* options_json,
                         uint64_t*   imported_out,
                         uint64_t*   failed_out);

    /**
     * Return a JSON string describing the source schema.
     * @param instance    Opaque instance handle.
     * @param source_path Null-terminated source path; may be NULL.
     * @return Null-terminated JSON string owned by the instance, or NULL if
     *         schema introspection is not supported.  The pointer is valid
     *         until the next call to any function on the same instance or
     *         until @c destroy_instance is called.
     */
    const char* (*get_schema)(void* instance, const char* source_path);

    /**
     * Signal a running import to stop at the next safe checkpoint.
     * Non-blocking; the import may continue briefly after this returns.
     * @param instance  Opaque instance handle.
     */
    void  (*cancel)(void* instance);

    /** Reserved for future ABI fields; must be zero-initialised in V1. */
    void* reserved[4];
} THEMIS_IMPORTER_PLUGIN_V1;

/* =========================================================================
 * Factory function type
 * ===================================================================== */

/**
 * @brief Type of the factory function exported by every plugin.
 *
 * The plugin must export a function with C linkage and this exact signature:
 * @code
 *   extern "C"
 *   THEMIS_IMPORTER_V1_EXPORT_ATTR
 *   const THEMIS_IMPORTER_PLUGIN_V1* themis_importer_create(void);
 * @endcode
 */
typedef const THEMIS_IMPORTER_PLUGIN_V1* (*themis_importer_create_fn_t)(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

/* =========================================================================
 * C++ / shared-library export helpers (C++ only)
 * ===================================================================== */

#ifdef __cplusplus

/** @brief Compiler attribute that marks a symbol as visible in a shared library. */
#if defined(_WIN32)
#  ifdef THEMIS_IMPORTER_PLUGIN_EXPORTS
#    define THEMIS_IMPORTER_V1_EXPORT_ATTR __declspec(dllexport)
#  else
#    define THEMIS_IMPORTER_V1_EXPORT_ATTR __declspec(dllimport)
#  endif
#else
#  define THEMIS_IMPORTER_V1_EXPORT_ATTR __attribute__((visibility("default")))
#endif

/**
 * @brief Generate the @c themis_importer_create entry point for a V1 plugin.
 *
 * Place this macro exactly **once** in one translation unit of your plugin
 * shared library.  Provide the seven function-pointer arguments that make up
 * the core of the V1 ABI.
 *
 * ### Parameters
 *
 * | Parameter         | Type / description                                        |
 * |-------------------|-----------------------------------------------------------|
 * | @p plugin_name    | String literal — plugin identifier (snake_case)           |
 * | @p plugin_ver     | String literal — semantic version string ("1.0.0")        |
 * | @p create_fn      | `void* (*)(const ThemisImporterAllocator*)`               |
 * | @p destroy_fn     | `void  (*)(void*, const ThemisImporterAllocator*)`        |
 * | @p init_fn        | `int   (*)(void*, const char*)`                           |
 * | @p validate_fn    | `int   (*)(void*, const char*, char*, size_t)`            |
 * | @p import_fn      | `int   (*)(void*, const char*, const char*, uint64_t*, uint64_t*)` |
 * | @p schema_fn      | `const char* (*)(void*, const char*)`                     |
 * | @p cancel_fn      | `void  (*)(void*)`                                        |
 *
 * ### Example
 * @code
 *   static void* oracle_create(const ThemisImporterAllocator*) { return new OracleImporter(); }
 *   static void  oracle_destroy(void* p, const ThemisImporterAllocator*) { delete (OracleImporter*)p; }
 *   static int   oracle_init(void* p, const char* cfg) {
 *       return ((OracleImporter*)p)->initialize(cfg) ? 0 : 1;
 *   }
 *   // ... other functions ...
 *
 *   THEMIS_IMPORTER_PLUGIN_V1_EXPORT("oracle_importer", "1.0.0",
 *       oracle_create, oracle_destroy, oracle_init,
 *       oracle_validate, oracle_import, oracle_schema, oracle_cancel)
 * @endcode
 *
 * See @c docs/importers/plugin_guide.md for a complete worked example.
 */
#define THEMIS_IMPORTER_PLUGIN_V1_EXPORT(plugin_name, plugin_ver,           \
                                         create_fn,  destroy_fn,            \
                                         init_fn,    validate_fn,           \
                                         import_fn,  schema_fn,  cancel_fn) \
    static const THEMIS_IMPORTER_PLUGIN_V1 themis_plugin_v1_descriptor_ = { \
        THEMIS_IMPORTER_PLUGIN_ABI_V1,                                       \
        static_cast<uint32_t>(sizeof(THEMIS_IMPORTER_PLUGIN_V1)),            \
        plugin_name,                                                          \
        plugin_ver,                                                           \
        create_fn,                                                            \
        destroy_fn,                                                           \
        init_fn,                                                              \
        validate_fn,                                                          \
        import_fn,                                                            \
        schema_fn,                                                            \
        cancel_fn,                                                            \
        {nullptr, nullptr, nullptr, nullptr}                                  \
    };                                                                        \
    extern "C" THEMIS_IMPORTER_V1_EXPORT_ATTR                                \
    const THEMIS_IMPORTER_PLUGIN_V1* themis_importer_create(void) {          \
        return &themis_plugin_v1_descriptor_;                                 \
    }

#endif /* __cplusplus */
