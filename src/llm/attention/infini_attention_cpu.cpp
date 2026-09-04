/**
 * @file infini_attention_cpu.cpp
 * @brief Infini-attention CPU fallback implementation (Phase 1 PoC).
 * @version 0.1.0-alpha
 * @note Maturity: EXPERIMENTAL
 * @note Status: Phase 1 CPU fallback for INFINI_COMPRESSIVE backend
 */

#include "llm/infini_attention_cpu.h"

#include <algorithm>
#include <cmath>

namespace themis::llm::attention {

InfiniAttentionCPU::InfiniAttentionCPU(const Config& config)
    : config_(config) {}

bool InfiniAttentionCPU::initialize() {
    if (initialized_) {
        return true;
    }

    try {
        if (config_.use_fp64) {
            memory_matrix_fp64_ =
                Eigen::MatrixXd::Zero(config_.hidden_dim, config_.memory_size);
            norm_vector_fp64_.resize(config_.memory_size, 0.0);
            temp_kv_product_fp64_ =
                Eigen::MatrixXd::Zero(config_.hidden_dim, config_.memory_size);
        } else {
            memory_matrix_ =
                Eigen::MatrixXf::Zero(config_.hidden_dim, config_.memory_size);
            norm_vector_.resize(config_.memory_size, 0.0f);
            temp_kv_product_ =
                Eigen::MatrixXf::Zero(config_.hidden_dim, config_.memory_size);
        }
        initialized_ = true;
        return true;
    } catch (...) {
        return false;
    }
}

bool InfiniAttentionCPU::forward(const Eigen::MatrixXf& Q,
                                 const Eigen::MatrixXf& K,
                                 const Eigen::MatrixXf& V,
                                 Eigen::MatrixXf& output) {
    if (!initialized_ || config_.use_fp64) {
        return false;
    }

    return computeAttentionFP32(Q, K, V, output);
}

bool InfiniAttentionCPU::forward64(const Eigen::MatrixXd& Q,
                                   const Eigen::MatrixXd& K,
                                   const Eigen::MatrixXd& V,
                                   Eigen::MatrixXd& output) {
    if (!initialized_ || !config_.use_fp64) {
        return false;
    }

    return computeAttentionFP64(Q, K, V, output);
}

bool InfiniAttentionCPU::computeAttentionFP32(const Eigen::MatrixXf& Q,
                                               const Eigen::MatrixXf& K,
                                               const Eigen::MatrixXf& V,
                                               Eigen::MatrixXf& output) {
    if (Q.rows() != config_.seq_len || Q.cols() != config_.hidden_dim ||
        K.rows() != config_.seq_len || K.cols() != config_.hidden_dim ||
        V.rows() != config_.seq_len || V.cols() != config_.hidden_dim) {
        return false;  // Shape mismatch
    }

    try {
        // 1. Update compressive memory: M += K^T · V
        updateMemory(K, V);

        // 2. Compute attention output
        // output_i = (M^T · Q_i) / (M^T · 1)
        output = Eigen::MatrixXf::Zero(config_.seq_len, config_.hidden_dim);

        for (int i = 0; i < config_.seq_len; ++i) {
            // Compute compressive attention: M^T · Q_i
            Eigen::VectorXf q_i = Q.row(i).transpose();
            Eigen::VectorXf compressed_attention =
                memory_matrix_.transpose() * q_i;

            // Compute normalization
            float norm_sum = 0.0f;
            for (int j = 0; j < config_.memory_size; ++j) {
                norm_sum += norm_vector_[j] + config_.epsilon;
            }

            // Normalize and accumulate
            for (int j = 0; j < config_.hidden_dim; ++j) {
                output(i, j) = compressed_attention.sum() / (norm_sum + config_.epsilon);
            }
        }

        return true;
    } catch (...) {
        return false;
    }
}

bool InfiniAttentionCPU::computeAttentionFP64(const Eigen::MatrixXd& Q,
                                               const Eigen::MatrixXd& K,
                                               const Eigen::MatrixXd& V,
                                               Eigen::MatrixXd& output) {
    if (Q.rows() != config_.seq_len || Q.cols() != config_.hidden_dim ||
        K.rows() != config_.seq_len || K.cols() != config_.hidden_dim ||
        V.rows() != config_.seq_len || V.cols() != config_.hidden_dim) {
        return false;  // Shape mismatch
    }

    try {
        // 1. Update compressive memory: M += K^T · V
        updateMemoryFP64(K, V);

        // 2. Compute attention output
        output = Eigen::MatrixXd::Zero(config_.seq_len, config_.hidden_dim);

        for (int i = 0; i < config_.seq_len; ++i) {
            // Compute compressive attention: M^T · Q_i
            Eigen::VectorXd q_i = Q.row(i).transpose();
            Eigen::VectorXd compressed_attention =
                memory_matrix_fp64_.transpose() * q_i;

            // Compute normalization
            double norm_sum = 0.0;
            for (int j = 0; j < config_.memory_size; ++j) {
                norm_sum += norm_vector_fp64_[j] + config_.epsilon;
            }

            // Normalize and accumulate
            for (int j = 0; j < config_.hidden_dim; ++j) {
                output(i, j) = compressed_attention.sum() / (norm_sum + config_.epsilon);
            }
        }

        return true;
    } catch (...) {
        return false;
    }
}

void InfiniAttentionCPU::updateMemory(const Eigen::MatrixXf& K,
                                       const Eigen::MatrixXf& V) {
    // M += K^T · V
    // K: (seq_len, hidden_dim), V: (seq_len, hidden_dim)
    // K^T: (hidden_dim, seq_len), K^T · V: (hidden_dim, hidden_dim)

    Eigen::MatrixXf kv_product = K.transpose() * V;

    // Accumulate into memory (with overflow protection)
    for (int i = 0; i < config_.hidden_dim && i < config_.memory_size; ++i) {
        for (int j = 0; j < config_.hidden_dim && j < config_.memory_size; ++j) {
            memory_matrix_(i, j) += kv_product(i, j);
        }
    }

    // Update normalization vector
    for (int i = 0; i < config_.memory_size; ++i) {
        norm_vector_[i] += static_cast<float>(K.rows());
    }
}

void InfiniAttentionCPU::updateMemoryFP64(const Eigen::MatrixXd& K,
                                           const Eigen::MatrixXd& V) {
    // M += K^T · V (FP64 version)
    Eigen::MatrixXd kv_product = K.transpose() * V;

    for (int i = 0; i < config_.hidden_dim && i < config_.memory_size; ++i) {
        for (int j = 0; j < config_.hidden_dim && j < config_.memory_size; ++j) {
            memory_matrix_fp64_(i, j) += kv_product(i, j);
        }
    }

    // Update normalization vector
    for (int i = 0; i < config_.memory_size; ++i) {
        norm_vector_fp64_[i] += static_cast<double>(K.rows());
    }
}

std::vector<float> InfiniAttentionCPU::getMemorySnapshot() const {
    std::vector<float> snapshot;

    if (config_.use_fp64) {
        // Convert FP64 to FP32 for serialization
        snapshot.resize(config_.hidden_dim * config_.memory_size);
        for (int i = 0; i < config_.hidden_dim; ++i) {
            for (int j = 0; j < config_.memory_size; ++j) {
                snapshot[i * config_.memory_size + j] =
                    static_cast<float>(memory_matrix_fp64_(i, j));
            }
        }
    } else {
        snapshot.resize(config_.hidden_dim * config_.memory_size);
        Eigen::Map<Eigen::MatrixXf>(snapshot.data(), config_.hidden_dim,
                                    config_.memory_size) = memory_matrix_;
    }

    return snapshot;
}

bool InfiniAttentionCPU::restoreMemory(const std::vector<float>& snapshot) {
    if (static_cast<int>(snapshot.size()) != static_cast<size_t>(config_.hidden_dim) *
                               config_.memory_size) {
        return false;
    }

    try {
        if (config_.use_fp64) {
            for (int i = 0; i < config_.hidden_dim; ++i) {
                for (int j = 0; j < config_.memory_size; ++j) {
                    memory_matrix_fp64_(i, j) =
                        static_cast<double>(snapshot[i * config_.memory_size + j]);
                }
            }
        } else {
            Eigen::Map<Eigen::MatrixXf>(
                const_cast<float*>(snapshot.data()), config_.hidden_dim,
                config_.memory_size) = memory_matrix_;
        }
        return true;
    } catch (...) {
        return false;
    }
}

void InfiniAttentionCPU::resetMemory() {
    if (config_.use_fp64) {
        memory_matrix_fp64_.setZero();
        std::fill(norm_vector_fp64_.begin(), norm_vector_fp64_.end(), 0.0);
    } else {
        memory_matrix_.setZero();
        std::fill(norm_vector_.begin(), norm_vector_.end(), 0.0f);
    }
}

InfiniAttentionCPU::MemStats InfiniAttentionCPU::getMemoryStats() const {
    MemStats stats;
    stats.memory_matrix_bytes = config_.hidden_dim * config_.memory_size *
                                (config_.use_fp64 ? sizeof(double) : sizeof(float));
    stats.temp_buffer_bytes =
        config_.hidden_dim * config_.memory_size *
            (config_.use_fp64 ? sizeof(double) : sizeof(float)) +
        config_.memory_size * (config_.use_fp64 ? sizeof(double) : sizeof(float));
    stats.total_bytes = stats.memory_matrix_bytes + stats.temp_buffer_bytes;
    return stats;
}

}  // namespace themis::llm::attention

