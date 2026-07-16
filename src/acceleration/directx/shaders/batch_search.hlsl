// Optimised batch vector search compute shader with groupshared query caching
// ThemisDB DirectX 12 Compute Shader (HLSL cs_5_0 / cs_6_0)
//
// Computes distances from all database vectors to a single query per workgroup,
// using groupshared memory to cache the query vector and avoid redundant global
// memory loads.  Supports L2 (squared), Cosine, and Inner Product metrics.
// DirectX equivalent of src/acceleration/vulkan/shaders/batch_search.comp.
//
// Root signature (matches DirectXVectorBackend::createRootSignature extended):
//   params[0]: UAV root descriptor at u0 — query matrix [numQueries × dim]
//   params[1]: UAV root descriptor at u1 — database matrix [numVectors × dim]
//   params[2]: UAV root descriptor at u2 — output distance matrix [numQueries × numVectors]
//   params[3]: 32-bit inline constants at b0 — { numQueries, numVectors, dim, metricType }
//              metricType: 0 = squared L2, 1 = Cosine, 2 = Inner Product (negated)
//
// Thread layout: [numthreads(256, 1, 1)]
//   One workgroup per query (GID.y == queryIdx); threads cover vector indices.
//   Dispatch: (ceil(numVectors / 256), numQueries, 1)
//
// Note: sharedQuery is sized for dim ≤ 256.  For larger dimensions the shader
//       falls back to direct global reads; extend the constant if needed.

RWStructuredBuffer<float> queries   : register(u0);
RWStructuredBuffer<float> vectors   : register(u1);
RWStructuredBuffer<float> distances : register(u2);

cbuffer Constants : register(b0)
{
    uint numQueries;
    uint numVectors;
    uint dim;
    uint metricType; // 0=squared L2, 1=Cosine, 2=InnerProduct (negated)
};

groupshared float sharedQuery[256];

[numthreads(256, 1, 1)]
void CSMain(uint3 DTid : SV_DispatchThreadID, uint3 GTid : SV_GroupThreadID, uint3 GID : SV_GroupID)
{
    uint queryIdx = GID.y;
    uint localIdx = GTid.x;

    if (queryIdx >= numQueries)
        return;

    uint queryOffset = queryIdx * dim;

    // Load query vector into groupshared memory (coalesced)
    for (uint i = localIdx; i < dim && i < 256u; i += 256u)
    {
        sharedQuery[i] = queries[queryOffset + i];
    }

    GroupMemoryBarrierWithGroupSync();

    uint vectorIdx = DTid.x;
    if (vectorIdx >= numVectors)
        return;

    uint vectorOffset = vectorIdx * dim;

    float result  = 0.0f;
    float normQ   = 0.0f;
    float normV   = 0.0f;

    if (metricType == 0u)
    {
        // Squared L2 distance (no sqrt — matches l2_distance.hlsl behavior for consistency)
        for (uint i = 0u; i < dim; ++i)
        {
            float q = (i < 256u) ? sharedQuery[i] : queries[queryOffset + i];
            float d = q - vectors[vectorOffset + i];
            result += d * d;
        }
    }
    else if (metricType == 1u)
    {
        // Cosine distance = 1 - cosine_similarity
        float dot = 0.0f;
        for (uint i = 0u; i < dim; ++i)
        {
            float q = (i < 256u) ? sharedQuery[i] : queries[queryOffset + i];
            float v = vectors[vectorOffset + i];
            dot   += q * v;
            normQ += q * q;
            normV += v * v;
        }
        float sim = (normQ > 1e-10f && normV > 1e-10f)
            ? dot / (sqrt(normQ) * sqrt(normV))
            : 0.0f;
        result = 1.0f - sim;
    }
    else
    {
        // Inner product distance = -dot_product
        for (uint i = 0u; i < dim; ++i)
        {
            float q = (i < 256u) ? sharedQuery[i] : queries[queryOffset + i];
            result += q * vectors[vectorOffset + i];
        }
        result = -result;
    }

    distances[queryIdx * numVectors + vectorIdx] = result;
}
