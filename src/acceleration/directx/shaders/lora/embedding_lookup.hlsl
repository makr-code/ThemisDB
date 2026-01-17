// Embedding Lookup Compute Shader for LoRA Training
// DirectX 12 Compute Shader (HLSL)
// Looks up embeddings from embedding matrix based on token IDs
// ThemisDB DirectX Compute Shader

// Constants
cbuffer Constants : register(b0)
{
    uint batch_size;
    uint seq_len;
    uint hidden_dim;
    uint vocab_size;
};

// Input/Output buffers
StructuredBuffer<float> token_ids : register(t0);              // [batch_size, seq_len]
StructuredBuffer<float> embedding_weights : register(t1);      // [vocab_size, hidden_dim]
RWStructuredBuffer<float> embeddings : register(u0);           // [batch_size, seq_len, hidden_dim]

[numthreads(256, 1, 1)]
void main(uint3 globalId : SV_DispatchThreadID)
{
    // Each thread processes one token
    uint idx = globalId.x;
    uint total_tokens = batch_size * seq_len;
    
    if (idx >= total_tokens)
        return;
    
    // Convert float token ID to int (with rounding)
    int token_id = (int)round(token_ids[idx]);
    
    // Bounds check
    if (token_id >= 0 && token_id < (int)vocab_size)
    {
        // Calculate source and destination offsets
        uint src_offset = (uint)token_id * hidden_dim;
        uint dst_offset = idx * hidden_dim;
        
        // Copy embedding vector
        for (uint i = 0; i < hidden_dim; i++)
        {
            embeddings[dst_offset + i] = embedding_weights[src_offset + i];
        }
    }
    else
    {
        // Out of bounds - fill with zeros
        uint dst_offset = idx * hidden_dim;
        for (uint i = 0; i < hidden_dim; i++)
        {
            embeddings[dst_offset + i] = 0.0f;
        }
    }
}
