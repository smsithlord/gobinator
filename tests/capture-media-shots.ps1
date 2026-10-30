# The listing's screenshots (media/screenshots/), as real frames with no window: the same set-up as
# run-openjkdf2-boot.ps1, rendered at 1920x1080 by openjkdf2_libretro's headless harness.
#
#   1. The player's episode folder holds LucasArts' JK1.GOB and COG Factory's first-generation episodes (copied in for
#      the run). gobinator.exe is handed The Crab Factory (P71.GOB, COG Factory's public release) and makes the empty
#      folder episode\P71 beside it, so Singleplayer > New Game lists it first.  -> episode-list.png
#   2. The engine starts as episode P71, which is what OpenJKDF2's -autostart -episode P71 does, and plays on with
#      nobody at the controls.  -> level-*.png
#   3. gobinator.exe with no GOB removes the folder, and the list is back in its usual order.  -> usual-list.png
#
#   & tests\make-openjkdf2-install.ps1      (once)
#   cmake --build build --config Release
#   & tests\capture-media-shots.ps1
#
# Everything it adds to the test install's episode folder is removed again at the end. Gobinator starts the fake JK
# here, not OpenJKDF2: starting the desktop OpenJKDF2 opens a game window, and that waits for the user's go.
param(
    [string]$Libretro = 'G:\Documents Sym Links\GitHub\openjkdf2_libretro',
    [string]$Release = 'G:\Documents Sym Links\GitHub\openjkdf2_libretro\website\dist\cogfactory\gen2\downloads\releases\p71-crab-factory\f0179c29c427983a6185\p71-crab-factory.zip'
)

$ErrorActionPreference = 'Stop'
$Repo = [IO.Path]::GetFullPath("$PSScriptRoot\..")
$Jk = "$Repo\testinstall\openjkdf2"
$Harness = "$Libretro\build_harness\Release\openjkdf2_harness.exe"
$Core = "$Libretro\build_libretro\Release\openjkdf2_libretro.dll"
foreach ($f in "$Jk\resource\Res2.gob", "$Jk\episode\JK1.GOB", $Release, $Harness, $Core, "$Repo\build\Release\gobinator.exe") {
    if (-not (Test-Path -LiteralPath $f)) { throw "missing: $f (build, and run make-openjkdf2-install.ps1)" }
}
$Out = Join-Path ([IO.Path]::GetTempPath()) 'gobinator-media-shots'
if (Test-Path $Out) { Remove-Item -Recurse -Force $Out }
New-Item -ItemType Directory -Force $Out | Out-Null

# The released GOB, unpacked from COG Factory's own zip.
Add-Type -AssemblyName System.IO.Compression.FileSystem
$zip = [IO.Compression.ZipFile]::OpenRead($Release)
try {
    $entry = $zip.Entries | Where-Object { $_.Name -eq 'P71.GOB' }
    [IO.Compression.ZipFileExtensions]::ExtractToFile($entry, "$Out\P71.GOB")
} finally { $zip.Dispose() }
$Gob = "$Out\P71.GOB"

# The rest of the collection: COG Factory's first-generation episode GOBs, the ones the folder lacks.
$Added = @()
foreach ($g in Get-ChildItem "$Libretro\tools\cogfactory\projects\*\dist\episode\*.GOB") {
    $to = "$Jk\episode\$($g.Name)"
    if (Test-Path -LiteralPath $to) { continue }
    Copy-Item -LiteralPath $g.FullName $to
    $Added += $to
}
$Copy = "$Jk\episode\P71.GOB"
$Entry = "$Jk\episode\P71"
$State = "$Jk\episode\gobinator.state"
foreach ($f in $Copy, $Entry, $State) {
    if (Test-Path -LiteralPath $f) { throw "left over from an earlier run: $f (remove it first)" }
}
$env:GOBINATOR_NO_MESSAGES = '1'

function Harness($name, $rom, [string[]]$lines, [string[]]$options = @()) {
    $dir = "$Out\$name"
    New-Item -ItemType Directory -Force $dir | Out-Null
    Set-Content "$dir\run.cmds" ($lines -join "`r`n") -Encoding ascii
    $opts = @('--option', 'openjkdf2_portable=no_writes', '--option', 'openjkdf2_cogfactory=enabled',
              '--option', 'openjkdf2_resolution=1920x1080')
    foreach ($o in $options) { $opts += '--option', $o }
    & $Harness --core $Core --rom $rom --content-path "$Jk\mod" @opts --pace turbo --out $dir --script "$dir\run.cmds" 2>&1 | Out-Null
    "  $($name): harness exit $LASTEXITCODE"
}

# Title > main menu > Singleplayer > New Game, the list screenshotted. The boot test's clicks are 640x480 menu pixels;
# the core takes window pixels, and at 1920x1080 the menu is a 1440x1080 pillarbox from x 240 (libretro_core.c,
# core_update_mouse_position). The cursor ends in the bottom-right corner, out of the menu.
function List($name) {
    $s = @('timestep 20fps', 'expect "[CF] timestep: FIXED at 50.0000 ms/frame" 300', 'run 200', 'key tap RETURN', 'run 30')
    foreach ($xy in @(318, 187), @(294, 109)) {
        $x = [int](240 + $xy[0] * 2.25); $y = [int]($xy[1] * 2.25)
        $s += 'mouse move -5000 -5000', 'run 2', "mouse move $x $y", 'run 5', 'mouse down left', 'run 1',
              'mouse up left', 'run 40'
    }
    $s += 'mouse move 5000 5000', 'run 10', "shot $Out\$name.png"
    Harness $name "$Jk\episode\JK1.GOB" $s @('openjkdf2_boot=menu')
}

function Gobinator([string]$gob) {
    $sp = @{ FilePath = "$Jk\gobinator.exe"; PassThru = $true }
    if ($gob) { $sp.ArgumentList = "`"$gob`"" }
    $p = Start-Process @sp
    $p.WaitForExit()
    return $p.ExitCode
}

Get-ChildItem -LiteralPath "$Jk\mod" | Remove-Item -Recurse -Force
Copy-Item "$Repo\build\Release\gobinator.exe" $Jk
Copy-Item "$Repo\build\Release\fakejk.exe" $Jk
Set-Content "$Jk\gobinator.ini" "JK_EXECUTABLE = fakejk.exe`r`n" -Encoding ascii
try {
    "$($Added.Count) episodes added to the collection"
    "gobinator.exe P71.GOB: exit $(Gobinator $Gob)"
    List 'episode-list'
    # Started as episode P71; frames at 2, 6, 12 and 20 seconds of play (20 frames a second).
    $s = @('timestep 20fps', "expect `"world 'crabfac.jkl' (episode 'P71')`" 1800", 'run 40', "shot $Out\level-02s.png",
           'run 80', "shot $Out\level-06s.png", 'run 120', "shot $Out\level-12s.png", 'run 160', "shot $Out\level-20s.png")
    Harness 'level' $Copy $s
    "gobinator.exe alone: exit $(Gobinator $null)"
    List 'usual-list'
}
finally {
    foreach ($f in @($Copy, $State) + $Added) { Remove-Item -LiteralPath $f -Force -ErrorAction SilentlyContinue }
    if (Test-Path -LiteralPath $Entry) { Remove-Item -LiteralPath $Entry -Recurse -Force }
}
Get-ChildItem "$Out\*.png" | ForEach-Object { "  $($_.Name)" }
"frames in $Out"
