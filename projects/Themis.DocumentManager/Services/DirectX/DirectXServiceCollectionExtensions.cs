using Microsoft.Extensions.DependencyInjection;

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
