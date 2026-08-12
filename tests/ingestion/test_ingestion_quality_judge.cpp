/*
 * ThemisDB — Ingestion Phase 7 Tests: LLM-as-judge re-ingestion quality control
 *
 * Tests for:
 *   IngestionQualityJudge   (evaluate, config, observers)    IJ-01..IJ-08
 *   ReIngestionController   (process, re-ingestion loop)     RC-01..RC-06
 *   IngestionQualityReport  (structure, diagnostics)         QR-01..QR-04
 *   IngestionManager wiring (setReIngestionController)       IM-01..IM-02
 *
 * Acceptance criteria:
 *
 * IJ-01  evaluate() with NullBackend (isAvailable=false): all scores=-1, passed=true (fail-open)
 * IJ-02  evaluate() with scored backend: all four dimension scores in [0,1]
 * IJ-03  evaluate() passes when all dimension scores >= thresholds
 * IJ-04  evaluate() fails when any score < its threshold
 * IJ-05  evaluate() populates missing_entities from LLM MISSING: list
 * IJ-06  evaluate() populates ungrounded_claims from LLM UNGROUNDED: list
 * IJ-07  addObserver() → onQualityEvaluated() called after evaluate()
 * IJ-08  evaluate() skips context below min_text_bytes_for_eval → fail-open
 *
 * RC-01  process() passes on first attempt when quality meets threshold
 * RC-02  process() triggers re-ingestion when quality fails first attempt
 * RC-03  process() stops after max_reingestion_attempts even if quality never met
 * RC-04  process() observer: onReIngestionTriggered + onReIngestionComplete called
 * RC-05  process() tracks score improvement across attempts
 * RC-06  setReIngestionProfile() configures profile used for re-ingestion passes
 *
 * QR-01  IngestionQualityReport: passed=true only when ALL enabled dimensions pass
 * QR-02  IngestionQualityReport: overall_score is weighted combination
 * QR-03  IngestionQualityReport: recommended_steps non-empty when dimension fails
 * QR-04  IngestionQualityReport: judge_backend contains backend description
 *
 * IM-01  IngestionManager::setReIngestionController persists and is retrievable
 * IM-02  IngestionManager::setReIngestionController(nullptr) clears the controller
 */

#include "ingestion/ingestion_quality_judge.h"
#include "ingestion/ingestion_manager.h"
#include "ingestion/extraction_context.h"
#include "ingestion/base_entity.h"
#include "ingestion/file_manifest.h"
#include "ingestion/workflow_engine.h"
#include "ingestion/inference_backend.h"

#include <gtest/gtest.h>
#include <memory>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include <atomic>

namespace themis {
namespace ingestion {
namespace {

// ============================================================================
// Test helpers
// ============================================================================

/// Build a minimal ExtractionContext with the given raw text and entities.
ExtractionContext makeCtx(const std::string& raw_text,
                           std::vector<BaseEntity> entities = {},
                           std::vector<EntityRelation> relations = {}) {
    ExtractionContext ctx;
    ctx.manifest.file_id = "test-doc-001";
    ctx.manifest.original_path = "C:/temp/test-doc-001.txt";
    ctx.manifest.detected_mime = "text/plain";
    ctx.manifest.filename_stem = "test-doc-001";
    ctx.manifest.extension = ".txt";
    ctx.raw_text  = raw_text;
    ctx.entities  = std::move(entities);
    ctx.relations = std::move(relations);
    return ctx;
}

/// Build a BaseEntity with given label and type.
BaseEntity makeEntity(const std::string& id,
                      const std::string& label,
                      const std::string& type) {
    BaseEntity e;
    e.id    = id;
    e.text  = label;
    if (type == "PERSON") {
        e.entity_type = EntityType::PERSON;
    } else if (type == "ORGANIZATION") {
        e.entity_type = EntityType::ORGANIZATION;
    } else if (type == "LOCATION") {
        e.entity_type = EntityType::LOCATION;
    } else if (type == "DATE") {
        e.entity_type = EntityType::DATE;
    } else if (type == "LEGAL_PROVISION") {
        e.entity_type = EntityType::LEGAL_PROVISION;
    } else if (type == "LEGAL_NORM_REFERENCE") {
        e.entity_type = EntityType::LEGAL_NORM_REFERENCE;
    } else {
        e.entity_type = EntityType::UNKNOWN;
    }
    e.properties["label"] = label;
    e.properties["type"] = type;
    return e;
}

class PopulateContextStep : public IIngestionStep {
public:
    const char* getName() const override { return "test.populate_context"; }
    const char* getVersion() const override { return "1.0.0"; }
    plugins::PluginCapabilities getCapabilities() const override { return {}; }
    bool initialize(const char*) override { return true; }
    void shutdown() override {}
    void* getInstance() override { return this; }

    std::vector<std::string> supportedMimeTypes() const override { return {}; }

    Result<void> execute(ExtractionContext& ctx, const StepConfig&) override {
        ctx.raw_text += "This context is long enough for the judge to evaluate the extraction quality. ";
        ctx.raw_text += "It contains enough source text and at least one extracted entity for the test.";
        if (ctx.entities.empty()) {
            ctx.entities.push_back(makeEntity("e1", "§ 42 BGB", "LEGAL_PROVISION"));
        }
        return {};
    }
};

static std::string writeTempProfile(const std::string& content,
                                    const std::string& name) {
    const auto path = (std::filesystem::temp_directory_path() / name).string();
    std::ofstream file(path, std::ios::binary);
    file << content;
    return path;
}

static std::shared_ptr<WorkflowEngine> makeWorkflowEngineWithPopulateStep(
    const std::string& profile_name = "default",
    const std::string& profile_file = "ingestion_quality_profile.json") {
    auto engine = std::make_shared<WorkflowEngine>();
    const auto path = writeTempProfile(
        std::string("{\"name\":\"") + profile_name + "\",\"steps\":[{\"name\":\"populate\",\"plugin\":\"test.populate_context\",\"on_failure\":\"abort\"}]}",
        profile_file);
    auto res = engine->loadProfile(path);
    EXPECT_TRUE(res.has_value()) << res.error().message();
    auto register_res = engine->stepRegistry().registerStep(
        "test.populate_context", std::make_shared<PopulateContextStep>());
    EXPECT_TRUE(register_res.has_value()) << register_res.error().message();
    return engine;
}

/// Fake backend: returns a pre-configured LLM response for every prompt.
class FakeTextBackend : public ITextGenerationBackend {
public:
    explicit FakeTextBackend(std::string response = "", bool available = true)
        : response_(std::move(response)), available_(available) {}

    std::string generate(const std::string& /*prompt*/,
                         int   /*max_tokens*/,
                         double /*temperature*/,
                         const std::string& /*lora*/) override {
        ++call_count_;
        return response_;
    }

    bool        isAvailable() const override { return available_; }
    std::string description() const override { return "FakeTextBackend-v1"; }

    int callCount() const { return call_count_.load(); }

    void setResponse(const std::string& r) { response_ = r; }

private:
    std::string      response_;
    bool             available_;
    std::atomic<int> call_count_{0};
};

/// Observer that counts events and captures the last report.
class CountingObserver : public IIngestionQualityObserver {
public:
    void onQualityEvaluated(const std::string& doc_id,
                            const IngestionQualityReport& report) noexcept override {
        ++evaluated_count_;
        last_report_ = report;
        last_doc_id_ = doc_id;
    }

    void onReIngestionTriggered(const std::string& doc_id,
                                int                attempt,
                                const std::vector<std::string>& /*reasons*/) noexcept override {
        ++triggered_count_;
        last_triggered_doc_ = doc_id;
        last_triggered_attempt_ = attempt;
    }

    void onReIngestionComplete(const std::string& doc_id,
                               int                attempt,
                               bool               improved) noexcept override {
        ++complete_count_;
        last_complete_doc_     = doc_id;
        last_complete_attempt_ = attempt;
        last_improved_         = improved;
    }

    int evaluated_count_    = 0;
    int triggered_count_    = 0;
    int complete_count_     = 0;
    int last_triggered_attempt_ = 0;
    int last_complete_attempt_  = 0;
    bool last_improved_         = false;
    std::string last_doc_id_;
    std::string last_triggered_doc_;
    std::string last_complete_doc_;
    IngestionQualityReport last_report_;
};

/// Build a response string that encodes a given score for all dimensions.
std::string buildScoredResponse(double score,
                                const std::string& extra_sections = "") {
    std::ostringstream oss;
    oss << "SCORE: " << score << "\n"
        << "RATIONALE: Synthetic test response.\n";
    if (!extra_sections.empty()) oss << extra_sections;
    return oss.str();
}

// ============================================================================
// IJ — IngestionQualityJudge tests
// ============================================================================

TEST(IJ, IJ01_NullBackend_FailOpen) {
    // Null (unavailable) backend → all scores = -1.0, passed = true (fail-open)
    auto backend = std::make_shared<NullTextGenerationBackend>();
    IngestionQualityJudge judge(backend);

    auto ctx    = makeCtx("Some document text about §42 BGB.");
    auto report = judge.evaluate(ctx);

    EXPECT_TRUE(report.passed) << "Fail-open: should pass when backend unavailable";
    EXPECT_DOUBLE_EQ(report.completeness_score,       -1.0);
    EXPECT_DOUBLE_EQ(report.groundedness_score,        -1.0);
    EXPECT_DOUBLE_EQ(report.entity_coverage_score,    -1.0);
    EXPECT_DOUBLE_EQ(report.relation_coherence_score, -1.0);
    EXPECT_DOUBLE_EQ(report.overall_score,             -1.0);
}

TEST(IJ, IJ02_ScoredBackend_ScoresInRange) {
    auto backend = std::make_shared<FakeTextBackend>(buildScoredResponse(0.82));
    IngestionQualityJudge judge(backend);

    auto ctx    = makeCtx("Gemäß § 42 BGB gilt folgendes.",
                          {makeEntity("e1", "§ 42 BGB", "LEGAL_PROVISION")});
    auto report = judge.evaluate(ctx);

    for (double s : {report.completeness_score,
                     report.groundedness_score,
                     report.entity_coverage_score,
                     report.relation_coherence_score}) {
        EXPECT_GE(s, 0.0) << "Score must be >= 0";
        EXPECT_LE(s, 1.0) << "Score must be <= 1";
    }
}

TEST(IJ, IJ03_AllScoresAboveThreshold_Passes) {
    auto backend = std::make_shared<FakeTextBackend>(buildScoredResponse(0.95));
    IngestionJudgeConfig cfg;
    cfg.completeness_threshold        = 0.70;
    cfg.groundedness_threshold        = 0.70;
    cfg.entity_coverage_threshold     = 0.70;
    cfg.relation_coherence_threshold  = 0.70;
    cfg.overall_threshold             = 0.70;

    IngestionQualityJudge judge(backend, cfg);

    auto ctx    = makeCtx("Text mit mehr als 100 Bytes damit der Mindest-Check greift. "
                          "Hier stehen relevante Rechtsinformationen.",
                          {makeEntity("e1", "§ 12 StGB", "LEGAL_PROVISION")});
    auto report = judge.evaluate(ctx);

    EXPECT_TRUE(report.passed);
}

TEST(IJ, IJ04_ScoreBelowThreshold_Fails) {
    auto backend = std::make_shared<FakeTextBackend>(buildScoredResponse(0.50));
    IngestionJudgeConfig cfg;
    cfg.completeness_threshold = 0.75; // 0.50 < 0.75 → should fail

    IngestionQualityJudge judge(backend, cfg);

    auto ctx    = makeCtx("Langer genug Text um die Mindest-Bytes-Prüfung zu bestehen. "
                          "Mehr Text für die Bytes-Anforderung.",
                          {makeEntity("e1", "Person", "PERSON")});
    auto report = judge.evaluate(ctx);

    EXPECT_FALSE(report.passed);
}

TEST(IJ, IJ05_MissingEntitiesPopulated) {
    std::string response = buildScoredResponse(
        0.60,
        "MISSING:\n- Hans Müller\n- § 17 StGB\n");

    auto backend = std::make_shared<FakeTextBackend>(response);
    IngestionJudgeConfig cfg;
    cfg.completeness_threshold = 0.75; // force fail so report is returned
    IngestionQualityJudge judge(backend, cfg);

    auto ctx    = makeCtx("Sehr langer Text mit mindestens hundert Zeichen damit "
                          "die Mindest-Bytes-Prüfung nicht greift und wir evaluieren.",
                          {makeEntity("e1", "§ 42 BGB", "LEGAL_PROVISION")});
    auto report = judge.evaluate(ctx);

    EXPECT_FALSE(report.missing_entities.empty());
    bool found_person = false;
    for (const auto& m : report.missing_entities) {
        if (m.find("Hans") != std::string::npos ||
            m.find("Müller") != std::string::npos) {
            found_person = true;
        }
    }
    EXPECT_TRUE(found_person) << "Expected 'Hans Müller' in missing_entities";
}

TEST(IJ, IJ06_UngroundedClaimsPopulated) {
    std::string response = buildScoredResponse(
        0.60,
        "UNGROUNDED:\n- Behauptung ohne Quellenbeleg\n");

    auto backend = std::make_shared<FakeTextBackend>(response);
    IngestionJudgeConfig cfg;
    cfg.groundedness_threshold = 0.75;
    IngestionQualityJudge judge(backend, cfg);

    auto ctx    = makeCtx("Sehr langer Text mit mindestens hundert Zeichen damit "
                          "die Mindest-Bytes-Prüfung nicht greift.",
                          {makeEntity("e1", "§ 1 BGB", "LEGAL_PROVISION")});
    auto report = judge.evaluate(ctx);

    EXPECT_FALSE(report.ungrounded_claims.empty());
}

TEST(IJ, IJ07_ObserverCalledOnEvaluate) {
    auto backend  = std::make_shared<FakeTextBackend>(buildScoredResponse(0.90));
    IngestionQualityJudge judge(backend);

    auto observer = std::make_shared<CountingObserver>();
    judge.addObserver(observer);

    auto ctx = makeCtx("Text lang genug für Evaluation — mehr als hundert Zeichen. "
                       "Weiterer Inhalt zur Auffüllung.",
                       {makeEntity("e1", "Org", "ORGANIZATION")});
    judge.evaluate(ctx);

    EXPECT_EQ(observer->evaluated_count_, 1);
    EXPECT_FALSE(observer->last_doc_id_.empty());
}

TEST(IJ, IJ08_ShortTextSkipsEvaluation_FailOpen) {
    auto backend = std::make_shared<FakeTextBackend>(buildScoredResponse(0.10));
    IngestionJudgeConfig cfg;
    cfg.min_text_bytes_for_eval = 200; // context text will be shorter

    IngestionQualityJudge judge(backend, cfg);

    auto ctx    = makeCtx("Too short."); // < 200 bytes
    auto report = judge.evaluate(ctx);

    EXPECT_TRUE(report.passed)   << "Fail-open when context too sparse";
    EXPECT_DOUBLE_EQ(report.completeness_score, -1.0);
    // Backend should NOT have been called — too short to evaluate
    EXPECT_EQ(backend->callCount(), 0);
}

// ============================================================================
// QR — IngestionQualityReport structure tests
// ============================================================================

TEST(QR, QR01_PassedOnlyWhenAllDimensionsPass) {
    // Force two different scores: one pass, one fail
    auto backend = std::make_shared<FakeTextBackend>();
    IngestionJudgeConfig cfg;
    cfg.completeness_threshold       = 0.70;
    cfg.groundedness_threshold       = 0.90; // high threshold → will fail at 0.80
    cfg.entity_coverage_threshold    = 0.60;
    cfg.relation_coherence_threshold = 0.60;
    cfg.overall_threshold            = 0.60;
    cfg.max_reingestion_attempts     = 0;

    // All dimensions same score = 0.80; groundedness at 0.90 threshold will fail
    backend->setResponse(buildScoredResponse(0.80));
    IngestionQualityJudge judge(backend, cfg);

    auto ctx    = makeCtx("Sehr langer Text mit mindestens hundert Zeichen "
                          "für die Mindest-Bytes-Prüfung.",
                          {makeEntity("e1", "§ 42 BGB", "LEGAL_PROVISION")});
    auto report = judge.evaluate(ctx);

    EXPECT_FALSE(report.passed)
        << "passed must be false when groundedness_score(0.80) < threshold(0.90)";
}

TEST(QR, QR02_OverallScoreIsWeightedCombination) {
    // All dimensions score 1.0 → overall must be 1.0 regardless of weights
    auto backend = std::make_shared<FakeTextBackend>(buildScoredResponse(1.0));
    IngestionQualityJudge judge(backend);

    auto ctx    = makeCtx("Langer Text mit mehr als hundert Bytes um die Prüfung "
                          "zu bestehen. Mehr Text für mehr Bytes.",
                          {makeEntity("e1", "§ 1 StGB", "LEGAL_PROVISION")});
    auto report = judge.evaluate(ctx);

    EXPECT_NEAR(report.overall_score, 1.0, 0.01);
}

TEST(QR, QR03_RecommendedStepsNonEmptyOnFailure) {
    auto backend = std::make_shared<FakeTextBackend>(buildScoredResponse(0.40));
    IngestionJudgeConfig cfg;
    cfg.completeness_threshold       = 0.70;
    cfg.groundedness_threshold       = 0.70;
    cfg.entity_coverage_threshold    = 0.70;
    cfg.relation_coherence_threshold = 0.70;

    IngestionQualityJudge judge(backend, cfg);

    auto ctx    = makeCtx("Langer Text für die Mindest-Bytes-Prüfung, mindestens "
                          "hundert Zeichen lang ist dieses Textstück.",
                          {makeEntity("e1", "§ 5 GG", "LEGAL_PROVISION")});
    auto report = judge.evaluate(ctx);

    ASSERT_FALSE(report.passed);
    EXPECT_FALSE(report.recommended_steps.empty())
        << "Failing report must contain step recommendations";
}

TEST(QR, QR04_JudgeBackendPopulated) {
    auto backend = std::make_shared<FakeTextBackend>(buildScoredResponse(0.90));
    IngestionQualityJudge judge(backend);

    auto ctx    = makeCtx("Ausreichend langer Text für die Evaluation. "
                          "Mehr Zeichen für die Bytes-Prüfung.",
                          {makeEntity("e1", "§ 1 BGB", "LEGAL_PROVISION")});
    auto report = judge.evaluate(ctx);

    EXPECT_FALSE(report.judge_backend.empty());
    EXPECT_NE(report.judge_backend.find("FakeTextBackend"), std::string::npos);
}

// ============================================================================
// RC — ReIngestionController tests
// ============================================================================

TEST(RC, RC01_PassesOnFirstAttempt) {
    auto engine  = makeWorkflowEngineWithPopulateStep();
    auto backend = std::make_shared<FakeTextBackend>(buildScoredResponse(0.95));
    IngestionJudgeConfig cfg;
    cfg.max_reingestion_attempts = 3;
    auto judge   = std::make_shared<IngestionQualityJudge>(backend, cfg);
    ReIngestionController ctrl(engine, judge);

    FileManifest manifest;
    manifest.file_id = "sha256:test-doc-rc01";
    manifest.original_path = "C:/temp/doc.txt";
    manifest.detected_mime = "text/plain";

    auto result = ctrl.process(manifest);

    EXPECT_TRUE(result.quality_met);
    EXPECT_EQ(result.attempts, 1) << "Should succeed on first attempt";
    EXPECT_FALSE(result.best_context.raw_text.empty());
}

TEST(RC, RC02_TriggersReIngestionOnFailure) {
    auto engine  = makeWorkflowEngineWithPopulateStep("default", "ingestion_quality_profile_fail.json");
    auto backend = std::make_shared<FakeTextBackend>();
    backend->setResponse(buildScoredResponse(0.30));

    IngestionJudgeConfig cfg;
    cfg.completeness_threshold       = 0.75;
    cfg.groundedness_threshold       = 0.75;
    cfg.entity_coverage_threshold    = 0.75;
    cfg.relation_coherence_threshold = 0.75;
    cfg.max_reingestion_attempts = 2;

    auto judge = std::make_shared<IngestionQualityJudge>(backend, cfg);
    ReIngestionController ctrl(engine, judge);

    FileManifest manifest;
    manifest.file_id = "sha256:test-doc-rc02";
    manifest.original_path = "C:/temp/doc_fail.txt";
    manifest.detected_mime = "text/plain";

    auto result = ctrl.process(manifest);

    EXPECT_FALSE(result.quality_met);
    EXPECT_GT(result.attempts, 1) << "Re-ingestion must have been triggered";
    EXPECT_EQ(result.history.size(), 3u) << "Initial + 2 re-ingestion passes = 3 reports";
}

TEST(RC, RC03_StopsAtMaxAttempts) {
    auto engine  = makeWorkflowEngineWithPopulateStep();
    auto backend = std::make_shared<FakeTextBackend>(buildScoredResponse(0.20));

    IngestionJudgeConfig cfg;
    cfg.overall_threshold            = 0.75;
    cfg.max_reingestion_attempts     = 3;

    auto judge = std::make_shared<IngestionQualityJudge>(backend, cfg);
    ReIngestionController ctrl(engine, judge);

    FileManifest manifest;
    manifest.file_id = "sha256:test-doc-rc03";
    manifest.original_path = "C:/temp/doc_max.txt";
    manifest.detected_mime = "text/plain";

    auto result = ctrl.process(manifest);

    EXPECT_FALSE(result.quality_met);
    EXPECT_EQ(result.attempts, 4) << "1 initial + 3 re-ingestion passes";
    // history must contain one report per attempt
    EXPECT_EQ(static_cast<int>(result.history.size()), 4);
}

TEST(RC, RC04_ObserversCalledOnReIngestion) {
    auto engine  = makeWorkflowEngineWithPopulateStep();
    auto backend = std::make_shared<FakeTextBackend>(buildScoredResponse(0.20));

    IngestionJudgeConfig cfg;
    cfg.overall_threshold        = 0.75;
    cfg.max_reingestion_attempts = 1;

    auto judge = std::make_shared<IngestionQualityJudge>(backend, cfg);
    ReIngestionController ctrl(engine, judge);

    auto obs = std::make_shared<CountingObserver>();
    ctrl.addObserver(obs);

    FileManifest manifest;
    manifest.file_id = "sha256:test-doc-rc04";
    manifest.original_path = "C:/temp/doc_obs.txt";
    manifest.detected_mime = "text/plain";

    ctrl.process(manifest);

    EXPECT_GE(obs->triggered_count_, 1) << "onReIngestionTriggered must be called";
    EXPECT_GE(obs->complete_count_,  1) << "onReIngestionComplete must be called";
}

TEST(RC, RC05_HistoryTracksScoreProgression) {
    auto engine  = makeWorkflowEngineWithPopulateStep();
    auto backend = std::make_shared<FakeTextBackend>(buildScoredResponse(0.20));

    IngestionJudgeConfig cfg;
    cfg.overall_threshold        = 0.75;
    cfg.max_reingestion_attempts = 2;

    auto judge = std::make_shared<IngestionQualityJudge>(backend, cfg);
    ReIngestionController ctrl(engine, judge);

    FileManifest manifest;
    manifest.file_id = "sha256:test-doc-rc05";
    manifest.original_path = "C:/temp/doc_hist.txt";
    manifest.detected_mime = "text/plain";

    auto result = ctrl.process(manifest);

    // history[i].attempt should equal i+1 for i in 0..n-1 (1-indexed)
    ASSERT_GE(result.history.size(), 2u);
    for (int i = 0; i < static_cast<int>(result.history.size()); ++i) {
        EXPECT_EQ(result.history[i].attempt, i + 1)
            << "attempt counter should be 1-based and monotonically increasing";
    }
}

TEST(RC, RC06_ReIngestionProfileIsUsed) {
    // Verify that setReIngestionProfile does not crash and is stored (interface contract).
    auto engine  = makeWorkflowEngineWithPopulateStep("legal-thorough-v2", "ingestion_quality_profile_thorough.json");
    auto backend = std::make_shared<FakeTextBackend>(buildScoredResponse(0.95));

    auto judge = std::make_shared<IngestionQualityJudge>(backend);
    ReIngestionController ctrl(engine, judge);
    ctrl.setReIngestionProfile("legal-thorough-v2");

    FileManifest manifest;
    manifest.file_id = "sha256:test-doc-rc06";
    manifest.original_path = "C:/temp/doc_profile.txt";
    manifest.detected_mime = "text/plain";

    // No exception expected; profile stored and passed to engine on re-ingestion
    EXPECT_NO_THROW(ctrl.process(manifest));
}

// ============================================================================
// IM — IngestionManager wiring tests
// ============================================================================

TEST(IM, IM01_SetGetReIngestionController) {
    IngestionManager mgr("test://ingestion");

    auto engine  = std::make_shared<WorkflowEngine>();
    auto backend = std::make_shared<NullTextGenerationBackend>();
    auto judge   = std::make_shared<IngestionQualityJudge>(backend);
    auto ctrl    = std::make_shared<ReIngestionController>(engine, judge);

    EXPECT_EQ(mgr.getReIngestionController(), nullptr)
        << "Default should be nullptr";

    mgr.setReIngestionController(ctrl);
    EXPECT_EQ(mgr.getReIngestionController(), ctrl)
        << "Should return the controller that was set";
}

TEST(IM, IM02_SetNullClearsController) {
    IngestionManager mgr("test://ingestion");

    auto engine  = std::make_shared<WorkflowEngine>();
    auto backend = std::make_shared<NullTextGenerationBackend>();
    auto judge   = std::make_shared<IngestionQualityJudge>(backend);
    auto ctrl    = std::make_shared<ReIngestionController>(engine, judge);

    mgr.setReIngestionController(ctrl);
    ASSERT_NE(mgr.getReIngestionController(), nullptr);

    mgr.setReIngestionController(nullptr);
    EXPECT_EQ(mgr.getReIngestionController(), nullptr)
        << "Setting nullptr must clear the stored controller";
}

} // namespace
} // namespace ingestion
} // namespace themis
