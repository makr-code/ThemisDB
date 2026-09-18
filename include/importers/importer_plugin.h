/**
 * @file importer_plugin.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * ABI version constants
 * ===================================================================== */

#define THEMIS_IMPORTER_PLUGIN_ABI_V1  ((uint32_t)1u)

#define THEMIS_IMPORTER_CREATE_SYMBOL  "themis_importer_create"

/* =========================================================================
 * Allocator hook (optional — enables sandbox memory-limit enforcement)
 * ===================================================================== */

typedef struct ThemisImporterAllocator {
    void* (*alloc)(size_t bytes, void* user_data);
    void  (*free)(void* ptr, void* user_data);
    void* user_data;
} ThemisImporterAllocator;

/* =========================================================================
 * THEMIS_IMPORTER_PLUGIN_V1 — stable C ABI descriptor struct
 * ===================================================================== */

typedef struct ThemisImporterPluginV1 {
    uint32_t  abi_version;
    uint32_t  struct_size;
    const char* name;
    const char* version;

    void* (*create_instance)(const ThemisImporterAllocator* allocator);

    void  (*destroy_instance)(void* instance,
                              const ThemisImporterAllocator* allocator);

    int   (*initialize)(void* instance, const char* config_json);

    int   (*validate_source)(void* instance,
                             const char* source_path,
                             char* error_buf, size_t error_buf_size);

    int   (*import_data)(void* instance,
                         const char* source_path,
                         const char* options_json,
                         uint64_t*   imported_out,
                         uint64_t*   failed_out);

    const char* (*get_schema)(void* instance, const char* source_path);

    void  (*cancel)(void* instance);

    void* reserved[4];
} THEMIS_IMPORTER_PLUGIN_V1;

/* =========================================================================
 * Factory function type
 * ===================================================================== */

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
    /**
     * @brief Themis importer create.
     * @return Pointer to the result.
     * @details Implements themis_importer_create without additional internal calls.
     */
    const THEMIS_IMPORTER_PLUGIN_V1* themis_importer_create(void) {          \
        return &themis_plugin_v1_descriptor_;                                 \
    }

#endif /* __cplusplus */
