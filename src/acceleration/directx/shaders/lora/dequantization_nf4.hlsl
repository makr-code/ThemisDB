// NF4 Dequantization Compute Shader — DirectX 12 / HLSL
// ThemisDB Hardware Acceleration — DirectX backend (LoRA module)
//
// Dequantizes 4-bit NormalFloat (NF4) packed values back to float32.
// One thread per output element; 8 NF4 values are packed per uint32.
//
// Binding layout (matches Vulkan dequantization_nf4.comp):
//   b0 — packed input data  (uint[ceil(N/8)])
//   b1 — per-block scales   (float[ceil(N/block_size)])
//   b2 — per-block zeros    (float[ceil(N/block_size)])
//   b3 — dequantized output (float[N])
//
// Root constants (push-constant equivalent):
//   num_elements : total number of elements to dequantize
//   block_size   : quantization block size (elements per scale/zero entry)
//
// Entry point: CSMain
// Shader model: cs_6_0
//
// This file is the HLSL equivalent of
// src/acceleration/vulkan/shaders/lora/dequantization_nf4.comp.

// ---------------------------------------------------------------------------
// Bindings
// ---------------------------------------------------------------------------

ByteAddressBuffer   InputData   : register(b0);  // Packed NF4 (uint per 8 values)
ByteAddressBuffer   ScalesData  : register(b1);  // Per-block scales (float)
ByteAddressBuffer   ZerosData   : register(b2);  // Per-block zeros  (float)
RWByteAddressBuffer OutputData  : register(b3);  // Dequantized output (float)

// ---------------------------------------------------------------------------
// Root constants
// ---------------------------------------------------------------------------

cbuffer PushConstants : register(b4)
{
    uint num_elements;
    uint block_size;
};

// ---------------------------------------------------------------------------
// NF4 dequantization lookup table
// 16 representative values evenly covering the standard normal distribution,
// matching the BitsAndBytes NF4 specification used by the Vulkan backend.
// ---------------------------------------------------------------------------

static const float NF4_VALUES[16] = {
    -1.0f,   -0.6962f, -0.5251f, -0.3949f,
    -0.2844f, -0.1848f, -0.0911f,  0.0f,
     0.0796f,  0.1609f,  0.2461f,  0.3379f,
     0.4407f,  0.5626f,  0.7230f,  1.0f
};

// ---------------------------------------------------------------------------
// Compute kernel
// ---------------------------------------------------------------------------

[numthreads(256, 1, 1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
    const uint idx = DTid.x;
    if (idx >= num_elements) return;

    // Determine block and load per-block scale / zero
    const uint block_id = idx / block_size;
    const float scale   = asfloat(ScalesData.Load(block_id * 4));
    const float zero    = asfloat(ZerosData.Load(block_id * 4));

    // Unpack the 4-bit NF4 value for this element.
    // Packing: 8 values per uint32, each occupying 4 bits (LSB-first).
    const uint uint_idx   = idx / 8;
    const uint bit_offset = (idx % 8) * 4;
    const uint packed     = InputData.Load(uint_idx * 4);
    const uint bin        = (packed >> bit_offset) & 0xFu;

    // Dequantize: normalised_value * scale + zero
    const float normalized = NF4_VALUES[bin];
    const float result     = normalized * scale + zero;

    OutputData.Store(idx * 4, asuint(result));
}
