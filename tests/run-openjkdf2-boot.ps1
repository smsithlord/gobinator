# End-to-end check on OpenJKDF2, with no window (docs/DECISIONS.md, 2026-10-27). gobinator.exe copies a real episode
# GOB (P03, the piano room) into the test install's episode folder, beside LucasArts' JK1.GOB, and makes the empty
# folder episode\P03, which lists it first; the engine must then reach the episode's level both ways a player gets
# there:
#
#   boot   started directly as episode P03, which is what OpenJKDF2's -autostart -episode P03 does;
#   menu   through the menus: Singleplayer > New Game > the FIRST episode > Ok, as on retail JK. Without the folder
#          the first is JK1's The Force Within (J before P), so reaching piano.jkl this way shows the folder at work.
#
# The control: after gobinator.exe runs with no GOB, which must remove the folder and leave P03.GOB, the first
# episode in the menus is JK1's again, and piano.jkl is not reached that way.
#
#   & tests\make-openjkdf2-install.ps1      (once)
#   cmake --build build --config Release
#   & tests\run-openjkdf2-boot.ps1
#
# The engine is openjkdf2_libretro's headless harness and core (the jk-content skill), used read-only. --content-path
# mounts the mod folder exactly as the desktop EXE's -path does. Gobinator itself starts the fake JK here, not
# OpenJKDF2: starting the desktop OpenJKDF2 opens a game window, and that waits for the user's go.
param([string]$Libretro = 'G:\Documents Sym Links\GitHub\openjkdf2_libretro')

$ErrorActionPreference = 'Stop'
$Repo = [IO.Path]::GetFullPath("$PSScriptRoot\..")
$Jk = "$Repo\testinstall\openjkdf2"
$Gob = "$Repo\testinstall\fixtures\P03.GOB"
$Harness = "$Libretro\build_harness\Release\openjkdf2_harness.exe"
$Core = "$Libretro\build_libretro\Release\openjkdf2_libretro.dll"
foreach ($f in "$Jk\resource\Res2.gob", "$Jk\episode\JK1.GOB", $Gob, $Harness, $Core, "$Repo\build\Release\gobinator.exe") {
    if (-not (Test-Path -LiteralPath $f)) { throw "missing: $f (build, and run make-openjkdf2-install.ps1)" }
}
$Copy = "$Jk\episode\P03.GOB"
$Entry = "$Jk\episode\P03"
$State = "$Jk\episode\gobinator.state"
foreach ($f in $Copy, $Entry, $State) {
    if (Test-Path -LiteralPath $f) { throw "left over from an earlier run: $f (remove it first)" }
}
$Out = Join-Path ([IO.Path]::GetTempPath()) 'gobinator-ojk-boot'
$env:GOBINATOR_NO_MESSAGES = '1'
$script:Failed = 0

function Check($what, [bool]$ok) {
    if ($ok) { "  PASS  $what" } else { "  FAIL  $what"; $script:Failed++ }
}

# Runs the harness on a script; returns its exit code (the number of failed script lines).
function Harness($name, $rom, [string[]]$lines, [string[]]$options = @()) {
    $dir = "$Out\$name"
    New-Item -ItemType Directory -Force $dir | Out-Null
    Set-Content "$dir\run.cmds" ($lines -join "`r`n") -Encoding ascii
    $opts = @('--option', 'openjkdf2_portable=no_writes', '--option', 'openjkdf2_cogfactory=enabled')
    foreach ($o in $options) { $opts += '--option', $o }
    & $Harness --core $Core --rom $rom --content-path "$Jk\mod" @opts --pace turbo --out $dir --script "$dir\run.cmds" 2>&1 | Out-Null
    return $LASTEXITCODE
}

# Started directly as episode P03.
function Boot($name) {
    Harness $name $Copy @("expect `"world 'piano.jkl' (episode 'P03')`" 1800")
}

# Title > main menu > Singleplayer > New Game (the list screenshotted) > the first episode > Ok (640x480 menu
# pixels, p69's clicks).
function Menu($name) {
    $s = @('timestep 20fps', 'expect "[CF] timestep: FIXED at 50.0000 ms/frame" 300', 'run 200', 'key tap RETURN', 'run 30')
    $n = 0
    foreach ($xy in @(318, 187), @(294, 109), @(330, 220), @(497, 449)) {
        $s += 'mouse move -5000 -5000', 'run 2', "mouse move $($xy[0]) $($xy[1])", 'run 5', 'mouse down left', 'run 1',
              'mouse up left', 'run 40'
        if (++$n -eq 2) { $s += "shot $Out\$name\episodes.png" }
    }
    $s += "expect `"world 'piano.jkl' (episode 'P03')`" 1800"
    Harness $name "$Jk\episode\JK1.GOB" $s @('openjkdf2_boot=menu')
}

function Gobinator([string]$gob) {
    $sp = @{ FilePath = "$Jk\gobinator.exe"; PassThru = $true }
    if ($gob) { $sp.ArgumentList = "`"$gob`"" }
    $p = Start-Process @sp		# not -Wait: it would wait for JK too
    $p.WaitForExit()
    return $p.ExitCode
}

if (Test-Path $Out) { Remove-Item -Recurse -Force $Out }
Get-ChildItem -LiteralPath "$Jk\mod" | Remove-Item -Recurse -Force
Copy-Item "$Repo\build\Release\gobinator.exe" $Jk
Copy-Item "$Repo\build\Release\fakejk.exe" $Jk
Set-Content "$Jk\gobinator.ini" "JK_EXECUTABLE = fakejk.exe`r`n" -Encoding ascii
try {
    "Gobinator copies P03.GOB into episode and lists it first"
    Check "exit code 0" ((Gobinator $Gob) -eq 0)
    Check "episode\P03.GOB is there" (Test-Path -LiteralPath $Copy)
    Check "the empty folder episode\P03 is there" (Test-Path -LiteralPath $Entry -PathType Container)

    "OpenJKDF2 reaches piano.jkl"
    Check "started directly as episode P03 (-autostart)" ((Boot 'boot') -eq 0)
    Check "through the menus: New Game, the FIRST episode, Ok" ((Menu 'menu') -eq 0)

    "gobinator.exe with no GOB cleans up"
    Check "exit code 0" ((Gobinator $null) -eq 0)
    Check "episode\P03 and the record are gone" (-not (Test-Path -LiteralPath $Entry) -and -not (Test-Path -LiteralPath $State))
    Check "episode\P03.GOB stays, one of the episodes" (Test-Path -LiteralPath $Copy)
    Check "control: the first episode in the menus is no longer P03" ((Menu 'cleaned') -ne 0)
}
finally {
    foreach ($f in $Copy, $State) { Remove-Item -LiteralPath $f -Force -ErrorAction SilentlyContinue }
    if (Test-Path -LiteralPath $Entry) { Remove-Item -LiteralPath $Entry -Recurse -Force }
}

''
if ($script:Failed) { "$($script:Failed) check(s) FAILED"; exit 1 } else { 'all checks passed'; exit 0 }
