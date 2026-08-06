[CmdletBinding()]
param (
    [Parameter(Mandatory = $true)]
    [string]$Elf
)

$ErrorActionPreference = "Stop"

if (-not (Test-Path -LiteralPath $Elf)) {
    throw "Firmware image not found: $Elf"
}

$candidates = @()
if (-not [string]::IsNullOrWhiteSpace($env:STM32_PROGRAMMER_CLI)) {
    $candidates += $env:STM32_PROGRAMMER_CLI
}

$command = Get-Command STM32_Programmer_CLI.exe -ErrorAction SilentlyContinue
if ($command) {
    $candidates += $command.Source
}

# Keep the existing local installation as a convenience fallback.
$candidates += "D:\STMCubeProgmer\bin\STM32_Programmer_CLI.exe"

$cli = $candidates |
    Where-Object { Test-Path -LiteralPath $_ } |
    Select-Object -First 1

if (-not $cli) {
    throw "STM32_Programmer_CLI.exe was not found. Set STM32_PROGRAMMER_CLI or add it to PATH."
}

& $cli -c port=SWD -w $Elf -v -rst
exit $LASTEXITCODE
