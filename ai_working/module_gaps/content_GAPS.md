# content Module - Developer Gap Note

> Auto-generated from ai_working\gap_scan_results.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: content
- Generated: 2026-06-04 08:50:22
- Status: Critical Findings Present
- Total Findings: 500
- Actionable Findings (Critical + High): 333
- Affected Files: 36

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 47 |
| High | 286 |
| Medium | 153 |
| Low | 14 |

## Category Summary

| Category | Count |
|---|---:|
| pointer_arithmetic_unbounded | 191 |
| string_concat_loop | 33 |
| resource_leaked_in_exception | 26 |
| data_race | 24 |
| manual_cleanup | 16 |
| hardcoded_path | 15 |
| null_dereference | 15 |
| uncaught_exception | 14 |
| unordered_container_iter | 13 |
| stale_doc_section_reference | 11 |
| cast_to_smaller_type | 9 |
| copy_overhead | 9 |
| uninitialized_access | 9 |
| generic_catch | 8 |
| hardcoded_output | 7 |
| array_bounds_violation | 6 |
| legacy_or_compat_path | 6 |
| missing_latency_metric | 6 |
| no_timeout | 6 |
| arithmetic_overflow | 5 |
| missing_resource_limits | 5 |
| missing_vector_reserve | 5 |
| unnecessary_copy | 5 |
| unstructured_log | 5 |
| range_temporary | 4 |
| blocking_no_timeout | 3 |
| delete_without_nullptr | 3 |
| explicit_delete | 3 |
| allocation_loop | 2 |
| deadlock_risk | 2 |
| expensive_inner_op | 2 |
| fp_exact_comparison | 2 |
| map_vs_unordered_map | 2 |
| module_doc_linkset_drift | 2 |
| nested_loop_find | 2 |
| o_n_squared | 2 |
| posix_only_api | 2 |
| primitive_no_volatile | 2 |
| stale_read_undocumented | 2 |
| thread_join_no_timeout | 2 |
| array_bounds | 1 |
| delete_no_nullptr | 1 |
| exception_in_destructor | 1 |
| missing_move_constructor_defaulted | 1 |
| missing_version_tracking | 1 |
| multiplication_overflow | 1 |
| new_without_raii | 1 |
| regex_in_loop | 1 |
| smart_ptr_misuse | 1 |
| unchecked_array_index | 1 |
| undefined_conflict_resolution | 1 |
| uninitialized_array | 1 |
| uninitialized_member_field | 1 |
| unspecified_consistency | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| content/office_processor.cpp | 74 | 3 | 61 | 10 | 0 |
| content/content_manager.cpp | 65 | 11 | 23 | 31 | 0 |
| content/video_processor.cpp | 49 | 6 | 32 | 11 | 0 |
| content/content_logger.cpp | 31 | 0 | 25 | 1 | 5 |
| content/pdf_processor.cpp | 26 | 3 | 13 | 10 | 0 |
| content/async_ingestion_worker.cpp | 23 | 10 | 9 | 4 | 0 |
| content/stt_processor.cpp | 23 | 0 | 15 | 6 | 2 |
| content/audio_processor.cpp | 22 | 0 | 19 | 3 | 0 |
| content/image_processor.cpp | 22 | 7 | 11 | 3 | 1 |
| content/html_processor.cpp | 20 | 0 | 8 | 12 | 0 |
| content/cad_processor.cpp | 15 | 0 | 13 | 2 | 0 |
| content/mime_detector.cpp | 13 | 1 | 1 | 9 | 2 |
| content/geo_processor.cpp | 12 | 1 | 9 | 2 | 0 |
| content/archive_processor.cpp | 10 | 0 | 3 | 5 | 2 |
| content/content_fs.cpp | 10 | 2 | 4 | 4 | 0 |
| content/ocr_processor.cpp | 10 | 0 | 7 | 3 | 0 |
| content/text_processor.cpp | 10 | 0 | 8 | 2 | 0 |
| content/content_manager_llm.cpp | 9 | 0 | 2 | 7 | 0 |
| content/markdown_processor.cpp | 8 | 0 | 3 | 5 | 0 |
| content/tts_processor.cpp | 8 | 1 | 4 | 3 | 0 |
| content/content_security.cpp | 6 | 0 | 2 | 4 | 0 |
| content/pipeline/multimodal_chunker.cpp | 5 | 1 | 1 | 3 | 0 |
| content/content_metrics.cpp | 4 | 0 | 1 | 3 | 0 |
| content/content_type.cpp | 4 | 0 | 2 | 2 | 0 |
| content/version_manager.cpp | 4 | 0 | 4 | 0 | 0 |
| content/language_detector.cpp | 3 | 0 | 1 | 2 | 0 |
| content/abuse_detector.cpp | 2 | 0 | 1 | 1 | 0 |
| content/embedding_pipeline.cpp | 2 | 0 | 1 | 1 | 0 |
| content/mock_clip_processor.cpp | 2 | 0 | 2 | 0 | 0 |
| content/pipeline/bulk_upload_interface.cpp | 2 | 0 | 0 | 2 | 0 |
| content/FUTURE_ENHANCEMENTS.md | 1 | 0 | 0 | 0 | 1 |
| content/PRODUCTION_REQUIREMENTS.md | 1 | 0 | 0 | 0 | 1 |
| content/content_validator.cpp | 1 | 0 | 0 | 1 | 0 |
| content/deduplication_checker.cpp | 1 | 1 | 0 | 0 | 0 |
| content/pipeline/async_bulk_uploader.cpp | 1 | 0 | 0 | 1 | 0 |
| content/pipeline/zstd_compression.cpp | 1 | 0 | 1 | 0 | 0 |

## Full Scanner Findings

### content/office_processor.cpp
Total findings: 74

- Line 844: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: ssize_t written = write(in_fd, bdata, remaining);
- Line 1004: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: int out_fd = open(out_path.c_str(), O_RDONLY);
- Line 1015: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: while ((n = read(out_fd, buf, sizeof(buf))) > 0) {
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #3780 fix(content/security): CON-... (2026-03-12) | #3738 feat(content): Libr
- Line 97: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Check for legacy Office formats (OLE Compound Document)
- Line 103: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Legacy Office format - try to determine type
- Line 162: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: // Detect document type

    OfficeDocumentType doc_type   = detectDocumentType(blob);

    result.metadata["size_bytes"] = blob.size();



    switch (doc_type) {

        case OfficeDocumentType::DOCX:
- Line 166: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: switch (doc_type) {

        case OfficeDocumentType::DOCX:

            result.metadata["document_type"] = "docx";

            result.metadata["mime_type"]     = DOCX_CONTENT_TYPE;

            result                           = extractDOCX(blob);

            break;
- Line 167: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: switch (doc_type) {

        case OfficeDocumentType::DOCX:

            result.metadata["document_type"] = "docx";

            result.metadata["mime_type"]     = DOCX_CONTENT_TYPE;

            result                           = extractDOCX(blob);

            break;
- Line 172: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: break;



        case OfficeDocumentType::XLSX:

            result.metadata["document_type"] = "xlsx";

            result.metadata["mime_type"]     = XLSX_CONTENT_TYPE;

            result                           = extractXLSX(blob);

            break;
- Line 173: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: case OfficeDocumentType::XLSX:

            result.metadata["document_type"] = "xlsx";

            result.metadata["mime_type"]     = XLSX_CONTENT_TYPE;

            result                           = extractXLSX(blob);

            break;
- Line 178: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: break;



        case OfficeDocumentType::PPTX:

            result.metadata["document_type"] = "pptx";

            result.metadata["mime_type"]     = PPTX_CONTENT_TYPE;

            result                           = extractPPTX(blob);

            break;
- Line 179: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: case OfficeDocumentType::PPTX:

            result.metadata["document_type"] = "pptx";

            result.metadata["mime_type"]     = PPTX_CONTENT_TYPE;

            result                           = extractPPTX(blob);

            break;
- Line 186: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: case OfficeDocumentType::ODT:

        case OfficeDocumentType::ODS:

        case OfficeDocumentType::ODP:

            result.metadata["document_type"] = "odf";

            result                           = extractODF(blob, doc_type);

            break;
- Line 197: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: break;



        case OfficeDocumentType::RTF:

            result.metadata["document_type"] = "rtf";

            // Basic RTF text extraction

            {

                std::string text;
- Line 214: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: }

                }

                result.text                          = text;

                result.metadata["extraction_method"] = "basic_rtf";

                result.ok                            = true;

            }

            break;
- Line 240: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: ExtractionResult result;

    result.ok                        = false;

    result.metadata                  = json::object();

    result.metadata["document_type"] = "docx";

    result.metadata["mime_type"]     = DOCX_CONTENT_TYPE;



#if OFFICE_LIBRARY_AVAILABLE
- Line 241: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: result.ok                        = false;

    result.metadata                  = json::object();

    result.metadata["document_type"] = "docx";

    result.metadata["mime_type"]     = DOCX_CONTENT_TYPE;



#if OFFICE_LIBRARY_AVAILABLE

    try {
- Line 255: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: // Extract metadata

        OfficeMetadata metadata          = extractOOXMLMetadata(blob);

        result.metadata["title"]         = metadata.title;

        result.metadata["author"]        = metadata.author;

        result.metadata["subject"]       = metadata.subject;

        result.metadata["keywords"]      = metadata.keywords;
- Line 256: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: // Extract metadata

        OfficeMetadata metadata          = extractOOXMLMetadata(blob);

        result.metadata["title"]         = metadata.title;

        result.metadata["author"]        = metadata.author;

        result.metadata["subject"]       = metadata.subject;

        result.metadata["keywords"]      = metadata.keywords;

        result.metadata["created_date"]  = metadata.created_date;
- Line 257: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: OfficeMetadata metadata          = extractOOXMLMetadata(blob);

        result.metadata["title"]         = metadata.title;

        result.metadata["author"]        = metadata.author;

        result.metadata["subject"]       = metadata.subject;

        result.metadata["keywords"]      = metadata.keywords;

        result.metadata["created_date"]  = metadata.created_date;

        result.metadata["modified_date"] = metadata.modified_date;
- Line 258: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: result.metadata["title"]         = metadata.title;

        result.metadata["author"]        = metadata.author;

        result.metadata["subject"]       = metadata.subject;

        result.metadata["keywords"]      = metadata.keywords;

        result.metadata["created_date"]  = metadata.created_date;

        result.metadata["modified_date"] = metadata.modified_date;

        result.metadata["application"]   = metadata.application;
- Line 259: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: result.metadata["author"]        = metadata.author;

        result.metadata["subject"]       = metadata.subject;

        result.metadata["keywords"]      = metadata.keywords;

        result.metadata["created_date"]  = metadata.created_date;

        result.metadata["modified_date"] = metadata.modified_date;

        result.metadata["application"]   = metadata.application;
- Line 260: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: result.metadata["subject"]       = metadata.subject;

        result.metadata["keywords"]      = metadata.keywords;

        result.metadata["created_date"]  = metadata.created_date;

        result.metadata["modified_date"] = metadata.modified_date;

        result.metadata["application"]   = metadata.application;



        // Parse XML and extract text
- Line 261: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: result.metadata["keywords"]      = metadata.keywords;

        result.metadata["created_date"]  = metadata.created_date;

        result.metadata["modified_date"] = metadata.modified_date;

        result.metadata["application"]   = metadata.application;



        // Parse XML and extract text

        pugi::xml_document doc;
- Line 289: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: }



        result.text                        = all_text.str();

        result.metadata["paragraph_count"] = paragraphs.size();

        result.metadata["token_count"]     = countTokens(result.text);

        result.ok                          = true;
- Line 290: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: result.text                        = all_text.str();

        result.metadata["paragraph_count"] = paragraphs.size();

        result.metadata["token_count"]     = countTokens(result.text);

        result.ok                          = true;



    } catch (const std::exception &e) {
- Line 298: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: }

#else

    // Fallback: Basic extraction without libzip/pugixml

    result.metadata["note"] = "Full DOCX extraction requires building with -DTHEMIS_ENABLE_OFFICE=ON";



    // Try to find text between <w:t> tags

    std::regex text_regex("<w:t[^>]*>([^<]+)</w:t>");
- Line 312: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: }



    result.text                          = extracted.str();

    result.metadata["extraction_method"] = "basic_regex";

    result.metadata["token_count"]       = countTokens(result.text);

    result.ok                            = true;

#endif
- Line 313: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: result.text                          = extracted.str();

    result.metadata["extraction_method"] = "basic_regex";

    result.metadata["token_count"]       = countTokens(result.text);

    result.ok                            = true;

#endif
- Line 324: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: ExtractionResult result;

    result.ok                        = false;

    result.metadata                  = json::object();

    result.metadata["document_type"] = "xlsx";

    result.metadata["mime_type"]     = XLSX_CONTENT_TYPE;



#if OFFICE_LIBRARY_AVAILABLE
- Line 325: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: result.ok                        = false;

    result.metadata                  = json::object();

    result.metadata["document_type"] = "xlsx";

    result.metadata["mime_type"]     = XLSX_CONTENT_TYPE;



#if OFFICE_LIBRARY_AVAILABLE

    try {
- Line 349: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: // Extract metadata

        OfficeMetadata metadata          = extractOOXMLMetadata(blob);

        result.metadata["title"]         = metadata.title;

        result.metadata["author"]        = metadata.author;

        result.metadata["created_date"]  = metadata.created_date;

        result.metadata["modified_date"] = metadata.modified_date;
- Line 350: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: // Extract metadata

        OfficeMetadata metadata          = extractOOXMLMetadata(blob);

        result.metadata["title"]         = metadata.title;

        result.metadata["author"]        = metadata.author;

        result.metadata["created_date"]  = metadata.created_date;

        result.metadata["modified_date"] = metadata.modified_date;
- Line 351: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: OfficeMetadata metadata          = extractOOXMLMetadata(blob);

        result.metadata["title"]         = metadata.title;

        result.metadata["author"]        = metadata.author;

        result.metadata["created_date"]  = metadata.created_date;

        result.metadata["modified_date"] = metadata.modified_date;



        // Read workbook.xml to get sheet names
- Line 352: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: result.metadata["title"]         = metadata.title;

        result.metadata["author"]        = metadata.author;

        result.metadata["created_date"]  = metadata.created_date;

        result.metadata["modified_date"] = metadata.modified_date;



        // Read workbook.xml to get sheet names

        std::string workbook_xml = readZipEntry(blob, "xl/workbook.xml");
- Line 369: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: }

        }



        result.metadata["sheet_count"] = sheet_names.size();

        result.metadata["sheet_names"] = sheet_names;



        // Read first sheet
- Line 370: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: }



        result.metadata["sheet_count"] = sheet_names.size();

        result.metadata["sheet_names"] = sheet_names;



        // Read first sheet

        std::string sheet1_xml = readZipEntry(blob, "xl/worksheets/sheet1.xml");
- Line 429: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: result.metadata["token_count"] = countTokens(result.text);
- Line 436: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: result.error_message = std::string("XLSX extraction error: ") + e.what();

    }

#else

    result.metadata["note"]              = "Full XLSX extraction requires building with -DTHEMIS_ENABLE_OFFICE=ON";

    result.metadata["extraction_method"] = "not_available";

    result.text                          = "";

    result.ok                            = true;
- Line 437: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: }

#else

    result.metadata["note"]              = "Full XLSX extraction requires building with -DTHEMIS_ENABLE_OFFICE=ON";

    result.metadata["extraction_method"] = "not_available";

    result.text                          = "";

    result.ok                            = true;

#endif
- Line 449: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: ExtractionResult result;

    result.ok                        = false;

    result.metadata                  = json::object();

    result.metadata["document_type"] = "pptx";

    result.metadata["mime_type"]     = PPTX_CONTENT_TYPE;



#if OFFICE_LIBRARY_AVAILABLE
- Line 450: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: result.ok                        = false;

    result.metadata                  = json::object();

    result.metadata["document_type"] = "pptx";

    result.metadata["mime_type"]     = PPTX_CONTENT_TYPE;



#if OFFICE_LIBRARY_AVAILABLE

    try {
- Line 456: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: try {

        // Extract metadata

        OfficeMetadata metadata          = extractOOXMLMetadata(blob);

        result.metadata["title"]         = metadata.title;

        result.metadata["author"]        = metadata.author;

        result.metadata["created_date"]  = metadata.created_date;

        result.metadata["modified_date"] = metadata.modified_date;
- Line 457: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: // Extract metadata

        OfficeMetadata metadata          = extractOOXMLMetadata(blob);

        result.metadata["title"]         = metadata.title;

        result.metadata["author"]        = metadata.author;

        result.metadata["created_date"]  = metadata.created_date;

        result.metadata["modified_date"] = metadata.modified_date;
- Line 458: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: OfficeMetadata metadata          = extractOOXMLMetadata(blob);

        result.metadata["title"]         = metadata.title;

        result.metadata["author"]        = metadata.author;

        result.metadata["created_date"]  = metadata.created_date;

        result.metadata["modified_date"] = metadata.modified_date;



        // List slides
- Line 459: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: result.metadata["title"]         = metadata.title;

        result.metadata["author"]        = metadata.author;

        result.metadata["created_date"]  = metadata.created_date;

        result.metadata["modified_date"] = metadata.modified_date;



        // List slides

        std::vector<std::string> slide_files = listZipEntries(blob);
- Line 465: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: if (entry.find("ppt/slides/slide") != std::string::npos && entry.find(".xml") != std::string::npos)
- Line 474: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: // Sort slides by number

        std::sort(slides.begin(), slides.end());



        result.metadata["slide_count"] = slides.size();



        std::ostringstream all_text;

        int slide_num = 1;
- Line 526: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: }



        result.text                    = all_text.str();

        result.metadata["token_count"] = countTokens(result.text);

        result.ok                      = true;



    } catch (const std::exception &e) {
- Line 533: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: result.error_message = std::string("PPTX extraction error: ") + e.what();

    }

#else

    result.metadata["note"]              = "Full PPTX extraction requires building with -DTHEMIS_ENABLE_OFFICE=ON";

    result.metadata["extraction_method"] = "not_available";

    result.text                          = "";

    result.ok                            = true;
- Line 534: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: }

#else

    result.metadata["note"]              = "Full PPTX extraction requires building with -DTHEMIS_ENABLE_OFFICE=ON";

    result.metadata["extraction_method"] = "not_available";

    result.text                          = "";

    result.ok                            = true;

#endif
- Line 562: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: type_str = "odf";

            break;

    }

    result.metadata["document_type"] = type_str;



#if OFFICE_LIBRARY_AVAILABLE

    try {
- Line 589: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: } else if (std::string(child.name()).find("text:") == 0) {
- Line 599: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: }



        result.text                    = all_text.str();

        result.metadata["token_count"] = countTokens(result.text);

        result.ok                      = true;



    } catch (const std::exception &e) {
- Line 606: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: result.error_message = std::string("ODF extraction error: ") + e.what();

    }

#else

    result.metadata["note"] = "Full ODF extraction requires building with -DTHEMIS_ENABLE_OFFICE=ON";

    result.text             = "";

    result.ok               = true;

#endif
- Line 749: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // LibreOffice headless fallback for legacy OLE formats (DOC/XLS/PPT)
- Line 777: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: result.metadata["document_type"]     = type_str;
- Line 778: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: result.metadata["extraction_method"] = "libreoffice_headless";
- Line 815: severity=HIGH; category=posix_only_api
  Description: POSIX-only API unlink( without platform guard
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: unlink(in_file.c_str());
- Line 817: severity=HIGH; category=posix_only_api
  Description: POSIX-only API unlink( without platform guard
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: unlink(out_file.c_str());
- Line 979: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: if (timed_out) {

        result.error_message = "LibreOffice conversion timed out after " + std::to_string(timeout_sec) + " seconds";

        result.metadata["timed_out"] = true;

        return result;

    }
- Line 1027: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: }



    result.text                    = extracted_text;

    result.metadata["token_count"] = countTokens(extracted_text);

    result.metadata["size_bytes"]  = blob.size();

    result.ok                      = true;

    return result;
- Line 1028: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: result.text                    = extracted_text;

    result.metadata["token_count"] = countTokens(extracted_text);

    result.metadata["size_bytes"]  = blob.size();

    result.ok                      = true;

    return result;
- Line 209: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: text += content + " ";
- Line 641: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: zip_close(archive);
- Line 731: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: zip_close(archive);
- Line 848: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: close(in_fd);
- Line 855: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: close(in_fd);
- Line 1019: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: close(out_fd);
- Line 1023: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: close(out_fd);
- Line 1077: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: current_chunk += "\n";
- Line 1101: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'OfficeProcessor Embedding Integration).' that was not found in 'src/content/FUTURE_ENHANCEMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/content/FUTURE_ENHANCEMENTS.md §OfficeProcessor Embedding Integration).
- Line 1125: severity=MEDIUM; category=cast_to_smaller_type
  Description: Explicit cast to int detected (verify no overflow on source)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::type_conversion
  Context: ['            const size_t combined = token_hash ^ (i * 31u) ^ (static_cast<size_t>(seed) * 97u);', '            for (int d = 0; d < 10; ++d) {', '                const int dim = static_cast<int>((combined + static_cast<size_t>(d) * 73u) % static_cast<size_t>(kDim));', '                const float weight = 1.0f / (1.0f + static_cast<float>(i) * 0.1f);', '                const float phase']

### content/content_manager.cpp
Total findings: 65

- Line 862: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto emb = embedding_pipeline_->generateEmbedding(c.text);
- Line 900: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: if (vector_index_->getDimension() == static_cast<int>(c.embedding.size())) {
- Line 1372: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto [st, results] = vector_index_->searchKnn(q, static_cast<size_t>(k), wptr);
- Line 2175: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: dup = dedup_checker_->isDuplicateImage(cached_phash);
- Line 2180: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: dup = dedup_checker_->isDuplicateText(cached_minhash);
- Line 2189: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: result.primary_content_id = dup->existing_id;
- Line 2636: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: const bool stream_embedding_active = [&]() -> bool {
- Line 2706: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto emb = embedding_pipeline_->generateEmbedding(text);
- Line 2730: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: if (vector_index_->getDimension() == static_cast<int>(cm.embedding.size())) {
- Line 2865: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: s.total_storage_bytes = static_cast<int64_t>(storage_->getApproximateSize());
- Line 2871: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: if (vector_index_) s.total_embeddings = static_cast<int>(vector_index_->getVectorCount());
- Line 63: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(retry_delay_ms));
- Line 253: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: bool allMatch = true;

                for (const auto& kv : wantedMeta) {

                    if (!m.user_metadata.contains(kv.first)) { allMatch = false; break; }

                    const auto& v = m.user_metadata[kv.first];

                    if (v.type() != kv.second.type()) {

                        // allow string/numeric loose comparison fallback

                        try {
- Line 342: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: match = (vptr->dump() == cond.dump());
- Line 952: severity=HIGH; category=allocation_loop
  Description: Dynamic allocation in loop — high overhead
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: // the previous raw-new / manual-delete pattern (CWE-401 / RAII).
- Line 952: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: // Use a unique_ptr to own the heap-allocated JSON object for the

                    // "tags" field.  For the other two fields we point at an existing

                    // ContentMeta member — no heap allocation needed.  This eliminates

                    // the previous raw-new / manual-delete pattern (CWE-401 / RAII).

                    std::unique_ptr<nlohmann::json> tags_json_owner;

                    nlohmann::json* target = nullptr;

                    std::optional<nlohmann::json> tags_json_holder;
- Line 952: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: // the previous raw-new / manual-delete pattern (CWE-401 / RAII).
- Line 952: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 991: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: // Temporär speichern in map structure

                        // Hänge verschlüsselte Strings in eine Zusatzliste (wird später gemerged)

                        if (!meta.extracted_metadata.contains("__enc_meta")) {

                            meta.extracted_metadata["__enc_meta"] = json::object();

                        }

                        meta.extracted_metadata["__enc_meta"][f + "_encrypted"] = enc_b64;

                        meta.extracted_metadata["__enc_meta"][f + "_enc"] = true;
- Line 993: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: if (!meta.extracted_metadata.contains("__enc_meta")) {

                            meta.extracted_metadata["__enc_meta"] = json::object();

                        }

                        meta.extracted_metadata["__enc_meta"][f + "_encrypted"] = enc_b64;

                        meta.extracted_metadata["__enc_meta"][f + "_enc"] = true;

                    } catch (const std::exception& ex) {

                        THEMIS_WARN("vector metadata encryption field {} failed: {}", f, ex.what());
- Line 994: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: meta.extracted_metadata["__enc_meta"] = json::object();

                        }

                        meta.extracted_metadata["__enc_meta"][f + "_encrypted"] = enc_b64;

                        meta.extracted_metadata["__enc_meta"][f + "_enc"] = true;

                    } catch (const std::exception& ex) {

                        THEMIS_WARN("vector metadata encryption field {} failed: {}", f, ex.what());

                    }
- Line 1153: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 1154: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 1155: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 1158: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 1576: severity=HIGH; category=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: if (beta != 0.0) {
- Line 1600: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: out.erase(std::remove_if(out.begin(), out.end(), [&](const auto& p){ return allowed.find(p.first) ==
- Line 2022: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: if (proc_config.strategy == content::ArchiveStrategy::EXTRACT_AND_INGEST && 

            proc_result.metadata.contains("extracted_files")) {

            

            auto extracted_files = proc_result.metadata["extracted_files"];

            std::string temp_dir = proc_result.metadata.value("temp_directory", "");

            

            // Ingest each extracted file
- Line 2111: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: result.success = true;

        result.metadata = proc_result.metadata;

        result.metadata["archive_id"] = archive_id;

        result.metadata["extracted_count"] = result.extracted_content_ids.size();

        

        return result;
- Line 2112: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: result.success = true;

        result.metadata = proc_result.metadata;

        result.metadata["archive_id"] = archive_id;

        result.metadata["extracted_count"] = result.extracted_content_ids.size();

        

        return result;

    }
- Line 2373: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: if (!ocr_text.empty()) {

            meta.text_extracted = true;

            meta.extracted_metadata["content_ocr_text"] = ocr_text;



            if (stage_cfg.chunking.enabled) {

                OcrProcessor::Config ocr_cfg;
- Line 2384: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: ExtractionResult ocr_extraction;

                ocr_extraction.ok   = true;

                ocr_extraction.text = ocr_text;

                ocr_extraction.metadata["content_ocr_text"] = ocr_text;



                int chunk_size = config.value("chunk_size", 512);

                int overlap    = config.value("chunk_overlap", 50);
- Line 2478: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: if (dedup_policy_enabled && dedup_checker_ && (dedup_is_image || dedup_is_text)) {

        if (dedup_is_image && !cached_phash.empty()) {

            dedup_checker_->registerImage(content_id, cached_phash);

            meta.extracted_metadata["phash_hex"] = cached_phash;

        } else if (dedup_is_text && !cached_minhash.empty()) {

            dedup_checker_->registerText(content_id, cached_minhash);

        }
- Line 2492: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: {"category", static_cast<int>(category)}

    };

    if (!chunks_json.empty()) {

        result.metadata["chunk_count"] = static_cast<int>(chunks_json.size());

    }

    

    return result;
- Line 61: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: for (int i = 0; i <= max_retries; ++i) {
- Line 150: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> allowedMimes;
- Line 151: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, json> wantedMeta;
- Line 152: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> wantedTags;
- Line 180: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: for (auto it = filters["metadata"].begin(); it != filters["metadata"].end(); ++it) {
- Line 202: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, std::string> fieldMap;
- Line 209: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: for (auto it = sc["field_map"].begin(); it != sc["field_map"].end(); ++it) {
- Line 631: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: threat_info += r.threat_name + " (" + r.scanner_name + "); ";
- Line 663: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (const auto& mv : cj["skip_compressed_mimes"]) if (mv.is_string()) skip_mimes.push_back(mv.get<s
- Line 813: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto ftcfg = cj["fulltext_config"];
- Line 936: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (const auto& f : mcfg["fields"]) if (f.is_string()) meta_fields.push_back(f.get<std::string>());
- Line 1077: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: }

            } catch (const json::exception&) {

                meta_encrypt_enabled = false;

            } catch (...) {

                meta_encrypt_enabled = false;

            }

            if (meta_encrypt_enabled) {
- Line 1077: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 1081: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto enc_section = j["_encrypted_meta"];
- Line 1134: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: bool needs_reencryption = false;
- Line 1144: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: THEMIS_INFO("Content blob {} uses outdated key version {} (latest: {}), triggering re-encryption", 

                                id, blob.key_version, latest_version);

                }

            } catch (...) {

                // If metadata check fails, skip re-encryption

            }
- Line 1144: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 1218: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: out.push_back(ChunkMeta::fromJson(j));
- Line 1391: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, float> vector_scores;
- Line 1392: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, size_t> vector_ranks;
- Line 1427: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, float> fulltext_scores;
- Line 1428: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, size_t> fulltext_ranks;
- Line 1491: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, float> rrf_scores;
- Line 1539: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: if (sc.contains("gamma")) gamma = sc["gamma"].get<double>();

        }

    } catch (const nlohmann::json::exception&) {

    } catch (...) {

    }



    // Erzeuge Map pk->score und Queue für Expansion
- Line 1539: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 1543: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, double> bestScore; bestScore.reserve(base.size()*2);
- Line 1565: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> seen;
- Line 1600: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> allowed(allow.begin(), allow.end());
- Line 2672: severity=MEDIUM; category=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: auto ftcfg = cj["fulltext_config"];
- Line 2684: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: }

        }

    } catch (const nlohmann::json::exception&) {

    } catch (...) {

    }



    if (auto_fulltext_index && secondary_index_) {
- Line 2684: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {

### content/video_processor.cpp
Total findings: 49

- Line 539: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: data.framerate = static_cast<double>(stream->avg_frame_rate.num) / stream->avg_frame_rate.den;
- Line 703: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: double aspect = static_cast<double>(frame->width) / frame->height;
- Line 716: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: = sws_getContext(frame->width, frame->height, static_cast<AVPixelFormat>(frame->format), thumb_width
- Line 746: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: if (rgb_frame->linesize[0] == static_cast<int>(row_size)) {
- Line 945: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: const uint8_t *ra = a->data[0] + static_cast<ptrdiff_t>(y) * a->linesize[0];
- Line 946: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: const uint8_t *rb = b->data[0] + static_cast<ptrdiff_t>(y) * b->linesize[0];
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #3120 [content] Implement video f... (2026-03-12) | #2996 feat(content): Vide
- Line 194: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: // Build metadata JSON

        json metadata;

        metadata["duration_ms"]      = media.duration_ms;

        metadata["width"]            = media.width;

        metadata["height"]           = media.height;

        metadata["video_codec"]      = media.video_codec;
- Line 195: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: // Build metadata JSON

        json metadata;

        metadata["duration_ms"]      = media.duration_ms;

        metadata["width"]            = media.width;

        metadata["height"]           = media.height;

        metadata["video_codec"]      = media.video_codec;

        metadata["audio_codec"]      = media.audio_codec;
- Line 196: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: json metadata;

        metadata["duration_ms"]      = media.duration_ms;

        metadata["width"]            = media.width;

        metadata["height"]           = media.height;

        metadata["video_codec"]      = media.video_codec;

        metadata["audio_codec"]      = media.audio_codec;

        metadata["bitrate_kbps"]     = media.bitrate_kbps;
- Line 197: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: metadata["duration_ms"]      = media.duration_ms;

        metadata["width"]            = media.width;

        metadata["height"]           = media.height;

        metadata["video_codec"]      = media.video_codec;

        metadata["audio_codec"]      = media.audio_codec;

        metadata["bitrate_kbps"]     = media.bitrate_kbps;

        metadata["framerate"]        = media.framerate;
- Line 198: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: metadata["width"]            = media.width;

        metadata["height"]           = media.height;

        metadata["video_codec"]      = media.video_codec;

        metadata["audio_codec"]      = media.audio_codec;

        metadata["bitrate_kbps"]     = media.bitrate_kbps;

        metadata["framerate"]        = media.framerate;

        metadata["container_format"] = media.container_format;
- Line 199: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: metadata["height"]           = media.height;

        metadata["video_codec"]      = media.video_codec;

        metadata["audio_codec"]      = media.audio_codec;

        metadata["bitrate_kbps"]     = media.bitrate_kbps;

        metadata["framerate"]        = media.framerate;

        metadata["container_format"] = media.container_format;
- Line 200: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: metadata["video_codec"]      = media.video_codec;

        metadata["audio_codec"]      = media.audio_codec;

        metadata["bitrate_kbps"]     = media.bitrate_kbps;

        metadata["framerate"]        = media.framerate;

        metadata["container_format"] = media.container_format;



        // Calculate aspect ratio
- Line 201: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: metadata["audio_codec"]      = media.audio_codec;

        metadata["bitrate_kbps"]     = media.bitrate_kbps;

        metadata["framerate"]        = media.framerate;

        metadata["container_format"] = media.container_format;



        // Calculate aspect ratio

        if (media.height > 0) {
- Line 205: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: // Calculate aspect ratio

        if (media.height > 0) {

            metadata["aspect_ratio"] = static_cast<double>(media.width) / media.height;

        }



        // Resolution classification
- Line 210: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: // Resolution classification

        if (media.height >= 2160) {

            metadata["resolution_class"] = "4K";

        } else if (media.height >= 1080) {

            metadata["resolution_class"] = "1080p";

        } else if (media.height >= 720) {
- Line 212: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: if (media.height >= 2160) {

            metadata["resolution_class"] = "4K";

        } else if (media.height >= 1080) {

            metadata["resolution_class"] = "1080p";

        } else if (media.height >= 720) {

            metadata["resolution_class"] = "720p";

        } else if (media.height >= 480) {
- Line 214: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: } else if (media.height >= 1080) {

            metadata["resolution_class"] = "1080p";

        } else if (media.height >= 720) {

            metadata["resolution_class"] = "720p";

        } else if (media.height >= 480) {

            metadata["resolution_class"] = "480p";

        } else {
- Line 216: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: } else if (media.height >= 720) {

            metadata["resolution_class"] = "720p";

        } else if (media.height >= 480) {

            metadata["resolution_class"] = "480p";

        } else {

            metadata["resolution_class"] = "SD";

        }
- Line 218: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: } else if (media.height >= 480) {

            metadata["resolution_class"] = "480p";

        } else {

            metadata["resolution_class"] = "SD";

        }



        result.metadata = metadata;
- Line 237: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: for (const auto &time : keyframes) {

                kf_times.push_back(time);

            }

            result.metadata["keyframe_timestamps_ms"] = kf_times;

        }



        // Extract subtitles if requested (via options or plugin config)
- Line 245: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: std::string subtitles = extractSubtitles(blob);

            if (!subtitles.empty()) {

                result.text                      = subtitles;

                result.metadata["has_subtitles"] = true;

                media.subtitles                  = subtitles;

            }

        }
- Line 258: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: for (auto time : scenes) {

                scene_times.push_back(time);

            }

            result.metadata["scene_changes_ms"] = scene_times;

        }



        result.media   = media;
- Line 507: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: if (fmt_ctx->iformat) {
- Line 508: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: data.container_format = fmt_ctx->iformat->name;
- Line 512: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: if (fmt_ctx->duration != AV_NOPTS_VALUE) {
- Line 513: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: data.duration_ms = fmt_ctx->duration / 1000;
- Line 517: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: if (fmt_ctx->bit_rate > 0) {
- Line 518: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: data.bitrate_kbps = fmt_ctx->bit_rate / 1000;
- Line 522: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: for (unsigned int i = 0; i < fmt_ctx->nb_streams; i++) {
- Line 523: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: AVStream *stream            = fmt_ctx->streams[i];
- Line 629: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: for (unsigned int i = 0; i < fmt_ctx->nb_streams; i++) {
- Line 630: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
- Line 642: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: AVStream *video_stream      = fmt_ctx->streams[video_stream_index];
- Line 676: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: int64_t seek_target = fmt_ctx->duration / 10;
- Line 828: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: AVRational time_base = fmt_ctx->streams[video_stream_index]->time_base;
- Line 902: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: AVStream *video_stream      = fmt_ctx->streams[video_stream_index];
- Line 143: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: bool VideoProcessor::canProcess(const std::string &mime_type) const {
- Line 306: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: current_text += " ";
- Line 374: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'Long-term: Video frame extraction' that was not found in 'src/content/ROADMAP.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/content/ROADMAP.md § "Long-term: Video frame extraction"
- Line 375: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'Video Processing".' that was not found in 'src/content/FUTURE_ENHANCEMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/content/FUTURE_ENHANCEMENTS.md § "Video Processing".
- Line 452: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: keyframes.push_back(i * interval_ms);
- Line 759: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: av_frame_free(&rgb_frame);
- Line 765: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: av_frame_free(&frame);
- Line 766: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: av_packet_free(&packet);
- Line 988: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: av_frame_free(&frame);
- Line 989: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: av_frame_free(&prev_frame);
- Line 990: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: av_packet_free(&packet);

### content/content_logger.cpp
Total findings: 31

- Line 41: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: const std::string& filename

) {

    json metadata;

    metadata["content_id"] = content_id;

    metadata["mime_type"] = mime_type;

    metadata["size_bytes"] = size_bytes;
- Line 42: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: ) {

    json metadata;

    metadata["content_id"] = content_id;

    metadata["mime_type"] = mime_type;

    metadata["size_bytes"] = size_bytes;

    

    if (!filename.empty()) {
- Line 43: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: json metadata;

    metadata["content_id"] = content_id;

    metadata["mime_type"] = mime_type;

    metadata["size_bytes"] = size_bytes;

    

    if (!filename.empty()) {

        metadata["filename"] = pii_sanitization_ ? sanitizeFilename(filename) : filename;
- Line 46: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: metadata["size_bytes"] = size_bytes;

    

    if (!filename.empty()) {

        metadata["filename"] = pii_sanitization_ ? sanitizeFilename(filename) : filename;

    }

    

    info("content.ingestion", "Content ingested", metadata);
- Line 61: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: double duration_ms

) {

    json metadata;

    metadata["content_id"] = content_id;

    metadata["mime_type"] = mime_type;

    metadata["size_bytes"] = size_bytes;

    metadata["success"] = success;
- Line 62: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: ) {

    json metadata;

    metadata["content_id"] = content_id;

    metadata["mime_type"] = mime_type;

    metadata["size_bytes"] = size_bytes;

    metadata["success"] = success;

    metadata["duration_ms"] = duration_ms;
- Line 63: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: json metadata;

    metadata["content_id"] = content_id;

    metadata["mime_type"] = mime_type;

    metadata["size_bytes"] = size_bytes;

    metadata["success"] = success;

    metadata["duration_ms"] = duration_ms;
- Line 64: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: metadata["content_id"] = content_id;

    metadata["mime_type"] = mime_type;

    metadata["size_bytes"] = size_bytes;

    metadata["success"] = success;

    metadata["duration_ms"] = duration_ms;

    

    if (!success && error_code != 0) {
- Line 65: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: metadata["mime_type"] = mime_type;

    metadata["size_bytes"] = size_bytes;

    metadata["success"] = success;

    metadata["duration_ms"] = duration_ms;

    

    if (!success && error_code != 0) {

        metadata["error_code"] = error_code;
- Line 86: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: int error_code

) {

    json metadata;

    metadata["content_id"] = content_id;

    metadata["operation"] = operation;

    metadata["duration_ms"] = duration_ms;

    metadata["success"] = success;
- Line 87: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: ) {

    json metadata;

    metadata["content_id"] = content_id;

    metadata["operation"] = operation;

    metadata["duration_ms"] = duration_ms;

    metadata["success"] = success;
- Line 88: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: json metadata;

    metadata["content_id"] = content_id;

    metadata["operation"] = operation;

    metadata["duration_ms"] = duration_ms;

    metadata["success"] = success;

    

    if (!success && error_code != 0) {
- Line 89: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: metadata["content_id"] = content_id;

    metadata["operation"] = operation;

    metadata["duration_ms"] = duration_ms;

    metadata["success"] = success;

    

    if (!success && error_code != 0) {

        metadata["error_code"] = error_code;
- Line 113: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: const std::string& error_category

) {

    json metadata;

    metadata["content_id"] = content_id;

    metadata["operation"] = operation;

    metadata["error_code"] = error_code;

    metadata["error_message"] = pii_sanitization_ ? sanitizeMessage(error_message) : error_message;
- Line 114: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: ) {

    json metadata;

    metadata["content_id"] = content_id;

    metadata["operation"] = operation;

    metadata["error_code"] = error_code;

    metadata["error_message"] = pii_sanitization_ ? sanitizeMessage(error_message) : error_message;
- Line 115: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: json metadata;

    metadata["content_id"] = content_id;

    metadata["operation"] = operation;

    metadata["error_code"] = error_code;

    metadata["error_message"] = pii_sanitization_ ? sanitizeMessage(error_message) : error_message;

    

    if (!error_category.empty()) {
- Line 116: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: metadata["content_id"] = content_id;

    metadata["operation"] = operation;

    metadata["error_code"] = error_code;

    metadata["error_message"] = pii_sanitization_ ? sanitizeMessage(error_message) : error_message;

    

    if (!error_category.empty()) {

        metadata["error_category"] = error_category;
- Line 119: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: metadata["error_message"] = pii_sanitization_ ? sanitizeMessage(error_message) : error_message;

    

    if (!error_category.empty()) {

        metadata["error_category"] = error_category;

    }

    

    error("content.error", "Content processing error", metadata);
- Line 132: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: double elapsed_seconds

) {

    json metadata;

    metadata["content_id"] = content_id;

    metadata["operation"] = operation;

    metadata["timeout_seconds"] = timeout_seconds;

    metadata["elapsed_seconds"] = elapsed_seconds;
- Line 133: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: ) {

    json metadata;

    metadata["content_id"] = content_id;

    metadata["operation"] = operation;

    metadata["timeout_seconds"] = timeout_seconds;

    metadata["elapsed_seconds"] = elapsed_seconds;
- Line 134: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: json metadata;

    metadata["content_id"] = content_id;

    metadata["operation"] = operation;

    metadata["timeout_seconds"] = timeout_seconds;

    metadata["elapsed_seconds"] = elapsed_seconds;

    

    warn("content.timeout", "Content operation timed out", metadata);
- Line 135: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: metadata["content_id"] = content_id;

    metadata["operation"] = operation;

    metadata["timeout_seconds"] = timeout_seconds;

    metadata["elapsed_seconds"] = elapsed_seconds;

    

    warn("content.timeout", "Content operation timed out", metadata);

}
- Line 145: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: bool hit

) {

    json metadata;

    metadata["content_id"] = content_id;

    metadata["hit"] = hit;

    

    debug("content.cache." + std::string(hit ? "hit" : "miss"),
- Line 146: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: ) {

    json metadata;

    metadata["content_id"] = content_id;

    metadata["hit"] = hit;

    

    debug("content.cache." + std::string(hit ? "hit" : "miss"),

          "Content cache " + std::string(hit ? "hit" : "miss"),
- Line 296: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: json metadata;

    

    if (!correlation_id_.empty()) {

        metadata["correlation_id"] = correlation_id_;

    }

    

    return metadata;
- Line 214: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: size_t pos = filename.find_last_of("/\\");
- Line 157: severity=LOW; category=unstructured_log
  Description: Unstructured logging (use structured format)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: void ContentLogger::log(
- Line 188: severity=LOW; category=unstructured_log
  Description: Unstructured logging (use structured format)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: log(utils::Logger::Level::INFO, event, message, metadata);
- Line 192: severity=LOW; category=unstructured_log
  Description: Unstructured logging (use structured format)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: log(utils::Logger::Level::WARN, event, message, metadata);
- Line 196: severity=LOW; category=unstructured_log
  Description: Unstructured logging (use structured format)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: log(utils::Logger::Level::ERROR, event, message, metadata);
- Line 200: severity=LOW; category=unstructured_log
  Description: Unstructured logging (use structured format)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: log(utils::Logger::Level::DEBUG, event, message, metadata);

### content/pdf_processor.cpp
Total findings: 26

- Line 162: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: int max_pages = config_.max_pages > 0 ? std::min(config_.max_pages, doc->pages()) : doc->pages();
- Line 329: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: int max_pages = config_.max_pages > 0 ? std::min(config_.max_pages, doc->pages()) : doc->pages();
- Line 497: severity=CRITICAL; category=new_without_raii
  Description: Raw new() without RAII wrapper — potential memory leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: chunks.push_back(chunk);

            seq_num++;



            // Start new chunk (with overlap)

            if (overlap > 0 && current_tokens > overlap) {

                // Keep last part for overlap

                current_chunk  = sentence;
- Line 139: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: if (doc->is_locked()) {

            result.error_message            = "PDF is encrypted and password is incorrect";

            result.metadata["is_encrypted"] = true;

            if (config_.metrics) {

                config_.metrics->recordExtractError();

            }
- Line 148: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: // Extract metadata

        PDFMetadata metadata                 = extractMetadata(blob);

        result.metadata["title"]             = metadata.title;

        result.metadata["author"]            = metadata.author;

        result.metadata["subject"]           = metadata.subject;

        result.metadata["keywords"]          = metadata.keywords;
- Line 149: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: // Extract metadata

        PDFMetadata metadata                 = extractMetadata(blob);

        result.metadata["title"]             = metadata.title;

        result.metadata["author"]            = metadata.author;

        result.metadata["subject"]           = metadata.subject;

        result.metadata["keywords"]          = metadata.keywords;

        result.metadata["creator"]           = metadata.creator;
- Line 201: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: }



        result.text                         = all_text.str();

        result.metadata["extracted_pages"]  = max_pages;

        result.metadata["pages"]            = pages_array;

        result.metadata["layout_preserved"] = config_.maintain_layout;

        result.metadata["token_count"]      = countTokens(result.text);
- Line 202: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: result.text                         = all_text.str();

        result.metadata["extracted_pages"]  = max_pages;

        result.metadata["pages"]            = pages_array;

        result.metadata["layout_preserved"] = config_.maintain_layout;

        result.metadata["token_count"]      = countTokens(result.text);

        result.ok                           = true;
- Line 203: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: result.text                         = all_text.str();

        result.metadata["extracted_pages"]  = max_pages;

        result.metadata["pages"]            = pages_array;

        result.metadata["layout_preserved"] = config_.maintain_layout;

        result.metadata["token_count"]      = countTokens(result.text);

        result.ok                           = true;
- Line 204: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: result.metadata["extracted_pages"]  = max_pages;

        result.metadata["pages"]            = pages_array;

        result.metadata["layout_preserved"] = config_.maintain_layout;

        result.metadata["token_count"]      = countTokens(result.text);

        result.ok                           = true;



    } catch (const std::exception &e) {
- Line 223: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: auto pages_begin              = std::sregex_iterator(blob.begin(), blob.end(), page_regex);

    auto pages_end                = std::sregex_iterator();

    int page_count                = static_cast<int>(std::distance(pages_begin, pages_end));

    result.metadata["page_count"] = page_count;



    // Try to extract text from BT/ET blocks

    std::ostringstream extracted;
- Line 236: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: }



    result.text                          = extracted.str();

    result.metadata["extraction_method"] = "basic_regex";

    result.metadata["note"]              = "Full PDF extraction requires building with -DTHEMIS_ENABLE_PDF=ON";

    result.metadata["layout_preserved"]  = false;

    result.metadata["token_count"]       = countTokens(result.text);
- Line 237: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: result.text                          = extracted.str();

    result.metadata["extraction_method"] = "basic_regex";

    result.metadata["note"]              = "Full PDF extraction requires building with -DTHEMIS_ENABLE_PDF=ON";

    result.metadata["layout_preserved"]  = false;

    result.metadata["token_count"]       = countTokens(result.text);

    result.ok                            = true;
- Line 238: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: result.text                          = extracted.str();

    result.metadata["extraction_method"] = "basic_regex";

    result.metadata["note"]              = "Full PDF extraction requires building with -DTHEMIS_ENABLE_PDF=ON";

    result.metadata["layout_preserved"]  = false;

    result.metadata["token_count"]       = countTokens(result.text);

    result.ok                            = true;

#endif
- Line 239: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: result.metadata["extraction_method"] = "basic_regex";

    result.metadata["note"]              = "Full PDF extraction requires building with -DTHEMIS_ENABLE_PDF=ON";

    result.metadata["layout_preserved"]  = false;

    result.metadata["token_count"]       = countTokens(result.text);

    result.ok                            = true;

#endif
- Line 412: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 194: severity=MEDIUM; category=cast_to_smaller_type
  Description: Explicit cast to int detected (verify no overflow on source)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::type_conversion
  Context: ['            page_obj["page"]     = i + 1;', '            page_obj["text"]     = page_text;', '            page_obj["width"]    = static_cast<int>(rect.width());', '            page_obj["height"]   = static_cast<int>(rect.height());', '            page_obj["rotation"] = page->orientation() * 90;']
- Line 195: severity=MEDIUM; category=cast_to_smaller_type
  Description: Explicit cast to int detected (verify no overflow on source)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::type_conversion
  Context: ['            page_obj["text"]     = page_text;', '            page_obj["width"]    = static_cast<int>(rect.width());', '            page_obj["height"]   = static_cast<int>(rect.height());', '            page_obj["rotation"] = page->orientation() * 90;', '            pages_array.push_back(std::move(page_obj));']
- Line 219: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: std::regex page_regex("/Type\\s*/Page[^s]");
- Line 300: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: std::regex pattern("/" + key + "\\s*\\(([^)]+)\\)");
- Line 351: severity=MEDIUM; category=cast_to_smaller_type
  Description: Explicit cast to int detected (verify no overflow on source)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::type_conversion
  Context: ['        // Get dimensions', '        poppler::rectf rect = page->page_rect();', '        info.width          = static_cast<int>(rect.width());', '        info.height         = static_cast<int>(rect.height());', '        info.rotation       = page->orientation() * 90;']
- Line 352: severity=MEDIUM; category=cast_to_smaller_type
  Description: Explicit cast to int detected (verify no overflow on source)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::type_conversion
  Context: ['        poppler::rectf rect = page->page_rect();', '        info.width          = static_cast<int>(rect.width());', '        info.height         = static_cast<int>(rect.height());', '        info.rotation       = page->orientation() * 90;', '']
- Line 371: severity=MEDIUM; category=uninitialized_member_field
  Description: Default constructor does not initialize POD members
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::uninitialized
  Context: Struct with uninitialized fields
- Line 507: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: current_chunk += " ";
- Line 532: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'PDF Embedding Integration).' that was not found in 'src/content/FUTURE_ENHANCEMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/content/FUTURE_ENHANCEMENTS.md §PDF Embedding Integration).
- Line 556: severity=MEDIUM; category=cast_to_smaller_type
  Description: Explicit cast to int detected (verify no overflow on source)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::type_conversion
  Context: ['            const size_t combined = token_hash ^ (i * 31u) ^ (static_cast<size_t>(seed) * 97u);', '            for (int d = 0; d < 10; ++d) {', '                const int dim = static_cast<int>((combined + static_cast<size_t>(d) * 73u) % static_cast<size_t>(kDim));', '                const float weight = 1.0f / (1.0f + static_cast<float>(i) * 0.1f);', '                const float phase']

### content/async_ingestion_worker.cpp
Total findings: 23

- Line 113: severity=CRITICAL; category=exception_in_destructor
  Description: Destructors must be noexcept; exceptions here cause std::terminate()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 180: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: worker.join();
- Line 187: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: cleanup_thread_.join();
- Line 265: severity=CRITICAL; category=blocking_no_timeout
  Description: Blocking operation without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: total_backpressure_events_.fetch_add(1, std::memory_order_relaxed);

        }

        // Block until queue depth is below the back-pressure threshold

        backpressure_cv_.wait(lock, [this] {

            return (job_queue_.size() + inflight_count_.load(std::memory_order_relaxed)) < config_.max_queue_depth

                   || !running_.load() || shutdown_requested_.load();

        });
- Line 265: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: backpressure_cv_.wait(lock, [this] {
- Line 326: severity=CRITICAL; category=blocking_no_timeout
  Description: Blocking operation without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: total_backpressure_events_.fetch_add(1, std::memory_order_relaxed);

        }

        // Block until queue depth is below the back-pressure threshold

        backpressure_cv_.wait(lock, [this] {

            return (job_queue_.size() + inflight_count_.load(std::memory_order_relaxed)) < config_.max_queue_depth

                   || !running_.load() || shutdown_requested_.load();

        });
- Line 326: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: backpressure_cv_.wait(lock, [this] {
- Line 592: severity=CRITICAL; category=blocking_no_timeout
  Description: Blocking operation without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: {

            std::unique_lock<std::mutex> lock(queue_mutex_);



            queue_cv_.wait(lock, [this] { return !job_queue_.empty() || shutdown_requested_.load(); });



            if (shutdown_requested_.load() && job_queue_.empty()) {

                break;
- Line 592: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: queue_cv_.wait(lock, [this] { return !job_queue_.empty() || shutdown_requested_.load(); });
- Line 914: severity=CRITICAL; category=missing_version_tracking
  Description: Concurrent update without version vector or causal ordering
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: job.config.merge_patch(additional_config);
- Line 140: severity=HIGH; category=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: cleanup_thread_ = std::thread(&AsyncIngestionWorker::cleanupLoop, this);
- Line 155: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: std::lock_guard<std::mutex> lock(queue_mutex_);
- Line 199: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: const std::string &mime_type, const std::string &user_context,

                                             const json &config) {

    if (!running_.load()) {

        throw std::runtime_error("Worker not running");

    }



    IngestionJob job;
- Line 296: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: const std::string &mime_type,

                                                            const std::string &user_context, const json &config) {

    if (!running_.load()) {

        throw std::runtime_error("Worker not running");

    }



    auto promise                    = std::make_shared<std::promise<std::string>>();
- Line 357: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: std::string AsyncIngestionWorker::submitArchive(const std::string &blob, const std::string &filename,

                                                const std::string &user_context, const json &config) {

    if (!running_.load()) {

        throw std::runtime_error("Worker not running");

    }



    IngestionJob job;
- Line 609: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: std::lock_guard<std::mutex> lock(history_mutex_);
- Line 880: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: std::string AsyncIngestionWorker::submitSourceJob(const IngestionSource &source, const json &additional_config,

                                                  const std::string &user_context) {

    if (!running_.load()) {

        throw std::runtime_error("Worker not running");

    }



    // For source-backed jobs, a registered plugin is preferred so we can
- Line 914: severity=HIGH; category=undefined_conflict_resolution
  Description: Merge without explicit conflict resolution strategy
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: job.config.merge_patch(additional_config);
- Line 1066: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: std::this_thread::sleep_for(std::chrono::minutes(5));
- Line 116: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: AsyncIngestionWorker::~AsyncIngestionWorker() noexcept {

    try {

        stop(false);  // Force stop without waiting

    } catch (...) {

        // Exceptions must not propagate from destructors (C++ standard §15.5.1).

        // stop() can throw if a mutex operation or promise::set_exception fails;

        // any such failure is silently absorbed here to prevent std::terminate().
- Line 116: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 277: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: && !queue_depth_high_watermark_.compare_exchange_weak(old_hwm, depth, std::memory_order_relaxed)) {
- Line 339: severity=MEDIUM; category=stale_read_undocumented
  Description: Eventual/stale read without documentation of correctness
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::distributed_consistency
  Context: && !queue_depth_high_watermark_.compare_exchange_weak(old_hwm, depth, std::memory_order_relaxed)) {

### content/stt_processor.cpp
Total findings: 23

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #3619 fix(content): build system ... (2026-03-12) | #3574 fix: clear all rema
- Line 164: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: // Add metadata

        json metadata;

        metadata["transcription"] = {{"language", transcription.detected_language},

                                     {"confidence", transcription.average_confidence},

                                     {"duration_ms", transcription.audio_duration_ms},

                                     {"segment_count", transcription.segments.size()}};
- Line 182: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: }

            segments_json.push_back(seg_json);

        }

        metadata["segments"] = segments_json;



        result.metadata = metadata;
- Line 213: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: // Chunk by transcription segments if available

    if (result.metadata.contains("segments")) {

        auto segments = result.metadata["segments"];



        std::string current_chunk;

        int sequence           = 0;
- Line 230: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: chunk.text                 = current_chunk;

                chunk.sequence             = sequence++;

                chunk.token_count          = current_tokens;

                chunk.metadata["start_ms"] = chunk_start_ms;

                chunk.metadata["end_ms"]   = chunk_end_ms;

                chunks.push_back(chunk);
- Line 231: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: chunk.sequence             = sequence++;

                chunk.token_count          = current_tokens;

                chunk.metadata["start_ms"] = chunk_start_ms;

                chunk.metadata["end_ms"]   = chunk_end_ms;

                chunks.push_back(chunk);



                current_chunk  = "";
- Line 255: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: chunk.text                 = current_chunk;

            chunk.sequence             = sequence++;

            chunk.token_count          = countTokens(current_chunk);

            chunk.metadata["start_ms"] = chunk_start_ms;

            chunk.metadata["end_ms"]   = chunk_end_ms;

            chunks.push_back(chunk);

        }
- Line 256: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: chunk.sequence             = sequence++;

            chunk.token_count          = countTokens(current_chunk);

            chunk.metadata["start_ms"] = chunk_start_ms;

            chunk.metadata["end_ms"]   = chunk_end_ms;

            chunks.push_back(chunk);

        }

    } else {
- Line 574: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: + " (only PCM [1] and IEEE float [3] are supported)");
- Line 666: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: if (bits_per_sample == 8) {

                    // 8-bit PCM is unsigned (0-255)

                    if (sample_offset >= wav_data.size()) {

                        throw std::runtime_error("Sample offset out of bounds");

                    }

                    uint8_t val = wav_data[sample_offset];

                    sample      = (val - 128) / 128.0f;
- Line 677: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: } else if (bits_per_sample == 24) {

                    // 24-bit PCM is signed

                    if (sample_offset + 3 > wav_data.size()) {

                        throw std::runtime_error("24-bit sample extends beyond buffer");

                    }

                    // Use explicit casting to avoid sign extension issues

                    int32_t val = static_cast<int32_t>(static_cast<uint32_t>(wav_data[sample_offset])
- Line 939: severity=HIGH; category=arithmetic_overflow
  Description: Arithmetic operation result assigned to variable (potential overflow)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::type_conversion
  Context: ['            size_t bs  = static_cast<size_t>(b) * band_size;', '            size_t be  = (b == kBands - 1) ? n : bs + band_size;', '            size_t len = be - bs;', '', '            float rms = 0.0f;']
- Line 1074: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 1087: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 1090: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 107: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: bool STTProcessor::canProcess(const std::string &mime_type) const {
- Line 242: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: current_chunk += " ";
- Line 280: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: current_chunk += " ";
- Line 334: severity=MEDIUM; category=missing_move_constructor_defaulted
  Description: Missing move ctor prevents efficient rvalue transfers
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 455: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: whisper_free(static_cast<struct whisper_context *>(whisper_ctx_));
- Line 837: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'STTProcessor WhisperActivation.' that was not found in 'src/content/FUTURE_ENHANCEMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/content/FUTURE_ENHANCEMENTS.md §STTProcessor WhisperActivation.
- Line 988: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: // Assumes both inputs are L2-normalised; result is in [-1, 1].
- Line 1151: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d.%03d", hours, minutes, seconds, millis);

### content/audio_processor.cpp
Total findings: 22

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #3619 fix(content): build system ... (2026-03-12) | #3109 feat(content): Audi
- Line 147: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: // Build metadata JSON

        json metadata;

        metadata["duration_ms"]      = media.duration_ms;

        metadata["audio_codec"]      = media.audio_codec;

        metadata["bitrate_kbps"]     = media.bitrate_kbps;

        metadata["sample_rate"]      = media.sample_rate;
- Line 148: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: // Build metadata JSON

        json metadata;

        metadata["duration_ms"]      = media.duration_ms;

        metadata["audio_codec"]      = media.audio_codec;

        metadata["bitrate_kbps"]     = media.bitrate_kbps;

        metadata["sample_rate"]      = media.sample_rate;

        metadata["channels"]         = media.channels;
- Line 149: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: json metadata;

        metadata["duration_ms"]      = media.duration_ms;

        metadata["audio_codec"]      = media.audio_codec;

        metadata["bitrate_kbps"]     = media.bitrate_kbps;

        metadata["sample_rate"]      = media.sample_rate;

        metadata["channels"]         = media.channels;

        metadata["container_format"] = media.container_format;
- Line 150: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: metadata["duration_ms"]      = media.duration_ms;

        metadata["audio_codec"]      = media.audio_codec;

        metadata["bitrate_kbps"]     = media.bitrate_kbps;

        metadata["sample_rate"]      = media.sample_rate;

        metadata["channels"]         = media.channels;

        metadata["container_format"] = media.container_format;
- Line 151: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: metadata["audio_codec"]      = media.audio_codec;

        metadata["bitrate_kbps"]     = media.bitrate_kbps;

        metadata["sample_rate"]      = media.sample_rate;

        metadata["channels"]         = media.channels;

        metadata["container_format"] = media.container_format;



        // Merge tags
- Line 152: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: metadata["bitrate_kbps"]     = media.bitrate_kbps;

        metadata["sample_rate"]      = media.sample_rate;

        metadata["channels"]         = media.channels;

        metadata["container_format"] = media.container_format;



        // Merge tags

        if (!tags.empty()) {
- Line 156: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: // Merge tags

        if (!tags.empty()) {

            metadata["tags"] = tags;

        }



        // Audio classification
- Line 161: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: // Audio classification

        if (media.channels == 1) {

            metadata["channel_layout"] = "mono";

        } else if (media.channels == 2) {

            metadata["channel_layout"] = "stereo";

        } else if (media.channels == 6) {
- Line 163: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: if (media.channels == 1) {

            metadata["channel_layout"] = "mono";

        } else if (media.channels == 2) {

            metadata["channel_layout"] = "stereo";

        } else if (media.channels == 6) {

            metadata["channel_layout"] = "5.1";

        } else if (media.channels == 8) {
- Line 165: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: } else if (media.channels == 2) {

            metadata["channel_layout"] = "stereo";

        } else if (media.channels == 6) {

            metadata["channel_layout"] = "5.1";

        } else if (media.channels == 8) {

            metadata["channel_layout"] = "7.1";

        }
- Line 167: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: } else if (media.channels == 6) {

            metadata["channel_layout"] = "5.1";

        } else if (media.channels == 8) {

            metadata["channel_layout"] = "7.1";

        }



        // Quality classification
- Line 172: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: // Quality classification

        if (media.sample_rate >= 96000) {

            metadata["quality_class"] = "Hi-Res";

        } else if (media.sample_rate >= 44100) {

            metadata["quality_class"] = "CD Quality";

        } else {
- Line 174: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: if (media.sample_rate >= 96000) {

            metadata["quality_class"] = "Hi-Res";

        } else if (media.sample_rate >= 44100) {

            metadata["quality_class"] = "CD Quality";

        } else {

            metadata["quality_class"] = "Standard";

        }
- Line 176: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: } else if (media.sample_rate >= 44100) {

            metadata["quality_class"] = "CD Quality";

        } else {

            metadata["quality_class"] = "Standard";

        }



        result.metadata = metadata;
- Line 188: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: for (float sample : waveform) {

                waveform_json.push_back(sample);

            }

            result.metadata["waveform"] = waveform_json;

        }



        // Transcription (if enabled and requested)
- Line 219: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: segments_json.push_back(seg_json);

                    }

                    trans_meta["segments"]           = segments_json;

                    result.metadata["transcription"] = trans_meta;

                    transcriptions_performed_++;

                    populated = true;

                }
- Line 239: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: trans_meta["segment_count"]     = 1;

                trans_meta["segments"]          = json::array(

                    {{{"text", placeholder}, {"start_ms", 0}, {"end_ms", media.duration_ms}, {"confidence", 0.0}}});

                result.metadata["transcription"] = trans_meta;

                transcriptions_performed_++;

            }

        }
- Line 563: severity=HIGH; category=arithmetic_overflow
  Description: Arithmetic operation result assigned to variable (potential overflow)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::type_conversion
  Context: ['        if (bitrate > 0) {', '            // Approximate: subtract ID3 header size from content length', '            size_t audio_bytes = (blob.size() > search_start) ? blob.size() - search_start : blob.size();', '            data.duration_ms   = static_cast<int64_t>(audio_bytes) * 8 * 1000 / (bitrate * 1000);', '        }']
- Line 223: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: transcriptions_performed_++;

                    populated = true;

                }

            } catch (...) {

                // Fall through to placeholder transcription below.

            }
- Line 223: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 289: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: current_chunk += " ";

### content/image_processor.cpp
Total findings: 22

- Line 444: severity=CRITICAL; category=array_bounds_violation
  Description: Array bounds violation: loop 32 > array size 13
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: {

        uint32_t pixel_offset =

            static_cast<uint32_t>(blob[10])

            | (static_cast<uint32_t>(blob[11]) << 8)

            | (static_cast<uint32_t>(blob[12]) << 16)

            | (static_cast<uint32_t>(blob[13]) << 24);



        int bmp_width  = static_cast<int32_t>(

            blob[18] | (blob[19] << 8) | (blob[20] << 16) | (blob[21] << 24));

        int bmp_height = static_cast<int32_t>(

            blob[22] | (blob[23] << 8) | (blob[24] << 16) | (blob[25] << 24));
- Line 447: severity=CRITICAL; category=array_bounds_violation
  Description: Array bounds violation: loop 32 > array size 18
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: | (static_cast<uint32_t>(blob[11]) << 8)

            | (static_cast<uint32_t>(blob[12]) << 16)

            | (static_cast<uint32_t>(blob[13]) << 24);



        int bmp_width  = static_cast<int32_t>(

            blob[18] | (blob[19] << 8) | (blob[20] << 16) | (blob[21] << 24));

        int bmp_height = static_cast<int32_t>(

            blob[22] | (blob[23] << 8) | (blob[24] << 16) | (blob[25] << 24));

        uint16_t bits_per_pixel = static_cast<uint16_t>(blob[28] | (blob[29] << 8));

        uint32_t compression    =

            blob[30] | (blob[31] << 8) | (blob[32] << 16) | (blob[33] << 24);
- Line 449: severity=CRITICAL; category=array_bounds_violation
  Description: Array bounds violation: loop 32 > array size 22
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: | (static_cast<uint32_t>(blob[13]) << 24);



        int bmp_width  = static_cast<int32_t>(

            blob[18] | (blob[19] << 8) | (blob[20] << 16) | (blob[21] << 24));

        int bmp_height = static_cast<int32_t>(

            blob[22] | (blob[23] << 8) | (blob[24] << 16) | (blob[25] << 24));

        uint16_t bits_per_pixel = static_cast<uint16_t>(blob[28] | (blob[29] << 8));

        uint32_t compression    =

            blob[30] | (blob[31] << 8) | (blob[32] << 16) | (blob[33] << 24);



        if (bits_per_pixel == 24 && compression == 0
- Line 450: severity=CRITICAL; category=array_bounds_violation
  Description: Array bounds violation: loop 32 > array size 28
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: int bmp_width  = static_cast<int32_t>(

            blob[18] | (blob[19] << 8) | (blob[20] << 16) | (blob[21] << 24));

        int bmp_height = static_cast<int32_t>(

            blob[22] | (blob[23] << 8) | (blob[24] << 16) | (blob[25] << 24));

        uint16_t bits_per_pixel = static_cast<uint16_t>(blob[28] | (blob[29] << 8));

        uint32_t compression    =

            blob[30] | (blob[31] << 8) | (blob[32] << 16) | (blob[33] << 24);



        if (bits_per_pixel == 24 && compression == 0

            && bmp_width > 0 && bmp_height != 0)
- Line 452: severity=CRITICAL; category=array_bounds_violation
  Description: Array bounds violation: loop 32 > array size 30
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: blob[18] | (blob[19] << 8) | (blob[20] << 16) | (blob[21] << 24));

        int bmp_height = static_cast<int32_t>(

            blob[22] | (blob[23] << 8) | (blob[24] << 16) | (blob[25] << 24));

        uint16_t bits_per_pixel = static_cast<uint16_t>(blob[28] | (blob[29] << 8));

        uint32_t compression    =

            blob[30] | (blob[31] << 8) | (blob[32] << 16) | (blob[33] << 24);



        if (bits_per_pixel == 24 && compression == 0

            && bmp_width > 0 && bmp_height != 0)

        {

            int abs_height = std::abs(bmp_height);
- Line 546: severity=CRITICAL; category=array_bounds
  Description: Array bounds violation: loop 64 > array 31
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: double median = (sorted_freq[31] + sorted_freq[32]) / 2.0;
- Line 546: severity=CRITICAL; category=array_bounds_violation
  Description: Array bounds violation: loop 64 > array size 31
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: }



    // 4. Compute median of the 64 values

    std::array<double, 64> sorted_freq = low_freq;

    std::sort(sorted_freq.begin(), sorted_freq.end());

    double median = (sorted_freq[31] + sorted_freq[32]) / 2.0;



    // 5. Build 64-bit hash: bit i = 1 if low_freq[i] > median

    uint64_t hash = 0;

    for (int i = 0; i < 64; ++i) {

        if (low_freq[i] > median) {
- Line 164: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: // Extract EXIF metadata

        json exif = extractExifMetadata(blob);

        if (!exif.empty()) {

            metadata["exif"] = exif;

        }

        

        // Extract XMP metadata
- Line 170: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: // Extract XMP metadata

        json xmp = extractXmpMetadata(blob);

        if (!xmp.empty()) {

            metadata["xmp"] = xmp;

        }

        

        // Detect image dimensions from header
- Line 177: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: int width = 0, height = 0;

        detectImageDimensions(blob, mime_type, width, height);

        

        metadata["width"] = width;

        metadata["height"] = height;

        metadata["aspect_ratio"] = height > 0 ? static_cast<double>(width) / height : 0;

        metadata["megapixels"] = (width * height) / 1000000.0;
- Line 178: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: detectImageDimensions(blob, mime_type, width, height);

        

        metadata["width"] = width;

        metadata["height"] = height;

        metadata["aspect_ratio"] = height > 0 ? static_cast<double>(width) / height : 0;

        metadata["megapixels"] = (width * height) / 1000000.0;
- Line 179: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: metadata["width"] = width;

        metadata["height"] = height;

        metadata["aspect_ratio"] = height > 0 ? static_cast<double>(width) / height : 0;

        metadata["megapixels"] = (width * height) / 1000000.0;

        

        // Color analysis
- Line 180: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: metadata["width"] = width;

        metadata["height"] = height;

        metadata["aspect_ratio"] = height > 0 ? static_cast<double>(width) / height : 0;

        metadata["megapixels"] = (width * height) / 1000000.0;

        

        // Color analysis

        if (enable_color_analysis_) {
- Line 194: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: {"hex", rgbToHex(color[0], color[1], color[2])}

                });

            }

            metadata["dominant_colors"] = color_array;

        }

        

        // OCR
- Line 207: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: if (enable_face_detection_) {

            json faces = detectFaces(blob);

            if (!faces.empty()) {

                metadata["faces"] = faces;

                faces_detected_ += faces.size();

            }

        }
- Line 216: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: if (enable_object_detection_) {

            json objects = detectObjects(blob);

            if (!objects.empty()) {

                metadata["objects"] = objects;

            }

        }
- Line 316: severity=HIGH; category=arithmetic_overflow
  Description: Arithmetic operation result assigned to variable (potential overflow)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::type_conversion
  Context: ['        for (size_t i = 2; i < blob.size() - 9; ++i) {', '            if (blob[i] == 0xFF && (blob[i + 1] == 0xC0 || blob[i + 1] == 0xC2)) {', '                height = (blob[i + 5] << 8) | blob[i + 6];', '                width = (blob[i + 7] << 8) | blob[i + 8];', '                return;']
- Line 317: severity=HIGH; category=arithmetic_overflow
  Description: Arithmetic operation result assigned to variable (potential overflow)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::type_conversion
  Context: ['            if (blob[i] == 0xFF && (blob[i + 1] == 0xC0 || blob[i + 1] == 0xC2)) {', '                height = (blob[i + 5] << 8) | blob[i + 6];', '                width = (blob[i + 7] << 8) | blob[i + 8];', '                return;', '            }']
- Line 118: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: bool ImageProcessor::canProcess(const std::string& mime_type) const {
- Line 273: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: current_chunk += " ";
- Line 274: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: current_chunk += " ";
- Line 347: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: snprintf(hex, sizeof(hex), "#%02X%02X%02X", r, g, b);

### content/html_processor.cpp
Total findings: 20

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #3619 fix(content): build system ... (2026-03-12) | #3012 [content] HTML cont
- Line 213: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 223: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 400: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 410: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 413: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 420: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 509: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 193: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: static const std::regex heading_close(R"(<\s*/\s*h[1-6][^>]*>)",
- Line 193: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: static const std::regex heading_close(R"(<\s*/\s*h[1-6][^>]*>)",
- Line 205: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: replaced += '\n';
- Line 206: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: replaced += '\n';
- Line 208: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: replaced += ' ';
- Line 219: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: R"(<\s*/?\s*(p|div|article|section|main|h[1-6]|li|ul|ol|blockquote|pre|br|tr|td|th|dt|dd)[^>]*>)",
- Line 324: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: R"(<title[^>]*>([\s\S]*?)<\/title>)",
- Line 407: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: result += '\n';
- Line 414: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: result += ' ';
- Line 515: severity=MEDIUM; category=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: paragraphs.push_back(para);
- Line 572: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (!overlap_text.empty()) overlap_text += ' ';
- Line 607: severity=MEDIUM; category=cast_to_smaller_type
  Description: Explicit cast to int detected (verify no overflow on source)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::type_conversion
  Context: ['            size_t combined = token_hash ^ (i * 31) ^ (static_cast<size_t>(seed) * 97);', '            for (int d = 0; d < 10; ++d) {', '                int dim = static_cast<int>((combined + static_cast<size_t>(d) * 73) % DIM);', '                float weight = 1.0f / (1.0f + static_cast<float>(i) * 0.1f);', '                float phase  = static_cast<float>((combined + static_cast<size_t>(dim)) % 360)']

### content/cad_processor.cpp
Total findings: 15

- Line 145: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: // Build metadata JSON

        json metadata;

        metadata["part_count"] = cad.part_count;

        metadata["units"]      = default_units_;



        // Bounding box
- Line 146: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: // Build metadata JSON

        json metadata;

        metadata["part_count"] = cad.part_count;

        metadata["units"]      = default_units_;



        // Bounding box

        metadata["bounding_box"]
- Line 149: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: metadata["units"]      = default_units_;



        // Bounding box

        metadata["bounding_box"]

            = {{"min", {cad.bounding_box_min[0], cad.bounding_box_min[1], cad.bounding_box_min[2]}},

               {"max", {cad.bounding_box_max[0], cad.bounding_box_max[1], cad.bounding_box_max[2]}}};
- Line 157: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: double dx              = cad.bounding_box_max[0] - cad.bounding_box_min[0];

        double dy              = cad.bounding_box_max[1] - cad.bounding_box_min[1];

        double dz              = cad.bounding_box_max[2] - cad.bounding_box_min[2];

        metadata["dimensions"] = {dx, dy, dz};



        // Volume and surface area

        if (calculate_volume_) {
- Line 161: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: // Volume and surface area

        if (calculate_volume_) {

            metadata["volume"]      = cad.volume;

            metadata["volume_unit"] = default_units_ + "³";

        }

        if (calculate_surface_area_) {
- Line 162: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: // Volume and surface area

        if (calculate_volume_) {

            metadata["volume"]      = cad.volume;

            metadata["volume_unit"] = default_units_ + "³";

        }

        if (calculate_surface_area_) {

            metadata["surface_area"]      = cad.surface_area;
- Line 165: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: metadata["volume_unit"] = default_units_ + "³";

        }

        if (calculate_surface_area_) {

            metadata["surface_area"]      = cad.surface_area;

            metadata["surface_area_unit"] = default_units_ + "²";

        }
- Line 166: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: }

        if (calculate_surface_area_) {

            metadata["surface_area"]      = cad.surface_area;

            metadata["surface_area_unit"] = default_units_ + "²";

        }



        // Assembly tree
- Line 171: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: // Assembly tree

        if (!cad.assembly_tree.empty()) {

            metadata["assembly_tree"] = cad.assembly_tree;

        }



        // Bill of Materials
- Line 176: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: // Bill of Materials

        if (extract_bom_ && !cad.bill_of_materials.empty()) {

            metadata["bill_of_materials"] = cad.bill_of_materials;

        }



        // Part IDs
- Line 181: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: // Part IDs

        if (!cad.part_ids.empty()) {

            metadata["part_ids"] = cad.part_ids;

        }



        result.metadata = metadata;
- Line 250: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: chunk.text                = text.str();

        chunk.sequence            = static_cast<int>(i);

        chunk.token_count         = countTokens(chunk.text);

        chunk.metadata["part_id"] = cad.part_ids[i];



        chunks.push_back(chunk);

    }
- Line 381: severity=HIGH; category=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: if (minX != std::numeric_limits<double>::max()) {
- Line 92: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: bool CADProcessor::canProcess(const std::string &mime_type) const {
- Line 134: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: } else if (mime_type == "image/vnd.dxf" || header.find("0\nSECTION") != std::string::npos) {

### content/mime_detector.cpp
Total findings: 13

- Line 75: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: bool verified = sig_mgr_->verifyFile(config_path, resource_id);
- Line 39: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Try multiple locations (new hierarchical structure first, then legacy)
- Line 311: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: buffer += "[extensions]\n";
- Line 323: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: snprintf(tmp, sizeof(tmp), "%02x", static_cast<unsigned int>(b));
- Line 327: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: wildcards += ":";
- Line 328: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: wildcards += ":";
- Line 330: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (!first) wildcards += ",";
- Line 331: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (!first) wildcards += ",";
- Line 349: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if (i) joined += ",";
- Line 350: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (i) joined += ",";
- Line 364: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: snprintf(&hex_out[i * 2], 3, "%02x", static_cast<unsigned int>(digest[i]));
- Line 323: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: snprintf(tmp, sizeof(tmp), "%02x", static_cast<unsigned int>(b));
- Line 364: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: snprintf(&hex_out[i * 2], 3, "%02x", static_cast<unsigned int>(digest[i]));

### content/geo_processor.cpp
Total findings: 12

- Line 531: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: OGRPoint* point = static_cast<OGRPoint*>(multipoint->getGeometryRef(g));
- Line 202: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: // Build metadata JSON

        json metadata;

        metadata["geometry_type"] = geo.geometry_type;

        metadata["crs"] = geo.crs;

        metadata["feature_count"] = geo.coordinates.size();
- Line 203: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: // Build metadata JSON

        json metadata;

        metadata["geometry_type"] = geo.geometry_type;

        metadata["crs"] = geo.crs;

        metadata["feature_count"] = geo.coordinates.size();

        

        if (geo.bounds[0] != 0 || geo.bounds[1] != 0 ||
- Line 204: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: json metadata;

        metadata["geometry_type"] = geo.geometry_type;

        metadata["crs"] = geo.crs;

        metadata["feature_count"] = geo.coordinates.size();

        

        if (geo.bounds[0] != 0 || geo.bounds[1] != 0 || 

            geo.bounds[2] != 0 || geo.bounds[3] != 0) {
- Line 208: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: if (geo.bounds[0] != 0 || geo.bounds[1] != 0 || 

            geo.bounds[2] != 0 || geo.bounds[3] != 0) {

            metadata["bounds"] = {

                {"minX", geo.bounds[0]},

                {"minY", geo.bounds[1]},

                {"maxX", geo.bounds[2]},
- Line 219: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: // Calculate centroid

        if (generate_centroid_ && !geo.coordinates.empty()) {

            auto [cx, cy] = calculateCentroid(geo);

            metadata["centroid"] = {{"lat", cx}, {"lon", cy}};

        }

        

        // Calculate area/length if applicable
- Line 224: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: // Calculate area/length if applicable

        if (geo.geometry_type == "Polygon" || geo.geometry_type == "MultiPolygon") {

            metadata["area_sq_degrees"] = calculateArea(geo);

        } else if (geo.geometry_type == "LineString" || geo.geometry_type == "MultiLineString") {

            metadata["length_degrees"] = calculateLength(geo);

        }
- Line 226: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: if (geo.geometry_type == "Polygon" || geo.geometry_type == "MultiPolygon") {

            metadata["area_sq_degrees"] = calculateArea(geo);

        } else if (geo.geometry_type == "LineString" || geo.geometry_type == "MultiLineString") {

            metadata["length_degrees"] = calculateLength(geo);

        }

        

        // Add properties
- Line 231: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: // Add properties

        if (!geo.properties.empty()) {

            metadata["properties"] = geo.properties;

        }

        

        result.metadata = metadata;
- Line 739: severity=HIGH; category=uninitialized_array
  Description: Array contains garbage values; use with zeros or explicit init
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::uninitialized
  Context: Array declared without initialization
- Line 135: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: bool GeoProcessor::canProcess(const std::string& mime_type) const {
- Line 181: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: (mime_type == "application/json" && content.find("\"type\"") != std::string::npos)) {

### content/archive_processor.cpp
Total findings: 10

- Line 331: severity=HIGH; category=unchecked_array_index
  Description: Array index not validated before access
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::input_validation
  Context: ['            // Determine entry type from typeflag (byte 156):', "            //   '0'/NUL = regular file, '5' = directory, '2' = symlink, etc.", '            const char typeflag = block[kTypeOffset];', "            const bool is_dir   = (typeflag == '5');", '']
- Line 893: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: members.push_back(

                {{"path", member.path}, {"size", member.uncompressed_size}, {"is_directory", member.is_directory}});

        }

        result.metadata["members"] = members;



        return result;

    }
- Line 918: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: for (const auto &file_path : extraction.extracted_files) {

        extracted_files.push_back(file_path);

    }

    result.metadata["extracted_files"] = extracted_files;



    // Note: The ContentManager will handle ingesting each extracted file

    // and creating graph relationships. We just provide the file list.
- Line 115: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: bool ArchiveProcessor::canHandle(const std::string &mime_type) const {
- Line 252: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: zip_close(za);
- Line 392: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: result += "/";
- Line 393: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: result += "/";
- Line 610: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: zip_close(za);
- Line 707: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::snprintf(name, sizeof(name), "%.*s/%.*s", 155, prefix, 100, reinterpret_cast<const char *>(hdr));
- Line 709: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::snprintf(name, sizeof(name), "%.*s", 100, reinterpret_cast<const char *>(hdr));

### content/content_fs.cpp
Total findings: 10

- Line 233: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: uint64_t part_end   = (i == end_idx) ? (end - chunk_off) : static_cast<uint64_t>(part->size());
- Line 235: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: part_end = static_cast<uint64_t>(part->size());
- Line 49: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 99: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Ensure legacy blob key is removed to avoid confusion
- Line 301: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: }

    }



    // Note: delete operations are best-effort, we succeed even if nothing was found

    // This makes remove() idempotent

    return OkVoid();

}
- Line 301: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: // Note: delete operations are best-effort, we succeed even if nothing was found
- Line 115: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: uint64_t old_chunks = jm.value("chunks", static_cast<uint64_t>(0));

                for (uint64_t i = 0; i < old_chunks; ++i) db_.del(chunkKey(pk, i));

            } catch (const nlohmann::json::exception&) {

            } catch (...) {

            }

        }

    }
- Line 115: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 286: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: auto j = nlohmann::json::from_cbor(*meta);

            chunks = j.value("chunks", static_cast<uint64_t>(0));

        } catch (const nlohmann::json::exception&) {

        } catch (...) {

        }

    }
- Line 286: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {

### content/ocr_processor.cpp
Total findings: 10

- Line 309: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: result.text = runTesseract(blob, &preprocess_info);



        if (config_.extract_metadata) {

            result.metadata["ocr_language"]     = config_.language;

            result.metadata["ocr_text_length"]  = static_cast<int>(result.text.size());

            result.metadata["content_ocr_text"] = result.text;

            result.metadata["mime_type"]        = content_type.mime_type;
- Line 310: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: if (config_.extract_metadata) {

            result.metadata["ocr_language"]     = config_.language;

            result.metadata["ocr_text_length"]  = static_cast<int>(result.text.size());

            result.metadata["content_ocr_text"] = result.text;

            result.metadata["mime_type"]        = content_type.mime_type;

            result.metadata["ocr_input_dpi"]    = preprocess_info.original_dpi;
- Line 311: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: if (config_.extract_metadata) {

            result.metadata["ocr_language"]     = config_.language;

            result.metadata["ocr_text_length"]  = static_cast<int>(result.text.size());

            result.metadata["content_ocr_text"] = result.text;

            result.metadata["mime_type"]        = content_type.mime_type;

            result.metadata["ocr_input_dpi"]    = preprocess_info.original_dpi;

            result.metadata["ocr_rescaled"]     = preprocess_info.rescaled;
- Line 312: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: result.metadata["ocr_language"]     = config_.language;

            result.metadata["ocr_text_length"]  = static_cast<int>(result.text.size());

            result.metadata["content_ocr_text"] = result.text;

            result.metadata["mime_type"]        = content_type.mime_type;

            result.metadata["ocr_input_dpi"]    = preprocess_info.original_dpi;

            result.metadata["ocr_rescaled"]     = preprocess_info.rescaled;

            result.metadata["ocr_binarized"]    = preprocess_info.binarized;
- Line 313: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: result.metadata["ocr_text_length"]  = static_cast<int>(result.text.size());

            result.metadata["content_ocr_text"] = result.text;

            result.metadata["mime_type"]        = content_type.mime_type;

            result.metadata["ocr_input_dpi"]    = preprocess_info.original_dpi;

            result.metadata["ocr_rescaled"]     = preprocess_info.rescaled;

            result.metadata["ocr_binarized"]    = preprocess_info.binarized;

        }
- Line 314: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: result.metadata["content_ocr_text"] = result.text;

            result.metadata["mime_type"]        = content_type.mime_type;

            result.metadata["ocr_input_dpi"]    = preprocess_info.original_dpi;

            result.metadata["ocr_rescaled"]     = preprocess_info.rescaled;

            result.metadata["ocr_binarized"]    = preprocess_info.binarized;

        }
- Line 315: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: result.metadata["mime_type"]        = content_type.mime_type;

            result.metadata["ocr_input_dpi"]    = preprocess_info.original_dpi;

            result.metadata["ocr_rescaled"]     = preprocess_info.rescaled;

            result.metadata["ocr_binarized"]    = preprocess_info.binarized;

        }



        result.ok      = true;
- Line 372: severity=MEDIUM; category=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: sentences.push_back(current);
- Line 401: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: current_chunk += " ";
- Line 471: severity=MEDIUM; category=cast_to_smaller_type
  Description: Explicit cast to int detected (verify no overflow on source)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::type_conversion
  Context: ['            size_t combined = token_hash ^ (i * 31) ^ (static_cast<size_t>(seed) * 97);', '            for (int d = 0; d < 10; ++d) {', '                int dim      = static_cast<int>((combined + static_cast<size_t>(d) * 73) % DIM);', '                float weight = 1.0f / (1.0f + static_cast<float>(i) * 0.1f);', '                float phase = static_cast<float>((combined + static_cast<size_t>(dim)) % 360) * 3.14159265359f / 180.0f;']

### content/text_processor.cpp
Total findings: 10

- Line 58: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: auto line_count = static_cast<int64_t>(std::count(result.text.begin(), result.text.end(), '\n')) + 1;

        result.metadata["line_count"] = line_count;

    } else {

        result.metadata["is_code"] = false;

    }



    // Word and token count
- Line 63: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: // Word and token count

    int token_count                = countTokens(result.text);

    result.metadata["token_count"] = token_count;



    // Sentence count (approximate)

    auto sentences                    = splitIntoSentences(result.text);
- Line 67: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: // Sentence count (approximate)

    auto sentences                    = splitIntoSentences(result.text);

    result.metadata["sentence_count"] = sentences.size();



    // Multi-language detection and routing

    {
- Line 73: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: {

        LanguageDetector lang_detector;

        DetectedLanguage lang                    = lang_detector.detect(result.text);

        result.metadata["detected_language"]     = lang.code;

        result.metadata["language_name"]         = lang.name;

        result.metadata["language_confidence"]   = lang.confidence;

        result.metadata["language_routing_hint"] = LanguageDetector::routingHint(lang.code);
- Line 74: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: LanguageDetector lang_detector;

        DetectedLanguage lang                    = lang_detector.detect(result.text);

        result.metadata["detected_language"]     = lang.code;

        result.metadata["language_name"]         = lang.name;

        result.metadata["language_confidence"]   = lang.confidence;

        result.metadata["language_routing_hint"] = LanguageDetector::routingHint(lang.code);

    }
- Line 75: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: DetectedLanguage lang                    = lang_detector.detect(result.text);

        result.metadata["detected_language"]     = lang.code;

        result.metadata["language_name"]         = lang.name;

        result.metadata["language_confidence"]   = lang.confidence;

        result.metadata["language_routing_hint"] = LanguageDetector::routingHint(lang.code);

    }
- Line 76: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: result.metadata["detected_language"]     = lang.code;

        result.metadata["language_name"]         = lang.name;

        result.metadata["language_confidence"]   = lang.confidence;

        result.metadata["language_routing_hint"] = LanguageDetector::routingHint(lang.code);

    }



    return result;
- Line 234: severity=HIGH; category=arithmetic_overflow
  Description: Arithmetic operation result assigned to variable (potential overflow)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::type_conversion
  Context: ['            // Distribute token influence across dimensions', '            for (int dim_offset = 0; dim_offset < 10; dim_offset++) {', '                int dim = (combined_hash + dim_offset * 73) % EMBEDDING_DIM;', '', '                // Add influence with varying weights based on position']
- Line 195: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'Phase 5: Performance / Hardening' that was not found in 'src/content/ROADMAP.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/content/ROADMAP.md § "Phase 5: Performance / Hardening"
- Line 199: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'Stub/Simulation Lifecycle' that was not found in 'src/content/FUTURE_ENHANCEMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/content/FUTURE_ENHANCEMENTS.md § "Stub/Simulation Lifecycle"

### content/content_manager_llm.cpp
Total findings: 9

- Line 206: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 214: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 76: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: prompt << "3. Sentiment (positive/negative/neutral)\n";
- Line 77: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: prompt << "4. Content category (article/technical/business/personal/other)\n\n";
- Line 82: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: std::string analysis = THEMIS_LLM_GENERATE(prompt.str());
- Line 129: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: std::string tags_text = THEMIS_LLM_GENERATE(prompt.str());
- Line 173: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: std::string summary = THEMIS_LLM_GENERATE(prompt.str());
- Line 211: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: std::string category = THEMIS_LLM_GENERATE(prompt.str());
- Line 262: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: std::string entities_text = THEMIS_LLM_GENERATE(prompt.str());

### content/markdown_processor.cpp
Total findings: 8

- Line 72: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 435: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 448: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 247: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: for (char c : tl) row += (c == '|') ? ' ' : c;
- Line 441: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (consecutive_nl <= 2) result += '\n';
- Line 448: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (!last_was_space && !last_was_newline) result += ' ';
- Line 590: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (!overlap_text.empty()) overlap_text += ' ';
- Line 625: severity=MEDIUM; category=cast_to_smaller_type
  Description: Explicit cast to int detected (verify no overflow on source)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::type_conversion
  Context: ['            size_t combined = token_hash ^ (i * 31) ^ (static_cast<size_t>(seed) * 97);', '            for (int d = 0; d < 10; ++d) {', '                int dim = static_cast<int>((combined + static_cast<size_t>(d) * 73) % DIM);', '                float weight = 1.0f / (1.0f + static_cast<float>(i) * 0.1f);', '                float phase  = static_cast<float>((combined + static_cast<size_t>(dim)) % 360)']

### content/tts_processor.cpp
Total findings: 8

- Line 249: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: auto *voice = new piper::PiperVoice();
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #4833 Continue Phase-6 tensorgrap... (2026-05-07) | #3619 fix(content): build
- Line 272: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: delete static_cast<piper::PiperVoice *>(tts_ctx_);
- Line 272: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: void TTSProcessor::unloadTTSModel() {

#ifdef THEMIS_ENABLE_PIPER_TTS

    if (tts_ctx_) {

        delete static_cast<piper::PiperVoice *>(tts_ctx_);

        tts_ctx_ = nullptr;

    }

#else
- Line 272: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: delete static_cast<piper::PiperVoice *>(tts_ctx_);
- Line 391: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'TTS Backend.' that was not found in 'src/content/FUTURE_ENHANCEMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/content/FUTURE_ENHANCEMENTS.md §TTS Backend.
- Line 465: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'TTS Audio Format Support' that was not found in 'src/content/FUTURE_ENHANCEMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/content/FUTURE_ENHANCEMENTS.md §TTS Audio Format Support.
- Line 483: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'TTS Audio Format Support' that was not found in 'src/content/FUTURE_ENHANCEMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/content/FUTURE_ENHANCEMENTS.md §TTS Audio Format Support.

### content/content_security.cpp
Total findings: 6

- Line 287: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: json safe_metadata;

        // Only keep non-sensitive fields

        if (sanitized.metadata.contains("size")) {

            safe_metadata["size"] = sanitized.metadata["size"];

        }

        if (sanitized.metadata.contains("mime_type")) {

            safe_metadata["mime_type"] = sanitized.metadata["mime_type"];
- Line 290: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: safe_metadata["size"] = sanitized.metadata["size"];

        }

        if (sanitized.metadata.contains("mime_type")) {

            safe_metadata["mime_type"] = sanitized.metadata["mime_type"];

        }

        sanitized.metadata = safe_metadata;

    }
- Line 404: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> seen_types;
- Line 529: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: static const std::regex unix_path_regex(R"((/(?:[a-zA-Z_][a-zA-Z0-9_\-]*)(?:/[a-zA-Z0-9_\-.]+)+))");
- Line 530: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: static const std::regex windows_path_regex(R"([A-Z]:\\([a-zA-Z0-9_\-]+\\)+[a-zA-Z0-9_\-./]*)");
- Line 531: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: static const std::regex home_path_regex(R"(~/[a-zA-Z0-9_\-./]+)");

### content/pipeline/multimodal_chunker.cpp
Total findings: 5

- Line 150: severity=CRITICAL; category=multiplication_overflow
  Description: Multi-factor multiplication detected (CWE-190, likely overflow risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::type_conversion
  Context: ['    }', '', '    const size_t expected_size = width * height * bytes_per_pixel;', '    if (data.size() != expected_size) {', '        // Size mismatch - fall back to generic chunking']
- Line 100: severity=HIGH; category=allocation_loop
  Description: Dynamic allocation in loop — high overhead
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: // Start new chunk with overlap
- Line 223: severity=MEDIUM; category=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: boundaries.push_back(boundary);
- Line 230: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: boundaries.push_back(text.size());  // End
- Line 270: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: boundaries.push_back(text.size());  // End

### content/content_metrics.cpp
Total findings: 4

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #3556 docs(content): reality-chec... (2026-03-12) | #2592 [content] PDF text
- Line 118: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::map<std::string, double> ContentMetrics::getLatencyPercentiles(const std::string& operation) const {
- Line 120: severity=MEDIUM; category=map_vs_unordered_map
  Description: std::map used only for lookups (consider std::unordered_map)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: std::map<std::string, double> result;
- Line 386: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: oss << "# HELP content_extract_errors_total Total PDF/document extraction errors\n";

### content/content_type.cpp
Total findings: 4

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #3012 [content] HTML content extr... (2026-03-12)
- Line 218: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (size_t i = 0; i < std::min(blob.size(), size_t(1000)); i++) {
- Line 235: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: result.push_back(&type);
- Line 244: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: result.push_back(&type);

### content/version_manager.cpp
Total findings: 4

- Line 64: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 71: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 82: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 91: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety

### content/language_detector.cpp
Total findings: 3

- Line 206: severity=HIGH; category=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: while ((pos = lower.find(pattern, pos)) != std::string::npos) {
- Line 114: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: lower += ' '; // sentinel: ensure first word gets a leading space
- Line 118: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: lower += ' '; // sentinel: ensure last word gets a trailing space

### content/abuse_detector.cpp
Total findings: 2

- Line 187: severity=HIGH; category=regex_in_loop
  Description: std::regex compiled in loop (compile once, reuse)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: for (const auto& f : node["flags"]) {
- Line 201: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: error += "Skipping pattern '" + name + "': " + re.what() + "; ";

### content/embedding_pipeline.cpp
Total findings: 2

- Line 84: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto status = future.wait_for(std::chrono::milliseconds(config_.timeout_ms));
- Line 21: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'Embedding Generation Pipeline":' that was not found in 'src/content/FUTURE_ENHANCEMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: FUTURE_ENHANCEMENTS.md § "Embedding Generation Pipeline":

### content/mock_clip_processor.cpp
Total findings: 2

- Line 24: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: ExtractionResult res;

    res.ok                              = true;

    res.metadata                        = nlohmann::json::object();

    res.metadata["mime_type"]           = content_type.mime_type;

    res.metadata["original_size_bytes"] = static_cast<int>(blob.size());



    // For images we don't extract text; instead produce a mock embedding
- Line 25: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: res.ok                              = true;

    res.metadata                        = nlohmann::json::object();

    res.metadata["mime_type"]           = content_type.mime_type;

    res.metadata["original_size_bytes"] = static_cast<int>(blob.size());



    // For images we don't extract text; instead produce a mock embedding

    res.embedding = computeMockEmbedding_(blob);

### content/pipeline/bulk_upload_interface.cpp
Total findings: 2

- Line 69: severity=MEDIUM; category=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: results.push_back(result);
- Line 77: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: results.push_back(upload(contents[i], metadata_list[i]));

### content/FUTURE_ENHANCEMENTS.md
Total findings: 1

- Line 1: severity=LOW; category=module_doc_linkset_drift
  Description: Module doc 'FUTURE_ENHANCEMENTS.md' is missing expected cross-links: ARCHITECTURE.md
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: Refresh the top-level <!-- Links: ... --> metadata to match the current module doc set

### content/PRODUCTION_REQUIREMENTS.md
Total findings: 1

- Line 1: severity=LOW; category=module_doc_linkset_drift
  Description: Module doc 'PRODUCTION_REQUIREMENTS.md' is missing expected cross-links: FUTURE_ENHANCEMENTS.md, README.md, ROADMAP.md
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: Refresh the top-level <!-- Links: ... --> metadata to match the current module doc set

### content/content_validator.cpp
Total findings: 1

- Line 302: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: normalised.push_back(c == '\\' ? '/' : c);

### content/deduplication_checker.cpp
Total findings: 1

- Line 160: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto val = band_cache_->get(makeBandKey(b, bh));

### content/pipeline/async_bulk_uploader.cpp
Total findings: 1

- Line 97: severity=MEDIUM; category=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: results.push_back(result);

### content/pipeline/zstd_compression.cpp
Total findings: 1

- Line 53: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // This maintains compatibility with standard decompress()

## Update Workflow

- Refresh scanner artifacts with: python tools/gs3_orchestrator.py ./src --output ai_working/gap_scan_results.json
- Regenerate docs with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- Add --no-mirror when you only want archive docs in ai_working/module_gaps.

Format: THEMIS_MODULE_GAPS_V4
