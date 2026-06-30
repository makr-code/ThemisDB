// NF4 Quantization Compute Shader — DirectX 12 / HLSL
// ThemisDB Hardware Acceleration — DirectX backend (LoRA module)
//
// Quantizes float32 values to 4-bit NormalFloat (NF4) format.
// Each workgroup processes one quantization block of block_size elements.
//
// Phase 1: parallel min/max reduction within the block (using groupshared memory).
// Phase 2: compute scale and zero point; write to ScalesData/ZerosData.
// Phase 3: normalize each element, find the nearest NF4 bin, and pack
//          8 values per uint32 via InterlockedOr.
//
// Binding layout (matches Vulkan quantization_nf4.comp):
//   b0 — float input data        (float[N])
//   b1 — packed NF4 output       (uint[ceil(N/8)])
//   b2 — per-block scales output (float[ceil(N/block_size)])
//   b3 — per-block zeros output  (float[ceil(N/block_size)])
//
// Root constants (push-constant equivalent):
//   num_elements : total number of elements
//   block_size   : quantization block size (elements per scale/zero entry)
//   num_blocks   : number of quantization blocks
//
// Entry point: CSMain
// Shader model: cs_6_0
//
// This file is the HLSL equivalent of
// src/acceleration/vulkan/shaders/lora/quantization_nf4.comp.

// ---------------------------------------------------------------------------
// Bindings
// ---------------------------------------------------------------------------

ByteAddressBuffer   InputData   : register(b0);  // Float32 input values
RWByteAddressBuffer OutputData  : register(b1);  // Packed NF4 (uint per 8 values)
RWByteAddressBuffer ScalesData  : register(b2);  // Per-block scales (float)
RWByteAddressBuffer ZerosData   : register(b3);  // Per-block zeros  (float)

// ---------------------------------------------------------------------------
// Root constants
// ---------------------------------------------------------------------------

cbuffer PushConstants : register(b4)
{
    uint num_elements;
    uint block_size;
    uint num_blocks;
};

// ---------------------------------------------------------------------------
// NF4 quantization lookup table (same values as dequantization_nf4.hlsl)
// ---------------------------------------------------------------------------

static const float NF4_VALUES[16] = {
    -1.0f,   -0.6962f, -0.5251f, -0.3949f,
    -0.2844f, -0.1848f, -0.0911f,  0.0f,
     0.0796f,  0.1609f,  0.2461f,  0.3379f,
     0.4407f,  0.5626f,  0.7230f,  1.0f
};

// ---------------------------------------------------------------------------
// Groupshared memory for parallel min/max reduction within a block
// ---------------------------------------------------------------------------

groupshared float gs_values[256];

// ---------------------------------------------------------------------------
// Helper: find the nearest NF4 bin for a normalized value in [-1, 1]
// ---------------------------------------------------------------------------

uint FindNF4Bin(float value)
{
    value = clamp(value, -1.0f, 1.0f);

    uint  best_bin  = 0;
    float min_dist  = abs(value - NF4_VALUES[0]);

    [unroll]
    for (uint i = 1u; i < 16u; ++i) {
        float d = abs(value - NF4_VALUES[i]);
        if (d < min_dist) {
            min_dist = d;
            best_bin = i;
        }
    }
    return best_bin;
}

// ---------------------------------------------------------------------------
// Compute kernel — one workgroup per quantization block
// ---------------------------------------------------------------------------

[numthreads(256, 1, 1)]
void CSMain(
    uint3 GTid  : SV_GroupThreadID,
    uint3 GID   : SV_GroupID)
{
    const uint lid        = GTid.x;
    const uint block_id   = GID.x;
    const uint block_start = block_id * block_size;
    const uint block_end   = min(block_start + block_size, num_elements);

    if (block_start >= num_elements) return;

    // ── Phase 1: parallel min/max within the block ────────────────────────────

    float local_max = -3.402823466e+38f;
    float local_min =  3.402823466e+38f;

    for (uint i = block_start + lid; i < block_end; i += 256u) {
        float val = asfloat(InputData.Load(i * 4));
        local_max = max(local_max, val);
        local_min = min(local_min, val);
    }

    // Tree reduction for max
    gs_values[lid] = local_max;
    GroupMemoryBarrierWithGroupSync();

    [unroll]
    for (uint stride = 128u; stride > 0u; stride >>= 1u) {
        if (lid < stride) {
            gs_values[lid] = max(gs_values[lid], gs_values[lid + stride]);
        }
        GroupMemoryBarrierWithGroupSync();
    }
    float block_max = gs_values[0];
    GroupMemoryBarrierWithGroupSync();

    // Tree reduction for min
    gs_values[lid] = local_min;
    GroupMemoryBarrierWithGroupSync();

    [unroll]
    for (uint stride = 128u; stride > 0u; stride >>= 1u) {
        if (lid < stride) {
            gs_values[lid] = min(gs_values[lid], gs_values[lid + stride]);
        }
        GroupMemoryBarrierWithGroupSync();
    }
    float block_min = gs_values[0];
    GroupMemoryBarrierWithGroupSync();

    // ── Phase 2: compute and write scale/zero ─────────────────────────────────

    float block_scale;
    float block_zero;

    if (lid == 0u) {
        if (abs(block_max - block_min) < 1e-8f) {
            block_scale = 1.0f;
            block_zero  = 0.0f;
        } else {
            block_scale = (block_max - block_min) * 0.5f;
            block_zero  = (block_max + block_min) * 0.5f;
        }
        ScalesData.Store(block_id * 4, asuint(block_scale));
        ZerosData.Store(block_id * 4, asuint(block_zero));
    }
    GroupMemoryBarrierWithGroupSync();

    // Re-load scale/zero from global memory so all threads share them.
    const float scale = asfloat(ScalesData.Load(block_id * 4));
    const float zero  = asfloat(ZerosData.Load(block_id * 4));

    // ── Phase 3: quantize and pack into output ────────────────────────────────

    for (uint i = block_start + lid; i < block_end; i += 256u) {
        float val        = asfloat(InputData.Load(i * 4));
        // Normalize to [-1, 1]
        float normalized = (scale > 1e-10f) ? (val - zero) / scale : 0.0f;

        uint bin        = FindNF4Bin(normalized);

        // Pack 8 values per uint32 (4 bits each, LSB-first)
        uint uint_idx   = i / 8u;
        uint bit_offset = (i % 8u) * 4u;

        // Atomic OR to safely merge contributions from multiple threads
        uint mask = bin << bit_offset;
        uint dummy;
        OutputData.InterlockedOr(uint_idx * 4, mask, dummy);
    }
}
