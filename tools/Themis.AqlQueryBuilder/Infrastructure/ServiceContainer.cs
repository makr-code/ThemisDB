/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            ServiceContainer.cs                                ║
  Version:         0.0.38                                             ║
  Last Modified:   2026-04-13 04:49:05                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     112                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Collections.Generic;

namespace Themis.AqlQueryBuilder.Infrastructure;

/// <summary>
/// Simple manual dependency injection container with no external dependencies
/// Supports singleton and transient service lifetimes
/// </summary>
public class ServiceContainer
{
    private readonly Dictionary<Type, Func<object>> _services = new();
    private readonly Dictionary<Type, object> _singletons = new();
    private readonly object _lock = new();

    /// <summary>
    /// Registers a singleton service
    /// </summary>
    public void RegisterSingleton<TInterface, TImplementation>()
        where TImplementation : TInterface, new()
    {
        _services[typeof(TInterface)] = () =>
        {
            lock (_lock)
            {
                if (!_singletons.ContainsKey(typeof(TInterface)))
                {
                    _singletons[typeof(TInterface)] = new TImplementation();
                }
                return _singletons[typeof(TInterface)];
            }
        };
    }

    /// <summary>
    /// Registers a singleton service with a factory function
    /// </summary>
    public void RegisterSingleton<TInterface>(Func<TInterface> factory)
    {
        _services[typeof(TInterface)] = () =>
        {
            lock (_lock)
            {
                if (!_singletons.ContainsKey(typeof(TInterface)))
                {
                    _singletons[typeof(TInterface)] = factory()!;
                }
                return _singletons[typeof(TInterface)];
            }
        };
    }

    /// <summary>
    /// Registers a transient service (new instance each time)
    /// </summary>
    public void RegisterTransient<TInterface, TImplementation>()
        where TImplementation : TInterface, new()
    {
        _services[typeof(TInterface)] = () => new TImplementation();
    }

    /// <summary>
    /// Registers a transient service with a factory function
    /// </summary>
    public void RegisterTransient<TInterface>(Func<TInterface> factory)
    {
        _services[typeof(TInterface)] = () => factory()!;
    }

    /// <summary>
    /// Resolves a service from the container
    /// </summary>
    public T Resolve<T>()
    {
        if (_services.TryGetValue(typeof(T), out var factory))
        {
            return (T)factory();
        }
        
        throw new InvalidOperationException($"Service of type {typeof(T).Name} not registered");
    }

    /// <summary>
    /// Checks if a service is registered
    /// </summary>
    public bool IsRegistered<T>()
    {
        return _services.ContainsKey(typeof(T));
    }
}
