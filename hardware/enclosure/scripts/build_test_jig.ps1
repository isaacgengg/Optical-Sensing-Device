# Builds the optics test jig (Enclosure.cs BuildJig): Test Jig\Test Sled.ipt, one optical module per aperture size
# in Test Jig\, and Optics Test Jig.iam (electronics + sled + the 1.6 mm module + a sliding pouch and LFA strip).
# Delete Test Jig\ and Optics Test Jig.iam first to rebuild.
# Usage (Windows PowerShell, Inventor open with this repo's project active, not as administrator):
#   powershell -ExecutionPolicy Bypass -File hardware\enclosure\scripts\build_test_jig.ps1
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot "load_cad_types.ps1")
$inv = [Runtime.InteropServices.Marshal]::GetActiveObject("Inventor.Application")
$root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
[Enclosure]::BuildJig($inv, $root, [double[]]@(1.2, 1.6, 2.0, 2.5))
[PouchSlide]::Run($inv)
[LfaStrip]::AddTo($inv, $root)
