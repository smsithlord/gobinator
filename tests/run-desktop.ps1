# The real check on a desktop JK: gobinator.exe, with nothing set in its ini, is given the P03 test episode in one of
# the test copies in testinstall\ (openjkdf2, openjkdf2-v099, steam-vanilla, ...), and must start that copy's JK.
# **It opens a game window on the user's screen: run it only with the user's go.**
# After $Seconds it screenshots the window, then closes the game (only a program started from that copy's folder).
# Ask the user NOT to touch the game window during the run: a screenshot cannot tell a level reached by itself from one
# reached after a click (2026-10-27: a run read as zero clicks had a click by the user in it).
#
#   & tests\run-desktop.ps1                          OpenJKDF2 v0.9.8 (testinstall\openjkdf2): straight into the level
#   & tests\run-desktop.ps1 -Install openjkdf2-v099  another copy (not a copy of a Steam install: see below)
#
# Two traps this avoids: Start-Process -Wait waits for the whole process tree, so it would sit until the game closed;
# and a screenshot from a process that is not DPI-aware comes out scaled and cropped on a HiDPI screen.
param([int]$Seconds = 15, [string]$Install = 'openjkdf2')

$ErrorActionPreference = 'Stop'
$Repo = [IO.Path]::GetFullPath("$PSScriptRoot\..")
$Jk = "$Repo\testinstall\$Install"
$Shot = Join-Path ([IO.Path]::GetTempPath()) "gobinator-desktop-$Install.png"
foreach ($f in $Jk, "$Repo\testinstall\fixtures\P03.GOB", "$Repo\build\Release\gobinator.exe") {
    if (-not (Test-Path -LiteralPath $f)) { throw "missing: $f" }
}
Add-Type -AssemblyName System.Drawing
Add-Type @'
using System; using System.Text; using System.Runtime.InteropServices;
public static class GobinatorDesktop {
    [StructLayout(LayoutKind.Sequential)] public struct RECT { public int L, T, R, B; }
    [DllImport("user32.dll")] public static extern bool SetProcessDPIAware();
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
    [DllImport("user32.dll")] public static extern IntPtr GetForegroundWindow();
    [DllImport("kernel32.dll")] static extern IntPtr OpenProcess(uint access, bool inherit, int pid);
    [DllImport("kernel32.dll", CharSet = CharSet.Unicode)] static extern bool QueryFullProcessImageNameW(IntPtr h, int flags, StringBuilder name, ref int size);
    [DllImport("kernel32.dll")] static extern bool CloseHandle(IntPtr h);
    // A process's EXE, even one running with admin rights (whose path Win32_Process does not show).
    public static string ExePath(int pid) {
        IntPtr h = OpenProcess(0x1000, false, pid);		// PROCESS_QUERY_LIMITED_INFORMATION
        if (h == IntPtr.Zero) return null;
        var name = new StringBuilder(1024); int size = name.Capacity;
        bool ok = QueryFullProcessImageNameW(h, 0, name, ref size);
        CloseHandle(h);
        return ok ? name.ToString() : null;
    }
}
'@
[void][GobinatorDesktop]::SetProcessDPIAware()

# A program running from the copy's folder, other than Gobinator itself. GOG's JK.EXE runs with admin rights.
function FromTheCopy {
    $folder = "$Jk\".ToLower()
    Get-CimInstance Win32_Process | Where-Object { $_.Name -ne 'gobinator.exe' } | ForEach-Object {
        $path = if ($_.ExecutablePath) { $_.ExecutablePath } else { [GobinatorDesktop]::ExePath([int]$_.ProcessId) }
        if ($path -and $path.ToLower().StartsWith($folder)) { $_ }
    }
}
if (FromTheCopy) { throw "a program from $Jk is already running: close it first" }
# Steam's JediKnight.exe hands any start to Steam, which runs the REGISTERED install (the user's own) with the same
# switches, never the copy (2026-10-27: a run on steam-vanilla started the user's patched Steam JK).
if (Get-ChildItem -LiteralPath $Jk -Filter '*_install.vdf' -ErrorAction SilentlyContinue) {
    throw "$Jk is a copy of a Steam install: Steam would start the registered install instead. Test Steam JK on the real install, with the user's go."
}

Copy-Item "$Repo\build\Release\gobinator.exe" $Jk -Force
Set-Content "$Jk\gobinator.ini" "# nothing set: Gobinator finds the JK program itself`r`n" -Encoding ascii
$env:GOBINATOR_NO_MESSAGES = '1'
$g = Start-Process "$Jk\gobinator.exe" -ArgumentList "`"$Repo\testinstall\fixtures\P03.GOB`"" `
    -WorkingDirectory "$Repo\testinstall\fixtures" -PassThru
$g.WaitForExit()
"gobinator exit code: $($g.ExitCode)"
Get-Content "$Jk\gobinator.log" -Encoding UTF8 | Select-Object -Skip 4 | ForEach-Object { "  log: $_" }

$p = $null
for ($i = 0; $i -lt 50 -and -not $p; $i++) { Start-Sleep -Milliseconds 200; $p = FromTheCopy | Select-Object -First 1 }
if (-not $p) { 'the game did not start'; exit 1 }
"started: $($p.ExecutablePath)"
"its command line: $($p.CommandLine)"
Start-Sleep -Seconds $Seconds
$running = @(FromTheCopy)
if (-not $running) { "the game exited within $Seconds s"; exit 1 }
foreach ($r in $running) { "running after $Seconds s: $($r.Name) (pid $($r.ProcessId))" }
$proc = Get-Process -Id $running[0].ProcessId
$h = $proc.MainWindowHandle
"its window has the foreground: $([GobinatorDesktop]::GetForegroundWindow() -eq $h)"
$rect = New-Object GobinatorDesktop+RECT
[void][GobinatorDesktop]::GetWindowRect($h, [ref]$rect)
$w = $rect.R - $rect.L; $hgt = $rect.B - $rect.T
if ($w -gt 0 -and $hgt -gt 0) {
    $bmp = New-Object Drawing.Bitmap $w, $hgt
    $gfx = [Drawing.Graphics]::FromImage($bmp)
    $gfx.CopyFromScreen($rect.L, $rect.T, 0, 0, $bmp.Size)
    $bmp.Save($Shot); $gfx.Dispose(); $bmp.Dispose()
    "screenshot: $Shot"
}
$left = @()
foreach ($r in $running) {
    try { Stop-Process -Id $r.ProcessId -Force -ErrorAction Stop; "closed: $($r.Name) (pid $($r.ProcessId))" }
    catch { $left += "$($r.Name) (pid $($r.ProcessId))" }
}
# Never elevate to close it: ask the user.
if ($left) { "NOT closed, it runs with admin rights: $($left -join ', '). Ask the user to close the game." }
exit 0
