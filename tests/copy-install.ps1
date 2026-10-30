# Copies a JK install the user offered for testing into testinstall\<Name> (gitignored), where tests may write.
# The source is only read; nothing is ever copied back.
#
#   & tests\copy-install.ps1 -From 'H:\SteamLibrary\steamapps\common\Star Wars Jedi Knight Vanilla' -Name steam-vanilla
#
# Retail JK keeps its settings in the registry (HKLM\SOFTWARE\WOW6432Node\LucasArts Entertainment Company\JediKnight,
# virtualised under HKCU), shared by every retail-style JK on the machine: running a copy changes those as playing does.
param([Parameter(Mandatory)][string]$From, [Parameter(Mandatory)][string]$Name)

$ErrorActionPreference = 'Stop'
$Repo = [IO.Path]::GetFullPath("$PSScriptRoot\..")
$From = [IO.Path]::GetFullPath($From)
$To = [IO.Path]::GetFullPath((Join-Path "$Repo\testinstall" $Name))
if (-not $To.StartsWith("$Repo\testinstall\")) { throw "refusing to write outside testinstall: $To" }
if ($From.StartsWith($Repo)) { throw "the source is inside this repo: $From" }
if (-not (Test-Path -LiteralPath $From -PathType Container)) { throw "no such folder: $From" }
if (Test-Path -LiteralPath $To) { throw "already there: $To (delete it first to copy again)" }

robocopy $From $To /E /COPY:DAT /R:1 /W:1 /NFL /NDL /NJH /NJS /NP | Out-Null
if ($LASTEXITCODE -ge 8) { throw "robocopy failed with exit code $LASTEXITCODE" }
$files = Get-ChildItem -LiteralPath $To -Recurse -File -Force
"copied to {0}: {1} files, {2:N0} MB" -f $To, $files.Count, (($files | Measure-Object Length -Sum).Sum / 1MB)
exit 0
