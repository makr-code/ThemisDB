/**
 * @file content_toolbox_bridge.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.1.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=4, M=0, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

/*
 * ThemisDB | File: content_toolbox_bridge.cpp | Version: 0.1.0 | Last Modified: 2026-05-31 12:17:24
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 259
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=5, M=0, L=0
 * PR History (last 5): none
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#include "toolbox/content_toolbox_bridge.h"
#include "utils/logger.h"

#include <atomic>
#include <mutex>
#include <sstream>
#include <stdexcept>

namespace themis {
namespace toolbox {

// ─────────────────────────────────────────────────────────────────────────────
// ContentToolboxBridge::Impl
// ─────────────────────────────────────────────────────────────────────────────

class ContentToolboxBridge::Impl {
public:
    Impl(std::shared_ptr<IngestionToolbox>          toolbox,
         std::shared_ptr<content::ContentManager>   content_manager,
         std::shared_ptr<ingestion::IGraphWriter>   graph_writer,
         std::shared_ptr<ingestion::IVectorWriter>  vector_writer)
        : toolbox_(std::move(toolbox))
        , content_manager_(std::move(content_manager))
        , graph_writer_(std::move(graph_writer))
        , vector_writer_(std::move(vector_writer))
        , bridge_failures_total_(0)
        , graph_write_failures_total_(0)
        , vector_write_failures_total_(0)
    {}

    std::shared_ptr<IngestionToolbox>         toolbox_;
    std::shared_ptr<content::ContentManager>  content_manager_;
    std::shared_ptr<ingestion::IGraphWriter>  graph_writer_;
    std::shared_ptr<ingestion::IVectorWriter> vector_writer_;
    mutable std::mutex                        mutex_;

    // Phase 2.4: Prometheus metrics for bridge failures
    std::atomic<uint64_t> bridge_failures_total_;        ///< Total ingest/enrichment failures
    std::atomic<uint64_t> graph_write_failures_total_;   ///< Graph writer failures
    std::atomic<uint64_t> vector_write_failures_total_;  ///< Vector writer failures
};

// ─────────────────────────────────────────────────────────────────────────────
// ContentToolboxBridge public API
// ─────────────────────────────────────────────────────────────────────────────

ContentToolboxBridge::ContentToolboxBridge(
    std::shared_ptr<IngestionToolbox>         toolbox,
    std::shared_ptr<content::ContentManager>  content_manager,
    std::shared_ptr<ingestion::IGraphWriter>  graph_writer,
    std::shared_ptr<ingestion::IVectorWriter> vector_writer)
{
    // Phase 2.1: Comprehensive null-check with descriptive errors
    if (!toolbox) {
        throw std::invalid_argument(
            "ContentToolboxBridge: toolbox must not be null; "
            "construct IngestionToolbox via ToolboxBuilder.build() or IngestionToolbox::createDefault()");
    }
    if (!content_manager) {
        throw std::invalid_argument(
            "ContentToolboxBridge: content_manager must not be null; "
            "ensure content::ContentManager has been initialized before bridge construction");
    }
    
    // Phase 2.2: Log optional writer status
    if (!graph_writer) {
        THEMIS_DEBUG("ContentToolboxBridge: no graph_writer provided; "
                     "NER entities will not be persisted to graph store");
    }
    if (!vector_writer) {
        THEMIS_DEBUG("ContentToolboxBridge: no vector_writer provided; "
                     "embedding vectors will not be persisted to vector store");
    }
    
    impl_ = std::make_unique<Impl>(
        std::move(toolbox),
        std::move(content_manager),
        std::move(graph_writer),
        std::move(vector_writer));
}

ContentToolboxBridge::~ContentToolboxBridge() = default;

ContentToolboxBridge::ContentToolboxBridge(ContentToolboxBridge&&) noexcept = default;
ContentToolboxBridge& ContentToolboxBridge::operator=(ContentToolboxBridge&&) noexcept = default;

// ─────────────────────────────────────────────────────────────────────────────
// ingest()
// ─────────────────────────────────────────────────────────────────────────────

ContentToolboxBridge::BridgeResult ContentToolboxBridge::ingest(
    std::span<const std::byte> data,
    const std::string& filename,
    const std::string& mime_type,
    [[maybe_unused]] const std::string& collection,
    const std::string& user_context)
{
    BridgeResult out;

    // Phase 2.3: Early validation for empty input
    if (data.empty()) {
        out.ok = false;
        out.error = "ingest: data span is empty; cannot ingest zero-byte content";
        impl_->bridge_failures_total_.fetch_add(1);
        return out;
    }

    // ── Step 1: ContentManager ingest (security, dedup, storage, embeddings)
    std::string blob(reinterpret_cast<const char*>(data.data()), data.size());

    content::ContentManager::IngestResult cm_result;
    {
        std::lock_guard<std::mutex> lk(impl_->mutex_);
        cm_result = impl_->content_manager_->ingestRawBlob(
            blob, filename, mime_type, user_context);
    }

    if (!cm_result.success) {
        out.error = "ContentManager::ingestRawBlob failed: " + cm_result.error_message;
        impl_->bridge_failures_total_.fetch_add(1);
        THEMIS_WARN("ContentToolboxBridge::ingest: ContentManager failed: {}", out.error);
        return out;
    }

    out.content_id  = cm_result.primary_content_id;
    out.child_ids   = cm_result.extracted_content_ids;

    // ── Step 2: Retrieve extracted text for Toolbox enrichment
    std::string extracted_text;
    {
        std::lock_guard<std::mutex> lk(impl_->mutex_);
        auto assembly = impl_->content_manager_->assembleContent(
            cm_result.primary_content_id, /*include_text=*/true);
        if (assembly && assembly->assembled_text) {
            extracted_text = *assembly->assembled_text;
        }
    }

    if (extracted_text.empty()) {
        // No text available (binary-only content or extraction failed).
        // Return the content_id from ContentManager but skip enrichment.
        out.ok = true;
        return out;
    }

    // Toolbox enrichment — capture pointer under lock, call outside
    // IngestionToolbox is internally thread-safe; holding the lock during
    // the potentially expensive extraction would serialize all concurrent
    // ingest calls unnecessarily.
    std::shared_ptr<IngestionToolbox> toolbox_ptr;
    {
        std::lock_guard<std::mutex> lk(impl_->mutex_);
        toolbox_ptr = impl_->toolbox_;
    }
    
    // Phase 2.5: Validate toolbox is available before enrichment
    if (!toolbox_ptr) {
        out.ok = false;
        out.error = "toolbox not initialized; cannot enrich extracted content";
        impl_->bridge_failures_total_.fetch_add(1);
        THEMIS_ERROR("ContentToolboxBridge::ingest: toolbox null check failed for content_id '{}'",
                     out.content_id);
        return out;
    }
    
    ingestion::BaseEntitySet entity_set =
        toolbox_ptr->extractEntitySet(extracted_text, mime_type, filename);
    out.entities = entity_set.nodes;
    out.vectors  = entity_set.chunks;

    // ── Step 4: Write entities to graph store sink
    bool sinks_written = false;
    if (impl_->graph_writer_ && !entity_set.nodes.empty()) {
        auto res = impl_->graph_writer_->writeEntities(entity_set.nodes);
        if (!res) {
            impl_->graph_write_failures_total_.fetch_add(1);
            THEMIS_WARN("ContentToolboxBridge: graph_writer failed for '{}': {}",
                        out.content_id, res.error().message());
        } else {
            sinks_written = true;
        }
    }

    // ── Step 5: Write vector records to vector store sink
    if (impl_->vector_writer_ && !out.vectors.empty()) {
        auto res = impl_->vector_writer_->writeVectors(out.vectors);
        if (!res) {
            impl_->vector_write_failures_total_.fetch_add(1);
            THEMIS_WARN("ContentToolboxBridge: vector_writer failed for '{}': {}",
                        out.content_id, res.error().message());
        } else {
            sinks_written = true;
        }
    }

    out.sinks_written = sinks_written;
    out.ok = true;
    return out;
}

// ─────────────────────────────────────────────────────────────────────────────
// enrichExisting()
// ─────────────────────────────────────────────────────────────────────────────

ContentToolboxBridge::BridgeResult ContentToolboxBridge::enrichExisting(
    const std::string& content_id,
    [[maybe_unused]] const std::string& collection)
{
    BridgeResult out;
    out.content_id = content_id;

    // Phase 2.6: Validate input before processing
    if (content_id.empty()) {
        out.ok = false;
        out.error = "enrichExisting: content_id must not be empty";
        impl_->bridge_failures_total_.fetch_add(1);
        THEMIS_WARN("ContentToolboxBridge::enrichExisting: empty content_id provided");
        return out;
    }

    // Retrieve metadata to get MIME type and filename hint
    std::string mime_type;
    std::string filename_hint;
    std::string extracted_text;

    {
        std::lock_guard<std::mutex> lk(impl_->mutex_);

        auto meta = impl_->content_manager_->getContentMeta(content_id);
        if (!meta) {
            out.error = "enrichExisting: content_id '" + content_id + "' not found in ContentManager";
            impl_->bridge_failures_total_.fetch_add(1);
            THEMIS_WARN("ContentToolboxBridge::enrichExisting: content '{}' not found", content_id);
            return out;
        }
        mime_type     = meta->mime_type;
        filename_hint = meta->original_filename;

        auto assembly = impl_->content_manager_->assembleContent(
            content_id, /*include_text=*/true);
        if (assembly && assembly->assembled_text) {
            extracted_text = *assembly->assembled_text;
        }
    }

    if (extracted_text.empty()) {
        out.ok = true;  // No text — nothing to enrich
        return out;
    }

    // Toolbox enrichment — capture pointer under lock, call outside
    std::shared_ptr<IngestionToolbox> toolbox_ptr2;
    {
        std::lock_guard<std::mutex> lk(impl_->mutex_);
        toolbox_ptr2 = impl_->toolbox_;
    }
    
    // Phase 2.7: Validate toolbox is available before enrichment
    if (!toolbox_ptr2) {
        out.ok = false;
        out.error = "toolbox not initialized; cannot enrich existing content for '" + content_id + "'";
        impl_->bridge_failures_total_.fetch_add(1);
        THEMIS_ERROR("ContentToolboxBridge::enrichExisting: toolbox null check failed for content_id '{}'",
                     content_id);
        return out;
    }
    
    ingestion::BaseEntitySet entity_set =
        toolbox_ptr2->extractEntitySet(extracted_text, mime_type, filename_hint);
    out.entities = entity_set.nodes;
    out.vectors  = entity_set.chunks;

    bool sinks_written = false;
    if (impl_->graph_writer_ && !entity_set.nodes.empty()) {
        auto res = impl_->graph_writer_->writeEntities(entity_set.nodes);
        if (!res) {
            impl_->graph_write_failures_total_.fetch_add(1);
            THEMIS_WARN("ContentToolboxBridge::enrichExisting: graph_writer failed for '{}': {}",
                        content_id, res.error().message());
        } else {
            sinks_written = true;
        }
    }

    if (impl_->vector_writer_ && !out.vectors.empty()) {
        auto res = impl_->vector_writer_->writeVectors(out.vectors);
        if (!res) {
            impl_->vector_write_failures_total_.fetch_add(1);
            THEMIS_WARN("ContentToolboxBridge::enrichExisting: vector_writer failed for '{}': {}",
                        content_id, res.error().message());
        } else {
            sinks_written = true;
        }
    }

    out.sinks_written = sinks_written;
    out.ok = true;
    return out;
}

// ─────────────────────────────────────────────────────────────────────────────
// Accessors
// ─────────────────────────────────────────────────────────────────────────────

std::shared_ptr<IngestionToolbox> ContentToolboxBridge::toolbox() const {
    return impl_->toolbox_;
}

std::shared_ptr<content::ContentManager> ContentToolboxBridge::contentManager() const {
    return impl_->content_manager_;
}

std::shared_ptr<ingestion::IGraphWriter> ContentToolboxBridge::graphWriter() const {
    return impl_->graph_writer_;
}

std::shared_ptr<ingestion::IVectorWriter> ContentToolboxBridge::vectorWriter() const {
    return impl_->vector_writer_;
}

// ─────────────────────────────────────────────────────────────────────────────
// Prometheus metrics
// ─────────────────────────────────────────────────────────────────────────────

uint64_t ContentToolboxBridge::failuresTotal() const noexcept {
    return impl_->bridge_failures_total_.load();
}

uint64_t ContentToolboxBridge::graphWriteFailuresTotal() const noexcept {
    return impl_->graph_write_failures_total_.load();
}

uint64_t ContentToolboxBridge::vectorWriteFailuresTotal() const noexcept {
    return impl_->vector_write_failures_total_.load();
}

} // namespace toolbox
} // namespace themis
