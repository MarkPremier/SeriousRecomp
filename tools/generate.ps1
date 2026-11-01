param(
    [string]$Xbe = 'P:\Recomp\SeriousSam\Serious Sam (USA)\default.xbe',
    [string]$Disassembler = 'P:\Recomp\xboxrecomp',
    [string]$Toolkit = 'P:\Recomp\xboxrecomp_ss'
)
$ErrorActionPreference = 'Stop'
$projectDir = Split-Path $PSScriptRoot -Parent
$python = Join-Path $projectDir '.venv\Scripts\python.exe'
$analysis = Join-Path $projectDir 'analysis'
$disasm = Join-Path $projectDir 'disasm_out'
$gen = Join-Path $projectDir 'src\recomp\gen'
New-Item -ItemType Directory -Force $analysis | Out-Null
if (Test-Path $gen) {
    Copy-Item $gen (Join-Path $analysis ('gen_backup_' + (Get-Date -Format 'yyyyMMdd_HHmmss'))) -Recurse
}
function Invoke-PipelineStep([string[]]$Arguments) {
    & $python @Arguments
    if ($LASTEXITCODE -ne 0) { throw "Pipeline failed: $($Arguments -join ' ')" }
}
$previousBytecode = $env:PYTHONDONTWRITEBYTECODE
$env:PYTHONDONTWRITEBYTECODE = '1'
try {
    Push-Location $Disassembler
    try {
        Invoke-PipelineStep @('-m', 'tools.xbe_parser', $Xbe, '--json', "$analysis\xbe.json")
        $disasmArgs = @('-u', '-m', 'tools.disasm', $Xbe, '--analysis-json', "$analysis\xbe.json",
            '--force', '-v', '-o', $disasm, '--seed-functions', "$projectDir\seeds\manual_seeds.json")
        if (Test-Path "$projectDir\seeds\runtime_feedback.json") {
            $disasmArgs += @('--seed-functions', "$projectDir\seeds\runtime_feedback.json")
        }
        Invoke-PipelineStep $disasmArgs
    } finally { Pop-Location }
    Push-Location $Toolkit
    try {
        Invoke-PipelineStep @('-m', 'tools.func_id', $Xbe,
            '--functions', "$disasm\functions.json", '--strings', "$disasm\strings.json",
            '--xrefs', "$disasm\xrefs.json", '--output', "$analysis\func_id")
        Invoke-PipelineStep @('-m', 'tools.abi_analysis', $Xbe, '--disasm-dir', $disasm,
            '--func-id-dir', "$analysis\func_id", '--output-dir', "$analysis\abi")
        Invoke-PipelineStep @('-u', '-m', 'tools.recomp', $Xbe, '--all', '--split', '250',
            '--game-name', 'Serious Sam', '--disasm-dir', $disasm,
            '--func-id-dir', "$analysis\func_id", '--abi-dir', "$analysis\abi",
            '--output-dir', "$analysis\recomp", '--gen-dir', $gen,
            '--exclude-manual', "$projectDir\src\recomp_manual.c")
    } finally { Pop-Location }
} finally { $env:PYTHONDONTWRITEBYTECODE = $previousBytecode }
