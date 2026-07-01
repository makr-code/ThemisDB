// Point-in-polygon test compute shader using ray-casting
// ThemisDB DirectX 12 Compute Shader (HLSL cs_5_0 / cs_6_0)
//
// For each test point, counts edge crossings along the +longitude ray.
// An odd crossing count indicates the point is inside the polygon.
// DirectX equivalent of src/acceleration/vulkan/shaders/point_in_polygon.comp
// and src/acceleration/cuda/geo_kernels.cu pointInPolygonKernel.
//
// Root signature (dedicated 4-UAV layout):
//   params[0]: UAV root descriptor at u0 — test-point latitudes   [numPoints]
//   params[1]: UAV root descriptor at u1 — test-point longitudes  [numPoints]
//   params[2]: UAV root descriptor at u2 — polygon vertex coords, interleaved [lat, lon]
//                                          flat array of length numVertices × 2
//   params[3]: UAV root descriptor at u3 — output results: 1 = inside, 0 = outside [numPoints]
//   params[4]: 32-bit inline constants at b0 — { numPoints, numVertices, padding0, padding1 }
//
// Thread layout: [numthreads(256, 1, 1)]
//   DTid.x → point index
//   Dispatch: (ceil(numPoints / 256), 1, 1)

RWStructuredBuffer<float> pointLats     : register(u0);
RWStructuredBuffer<float> pointLons     : register(u1);
RWStructuredBuffer<float> polygonCoords : register(u2); // interleaved [lat, lon] per vertex
RWStructuredBuffer<uint>  results       : register(u3);

cbuffer Constants : register(b0)
{
    uint numPoints;
    uint numVertices;
    uint padding0;
    uint padding1;
};

[numthreads(256, 1, 1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
    uint p = DTid.x;
    if (p >= numPoints)
        return;

    float testLat = pointLats[p];
    float testLon = pointLons[p];

    bool inside = false;
    uint j = numVertices - 1u;

    for (uint i = 0u; i < numVertices; ++i)
    {
        float lat_i = polygonCoords[i * 2u];
        float lon_i = polygonCoords[i * 2u + 1u];
        float lat_j = polygonCoords[j * 2u];
        float lon_j = polygonCoords[j * 2u + 1u];

        // Ray-casting: count crossings of the horizontal ray
        if (((lon_i > testLon) != (lon_j > testLon)) &&
            (testLat < (lat_j - lat_i) * (testLon - lon_i) / (lon_j - lon_i) + lat_i))
        {
            inside = !inside;
        }
        j = i;
    }

    results[p] = inside ? 1u : 0u;
}
