/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            DirectXServiceCollectionExtensions.cs              ║
  Version:         0.0.10                                             ║
  Last Modified:   2026-02-21 13:56:49                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   97.0/100                                       ║
    • Total Lines:     108                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using Microsoft.Extensions.DependencyInjection;
using Themis.DocumentManager.Services.DirectX;

namespace Themis.DocumentManager.Services;

/// <summary>
/// Dependency Injection Extension für DirectX 3D Services
/// </summary>
public static class DirectXServiceCollectionExtensions
{
    public static IServiceCollection AddDirectX3DServices(this IServiceCollection services)
    {
        // Register DirectX Device Manager
        services.AddSingleton<IDirectXDevice, DirectXDevice>();
        
        // Register Shader Manager
        services.AddSingleton<IShaderManager, ShaderManager>();
        
        // Register Mesh Buffer Manager
        services.AddSingleton<IMeshBufferManager, MeshBufferManager>();
        
        // Register Constant Buffer Manager
        services.AddSingleton<IConstantBufferManager, ConstantBufferManager>();
        
        // Register 3D Graph Renderer
        services.AddSingleton<IDirectX3DGraphRenderer, DirectX3DGraphRenderer>();
        
        // Legacy: Register DirectX Graph Renderer interface for compatibility
        services.AddSingleton<IDirectXGraphRenderer>(sp => 
            new DirectXGraphRendererAdapter(sp.GetRequiredService<IDirectX3DGraphRenderer>()));

        return services;
    }
}

/// <summary>
/// Adapter für alte IDirectXGraphRenderer Schnittstelle
/// </summary>
public class DirectXGraphRendererAdapter : IDirectXGraphRenderer
{
    private readonly IDirectX3DGraphRenderer _renderer;

    public DirectXGraphRendererAdapter(IDirectX3DGraphRenderer renderer)
    {
        _renderer = renderer;
    }

    public void Initialize(IntPtr windowHandle, int width, int height)
    {
        _renderer.Initialize(windowHandle, width, height);
    }

    public void Render(Models.Graph graph)
    {
        _renderer.Render(graph);
    }

    public void SetCamera(Models.Vector3D position, Models.Vector3D target)
    {
        _renderer.SetCameraPosition(position.X, position.Y, position.Z);
    }

    public void Rotate(float deltaX, float deltaY)
    {
        _renderer.Rotate(deltaX, deltaY);
    }

    public void Zoom(float delta)
    {
        _renderer.Zoom(delta);
    }

    public void Resize(int width, int height)
    {
        _renderer.Resize(width, height);
    }

    public void Cleanup()
    {
        _renderer.Cleanup();
    }
}
