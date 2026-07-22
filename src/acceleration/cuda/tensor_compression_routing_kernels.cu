#include <cuda_runtime.h>

#include <cstdint>

namespace {

__global__ void compressToInt8Kernel(const float* input, int8_t* output, int n, float scale) {
    const int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= n) {
        return;
    }

    float q = input[i] / scale;
    q = roundf(q);
    q = fminf(127.0f, fmaxf(-127.0f, q));
    output[i] = static_cast<int8_t>(q);
}

__global__ void routingScoresKernel(const float* tensor,
                                    const float* route_weights,
                                    float* scores,
                                    int num_routes,
                                    int dim) {
    const int route = blockIdx.x;
    if (route >= num_routes) {
        return;
    }

    float dot = 0.0f;
    const float* row = route_weights + static_cast<std::size_t>(route) * dim;
    for (int i = threadIdx.x; i < dim; i += blockDim.x) {
        dot += tensor[i] * row[i];
    }

    __shared__ float block_sums[256];
    block_sums[threadIdx.x] = dot;
    __syncthreads();

    for (int stride = blockDim.x / 2; stride > 0; stride >>= 1) {
        if (threadIdx.x < stride) {
            block_sums[threadIdx.x] += block_sums[threadIdx.x + stride];
        }
        __syncthreads();
    }

    if (threadIdx.x == 0) {
        scores[route] = block_sums[0];
    }
}

}  // namespace

extern "C" int themisCudaCompressToInt8Host(const float* input, int8_t* output, int n, float scale) {
    if (!input || !output || n <= 0 || scale <= 0.0f) {
        return static_cast<int>(cudaErrorInvalidValue);
    }

    float* d_input = nullptr;
    int8_t* d_output = nullptr;

    cudaError_t err = cudaMalloc(&d_input, static_cast<std::size_t>(n) * sizeof(float));
    if (err != cudaSuccess) {
        return static_cast<int>(err);
    }

    err = cudaMalloc(&d_output, static_cast<std::size_t>(n) * sizeof(int8_t));
    if (err != cudaSuccess) {
        cudaFree(d_input);
        return static_cast<int>(err);
    }

    err = cudaMemcpy(d_input, input, static_cast<std::size_t>(n) * sizeof(float), cudaMemcpyHostToDevice);
    if (err == cudaSuccess) {
        const int block = 256;
        const int grid = (n + block - 1) / block;
        compressToInt8Kernel<<<grid, block>>>(d_input, d_output, n, scale);
        err = cudaGetLastError();
    }

    if (err == cudaSuccess) {
        err = cudaMemcpy(output, d_output, static_cast<std::size_t>(n) * sizeof(int8_t), cudaMemcpyDeviceToHost);
    }

    cudaFree(d_output);
    cudaFree(d_input);
    return static_cast<int>(err);
}

extern "C" int themisCudaComputeRoutingScoresHost(const float* tensor,
                                                    const float* route_weights,
                                                    float* scores,
                                                    int num_routes,
                                                    int dim) {
    if (!tensor || !route_weights || !scores || num_routes <= 0 || dim <= 0) {
        return static_cast<int>(cudaErrorInvalidValue);
    }

    float* d_tensor = nullptr;
    float* d_routes = nullptr;
    float* d_scores = nullptr;

    const std::size_t tensor_bytes = static_cast<std::size_t>(dim) * sizeof(float);
    const std::size_t routes_bytes = static_cast<std::size_t>(num_routes) * static_cast<std::size_t>(dim) * sizeof(float);
    const std::size_t scores_bytes = static_cast<std::size_t>(num_routes) * sizeof(float);

    cudaError_t err = cudaMalloc(&d_tensor, tensor_bytes);
    if (err != cudaSuccess) {
        return static_cast<int>(err);
    }
    err = cudaMalloc(&d_routes, routes_bytes);
    if (err != cudaSuccess) {
        cudaFree(d_tensor);
        return static_cast<int>(err);
    }
    err = cudaMalloc(&d_scores, scores_bytes);
    if (err != cudaSuccess) {
        cudaFree(d_routes);
        cudaFree(d_tensor);
        return static_cast<int>(err);
    }

    err = cudaMemcpy(d_tensor, tensor, tensor_bytes, cudaMemcpyHostToDevice);
    if (err == cudaSuccess) {
        err = cudaMemcpy(d_routes, route_weights, routes_bytes, cudaMemcpyHostToDevice);
    }

    if (err == cudaSuccess) {
        routingScoresKernel<<<num_routes, 256>>>(d_tensor, d_routes, d_scores, num_routes, dim);
        err = cudaGetLastError();
    }

    if (err == cudaSuccess) {
        err = cudaMemcpy(scores, d_scores, scores_bytes, cudaMemcpyDeviceToHost);
    }

    cudaFree(d_scores);
    cudaFree(d_routes);
    cudaFree(d_tensor);
    return static_cast<int>(err);
}
