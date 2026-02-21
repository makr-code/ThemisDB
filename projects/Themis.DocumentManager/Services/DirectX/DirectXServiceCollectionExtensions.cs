/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            DirectXServiceCollectionExtensions.cs              ║
  Version:         0.0.15                                             ║
  Last Modified:   2026-02-21 17:07:32                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   97.0/100                                       ║
    • Total Lines:     58                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
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
