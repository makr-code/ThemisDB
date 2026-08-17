$root = (Get-Location).ProviderPath
Get-ChildItem -Path $root -Filter '*.md' -File | ForEach-Object {
    $file = $_.FullName
    $name = $_.Name
    $text = Get-Content -Raw -ErrorAction SilentlyContinue -Path $file
    if ($null -ne $text) {
        if ($text -match '(?i)\b(ai|llm|llama|model|ml|analytics|rag|prompt|embedding|vector)\b' -or $name -match '(?i)ai|llm|llama|ml|analytics|model|prompt|embedding') {
            $matches = [regex]::Matches($text,'(?i)\b(ai|llm|llama|model|ml|analytics|rag|prompt|embedding|vector)\b') | ForEach-Object { $_.Value } | Select-Object -Unique
            $matchList = $matches -join ', '
            Write-Output "$name`t$matchList"
        }
    }
}
