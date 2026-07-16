// Haversine geodesic distance compute shader
// ThemisDB DirectX 12 Compute Shader (HLSL cs_5_0 / cs_6_0)
//
// Computes per-pair Haversine distances in kilometres for batches of coordinate pairs.
// DirectX equivalent of src/acceleration/vulkan/shaders/haversine_distance.comp
// and src/acceleration/cuda/geo_kernels.cu haversineDistanceKernel.
//
// Root signature (dedicated; different from the ANN 3-UAV layout):
//   params[0]: UAV root descriptor at u0 — input latitudes  set 1  [count]
//   params[1]: UAV root descriptor at u1 — input longitudes set 1  [count]
//   params[2]: UAV root descriptor at u2 — input latitudes  set 2  [count]
//   params[3]: UAV root descriptor at u3 — input longitudes set 2  [count]
//   params[4]: UAV root descriptor at u4 — output distances in km  [count]
//   params[5]: 32-bit inline constants at b0 — { count, padding0, padding1, padding2 }
//
// Thread layout: [numthreads(256, 1, 1)]
//   DTid.x → point-pair index
//   Dispatch: (ceil(count / 256), 1, 1)

RWStructuredBuffer<float> lats1 : register(u0);
RWStructuredBuffer<float> lons1 : register(u1);
RWStructuredBuffer<float> lats2 : register(u2);
RWStructuredBuffer<float> lons2 : register(u3);
RWStructuredBuffer<float> dists : register(u4);

cbuffer Constants : register(b0)
{
    uint count;
    uint padding0;
    uint padding1;
    uint padding2;
};

static const float PI               = 3.14159265358979323846f;
static const float EARTH_RADIUS_KM  = 6371.0f;

[numthreads(256, 1, 1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
    uint i = DTid.x;
    if (i >= count)
        return;

    // Degrees to radians
    float lat1 = lats1[i] * PI / 180.0f;
    float lon1 = lons1[i] * PI / 180.0f;
    float lat2 = lats2[i] * PI / 180.0f;
    float lon2 = lons2[i] * PI / 180.0f;

    float dlat = lat2 - lat1;
    float dlon = lon2 - lon1;

    float a = sin(dlat * 0.5f) * sin(dlat * 0.5f)
            + cos(lat1) * cos(lat2) * sin(dlon * 0.5f) * sin(dlon * 0.5f);

    dists[i] = EARTH_RADIUS_KM * 2.0f * atan2(sqrt(a), sqrt(1.0f - a));
}
