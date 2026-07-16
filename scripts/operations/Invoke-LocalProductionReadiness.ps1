param(
    [string]$BuildPreset = "windows-release",
    [int]$RepeatCount = 20,
    [string]$OutputRoot = "artifacts/production-readiness",
    [switch]$SkipPhase4Tests,
    [switch]$SkipPentest,
    [switch]$RunPentest,
    [string]$PentestTarget = "",
    [string]$PentestCategory = "all",
    [switch]$SkipContentFocusedTests,
    [switch]$SkipContentCoverage,
    [double]$ContentProcessorCoverageMinPercent = 80.0,
    [switch]$SkipContentBenchmarks,
    [int]$ContentBenchmarkMinCount = 18,
    [int]$ContentFormatBenchmarkMinCount = 20,
    [double]$ContentVersionCreationMaxMs = 50.0,
    [double]$ContentDiffComputationMaxMs = 50.0,
    [double]$ContentVersionRetrievalMaxUs = 5.0,
    [double]$ContentPdfExtractionMaxMs = 500.0,
    [double]$ContentDocxExtractionMaxMs = 500.0,
    [double]$ContentHtmlExtractionMaxMs = 500.0,
    [double]$ContentPlainTextExtractionMaxMs = 500.0,
    [int]$ContentProcessorBenchmarkMinCount = 12,
    [double]$ContentOfficeProcessorPathMaxMs = 750.0,
    [double]$ContentOcrProcessorPathMaxMs = 750.0,
    [double]$ContentArchiveProcessorPathMaxMs = 750.0,
    [switch]$SkipGeoGate,
    [int]$GeoBenchmarkMinCount = 25,
    [switch]$SkipProcessGate,
    [int]$ProcessBenchmarkMinCount = 12,
    [switch]$SkipGpuGate,
    [int]$GpuBenchmarkMinCount = 10,
    [int]$GpuTestFileMinCount = 24,
    [switch]$SkipGraphGate,
    [int]$GraphBenchmarkMinCount = 10,
    [int]$GraphFocusedTestMinCount = 2,
    [switch]$SkipShardingGate,
    [int]$ShardingBenchmarkMinCount = 12,
    [int]$ShardingFocusedTestMinCount = 3,
    [double]$ShardingRoutingOpsPerSecMin = 10000.0,
    [switch]$AllowBetaModules,
    [switch]$SkipOpenApiGate,
    [switch]$NoFailOnGate
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function New-GateResult {
    param(
        [string]$Name,
        [bool]$Passed,
        [string]$Details,
        [string]$Evidence
    )

    [pscustomobject]@{
        name = $Name
        passed = $Passed
        details = $Details
        evidence = $Evidence
    }
}

function Find-BenchmarkExecutable {
    param(
        [string]$BinaryDir,
        [string]$ExecutableBaseName
    )

    $candidates = @(
        (Join-Path $BinaryDir ("cmake/benchmarks/{0}.exe" -f $ExecutableBaseName)),
        (Join-Path $BinaryDir ("benchmarks/{0}.exe" -f $ExecutableBaseName)),
        (Join-Path $BinaryDir ("bin/{0}.exe" -f $ExecutableBaseName))
    )

    foreach ($candidate in $candidates) {
        if (Test-Path $candidate) {
            return $candidate
        }
    }

    if (Test-Path $BinaryDir) {
        $found = Get-ChildItem -Path $BinaryDir -Recurse -Filter ("{0}.exe" -f $ExecutableBaseName) -File -ErrorAction SilentlyContinue | Select-Object -First 1
        if ($found) {
            return $found.FullName
        }
    }

    return ""
}

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
$timestamp = Get-Date -Format "yyyyMMdd_HHmmss"
$outputDir = Join-Path $repoRoot (Join-Path $OutputRoot $timestamp)
New-Item -ItemType Directory -Path $outputDir -Force | Out-Null

$results = New-Object System.Collections.Generic.List[object]

# Gate 0: OpenAPI completeness (API beta-exit support)
$openapiReport = Join-Path $outputDir "openapi-completeness.json"
if (-not $SkipOpenApiGate) {
    $openapiPassed = $false
    $openapiDetails = ""
    $openapiExit = 1
    $pythonCmd = Get-Command python -ErrorAction SilentlyContinue

    if (-not $pythonCmd) {
        $openapiDetails = "python executable not found"
    } else {
        Push-Location $repoRoot
        try {
            $openapiArgs = @(
                "scripts/operations/check_openapi_completeness.py",
                "--repo-root", $repoRoot,
                "--output", $openapiReport
            )

            & python @openapiArgs
            $openapiExit = $LASTEXITCODE
        }
        finally {
            Pop-Location
        }

        $openapiPassed = ($openapiExit -eq 0)
        $openapiDetails = if ($openapiPassed) {
            "All source route hints in src/server are documented in openapi/openapi.yaml"
        } else {
            "Undocumented source route hints found (see report)"
        }
    }

    $results.Add((New-GateResult -Name "openapi-completeness-local" -Passed $openapiPassed -Details $openapiDetails -Evidence $openapiReport))
} else {
    $results.Add((New-GateResult -Name "openapi-completeness-local" -Passed $false -Details "Skipped by user" -Evidence ""))
}

# Gate 1: Content-focused quality evidence
$contentLog = Join-Path $outputDir "content_focused_ctest.log"
$contentJunit = Join-Path $outputDir "content_focused_ctest.junit.xml"
$contentPassed = $false
$contentTotalTests = 0
$contentFailedTests = 0

if (-not $SkipContentFocusedTests) {
    Push-Location $repoRoot
    try {
        $contentRegex = "LegacyOfficeExtractionFocusedTests|LibreOfficeSecurityFocusedTests|AsyncIngestionBackpressureFocusedTests|ContentEmbeddingPipelineFocusedTests"
        $contentArgs = @(
            "--preset", $BuildPreset,
            "-R", $contentRegex,
            "--output-on-failure",
            "--output-junit", $contentJunit
        )

        & ctest @contentArgs *>&1 | Tee-Object -FilePath $contentLog
        $contentExit = $LASTEXITCODE

        if (Test-Path $contentJunit) {
            [xml]$contentXml = Get-Content -Raw $contentJunit
            $tests = 0
            $failures = 0

            $suiteNode = $contentXml.SelectSingleNode("/testsuites")
            if (-not $suiteNode) {
                $suiteNode = $contentXml.SelectSingleNode("/testsuite")
            }

            if ($suiteNode) {
                $testsAttr = $suiteNode.Attributes["tests"]
                $failuresAttr = $suiteNode.Attributes["failures"]
                if ($testsAttr) {
                    $tests = [int]$testsAttr.Value
                }
                if ($failuresAttr) {
                    $failures = [int]$failuresAttr.Value
                }
            }

            $contentTotalTests = $tests
            $contentFailedTests = $failures
        }

        $contentPassed = ($contentExit -eq 0 -and $contentFailedTests -eq 0 -and $contentTotalTests -gt 0)
    }
    finally {
        Pop-Location
    }

    $results.Add((New-GateResult -Name "content-focused-local" -Passed $contentPassed -Details ("Focused content tests; tests={0}; failed={1}" -f $contentTotalTests, $contentFailedTests) -Evidence $contentLog))
} else {
    $results.Add((New-GateResult -Name "content-focused-local" -Passed $false -Details "Skipped by user" -Evidence ""))
}

# Gate 2: Content processor unit-coverage proxy (>80% processors covered by dedicated tests)
$contentCoverageReport = Join-Path $outputDir "content_processor_coverage.json"
if (-not $SkipContentCoverage) {
    $processorToPatterns = [ordered]@{
        "pdf" = @("test_pdf_processor.cpp")
        "office" = @("test_office_processor.cpp")
        "html" = @("test_content_html_processor.cpp")
        "markdown" = @("test_content_markdown_processor.cpp")
        "image" = @("test_image_analysis_interface.cpp", "test_image_analysis_quality.cpp")
        "audio" = @("test_content_audio_processor.cpp")
        "video" = @("test_video_processor_extended.cpp")
        "stt" = @("test_stt_wav_pcm.cpp", "test_stt_diarization.cpp")
        "tts" = @("test_tts_processor.cpp")
        "ocr" = @("test_ocr_processor.cpp")
        "cad" = @("test_cad_processor.cpp", "performance/test_cicada.cpp")
        "geo" = @("test_geo_processor_gdal.cpp")
        "archive" = @("test_archive_processor.cpp")
    }

    $coveredProcessors = New-Object System.Collections.Generic.List[string]
    $missingProcessors = New-Object System.Collections.Generic.List[string]
    $foundEvidence = [ordered]@{}

    foreach ($processor in $processorToPatterns.Keys) {
        $patterns = $processorToPatterns[$processor]
        $matched = New-Object System.Collections.Generic.List[string]

        foreach ($pattern in $patterns) {
            $candidate = Join-Path $repoRoot (Join-Path "tests" $pattern)
            if (Test-Path -LiteralPath $candidate) {
                $matched.Add($candidate)
            }
        }

        if ($matched.Count -gt 0) {
            $coveredProcessors.Add($processor)
            $foundEvidence[$processor] = @($matched)
        } else {
            $missingProcessors.Add($processor)
            $foundEvidence[$processor] = @()
        }
    }

    $totalProcessors = $processorToPatterns.Count
    $coveredCount = $coveredProcessors.Count
    $coveragePercent = if ($totalProcessors -gt 0) { ($coveredCount * 100.0 / $totalProcessors) } else { 0.0 }
    $coveragePassed = $coveragePercent -ge $ContentProcessorCoverageMinPercent

    $coverageDoc = [ordered]@{
        generated_at = (Get-Date).ToString("o")
        threshold_percent = $ContentProcessorCoverageMinPercent
        covered_processors = @($coveredProcessors)
        missing_processors = @($missingProcessors)
        covered_count = $coveredCount
        total_processors = $totalProcessors
        coverage_percent = [Math]::Round($coveragePercent, 3)
        evidence = $foundEvidence
    }

    $coverageDoc | ConvertTo-Json -Depth 6 | Set-Content -Path $contentCoverageReport -Encoding UTF8

    $results.Add((New-GateResult -Name "content-processor-coverage-local" -Passed $coveragePassed -Details ("Processor coverage={0:N2}% ({1}/{2}); threshold={3:N2}%" -f $coveragePercent, $coveredCount, $totalProcessors, $ContentProcessorCoverageMinPercent) -Evidence $contentCoverageReport))
} else {
    $results.Add((New-GateResult -Name "content-processor-coverage-local" -Passed $false -Details "Skipped by user" -Evidence ""))
}

# Gate 3: Content benchmark evidence (Issue #1699)
$contentBenchBuildLog = Join-Path $outputDir "content_bench_build.log"
$contentBenchRunLog = Join-Path $outputDir "content_bench_run.log"
$contentBenchTextRunLog = Join-Path $outputDir "content_bench_text_run.log"
$contentBenchProcessorRunLog = Join-Path $outputDir "content_bench_processor_run.log"
$contentBenchJson = Join-Path $outputDir "content_bench_content_versioning.json"
$contentBenchTextJson = Join-Path $outputDir "content_bench_text_extraction.json"
$contentBenchProcessorJson = Join-Path $outputDir "content_bench_processor_paths.json"
$contentBenchPassed = $false
$contentBenchDetails = ""
$contentBenchEvidence = ""

if (-not $SkipContentBenchmarks) {
    Push-Location $repoRoot
    try {
        & cmake --build --preset $BuildPreset --target bench_content_versioning bench_text_extraction bench_content_processor_paths *>&1 | Tee-Object -FilePath $contentBenchBuildLog
        $buildExit = $LASTEXITCODE

        if ($buildExit -ne 0) {
            $contentBenchDetails = "Build failed for content benchmarks (exit=$buildExit)"
            $contentBenchEvidence = $contentBenchBuildLog
        } else {
            $binaryDir = Join-Path $repoRoot ("build-" + $BuildPreset)
            $versionBenchExe = Find-BenchmarkExecutable -BinaryDir $binaryDir -ExecutableBaseName "bench_content_versioning"
            $textBenchExe = Find-BenchmarkExecutable -BinaryDir $binaryDir -ExecutableBaseName "bench_text_extraction"
            $processorBenchExe = Find-BenchmarkExecutable -BinaryDir $binaryDir -ExecutableBaseName "bench_content_processor_paths"

            if ([string]::IsNullOrWhiteSpace($versionBenchExe) -or [string]::IsNullOrWhiteSpace($textBenchExe) -or [string]::IsNullOrWhiteSpace($processorBenchExe)) {
                $contentBenchDetails = "Benchmark executable not found after searching expected build output locations"
                $contentBenchEvidence = $contentBenchBuildLog
            } else {
                $oldPath = $env:PATH
                try {
                    $dllPathCandidates = @(
                        (Join-Path $binaryDir "bin"),
                        (Join-Path $binaryDir "cmake")
                    ) | Where-Object { Test-Path $_ }

                    if (@($dllPathCandidates).Count -gt 0) {
                        $env:PATH = ((@($dllPathCandidates) -join ";") + ";" + $env:PATH)
                    }

                    & $versionBenchExe "--benchmark_min_time=0.01s" "--benchmark_repetitions=1" "--benchmark_format=json" "--benchmark_out=$contentBenchJson" *>&1 | Tee-Object -FilePath $contentBenchRunLog
                    $runExitVersion = $LASTEXITCODE

                    & $textBenchExe "--benchmark_min_time=0.01s" "--benchmark_repetitions=1" "--benchmark_format=json" "--benchmark_out=$contentBenchTextJson" *>&1 | Tee-Object -FilePath $contentBenchTextRunLog
                    $runExitText = $LASTEXITCODE

                    & $processorBenchExe "--benchmark_min_time=0.01s" "--benchmark_repetitions=1" "--benchmark_format=json" "--benchmark_out=$contentBenchProcessorJson" *>&1 | Tee-Object -FilePath $contentBenchProcessorRunLog
                    $runExitProcessor = $LASTEXITCODE
                }
                finally {
                    $env:PATH = $oldPath
                }

                if (($runExitVersion -eq 0) -and ($runExitText -eq 0) -and ($runExitProcessor -eq 0) -and (Test-Path $contentBenchJson) -and (Test-Path $contentBenchTextJson) -and (Test-Path $contentBenchProcessorJson)) {
                    try {
                        $benchDoc = Get-Content -Raw $contentBenchJson | ConvertFrom-Json
                        $benchmarkCount = @($benchDoc.benchmarks).Count

                        $textBenchDoc = Get-Content -Raw $contentBenchTextJson | ConvertFrom-Json
                        $textBenchmarkCount = @($textBenchDoc.benchmarks).Count

                        $processorBenchDoc = Get-Content -Raw $contentBenchProcessorJson | ConvertFrom-Json
                        $processorBenchmarkCount = @($processorBenchDoc.benchmarks).Count

                        $v1m = $benchDoc.benchmarks | Where-Object { $_.name -eq "BM_VersionCreation/1048576" } | Select-Object -First 1
                        $diff1m = $benchDoc.benchmarks | Where-Object { $_.name -eq "BM_DiffComputation/1048576" } | Select-Object -First 1
                        $retrieval = $benchDoc.benchmarks | Where-Object { $_.name -eq "BM_VersionRetrieval" } | Select-Object -First 1

                        $pdf1m = $textBenchDoc.benchmarks | Where-Object { $_.name -eq "BM_PDFExtraction/1048576" } | Select-Object -First 1
                        $docx1m = $textBenchDoc.benchmarks | Where-Object { $_.name -eq "BM_DOCXExtraction/1048576" } | Select-Object -First 1
                        $html1m = $textBenchDoc.benchmarks | Where-Object { $_.name -eq "BM_HTMLExtraction/1048576" } | Select-Object -First 1
                        $plain1m = $textBenchDoc.benchmarks | Where-Object { $_.name -eq "BM_PlainTextExtraction/1048576" } | Select-Object -First 1

                        $office1m = $processorBenchDoc.benchmarks | Where-Object { $_.name -eq "BM_OfficeProcessorPath/1048576" } | Select-Object -First 1
                        $ocr1m = $processorBenchDoc.benchmarks | Where-Object { $_.name -eq "BM_OcrProcessorPath/1048576" } | Select-Object -First 1
                        $archive1m = $processorBenchDoc.benchmarks | Where-Object { $_.name -eq "BM_ArchiveProcessorPath/1048576" } | Select-Object -First 1

                        $v1mMs = if ($v1m) { [double]$v1m.real_time / 1e6 } else { [double]::PositiveInfinity }
                        $diff1mMs = if ($diff1m) { [double]$diff1m.real_time / 1e6 } else { [double]::PositiveInfinity }
                        $retrievalUs = if ($retrieval) { [double]$retrieval.real_time / 1e3 } else { [double]::PositiveInfinity }

                        $pdf1mMs = if ($pdf1m) { [double]$pdf1m.real_time / 1e6 } else { [double]::PositiveInfinity }
                        $docx1mMs = if ($docx1m) { [double]$docx1m.real_time / 1e6 } else { [double]::PositiveInfinity }
                        $html1mMs = if ($html1m) { [double]$html1m.real_time / 1e6 } else { [double]::PositiveInfinity }
                        $plain1mMs = if ($plain1m) { [double]$plain1m.real_time / 1e6 } else { [double]::PositiveInfinity }

                        $office1mMs = if ($office1m) { [double]$office1m.real_time / 1e6 } else { [double]::PositiveInfinity }
                        $ocr1mMs = if ($ocr1m) { [double]$ocr1m.real_time / 1e6 } else { [double]::PositiveInfinity }
                        $archive1mMs = if ($archive1m) { [double]$archive1m.real_time / 1e6 } else { [double]::PositiveInfinity }

                        $hasKeyBenchmarks = ($v1m -ne $null -and $diff1m -ne $null -and $retrieval -ne $null)
                        $hasFormatBenchmarks = ($pdf1m -ne $null -and $docx1m -ne $null -and $html1m -ne $null -and $plain1m -ne $null)
                        $hasProcessorBenchmarks = ($office1m -ne $null -and $ocr1m -ne $null -and $archive1m -ne $null)
                        $withinThresholds = (
                            $v1mMs -le $ContentVersionCreationMaxMs -and
                            $diff1mMs -le $ContentDiffComputationMaxMs -and
                            $retrievalUs -le $ContentVersionRetrievalMaxUs
                        )
                        $withinFormatThresholds = (
                            $pdf1mMs -le $ContentPdfExtractionMaxMs -and
                            $docx1mMs -le $ContentDocxExtractionMaxMs -and
                            $html1mMs -le $ContentHtmlExtractionMaxMs -and
                            $plain1mMs -le $ContentPlainTextExtractionMaxMs
                        )
                        $withinProcessorThresholds = (
                            $office1mMs -le $ContentOfficeProcessorPathMaxMs -and
                            $ocr1mMs -le $ContentOcrProcessorPathMaxMs -and
                            $archive1mMs -le $ContentArchiveProcessorPathMaxMs
                        )

                        $contentBenchPassed = (
                            $benchmarkCount -ge $ContentBenchmarkMinCount -and
                            $textBenchmarkCount -ge $ContentFormatBenchmarkMinCount -and
                            $processorBenchmarkCount -ge $ContentProcessorBenchmarkMinCount -and
                            $hasKeyBenchmarks -and
                            $hasFormatBenchmarks -and
                            $hasProcessorBenchmarks -and
                            $withinThresholds -and
                            $withinFormatThresholds -and
                            $withinProcessorThresholds
                        )
                        $contentBenchDetails = (
                            "bench_content_versioning count={0} (min={1}); bench_text_extraction count={2} (min={3}); bench_content_processor_paths count={4} (min={5}); " +
                            "VersionCreation1MiB={6:N3}ms<= {7:N3}ms; Diff1MiB={8:N3}ms<= {9:N3}ms; Retrieval={10:N3}us<= {11:N3}us; " +
                            "PDF1MiB={12:N3}ms<= {13:N3}ms; DOCX1MiB={14:N3}ms<= {15:N3}ms; HTML1MiB={16:N3}ms<= {17:N3}ms; Plain1MiB={18:N3}ms<= {19:N3}ms; " +
                            "OfficePath1MiB={20:N3}ms<= {21:N3}ms; OcrPath1MiB={22:N3}ms<= {23:N3}ms; ArchivePath1MiB={24:N3}ms<= {25:N3}ms"
                        ) -f $benchmarkCount, $ContentBenchmarkMinCount, $textBenchmarkCount, $ContentFormatBenchmarkMinCount, $processorBenchmarkCount, $ContentProcessorBenchmarkMinCount, $v1mMs, $ContentVersionCreationMaxMs, $diff1mMs, $ContentDiffComputationMaxMs, $retrievalUs, $ContentVersionRetrievalMaxUs, $pdf1mMs, $ContentPdfExtractionMaxMs, $docx1mMs, $ContentDocxExtractionMaxMs, $html1mMs, $ContentHtmlExtractionMaxMs, $plain1mMs, $ContentPlainTextExtractionMaxMs, $office1mMs, $ContentOfficeProcessorPathMaxMs, $ocr1mMs, $ContentOcrProcessorPathMaxMs, $archive1mMs, $ContentArchiveProcessorPathMaxMs
                        $contentBenchEvidence = $contentBenchProcessorJson
                    }
                    catch {
                        $contentBenchDetails = "Benchmark JSON parse failed: $($_.Exception.Message)"
                        $contentBenchEvidence = $contentBenchProcessorRunLog
                    }
                } else {
                    $contentBenchDetails = "Benchmark run failed (bench_content_versioning exit=$runExitVersion; bench_text_extraction exit=$runExitText; bench_content_processor_paths exit=$runExitProcessor)"
                    $contentBenchEvidence = $contentBenchProcessorRunLog
                }
            }
        }
    }
    finally {
        Pop-Location
    }

    $results.Add((New-GateResult -Name "content-benchmark-local" -Passed $contentBenchPassed -Details $contentBenchDetails -Evidence $contentBenchEvidence))
} else {
    $results.Add((New-GateResult -Name "content-benchmark-local" -Passed $false -Details "Skipped by user" -Evidence ""))
}

# Gate 4: Geo readiness (focused test evidence + CPU/GPU parity benchmark)
$geoBenchBuildLog = Join-Path $outputDir "geo_bench_build.log"
$geoBenchRunLog = Join-Path $outputDir "geo_bench_run.log"
$geoParityJson = Join-Path $outputDir "geo_cpu_gpu_parity.json"

if (-not $SkipGeoGate) {
    $requiredGeoFiles = @(
        (Join-Path $repoRoot "tests/geo/test_geo_st_buffer.cpp"),
        (Join-Path $repoRoot "tests/geo/test_geo_st_union_difference.cpp"),
        (Join-Path $repoRoot "tests/geo/test_geo_clustering.cpp"),
        (Join-Path $repoRoot "tests/geo/test_geo_wgs84_spherical.cpp"),
        (Join-Path $repoRoot "benchmarks/bench_geo_cpu_gpu.cpp"),
        (Join-Path $repoRoot "src/geo/ROADMAP.md"),
        (Join-Path $repoRoot "docs/de/geo/README.md")
    )

    $missingGeoFiles = @($requiredGeoFiles | Where-Object { -not (Test-Path -LiteralPath $_) })
    if ($missingGeoFiles.Count -gt 0) {
        $results.Add((New-GateResult -Name "geo-readiness-local" -Passed $false -Details ("Missing required geo evidence files: {0}" -f ($missingGeoFiles -join "; ")) -Evidence ""))
    } else {
        Push-Location $repoRoot
        try {
            & cmake --build --preset $BuildPreset --target bench_geo_cpu_gpu *>&1 | Tee-Object -FilePath $geoBenchBuildLog
            $geoBuildExit = $LASTEXITCODE

            if ($geoBuildExit -ne 0) {
                $results.Add((New-GateResult -Name "geo-readiness-local" -Passed $false -Details ("Build failed for bench_geo_cpu_gpu (exit={0})" -f $geoBuildExit) -Evidence $geoBenchBuildLog))
            } else {
                $binaryDir = Join-Path $repoRoot ("build-" + $BuildPreset)
                $geoBenchExe = Find-BenchmarkExecutable -BinaryDir $binaryDir -ExecutableBaseName "bench_geo_cpu_gpu"

                if ([string]::IsNullOrWhiteSpace($geoBenchExe)) {
                    $results.Add((New-GateResult -Name "geo-readiness-local" -Passed $false -Details "bench_geo_cpu_gpu executable not found" -Evidence $geoBenchBuildLog))
                } else {
                    $oldPath = $env:PATH
                    try {
                        $dllPathCandidates = @(
                            (Join-Path $binaryDir "bin"),
                            (Join-Path $binaryDir "cmake")
                        ) | Where-Object { Test-Path $_ }

                        if (@($dllPathCandidates).Count -gt 0) {
                            $env:PATH = ((@($dllPathCandidates) -join ";") + ";" + $env:PATH)
                        }

                        & $geoBenchExe "--benchmark_min_time=0.01s" "--benchmark_repetitions=1" "--benchmark_format=json" "--benchmark_out=$geoParityJson" *>&1 | Tee-Object -FilePath $geoBenchRunLog
                        $geoRunExit = $LASTEXITCODE
                    }
                    finally {
                        $env:PATH = $oldPath
                    }

                    if (($geoRunExit -eq 0) -and (Test-Path $geoParityJson)) {
                        try {
                            $geoDoc = Get-Content -Raw $geoParityJson | ConvertFrom-Json
                            $geoCount = @($geoDoc.benchmarks).Count

                            $hasCpuGpuStBuffer = (
                                ($geoDoc.benchmarks | Where-Object { $_.name -like "BM_GeoCPUExact_StBuffer*" } | Select-Object -First 1) -ne $null -and
                                ($geoDoc.benchmarks | Where-Object { $_.name -like "BM_GeoGPU_StBuffer*" } | Select-Object -First 1) -ne $null
                            )
                            $hasCpuGpuIntersects = (
                                ($geoDoc.benchmarks | Where-Object { $_.name -like "BM_GeoCPUExact_ExactIntersects*" } | Select-Object -First 1) -ne $null -and
                                ($geoDoc.benchmarks | Where-Object { $_.name -like "BM_GeoGPU_ExactIntersects*" } | Select-Object -First 1) -ne $null
                            )
                            $hasCpuGpuDistance = (
                                ($geoDoc.benchmarks | Where-Object { $_.name -like "BM_GeoCPUExact_GeodesicDistance*" } | Select-Object -First 1) -ne $null -and
                                ($geoDoc.benchmarks | Where-Object { $_.name -like "BM_GeoGPU_GeodesicDistance*" } | Select-Object -First 1) -ne $null
                            )

                            $geoPassed = ($geoCount -ge $GeoBenchmarkMinCount -and $hasCpuGpuStBuffer -and $hasCpuGpuIntersects -and $hasCpuGpuDistance)
                            $geoDetails = (
                                "bench_geo_cpu_gpu count={0} (min={1}); CPU/GPU pairs present: ST_BUFFER={2}, exactIntersects={3}, geodesicDistance={4}"
                            ) -f $geoCount, $GeoBenchmarkMinCount, $hasCpuGpuStBuffer, $hasCpuGpuIntersects, $hasCpuGpuDistance

                            $results.Add((New-GateResult -Name "geo-readiness-local" -Passed $geoPassed -Details $geoDetails -Evidence $geoParityJson))
                        }
                        catch {
                            $results.Add((New-GateResult -Name "geo-readiness-local" -Passed $false -Details ("Failed to parse geo parity JSON: {0}" -f $_.Exception.Message) -Evidence $geoBenchRunLog))
                        }
                    } else {
                        $results.Add((New-GateResult -Name "geo-readiness-local" -Passed $false -Details ("bench_geo_cpu_gpu run failed (exit={0})" -f $geoRunExit) -Evidence $geoBenchRunLog))
                    }
                }
            }
        }
        finally {
            Pop-Location
        }
    }
} else {
    $results.Add((New-GateResult -Name "geo-readiness-local" -Passed $false -Details "Skipped by user" -Evidence ""))
}

# Gate 5: Process readiness (embedding pipeline + retrieval API + security audit)
$processBenchBuildLog = Join-Path $outputDir "process_bench_build.log"
$processBenchRunLog   = Join-Path $outputDir "process_bench_run.log"
$processRetrievalJson = Join-Path $outputDir "process_retrieval_bench.json"
$processApiReport     = Join-Path $outputDir "process_api_check.json"

if (-not $SkipProcessGate) {
    $requiredProcessFiles = @(
        (Join-Path $repoRoot "src/process/process_model_manager.cpp"),
        (Join-Path $repoRoot "include/process/process_model_manager.h"),
        (Join-Path $repoRoot "include/process/AUDIT.md"),
        (Join-Path $repoRoot "src/process/AUDIT.md"),
        (Join-Path $repoRoot "include/process/SECURITY.md"),
        (Join-Path $repoRoot "tests/test_process_module.cpp"),
        (Join-Path $repoRoot "benchmarks/bench_process_retrieval.cpp")
    )

    $missingProcessFiles = @($requiredProcessFiles | Where-Object { -not (Test-Path -LiteralPath $_) })
    if ($missingProcessFiles.Count -gt 0) {
        $results.Add((New-GateResult -Name "process-readiness-local" -Passed $false -Details ("Missing required process evidence files: {0}" -f ($missingProcessFiles -join "; ")) -Evidence ""))
    } else {
        # Verify embedding + retrieval APIs are present in ProcessModelManager header
        $headerPath    = Join-Path $repoRoot "include/process/process_model_manager.h"
        $auditPath     = Join-Path $repoRoot "src/process/AUDIT.md"
        $secPath       = Join-Path $repoRoot "src/process/SECURITY.md"
        $headerContent = Get-Content -Path $headerPath -Raw
        $auditContent  = Get-Content -Path $auditPath  -Raw
        $secContent    = Get-Content -Path $secPath    -Raw

        $hasEmbeddingField      = $headerContent -match '\bembedding\b'
        $hasFindSimilar         = $headerContent -match '\bfindSimilar\b'
        $hasSearch              = $headerContent -match '\bsearch\b'
        $hasEmbeddingAuditItem  = $auditContent  -match 'PROC-OPEN-01'
        $hasSecurityItems       = $secContent    -match 'PROC-SEC'

        $apiDoc = [ordered]@{
            generated_at                  = (Get-Date).ToString("o")
            embedding_field_present       = $hasEmbeddingField
            find_similar_api_present      = $hasFindSimilar
            full_text_search_api_present  = $hasSearch
            embedding_audit_item_tracked  = $hasEmbeddingAuditItem
            security_items_tracked        = $hasSecurityItems
        }
        $apiDoc | ConvertTo-Json -Depth 3 | Set-Content -Path $processApiReport -Encoding UTF8

        if (-not $hasEmbeddingField -or -not $hasFindSimilar -or -not $hasSearch) {
            $missing = @()
            if (-not $hasEmbeddingField) { $missing += "embedding field" }
            if (-not $hasFindSimilar)    { $missing += "findSimilar API" }
            if (-not $hasSearch)         { $missing += "search API" }
            $results.Add((New-GateResult -Name "process-readiness-local" -Passed $false -Details ("Missing process retrieval APIs: {0}" -f ($missing -join ", ")) -Evidence $processApiReport))
        } else {
            # Build bench_process_retrieval
            Push-Location $repoRoot
            try {
                & cmake --build --preset $BuildPreset --target bench_process_retrieval *>&1 | Tee-Object -FilePath $processBenchBuildLog
                $processBuildExit = $LASTEXITCODE

                if ($processBuildExit -ne 0) {
                    $results.Add((New-GateResult -Name "process-readiness-local" -Passed $false -Details ("Build failed for bench_process_retrieval (exit={0})" -f $processBuildExit) -Evidence $processBenchBuildLog))
                } else {
                    $binaryDir       = Join-Path $repoRoot ("build-" + $BuildPreset)
                    $processBenchExe = Find-BenchmarkExecutable -BinaryDir $binaryDir -ExecutableBaseName "bench_process_retrieval"

                    if ([string]::IsNullOrWhiteSpace($processBenchExe)) {
                        $results.Add((New-GateResult -Name "process-readiness-local" -Passed $false -Details "bench_process_retrieval executable not found" -Evidence $processBenchBuildLog))
                    } else {
                        $oldPath = $env:PATH
                        try {
                            $dllCandidates = @(
                                (Join-Path $binaryDir "bin"),
                                (Join-Path $binaryDir "cmake")
                            ) | Where-Object { Test-Path $_ }
                            if (@($dllCandidates).Count -gt 0) {
                                $env:PATH = ((@($dllCandidates) -join ";") + ";" + $env:PATH)
                            }

                            & $processBenchExe "--benchmark_min_time=0.01s" "--benchmark_repetitions=1" "--benchmark_format=json" "--benchmark_out=$processRetrievalJson" *>&1 | Tee-Object -FilePath $processBenchRunLog
                            $processRunExit = $LASTEXITCODE
                        }
                        finally {
                            $env:PATH = $oldPath
                        }

                        if (($processRunExit -eq 0) -and (Test-Path $processRetrievalJson)) {
                            try {
                                $processDoc   = Get-Content -Raw $processRetrievalJson | ConvertFrom-Json
                                $processCount = @($processDoc.benchmarks).Count

                                $hasEmbedGenBench  = ($processDoc.benchmarks | Where-Object { $_.name -like "BM_ProcessEmbeddingGenerate*"  } | Select-Object -First 1) -ne $null
                                $hasHnswBench      = ($processDoc.benchmarks | Where-Object { $_.name -like "BM_ProcessHnswRetrieve*"         } | Select-Object -First 1) -ne $null
                                $hasFullTextBench  = ($processDoc.benchmarks | Where-Object { $_.name -like "BM_ProcessFullTextSearch*"        } | Select-Object -First 1) -ne $null
                                $hasStateChangeBench = ($processDoc.benchmarks | Where-Object { $_.name -like "BM_ProcessStateChangeEmbed*"   } | Select-Object -First 1) -ne $null

                                $processGatePassed = (
                                    $processCount      -ge $ProcessBenchmarkMinCount -and
                                    $hasEmbedGenBench  -and
                                    $hasHnswBench      -and
                                    $hasFullTextBench
                                )

                                $processDetails = (
                                    "bench_process_retrieval count={0} (min={1}); " +
                                    "embedding_field=present; findSimilar_api=present; search_api=present; " +
                                    "BM_ProcessEmbeddingGenerate={2}; BM_ProcessHnswRetrieve={3}; " +
                                    "BM_ProcessFullTextSearch={4}; BM_ProcessStateChangeEmbed={5}; " +
                                    "security_items_tracked={6}; embedding_audit_tracked={7}"
                                ) -f $processCount, $ProcessBenchmarkMinCount,
                                     $hasEmbedGenBench, $hasHnswBench,
                                     $hasFullTextBench, $hasStateChangeBench,
                                     $hasSecurityItems, $hasEmbeddingAuditItem

                                $results.Add((New-GateResult -Name "process-readiness-local" -Passed $processGatePassed -Details $processDetails -Evidence $processRetrievalJson))
                            }
                            catch {
                                $results.Add((New-GateResult -Name "process-readiness-local" -Passed $false -Details ("Failed to parse process retrieval JSON: {0}" -f $_.Exception.Message) -Evidence $processBenchRunLog))
                            }
                        } else {
                            $results.Add((New-GateResult -Name "process-readiness-local" -Passed $false -Details ("bench_process_retrieval run failed (exit={0})" -f $processRunExit) -Evidence $processBenchRunLog))
                        }
                    }
                }
            }
            finally {
                Pop-Location
            }
        }
    }
} else {
    $results.Add((New-GateResult -Name "process-readiness-local" -Passed $false -Details "Skipped by user" -Evidence ""))
}

# ─────────────────────────────────────────────────────────────────────────────
# Gate: gpu-readiness-local
# Evidence for Issue #1800 (P2P transfer), #1802 (NVLink scheduling), #1805 (coverage > 80%)
# ─────────────────────────────────────────────────────────────────────────────

if (-not $SkipGpuGate) {

    # 1. Required evidence files
    $requiredGpuFiles = @(
        "src/gpu/p2p_transfer.cpp",
        "include/themis/gpu/p2p_transfer.h",
        "tests/test_gpu_p2p_transfer.cpp",
        "src/gpu/load_balancer.cpp",
        "include/themis/gpu/load_balancer.h",
        "tests/test_gpu_load_balancer.cpp",
        "src/gpu/cluster_topology.cpp",
        "include/themis/gpu/cluster_topology.h",
        "src/gpu/AUDIT.md",
        "src/gpu/ROADMAP.md"
    )
    $missingGpuFiles = @($requiredGpuFiles | Where-Object { -not (Test-Path (Join-Path $repoRoot $_)) })

    if ($missingGpuFiles.Count -gt 0) {
        $results.Add((New-GateResult -Name "gpu-readiness-local" -Passed $false `
            -Details ("Missing GPU evidence files: {0}" -f ($missingGpuFiles -join "; ")) -Evidence ""))
    } else {

        # 2. API surface checks
        $loadBalancerHeader = Join-Path $repoRoot "include/themis/gpu/load_balancer.h"
        $p2pHeader          = Join-Path $repoRoot "include/themis/gpu/p2p_transfer.h"

        $lbContent  = Get-Content -Raw $loadBalancerHeader
        $p2pContent = Get-Content -Raw $p2pHeader

        $missingApis = [System.Collections.Generic.List[string]]::new()
        if ($lbContent -notmatch 'TOPOLOGY_AWARE')  { $missingApis.Add("GPULoadBalancer::TOPOLOGY_AWARE (#1802)") }
        if ($p2pContent -notmatch 'PEER_TO_PEER|P2PTransfer|GPUP2PTransfer') { $missingApis.Add("GPUP2PTransferManager (#1800)") }

        $gpuApiReport = Join-Path $outputDir "gpu_api_check.json"
        @{
            topology_aware_present  = ($lbContent -match 'TOPOLOGY_AWARE')
            p2p_transfer_present    = ($p2pContent -match 'PEER_TO_PEER|P2PTransfer|GPUP2PTransfer')
            checked_at              = (Get-Date -Format "o")
        } | ConvertTo-Json | Set-Content -Path $gpuApiReport -Encoding UTF8

        if ($missingApis.Count -gt 0) {
            $results.Add((New-GateResult -Name "gpu-readiness-local" -Passed $false `
                -Details ("Missing GPU APIs: {0}" -f ($missingApis -join ", ")) -Evidence $gpuApiReport))
        } else {

            # 3. Test-file coverage check (>= 80% of src files)
            $gpuSrcCount  = @(Get-ChildItem (Join-Path $repoRoot "src/gpu") -Filter "*.cpp" -Recurse -ErrorAction SilentlyContinue).Count
            $gpuTestCount = @(Get-ChildItem (Join-Path $repoRoot "tests")    -Filter "test_gpu*.cpp" -Recurse -ErrorAction SilentlyContinue).Count
            $coverageOk   = ($gpuTestCount -ge $GpuTestFileMinCount) -and ($gpuSrcCount -gt 0 -and $gpuTestCount -ge [int][Math]::Ceiling($gpuSrcCount * 0.80))

            $gpuCovReport = Join-Path $outputDir "gpu_coverage_check.json"
            @{
                src_file_count  = $gpuSrcCount
                test_file_count = $gpuTestCount
                min_required    = $GpuTestFileMinCount
                threshold_80pct = [int][Math]::Ceiling($gpuSrcCount * 0.80)
                coverage_ok     = $coverageOk
                checked_at      = (Get-Date -Format "o")
            } | ConvertTo-Json | Set-Content -Path $gpuCovReport -Encoding UTF8

            if (-not $coverageOk) {
                $results.Add((New-GateResult -Name "gpu-readiness-local" -Passed $false `
                    -Details ("GPU test coverage insufficient: {0} tests / {1} src files (need >= 80%, min={2})" -f $gpuTestCount, $gpuSrcCount, $GpuTestFileMinCount) `
                    -Evidence $gpuCovReport))
            } else {

                # 4. Build + run bench_gpu_hardware_capability
                $gpuBenchBuildLog = Join-Path $outputDir "gpu_bench_build.log"
                $gpuBenchRunLog   = Join-Path $outputDir "gpu_bench_run.log"
                $gpuCapabilityJson = Join-Path $outputDir "gpu_hardware_capability.json"
                $binaryDir = Join-Path $repoRoot ("build-" + $BuildPreset)

                Push-Location $repoRoot
                try {
                    # Build
                    & cmake --build --preset $BuildPreset --target bench_gpu_hardware_capability *>&1 | Tee-Object -FilePath $gpuBenchBuildLog
                    $gpuBuildExit = $LASTEXITCODE

                    if ($gpuBuildExit -ne 0) {
                        $results.Add((New-GateResult -Name "gpu-readiness-local" -Passed $false `
                            -Details ("Build failed for bench_gpu_hardware_capability (exit={0})" -f $gpuBuildExit) `
                            -Evidence $gpuBenchBuildLog))
                    } else {
                        $gpuExe = Find-BenchmarkExecutable -BinaryDir $binaryDir -ExecutableBaseName "bench_gpu_hardware_capability"

                        if (-not $gpuExe) {
                            $results.Add((New-GateResult -Name "gpu-readiness-local" -Passed $false `
                                -Details "bench_gpu_hardware_capability executable not found" -Evidence $gpuBenchBuildLog))
                        } else {
                            # Run
                            & $gpuExe --benchmark_format=json --benchmark_out=$gpuCapabilityJson *>&1 | Tee-Object -FilePath $gpuBenchRunLog
                            $gpuRunExit = $LASTEXITCODE

                            if ($gpuRunExit -eq 0 -and (Test-Path $gpuCapabilityJson)) {
                                try {
                                    $gpuJson   = Get-Content -Raw $gpuCapabilityJson | ConvertFrom-Json
                                    $benchNames = @($gpuJson.benchmarks | ForEach-Object { $_.name })
                                    $benchCount = $benchNames.Count

                                    $requiredBenches = @(
                                        "BM_GpuP2PTransfer_CPUFallback",
                                        "BM_GpuNVLinkTopologyDetect",
                                        "BM_GpuNVLinkScheduleSelect"
                                    )
                                    $missingBenches = @($requiredBenches | Where-Object { $name = $_; -not ($benchNames | Where-Object { $_ -like "$name*" }) })

                                    $gpuGatePassed = ($benchCount -ge $GpuBenchmarkMinCount) -and (@($missingBenches).Count -eq 0)
                                    $gpuDetails    = ("GPU benchmark: {0} entries (min={1}); P2P #1800 present={2}; NVLink #1802 present={3}; coverage={4}/{5}" -f `
                                        $benchCount, $GpuBenchmarkMinCount, `
                                        (-not ($missingBenches -contains "BM_GpuP2PTransfer_CPUFallback")), `
                                        (-not ($missingBenches -contains "BM_GpuNVLinkScheduleSelect")), `
                                        $gpuTestCount, $gpuSrcCount)

                                    $results.Add((New-GateResult -Name "gpu-readiness-local" -Passed $gpuGatePassed `
                                        -Details $gpuDetails -Evidence $gpuCapabilityJson))
                                } catch {
                                    $results.Add((New-GateResult -Name "gpu-readiness-local" -Passed $false `
                                        -Details ("Failed to parse GPU benchmark JSON: {0}" -f $_.Exception.Message) -Evidence $gpuBenchRunLog))
                                }
                            } else {
                                $results.Add((New-GateResult -Name "gpu-readiness-local" -Passed $false `
                                    -Details ("bench_gpu_hardware_capability run failed (exit={0})" -f $gpuRunExit) -Evidence $gpuBenchRunLog))
                            }
                        }
                    }
                }
                finally {
                    Pop-Location
                }
            }
        }
    }
} else {
    $results.Add((New-GateResult -Name "gpu-readiness-local" -Passed $false -Details "Skipped by user" -Evidence ""))
}

# Gate: graph-readiness-local (optional hardening)
if (-not $SkipGraphGate) {
    $graphBenchBuildLog = Join-Path $outputDir "graph_bench_build.log"
    $graphBenchRunLog   = Join-Path $outputDir "graph_bench_run.log"
    $graphBenchJson     = Join-Path $outputDir "graph_traversal_bench.json"
    $graphFocusedLog    = Join-Path $outputDir "graph_focused_ctest.log"
    $graphFocusedJunit  = Join-Path $outputDir "graph_focused_ctest.junit.xml"
    $graphKernelReport  = Join-Path $outputDir "graph_kernel_check.json"

    $requiredGraphFiles = @(
        (Join-Path $repoRoot "src/graph/gpu_traversal.cpp"),
        (Join-Path $repoRoot "include/graph/gpu_traversal.h"),
        (Join-Path $repoRoot "tests/test_gpu_graph_traversal.cpp"),
        (Join-Path $repoRoot "tests/graph/test_query_explain.cpp"),
        (Join-Path $repoRoot "benchmarks/bench_graph_traversal.cpp"),
        (Join-Path $repoRoot "src/graph/ROADMAP.md")
    )

    $missingGraphFiles = @($requiredGraphFiles | Where-Object { -not (Test-Path -LiteralPath $_) })
    if ($missingGraphFiles.Count -gt 0) {
        $results.Add((New-GateResult -Name "graph-readiness-local" -Passed $false -Details ("Missing graph evidence files: {0}" -f ($missingGraphFiles -join "; ")) -Evidence ""))
    } else {
        $gpuTraversalCpp = Get-Content -Raw (Join-Path $repoRoot "src/graph/gpu_traversal.cpp")
        $hasCudaKernelBfs = ($gpuTraversalCpp -match 'bfsExpandKernel')
        $hasCudaKernelDfs = ($gpuTraversalCpp -match 'dfsSingleSourceKernel')
        $hasCudaDispatch = ($gpuTraversalCpp -match 'runBFSCudaIfAvailable' -and $gpuTraversalCpp -match 'runDFSCudaIfAvailable')
        $hasNonFallbackPath = ($gpuTraversalCpp -match 'used_cpu_fallback = false')

        @{
            checked_at = (Get-Date -Format "o")
            bfs_kernel_present = $hasCudaKernelBfs
            dfs_kernel_present = $hasCudaKernelDfs
            cuda_dispatch_present = $hasCudaDispatch
            non_fallback_path_present = $hasNonFallbackPath
        } | ConvertTo-Json | Set-Content -Path $graphKernelReport -Encoding UTF8

        if (-not $hasCudaKernelBfs -or -not $hasCudaKernelDfs -or -not $hasCudaDispatch -or -not $hasNonFallbackPath) {
            $results.Add((New-GateResult -Name "graph-readiness-local" -Passed $false -Details "Graph CUDA kernel path incomplete (BFS/DFS kernels or dispatch missing)" -Evidence $graphKernelReport))
        } else {
            Push-Location $repoRoot
            try {
                & cmake --build --preset $BuildPreset --target test_gpu_graph_traversal test_graph_query_explain_focused bench_graph_traversal *>&1 | Tee-Object -FilePath $graphBenchBuildLog
                $graphBuildExit = $LASTEXITCODE

                if ($graphBuildExit -ne 0) {
                    $results.Add((New-GateResult -Name "graph-readiness-local" -Passed $false -Details ("Graph targets build failed (exit={0})" -f $graphBuildExit) -Evidence $graphBenchBuildLog))
                } else {
                    & ctest --preset $BuildPreset -R "GPUGraphTraversalTests|GraphQueryExplainFocusedTests" --output-on-failure --output-junit $graphFocusedJunit *>&1 | Tee-Object -FilePath $graphFocusedLog
                    $graphTestsExit = $LASTEXITCODE

                    $graphFocusedTests = 0
                    $graphFocusedFails = 0
                    if (Test-Path $graphFocusedJunit) {
                        [xml]$gx = Get-Content -Raw $graphFocusedJunit
                        $suiteNode = $gx.SelectSingleNode("/testsuites")
                        if (-not $suiteNode) {
                            $suiteNode = $gx.SelectSingleNode("/testsuite")
                        }
                        if ($suiteNode) {
                            if ($suiteNode.Attributes["tests"]) { $graphFocusedTests = [int]$suiteNode.Attributes["tests"].Value }
                            if ($suiteNode.Attributes["failures"]) { $graphFocusedFails = [int]$suiteNode.Attributes["failures"].Value }
                        }
                    }

                    $binaryDir = Join-Path $repoRoot ("build-" + $BuildPreset)
                    $graphBenchExe = Find-BenchmarkExecutable -BinaryDir $binaryDir -ExecutableBaseName "bench_graph_traversal"
                    if ([string]::IsNullOrWhiteSpace($graphBenchExe)) {
                        $results.Add((New-GateResult -Name "graph-readiness-local" -Passed $false -Details "bench_graph_traversal executable not found" -Evidence $graphBenchBuildLog))
                    } else {
                        $oldPath = $env:PATH
                        try {
                            $dllCandidates = @(
                                (Join-Path $binaryDir "bin"),
                                (Join-Path $binaryDir "cmake")
                            ) | Where-Object { Test-Path $_ }
                            if (@($dllCandidates).Count -gt 0) {
                                $env:PATH = ((@($dllCandidates) -join ";") + ";" + $env:PATH)
                            }
                            & $graphBenchExe "--benchmark_min_time=0.01s" "--benchmark_repetitions=1" "--benchmark_format=json" "--benchmark_out=$graphBenchJson" *>&1 | Tee-Object -FilePath $graphBenchRunLog
                            $graphBenchExit = $LASTEXITCODE
                        }
                        finally {
                            $env:PATH = $oldPath
                        }

                        if (($graphBenchExit -eq 0) -and (Test-Path $graphBenchJson)) {
                            try {
                                $graphDoc = Get-Content -Raw $graphBenchJson | ConvertFrom-Json
                                $graphBenchCount = @($graphDoc.benchmarks).Count
                                $hasBfsBench = ($graphDoc.benchmarks | Where-Object { $_.name -like "*BFSTraversal*" } | Select-Object -First 1) -ne $null
                                $hasDfsBench = ($graphDoc.benchmarks | Where-Object { $_.name -like "*DFSTraversal*" } | Select-Object -First 1) -ne $null

                                $graphPassed = (
                                    $graphTestsExit -eq 0 -and
                                    $graphFocusedFails -eq 0 -and
                                    $graphFocusedTests -ge $GraphFocusedTestMinCount -and
                                    $graphBenchCount -ge $GraphBenchmarkMinCount -and
                                    $hasBfsBench -and
                                    $hasDfsBench
                                )

                                $graphDetails = (
                                    "Graph CUDA kernels present (BFS/DFS); focused tests={0} failed={1}; " +
                                    "bench_graph_traversal count={2} (min={3}); BFSTraversal={4}; DFSTraversal={5}"
                                ) -f $graphFocusedTests, $graphFocusedFails, $graphBenchCount, $GraphBenchmarkMinCount, $hasBfsBench, $hasDfsBench

                                $results.Add((New-GateResult -Name "graph-readiness-local" -Passed $graphPassed -Details $graphDetails -Evidence $graphBenchJson))
                            }
                            catch {
                                $results.Add((New-GateResult -Name "graph-readiness-local" -Passed $false -Details ("Failed to parse graph benchmark JSON: {0}" -f $_.Exception.Message) -Evidence $graphBenchRunLog))
                            }
                        } else {
                            $results.Add((New-GateResult -Name "graph-readiness-local" -Passed $false -Details ("bench_graph_traversal run failed (exit={0})" -f $graphBenchExit) -Evidence $graphBenchRunLog))
                        }
                    }
                }
            }
            finally {
                Pop-Location
            }
        }
    }
} else {
    $results.Add((New-GateResult -Name "graph-readiness-local" -Passed $false -Details "Skipped by user" -Evidence ""))
}

# Gate: sharding-readiness-local
if (-not $SkipShardingGate) {
    $shardingBenchBuildLog = Join-Path $outputDir "sharding_bench_build.log"
    $shardingBenchRunLog = Join-Path $outputDir "sharding_bench_run.log"
    $shardingRoutingJson = Join-Path $outputDir "sharding_bench_routing.json"
    $shardingPerfJson = Join-Path $outputDir "sharding_bench_performance.json"
    $shardingFocusedLog = Join-Path $outputDir "sharding_focused_ctest.log"
    $shardingFocusedJunit = Join-Path $outputDir "sharding_focused_ctest.junit.xml"
    $shardingApiReport = Join-Path $outputDir "sharding_api_check.json"

    $requiredShardingFiles = @(
        (Join-Path $repoRoot "src/sharding/shard_rpc_server.cpp"),
        (Join-Path $repoRoot "src/sharding/shard_rpc_client.cpp"),
        (Join-Path $repoRoot "src/sharding/shard_durability.cpp"),
        (Join-Path $repoRoot "include/sharding/shard_rpc_server.h"),
        (Join-Path $repoRoot "include/sharding/shard_rpc_client.h"),
        (Join-Path $repoRoot "tests/test_shard_rpc_mtls_config.cpp"),
        (Join-Path $repoRoot "tests/test_shard_rpc_integration.cpp"),
        (Join-Path $repoRoot "tests/test_shard_durability.cpp"),
        (Join-Path $repoRoot "tests/test_sharding_chaos.cpp"),
        (Join-Path $repoRoot "benchmarks/bench_shard_routing.cpp"),
        (Join-Path $repoRoot "benchmarks/bench_sharding_performance.cpp")
    )

    $missingShardingFiles = @($requiredShardingFiles | Where-Object { -not (Test-Path -LiteralPath $_) })
    if ($missingShardingFiles.Count -gt 0) {
        $results.Add((New-GateResult -Name "sharding-readiness-local" -Passed $false -Details ("Missing sharding evidence files: {0}" -f ($missingShardingFiles -join "; ")) -Evidence ""))
    } else {
        $rpcClientHeader = Get-Content -Raw (Join-Path $repoRoot "include/sharding/shard_rpc_client.h")
        $rpcServerHeader = Get-Content -Raw (Join-Path $repoRoot "include/sharding/shard_rpc_server.h")
        $shardDurability = Get-Content -Raw (Join-Path $repoRoot "src/sharding/shard_durability.cpp")

        $hasClientMtls = ($rpcClientHeader -match 'enable_mtls' -and $rpcClientHeader -match 'tls_cert_path' -and $rpcClientHeader -match 'tls_key_path')
        $hasServerMtls = ($rpcServerHeader -match 'enable_mtls' -and $rpcServerHeader -match 'tls_cert_path' -and $rpcServerHeader -match 'tls_require_client_cert')
        $hasRecoveryPath = ($shardDurability -match 'performRecovery' -or $shardDurability -match 'recovery')

        @{
            checked_at = (Get-Date -Format "o")
            client_mtls_fields_present = $hasClientMtls
            server_mtls_fields_present = $hasServerMtls
            durability_recovery_present = $hasRecoveryPath
        } | ConvertTo-Json | Set-Content -Path $shardingApiReport -Encoding UTF8

        if (-not $hasClientMtls -or -not $hasServerMtls -or -not $hasRecoveryPath) {
            $results.Add((New-GateResult -Name "sharding-readiness-local" -Passed $false -Details "Sharding API checks failed (mTLS or recovery path missing)" -Evidence $shardingApiReport))
        } else {
            Push-Location $repoRoot
            try {
                $binaryDir = Join-Path $repoRoot ("build-" + $BuildPreset)
                $routingExe = Find-BenchmarkExecutable -BinaryDir $binaryDir -ExecutableBaseName "bench_shard_routing"
                $perfExe = Find-BenchmarkExecutable -BinaryDir $binaryDir -ExecutableBaseName "bench_sharding_performance"
                $allBenchesExist = -not [string]::IsNullOrWhiteSpace($routingExe) -and -not [string]::IsNullOrWhiteSpace($perfExe)

                if ($allBenchesExist) {
                    $shardingBuildExit = 0
                    "Using existing sharding benchmark binaries; skipping rebuild" | Tee-Object -FilePath $shardingBenchBuildLog
                } else {
                    & cmake --build --preset $BuildPreset --target test_shard_rpc_integration_focused test_sharding_transaction_wal_focused test_sharding_core_focused test_sharding_chaos_focused bench_shard_routing bench_sharding_performance *>&1 | Tee-Object -FilePath $shardingBenchBuildLog
                    $shardingBuildExit = $LASTEXITCODE
                    $routingExe = Find-BenchmarkExecutable -BinaryDir $binaryDir -ExecutableBaseName "bench_shard_routing"
                    $perfExe = Find-BenchmarkExecutable -BinaryDir $binaryDir -ExecutableBaseName "bench_sharding_performance"
                }

                if ($shardingBuildExit -ne 0) {
                    $results.Add((New-GateResult -Name "sharding-readiness-local" -Passed $false -Details ("Sharding focused targets build failed (exit={0})" -f $shardingBuildExit) -Evidence $shardingBenchBuildLog))
                } else {
                    & ctest --preset $BuildPreset -R "ShardRpcIntegrationFocusedTests|ShardingTransactionWALFocusedTests|ShardingChaosFocusedTests" --timeout 300 --output-on-failure --output-junit $shardingFocusedJunit *>&1 | Tee-Object -FilePath $shardingFocusedLog
                    $shardingTestsExit = $LASTEXITCODE

                    $shardingFocusedTests = 0
                    $shardingFocusedFails = 0
                    if (Test-Path $shardingFocusedJunit) {
                        [xml]$sx = Get-Content -Raw $shardingFocusedJunit
                        $suiteNode = $sx.SelectSingleNode("/testsuites")
                        if (-not $suiteNode) {
                            $suiteNode = $sx.SelectSingleNode("/testsuite")
                        }
                        if ($suiteNode) {
                            if ($suiteNode.Attributes["tests"]) { $shardingFocusedTests = [int]$suiteNode.Attributes["tests"].Value }
                            if ($suiteNode.Attributes["failures"]) { $shardingFocusedFails = [int]$suiteNode.Attributes["failures"].Value }
                        }
                    }

                    if ([string]::IsNullOrWhiteSpace($routingExe) -or [string]::IsNullOrWhiteSpace($perfExe)) {
                        $results.Add((New-GateResult -Name "sharding-readiness-local" -Passed $false -Details "Sharding benchmark executable not found (routing/performance)" -Evidence $shardingBenchBuildLog))
                    } else {
                        $oldPath = $env:PATH
                        try {
                            $dllCandidates = @(
                                (Join-Path $binaryDir "bin"),
                                (Join-Path $binaryDir "cmake")
                            ) | Where-Object { Test-Path $_ }
                            if (@($dllCandidates).Count -gt 0) {
                                $env:PATH = ((@($dllCandidates) -join ";") + ";" + $env:PATH)
                            }

                            & $routingExe "--benchmark_min_time=0.01s" "--benchmark_repetitions=1" "--benchmark_format=json" "--benchmark_out=$shardingRoutingJson" *>&1 | Tee-Object -FilePath $shardingBenchRunLog
                            $routingExit = $LASTEXITCODE

                            & $perfExe "--benchmark_min_time=0.01s" "--benchmark_repetitions=1" "--benchmark_format=json" "--benchmark_out=$shardingPerfJson" *>&1 | Tee-Object -FilePath $shardingBenchRunLog -Append
                            $perfExit = $LASTEXITCODE
                        }
                        finally {
                            $env:PATH = $oldPath
                        }

                        if (($routingExit -eq 0) -and ($perfExit -eq 0) -and (Test-Path $shardingRoutingJson) -and (Test-Path $shardingPerfJson)) {
                            try {
                                $routingDoc = Get-Content -Raw $shardingRoutingJson | ConvertFrom-Json
                                $perfDoc = Get-Content -Raw $shardingPerfJson | ConvertFrom-Json

                                $routingBenchCount = @($routingDoc.benchmarks).Count
                                $perfBenchCount = @($perfDoc.benchmarks).Count
                                $totalBenchCount = $routingBenchCount + $perfBenchCount

                                $hasSingleShardLookup = ($routingDoc.benchmarks | Where-Object { $_.name -like "*SingleShardLookup*" } | Select-Object -First 1) -ne $null
                                $hasBatchRouting = ($routingDoc.benchmarks | Where-Object { $_.name -like "*BatchRouting*" } | Select-Object -First 1) -ne $null
                                $hasScatterGather = ($perfDoc.benchmarks | Where-Object { $_.name -like "*ScatterGatherLatency*" } | Select-Object -First 1) -ne $null

                                $opsCandidates = New-Object System.Collections.Generic.List[double]
                                foreach ($b in @($routingDoc.benchmarks) + @($perfDoc.benchmarks)) {
                                    if ($null -ne $b.ops_per_sec) { $opsCandidates.Add([double]$b.ops_per_sec) }
                                    if ($null -ne $b.requests_per_sec) { $opsCandidates.Add([double]$b.requests_per_sec) }
                                    if ($null -ne $b.lookups_per_sec) { $opsCandidates.Add([double]$b.lookups_per_sec) }
                                }

                                $maxOpsPerSec = if ($opsCandidates.Count -gt 0) { ($opsCandidates | Measure-Object -Maximum).Maximum } else { 0.0 }

                                $shardingPassed = (
                                    $shardingTestsExit -eq 0 -and
                                    $shardingFocusedFails -eq 0 -and
                                    $shardingFocusedTests -ge $ShardingFocusedTestMinCount -and
                                    $totalBenchCount -ge $ShardingBenchmarkMinCount -and
                                    $hasSingleShardLookup -and
                                    $hasBatchRouting -and
                                    $hasScatterGather -and
                                    $maxOpsPerSec -ge $ShardingRoutingOpsPerSecMin
                                )

                                $shardingDetails = (
                                    "Focused tests={0} failed={1}; benchmark_count={2} (min={3}); " +
                                    "SingleShardLookup={4}; BatchRouting={5}; ScatterGatherLatency={6}; " +
                                    "max_ops_per_sec={7:N2} (min={8:N2})"
                                ) -f $shardingFocusedTests, $shardingFocusedFails, $totalBenchCount, $ShardingBenchmarkMinCount, $hasSingleShardLookup, $hasBatchRouting, $hasScatterGather, [double]$maxOpsPerSec, $ShardingRoutingOpsPerSecMin

                                $results.Add((New-GateResult -Name "sharding-readiness-local" -Passed $shardingPassed -Details $shardingDetails -Evidence $shardingPerfJson))
                            }
                            catch {
                                $results.Add((New-GateResult -Name "sharding-readiness-local" -Passed $false -Details ("Failed to parse sharding benchmark JSON: {0}" -f $_.Exception.Message) -Evidence $shardingBenchRunLog))
                            }
                        } else {
                            $results.Add((New-GateResult -Name "sharding-readiness-local" -Passed $false -Details ("Sharding benchmark run failed (routing={0}, performance={1})" -f $routingExit, $perfExit) -Evidence $shardingBenchRunLog))
                        }
                    }
                }
            }
            finally {
                Pop-Location
            }
        }
    }
} else {
    $results.Add((New-GateResult -Name "sharding-readiness-local" -Passed $false -Details "Skipped by user" -Evidence ""))
}

# Gate 6 + 7: Cluster fault-injection and SLA proxy via repeated phase4 suite
$phase4Log = Join-Path $outputDir "phase4_ctest.log"
$phase4Junit = Join-Path $outputDir "phase4_ctest.junit.xml"
$phase4Passed = $false
$phase4PassRate = 0.0
$phase4TotalTests = 0
$phase4FailedTests = 0

if (-not $SkipPhase4Tests) {
    Push-Location $repoRoot
    try {
        $ctestArgs = @(
            "--preset", $BuildPreset,
            "-L", "phase4",
            "--output-on-failure",
            "--repeat", "until-fail:$RepeatCount",
            "--output-junit", $phase4Junit
        )

        & ctest @ctestArgs *>&1 | Tee-Object -FilePath $phase4Log
        $ctestExit = $LASTEXITCODE

        if (Test-Path $phase4Junit) {
            [xml]$xml = Get-Content -Raw $phase4Junit
            $tests = 0
            $failures = 0

            $suiteNode = $xml.SelectSingleNode("/testsuites")
            if (-not $suiteNode) {
                $suiteNode = $xml.SelectSingleNode("/testsuite")
            }

            if ($suiteNode) {
                $testsAttr = $suiteNode.Attributes["tests"]
                $failuresAttr = $suiteNode.Attributes["failures"]
                if ($testsAttr) {
                    $tests = [int]$testsAttr.Value
                }
                if ($failuresAttr) {
                    $failures = [int]$failuresAttr.Value
                }
            }

            $phase4TotalTests = $tests
            $phase4FailedTests = $failures

            if ($tests -gt 0) {
                $phase4PassRate = ((($tests - $failures) * 100.0) / $tests)
            }
        }

        $phase4Passed = ($ctestExit -eq 0 -and $phase4FailedTests -eq 0)
    }
    finally {
        Pop-Location
    }

    $results.Add((New-GateResult -Name "cluster-chaos-local" -Passed $phase4Passed -Details ("Phase4 slice repeated until-fail:{0}; tests={1}; failed={2}" -f $RepeatCount, $phase4TotalTests, $phase4FailedTests) -Evidence $phase4Log))

    $slaPassed = $phase4PassRate -ge 99.99
    $results.Add((New-GateResult -Name "sla-99.99-local-proxy" -Passed $slaPassed -Details ("Pass-rate={0:N3}% (threshold 99.99%) based on repeated phase4 suite" -f $phase4PassRate) -Evidence $phase4Junit))
} else {
    $results.Add((New-GateResult -Name "cluster-chaos-local" -Passed $false -Details "Skipped by user" -Evidence ""))
    $results.Add((New-GateResult -Name "sla-99.99-local-proxy" -Passed $false -Details "Skipped by user" -Evidence ""))
}

# Gate 3: Penetration report evidence
$pentestLog = Join-Path $outputDir "pentest.log"
$pentestReportCandidates = @(
    (Join-Path $repoRoot "security/pentest/LOCAL_PENTEST_REPORT.md"),
    (Join-Path $repoRoot "security/pentest/reports/latest/report.html"),
    (Join-Path $repoRoot "security/pentest/reports/report.html")
)

$pentestPassed = $false
$pentestEvidence = ""
$pentestDetail = ""

if ($SkipPentest) {
    $pentestDetail = "Skipped by user"
} else {
    if ($RunPentest) {
        $bash = Get-Command bash -ErrorAction SilentlyContinue
        if (-not $bash) {
            $pentestDetail = "bash not available; cannot execute security/pentest/run_pentest.sh"
        } elseif ([string]::IsNullOrWhiteSpace($PentestTarget)) {
            $pentestDetail = "RunPentest requires -PentestTarget HOST:PORT"
        } else {
            Push-Location $repoRoot
            try {
                & bash "security/pentest/run_pentest.sh" --target $PentestTarget --category $PentestCategory *>&1 | Tee-Object -FilePath $pentestLog
                if ($LASTEXITCODE -eq 0) {
                    $pentestDetail = "Pentest script executed successfully"
                } else {
                    $pentestDetail = "Pentest script failed (exit=$LASTEXITCODE)"
                }
            }
            finally {
                Pop-Location
            }
        }
    }

    foreach ($candidate in $pentestReportCandidates) {
        if (Test-Path $candidate) {
            $pentestPassed = $true
            $pentestEvidence = $candidate
            if ([string]::IsNullOrWhiteSpace($pentestDetail)) {
                $pentestDetail = "Found penetration-test evidence"
            }
            break
        }
    }

    if (-not $pentestPassed -and [string]::IsNullOrWhiteSpace($pentestDetail)) {
        $pentestDetail = "No penetration-test report found"
    }
}

if ([string]::IsNullOrWhiteSpace($pentestEvidence) -and (Test-Path $pentestLog)) {
    $pentestEvidence = $pentestLog
}

$results.Add((New-GateResult -Name "pentest-report-local" -Passed $pentestPassed -Details $pentestDetail -Evidence $pentestEvidence))

# Beta-module gate (optional for local release readiness)
$roadmapPath = Join-Path $repoRoot "roadmap.md"
$betaModules = @()
if (Test-Path $roadmapPath) {
    $betaModules = Select-String -Path $roadmapPath -Pattern "\| \*\*.*\*\* \| 🟡 Beta" | ForEach-Object { $_.Line.Trim() }
}

$betaListPath = Join-Path $outputDir "beta_modules.txt"
$betaModules | Set-Content -Path $betaListPath -Encoding UTF8

$betaPassed = $AllowBetaModules.IsPresent -or ($betaModules.Count -eq 0)
$betaDetails = if ($betaModules.Count -eq 0) {
    "No beta modules listed in roadmap.md"
} elseif ($AllowBetaModules.IsPresent) {
    "{0} beta modules accepted via -AllowBetaModules" -f $betaModules.Count
} else {
    "{0} beta modules still listed in roadmap.md" -f $betaModules.Count
}

$results.Add((New-GateResult -Name "beta-module-exit" -Passed $betaPassed -Details $betaDetails -Evidence $betaListPath))

$failed = $results | Where-Object { -not $_.passed }
$summary = [pscustomobject]@{
    generated_at = (Get-Date).ToString("o")
    build_preset = $BuildPreset
    repeat_count = $RepeatCount
    output_dir = $outputDir
    gates = $results
    failed_gate_count = @($failed).Count
}

$jsonPath = Join-Path $outputDir "readiness-summary.json"
$summary | ConvertTo-Json -Depth 6 | Set-Content -Path $jsonPath -Encoding UTF8

$mdPath = Join-Path $outputDir "readiness-summary.md"
$mdLines = @()
$mdLines += "# Local Production Readiness Summary"
$mdLines += ""
$mdLines += "- Generated: $($summary.generated_at)"
$mdLines += "- Build preset: $BuildPreset"
$mdLines += "- Repeat count: $RepeatCount"
$mdLines += ""
$mdLines += "## Gates"
$mdLines += ""
foreach ($gate in $results) {
    $status = if ($gate.passed) { "PASS" } else { "FAIL" }
    $mdLines += "- **$($gate.name)**: $status"
    $mdLines += "  - Details: $($gate.details)"
    if (-not [string]::IsNullOrWhiteSpace($gate.evidence)) {
        $mdLines += "  - Evidence: $($gate.evidence)"
    }
}
$mdLines += ""
$mdLines += "## Result"
$mdLines += ""
$mdLines += if (@($failed).Count -eq 0) { "All local gates passed." } else { "Failed gates: $(@($failed).Count)" }
$mdLines | Set-Content -Path $mdPath -Encoding UTF8

Write-Host "Local readiness report written to: $outputDir"
Write-Host "Summary JSON: $jsonPath"
Write-Host "Summary Markdown: $mdPath"

if ((@($failed).Count -gt 0) -and -not $NoFailOnGate) {
    exit 2
}

exit 0
