#include <gtest/gtest.h>

#include "llm/lora_framework/lora_layers.h"

using namespace themis::llm::lora;

TEST(LoRAGradientLifecycle, AllocatesGradientsOnFirstBackward) {
    LoRALayer layer(4, 4, 2);
    auto params = layer.parameters();
    ASSERT_EQ(params.size(), 2u);
    ASSERT_NE(params[0], nullptr);
    ASSERT_NE(params[1], nullptr);
    EXPECT_FALSE(params[0]->grad);
    EXPECT_FALSE(params[1]->grad);

    Tensor input({1, 4}, 0.5f);
    Tensor grad_output({1, 4}, 1.0f);
    const Tensor output = layer.forward(input);
    const Tensor grad_input = layer.backward(grad_output);

    EXPECT_EQ(output.shape(), (std::vector<size_t>{1, 4}));
    EXPECT_EQ(grad_input.shape(), (std::vector<size_t>{1, 4}));
    EXPECT_TRUE(params[0]->grad);
    EXPECT_TRUE(params[1]->grad);
}
