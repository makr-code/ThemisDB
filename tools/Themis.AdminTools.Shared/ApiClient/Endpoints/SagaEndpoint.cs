/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            SagaEndpoint.cs                                    ║
  Version:         0.0.22                                             ║
  Last Modified:   2026-02-21 19:29:20                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     126                                            ║
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

using System.Net.Http;
using System.Net.Http.Json;
using System.Text.Json;
using Themis.AdminTools.Shared.Models;

namespace Themis.AdminTools.Shared.ApiClient.Endpoints;

public class SagaEndpoint
{
    private readonly HttpClient _httpClient;
    private readonly JsonSerializerOptions _jsonOptions;

    public SagaEndpoint(HttpClient httpClient, JsonSerializerOptions jsonOptions)
    {
        _httpClient = httpClient ?? throw new ArgumentNullException(nameof(httpClient));
        _jsonOptions = jsonOptions ?? throw new ArgumentNullException(nameof(jsonOptions));
    }

    public async Task<ApiResponse<SAGABatchListResponse>> GetBatchesAsync(
        CancellationToken cancellationToken = default)
    {
        try
        {
            var response = await _httpClient.GetAsync("/api/saga/batches", cancellationToken);
            if (response.IsSuccessStatusCode)
            {
                var data = await response.Content.ReadFromJsonAsync<SAGABatchListResponse>(_jsonOptions, cancellationToken);
                return new ApiResponse<SAGABatchListResponse> { Success = true, Data = data, StatusCode = (int)response.StatusCode };
            }
            var error = await response.Content.ReadAsStringAsync(cancellationToken);
            return new ApiResponse<SAGABatchListResponse> { Success = false, Error = error, StatusCode = (int)response.StatusCode };
        }
        catch (Exception ex)
        {
            return new ApiResponse<SAGABatchListResponse> { Success = false, Error = ex.Message, StatusCode = 0 };
        }
    }

    public async Task<ApiResponse<SAGABatchDetail>> GetBatchAsync(
        string batchId,
        CancellationToken cancellationToken = default)
    {
        try
        {
            var response = await _httpClient.GetAsync($"/api/saga/batch/{batchId}", cancellationToken);
            if (response.IsSuccessStatusCode)
            {
                var data = await response.Content.ReadFromJsonAsync<SAGABatchDetail>(_jsonOptions, cancellationToken);
                return new ApiResponse<SAGABatchDetail> { Success = true, Data = data, StatusCode = (int)response.StatusCode };
            }
            var error = await response.Content.ReadAsStringAsync(cancellationToken);
            return new ApiResponse<SAGABatchDetail> { Success = false, Error = error, StatusCode = (int)response.StatusCode };
        }
        catch (Exception ex)
        {
            return new ApiResponse<SAGABatchDetail> { Success = false, Error = ex.Message, StatusCode = 0 };
        }
    }

    public async Task<ApiResponse<SAGAVerificationResult>> VerifyBatchAsync(
        string batchId,
        CancellationToken cancellationToken = default)
    {
        try
        {
            var response = await _httpClient.PostAsync($"/api/saga/batch/{batchId}/verify", null, cancellationToken);
            if (response.IsSuccessStatusCode)
            {
                var data = await response.Content.ReadFromJsonAsync<SAGAVerificationResult>(_jsonOptions, cancellationToken);
                return new ApiResponse<SAGAVerificationResult> { Success = true, Data = data, StatusCode = (int)response.StatusCode };
            }
            var error = await response.Content.ReadAsStringAsync(cancellationToken);
            return new ApiResponse<SAGAVerificationResult> { Success = false, Error = error, StatusCode = (int)response.StatusCode };
        }
        catch (Exception ex)
        {
            return new ApiResponse<SAGAVerificationResult> { Success = false, Error = ex.Message, StatusCode = 0 };
        }
    }

    public async Task<ApiResponse<SAGAFlushResponse>> FlushCurrentBatchAsync(
        CancellationToken cancellationToken = default)
    {
        try
        {
            var response = await _httpClient.PostAsync("/api/saga/flush", null, cancellationToken);
            if (response.IsSuccessStatusCode)
            {
                var data = await response.Content.ReadFromJsonAsync<SAGAFlushResponse>(_jsonOptions, cancellationToken);
                return new ApiResponse<SAGAFlushResponse> { Success = true, Data = data, StatusCode = (int)response.StatusCode };
            }
            var error = await response.Content.ReadAsStringAsync(cancellationToken);
            return new ApiResponse<SAGAFlushResponse> { Success = false, Error = error, StatusCode = (int)response.StatusCode };
        }
        catch (Exception ex)
        {
            return new ApiResponse<SAGAFlushResponse> { Success = false, Error = ex.Message, StatusCode = 0 };
        }
    }
}
