/**
 * @file monitoring_api_routes.cpp
 * @brief Monitoring route annotations isolated to reduce compiler heap pressure.
 */

#include "server/monitoring_api_handler.h"
#include "server/openapi_route_registry.h"

namespace themis {
namespace server {

namespace {

// Bound each function's nested JSON initializer temporaries: MSVC can exhaust
// its compiler heap even with /Od /Ob0 when all routes share one function.
void registerHealthAndMetricsRoutes(RouteRegistry& reg) {
    // --- Monitoring / Health ---
    reg.registerRoute({"/health", "get", {
        "Basic health check", "", "getHealth", {"monitoring"}, {}, {},
        {{"200", {{"description","Server is healthy"},
                  {"content",{{"application/json",{{"schema",{{"type","object"}}}}}}}}},
         {"503", {{"description","Server is unhealthy"}}}}
    }});
    reg.registerRoute({"/health/live", "get", {
        "Liveness probe",
        "Returns 200 if the server process is running, 503 if it has stopped",
        "getLiveness", {"monitoring"}, {}, {},
        {{"200", {{"description","Server process is alive"}}},
         {"503", {{"description","Server process is dead"}}}}
    }});
    reg.registerRoute({"/health/ready", "get", {
        "Readiness probe",
        "Returns 200 when all subsystems (storage, connections) are ready. "
        "Returns per-layer health details.",
        "getReadiness", {"monitoring"}, {}, {},
        {{"200", {{"description","All layers ready"}}},
         {"503", {{"description","One or more layers not ready"}}}}
    }});

    // --- Version / Stats / Capabilities ---
    reg.registerRoute({"/version", "get", {
        "Get server version", "", "getVersion", {"monitoring"}, {}, {},
        {{"200", {{"description","Version and build information"}}}}
    }});
    reg.registerRoute({"/stats", "get", {
        "Runtime statistics", "", "getStats", {"monitoring"}, {}, {},
        {{"200", {{"description","Runtime statistics in JSON"}}}}
    }});
    reg.registerRoute({"/api/capabilities", "get", {
        "Server capabilities", "", "getCapabilities", {"monitoring"}, {}, {},
        {{"200", {{"description","Server feature capability map"}}}}
    }});

    // --- Metrics ---
    reg.registerRoute({"/metrics", "get", {
        "Prometheus metrics",
        "Returns server metrics in Prometheus text exposition format",
        "getMetrics", {"monitoring"}, {}, {},
        {{"200", {{"description","Prometheus text format metrics"},
                  {"content",{{"text/plain",{{"schema",{{"type","string"}}}}}}}}}}
    }});
    reg.registerRoute({"/metrics/html", "get", {
        "HTML metrics dashboard",
        "Renders current Prometheus metrics as a human-readable HTML table "
        "with dark-mode styling",
        "getMetricsHtml", {"monitoring"}, {}, {},
        {{"200", {{"description","HTML metrics dashboard"},
                  {"content",{{"text/html",{{"schema",{{"type","string"}}}}}}}}}}
    }});

}

void registerObservabilityRoutes(RouteRegistry& reg) {
    // --- Observability operator API ---
    reg.registerRoute({"/api/v1/observability/alerts", "get", {
        "List active alerts",
        "Returns the list of currently active (firing or silenced) alerts as a JSON array",
        "getObservabilityAlerts", {"observability"}, {}, {},
        {{"200", {{"description","Active alert list"},
                  {"content",{{"application/json",{{"schema",{
                      {"type","object"},
                      {"properties",{
                          {"alerts",{{"type","array"}}},
                          {"count",{{"type","integer"}}},
                          {"alertmanager_enabled",{{"type","boolean"}}}
                      }}
                  }}}}}}}}}
    }});
    reg.registerRoute({"/api/v1/observability/alerts/{id}/silence", "post", {
        "Silence an alert",
        "Silences the named alert for a configurable duration. "
        "Body: {\"duration_minutes\": <int>} (default 60)",
        "silenceObservabilityAlert", {"observability"},
        {RouteParam{"id", "path", true, "Alert identifier", {{"type","string"}}}},
        {{"required",false},
         {"content",{{"application/json",{{"schema",{
             {"type","object"},
             {"properties",{{"duration_minutes",{{"type","integer"},{"default",60}}}}}
         }}}}}}},
        {{"200",{{"description","Alert silenced"}}},
         {"400",{{"description","Bad request"}}},
         {"404",{{"description","Alert not found"}}},
         {"503",{{"description","Alertmanager not configured"}}}}
    }});
    reg.registerRoute({"/api/v1/observability/health", "get", {
        "Observability subsystem health",
        "Returns aggregate health of the observability stack: Alertmanager "
        "connectivity, tracing span counters, and MetricsCollector cardinality",
        "getObservabilityHealth", {"observability"}, {}, {},
        {{"200",{{"description","Observability health status"},
                 {"content",{{"application/json",{{"schema",{{"type","object"}}}}}}}}}  }
    }});
    reg.registerRoute({"/api/v1/observability/provenance", "get", {
        "Query persisted retrieval provenance",
        "Returns retrieval provenance records from the persistent store. "
        "Supports query_id chain export, time-range filtering, and limit capping.",
        "getObservabilityProvenance", {"observability"},
        {
            RouteParam{"query_id", "query", false, "Query identifier", {{"type","string"}}},
            RouteParam{"start_ts_ms", "query", false, "Start timestamp (ms since epoch)", {{"type","integer"}}},
            RouteParam{"end_ts_ms", "query", false, "End timestamp (ms since epoch)", {{"type","integer"}}},
            RouteParam{"limit", "query", false, "Maximum returned records (default 1000, max 10000)", {{"type","integer"}}}
        },
        {},
        {
            {"200",{{"description","Provenance export payload"},
                     {"content",{{"application/json",{{"schema",{{"type","object"}}}}}}}}},
            {"400",{{"description","Invalid query parameters"}}},
            {"503",{{"description","Provenance store not configured"}}}
        }
    }});

}

void registerLicenseRoutes(RouteRegistry& reg) {
    // --- License ---
    reg.registerRoute({"/api/v1/license/status", "get", {
        "Runtime license status",
        "Returns the current runtime license state: initialized flag, status string "
        "(active/grace/expired/invalid), grace days remaining, masked license key, "
        "organization, edition, expiry date, days until expiry, and validity flag.",
        "getLicenseStatus", {"license"}, {}, {},
        {{"200",{{"description","License status document"},
                 {"content",{{"application/json",{{"schema",{
                     {"type","object"},
                     {"properties",{
                         {"initialized",         {{"type","boolean"}}},
                         {"status",              {{"type","string"},{"example","active"}}},
                         {"grace_days_remaining",{{"type","integer"}}},
                         {"license_key",         {{"type","string"},{"example","THEMIS-EN..."}}},
                         {"organization",        {{"type","string"}}},
                         {"edition",             {{"type","string"},{"example","ENTERPRISE"}}},
                         {"expiry_date",         {{"type","string"},{"format","date"}}},
                         {"days_until_expiry",   {{"type","integer"}}},
                         {"valid",               {{"type","boolean"}}}
                     }}
                 }}}}}}}}}
    }});

}

void registerEntityRoutes(RouteRegistry& reg) {
    // --- OpenAPI self-reference ---
    reg.registerRoute({"/api/openapi.json", "get", {
        "OpenAPI specification",
        "Returns this OpenAPI 3.1 specification document",
        "getOpenApiSpec", {"monitoring"}, {}, {},
        {{"200",{{"description","OpenAPI 3.1 specification"},
                 {"content",{{"application/json",{{"schema",{{"type","object"}}}}}}}}}  }
    }});

    // --- Entities CRUD ---
    reg.registerRoute({"/entities", "get", {
        "List entities", "", "listEntities", {"entities"},
        {RouteParam{"Accept-Version","header",false,
                    "Request a specific API version",{{"type","string"}}}},
        {},
        {{"200",{{"description","Entity list"},
                 {"headers",{{"API-Version",{{"$ref","#/components/headers/API-Version"}}},
                              {"Deprecation",{{"$ref","#/components/headers/Deprecation"}}},
                              {"Sunset",     {{"$ref","#/components/headers/Sunset"}}},
                              {"Link",       {{"$ref","#/components/headers/Link"}}}}}}},
         {"401",{{"description","Unauthorized"}}}}
    }});
    reg.registerRoute({"/entities", "post", {
        "Create entity", "", "createEntity", {"entities"},
        {RouteParam{"Accept-Version","header",false,
                    "Request a specific API version",{{"type","string"}}}},
        {{"required",true},{"content",{{"application/json",{{"schema",{{"type","object"}}}}}}}},
        {{"201",{{"description","Entity created"},
                 {"headers",{{"API-Version",{{"$ref","#/components/headers/API-Version"}}}}}}},
         {"400",{{"description","Bad request"}}},
         {"413",{{"description","Payload too large"}}}}
    }});
    reg.registerRoute({"/entities/{key}", "get", {
        "Get entity by key", "", "getEntity", {"entities"},
        {RouteParam{"key","path",true,"Entity key",{{"type","string"}}},
         RouteParam{"Accept-Version","header",false,
                    "Request a specific API version",{{"type","string"}}}},
        {},
        {{"200",{{"description","Entity found"},
                 {"headers",{{"API-Version",{{"$ref","#/components/headers/API-Version"}}},
                              {"Deprecation",{{"$ref","#/components/headers/Deprecation"}}},
                              {"Sunset",     {{"$ref","#/components/headers/Sunset"}}},
                              {"Link",       {{"$ref","#/components/headers/Link"}}}}}}},
         {"404",{{"description","Not found"}}}}
    }});
    reg.registerRoute({"/entities/{key}", "put", {
        "Upsert entity by key", "", "upsertEntity", {"entities"},
        {RouteParam{"key","path",true,"Entity key",{{"type","string"}}},
         RouteParam{"Accept-Version","header",false,
                    "Request a specific API version",{{"type","string"}}}},
        {{"required",true},{"content",{{"application/json",{{"schema",{{"type","object"}}}}}}}},
        {{"200",{{"description","Entity updated"},
                 {"headers",{{"API-Version",{{"$ref","#/components/headers/API-Version"}}}}}}},
         {"201",{{"description","Entity created"}}}}
    }});
    reg.registerRoute({"/entities/{key}", "delete", {
        "Delete entity by key", "", "deleteEntity", {"entities"},
        {RouteParam{"key","path",true,"Entity key",{{"type","string"}}},
         RouteParam{"Accept-Version","header",false,
                    "Request a specific API version",{{"type","string"}}}},
        {},
        {{"200",{{"description","Entity deleted"},
                 {"headers",{{"API-Version",{{"$ref","#/components/headers/API-Version"}}}}}}},
         {"404",{{"description","Not found"}}}}
    }});

}

void registerQueryRoutes(RouteRegistry& reg) {
    // --- Query ---
    reg.registerRoute({"/query", "post", {
        "Execute a query", "", "postQuery", {"query"},
        {RouteParam{"Accept-Version","header",false,
                    "Request a specific API version",{{"type","string"}}}},
        {{"required",true},{"content",{{"application/json",{{"schema",{{"type","object"}}}}}}}},
        {{"200",{{"description","Query results"},
                 {"headers",{{"API-Version",{{"$ref","#/components/headers/API-Version"}}}}}}},
         {"400",{{"description","Bad query"}}}}
    }});
    reg.registerRoute({"/query/aql", "post", {
        "Execute an AQL (ThemisDB Query Language) query", "", "postAqlQuery", {"query"},
        {RouteParam{"Accept-Version","header",false,
                    "Request a specific API version",{{"type","string"}}}},
        {{"required",true},{"content",{{"application/json",{{"schema",{{"type","object"}}}}}}}},
        {{"200",{{"description","AQL query results"},
                 {"headers",{{"API-Version",{{"$ref","#/components/headers/API-Version"}}}}}}},
         {"400",{{"description","AQL syntax error"}}}}
    }});

    // --- UDF Registration ---
    reg.registerRoute({"/api/v1/query/udfs", "post", {
        "Register or replace an AQL user-defined function (UDF)", "", "registerUdf", {"udf"},
        {},
        {{"required",true},{"content",{{"application/json",{{"schema",{{"type","object"}}}}}}}},
        {{"201",{{"description","UDF registered"}}},
         {"400",{{"description","Invalid UDF definition"}}}}
    }});
    reg.registerRoute({"/api/v1/query/udfs", "get", {
        "List all registered AQL user-defined functions", "", "listUdfs", {"udf"},
        {},
        {},
        {{"200",{{"description","UDF list"}}}}
    }});
    reg.registerRoute({"/api/v1/query/udfs/{name}", "get", {
        "Get a single AQL user-defined function definition", "", "getUdf", {"udf"},
        {RouteParam{"name","path",true,"UDF name",{{"type","string"}}}},
        {},
        {{"200",{{"description","UDF definition"}}},
         {"404",{{"description","UDF not found"}}}}
    }});
    reg.registerRoute({"/api/v1/query/udfs/{name}", "delete", {
        "Unregister an AQL user-defined function", "", "deleteUdf", {"udf"},
        {RouteParam{"name","path",true,"UDF name",{{"type","string"}}}},
        {},
        {{"204",{{"description","UDF deleted"}}},
         {"404",{{"description","UDF not found"}}}}
    }});

}

void registerGraphAndStreamingRoutes(RouteRegistry& reg) {
    // --- Changefeed Compact ---
    reg.registerRoute({"/changefeed/compact", "post", {
        "Compact changefeed log",
        "Runs a compaction pass retaining only the latest event per key. "
        "Requires cdc:admin scope.",
        "compactChangefeed", {"changefeed"}, {}, {},
        {{"200",{{"description","Compaction completed"}}},
         {"401",{{"description","Unauthorized"}}},
         {"404",{{"description","Feature cdc disabled"}}},
         {"503",{{"description","Service unavailable"}}},
         {"500",{{"description","Internal error"}}}}
    }});

    // --- Incremental Graph Query ---
    reg.registerRoute({"/graph/query/incremental", "post", {
        "Register incremental BFS graph query",
        "Registers a live BFS query that is automatically re-executed on graph changes.",
        "registerIncrementalGraphQuery", {"graph"},
        {},
        {{"required",true},{"content",{{"application/json",{{"schema",{{"type","object"}}}}}}}},
        {{"201",{{"description","Incremental query registered"}}},
         {"400",{{"description","Bad request"}}},
         {"401",{{"description","Unauthorized"}}},
         {"503",{{"description","Service unavailable"}}},
         {"500",{{"description","Internal error"}}}}
    }});
    reg.registerRoute({"/graph/query/incremental/{handle}", "delete", {
        "Unregister incremental BFS graph query",
        "Stops re-execution of the incremental query identified by handle.",
        "unregisterIncrementalGraphQuery", {"graph"},
        {RouteParam{"handle","path",true,"Registration handle",{{"type","integer"},{"format","int64"}}}},
        {},
        {{"200",{{"description","Query unregistered"}}},
         {"400",{{"description","Bad request"}}},
         {"401",{{"description","Unauthorized"}}},
         {"404",{{"description","Handle not found"}}},
         {"503",{{"description","Service unavailable"}}}}
    }});

    // --- Graph Changes Notification ---
    reg.registerRoute({"/graph/changes", "post", {
        "Notify graph of topology changes",
        "Pushes a batch of vertex/edge mutations; triggers re-execution of affected incremental queries.",
        "notifyGraphChanges", {"graph"},
        {},
        {{"required",true},{"content",{{"application/json",{{"schema",{{"type","object"}}}}}}}},
        {{"200",{{"description","Changes applied"}}},
         {"400",{{"description","Bad request"}}},
         {"401",{{"description","Unauthorized"}}},
         {"503",{{"description","Service unavailable"}}},
         {"500",{{"description","Internal error"}}}}
    }});

    // --- SSE Streaming Query ---
    reg.registerRoute({"/v2/query/stream", "get", {
        "Stream AQL query results via Server-Sent Events",
        "Executes an AQL query and streams each result document as an SSE 'data:' event. "
        "Requires query parameter 'q' (URL-encoded AQL string).",
        "queryStreamSse", {"query"},
        {RouteParam{"q","query",true,"URL-encoded AQL query",{{"type","string"}}},
         RouteParam{"max_seconds","query",false,"Max stream duration (1-60)",{{"type","integer"},{"default",30}}},
         RouteParam{"heartbeat_ms","query",false,"Heartbeat interval ms",{{"type","integer"},{"default",15000}}},
         RouteParam{"retry_ms","query",false,"SSE reconnect interval ms",{{"type","integer"},{"default",3000}}}},
        {},
        {{"200",{{"description","SSE stream"},
                 {"content",{{"text/event-stream",{{"schema",{{"type","string"}}}}}}}  }},
         {"400",{{"description","Missing or invalid query parameter"}}},
         {"401",{{"description","Unauthorized"}}},
         {"500",{{"description","Internal error"}}}}
    }});

}

void registerServerlessFunctionRoutes(RouteRegistry& reg) {
    // --- Serverless Functions ---
    reg.registerRoute({"/api/v1/functions", "post", {
        "Register a serverless function",
        "Registers a new in-process function defined by a JSON DSL code block.",
        "registerServerlessFunction", {"serverless"},
        {},
        {{"required",true},{"content",{{"application/json",{{"schema",{{"type","object"}}}}}}}},
        {{"201",{{"description","Function registered"}}},
         {"400",{{"description","Invalid function definition"}}},
         {"401",{{"description","Unauthorized"}}}}
    }});
    reg.registerRoute({"/api/v1/functions", "get", {
        "List registered serverless functions", "", "listServerlessFunctions", {"serverless"},
        {RouteParam{"tenant_id","query",false,"Filter by tenant",{{"type","string"}}}},
        {},
        {{"200",{{"description","List of functions"}}},
         {"401",{{"description","Unauthorized"}}}}
    }});
    reg.registerRoute({"/api/v1/functions/{id}", "get", {
        "Get serverless function definition", "", "getServerlessFunction", {"serverless"},
        {RouteParam{"id","path",true,"Function ID",{{"type","string"}}}},
        {},
        {{"200",{{"description","Function definition"}}},
         {"401",{{"description","Unauthorized"}}},
         {"404",{{"description","Function not found"}}}}
    }});
    reg.registerRoute({"/api/v1/functions/{id}", "put", {
        "Update serverless function definition", "", "updateServerlessFunction", {"serverless"},
        {RouteParam{"id","path",true,"Function ID",{{"type","string"}}}},
        {{"required",true},{"content",{{"application/json",{{"schema",{{"type","object"}}}}}}}},
        {{"200",{{"description","Function updated"}}},
         {"400",{{"description","Invalid definition"}}},
         {"401",{{"description","Unauthorized"}}},
         {"404",{{"description","Function not found"}}}}
    }});
    reg.registerRoute({"/api/v1/functions/{id}", "delete", {
        "Delete a serverless function", "", "deleteServerlessFunction", {"serverless"},
        {RouteParam{"id","path",true,"Function ID",{{"type","string"}}}},
        {},
        {{"200",{{"description","Function deleted"}}},
         {"401",{{"description","Unauthorized"}}},
         {"404",{{"description","Function not found"}}}}
    }});
    reg.registerRoute({"/api/v1/functions/{id}/invoke", "post", {
        "Invoke a serverless function",
        "Executes the function pipeline with the provided JSON payload.",
        "invokeServerlessFunction", {"serverless"},
        {RouteParam{"id","path",true,"Function ID",{{"type","string"}}}},
        {{"required",true},{"content",{{"application/json",{{"schema",{{"type","object"}}}}}}}},
        {{"200",{{"description","Function result"}}},
         {"400",{{"description","Bad request"}}},
         {"401",{{"description","Unauthorized"}}},
         {"404",{{"description","Function not found"}}},
         {"504",{{"description","Execution timed out"}}}}
    }});
    reg.registerRoute({"/api/v1/functions/{id}/versions", "get", {
        "List version history of a serverless function", "", "listServerlessFunctionVersions", {"serverless"},
        {RouteParam{"id","path",true,"Function ID",{{"type","string"}}}},
        {},
        {{"200",{{"description","Version history"}}},
         {"401",{{"description","Unauthorized"}}},
         {"404",{{"description","Function not found"}}}}
    }});

}

void registerTaskSchedulerRoutes(RouteRegistry& reg) {
    // --- Task Scheduler ---
    reg.registerRoute({"/api/tasks", "post", {
        "Register a new scheduled task",
        "Creates a new AQL-query or function task with interval, cron, or manual trigger.",
        "registerTask", {"tasks"},
        {},
        {{"required",true},{"content",{{"application/json",{{"schema",{{"type","object"}}}}}}}},
        {{"201",{{"description","Task created"}}},
         {"400",{{"description","Invalid task definition"}}},
         {"401",{{"description","Unauthorized"}}},
         {"403",{{"description","Forbidden"}}}}
    }});
    reg.registerRoute({"/api/tasks", "get", {
        "List all registered scheduled tasks", "", "listTasks", {"tasks"},
        {},
        {},
        {{"200",{{"description","Task list"}}},
         {"401",{{"description","Unauthorized"}}},
         {"403",{{"description","Forbidden"}}}}
    }});
    reg.registerRoute({"/api/tasks/stats", "get", {
        "Get task scheduler statistics", "", "getTaskStats", {"tasks"},
        {},
        {},
        {{"200",{{"description","Scheduler statistics"}}},
         {"401",{{"description","Unauthorized"}}},
         {"403",{{"description","Forbidden"}}}}
    }});
    reg.registerRoute({"/api/tasks/{id}", "get", {
        "Get a scheduled task by ID", "", "getTask", {"tasks"},
        {RouteParam{"id","path",true,"Task ID",{{"type","string"}}}},
        {},
        {{"200",{{"description","Task definition"}}},
         {"401",{{"description","Unauthorized"}}},
         {"403",{{"description","Forbidden"}}},
         {"404",{{"description","Task not found"}}}}
    }});
    reg.registerRoute({"/api/tasks/{id}", "put", {
        "Update an existing scheduled task", "", "updateTask", {"tasks"},
        {RouteParam{"id","path",true,"Task ID",{{"type","string"}}}},
        {{"required",true},{"content",{{"application/json",{{"schema",{{"type","object"}}}}}}}},
        {{"200",{{"description","Task updated"}}},
         {"400",{{"description","Invalid task definition"}}},
         {"401",{{"description","Unauthorized"}}},
         {"403",{{"description","Forbidden"}}},
         {"404",{{"description","Task not found"}}}}
    }});
    reg.registerRoute({"/api/tasks/{id}", "delete", {
        "Unregister a scheduled task", "", "deleteTask", {"tasks"},
        {RouteParam{"id","path",true,"Task ID",{{"type","string"}}}},
        {},
        {{"200",{{"description","Task deleted"}}},
         {"401",{{"description","Unauthorized"}}},
         {"403",{{"description","Forbidden"}}},
         {"404",{{"description","Task not found"}}}}
    }});
    reg.registerRoute({"/api/tasks/{id}/enable", "post", {
        "Enable (resume) a disabled task", "", "enableTask", {"tasks"},
        {RouteParam{"id","path",true,"Task ID",{{"type","string"}}}},
        {},
        {{"200",{{"description","Task enabled"}}},
         {"401",{{"description","Unauthorized"}}},
         {"403",{{"description","Forbidden"}}},
         {"404",{{"description","Task not found"}}}}
    }});
    reg.registerRoute({"/api/tasks/{id}/disable", "post", {
        "Disable (pause) a scheduled task", "", "disableTask", {"tasks"},
        {RouteParam{"id","path",true,"Task ID",{{"type","string"}}}},
        {},
        {{"200",{{"description","Task disabled"}}},
         {"401",{{"description","Unauthorized"}}},
         {"403",{{"description","Forbidden"}}},
         {"404",{{"description","Task not found"}}}}
    }});
    reg.registerRoute({"/api/tasks/{id}/execute", "post", {
        "Execute a task immediately (out of schedule)",
        "Triggers immediate execution of a task regardless of its scheduled trigger. "
        "Requires tasks:admin scope.",
        "executeTask", {"tasks"},
        {RouteParam{"id","path",true,"Task ID",{{"type","string"}}}},
        {},
        {{"200",{{"description","Task executed"}}},
         {"401",{{"description","Unauthorized"}}},
         {"403",{{"description","Forbidden – requires tasks:admin scope"}}},
         {"404",{{"description","Task not found"}}}}
    }});
    reg.registerRoute({"/ui/tasks", "get", {
        "Task Scheduler Web UI",
        "Self-contained single-page application for managing scheduled tasks.",
        "tasksWebUi", {"tasks"},
        {},
        {},
        {{"200",{{"description","HTML dashboard"},
                 {"content",{{"text/html",{{"schema",{{"type","string"}}}}}}}  }},
         {"401",{{"description","Unauthorized"}}},
         {"403",{{"description","Forbidden"}}}}
    }});
}

} // namespace

/**
 * @brief Populate the global registry with monitoring handler OpenAPI annotations.
 * @details Small registration batches bound MSVC compiler heap usage without
 * changing route order or metadata. Repeated calls replace the same routes;
 * allocation failures propagate to the caller.
 */
void MonitoringApiHandler::registerRoutes() {
    auto& reg = RouteRegistry::instance();
    registerHealthAndMetricsRoutes(reg);
    registerObservabilityRoutes(reg);
    registerLicenseRoutes(reg);
    registerEntityRoutes(reg);
    registerQueryRoutes(reg);
    registerGraphAndStreamingRoutes(reg);
    registerServerlessFunctionRoutes(reg);
    registerTaskSchedulerRoutes(reg);
}

} // namespace server
} // namespace themis
