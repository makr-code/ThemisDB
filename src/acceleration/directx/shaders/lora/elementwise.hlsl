// Element-wise operations for LoRA Training
// DirectX 12 Compute Shader (HLSL)
// Supports: add, subtract, multiply, divide, transpose
// ThemisDB DirectX Compute Shader for LoRA

// Constants
cbuffer Constants : register(b0)
{
    uint size;    // Total number of elements
    uint op;      // Operation type: 0=add, 1=sub, 2=mul, 3=div, 4=scalar_mul
    uint rows;    // For transpose operation
    uint cols;    // For transpose operation
    float scalar; // Scalar value for scalar operations
};

// Input/Output buffers
StructuredBuffer<float> A : register(t0);
StructuredBuffer<float> B : register(t1);
RWStructuredBuffer<float> C : register(u0);

[numthreads(256, 1, 1)]
void main(uint3 globalId : SV_DispatchThreadID)
{
    uint idx = globalId.x;
    
    if (idx >= size)
        return;
    
    float result = 0.0f;
    
    // Select operation
    switch (op)
    {
        case 0: // Add: C = A + B
            result = A[idx] + B[idx];
            break;
            
        case 1: // Subtract: C = A - B
            result = A[idx] - B[idx];
            break;
            
        case 2: // Multiply: C = A * B (element-wise)
            result = A[idx] * B[idx];
            break;
            
        case 3: // Divide: C = A / B (element-wise)
            result = A[idx] / B[idx];
            break;
            
        case 4: // Scalar multiply: C = A * scalar
            result = A[idx] * scalar;
            break;
            
        case 5: // Transpose: C[j * rows + i] = A[i * cols + j]
        {
            uint out_row = idx / rows;
            uint out_col = idx % rows;
            uint in_idx = out_col * cols + out_row;
            
            if (in_idx < size)
            {
                result = A[in_idx];
            }
            break;
        }
        
        case 6: // ReLU activation: C = max(0, A)
            result = max(0.0f, A[idx]);
            break;
            
        case 7: // Square: C = A * A
            result = A[idx] * A[idx];
            break;
            
        case 8: // Sqrt: C = sqrt(A)
            result = sqrt(A[idx]);
            break;
            
        default:
            result = A[idx];
            break;
    }
    
    C[idx] = result;
}
