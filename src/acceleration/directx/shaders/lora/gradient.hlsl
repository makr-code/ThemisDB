// Gradient computation for LoRA backward pass
// DirectX 12 Compute Shader (HLSL)
// Computes gradients for LoRA parameters (A and B matrices)
// ThemisDB DirectX Compute Shader for LoRA

// Constants
cbuffer Constants : register(b0)
{
    uint batch_size;
    uint in_dim;
    uint rank;
    uint out_dim;
    float scaling;
    uint compute_mode; // 0=grad_A, 1=grad_B, 2=grad_input
};

// Input buffers
StructuredBuffer<float> input : register(t0);        // Cached input from forward pass
StructuredBuffer<float> B : register(t1);            // LoRA B matrix
StructuredBuffer<float> A : register(t2);            // LoRA A matrix
StructuredBuffer<float> grad_output : register(t3); // Gradient from next layer

// Output buffers
RWStructuredBuffer<float> grad_A : register(u0);     // Gradient w.r.t. A
RWStructuredBuffer<float> grad_B : register(u1);     // Gradient w.r.t. B
RWStructuredBuffer<float> grad_input : register(u2); // Gradient w.r.t. input

[numthreads(16, 16, 1)]
void main(uint3 globalId : SV_DispatchThreadID)
{
    uint row = globalId.y;
    uint col = globalId.x;
    
    if (compute_mode == 0)
    {
        // Compute grad_A = B.T @ (input.T @ (grad_output * scaling))
        // Result shape: (rank, out_dim)
        
        if (row >= rank || col >= out_dim)
            return;
        
        float sum = 0.0f;
        
        // Accumulate over batch and input dimensions
        for (uint b = 0; b < batch_size; b++)
        {
            for (uint i = 0; i < in_dim; i++)
            {
                float input_val = input[b * in_dim + i];
                float grad_val = grad_output[b * out_dim + col] * scaling;
                float b_val = B[i * rank + row];
                
                sum += b_val * input_val * grad_val;
            }
        }
        
        grad_A[row * out_dim + col] = sum;
    }
    else if (compute_mode == 1)
    {
        // Compute grad_B = (grad_output * scaling @ A.T) @ input.T
        // Result shape: (in_dim, rank)
        
        if (row >= in_dim || col >= rank)
            return;
        
        float sum = 0.0f;
        
        // Accumulate over batch and output dimensions
        for (uint b = 0; b < batch_size; b++)
        {
            for (uint o = 0; o < out_dim; o++)
            {
                float grad_val = grad_output[b * out_dim + o] * scaling;
                float a_val = A[col * out_dim + o];
                float input_val = input[b * in_dim + row];
                
                sum += grad_val * a_val * input_val;
            }
        }
        
        grad_B[row * rank + col] = sum;
    }
    else if (compute_mode == 2)
    {
        // Compute grad_input = (grad_output * scaling) @ (BA).T
        // where BA = B @ A
        // Result shape: (batch_size, in_dim)
        
        if (row >= batch_size || col >= in_dim)
            return;
        
        float sum = 0.0f;
        
        // Compute BA on-the-fly and multiply with grad_output
        for (uint o = 0; o < out_dim; o++)
        {
            float grad_val = grad_output[row * out_dim + o] * scaling;
            
            // Compute BA[col, o] = sum_r B[col, r] * A[r, o]
            float ba_val = 0.0f;
            for (uint r = 0; r < rank; r++)
            {
                ba_val += B[col * rank + r] * A[r * out_dim + o];
            }
            
            sum += grad_val * ba_val;
        }
        
        grad_input[row * in_dim + col] = sum;
    }
}
