# Where may Gobinator put an episode GOB? An experiment, with no window: the test episode P03 is placed in one folder of
# the OpenJKDF2 test install at a time -- mod (what v1.1 does, started with -path mod), resource, mods (OpenJKDF2's
# always-on mods folder), or nowhere (the control) -- and each placement is tried two ways:
#
#   boot   the engine starts episode P03 directly, as OpenJKDF2's -autostart -episode P03 does: must it reach piano.jkl?
#   menu   the engine starts at its menus and clicks through Singleplayer > (Debug), which lists the current
#          episode's levels (P03's is PIANO), then New Game, which lists the episodes, a screenshot at each: is P03
#          ("COG Factory p03 Piano") the episode on offer?
#
#   & tests\make-openjkdf2-install.ps1      (once)
#   & tests\run-mount-experiment.ps1
#
# The engine is openjkdf2_libretro's headless harness and core (the jk-content skill), used read-only;
# --content-path mounts mod exactly as the desktop EXE's -path does. The test install is put back as it was.
param([string]$Libretro = 'G:\Documents Sym Links\GitHub\openjkdf2_libretro')

$ErrorActionPreference = 'Stop'
$Repo = [IO.Path]::GetFullPath("$PSScriptRoot\..")
$Jk = "$Repo\testinstall\openjkdf2"
$Gob = "$Repo\testinstall\fixtures\P03.GOB"
$Harness = "$Libretro\build_harness\Release\openjkdf2_harness.exe"
$Core = "$Libretro\build_libretro\Release\openjkdf2_libretro.dll"
foreach ($f in "$Jk\resource\Res2.gob", "$Jk\episode\JK1.GOB", $Gob, $Harness, $Core) {
    if (-not (Test-Path -LiteralPath $f)) { throw "missing: $f (run make-openjkdf2-install.ps1)" }
}
foreach ($d in 'mod', 'resource', 'mods') {
    if (Test-Path -LiteralPath "$Jk\$d\P03.GOB") { throw "$Jk\$d\P03.GOB is already there: remove it first" }
}
$Out = Join-Path ([IO.Path]::GetTempPath()) 'gobinator-mount-experiment'
if (Test-Path $Out) { Remove-Item -Recurse -Force $Out }

function Harness($dir, $rom, [string[]]$lines, [string[]]$options) {
    New-Item -ItemType Directory -Force $dir | Out-Null
    Set-Content "$dir\run.cmds" ($lines -join "`r`n") -Encoding ascii
    $opts = @('--option', 'openjkdf2_portable=no_writes', '--option', 'openjkdf2_cogfactory=enabled')
    foreach ($o in $options) { $opts += '--option', $o }
    & $Harness --core $Core --rom $rom --content-path "$Jk\mod" @opts --pace turbo --out $dir --script "$dir\run.cmds" 2>&1 | Out-Null
    return $LASTEXITCODE
}

# Straight into episode P03, as the desktop's -autostart does; the harness exit code is the number of failed lines.
function Boot($dir) {
    Harness $dir "$Jk\episode\P03.GOB" @("expect `"world 'piano.jkl' (episode 'P03')`" 1800")
}

# Title > main menu > Singleplayer > New Game (p69's clicks, in 640x480 menu pixels), a screenshot at each screen.
function Menu($dir) {
    $s = @('timestep 20fps', 'expect "[CF] timestep: FIXED at 50.0000 ms/frame" 300', 'run 200', "shot $dir\1-title.png",
           'key tap RETURN', 'run 30', "shot $dir\2-main.png")
    $n = 3
    foreach ($xy in @(318, 187), @(97, 109), @(294, 109)) {		# Singleplayer, (Debug): its levels, New Game: the episodes
        $s += 'mouse move -5000 -5000', 'run 2', "mouse move $($xy[0]) $($xy[1])", 'run 5', 'mouse down left', 'run 1',
              'mouse up left', 'run 40', "shot $dir\$n-after-click.png"
        $n++
    }
    Harness $dir "$Jk\episode\JK1.GOB" $s @('openjkdf2_boot=menu') | Out-Null
}

# resource+entry: P03 in resource, plus an empty Episode subfolder. An Episode entry with no episode.jk of its own
# reads the first one in resource (how the user's Steam JK lists Resource\DuelOfFates_SE.gob, 2026-10-27).
$Entry = "$Jk\episode\Gobinator"
if (Test-Path -LiteralPath $Entry) { throw "$Entry is already there: remove it first" }
# From the episode list, choose the first episode and OK: must it reach piano.jkl? The exit code is the failed lines.
function Play($dir) {
    $s = @('timestep 20fps', 'expect "[CF] timestep: FIXED at 50.0000 ms/frame" 300', 'run 200', 'key tap RETURN', 'run 30')
    foreach ($xy in @(318, 187), @(294, 109), @(330, 220), @(497, 449)) {	# Singleplayer, New Game, row 1, Ok
        $s += 'mouse move -5000 -5000', 'run 2', "mouse move $($xy[0]) $($xy[1])", 'run 5', 'mouse down left', 'run 1',
              'mouse up left', 'run 40'
    }
    $s += "expect `"world 'piano.jkl'`" 1800", "shot $dir\playing.png"
    Harness $dir "$Jk\episode\JK1.GOB" $s @('openjkdf2_boot=menu')
}

$results = @()
try {
    foreach ($place in 'mod', 'resource', 'mods', 'nowhere', 'resource+entry') {
        $folder = $place -replace '\+entry$', ''
        if ($folder -ne 'nowhere') { Copy-Item $Gob "$Jk\$folder\P03.GOB" }
        if ($place -like '*+entry') { New-Item -ItemType Directory $Entry | Out-Null }
        try {
            $boot = Boot "$Out\$place-boot"
            Menu "$Out\$place-menu"
            $play = if ($place -like '*+entry') { (Play "$Out\$place-play") -eq 0 } else { '' }
        }
        finally {
            if ($folder -ne 'nowhere') { Remove-Item -LiteralPath "$Jk\$folder\P03.GOB" -Force }
            if (Test-Path -LiteralPath $Entry) { Remove-Item -LiteralPath $Entry -Force }
        }
        $results += [pscustomobject]@{ Place = $place; 'Boots into piano.jkl' = ($boot -eq 0)
                                       'Listed entry plays it' = $play; Screens = "$Out\$place-menu" }
    }
}
finally {
    foreach ($d in 'mod', 'resource', 'mods') { Remove-Item -LiteralPath "$Jk\$d\P03.GOB" -Force -ErrorAction SilentlyContinue }
    if (Test-Path -LiteralPath $Entry) { Remove-Item -LiteralPath $Entry -Force }
}
$results | Format-Table -AutoSize | Out-String
"Read each menu folder's last screenshot for the episode list."
exit 0
