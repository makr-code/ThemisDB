/**
 * @file wasm_component_model.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.1.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once
// WASM Component Model support (WIT bindings)
// Per https://github.com/WebAssembly/component-model
#include <string>
#include <vector>
#include <map>
#include <memory>

namespace themis { namespace plugins {

enum class WITValueKind { BOOL, U8, U16, U32, U64, S8, S16, S32, S64, F32, F64, STRING, LIST, RECORD, VARIANT, OPTION, RESULT, TUPLE };

struct WITValue {
    WITValueKind kind;
    std::string string_val;
    double numeric_val = 0.0;
    std::vector<WITValue> list_val;
    std::map<std::string, WITValue> record_fields;
};

struct WITInterface {
    std::string interface_name;
    std::string package;
    std::vector<std::string> function_names;
};

struct ComponentModelConfig {
    std::string component_path;
    std::vector<WITInterface> imports;
    bool enable_wasi = true;
    std::string runtime_hint;
};

/** @brief I wasm component instance. */
class IWasmComponentInstance {
public:
    /**
     * @brief TBD: Describe ~IWasmComponentInstance.
     * @return Return value.
     */
    virtual ~IWasmComponentInstance() = default;
    /**
     * @brief TBD: Describe call.
     * @param[in] function_name Input parameter.
     * @param[in] args Input parameter.
     * @return Return value.
     */
    virtual WITValue call(const std::string& function_name,
                          const std::vector<WITValue>& args) = 0;
    /**
     * @brief TBD: Describe hasFunction.
     * @param[in] function_name Input parameter.
     * @return True on success.
     */
    virtual bool hasFunction(const std::string& function_name) const = 0;
    /**
     * @brief TBD: Describe exportedFunctions.
     * @return Return value.
     */
    virtual std::vector<std::string> exportedFunctions() const = 0;
    /**
     * @brief TBD: Describe isValid.
     * @return True on success.
     */
    virtual bool isValid() const = 0;
};

/** @brief I wasm component loader component. */
class IWasmComponentLoader {
public:
    /**
     * @brief TBD: Describe ~IWasmComponentLoader.
     * @return Return value.
     */
    virtual ~IWasmComponentLoader() = default;
    /**
     * @brief TBD: Describe load.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    virtual std::unique_ptr<IWasmComponentInstance> load(const ComponentModelConfig& config) = 0;
    /**
     * @brief TBD: Describe validate.
     * @param[in] component_path Input parameter.
     * @return True on success.
     */
    virtual bool validate(const std::string& component_path) = 0;
    /**
     * @brief TBD: Describe introspect.
     * @param[in] component_path Input parameter.
     * @return Return value.
     */
    virtual std::vector<WITInterface> introspect(const std::string& component_path) = 0;
};

}} // namespace themis::plugins
