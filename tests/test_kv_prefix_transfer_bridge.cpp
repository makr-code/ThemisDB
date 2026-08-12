#include <gtest/gtest.h>

#include "llm/kv_prefix_transfer_manager.h"
#include <stdexcept>

using namespace themis::llm;

TEST(KVPrefixTransferBridgeTest, NullSerializerCallbackBridge) {
    NullKVStateSerializer::setSerialiseFn(
        [](const std::string& prefix_text, const std::string&) -> std::vector<std::uint8_t> {
            return std::vector<std::uint8_t>(prefix_text.begin(), prefix_text.end());
        });
    NullKVStateSerializer::setModelFingerprintFn(
        [](const std::string& model_id) { return std::string("bridge:") + model_id; });

    NullKVStateSerializer serializer;
    auto payload = serializer.serialise("abc", "m1");
    ASSERT_EQ(payload.size(), 3u);
    EXPECT_EQ(payload[0], static_cast<std::uint8_t>('a'));
    EXPECT_EQ(serializer.modelFingerprint("m1"), "bridge:m1");

    NullKVStateSerializer::setSerialiseFn({});
    NullKVStateSerializer::setModelFingerprintFn({});
}

TEST(KVPrefixTransferBridgeTest, NullSerializerCallbackExceptionFailClosed) {
    NullKVStateSerializer::setSerialiseFn(
        [](const std::string&, const std::string&) -> std::vector<std::uint8_t> {
            throw std::runtime_error("serialise failed");
        });
    NullKVStateSerializer::setModelFingerprintFn(
        [](const std::string&) -> std::string {
            throw std::runtime_error("fingerprint failed");
        });

    NullKVStateSerializer serializer;
    const auto payload = serializer.serialise("abc", "m1");
    ASSERT_EQ(payload.size(), 3u);
    EXPECT_EQ(payload[0], static_cast<std::uint8_t>('a'));
    EXPECT_EQ(serializer.modelFingerprint("m1"), "null:m1");

    NullKVStateSerializer::setSerialiseFn({});
    NullKVStateSerializer::setModelFingerprintFn({});
}

TEST(KVPrefixTransferBridgeTest, DefaultSerializerFactorySetterAcceptsBridge) {
    KVPrefixTransferManager::setDefaultSerializerFactory(
        []() -> std::unique_ptr<IKVStateSerializer> {
            return std::make_unique<NullKVStateSerializer>();
        });
    KVPrefixTransferManager::setDefaultSerializerFactory({});
    SUCCEED();
}
