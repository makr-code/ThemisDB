# REST API Example
$body = @{ query = "FOR doc IN users RETURN doc" } | ConvertTo-Json
$response = Invoke-RestMethod -Uri "http://localhost:8765/api/query" -Method POST -Body $body -ContentType "application/json"
$response | ConvertTo-Json
