param(
    [ValidateRange(1, 300)][int]$Seconds = 15,
    [string]$Label = 'latest',
    [switch]$Visible,
    [switch]$DspPassthrough
)
$ErrorActionPreference = 'Stop'
$projectDir = Split-Path $PSScriptRoot -Parent
if ($Label -notmatch '^[a-zA-Z0-9_-]+$') { throw 'Use a simple label without path separators.' }
$gameDir = Join-Path $projectDir 'test_game'
if (-not (Test-Path (Join-Path $gameDir 'Main.gro'))) {
    throw 'Copy the extracted disc into test_game before running a boot probe.'
}
$logDir = Join-Path $projectDir "analysis\boot_$Label"
New-Item -ItemType Directory -Force $logDir | Out-Null
$settings = @{
    RECOMP_GAME_DIR = $gameDir
    RECOMP_SAVE_DIR = (Join-Path $gameDir 'saves')
    RECOMP_WATCHDOG_SECS = "$Seconds"
    RECOMP_FB_WINDOW = $(if ($Visible) { '1' } else { '0' })
    RECOMP_FPS_LOG = '5'
    RECOMP_STREAM_PREFETCH = '1'
    RECOMP_OPTIONAL_DEFAULTS = '1'
    RECOMP_EXCEPTION_TRACE = '1'
}
if ($DspPassthrough) {
    $settings['RECOMP_APU_DSP'] = '0'
    $settings['RECOMP_DSP_ACK'] = '0x803A4810'
}
$previous = @{}
try {
    foreach ($name in $settings.Keys) {
        $previous[$name] = [Environment]::GetEnvironmentVariable($name, 'Process')
        [Environment]::SetEnvironmentVariable($name, $settings[$name], 'Process')
    }
    $probe = Start-Process -FilePath (Join-Path $projectDir 'build\Release\serious_sam_recomp.exe') `
        -WorkingDirectory $projectDir -WindowStyle Hidden `
        -RedirectStandardOutput (Join-Path $logDir 'stdout.txt') `
        -RedirectStandardError (Join-Path $logDir 'stderr.txt') -PassThru
    if (-not $probe.WaitForExit(($Seconds + 10) * 1000)) {
        Stop-Process -Id $probe.Id
        throw "Boot probe exceeded timeout; logs: $logDir"
    }
    Write-Output "Boot exit: $($probe.ExitCode); logs: $logDir"
} finally {
    foreach ($name in $previous.Keys) {
        [Environment]::SetEnvironmentVariable($name, $previous[$name], 'Process')
    }
}
