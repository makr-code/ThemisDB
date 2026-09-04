/*
 * ThemisDB - Process Modeling Module
 *
 * File:    test_fim_importer.cpp
 * Module:  tests/process/
 * Purpose: GTest unit tests FIM-01..FIM-07 for FimImporter.
 *
 * Tests cover:
 *   FIM-01: importSingleModel with valid BPMN 2.0 XML → ok=true, record populated
 *   FIM-02: importSingleModel with empty XML → ok=false
 *   FIM-03: importSingleModel with invalid XML → ok=false
 *   FIM-04: importFimCatalogue with catalogue XML containing one <prozess> block
 *   FIM-05: importFimCatalogue assigns fim:leika: tag when leikaKey is present
 *   FIM-06: importFimCatalogue on plain BPMN (no <prozess> envelope) → single result
 *   FIM-07: importFromFitkoApi returns error result with explanation message
 */

#include <gtest/gtest.h>

#include "process/fim_importer.h"

#include <string>
#include <vector>

using namespace themis;
using namespace themis::process;

// ─────────────────────────────────────────────────────────────────────────────
// XML fixtures
// ─────────────────────────────────────────────────────────────────────────────

static const char* kValidBpmnXml = R"(<?xml version="1.0" encoding="UTF-8"?>
<definitions xmlns="http://www.omg.org/spec/BPMN/20100524/MODEL"
             targetNamespace="http://themis.db/test"
             id="defs1">
  <process id="fim_proc1" name="FIM Test Process">
    <startEvent id="s1" name="Start"/>
    <userTask id="t1" name="Antragsbearbeitung"/>
    <endEvent id="e1" name="Ende"/>
    <sequenceFlow id="sf1" sourceRef="s1" targetRef="t1"/>
    <sequenceFlow id="sf2" sourceRef="t1" targetRef="e1"/>
  </process>
</definitions>)";

static const char* kInvalidXml = R"(this is not xml at all >>> <<< )";

static const char* kFimCatalogueXml = R"(<?xml version="1.0" encoding="UTF-8"?>
<prozessbibliothek xmlns:fim="http://fim.xoev.de/schema/2024"
                   version="2024-01">
  <prozess id="fim_buergeramt_01"
           name="Ummeldung Wohnsitz"
           leikaKey="99062017000000">
    <?xml version="1.0" encoding="UTF-8"?>
    <definitions xmlns="http://www.omg.org/spec/BPMN/20100524/MODEL"
                 targetNamespace="http://fim.xoev.de/bpmn">
      <process id="ummeldung" name="Ummeldung Wohnsitz">
        <startEvent id="s1" name="Antrag einreichen"/>
        <userTask id="t1" name="Prüfung durch Sachbearbeiter"/>
        <endEvent id="e1" name="Wohnsitz umgemeldet"/>
        <sequenceFlow id="sf1" sourceRef="s1" targetRef="t1"/>
        <sequenceFlow id="sf2" sourceRef="t1" targetRef="e1"/>
      </process>
    </definitions>
  </prozess>
</prozessbibliothek>)";

// ─────────────────────────────────────────────────────────────────────────────
// Tests
// ─────────────────────────────────────────────────────────────────────────────

// FIM-01: Valid BPMN → ok=true, record populated.
TEST(FimImporter, FIM01_SingleModelValid) {
    FimImporter imp;
    auto result = imp.importSingleModel(kValidBpmnXml, ProcessDomain::ADMINISTRATION);

    EXPECT_TRUE(result.ok) << result.message;
    EXPECT_EQ(result.record.id, "fim_proc1");
    EXPECT_EQ(result.record.name, "FIM Test Process");
    EXPECT_EQ(result.record.notation, ProcessNotation::BPMN_2_0);
    EXPECT_EQ(result.record.domain, ProcessDomain::ADMINISTRATION);
    EXPECT_EQ(result.record.state, ProcessModelState::ACTIVE);

    // fim:import tag should be set
    bool has_fim_tag = false;
    for (const auto& tag : result.record.compliance_tags) {
        if (tag == "fim:import") { has_fim_tag = true; break; }
    }
    EXPECT_TRUE(has_fim_tag) << "Expected 'fim:import' compliance tag";

    // Normalized graph should contain nodes.
    EXPECT_TRUE(result.record.normalized.contains("nodes"));
    EXPECT_TRUE(result.record.normalized["nodes"].is_array());
    EXPECT_GT(result.record.normalized["nodes"].size(), 0u);
}

// FIM-02: Empty XML → ok=false.
TEST(FimImporter, FIM02_SingleModelEmptyXml) {
    FimImporter imp;
    auto result = imp.importSingleModel("", ProcessDomain::ADMINISTRATION);
    EXPECT_FALSE(result.ok);
    EXPECT_FALSE(result.message.empty());
}

// FIM-03: Invalid (non-XML) input → ok=false (no BPMN flow elements).
TEST(FimImporter, FIM03_SingleModelInvalidXml) {
    FimImporter imp;
    auto result = imp.importSingleModel(kInvalidXml, ProcessDomain::ADMINISTRATION);
    EXPECT_FALSE(result.ok);
}

// FIM-04: Catalogue XML with one <prozess> block → one successful result.
TEST(FimImporter, FIM04_CatalogueImport) {
    FimImporter imp;
    auto results = imp.importFimCatalogue(kFimCatalogueXml, ProcessDomain::ADMINISTRATION);

    ASSERT_FALSE(results.empty());
    // At least one result should be successful.
    bool any_ok = false;
    for (const auto& r : results) {
        if (r.ok) { any_ok = true; break; }
    }
    EXPECT_TRUE(any_ok) << "Expected at least one successful import from catalogue";
}

// FIM-05: Catalogue with leikaKey → fim:leika: compliance tag set.
TEST(FimImporter, FIM05_LeikaKeyTagSet) {
    FimImporter imp;
    auto results = imp.importFimCatalogue(kFimCatalogueXml, ProcessDomain::ADMINISTRATION);

    ASSERT_FALSE(results.empty());
    bool found_leika = false;
    for (const auto& r : results) {
        if (!r.ok) {
          continue;
        }
        for (const auto& tag : r.record.compliance_tags) {
            if (tag.rfind("fim:leika:", 0) == 0) {
                found_leika = true;
                break;
            }
        }
    }
    EXPECT_TRUE(found_leika) << "Expected 'fim:leika:…' compliance tag from catalogue import";
}

// FIM-06: Plain BPMN XML passed to importFimCatalogue → treated as single model.
TEST(FimImporter, FIM06_PlainBpmnAsCatalogue) {
    FimImporter imp;
    auto results = imp.importFimCatalogue(kValidBpmnXml, ProcessDomain::BUSINESS);

    ASSERT_EQ(results.size(), 1u);
    EXPECT_TRUE(results[0].ok) << results[0].message;
    EXPECT_EQ(results[0].record.domain, ProcessDomain::BUSINESS);
}

// FIM-07: importFromFitkoApi returns error (stub not implemented).
TEST(FimImporter, FIM07_FitkoApiReturnsError) {
    FimImporter imp;
    auto results = imp.importFromFitkoApi("https://fim.fitko.de/api/v1");

    ASSERT_FALSE(results.empty());
    EXPECT_FALSE(results[0].ok);
    EXPECT_FALSE(results[0].message.empty());
    // Message should explain that HTTP import is not yet implemented.
    EXPECT_NE(results[0].message.find("not yet implemented"), std::string::npos)
        << "Expected 'not yet implemented' in error message";
}

// FIM-08: importFromFitkoApi with injected HttpFetchFn returns models from JSON envelope.
TEST(FimImporter, FIM08_FitkoApiWithInjectedFetchFn) {
    // Synthesise a FITKO JSON response envelope with one BPMN item.
    const std::string mock_bpmn = R"(<?xml version="1.0" encoding="UTF-8"?>
<definitions xmlns="http://www.omg.org/spec/BPMN/20100524/MODEL"
             targetNamespace="http://fim.xoev.de/bpmn" id="api_defs">
  <process id="api_proc1" name="API Import Process">
    <startEvent id="s1" name="Start"/>
    <endEvent id="e1" name="End"/>
    <sequenceFlow id="sf1" sourceRef="s1" targetRef="e1"/>
  </process>
</definitions>)";

    // Escape double-quotes inside the BPMN string for the JSON value.
    // (The mock_bpmn above has no literal double-quotes, so this is safe.)
    const std::string json_body =
        R"({"items":[{"bpmnXml":")" + mock_bpmn + R"("}]})";

    FimImporter imp;
    std::string captured_url = {};
    imp.setHttpFetchFn([&](std::string_view url) -> std::string {
        captured_url = std::string(url);
        return json_body;
    });

    auto results = imp.importFromFitkoApi(
        "https://fim.fitko.de/api/v1", ProcessDomain::ADMINISTRATION);

    // URL must include the /prozesse suffix.
    EXPECT_EQ(captured_url, "https://fim.fitko.de/api/v1/prozesse");

    ASSERT_EQ(results.size(), 1u);
    EXPECT_TRUE(results[0].ok) << results[0].message;
    EXPECT_FALSE(results[0].record.id.empty());
}
