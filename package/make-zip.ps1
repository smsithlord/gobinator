# Makes the release zip (docs/PLAN.md, part 8): gobinator.exe, the commented gobinator.ini, README.md and LICENSE in
# dist\gobinator-<tag>\ and dist\gobinator-<tag>.zip, and prints the zip's SHA-256. The tag and the version text come
# from src\version.h, and the built EXE must carry that version, so a stale build cannot ship. Unless -SkipTests, the
# unit tests and the acceptance scenarios run first (in TEMP, against the fake JK); a failure stops everything.
#
#   cmake --build build --config Release
#   & package\make-zip.ps1
param([string]$Bin = "$PSScriptRoot\..\build\Release", [switch]$SkipTests)

$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path "$PSScriptRoot\..").Path
$versionH = Get-Content "$repo\src\version.h" -Raw
if ($versionH -notmatch 'GOBINATOR_VERSION_TAG\s+"([^"]+)"') { throw "no GOBINATOR_VERSION_TAG in src\version.h" }
$tag = $Matches[1]
if ($versionH -notmatch 'GOBINATOR_VERSION_TEXT\s+"([^"]+)"') { throw "no GOBINATOR_VERSION_TEXT in src\version.h" }
$text = $Matches[1]

$exe = Join-Path (Resolve-Path $Bin).Path 'gobinator.exe'
if (-not (Test-Path -LiteralPath $exe)) { throw "not built: $exe (cmake --build build --config Release)" }
$fileVersion = (Get-Item -LiteralPath $exe).VersionInfo.FileVersion
if ($fileVersion -ne $text) { throw "$exe says '$fileVersion' but src\version.h says '$text': rebuild first" }

if (-not $SkipTests) {
    & ctest --test-dir "$repo\build" -C Release
    if ($LASTEXITCODE -ne 0) { throw "the unit tests failed" }
    & "$repo\tests\run-v11-scenarios.ps1" -Bin (Split-Path -Parent $exe)
    if ($LASTEXITCODE -ne 0) { throw "the acceptance scenarios failed" }
}

$dir = "$repo\dist\gobinator-$tag"
if (Test-Path -LiteralPath $dir) { Remove-Item -LiteralPath $dir -Recurse -Force }
New-Item -ItemType Directory -Force -Path $dir | Out-Null
foreach ($f in $exe, "$repo\package\gobinator.ini", "$repo\README.md", "$repo\LICENSE") {
    Copy-Item -LiteralPath $f -Destination $dir
}

$zip = "$repo\dist\gobinator-$tag.zip"
if (Test-Path -LiteralPath $zip) { Remove-Item -LiteralPath $zip -Force }
Compress-Archive -Path "$dir\*" -DestinationPath $zip -CompressionLevel Optimal

Write-Output "Gobinator $text"
Get-ChildItem -LiteralPath $dir | ForEach-Object { Write-Output ("  {0,-16} {1,9:n0} bytes" -f $_.Name, $_.Length) }
Write-Output $zip
Write-Output ("SHA-256 " + (Get-FileHash -LiteralPath $zip -Algorithm SHA256).Hash)
