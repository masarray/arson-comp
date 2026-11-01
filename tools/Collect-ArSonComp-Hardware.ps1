# ArSonComp M7 read-only Windows audio hardware inventory and manual QA form.
# Does not modify devices, measure audio callback timing, or declare xrun success.
[CmdletBinding()]
param([string]$ReportDirectory = (Join-Path $PSScriptRoot "Reports"))
$ErrorActionPreference = "Stop"
New-Item -ItemType Directory -Force -Path $ReportDirectory | Out-Null
$stamp = (Get-Date).ToUniversalTime().ToString("yyyyMMdd-HHmmss")
$prefix = Join-Path $ReportDirectory ("ArSonComp-" + $stamp)
$osInfo = $null
$audioInfo = @()
try {
    $osInfo = Get-CimInstance Win32_OperatingSystem |
        Select-Object Caption, Version, BuildNumber, OSArchitecture
} catch { Write-Warning ("OS query: " + $_.Exception.Message) }
try {
    $audioInfo = @(Get-CimInstance Win32_SoundDevice |
        Select-Object Name, Manufacturer, DriverVersion, Status)
} catch { Write-Warning ("Audio devices: " + $_.Exception.Message) }
$inventory = [ordered]@{
    generated_utc = (Get-Date).ToUniversalTime().ToString("o")
    purpose = "Device metadata only. Not a latency or xrun measurement."
    windows = $osInfo
    audio_devices = $audioInfo
}
$inventory | ConvertTo-Json -Depth 6 |
    Set-Content -Path ($prefix + "-devices.json") -Encoding UTF8
$report = @"
# ArSonComp M7 physical test — manual record
Date: $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss zzz')
Build SHA (BUILD-INFO.txt):
Host and version / Standalone or VST3:
Windows / CPU:
Audio interface (Maono PS22 if used):
Driver and version (ASIO/WASAPI):
Sample rate (start 48000 Hz):
Audio input/output channels:
Buffer tested (128 / 64 / 32 samples):
Mode / preset / threshold / makeup / ratio:
Host plugin reported latency:
Actual hardware round-trip latency, if measured:
Actual host xrun count and callback spikes:
A Device direct monitoring crackles?:
B Host without plugin crackles?:
C Host-bypassed plugin crackles?:
D Plugin active crackles?:
Audio recordings for A/B/C/D:
1-instance xrun count:
8-instance xrun count:
60-minute session xrun count:
Editor open/close while playing:
Private bytes before/after:
Host project saved/restored and automation tested?:
Mono / stereo / external sidechain tested?:
GUI minimum/default/MINI and control visibility:
Result: PASS / FAIL / NOT TESTED
Reproduction notes:
"@
$report | Set-Content -Path ($prefix + "-manual-checklist.md") -Encoding UTF8
Write-Host "Saved device inventory:" ($prefix + "-devices.json")
Write-Host "Manual QA form:" ($prefix + "-manual-checklist.md")
Write-Host "No xrun or hardware latency measurement was performed automatically."
