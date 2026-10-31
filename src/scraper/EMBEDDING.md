---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Scraper Embedding Boundary

**Purpose and audience:** Clarifies the vector-record contract for developers integrating scraper output with embedding or ANN components.

## Current behavior

The scraper defines `ScraperVectorRecord` with a document ID, gap ID, source URL, quality score, provenance fields, and `std::vector<float> embedding`. The header explicitly describes the embedding as externally produced and empty until generated. `ScraperRecordBuilder::buildVector()` copies record metadata; it does not call an embedding model, tokenize text, choose dimensions, normalize vectors, or validate vector contents.

The default `ScraperPlugin` constructs an `InMemoryScraperMetadataWriter`. The plugin builds a vector record and passes it to `IScraperMetadataWriter`; the standard builder currently leaves `embedding` empty. Consequently, successful scraper evaluation or writing does not prove that an embedding exists or that ANN indexing can return the document.

## Integration contract

An embedding provider and its model/version, input text policy, dimensions, normalization, error behavior, and retry semantics are outside the current scraper API. A writer integration must not treat an empty vector as a valid embedding. Before enabling a vector-backed consumer, verify that the upstream integration populates a non-empty vector with the expected dimension and preserves the record's `doc_id`, source URL, and mandatory scraper provenance.

Do not silently generate embeddings in a writer adapter with unspecified model behavior: make ownership and failure semantics explicit in the integration contract. If embedding generation fails, surface it as an integration/write failure rather than reporting the vector as usable.

## Validation

For an embedding-enabled integration, test the builder's current empty-vector behavior, the external provider's dimensions and finite values, and writer behavior on missing or rejected vectors. Verify provenance and document identity survive the embedding step. The scraper module itself has no embedding quality metric or embedding benchmark; evaluate retrieval quality in the consuming subsystem.

Evidence: [`scraper_metadata_writer.h`](../../include/scraper/scraper_metadata_writer.h), [`scraper_metadata_writer.cpp`](./scraper_metadata_writer.cpp), and [API.md](./API.md).

**AI-assisted documentation:** Exact model backend not verifiable from final document text alone.
