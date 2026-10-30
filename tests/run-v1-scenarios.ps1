# Runs the rebuilt v1.0 against fresh scratch JK installs, one per scenario, and reports what it did.
# Never touches the user's install: every path is under $Root, a folder in TEMP, and the script refuses anything else.
#
#   cmake --build build --config Release          (builds gobinator-v1.0.exe and the fake JK)
#   & tests\run-v1-scenarios.ps1                  (from PowerShell; the report goes to stdout)
param([string]$Gobinator = "$PSScriptRoot\..\build\Release\gobinator-v1.0.exe",
      [string]$FakeJk    = "$PSScriptRoot\..\build\Release\fakejk.exe",
      [string]$Root      = (Join-Path ([IO.Path]::GetTempPath()) 'gobinator-tests'))

$ErrorActionPreference = 'Stop'
$Root = [IO.Path]::GetFullPath($Root)
if (-not $Root.StartsWith([IO.Path]::GetFullPath([IO.Path]::GetTempPath()))) { throw "refusing to run outside TEMP: $Root" }
foreach ($f in $Gobinator, $FakeJk) { if (-not (Test-Path -LiteralPath $f)) { throw "not built: $f" } }
if (Test-Path $Root) { Remove-Item -Recurse -Force $Root }

$e  = [char]0xC9          # É
$ue = [char]0xFC          # ü
$jp = "$([char]0x65E5)$([char]0x672C)$([char]0x8A9E)"   # 日本語
$cafe = "Caf$([char]0xE9)"

function Write-Text($path, $text, [bool]$bom = $false) {
    [IO.File]::WriteAllText($path, $text, (New-Object Text.UTF8Encoding $bom))
}

# Guard: v1.0 runs system("gobinator.bat") and START by bare name, so cmd searches for them. On this machine
# the 32-bit cmd's PATH reaches C:\Windows\SysWOW64\gobinator.bat, a real one that launches the user's JK.
# A decoy folder goes first on PATH (a logging gobinator.bat, jk.exe and inject.exe), the current folder is
# searched first again (as under Explorer), and a 32-bit cmd must resolve every name to the decoy, or nothing runs.
$Decoy = Join-Path $Root 'decoy'
New-Item -ItemType Directory -Force $Decoy | Out-Null
Copy-Item $FakeJk (Join-Path $Decoy 'jk.exe')
Copy-Item $FakeJk (Join-Path $Decoy 'inject.exe')
Write-Text (Join-Path $Decoy 'gobinator.bat') "@echo DECOY gobinator.bat ran (cwd %CD%)>>`"$Decoy\launched.txt`"`r`n"
Remove-Item Env:NoDefaultCurrentDirectoryInExePath -ErrorAction SilentlyContinue
$env:Path = "$Decoy;$env:Path"
$resolved = & C:\Windows\SysWOW64\cmd.exe /d /c 'for %i in (gobinator.bat inject.exe jk.exe) do @echo %~$PATH:i'
foreach ($r in $resolved) { if (-not $r.StartsWith($Decoy)) { throw "guard failed: a 32-bit cmd resolves to [$r], not the decoy" } }
"guard: a 32-bit cmd resolves gobinator.bat, inject.exe and jk.exe to the decoy"
''

# The user's ini (reference/gobinator.ini), with jk.exe for inject.exe and a scratch exclude list.
function Ini([string[]]$excludes, [string]$exe = 'inject.exe') {
    $lines = @('# GOBINATOR FORMAT. LINE DELAMINATED. PROPERTY FOLLOWED BY VALUE. DO NOT SKIP LINES.', '#',
               '# !!WARNING!! Your MOD_FOLDER will be auto-purged of all GOBs!',
               'MOD_FOLDER', 'mod', 'JK_EXECUTABLE', $exe, 'ADDITIONAL_LAUNCH_OPTIONS', '-devmode')
    foreach ($x in $excludes) { $lines += 'MOD_FOLDER_PURGE_EXCLUDE_FILE'; $lines += $x }
    $lines += @('BFP_RANDOMIZE_MODEL', '1')
    return ($lines -join "`r`n")
}

function New-Install($name, [string[]]$modFiles, $ini, [bool]$bom = $false) {
    $base = Join-Path $Root $name
    if (Test-Path $base) { Remove-Item -Recurse -Force $base }
    $jk = Join-Path $base 'Fake JK Install'
    $dl = Join-Path $base 'Downloads'
    New-Item -ItemType Directory -Force (Join-Path $jk 'mod'), $dl | Out-Null
    Copy-Item $Gobinator (Join-Path $jk 'gobinator.exe')
    Copy-Item $FakeJk (Join-Path $jk 'jk.exe')
    Copy-Item $FakeJk (Join-Path $jk 'inject.exe')
    Write-Text (Join-Path $jk 'gobinator.ini') $ini $bom
    foreach ($f in $modFiles) { Write-Text (Join-Path $jk "mod\$f") "dummy $f" }
    return @{ Base = $base; Jk = $jk; Dl = $dl; Mod = (Join-Path $jk 'mod') }
}

function New-Gob($dir, $name) {
    New-Item -ItemType Directory -Force $dir | Out-Null
    $p = Join-Path $dir $name
    Write-Text $p "dummy episode $name"
    return $p
}

function Run($t, $title, $gob, $cwd) {
    "=== $title"
    "mod before: " + ((Get-ChildItem -LiteralPath $t.Mod -Name) -join ', ')
    $out = Join-Path $t.Base 'stdout.txt'
    $nul = Join-Path $t.Base 'stdin.txt'; Write-Text $nul ''
    $exe = Join-Path $t.Jk 'gobinator.exe'
    $sp = @{ FilePath = $exe; WorkingDirectory = $cwd; Wait = $true; NoNewWindow = $true
               RedirectStandardOutput = $out; RedirectStandardInput = $nul }
    if ($gob) { $sp.ArgumentList = '"' + $gob + '"' }
    Start-Process @sp
    Start-Sleep -Milliseconds 1500     # START returns before the fake JK writes its log
    "mod after:  " + ((Get-ChildItem -LiteralPath $t.Mod -Name) -join ', ')
    if ($gob) { "source GOB still there: " + (Test-Path -LiteralPath $gob) }
    "--- gobinator's console output:"
    Get-Content -LiteralPath $out -Encoding UTF8
    "--- gobinator.bat files written:"
    Get-ChildItem -LiteralPath $t.Base -Recurse -Filter gobinator.bat | ForEach-Object { $_.FullName.Substring($t.Base.Length) }
    "--- fake JK launched:"
    $log = Join-Path $t.Jk 'launched.txt'
    if (Test-Path $log) { Get-Content -LiteralPath $log -Encoding UTF8 } else { '(not launched)' }
    $dlog = Join-Path $Decoy 'launched.txt'
    if (Test-Path $dlog) { '!!! THE DECOY RAN:'; Get-Content -LiteralPath $dlog -Encoding UTF8; Remove-Item $dlog }
    ''
}

# A: the everyday case. GOBs in mod with every kind of name; an exclude whose case differs from the file on disk.
$t = New-Install 'A-everyday' @('keep.gob', 'other.gob', 'JK1.GOB', 'Bfp2x.gob', 'notes.gob.txt', 'my.goblin.cfg', 'readme.txt') (Ini @('keep.gob', 'BFP2X.gob'))
Run $t 'A: double-click Downloads\Episode.gob (cwd = Downloads, as Explorer does)' (New-Gob $t.Dl 'Episode.gob') $t.Dl

# B: an upper-case extension.
$t = New-Install 'B-uppercase' @('keep.gob', 'other.gob') (Ini @('keep.gob'))
Run $t 'B: double-click Downloads\EPISODE.GOB' (New-Gob $t.Dl 'EPISODE.GOB') $t.Dl

# C: the double-clicked GOB already sits in the mod folder.
$t = New-Install 'C-inmod' @('keep.gob', 'mine.gob') (Ini @('keep.gob'))
Run $t 'C: double-click mod\mine.gob itself' (Join-Path $t.Mod 'mine.gob') $t.Mod

# D: non-ASCII file names.
$t = New-Install 'D-nonascii-name' @('other.gob') (Ini @())
Run $t "D1: double-click Downloads\${e}pisode ${ue}ber.gob" (New-Gob $t.Dl "${e}pisode ${ue}ber.gob") $t.Dl
$t = New-Install 'D2-nonascii-name' @('other.gob') (Ini @())
Run $t "D2: double-click Downloads\$jp.gob" (New-Gob $t.Dl "$jp.gob") $t.Dl

# E: a non-ASCII folder.
$t = New-Install 'E-nonascii-folder' @('other.gob') (Ini @())
Run $t "E: double-click Downloads\$cafe\level.gob" (New-Gob (Join-Path $t.Dl $cafe) 'level.gob') (Join-Path $t.Dl $cafe)

# F: the same ini saved as UTF-8 with a byte-order mark (Notepad's "UTF-8 with BOM").
$t = New-Install 'F-ini-bom' @('keep.gob', 'other.gob') (Ini @('keep.gob')) $true
Run $t 'F: ini saved with a BOM; keep.gob is on the exclude list' (New-Gob $t.Dl 'Episode.gob') $t.Dl

# G: one blank line in the ini.
$t = New-Install 'G-ini-blank' @('keep.gob', 'other.gob') ((Ini @('keep.gob')) -replace "MOD_FOLDER`r`nmod", "`r`nMOD_FOLDER`r`nmod")
Run $t 'G: ini with one blank line before MOD_FOLDER; keep.gob is on the exclude list' (New-Gob $t.Dl 'Episode.gob') $t.Dl

# H: started with no GOB (the EXE itself double-clicked; cwd = the JK folder).
$t = New-Install 'H-noarg' @('keep.gob', 'other.gob') (Ini @('keep.gob'))
Run $t 'H: gobinator.exe started with no GOB' $null $t.Jk

# I: no ini.
$t = New-Install 'I-noini' @('other.gob') ''
Remove-Item (Join-Path $t.Jk 'gobinator.ini')
Run $t 'I: no gobinator.ini' (New-Gob $t.Dl 'Episode.gob') $t.Dl
