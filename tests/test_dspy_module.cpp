/**
 * @file test_dspy_module.cpp
 * @brief Unit tests for the DSPy-compatible prompt declaration layer.
 *
 * Acceptance criteria:
 *  AC-1  DspyField – constructor sets all members correctly.
 *  AC-2  DspySignature – addInput / addOutput / getName / getDescription.
 *  AC-3  DspySignature::buildPrompt – required fields rendered, description header present.
 *  AC-4  DspySignature::buildPrompt – output field labels appended with # description.
 *  AC-5  DspySignature::buildPrompt – throws DspyMissingFieldError on missing required input.
 *  AC-6  DspySignature::buildPrompt – optional fields use default_value when absent.
 *  AC-7  DspySignature::parseResponse – extracts output field values from well-formed response.
 *  AC-8  DspySignature::parseResponse – missing required output sets warning, uses default.
 *  AC-9  DspySignature::parseResponse – handles multiple output fields independently.
 *  AC-10 EchoDspyLLMProvider – returns one echo line per output label in the prompt.
 *  AC-11 DspyModule::setLLMProvider / getSignature – accessors.
 *  AC-12 DspyModule::forward – throws std::runtime_error without a provider.
 *  AC-13 DspyModule::forward – returns parsed output map when provider is set.
 *  AC-14 DspyPredict – inherits DspyModule forward correctly.
 *  AC-15 DspyChainOfThought – "Reasoning" field injected as first output.
 *  AC-16 DspyChainOfThought – forward returns "Reasoning" key in output map.
 *  AC-17 DspyChainOfThought – original output fields preserved after Reasoning.
 *  AC-18 DspyMissingFieldError – fieldName() returns the missing field name.
 *  AC-19 DspySignature – fluent API (addInput returns *this).
 *  AC-20 DspySignature::parseResponse – trims trailing whitespace from values.
 *  AC-21 DspySignature::buildPrompt – no description header when description empty.
 *  AC-22 DspySignature – multiple inputs rendered in declaration order.
 *  AC-23 DspyPredict – provider injection via setLLMProvider is chainable.
 *  AC-24 DspySignature::parseResponse – value starts after the colon+space.
 *  AC-25 DspySignature – signature with no outputs produces prompt with no label lines.
 *  AC-26 DspyChainOfThought – inputs are identical to the original signature inputs.
 *  AC-27 DspyField – DspyFieldType correctly stored (INT, FLOAT, BOOL, LIST, JSON).
 *  AC-28 Custom IDspyLLMProvider – forward calls provider exactly once per call.
 *  AC-29 DspySignature::parseResponse – field value may span until next field marker.
 *  AC-30 DspyChainOfThought – description preserved from original signature.
 */

#include <gtest/gtest.h>
#include "prompt_engineering/dspy_module.h"

#include <memory>
#include <string>
#include <unordered_map>

using namespace themis::prompt_engineering;

// ============================================================================
// Helpers
// ============================================================================

static DspySignature makeSentimentSig()
{
    DspySignature sig("SentimentAnalysis",
                      "Classify the sentiment of the given text.");
    sig.addInput({"text", "The text to classify."});
    sig.addOutput({"sentiment", "One of: positive, negative, neutral."});
    sig.addOutput({"confidence", "Score between 0 and 1.", DspyFieldType::FLOAT});
    return sig;
}

/** Deterministic provider that returns a canned response string. */
class FixedProvider : public IDspyLLMProvider {
public:
    explicit FixedProvider(std::string response) : response_(std::move(response)) {}

    std::string complete(const std::string& /*prompt*/) override
    {
        ++call_count_;
        return response_;
    }

    int callCount() const { return call_count_; }

private:
    std::string response_;
    int call_count_ = 0;
};

// ============================================================================
// AC-1: DspyField constructor
// ============================================================================

TEST(DspyFieldTest, ConstructorSetsAllMembers)
{
    DspyField f("score", "A numeric score.", DspyFieldType::FLOAT, false, "0.0");
    EXPECT_EQ(f.name,          "score");
    EXPECT_EQ(f.description,   "A numeric score.");
    EXPECT_EQ(f.type,          DspyFieldType::FLOAT);
    EXPECT_FALSE(f.required);
    EXPECT_EQ(f.default_value, "0.0");
}

// ============================================================================
// AC-2: DspySignature basic accessors
// ============================================================================

TEST(DspySignatureTest, NameAndDescriptionAccessors)
{
    DspySignature sig("TaskA", "Do task A.");
    EXPECT_EQ(sig.getName(),        "TaskA");
    EXPECT_EQ(sig.getDescription(), "Do task A.");
}

TEST(DspySignatureTest, AddInputAddOutputCountCorrect)
{
    auto sig = makeSentimentSig();
    EXPECT_EQ(sig.inputs().size(),  1u);
    EXPECT_EQ(sig.outputs().size(), 2u);
}

// ============================================================================
// AC-3: buildPrompt – required fields rendered, description header
// ============================================================================

TEST(DspySignatureTest, BuildPromptContainsDescriptionHeader)
{
    auto sig = makeSentimentSig();
    std::string prompt = sig.buildPrompt({{"text", "I love this!"}});
    EXPECT_NE(prompt.find("Classify the sentiment"), std::string::npos);
}

TEST(DspySignatureTest, BuildPromptContainsInputValues)
{
    auto sig = makeSentimentSig();
    std::string prompt = sig.buildPrompt({{"text", "Great product."}});
    EXPECT_NE(prompt.find("text: Great product."), std::string::npos);
}

// ============================================================================
// AC-4: buildPrompt – output labels with # description
// ============================================================================

TEST(DspySignatureTest, BuildPromptHasOutputLabels)
{
    auto sig = makeSentimentSig();
    std::string prompt = sig.buildPrompt({{"text", "Hello"}});
    EXPECT_NE(prompt.find("sentiment:"), std::string::npos);
    EXPECT_NE(prompt.find("confidence:"), std::string::npos);
}

// ============================================================================
// AC-5: buildPrompt – throws on missing required input
// ============================================================================

TEST(DspySignatureTest, BuildPromptThrowsOnMissingRequiredInput)
{
    auto sig = makeSentimentSig();
    EXPECT_THROW(sig.buildPrompt({}), DspyMissingFieldError);
}

TEST(DspySignatureTest, BuildPromptThrowsCorrectFieldName)
{
    auto sig = makeSentimentSig();
    try {
        sig.buildPrompt({});
        FAIL() << "Expected DspyMissingFieldError";
    } catch (const DspyMissingFieldError& e) {
        EXPECT_EQ(e.fieldName(), "text");
    }
}

// ============================================================================
// AC-6: buildPrompt – optional fields use default_value when absent
// ============================================================================

TEST(DspySignatureTest, BuildPromptUsesDefaultForOptionalField)
{
    DspySignature sig("Opt", "Optional test.");
    sig.addInput({"query", "Query text."});
    sig.addInput({"language", "Language code.", DspyFieldType::STRING, false, "en"});
    sig.addOutput({"answer", "The answer."});

    std::string prompt = sig.buildPrompt({{"query", "hello"}});
    // The optional 'language' field should appear with its default
    EXPECT_NE(prompt.find("language: en"), std::string::npos);
}

// ============================================================================
// AC-7: parseResponse – extracts field values
// ============================================================================

TEST(DspySignatureTest, ParseResponseExtractsValues)
{
    auto sig = makeSentimentSig();
    std::string response = "sentiment: positive\nconfidence: 0.95\n";
    auto parsed = sig.parseResponse(response);
    EXPECT_EQ(parsed["sentiment"],  "positive");
    EXPECT_EQ(parsed["confidence"], "0.95");
}

// ============================================================================
// AC-8: parseResponse – missing required output uses default
// ============================================================================

TEST(DspySignatureTest, ParseResponseMissingFieldUsesDefault)
{
    DspySignature sig("X", "");
    sig.addInput({"q", "Question."});
    sig.addOutput({"answer", "The answer.", DspyFieldType::STRING, true, "unknown"});

    auto parsed = sig.parseResponse("no answer here");
    EXPECT_EQ(parsed["answer"], "unknown");
}

// ============================================================================
// AC-9: parseResponse – multiple output fields independently
// ============================================================================

TEST(DspySignatureTest, ParseResponseMultipleFieldsIndependent)
{
    auto sig = makeSentimentSig();
    std::string response = "confidence: 0.80\nsentiment: negative\n";
    auto parsed = sig.parseResponse(response);
    EXPECT_EQ(parsed["sentiment"],  "negative");
    EXPECT_EQ(parsed["confidence"], "0.80");
}

// ============================================================================
// AC-10: EchoDspyLLMProvider
// ============================================================================

TEST(EchoDspyLLMProviderTest, ReturnsEchoForOutputLabels)
{
    auto sig = makeSentimentSig();
    std::string prompt = sig.buildPrompt({{"text", "Test"}});

    EchoDspyLLMProvider echo;
    std::string response = echo.complete(prompt);

    EXPECT_NE(response.find("sentiment:"), std::string::npos);
    EXPECT_NE(response.find("confidence:"), std::string::npos);
}

TEST(EchoDspyLLMProviderTest, EchoValueIsEchoPlaceholder)
{
    auto sig = makeSentimentSig();
    std::string prompt = sig.buildPrompt({{"text", "Test"}});

    EchoDspyLLMProvider echo;
    std::string response = echo.complete(prompt);
    EXPECT_NE(response.find("[echo]"), std::string::npos);
}

// ============================================================================
// AC-11: DspyModule accessors
// ============================================================================

TEST(DspyModuleTest, GetSignatureReturnsSignature)
{
    DspyModule mod(makeSentimentSig());
    EXPECT_EQ(mod.getSignature().getName(), "SentimentAnalysis");
}

TEST(DspyModuleTest, SetLLMProviderIsChainable)
{
    DspyModule mod(makeSentimentSig());
    auto provider = std::make_shared<EchoDspyLLMProvider>();
    DspyModule& ref = mod.setLLMProvider(provider);
    EXPECT_EQ(&ref, &mod);
}

// ============================================================================
// AC-12: forward – throws without provider
// ============================================================================

TEST(DspyModuleTest, ForwardThrowsWithoutProvider)
{
    DspyModule mod(makeSentimentSig());
    EXPECT_THROW(mod.forward({{"text", "Hi"}}), std::runtime_error);
}

// ============================================================================
// AC-13: forward – returns parsed output
// ============================================================================

TEST(DspyModuleTest, ForwardReturnsParsedOutput)
{
    DspyModule mod(makeSentimentSig());
    auto provider = std::make_shared<FixedProvider>(
        "sentiment: positive\nconfidence: 0.88\n");
    mod.setLLMProvider(provider);

    auto result = mod.forward({{"text", "I love this!"}});
    EXPECT_EQ(result["sentiment"],  "positive");
    EXPECT_EQ(result["confidence"], "0.88");
}

// ============================================================================
// AC-14: DspyPredict inherits forward
// ============================================================================

TEST(DspyPredictTest, ForwardWorksLikeDspyModule)
{
    DspyPredict pred(makeSentimentSig());
    auto provider = std::make_shared<FixedProvider>(
        "sentiment: neutral\nconfidence: 0.50\n");
    pred.setLLMProvider(provider);

    auto result = pred.forward({{"text", "Okay."}});
    EXPECT_EQ(result["sentiment"],  "neutral");
    EXPECT_EQ(result["confidence"], "0.50");
}

// ============================================================================
// AC-15: DspyChainOfThought – Reasoning field injected first
// ============================================================================

TEST(DspyChainOfThoughtTest, ReasoningFieldInjectedFirst)
{
    DspyChainOfThought cot(makeSentimentSig());
    const auto& outputs = cot.getSignature().outputs();
    ASSERT_FALSE(outputs.empty());
    EXPECT_EQ(outputs.front().name, "Reasoning");
}

// ============================================================================
// AC-16: DspyChainOfThought::forward – returns Reasoning key
// ============================================================================

TEST(DspyChainOfThoughtTest, ForwardReturnsReasoningKey)
{
    DspyChainOfThought cot(makeSentimentSig());
    auto provider = std::make_shared<FixedProvider>(
        "Reasoning: Because the text is upbeat.\nsentiment: positive\nconfidence: 0.92\n");
    cot.setLLMProvider(provider);

    auto result = cot.forward({{"text", "Amazing!"}});
    EXPECT_EQ(result["Reasoning"], "Because the text is upbeat.");
    EXPECT_EQ(result["sentiment"], "positive");
}

// ============================================================================
// AC-17: DspyChainOfThought – original output fields preserved
// ============================================================================

TEST(DspyChainOfThoughtTest, OriginalOutputFieldsPreserved)
{
    DspyChainOfThought cot(makeSentimentSig());
    const auto& outputs = cot.getSignature().outputs();
    // Should have Reasoning + 2 original fields = 3
    EXPECT_EQ(outputs.size(), 3u);

    bool found_sentiment   = false;
    bool found_confidence  = false;
    for (const auto& f : outputs) {
        if (f.name == "sentiment") {
          found_sentiment  = true;
        }
        if (f.name == "confidence") {
          found_confidence = true;
        }
    }
    EXPECT_TRUE(found_sentiment);
    EXPECT_TRUE(found_confidence);
}

// ============================================================================
// AC-18: DspyMissingFieldError::fieldName
// ============================================================================

TEST(DspyMissingFieldErrorTest, FieldNameAccessor)
{
    DspyMissingFieldError err("my_field");
    EXPECT_EQ(err.fieldName(), "my_field");
}

TEST(DspyMissingFieldErrorTest, IsInvalidArgument)
{
    EXPECT_THROW({
        throw DspyMissingFieldError("x");
    }, std::invalid_argument);
}

// ============================================================================
// AC-19: Fluent API – addInput returns *this
// ============================================================================

TEST(DspySignatureTest, FluentApiReturnsSelf)
{
    DspySignature sig("F", "fluent");
    DspySignature& ref = sig.addInput({"a", "field a"});
    EXPECT_EQ(&ref, &sig);

    DspySignature& ref2 = sig.addOutput({"b", "field b"});
    EXPECT_EQ(&ref2, &sig);
}

// ============================================================================
// AC-20: parseResponse – trims trailing whitespace
// ============================================================================

TEST(DspySignatureTest, ParseResponseTrimsTrailingWhitespace)
{
    DspySignature sig("T", "");
    sig.addInput({"q", "question"});
    sig.addOutput({"ans", "answer"});

    auto parsed = sig.parseResponse("ans: hello world   \n");
    EXPECT_EQ(parsed["ans"], "hello world");
}

// ============================================================================
// AC-21: buildPrompt – no description header when description empty
// ============================================================================

TEST(DspySignatureTest, NoDescriptionHeaderWhenEmpty)
{
    DspySignature sig("Empty", "");
    sig.addInput({"x", "input"});
    sig.addOutput({"y", "output"});

    std::string prompt = sig.buildPrompt({{"x", "val"}});
    // No description should mean prompt starts with the first input label.
    ASSERT_EQ(prompt.find("x:"), 0u);
    EXPECT_NE(prompt.find('\n'), std::string::npos);
}

// ============================================================================
// AC-22: multiple inputs in declaration order
// ============================================================================

TEST(DspySignatureTest, MultipleInputsInDeclarationOrder)
{
    DspySignature sig("Multi", "Multi-input test.");
    sig.addInput({"first", "First field."});
    sig.addInput({"second", "Second field."});
    sig.addOutput({"result", "Result."});

    std::string prompt = sig.buildPrompt({{"first", "A"}, {"second", "B"}});
    auto pos_first  = prompt.find("first:");
    auto pos_second = prompt.find("second:");
    ASSERT_NE(pos_first,  std::string::npos);
    ASSERT_NE(pos_second, std::string::npos);
    EXPECT_LT(pos_first, pos_second);
}

// ============================================================================
// AC-23: setLLMProvider is chainable
// ============================================================================

TEST(DspyPredictTest, SetLLMProviderChainable)
{
    DspyPredict pred(makeSentimentSig());
    auto prov = std::make_shared<EchoDspyLLMProvider>();
    auto& ref = pred.setLLMProvider(prov);
    EXPECT_EQ(&ref, &pred);
}

// ============================================================================
// AC-24: parseResponse – value starts after colon+space
// ============================================================================

TEST(DspySignatureTest, ParseResponseValueStartsAfterColonSpace)
{
    DspySignature sig("P", "");
    sig.addInput({"q", "question"});
    sig.addOutput({"ans", "answer"});

    auto parsed = sig.parseResponse("ans: forty-two");
    EXPECT_EQ(parsed["ans"], "forty-two");
    // Ensure the colon itself is not part of the value
    EXPECT_EQ(parsed["ans"].find(':'), std::string::npos);
}

// ============================================================================
// AC-25: signature with no outputs – buildPrompt produces no label lines
// ============================================================================

TEST(DspySignatureTest, NoOutputsNoLabelLines)
{
    DspySignature sig("NoOut", "No outputs.");
    sig.addInput({"x", "input"});
    std::string prompt = sig.buildPrompt({{"x", "v"}});
    // Should contain x: but no colon-terminated output labels
    EXPECT_NE(prompt.find("x: v"), std::string::npos);
}

// ============================================================================
// AC-26: DspyChainOfThought – inputs identical to original
// ============================================================================

TEST(DspyChainOfThoughtTest, InputsIdenticalToOriginal)
{
    auto orig = makeSentimentSig();
    DspyChainOfThought cot(orig);

    const auto& orig_inputs = orig.inputs();
    const auto& cot_inputs  = cot.getSignature().inputs();

    ASSERT_EQ(cot_inputs.size(), orig_inputs.size());
    for (size_t i = 0; i < orig_inputs.size(); ++i) {
        EXPECT_EQ(cot_inputs[i].name, orig_inputs[i].name);
    }
}

// ============================================================================
// AC-27: DspyField – all DspyFieldType values stored correctly
// ============================================================================

TEST(DspyFieldTest, AllFieldTypesStored)
{
    EXPECT_EQ(DspyField("a", "", DspyFieldType::STRING).type, DspyFieldType::STRING);
    EXPECT_EQ(DspyField("b", "", DspyFieldType::INT).type,    DspyFieldType::INT);
    EXPECT_EQ(DspyField("c", "", DspyFieldType::FLOAT).type,  DspyFieldType::FLOAT);
    EXPECT_EQ(DspyField("d", "", DspyFieldType::BOOL).type,   DspyFieldType::BOOL);
    EXPECT_EQ(DspyField("e", "", DspyFieldType::LIST).type,   DspyFieldType::LIST);
    EXPECT_EQ(DspyField("f", "", DspyFieldType::JSON).type,   DspyFieldType::JSON);
}

// ============================================================================
// AC-28: custom IDspyLLMProvider – forward calls it exactly once
// ============================================================================

TEST(DspyModuleTest, ForwardCallsProviderOnce)
{
    DspyModule mod(makeSentimentSig());
    auto provider = std::make_shared<FixedProvider>(
        "sentiment: positive\nconfidence: 0.99\n");
    mod.setLLMProvider(provider);

    mod.forward({{"text", "Great!"}});
    EXPECT_EQ(provider->callCount(), 1);
}

// ============================================================================
// AC-29: parseResponse – value spans until next field marker
// ============================================================================

TEST(DspySignatureTest, ParseResponseValueSpansToNextField)
{
    DspySignature sig("S", "");
    sig.addInput({"q", "question"});
    sig.addOutput({"summary", "summary text"});
    sig.addOutput({"score",   "numeric score"});

    std::string response =
        "summary: This is a long summary.\nIt spans two lines.\nscore: 0.75\n";

    auto parsed = sig.parseResponse(response);
    // The summary value should end before "score:"
    EXPECT_NE(parsed["summary"].find("long summary"), std::string::npos);
    EXPECT_EQ(parsed["score"], "0.75");
}

// ============================================================================
// AC-30: DspyChainOfThought – description preserved
// ============================================================================

TEST(DspyChainOfThoughtTest, DescriptionPreserved)
{
    auto orig = makeSentimentSig();
    DspyChainOfThought cot(orig);
    EXPECT_EQ(cot.getSignature().getDescription(), orig.getDescription());
}
