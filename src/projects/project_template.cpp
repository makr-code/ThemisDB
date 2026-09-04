/**
 * @file project_template.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.3
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=3, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "projects/project_template.h"

#include <chrono>
#include <random>
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace themis {
namespace projects {

// ─── builtinTemplateToString ──────────────────────────────────────────────────

const char* builtinTemplateToString(BuiltinTemplate tmpl) noexcept {
    switch (tmpl) {
        case BuiltinTemplate::EMPTY:            return "empty";
        case BuiltinTemplate::WEB_APPLICATION:  return "web_application";
        case BuiltinTemplate::MACHINE_LEARNING: return "machine_learning";
        case BuiltinTemplate::ANALYTICS:        return "analytics";
        case BuiltinTemplate::TIME_SERIES:      return "time_series";
        case BuiltinTemplate::GRAPH_ANALYTICS:  return "graph_analytics";
        case BuiltinTemplate::DOCUMENT_STORE:   return "document_store";
    }
    return "unknown";
}

// ─── ProjectTemplate ─────────────────────────────────────────────────────────

ProjectTemplate::ProjectTemplate(std::shared_ptr<RocksDBWrapper> storage)
    : storage_(std::move(storage)) {}

std::string ProjectTemplate::generateUuid() const {
    static thread_local std::mt19937_64 rng{
        static_cast<std::mt19937_64::result_type>(
            std::chrono::steady_clock::now().time_since_epoch().count())
    };
    std::uniform_int_distribution<uint64_t> dist;
    std::ostringstream oss = {};
    oss << std::hex << std::setfill('0')
        << std::setw(16) << dist(rng)
        << std::setw(16) << dist(rng);
    auto s = oss.str();
    return s.substr(0,8) + "-" + s.substr(8,4) + "-" + s.substr(12,4)
         + "-" + s.substr(16,4) + "-" + s.substr(20,12);
}

// ─── Built-in template schemas ────────────────────────────────────────────────

json ProjectTemplate::getBuiltinTemplateSchema(BuiltinTemplate tmpl) const {
    switch (tmpl) {
        case BuiltinTemplate::EMPTY:
            return json{{"name", "empty"}, {"objects", json::array()}};

        case BuiltinTemplate::WEB_APPLICATION:
            return json{
                {"name", "web_application"},
                {"objects", json::array({
                    {{"type","collection"},{"name","users"}},
                    {{"type","collection"},{"name","sessions"}},
                    {{"type","collection"},{"name","posts"}},
                    {{"type","collection"},{"name","comments"}},
                    {{"type","index"},{"name","idx_users_email"},
                     {"on","users"},{"field","email"}},
                })}
            };

        case BuiltinTemplate::MACHINE_LEARNING:
            return json{
                {"name", "machine_learning"},
                {"objects", json::array({
                    {{"type","collection"},{"name","datasets"}},
                    {{"type","collection"},{"name","features"}},
                    {{"type","collection"},{"name","models"}},
                    {{"type","collection"},{"name","experiments"}},
                    {{"type","collection"},{"name","predictions"}},
                })}
            };

        case BuiltinTemplate::ANALYTICS:
            return json{
                {"name", "analytics"},
                {"objects", json::array({
                    {{"type","collection"},{"name","facts"}},
                    {{"type","collection"},{"name","dimensions"}},
                    {{"type","collection"},{"name","aggregates"}},
                    {{"type","index"},{"name","idx_facts_ts"},
                     {"on","facts"},{"field","timestamp"}},
                })}
            };

        case BuiltinTemplate::TIME_SERIES:
            return json{
                {"name", "time_series"},
                {"objects", json::array({
                    {{"type","collection"},{"name","sensors"}},
                    {{"type","collection"},{"name","metrics"}},
                    {{"type","collection"},{"name","alerts"}},
                    {{"type","index"},{"name","idx_metrics_ts"},
                     {"on","metrics"},{"field","timestamp"}},
                })}
            };

        case BuiltinTemplate::GRAPH_ANALYTICS:
            return json{
                {"name", "graph_analytics"},
                {"objects", json::array({
                    {{"type","collection"},{"name","nodes"}},
                    {{"type","collection"},{"name","edges"}},
                    {{"type","graph_index"},{"name","g_main"},
                     {"vertices","nodes"},{"edges","edges"}},
                })}
            };

        case BuiltinTemplate::DOCUMENT_STORE:
            return json{
                {"name", "document_store"},
                {"objects", json::array({
                    {{"type","collection"},{"name","documents"}},
                    {{"type","collection"},{"name","attachments"}},
                    {{"type","vector_index"},{"name","vi_docs"},
                     {"on","documents"},{"field","embedding"},{"dim",768}},
                })}
            };
    }
    return json{{"name","unknown"},{"objects",json::array()}};
}

// ─── Validation ───────────────────────────────────────────────────────────────

Status ProjectTemplate::validateTemplateDefinition(const json& def) {
    if (!def.is_object())
        return Status::Error("Template definition must be a JSON object");
    if (!def.contains("name") || !def["name"].is_string())
        return Status::Error("Template definition must have a 'name' string field");
    if (!def.contains("objects") || !def["objects"].is_array())
        return Status::Error("Template definition must have an 'objects' array");

    for (const auto& obj : def["objects"]) {
        if (!obj.is_object())
            return Status::Error("Each object entry must be a JSON object");
        if (!obj.contains("type") || !obj["type"].is_string())
            return Status::Error("Object entry missing 'type' string");
        if (!obj.contains("name") || !obj["name"].is_string())
            return Status::Error("Object entry missing 'name' string");
    }
    return Status::OK();
}

// ─── Object creation ─────────────────────────────────────────────────────────

std::optional<std::string> ProjectTemplate::createObjectFromDefinition(
    const std::string& project_id,
    const json&        obj_def,
    bool               include_sample_data)
{
    const std::string type = obj_def.value("type", std::string{});
    const std::string name = obj_def.value("name", std::string{});
    if (name.empty()) {
      return std::nullopt;
    }

    const std::string key =
        "template_obj:" + project_id + ":" + type + ":" + name;
    json record = {
        {"project_id", project_id},
        {"type",       type},
        {"name",       name},
        {"definition", obj_def},
        {"created_at",
         static_cast<int64_t>(
             std::chrono::system_clock::now().time_since_epoch() /
             std::chrono::seconds(1))},
    };

    if (include_sample_data && type == "collection") {
        record["sample_data"] = json::array({
            {{"id","sample-1"},{"note","sample record"}},
        });
    }

    if (!storage_->put(key, record.dump()))
        return std::nullopt;

    return name;
}

// ─── instantiate ─────────────────────────────────────────────────────────────

TemplateInstantiationResult ProjectTemplate::instantiate(
    BuiltinTemplate        tmpl,
    const TemplateOptions& options)
{
    return instantiateFromDefinition(getBuiltinTemplateSchema(tmpl), options);
}

TemplateInstantiationResult ProjectTemplate::instantiateFromDefinition(
    const json&            template_def,
    const TemplateOptions& options)
{
    TemplateInstantiationResult result;

    // Validate first — no partial writes on failure
    const auto validation = validateTemplateDefinition(template_def);
    if (!validation.ok) {
        result.message = validation.message;
        return result;
    }

    if (options.project_name.empty()) {
        result.message = "options.project_name must not be empty";
        return result;
    }

    const std::string project_id = generateUuid();
    std::vector<std::string> created;

    // Write project record
    const json project_record = {
        {"id",          project_id},
        {"name",        options.project_name},
        {"description", options.description},
        {"template",    template_def.value("name", std::string{})},
        {"extra_config",options.extra_config},
        {"created_at",
         static_cast<int64_t>(
             std::chrono::system_clock::now().time_since_epoch() /
             std::chrono::seconds(1))},
    };
    if (!storage_->put("project:" + project_id, project_record.dump())) {
        result.message = "Failed to write project record";
        return result;
    }

    // Create each object — roll back on failure
    for (const auto& obj : template_def["objects"]) {
        auto name = createObjectFromDefinition(
            project_id, obj, options.include_sample_data);
        if (!name.has_value()) {
            // Roll back already-created objects
            for (const auto& n : created) {
                const std::string type = obj.value("type", std::string{});
                storage_->del("template_obj:" + project_id + ":" + type + ":" + n);
            }
            storage_->del("project:" + project_id);
            result.message = "Failed to create object: " +
                             obj.value("name", std::string{"<unknown>"});
            return result;
        }
        created.push_back(*name);
    }

    result.ok              = true;
    result.project_id      = project_id;
    result.objects_created = std::move(created);
    result.message         = "Project created from template";
    return result;
}

// ─── listBuiltinTemplates ─────────────────────────────────────────────────────

std::vector<std::string> ProjectTemplate::listBuiltinTemplates() {
    return {
        builtinTemplateToString(BuiltinTemplate::EMPTY),
        builtinTemplateToString(BuiltinTemplate::WEB_APPLICATION),
        builtinTemplateToString(BuiltinTemplate::MACHINE_LEARNING),
        builtinTemplateToString(BuiltinTemplate::ANALYTICS),
        builtinTemplateToString(BuiltinTemplate::TIME_SERIES),
        builtinTemplateToString(BuiltinTemplate::GRAPH_ANALYTICS),
        builtinTemplateToString(BuiltinTemplate::DOCUMENT_STORE),
    };
}

} // namespace projects
} // namespace themis
