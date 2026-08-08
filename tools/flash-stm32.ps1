[CmdletBinding()]
param (
    [string]$Elf,
    [string]$BootloaderElf,
    [string]$ApplicationElf
)

$ErrorActionPreference = "Stop"

if ($Elf -and ($BootloaderElf -or $ApplicationElf)) {
    throw "Use either -Elf or the -BootloaderElf/-ApplicationElf pair, not both."
}

if (-not $Elf -and (-not $BootloaderElf -or -not $ApplicationElf)) {
    throw "Specify -Elf, or specify both -BootloaderElf and -ApplicationElf."
}

$images = @()
if ($Elf) {
    $images += $Elf
} else {
    $images += $BootloaderElf
    $images += $ApplicationElf
}

foreach ($image in $images) {
    if (-not (Test-Path -LiteralPath $image)) {
        throw "Firmware image not found: $image"
    }
}

$candidates = @()
if (-not [string]::IsNullOrWhiteSpace($env:STM32_PROGRAMMER_CLI)) {
    $candidates += $env:STM32_PROGRAMMER_CLI
}

$command = Get-Command STM32_Programmer_CLI.exe -ErrorAction SilentlyContinue
if ($command) {
    $candidates += $command.Source
}

$cli = $candidates |
    Where-Object { Test-Path -LiteralPath $_ } |
    Select-Object -First 1

if (-not $cli) {
    throw "STM32_Programmer_CLI.exe was not found. Set STM32_PROGRAMMER_CLI or add it to PATH."
}

for ($index = 0; $index -lt $images.Count; $index++) {
    $arguments = @("-c", "port=SWD", "-w", $images[$index], "-v")
    if ($index -eq ($images.Count - 1)) {
        $arguments += "-rst"
    }

    & $cli @arguments
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }
}

exit 0
