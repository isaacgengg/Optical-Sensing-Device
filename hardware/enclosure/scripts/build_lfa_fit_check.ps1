# Builds Test Strips\LFA Strip (bare).ipt (if missing) and LFA Fit Check.iam: the electronics with an LFA strip
# that slides under the two sensors (LfaStrip.cs). Delete LFA Fit Check.iam first to rebuild it.
# Usage (Windows PowerShell, Inventor open with this repo's project active, not as administrator):
#   powershell -ExecutionPolicy Bypass -File hardware\enclosure\scripts\build_lfa_fit_check.ps1
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot "load_cad_types.ps1")
$inv = [Runtime.InteropServices.Marshal]::GetActiveObject("Inventor.Application")
[LfaStrip]::Run($inv, (Resolve-Path (Join-Path $PSScriptRoot "..")).Path)
