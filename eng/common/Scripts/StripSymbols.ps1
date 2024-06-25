param(
    [parameter(mandatory)][string] $InputDirectory,
    [parameter(mandatory)][string] $OutputDirectory
)

$ErrorActionPreference = "Stop"
function Get-IsAmd64Bit
{
    return $env:PROCESSOR_ARCHITECTURE -eq "AMD64"
}

function Get-IsArm64Bit
{
    return $env:PROCESSOR_ARCHITECTURE -eq "ARM64"
}

function Get-DebuggingToolsRoot
{
    # Constants
    $windowsSDKRegPath = if (Get-IsAmd64Bit -or Get-IsArm64Bit) { "HKLM:\Software\WOW6432Node\Microsoft\Windows Kits\Installed Roots" } else { "HKLM:\Software\Microsoft\Windows Kits\Installed Roots" }
    $windowsDebuggingToolsRegRootKey = "WindowsDebuggersRoot10"

    try
    {
        return Get-ItemProperty -Path $windowsSDKRegPath | Select-Object -ExpandProperty $windowsDebuggingToolsRegRootKey
    }
    catch
    {
        return $null
    }
}

function Remove-PrivateSymbolInformation(
    [parameter(mandatory)][ValidateNotNullOrEmpty()] [string] $inputPdbPath,
    [parameter(mandatory)][ValidateNotNullOrEmpty()] [string] $outputPdbPath)
{
    $debuggingToolsRoot = Get-DebuggingToolsRoot
    if (Get-IsAmd64Bit)
    {
        $pdbCopy = Join-Path $debuggingToolsRoot "x64\pdbcopy.exe"
    }
    elseif (Get-IsArm64Bit)
    {
        $pdbCopy = Join-Path $debuggingToolsRoot "arm64\pdbcopy.exe"
    }
    else
    {
        $pdbCopy = Join-Path $debuggingToolsRoot "x86\pdbcopy.exe"
    }

    $arguments = "$inputPdbPath $outputPdbPath -p"

    Write-Host "$inputPdbPath => $outputPdbPath"
    Start-Process -Wait -NoNewWindow $pdbCopy $arguments
} 

Write-Host -NoNewline "Checking for installed Debugging Tools for Windows..."

$debuggingToolsRoot = Get-DebuggingToolsRoot
if ($debuggingToolsRoot)
{
    Write-Host -ForegroundColor Green "FOUND"
}
else
{
    Write-Host -ForegroundColor Yellow "MISSING"
    exit 1
}

if (Test-Path $OutputDirectory)
{
    Remove-Item -Recurse $OutputDirectory
}
New-Item -ItemType Directory $OutputDirectory | Out-Null

Write-Host "Stripping private information from symbols..."

foreach ($inputPdb in (Get-ChildItem -Recurse -Filter "*.pdb" $InputDirectory))
{
    $inputPdbPath = $inputPdb.FullName
    $outputPdbPath = Join-Path $OutputDirectory $inputPdb.name

    Remove-PrivateSymbolInformation $inputPdbPath $outputPdbPath
}
