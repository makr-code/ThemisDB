# Security Issue: Server Crash on Invalid UTF-8 Input

## Summary
ThemisDB HTTP server crashes (SIGSEGV/terminate) when receiving JSON payloads with invalid UTF-8 byte sequences. This is a Denial of Service (DoS) vulnerability.

## Affected Version
- ThemisDB 0.1.0

## Reproduction Steps

1. Start ThemisDB:
```bash
docker run -d -p 8765:8765 themisdb/themisdb:latest
```

2. Send PUT request with invalid UTF-8:
```powershell
$bytes = [System.Text.Encoding]::GetEncoding("Windows-1252").GetBytes('{"name":"Test-Umlaut-ü"}')
$request = [System.Net.WebRequest]::Create("http://localhost:8765/entities/test:1")
$request.Method = "PUT"
$request.ContentType = "application/json"
$request.ContentLength = $bytes.Length
$stream = $request.GetRequestStream()
$stream.Write($bytes, 0, $bytes.Length)
$stream.Close()
$response = $request.GetResponse()
```

Or with curl:
```bash
printf '{"name":"Test-Uml\xc3\xa4ut"}' | curl -X PUT http://localhost:8765/entities/test:1 -d @- -H "Content-Type: application/json"
```

## Expected Behavior
- Server returns HTTP 400 Bad Request
- Request is logged with error detail
- Server continues running

## Actual Behavior
- Server terminates with exit code 139 (SIGSEGV)
- Last log entry: `[json.exception.type_error.316] invalid UTF-8 byte at index 176: 0xFC`
- No graceful error response sent

## Root Cause Analysis
The JSON parser (nlohmann::json) throws an uncaught exception when parsing invalid UTF-8. The HTTP request handler does not wrap the parse operation in a try-catch block.

## Impact
- **Severity**: High (Denial of Service)
- **CVSS Score**: 7.5 (Network-based DoS, no auth required)
- **Attack Vector**: Network, unauthenticated

## Remediation

### Option 1: Validate UTF-8 Before Parsing (Recommended)
```cpp
// In HttpServer::handleRequest()
if (!isValidUtf8(bodyBytes)) {
    sendErrorResponse(400, "Invalid UTF-8 encoding");
    return;
}
nlohmann::json payload = nlohmann::json::parse(bodyBytes);
```

### Option 2: Wrap Parser in Try-Catch
```cpp
try {
    nlohmann::json payload = nlohmann::json::parse(bodyBytes);
} catch (const nlohmann::json::exception& e) {
    sendErrorResponse(400, std::string("JSON parse error: ") + e.what());
    return;
}
```

### Option 3: Configure JSON Parser
Some JSON libraries accept an `allow_exceptions` flag or similar to return error codes instead of throwing.

## Testing Recommendations
1. Add unit tests for invalid UTF-8 payloads
2. Add integration tests for malformed requests
3. Fuzz the `/entities` and `/query` endpoints
4. Add metrics/alerts for parse errors

## Timeline
- **Reported**: 2025-12-10
- **Target Fix**: ASAP (security issue)

## References
- UTF-8 Validation: https://en.wikipedia.org/wiki/UTF-8#Encoding
- nlohmann/json docs: https://github.com/nlohmann/json
- OWASP DoS: https://owasp.org/www-community/attacks/Denial_of_Service

---

**Reporter**: GitHub Copilot (via Themis.DocumentManager project)  
**Repo**: https://github.com/makr-code/ThemisDB
