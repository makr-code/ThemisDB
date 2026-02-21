/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            DirectXServiceCollectionExtensions.cs              ║
  Version:         0.0.7                                              ║
  Last Modified:   2026-02-21 11:48:38                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   97.0/100                                       ║
    • Total Lines:     58                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using Microsoft.Extensions.DependencyInjection;
using Themis.DocumentManager.Features.Graph.Services;

namespace Themis.DocumentManager.Services.DirectX;

/// <summary>
/// Dependency Injection Extensions für DirectX 11 Services
/// </summary>
public static class DirectXServiceCollectionExtensions
{
    /// <summary>
    /// Add all DirectX 11 rendering services to the DI container
    /// </summary>
    public static IServiceCollection AddDirectX3DServices(this IServiceCollection services)
    {
        // Register core DirectX services
        services.AddSingleton<IDirectXDevice, DirectXDevice>();
        services.AddSingleton<IShaderManager, ShaderManager>();
        services.AddSingleton<IMeshBufferManager, MeshBufferManager>();
        services.AddSingleton<IConstantBufferManager, ConstantBufferManager>();

        // Register graph renderers
        services.AddSingleton<IDirectX3DGraphRenderer, AdvancedDirectX3DGraphRenderer>();
        services.AddSingleton<IDirectXGraphRenderer>(sp => 
            new DirectXGraphRendererAdapter(sp.GetRequiredService<IDirectX3DGraphRenderer>()));

        // Register visualization service
        services.AddSingleton<IGraphVisualizationService, GraphVisualizationService>();

        return services;
    }
}
