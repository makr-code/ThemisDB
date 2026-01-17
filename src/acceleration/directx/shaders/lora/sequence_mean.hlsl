// Sequence Mean Reduction Compute Shader for LoRA Training
// DirectX 12 Compute Shader (HLSL)
// Computes mean over sequence dimension: [batch, seq_len, hidden_dim] -> [batch, hidden_dim]
// ThemisDB DirectX Compute Shader

// Constants
cbuffer Constants : register(b0)
{
    uint batch_size;
    uint seq_len;
    uint hidden_dim;
    uint reserved;  // Padding for alignment
};

// Input/Output buffers
StructuredBuffer<float> input_embeddings : register(t0);       // [batch_size, seq_len, hidden_dim]
RWStructuredBuffer<float> output_embeddings : register(u0);    // [batch_size, hidden_dim]

[numthreads(256, 1, 1)]
void main(uint3 globalId : SV_DispatchThreadID)
{
    // Each thread handles one output element [batch_idx, hidden_idx]
    uint idx = globalId.x;
    uint total_outputs = batch_size * hidden_dim;
    
    if (idx >= total_outputs)
        return;
    
    uint batch_idx = idx / hidden_dim;
    uint hidden_idx = idx % hidden_dim;
    
    // Compute mean over sequence dimension
    float sum = 0.0f;
    for (uint seq_idx = 0; seq_idx < seq_len; seq_idx++)
    {
        uint input_idx = batch_idx * seq_len * hidden_dim + 
                        seq_idx * hidden_dim + 
                        hidden_idx;
        sum += input_embeddings[input_idx];
    }
    
    output_embeddings[idx] = sum / (float)seq_len;
}
