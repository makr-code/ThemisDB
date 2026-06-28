// Matrix Multiplication (GEMM) for LoRA Training
// DirectX 12 Compute Shader (HLSL)
// Computes C = A @ B where A: (M, K), B: (K, N), C: (M, N)
// ThemisDB DirectX Compute Shader for LoRA

// Constants
cbuffer Constants : register(b0)
{
    uint M;      // Rows of A and C
    uint K;      // Cols of A, Rows of B
    uint N;      // Cols of B and C
    float alpha; // Scaling factor
};

// Input/Output buffers
StructuredBuffer<float> A : register(t0);
StructuredBuffer<float> B : register(t1);
RWStructuredBuffer<float> C : register(u0);

// Shared memory for tile-based optimization
groupshared float tileA[16][16];
groupshared float tileB[16][16];

[numthreads(16, 16, 1)]
void main(uint3 globalId : SV_DispatchThreadID, uint3 localId : SV_GroupThreadID)
{
    uint row = globalId.y;
    uint col = globalId.x;
    
    // Avoid early return to keep control flow uniform across the threadgroup.
    // Threads outside the valid output range become inactive but still
    // participate in group synchronization calls.
    bool active = (row < M && col < N);
    
    uint localRow = localId.y;
    uint localCol = localId.x;
    
    float sum = 0.0f;
    
    // Tile-based computation
    uint numTiles = (K + 15) / 16;
    
    for (uint tile = 0; tile < numTiles; tile++)
    {
        // Load tile of A into shared memory
        uint aCol = tile * 16 + localCol;
        if (row < M && aCol < K)
        {
            tileA[localRow][localCol] = A[row * K + aCol];
        }
        else
        {
            tileA[localRow][localCol] = 0.0f;
        }
        
        // Load tile of B into shared memory
        uint bRow = tile * 16 + localRow;
        if (bRow < K && col < N)
        {
            tileB[localRow][localCol] = B[bRow * N + col];
        }
        else
        {
            tileB[localRow][localCol] = 0.0f;
        }
        
        // Synchronize to ensure tile is loaded
        GroupMemoryBarrierWithGroupSync();
        
        // Compute partial dot product
        [unroll]
        for (uint k = 0; k < 16; k++)
        {
            sum += tileA[localRow][k] * tileB[k][localCol];
        }
        
        // Synchronize before loading next tile
        GroupMemoryBarrierWithGroupSync();
    }
    
    // Write result with scaling (only active threads write)
    if (active)
    {
        C[row * N + col] = sum * alpha;
    }
}
