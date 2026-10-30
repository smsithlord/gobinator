# v1.1's acceptance scenarios (docs/PLAN.md, part 9): v1.0's scenarios from docs/OVERVIEW.md, and more, each with
# the result v1.1 must give. Runs build\Release\gobinator.exe in throwaway JK-like installs in TEMP, against the fake
# JK (tests/fakejk.c), with the user's real ini (reference/gobinator.ini). Prints PASS or FAIL per check and exits 1
# if any failed.
#
#   cmake --build build --config Release
#   & tests\run-v11-scenarios.ps1
#
# v1.1 copies the GOB into Episode (it stays there), makes an empty folder of its name beside it, which lists it
# first, removes the folder it made for the episode before, and starts JK with -path mod; gobinator.exe with no GOB
# removes its folders (docs/DECISIONS.md, 2026-10-27). The player's own files in mod must never change. A GOB with no
# episode.jk (a mod), or one that cannot be read as a GOB, is left where it is, and JK starts at its menu; OpenJKDF2
# starts single-player episodes with -sp and hosts deathmatch ones with -mp (2026-10-29).
#
# v1.1 starts JK by its full path, with no cmd and no PATH search, so it needs no decoy (compare run-v1-scenarios.ps1).
# GOBINATOR_NO_MESSAGES keeps every failure in gobinator.log, so no dialog ever waits for a click.
param([string]$Bin  = "$PSScriptRoot\..\build\Release",
      [string]$Root = (Join-Path ([IO.Path]::GetTempPath()) 'gobinator-v11-tests'))

$ErrorActionPreference = 'Stop'
$Root = [IO.Path]::GetFullPath($Root)
if (-not $Root.StartsWith([IO.Path]::GetFullPath([IO.Path]::GetTempPath()))) { throw "refusing to run outside TEMP: $Root" }
foreach ($f in "$Bin\gobinator.exe", "$Bin\fakejk.exe", "$Bin\fakesteamjk.exe") {
    if (-not (Test-Path -LiteralPath $f)) { throw "not built: $f" }
}
if (Test-Path $Root) { Remove-Item -Recurse -Force $Root }
$env:GOBINATOR_NO_MESSAGES = '1'

$UsersIni = [IO.File]::ReadAllText("$PSScriptRoot\..\reference\gobinator.ini")
$script:Failed = 0

function Write-Text($path, $text, [bool]$bom = $false) {
    [IO.File]::WriteAllText($path, $text, (New-Object Text.UTF8Encoding $bom))
}

function Check($what, [bool]$ok) {
    if ($ok) { "  PASS  $what" } else { "  FAIL  $what"; $script:Failed++ }
}

function Names($folder) {
    if (-not (Test-Path -LiteralPath $folder)) { return '' }
    (@(Get-ChildItem -LiteralPath $folder -File -Name | Sort-Object)) -join ' '
}
function Folders($folder) {
    if (-not (Test-Path -LiteralPath $folder)) { return '' }
    (@(Get-ChildItem -LiteralPath $folder -Directory -Name | Sort-Object)) -join ' '
}
function Sorted([string[]]$names) { (@($names | Sort-Object)) -join ' ' }

# A JK-like install: gobinator.exe, the ini, the fake JK under each of $programs' names, $other as empty files, the
# player's $modFiles in mod, and Episode holding LucasArts' JK1.GOB.
function New-Install($name, [string[]]$modFiles, $ini = $UsersIni, [bool]$bom = $false,
                     [string[]]$programs = @('jk.exe', 'inject.exe'), [string[]]$other = @()) {
    $base = Join-Path $Root $name
    $jk = "$base\Fake JK Install"
    $t = @{ Base = $base; Jk = $jk; Dl = "$base\Downloads"; Mod = "$jk\mod"; Ep = "$jk\Episode" }
    New-Item -ItemType Directory -Force $t.Mod, $t.Dl, $t.Ep | Out-Null
    Copy-Item "$Bin\gobinator.exe" $t.Jk
    foreach ($p in $programs) { Copy-Item "$Bin\fakejk.exe" "$($t.Jk)\$p" }
    foreach ($f in $other) { Write-Text "$($t.Jk)\$f" '' }
    if ($null -ne $ini) { Write-Text "$($t.Jk)\gobinator.ini" $ini $bom }
    foreach ($f in $modFiles) { Write-Text "$($t.Mod)\$f" "player's own $f" }
    Write-Text "$($t.Ep)\JK1.GOB" "LucasArts' JK1.GOB"
    return $t
}

# A real GOB: "GOB ", version 20, the table after the header, then the files ($files: entry name = its text).
function New-RealGob($folder, $name, $files) {
    New-Item -ItemType Directory -Force $folder | Out-Null
    $bodies = New-Object 'Collections.Generic.List[byte[]]'
    foreach ($v in $files.Values) { $bodies.Add([Text.Encoding]::ASCII.GetBytes($v)) }
    $s = New-Object IO.MemoryStream
    $w = New-Object IO.BinaryWriter $s
    $w.Write([Text.Encoding]::ASCII.GetBytes('GOB ')); $w.Write([uint32]20); $w.Write([uint32]12)
    $w.Write([uint32]$files.Count)
    $at = 12 + 4 + 136 * $files.Count
    $i = 0
    foreach ($k in $files.Keys) {
        $w.Write([uint32]$at); $w.Write([uint32]$bodies[$i].Length)
        $entry = New-Object byte[] 128; [Text.Encoding]::ASCII.GetBytes($k).CopyTo($entry, 0); $w.Write($entry)
        $at += $bodies[$i].Length; $i++
    }
    foreach ($b in $bodies) { $w.Write($b) }
    $w.Flush()
    $p = Join-Path $folder $name
    [IO.File]::WriteAllBytes($p, $s.ToArray())
    return $p
}

# An episode GOB: its episode.jk gives its title, its TYPE (1 single player, 2 deathmatch, 8 CTF) and first level.
function New-EpisodeGob($folder, $name, $level, $type = 1, $title = 'Test') {
    New-RealGob $folder $name ([ordered]@{
        'episode.jk' = "`"$title`"`r`nTYPE $type`r`nSEQ 1`r`n10: 1 1 LEVEL $level 0 0 -1 -1`r`nend`r`n" })
}

# A mod: a GOB with no episode.jk.
function New-ModGob($folder, $name) {
    New-RealGob $folder $name ([ordered]@{ 'mat\saber.mat' = 'MAT'; 'cog\saber.cog' = '# a saber cog' })
}

# A file; one named .gob is an episode GOB titled $content, so two of them differ when their contents do.
function New-Gob($folder, $name, $content = "episode $name") {
    if ($name -match '\.gob$') { return New-EpisodeGob $folder $name '01first.jkl' 1 $content }
    New-Item -ItemType Directory -Force $folder | Out-Null
    $p = Join-Path $folder $name
    Write-Text $p $content
    return $p
}

# Whether the file at $path is there and holds $text: a GOB's title, or a text file's words.
function Holds($path, $text) { (Test-Path -LiteralPath $path) -and [IO.File]::ReadAllText($path).Contains($text) }

# Makes the fake JK at $exe an OpenJKDF2 that knows these switches: strings standing on their own (a NUL before and
# after), the way OpenJKDF2 keeps them, appended after the EXE's image, where Windows ignores them.
function Add-Switches($exe, [string[]]$switches) {
    $bytes = [Text.Encoding]::ASCII.GetBytes("`0" + ($switches -join "`0") + "`0")
    $f = [IO.File]::Open($exe, 'Append')
    $f.Write($bytes, 0, $bytes.Length)
    $f.Close()
}
$Upstream = @('-path', '-devMode', '-autostart', '-sp', '-mp', '-episode', '-map')	# OpenJKDF2 v0.9.1 to v0.9.9

# Double-clicks: starts gobinator.exe as Explorer's association does ("...\gobinator.exe" "%1"), from $cwd.
function Run($t, $gob, $cwd) {
    $log = "$($t.Jk)\launched.txt"
    if (Test-Path $log) { Remove-Item $log }
    # -PassThru and WaitForExit, not -Wait, which would also wait for the JK that gobinator.exe starts.
    $sp = @{ FilePath = "$($t.Jk)\gobinator.exe"; WorkingDirectory = $cwd; PassThru = $true }
    if ($gob) { $sp.ArgumentList = '"' + $gob + '"' }
    $p = Start-Process @sp
    $p.WaitForExit()
    for ($i = 0; $i -lt 30 -and -not (Test-Path $log); $i++) { Start-Sleep -Milliseconds 100 }
    $gl = "$($t.Jk)\gobinator.log"
    return @{
        Exit     = $p.ExitCode
        Launched = if (Test-Path $log) { (Get-Content -LiteralPath $log -Encoding UTF8) -join "`n" } else { '' }
        Log      = if (Test-Path $gl) { (Get-Content -LiteralPath $gl -Encoding UTF8) -join "`n" } else { '' }
    }
}

function Assert-Launched($t, $r, $cmdline = 'inject.exe -devmode -path mod') {
    Check "exit code 0" ($r.Exit -eq 0)
    # Windows gives a program named JK.EXE its full quoted path as the command line's first word, whoever starts it
    # (v1.0's START did the same: run-v1-scenarios.ps1, F and G); its arguments arrive unchanged.
    $name, $rest = $cmdline -split ' ', 2
    $asWindowsGivesIt = "`"$($t.Jk)\$name`" $rest"
    $got = $r.Launched.Contains("cmdline=$cmdline`n") -or ($name -eq 'JK.EXE' -and $r.Launched.Contains("cmdline=$asWindowsGivesIt`n"))
    Check "started: $cmdline" $got
    Check "started in the JK folder" ($r.Launched.Contains("cwd=$($t.Jk)`n"))
    Check "no .bat written anywhere" (@(Get-ChildItem -LiteralPath $t.Base -Recurse -Filter *.bat).Count -eq 0)
}

# Episode holds $files (besides JK1.GOB and the record), and the one empty folder $folder, which lists $gob first.
function Assert-Listed($t, [string[]]$files, $folder) {
    Check "Episode holds $($files -join ', ') beside JK1.GOB" ((Names $t.Ep) -eq (Sorted ($files + 'JK1.GOB', 'gobinator.state')))
    Check "the folder Episode\$folder, empty, and no other" ((Folders $t.Ep) -eq $folder -and -not (Get-ChildItem -LiteralPath "$($t.Ep)\$folder"))
}

# Nothing copied or listed, and nothing of the player's changed.
function Assert-Untouched($t, $modBefore, $epBefore = 'JK1.GOB') {
    Check "mod folder unchanged" ((Names $t.Mod) -eq $modBefore)
    Check "Episode unchanged, no folder made" ((Names $t.Ep) -eq $epBefore -and (Folders $t.Ep) -eq '')
}

function Assert-Refused($t, $r, $modBefore, $message) {
    Check "exit code 1" ($r.Exit -eq 1)
    Check "JK not started" ($r.Launched -eq '')
    Check "the log says: $message" ($r.Log.Contains('error: ') -and $r.Log.Contains($message))
    Assert-Untouched $t $modBefore
}

$e = [char]0xC9; $ue = [char]0xFC; $jp = "$([char]0x65E5)$([char]0x672C)$([char]0x8A9E)"; $cafe = "Caf$([char]0xE9)"
$own = @('Bfp2x.gob', 'JK1.GOB', 'keep.gob', 'my.goblin.cfg', 'notes.gob.txt', 'other.gob', 'readme.txt')

"A: double-click Downloads\Episode.gob, with the player's own files in mod"
$t = New-Install 'A' $own
$r = Run $t (New-Gob $t.Dl 'Episode.gob') $t.Dl
Assert-Launched $t $r
Check "nothing of the player's touched" ((Names $t.Mod) -eq (Sorted $own))
Assert-Listed $t @('Episode.gob') 'Episode'
Check "the log says it copied Episode.gob" ($r.Log.Contains('copied Episode.gob'))

"B: double-click EPISODE.GOB (upper case)"
$t = New-Install 'B' @('other.gob')
$r = Run $t (New-Gob $t.Dl 'EPISODE.GOB') $t.Dl
Assert-Launched $t $r
Assert-Listed $t @('EPISODE.GOB') 'EPISODE'
Check "other.gob kept" ((Names $t.Mod) -eq 'other.gob')

"C: double-click mod\mine.gob itself"
$t = New-Install 'C' @('keep.gob')
$null = New-Gob $t.Mod 'mine.gob' "player's own mine.gob"
$r = Run $t "$($t.Mod)\mine.gob" $t.Mod
Assert-Launched $t $r
Check "mine.gob still there, untouched" ((Names $t.Mod) -eq (Sorted @('keep.gob', 'mine.gob')) -and (Holds "$($t.Mod)\mine.gob" "player's own mine.gob"))
Assert-Listed $t @('mine.gob') 'mine'

"C2: double-click Episode\Dralloc Sirhc 3.gob, already one of the episodes: used where it is, listed first"
$t = New-Install 'C2' @('keep.gob')
$r = Run $t (New-Gob $t.Ep 'Dralloc Sirhc 3.gob' 'already there') $t.Ep
Assert-Launched $t $r
Assert-Listed $t @('Dralloc Sirhc 3.gob') 'Dralloc Sirhc 3'
Check "untouched, and the log says it is using it" ((Holds "$($t.Ep)\Dralloc Sirhc 3.gob" 'already there') -and $r.Log.Contains('using Dralloc Sirhc 3.gob'))

"D1: double-click Downloads\${e}pisode ${ue}ber.gob"
$t = New-Install 'D1' @('other.gob')
$r = Run $t (New-Gob $t.Dl "${e}pisode ${ue}ber.gob" 'accented') $t.Dl
Assert-Launched $t $r
Check "copied as Episode_uber.gob" (Holds "$($t.Ep)\Episode_uber.gob" 'accented')
Assert-Listed $t @('Episode_uber.gob') 'Episode_uber'

"D2: double-click Downloads\$jp.gob"
$t = New-Install 'D2' @('other.gob')
$r = Run $t (New-Gob $t.Dl "$jp.gob" 'japanese') $t.Dl
Assert-Launched $t $r
Check "copied as episode.gob" (Holds "$($t.Ep)\episode.gob" 'japanese')

"E: double-click Downloads\$cafe\level.gob"
$t = New-Install 'E' @('other.gob')
$r = Run $t (New-Gob "$($t.Dl)\$cafe" 'level.gob' 'from cafe') "$($t.Dl)\$cafe"
Assert-Launched $t $r
Check "copied as level.gob" (Holds "$($t.Ep)\level.gob" 'from cafe')

"F: the user's ini saved with a BOM"
$t = New-Install 'F' @('keep.gob', 'other.gob') $UsersIni $true
$r = Run $t (New-Gob $t.Dl 'Episode.gob') $t.Dl
Assert-Launched $t $r
Check "keep.gob and other.gob kept" ((Names $t.Mod) -eq (Sorted @('keep.gob', 'other.gob')))
Assert-Listed $t @('Episode.gob') 'Episode'

"G: the user's ini with a blank line before MOD_FOLDER"
$t = New-Install 'G' @('keep.gob', 'other.gob') ($UsersIni -replace "(\r?\n)MOD_FOLDER(\r?\n)", '$1$1MOD_FOLDER$2')
$r = Run $t (New-Gob $t.Dl 'Episode.gob') $t.Dl
Assert-Launched $t $r
Check "keep.gob and other.gob kept" ((Names $t.Mod) -eq (Sorted @('keep.gob', 'other.gob')))
Assert-Listed $t @('Episode.gob') 'Episode'

"H: gobinator.exe started with no GOB, after one: its folder comes out, the GOB stays, JK starts as usual"
$t = New-Install 'H' @('keep.gob', 'other.gob')
$null = Run $t (New-Gob $t.Dl 'Episode.gob') $t.Dl
$r = Run $t $null $t.Jk
Assert-Launched $t $r
Assert-Untouched $t (Sorted @('keep.gob', 'other.gob')) (Sorted @('Episode.gob', 'JK1.GOB'))
Check "the log says it removed Episode\Episode" ($r.Log.Contains('removed Episode\Episode') -and $r.Log.Contains('JK starts with its usual episode list'))

"H2: gobinator.exe started with no GOB, with nothing to clean up"
$t = New-Install 'H2' @('keep.gob')
$r = Run $t $null $t.Jk
Assert-Launched $t $r
Assert-Untouched $t 'keep.gob'
Check "the log says there was nothing to clean up" ($r.Log.Contains("nothing of Gobinator's to clean up"))

"H3: a copy an earlier v1.1 build left in mod comes out (from mod it would hide every other episode)"
$t = New-Install 'H3' @('keep.gob')
$old = New-EpisodeGob $t.Mod 'bunker.gob' 'bunker.jkl'
$i = Get-Item -LiteralPath $old
Write-Text "$($t.Mod)\gobinator.state" "# an earlier build's record`r`nbunker.gob`t$($i.Length)`t$($i.LastWriteTimeUtc.ToFileTimeUtc())`r`n"
$r = Run $t (New-Gob $t.Dl 'Episode.gob') $t.Dl
Assert-Launched $t $r
Check "bunker.gob and its record gone from mod, keep.gob kept" ((Names $t.Mod) -eq 'keep.gob')
Check "the log says it removed bunker.gob" ($r.Log.Contains('removed bunker.gob'))
Assert-Listed $t @('Episode.gob') 'Episode'

"H4: a folder of the player's of the episode's name (DeaTH-WaLK's, with its readme) is used, and never removed"
$t = New-Install 'H4' @('keep.gob')
$dw = New-Gob $t.Ep 'DeaTH-WaLK.gob' 'death walk'
$null = New-Gob "$($t.Ep)\DeaTH-WaLK" 'dw_readme.txt' "the player's"
$r = Run $t $dw $t.Ep
Assert-Launched $t $r
Check "nothing recorded" (-not (Test-Path -LiteralPath "$($t.Ep)\gobinator.state"))
$r = Run $t $null $t.Jk
Check "after gobinator.exe alone: the folder and its readme are still there" ((Folders $t.Ep) -eq 'DeaTH-WaLK' -and (Names "$($t.Ep)\DeaTH-WaLK") -eq 'dw_readme.txt')

"I: no gobinator.ini, in a JK folder: the defaults apply, and the packaged ini is written beside the EXE"
$t = New-Install 'I' @('other.gob') $null $false @('JK.EXE')
$r = Run $t (New-Gob $t.Dl 'Episode.gob') $t.Dl
Assert-Launched $t $r 'JK.EXE -devmode -path mod'
Check "the GOB is one of the episodes" (Holds "$($t.Ep)\Episode.gob" 'episode Episode.gob')
$ini = if (Test-Path "$($t.Jk)\gobinator.ini") { [IO.File]::ReadAllText("$($t.Jk)\gobinator.ini") } else { '' }
Check "gobinator.ini written with the packaged text, nothing set" ($ini.Contains('# JK_FOLDER =') -and $ini.Contains('# MOD_FOLDER = mod'))
Check "the log says so" ($r.Log.Contains('wrote ') -and $r.Log.Contains('with the default settings'))

"J: a second GOB: the first stays one of the episodes, only the second is listed first"
$t = New-Install 'J' @('keep.gob')
$null = Run $t (New-Gob $t.Dl 'One.gob') $t.Dl
$r = Run $t (New-Gob $t.Dl 'Two.gob') $t.Dl
Assert-Launched $t $r
Assert-Listed $t @('One.gob', 'Two.gob') 'Two'
Check "keep.gob kept" ((Names $t.Mod) -eq 'keep.gob')
Check "the log says it removed Episode\One" ($r.Log.Contains('removed Episode\One'))

"K: JK_EXECUTABLE names a program that is not there"
$t = New-Install 'K' @('keep.gob') ($UsersIni -replace 'inject\.exe', 'nothing.exe')
$r = Run $t (New-Gob $t.Dl 'Episode.gob') $t.Dl
Assert-Refused $t $r 'keep.gob' 'JK was not found'

"L: MOD_FOLDER with a space"
$t = New-Install 'L' @('keep.gob') "JK_EXECUTABLE = inject.exe`r`nMOD_FOLDER = my mods`r`n"
$r = Run $t (New-Gob $t.Dl 'Episode.gob') $t.Dl
Assert-Refused $t $r 'keep.gob' 'contains a space'

"M: double-click something that is not a GOB"
$t = New-Install 'M' @('keep.gob')
$r = Run $t (New-Gob $t.Dl 'readme.txt' 'text') $t.Dl
Assert-Refused $t $r 'keep.gob' 'is not a .gob file'

"N: a different GOB of the same name is already one of the episodes"
$t = New-Install 'N' @('keep.gob')
$null = New-Gob $t.Ep 'Episode.gob' "player's own Episode.gob"
$r = Run $t (New-Gob $t.Dl 'Episode.gob' 'downloaded') $t.Dl
Assert-Launched $t $r
Check "the player's Episode.gob untouched" (Holds "$($t.Ep)\Episode.gob" "player's own Episode.gob")
Check "the download copied as Episode_2.gob" (Holds "$($t.Ep)\Episode_2.gob" 'downloaded')
Assert-Listed $t @('Episode.gob', 'Episode_2.gob') 'Episode_2'

"O: nothing set in the ini; JK Remastered is detected (inject.exe with jkgm.json)"
$t = New-Install 'O' @('keep.gob') "# nothing set`r`n" $false @('inject.exe', 'JediKnight.exe') @('jkgm.json')
$r = Run $t (New-Gob $t.Dl 'Episode.gob') $t.Dl
Assert-Launched $t $r
Check "the log names JK Remastered, found in the JK folder" ($r.Log.Contains('JK: JK Remastered') -and $r.Log.Contains('found in the JK folder'))

"P: nothing set; the community-patched JK is detected (JediKnight.exe beside JK.EXE)"
$t = New-Install 'P' @('keep.gob') "# nothing set`r`n" $false @('JediKnight.exe', 'JK.EXE', 'inject.exe')
$r = Run $t (New-Gob $t.Dl 'Episode.gob') $t.Dl
Assert-Launched $t $r 'JediKnight.exe -devmode -path mod'

"Q: nothing set, and no JK program in the folder: asked where JK is, and the player cancels"
$t = New-Install 'Q' @('keep.gob') "# nothing set`r`n" $false @()
Remove-Item env:GOBINATOR_PICK_FOLDER -ErrorAction SilentlyContinue
$r = Run $t (New-Gob $t.Dl 'Episode.gob') $t.Dl
Assert-Refused $t $r 'keep.gob' 'No JK program was found'
Check "the log says it asked, and got no folder" ($r.Log.Contains('asking where JK is') -and $r.Log.Contains('no folder chosen'))
Check "the ini is unchanged" ([IO.File]::ReadAllText("$($t.Jk)\gobinator.ini") -eq "# nothing set`r`n")

"R: OpenJKDF2 is detected and started straight into the GOB's first level"
$t = New-Install 'R' @('keep.gob') "# nothing set`r`n" $false @('openjkdf2-64.exe', 'JK.EXE')
Add-Switches "$($t.Jk)\openjkdf2-64.exe" $Upstream
$r = Run $t (New-EpisodeGob $t.Dl 'My Episode.gob' '01first.jkl') $t.Dl
Assert-Launched $t $r 'openjkdf2-64.exe -devmode -path mod -autostart -sp -episode My_Episode -map 01first.jkl'
Check "the log says: autostart" ($r.Log.Contains('autostart: episode My_Episode (My_Episode.gob), level 01first.jkl'))

"R2: an OpenJKDF2 from before v0.9.1 (no -autostart in its EXE) is started at its menu, not made to quit"
$t = New-Install 'R2' @('keep.gob') "# nothing set`r`n" $false @('openjkdf2-64.exe')
Add-Switches "$($t.Jk)\openjkdf2-64.exe" @('-path', '-devMode', '-windowGUI')
$r = Run $t (New-EpisodeGob $t.Dl 'Episode.gob' '01first.jkl') $t.Dl
Assert-Launched $t $r 'openjkdf2-64.exe -devmode -path mod'
Check "the log says why: no autostart" ($r.Log.Contains('no autostart: openjkdf2-64.exe does not know -autostart'))

"R3: the user's fork (its EXE knows -doNotResumeLast) gets it too"
$t = New-Install 'R3' @('keep.gob') "# nothing set`r`n" $false @('openjkdf2-64.exe')
Add-Switches "$($t.Jk)\openjkdf2-64.exe" ($Upstream + '-doNotResumeLast')
$r = Run $t (New-EpisodeGob $t.Dl 'Episode.gob' '01first.jkl') $t.Dl
Assert-Launched $t $r 'openjkdf2-64.exe -devmode -path mod -autostart -sp -episode Episode -map 01first.jkl -doNotResumeLast'

"R4: OpenJKDF2 and Episode\Dralloc Sirhc 3.gob: no autostart (OpenJKDF2 cannot take a space), but listed first"
$t = New-Install 'R4' @('keep.gob') "# nothing set`r`n" $false @('openjkdf2-64.exe')
Add-Switches "$($t.Jk)\openjkdf2-64.exe" $Upstream
$r = Run $t (New-EpisodeGob $t.Ep 'Dralloc Sirhc 3.gob' '01first.jkl') $t.Ep
Assert-Launched $t $r 'openjkdf2-64.exe -devmode -path mod'
Check "the log says why: the space" ($r.Log.Contains('no autostart: OpenJKDF2 cannot take an episode name with a space'))
Assert-Listed $t @('Dralloc Sirhc 3.gob') 'Dralloc Sirhc 3'

"R5: OpenJKDF2 and a multiplayer episode (TYPE 2): hosted on this PC, straight into its first level"
$t = New-Install 'R5' @('keep.gob') "# nothing set`r`n" $false @('openjkdf2-64.exe')
Add-Switches "$($t.Jk)\openjkdf2-64.exe" $Upstream
$r = Run $t (New-EpisodeGob $t.Dl 'SMHQ.gob' 'smhq.jkl' 2) $t.Dl
Assert-Launched $t $r 'openjkdf2-64.exe -devmode -path mod -autostart -mp -episode SMHQ -map smhq.jkl'
Check "the log says: hosted" ($r.Log.Contains('level smhq.jkl, multiplayer: a game hosted on this PC'))
Assert-Listed $t @('SMHQ.gob') 'SMHQ'

"R6: OpenJKDF2 and a Capture the Flag episode (TYPE 8): at the menu, listed first (docs/DECISIONS.md, 2026-10-29)"
$t = New-Install 'R6' @('keep.gob') "# nothing set`r`n" $false @('openjkdf2-64.exe')
Add-Switches "$($t.Jk)\openjkdf2-64.exe" $Upstream
$r = Run $t (New-EpisodeGob $t.Dl 'Ctf.gob' 'ctf01.jkl' 8) $t.Dl
Assert-Launched $t $r 'openjkdf2-64.exe -devmode -path mod'
Check "the log says why: Capture the Flag" ($r.Log.Contains('no autostart: Ctf.gob is a Capture the Flag episode (TYPE 8)'))
Assert-Listed $t @('Ctf.gob') 'Ctf'

"U: a mod (a GOB with no episode.jk): left where it is, nothing changed, JK started at its menu"
$t = New-Install 'U' @('keep.gob') "# nothing set`r`n" $false @('openjkdf2-64.exe')
Add-Switches "$($t.Jk)\openjkdf2-64.exe" $Upstream
$null = Run $t (New-Gob $t.Dl 'Episode.gob') $t.Dl
$saber = New-ModGob $t.Dl 'Sith Sabers.gob'
$r = Run $t $saber $t.Dl
Assert-Launched $t $r 'openjkdf2-64.exe -devmode -path mod'
Check "the log says it is a mod" ($r.Log.Contains('Sith Sabers.gob holds no episode.jk: it is a mod, not an episode'))
Check "mod unchanged, and the mod still in Downloads" ((Names $t.Mod) -eq 'keep.gob' -and (Test-Path -LiteralPath $saber))
Assert-Listed $t @('Episode.gob') 'Episode'

"U2: a .gob that cannot be read as a GOB: left where it is, JK started at its menu"
$t = New-Install 'U2' @('keep.gob')
Write-Text "$($t.Dl)\Broken.gob" 'not a GOB at all'
$r = Run $t "$($t.Dl)\Broken.gob" $t.Dl
Assert-Launched $t $r
Check "the log says it could not be read" ($r.Log.Contains('Broken.gob could not be read as a GOB'))
Assert-Untouched $t 'keep.gob'

"S: retail JK gets no autostart switches"
$t = New-Install 'S' @('keep.gob') "# nothing set`r`n" $false @('JK.EXE')
$r = Run $t (New-EpisodeGob $t.Dl 'Episode.gob' '01first.jkl') $t.Dl
Assert-Launched $t $r 'JK.EXE -devmode -path mod'
Assert-Listed $t @('Episode.gob') 'Episode'

# Steam's own JediKnight.exe starts the install Steam registered, which this TEMP folder never is. The registry is
# only read, here and by Gobinator: with no Steam JK registered, Gobinator cannot tell, and goes on with a note.
$steamJk = (Get-ItemProperty 'HKLM:\SOFTWARE\WOW6432Node\LucasArts Entertainment Company\JediKnight\1.0' `
    -ErrorAction SilentlyContinue).'Install Path'
"T: Steam's JediKnight.exe (DRM'd) in a folder Steam does not start"
$t = New-Install 'T' @('keep.gob') "# nothing set`r`n" $false @('JediKnight.exe')
Copy-Item "$Bin\fakesteamjk.exe" "$($t.Jk)\JediKnight.exe" -Force
$r = Run $t (New-Gob $t.Dl 'Episode.gob') $t.Dl
if ($steamJk -and (Test-Path -LiteralPath $steamJk -PathType Container)) {
    Assert-Refused $t $r 'keep.gob' "JediKnight.exe is Steam's: it hands every start to Steam"
    Check "the message names the folder Steam starts" ($r.Log.Contains("Steam starts the JK in`n$([IO.Path]::GetFullPath($steamJk).TrimEnd('\'))`n"))
}
else {
    Assert-Launched $t $r 'JediKnight.exe -devmode -path mod'
    Check "the log says it cannot check" ($r.Log.Contains("Steam's JK install could not be found"))
}

''
# The fake JK's record in $folder (the JK folder the player chose, not the one gobinator.exe sits in).
function Read-Launched($folder) {
    $log = "$folder\launched.txt"
    for ($i = 0; $i -lt 30 -and -not (Test-Path $log); $i++) { Start-Sleep -Milliseconds 100 }
    if (Test-Path $log) { (Get-Content -LiteralPath $log -Encoding UTF8) -join "`n" } else { '' }
}

"V: no JK program beside gobinator.exe; the player chooses the folder that holds JK: remembered as JK_FOLDER"
$t = New-Install 'V' @('keep.gob') "# nothing set`r`n" $false @()
$real = New-Install 'V-real' @('mine.gob') $null $false @('JK.EXE')
$env:GOBINATOR_PICK_FOLDER = $real.Jk
$r = Run $t (New-Gob $t.Dl 'Episode.gob') $t.Dl
Remove-Item env:GOBINATOR_PICK_FOLDER
$launched = Read-Launched $real.Jk
Check "exit code 0" ($r.Exit -eq 0)
Check "JK started in the chosen folder" ($launched.Contains("cwd=$($real.Jk)`n") -and $launched.Contains('-devmode -path mod'))
Check "the GOB went into the chosen folder's Episode, listed first" ((Holds "$($real.Ep)\Episode.gob" 'episode Episode.gob') -and (Test-Path "$($real.Ep)\Episode" -PathType Container))
Check "nothing changed beside gobinator.exe" ((Names $t.Ep) -eq 'JK1.GOB' -and (Folders $t.Ep) -eq '' -and (Names $t.Mod) -eq 'keep.gob')
$ini = [IO.File]::ReadAllText("$($t.Jk)\gobinator.ini")
Check "the ini remembers the folder, and keeps its text" ($ini.Contains("JK_FOLDER = $($real.Jk)") -and $ini.Contains('# nothing set'))
Check "the log says so" ($r.Log.Contains("the player chose $($real.Jk)") -and $r.Log.Contains("wrote JK_FOLDER = $($real.Jk) into "))
$r2 = Run $t (New-Gob $t.Dl 'Second.gob') $t.Dl
Check "the next run asks nothing and uses it" ($r2.Exit -eq 0 -and -not $r2.Log.Contains('asking') -and (Holds "$($real.Ep)\Second.gob" 'episode Second.gob'))

"V2: no gobinator.ini and no JK program beside gobinator.exe: asked, and a new gobinator.ini keeps the answer"
$t = New-Install 'V2' @() $null $false @()
$real = New-Install 'V2-real' @() $null $false @('JK.EXE')
$env:GOBINATOR_PICK_FOLDER = $real.Jk
$r = Run $t (New-Gob $t.Dl 'Episode.gob') $t.Dl
Remove-Item env:GOBINATOR_PICK_FOLDER
$ini = if (Test-Path "$($t.Jk)\gobinator.ini") { [IO.File]::ReadAllText("$($t.Jk)\gobinator.ini") } else { '' }
Check "exit code 0" ($r.Exit -eq 0)
Check "a new ini: the packaged text with JK_FOLDER set in place of its example" ($ini.Contains("JK_FOLDER = $($real.Jk)") -and $ini.Contains('# MOD_FOLDER = mod') -and -not $ini.Contains('# JK_FOLDER ='))
Check "the GOB went into the chosen folder's Episode" (Holds "$($real.Ep)\Episode.gob" 'episode Episode.gob')
Check "the log says so" ($r.Log.Contains("with the default settings and JK_FOLDER = $($real.Jk)"))

"V3: the player chooses a folder with no JK program in it (and, asked again, gives up)"
$t = New-Install 'V3' @('keep.gob') "# nothing set`r`n" $false @()
$env:GOBINATOR_PICK_FOLDER = $t.Dl
$r = Run $t (New-Gob $t.Dl 'Episode.gob') $t.Dl
Remove-Item env:GOBINATOR_PICK_FOLDER
Assert-Refused $t $r 'keep.gob' 'No JK program was found'
Check "the log names the folder and says it holds none" ($r.Log.Contains("the player chose $($t.Dl), which holds no JK program"))
Check "the ini is unchanged" ([IO.File]::ReadAllText("$($t.Jk)\gobinator.ini") -eq "# nothing set`r`n")

"V4: JK_FOLDER names a folder that holds no JK any more (the game moved): asked, and the new answer replaces it"
$t = New-Install 'V4' @('keep.gob') "JK_FOLDER = $Root\V4\gone`r`n" $false @()
New-Item -ItemType Directory -Force "$Root\V4\gone" | Out-Null
$real = New-Install 'V4-real' @() $null $false @('JK.EXE')
$env:GOBINATOR_PICK_FOLDER = $real.Jk
$r = Run $t (New-Gob $t.Dl 'Episode.gob') $t.Dl
Remove-Item env:GOBINATOR_PICK_FOLDER
$ini = [IO.File]::ReadAllText("$($t.Jk)\gobinator.ini")
Check "exit code 0" ($r.Exit -eq 0)
Check "JK_FOLDER replaced, not doubled" ($ini.Contains("JK_FOLDER = $($real.Jk)") -and -not $ini.Contains('gone'))
Check "the GOB went into the chosen folder's Episode" (Holds "$($real.Ep)\Episode.gob" 'episode Episode.gob')

if ($script:Failed) { "$($script:Failed) check(s) FAILED"; exit 1 } else { 'all scenarios passed'; exit 0 }
