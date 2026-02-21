/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            UnrealEngineConnector.cs                           ║
  Version:         0.0.7                                              ║
  Last Modified:   2026-02-21 11:49:35                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     129                                            ║
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

using System.IO.Pipes;
using System.Text;
using System.Text.Json;

namespace Themis.GISViewer.ControlPanel.Services;

public interface IUnrealEngineConnector
{
    Task<bool> ConnectAsync();
    Task DisconnectAsync();
    Task SendCommandAsync(string command, Dictionary<string, object> parameters);
    Task<string> ReceiveDataAsync();
    bool IsConnected { get; }
}

public class UnrealEngineConnector : IUnrealEngineConnector, IDisposable
{
    private NamedPipeClientStream? _pipeClient;
    private readonly string _pipeName;
    private bool _isConnected;

    public UnrealEngineConnector(Microsoft.Extensions.Options.IOptions<UnrealEngineConfiguration> config)
    {
        _pipeName = config.Value.PipeName;
    }

    public bool IsConnected => _isConnected && _pipeClient?.IsConnected == true;

    public async Task<bool> ConnectAsync()
    {
        try
        {
            _pipeClient = new NamedPipeClientStream(".", _pipeName, PipeDirection.InOut, PipeOptions.Asynchronous);
            await _pipeClient.ConnectAsync(5000); // 5 second timeout
            _isConnected = true;
            return true;
        }
        catch (Exception)
        {
            _isConnected = false;
            return false;
        }
    }

    public async Task DisconnectAsync()
    {
        if (_pipeClient != null)
        {
            await _pipeClient.DisposeAsync();
            _pipeClient = null;
        }
        _isConnected = false;
    }

    public async Task SendCommandAsync(string command, Dictionary<string, object> parameters)
    {
        if (!IsConnected || _pipeClient == null)
            throw new InvalidOperationException("Not connected to Unreal Engine");

        var message = new
        {
            Command = command,
            Parameters = parameters,
            Timestamp = DateTime.UtcNow
        };

        var json = JsonSerializer.Serialize(message);
        var bytes = Encoding.UTF8.GetBytes(json);

        // Write length prefix
        await _pipeClient.WriteAsync(BitConverter.GetBytes(bytes.Length), 0, 4);
        // Write message
        await _pipeClient.WriteAsync(bytes, 0, bytes.Length);
        await _pipeClient.FlushAsync();
    }

    public async Task<string> ReceiveDataAsync()
    {
        if (!IsConnected || _pipeClient == null)
            throw new InvalidOperationException("Not connected to Unreal Engine");

        // Read length prefix
        var lengthBytes = new byte[4];
        await _pipeClient.ReadAsync(lengthBytes, 0, 4);
        var length = BitConverter.ToInt32(lengthBytes, 0);

        // Read message
        var buffer = new byte[length];
        var bytesRead = 0;
        while (bytesRead < length)
        {
            bytesRead += await _pipeClient.ReadAsync(buffer, bytesRead, length - bytesRead);
        }

        return Encoding.UTF8.GetString(buffer);
    }

    public void Dispose()
    {
        _pipeClient?.Dispose();
        GC.SuppressFinalize(this);
    }
}
