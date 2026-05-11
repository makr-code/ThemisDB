/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            wasm_component_model.h                             ║
  Version:         0.1.0                                              ║
  Last Modified:   2026-07-01 00:00:00                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: 🟡 Interface Header (Target: Q3 2026)                       ║
╚═════════════════════════════════════════════════════════════════════╝
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

class IWasmComponentInstance {
public:
    virtual ~IWasmComponentInstance() = default;
    virtual WITValue call(const std::string& function_name,
                          const std::vector<WITValue>& args) = 0;
    virtual bool hasFunction(const std::string& function_name) const = 0;
    virtual std::vector<std::string> exportedFunctions() const = 0;
    virtual bool isValid() const = 0;
};

class IWasmComponentLoader {
public:
    virtual ~IWasmComponentLoader() = default;
    virtual std::unique_ptr<IWasmComponentInstance> load(const ComponentModelConfig& config) = 0;
    virtual bool validate(const std::string& component_path) = 0;
    virtual std::vector<WITInterface> introspect(const std::string& component_path) = 0;
};

}} // namespace themis::plugins
