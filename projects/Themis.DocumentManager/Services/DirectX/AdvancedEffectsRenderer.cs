/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            AdvancedEffectsRenderer.cs                         ║
  Version:         0.0.24                                             ║
  Last Modified:   2026-02-22 08:12:15                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     468                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 00c723d27  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 03329d86d  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31e8b8df0  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 0d722b04c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Collections.Generic;
using System.Numerics;
using Themis.DocumentManager.Models;

namespace Themis.DocumentManager.Services.DirectX
{
    /// <summary>
    /// Advanced shadow mapping system for realistic lighting
    /// </summary>
    public class ShadowMappingSystem
    {
        public class ShadowMapConfig
        {
            public int Resolution { get; set; } = 2048;
            public float NearPlane { get; set; } = 0.1f;
            public float FarPlane { get; set; } = 100.0f;
            public float BiasMin { get; set; } = 0.005f;
            public float BiasMax { get; set; } = 0.02f;
            public int PCFKernelSize { get; set; } = 3;  // 3x3 = 9 samples
            public bool SoftShadows { get; set; } = true;
        }

        private ShadowMapConfig _config = new();
        private Dictionary<int, float[]> _shadowMaps = new();
        private int _nextShadowMapId = 1;
        private long _shadowTestsPerformed = 0;
        private long _pixelsInShadow = 0;

        /// <summary>
        /// Create shadow map for light
        /// </summary>
        public int CreateShadowMap(int lightId)
        {
            var mapId = _nextShadowMapId++;
            var shadowMap = new float[_config.Resolution * _config.Resolution];
            
            // Initialize with max depth (1.0)
            for (int i = 0; i < shadowMap.Length; i++)
                shadowMap[i] = 1.0f;

            _shadowMaps[mapId] = shadowMap;
            return mapId;
        }

        /// <summary>
        /// Write depth to shadow map
        /// </summary>
        public void WriteDepth(int shadowMapId, int x, int y, float depth)
        {
            if (!_shadowMaps.TryGetValue(shadowMapId, out var map))
                return;

            x = Math.Clamp(x, 0, _config.Resolution - 1);
            y = Math.Clamp(y, 0, _config.Resolution - 1);

            var index = y * _config.Resolution + x;
            if (index >= 0 && index < map.Length)
                map[index] = depth;
        }

        /// <summary>
        /// Sample shadow map with PCF filtering
        /// </summary>
        public float SampleShadow(int shadowMapId, float u, float v, float currentDepth)
        {
            _shadowTestsPerformed++;

            if (!_shadowMaps.TryGetValue(shadowMapId, out var map))
                return 1.0f;

            float shadowFactor = 0.0f;
            int kernelSize = _config.PCFKernelSize;
            int samples = kernelSize * kernelSize;

            // Convert UV to texel coordinates
            int centerX = (int)(u * (_config.Resolution - 1));
            int centerY = (int)(v * (_config.Resolution - 1));

            // PCF: Percentage Closer Filtering
            for (int dy = -kernelSize / 2; dy <= kernelSize / 2; dy++)
            {
                for (int dx = -kernelSize / 2; dx <= kernelSize / 2; dx++)
                {
                    int x = Math.Clamp(centerX + dx, 0, _config.Resolution - 1);
                    int y = Math.Clamp(centerY + dy, 0, _config.Resolution - 1);

                    var index = y * _config.Resolution + x;
                    if (index >= 0 && index < map.Length)
                    {
                        float shadowDepth = map[index];
                        float bias = _config.BiasMin + (1.0f - currentDepth) * _config.BiasMax;
                        
                        if (currentDepth + bias <= shadowDepth)
                            shadowFactor += 1.0f;
                    }
                }
            }

            shadowFactor /= samples;

            if (shadowFactor < 1.0f)
                _pixelsInShadow++;

            return shadowFactor;  // 1.0 = fully lit, 0.0 = fully shadowed
        }

        /// <summary>
        /// Get shadow statistics
        /// </summary>
        public (long Tests, long InShadow, double ShadowRatio) GetStatistics()
        {
            double ratio = _shadowTestsPerformed > 0 ? _pixelsInShadow / (double)_shadowTestsPerformed : 0;
            return (_shadowTestsPerformed, _pixelsInShadow, ratio);
        }

        /// <summary>
        /// Clear shadow maps
        /// </summary>
        public void Clear()
        {
            _shadowMaps.Clear();
            _shadowTestsPerformed = 0;
            _pixelsInShadow = 0;
        }
    }

    /// <summary>
    /// Normal mapping for surface detail enhancement
    /// </summary>
    public class NormalMappingSystem
    {
        public class NormalMapData
        {
            public int TextureId { get; set; }
            public int Width { get; set; }
            public int Height { get; set; }
            public float[] NormalData { get; set; } = Array.Empty<float>();  // RGB stored as normals
        }

        private Dictionary<int, NormalMapData> _normalMaps = new();
        private int _nextNormalMapId = 1;
        private long _normalsComputed = 0;

        /// <summary>
        /// Create normal map from texture
        /// </summary>
        public int CreateNormalMap(int width, int height, byte[] textureData)
        {
            var mapId = _nextNormalMapId++;
            var normalData = new float[width * height * 3];  // RGB per pixel

            // Compute normals using Sobel filter
            for (int y = 1; y < height - 1; y++)
            {
                for (int x = 1; x < width - 1; x++)
                {
                    // Sobel X
                    float gx = GetPixelHeight(textureData, x - 1, y - 1, width) * -1 +
                               GetPixelHeight(textureData, x + 1, y - 1, width) * 1 +
                               GetPixelHeight(textureData, x - 1, y, width) * -2 +
                               GetPixelHeight(textureData, x + 1, y, width) * 2 +
                               GetPixelHeight(textureData, x - 1, y + 1, width) * -1 +
                               GetPixelHeight(textureData, x + 1, y + 1, width) * 1;

                    // Sobel Y
                    float gy = GetPixelHeight(textureData, x - 1, y - 1, width) * -1 +
                               GetPixelHeight(textureData, x - 1, y + 1, width) * 1 +
                               GetPixelHeight(textureData, x, y - 1, width) * -2 +
                               GetPixelHeight(textureData, x, y + 1, width) * 2 +
                               GetPixelHeight(textureData, x + 1, y - 1, width) * -1 +
                               GetPixelHeight(textureData, x + 1, y + 1, width) * 1;

                    // Compute normal (normalize after)
                    var normal = new Vector3(-gx, -gy, 1.0f);
                    normal = Vector3.Normalize(normal);

                    var index = (y * width + x) * 3;
                    normalData[index] = normal.X;
                    normalData[index + 1] = normal.Y;
                    normalData[index + 2] = normal.Z;

                    _normalsComputed++;
                }
            }

            var map = new NormalMapData
            {
                TextureId = mapId,
                Width = width,
                Height = height,
                NormalData = normalData
            };

            _normalMaps[mapId] = map;
            return mapId;
        }

        /// <summary>
        /// Sample normal from map
        /// </summary>
        public Vector3 SampleNormal(int normalMapId, float u, float v)
        {
            if (!_normalMaps.TryGetValue(normalMapId, out var map))
                return Vector3.UnitZ;  // Default upward normal

            int x = (int)(u * (map.Width - 1));
            int y = (int)(v * (map.Height - 1));

            x = Math.Clamp(x, 0, map.Width - 1);
            y = Math.Clamp(y, 0, map.Height - 1);

            var index = (y * map.Width + x) * 3;
            return new Vector3(
                map.NormalData[index],
                map.NormalData[index + 1],
                map.NormalData[index + 2]);
        }

        /// <summary>
        /// Compute TBN matrix for normal mapping
        /// </summary>
        public static (Vector3 Tangent, Vector3 Bitangent, Vector3 Normal) ComputeTBN(
            Vector3 position, Vector3 normal,
            Vector2 uv, Vector3 neighborPos, Vector2 neighborUv)
        {
            Vector3 edge = neighborPos - position;
            Vector2 uvEdge = neighborUv - uv;

            float f = 1.0f / (uvEdge.X * uvEdge.Y);

            Vector3 tangent = new Vector3(
                f * (uvEdge.Y * edge.X - uvEdge.X * edge.X),
                f * (uvEdge.Y * edge.Y - uvEdge.X * edge.Y),
                f * (uvEdge.Y * edge.Z - uvEdge.X * edge.Z));

            tangent = Vector3.Normalize(tangent);

            Vector3 bitangent = Vector3.Cross(normal, tangent);
            bitangent = Vector3.Normalize(bitangent);

            return (tangent, bitangent, normal);
        }

        /// <summary>
        /// Transform normal from tangent space to world space
        /// </summary>
        public static Vector3 TransformNormalToWorldSpace(
            Vector3 normalTS, Vector3 tangent, Vector3 bitangent, Vector3 normal)
        {
            return Vector3.Normalize(
                normalTS.X * tangent +
                normalTS.Y * bitangent +
                normalTS.Z * normal);
        }

        private float GetPixelHeight(byte[] data, int x, int y, int width)
        {
            if (x < 0 || x >= width) return 0.5f;
            
            var index = (y * width + x) * 4;
            if (index + 2 >= data.Length) return 0.5f;

            // Average RGB for grayscale
            return (data[index] + data[index + 1] + data[index + 2]) / (3.0f * 255.0f);
        }

        public long GetNormalsComputed() => _normalsComputed;
    }

    /// <summary>
    /// Parallax occlusion mapping for enhanced depth perception
    /// </summary>
    public class ParallaxOcclusionMapping
    {
        public class ParallaxConfig
        {
            public float HeightScale { get; set; } = 0.1f;
            public int SamplingSteps { get; set; } = 8;
            public bool BinarySearch { get; set; } = true;
            public int BinarySearchSteps { get; set; } = 4;
        }

        private ParallaxConfig _config = new();
        private long _parallaxSamplesPerformed = 0;
        private long _parallaxOcclusionDetected = 0;

        /// <summary>
        /// Apply parallax occlusion mapping
        /// </summary>
        public Vector2 ComputeParallaxUV(
            Vector2 uv,
            Vector3 viewDir,
            Vector3 tangent,
            Vector3 bitangent,
            Vector3 normal,
            Func<float, float, float> heightSampler)
        {
            // Transform view direction to tangent space
            var tbnMatrix = new Matrix4x4(
                tangent.X, bitangent.X, normal.X, 0,
                tangent.Y, bitangent.Y, normal.Y, 0,
                tangent.Z, bitangent.Z, normal.Z, 0,
                0, 0, 0, 1);

            var viewDirTS = Vector3.TransformNormal(viewDir, tbnMatrix);

            Vector2 parallaxUV = uv;
            float currentHeight = heightSampler(uv.X, uv.Y);
            Vector2 uvStep = new Vector2(-viewDirTS.X, -viewDirTS.Y) / viewDirTS.Z * _config.HeightScale / _config.SamplingSteps;

            // Parallax mapping iterations
            for (int i = 0; i < _config.SamplingSteps; i++)
            {
                _parallaxSamplesPerformed++;

                float sampledHeight = heightSampler(parallaxUV.X, parallaxUV.Y);
                float rayHeight = 1.0f - (i / (float)_config.SamplingSteps);

                if (sampledHeight > rayHeight)
                {
                    _parallaxOcclusionDetected++;
                    parallaxUV -= uvStep;
                    break;
                }

                parallaxUV += uvStep;
            }

            return parallaxUV;
        }

        /// <summary>
        /// Get parallax statistics
        /// </summary>
        public (long Samples, long Occlusions, double OcclusionRate) GetStatistics()
        {
            double rate = _parallaxSamplesPerformed > 0 
                ? _parallaxOcclusionDetected / (double)_parallaxSamplesPerformed 
                : 0;
            return (_parallaxSamplesPerformed, _parallaxOcclusionDetected, rate);
        }
    }

    /// <summary>
    /// Screen-space ambient occlusion (SSAO)
    /// </summary>
    public class ScreenSpaceAmbientOcclusion
    {
        public class SSAOConfig
        {
            public float Radius { get; set; } = 1.0f;
            public float Bias { get; set; } = 0.025f;
            public int SampleCount { get; set; } = 16;
            public float Strength { get; set; } = 1.5f;
            public float MaxDistance { get; set; } = 100.0f;
        }

        private SSAOConfig _config = new();
        private Dictionary<(int, int), float> _occlusionBuffer = new();
        private int _width = 0;
        private int _height = 0;
        private long _pixelsProcessed = 0;
        private long _pixelsOccluded = 0;

        /// <summary>
        /// Initialize SSAO buffer
        /// </summary>
        public void Initialize(int width, int height)
        {
            _width = width;
            _height = height;
            _occlusionBuffer.Clear();
        }

        /// <summary>
        /// Compute SSAO for pixel
        /// </summary>
        public float ComputeOcclusion(
            int x, int y,
            float depth,
            float normalX, float normalY, float normalZ,
            Func<int, int, float> depthSampler)
        {
            _pixelsProcessed++;

            float occlusion = 0.0f;
            Vector3 normal = new Vector3(normalX, normalY, normalZ);

            // Random sample directions
            var sampleOffsets = GeneratePoissonOffsets(_config.SampleCount);

            for (int i = 0; i < _config.SampleCount; i++)
            {
                var offset = sampleOffsets[i];
                int sampleX = x + (int)(offset.X * _config.Radius);
                int sampleY = y + (int)(offset.Y * _config.Radius);

                if (sampleX >= 0 && sampleX < _width && sampleY >= 0 && sampleY < _height)
                {
                    float sampleDepth = depthSampler(sampleX, sampleY);
                    float depthDiff = sampleDepth - depth;

                    if (depthDiff > _config.Bias && depthDiff < _config.MaxDistance)
                    {
                        occlusion += 1.0f;
                        _pixelsOccluded++;
                    }
                }
            }

            occlusion = 1.0f - (occlusion / _config.SampleCount) * _config.Strength;
            return Math.Clamp(occlusion, 0.0f, 1.0f);
        }

        /// <summary>
        /// Get SSAO statistics
        /// </summary>
        public (long Processed, long Occluded, double OcclusionRatio) GetStatistics()
        {
            double ratio = _pixelsProcessed > 0 ? _pixelsOccluded / (double)_pixelsProcessed : 0;
            return (_pixelsProcessed, _pixelsOccluded, ratio);
        }

        private List<Vector2> GeneratePoissonOffsets(int count)
        {
            var offsets = new List<Vector2>();
            var random = new Random(42);  // Fixed seed for reproducibility

            for (int i = 0; i < count; i++)
            {
                float angle = (i / (float)count) * (float)Math.PI * 2.0f;
                float radius = (float)random.NextDouble();
                offsets.Add(new Vector2(
                    (float)Math.Cos(angle) * radius,
                    (float)Math.Sin(angle) * radius));
            }

            return offsets;
        }
    }
}
