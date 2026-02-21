/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            BufferManagement.cs                                ║
  Version:         0.0.8                                              ║
  Last Modified:   2026-02-21 12:08:57                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     474                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace Themis.DocumentManager.Services.DirectX;

/// <summary>
/// Depth Testing & Z-Buffer Management für korrektes 3D Rendering
/// </summary>
public class DepthBufferManager
{
    private float[] _depthBuffer;
    private int _width;
    private int _height;
    private float _nearPlane = 0.1f;
    private float _farPlane = 1000.0f;

    public DepthBufferManager(int width, int height)
    {
        _width = width;
        _height = height;
        _depthBuffer = new float[width * height];
        ClearDepthBuffer();
    }

    /// <summary>
    /// Clear Depth Buffer to Far Plane Value
    /// </summary>
    public void ClearDepthBuffer()
    {
        for (int i = 0; i < _depthBuffer.Length; i++)
        {
            _depthBuffer[i] = _farPlane;
        }
    }

    /// <summary>
    /// Test und Update Pixel Depth
    /// </summary>
    public bool TestDepth(int x, int y, float depth)
    {
        if (x < 0 || x >= _width || y < 0 || y >= _height)
            return false;

        int index = y * _width + x;
        if (depth < _depthBuffer[index])
        {
            _depthBuffer[index] = depth;
            return true;  // Pixel passes depth test
        }
        return false;  // Pixel fails depth test
    }

    /// <summary>
    /// Normalize Depth für [0,1] Range
    /// </summary>
    public float NormalizeDepth(float depth)
    {
        return (depth - _nearPlane) / (_farPlane - _nearPlane);
    }

    /// <summary>
    /// Get Depth Value at Pixel
    /// </summary>
    public float GetDepth(int x, int y)
    {
        if (x < 0 || x >= _width || y < 0 || y >= _height)
            return _farPlane;

        int index = y * _width + x;
        return _depthBuffer[index];
    }

    /// <summary>
    /// Resize Depth Buffer
    /// </summary>
    public void Resize(int newWidth, int newHeight)
    {
        _width = newWidth;
        _height = newHeight;
        _depthBuffer = new float[newWidth * newHeight];
        ClearDepthBuffer();
    }

    /// <summary>
    /// Get Depth Buffer Statistics
    /// </summary>
    public DepthBufferStats GetStatistics()
    {
        float minDepth = float.MaxValue;
        float maxDepth = float.MinValue;
        float avgDepth = 0;

        foreach (float depth in _depthBuffer)
        {
            if (depth < _farPlane)
            {
                minDepth = Math.Min(minDepth, depth);
                maxDepth = Math.Max(maxDepth, depth);
                avgDepth += depth;
            }
        }

        int pixelCount = _depthBuffer.Length;
        avgDepth /= pixelCount;

        return new DepthBufferStats
        {
            Width = _width,
            Height = _height,
            TotalPixels = pixelCount,
            MinDepth = minDepth == float.MaxValue ? _farPlane : minDepth,
            MaxDepth = maxDepth == float.MinValue ? _nearPlane : maxDepth,
            AverageDepth = avgDepth,
            NearPlane = _nearPlane,
            FarPlane = _farPlane
        };
    }
}

/// <summary>
/// Depth Buffer Statistics
/// </summary>
public class DepthBufferStats
{
    public int Width { get; set; }
    public int Height { get; set; }
    public int TotalPixels { get; set; }
    public float MinDepth { get; set; }
    public float MaxDepth { get; set; }
    public float AverageDepth { get; set; }
    public float NearPlane { get; set; }
    public float FarPlane { get; set; }

    public override string ToString()
    {
        return $"Depth: {Width}x{Height} | Min={MinDepth:F3} Max={MaxDepth:F3} Avg={AverageDepth:F3}";
    }
}

/// <summary>
/// GPU Buffer Management für Vertex, Index, Constant Buffers
/// </summary>
public class GPUBufferManager
{
    private Dictionary<string, GPUBuffer> _buffers = new();
    private long _totalMemoryAllocated = 0;
    private int _nextBufferId = 0;

    /// <summary>
    /// Create Vertex Buffer
    /// </summary>
    public GPUBuffer CreateVertexBuffer<T>(string name, T[] data) where T : struct
    {
        int elementSize = System.Runtime.InteropServices.Marshal.SizeOf<T>();
        long totalSize = (long)data.Length * elementSize;

        var buffer = new GPUBuffer
        {
            Id = _nextBufferId++,
            Name = name,
            Type = GPUBufferType.Vertex,
            ElementSize = elementSize,
            ElementCount = data.Length,
            TotalSize = totalSize,
            CreatedAt = DateTime.UtcNow,
            DataHandle = GCHandle.Alloc(data)
        };

        _buffers[name] = buffer;
        _totalMemoryAllocated += totalSize;

        System.Diagnostics.Debug.WriteLine(
            $"Created vertex buffer: {name} ({totalSize / 1024}KB, {data.Length} elements)");

        return buffer;
    }

    /// <summary>
    /// Create Index Buffer
    /// </summary>
    public GPUBuffer CreateIndexBuffer(string name, uint[] indices)
    {
        long totalSize = (long)indices.Length * 4; // uint = 4 bytes

        var buffer = new GPUBuffer
        {
            Id = _nextBufferId++,
            Name = name,
            Type = GPUBufferType.Index,
            ElementSize = 4,
            ElementCount = indices.Length,
            TotalSize = totalSize,
            CreatedAt = DateTime.UtcNow,
            DataHandle = GCHandle.Alloc(indices)
        };

        _buffers[name] = buffer;
        _totalMemoryAllocated += totalSize;

        System.Diagnostics.Debug.WriteLine(
            $"Created index buffer: {name} ({totalSize / 1024}KB, {indices.Length} indices)");

        return buffer;
    }

    /// <summary>
    /// Create Constant Buffer
    /// </summary>
    public GPUBuffer CreateConstantBuffer<T>(string name, T data) where T : struct
    {
        int size = System.Runtime.InteropServices.Marshal.SizeOf<T>();

        var buffer = new GPUBuffer
        {
            Id = _nextBufferId++,
            Name = name,
            Type = GPUBufferType.Constant,
            ElementSize = size,
            ElementCount = 1,
            TotalSize = size,
            CreatedAt = DateTime.UtcNow,
            DataHandle = GCHandle.Alloc(data)
        };

        _buffers[name] = buffer;
        _totalMemoryAllocated += size;

        System.Diagnostics.Debug.WriteLine(
            $"Created constant buffer: {name} ({size} bytes)");

        return buffer;
    }

    /// <summary>
    /// Get Buffer by Name
    /// </summary>
    public GPUBuffer? GetBuffer(string name)
    {
        return _buffers.ContainsKey(name) ? _buffers[name] : null;
    }

    /// <summary>
    /// Update Buffer Data
    /// </summary>
    public void UpdateBuffer(string name, object data)
    {
        if (!_buffers.ContainsKey(name))
            return;

        var buffer = _buffers[name];
        buffer.LastUpdated = DateTime.UtcNow;
        buffer.UpdateCount++;

        System.Diagnostics.Debug.WriteLine(
            $"Updated buffer: {name} (update #{buffer.UpdateCount})");
    }

    /// <summary>
    /// Release Buffer
    /// </summary>
    public void ReleaseBuffer(string name)
    {
        if (!_buffers.ContainsKey(name))
            return;

        var buffer = _buffers[name];
        if (buffer.DataHandle.IsAllocated)
        {
            buffer.DataHandle.Free();
        }

        _totalMemoryAllocated -= buffer.TotalSize;
        _buffers.Remove(name);

        System.Diagnostics.Debug.WriteLine(
            $"Released buffer: {name} ({buffer.TotalSize / 1024}KB freed)");
    }

    /// <summary>
    /// Release All Buffers
    /// </summary>
    public void ReleaseAll()
    {
        var bufferNames = _buffers.Keys.ToList();
        foreach (var name in bufferNames)
        {
            ReleaseBuffer(name);
        }
        _totalMemoryAllocated = 0;

        System.Diagnostics.Debug.WriteLine(
            $"Released all {bufferNames.Count} buffers");
    }

    /// <summary>
    /// Get Memory Statistics
    /// </summary>
    public GPUBufferManagerStats GetStatistics()
    {
        long vertexMemory = 0;
        long indexMemory = 0;
        long constantMemory = 0;

        foreach (var buffer in _buffers.Values)
        {
            switch (buffer.Type)
            {
                case GPUBufferType.Vertex:
                    vertexMemory += buffer.TotalSize;
                    break;
                case GPUBufferType.Index:
                    indexMemory += buffer.TotalSize;
                    break;
                case GPUBufferType.Constant:
                    constantMemory += buffer.TotalSize;
                    break;
            }
        }

        return new GPUBufferManagerStats
        {
            TotalBuffers = _buffers.Count,
            TotalMemory = _totalMemoryAllocated,
            VertexMemory = vertexMemory,
            IndexMemory = indexMemory,
            ConstantMemory = constantMemory
        };
    }
}

/// <summary>
/// GPU Buffer Container
/// </summary>
public class GPUBuffer
{
    public int Id { get; set; }
    public string Name { get; set; } = "";
    public GPUBufferType Type { get; set; }
    public int ElementSize { get; set; }
    public int ElementCount { get; set; }
    public long TotalSize { get; set; }
    public DateTime CreatedAt { get; set; }
    public DateTime LastUpdated { get; set; }
    public int UpdateCount { get; set; } = 0;
    public GCHandle DataHandle { get; set; }
    public IntPtr GpuMemoryAddress { get; set; } = IntPtr.Zero;
}

/// <summary>
/// GPU Buffer Type Enumeration
/// </summary>
public enum GPUBufferType
{
    Vertex,
    Index,
    Constant,
    Texture,
    RenderTarget,
    DepthStencil
}

/// <summary>
/// GPU Buffer Manager Statistics
/// </summary>
public class GPUBufferManagerStats
{
    public int TotalBuffers { get; set; }
    public long TotalMemory { get; set; }
    public long VertexMemory { get; set; }
    public long IndexMemory { get; set; }
    public long ConstantMemory { get; set; }

    public override string ToString()
    {
        return $"Buffers: {TotalBuffers} | Total: {TotalMemory / 1024}KB | " +
               $"Vertex: {VertexMemory / 1024}KB Index: {IndexMemory / 1024}KB Constant: {ConstantMemory / 1024}KB";
    }
}

/// <summary>
/// Render Target for Off-Screen Rendering
/// </summary>
public class RenderTarget
{
    public string Name { get; set; } = "";
    public int Width { get; set; }
    public int Height { get; set; }
    public IntPtr TextureHandle { get; set; } = IntPtr.Zero;
    public IntPtr RenderViewHandle { get; set; } = IntPtr.Zero;
    public IntPtr DepthStencilHandle { get; set; } = IntPtr.Zero;
    public DateTime CreatedAt { get; set; } = DateTime.UtcNow;

    public long GetMemorySize()
    {
        return (long)Width * Height * 4; // RGBA = 4 bytes per pixel
    }
}

/// <summary>
/// Stencil Buffer für Advanced Rendering Techniques
/// </summary>
public class StencilBuffer
{
    private byte[] _stencilData;
    private int _width;
    private int _height;

    public StencilBuffer(int width, int height)
    {
        _width = width;
        _height = height;
        _stencilData = new byte[width * height];
        Clear();
    }

    public void Clear()
    {
        Array.Fill(_stencilData, (byte)0);
    }

    public void SetStencil(int x, int y, byte value)
    {
        if (x >= 0 && x < _width && y >= 0 && y < _height)
        {
            int index = y * _width + x;
            _stencilData[index] = value;
        }
    }

    public byte GetStencil(int x, int y)
    {
        if (x >= 0 && x < _width && y >= 0 && y < _height)
        {
            int index = y * _width + x;
            return _stencilData[index];
        }
        return 0;
    }

    public void Resize(int newWidth, int newHeight)
    {
        _width = newWidth;
        _height = newHeight;
        _stencilData = new byte[newWidth * newHeight];
        Clear();
    }
}
