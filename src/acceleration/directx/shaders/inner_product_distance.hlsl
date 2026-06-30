// Inner product distance compute shader for vector similarity search
// ThemisDB DirectX 12 Compute Shader (HLSL cs_5_0 / cs_6_0)
//
// Computes inner product distance (-dot_product) between all (query, vector) pairs.
// Stores the negative dot product so that lower distance = higher similarity,
// consistent with the Vulkan and CUDA backends.
//
// Root signature (matches DirectXVectorBackend::createRootSignature):
//   params[0]: UAV root descriptor at u0 — query matrix [numQueries × dim]
//   params[1]: UAV root descriptor at u1 — database matrix [numVectors × dim]
//   params[2]: UAV root descriptor at u2 — output distance matrix [numQueries × numVectors]
//   params[3]: 32-bit inline constants at b0 — { numQueries, numVectors, dim, padding }
//
// Thread layout: [numthreads(16, 16, 1)]
//   DTid.x → vector index
//   DTid.y → query index
//   Dispatch: (ceil(numVectors/16), ceil(numQueries/16), 1)

RWStructuredBuffer<float> queries   : register(u0);
RWStructuredBuffer<float> vectors   : register(u1);
RWStructuredBuffer<float> distances : register(u2);

cbuffer Constants : register(b0)
{
    uint numQueries;
    uint numVectors;
    uint dim;
    uint padding;
};

[numthreads(16, 16, 1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
    uint qIdx = DTid.y;
    uint vIdx = DTid.x;
    if (qIdx >= numQueries || vIdx >= numVectors)
        return;

    float dot = 0.0f;
    uint qOff = qIdx * dim;
    uint vOff = vIdx * dim;
    for (uint i = 0; i < dim; ++i)
    {
        dot += queries[qOff + i] * vectors[vOff + i];
    }
    // Negate: lower distance value → higher similarity
    distances[qIdx * numVectors + vIdx] = -dot;
}
