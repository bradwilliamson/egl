param(
  [Parameter(Mandatory=$true)]
  [string]$SourceDir,

  [Parameter(Mandatory=$true)]
  [string]$OutputPkz
)

$ErrorActionPreference = "Stop"

if (-not (Test-Path -LiteralPath $SourceDir)) {
  throw "SourceDir does not exist: $SourceDir"
}

$resolvedSource = (Resolve-Path -LiteralPath $SourceDir).Path
$resolvedOut = $OutputPkz

function Get-ZipPathForOutput([string]$outPath) {
  if ($outPath.ToLower().EndsWith('.pkz')) {
    return $outPath.Substring(0, $outPath.Length - 4) + '.zip'
  }
  return $outPath
}

$zipOut = Get-ZipPathForOutput $resolvedOut

$outDir = Split-Path -Parent $resolvedOut
if ($outDir -and -not (Test-Path -LiteralPath $outDir)) {
  New-Item -ItemType Directory -Path $outDir | Out-Null
}

if (Test-Path -LiteralPath $zipOut) {
  Remove-Item -LiteralPath $zipOut -Force
}

if ($zipOut -ne $resolvedOut -and (Test-Path -LiteralPath $resolvedOut)) {
  Remove-Item -LiteralPath $resolvedOut -Force
}

# Compress-Archive zips the *contents* of the folder when you pass "*".
Compress-Archive -Path (Join-Path $resolvedSource '*') -DestinationPath $zipOut -CompressionLevel Optimal

if ($zipOut -ne $resolvedOut) {
  Move-Item -LiteralPath $zipOut -Destination $resolvedOut -Force
}

Write-Host "Wrote: $resolvedOut"
