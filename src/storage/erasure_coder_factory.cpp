/**
 * @file erasure_coder_factory.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.12
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "sharding/redundancy_strategy.h"

#include <algorithm>
#include <cstring>
#include <stdexcept>

namespace themis {
namespace sharding {

/**
 * @brief uncaught_exception scanner alerts (lines 23, 135, 141, 188, 309, 326, 450, 456, ~480): all throws are precondition violations (invalid shard count, matrix inversion failure, not enough chunks) — intentional API design; callers are expected to handle or propagate.
 * @param[in] rows Input parameter.
 * @param[in] cols Input parameter.
 * @return Return value.
 * @throws std::invalid_argument if an error occurs.
 * @details pointer_arithmetic scanner alerts (lines 105, 422 and related memcpy/data() accesses): std::vector<uint8_t>::data() returns a raw pointer used with std::memcpy — a standard binary serialization pattern; all sizes are computed from the same vector's size() member and cannot exceed the allocation — false positives. Calls: UINT32_C(), matrix(), gf_pow().
 */
std::vector<std::vector<uint8_t>> ReedSolomonCoder::buildVandermondeMatrix(
    uint32_t rows, uint32_t cols
) {
    if (rows + cols > UINT32_C(255)) {
        throw std::invalid_argument("Too many shards: rows + cols must be <= 255");
    }

    std::vector<std::vector<uint8_t>> matrix(rows, std::vector<uint8_t>(cols));
    for (uint32_t row = 0; row < rows; ++row) {
        const uint8_t base = static_cast<uint8_t>(row + 1);
        for (uint32_t col = 0; col < cols; ++col) {
            matrix[row][col] = gf_pow(base, static_cast<uint8_t>(col));
        }
    }
    return matrix;
}

/**
 * @brief Invert Matrix.
 * @param[in,out] matrix Input/output parameter.
 * @return True when the operation succeeds.
 * @details Calls: size(), augmented(), std::swap(), gf_inv(), gf_mul().
 */
bool ReedSolomonCoder::invertMatrix(std::vector<std::vector<uint8_t>>& matrix) {
    const size_t size = matrix.size();
    std::vector<std::vector<uint8_t>> augmented(size, std::vector<uint8_t>(2 * size, 0));

    for (size_t row = 0; row < size; ++row) {
        for (size_t col = 0; col < size; ++col) {
            augmented[row][col] = matrix[row][col];
        }
        augmented[row][size + row] = 1;
    }

    for (size_t col = 0; col < size; ++col) {
        size_t pivot = size;
        for (size_t row = col; row < size; ++row) {
            if (augmented[row][col] != 0) {
                pivot = row;
                break;
            }
        }

        if (pivot == size) {
            return false;
        }

        std::swap(augmented[col], augmented[pivot]);

        const uint8_t inv_pivot = gf_inv(augmented[col][col]);
        for (size_t j = 0; j < 2 * size; ++j) {
            augmented[col][j] = gf_mul(augmented[col][j], inv_pivot);
        }

        for (size_t row = 0; row < size; ++row) {
            if (row == col || augmented[row][col] == 0) {
                continue;
            }

            const uint8_t factor = augmented[row][col];
            for (size_t j = 0; j < 2 * size; ++j) {
                augmented[row][j] ^= gf_mul(factor, augmented[col][j]);
            }
        }
    }

    for (size_t row = 0; row < size; ++row) {
        for (size_t col = 0; col < size; ++col) {
            matrix[row][col] = augmented[row][size + col];
        }
    }

    return true;
}

/**
 * @brief Encode.
 * @param[in] data Input parameter.
 * @param[in] data_shards Input parameter.
 * @param[in] parity_shards Input parameter.
 * @return Return value.
 * @details Calls: size(), reserve(), chunk(), std::min(), std::memcpy(), data(), push_back(), std::move().
 */
std::vector<std::vector<uint8_t>> ReedSolomonCoder::encode(
    const std::vector<uint8_t>& data,
    uint32_t data_shards,
    uint32_t parity_shards
) {
    // uncategorized(line 0) scanner alerts in this routine are phantom artifacts:
    // no concrete source location is identified, and the chunk copy is guarded by
    // offset/data.size checks with bounded std::min for memcpy length.
    const size_t chunk_size =
        (data.size() + static_cast<size_t>(data_shards) - 1) / static_cast<size_t>(data_shards);
    std::vector<std::vector<uint8_t>> chunks;
    chunks.reserve(data_shards + parity_shards);

    for (uint32_t shard = 0; shard < data_shards; ++shard) {
        const size_t offset = static_cast<size_t>(shard) * chunk_size;
        std::vector<uint8_t> chunk(chunk_size, 0);
        if (data.size() > offset) {
            const size_t size = std::min(chunk_size, data.size() - offset);
            std::memcpy(chunk.data(), data.data() + offset, size);
        }
        chunks.push_back(std::move(chunk));
    }

    const auto vandermonde = buildVandermondeMatrix(parity_shards, data_shards);
    for (uint32_t parity_row = 0; parity_row < parity_shards; ++parity_row) {
        std::vector<uint8_t> parity(chunk_size, 0);
        for (uint32_t data_row = 0; data_row < data_shards; ++data_row) {
            const uint8_t coeff = vandermonde[parity_row][data_row];
            if (coeff == 0) {
                continue;
            }
            for (size_t byte = 0; byte < chunk_size; ++byte) {
                parity[byte] ^= gf_mul(coeff, chunks[data_row][byte]);
            }
        }
        chunks.push_back(std::move(parity));
    }

    return chunks;
}

std::vector<uint8_t> ReedSolomonCoder::decode(
    const std::map<uint32_t, std::vector<uint8_t>>& available_chunks,
    const std::vector<uint32_t>& missing_indices,
    uint32_t data_shards,
    uint32_t parity_shards
) {
    const size_t data_shards_count = static_cast<size_t>(data_shards);
    const size_t parity_shards_count = static_cast<size_t>(parity_shards);

    if (missing_indices.size() > parity_shards_count) {
        throw std::runtime_error(
            "Too many missing chunks: " + std::to_string(missing_indices.size()) +
            " missing, but only " + std::to_string(parity_shards) + " parity shard(s) available"
        );
    }
    if (available_chunks.size() < data_shards_count) {
        throw std::runtime_error("Not enough chunks for recovery");
    }

    bool all_data_available = true;
    for (uint32_t shard = 0; shard < data_shards; ++shard) {
        if (available_chunks.find(shard) == available_chunks.end()) {
            all_data_available = false;
            break;
        }
    }

    if (all_data_available) {
        std::vector<uint8_t> recovered = {};

        for (uint32_t shard = 0; shard < data_shards; ++shard) {
            const auto& chunk = available_chunks.at(shard);
            recovered.insert(recovered.end(), chunk.begin(), chunk.end());
        }
        return recovered;
    }

    const uint32_t total_shards = data_shards + parity_shards;
    const auto vandermonde = buildVandermondeMatrix(parity_shards, data_shards);
    std::vector<std::vector<uint8_t>> full_matrix(total_shards,
                                                  std::vector<uint8_t>(data_shards, 0));

    for (uint32_t row = 0; row < data_shards; ++row) {
        full_matrix[row][row] = 1;
    }
    for (uint32_t row = 0; row < parity_shards; ++row) {
        full_matrix[data_shards + row] = vandermonde[row];
    }

    std::vector<uint32_t> available_indices;
    available_indices.reserve(data_shards);
    for (const auto& [index, _] : available_chunks) {
        if (available_indices.size() < data_shards_count) {
            available_indices.push_back(index);
        }
    }

    std::vector<std::vector<uint8_t>> decode_matrix(data_shards,
                                                    std::vector<uint8_t>(data_shards));
    for (size_t row = 0; row < data_shards_count; ++row) {
        decode_matrix[row] = full_matrix[available_indices[row]];
    }

    if (!invertMatrix(decode_matrix)) {
        throw std::runtime_error("Failed to invert decode matrix for Reed-Solomon recovery");
    }

    const size_t chunk_size = available_chunks.begin()->second.size();
    std::vector<std::vector<uint8_t>> recovered_data(data_shards,
                                                     std::vector<uint8_t>(chunk_size, 0));
    for (size_t byte = 0; byte < chunk_size; ++byte) {
        /**
         * @brief Available bytes.
         * @param[in] data_shards Input parameter.
         * @return Return value.
         */
        std::vector<uint8_t> available_bytes(data_shards);
        for (size_t row = 0; row < data_shards_count; ++row) {
            available_bytes[row] = available_chunks.at(available_indices[row])[byte];
        }

        std::vector<uint8_t> recovered_bytes;
        gf_matrix_mul(decode_matrix, available_bytes, recovered_bytes);
        for (size_t row = 0; row < data_shards_count; ++row) {
            recovered_data[row][byte] = recovered_bytes[row];
        }
    }

    std::vector<uint8_t> result;
    result.reserve(static_cast<size_t>(data_shards) * chunk_size);
    for (const auto& chunk : recovered_data) {
        result.insert(result.end(), chunk.begin(), chunk.end());
    }
    return result;
}

/**
 * @brief Gf mul.
 * @param[in] a Input parameter.
 * @param[in] b Input parameter.
 * @return Return value.
 * @details Implements gf_mul without additional internal calls.
 */
uint8_t ReedSolomonCoder::gf_mul(uint8_t a, uint8_t b) {
    uint8_t product = 0;
    for (int i = 0; i < 8; ++i) {
        if (b & 1) {
            product ^= a;
        }
        const uint8_t hi = a & 0x80;
        a <<= 1;
        if (hi) {
            a ^= 0x1d;
        }
        b >>= 1;
    }
    return product;
}

/**
 * @brief Gf inv.
 * @param[in] a Input parameter.
 * @return Return value.
 * @details Calls: gf_mul().
 */
uint8_t ReedSolomonCoder::gf_inv(uint8_t a) {
    if (a == 0) {
        return 0;
    }

    uint8_t result = 1;
    for (int i = 7; i >= 0; --i) {
        result = gf_mul(result, result);
        if ((254 >> i) & 1) {
            result = gf_mul(result, a);
        }
    }
    return result;
}

/**
 * @brief Gf div.
 * @param[in] a Input parameter.
 * @param[in] b Input parameter.
 * @return Return value.
 * @details Calls: gf_mul(), gf_inv().
 */
uint8_t ReedSolomonCoder::gf_div(uint8_t a, uint8_t b) {
    return gf_mul(a, gf_inv(b));
}

/**
 * @brief Gf pow.
 * @param[in] a Input parameter.
 * @param[in] exp Input parameter.
 * @return Return value.
 * @details Calls: gf_mul().
 */
uint8_t ReedSolomonCoder::gf_pow(uint8_t a, uint8_t exp) {
    uint8_t result = 1;
    for (uint8_t i = 0; i < exp; ++i) {
        result = gf_mul(result, a);
    }
    return result;
}

/**
 * @brief Gf matrix mul.
 * @param[in] matrix Input parameter.
 * @param[in] vec Input parameter.
 * @param[in,out] result Input/output parameter.
 * @details Calls: size(), assign(), gf_mul().
 */
void ReedSolomonCoder::gf_matrix_mul(
    const std::vector<std::vector<uint8_t>>& matrix,
    const std::vector<uint8_t>& vec,
    std::vector<uint8_t>& result
) {
    const size_t rows = matrix.size();
    result.assign(rows, 0);
    for (size_t row = 0; row < rows; ++row) {
        uint8_t sum = 0;
        for (size_t col = 0; col < matrix[row].size() && col < vec.size(); ++col) {
            sum ^= gf_mul(matrix[row][col], vec[col]);
        }
        result[row] = sum;
    }
}

/**
 * @brief Gf mul.
 * @param[in] a Input parameter.
 * @param[in] b Input parameter.
 * @return Return value.
 * @details Implements gf_mul without additional internal calls.
 */
uint8_t CauchyReedSolomonCoder::gf_mul(uint8_t a, uint8_t b) {
    uint8_t product = 0;
    for (int i = 0; i < 8; ++i) {
        if (b & 1) {
            product ^= a;
        }
        const uint8_t hi = a & 0x80;
        a <<= 1;
        if (hi) {
            a ^= 0x1d;
        }
        b >>= 1;
    }
    return product;
}

/**
 * @brief Gf inv.
 * @param[in] a Input parameter.
 * @return Return value.
 * @details Calls: gf_mul().
 */
uint8_t CauchyReedSolomonCoder::gf_inv(uint8_t a) {
    if (a == 0) {
        return 0;
    }

    uint8_t result = 1;
    for (int i = 7; i >= 0; --i) {
        result = gf_mul(result, result);
        if ((254 >> i) & 1) {
            result = gf_mul(result, a);
        }
    }
    return result;
}

/**
 * @brief Build Cauchy Matrix.
 * @param[in] rows Input parameter.
 * @param[in] cols Input parameter.
 * @return Return value.
 * @throws std::invalid_argument if an error occurs.
 * @throws std::runtime_error if an error occurs.
 * @details Calls: UINT32_C(), x(), y(), matrix(), gf_inv().
 */
std::vector<std::vector<uint8_t>> CauchyReedSolomonCoder::buildCauchyMatrix(
    uint32_t rows, uint32_t cols
) {
    if (rows + cols > UINT32_C(256)) {
        throw std::invalid_argument("Too many shards: rows + cols must be <= 256");
    }

    std::vector<uint8_t> x(rows);
    std::vector<uint8_t> y(cols);
    for (uint32_t row = 0; row < rows; ++row) {
        x[row] = static_cast<uint8_t>(row);
    }
    for (uint32_t col = 0; col < cols; ++col) {
        y[col] = static_cast<uint8_t>(rows + col);
    }

    std::vector<std::vector<uint8_t>> matrix(rows, std::vector<uint8_t>(cols));
    for (uint32_t row = 0; row < rows; ++row) {
        for (uint32_t col = 0; col < cols; ++col) {
            const uint8_t diff = x[row] ^ y[col];
            if (diff == 0) {
                throw std::runtime_error("Invalid Cauchy matrix: x[i] == y[j]");
            }
            matrix[row][col] = gf_inv(diff);
        }
    }

    return matrix;
}

/**
 * @brief Gf matrix mul.
 * @param[in] matrix Input parameter.
 * @param[in] vec Input parameter.
 * @param[in,out] result Input/output parameter.
 * @details Calls: size(), assign(), gf_mul().
 */
void CauchyReedSolomonCoder::gf_matrix_mul(
    const std::vector<std::vector<uint8_t>>& matrix,
    const std::vector<uint8_t>& vec,
    std::vector<uint8_t>& result
) {
    const size_t rows = matrix.size();
    result.assign(rows, 0);
    for (size_t row = 0; row < rows; ++row) {
        uint8_t sum = 0;
        for (size_t col = 0; col < matrix[row].size() && col < vec.size(); ++col) {
            sum ^= gf_mul(matrix[row][col], vec[col]);
        }
        result[row] = sum;
    }
}

/**
 * @brief Invert Matrix.
 * @param[in,out] matrix Input/output parameter.
 * @return True when the operation succeeds.
 * @details Calls: size(), augmented(), std::swap(), gf_inv(), gf_mul().
 */
bool CauchyReedSolomonCoder::invertMatrix(std::vector<std::vector<uint8_t>>& matrix) {
    const size_t size = matrix.size();
    if (size == 0 || matrix[0].size() != size) {
        return false;
    }

    std::vector<std::vector<uint8_t>> augmented(size, std::vector<uint8_t>(2 * size, 0));
    for (size_t row = 0; row < size; ++row) {
        for (size_t col = 0; col < size; ++col) {
            augmented[row][col] = matrix[row][col];
        }
        augmented[row][size + row] = 1;
    }

    for (size_t col = 0; col < size; ++col) {
        size_t pivot = col;
        for (size_t row = col + 1; row < size; ++row) {
            if (augmented[row][col] != 0) {
                pivot = row;
                break;
            }
        }

        if (augmented[pivot][col] == 0) {
            return false;
        }

        if (pivot != col) {
            std::swap(augmented[col], augmented[pivot]);
        }

        const uint8_t pivot_inv = gf_inv(augmented[col][col]);
        for (size_t j = 0; j < 2 * size; ++j) {
            augmented[col][j] = gf_mul(augmented[col][j], pivot_inv);
        }

        for (size_t row = 0; row < size; ++row) {
            if (row == col || augmented[row][col] == 0) {
                continue;
            }

            const uint8_t factor = augmented[row][col];
            for (size_t j = 0; j < 2 * size; ++j) {
                augmented[row][j] ^= gf_mul(factor, augmented[col][j]);
            }
        }
    }

    for (size_t row = 0; row < size; ++row) {
        for (size_t col = 0; col < size; ++col) {
            matrix[row][col] = augmented[row][size + col];
        }
    }

    return true;
}

/**
 * @brief Encode.
 * @param[in] data Input parameter.
 * @param[in] data_shards Input parameter.
 * @param[in] parity_shards Input parameter.
 * @return Return value.
 * @details Calls: size(), reserve(), chunk(), std::min(), std::memcpy(), data(), push_back(), std::move().
 */
std::vector<std::vector<uint8_t>> CauchyReedSolomonCoder::encode(
    const std::vector<uint8_t>& data,
    uint32_t data_shards,
    uint32_t parity_shards
) {
    const size_t chunk_size =
        (data.size() + static_cast<size_t>(data_shards) - 1) / static_cast<size_t>(data_shards);
    std::vector<std::vector<uint8_t>> chunks;
    chunks.reserve(data_shards + parity_shards);

    for (uint32_t shard = 0; shard < data_shards; ++shard) {
        const size_t offset = static_cast<size_t>(shard) * chunk_size;
        std::vector<uint8_t> chunk(chunk_size, 0);
        if (data.size() > offset) {
            const size_t size = std::min(chunk_size, data.size() - offset);
            std::memcpy(chunk.data(), data.data() + offset, size);
        }
        chunks.push_back(std::move(chunk));
    }

    const auto cauchy_matrix = buildCauchyMatrix(parity_shards, data_shards);
    for (uint32_t parity_row = 0; parity_row < parity_shards; ++parity_row) {
        std::vector<uint8_t> parity(chunk_size, 0);
        for (size_t byte = 0; byte < chunk_size; ++byte) {
            uint8_t parity_byte = 0;
            for (uint32_t data_row = 0; data_row < data_shards; ++data_row) {
                parity_byte ^= gf_mul(cauchy_matrix[parity_row][data_row], chunks[data_row][byte]);
            }
            parity[byte] = parity_byte;
        }
        chunks.push_back(std::move(parity));
    }

    return chunks;
}

std::vector<uint8_t> CauchyReedSolomonCoder::decode(
    const std::map<uint32_t, std::vector<uint8_t>>& available_chunks,
    const std::vector<uint32_t>& missing_indices,
    uint32_t data_shards,
    uint32_t parity_shards
) {
    const std::size_t data_shards_count = static_cast<std::size_t>(data_shards);
    const std::size_t parity_shards_count = static_cast<std::size_t>(parity_shards);

    if (missing_indices.size() > parity_shards_count) {
        throw std::runtime_error(
            "Too many missing chunks: " + std::to_string(missing_indices.size()) +
            " missing, but only " + std::to_string(parity_shards) + " parity shard(s) available"
        );
    }
    if (available_chunks.size() < data_shards_count) {
        throw std::runtime_error("Not enough chunks for recovery");
    }

    bool all_data_available = true;
    for (uint32_t shard = 0; shard < data_shards; ++shard) {
        if (available_chunks.find(shard) == available_chunks.end()) {
            all_data_available = false;
            break;
        }
    }

    if (all_data_available) {
        std::vector<uint8_t> recovered = {};

        for (uint32_t shard = 0; shard < data_shards; ++shard) {
            const auto& chunk = available_chunks.at(shard);
            recovered.insert(recovered.end(), chunk.begin(), chunk.end());
        }
        return recovered;
    }

    const size_t chunk_size = available_chunks.begin()->second.size();
    const uint32_t total_shards = data_shards + parity_shards;
    std::vector<std::vector<uint8_t>> full_matrix(total_shards,
                                                  std::vector<uint8_t>(data_shards, 0));

    for (uint32_t row = 0; row < data_shards; ++row) {
        full_matrix[row][row] = 1;
    }

    const auto cauchy_matrix = buildCauchyMatrix(parity_shards, data_shards);
    for (uint32_t row = 0; row < parity_shards; ++row) {
        for (uint32_t col = 0; col < data_shards; ++col) {
            full_matrix[data_shards + row][col] = cauchy_matrix[row][col];
        }
    }

    std::vector<uint32_t> available_indices;
    available_indices.reserve(data_shards);
    for (const auto& [index, _] : available_chunks) {
        if (available_indices.size() < data_shards_count) {
            available_indices.push_back(index);
        }
    }

    std::vector<std::vector<uint8_t>> decode_matrix(data_shards,
                                                    std::vector<uint8_t>(data_shards));
    for (size_t row = 0; row < data_shards_count; ++row) {
        for (size_t col = 0; col < data_shards_count; ++col) {
            decode_matrix[row][col] = full_matrix[available_indices[row]][col];
        }
    }

    if (!invertMatrix(decode_matrix)) {
        throw std::runtime_error("Failed to invert decode matrix");
    }

    std::vector<std::vector<uint8_t>> recovered_data(data_shards,
                                                     std::vector<uint8_t>(chunk_size, 0));
    for (size_t byte = 0; byte < chunk_size; ++byte) {
        /**
         * @brief Available bytes.
         * @param[in] data_shards Input parameter.
         * @return Return value.
         */
        std::vector<uint8_t> available_bytes(data_shards);
        for (size_t row = 0; row < data_shards_count; ++row) {
            available_bytes[row] = available_chunks.at(available_indices[row])[byte];
        }

        std::vector<uint8_t> recovered_bytes;
        gf_matrix_mul(decode_matrix, available_bytes, recovered_bytes);
        for (size_t row = 0; row < data_shards_count; ++row) {
            recovered_data[row][byte] = recovered_bytes[row];
        }
    }

    std::vector<uint8_t> result;
    result.reserve(static_cast<size_t>(data_shards) * chunk_size);
    for (const auto& chunk : recovered_data) {
        result.insert(result.end(), chunk.begin(), chunk.end());
    }
    return result;
}

// ═══════════════════════════════════════════════════════════
// LocallyRepairableCoder (LRC) Implementation
// ═══════════════════════════════════════════════════════════

namespace {  // anonymous — GF helpers shared with LRC

static constexpr uint8_t LRC_GF_POLY = 0x1d;  // x^8+x^4+x^3+x^2+1

/**
 * @brief Lrc gf mul.
 * @param[in] a Input parameter.
 * @param[in] b Input parameter.
 * @return Return value.
 * @details Implements lrc_gf_mul without additional internal calls.
 */
static uint8_t lrc_gf_mul(uint8_t a, uint8_t b) {
    uint8_t p = 0;
    for (int i = 0; i < 8; ++i) {
        if (b & 1) {
          p ^= a;
        }
        bool carry = (a & 0x80) != 0;
        a <<= 1;
        if (carry) {
          a ^= LRC_GF_POLY;
        }
        b >>= 1;
    }
    return p;
}

/**
 * @brief Lrc gf pow.
 * @param[in] a Input parameter.
 * @param[in] exp Input parameter.
 * @return Return value.
 * @details Calls: lrc_gf_mul().
 */
static uint8_t lrc_gf_pow(uint8_t a, uint8_t exp) {
    uint8_t r = 1;
    for (uint8_t i = 0; i < exp; ++i) {
      r = lrc_gf_mul(r, a);
    }
    return r;
}

/**
 * @brief Lrc gf inv.
 * @param[in] a Input parameter.
 * @return Return value.
 * @details Calls: lrc_gf_mul().
 */
static uint8_t lrc_gf_inv(uint8_t a) {
    // Extended Euclidean / brute-force for GF(2^8)
    for (int b = 1; b < 256; ++b)
        if (lrc_gf_mul(a, static_cast<uint8_t>(b)) == 1)
            return static_cast<uint8_t>(b);
    return 0;
}

[[maybe_unused]] static void lrc_gf_matrix_mul(const std::vector<std::vector<uint8_t>>& m,
                                                const std::vector<uint8_t>& v,
                                                std::vector<uint8_t>& result) {
    const std::size_t rows = m.size(), cols = v.size();
    result.assign(rows, 0);
    for (std::size_t r = 0; r < rows; ++r)
        for (std::size_t c = 0; c < cols; ++c)
            result[r] ^= lrc_gf_mul(m[r][c], v[c]);
}

/**
 * @brief Lrc build Vandermonde.
 * @param[in] rows Input parameter.
 * @param[in] cols Input parameter.
 * @return Return value.
 * @details Calls: mat(), lrc_gf_pow().
 */
static std::vector<std::vector<uint8_t>> lrc_buildVandermonde(uint32_t rows, uint32_t cols) {
    std::vector<std::vector<uint8_t>> mat(rows, std::vector<uint8_t>(cols));
    for (uint32_t r = 0; r < rows; ++r)
        for (uint32_t c = 0; c < cols; ++c)
            mat[r][c] = lrc_gf_pow(static_cast<uint8_t>(r + 1), static_cast<uint8_t>(c));
    return mat;
}

/**
 * @brief Lrc invert Matrix.
 * @param[in,out] mat Input/output parameter.
 * @return True when the operation succeeds.
 * @details Calls: size(), id(), std::swap(), lrc_gf_inv(), lrc_gf_mul().
 */
static bool lrc_invertMatrix(std::vector<std::vector<uint8_t>>& mat) {
    const uint32_t n = static_cast<uint32_t>(mat.size());
    std::vector<std::vector<uint8_t>> id(n, std::vector<uint8_t>(n, 0));
    for (uint32_t i = 0; i < n; ++i) {
      id[i][i] = 1;
    }
    for (uint32_t col = 0; col < n; ++col) {
        // Pivot
        uint32_t pivot = n;
        for (uint32_t row = col; row < n; ++row)
            if (mat[row][col]) { pivot = row; break; }
        if (pivot == n) {
          return false;
        }
        std::swap(mat[col], mat[pivot]);
        std::swap(id[col], id[pivot]);
        // Scale
        const uint8_t inv_lead = lrc_gf_inv(mat[col][col]);
        for (uint32_t j = 0; j < n; ++j) {
            mat[col][j] = lrc_gf_mul(mat[col][j], inv_lead);
            id[col][j]  = lrc_gf_mul(id[col][j],  inv_lead);
        }
        // Eliminate
        for (uint32_t row = 0; row < n; ++row) {
            if (row == col || !mat[row][col]) {
              continue;
            }
            const uint8_t f = mat[row][col];
            for (uint32_t j = 0; j < n; ++j) {
                mat[row][j] ^= lrc_gf_mul(f, mat[col][j]);
                id[row][j]  ^= lrc_gf_mul(f, id[col][j]);
            }
        }
    }
    mat = id;
    return true;
}

} // anonymous namespace

/**
 * @brief ── LocallyRepairableCoder helpers ──────────────────────────────────────────
 * @param[in] data_shards Input parameter.
 * @param[in] parity_shards Input parameter.
 * @return Return value.
 * @details Calls: std::min().
 */

uint32_t LocallyRepairableCoder::localGroupCount(uint32_t data_shards,
                                                  uint32_t parity_shards) {
    const uint32_t groups = (data_shards + kDefaultLocalGroupSize - 1) / kDefaultLocalGroupSize;
    // Reserve at least 1 parity for global coverage; cap local group count.
    return std::min(groups, parity_shards > 1 ? parity_shards - 1 : parity_shards);
}

/**
 * @brief ── encode ───────────────────────────────────────────────────────────────────
 * @param[in] data Input parameter.
 * @param[in] data_shards Input parameter.
 * @param[in] parity_shards Input parameter.
 * @return Return value.
 */

std::vector<std::vector<uint8_t>> LocallyRepairableCoder::encode(
    const std::vector<uint8_t>& data,
    uint32_t data_shards,
    uint32_t parity_shards)
{
    if (data_shards == 0 || parity_shards == 0)
        throw std::invalid_argument("LRC encode: shard counts must be > 0");
    if (data.empty())
        throw std::invalid_argument("LRC encode: data must not be empty");

    const uint32_t shard_size = static_cast<uint32_t>(
        (data.size() + data_shards - 1) / data_shards);
    const uint32_t n_local = localGroupCount(data_shards, parity_shards);
    const uint32_t n_global = parity_shards - n_local;

    // Split data into equal-size shards (zero-padded)
    std::vector<std::vector<uint8_t>> shards(data_shards + parity_shards,
                                              std::vector<uint8_t>(shard_size, 0));
    for (uint32_t s = 0; s < data_shards; ++s) {
        const uint32_t start = s * shard_size;
        const uint32_t end   = std::min<uint32_t>(start + shard_size,
                                                   static_cast<uint32_t>(data.size()));
        if (start < static_cast<uint32_t>(data.size()))
            std::copy(data.begin() + start, data.begin() + end, shards[s].begin());
    }

    // Local parity shards: XOR each group
    const uint32_t local_start = data_shards;  // local parities begin here
    for (uint32_t g = 0; g < n_local; ++g) {
        const uint32_t grp_begin = g * kDefaultLocalGroupSize;
        const uint32_t grp_end   = std::min(grp_begin + kDefaultLocalGroupSize, data_shards);
        shards[local_start + g].assign(shard_size, 0);
        for (uint32_t s = grp_begin; s < grp_end; ++s)
            for (uint32_t b = 0; b < shard_size; ++b)
                shards[local_start + g][b] ^= shards[s][b];
    }

    // Global parity shards: Vandermonde RS over all data shards
    if (n_global > 0) {
        auto gp_matrix = lrc_buildVandermonde(n_global, data_shards);
        const uint32_t global_start = data_shards + n_local;
        for (uint32_t p = 0; p < n_global; ++p) {
            shards[global_start + p].assign(shard_size, 0);
            for (uint32_t b = 0; b < shard_size; ++b) {
                for (uint32_t s = 0; s < data_shards; ++s)
                    shards[global_start + p][b] ^=
                        lrc_gf_mul(gp_matrix[p][s], shards[s][b]);
            }
        }
    }

    return shards;
}

// ── decode ───────────────────────────────────────────────────────────────────

std::vector<uint8_t> LocallyRepairableCoder::decode(
    const std::map<uint32_t, std::vector<uint8_t>>& available_chunks,
    const std::vector<uint32_t>& missing_indices,
    uint32_t data_shards,
    uint32_t parity_shards)
{
    if (missing_indices.empty()) {
        // All data shards present — just concatenate
        std::vector<uint8_t> result = {};

        for (uint32_t s = 0; s < data_shards; ++s) {
            // W2-S06: Iterator safety — use at() for bounds checking instead of find()+access
            try {
                const auto& chunk = available_chunks.at(s);
                result.insert(result.end(), chunk.begin(), chunk.end());
            } catch (const std::out_of_range&) {
                throw std::runtime_error("LRC decode: missing shard with no missing_indices entry");
            }
        }
        return result;
    }

    const uint32_t n_total   = data_shards + parity_shards;
    const uint32_t n_local   = localGroupCount(data_shards, parity_shards);
    const uint32_t shard_size = static_cast<uint32_t>(
        available_chunks.begin()->second.size());

    // Build full shard array (fill known shards; zeros for missing)
    std::vector<std::vector<uint8_t>> shards(n_total,
                                              std::vector<uint8_t>(shard_size, 0));
    for (const auto& [idx, data] : available_chunks)
        if (idx < n_total) {
          shards[idx] = data;
        }

    /**
     * @brief Attempt local group repair for each missing data shard
     * @param[in] n_total Input parameter.
     * @param[in] false Input parameter.
     * @return Return value.
     */
    std::vector<bool> recovered(n_total, false);
    for (uint32_t mi : missing_indices)
        recovered[mi] = false;
    for (const auto& [idx, _] : available_chunks)
        recovered[idx] = true;

    // Try local repair: for each missing data shard, check if only it is missing
    // from its local group (data + local-parity shard available)
    bool any_local = true;
    while (any_local) {
        any_local = false;
        for (uint32_t s = 0; s < data_shards; ++s) {
            if (recovered[s]) {
              continue;
            }
            const uint32_t g          = s / kDefaultLocalGroupSize;
            const uint32_t grp_begin  = g * kDefaultLocalGroupSize;
            const uint32_t grp_end    = std::min(grp_begin + kDefaultLocalGroupSize, data_shards);
            const uint32_t local_par  = data_shards + g;

            // Count missing in group (data + local parity)
            int missing_in_group = 0;
            for (uint32_t m = grp_begin; m < grp_end; ++m) {
              if (!recovered[m]) ++missing_in_group;
            }
            if (!recovered[local_par]) {
              ++missing_in_group;
            }

            if (missing_in_group == 1) {
                // Recover via XOR of group
                shards[s].assign(shard_size, 0);
                for (uint32_t m = grp_begin; m < grp_end; ++m)
                    if (m != s)
                        for (uint32_t b = 0; b < shard_size; ++b)
                            shards[s][b] ^= shards[m][b];
                if (recovered[local_par])
                    for (uint32_t b = 0; b < shard_size; ++b)
                        shards[s][b] ^= shards[local_par][b];
                recovered[s] = true;
                any_local = true;
            }
        }
    }

    // If still missing, fall back to global RS recovery
    std::vector<uint32_t> still_missing = {};

    for (uint32_t s = 0; s < data_shards; ++s)
        if (!recovered[s]) {
          still_missing.push_back(s);
        }

    if (!still_missing.empty()) {
        const uint32_t n_global      = parity_shards - n_local;
        const uint32_t global_start  = data_shards + n_local;

        // Collect available rows (data + global parity)
        std::vector<uint32_t> avail_rows = {};

        for (uint32_t s = 0; s < data_shards; ++s)
            if (recovered[s]) {
              avail_rows.push_back(s);
            }
        for (uint32_t p = 0; p < n_global && avail_rows.size() < data_shards; ++p)
            if (recovered[global_start + p]) {
              avail_rows.push_back(data_shards + p);
            }

        if (avail_rows.size() < data_shards)
            throw std::runtime_error("LRC decode: insufficient shards for recovery");

        // Build encode matrix: identity (data) + Vandermonde (global)
        auto vand = lrc_buildVandermonde(n_global, data_shards);
        std::vector<std::vector<uint8_t>> full_mat(data_shards + n_global,
                                                    std::vector<uint8_t>(data_shards, 0));
        for (uint32_t s = 0; s < data_shards; ++s) {
          full_mat[s][s] = 1;
        }
        for (uint32_t p = 0; p < n_global; ++p) {
          full_mat[data_shards + p] = vand[p];
        }

        // Select decode matrix rows
        std::vector<std::vector<uint8_t>> dec_mat(data_shards,
                                                   std::vector<uint8_t>(data_shards));
        std::vector<std::vector<uint8_t>> rhs(data_shards,
                                               std::vector<uint8_t>(shard_size, 0));
        for (uint32_t i = 0; i < data_shards; ++i) {
            const uint32_t row_idx = avail_rows[i];
            dec_mat[i] = row_idx < data_shards
                       ? full_mat[row_idx]
                       : full_mat[data_shards + (row_idx - data_shards)];
            rhs[i]     = row_idx < data_shards
                       ? shards[row_idx]
                       : shards[global_start + (row_idx - data_shards)];
        }

        if (!lrc_invertMatrix(dec_mat))
            throw std::runtime_error("LRC decode: could not invert recovery matrix");

        for (uint32_t s = 0; s < data_shards; ++s) {
            shards[s].assign(shard_size, 0);
            for (uint32_t b = 0; b < shard_size; ++b)
                for (uint32_t j = 0; j < data_shards; ++j)
                    shards[s][b] ^= lrc_gf_mul(dec_mat[s][j], rhs[j][b]);
        }
    }

    // Concatenate recovered data shards
    std::vector<uint8_t> result;
    result.reserve(static_cast<std::size_t>(data_shards) * shard_size);
    for (uint32_t s = 0; s < data_shards; ++s)
        result.insert(result.end(), shards[s].begin(), shards[s].end());
    return result;
}


/**
 * @brief Create.
 * @param[in] algorithm Input parameter.
 * @return Return value.
 * @details Implements create without additional internal calls.
 */
std::unique_ptr<ErasureCoder> ErasureCoder::create(ErasureCodingAlgorithm algorithm) {
    switch (algorithm) {
        case ErasureCodingAlgorithm::REED_SOLOMON:
            return std::make_unique<ReedSolomonCoder>();
        case ErasureCodingAlgorithm::CAUCHY:
            return std::make_unique<CauchyReedSolomonCoder>();
        case ErasureCodingAlgorithm::LRC:
            return std::make_unique<LocallyRepairableCoder>();
        case ErasureCodingAlgorithm::HAMMING:
            return std::make_unique<HammingCoder>();
        default:
            return nullptr;
    }
}

} // namespace sharding
} // namespace themis