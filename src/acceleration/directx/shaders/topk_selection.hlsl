// Top-K selection compute shader for nearest neighbor search
// ThemisDB DirectX 12 Compute Shader (HLSL cs_5_0 / cs_6_0)
//
// Selects the k nearest neighbors for each query from a pre-computed distance matrix
// using iterative selection with groupshared memory reduction.
// DirectX equivalent of src/acceleration/vulkan/shaders/topk_selection.comp.
//
// Root signature (dedicated; requires 3-UAV + constants layout):
//   params[0]: UAV root descriptor at u0 — distance matrix [numQueries × numVectors]
//   params[1]: UAV root descriptor at u1 — output top-k distances [numQueries × k]
//   params[2]: UAV root descriptor at u2 — output top-k indices   [numQueries × k]
//   params[3]: 32-bit inline constants at b0 — { numQueries, numVectors, k, padding }
//
// Thread layout: [numthreads(256, 1, 1)]
//   One workgroup per query (GroupID.x == queryIdx).
//   Dispatch: (numQueries, 1, 1)

RWStructuredBuffer<float> distances    : register(u0);
RWStructuredBuffer<float> topKDists    : register(u1);
RWStructuredBuffer<uint>  topKIndices  : register(u2);

cbuffer Constants : register(b0)
{
    uint numQueries;
    uint numVectors;
    uint k;
    uint padding;
};

static const float kMaxFloat = 1e38f;

groupshared float sharedDistances[256];
groupshared uint  sharedIndices[256];

[numthreads(256, 1, 1)]
void CSMain(uint3 DTid : SV_DispatchThreadID, uint3 GTid : SV_GroupThreadID, uint3 GID : SV_GroupID)
{
    uint queryIdx     = GID.x;
    uint localIdx     = GTid.x;

    if (queryIdx >= numQueries)
        return;

    uint distanceOffset = queryIdx * numVectors;

    // Iterative selection: find the k-nearest neighbors one by one
    for (uint kIter = 0; kIter < k; ++kIter)
    {
        float minDist = kMaxFloat;
        uint  minIdx  = 0u;

        // Each thread scans its portion of the distance row
        for (uint v = localIdx; v < numVectors; v += 256u)
        {
            float d = distances[distanceOffset + v];

            // Exclude already-selected indices
            bool alreadySelected = false;
            for (uint j = 0u; j < kIter; ++j)
            {
                if (topKIndices[queryIdx * k + j] == v)
                {
                    alreadySelected = true;
                    break;
                }
            }

            if (!alreadySelected && d < minDist)
            {
                minDist = d;
                minIdx  = v;
            }
        }

        sharedDistances[localIdx] = minDist;
        sharedIndices[localIdx]   = minIdx;

        GroupMemoryBarrierWithGroupSync();

        // Parallel reduction to find the global minimum
        [unroll]
        for (uint stride = 128u; stride > 0u; stride >>= 1u)
        {
            if (localIdx < stride)
            {
                if (sharedDistances[localIdx + stride] < sharedDistances[localIdx])
                {
                    sharedDistances[localIdx] = sharedDistances[localIdx + stride];
                    sharedIndices[localIdx]   = sharedIndices[localIdx + stride];
                }
            }
            GroupMemoryBarrierWithGroupSync();
        }

        if (localIdx == 0u)
        {
            topKDists[queryIdx * k + kIter]   = sharedDistances[0];
            topKIndices[queryIdx * k + kIter] = sharedIndices[0];
        }

        GroupMemoryBarrierWithGroupSync();
    }
}
