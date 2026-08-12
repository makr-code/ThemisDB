/**
 * @file catalog_exporter.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=7, M=3, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#include "metadata/catalog_exporter.h"
#include <stdexcept>

#include <curl/curl.h>
#include <spdlog/spdlog.h>

namespace themis {

// ============================================================================
// libcurl helpers (implementation-private)
// ============================================================================

namespace {

static size_t curlWriteCallback(char* ptr, size_t size, size_t nmemb,
                                 void* userdata) {
    const auto total = size * nmemb;
    static_cast<std::string*>(userdata)->append(ptr, total);
    return total;
}

/// Execute a real HTTP POST using libcurl.
static int curlHttpPost(const std::string& url,
                        const std::string& body,
                        const std::string& auth_header,
                        int                timeout_ms,
                        std::string&       response_body) {
    response_body.clear();

    CURL* curl = curl_easy_init();
    if (!curl) {
        spdlog::error("CatalogExporter: Failed to initialise libcurl handle");
        return 0;
    }

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "Accept: application/json");
    if (!auth_header.empty()) {
        headers = curl_slist_append(headers, auth_header.c_str());
    }

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(body.size()));
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, static_cast<long>(timeout_ms));
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_body);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);

    const CURLcode res = curl_easy_perform(curl);

    int status_code = 0;
    if (res != CURLE_OK) {
        spdlog::error("CatalogExporter: libcurl error: {}", curl_easy_strerror(res));
    } else {
        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        status_code = static_cast<int>(http_code);
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    return status_code;
}

} // anonymous namespace

// ============================================================================
// Construction
// ============================================================================

CatalogExporter::CatalogExporter(Config config)
    : config_(std::move(config)) {}

// ============================================================================
// Public API
// ============================================================================

CatalogExporter::PublishResult
CatalogExporter::publishSchema(const std::vector<SchemaManager::TableSchema>& tables) {
    if (tables.empty()) {
        return {true, 0, ""};
    }

    switch (config_.type) {
        case CatalogType::APACHE_ATLAS: {
            auto payload = buildAtlasPayload(tables);
            return sendToAtlas(payload);
        }
        case CatalogType::DATAHUB: {
            auto proposals = buildDataHubProposals(tables);
            return sendToDataHub(proposals);
        }
    }

    return {false, 0, "Unknown catalog type"};
}

CatalogExporter::PublishResult
CatalogExporter::publishTable(const SchemaManager::TableSchema& table) {
    return publishSchema({table});
}

void CatalogExporter::setHttpPostForTesting(HttpPostFn fn) {
    http_post_fn_ = std::move(fn);
}

// ============================================================================
// Apache Atlas helpers
// ============================================================================

json CatalogExporter::buildAtlasPayload(
    const std::vector<SchemaManager::TableSchema>& tables) const {

    // Atlas bulk-entity request: { "entities": [ ... ] }
    json entities = json::array();

    // Top-level rdbms_db entity (one per publish call)
    json db_entity;
    db_entity["typeName"] = "rdbms_db";
    db_entity["attributes"]["name"]            = config_.database_name;
    db_entity["attributes"]["description"]     = "ThemisDB hybrid database";
    db_entity["attributes"]["prodOrOther"]      = "PROD";
    db_entity["attributes"]["qualifiedName"]   = config_.database_name;
    entities.push_back(db_entity);

    for (const auto& table : tables) {
        const std::string table_qname =
            config_.database_name + "." + table.name;

        // rdbms_table entity
        json tbl_entity;
        tbl_entity["typeName"] = "rdbms_table";
        tbl_entity["attributes"]["name"]          = table.name;
        tbl_entity["attributes"]["qualifiedName"] = table_qname;
        tbl_entity["attributes"]["tableType"]     = table.type;
        tbl_entity["attributes"]["description"]   =
            "Managed by ThemisDB (approx. " +
            std::to_string(table.estimated_row_count) + " rows)";

        // db reference (Atlas expects a reference object)
        json db_ref;
        db_ref["typeName"]                       = "rdbms_db";
        db_ref["uniqueAttributes"]["qualifiedName"] = config_.database_name;
        tbl_entity["attributes"]["db"]           = db_ref;

        // rdbms_column entities
        json columns = json::array();
        for (const auto& prop : table.properties) {
            const std::string col_qname = table_qname + "." + prop.name;

            json col_entity;
            col_entity["typeName"] = "rdbms_column";
            col_entity["attributes"]["name"]          = prop.name;
            col_entity["attributes"]["qualifiedName"] = col_qname;
            col_entity["attributes"]["dataType"]      = prop.type;
            col_entity["attributes"]["isNullable"]    = prop.nullable;
            col_entity["attributes"]["isPrimaryKey"]  = false;
            col_entity["attributes"]["isUnique"]      = false;

            json tbl_ref;
            tbl_ref["typeName"]                         = "rdbms_table";
            tbl_ref["uniqueAttributes"]["qualifiedName"] = table_qname;
            col_entity["attributes"]["table"]           = tbl_ref;

            entities.push_back(col_entity);

            // Column reference for table's column list
            json col_ref;
            col_ref["typeName"]                         = "rdbms_column";
            col_ref["uniqueAttributes"]["qualifiedName"] = col_qname;
            columns.push_back(col_ref);
        }
        tbl_entity["attributes"]["columns"] = columns;

        entities.push_back(tbl_entity);
    }

    json payload;
    payload["entities"] = entities;
    return payload;
}

CatalogExporter::PublishResult CatalogExporter::sendToAtlas(const json& payload) {
    const std::string url = config_.endpoint + "/api/atlas/v2/entity/bulk";

    // Basic auth encoded inline as "Authorization: Basic <base64(user:pass)>"
    // libcurl handles base64 encoding when we set CURLOPT_USERPWD; for our
    // injected test path we build the header string directly.
    std::string auth_header;
    if (!config_.username.empty()) {
        // Build a simple "user:pass" credential string for the Authorization header.
        // libcurl will base64-encode it when using CURLOPT_USERPWD, but since our
        // httpPost helper takes a pre-built header string we encode it here.
        const std::string credentials = config_.username + ":" + config_.password;

        // Manual base64 encoding (no external dependency required)
        static const char b64_table[] =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        std::string encoded_creds;
        const auto& src = credentials;
        for (size_t i = 0; i < src.size(); i += 3) {
            const unsigned char b0 = static_cast<unsigned char>(src[i]);
            const unsigned char b1 = (i + 1 < src.size()) ?
                static_cast<unsigned char>(src[i + 1]) : 0;
            const unsigned char b2 = (i + 2 < src.size()) ?
                static_cast<unsigned char>(src[i + 2]) : 0;

            encoded_creds += b64_table[(b0 >> 2) & 0x3F];
            encoded_creds += b64_table[((b0 & 0x03) << 4) | ((b1 >> 4) & 0x0F)];
            encoded_creds += (i + 1 < src.size()) ?
                b64_table[((b1 & 0x0F) << 2) | ((b2 >> 6) & 0x03)] : '=';
            encoded_creds += (i + 2 < src.size()) ?
                b64_table[b2 & 0x3F] : '=';
        }

        auth_header = "Authorization: Basic " + encoded_creds;
    }

    const std::string body = payload.dump();
    std::string response_body;

    spdlog::info("CatalogExporter: Publishing to Apache Atlas at {}", url);

    const int status = httpPost(url, body, auth_header, response_body);

    if (status == 200 || status == 201) {
        // Atlas returns {"mutatedEntities": {"CREATE": [...], "UPDATE": [...]}}
        int count = 0;
        try {
            auto resp = json::parse(response_body);
            if (resp.contains("mutatedEntities")) {
                for (const auto& [op, arr] : resp["mutatedEntities"].items()) {
                    if (arr.is_array()) count += static_cast<int>(arr.size());
                }
            }
        } catch (...) { /* count stays at 0 */ }

        // Use entity array size as lower-bound count when response is empty
        const int sent = static_cast<int>(
            payload.contains("entities") ? payload["entities"].size() : 0u);
        if (count == 0) count = sent;

        spdlog::info("CatalogExporter: Atlas accepted {} entities (HTTP {})",
                     count, status);
        return {true, count, ""};
    }

    const std::string err = "Atlas returned HTTP " + std::to_string(status) +
                            ": " + response_body;
    spdlog::error("CatalogExporter: {}", err);
    return {false, 0, err};
}

// ============================================================================
// DataHub helpers
// ============================================================================

json CatalogExporter::buildDataHubProposals(
    const std::vector<SchemaManager::TableSchema>& tables) const {

    json proposals = json::array();

    for (const auto& table : tables) {
        const std::string urn =
            "urn:li:dataset:(urn:li:dataPlatform:themisdb," +
            config_.database_name + "." + table.name + ",PROD)";

        // ── datasetProperties aspect ──────────────────────────────────────────
        {
            json proposal;
            proposal["entityType"]     = "dataset";
            proposal["entityUrn"]      = urn;
            proposal["changeType"]     = "UPSERT";
            proposal["aspectName"]     = "datasetProperties";

            json aspect;
            aspect["customProperties"]["database"]  = config_.database_name;
            aspect["customProperties"]["tableType"] = table.type;
            aspect["customProperties"]["rowCount"]  =
                std::to_string(table.estimated_row_count);
            aspect["description"] =
                "Managed by ThemisDB (approx. " +
                std::to_string(table.estimated_row_count) + " rows)";
            aspect["name"]  = table.name;
            aspect["tags"]  = json::array();
            proposal["aspect"] = aspect;

            proposals.push_back(proposal);
        }

        // ── schemaMetadata aspect ─────────────────────────────────────────────
        {
            json proposal;
            proposal["entityType"]  = "dataset";
            proposal["entityUrn"]   = urn;
            proposal["changeType"]  = "UPSERT";
            proposal["aspectName"]  = "schemaMetadata";

            json aspect;
            aspect["schemaName"]  = config_.database_name + "." + table.name;
            aspect["platform"]    = "urn:li:dataPlatform:themisdb";
            aspect["version"]     = 0;
            aspect["hash"]        = "";
            aspect["platformSchema"] = {
                {"com.linkedin.schema.OtherSchema", {{"rawSchema", table.name}}}
            };

            json fields = json::array();
            int field_order = 0;
            for (const auto& prop : table.properties) {
                json field;
                field["fieldPath"] = prop.name;
                field["type"] = {
                    {"type", {
                        {"com.linkedin.schema.StringType", json::object()}
                    }}
                };
                field["nativeDataType"] = prop.type;
                field["nullable"]       = prop.nullable;
                field["isPartOfKey"]    = false;
                field["fieldOrder"]     = field_order++;
                fields.push_back(field);
            }
            aspect["fields"] = fields;
            proposal["aspect"] = aspect;

            proposals.push_back(proposal);
        }
    }

    return proposals;
}

CatalogExporter::PublishResult
CatalogExporter::sendToDataHub(const json& proposals) {
    if (!proposals.is_array() || proposals.empty()) {
        return {true, 0, ""};
    }

    const std::string url =
        config_.endpoint + "/aspects?action=ingestProposal";

    std::string auth_header;
    if (!config_.token.empty()) {
        auth_header = "Authorization: Bearer " + config_.token;
    }

    int published = 0;
    for (const auto& proposal : proposals) {
        // DataHub GMS expects {"proposal": {...}}
        json wrapper;
        wrapper["proposal"] = proposal;

        const std::string body = wrapper.dump();
        std::string response_body;

        spdlog::debug("CatalogExporter: Sending DataHub proposal for {}",
                      proposal.value("entityUrn", "(unknown)"));

        const int status = httpPost(url, body, auth_header, response_body);

        if (status == 200 || status == 201) {
            ++published;
        } else {
            const std::string err =
                "DataHub returned HTTP " + std::to_string(status) +
                " for " + proposal.value("entityUrn", "?") +
                ": " + response_body;
            spdlog::error("CatalogExporter: {}", err);
            return {false, published, err};
        }
    }

    spdlog::info("CatalogExporter: Published {} proposals to DataHub", published);
    return {true, published, ""};
}

// ============================================================================
// HTTP helper
// ============================================================================

int CatalogExporter::httpPost(const std::string& url,
                               const std::string& body,
                               const std::string& auth_header,
                               std::string&       response_body) {
    if (http_post_fn_) {
        return http_post_fn_(url, body, auth_header, response_body);
    }
    return curlHttpPost(url, body, auth_header, config_.timeout_ms, response_body);
}

} // namespace themis


