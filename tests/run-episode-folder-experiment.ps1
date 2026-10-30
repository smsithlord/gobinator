# Does an empty Episode folder named like an episode GOB put that episode at the top of JK's list, once, and past the
# episode limit? An experiment, with no window, on the OpenJKDF2 test install (the user's question, 2026-10-27).
#
# JK lists Episode's subfolders before its GOB files, and an entry named X loads Episode\X.gob as its episode GOB
# (jkEpisode.c, jkRes.c), so Episode\P03\ beside Episode\P03.GOB should be listed first, and only once. Four layouts:
#
#   alone         P03.GOB in episode beside JK1.GOB: listed after JK1 (J before P)
#   folder        the same plus an empty episode\P03 folder: P03 first?
#   full          515 filler episodes (A000..A514) before P03.GOB, past this engine's cap of 512 (retail's is 64):
#                 P03 not listed at all
#   full+folder   the same plus episode\P03: P03 first?
#
# Each: the menus are clicked through to New Game's episode list (screenshotted), then the first row is chosen and
# played; the harness must reach piano.jkl when P03 is that row.
#
#   & tests\make-openjkdf2-install.ps1      (once)
#   & tests\run-episode-folder-experiment.ps1
#
# The engine is openjkdf2_libretro's headless harness and core (the jk-content skill), used read-only. The test
# install is put back as it was.
param([string]$Libretro = 'G:\Documents Sym Links\GitHub\openjkdf2_libretro')

$ErrorActionPreference = 'Stop'
$Repo = [IO.Path]::GetFullPath("$PSScriptRoot\..")
$Jk = "$Repo\testinstall\openjkdf2"
$Gob = "$Repo\testinstall\fixtures\P03.GOB"
$Harness = "$Libretro\build_harness\Release\openjkdf2_harness.exe"
$Core = "$Libretro\build_libretro\Release\openjkdf2_libretro.dll"
foreach ($f in "$Jk\episode\JK1.GOB", $Gob, $Harness, $Core) {
    if (-not (Test-Path -LiteralPath $f)) { throw "missing: $f (run make-openjkdf2-install.ps1)" }
}
$Fillers = 515
$Copy = "$Jk\episode\P03.GOB"
$Entry = "$Jk\episode\P03"
if ((Test-Path -LiteralPath $Copy) -or (Test-Path -LiteralPath $Entry) -or (Test-Path "$Jk\episode\A000.gob")) {
    throw "left over from an earlier run in $Jk\episode: remove it first"
}
$Out = Join-Path ([IO.Path]::GetTempPath()) 'gobinator-episode-folder'
if (Test-Path $Out) { Remove-Item -Recurse -Force $Out }

# An episode GOB holding only an episode.jk titled $title, whose one level does not exist.
function New-Filler($path, $title) {
    $jk = [Text.Encoding]::ASCII.GetBytes("`"$title`"`r`nTYPE 1`r`nSEQ 1`r`n10: 1 1 LEVEL filler.jkl 0 0 -1 -1`r`nend`r`n")
    $s = New-Object IO.MemoryStream
    $w = New-Object IO.BinaryWriter $s
    $w.Write([Text.Encoding]::ASCII.GetBytes('GOB ')); $w.Write([uint32]20); $w.Write([uint32]12)
    $w.Write([uint32]1); $w.Write([uint32](12 + 4 + 136)); $w.Write([uint32]$jk.Length)
    $entry = New-Object byte[] 128; [Text.Encoding]::ASCII.GetBytes('episode.jk').CopyTo($entry, 0); $w.Write($entry)
    $w.Write($jk); $w.Flush()
    [IO.File]::WriteAllBytes($path, $s.ToArray())
}

# Title > main menu > Singleplayer > New Game (a screenshot of the episode list) > the first row > Ok. Returns the
# harness exit code: 0 when the first row was P03 and piano.jkl was reached.
function Play($name) {
    $dir = "$Out\$name"
    New-Item -ItemType Directory -Force $dir | Out-Null
    $s = @('timestep 20fps', 'expect "[CF] timestep: FIXED at 50.0000 ms/frame" 300', 'run 200', 'key tap RETURN', 'run 30')
    $n = 0
    foreach ($xy in @(318, 187), @(294, 109), @(330, 220), @(497, 449)) {	# Singleplayer, New Game, row 1, Ok
        $s += 'mouse move -5000 -5000', 'run 2', "mouse move $($xy[0]) $($xy[1])", 'run 5', 'mouse down left', 'run 1',
              'mouse up left', 'run 40'
        if (++$n -eq 2) { $s += "shot $dir\episodes.png" }
    }
    $s += "expect `"world 'piano.jkl' (episode 'P03')`" 1800"
    Set-Content "$dir\run.cmds" ($s -join "`r`n") -Encoding ascii
    & $Harness --core $Core --rom "$Jk\episode\JK1.GOB" --content-path "$Jk\mod" --option openjkdf2_portable=no_writes `
        --option openjkdf2_cogfactory=enabled --option openjkdf2_boot=menu --pace turbo --out $dir --script "$dir\run.cmds" 2>&1 | Out-Null
    return $LASTEXITCODE
}

$results = @()
try {
    Copy-Item $Gob $Copy
    foreach ($layout in 'alone', 'folder', 'full', 'full+folder') {
        if ($layout -like 'full*' -and -not (Test-Path "$Jk\episode\A000.gob")) {
            for ($i = 0; $i -lt $Fillers; $i++) { New-Filler ("$Jk\episode\A{0:000}.gob" -f $i) ("Filler {0:000}" -f $i) }
        }
        if ($layout -like '*folder') { New-Item -ItemType Directory -Force $Entry | Out-Null }
        elseif (Test-Path -LiteralPath $Entry) { Remove-Item -LiteralPath $Entry -Force }
        $code = Play $layout
        $full = Select-String -LiteralPath "$Out\$layout\events.jsonl" -Pattern 'episode list full' -Quiet
        $results += [pscustomobject]@{ Layout = $layout; 'First row is P03 and plays' = ($code -eq 0); 'List full' = [bool]$full
                                       Screenshot = "$Out\$layout\episodes.png" }
    }
}
finally {
    Remove-Item -LiteralPath $Copy -Force -ErrorAction SilentlyContinue
    if (Test-Path -LiteralPath $Entry) { Remove-Item -LiteralPath $Entry -Force }
    Get-ChildItem -LiteralPath "$Jk\episode" -Filter 'A???.gob' | Remove-Item -Force
}
$results | Format-Table -AutoSize | Out-String -Width 200
exit 0
