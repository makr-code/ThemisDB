/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            DynamicAssemblyLoader.cs                           ║
  Version:         0.0.22                                             ║
  Last Modified:   2026-02-21 19:28:58                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   98.0/100                                       ║
    • Total Lines:     181                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 31e8b8df0  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 0d722b04c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;

namespace Themis.DocumentManager.Services;

/// <summary>
/// Service für dynamisches Laden von Assemblies und Typen
/// </summary>
public class DynamicAssemblyLoader
{
    private readonly List<Assembly> _loadedAssemblies = new();
    private readonly string _assemblyDirectory;

    public DynamicAssemblyLoader(string? assemblyDirectory = null)
    {
        _assemblyDirectory = assemblyDirectory ?? Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location) ?? AppDomain.CurrentDomain.BaseDirectory;
    }

    /// <summary>
    /// Lädt alle DLLs aus dem angegebenen Verzeichnis
    /// </summary>
    public void LoadAssembliesFromDirectory(string? directory = null, string pattern = "*.dll")
    {
        var targetDirectory = directory ?? _assemblyDirectory;
        
        if (!Directory.Exists(targetDirectory))
            return;

        var dllFiles = Directory.GetFiles(targetDirectory, pattern, SearchOption.TopDirectoryOnly);
        
        foreach (var dllFile in dllFiles)
        {
            try
            {
                LoadAssembly(dllFile);
            }
            catch (Exception ex)
            {
                // Log error but continue loading other assemblies
                System.Diagnostics.Debug.WriteLine($"Failed to load assembly {dllFile}: {ex.Message}");
            }
        }
    }

    /// <summary>
    /// Lädt eine spezifische Assembly
    /// </summary>
    public Assembly? LoadAssembly(string assemblyPath)
    {
        try
        {
            var assembly = Assembly.LoadFrom(assemblyPath);
            
            if (!_loadedAssemblies.Contains(assembly))
            {
                _loadedAssemblies.Add(assembly);
            }
            
            return assembly;
        }
        catch (Exception ex)
        {
            System.Diagnostics.Debug.WriteLine($"Error loading assembly from {assemblyPath}: {ex.Message}");
            return null;
        }
    }

    /// <summary>
    /// Lädt einen Typ nach Namen aus allen geladenen Assemblies
    /// </summary>
    public Type? LoadType(string typeName, bool fullNameMatch = false)
    {
        foreach (var assembly in _loadedAssemblies)
        {
            try
            {
                var type = fullNameMatch 
                    ? assembly.GetType(typeName)
                    : assembly.GetTypes().FirstOrDefault(t => t.Name == typeName || t.FullName == typeName);
                
                if (type != null)
                    return type;
            }
            catch (ReflectionTypeLoadException ex)
            {
                System.Diagnostics.Debug.WriteLine($"Error loading types from {assembly.FullName}: {ex.Message}");
            }
        }
        
        return null;
    }

    /// <summary>
    /// Findet alle Typen, die ein bestimmtes Interface implementieren
    /// </summary>
    public IEnumerable<Type> FindTypesImplementing<TInterface>()
    {
        var interfaceType = typeof(TInterface);
        var types = new List<Type>();

        foreach (var assembly in _loadedAssemblies)
        {
            try
            {
                var assemblyTypes = assembly.GetTypes()
                    .Where(t => t.IsClass && !t.IsAbstract && interfaceType.IsAssignableFrom(t));
                
                types.AddRange(assemblyTypes);
            }
            catch (ReflectionTypeLoadException ex)
            {
                System.Diagnostics.Debug.WriteLine($"Error loading types from {assembly.FullName}: {ex.Message}");
            }
        }

        return types;
    }

    /// <summary>
    /// Erstellt eine Instanz eines Typs
    /// </summary>
    public object? CreateInstance(Type type, params object[] args)
    {
        try
        {
            return Activator.CreateInstance(type, args);
        }
        catch (Exception ex)
        {
            System.Diagnostics.Debug.WriteLine($"Error creating instance of {type.FullName}: {ex.Message}");
            return null;
        }
    }

    /// <summary>
    /// Gibt alle geladenen Assemblies zurück
    /// </summary>
    public IReadOnlyList<Assembly> LoadedAssemblies => _loadedAssemblies.AsReadOnly();

    /// <summary>
    /// Lädt Assemblies basierend auf einem Präfix (z.B. "Themis.")
    /// </summary>
    public void LoadAssembliesByPrefix(string prefix)
    {
        var dllFiles = Directory.GetFiles(_assemblyDirectory, $"{prefix}*.dll", SearchOption.TopDirectoryOnly);
        
        foreach (var dllFile in dllFiles)
        {
            LoadAssembly(dllFile);
        }
    }
}
