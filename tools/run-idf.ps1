[CmdletBinding()]
param (
    [Parameter(Mandatory = $true)]
    [ValidateSet("build", "flash", "monitor", "flash-monitor")]
    [string]$Action,

    [string]$Port = ""
)

$ErrorActionPreference = "Stop"
$env:PYTHONUTF8 = "1"
$env:PYTHONIOENCODING = "utf-8"
[Console]::OutputEncoding = [System.Text.UTF8Encoding]::new($false)

$idfToolsPath = if ($env:IDF_TOOLS_PATH) {
    $env:IDF_TOOLS_PATH
} else {
    "D:\Espressif"
}
$configPath = Join-Path $idfToolsPath "esp_idf.json"

if (-not (Test-Path -LiteralPath $configPath)) {
    throw "ESP-IDF installer configuration not found: $configPath"
}

$config = Get-Content -Raw -Encoding utf8 -LiteralPath $configPath | ConvertFrom-Json
$selected = $config.idfInstalled.PSObject.Properties[$config.idfSelectedId].Value

if (-not $selected) {
    throw "Selected ESP-IDF installation is missing from $configPath"
}

$env:IDF_TOOLS_PATH = $idfToolsPath
. (Join-Path $idfToolsPath "Initialize-Idf.ps1") -IdfId $config.idfSelectedId
$env:IDF_PATH = $selected.path.TrimEnd([char[]]"/\")

[string[]]$idfArgs = switch ($Action) {
    "build" { @("build") }
    "flash" { @("-p", $Port, "flash") }
    "monitor" { @("-p", $Port, "monitor") }
    "flash-monitor" { @("-p", $Port, "flash", "monitor") }
}

if ($Action -ne "build" -and [string]::IsNullOrWhiteSpace($Port)) {
    throw "A serial port is required for action '$Action'"
}

& $selected.python (Join-Path $env:IDF_PATH "tools\idf.py") @idfArgs
exit $LASTEXITCODE
