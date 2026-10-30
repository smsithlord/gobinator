# Builds one of Gobinator's OpenJKDF2 test installs: a JK folder that is ours to write, which a player's OpenJKDF2
# install resembles (docs/PLAN.md, part 9; the board's "An OpenJKDF2 test install for Gobinator").
#
#   & tests\make-openjkdf2-install.ps1        -> testinstall\openjkdf2\ (gitignored) and testinstall\fixtures\
#   & tests\make-openjkdf2-install.ps1 -Engine <folder with openjkdf2-64.exe> -Name openjkdf2-v099 -ProfileFrom openjkdf2
#                                             -> another build in testinstall\openjkdf2-v099\, with that copy's profile
#
# It only COPIES OUT of its sources, which are other folders and stay read-only:
#   - OpenJKDF2 (openjkdf2-64.exe and the DLLs its build ships): by default upstream v0.9.8 from the user's MOTS install;
#   - JK's game data (resource\ without VIDEO, episode\JK1.GOB, a player profile), from openjkdf2_libretro's test data;
#   - a tiny test episode, P03.GOB (the piano room), from openjkdf2_libretro's cogfactory projects.
# OpenJKDF2 writes its settings (registry.json, player\...) into its working directory, which is why it gets a copy.
param([string]$Engine      = 'H:\SteamLibrary\steamapps\common\Jedi Knight Mysteries of the Sith',
      [string]$Name        = 'openjkdf2',
      [string]$ProfileFrom = '',
      [string]$GameData    = 'G:\Documents Sym Links\GitHub\openjkdf2_libretro\testdata\jk1',
      [string]$TestGob     = 'G:\Documents Sym Links\GitHub\openjkdf2_libretro\tools\cogfactory\projects\p03-piano\dist\episode\P03.GOB')

$ErrorActionPreference = 'Stop'
$Repo = [IO.Path]::GetFullPath("$PSScriptRoot\..")
$Out = Join-Path $Repo 'testinstall'
$Jk = Join-Path $Out $Name
$From = if ($ProfileFrom) { Join-Path $Out $ProfileFrom } else { '' }

foreach ($source in "$Engine\openjkdf2-64.exe", "$GameData\resource\JK_.CD", "$GameData\resource\Res2.gob", $TestGob) {
    if (-not (Test-Path -LiteralPath $source)) { throw "missing source: $source" }
}
if ($From -and -not (Test-Path -LiteralPath "$From\player")) { throw "no profile to take from: $From\player" }

New-Item -ItemType Directory -Force "$Jk\resource", "$Jk\episode", "$Jk\mod", "$Out\fixtures" | Out-Null

# The engine and the DLLs an OpenJKDF2 build ships, those that are there (a release zip has fewer). Never the MOTS
# folder's other DLLs: they belong to MOTS Remastered, not OpenJKDF2.
Copy-Item -LiteralPath "$Engine\openjkdf2-64.exe" $Jk -Force
foreach ($dll in 'OpenAL32.dll', 'libssp-0.dll', 'exchndl.dll', 'mgwhelp.dll', 'symsrv.dll', 'libGameNetworkingSockets.dll') {
    if (Test-Path -LiteralPath "$Engine\$dll") { Copy-Item -LiteralPath "$Engine\$dll" $Jk -Force }
}

# JK's data: the resource GOBs and loose cogs, without the cutscene videos; the first episode.
foreach ($f in 'JK_.CD', 'Res1hi.gob', 'Res2.gob') { Copy-Item -LiteralPath "$GameData\resource\$f" "$Jk\resource" }
Copy-Item -LiteralPath "$GameData\resource\cog" "$Jk\resource" -Recurse -Force
Copy-Item -LiteralPath "$GameData\episode\JK1.GOB" "$Jk\episode"

# A player profile: without one, a direct boot into an episode is cancelled for character creation. An existing one
# is kept, with whatever the user set in it (General > "Disable mission start confirmation", for autostart).
# -ProfileFrom takes another test copy's profile and settings, so two builds are tested on the same ones.
if (-not (Test-Path -LiteralPath "$Jk\player")) {
    if ($From) {
        Copy-Item -LiteralPath "$From\player" $Jk -Recurse
        foreach ($f in 'registry.json', 'openjkdf2_cvars.json', 'openjkdf2_binds.json') {
            if (Test-Path -LiteralPath "$From\$f") { Copy-Item -LiteralPath "$From\$f" $Jk }
        }
    }
    else { Copy-Item -LiteralPath "$GameData\player" $Jk -Recurse }
}

Copy-Item -LiteralPath $TestGob "$Out\fixtures" -Force

$size = (Get-ChildItem -LiteralPath $Jk -Recurse -File | Measure-Object Length -Sum).Sum / 1MB
"test install ready: $Jk ({0:N0} MB)" -f $size
