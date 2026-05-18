// THEMIS_GAP_STATS: gaps=1 unimpl=0 stub=1 mock=0 sim=0 todo=0 debt=0 scanned=2026-05-18
#include "llm/lora_framework/vulkan_kernels.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace themis {
namespace lora {
namespace vulkan {

namespace {

bool g_vulkan_sim_available = true;

inline void validate_ptr(const void* ptr, const char* name) {
    if (!ptr) {
        throw std::invalid_argument(std::string("null pointer: ") + name);
    }
}

} // namespace

// STUB/SIMULATION NOTE:
// Purpose: Provide a build-safe Vulkan-kernel API implementation in modular
//          builds where full Vulkan compute kernels are not compiled.
// Activation: Compiled when THEMIS_ENABLE_VULKAN is enabled in modular LLM build.
// Production Delta: All operations run on CPU memory, no Vulkan dispatch occurs.
// Removal Plan: Replace with full Vulkan kernel dispatch once
//               vulkan_kernels.cpp is compile-safe and reintegrated.
bool initialize_vulkan_lora(int /*device_id*/) {
    g_vulkan_sim_available = true;
    return true;
}

void cleanup_vulkan_lora() {
    g_vulkan_sim_available = false;
}

bool is_vulkan_available() {
    return g_vulkan_sim_available;
}

void launch_matmul_shader(
    const float* A,
    const float* B,
    float* C,
    int M,
    int N,
    int K,
    float alpha) {
    validate_ptr(A, "A");
    validate_ptr(B, "B");
    validate_ptr(C, "C");
    if (M <= 0 || N <= 0 || K <= 0) {
        throw std::invalid_argument("launch_matmul_shader received invalid dimensions");
    }

    for (int m = 0; m < M; ++m) {
        for (int n = 0; n < N; ++n) {
            float acc = 0.0f;
            for (int k = 0; k < K; ++k) {
                acc += A[static_cast<size_t>(m) * static_cast<size_t>(K) + static_cast<size_t>(k)] *
                       B[static_cast<size_t>(k) * static_cast<size_t>(N) + static_cast<size_t>(n)];
            }
            C[static_cast<size_t>(m) * static_cast<size_t>(N) + static_cast<size_t>(n)] = alpha * acc;
        }
    }
}

void launch_add_shader(const float* A, const float* B, float* C, size_t size) {
    validate_ptr(A, "A");
    validate_ptr(B, "B");
    validate_ptr(C, "C");
    for (size_t i = 0; i < size; ++i) {
        C[i] = A[i] + B[i];
    }
}

void launch_multiply_shader(const float* A, const float* B, float* C, size_t size) {
    validate_ptr(A, "A");
    validate_ptr(B, "B");
    validate_ptr(C, "C");
    for (size_t i = 0; i < size; ++i) {
        C[i] = A[i] * B[i];
    }
}

void launch_scalar_multiply_shader(const float* A, float* B, float scalar, size_t size) {
    validate_ptr(A, "A");
    validate_ptr(B, "B");
    for (size_t i = 0; i < size; ++i) {
        B[i] = A[i] * scalar;
    }
}

void launch_transpose_shader(const float* input, float* output, int rows, int cols) {
    validate_ptr(input, "input");
    validate_ptr(output, "output");
    if (rows <= 0 || cols <= 0) {
        throw std::invalid_argument("launch_transpose_shader received invalid dimensions");
    }

    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            output[static_cast<size_t>(c) * static_cast<size_t>(rows) + static_cast<size_t>(r)] =
                input[static_cast<size_t>(r) * static_cast<size_t>(cols) + static_cast<size_t>(c)];
        }
    }
}

void launch_lora_grad_A_shader(
    const float* h,
    const float* grad_output,
    float* grad_A,
    int M,
    int K,
    int N,
    float scaling) {
    validate_ptr(h, "h");
    validate_ptr(grad_output, "grad_output");
    validate_ptr(grad_A, "grad_A");
    if (M <= 0 || K <= 0 || N <= 0) {
        throw std::invalid_argument("launch_lora_grad_A_shader received invalid dimensions");
    }

    for (int k = 0; k < K; ++k) {
        for (int n = 0; n < N; ++n) {
            float acc = 0.0f;
            for (int m = 0; m < M; ++m) {
                acc += h[static_cast<size_t>(m) * static_cast<size_t>(K) + static_cast<size_t>(k)] *
                       grad_output[static_cast<size_t>(m) * static_cast<size_t>(N) + static_cast<size_t>(n)];
            }
            grad_A[static_cast<size_t>(k) * static_cast<size_t>(N) + static_cast<size_t>(n)] = acc * scaling;
        }
    }
}

void launch_lora_grad_B_shader(
    const float* input,
    const float* grad_h,
    float* grad_B,
    int M,
    int D,
    int K) {
    validate_ptr(input, "input");
    validate_ptr(grad_h, "grad_h");
    validate_ptr(grad_B, "grad_B");
    if (M <= 0 || D <= 0 || K <= 0) {
        throw std::invalid_argument("launch_lora_grad_B_shader received invalid dimensions");
    }

    for (int d = 0; d < D; ++d) {
        for (int k = 0; k < K; ++k) {
            float acc = 0.0f;
            for (int m = 0; m < M; ++m) {
                acc += input[static_cast<size_t>(m) * static_cast<size_t>(D) + static_cast<size_t>(d)] *
                       grad_h[static_cast<size_t>(m) * static_cast<size_t>(K) + static_cast<size_t>(k)];
            }
            grad_B[static_cast<size_t>(d) * static_cast<size_t>(K) + static_cast<size_t>(k)] = acc;
        }
    }
}

void launch_embedding_lookup_shader(
    float* output,
    const float* token_ids,
    const float* embedding_weights,
    int batch_size,
    int seq_len,
    int hidden_dim,
    int vocab_size) {
    validate_ptr(output, "output");
    validate_ptr(token_ids, "token_ids");
    validate_ptr(embedding_weights, "embedding_weights");
    if (batch_size <= 0 || seq_len <= 0 || hidden_dim <= 0 || vocab_size <= 0) {
        throw std::invalid_argument("launch_embedding_lookup_shader received invalid dimensions");
    }

    const size_t total_tokens = static_cast<size_t>(batch_size) * static_cast<size_t>(seq_len);
    for (size_t token_idx = 0; token_idx < total_tokens; ++token_idx) {
        const int raw_token = static_cast<int>(std::lround(token_ids[token_idx]));
        const int clamped_token = std::clamp(raw_token, 0, vocab_size - 1);

        const size_t src_offset = static_cast<size_t>(clamped_token) * static_cast<size_t>(hidden_dim);
        const size_t dst_offset = token_idx * static_cast<size_t>(hidden_dim);
        for (int h = 0; h < hidden_dim; ++h) {
            output[dst_offset + static_cast<size_t>(h)] =
                embedding_weights[src_offset + static_cast<size_t>(h)];
        }
    }
}

void launch_sequence_mean_shader(
    float* output,
    const float* input,
    int batch_size,
    int seq_len,
    int hidden_dim) {
    validate_ptr(output, "output");
    validate_ptr(input, "input");
    if (batch_size <= 0 || seq_len <= 0 || hidden_dim <= 0) {
        throw std::invalid_argument("launch_sequence_mean_shader received invalid dimensions");
    }

    const float inv_seq = 1.0f / static_cast<float>(seq_len);
    for (int b = 0; b < batch_size; ++b) {
        for (int h = 0; h < hidden_dim; ++h) {
            float acc = 0.0f;
            for (int s = 0; s < seq_len; ++s) {
                const size_t idx = (static_cast<size_t>(b) * static_cast<size_t>(seq_len) + static_cast<size_t>(s)) *
                                       static_cast<size_t>(hidden_dim) +
                                   static_cast<size_t>(h);
                acc += input[idx];
            }
            output[static_cast<size_t>(b) * static_cast<size_t>(hidden_dim) + static_cast<size_t>(h)] =
                acc * inv_seq;
        }
    }
}

void launch_fused_lora_forward(
    const float* input,
    const float* B,
    const float* A,
    float* output,
    size_t batch_size,
    size_t in_dim,
    size_t rank,
    size_t out_dim,
    float scaling) {
    validate_ptr(input, "input");
    validate_ptr(B, "B");
    validate_ptr(A, "A");
    validate_ptr(output, "output");

    for (size_t b = 0; b < batch_size; ++b) {
        for (size_t o = 0; o < out_dim; ++o) {
            float acc = 0.0f;
            for (size_t r = 0; r < rank; ++r) {
                float h_val = 0.0f;
                for (size_t i = 0; i < in_dim; ++i) {
                    h_val += input[b * in_dim + i] * B[i * rank + r];
                }
                acc += h_val * A[r * out_dim + o];
            }
            output[b * out_dim + o] = acc * scaling;
        }
    }
}

void launch_fused_lora_backward(
    const float* input,
    const float* B,
    const float* A,
    const float* grad_output,
    float* grad_A,
    float* grad_B,
    float* grad_input,
    size_t batch_size,
    size_t in_dim,
    size_t rank,
    size_t out_dim,
    float scaling) {
    validate_ptr(input, "input");
    validate_ptr(B, "B");
    validate_ptr(A, "A");
    validate_ptr(grad_output, "grad_output");
    validate_ptr(grad_A, "grad_A");
    validate_ptr(grad_B, "grad_B");
    validate_ptr(grad_input, "grad_input");

    std::fill(grad_A, grad_A + rank * out_dim, 0.0f);
    std::fill(grad_B, grad_B + in_dim * rank, 0.0f);
    std::fill(grad_input, grad_input + batch_size * in_dim, 0.0f);

    for (size_t b = 0; b < batch_size; ++b) {
        for (size_t r = 0; r < rank; ++r) {
            float h_val = 0.0f;
            for (size_t i = 0; i < in_dim; ++i) {
                h_val += input[b * in_dim + i] * B[i * rank + r];
            }
            for (size_t o = 0; o < out_dim; ++o) {
                grad_A[r * out_dim + o] += h_val * grad_output[b * out_dim + o] * scaling;
            }
        }
    }

    for (size_t b = 0; b < batch_size; ++b) {
        for (size_t r = 0; r < rank; ++r) {
            float grad_h = 0.0f;
            for (size_t o = 0; o < out_dim; ++o) {
                grad_h += grad_output[b * out_dim + o] * A[r * out_dim + o];
            }
            for (size_t i = 0; i < in_dim; ++i) {
                grad_B[i * rank + r] += input[b * in_dim + i] * grad_h * scaling;
                grad_input[b * in_dim + i] += grad_h * B[i * rank + r] * scaling;
            }
        }
    }
}

} // namespace vulkan
} // namespace lora
} // namespace themis
