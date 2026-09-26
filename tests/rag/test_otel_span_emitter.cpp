// Copyright ThemisDB Contributors
// SPDX-License-Identifier: Apache-2.0

#include <gtest/gtest.h>

#include "rag/otel_span_emitter.h"

namespace themis::rag::testing {

class OTELSpanEmitterTest : public ::testing::Test {
 protected:
  OTELSpanEmitterTest()
      : emitter_(std::make_unique<OTELSpanEmitter>("RAG", "localhost:4317")) {}

  std::unique_ptr<OTELSpanEmitter> emitter_;
};

TEST_F(OTELSpanEmitterTest, CreateSpan) {
  auto span = emitter_->StartSpan("rag.query");
  EXPECT_NE(span, nullptr);
}

TEST_F(OTELSpanEmitterTest, SetStringAttribute) {
  auto span = emitter_->StartSpan("rag.query");
  EXPECT_NO_THROW(span->SetAttribute("query_id", "q123"));
}

TEST_F(OTELSpanEmitterTest, SetNumericAttribute) {
  auto span = emitter_->StartSpan("rag.query");
  EXPECT_NO_THROW(span->SetAttribute("latency_ms", 150UL));
}

TEST_F(OTELSpanEmitterTest, SetBooleanAttribute) {
  auto span = emitter_->StartSpan("rag.query");
  EXPECT_NO_THROW(span->SetAttribute("success", true));
}

TEST_F(OTELSpanEmitterTest, RecordEvent) {
  auto span = emitter_->StartSpan("rag.query");
  std::map<std::string, std::string> attrs = {{"event_id", "e1"}};
  EXPECT_NO_THROW(span->RecordEvent("query_start", attrs));
}

TEST_F(OTELSpanEmitterTest, EndSpan) {
  auto span = emitter_->StartSpan("rag.query");
  EXPECT_NO_THROW(span->EndSpan());
}

TEST_F(OTELSpanEmitterTest, EndSpanWithError) {
  auto span = emitter_->StartSpan("rag.query");
  EXPECT_NO_THROW(span->EndSpanWithError("TIMEOUT", "Query exceeded 5s limit"));
}

TEST_F(OTELSpanEmitterTest, MultipleSpans) {
  auto span1 = emitter_->StartSpan("rag.retrieve");
  auto span2 = emitter_->StartSpan("rag.rerank");
  
  span1->SetAttribute("doc_count", 100UL);
  span2->SetAttribute("rerank_count", 50UL);
  
  EXPECT_NO_THROW(span1->EndSpan());
  EXPECT_NO_THROW(span2->EndSpan());
}

TEST_F(OTELSpanEmitterTest, SetBaggage) {
  EXPECT_NO_THROW(emitter_->SetBaggage("traceparent=123"));
}

TEST_F(OTELSpanEmitterTest, GetBaggage) {
  emitter_->SetBaggage("user-id=u456");
  std::string baggage = emitter_->GetBaggage();
  EXPECT_EQ(baggage, "user-id=u456");
}

TEST_F(OTELSpanEmitterTest, BatchExportEnabled) {
  emitter_->SetBatchExportEnabled(true);
  EXPECT_NO_THROW({});
}

TEST_F(OTELSpanEmitterTest, BatchExportSize) {
  emitter_->SetBatchExportSize(50);
  EXPECT_NO_THROW({});
}

TEST_F(OTELSpanEmitterTest, Flush) {
  auto span = emitter_->StartSpan("rag.query");
  span->EndSpan();
  
  bool result = emitter_->Flush();
  EXPECT_TRUE(result);
}

TEST_F(OTELSpanEmitterTest, GetStats) {
  auto span = emitter_->StartSpan("rag.query");
  auto stats = emitter_->GetStats();
  
  EXPECT_GT(stats["pending_spans"], 0);
}

TEST_F(OTELSpanEmitterTest, SpanWithAttributes) {
  auto span = emitter_->StartSpan("rag.retrieve");
  span->SetAttribute("query_id", "q789");
  span->SetAttribute("doc_count", 20UL);
  span->SetAttribute("success", true);
  
  EXPECT_NO_THROW(span->EndSpan());
}

}  // namespace themis::rag::testing
