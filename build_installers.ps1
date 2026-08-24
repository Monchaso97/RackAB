# ============================================================
#  RackAB - build both installers and produce the two ZIPs.
#
#  Output (inside the plugin folder):
#    RackAB-Windows-Installer.zip  -> finished Windows setup .exe
#    RackAB-macOS-Installer.zip    -> macOS installer kit (build on a Mac)
#
#  Run after the plugin is built (build\RackAB_artefacts\...\RackAB.vst3).
# ============================================================
$ErrorActionPreference = 'Stop'
$root      = $PSScriptRoot
$winDir    = Join-Path $root 'installer\windows'
$macDir    = Join-Path $root 'installer\mac'
$vst3      = Join-Path $root 'build\RackAB_artefacts\Release\VST3\RackAB.vst3'
$iscc      = "$env:LOCALAPPDATA\Programs\Inno Setup 6\ISCC.exe"

if (-not (Test-Path $vst3)) { throw "VST3 not built yet: $vst3  (build the plugin first)" }
if (-not (Test-Path $iscc)) { throw "Inno Setup not found at $iscc" }

# 1. Regenerate the side image, then compile the Windows installer.
Write-Host '==> Generating wizard image'
& (Join-Path $root 'installer\make_welcome_image.ps1') | Out-Null

Write-Host '==> Compiling Windows installer (Inno Setup)'
& $iscc (Join-Path $winDir 'RackAB.iss') | Select-Object -Last 3
$setupExe = Get-ChildItem (Join-Path $winDir 'output') -Filter '*.exe' |
            Sort-Object LastWriteTime -Descending | Select-Object -First 1
if (-not $setupExe) { throw 'Windows setup .exe was not produced' }

# 2. Zip the Windows installer.
$winZip = Join-Path $root 'RackAB-Windows-Installer.zip'
if (Test-Path $winZip) { Remove-Item $winZip -Force }
Compress-Archive -Path $setupExe.FullName -DestinationPath $winZip -CompressionLevel Optimal
Write-Host "==> $winZip"

# 3. Zip the macOS installer kit (scripts + resources + README).
#    Built manually with forward-slash paths so it extracts correctly on macOS
#    (Compress-Archive writes backslashes, which break folders on mac/Linux).
$macZip = Join-Path $root 'RackAB-macOS-Installer.zip'
if (Test-Path $macZip) { Remove-Item $macZip -Force }
Add-Type -AssemblyName System.IO.Compression.FileSystem
$zip = [System.IO.Compression.ZipFile]::Open($macZip, 'Create')
try {
    Get-ChildItem $macDir -Recurse -File |
        Where-Object { $_.FullName -notmatch '\\(output|stage)\\' } |
        ForEach-Object {
            $rel = $_.FullName.Substring($macDir.Length + 1).Replace('\','/')
            $entry = [System.IO.Compression.ZipFileExtensions]::CreateEntryFromFile(
                        $zip, $_.FullName, $rel, 'Optimal')
            if ($rel -eq 'build_dmg.sh') {
                # mark executable (rwxr-xr-x) in the zip's unix permission field
                $entry.ExternalAttributes = 0x81ED -shl 16
            }
        }
} finally { $zip.Dispose() }
Write-Host "==> $macZip"

Write-Host ''
Write-Host 'Done. Two installers created in the plugin folder:'
Write-Host "  $winZip"
Write-Host "  $macZip"
